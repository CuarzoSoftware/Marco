#include <CZ/Marco/Private/MFreezeWatchdog.h>
#include <CZ/Marco/MLog.h>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <format>
#include <string>
#include <thread>
#include <csignal>
#include <execinfo.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

using namespace CZ;

namespace
{
    std::atomic<bool> s_enabled { false };
    std::atomic<Int64> s_busySinceUs { 0 }; // 0: idle
    std::atomic<int> s_dumpFd { -1 };
    std::atomic<bool> s_sampled { false };
    pthread_t s_main {};
    Int64 s_thresholdUs {};

    Int64 NowUs() noexcept
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    void OnSample(int) noexcept
    {
        const int saved { errno };
        const int fd { s_dumpFd.load() };

        if (fd >= 0)
        {
            void *frames[64];
            const int n { backtrace(frames, 64) };
            backtrace_symbols_fd(frames, n, fd);
        }

        s_sampled.store(true);
        errno = saved;
    }

    void Write(int fd, const std::string &text) noexcept
    {
        [[maybe_unused]] auto r { write(fd, text.data(), text.size()) };
    }

    void CopyMaps(int fd) noexcept
    {
        Write(fd, "\n--- /proc/self/maps ---\n");
        const int maps { open("/proc/self/maps", O_RDONLY | O_CLOEXEC) };

        if (maps < 0)
            return;

        char buf[4096];
        ssize_t n;

        while ((n = read(maps, buf, sizeof(buf))) > 0)
            [[maybe_unused]] auto r { write(fd, buf, size_t(n)) };

        close(maps);
    }

    void Loop() noexcept
    {
        char name[32] {};
        pthread_getname_np(s_main, name, sizeof(name));
        int count { 0 };

        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            const Int64 since { s_busySinceUs.load() };

            if (since == 0 || NowUs() - since < s_thresholdUs)
                continue;

            // Stuck: sample until this iteration ends (at most 300 samples)
            const auto path { std::format("/tmp/cz-marco-freeze-{}-{}-{}.txt", name, getpid(), ++count) };
            const int fd { open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, 0644) };

            if (fd < 0)
                return;

            s_dumpFd.store(fd);

            for (int i = 0; i < 300 && s_busySinceUs.load() == since; i++)
            {
                Write(fd, std::format("\n--- sample {} at +{} ms ---\n", i, (NowUs() - since) / 1000));
                s_sampled.store(false);
                pthread_kill(s_main, SIGPROF);

                // Until written (the handler runs once the thread gets the signal)
                for (int w = 0; w < 100 && !s_sampled.load(); w++)
                    std::this_thread::sleep_for(std::chrono::microseconds(200));

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            while (s_busySinceUs.load() == since)
                std::this_thread::sleep_for(std::chrono::milliseconds(5));

            const Int64 totalMs { (NowUs() - since) / 1000 };
            s_dumpFd.store(-1);
            Write(fd, std::format("\n--- main loop iteration took ~{} ms ---\n", totalMs));
            CopyMaps(fd);
            close(fd);
            MLog(CZWarning, CZLN, "[watchdog] Main loop stuck for ~{} ms, stacks: {}", totalMs, path);
        }
    }
}

void MFreezeWatchdog::Init() noexcept
{
    const char *env { getenv("CZ_MARCO_FREEZE_WATCHDOG_MS") };

    if (!env || atoll(env) <= 0 || s_enabled.exchange(true))
        return;

    s_thresholdUs = atoll(env) * 1000;
    s_main = pthread_self();

    // backtrace() loads libgcc the first time: never inside the signal handler
    void *frames[4];
    backtrace(frames, 4);

    struct sigaction sa {};
    sa.sa_handler = OnSample;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPROF, &sa, nullptr);

    MLog(CZWarning, CZLN, "[watchdog] Freeze watchdog enabled (threshold {} ms)", s_thresholdUs / 1000);
    std::thread(Loop).detach();
}

void MFreezeWatchdog::Busy() noexcept
{
    if (s_enabled.load(std::memory_order_relaxed))
        s_busySinceUs.store(NowUs());
}

void MFreezeWatchdog::Idle() noexcept
{
    if (s_enabled.load(std::memory_order_relaxed))
        s_busySinceUs.store(0);
}

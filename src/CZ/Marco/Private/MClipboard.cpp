#include <CZ/Marco/Private/MClipboard.h>
#include <CZ/Marco/Protocols/Wayland/MWlDataDevice.h>
#include <CZ/Marco/MDNDOffer.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/MLog.h>
#include <CZ/Core/CZTimer.h>
#include <CZ/Core/Events/CZInputEvent.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <algorithm>

using namespace CZ;

static constexpr const char *TextMimeTypes[] { "text/plain;charset=utf-8", "UTF8_STRING", "text/plain", "TEXT", "STRING" };
static constexpr size_t MaxPasteBytes { 16 * 1024 * 1024 };

const wl_data_source_listener MClipboard::SourceListener
{
    .target = source_target,
    .send = source_send,
    .cancelled = source_cancelled,
    .dnd_drop_performed = source_dnd_drop_performed,
    .dnd_finished = source_dnd_finished,
    .action = source_action
};

MClipboard::~MClipboard() noexcept
{
    clearSource();
}

void MClipboard::clearSource() noexcept
{
    if (m_source)
    {
        wl_data_source_destroy(m_source);
        m_source = nullptr;
    }

    m_text.clear();
}

void MClipboard::setText(const std::string &text, const CZInputEvent &triggeringEvent) noexcept
{
    auto app { MApp::Get() };

    if (!app || !app->wl.dataDeviceManager || !app->wl.dataDevice)
        return;

    clearSource();
    m_text = text;
    m_source = wl_data_device_manager_create_data_source(app->wl.dataDeviceManager);
    wl_data_source_add_listener(m_source, &SourceListener, this);

    for (const char *mimeType : TextMimeTypes)
        wl_data_source_offer(m_source, mimeType);

    wl_data_device_set_selection(app->wl.dataDevice, m_source, triggeringEvent.serial);
    wl_display_flush(app->wl.display);
}

void MClipboard::source_send(void *data, wl_data_source */*source*/, const char */*mimeType*/, Int32 fd)
{
    auto &self { *static_cast<MClipboard*>(data) };

    // Blocking (short) writes: the reader is another process
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK);
    size_t written { 0 };

    while (written < self.m_text.size())
    {
        const ssize_t n { write(fd, self.m_text.data() + written, self.m_text.size() - written) };

        if (n < 0 && errno == EINTR)
            continue;

        if (n <= 0)
            break;

        written += size_t(n);
    }

    close(fd);
}

void MClipboard::source_cancelled(void *data, wl_data_source *source)
{
    // Someone else owns the selection now
    auto &self { *static_cast<MClipboard*>(data) };

    if (self.m_source == source)
        self.clearSource();
    else
        wl_data_source_destroy(source);
}

void MClipboard::requestText(std::function<void(std::optional<std::string>)> done) noexcept
{
    // Ours (also avoids reading from our own pipe), or nothing to read: answered from the loop
    const auto answerLater = [&done](std::optional<std::string> text) {
        CZTimer::OneShot(0, [done = std::move(done), text = std::move(text)](CZTimer*) { done(text); });
    };

    if (m_source)
    {
        answerLater(m_text);
        return;
    }

    MDNDOffer *offer { MWlDataDevice::SelectionOffer() };

    if (!offer)
    {
        answerLater(std::nullopt);
        return;
    }

    const char *mimeType { nullptr };

    for (const char *candidate : TextMimeTypes)
        if (offer->hasMimeType(candidate)) { mimeType = candidate; break; }

    const Int32 fd { mimeType ? offer->receive(mimeType) : -1 };

    if (fd < 0)
    {
        answerLater(std::nullopt);
        return;
    }

    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    auto read { std::make_unique<Read>() };
    read->done = std::move(done);
    Read *raw { read.get() };

    read->source = CZEventSource::Make(fd, EPOLLIN, CZOwn::Own, [this, raw](int fd, UInt32, CZEventSource*) {
        if (raw->finished)
            return;

        char buf[4096];

        while (true)
        {
            const ssize_t n { ::read(fd, buf, sizeof(buf)) };

            if (n > 0)
            {
                raw->data.append(buf, size_t(n));

                if (raw->data.size() > MaxPasteBytes)
                {
                    finishRead(raw, false);
                    return;
                }
                continue;
            }

            if (n < 0 && errno == EINTR)
                continue;

            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                return; // More later

            finishRead(raw, n == 0); // EOF (or an error)
            return;
        }
    });

    m_reads.emplace_back(std::move(read));
}

void MClipboard::finishRead(Read *read, bool ok) noexcept
{
    // Destroyed out of its event source's callback
    read->finished = true;

    CZTimer::OneShot(0, [this, read, ok](CZTimer*) {
        auto it { std::find_if(m_reads.begin(), m_reads.end(), [read](const auto &r){ return r.get() == read; }) };

        if (it == m_reads.end())
            return;

        auto owned { std::move(*it) };
        m_reads.erase(it);

        if (owned->done)
            owned->done(ok ? std::optional(std::move(owned->data)) : std::nullopt);
    });
}

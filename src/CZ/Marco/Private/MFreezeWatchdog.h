#ifndef MFREEZEWATCHDOG_H
#define MFREEZEWATCHDOG_H

namespace CZ
{
    /**
     * @brief Debug tool: finds where the app's main thread gets stuck (UI freezes).
     *
     * Disabled unless CZ_MARCO_FREEZE_WATCHDOG_MS is set to a threshold in ms (> 0). Once an event
     * loop iteration (MApp::dispatch()) takes longer, the main thread's stack is sampled every 10 ms
     * until it's done (a backtrace written by the thread itself from a SIGPROF handler: catches even
     * short freezes, unlike attaching gdb). Samples, the total duration and the memory maps (to
     * resolve addresses with addr2line) go to /tmp/cz-marco-freeze-<name>-<pid>-<n>.txt.
     */
    namespace MFreezeWatchdog
    {
        /// Starts the watchdog thread if enabled (from the main thread).
        void Init() noexcept;

        /// The main thread starts handling events / goes idle.
        void Busy() noexcept;
        void Idle() noexcept;
    }
}

#endif // MFREEZEWATCHDOG_H

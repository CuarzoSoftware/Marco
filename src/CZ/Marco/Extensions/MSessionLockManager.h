#ifndef MSESSIONLOCKMANAGER_H
#define MSESSIONLOCKMANAGER_H

#include <CZ/AK/AKObject.h>
#include <CZ/Marco/Marco.h>
#include <CZ/Core/CZSignal.h>
#include <unordered_set>

struct ext_session_lock_manager_v1;
struct ext_session_lock_v1;

/**
 * @brief Locks the session (ext-session-lock-v1).
 *
 * Get it with MApp::sessionLockManager(): created (bound) on first use, nullptr if the compositor
 * doesn't support the protocol (or doesn't expose it to this client, e.g. it's not privileged).
 *
 * While the session is locked the compositor shows nothing but the lock surfaces of this client
 * (MSessionLockSurface), one per screen, until it's unlocked:
 *
 * 1. lock() asks the compositor to lock it (state() Locking). Create the lock surfaces right away
 *    (or before), mapped and with their screen set: they're shown while it's Locking or Locked
 *    (the compositor may wait for them before locking, to avoid a blank frame).
 * 2. The compositor either locks it (state() Locked, onStateChanged) or refuses (onFinished).
 * 3. Once the user is authenticated, unlock() unlocks it (state() Unlocked).
 *
 * The compositor may also end the lock on its own (onFinished), e.g. another client locked it.
 * Lock surfaces are unmapped whenever the lock ends and mapped again with the next one, so they can
 * be kept around.
 *
 * If the client exits (or this object is destroyed) while Locked, the session stays locked: that's
 * on purpose (the compositor decides what happens then). To exit right after unlock(), make sure
 * the compositor received it first (e.g. with a wl_display_roundtrip()).
 */
class CZ::MSessionLockManager : public AKObject
{
public:
    enum State
    {
        Unlocked, ///< Not locked (or the lock ended)
        Locking,  ///< Requested, waiting for the compositor
        Locked    ///< Locked: this client decides when it's unlocked
    };

    State state() const noexcept { return m_state; }

    /**
     * @brief Asks the compositor to lock the session.
     *
     * @return `false` if already Locking or Locked.
     */
    bool lock() noexcept;

    /**
     * @brief Unlocks the session (if Locked) or cancels the request (if Locking).
     *
     * The lock surfaces are unmapped.
     *
     * @return `false` if Unlocked.
     */
    bool unlock() noexcept;

    /// The lock surfaces created (mapped or not).
    const std::unordered_set<MSessionLockSurface*> &surfaces() const noexcept { return m_surfaces; }

    /// The state changed (see state()).
    CZSignal<State> onStateChanged;

    /// The compositor refused to lock the session, or ended the lock (state() Unlocked).
    CZSignal<> onFinished;

    ~MSessionLockManager() noexcept;

private:
    friend class MApp;
    friend class MSessionLockSurface;
    MSessionLockManager(ext_session_lock_manager_v1 *manager) noexcept;
    void setState(State state) noexcept;

    /// Unmaps the lock surfaces (the lock ended).
    void resetSurfaces() noexcept;

    static void locked(void *data, ext_session_lock_v1 *lock);
    static void finished(void *data, ext_session_lock_v1 *lock);

    ext_session_lock_manager_v1 *m_manager { nullptr };
    ext_session_lock_v1 *m_lock { nullptr };
    State m_state { Unlocked };
    std::unordered_set<MSessionLockSurface*> m_surfaces;
};

#endif // MSESSIONLOCKMANAGER_H

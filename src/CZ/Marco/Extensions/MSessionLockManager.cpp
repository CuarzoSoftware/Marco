#include <CZ/Marco/Extensions/MSessionLockManager.h>
#include <CZ/Marco/Private/MSessionLockSurfacePrivate.h>
#include <CZ/Marco/Protocols/SessionLock/ext-session-lock-v1-client.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/MLog.h>

using namespace CZ;

MSessionLockManager::MSessionLockManager(ext_session_lock_manager_v1 *manager) noexcept :
    m_manager(manager) {}

MSessionLockManager::~MSessionLockManager() noexcept
{
    for (MSessionLockSurface *surface : m_surfaces)
        surface->imp()->manager.reset();

    if (m_lock)
    {
        // Locked: kept (destroying it now is a protocol error, and unlocking isn't up to this)
        if (m_state == Locking)
            ext_session_lock_v1_destroy(m_lock);

        m_lock = nullptr;
    }

    if (m_manager)
        ext_session_lock_manager_v1_destroy(m_manager);
}

bool MSessionLockManager::lock() noexcept
{
    if (m_lock)
        return false;

    static const ext_session_lock_v1_listener listener
    {
        .locked = &MSessionLockManager::locked,
        .finished = &MSessionLockManager::finished
    };

    m_lock = ext_session_lock_manager_v1_lock(m_manager);
    ext_session_lock_v1_add_listener(m_lock, &listener, this);
    setState(Locking);

    // Their lock surfaces right away (the compositor may wait for them to lock)
    for (MSessionLockSurface *surface : m_surfaces)
        surface->update(true);

    return true;
}

bool MSessionLockManager::unlock() noexcept
{
    if (!m_lock)
        return false;

    if (m_state == Locked)
        ext_session_lock_v1_unlock_and_destroy(m_lock);
    else
        ext_session_lock_v1_destroy(m_lock);

    m_lock = nullptr;

    // After the request: destroying them while locked would show a solid color meanwhile
    resetSurfaces();
    wl_display_flush(MApp::Get()->wl.display);
    setState(Unlocked);
    return true;
}

void MSessionLockManager::setState(State state) noexcept
{
    if (m_state == state)
        return;

    m_state = state;
    onStateChanged.notify(state);
}

void MSessionLockManager::resetSurfaces() noexcept
{
    for (MSessionLockSurface *surface : m_surfaces)
        surface->imp()->reset();
}

void MSessionLockManager::locked(void *data, ext_session_lock_v1 *)
{
    auto &manager { *static_cast<MSessionLockManager*>(data) };
    manager.setState(Locked);
}

void MSessionLockManager::finished(void *data, ext_session_lock_v1 *)
{
    auto &manager { *static_cast<MSessionLockManager*>(data) };

    if (!manager.m_lock)
        return;

    // Depending on whether it was locked (see the protocol)
    if (manager.m_state == Locked)
        ext_session_lock_v1_unlock_and_destroy(manager.m_lock);
    else
        ext_session_lock_v1_destroy(manager.m_lock);

    manager.m_lock = nullptr;
    manager.resetSurfaces();

    CZWeak<MSessionLockManager> ref { &manager };
    manager.setState(Unlocked);

    if (ref)
        manager.onFinished.notify();
}

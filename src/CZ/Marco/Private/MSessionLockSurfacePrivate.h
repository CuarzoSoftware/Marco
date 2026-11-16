#ifndef MSESSIONLOCKSURFACEPRIVATE_H
#define MSESSIONLOCKSURFACEPRIVATE_H

#include <CZ/Marco/Roles/MSessionLockSurface.h>
#include <CZ/Marco/Extensions/MSessionLockManager.h>
#include <CZ/Marco/Protocols/SessionLock/ext-session-lock-v1-client.h>
#include <CZ/Marco/MScreen.h>
#include <CZ/Core/CZWeak.h>

class CZ::MSessionLockSurface::Imp
{
public:
    Imp(MSessionLockSurface &obj) noexcept;
    MSessionLockSurface &obj;

    CZWeak<MSessionLockManager> manager;
    CZWeak<MScreen> screen;     // Set by the user
    CZWeak<MScreen> roleScreen; // The role's

    SkISize suggestedSize { 0, 0 };
    UInt32 configureSerial { 0 };

    ext_session_lock_surface_v1 *lockSurface { nullptr };
    static inline ext_session_lock_surface_v1_listener lockSurfaceListener;
    static void configure(void *data, ext_session_lock_surface_v1 *lockSurface, UInt32 serial, UInt32 width, UInt32 height);

    /// Whether it should be shown (mapped by the user, with a screen, while there's a lock).
    bool wanted() const noexcept;

    /// Creates the role (the wl_surface has no buffer). `false` if another one is on its screen.
    bool createRole() noexcept;

    /// Unmaps it, destroying the role (the props are kept).
    void reset() noexcept;
};

#endif // MSESSIONLOCKSURFACEPRIVATE_H

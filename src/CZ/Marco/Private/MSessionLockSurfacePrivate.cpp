#include <CZ/Marco/Private/MSessionLockSurfacePrivate.h>
#include <CZ/Marco/Private/MSurfacePrivate.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/MLog.h>

using namespace CZ;

MSessionLockSurface::Imp::Imp(MSessionLockSurface &obj) noexcept : obj(obj)
{
    lockSurfaceListener.configure = &configure;
}

bool MSessionLockSurface::Imp::wanted() const noexcept
{
    return obj.MSurface::imp()->flags.has(MSurface::Imp::UserMapped) &&
           manager && manager->m_lock && screen;
}

bool MSessionLockSurface::Imp::createRole() noexcept
{
    assert(!lockSurface);

    // One per screen (duplicate_output protocol error)
    for (MSessionLockSurface *other : manager->m_surfaces)
    {
        if (other != &obj && other->imp()->lockSurface && other->imp()->roleScreen == screen)
        {
            MLog(CZError, CZLN, "Screen {} already has a session lock surface", screen->props().name);
            return false;
        }
    }

    // The wl_surface has no buffer: new, or emptied by reset()
    lockSurface = ext_session_lock_v1_get_lock_surface(manager->m_lock, obj.wlSurface(), screen->wlOutput());
    ext_session_lock_surface_v1_add_listener(lockSurface, &lockSurfaceListener, &obj);
    roleScreen = screen;

    // Nothing is committed until the first configure is acked (protocol error)
    using SF = MSurface::Imp::Flags;
    obj.MSurface::imp()->flags.add(SF::PendingFirstConfigure);
    obj.MSurface::imp()->flags.remove(SF::PendingConfigureAck);
    return true;
}

void MSessionLockSurface::Imp::reset() noexcept
{
    obj.MSurface::imp()->setMapped(false);

    if (!lockSurface)
        return;

    ext_session_lock_surface_v1_destroy(lockSurface);
    lockSurface = nullptr;
    roleScreen.reset();
    configureSerial = 0;

    // Without the role, so the next one gets a wl_surface without a buffer (already_constructed
    // protocol error otherwise). Never with the role: committing a null buffer is an error then.
    wl_surface_attach(obj.wlSurface(), nullptr, 0, 0);
    wl_surface_commit(obj.wlSurface());

    using SF = MSurface::Imp::Flags;
    obj.MSurface::imp()->flags.remove(SF::PendingConfigureAck | SF::PendingFirstConfigure | SF::HasBufferAttached);

    // Another one waiting for this screen may take it now
    if (manager)
        for (MSessionLockSurface *other : manager->m_surfaces)
            if (other != &obj && !other->imp()->lockSurface)
                other->update();
}

void MSessionLockSurface::Imp::configure(void *data, ext_session_lock_surface_v1 *, UInt32 serial, UInt32 width, UInt32 height)
{
    auto &role { *static_cast<MSessionLockSurface*>(data) };
    auto &flags { role.MSurface::imp()->flags };
    using SF = MSurface::Imp::Flags;

    flags.add(SF::PendingConfigureAck);
    role.imp()->configureSerial = serial;

    const bool first { flags.has(SF::PendingFirstConfigure) };
    flags.remove(SF::PendingFirstConfigure);

    const SkISize size (width, height);

    if (first || role.imp()->suggestedSize != size)
    {
        role.imp()->suggestedSize = size;
        role.suggestedSizeChanged();
    }

    if (!flags.has(SF::Mapped))
        role.MSurface::imp()->setMapped(true);

    // The size must match in the next commit after the ack
    role.update(true);
}

#include <CZ/Marco/Protocols/CursorShape/cursor-shape-v1-client.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/Private/MSurfacePrivate.h>
#include <CZ/Marco/Private/MPopupPrivate.h>
#include <CZ/Marco/Private/MToplevelPrivate.h>
#include <CZ/Marco/Private/MLayerSurfacePrivate.h>
#include <CZ/Marco/Roles/MSubsurface.h>
#include <CZ/Marco/MTheme.h>

#include <CZ/Marco/Protocols/Wayland/MWlRegistry.h>
#include <CZ/Marco/MLog.h>

#include <CZ/Core/CZCore.h>
#include <CZ/Ream/RCore.h>
#include <CZ/Ream/WL/RWLPlatformHandle.h>
#include <CZ/AK/AKApp.h>

#include <CZ/AK/Input/AKKeyboard.h>

#include <sys/mman.h>
#include <assert.h>
#include <sys/poll.h>

using namespace CZ;

std::weak_ptr<MApp> m_app;

MApp::MApp() noexcept {}

std::shared_ptr<MApp> MApp::GetOrMake() noexcept
{
    if (auto app = m_app.lock())
        return app;

    auto app { std::shared_ptr<MApp>(new MApp()) };
    m_app = app;

    if (!app->init())
    {
        MLog(CZFatal, CZLN, "Failed to create MApp");
        return {};
    }

    return app;
}

std::shared_ptr<MApp> MApp::Get() noexcept
{
    return m_app.lock();
}

int MApp::run() noexcept
{
    while (running())
        dispatch(-1);
    return 0;
}

int MApp::dispatch(int timeoutMs) noexcept
{
    if (!m_running)
        return 0;

    pollfd pollfd {
        .fd = m_core->fd(),
        .events = POLLIN,
        .revents = 0
    };

    const int ret { poll(&pollfd, 1, timeoutMs) };

    if (ret == 1)
    {
        m_core->dispatch(0);
        updateSurfaces();
    }

    return ret;
}

void MApp::update() noexcept
{
    m_core->unlockLoop();
}

bool MApp::init() noexcept
{
    m_core = CZCore::Get();

    if (!m_core)
    {
        MLog(CZFatal, CZLN, "Failed to create CZCore");
        return false;
    }

    wl.display = wl_display_connect(NULL);

    if (!wl.display)
    {
        MLog(CZFatal, CZLN, "wl_display_connect failed");
        return false;
    }

    RCore::Options options {};
    options.platformHandle = RWLPlatformHandle::Make(wl.display, CZOwn::Own);
    m_ream = RCore::Make(options);

    if (!m_ream)
    {
        MLog(CZFatal, CZLN, "Failed to create RCore");
        return false;
    }

    m_kay = AKApp::GetOrMake();

    if (!m_kay)
    {
        MLog(CZFatal, CZLN, "Failed to create AKApp");
        return false;
    }

    CZ::setTheme(new MTheme());

    m_source = CZEventSource::Make(wl_display_get_fd(wl.display), POLLIN, CZOwn::Borrow, [this](auto, auto) {
        wl_display_dispatch(wl.display);
        updateSurfaces();
    });

    wl.registry.set(wl_display_get_registry(wl.display));
    wl_registry_add_listener(wl.registry, &MWlRegistry::Listener, &wl);
    usleep(10000);
    wl_display_roundtrip(wl.display);
    wl_display_roundtrip(wl.display);
    wl_display_roundtrip(wl.display);

    if (!wl.shm)
    {
        MLog(CZFatal, CZLN, "wl_shm not supported by the compositor");
        return false;
    }

    if (!wl.compositor)
    {
        MLog(CZFatal, CZLN, "wl_compositor not supported by the compositor");
        return false;
    }

    if (!wl.seat)
    {
        MLog(CZFatal, CZLN, "wl_seat not supported by the compositor");
        return false;
    }

    if (!wl.pointer)
    {
        MLog(CZFatal, CZLN, "wl_pointer not supported by the compositor");
        return false;
    }

    if (!wl.cursorShapeManager)
    {
        MLog(CZFatal, CZLN, "wp_cursor_shape_v1 not supported by the compositor");
        return false;
    }

    wl.cursorShapePointer.set(wp_cursor_shape_manager_v1_get_pointer(wl.cursorShapeManager, wl.pointer));

    if (!wl.xdgWmBase)
    {
        MLog(CZFatal, CZLN, "xdg_wm_base not supported by the compositor");
        return false;
    }

    if (!wl.viewporter)
    {
        MLog(CZFatal, CZLN, "wp_viewporter not supported by the compositor");
        return false;
    }

    m_running = true;
    return true;
}

void MApp::updateSurfaces()
{
    for (MSurface *surf : m_surfaces)
    {
        if (surf->role() == MSurface::Role::SubSurface || surf->role() == MSurface::Role::Popup)
            continue;

        updateSurface(surf);
    }

    wl_display_flush(wl.display);
}

void MApp::updateSurface(MSurface *surf)
{
    // First handle child subsurfaces
    for (MSubsurface *subSurf : surf->subSurfaces())
        updateSurface((MSurface*)subSurf);

    // Then the surface itself
    if (surf->imp()->flags.has(MSurface::Imp::PendingUpdate))
    {
        if (!surf->wlCallback())
            surf->imp()->flags.remove(MSurface::Imp::PendingUpdate);
        surf->onUpdate();
        surf->imp()->tmpFlags.set(0);
    }

    // Finally child popups if any
    std::unordered_set<MPopup*> *childPopups { nullptr };

    switch (surf->role())
    {
    case MSurface::Role::Popup:
        childPopups = &((MPopup*)surf)->imp()->childPopups;
        break;
    case MSurface::Role::Toplevel:
        childPopups = &((MToplevel*)surf)->imp()->childPopups;
        break;
    case MSurface::Role::LayerSurface:
        childPopups = &((MLayerSurface*)surf)->imp()->childPopups;
        break;
    default:
        break;
    }

    if (childPopups)
        for (auto &child : *childPopups)
            updateSurface((MSurface*)child);
}

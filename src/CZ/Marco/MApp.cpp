#include <CZ/Marco/Protocols/Wayland/MWlDataDevice.h>
#include <CZ/Marco/Protocols/CursorShape/cursor-shape-v1-client.h>
#include <CZ/Marco/Protocols/Wayland/MWlPointer.h>
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
#include <CZ/Core/Events/CZColorSchemeEvent.h>
#include <systemd/sd-bus.h>
#include <CZ/Ream/RCore.h>
#include <CZ/Ream/WL/RWLPlatformHandle.h>
#include <CZ/AK/AKApp.h>

#include <CZ/AK/Input/AKKeyboard.h>

#include <sys/mman.h>
#include <assert.h>
#include <sys/poll.h>

using namespace CZ;

std::weak_ptr<MApp> m_app;

std::shared_ptr<MApp> MApp::Make(const MOptions &options) noexcept
{
    if (auto app = m_app.lock())
        return app;

    auto app { std::shared_ptr<MApp>(new MApp(options)) };
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
        MWlPointer::SetCursorFromFocus();
    }

    return ret;
}

void MApp::update() noexcept
{
    m_core->unlockLoop();
}

bool MApp::init() noexcept
{
    m_core = CZCore::GetOrMake();

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

    m_source = CZEventSource::Make(wl_display_get_fd(wl.display), POLLIN, CZOwn::Borrow, [this](auto, auto, auto) {
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

    // Drag & drop (optional)
    if (wl.dataDeviceManager && wl.seat)
    {
        wl.dataDevice.set(wl_data_device_manager_get_data_device(wl.dataDeviceManager, wl.seat));
        wl_data_device_add_listener(wl.dataDevice, &MWlDataDevice::Listener, nullptr);
    }

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
    initColorScheme();
    return true;
}

MApp::~MApp() noexcept
{
    sd_bus_slot_unref(m_colorSchemeSlot);
}

// org.freedesktop.appearance color-scheme: 0 no preference, 1 prefer dark, 2 prefer light.
static CZColorScheme ReadPortalColorScheme(sd_bus_message *m) noexcept
{
    UInt32 value { 0 };

    // The value is a variant, possibly nested in another one (ReadOne vs the older Read).
    if (sd_bus_message_enter_container(m, 'v', "u") >= 0)
    {
        sd_bus_message_read(m, "u", &value);
        sd_bus_message_exit_container(m);
    }
    else if (sd_bus_message_enter_container(m, 'v', "v") >= 0 && sd_bus_message_enter_container(m, 'v', "u") >= 0)
        sd_bus_message_read(m, "u", &value);

    return value == 1 ? CZColorScheme::Dark : CZColorScheme::Light;
}

void MApp::initColorScheme() noexcept
{
    m_userBus = CZBus::GetOrMakeUser();

    if (!m_userBus)
        return;

    static constexpr const char *Portal { "org.freedesktop.portal.Desktop" };
    static constexpr const char *Path { "/org/freedesktop/portal/desktop" };
    static constexpr const char *Iface { "org.freedesktop.portal.Settings" };

    // Changes...
    sd_bus_match_signal_async(m_userBus->bus(), &m_colorSchemeSlot, nullptr, Path, Iface, "SettingChanged",
        [](sd_bus_message *m, void *, sd_bus_error *) -> int {
            const char *ns {}, *key {};

            if (sd_bus_message_read(m, "ss", &ns, &key) >= 0 && ns && key &&
                strcmp(ns, "org.freedesktop.appearance") == 0 && strcmp(key, "color-scheme") == 0)
                if (auto app { MApp::Get() })
                    app->setColorScheme(ReadPortalColorScheme(m));

            return 0;
        }, nullptr, nullptr);

    // ...and the current value (async: the portal may need to be started).
    sd_bus_call_method_async(m_userBus->bus(), nullptr, Portal, Path, Iface, "ReadOne",
        [](sd_bus_message *m, void *, sd_bus_error *) -> int {
            if (!sd_bus_message_is_method_error(m, nullptr))
                if (auto app { MApp::Get() })
                    app->setColorScheme(ReadPortalColorScheme(m));

            return 0;
        }, nullptr, "ss", "org.freedesktop.appearance", "color-scheme");
}

void MApp::setColorScheme(CZColorScheme scheme) noexcept
{
    if (scheme == CZColorScheme::Unknown)
        scheme = CZColorScheme::Light;

    if (m_colorScheme == scheme)
        return;

    m_colorScheme = scheme;

    // Every surface's whole tree (new nodes adopt their parent's scheme, see AKNode::setColorScheme()).
    const CZColorSchemeEvent event { scheme };

    for (MSurface *surface : m_surfaces)
        CZCore::Get()->sendEvent(event, surface->scene());

    onColorSchemeChanged.notify(scheme);
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

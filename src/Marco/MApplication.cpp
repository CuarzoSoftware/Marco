#include <Marco/MApplication.h>
#include <Marco/roles/MSurface.h>
#include <Marco/MTheme.h>
#include <include/gpu/gl/GrGLAssembleInterface.h>
#include <assert.h>

using namespace Marco;

static MApplication *m_app { nullptr };

MApplication *Marco::app() noexcept
{
    return m_app;
}

Marco::MPointer &Marco::pointer() noexcept
{
    return m_app->pointer();
}

static wl_registry_listener wlRegistryListener;
static wl_output_listener wlOutputListener;
static wl_seat_listener wlSeatListener;
static wl_pointer_listener wlPointerListener;
static wl_keyboard_listener wlKeyboardListener;
static xdg_wm_base_listener xdgWmBaseListener;

MApplication::MApplication() noexcept
{
    assert(!app() && "There can not be more than one MApplication per process");
    m_app = this;
    AK::setTheme(new MTheme());
    initWayland();
    initGraphics();
}

int MApplication::exec()
{
    if (m_running)
        return EXIT_FAILURE;

    m_running = true;

    update();

    while (m_running)
    {
        updateEventSources();
        poll(m_fds.data(), m_fds.size(), m_timeout);
        m_timeout = -1;

        for (size_t i = 0; i < m_fds.size(); i++)
        {
            if (m_currentEventSources[i]->m_callback && (m_fds[i].revents & m_fds[i].events))
                m_currentEventSources[i]->m_callback(m_fds[i].fd, m_fds[i].revents);
        }

        for (MSurface *surf : m_surfaces)
        {
            if (surf->cl.pendingUpdate)
            {
                surf->cl.pendingUpdate = false;
                surf->onUpdate();
                surf->cl.changes.reset();
                surf->se.changes.reset();
            }
        }

        wl_display_flush(wl.display);
    }

    return EXIT_SUCCESS;
}

MEventSource *MApplication::addEventSource(Int32 fd, UInt32 events, const MEventSource::Callback &callback) noexcept
{
    m_eventSourcesChanged = true;
    MEventSource *source { new MEventSource(fd, events, callback) };
    m_pendingEventSources.push_back(std::shared_ptr<MEventSource>(source));
    return source;
}

void MApplication::removeEventSource(MEventSource *source) noexcept
{
    for (size_t i = 0; i < m_pendingEventSources.size(); i++)
    {
        if (m_pendingEventSources[i].get() == source)
        {
            m_pendingEventSources.erase(m_pendingEventSources.begin() + i);
            m_eventSourcesChanged = true;
        }
    }
}

void MApplication::wl_registry_global(void *data, wl_registry *registry, UInt32 name, const char *interface, UInt32 version)
{
    auto &wl { *static_cast<MApplication::Wayland*>(data) };

    if (!wl.shm && strcmp(interface, wl_shm_interface.name) == 0)
    {
        wl.shm.set(wl_registry_bind(registry, name, &wl_shm_interface, version), name);
    }
    else if (!wl.compositor && strcmp(interface, wl_compositor_interface.name) == 0)
    {
        wl.compositor.set(wl_registry_bind(registry, name, &wl_compositor_interface, version), name);
    }
    else if (!wl.xdgWmBase && strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        wl.xdgWmBase.set(wl_registry_bind(registry, name, &xdg_wm_base_interface, version), name);
        xdg_wm_base_add_listener(wl.xdgWmBase, &xdgWmBaseListener, NULL);
    }
    else if (strcmp(interface, wl_output_interface.name) == 0)
    {
        wl_output *output = (wl_output*)wl_registry_bind(registry, name, &wl_output_interface, version);
        MScreen *screen { new MScreen(output, name) };
        wl_output_set_user_data(output, screen);
        wl_output_add_listener(output, &wlOutputListener, screen);
        screen->m_appLink = app()->m_pendingScreens.size();
        app()->m_pendingScreens.push_back(screen);
    }
    else if (!wl.seat && strcmp(interface, wl_seat_interface.name) == 0)
    {
        wl.seat.set(wl_registry_bind(registry, name, &wl_seat_interface, version), name);
        wl_seat_add_listener(wl.seat, &wlSeatListener, NULL);
    }
    else if (!wl.xdgDecorationManager && strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0)
    {
        wl.xdgDecorationManager.set(wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, version), name);
    }
}

void MApplication::wl_registry_global_remove(void */*data*/, wl_registry */*registry*/, UInt32 name)
{
    for (MScreen *screen : app()->m_screens)
    {
        if (screen->m_proxy.name() == name)
        {
            if (app()->m_running)
                app()->on.screenUnplugged.notify(*screen);
            app()->m_screens[screen->m_appLink] = app()->m_screens.back();
            app()->m_screens.pop_back();
            delete screen;
            return;
        }
    }

    for (MScreen *screen : app()->m_pendingScreens)
    {
        if (screen->m_proxy.name() == name)
        {
            app()->m_pendingScreens[screen->m_appLink] = app()->m_pendingScreens.back();
            app()->m_pendingScreens.pop_back();
            delete screen;
            return;
        }
    }
}

void MApplication::wl_output_geometry(void *data, wl_output */*output*/, Int32 x, Int32 y, Int32 physicalWidth, Int32 physicalHeight, Int32 subpixel, const char *make, const char *model, Int32 transform)
{
    MScreen &screen { *static_cast<MScreen*>(data) };
    screen.m_pending.pos.fX = x;
    screen.m_pending.pos.fY = y;
    screen.m_pending.physicalSize.fWidth = physicalWidth;
    screen.m_pending.physicalSize.fHeight = physicalHeight;
    screen.m_pending.pixelGeometry = MScreen::wl2SkPixelGeometry(subpixel);
    screen.m_pending.make = make;
    screen.m_pending.model = model;
    screen.m_pending.transform = static_cast<AK::AKTransform>(transform);
    screen.m_changes.add(MScreen::Position | MScreen::PhysicalSize | MScreen::PixelGeometry | MScreen::Make | MScreen::Model | MScreen::Transform);
}

void MApplication::wl_output_mode(void *data, wl_output */*output*/, UInt32 flags, Int32 width, Int32 height, Int32 refresh)
{
    MScreen &screen { *static_cast<MScreen*>(data) };
    screen.m_pending.modes.emplace_back(SkISize::Make(width, height), refresh, bool(flags & WL_OUTPUT_MODE_CURRENT), bool(flags & WL_OUTPUT_MODE_PREFERRED));
    screen.m_changes.add(MScreen::Modes);
}

void MApplication::wl_output_done(void *data, wl_output */*output*/)
{
    MScreen &screen { *static_cast<MScreen*>(data) };
    screen.m_current = screen.m_pending;

    if (screen.m_pendingFirstDone)
    {
        screen.m_pendingFirstDone = false;
        app()->m_pendingScreens[screen.m_appLink] = app()->m_pendingScreens.back();
        app()->m_pendingScreens.pop_back();
        screen.m_appLink = app()->m_screens.size();
        app()->m_screens.push_back(&screen);

        if (app()->m_running)
            app()->on.screenPlugged.notify(screen);
    }
    else if (app()->m_running)
        screen.on.propsChanged.notify(screen, screen.m_changes);

    screen.m_changes.set(0);
}

void MApplication::wl_output_scale(void *data, wl_output */*output*/, Int32 factor)
{
    MScreen &screen { *static_cast<MScreen*>(data) };
    screen.m_pending.scale = factor;
    screen.m_changes.add(MScreen::Scale);
}

void MApplication::wl_output_name(void *data, wl_output */*output*/, const char *name)
{
    MScreen &screen { *static_cast<MScreen*>(data) };
    screen.m_pending.name = name;
    screen.m_changes.add(MScreen::Name);
}

void MApplication::wl_output_description(void *data, wl_output */*output*/, const char *description)
{
    MScreen &screen { *static_cast<MScreen*>(data) };
    screen.m_pending.description = description;
    screen.m_changes.add(MScreen::Description);
}

void MApplication::wl_seat_capabilities(void */*data*/, wl_seat *seat, UInt32 capabilities)
{
    if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && !app()->wl.pointer)
    {
        app()->wl.pointer.set(wl_seat_get_pointer(seat));
        wl_pointer_add_listener(app()->wl.pointer, &wlPointerListener, nullptr);
    }
}

void MApplication::wl_seat_name(void */*data*/, wl_seat */*seat*/, const char */*name*/) {}

void MApplication::wl_pointer_enter(void */*data*/, wl_pointer */*pointer*/, UInt32 serial, wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
{
    MSurface *surf { static_cast<MSurface*>(wl_surface_get_user_data(surface)) };
    auto &p { app()->m_pointer };
    p.m_focus.reset(surf);
    p.m_eventHistory.enter.setX(wl_fixed_to_double(x));
    p.m_eventHistory.enter.setY(wl_fixed_to_double(y));
    p.m_eventHistory.enter.setSerial(serial);
    p.m_forceCursorUpdate = true;
    surf->ak.scene.postEvent(p.m_eventHistory.enter);
}

void MApplication::wl_pointer_leave(void */*data*/, wl_pointer */*pointer*/, UInt32 serial, wl_surface *surface)
{
    MSurface *surf { static_cast<MSurface*>(wl_surface_get_user_data(surface)) };
    auto &p { app()->m_pointer };
    p.m_focus.reset();

    // TODO: notify
    p.m_pressedButtons.clear();
    p.m_eventHistory.leave.setSerial(serial);
    surf->ak.scene.postEvent(p.m_eventHistory.leave);
}

void MApplication::wl_pointer_motion(void */*data*/, wl_pointer */*pointer*/, UInt32 time, wl_fixed_t x, wl_fixed_t y)
{
    auto &p { app()->m_pointer };
    if (!p.focus()) return;

    p.m_eventHistory.move.setMs(time);
    p.m_eventHistory.move.setX(wl_fixed_to_double(x));
    p.m_eventHistory.move.setY(wl_fixed_to_double(y));
    p.focus()->ak.scene.postEvent(p.m_eventHistory.move);

    if (p.focus()->ak.scene.pointerFocus())
        p.setCursor(p.findNonDefaultCursor(p.focus()->ak.scene.pointerFocus()));
}

void MApplication::wl_pointer_button(void */*data*/, wl_pointer */*pointer*/, UInt32 serial, UInt32 time, UInt32 button, UInt32 state)
{
    auto &p { app()->m_pointer };
    if (!p.focus()) return;

    if (state == WL_POINTER_BUTTON_STATE_PRESSED)
        p.m_pressedButtons.insert(button);
    else
        p.m_pressedButtons.erase(button);

    p.m_eventHistory.button.setMs(time);
    p.m_eventHistory.button.setSerial(serial);
    p.m_eventHistory.button.setButton((AK::AKPointerButtonEvent::Button)button);
    p.m_eventHistory.button.setState((AK::AKPointerButtonEvent::State)state);
    p.focus()->ak.scene.postEvent(p.m_eventHistory.button);
}

void MApplication::wl_pointer_axis(void *data, wl_pointer *pointer, UInt32 time, UInt32 axis, wl_fixed_t value)
{

}

void MApplication::wl_pointer_frame(void *data, wl_pointer *pointer)
{

}

void MApplication::wl_pointer_axis_source(void *data, wl_pointer *pointer, UInt32 axis_source)
{

}

void MApplication::wl_pointer_axis_stop(void *data, wl_pointer *pointer, UInt32 time, UInt32 axis)
{

}

void MApplication::wl_pointer_axis_discrete(void *data, wl_pointer *pointer, UInt32 axis, Int32 discrete)
{

}

void MApplication::wl_pointer_axis_value120(void *data, wl_pointer *pointer, UInt32 axis, Int32 value120)
{

}

void MApplication::wl_pointer_axis_relative_direction(void *data, wl_pointer *pointer, UInt32 axis, UInt32 direction)
{

}

void MApplication::xdg_wm_base_ping(void */*data*/, xdg_wm_base *xdgWmBase, UInt32 serial)
{
    xdg_wm_base_pong(xdgWmBase, serial);
}

void MApplication::initWayland() noexcept
{
    wlRegistryListener.global = wl_registry_global;
    wlRegistryListener.global_remove = wl_registry_global_remove;
    wlOutputListener.description = wl_output_description;
    wlOutputListener.done = wl_output_done;
    wlOutputListener.geometry = wl_output_geometry;
    wlOutputListener.mode = wl_output_mode;
    wlOutputListener.name = wl_output_name;
    wlOutputListener.scale = wl_output_scale;
    wlSeatListener.capabilities = wl_seat_capabilities;
    wlSeatListener.name = wl_seat_name;
    wlPointerListener.enter =  wl_pointer_enter;
    wlPointerListener.leave = wl_pointer_leave;
    wlPointerListener.motion = wl_pointer_motion;
    wlPointerListener.button = wl_pointer_button;
    wlPointerListener.axis = wl_pointer_axis;
    wlPointerListener.frame = wl_pointer_frame;
    wlPointerListener.axis_source = wl_pointer_axis_source;
    wlPointerListener.axis_stop = wl_pointer_axis_stop;
    wlPointerListener.axis_discrete = wl_pointer_axis_discrete;
    wlPointerListener.axis_value120 = wl_pointer_axis_value120;
    wlPointerListener.axis_relative_direction = wl_pointer_axis_relative_direction;
    xdgWmBaseListener.ping = xdg_wm_base_ping;

    wl.display = wl_display_connect(NULL);
    assert(wl.display && "wl_display_connect failed");

    m_waylandEventSource = addEventSource(wl_display_get_fd(wl.display), POLLIN, [this](Int32, UInt32){
        if (wl_display_dispatch(wl.display) == -1)
            exit(EXIT_FAILURE);
    });

    m_eventFdEventSource = addEventSource(eventfd(0, O_CLOEXEC), POLLIN, [this](Int32 fd, UInt32){
        m_pendingUpdate = false;
        static eventfd_t val;
        eventfd_read(fd, &val);
    });

    wl.registry.set(wl_display_get_registry(wl.display));
    wl_registry_add_listener(wl.registry, &wlRegistryListener, &wl);
    wl_display_roundtrip(wl.display);
    wl_display_roundtrip(wl.display);

    assert(wl.shm && "wl_shm not supported by the compositor");
    assert(wl.compositor && "wl_compositor not supported by the compositor");
    assert(wl.seat && "wl_seat not supported by the compositor");
    assert(wl.pointer && "wl_pointer not supported by the compositor");
    assert(wl.xdgWmBase && "xdg_wm_base not supported by the compositor");
}

void MApplication::initGraphics() noexcept
{
    gl.eglDisplay = eglGetDisplay(wl.display);

    assert("Failed to create EGLDisplay" && gl.eglDisplay != EGL_NO_DISPLAY);
    assert("Failed to initialize EGLDisplay." && eglInitialize(gl.eglDisplay, NULL, NULL) == EGL_TRUE);
    assert("Failed to bind GL_OPENGL_ES_API." && eglBindAPI(EGL_OPENGL_ES_API) == EGL_TRUE);

    EGLint numConfigs;
    assert("Failed to get EGL configurations." &&
           eglGetConfigs(gl.eglDisplay, NULL, 0, &numConfigs) == EGL_TRUE && numConfigs > 0);

    const EGLint fbAttribs[]
    {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_NONE
    };

    assert("Failed to choose EGL configuration." &&
           eglChooseConfig(gl.eglDisplay, fbAttribs, &gl.eglConfig, 1, &numConfigs) == EGL_TRUE && numConfigs == 1);

    const EGLint contextAttribs[] { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };
    gl.eglContext = eglCreateContext(gl.eglDisplay, gl.eglConfig, EGL_NO_CONTEXT, contextAttribs);
    assert("Failed to create EGL context." && gl.eglContext != EGL_NO_CONTEXT);
    eglMakeCurrent(gl.eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, gl.eglContext);

    // TODO: Check extension
    gl.eglSwapBuffersWithDamageKHR = (PFNEGLSWAPBUFFERSWITHDAMAGEKHRPROC)eglGetProcAddress("eglSwapBuffersWithDamageKHR");

    auto interface = GrGLMakeAssembledInterface(nullptr, (GrGLGetProc)*[](void *, const char *p) -> void * {
        return (void *)eglGetProcAddress(p);
    });

    GrContextOptions contextOptions;
    contextOptions.fShaderCacheStrategy = GrContextOptions::ShaderCacheStrategy::kBackendBinary;
    contextOptions.fAvoidStencilBuffers = true;
    contextOptions.fPreferExternalImagesOverES3 = true;
    contextOptions.fDisableGpuYUVConversion = true;
    contextOptions.fReducedShaderVariations = false;
    contextOptions.fSuppressPrints = true;
    contextOptions.fSuppressMipmapSupport = true;
    contextOptions.fSkipGLErrorChecks = GrContextOptions::Enable::kYes;
    contextOptions.fBufferMapThreshold = -1;
    contextOptions.fDisableDistanceFieldPaths = true;
    contextOptions.fAllowPathMaskCaching = true;
    contextOptions.fGlyphCacheTextureMaximumBytes = 2048 * 1024 * 4;
    contextOptions.fUseDrawInsteadOfClear = GrContextOptions::Enable::kYes;
    contextOptions.fReduceOpsTaskSplitting = GrContextOptions::Enable::kYes;
    contextOptions.fDisableDriverCorrectnessWorkarounds = true;
    contextOptions.fRuntimeProgramCacheSize = 1024;
    contextOptions.fInternalMultisampleCount = 0;
    contextOptions.fDisableTessellationPathRenderer = false;
    contextOptions.fAllowMSAAOnNewIntel = true;
    contextOptions.fAlwaysUseTexStorageWhenAvailable = true;
    gl.skContext = GrDirectContext::MakeGL(interface, contextOptions);
    assert("Failed to create Skia context." && gl.skContext.get());
}

void MApplication::updateEventSources() noexcept
{
    if (!m_eventSourcesChanged) return;
    m_eventSourcesChanged = false;
    m_currentEventSources = m_pendingEventSources;
    m_fds.clear();
    m_fds.reserve(m_currentEventSources.size());

    for (auto &source : m_currentEventSources)
        m_fds.push_back({
            .fd = source->fd(),
            .events = (short)source->events(),
            .revents = 0
        });
}

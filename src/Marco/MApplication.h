#ifndef MAPPLICATION_H
#define MAPPLICATION_H

#include <Marco/MProxy.h>
#include <Marco/MScreen.h>
#include <Marco/input/MPointer.h>
#include <Marco/protocols/xdg-shell-client.h>
#include <Marco/protocols/xdg-decoration-unstable-v1-client.h>
#include <Marco/protocols/wlr-layer-shell-unstable-v1-client.h>
#include <Marco/utils/MEventSource.h>
#include <AK/AKObject.h>
#include <AK/AKWeak.h>
#include <AK/events/AKPointerEnterEvent.h>
#include <AK/events/AKPointerMoveEvent.h>
#include <AK/events/AKPointerLeaveEvent.h>
#include <AK/events/AKPointerButtonEvent.h>
#include <include/gpu/GrDirectContext.h>
#include <wayland-client.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <sys/eventfd.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <array>

class Marco::MApplication : public AK::AKObject
{
public:
    struct Wayland
    {
        wl_display *display { nullptr };
        MProxy<wl_registry> registry;
        MProxy<wl_shm> shm;
        MProxy<wl_compositor> compositor;
        MProxy<xdg_wm_base> xdgWmBase;
        MProxy<zxdg_decoration_manager_v1> xdgDecorationManager;
        MProxy<wl_seat> seat;
        MProxy<wl_pointer> pointer;
        MProxy<wl_keyboard> keyboard;
    };

    struct Graphics
    {
        EGLDisplay eglDisplay;
        EGLConfig eglConfig;
        EGLContext eglContext;
        sk_sp<GrDirectContext> skContext;
        PFNEGLSWAPBUFFERSWITHDAMAGEKHRPROC eglSwapBuffersWithDamageKHR;
    };

    MApplication() noexcept;
    int exec();

    static MTheme *theme() noexcept
    {
        return (MTheme*)AK::theme();
    }

    const std::string &appId() const noexcept
    {
        return m_appId;
    }

    void setAppId(const std::string &appId)
    {
        if (m_appId == appId)
            return;

        m_appId = appId;
        on.appIdChanged.notify(m_appId);
    }

    Wayland &wayland() noexcept
    {
        return wl;
    }

    Graphics &graphics() noexcept
    {
        return gl;
    }

    const std::vector<MSurface*> &surfaces() const noexcept
    {
        return m_surfaces;
    }

    const std::vector<MScreen*> &screens() const noexcept
    {
        return m_screens;
    }

    MEventSource *addEventSource(Int32 fd, UInt32 events, const MEventSource::Callback &callback) noexcept;
    void removeEventSource(MEventSource *source) noexcept;

    void update() noexcept
    {
        //if (m_pendingUpdate)
        //    return;
        m_pendingUpdate = true;
        eventfd_write(m_eventFdEventSource->fd(), 1);
    }

    void setTimeout(Int32 timeout = -1) noexcept
    {
        if (timeout < 0)
            m_timeout = 0;
        else if (timeout < m_timeout || m_timeout < 0)
            m_timeout = timeout;
    }

    MPointer &pointer() noexcept { return m_pointer; }

    struct
    {
        AK::AKSignal<MScreen&> screenPlugged;
        AK::AKSignal<MScreen&> screenUnplugged;
        AK::AKSignal<const std::string&> appIdChanged;
    } on;
private:
    friend class MSurface;
    friend class MScreen;
    static void wl_registry_global(void *data, wl_registry *registry, UInt32 name, const char *interface, UInt32 version);
    static void wl_registry_global_remove(void *data, wl_registry *registry, UInt32 name);
    static void wl_output_geometry(void *data, wl_output *output, Int32 x, Int32 y, Int32 physicalWidth, Int32 physicalHeight, Int32 subpixel, const char *make, const char *model, Int32 transform);
    static void wl_output_mode(void *data, wl_output *output, UInt32 flags, Int32 width, Int32 height, Int32 refresh);
    static void wl_output_done(void *data, wl_output *output);
    static void wl_output_scale(void *data, wl_output *output, Int32 factor);
    static void wl_output_name(void *data, wl_output *output, const char *name);
    static void wl_output_description(void *data, wl_output *output, const char *description);
    static void wl_seat_capabilities(void *data, wl_seat *seat, UInt32 capabilities);
    static void wl_seat_name(void *data, wl_seat *seat, const char *name);
    static void wl_pointer_enter(void *data, wl_pointer *pointer, UInt32 serial, wl_surface *surface, wl_fixed_t x, wl_fixed_t y);
    static void wl_pointer_leave(void *data, wl_pointer *pointer, UInt32 serial, wl_surface *surface);
    static void wl_pointer_motion(void *data,wl_pointer *pointer, UInt32 time, wl_fixed_t x, wl_fixed_t y);
    static void wl_pointer_button(void *data, wl_pointer *pointer, UInt32 serial, UInt32 time, UInt32 button, UInt32 state);
    static void wl_pointer_axis(void *data, wl_pointer *pointer, UInt32 time, UInt32 axis, wl_fixed_t value);
    static void wl_pointer_frame(void *data, wl_pointer *pointer);
    static void wl_pointer_axis_source(void *data, wl_pointer *pointer, UInt32 axis_source);
    static void wl_pointer_axis_stop(void *data, wl_pointer *pointer, UInt32 time, UInt32 axis);
    static void wl_pointer_axis_discrete(void *data, wl_pointer *pointer, UInt32 axis, Int32 discrete);
    static void wl_pointer_axis_value120(void *data, wl_pointer *pointer, UInt32 axis, Int32 value120);
    static void wl_pointer_axis_relative_direction(void *data, wl_pointer *pointer, UInt32 axis, UInt32 direction);
    static void xdg_wm_base_ping(void *data, xdg_wm_base *xdgWmBase, UInt32 serial);
    void initWayland() noexcept;
    void initGraphics() noexcept;
    void updateEventSources() noexcept;
    bool m_running { false };
    bool m_pendingUpdate { false };
    Int32 m_timeout { -1 };

    Wayland wl;
    Graphics gl;
    std::string m_appId;

    MEventSource* m_waylandEventSource, *m_eventFdEventSource;
    std::vector<pollfd> m_fds;
    bool m_eventSourcesChanged { false };
    std::vector<std::shared_ptr<MEventSource>> m_pendingEventSources;
    std::vector<std::shared_ptr<MEventSource>> m_currentEventSources;
    MPointer m_pointer;
    std::vector<MSurface*> m_surfaces;
    std::vector<MScreen*> m_screens, m_pendingScreens;
};

#endif // MAPPLICATION_H

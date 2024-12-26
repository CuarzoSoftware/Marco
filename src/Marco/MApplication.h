#ifndef MAPPLICATION_H
#define MAPPLICATION_H

#include <Marco/MProxy.h>
#include <Marco/MScreen.h>
#include <AK/AKObject.h>
#include <Marco/protocols/xdg-shell-client.h>
#include <Marco/protocols/xdg-decoration-unstable-v1-client.h>
#include <Marco/protocols/wlr-layer-shell-unstable-v1-client.h>
#include <include/gpu/GrDirectContext.h>
#include <wayland-client.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

class Marco::MApplication : public AK::AKObject
{
public:
    struct Wayland
    {
        wl_display *display { nullptr };
        MProxy<wl_registry> registry;
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
    static void xdg_wm_base_ping(void *data, xdg_wm_base *xdgWmBase, UInt32 serial);
    void initWayland() noexcept;
    void initGraphics() noexcept;
    bool m_running { false };
    Wayland wl;
    Graphics gl;
    std::string m_appId;
    std::vector<MSurface*> m_surfaces;
    std::vector<MScreen*> m_screens, m_pendingScreens;
};

#endif // MAPPLICATION_H

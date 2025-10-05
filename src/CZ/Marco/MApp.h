#ifndef MAPPLICATION_H
#define MAPPLICATION_H

#include <CZ/Marco/MProxy.h>
#include <CZ/Marco/MScreen.h>
#include <CZ/Marco/input/MPointer.h>
#include <CZ/AK/AKApp.h>
#include <CZ/Events/CZPointerScrollEvent.h>

class CZ::MApp : public AKObject
{
public:
    struct Wayland
    {
        wl_display *display;
        MProxy<wl_registry> registry;
        MProxy<wl_shm> shm;
        MProxy<wl_compositor> compositor;
        MProxy<wl_subcompositor> subCompositor;
        MProxy<xdg_wm_base> xdgWmBase;
        MProxy<zxdg_decoration_manager_v1> xdgDecorationManager;
        MProxy<wl_seat> seat;
        MProxy<wl_pointer> pointer;
        MProxy<wl_keyboard> keyboard;
        MProxy<wp_viewporter> viewporter;
        MProxy<zwlr_layer_shell_v1> layerShell;
        MProxy<lvr_background_blur_manager> backgroundBlurManager;
        MProxy<lvr_svg_path_manager> svgPathManager;
        MProxy<lvr_invisible_region_manager> invisibleRegionManager;
    };

    enum MaskingCapabilities : UInt32
    {
        NoMaskCap       = 0U,
        RoundRectCap    = 1U,
        SVGPathCap      = 2U
    };

    static std::shared_ptr<MApp> GetOrMake() noexcept;
    static std::shared_ptr<MApp> Get() noexcept;

    int run() noexcept;
    bool running() const noexcept { return m_running; }
    int dispatch(int timeoutMs) noexcept;
    void update() noexcept;

    static MTheme *theme() noexcept { return (MTheme*)CZ::theme(); }
    const std::vector<MSurface*> &surfaces() const noexcept { return m_surfaces; }
    const std::vector<MScreen*> &screens() const noexcept { return m_screens; }
    CZBitset<MaskingCapabilities> maskingCapabilities() const noexcept { return m_maskingcaps; }

    const std::string &appId() const noexcept { return m_appId; }
    void setAppId(const std::string &appId)
    {
        if (m_appId == appId)
            return;

        m_appId = appId;
        onAppIdChanged.notify();
    }

    Wayland wl {};

    CZSignal<MScreen&> onScreenPlugged;
    CZSignal<MScreen&> onScreenUnplugged;
    CZSignal<> onAppIdChanged;
private:
    friend class MSurface;
    friend class MScreen;
    friend class MWlRegistry;
    friend class MWlPointer;
    friend class MWlOutput;
    friend class MLvrBackgroundBlurManager;

    MApp() noexcept;

    bool init() noexcept;
    void updateSurfaces();
    void updateSurface(MSurface *surface);
    std::shared_ptr<CZCore> m_core;
    std::shared_ptr<RCore> m_ream;
    std::shared_ptr<AKApp> m_kay;
    std::shared_ptr<CZEventSource> m_source;

    bool m_running { false };
    std::string m_appId;
    std::vector<MSurface*> m_surfaces;
    std::vector<MScreen*> m_screens, m_pendingScreens;
    CZBitset<MaskingCapabilities> m_maskingcaps;

    std::optional<CZPointerScrollEvent> m_pendingScrollEvent;
};

#endif // MAPPLICATION_H

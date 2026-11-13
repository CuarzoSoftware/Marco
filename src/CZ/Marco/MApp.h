#ifndef MAPPLICATION_H
#define MAPPLICATION_H

#include <CZ/Marco/MProxy.h>
#include <CZ/Marco/MScreen.h>
#include <CZ/AK/AKApp.h>
#include <CZ/Core/Events/CZPointerScrollEvent.h>
#include <CZ/Marco/Extensions/MForeignToplevelManager.h>
#include <CZ/Core/CZColorScheme.h>
#include <CZ/Core/CZColorSchemeMonitor.h>


namespace CZ
{
    enum MExtension
    {
        MForeignToplevelManagerExt = 1 << 0
    };

    struct MOptions
    {
        CZBitset<MExtension> extensions;
    };
};

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
        MProxy<wl_data_device_manager> dataDeviceManager;
        MProxy<wl_data_device> dataDevice;
        MProxy<wl_pointer> pointer;
        MProxy<wl_keyboard> keyboard;
        MProxy<wp_viewporter> viewporter;
        MProxy<zwlr_layer_shell_v1> layerShell;
        MProxy<zwlr_foreign_toplevel_manager_v1> foreignToplevelManager;
        MProxy<lvr_background_blur_manager> backgroundBlurManager;
        MProxy<lvr_svg_path_manager> svgPathManager;
        MProxy<lvr_invisible_region_manager> invisibleRegionManager;
        MProxy<lvr_private_handle_manager> privateHandleManager;
        MProxy<ext_image_copy_capture_manager_v1> imageCopyCaptureManager;
        MProxy<ext_foreign_toplevel_list_v1> foreignToplevelList;
        MProxy<ext_foreign_toplevel_image_capture_source_manager_v1> toplevelCaptureSourceManager;
        MProxy<wp_cursor_shape_manager_v1> cursorShapeManager;
        MProxy<wp_cursor_shape_device_v1> cursorShapePointer;
    };

    enum MaskingCapabilities : UInt32
    {
        NoMaskCap       = 0U,
        RoundRectCap    = 1U,
        SVGPathCap      = 2U
    };

    static std::shared_ptr<MApp> Make(const MOptions &options = {}) noexcept;
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

    const std::string &privateHandle() const noexcept { return m_privateHandle; }

    /**
     * @brief The system color scheme, applied to every surface (and its nodes).
     *
     * Follows the desktop's preference (the XDG desktop portal's `org.freedesktop.appearance`
     * `color-scheme` setting, e.g. GNOME's `org.gnome.desktop.interface color-scheme`), Light if
     * unavailable. Changes are applied live and notified through onColorSchemeChanged.
     */
    CZColorScheme colorScheme() const noexcept { return m_colorScheme; }

    /// Applies @p scheme to every surface (overridden by the desktop's preference when it changes).
    void setColorScheme(CZColorScheme scheme) noexcept;

    CZSignal<CZColorScheme> onColorSchemeChanged;


    Wayland wl {};

    struct
    {
        MForeignToplevelManager foreignToplevelManager;
    } ext;

    CZSignal<MScreen&> onScreenPlugged;
    CZSignal<MScreen&> onScreenUnplugged;
    CZSignal<> onAppIdChanged;
    CZSignal<> onPrivateHandleChanged;
private:
    friend class MSurface;
    friend class MScreen;
    friend struct MWlRegistry;
    friend struct MWlPointer;
    friend struct MWlOutput;
    friend struct MLvrBackgroundBlurManager;
    friend struct MLvrPrivateHandleManager;

    MApp(const MOptions &options) noexcept : m_options(options) {}
public:
    ~MApp() noexcept;
private:
    void initColorScheme() noexcept;
    CZColorScheme m_colorScheme { CZColorScheme::Light };
    std::shared_ptr<CZColorSchemeMonitor> m_colorSchemeMonitor;

    bool init() noexcept;
    void updateSurfaces();
    void updateSurface(MSurface *surface);
    std::shared_ptr<CZCore> m_core;
    std::shared_ptr<RCore> m_ream;
    std::shared_ptr<AKApp> m_kay;
    std::shared_ptr<CZEventSource> m_source;

    bool m_running { false };
    std::string m_appId, m_privateHandle;
    std::vector<MSurface*> m_surfaces;
    std::vector<MScreen*> m_screens, m_pendingScreens;
    CZBitset<MaskingCapabilities> m_maskingcaps;

    std::optional<CZPointerScrollEvent> m_pendingScrollEvent;
    MOptions m_options;
};

#endif // MAPPLICATION_H

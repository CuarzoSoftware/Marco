#ifndef MAPPLICATION_H
#define MAPPLICATION_H

#include <CZ/Marco/MProxy.h>
#include <CZ/Marco/MScreen.h>
#include <CZ/AK/AKApp.h>
#include <CZ/Core/Events/CZPointerScrollEvent.h>
#include <CZ/Marco/Extensions/MForeignToplevelManager.h>
#include <CZ/Marco/Extensions/MSessionLockManager.h>
#include <CZ/Marco/Extensions/MOutputManager.h>
#include <CZ/Core/CZColorScheme.h>
#include <CZ/Core/CZColorSchemeMonitor.h>


namespace CZ
{
    /**
     * @brief Optional protocol extensions MApp can enable (see MOptions::extensions).
     */
    enum MExtension
    {
        /// Binds wlr-foreign-toplevel-management (and ext-foreign-toplevel-list / image capture) to list and control other clients' windows, see MApp::ext and MForeignToplevelManager.
        MForeignToplevelManagerExt = 1 << 0,

        /// Binds wlr-output-management to list the outputs (enabled or not) and change how they're set up, see MApp::ext and MOutputManager.
        MOutputManagerExt = 1 << 1
    };

    /**
     * @brief Options passed to MApp::Make().
     */
    struct MOptions
    {
        CZBitset<MExtension> extensions; ///< Optional extensions to enable (none by default).
    };
};

/**
 * @brief The Marco application: connects to the %Wayland compositor and runs the event loop.
 *
 * There is a single instance, created with Make() and retrieved anywhere with Get(). Make() sets
 * up everything a Kay app needs on %Wayland: the CZCore event loop, the Ream graphics core (on the
 * %Wayland display), the AKApp (Kay) instance and the default MTheme. It then connects to the
 * compositor and binds its globals (see wl); it fails if a required one (wl_compositor, wl_shm,
 * wl_seat with a pointer, xdg_wm_base, wp_viewporter, cursor-shape-v1) is missing.
 *
 * After creating the app, create surfaces (MToplevel, MLayerSurface, ...) with their Kay node
 * trees and call run():
 *
 * @code
 * #include <CZ/Marco/MApp.h>
 * #include <CZ/Marco/Roles/MToplevel.h>
 *
 * using namespace CZ;
 *
 * int main()
 * {
 *     auto app { MApp::Make() };
 *     app->setAppId("org.example.hello");
 *
 *     MToplevel window;
 *     window.setTitle("Hello");
 *     window.setMapped(true);
 *     return app->run();
 * }
 * @endcode
 *
 * It also exposes the compositor's screens (screens(), onScreenPlugged), the system color scheme
 * (colorScheme()) and optional extensions (ext, sessionLockManager()).
 */
class CZ::MApp : public AKObject
{
public:
    /**
     * @brief The %Wayland display and the globals bound by the app.
     *
     * Optional globals are `nullptr` when the compositor doesn't advertise them (or, for extension
     * globals, when the extension wasn't requested in MOptions).
     */
    struct Wayland
    {
        wl_display *display; ///< The connection to the compositor.
        MProxy<wl_registry> registry; ///< The global registry.
        MProxy<wl_shm> shm; ///< wl_shm (required).
        MProxy<wl_compositor> compositor; ///< wl_compositor (required).
        MProxy<wl_subcompositor> subCompositor; ///< wl_subcompositor (for MSubsurface).
        MProxy<xdg_wm_base> xdgWmBase; ///< xdg_wm_base (required).
        MProxy<zxdg_decoration_manager_v1> xdgDecorationManager; ///< xdg-decoration (server-side decorations).
        MProxy<wl_seat> seat; ///< wl_seat (required).
        MProxy<wl_data_device_manager> dataDeviceManager; ///< wl_data_device_manager (drag & drop, clipboard).
        MProxy<wl_data_device> dataDevice; ///< The seat's data device (if dataDeviceManager is available).
        MProxy<wl_pointer> pointer; ///< The seat's pointer (required).
        MProxy<wl_keyboard> keyboard; ///< The seat's keyboard.
        MProxy<wp_viewporter> viewporter; ///< wp_viewporter (required).
        MProxy<zwlr_layer_shell_v1> layerShell; ///< wlr-layer-shell (for MLayerSurface).
        MProxy<zwlr_foreign_toplevel_manager_v1> foreignToplevelManager; ///< wlr-foreign-toplevel-management (MForeignToplevelManagerExt).
        MProxy<zwlr_output_manager_v1> outputManager; ///< wlr-output-management (MOutputManagerExt).
        MProxy<zxdg_output_manager_v1> xdgOutputManager; ///< xdg-output (the screens' logical size, see MScreen::logicalSize()).
        MProxy<lvr_background_blur_manager> backgroundBlurManager; ///< Louvre's background blur.
        MProxy<lvr_svg_path_manager> svgPathManager; ///< Louvre's SVG paths (background blur masks).
        MProxy<lvr_invisible_region_manager> invisibleRegionManager; ///< Louvre's invisible regions.
        MProxy<lvr_private_handle_manager> privateHandleManager; ///< Louvre's private handle (see privateHandle()).
        MProxy<ext_image_copy_capture_manager_v1> imageCopyCaptureManager; ///< ext-image-copy-capture (MForeignToplevel::captureImage()).
        MProxy<ext_foreign_toplevel_list_v1> foreignToplevelList; ///< ext-foreign-toplevel-list (MForeignToplevelManagerExt).
        MProxy<ext_foreign_toplevel_image_capture_source_manager_v1> toplevelCaptureSourceManager; ///< ext-image-capture-source for toplevels (MForeignToplevelManagerExt).
        MProxy<wp_cursor_shape_manager_v1> cursorShapeManager; ///< cursor-shape-v1 (required).
        MProxy<wp_cursor_shape_device_v1> cursorShapePointer; ///< The cursor shape device of pointer.
        MProxy<xdg_activation_v1> activation; ///< xdg-activation (MToplevel::activate()).
    };

    /**
     * @brief Background blur masks supported by the compositor (lvr-background-blur), see maskingCapabilities().
     */
    enum MaskingCapabilities : UInt32
    {
        NoMaskCap       = 0U, ///< Only a blur region.
        RoundRectCap    = 1U, ///< Rounded rectangle masks.
        SVGPathCap      = 2U  ///< SVG path masks.
    };

    /**
     * @brief Creates the app (or returns the existing one) and connects to the compositor.
     *
     * @return The app, or `nullptr` if initialization failed (e.g. no compositor or a required
     *         global is missing).
     */
    static std::shared_ptr<MApp> Make(const MOptions &options = {}) noexcept;

    /** @brief The app created with Make(), or `nullptr`. */
    static std::shared_ptr<MApp> Get() noexcept;

    /** @brief Runs the event loop, dispatching events while running(). Returns the exit() code (0 by default). */
    int run() noexcept;

    /**
     * @brief Stops the event loop: run() returns @p code once the current dispatch ends.
     *
     * E.g. when the app's last window is closed. dispatch() does nothing afterwards.
     */
    void exit(int code = 0) noexcept;

    /** @brief Whether the app was initialized and its loop can be dispatched. */
    bool running() const noexcept { return m_running; }

    /**
     * @brief Waits up to @p timeoutMs for events (-1 blocks) and dispatches them, then updates the surfaces.
     *
     * For integrating the app into a custom loop instead of run().
     *
     * @return The result of `poll()`: 1 if events were dispatched, 0 on timeout (or if not running()), -1 on error.
     */
    int dispatch(int timeoutMs) noexcept;

    /** @brief Wakes the event loop if it is waiting (see CZCore::unlockLoop()). */
    void update() noexcept;

    /** @brief The current theme (an MTheme by default). */
    static MTheme *theme() noexcept { return (MTheme*)CZ::theme(); }

    /** @brief All surfaces created by the app (any role). */
    const std::vector<MSurface*> &surfaces() const noexcept { return m_surfaces; }

    /** @brief The compositor's screens (wl_output), see onScreenPlugged and onScreenUnplugged. */
    const std::vector<MScreen*> &screens() const noexcept { return m_screens; }

    /** @brief Background blur masks supported by the compositor (NoMaskCap if unknown). */
    CZBitset<MaskingCapabilities> maskingCapabilities() const noexcept { return m_maskingcaps; }

    /** @brief The app ID (e.g. `org.example.app`), sent to the compositor for every MToplevel. */
    const std::string &appId() const noexcept { return m_appId; }

    /** @brief Sets the app ID, updating existing toplevels (notifies onAppIdChanged). */
    void setAppId(const std::string &appId)
    {
        if (m_appId == appId)
            return;

        m_appId = appId;
        onAppIdChanged.notify();
    }

    /**
     * @brief The private handle sent by the compositor (lvr-private-handle), empty until received.
     *
     * An opaque, unguessable identifier the compositor assigns to this client, used to identify
     * it through other IPC mechanisms (e.g. D-Bus). See onPrivateHandleChanged.
     */
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

    /** @brief Emitted when colorScheme() changes. */
    CZSignal<CZColorScheme> onColorSchemeChanged;

    /**
     * @brief The session lock manager (ext-session-lock-v1), see MSessionLockManager.
     *
     * Created (the global bound) on the first call. nullptr if the compositor doesn't advertise the
     * protocol to this client (it may restrict it to privileged clients).
     */
    MSessionLockManager *sessionLockManager() noexcept;


    /** @brief The %Wayland display and bound globals. */
    Wayland wl {};

    /** @brief Optional extensions (enabled through MOptions::extensions). */
    struct
    {
        MForeignToplevelManager foreignToplevelManager; ///< Other clients' windows (MForeignToplevelManagerExt).
        MOutputManager outputManager; ///< The outputs and their setup (MOutputManagerExt).
    } ext;

    /** @brief Emitted when a screen is added (after its properties are known). */
    CZSignal<MScreen&> onScreenPlugged;

    /** @brief Emitted when a screen is removed, before it is destroyed. */
    CZSignal<MScreen&> onScreenUnplugged;

    /** @brief Emitted when appId() changes. */
    CZSignal<> onAppIdChanged;

    /** @brief Emitted when privateHandle() is received. */
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
    int m_exitCode { 0 };
    std::string m_appId, m_privateHandle;
    std::vector<MSurface*> m_surfaces;
    std::vector<MScreen*> m_screens, m_pendingScreens;
    CZBitset<MaskingCapabilities> m_maskingcaps;

    std::optional<CZPointerScrollEvent> m_pendingScrollEvent;
    MOptions m_options;

    // ext_session_lock_manager_v1: the global (bound only when used, see sessionLockManager())
    UInt32 m_sessionLockGlobalName { 0 };
    UInt32 m_sessionLockGlobalVersion { 0 };
    std::unique_ptr<MSessionLockManager> m_sessionLockManager;
};

#endif // MAPPLICATION_H

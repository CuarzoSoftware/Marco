#ifndef MWINDOWROLE_H
#define MWINDOWROLE_H

#include <CZ/Marco/Protocols/LvrInvisibleRegion/lvr-invisible-region-client.h>
#include <CZ/Marco/Protocols/Viewporter/viewporter-client.h>
#include <CZ/AK/AKVibrancy.h>
#include <CZ/Core/CZSignal.h>
#include <CZ/Core/CZBorderRadius.h>
#include <CZ/AK/Nodes/AKContainer.h>
#include <CZ/AK/Nodes/AKSolidColor.h>
#include <CZ/AK/AKScene.h>
#include <CZ/skia/core/SkPath.h>
#include <CZ/Marco/Marco.h>
#include <CZ/Core/CZAdaptive.h>
#include <optional>
#include <CZ/Ream/RSwapchain.h>
#include <wayland-client.h>
#include <memory>
#include <set>

namespace CZ
{
    /**
     * @brief How the compositor renders a surface's blurred backdrop (see MSurface::setBlurMaterial()).
     *
     * The backdrop is blurred, saturated and then covered by a translucent tint, like the Apple
     * materials. The presets approximate the macOS ones for each kind of surface.
     */
    struct MBlurMaterial
    {
        /// Gaussian standard deviation in logical units (like CSS `blur()`).
        SkScalar radius { 20.f };

        /// Saturation multiplier: 1 unchanged, 0 grayscale (like CSS `saturate()`).
        SkScalar saturation { 1.8f };

        /// Unpremultiplied tint drawn over the saturated backdrop (alpha = opacity), per color scheme.
        CZAdaptiveColor tint { 0xCCF6F6F6, 0xCC1E1E1E };

        /** @brief Equal when the radius, saturation and tint are equal. */
        bool operator==(const MBlurMaterial &other) const noexcept
        {
            return radius == other.radius && saturation == other.saturation && tint == other.tint;
        }

        static const MBlurMaterial Menu;    ///< Menus (context, dropdown).
        static const MBlurMaterial Popover; ///< Popovers and panels (e.g. a control center).
        static const MBlurMaterial Tooltip; ///< Small labels (e.g. a dock item's name).
        static const MBlurMaterial MenuBar; ///< A translucent top bar.
        static const MBlurMaterial Dock;    ///< A dock: the most translucent one.
    };

    inline const MBlurMaterial MBlurMaterial::Menu    { 20.f, 1.8f, { 0xC7F2F2F2, 0xBF282828 } };
    inline const MBlurMaterial MBlurMaterial::Popover { 24.f, 1.8f, { 0xB8EDEDED, 0xB3242424 } };
    inline const MBlurMaterial MBlurMaterial::Tooltip { 20.f, 1.8f, { 0xD9F5F5F5, 0xD9262626 } };
    inline const MBlurMaterial MBlurMaterial::MenuBar { 24.f, 1.8f, { 0x99F6F6F6, 0x8C1E1E1E } };
    inline const MBlurMaterial MBlurMaterial::Dock    { 24.f, 2.0f, { 0x66F6F6F6, 0x66282828 } };
}

/**
 * @brief Base class for surface roles.
 *
 * ### UI Structure
 *
 * Each MSurface is an AKSolidColor node that represents the central node of the surface.
 * From the data perspective, the MSurface class contains the following members:
 * - `AKScene` (accessible via `scene()`)
 * - `AKTarget` (accessible via `target()`)
 * - `AKContainer` (accessible via `rootNode()`)
 *
 * From the UI perspective, the hierarchy is structured as follows:
 *
 * | - AKScene (scene())
 *   | - AKTarget (target())
 *     | - AKContainer (rootNode())
 *       |- MSurface (this class, the central node)
 *
 * ### Wayland Surface and Layout
 *
 * Most roles configure the internal Wayland surface dimensions to align with the layout of the central node.
 * Use the `layout()` functions to modify the dimensions of the Wayland surface accordingly.
 *
 * In specific cases, such as in `MToplevel`, the central node may be smaller than the actual surface size.
 * This allows room for additional features like window decorations.
 *
 * ### Adding Child Nodes
 *
 * As the central node itself, `MSurface` supports the addition of child nodes, like any other node. However,
 * its `setParent()` function is private and should not be accessed, as doing so would disrupt it's internal logic.
 */
class CZ::MSurface : public AKSolidColor
{
public:
    /** @brief The role of a surface, fixed at construction (see role()). */
    enum class Role
    {
        SubSurface,         ///< MSubsurface (`wl_subsurface`).
        Toplevel,           ///< MToplevel (`xdg_toplevel`).
        Popup,              ///< MPopup (`xdg_popup`), also MMenu and MTooltip.
        LayerSurface,       ///< MLayerSurface (`zwlr_layer_surface_v1`).
        DNDIcon,            ///< MDND, the drag & drop icon.
        SessionLockSurface  ///< MSessionLockSurface (`ext_session_lock_surface_v1`).
    };

    /**
     * @brief Node change ids added by MSurface (see AKNode::changes()).
     *
     * Continues the change ids of the base classes so they never overlap.
     */
    enum Changes
    {
        CHBorderRadius = AKSolidColor::CHLast, ///< The border radius changed (setBorderRadius()).
        CHDecorationMargins,                   ///< The decorations or their margins changed.
        CHLast                                 ///< First id available to subclasses.
    };

    /** @brief Destroys the Wayland surface (and the role objects of subclasses). */
    ~MSurface();

    /** @brief The role of this surface. */
    Role role() noexcept;

    /**
     * @brief The current scale factor.
     *
     * Equal to the larger scale of the intersected screens or the scale suggested by the compositor.
     *
     * Defaults to 1.
     */
    Int32 scale() noexcept;

    /**
     * @brief The Wayland surface size in logical units.
     *
     * Equal to the central node's layout size plus its margins (e.g. the decoration margins).
     */
    SkISize surfaceSize() const noexcept;

    /**
     * @brief The current swapchain buffer size in pixels, or (0,0) if no swapchain has been created yet.
     *
     * May differ from `surfaceSize() * scale()` when wp_viewporter is used.
     */
    SkISize bufferSize() const noexcept;

    /** @brief The screens the surface is currently on (`wl_surface.enter` / `leave`). */
    const std::set<MScreen*> &screens() const noexcept;

    /**
     * @brief Requests to show (map) or hide (unmap) the surface.
     *
     * Mapping is asynchronous: the role object is created on the next update and the surface
     * becomes mapped() once the compositor configures it and a buffer is committed (e.g. a popup
     * also needs a mapped parent). onMappedChanged is emitted when the actual state changes.
     * Surfaces start unmapped.
     */
    void setMapped(bool mapped) noexcept;

    /**
     * @brief Whether the surface is actually mapped (visible), as opposed to requested with setMapped().
     */
    bool mapped() const noexcept;

    /**
     * @brief Schedules a new frame for this surface.
     *
     * Called automatically when the scene is marked dirty. The frame is rendered from the app's
     * event loop, once the previous frame callback is done unless @p force is true.
     */
    void update(bool force = false) noexcept;

    /**
     * @brief The size the central node would have with an auto width and height (its content size).
     */
    SkISize minContentSize() noexcept;

    /** @brief The MSubsurface children of this surface. */
    const std::list<MSubsurface*> &subSurfaces() const noexcept;

    /** @brief The AKScene that renders this surface. */
    AKScene &scene() const noexcept;

    /** @brief The scene target that renders into the surface's swapchain. */
    std::shared_ptr<AKTarget> target() const noexcept;

    /** @brief The scene's root node, parent of the central node (this) and its decorations. */
    AKNode *rootNode() const noexcept;

    /**
     * @brief Rounded-corner radii applied to the content (central node).
     *
     * MSurface renders the rounded corners (via DstIn masks over the content); consumers such as the
     * drop shadow and the background-blur mask read this to match the corners.
     */
    const CZBorderRadius &borderRadius() const noexcept;

    /**
     * @brief Sets the four corner radii. A radius of 0 hides that corner's mask. Emits
     * onBorderRadiusChanged.
     */
    void setBorderRadius(const CZBorderRadius &borderRadius) noexcept;

    /**
     * @brief Installs the surface decorations (shadow, rounded corners, ...), or removes them.
     *
     * Ownership is taken. Pass `nullptr` for no decorations. Decorations are only drawn while
     * a decorations object is set and decorationsEnabled() is true.
     */
    void setDecorations(std::unique_ptr<MDecorations> decorations) noexcept;

    /** @brief The current decorations, or `nullptr` if none is set. */
    MDecorations *decorations() const noexcept;

    /** @brief Whether decorations are currently enabled (independent of whether one is set). */
    bool decorationsEnabled() const noexcept;

    /**
     * @brief Overrides the Wayland input region, in surface-local logical coordinates.
     *
     * By default the input region is the central node's rect (outset a few units for resize zones).
     * Pass `nullptr` to restore the default. The region may extend past the central node (e.g. into
     * decoration margins) but is clipped by the compositor to the surface bounds.
     */
    void setCustomInputRegion(const SkRegion *region) noexcept;

    /** @brief The custom input region, or `nullptr` if the default one is used. */
    const SkRegion *customInputRegion() const noexcept;

    /**
     * @brief Masks the background blur with a path, in central-node-local logical coordinates.
     *
     * By default the blur is masked by the central node's rounded rect (see borderRadius()). With a
     * path set, the mask is sent as an SVG path when the compositor supports it
     * (MApp::SVGPathCap), falling back to the rounded rect otherwise. Pass `nullptr` to restore
     * the default.
     */
    void setBlurMaskPath(const SkPath *path) noexcept;

    /** @brief The blur mask path, or `nullptr` if the default rounded rect is used. */
    const SkPath *blurMaskPath() const noexcept;

    /**
     * @brief Sets how the compositor renders the blurred backdrop (radius, saturation and tint).
     *
     * The tint follows the surface's color scheme. Pass `std::nullopt` (the default) to let the
     * compositor choose. Requires lvr_background_blur version 2, ignored otherwise.
     */
    void setBlurMaterial(const std::optional<MBlurMaterial> &material) noexcept;

    /** @brief The blur material, or `std::nullopt` if the compositor chooses it. */
    const std::optional<MBlurMaterial> &blurMaterial() const noexcept;

    /**
     * @brief Fades the compositor's background blur, from 0 (invisible) to 1 (default).
     *
     * The opacity sent is this one multiplied by the surface's own opacity(), so fading the
     * surface fades its blur too. Requires lvr_background_blur version 2, ignored otherwise.
     */
    void setBlurOpacity(SkScalar opacity) noexcept;

    /** @brief See setBlurOpacity(). */
    SkScalar blurOpacity() const noexcept;

    /** @brief The underlying `wl_surface`. */
    wl_surface *wlSurface() const noexcept;

    /** @brief The pending `wl_surface.frame` callback, or `nullptr` if none. */
    wl_callback *wlCallback() const noexcept;

    /** @brief The `wp_viewport` used to crop the buffer to surfaceSize(). */
    wp_viewport *wlViewport() const noexcept;

    /** @brief The `lvr_invisible_region` of the surface, or `nullptr` if unsupported by the compositor. */
    lvr_invisible_region *wlInvisibleRegion() const noexcept;

    /**
     * @brief Whether vibrancy (the compositor's background blur) is applied.
     *
     * Disabled if the compositor doesn't support it or turned it off, and while the surface is a
     * fullscreen toplevel. Changes are notified to the nodes with an AKVibrancyEvent.
     */
    AKVibrancyState vibrancyState() const noexcept;

    /**
     * @brief Handles vibrancy state changes.
     *
     * The default implementation forwards the event to every node of the surface and then emits
     * onVibrancyChanged.
     */
    virtual void vibrancyEvent(const AKVibrancyEvent &event);

    /** @brief Emitted by vibrancyEvent() when the vibrancy state changes. */
    CZSignal<const AKVibrancyEvent &> onVibrancyChanged;

    /** @brief Emitted by setBorderRadius() when the radii change. */
    CZSignal<> onBorderRadiusChanged;

    /** @brief Emitted when mapped() changes. */
    CZSignal<> onMappedChanged;

    /**
     * @brief Emitted right before the first buffer is committed, the commit that makes the surface visible.
     *
     * E.g. when an `xdg_popup` has been configured by the compositor and its first frame is presented.
     */
    CZSignal<> onWillMap;

    /** @brief Emitted when the surface enters a screen (`wl_surface.enter`), see screens(). */
    CZSignal<MScreen&> onEnteredScreen;

    /** @brief Emitted when the surface leaves a screen (`wl_surface.leave`) or the screen is unplugged. */
    CZSignal<MScreen&> onLeftScreen;

    /**
     * @brief Emitted when the compositor signals the last frame callback (`wl_callback.done`).
     *
     * The argument is the callback timestamp in milliseconds.
     */
    CZSignal<UInt32> onCallbackDone;

    /**
     * @name Drag & drop target
     *
     * Emitted while a drag & drop session (from any client, see MDND) is over this surface.
     * Positions are surface-local (decoration margins included, like worldRect()). The offer is
     * only valid until onDNDLeave / onDNDDrop return; no leave follows a drop.
     */
    ///@{
    /** @brief Emitted when a drag enters the surface, with the offer and the position. */
    CZSignal<MDNDOffer&, const SkPoint&> onDNDEnter;
    /** @brief Emitted when a drag moves over the surface, with the offer and the new position. */
    CZSignal<MDNDOffer&, const SkPoint&> onDNDMotion;
    /** @brief Emitted when a drag leaves the surface without dropping. */
    CZSignal<MDNDOffer&> onDNDLeave;
    /** @brief Emitted when the drag is dropped on the surface, with the offer and the position. */
    CZSignal<MDNDOffer&, const SkPoint&> onDNDDrop;
    ///@}

    /// @cond INTERNAL
    class Imp;
    Imp *imp() const noexcept;
    /// @endcond
protected:
    friend class MApp;
    friend class MDecorations;

    /** @brief Creates the `wl_surface`; the role object is created by the subclass. */
    MSurface(Role role) noexcept;

    /// @cond INTERNAL
    virtual void onUpdate() noexcept;
    /// @endcond
    void colorSchemeEvent(const CZColorSchemeEvent &event) override;
    bool event(const CZEvent &event) noexcept override;

    /** @brief Enables/disables decorations without removing them (roles toggle this, e.g. fullscreen). */
    void enableDecorations(bool enabled) noexcept;

    /**
     * @brief Called after the decorations object is set/replaced/removed via setDecorations().
     *
     * Roles override this to re-derive state that depends on whether decorations are present
     * (e.g. MToplevel re-applies the border radius). Not called by enableDecorations().
     */
    virtual void decorationsChanged() noexcept;

    /** @brief True when a decorations object is set and decorations are enabled. */
    bool decorationsActive() const noexcept;

    /// @cond INTERNAL
    static bool ShouldUpdate(MSurface &window) noexcept;
    static SkISize CalculatedSize(MSurface &window) noexcept;
    static void HandleBackgroundBlur(MSurface &window) noexcept;
    static void PrepareTarget(MSurface &window, const RSwapchainImage &ssImage, SkRegion *outDamage, SkRegion *outOpaque, SkRegion *outInvisible, bool forceFullDamage) noexcept;
    static void AttachInputRegion(MSurface &window) noexcept;
    static void AttachOpaqueRegion(MSurface &window, SkRegion &outOpaque) noexcept;
    static void AttachInvisibleRegion(MSurface &window, SkRegion &outInvisible) noexcept;
    static void PresentImage(MSurface &window, const RSwapchainImage &ssImage, SkRegion &outDamage) noexcept;
    /// @endcond

private:
    void syncDecorationsMargins() noexcept;
    // Positions/sizes/shows the 4 corner masks at the content corners and refreshes their images.
    void updateBorderRadiusMasks() noexcept;

    std::unique_ptr<Imp> m_imp;
    using AKNode::setParent;
    using AKNode::parent;
    using AKNode::setVisible;
};

#endif // MWINDOWROLE_H

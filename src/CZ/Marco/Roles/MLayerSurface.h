#ifndef MLAYERSHELL_H
#define MLAYERSHELL_H

#include <CZ/Marco/Roles/MSurface.h>
#include <CZ/Core/CZEdge.h>

/**
 * @brief WLR Layer Shell Role
 *
 * This role can be used to create many different kind of shell components such as docks, top bars, notifications, wallpapers, and more.
 *
 * For a complete understanding of its semantics, refer to the [protocol documentation](https://wayland.app/protocols/wlr-layer-shell-unstable-v1#zwlr_layer_surface_v1).
 *
 * It implements the `zwlr_layer_surface_v1` role and requires the compositor to support
 * `zwlr_layer_shell_v1`. Properties set with the setters are pending: they are sent on the next
 * update and the surface is first shown after the compositor's initial configure. The size of the
 * surface follows the central node's layout, unless requestAvailableWidth() / requestAvailableHeight()
 * ask the compositor for the available space, which arrives through suggestedSizeChanged().
 *
 * @code
 * class Panel : public MLayerSurface
 * {
 * public:
 *     Panel() noexcept
 *     {
 *         setLayer(Top);
 *         setAnchor(CZEdgeLeft | CZEdgeTop | CZEdgeRight);
 *         setExclusiveZone(32);
 *         layout().setHeight(32);
 *         requestAvailableWidth(); // Span the whole screen width
 *         setMapped(true);
 *     }
 * };
 * @endcode
 *
 * @note Changing the screen or scope after the surface is mapped requires destroying and recreating the layer role, briefly unmapping the surface in the compositor.
 *       This also applies when changing the layer if the compositor supports a version lower than 2.
 */
class CZ::MLayerSurface : public MSurface
{
public:
    /** @brief Stacking layers, from bottom to top (see setLayer()). */
    enum Layer
    {
        Background, ///< Below everything, e.g. wallpapers.
        Bottom,     ///< Below windows, e.g. desktop icons.
        Top,        ///< Above windows, e.g. panels and docks.
        Overlay     ///< Above everything, including fullscreen windows, e.g. lock screens and notifications.
    };

    /** @brief Whether the surface can receive keyboard focus (see setKeyboardInteractivity()). */
    enum KeyboardInteractivity
    {
        None,      ///< Never receives keyboard focus.
        Exclusive, ///< Takes the keyboard focus while mapped (in the Top and Overlay layers).
        OnDemand   ///< Can be focused like a regular window, e.g. when clicked. Requires version 4.
    };

    /**
     * @brief Creates an unmapped layer surface.
     *
     * Asserts that the compositor supports `zwlr_layer_shell_v1`.
     */
    MLayerSurface() noexcept;

    /**
     * @brief Destructor for `MLayerSurface`.
     */
    ~MLayerSurface() noexcept;

    /**
     * @brief Asks the compositor for the available width, notified later through suggestedSizeChanged().
     *
     * Sends a width of 0 in the next `set_size` request, which the compositor replaces with the
     * available width. A no-op unless both the left and right edges are anchored.
     */
    void requestAvailableWidth() noexcept;

    /**
     * @brief Asks the compositor for the available height, notified later through suggestedSizeChanged().
     *
     * Sends a height of 0 in the next `set_size` request, which the compositor replaces with the
     * available height. A no-op unless both the top and bottom edges are anchored.
     */
    void requestAvailableHeight() noexcept;

    /**
     * @brief Sets the screen the surface is shown on, `nullptr` (the default) to let the compositor choose.
     *
     * Changing it recreates the role (see the class note).
     *
     * @return `false` if it was already set.
     */
    bool setScreen(MScreen *screen) noexcept;

    /** @brief The screen set with setScreen(), or `nullptr`. */
    MScreen *screen() const noexcept;

    /**
     * @brief Sets the screen edges the surface is anchored to. Defaults to none (centered).
     *
     * Anchoring to two opposite edges allows requesting the available size along that axis.
     *
     * @return `false` if it was already set.
     */
    bool setAnchor(CZBitset<CZEdge> edges) noexcept;

    /** @brief The anchored edges, see setAnchor(). */
    CZBitset<CZEdge> anchor() const noexcept;

    /**
     * @brief Sets the size of the exclusive zone, the area other surfaces (e.g. maximized windows) avoid.
     *
     * 0 (the default) to be moved to avoid other exclusive zones, -1 to ignore them (stretch to the
     * screen edges), or a positive size reserved along the anchored edge (see setExclusiveEdge()).
     * Values below -1 are clamped to -1.
     *
     * @return `false` if it was already set.
     */
    bool setExclusiveZone(Int32 size) noexcept;

    /** @brief The exclusive zone, see setExclusiveZone(). */
    Int32 exclusiveZone() const noexcept;

    /**
     * @brief Sets the distance from the anchored edges, each rect side being the margin of that edge.
     *
     * Defaults to 0 on all edges.
     *
     * @return `false` if it was already set.
     */
    bool setMargin(const SkIRect &margin) noexcept;

    /** @brief The margins, see setMargin(). */
    const SkIRect &margin() const noexcept;

    /**
     * @brief Sets whether the surface can receive keyboard focus. Defaults to KeyboardInteractivity::None.
     *
     * @return `false` if it was already set or OnDemand is not supported by the compositor.
     */
    bool setKeyboardInteractivity(KeyboardInteractivity mode) noexcept;

    /** @brief The keyboard interactivity, see setKeyboardInteractivity(). */
    KeyboardInteractivity keyboardInteractivity() const noexcept;

    /**
     * @brief Sets the stacking layer. Defaults to Layer::Background.
     *
     * With compositors below version 2 this recreates the role (see the class note).
     *
     * @return `false` if it was already set.
     */
    bool setLayer(Layer layer) noexcept;

    /** @brief The stacking layer, see setLayer(). */
    Layer layer() const noexcept;

    /**
     * @brief Sets the edge the exclusive zone applies to, when the surface is anchored to more than one edge.
     *
     * Ignored (sent as no edge) unless the edge is also anchored. Requires version 5.
     *
     * @return `false` if it was already set or the compositor doesn't support it.
     */
    bool setExclusiveEdge(CZEdge edge) noexcept;

    /** @brief The exclusive edge, see setExclusiveEdge(). */
    CZEdge exclusiveEdge() const noexcept;

    /**
     * @brief Sets the namespace of the surface, which lets the compositor identify its purpose (e.g. "dock").
     *
     * Changing it recreates the role (see the class note).
     *
     * @return `false` if it was already set.
     */
    bool setScope(const std::string &scope) noexcept;

    /** @brief The namespace, see setScope(). */
    const std::string &scope() const noexcept;

    /**
     * @brief The last size sent by the compositor in a configure, in logical units.
     *
     * A component is 0 when the compositor lets the client choose it.
     */
    const SkISize &suggestedSize() const noexcept;

    /**
     * @brief Signal triggered when the compositor requests to close the window.
     *
     * To ignore the request, call `event->ignore()`.
     *
     * If the request is not ignored, the layer surface will be unmapped (see setMapped()) but not
     * destroyed.
     *
     * @note The protocol requires the client to destroy the role, so even if ignored, the surface will be briefly unmapped.
     *       This typically occurs when the assigned output is no longer available. Use this opportunity to reassign it to
     *       another output, or the compositor will pick one.
     */
    CZSignal<const CZCloseEvent&> onBeforeClose;

    /// @cond INTERNAL
    class Imp;
    Imp *imp() const noexcept;
    /// @endcond

protected:
    /**
     * @brief Called when the compositor configures the surface with a new suggestedSize(), and on the first configure.
     *
     * The default implementation applies the non-zero components to the central node's layout,
     * minus the decoration margins.
     */
    virtual void suggestedSizeChanged();

    /// @cond INTERNAL
    virtual void render() noexcept;
    void onUpdate() noexcept override;
    /// @endcond

private:
    std::unique_ptr<Imp> m_imp;
};

#endif // MLAYERSHELL_H

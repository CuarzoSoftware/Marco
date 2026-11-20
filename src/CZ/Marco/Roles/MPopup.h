#ifndef MPOPUP_H
#define MPOPUP_H

#include <CZ/Marco/Roles/MSurface.h>

/**
 * @brief A popup surface, such as a menu or a tooltip, positioned relative to a parent surface.
 *
 * Implements the `xdg_popup` role of the `xdg_shell` protocol. The parent can be an MToplevel,
 * another MPopup or an MLayerSurface (through `zwlr_layer_surface_v1.get_popup`).
 *
 * The position is not chosen by the client: an `xdg_positioner` built from anchorRect(), anchor(),
 * gravity(), offset() and constraintAdjustment() is sent to the compositor, which places the popup
 * (possibly sliding, flipping or resizing it to keep it on screen) and reports the result in
 * assignedRect(). The size follows the central node's layout.
 *
 * Popups are created when mapped with setMapped(true), and only while the parent is mapped.
 * Changing any positioner property while mapped recreates the `xdg_popup`, except a size change,
 * which repositions it (`xdg_popup.reposition`, xdg_wm_base version 3). The compositor can
 * dismiss a popup at any time (e.g. a click outside a grabbing popup): closeEvent() is called and
 * the popup is unmapped, but not destroyed.
 *
 * @code
 * MPopup popup;
 * popup.setParent(&window);
 * popup.setAnchorRect(SkIRect::MakeXYWH(10, 10, 100, 30)); // e.g. a button
 * popup.setAnchor(MPopup::Anchor::BottomLeft);
 * popup.setGravity(MPopup::Gravity::BottomRight);
 * popup.setGrab(&clickEvent); // Dismissed when clicking outside
 * popup.setMapped(true);
 * @endcode
 */
class CZ::MPopup : public MSurface
{
public:
    /** @brief Point of anchorRect() the popup is anchored to (`xdg_positioner.anchor`). */
    enum class Anchor
    {
        None,       ///< The center of the anchor rect.
        Top,        ///< The center of the top edge.
        Bottom,     ///< The center of the bottom edge.
        Left,       ///< The center of the left edge.
        Right,      ///< The center of the right edge.
        TopLeft,    ///< The top-left corner.
        BottomLeft, ///< The bottom-left corner.
        TopRight,   ///< The top-right corner.
        BottomRight ///< The bottom-right corner.
    };

    /** @brief Direction in which the popup extends from the anchor point (`xdg_positioner.gravity`). */
    enum class Gravity
    {
        None,       ///< Centered on the anchor point.
        Top,        ///< Above the anchor point, horizontally centered.
        Bottom,     ///< Below the anchor point, horizontally centered.
        Left,       ///< To the left of the anchor point, vertically centered.
        Right,      ///< To the right of the anchor point, vertically centered.
        TopLeft,    ///< Above and to the left of the anchor point.
        BottomLeft, ///< Below and to the left of the anchor point.
        TopRight,   ///< Above and to the right of the anchor point.
        BottomRight ///< Below and to the right of the anchor point.
    };

    /**
     * @brief How the compositor may adjust the popup if it would be constrained, e.g. off screen
     * (`xdg_positioner.constraint_adjustment`).
     */
    enum ConstraintAdjustment
    {
        SlideX  = 1,  ///< Slide it horizontally.
        SlideY  = 2,  ///< Slide it vertically.
        FlipX   = 4,  ///< Flip the anchor and gravity horizontally.
        FlipY   = 8,  ///< Flip the anchor and gravity vertically.
        ResizeX = 16, ///< Shrink its width.
        ResizeY = 32, ///< Shrink its height.
        All     = SlideX | SlideY | FlipX | FlipY | ResizeX | ResizeY ///< All of the above.
    };

    /** @brief Creates an unmapped popup without a parent. */
    MPopup() noexcept;

    /** @brief Unsets the parent and the parent of its child popups, destroying the role. */
    ~MPopup();

    // Applied only during map

    // setSize internal

    /**
     * @brief Sets the parent surface, `nullptr` to unset it.
     *
     * The popup is only shown while it has a mapped parent.
     *
     * @return `false` if the parent isn't an MToplevel, MPopup or MLayerSurface, or if it would
     *         create a cycle of popups.
     */
    bool setParent(MSurface *parent) noexcept;

    /** @brief The parent surface, or `nullptr`. */
    MSurface *parent() const noexcept;

    /** @brief The popups whose parent is this popup. */
    const std::unordered_set<MPopup*> &childPopups() const noexcept;

    /**
     * @brief Sets the rect of the parent the popup is positioned against, in the parent's central-node coordinates.
     *
     * If any component is negative the whole parent is used.
     */
    void setAnchorRect(const SkIRect &rect) noexcept;

    /** @brief See setAnchorRect(). */
    const SkIRect &anchorRect() const noexcept;

    /** @brief Sets the point of anchorRect() the popup is anchored to. */
    void setAnchor(Anchor anchor) noexcept;

    /** @brief See setAnchor(). */
    Anchor anchor() const noexcept;

    /** @brief Sets the direction in which the popup extends from the anchor point. */
    void setGravity(Gravity gravity) noexcept;

    /** @brief See setGravity(). */
    Gravity gravity() const noexcept;

    /**
     * @brief Sets how the compositor may adjust the popup when constrained. Defaults to ConstraintAdjustment::All.
     */
    void setConstraintAdjustment(CZBitset<ConstraintAdjustment> adjustment) noexcept;

    /** @brief See setConstraintAdjustment(). */
    CZBitset<ConstraintAdjustment> constraintAdjustment() const noexcept;

    /** @brief Sets an offset added to the position computed from the anchor and gravity. Defaults to (0,0). */
    void setOffset(const SkIPoint &offset) noexcept;

    /** @brief Same as setOffset(const SkIPoint &). */
    void setOffset(Int32 x, Int32 y) noexcept { setOffset({x,y}); }

    /** @brief See setOffset(). */
    const SkIPoint &offset() const noexcept;

    /**
     * @brief Requests an explicit grab (`xdg_popup.grab`) when the popup is next created.
     *
     * @p event must be the user input event (e.g. a button press) that triggered the popup; its
     * serial is sent. While grabbed, the popup gets the keyboard focus and is dismissed when the
     * user clicks outside the popup chain. The event is consumed when the popup is created; pass
     * `nullptr` to cancel a pending grab.
     */
    void setGrab(const CZInputEvent *event) noexcept;

    /** @brief The pending grab event, see setGrab(). */
    CZInputEvent *grab() const noexcept;

    /**
     * @brief The position and size assigned by the compositor, relative to the parent's window geometry.
     *
     * Empty until the popup is configured.
     */
    const SkIRect &assignedRect() const noexcept;

    /** @brief Emitted by assignedRectChanged(). */
    CZSignal<> onAssignedRectChanged;

    /** @brief Emitted by closeEvent() when the compositor dismisses the popup (`xdg_popup.popup_done`). */
    CZSignal<const CZCloseEvent&> onClose;

    /// @cond INTERNAL
    class Imp;
    Imp *imp() const noexcept;
    /// @endcond
protected:
    /**
     * @brief Called when the compositor dismisses the popup (`xdg_popup.popup_done`).
     *
     * The default implementation emits onClose. The popup is unmapped afterwards regardless.
     */
    virtual void closeEvent(const CZCloseEvent &event);

    /**
     * @brief Called when the compositor assigns a new assignedRect(), always after the popup is (re)created.
     *
     * The default implementation emits onAssignedRectChanged and applies the assigned size to the layout.
     */
    virtual void assignedRectChanged();
    bool event(const CZEvent &e) noexcept override;
    /// @cond INTERNAL
    void onUpdate() noexcept override;
    /// @endcond

private:
    std::unique_ptr<Imp> m_imp;
    void render() noexcept;
};

#endif // MPOPUP_H

#ifndef MMENU_H
#define MMENU_H

#include <CZ/Marco/Roles/MPopup.h>
#include <CZ/Marco/Nodes/MVibrancyView.h>
#include <CZ/Marco/Nodes/MMenuView.h>
#include <CZ/Marco/Nodes/MMenuAction.h>
#include <CZ/AK/Nodes/AKFontIcon.h>
#include <CZ/Core/CZTimer.h>
#include <functional>
#include <memory>
#include <vector>

/**
 * @brief A popup menu with a vibrancy background and rounded corners, like macOS's.
 *
 * Its content is an MMenuView (view, the slot). Build it with addAction() and addDivider() (owned
 * by the menu, see clear()) and open it under a node with popup(). Activating an item closes it.
 *
 * Taller than the screen, the compositor gives it a smaller height (popup() lets it, ResizeY): like
 * macOS's, its items then scroll, with arrows at the top and/or bottom (where there's more) that
 * scroll while hovered, and with the mouse wheel / touchpad.
 *
 * @code
 * MMenu menu;
 * menu.addAction("Auto-Join", []{ ... }).setChecked(true);
 * menu.addDivider();
 * menu.addAction("Forget This Network…", []{ ... });
 * menu.popup(button, &event); // e.g. from the button's click
 * @endcode
 */
class CZ::MMenu : public MPopup
{
public:
    MMenu() noexcept;
    ~MMenu();

    static constexpr Int32 Padding { 5 }; ///< Around the items
    static constexpr Int32 ScrollArrowHeight { 18 }; ///< The scroll arrows, see the class description
    static constexpr SkScalar ScrollSpeed { 450.f };  ///< Px per second while an arrow is hovered

    /** @brief Adds an item (owned by the menu) running @p onClicked when activated. */
    MMenuAction &addAction(const std::string &text, std::function<void()> onClicked = {}, bool enabled = true) noexcept;

    /** @brief Adds a divider line (owned by the menu). */
    void addDivider() noexcept;

    /** @brief Removes the items added with addAction() and addDivider(). */
    void clear() noexcept;

    /**
     * @brief Opens it below @p anchor (a node of another surface), left-aligned with it (flipped or
     *        slid if there's no room).
     *
     * @param grab The input event that opened it (e.g. a button press), for the popup grab.
     */
    void popup(const AKNode &anchor, const CZInputEvent *grab = nullptr) noexcept;

    /// Scrolls (if it scrolls, see the class description) so @p node (inside view) is shown whole.
    void scrollToShow(const AKNode &node) noexcept;

    /// @cond INTERNAL
    class Imp;
    Imp *imp() const noexcept;
    /// @endcond

protected:
    void assignedRectChanged() override;
    void pointerScrollEvent(const CZPointerScrollEvent &event) override;

    MVibrancyView m_vibrancy { this };

public:
    /** @brief The items (the slot: nodes added to the menu go here). */
    MMenuView view { this };

private:
    // A scroll arrow (top or bottom), scrolling while hovered
    class ScrollArrow final : public AKContainer
    {
    public:
        ScrollArrow(bool up, AKNode *parent) noexcept;
        AKFontIcon icon;
        std::function<void(bool)> onHoverChanged;
    protected:
        void pointerEnterEvent(const CZPointerEnterEvent &event) override;
        void pointerLeaveEvent(const CZPointerLeaveEvent &event) override;
        void colorSchemeEvent(const CZColorSchemeEvent &event) override;
    };

    /// Shows the items from @p offset (clamped), the arrows where there's more.
    void setScrollOffset(SkScalar offset) noexcept;
    void scrollTick() noexcept;

    AKContainer m_clip { YGFlexDirectionColumn, true, this }; // The view, scrolled inside it
    ScrollArrow m_upArrow { true, this }, m_downArrow { false, this };
    std::unique_ptr<CZTimer> m_scrollTimer; // While an arrow is hovered
    SkScalar m_scrollOffset { 0.f };
    Int32 m_scrollDirection { 0 };          // -1 up, 1 down, 0 none
    bool m_scrolling { false };              // Ticking (m_scrollTimer is one-shot, restarted each tick)
    UInt64 m_lastScrollMs { 0 };

    std::vector<std::unique_ptr<AKNode>> m_items;
    std::unique_ptr<Imp> m_imp;
};

#endif // MMENU_H

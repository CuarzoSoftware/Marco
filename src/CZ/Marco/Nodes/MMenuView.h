#ifndef MMENUVIEW_H
#define MMENUVIEW_H

#include <CZ/Marco/Marco.h>
#include <CZ/AK/Nodes/AKContainer.h>
#include <CZ/AK/Nodes/AKRoundSolidColor.h>
#include <CZ/Core/CZSignal.h>
#include <CZ/Core/CZWeak.h>

/**
 * @brief A vertical list of menu rows (MMenuItem) with a macOS-style selection outline.
 *
 * Reusable inside any surface: an MMenu, an MTooltip, ... Add items by parenting them to the view.
 * Dividers at the very top or bottom are hidden automatically.
 */
class CZ::MMenuView : public AKContainer
{
public:
    MMenuView(AKNode *parent = nullptr) noexcept;

    /// Selects (hovers) an item, or clears the selection with nullptr. Disabled items aren't selected.
    void setActiveItem(MMenuItem *item) noexcept;
    MMenuItem *activeItem() const noexcept { return m_activeItem; }

    /// Dims the selection outline while an item is being pressed.
    void setOutlinePressed(bool pressed) noexcept;

    /// Reserves a leading gutter in every item for check marks (keeps titles aligned).
    void setCheckGutter(bool enabled) noexcept;
    bool checkGutter() const noexcept { return m_checkGutter; }

    /**
     * @brief Activates @p item like a click: its highlight blinks a few times (like macOS), then
     *        its onClicked runs and onItemActivated is emitted.
     *
     * Hovering other items does nothing meanwhile. Safe if the menu goes away during the blink.
     */
    void activate(MMenuItem *item) noexcept;

    /// Whether an item is blinking before being activated (see activate()).
    bool activating() const noexcept { return m_activating; }

    /// Emitted after an item is activated (clicked).
    CZSignal<MMenuItem*> onItemActivated;

    AKRoundSolidColor selectionOutline;

protected:
    void pointerLeaveEvent(const CZPointerLeaveEvent &event) override;

private:
    friend class MMenuItem;
    void itemsChanged() noexcept;
    CZWeak<MMenuItem> m_activeItem;
    bool m_checkGutter { false };
    bool m_activating { false };
};

#endif // MMENUVIEW_H

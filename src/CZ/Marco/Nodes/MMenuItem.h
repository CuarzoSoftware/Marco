#ifndef MMENUITEM_H
#define MMENUITEM_H

#include <CZ/Marco/Marco.h>
#include <CZ/AK/Nodes/AKContainer.h>
#include <functional>

/**
 * @brief Base class for the rows of an MMenuView (actions, dividers, ...).
 *
 * Hovering an item makes it the view's active item (selection outline), and releasing the left
 * button over an enabled item activates it: onClicked is invoked and the view emits
 * MMenuView::onItemActivated (owners typically close the menu then).
 */
class CZ::MMenuItem : public AKContainer
{
public:
    MMenuItem(AKNode *parent = nullptr) noexcept;

    /// Called by the view when the item gains/loses the selection.
    virtual void setHover(bool hover) noexcept { CZ_UNUSED(hover) }

    /// Whether the item can be selected and activated.
    virtual bool enabled() const noexcept { return true; }

    /// Shows/hides the leading gutter reserved for check marks (see MMenuView::setCheckGutter()).
    virtual void setCheckGutterVisible(bool visible) noexcept { CZ_UNUSED(visible) }

    /// The view this item belongs to, if any.
    MMenuView *menuView() const noexcept;

    /// Invoked when the item is activated.
    std::function<void()> onClicked;

protected:
    /**
     * @brief Subclasses call this at the end of their constructor.
     *
     * The view refreshes its rows (divider visibility, check gutter) when a child is added, but a
     * row is added from the AKNode constructor, before it is fully built, so the view can't yet see
     * what kind of row it is. This refreshes it again once it can.
     */
    void itemReady() noexcept;
    void pointerEnterEvent(const CZPointerEnterEvent &event) override;
    void pointerButtonEvent(const CZPointerButtonEvent &event) override;
};

#endif // MMENUITEM_H

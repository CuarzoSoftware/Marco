#include <CZ/Marco/Nodes/MMenuItem.h>
#include <CZ/Marco/Nodes/MMenuView.h>
#include <CZ/Core/Events/CZPointerButtonEvent.h>

using namespace CZ;

MMenuItem::MMenuItem(AKNode *parent) noexcept : AKContainer(YGFlexDirectionRow, false, parent)
{
    layout().setAlignItems(YGAlignCenter);
    layout().setJustifyContent(YGJustifyFlexStart);
    layout().setGap(YGGutterAll, 8.f);
}

void MMenuItem::itemReady() noexcept
{
    if (auto *view { menuView() })
        view->itemsChanged();
}

MMenuView *MMenuItem::menuView() const noexcept
{
    return dynamic_cast<MMenuView*>(parent());
}

void MMenuItem::pointerEnterEvent(const CZPointerEnterEvent &event)
{
    AKContainer::pointerEnterEvent(event);

    if (auto *view { menuView() })
        view->setActiveItem(this);
}

void MMenuItem::pointerButtonEvent(const CZPointerButtonEvent &event)
{
    AKContainer::pointerButtonEvent(event);

    if (event.button != BTN_LEFT || !enabled())
        return;

    auto *view { menuView() };

    if (view && view->activating())
        return;

    // Dim the selection outline while the item is held.
    if (view)
        view->setOutlinePressed(event.pressed);

    if (event.pressed)
        return;

    // Blinks, then runs onClicked and notifies onItemActivated (see MMenuView::activate()).
    if (view)
    {
        view->activate(this);
        return;
    }

    if (onClicked)
        onClicked();
}

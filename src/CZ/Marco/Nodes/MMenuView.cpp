#include <CZ/Marco/Nodes/MMenuView.h>
#include <CZ/Marco/Nodes/MMenuItem.h>
#include <CZ/Marco/Nodes/MMenuDivider.h>
#include <CZ/AK/AKTheme.h>

using namespace CZ;

MMenuView::MMenuView(AKNode *parent) noexcept :
    AKContainer(YGFlexDirectionColumn, false, parent),
    selectionOutline(AKTheme::SystemBlue.light(), CZBorderRadius::Make(4))
{
    layout().setAlignItems(YGAlignStretch);
    layout().setMinWidth(160.f);

    SkRegion empty;
    selectionOutline.setInputRegion(&empty);
    selectionOutline.layout().setPositionType(YGPositionTypeAbsolute);
    selectionOutline.layout().setWidthPercent(100.f);

    onChildAdded.subscribe(this, [this](AKNode *, AKNode *){ itemsChanged(); });
    onChildRemoved.subscribe(this, [this](AKNode *){ itemsChanged(); });
}

void MMenuView::itemsChanged() noexcept
{
    // Hide untitled dividers with nothing to separate (first/last item).
    MMenuItem *first {}, *last {};

    for (AKNode *child : children())
        if (auto *item { dynamic_cast<MMenuItem*>(child) })
        {
            if (!first) first = item;
            last = item;
        }

    for (AKNode *child : children())
        if (auto *divider { dynamic_cast<MMenuDivider*>(child) })
            divider->setVisible(divider != first && divider != last);

    for (AKNode *child : children())
        if (auto *item { dynamic_cast<MMenuItem*>(child) })
            item->setCheckGutterVisible(m_checkGutter);
}

void MMenuView::setCheckGutter(bool enabled) noexcept
{
    if (m_checkGutter == enabled)
        return;

    m_checkGutter = enabled;
    itemsChanged();
}

void MMenuView::setActiveItem(MMenuItem *item) noexcept
{
    if (item == m_activeItem)
        return;

    if (m_activeItem)
        m_activeItem->setHover(false);

    m_activeItem.reset(item);

    if (m_activeItem && m_activeItem->enabled())
    {
        m_activeItem->setHover(true);
        selectionOutline.setOpacity(1.f);
        selectionOutline.insertBefore(m_activeItem);
        selectionOutline.layout().setHeight(m_activeItem->worldRect().height());
        selectionOutline.layout().setPosition(YGEdgeLeft, 0.f);
        selectionOutline.layout().setPosition(YGEdgeTop, m_activeItem->layout().calculatedTop());
    }
    else
        selectionOutline.setParent(nullptr);
}

void MMenuView::setOutlinePressed(bool pressed) noexcept
{
    selectionOutline.setOpacity(pressed ? 0.8f : 1.f);
}

void MMenuView::pointerLeaveEvent(const CZPointerLeaveEvent &event)
{
    AKContainer::pointerLeaveEvent(event);
    setActiveItem(nullptr);
}

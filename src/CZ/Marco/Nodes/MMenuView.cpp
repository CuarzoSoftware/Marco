#include <CZ/Marco/Nodes/MMenuView.h>
#include <CZ/Core/CZTimer.h>
#include <CZ/Core/CZWeak.h>
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
    // The item being activated keeps the selection while it blinks.
    if (m_activating)
        return;

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

void MMenuView::activate(MMenuItem *item) noexcept
{
    if (m_activating || !item)
        return;

    m_activating = true;

    // Highlight off, on, off, on (like macOS), then the click.
    static constexpr UInt32 StepMs { 60 };
    static constexpr int Steps { 4 };

    for (int i = 1; i <= Steps; i++)
    {
        CZTimer::OneShot(StepMs * i, [view = CZWeak<MMenuView>(this), weakItem = CZWeak<MMenuItem>(item), on = i % 2 == 0, last = i == Steps](CZTimer *) {
            if (!view || !weakItem)
                return;

            view->selectionOutline.setOpacity(on ? 1.f : 0.f);
            weakItem->setHover(on);

            if (!last)
                return;

            view->m_activating = false;

            if (weakItem->onClicked)
                weakItem->onClicked();

            // The callback may have destroyed the view or the item (e.g. by closing the menu).
            if (view && weakItem)
                view->onItemActivated.notify(weakItem.get());
        });
    }
}

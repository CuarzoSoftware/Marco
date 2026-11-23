#include <CZ/Marco/Roles/MMenu.h>
#include <CZ/Marco/Private/MMenuPrivate.h>
#include <CZ/Marco/Nodes/MShadowDecorations.h>
#include <CZ/Marco/Nodes/MMenuDivider.h>
#include <CZ/AK/AKScene.h>
#include <CZ/AK/AKTheme.h>
#include <CZ/Core/CZTime.h>
#include <CZ/Core/Events/CZPointerScrollEvent.h>
#include <CZ/Core/Events/CZColorSchemeEvent.h>
#include <algorithm>

using namespace CZ;

MMenu::MMenu() noexcept : MPopup()
{
    setBlurMaterial(MBlurMaterial::Menu);
    setColor(CZAdaptiveColor(0));
    setDecorations(std::make_unique<MShadowDecorations>());
    setBorderRadius(CZBorderRadius::Make(8));

    // The blur behind everything, the items (the slot) sizing the menu
    const SkRegion empty;
    m_vibrancy.setInputRegion(&empty);
    m_vibrancy.layout().setPositionType(YGPositionTypeAbsolute);
    m_vibrancy.layout().setPosition(YGEdgeLeft, 0.f);
    m_vibrancy.layout().setPosition(YGEdgeTop, 0.f);
    m_vibrancy.layout().setWidthPercent(100.f);
    m_vibrancy.layout().setHeightPercent(100.f);

    view.layout().setPadding(YGEdgeAll, Padding);
    view.setCheckGutter(true);

    // The items inside a clip: scrolled when the menu gets less height than they need
    view.setParent(&m_clip);
    view.layout().setFlexShrink(0.f); // Always their whole height (the clip shrinks instead)
    m_clip.layout().setFlexShrink(1.f);
    m_clip.layout().setMinHeight(0.f);
    setSlot(&view);

    m_upArrow.onHoverChanged = [this](bool hover){
        m_scrollDirection = hover ? -1 : (m_scrollDirection == -1 ? 0 : m_scrollDirection);
        scrollTick();
    };

    m_downArrow.onHoverChanged = [this](bool hover){
        m_scrollDirection = hover ? 1 : (m_scrollDirection == 1 ? 0 : m_scrollDirection);
        scrollTick();
    };

    // Opened again from the top
    onMappedChanged.subscribe(this, [this]{
        if (!mapped())
        {
            m_scrollDirection = 0;
            setScrollOffset(0.f);
        }
    });

    view.onItemActivated.subscribe(this, [this](MMenuItem*){ setMapped(false); });
}

void MMenu::assignedRectChanged()
{
    MPopup::assignedRectChanged();
    setScrollOffset(m_scrollOffset);
}

void MMenu::setScrollOffset(SkScalar offset) noexcept
{
    const SkScalar arrow { SkScalar(ScrollArrowHeight) };

    // The items' whole height vs the height the compositor gave it
    scene().root()->layout().calculate();
    const SkScalar content { view.layout().calculatedHeight() };
    const SkScalar height { SkScalar(assignedRect().height()) };
    const bool scrolls { height > 0.f && content > height + 0.5f };

    // Scrolled to the end: the top arrow shown, the bottom one hidden
    const SkScalar maxOffset { scrolls ? content - (height - arrow) : 0.f };
    m_scrollOffset = std::clamp(offset, 0.f, std::max(0.f, maxOffset));

    const bool up { scrolls && m_scrollOffset > 0.5f };
    const bool down { scrolls && m_scrollOffset < maxOffset - 0.5f };
    m_upArrow.setVisible(up);
    m_downArrow.setVisible(down);
    m_clip.layout().setMargin(YGEdgeTop, up ? arrow : 0.f);
    m_clip.layout().setMargin(YGEdgeBottom, down ? arrow : 0.f);
    view.layout().setPosition(YGEdgeTop, -m_scrollOffset);

    // Nothing more that way: stop
    if ((m_scrollDirection < 0 && !up) || (m_scrollDirection > 0 && !down))
        m_scrollDirection = 0;

    update();
}

void MMenu::scrollToShow(const AKNode &node) noexcept
{
    if (!m_upArrow.visible() && !m_downArrow.visible())
        return;

    // Its top in view coordinates
    scene().root()->layout().calculate();
    SkScalar top { 0.f };
    const AKNode *n { &node };

    for (; n && n != &view; n = n->parent())
        top += n->layout().calculatedTop();

    if (!n)
        return;

    const SkScalar arrow { SkScalar(ScrollArrowHeight) };
    const SkScalar bottom { top + node.layout().calculatedHeight() };
    const SkScalar visible { SkScalar(assignedRect().height()) - 2.f * arrow }; // Both arrows: the least room

    if (top < m_scrollOffset + arrow)
        setScrollOffset(top - arrow);
    else if (bottom > m_scrollOffset + visible)
        setScrollOffset(bottom - visible);
}

void MMenu::scrollTick() noexcept
{
    if (m_scrollDirection == 0)
    {
        m_scrolling = false;

        if (m_scrollTimer)
            m_scrollTimer->stop();
        return;
    }

    const UInt64 now { CZTime::Ms() };

    if (!m_scrollTimer)
        m_scrollTimer = std::make_unique<CZTimer>([this](CZTimer*){ scrollTick(); });

    // Started now: its first step on the next tick
    if (!m_scrolling)
    {
        m_scrolling = true;
        m_lastScrollMs = now;
    }
    else
    {
        const SkScalar elapsed { SkScalar(std::min<UInt64>(now - m_lastScrollMs, 100)) / 1000.f };
        m_lastScrollMs = now;
        setScrollOffset(m_scrollOffset + m_scrollDirection * ScrollSpeed * elapsed);
    }

    if (m_scrollDirection != 0)
        m_scrollTimer->start(16);
    else
    {
        m_scrolling = false;
        m_scrollTimer->stop();
    }
}

void MMenu::pointerScrollEvent(const CZPointerScrollEvent &event)
{
    MPopup::pointerScrollEvent(event);

    if (event.hasY && (m_upArrow.visible() || m_downArrow.visible()))
        setScrollOffset(m_scrollOffset + event.axes.y());
}

/* ---------------------------------------- ScrollArrow ---------------------------------------- */

MMenu::ScrollArrow::ScrollArrow(bool up, AKNode *parent) noexcept :
    AKContainer(YGFlexDirectionRow, false, parent),
    icon(up ? "expand_less" : "expand_more", 14, this)
{
    const SkRegion empty;
    icon.setInputRegion(&empty);
    layout().setPositionType(YGPositionTypeAbsolute);
    layout().setPosition(YGEdgeLeft, 0.f);
    layout().setPosition(up ? YGEdgeTop : YGEdgeBottom, 0.f);
    layout().setWidthPercent(100.f);
    layout().setHeight(ScrollArrowHeight);
    layout().setJustifyContent(YGJustifyCenter);
    layout().setAlignItems(YGAlignCenter);
    icon.setColor(AKTheme::Label.resolve(colorScheme()));
    setVisible(false);
}

void MMenu::ScrollArrow::pointerEnterEvent(const CZPointerEnterEvent &event)
{
    AKContainer::pointerEnterEvent(event);

    if (onHoverChanged)
        onHoverChanged(true);
}

void MMenu::ScrollArrow::pointerLeaveEvent(const CZPointerLeaveEvent &event)
{
    AKContainer::pointerLeaveEvent(event);

    if (onHoverChanged)
        onHoverChanged(false);
}

void MMenu::ScrollArrow::colorSchemeEvent(const CZColorSchemeEvent &event)
{
    AKContainer::colorSchemeEvent(event);
    icon.setColor(AKTheme::Label.resolve(event.scheme));
}

MMenu::~MMenu()
{
    m_items.clear();
}

MMenu::Imp *MMenu::imp() const noexcept
{
    return m_imp.get();
}

MMenuAction &MMenu::addAction(const std::string &text, std::function<void()> onClicked, bool enabled) noexcept
{
    auto action { std::make_unique<MMenuAction>(text, &view) };
    action->onClicked = std::move(onClicked);
    action->setEnabled(enabled);
    MMenuAction &ref { *action };
    m_items.emplace_back(std::move(action));
    return ref;
}

void MMenu::addDivider() noexcept
{
    m_items.emplace_back(std::make_unique<MMenuDivider>(&view));
}

void MMenu::clear() noexcept
{
    m_items.clear();
}

void MMenu::popup(const AKNode &anchor, const CZInputEvent *grab) noexcept
{
    MSurface *parent { anchor.scene() ? anchor.scene()->window() : nullptr };

    if (!parent)
        return;

    setMapped(false);
    setParent(parent);

    // Its whole size again (the last one may have been shrunk, see assignedRectChanged())
    layout().setWidthAuto();
    layout().setHeightAuto();

    // In the parent's central node coordinates
    SkIRect rect { anchor.worldRect() };
    rect.offset(-parent->worldRect().x(), -parent->worldRect().y());
    setAnchorRect(rect);
    setAnchor(Anchor::BottomLeft);
    setGravity(Gravity::BottomRight);
    setConstraintAdjustment(SlideX | SlideY | FlipX | FlipY | ResizeY); // Too tall: scrolls
    setOffset(0, 4);
    setGrab(grab);
    setMapped(true);
}

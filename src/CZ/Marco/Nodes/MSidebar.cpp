#include <CZ/Marco/Nodes/MSidebar.h>
#include <CZ/AK/AKApp.h>
#include <CZ/AK/AKScene.h>
#include <CZ/Core/Events/CZPointerButtonEvent.h>
#include <CZ/Core/Events/CZPointerMoveEvent.h>
#include <linux/input-event-codes.h>
#include <algorithm>

using namespace CZ;

MSidebar::MSidebar(AKNode *parent) noexcept : MVibrancyView(parent)
{
    layout().setFlexDirection(YGFlexDirectionColumn);
    layout().setHeightPercent(100.f);
    layout().setFlexShrink(0.f);
    layout().setWidth(m_width);
    layout().setMinWidth(m_minWidth);

    header.userCaps.add(UCWindowMove);
    header.layout().setHeight(HeaderHeight);
    header.layout().setFlexShrink(0.f);
    header.layout().setAlignItems(YGAlignCenter);
    header.layout().setPadding(YGEdgeLeft, HeaderPadding);
    header.layout().setPadding(YGEdgeRight, HeaderPadding);
    header.enableChildrenClipping(false);

    content.layout().setFlex(1.f);
}

void MSidebar::setWidth(SkScalar width) noexcept
{
    width = std::clamp(width, m_minWidth, m_maxWidth);

    if (width == m_width)
        return;

    m_width = width;
    layout().setWidth(width);
    onWidthChanged.notify(width);
}

void MSidebar::setWidthRange(SkScalar minWidth, SkScalar maxWidth) noexcept
{
    m_minWidth = std::max(0.f, minWidth);
    m_maxWidth = std::max(m_minWidth, maxWidth);
    layout().setMinWidth(m_minWidth);
    setWidth(m_width);
}

void MSidebar::setResizable(bool resizable) noexcept
{
    m_resizable = resizable;
    m_handle.setVisible(resizable);

    if (!resizable)
        m_handle.stop();
}

SkScalar MSidebar::availableMaxWidth() const noexcept
{
    AKNode *p { parent() };

    if (!p)
        return m_maxWidth;

    // What the parent row has, minus what the siblings need at least.
    SkScalar free { p->layout().calculatedWidth()
        - p->layout().calculatedPadding(YGEdgeLeft)
        - p->layout().calculatedPadding(YGEdgeRight) };

    for (AKNode *sibling : p->children())
    {
        if (sibling == this || !sibling->visible() || sibling->layout().positionType() == YGPositionTypeAbsolute)
            continue;

        if (const YGValue min { sibling->layout().minWidth() }; min.unit == YGUnitPoint)
            free -= min.value;
    }

    return std::clamp(free, m_minWidth, m_maxWidth);
}

/* ------------------------------------- Resize handle ------------------------------------- */

MSidebar::ResizeHandle::ResizeHandle(MSidebar &sidebar) noexcept :
    AKContainer(YGFlexDirectionColumn, false, &sidebar),
    m_sidebar(sidebar)
{
    // Over the trailing edge, above everything else in the sidebar.
    layout().setPositionType(YGPositionTypeAbsolute);
    layout().setPosition(YGEdgeRight, 0.f);
    layout().setPosition(YGEdgeTop, 0.f);
    layout().setWidth(ResizeHandleWidth);
    layout().setHeightPercent(100.f);
    setCursor(CZCursorShape::ResizeColumn);
}

void MSidebar::ResizeHandle::pointerButtonEvent(const CZPointerButtonEvent &e)
{
    AKContainer::pointerButtonEvent(e);

    if (e.button != BTN_LEFT)
        return;

    if (e.pressed && m_sidebar.resizable())
    {
        m_resizing = true;
        m_startWidth = m_sidebar.layout().calculatedWidth();
        m_startX = AKApp::Get()->pointer().pos().x();

        // Keep getting the motion while the pointer is over the content (or outside the window).
        if (scene())
            scene()->setPointerGrab(this);
    }
    else
        stop();
}

void MSidebar::ResizeHandle::pointerMoveEvent(const CZPointerMoveEvent &e)
{
    AKContainer::pointerMoveEvent(e);

    if (!m_resizing)
        return;

    const SkScalar width { m_startWidth + AKApp::Get()->pointer().pos().x() - m_startX };
    m_sidebar.setWidth(std::min(width, m_sidebar.availableMaxWidth()));
}

void MSidebar::ResizeHandle::stop() noexcept
{
    m_resizing = false;

    if (scene() && scene()->pointerGrab() == this)
        scene()->setPointerGrab(nullptr);
}

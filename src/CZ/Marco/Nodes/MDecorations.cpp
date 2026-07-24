#include <CZ/Marco/Nodes/MDecorations.h>
#include <CZ/Marco/Private/MSurfacePrivate.h>
#include <CZ/Marco/Roles/MSurface.h>

using namespace CZ;

MDecorations::MDecorations() noexcept : AKRenderable(RenderableHint::Image)
{
    SkRegion empty;
    setInputRegion(&empty);
    layout().setPositionType(YGPositionTypeAbsolute);
    layout().setPosition(YGEdgeLeft, 0.f);
    layout().setPosition(YGEdgeTop, 0.f);
    layout().setWidthPercent(100);
    layout().setHeightPercent(100);
}

void MDecorations::onBind() noexcept
{
    setParent(surface()->rootNode());
}

void MDecorations::onUnbind() noexcept
{
    setParent(nullptr);
}

void MDecorations::renderEvent(const AKRenderEvent &) {}

bool MDecorations::setMargins(SkIRect margins) noexcept
{
    margins.fLeft = std::max(0, margins.fLeft);
    margins.fTop = std::max(0, margins.fTop);
    margins.fRight = std::max(0, margins.fRight);
    margins.fBottom = std::max(0, margins.fBottom);

    if (margins == m_margins) return false;
    m_margins = margins;

    if (surface())
        surface()->syncDecorationsMargins();

    return true;
}

void MDecorations::setSurface(MSurface *surface) noexcept
{
    if (m_surface == surface) return;

    if (m_surface)
        onUnbind();

    m_surface = surface;

    if (m_surface)
        onBind();
}

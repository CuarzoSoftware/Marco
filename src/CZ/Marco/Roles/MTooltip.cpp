#include <CZ/Marco/Roles/MTooltip.h>
#include <CZ/AK/Events/AKBakeEvent.h>
#include <CZ/AK/Events/AKVibrancyEvent.h>
#include <CZ/Core/Events/CZLayoutEvent.h>
#include <CZ/Ream/RSurface.h>
#include <CZ/Ream/RPass.h>
#include <CZ/skia/core/SkCanvas.h>
#include <CZ/skia/core/SkMatrix.h>
#include <CZ/skia/effects/SkImageFilters.h>
#include <algorithm>

using namespace CZ;

/*
 * Builds the outline for a tooltip of size W (along the caret edge) x H, with the caret on the
 * bottom edge centered at x = caretCenter.
 *
 * Recorded with cubic Béziers only (no arcs/conics, which SVG can't express and would be split into
 * many quads), so the SVG blur mask stays at ~12 commands:
 *  - Body corners: circular quarter arcs (cubic approximation, k = 0.5523).
 *  - Caret: one S-shaped cubic per side, horizontal at both ends. The base flares tangentially out
 *    of the edge and the tip is a smooth round cap, like the macOS popover arrow.
 */
static SkPath CanonicalShape(SkScalar W, SkScalar H, SkScalar caretCenter) noexcept
{
    constexpr SkScalar k { 0.5523f };          // Circle quadrant handle length / radius
    constexpr SkScalar baseHandle { 0.56f };   // How far the base flare runs along the edge (× half width)
    constexpr SkScalar tipHandle  { 0.22f };   // How round the tip is (× half width)

    const SkScalar bodyH { H - MTooltip::CaretHeight };
    const SkScalar hw { MTooltip::CaretWidth * 0.5f };
    const SkScalar r { std::min<SkScalar>(MTooltip::BorderRadius, std::min(W, bodyH) * 0.5f) };
    const SkScalar c { r * k };

    // Keep the caret on the straight part of the edge.
    const SkScalar margin { r + hw };
    const SkScalar cx { margin * 2.f < W ? std::clamp(caretCenter, margin, W - margin) : W * 0.5f };

    SkPath p;
    p.moveTo(r, 0.f);
    p.lineTo(W - r, 0.f);
    p.cubicTo(W - r + c, 0.f,    W, r - c,            W, r);              // Top right
    p.lineTo(W, bodyH - r);
    p.cubicTo(W, bodyH - r + c,  W - r + c, bodyH,    W - r, bodyH);      // Bottom right
    p.lineTo(cx + hw, bodyH);
    p.cubicTo(cx + hw * (1.f - baseHandle), bodyH,    cx + hw * tipHandle, H,   cx, H);         // Caret right side
    p.cubicTo(cx - hw * tipHandle, H,    cx - hw * (1.f - baseHandle), bodyH,   cx - hw, bodyH); // Caret left side
    p.lineTo(r, bodyH);
    p.cubicTo(r - c, bodyH,      0.f, bodyH - r + c,  0.f, bodyH - r);    // Bottom left
    p.lineTo(0.f, r);
    p.cubicTo(0.f, r - c,        r - c, 0.f,          r, 0.f);            // Top left
    p.close();
    return p;
}

MTooltip::MTooltip() noexcept : MPopup()
{
    // The central node is transparent: the shape node draws the outline, and the blur is masked by
    // the same path.
    setColor(CZAdaptiveColor(0));
    setBorderRadius(CZBorderRadius::Make(0));

    SkRegion empty;
    m_vibrancy.setParent(this);
    m_vibrancy.setDisabledColor(CZAdaptiveColor(0)); // The fallback fill is drawn along the shape
    m_vibrancy.setInputRegion(&empty);
    m_vibrancy.layout().setPositionType(YGPositionTypeAbsolute);
    m_vibrancy.layout().setPosition(YGEdgeLeft, 0.f);
    m_vibrancy.layout().setPosition(YGEdgeTop, 0.f);
    m_vibrancy.layout().setWidthPercent(100.f);
    m_vibrancy.layout().setHeightPercent(100.f);

    m_shapeNode.setParent(this);

    m_content.setParent(this);
    m_content.layout().setFlexDirection(YGFlexDirectionColumn);
    setSlot(&m_content);
    updatePadding();

    auto shadow { std::make_unique<Shadow>(*this) };
    m_shadow = shadow.get();
    setDecorations(std::move(shadow));

    // Redraw the fill when blur becomes (un)available.
    onVibrancyChanged.subscribe(this, [this](const AKVibrancyEvent &){ m_shapeNode.rebake(); });
}

void MTooltip::setFallbackColor(CZAdaptiveColor color) noexcept
{
    if (m_fallbackColor == color)
        return;

    m_fallbackColor = color;
    m_shapeNode.rebake();
}

void MTooltip::setBlurEnabled(bool enabled) noexcept
{
    if (m_blurEnabled == enabled)
        return;

    m_blurEnabled = enabled;
    m_vibrancy.setVisible(enabled); // No vibrancy view: no blur region
    setBlurMaskPath(enabled ? &m_shape : nullptr);
    m_shapeNode.rebake();
    update();
}

static void SetSubtreeOpacity(AKNode *node, SkScalar opacity) noexcept
{
    for (AKNode *child : node->children(true))
    {
        if (auto *renderable { dynamic_cast<AKRenderable*>(child) })
            renderable->setOpacity(opacity);

        SetSubtreeOpacity(child, opacity);
    }
}

void MTooltip::setFadeOpacity(SkScalar opacity) noexcept
{
    opacity = std::clamp(opacity, 0.f, 1.f);
    m_fadeOpacity = opacity;
    m_shapeNode.setOpacity(opacity);

    if (m_shadow)
        m_shadow->setShadowOpacity(opacity);

    SetSubtreeOpacity(&m_content, opacity);
    update();
}

void MTooltip::pointTo(MSurface *parent, const SkIPoint &point, CZEdge caretEdge) noexcept
{
    m_target = point;
    m_requestedEdge = caretEdge;
    m_caretEdge = caretEdge;
    m_caretOffset = -1.f;

    setParent(parent);
    setAnchorRect(SkIRect::MakeXYWH(point.x(), point.y(), 1, 1));
    setOffset(0, 0);

    // The tooltip goes on the opposite side of its caret. It may slide along the caret edge (the
    // caret then follows the target) or flip across it.
    switch (caretEdge)
    {
    case CZEdgeTop:
        setAnchor(Anchor::Bottom);
        setGravity(Gravity::Bottom);
        setConstraintAdjustment(SlideX | FlipY);
        break;
    case CZEdgeLeft:
        setAnchor(Anchor::Right);
        setGravity(Gravity::Right);
        setConstraintAdjustment(SlideY | FlipX);
        break;
    case CZEdgeRight:
        setAnchor(Anchor::Left);
        setGravity(Gravity::Left);
        setConstraintAdjustment(SlideY | FlipX);
        break;
    default:
        m_requestedEdge = m_caretEdge = CZEdgeBottom;
        setAnchor(Anchor::Top);
        setGravity(Gravity::Top);
        setConstraintAdjustment(SlideX | FlipY);
        break;
    }

    updatePadding();
    updateShape();
}

void MTooltip::assignedRectChanged()
{
    // Keep the size the content asks for (MPopup would apply the assigned size, which never differs
    // as resizing isn't allowed).
    onAssignedRectChanged.notify();
    updateCaret();
}

void MTooltip::layoutEvent(const CZLayoutEvent &event)
{
    MPopup::layoutEvent(event);

    if (event.changes.has(CZLayoutChangeSize))
        updateShape();
}

void MTooltip::updateCaret() noexcept
{
    const SkIRect &rect { assignedRect() };

    if (rect.isEmpty())
        return;

    // Flipped by the compositor: the caret goes to the other edge.
    CZEdge edge { m_requestedEdge };

    switch (m_requestedEdge)
    {
    case CZEdgeBottom: if (rect.top() >= m_target.y()) edge = CZEdgeTop; break;
    case CZEdgeTop:    if (rect.bottom() <= m_target.y()) edge = CZEdgeBottom; break;
    case CZEdgeLeft:   if (rect.right() <= m_target.x()) edge = CZEdgeRight; break;
    case CZEdgeRight:  if (rect.left() >= m_target.x()) edge = CZEdgeLeft; break;
    default: break;
    }

    // Slid by the compositor: point at the target from wherever the popup is.
    const bool horizontal { edge == CZEdgeTop || edge == CZEdgeBottom };
    m_caretOffset = horizontal ? Float32(m_target.x() - rect.x()) : Float32(m_target.y() - rect.y());

    if (edge != m_caretEdge)
    {
        m_caretEdge = edge;
        updatePadding();
    }

    updateShape();
}

void MTooltip::updatePadding() noexcept
{
    m_content.layout().setPadding(YGEdgeAll, Padding);

    switch (m_caretEdge)
    {
    case CZEdgeTop:    m_content.layout().setPadding(YGEdgeTop, Padding + CaretHeight); break;
    case CZEdgeLeft:   m_content.layout().setPadding(YGEdgeLeft, Padding + CaretHeight); break;
    case CZEdgeRight:  m_content.layout().setPadding(YGEdgeRight, Padding + CaretHeight); break;
    default:           m_content.layout().setPadding(YGEdgeBottom, Padding + CaretHeight); break;
    }
}

void MTooltip::updateShape() noexcept
{
    const SkScalar w { layout().calculatedWidth() };
    const SkScalar h { layout().calculatedHeight() };

    if (w <= 0.f || h <= 0.f)
        return;

    const bool horizontal { m_caretEdge == CZEdgeTop || m_caretEdge == CZEdgeBottom };

    // Canonical frame: caret on the bottom edge, W along it, H across it.
    const SkScalar W { horizontal ? w : h };
    const SkScalar H { horizontal ? h : w };
    const SkScalar center { m_caretOffset < 0.f ? W * 0.5f : m_caretOffset };

    SkMatrix m;

    switch (m_caretEdge)
    {
    case CZEdgeTop:   m.setAll(1, 0, 0,   0, -1, H,   0, 0, 1); break; // Flip vertically
    case CZEdgeRight: m.setAll(0, 1, 0,   1,  0, 0,   0, 0, 1); break; // Swap axes
    case CZEdgeLeft:  m.setAll(0, -1, H,  1,  0, 0,   0, 0, 1); break; // Swap axes, flip x
    default:          m.reset(); break;
    }

    SkPath shape;
    CanonicalShape(W, H, center).transform(m, &shape);

    if (shape == m_shape)
        return;

    m_shape = shape;
    m_shapeSize = SkISize(SkScalarRoundToInt(w), SkScalarRoundToInt(h));
    if (m_blurEnabled)
        setBlurMaskPath(&m_shape);
    m_shapeNode.rebake();

    if (m_shadow)
        m_shadow->rebake();

    onShapeChanged.notify();
}

/* --------------------------------------- ShapeNode --------------------------------------- */

MTooltip::ShapeNode::ShapeNode(MTooltip &tooltip) noexcept : m_tooltip(tooltip)
{
    SkRegion empty;
    setInputRegion(&empty);
    layout().setPositionType(YGPositionTypeAbsolute);
    layout().setPosition(YGEdgeLeft, 0.f);
    layout().setPosition(YGEdgeTop, 0.f);
    layout().setWidthPercent(100.f);
    layout().setHeightPercent(100.f);
    onColorSchemeChanged.subscribe(this, [this](CZColorScheme){ rebake(); });
}

void MTooltip::ShapeNode::rebake() noexcept
{
    addDamage(AK_IRECT_INF);
}

void MTooltip::ShapeNode::bakeEvent(const AKBakeEvent &e)
{
    if (e.damage.isEmpty())
        return;

    auto pass { e.surface->beginPass(RPassCap_SkCanvas) };
    SkCanvas *c { pass->getCanvas() };
    c->clear(SK_ColorTRANSPARENT);

    const SkPath &shape { m_tooltip.shape() };

    if (shape.isEmpty())
        return;

    SkPaint paint;
    paint.setAntiAlias(true);

    // Without blur, fill the shape (with blur, the compositor's material shows through).
    if (!m_tooltip.m_blurEnabled || m_tooltip.vibrancyState() != AKVibrancyState::Enabled)
    {
        paint.setColor(m_tooltip.m_fallbackColor.resolve(colorScheme()));
        c->drawPath(shape, paint);
    }

    // Hairlines inside the edge, one physical pixel each: a light line under a dark outer one.
    // Clipping to the shape keeps only the inner half of each stroke.
    const SkScalar px { 1.f / std::max<Int32>(1, scale()) };
    const bool dark { colorScheme() == CZColorScheme::Dark };

    c->save();
    c->clipPath(shape, true);
    paint.setStroke(true);
    paint.setColor(dark ? 0x33FFFFFF : 0x80FFFFFF);
    paint.setStrokeWidth(px * 4.f);
    c->drawPath(shape, paint);
    paint.setColor(dark ? 0x99000000 : 0x26000000);
    paint.setStrokeWidth(px * 2.f);
    c->drawPath(shape, paint);
    c->restore();
}

/* ----------------------------------------- Shadow ----------------------------------------- */

MTooltip::Shadow::Shadow(MTooltip &tooltip) noexcept :
    MDecorations(),
    m_node(tooltip, *this)
{
    invisibleRegion.setRect(AK_IRECT_INF);
    SkRegion empty;
    setInputRegion(&empty);
}

void MTooltip::Shadow::onBind() noexcept
{
    MDecorations::onBind();
    setMargins(SkIRect {
        ShadowRadius,                   // L
        ShadowRadius - ShadowOffsetY,   // T
        ShadowRadius,                   // R
        ShadowRadius + ShadowOffsetY    // B
    });
    rebake();
}

MTooltip::Shadow::Node::Node(MTooltip &tooltip, Shadow &shadow) noexcept :
    m_tooltip(tooltip),
    m_shadow(shadow)
{
    setParent(&shadow);
    SkRegion empty;
    setInputRegion(&empty);
    layout().setPositionType(YGPositionTypeAbsolute);
    layout().setPosition(YGEdgeLeft, 0.f);
    layout().setPosition(YGEdgeTop, 0.f);
    layout().setWidthPercent(100.f);
    layout().setHeightPercent(100.f);
}

void MTooltip::Shadow::Node::rebake() noexcept
{
    addDamage(AK_IRECT_INF);
}

void MTooltip::Shadow::Node::bakeEvent(const AKBakeEvent &e)
{
    if (e.damage.isEmpty())
        return;

    auto pass { e.surface->beginPass(RPassCap_SkCanvas) };
    SkCanvas *c { pass->getCanvas() };
    c->clear(SK_ColorTRANSPARENT);

    if (m_tooltip.shape().isEmpty())
        return;

    // The shape is central-node-local; the decorations area starts at the margins.
    SkPath shape;
    m_tooltip.shape().offset(m_shadow.margins().fLeft, m_shadow.margins().fTop, &shape);

    SkPaint paint;
    paint.setAntiAlias(true);
    const SkScalar sigma { ShadowRadius / 3.f };
    paint.setImageFilter(SkImageFilters::DropShadowOnly(0.f, ShadowOffsetY, sigma, sigma, 0x59000000, nullptr));
    c->drawPath(shape, paint);

    // Only outside the tooltip: its (translucent) interior must not be darkened.
    paint.setImageFilter(nullptr);
    paint.setBlendMode(SkBlendMode::kClear);
    c->drawPath(shape, paint);
}

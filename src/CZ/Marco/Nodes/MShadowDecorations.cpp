#include <CZ/Marco/Nodes/MShadowDecorations.h>
#include <CZ/Marco/Private/MSurfacePrivate.h>
#include <CZ/Marco/Roles/MSurface.h>
#include <CZ/Marco/MApp.h>
#include <CZ/AK/Events/AKRenderEvent.h>
#include <CZ/Core/Events/CZLayoutEvent.h>
#include <CZ/Ream/RImage.h>
#include <CZ/Ream/RPainter.h>
#include <CZ/Ream/RPass.h>

using namespace CZ;

MShadowDecorations::MShadowDecorations() noexcept :
    MDecorations(),
    m_shadow(this)
{
    auto app { MApp::Get() };

    for (int i = 0; i < 4; i++)
    {
        m_cornerRadius[i].setParent(this);
        m_cornerRadius[i].layout().setPositionType(YGPositionTypeAbsolute);
        m_cornerRadius[i].layout().setWidth(app->theme()->CSDBorderRadius);
        m_cornerRadius[i].layout().setHeight(app->theme()->CSDBorderRadius);
        m_cornerRadius[i].enableCustomBlendFunc(true);
        m_cornerRadius[i].enableAutoDamage(false);
        m_cornerRadius[i].setCustomBlendMode(RBlendMode::DstIn);
    }

    // TL
    m_cornerRadius[0].setSrcTransform(CZTransform::Normal);

    // TR
    m_cornerRadius[1].setSrcTransform(CZTransform::Rotated90);

    // BR
    m_cornerRadius[2].setSrcTransform(CZTransform::Rotated180);

    // BL
    m_cornerRadius[3].setSrcTransform(CZTransform::Rotated270);

    // Shadow renders last (after the corner masks), so the masks don't carve into it.
    m_shadow.setParent(this);

    invisibleRegion.setRect(AK_IRECT_INF);
}

void MShadowDecorations::setRadius(Int32 radius) noexcept
{
    radius = std::max(0, radius);

    if (m_radius == radius)
        return;

    m_radius = radius;
    updateMargins();
}

void MShadowDecorations::setOffset(const SkIPoint &offset) noexcept
{
    if (m_offset == offset)
        return;

    m_offset = offset;
    updateMargins();
}

void MShadowDecorations::subtractOpaque(SkRegion &opaque) const noexcept
{
    for (int i = 0; i < 4; i++)
        opaque.op(m_cornerRadius[i].worldRect(), SkRegion::Op::kDifference_Op);
}

void MShadowDecorations::onBind() noexcept
{
    MDecorations::onBind();

    auto app { MApp::Get() };

    auto mask { app->theme()->csdBorderRadiusMask(scale()) };

    for (int i = 0; i < 4; i++)
        m_cornerRadius[i].setImage(mask);

    updateMargins();
}

void MShadowDecorations::updateMargins() noexcept
{
    // Margins are derived purely from the radius and offset (no activation check). A positive
    // offset shifts the shadow towards that edge, leaving less room on the opposite side.
    setMargins(SkIRect {
        m_radius - m_offset.x(),  // L
        m_radius - m_offset.y(),  // T
        m_radius + m_offset.x(),  // R
        m_radius + m_offset.y()   // B
    });

    const SkIRect m { margins() };

    m_cornerRadius[0].layout().setPosition(YGEdgeLeft, m.fLeft);
    m_cornerRadius[0].layout().setPosition(YGEdgeTop, m.fTop);
    m_cornerRadius[1].layout().setPosition(YGEdgeRight, m.fRight);
    m_cornerRadius[1].layout().setPosition(YGEdgeTop, m.fTop);
    m_cornerRadius[2].layout().setPosition(YGEdgeRight, m.fRight);
    m_cornerRadius[2].layout().setPosition(YGEdgeBottom, m.fBottom);
    m_cornerRadius[3].layout().setPosition(YGEdgeLeft, m.fLeft);
    m_cornerRadius[3].layout().setPosition(YGEdgeBottom, m.fBottom);

    // An offset-only change keeps the total margins (and thus the surface size) unchanged, so no
    // layout event would fire; rebuild the shadow image here so the new offset takes effect.
    m_shadow.rebuild();
}

void MShadowDecorations::updateInvisibleRegion() noexcept
{
    const SkIRect m { margins() };
    const Int32 w { Int32(m_shadow.layout().calculatedWidth()) };
    const Int32 h { Int32(m_shadow.layout().calculatedHeight()) };

    // The central widget is opaque and fully covers the shadow behind it, except at its rounded
    // corners. Mark that area (the central rect minus the border-radius corners) as invisible so
    // the shadow is not composited there.
    const SkIRect central { m.fLeft, m.fTop, w - m.fRight, h - m.fBottom };

    SkRegion invisible;

    if (!central.isEmpty())
    {
        SkIRect vertical { central };
        vertical.inset(MTheme::CSDBorderRadius, 0); // full height, trimmed left/right by the radius
        SkIRect horizontal { central };
        horizontal.inset(0, MTheme::CSDBorderRadius); // full width, trimmed top/bottom by the radius
        invisible.op(vertical, SkRegion::kUnion_Op);
        invisible.op(horizontal, SkRegion::kUnion_Op);
    }

    m_shadow.invisibleRegion = invisible;
}

/* --------------------------------- ShadowNode --------------------------------- */

MShadowDecorations::ShadowNode::ShadowNode(MShadowDecorations *deco) noexcept :
    AKRenderable(RenderableHint::Image),
    m_deco(deco)
{
    SkRegion empty;
    setInputRegion(&empty);
    layout().setPositionType(YGPositionTypeAbsolute);
    layout().setPosition(YGEdgeLeft, 0.f);
    layout().setPosition(YGEdgeTop, 0.f);
    layout().setWidthPercent(100);
    layout().setHeightPercent(100);
    invisibleRegion.setRect(AK_IRECT_INF);
}

void MShadowDecorations::ShadowNode::rebuild() noexcept
{
    auto app { MApp::Get() };
    const SkIRect m { m_deco->margins() };

    const SkISize inner {
        Int32(layout().calculatedWidth()  - m.fLeft - m.fRight),
        Int32(layout().calculatedHeight() - m.fTop  - m.fBottom) };

    m_image = app->theme()->csdShadow(scale(), inner, m_deco->m_radius, m_deco->m_offset.x(), m_deco->m_offset.y(), m_clampSides);

    m_deco->updateInvisibleRegion();
    addDamage(AK_IRECT_INF);
}

void MShadowDecorations::ShadowNode::layoutEvent(const CZLayoutEvent &e)
{
    if (!e.changes.has(CZLayoutChangeSize | CZLayoutChangeScale))
        return;

    if (e.changes.has(CHLayoutScale))
    {
        auto app { MApp::Get() };
        for (int i = 0; i < 4; i++)
            m_deco->m_cornerRadius[i].setImage(app->theme()->csdBorderRadiusMask(scale()));
    }

    rebuild();
}

void MShadowDecorations::ShadowNode::renderEvent(const AKRenderEvent &e)
{
    if (!m_deco->surface() || !m_image || e.damage.isEmpty())
        return;

    const SkIRect m { m_deco->margins() };
    const Int32 BR { MTheme::CSDBorderRadius };

    const Int32 w { e.rect.width() };
    const Int32 h { e.rect.height() };

    // Exclude the opaque content centre (minus the rounded corners) from what we paint.
    SkIRect centerV { SkIRect::MakeLTRB(m.fLeft, m.fTop, w - m.fRight, h - m.fBottom) };
    SkIRect centerH { centerV };
    centerV.inset(BR, 1);
    centerH.inset(1, BR);

    SkRegion maskedDamage { e.damage };
    maskedDamage.op(centerV, SkRegion::Op::kDifference_Op);
    maskedDamage.op(centerH, SkRegion::Op::kDifference_Op);

    if (maskedDamage.isEmpty())
        return;

    auto *p { e.pass->getPainter() };

    RDrawImageInfo info {};
    info.image = m_image;
    info.srcScale = scale();
    info.srcTransform = CZTransform::Normal;

    auto blit = [&](const SkIRect &dst, const SkIRect &src)
    {
        if (dst.isEmpty() || src.isEmpty())
            return;
        info.dst = dst;
        info.src = SkRect::Make(src);
        p->drawImage(info, &maskedDamage);
    };

    if (m_clampSides.get() == 0)
    {
        // Not clamped: the image is the shadow at its real size; blit it 1:1 over the node.
        blit(SkIRect::MakeWH(w, h), SkIRect::MakeWH(w, h));
        return;
    }

    // Clamped: 9-slice the minimal image. Corner extents cover the shadow margin, the rounded
    // corner (BR) and the blur transition (~radius) so the stretched 1px middle strip is sampled
    // from the fully-developed straight edge (matching MTheme::csdShadow's minClamp).
    const Int32 rad { m_deco->m_radius };
    const Int32 cLW { m.fLeft   + BR + rad };
    const Int32 cRW { m.fRight  + BR + rad };
    const Int32 cTH { m.fTop    + BR + rad };
    const Int32 cBH { m.fBottom + BR + rad };

    const Int32 imgW { cLW + 1 + cRW }; // == minimal image logical width
    const Int32 imgH { cTH + 1 + cBH };

    const Int32 midW { w - cLW - cRW }; // stretched horizontally
    const Int32 midH { h - cTH - cBH }; // stretched vertically

    /* Corners */
    blit(SkIRect::MakeXYWH(0,       0,       cLW, cTH), SkIRect::MakeXYWH(0,          0,          cLW, cTH)); // TL
    blit(SkIRect::MakeXYWH(w - cRW, 0,       cRW, cTH), SkIRect::MakeXYWH(imgW - cRW, 0,          cRW, cTH)); // TR
    blit(SkIRect::MakeXYWH(0,       h - cBH, cLW, cBH), SkIRect::MakeXYWH(0,          imgH - cBH, cLW, cBH)); // BL
    blit(SkIRect::MakeXYWH(w - cRW, h - cBH, cRW, cBH), SkIRect::MakeXYWH(imgW - cRW, imgH - cBH, cRW, cBH)); // BR

    /* Edges */
    if (midW > 0)
    {
        blit(SkIRect::MakeXYWH(cLW, 0,       midW, cTH), SkIRect::MakeXYWH(cLW, 0,          1, cTH)); // Top
        blit(SkIRect::MakeXYWH(cLW, h - cBH, midW, cBH), SkIRect::MakeXYWH(cLW, imgH - cBH, 1, cBH)); // Bottom
    }
    if (midH > 0)
    {
        blit(SkIRect::MakeXYWH(0,       cTH, cLW, midH), SkIRect::MakeXYWH(0,          cTH, cLW, 1)); // Left
        blit(SkIRect::MakeXYWH(w - cRW, cTH, cRW, midH), SkIRect::MakeXYWH(imgW - cRW, cTH, cRW, 1)); // Right
    }
}

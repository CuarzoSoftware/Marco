#include "CZ/Marco/MLog.h"
#include <CZ/Core/Events/CZWindowStateEvent.h>
#include <CZ/AK/AKLog.h>
#include <CZ/Marco/Private/MSurfacePrivate.h>
#include <CZ/Marco/Nodes/MShadowDecorations.h>
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

void MShadowDecorations::windowStateEvent(const CZWindowStateEvent &event)
{
    MDecorations::windowStateEvent(event);

    if (event.changes.has(CZWindowState::CZWinActivated))
        updateMargins();
}

void MShadowDecorations::updateMargins() noexcept
{
    auto app { MApp::Get() };

    // Latch the activation state the margins are computed from, so the shadow slicing uses the
    // exact same state (see m_active) instead of a live activated() read that can disagree.
    m_active = activated();

    bool changed;

    if (m_active)
        changed = setMargins(SkIRect {
                   app->theme()->CSDShadowActiveRadius,
                   app->theme()->CSDShadowActiveRadius - app->theme()->CSDShadowActiveOffsetY,
                   app->theme()->CSDShadowActiveRadius,
                   app->theme()->CSDShadowActiveRadius + app->theme()->CSDShadowActiveOffsetY });
    else
        changed = setMargins(SkIRect {
                   app->theme()->CSDShadowInactiveRadius,
                   app->theme()->CSDShadowInactiveRadius - app->theme()->CSDShadowInactiveOffsetY,
                   app->theme()->CSDShadowInactiveRadius,
                   app->theme()->CSDShadowInactiveRadius + app->theme()->CSDShadowInactiveOffsetY });

    if (!changed) return;

    const SkIRect m { margins() };

    m_cornerRadius[0].layout().setPosition(YGEdgeLeft, m.fLeft);
    m_cornerRadius[0].layout().setPosition(YGEdgeTop, m.fTop);
    m_cornerRadius[1].layout().setPosition(YGEdgeRight, m.fRight);
    m_cornerRadius[1].layout().setPosition(YGEdgeTop, m.fTop);
    m_cornerRadius[2].layout().setPosition(YGEdgeRight, m.fRight);
    m_cornerRadius[2].layout().setPosition(YGEdgeBottom, m.fBottom);
    m_cornerRadius[3].layout().setPosition(YGEdgeLeft, m.fLeft);
    m_cornerRadius[3].layout().setPosition(YGEdgeBottom, m.fBottom);

    updateInvisibleRegion();
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

void MShadowDecorations::ShadowNode::layoutEvent(const CZLayoutEvent &e)
{
    if (!e.changes.has(CZLayoutChangeSize | CZLayoutChangeScale))
        return;

    auto app { MApp::Get() };

    if (e.changes.has(CHLayoutScale))
    {
        for (int i = 0; i < 4; i++)
            m_deco->m_cornerRadius[i].setImage(app->theme()->csdBorderRadiusMask(scale()));
    }

    const SkIRect m { m_deco->margins() };

    const SkISize inner {
        Int32(layout().calculatedWidth()  - m.fLeft - m.fRight),
        Int32(layout().calculatedHeight() - m.fTop  - m.fBottom) };

    m_image = m_deco->m_active
        ? app->theme()->csdShadowActive(scale(), inner, m_clampSides)
        : app->theme()->csdShadowInactive(scale(), inner, m_clampSides);

    m_deco->updateInvisibleRegion();

    addDamage(AK_IRECT_INF);
}

void MShadowDecorations::ShadowNode::renderEvent(const AKRenderEvent &e)
{
    if (!m_deco->surface() || !m_image || e.damage.isEmpty())
        return;

    SkIRect rect = e.rect;
    rect.fRight++;

    const SkIRect m { m_deco->margins() };
    SkIRect centerV = SkIRect(
        m.fLeft,
        m.fTop,
        rect.width() - m.fRight,
        rect.height() - m.fBottom);

    SkIRect centerH = centerV;
    centerV.inset(MTheme::CSDBorderRadius, 1);
    centerH.inset(1, MTheme::CSDBorderRadius);

    SkRegion maskedDamage = e.damage;
    maskedDamage.op(centerV, SkRegion::Op::kDifference_Op);
    maskedDamage.op(centerH, SkRegion::Op::kDifference_Op);

    if (maskedDamage.isEmpty())
        return;

    const Int32 halfWidth { rect.width()/2 };

    auto *p { e.pass->getPainter() };

    RDrawImageInfo info {};

    info.image = m_image;
    info.srcScale = scale();

    /* No clamp */
    if (m_clampSides.get() == 0)
    {
        /* Left side */
        info.dst.setWH(halfWidth, rect.height());
        info.src.setWH(halfWidth, rect.height()),
        info.srcTransform = CZTransform::Normal;
        p->drawImage(info, &maskedDamage);

        /* Right mirrored side */
        info.dst.offsetTo(halfWidth, info.dst.y());
        info.srcTransform = CZTransform::Flipped;
        p->drawImage(info, &maskedDamage);
    }
    else
    {
        const Int32 B { (m_deco->m_active ? MTheme::CSDShadowActiveOffsetY : MTheme::CSDShadowInactiveOffsetY) + 1  };
        const Int32 T { (m_deco->m_active ?
            (MTheme::CSDShadowActiveRadius - MTheme::CSDShadowActiveOffsetY) :
            (MTheme::CSDShadowInactiveRadius - MTheme::CSDShadowInactiveOffsetY)) + (2 * MTheme::CSDBorderRadius) + 1 };
        const Int32 L { m.fLeft + 2 };

        /* TOP LEFT */
        info.dst.setWH(m.fLeft + L, m.fTop  + T);
        info.src.setWH(info.dst.width(), info.dst.height()),
        info.srcTransform = CZTransform::Normal;
        p->drawImage(info, &maskedDamage);

        /* TOP RIGHT */
        info.dst.offsetTo(rect.width() - m.fLeft - L, 0);
        info.srcTransform = CZTransform::Flipped;
        p->drawImage(info, &maskedDamage);

        /* TOP */
        info.dst = SkIRect::MakeXYWH(
            m.fLeft + L,
            0,
            rect.width() - 2 * ( m.fLeft + L),
            info.dst.height());
        info.src.setXYWH(info.dst.x(), 0, 1, info.dst.height()),
        info.srcTransform = CZTransform::Normal;
        p->drawImage(info, &maskedDamage);

        /* LEFT */
        info.dst = SkIRect::MakeXYWH(
            0,
            m.fTop + T,
            m.fLeft + L,
            rect.height() - m.fTop - m.fBottom - T - B);
        info.src.setXYWH(0, info.dst.y(), info.dst.width(), 0.5);
        info.srcTransform = CZTransform::Normal;
        p->drawImage(info, &maskedDamage);

        /* RIGHT */
        info.dst.offsetTo(rect.width() - m.fLeft - L, info.dst.top());
        info.srcTransform = CZTransform::Flipped;
        p->drawImage(info, &maskedDamage);

        /* BOTTOM LEFT */
        const Int32 bottom { m_image->size().height()/scale() - m.bottom() - B };
        info.dst = SkIRect::MakeXYWH(
            0,
            rect.height() - m.fBottom - B,
            info.dst.width(),
            m.fBottom + B);
        info.src.setXYWH(0, bottom, info.dst.width(), info.dst.height());
        info.srcTransform = CZTransform::Normal;
        p->drawImage(info, &maskedDamage);

        /* BOTTOM RIGHT */
        info.dst.offsetTo(rect.width() - m.fLeft - L, info.dst.top());
        info.src.setXYWH(0, bottom, info.dst.width(), info.dst.height());
        info.srcTransform = CZTransform::Flipped;
        p->drawImage(info, &maskedDamage);

        /* BOTTOM */
        info.dst = SkIRect::MakeXYWH(
            m.fLeft + L,
            info.dst.top(),
            rect.width() - (m.fLeft  + L) * 2,
            info.dst.height());
        info.src.setXYWH(info.dst.x(), bottom, info.dst.width(), info.dst.height());
        info.srcTransform = CZTransform::Normal;
        p->drawImage(info, &maskedDamage);
    }
}

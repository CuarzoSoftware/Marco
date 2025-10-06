#include <CZ/AK/AKLog.h>
#include <CZ/Marco/Private/MSurfacePrivate.h>
#include <CZ/Marco/Nodes/MCSDShadow.h>
#include <CZ/Marco/Roles/MSurface.h>
#include <CZ/Marco/Roles/MToplevel.h>
#include <CZ/Marco/MApp.h>
#include <CZ/AK/Events/AKRenderEvent.h>
#include <CZ/Events/CZLayoutEvent.h>
#include <CZ/Ream/RImage.h>
#include <CZ/Ream/RPainter.h>
#include <CZ/Ream/RPass.h>

using namespace CZ;

MCSDShadow::MCSDShadow(MToplevel *toplevel) noexcept :
    AKRenderable(RenderableHint::Image, &toplevel->MSurface::imp()->root),
    m_toplevel(toplevel)
{
    SkRegion reg;
    setInputRegion(&reg);
    layout().setPositionType(YGPositionTypeAbsolute);
    layout().setWidthPercent(100);
    layout().setHeightPercent(100);
}

void MCSDShadow::layoutEvent(const CZLayoutEvent &e)
{
    if (!m_toplevel || !e.changes.has(CZLayoutChangeSize | CZLayoutChangeScale))
        return;

    auto app { MApp::Get() };

    if (m_toplevel->activated())
    {
        m_image = app->theme()->csdShadowActive(
            scale(),
            SkISize(layout().calculatedWidth() -  m_toplevel->builtinDecorationMargins().fLeft - m_toplevel->builtinDecorationMargins().fRight,
                    layout().calculatedHeight() - m_toplevel->builtinDecorationMargins().fTop - m_toplevel->builtinDecorationMargins().fBottom),
            m_clampSides);
    }
    else
    {
        m_image = app->theme()->csdShadowInactive(
            scale(),
            SkISize(layout().calculatedWidth() -  m_toplevel->builtinDecorationMargins().fLeft - m_toplevel->builtinDecorationMargins().fRight,
                    layout().calculatedHeight() - m_toplevel->builtinDecorationMargins().fTop - m_toplevel->builtinDecorationMargins().fBottom),
            m_clampSides);
    }

    addDamage(AK_IRECT_INF);
}

void MCSDShadow::renderEvent(const AKRenderEvent &e)
{
    if (!m_toplevel || !m_image || e.damage.isEmpty())
        return;

    /* Remove invisible region */

    SkIRect rect = e.rect;
    rect.fRight++;

    const auto &margins { m_toplevel->builtinDecorationMargins() };
    SkIRect centerV = SkIRect(
        margins.fLeft,
        margins.fTop,
        rect.width() - margins.fRight,
        rect.height() - margins.fBottom);

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
        const Int32 B { (activated() ? MTheme::CSDShadowActiveOffsetY : MTheme::CSDShadowInactiveOffsetY) + 1  };
        const Int32 T { (activated() ?
            (MTheme::CSDShadowActiveRadius - MTheme::CSDShadowActiveOffsetY) :
            (MTheme::CSDShadowInactiveRadius - MTheme::CSDShadowInactiveOffsetY)) + (2 * MTheme::CSDBorderRadius) + 1 };
        const Int32 L { margins.fLeft + 2 };

        /* TOP LEFT */
        info.dst.setWH(margins.fLeft + L, margins.fTop  + T);
        info.src.setWH(info.dst.width(), info.dst.height()),
        info.srcTransform = CZTransform::Normal;
        p->drawImage(info, &maskedDamage);

        /* TOP RIGHT */
        info.dst.offsetTo(rect.width() - margins.fLeft - L, 0);
        info.srcTransform = CZTransform::Flipped;
        p->drawImage(info, &maskedDamage);

        /* TOP */
        info.dst = SkIRect::MakeXYWH(
            margins.fLeft + L,
            0,
            rect.width() - 2 * ( margins.fLeft + L),
            info.dst.height());
        info.src.setXYWH(info.dst.x(), 0, 1, info.dst.height()),
        info.srcTransform = CZTransform::Normal;
        p->drawImage(info, &maskedDamage);

        /* LEFT */
        info.dst = SkIRect::MakeXYWH(
            0,
            margins.fTop + T,
            margins.fLeft + L,
            rect.height() - margins.fTop - margins.fBottom - T - B);
        info.src.setXYWH(0, info.dst.y(), info.dst.width(), 0.5);
        info.srcTransform = CZTransform::Normal;
        p->drawImage(info, &maskedDamage);

        /* RIGHT */
        info.dst.offsetTo(rect.width() - margins.fLeft - L, info.dst.top());
        info.srcTransform = CZTransform::Flipped;
        p->drawImage(info, &maskedDamage);

        /* BOTTOM LEFT */
        const Int32 bottom { m_image->size().height()/scale() - margins.bottom() - B };
        info.dst = SkIRect::MakeXYWH(
            0,
            rect.height() - margins.fBottom - B,
            info.dst.width(),
            margins.fBottom + B);
        info.src.setXYWH(0, bottom, info.dst.width(), info.dst.height());
        info.srcTransform = CZTransform::Normal;
        p->drawImage(info, &maskedDamage);

        /* BOTTOM RIGHT */
        info.dst.offsetTo(rect.width() - margins.fLeft - L, info.dst.top());
        info.src.setXYWH(0, bottom, info.dst.width(), info.dst.height());
        info.srcTransform = CZTransform::Flipped;
        p->drawImage(info, &maskedDamage);

        /* BOTTOM */
        info.dst = SkIRect::MakeXYWH(
            margins.fLeft + L,
            info.dst.top(),
            rect.width() - (margins.fLeft  + L) * 2,
            info.dst.height());
        info.src.setXYWH(info.dst.x(), bottom, info.dst.width(), info.dst.height());
        info.srcTransform = CZTransform::Normal;
        p->drawImage(info, &maskedDamage);
    }
}

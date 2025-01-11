#include <Marco/roles/MToplevel.h>
#include <Marco/nodes/MWindowShadow.h>
#include <Marco/MApplication.h>
#include <Marco/MTheme.h>
#include <AK/AKSurface.h>

#include <include/core/SkCanvas.h>
#include <include/effects/SkGradientShader.h>

using namespace AK;
using namespace Marco;

MWindowShadow::MWindowShadow(MToplevel *toplevel) noexcept : m_toplevel(toplevel) {
    SkRegion r;
    setInputRegion(&r);
}

void MWindowShadow::onBake(OnBakeParams *params)
{
    if (!m_toplevel || (params->damage->isEmpty() && m_size == rect().size()))
        return;
    m_size = rect().size();
    SkCanvas *c = params->surface->surface()->getCanvas();
    c->clear(SK_ColorTRANSPARENT);

    /* Shadow */
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setBlendMode(SkBlendMode::kSrc);
    paint.setImageFilter(SkImageFilters::DropShadowOnly(0, 16, 48.f/3.f, 48.f/3.f, 0x69000000, nullptr));
    SkRect rrect = SkRect(
        m_toplevel->csdMargins().left(),
        m_toplevel->csdMargins().top(),
        rect().width() - m_toplevel->csdMargins().right(),
        rect().height() - m_toplevel->csdMargins().bottom());
    c->drawRoundRect(
        rrect,
        app()->theme()->CSDBorderRadius,
        app()->theme()->CSDBorderRadius,
        paint);

    /* Black border */
    paint.setImageFilter(nullptr);
    paint.setAntiAlias(true);
    paint.setStrokeWidth(1.f);
    paint.setColor(0x33000000);
    paint.setStroke(true);
    paint.setBlendMode(SkBlendMode::kSrcOver);
    c->drawRoundRect(
        rrect,
        app()->theme()->CSDBorderRadius,
        app()->theme()->CSDBorderRadius,
        paint);

    /* Clear center */
    paint.setStroke(false);
    paint.setBlendMode(SkBlendMode::kClear);
    c->drawRoundRect(
        rrect,
        app()->theme()->CSDBorderRadius,
        app()->theme()->CSDBorderRadius,
        paint);

    /* White border */

    paint.setStrokeWidth(0.25f);
    paint.setColor(0xFFFFFFFF);
    paint.setStroke(true);
    paint.setBlendMode(SkBlendMode::kSrcOver);

    SkPoint gPoints[2] { SkPoint(0, rrect.fTop), SkPoint(0, rrect.fTop + app()->theme()->CSDBorderRadius * 0.5f)};
    SkColor gColors[2] { 0xFAFFFFFF, 0x00FFFFFF };
    SkScalar gPos[2] { 0.f, 1.f };
    paint.setShader(SkGradientShader::MakeLinear(gPoints, gColors, gPos, 2, SkTileMode::kClamp));

    c->save();
    c->clipRect(SkRect(
        rrect.fLeft, rrect.fTop - 1, rrect.fRight, rrect.fTop + app()->theme()->CSDBorderRadius));

    rrect.inset(0.5f, 0.5f);
    c->drawRoundRect(
        rrect,
        app()->theme()->CSDBorderRadius,
        app()->theme()->CSDBorderRadius,
        paint);

    c->restore();
}

void MWindowShadow::onRender(AK::AKPainter *painter, const SkRegion &damage)
{
    SkIRect centerV = SkIRect(
        m_toplevel->csdMargins().left(),
        m_toplevel->csdMargins().top(),
        rect().width() - m_toplevel->csdMargins().right(),
        rect().height() - m_toplevel->csdMargins().bottom());

    SkIRect centerH = centerV;
    centerV.inset(app()->theme()->CSDBorderRadius, 1);
    centerH.inset(1, app()->theme()->CSDBorderRadius);

    SkRegion finalDamage = damage;
    finalDamage.op(centerV, SkRegion::Op::kDifference_Op);
    finalDamage.op(centerH, SkRegion::Op::kDifference_Op);
    AKBakeable::onRender(painter, finalDamage);
}

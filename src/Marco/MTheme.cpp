#include <Marco/MTheme.h>
#include <AK/AKTarget.h>
#include <AK/AKSurface.h>

#include <include/effects/SkGradientShader.h>
#include <include/effects/SkImageFilters.h>
#include <include/gpu/GrDirectContext.h>
#include <include/core/SkCanvas.h>

#include <iostream>

using namespace Marco;
using namespace AK;

MTheme::MTheme() noexcept {}

sk_sp<SkImage> MTheme::csdBorderRadiusMask(AKTarget *target) noexcept
{
    auto it = m_csdBorderRadiusMask.find(target->bakedComponentsScale());

    if (it != m_csdBorderRadiusMask.end())
        return it->second;

    auto surface = AKSurface::Make(target->surface()->recordingContext(),
                                SkSize::Make(CSDBorderRadius, CSDBorderRadius),
                                target->bakedComponentsScale());

    target->surface()->recordingContext()->asDirectContext()->resetContext();
    SkCanvas &c { *surface->surface()->getCanvas() };
    c.scale(surface->scale(), surface->scale());
    c.clear(SK_ColorTRANSPARENT);

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SK_ColorBLACK);
    c.drawCircle(SkPoint::Make(CSDBorderRadius, CSDBorderRadius), CSDBorderRadius, paint);
    surface->surface()->flush();

    sk_sp<SkImage> result { surface->releaseImage() };
    m_csdBorderRadiusMask[target->bakedComponentsScale()] = result;
    return result;
}

sk_sp<SkImage> MTheme::csdShadowActive(AK::AKTarget *target, const SkISize &winSize, AK::AKBitset<ShadowClamp> &sides) noexcept
{
    if (winSize.isEmpty())
    {
        sides.set(0);
        return sk_sp<SkImage>();
    }

    SkISize windowSize { winSize };

    if (windowSize.width() < 72 || windowSize.height() < 140)
        sides.set(0);
    else
    {
        sides.set(ShadowClampX | ShadowClampY);

        auto it = m_csdShadowActive.find(target->bakedComponentsScale());

        if (it != m_csdShadowActive.end())
            return it->second;

        windowSize = { 73, 141 };
    }

    const SkSize surfaceSize(
        Float32(windowSize.width()) * 0.5f + Float32(CSDShadowActiveMargins.fLeft),
        windowSize.height() + CSDShadowActiveMargins.fTop + CSDShadowActiveMargins.fBottom);
    auto surface = AKSurface::Make(target->surface()->recordingContext(), surfaceSize, target->bakedComponentsScale());
    target->surface()->recordingContext()->asDirectContext()->resetContext();
    SkCanvas *c = surface->surface()->getCanvas();
    c->clear(SK_ColorTRANSPARENT);
    c->scale(surface->scale(), surface->scale());

    /* Shadow */
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setBlendMode(SkBlendMode::kSrc);
    paint.setImageFilter(SkImageFilters::DropShadowOnly(0, 16, 48.f/3.f, 48.f/3.f, 0x69000000, nullptr));
    SkRect rrect = SkRect::MakeXYWH(
        CSDShadowActiveMargins.left(),
        CSDShadowActiveMargins.top(),
        windowSize.width(),
        windowSize.height());
    c->drawRoundRect(
        rrect,
        CSDBorderRadius,
        CSDBorderRadius,
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
        CSDBorderRadius,
        CSDBorderRadius,
        paint);

    /* Clear center */
    paint.setStroke(false);
    paint.setBlendMode(SkBlendMode::kClear);
    c->drawRoundRect(
        rrect,
        CSDBorderRadius,
        CSDBorderRadius,
        paint);

    /* White border */

    paint.setStrokeWidth(0.25f);
    paint.setColor(0xFFFFFFFF);
    paint.setStroke(true);
    paint.setBlendMode(SkBlendMode::kSrcOver);

    SkPoint gPoints[2] { SkPoint(0, rrect.fTop), SkPoint(0, rrect.fTop + CSDBorderRadius * 0.5f)};
    SkColor gColors[2] { 0xFAFFFFFF, 0x00FFFFFF };
    SkScalar gPos[2] { 0.f, 1.f };
    paint.setShader(SkGradientShader::MakeLinear(gPoints, gColors, gPos, 2, SkTileMode::kClamp));

    c->save();
    c->clipRect(SkRect(
        rrect.fLeft, rrect.fTop - 1, rrect.fRight, rrect.fTop + CSDBorderRadius));

    rrect.inset(0.5f, 0.5f);
    c->drawRoundRect(
        rrect,
        CSDBorderRadius,
        CSDBorderRadius,
        paint);

    c->restore();
    surface->surface()->flush();

    if (sides.get() == 0)
        return sk_sp<SkImage>(surface->releaseImage());
    else
    {
        sk_sp<SkImage> result { surface->releaseImage() };
        m_csdShadowActive[target->bakedComponentsScale()] = result;
        return result;
    }
}

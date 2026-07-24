#include <CZ/Marco/MTheme.h>
#include <CZ/AK/AKTarget.h>
#include <CZ/Ream/RSurface.h>
#include <CZ/Ream/RPass.h>

#include <CZ/skia/gpu/ganesh/GrDirectContext.h>
#include <CZ/skia/effects/SkGradientShader.h>
#include <CZ/skia/effects/SkImageFilters.h>
#include <CZ/skia/core/SkCanvas.h>

using namespace CZ;

MTheme::MTheme() noexcept : AKTheme() {}

std::shared_ptr<RImage> MTheme::csdBorderRadiusMask(Int32 scale) noexcept
{
    auto it = m_csdBorderRadiusMask.find(scale);

    if (it != m_csdBorderRadiusMask.end())
        return it->second;

    auto surface = RSurface::Make(
        SkISize::Make(CSDBorderRadius, CSDBorderRadius),
        scale,
        true);

    auto pass { surface->beginPass(RPassCap_SkCanvas) };
    SkCanvas &c { *pass->getCanvas() };

    c.clear(SK_ColorTRANSPARENT);

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SK_ColorBLACK);
    c.drawCircle(SkPoint::Make(CSDBorderRadius, CSDBorderRadius), CSDBorderRadius, paint);
    pass.reset();

    std::shared_ptr<RImage> result { surface->image() };
    m_csdBorderRadiusMask[scale] = result;
    return result;
}

std::shared_ptr<RImage> MTheme::csdShadow(Int32 scale, const SkISize &innerSize, Int32 radius, Int32 offsetX, Int32 offsetY, CZ::CZBitset<ShadowClamp> &sides) noexcept
{
    if (innerSize.isEmpty() || radius <= 0)
    {
        sides.set(0);
        return std::shared_ptr<RImage>();
    }

    // Margins the shadow reserves around the content. A positive offset shifts the shadow towards
    // that edge, leaving less room on the opposite one. L + R and T + B always sum to 2 * radius.
    const Int32 L { std::max(0, radius - offsetX) };
    const Int32 T { std::max(0, radius - offsetY) };
    const Int32 R { std::max(0, radius + offsetX) };
    const Int32 B { std::max(0, radius + offsetY) };

    // Smallest content that still yields a correct 9-slice. The stretched middle samples a 1px
    // strip of the *straight* edge, which is only fully developed past the rounded corner (radius
    // BR) plus the blur transition (~radius). If the clamp were smaller the sampled strip would sit
    // in the corner falloff and the stretched shadow would look lighter than the real one.
    const Int32 minSide { 2 * (radius + CSDBorderRadius) + 1 };
    const SkISize minClamp { minSide, minSide };

    SkISize content { innerSize };

    if (content.width() < minClamp.width() || content.height() < minClamp.height())
        sides.set(0); // too small to clamp: render the shadow at its real size
    else
    {
        sides.set(ShadowClampX | ShadowClampY);

        const auto key { std::make_tuple(scale, radius, offsetX, offsetY) };
        auto it = m_csdShadow.find(key);

        if (it != m_csdShadow.end())
            return it->second;

        content = minClamp; // minimal representative image; the renderer stretches the middle
    }

    const SkISize surfaceSize(content.width() + L + R, content.height() + T + B);
    auto surface = RSurface::Make(surfaceSize, scale, true);
    auto pass { surface->beginPass(RPassCap_SkCanvas) };
    SkCanvas *c { pass->getCanvas() };

    c->clear(SK_ColorTRANSPARENT);

    SkRect rrect = SkRect::MakeXYWH(L, T, content.width(), content.height());

    /* Shadow */
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setBlendMode(SkBlendMode::kSrc);
    paint.setImageFilter(SkImageFilters::DropShadowOnly(offsetX, offsetY, Float32(radius)/3.f, Float32(radius)/3.f, 0x69000000, nullptr));
    c->drawRoundRect(rrect, CSDBorderRadius, CSDBorderRadius, paint);

    /* Black border */
    paint.setImageFilter(nullptr);
    paint.setStrokeWidth(1.f);
    paint.setColor(0x33000000);
    paint.setStroke(true);
    paint.setBlendMode(SkBlendMode::kSrcOver);
    c->drawRoundRect(rrect, CSDBorderRadius, CSDBorderRadius, paint);

    /* Clear center */
    paint.setStroke(false);
    paint.setBlendMode(SkBlendMode::kClear);
    c->drawRoundRect(rrect, CSDBorderRadius, CSDBorderRadius, paint);

    /* White top border */
    paint.setStrokeWidth(0.25f);
    paint.setColor(0xFFFFFFFF);
    paint.setStroke(true);
    paint.setBlendMode(SkBlendMode::kSrcOver);

    SkPoint gPoints[2] { SkPoint(0, rrect.fTop), SkPoint(0, rrect.fTop + CSDBorderRadius * 0.5f)};
    SkColor gColors[2] { 0xFAFFFFFF, 0x00FFFFFF };
    SkScalar gPos[2] { 0.f, 1.f };
    paint.setShader(SkGradientShader::MakeLinear(gPoints, gColors, gPos, 2, SkTileMode::kClamp));

    c->save();
    c->clipRect(SkRect(rrect.fLeft, rrect.fTop - 1, rrect.fRight, rrect.fTop + CSDBorderRadius));
    rrect.inset(0.5f, 0.5f);
    c->drawRoundRect(rrect, CSDBorderRadius, CSDBorderRadius, paint);
    c->restore();
    pass.reset();

    std::shared_ptr<RImage> result { surface->image() };

    if (sides.get() != 0)
        m_csdShadow[std::make_tuple(scale, radius, offsetX, offsetY)] = result;

    return result;
}

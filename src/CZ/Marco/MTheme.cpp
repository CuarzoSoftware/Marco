#include <CZ/Marco/MTheme.h>
#include <CZ/AK/AKTarget.h>
#include <CZ/Ream/RSurface.h>
#include <CZ/Ream/RPass.h>

#include <CZ/skia/gpu/ganesh/GrDirectContext.h>
#include <CZ/skia/effects/SkGradientShader.h>
#include <CZ/skia/effects/SkImageFilters.h>
#include <CZ/skia/core/SkCanvas.h>
#include <CZ/skia/core/SkRRect.h>

using namespace CZ;

MTheme::MTheme() noexcept : AKTheme() {}

std::shared_ptr<RImage> MTheme::csdBorderRadiusMask(Int32 scale, Int32 radius) noexcept
{
    if (radius <= 0)
        return std::shared_ptr<RImage>();

    const auto key { std::make_tuple(scale, radius) };
    auto it = m_csdBorderRadiusMask.find(key);

    if (it != m_csdBorderRadiusMask.end())
        return it->second;

    auto surface = RSurface::Make(
        SkISize::Make(radius, radius),
        scale,
        true);

    auto pass { surface->beginPass(RPassCap_SkCanvas) };
    SkCanvas &c { *pass->getCanvas() };

    c.clear(SK_ColorTRANSPARENT);

    // A quarter disk in the corner opposite (0,0): as a DstIn mask it keeps the rounded content and
    // erases the square corner. The node rotates a single image to cover all four corners.
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SK_ColorBLACK);
    c.drawCircle(SkPoint::Make(radius, radius), radius, paint);
    pass.reset();

    std::shared_ptr<RImage> result { surface->image() };
    m_csdBorderRadiusMask[key] = result;
    return result;
}

std::shared_ptr<RImage> MTheme::csdShadow(Int32 scale, const SkISize &innerSize, Int32 radius, Int32 offsetX, Int32 offsetY, const CZBorderRadius &corners, CZ::CZBitset<ShadowClamp> &sides) noexcept
{
    if (innerSize.isEmpty() || radius <= 0)
    {
        sides.set(0);
        return std::shared_ptr<RImage>();
    }

    const Int32 rTL { std::max(0, corners.fTL) };
    const Int32 rTR { std::max(0, corners.fTR) };
    const Int32 rBR { std::max(0, corners.fBR) };
    const Int32 rBL { std::max(0, corners.fBL) };

    // Per-side corner extent: a side's 9-slice column/row must fit both of its corners.
    const Int32 maxL { std::max(rTL, rBL) };
    const Int32 maxR { std::max(rTR, rBR) };
    const Int32 maxT { std::max(rTL, rTR) };
    const Int32 maxB { std::max(rBL, rBR) };

    // Margins the shadow reserves around the content. A positive offset shifts the shadow towards
    // that edge, leaving less room on the opposite one. L + R and T + B always sum to 2 * radius.
    const Int32 L { std::max(0, radius - offsetX) };
    const Int32 T { std::max(0, radius - offsetY) };
    const Int32 R { std::max(0, radius + offsetX) };
    const Int32 B { std::max(0, radius + offsetY) };

    // Smallest content that still yields a correct 9-slice. The stretched middle samples a 1px strip
    // of the *straight* edge, which is only fully developed past each side's rounded corner plus the
    // blur transition (~radius). Must match the renderer's corner extents (mL+maxL+radius, ...).
    const SkISize minClamp {
        maxL + maxR + 2 * radius + 1,
        maxT + maxB + 2 * radius + 1
    };

    SkISize content { innerSize };

    if (content.width() < minClamp.width() || content.height() < minClamp.height())
        sides.set(0); // too small to clamp: render the shadow at its real size
    else
    {
        sides.set(ShadowClampX | ShadowClampY);

        const auto key { std::make_tuple(scale, radius, offsetX, offsetY, rTL, rTR, rBR, rBL) };
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

    const SkRect rect { SkRect::MakeXYWH(L, T, content.width(), content.height()) };
    SkVector radii[4] {
        { Float32(rTL), Float32(rTL) }, // upper-left
        { Float32(rTR), Float32(rTR) }, // upper-right
        { Float32(rBR), Float32(rBR) }, // lower-right
        { Float32(rBL), Float32(rBL) }  // lower-left
    };
    SkRRect rrect;
    rrect.setRectRadii(rect, radii);

    /* Shadow */
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setBlendMode(SkBlendMode::kSrc);
    paint.setImageFilter(SkImageFilters::DropShadowOnly(offsetX, offsetY, Float32(radius)/3.f, Float32(radius)/3.f, 0x69000000, nullptr));
    c->drawRRect(rrect, paint);

    /* Black border */
    paint.setImageFilter(nullptr);
    paint.setStrokeWidth(1.f);
    paint.setColor(0x33000000);
    paint.setStroke(true);
    paint.setBlendMode(SkBlendMode::kSrcOver);
    c->drawRRect(rrect, paint);

    /* Clear center */
    paint.setStroke(false);
    paint.setBlendMode(SkBlendMode::kClear);
    c->drawRRect(rrect, paint);

    /* White top border */
    paint.setStrokeWidth(0.25f);
    paint.setColor(0xFFFFFFFF);
    paint.setStroke(true);
    paint.setBlendMode(SkBlendMode::kSrcOver);

    const Int32 topEdge { std::max(maxT, 1) };
    SkPoint gPoints[2] { SkPoint(0, rect.fTop), SkPoint(0, rect.fTop + topEdge * 0.5f)};
    SkColor gColors[2] { 0xFAFFFFFF, 0x00FFFFFF };
    SkScalar gPos[2] { 0.f, 1.f };
    paint.setShader(SkGradientShader::MakeLinear(gPoints, gColors, gPos, 2, SkTileMode::kClamp));

    c->save();
    c->clipRect(SkRect(rect.fLeft, rect.fTop - 1, rect.fRight, rect.fTop + topEdge));
    SkRRect inner { rrect };
    inner.inset(0.5f, 0.5f);
    c->drawRRect(inner, paint);
    c->restore();
    pass.reset();

    std::shared_ptr<RImage> result { surface->image() };

    if (sides.get() != 0)
        m_csdShadow[std::make_tuple(scale, radius, offsetX, offsetY, rTL, rTR, rBR, rBL)] = result;

    return result;
}

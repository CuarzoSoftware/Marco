#include <Marco/MTheme.h>
#include <AK/AKTarget.h>
#include <AK/AKSurface.h>

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

    std::cout << "AJ" << std::endl;

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

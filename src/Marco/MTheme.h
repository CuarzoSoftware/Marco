#ifndef MTHEME_H
#define MTHEME_H

#include <Marco/Marco.h>
#include <AK/AKTheme.h>
#include <AK/AKBitset.h>

class Marco::MTheme : public AK::AKTheme
{
public:
    MTheme() noexcept;

    /* CSD */

    enum ShadowClamp
    {
        ShadowClampX = 1 << 0,
        ShadowClampY = 1 << 1
    };

    static inline Int32 CSDBorderRadius { 10 };
    constexpr static SkIRect CSDShadowActiveMargins { 48, 30, 48, 66 };
    virtual sk_sp<SkImage> csdBorderRadiusMask(AK::AKTarget *target) noexcept;
    virtual sk_sp<SkImage> csdShadowActive(AK::AKTarget *target, const SkISize &windowSize, AK::AKBitset<ShadowClamp> &sides) noexcept;
protected:
    std::unordered_map<Int32,sk_sp<SkImage>> m_csdBorderRadiusMask;
    std::unordered_map<Int32,sk_sp<SkImage>> m_csdShadowActive;
};

#endif // MTHEME_H

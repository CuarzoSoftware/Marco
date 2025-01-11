#ifndef MTHEME_H
#define MTHEME_H

#include <Marco/Marco.h>
#include <AK/AKTheme.h>

class Marco::MTheme : public AK::AKTheme
{
public:
    MTheme() noexcept;

    /* CSD */

    static inline Int32 CSDBorderRadius { 10 };
    virtual sk_sp<SkImage> csdBorderRadiusMask(AK::AKTarget *target) noexcept;
protected:
    std::unordered_map<Int32,sk_sp<SkImage>> m_csdBorderRadiusMask;
};

#endif // MTHEME_H

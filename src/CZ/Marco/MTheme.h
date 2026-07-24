#ifndef MTHEME_H
#define MTHEME_H

#include <CZ/Marco/Marco.h>
#include <CZ/AK/AKTheme.h>
#include <CZ/Core/CZBitset.h>
#include <map>
#include <tuple>

class CZ::MTheme : public AKTheme
{
public:
    MTheme() noexcept;

    /* CSD */

    enum ShadowClamp
    {
        ShadowClampX = 1 << 0,
        ShadowClampY = 1 << 1
    };

    static inline Int32 CSDMoveOutset { 54 };
    static inline Int32 CSDResizeOutset { 6 };
    static inline Int32 CSDBorderRadius { 10 };
    // Suggested shadow parameters (radius / vertical offset) roles may use, e.g. MToplevel picks
    // active vs inactive based on its activation state. MShadowDecorations itself is agnostic.
    constexpr static Int32 CSDShadowActiveRadius { 48 };
    constexpr static Int32 CSDShadowActiveOffsetY { 18 };
    constexpr static Int32 CSDShadowInactiveRadius { 27 };
    constexpr static Int32 CSDShadowInactiveOffsetY { 8 };
    virtual std::shared_ptr<RImage> csdBorderRadiusMask(Int32 scale) noexcept;

    /**
     * @brief Generates (and caches) a drop-shadow frame for a rounded-rect window.
     *
     * The image is a shadow "frame" with a transparent centre, sized to @p innerSize plus the
     * margins implied by @p radius / @p offsetX / @p offsetY. When @p innerSize is large enough a
     * minimal clampable image is returned and @p sides reports which axes may be 9-slice stretched;
     * otherwise the shadow is rendered at full size and @p sides is cleared.
     */
    virtual std::shared_ptr<RImage> csdShadow(Int32 scale, const SkISize &innerSize, Int32 radius, Int32 offsetX, Int32 offsetY, CZBitset<ShadowClamp> &sides) noexcept;
protected:
    std::unordered_map<Int32,std::shared_ptr<RImage>> m_csdBorderRadiusMask;
    std::map<std::tuple<Int32,Int32,Int32,Int32>,std::shared_ptr<RImage>> m_csdShadow; // key: (scale, radius, offsetX, offsetY)
};

#endif // MTHEME_H

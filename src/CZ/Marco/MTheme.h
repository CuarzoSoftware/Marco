#ifndef MTHEME_H
#define MTHEME_H

#include <CZ/Marco/Marco.h>
#include <CZ/AK/AKTheme.h>
#include <CZ/Core/CZBitset.h>
#include <CZ/Core/CZBorderRadius.h>
#include <map>
#include <tuple>

/**
 * @brief The default Marco theme: AKTheme plus client-side decoration (CSD) parameters and assets.
 *
 * Installed by MApp::Make(); get it with MApp::theme(). Subclass it to customize the CSD look.
 */
class CZ::MTheme : public AKTheme
{
public:
    /** @brief Creates the theme. */
    MTheme() noexcept;

    /* CSD */

    /** @brief Axes along which a csdShadow() image can be stretched (9-slice). */
    enum ShadowClamp
    {
        ShadowClampX = 1 << 0, ///< Horizontally stretchable.
        ShadowClampY = 1 << 1  ///< Vertically stretchable.
    };

    /** @brief Height of the area at the top of an MToplevel where nodes with `UCWindowMove` start a window move. */
    static inline Int32 CSDMoveOutset { 54 };

    /** @brief Longest time between the two presses of a double click on an MToplevel's move area (it toggles maximized), in milliseconds. */
    static inline UInt32 DoubleClickMs { 400 };

    /** @brief Farthest the pointer can move between those two presses, in logical pixels. */
    static inline SkScalar DoubleClickDistance { 4.f };

    /** @brief Width of the area around an MToplevel's edges that starts a resize. */
    static inline Int32 CSDResizeOutset { 6 };

    /** @brief Corner radius of MToplevel windows with client-side decorations. */
    static inline Int32 CSDBorderRadius { 10 };

    // Suggested shadow parameters (radius / vertical offset) roles may use, e.g. MToplevel picks
    // active vs inactive based on its activation state. MShadowDecorations itself is agnostic.

    /** @brief Suggested shadow blur radius of an active window (see MShadowDecorations). */
    constexpr static Int32 CSDShadowActiveRadius { 48 };
    /** @brief Suggested shadow vertical offset of an active window. */
    constexpr static Int32 CSDShadowActiveOffsetY { 18 };
    /** @brief Suggested shadow blur radius of an inactive window. */
    constexpr static Int32 CSDShadowInactiveRadius { 27 };
    /** @brief Suggested shadow vertical offset of an inactive window. */
    constexpr static Int32 CSDShadowInactiveOffsetY { 8 };

    /**
     * @brief Generates (and caches) the mask used to round a surface's corners.
     *
     * A @p radius x @p radius image (at @p scale) with a quarter disk, used as a DstIn mask for a
     * corner and rotated for the other ones.
     *
     * @return The mask, or `nullptr` if @p radius <= 0.
     */
    virtual std::shared_ptr<RImage> csdBorderRadiusMask(Int32 scale, Int32 radius) noexcept;

    /**
     * @brief Generates (and caches) a drop-shadow frame for a rounded-rect window.
     *
     * The image is a shadow "frame" with a transparent centre, sized to @p innerSize plus the
     * margins implied by @p radius / @p offsetX / @p offsetY. When @p innerSize is large enough a
     * minimal clampable image is returned and @p sides reports which axes may be 9-slice stretched;
     * otherwise the shadow is rendered at full size and @p sides is cleared.
     */
    virtual std::shared_ptr<RImage> csdShadow(Int32 scale, const SkISize &innerSize, Int32 radius, Int32 offsetX, Int32 offsetY, const CZBorderRadius &corners, CZBitset<ShadowClamp> &sides) noexcept;
protected:
    /// @cond INTERNAL
    std::map<std::tuple<Int32,Int32>,std::shared_ptr<RImage>> m_csdBorderRadiusMask; // key: (scale, radius)
    // key: (scale, blurRadius, offsetX, offsetY, radTL, radTR, radBR, radBL)
    std::map<std::tuple<Int32,Int32,Int32,Int32,Int32,Int32,Int32,Int32>,std::shared_ptr<RImage>> m_csdShadow;
    /// @endcond
};

#endif // MTHEME_H

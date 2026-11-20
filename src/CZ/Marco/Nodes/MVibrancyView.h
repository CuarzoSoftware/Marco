#ifndef MVIBRANCYVIEW_H
#define MVIBRANCYVIEW_H

#include <CZ/AK/Nodes/AKSolidColor.h>
#include <CZ/Marco/Marco.h>
#include <CZ/Core/CZAdaptive.h>

/**
 * @brief A node whose area shows the surface's background blur (vibrancy).
 *
 * The surface requests background blur for the area of its visible MVibrancyView nodes. While
 * vibrancy is enabled (the compositor supports background blur and it's active, see
 * MSurface::vibrancyState()) the view is transparent so the blur shows through; otherwise it's
 * filled with disabledColor(). Typically used as the background of sidebars and menus.
 */
class CZ::MVibrancyView : public AKSolidColor
{
public:

    /** @brief Creates the view, added to @p parent. */
    MVibrancyView(AKNode *parent = nullptr) noexcept :
        AKSolidColor(0x00000000, parent) {
        applyColor();
    }

    /** @brief Whether vibrancy is currently enabled (the view is transparent). */
    bool vibrancyEnabled() const noexcept { return m_vibrancyEnabled; };

    /**
     * @brief Fill shown while vibrancy is disabled (adaptive; resolved for the current color scheme).
     *
     * Defaults to a light gray in light schemes and a dark gray in dark schemes.
     */
    void setDisabledColor(CZAdaptiveColor color) noexcept;

    /** @brief The fill shown while vibrancy is disabled, see setDisabledColor(). */
    CZAdaptiveColor disabledColor() const noexcept { return m_disabledColor; };

protected:
    using AKSolidColor::setColor;
    bool event(const CZEvent &event) noexcept override;
    /// @cond INTERNAL
    void applyColor() noexcept;
    bool m_vibrancyEnabled { false };
    CZAdaptiveColor m_disabledColor { 0xFFe6e7e7, 0xFF323232 };
    /// @endcond
};

#endif // MVIBRANCYVIEW_H

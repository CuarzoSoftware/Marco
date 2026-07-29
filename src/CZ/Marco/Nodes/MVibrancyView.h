#ifndef MVIBRANCYVIEW_H
#define MVIBRANCYVIEW_H

#include <CZ/AK/Nodes/AKSolidColor.h>
#include <CZ/Marco/Marco.h>
#include <CZ/Core/CZAdaptive.h>

class CZ::MVibrancyView : public AKSolidColor
{
public:

    MVibrancyView(AKNode *parent = nullptr) noexcept :
        AKSolidColor(0x00000000, parent) {
        applyColor();
    }

    bool vibrancyEnabled() const noexcept { return m_vibrancyEnabled; };

    /**
     * @brief Fill shown while vibrancy is disabled (adaptive; resolved for the current color scheme).
     *
     * Defaults to a light gray in light schemes and a dark gray in dark schemes.
     */
    void setDisabledColor(CZAdaptiveColor color) noexcept;
    CZAdaptiveColor disabledColor() const noexcept { return m_disabledColor; };

protected:
    using AKSolidColor::setColor;
    bool event(const CZEvent &event) noexcept override;
    void applyColor() noexcept;
    bool m_vibrancyEnabled { false };
    CZAdaptiveColor m_disabledColor { 0xFFe6e7e7, 0xFF323232 };
};

#endif // MVIBRANCYVIEW_H

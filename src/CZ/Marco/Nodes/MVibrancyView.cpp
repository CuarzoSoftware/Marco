#include <CZ/Marco/MLog.h>
#include <CZ/Marco/Nodes/MVibrancyView.h>
#include <CZ/AK/Events/AKVibrancyEvent.h>

using namespace CZ;

bool MVibrancyView::event(const CZEvent &event) noexcept
{
    if (event.type() == AKVibrancyEvent::Type::Vibrancy)
    {
        const auto &e { static_cast<const AKVibrancyEvent&>(event) };
        m_vibrancyEnabled = (e.state == AKVibrancyState::Enabled);
        MLog(CZFatal, "Vibrancy {}", m_vibrancyEnabled);
        applyColor();
        return true;
    }

    return AKSolidColor::event(event);
}

void MVibrancyView::applyColor() noexcept
{
    // While vibrancy is enabled the fill is transparent so the blur shows through; otherwise it
    // falls back to the disabled color resolved for the current color scheme.
    setColor(m_vibrancyEnabled ? CZAdaptiveColor(0x00000000) : m_disabledColor);
}

void MVibrancyView::setDisabledColor(CZAdaptiveColor color) noexcept
{
    if (m_disabledColor == color)
        return;

    m_disabledColor = color;

    if (!m_vibrancyEnabled)
        applyColor();
}

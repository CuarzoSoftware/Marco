#include <CZ/Marco/Nodes/MVibrancyView.h>
#include <CZ/AK/Events/AKVibrancyEvent.h>

using namespace CZ;

bool MVibrancyView::event(const CZEvent &event) noexcept
{
    if (event.type() == AKVibrancyEvent::Type::Vibrancy)
    {
        const auto &e { static_cast<const AKVibrancyEvent&>(event) };
        if (e.state == AKVibrancyState::Enabled)
            setColor(0x00000000);
        else
            setColor(disabledColor());

        return true;
    }

    return AKSolidColor::event(event);
}

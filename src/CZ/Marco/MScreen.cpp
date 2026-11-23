#include <CZ/Marco/MScreen.h>
#include <CZ/Marco/Protocols/XdgOutput/xdg-output-unstable-v1-client.h>
#include <algorithm>

using namespace CZ;

MScreen::~MScreen()
{
    if (m_xdgOutput)
        zxdg_output_v1_destroy(m_xdgOutput);

    wl_output_destroy(m_proxy);
}

SkISize MScreen::logicalSize() const noexcept
{
    if (m_current.logicalSize)
        return *m_current.logicalSize;

    for (const auto &mode : m_current.modes)
    {
        if (!mode.isCurrent)
            continue;

        const Int32 scale { std::max(1, m_current.scale) };
        const SkISize size { Is90Transform(m_current.transform) ? SkISize::Make(mode.size.height(), mode.size.width()) : mode.size };
        return SkISize::Make(size.width() / scale, size.height() / scale);
    }

    return SkISize::MakeEmpty();
}

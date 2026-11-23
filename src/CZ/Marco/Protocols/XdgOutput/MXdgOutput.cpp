#include <CZ/Marco/Protocols/XdgOutput/MXdgOutput.h>
#include <CZ/Marco/Protocols/Wayland/MWlOutput.h>
#include <CZ/Marco/MScreen.h>
#include <CZ/Marco/MApp.h>

using namespace CZ;

void MXdgOutput::Create(MScreen &screen) noexcept
{
    auto &wl { MApp::Get()->wl };

    if (screen.m_xdgOutput || !wl.xdgOutputManager)
        return;

    screen.m_xdgOutput = zxdg_output_manager_v1_get_xdg_output(wl.xdgOutputManager, screen.wlOutput());
    zxdg_output_v1_add_listener(screen.m_xdgOutput, &Listener, &screen);
}

void MXdgOutput::logical_position(void *data, zxdg_output_v1 *output, Int32 x, Int32 y)
{
    CZ_UNUSED(output)
    MScreen &screen { *static_cast<MScreen*>(data) };
    screen.m_pending.logicalPos = SkIPoint::Make(x, y);
    screen.m_changes.add(MScreen::LogicalPosition);
}

void MXdgOutput::logical_size(void *data, zxdg_output_v1 *output, Int32 width, Int32 height)
{
    CZ_UNUSED(output)
    MScreen &screen { *static_cast<MScreen*>(data) };
    screen.m_pending.logicalSize = SkISize::Make(width, height);
    screen.m_changes.add(MScreen::LogicalSize);
}

void MXdgOutput::done(void *data, zxdg_output_v1 *output)
{
    // Deprecated since version 3 (wl_output.done applies everything): only older ones send it
    if (zxdg_output_v1_get_version(output) < 3)
        MWlOutput::done(data, static_cast<MScreen*>(data)->wlOutput());
}

void MXdgOutput::name(void *data, zxdg_output_v1 *output, const char *name)
{
    // wl_output.name (version 4) also has it
    CZ_UNUSED(data) CZ_UNUSED(output) CZ_UNUSED(name)
}

void MXdgOutput::description(void *data, zxdg_output_v1 *output, const char *description)
{
    CZ_UNUSED(data) CZ_UNUSED(output) CZ_UNUSED(description)
}

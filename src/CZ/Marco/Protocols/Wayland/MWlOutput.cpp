#include <CZ/Marco/Protocols/Wayland/MWlOutput.h>
#include <CZ/Ream/WL/RWLSubpixel.h>
#include <CZ/Marco/MScreen.h>
#include <CZ/Marco/MApp.h>

using namespace CZ;

void MWlOutput::geometry(void *data, wl_output *output, Int32 x, Int32 y, Int32 physicalWidth, Int32 physicalHeight, Int32 subpixel, const char *make, const char *model, Int32 transform)
{
    CZ_UNUSED(output)
    MScreen &screen { *static_cast<MScreen*>(data) };
    screen.m_pending.pos.fX = x;
    screen.m_pending.pos.fY = y;
    screen.m_pending.physicalSize.fWidth = physicalWidth;
    screen.m_pending.physicalSize.fHeight = physicalHeight;
    screen.m_pending.pixelGeometry = RWLSubpixel::ToReam((wl_output_subpixel)subpixel);
    screen.m_pending.make = make;
    screen.m_pending.model = model;
    screen.m_pending.transform = static_cast<CZ::CZTransform>(transform);
    screen.m_changes.add(MScreen::Position | MScreen::PhysicalSize | MScreen::PixelGeometry | MScreen::Make | MScreen::Model | MScreen::Transform);
}

void MWlOutput::mode(void *data, wl_output *output, UInt32 flags, Int32 width, Int32 height, Int32 refresh)
{
    CZ_UNUSED(output)
    MScreen &screen { *static_cast<MScreen*>(data) };
    screen.m_pending.modes.emplace_back(SkISize::Make(width, height), refresh, bool(flags & WL_OUTPUT_MODE_CURRENT), bool(flags & WL_OUTPUT_MODE_PREFERRED));
    screen.m_changes.add(MScreen::Modes);
}

void MWlOutput::done(void *data, wl_output *output)
{
    CZ_UNUSED(output)
    auto app { MApp::Get() };
    MScreen &screen { *static_cast<MScreen*>(data) };
    screen.m_current = screen.m_pending;

    if (screen.m_pendingFirstDone)
    {
        screen.m_pendingFirstDone = false;

        for (size_t i = 0; i < app->m_pendingScreens.size(); i++)
        {
            if (app->m_pendingScreens[i] == &screen)
            {
                app->m_pendingScreens[i] = app->m_pendingScreens.back();
                app->m_pendingScreens.pop_back();
                break;
            }
        }

        app->m_screens.push_back(&screen);

        if (app->m_running)
            app->onScreenPlugged.notify(screen);
    }
    else if (app->m_running)
        screen.onPropsChanged.notify(screen, screen.m_changes);

    screen.m_changes.set(0);
}

void MWlOutput::scale(void *data, wl_output *output, Int32 factor)
{
    CZ_UNUSED(output)
    MScreen &screen { *static_cast<MScreen*>(data) };
    screen.m_pending.scale = factor;
    screen.m_changes.add(MScreen::Scale);
}

void MWlOutput::name(void *data, wl_output *output, const char *name)
{
    CZ_UNUSED(output)
    MScreen &screen { *static_cast<MScreen*>(data) };
    screen.m_pending.name = name;
    screen.m_changes.add(MScreen::Name);
}

void MWlOutput::description(void *data, wl_output *output, const char *description)
{
    CZ_UNUSED(output)
    MScreen &screen { *static_cast<MScreen*>(data) };
    screen.m_pending.description = description;
    screen.m_changes.add(MScreen::Description);
}

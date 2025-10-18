#include <CZ/Marco/Extensions/MForeignToplevel.h>
#include <CZ/Marco/Roles/MSurface.h>
#include <CZ/Marco/MApp.h>

using namespace CZ;

void MForeignToplevel::setMaximized(bool maximized) noexcept
{
    if (maximized)
        zwlr_foreign_toplevel_handle_v1_set_maximized(handle());
    else
        zwlr_foreign_toplevel_handle_v1_unset_maximized(handle());
}

void MForeignToplevel::setMinimized(bool minimized) noexcept
{
    if (minimized)
        zwlr_foreign_toplevel_handle_v1_set_minimized(handle());
    else
        zwlr_foreign_toplevel_handle_v1_unset_minimized(handle());
}

void MForeignToplevel::activate() noexcept
{
    auto app { MApp::Get() };

    if (app->wl.seat)
        zwlr_foreign_toplevel_handle_v1_activate(handle(), app->wl.seat);
}

void MForeignToplevel::close() noexcept
{
    zwlr_foreign_toplevel_handle_v1_close(handle());
}

bool MForeignToplevel::setFullscreen(MScreen *screen) noexcept
{
    if (handle().version() < 2)
        return false;

    zwlr_foreign_toplevel_handle_v1_set_fullscreen(handle(), screen ? screen->wlOutput() : nullptr);
    return true;
}

void MForeignToplevel::setRect(MSurface &surface, SkIRect rect) noexcept
{
    m_surface = &surface;
    m_rect = rect;
    zwlr_foreign_toplevel_handle_v1_set_rectangle(handle(), surface.wlSurface(),
                                                  rect.x(), rect.y(), rect.width(), rect.height());
}

MForeignToplevel::~MForeignToplevel() noexcept
{
    zwlr_foreign_toplevel_handle_v1_destroy(handle());
}

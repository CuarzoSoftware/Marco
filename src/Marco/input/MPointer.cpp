#include <Marco/input/MPointer.h>
#include <Marco/MApplication.h>
#include <Marco/roles/MSurface.h>

using namespace Marco;
using namespace AK;

void MPointer::setCursor(AKCursor cursor) noexcept
{
    if (m_cursor == cursor && m_cursorSurface)
        return;

    m_cursor = cursor;

    if (cursor == AKCursor::Hidden)
    {
        wl_pointer_set_cursor(app()->wayland().pointer, eventHistory().enter.serial(), NULL, 0, 0);
        return;
    }

    if (!m_cursorSurface)
        m_cursorSurface = wl_compositor_create_surface(app()->wayland().compositor);

    if (!m_cursorTheme)
        m_cursorTheme = wl_cursor_theme_load(NULL, 48, app()->wayland().shm);

    if (!m_cursorTheme)
        return;

    wl_cursor_image *image { nullptr };

    auto it = m_cursors.find(cursor);

    if (it != m_cursors.end())
    {
        image = it->second->images[0];
    }
    else
    {
        wl_cursor *wlCursor { wl_cursor_theme_get_cursor(m_cursorTheme, cursorToString(cursor)) };

        if (wlCursor && wlCursor->image_count > 0)
        {
            m_cursors[cursor] = wlCursor;
            image = wlCursor->images[0];
        }
    }

    if (!image)
        return;

    wl_surface_attach(m_cursorSurface, wl_cursor_image_get_buffer(image), 0, 0);
    wl_surface_set_buffer_scale(m_cursorSurface, 2);
    wl_surface_damage(m_cursorSurface, 0, 0, 128, 128);
    wl_surface_commit(m_cursorSurface);
    wl_pointer_set_cursor(app()->wayland().pointer, eventHistory().enter.serial(), m_cursorSurface, image->hotspot_x/2, image->hotspot_y/2);
}

AKCursor MPointer::findNonDefaultCursor(AKNode *node) const noexcept
{
    while (node && node->cursor() == AKCursor::Default)
        node = node->parent();

    if (node)
        return node->cursor();

    return AKCursor::Default;
}

#ifndef MARCO_H
#define MARCO_H

#include <CZ/AK/AK.h>

namespace CZ
{
    class MApp;
    class MScreen;

    class MSurface;
    class MToplevel;
    class MPopup;
    class MLayerSurface;
    class MSubsurface;

    class MTheme;
    class MCSDShadow;
    class MRootSurfaceNode;
    class MVibrancyView;

    /* Input */
    class MPointer;
    class MKeyboard;

    template <class T> class MProxy;

    MPointer &pointer() noexcept;
    MKeyboard &keyboard() noexcept;
};

struct wl_display;
struct wl_registry;
struct wl_shm;
struct wl_compositor;
struct wl_subcompositor;
struct xdg_wm_base;
struct zxdg_decoration_manager_v1;
struct wl_seat;
struct wl_pointer;
struct wl_keyboard;
struct wp_viewporter;
struct zwlr_layer_shell_v1;
struct lvr_background_blur_manager;
struct lvr_svg_path_manager;
struct lvr_invisible_region_manager;
struct wp_cursor_shape_manager_v1;
struct wp_cursor_shape_device_v1;

#endif // MARCO_H

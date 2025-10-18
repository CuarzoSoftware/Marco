#ifndef MWLRFOREIGNTOPLEVELHANDLE_H
#define MWLRFOREIGNTOPLEVELHANDLE_H

#include <CZ/Marco/Protocols/WlrForeignToplevel/wlr-foreign-toplevel-management-unstable-v1-client.h>
#include <CZ/Marco/Marco.h>

namespace CZ
{
struct MWlrForeignToplevelHandle
{
    static void title(void *data, zwlr_foreign_toplevel_handle_v1 *handle, const char *title);
    static void app_id(void *data, zwlr_foreign_toplevel_handle_v1 *handle, const char *appId);
    static void output_enter(void *data, zwlr_foreign_toplevel_handle_v1 *handle, wl_output *output);
    static void output_leave(void *data, zwlr_foreign_toplevel_handle_v1 *handle, wl_output *output);
    static void state(void *data, zwlr_foreign_toplevel_handle_v1 *handle, wl_array *state);
    static void done(void *data, zwlr_foreign_toplevel_handle_v1 *handle);
    static void closed(void *data, zwlr_foreign_toplevel_handle_v1 *handle);
    static void parent(void *data, zwlr_foreign_toplevel_handle_v1 *handle, zwlr_foreign_toplevel_handle_v1 *parent);

    static constexpr zwlr_foreign_toplevel_handle_v1_listener Listener
    {
        .title = title,
        .app_id = app_id,
        .output_enter = output_enter,
        .output_leave = output_leave,
        .state = state,
        .done = done,
        .closed = closed,
        .parent = parent
    };
};
}

#endif // MWLRFOREIGNTOPLEVELHANDLE_H

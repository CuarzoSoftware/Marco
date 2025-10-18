#ifndef MWLRFOREIGNTOPLEVELMANAGER_H
#define MWLRFOREIGNTOPLEVELMANAGER_H

#include <CZ/Marco/Protocols/WlrForeignToplevel/wlr-foreign-toplevel-management-unstable-v1-client.h>
#include <CZ/Marco/Marco.h>

namespace CZ
{
struct MWlrForeignToplevelManager
{
    static void bound() noexcept;
    static void toplevel(void *data, zwlr_foreign_toplevel_manager_v1 *manager, zwlr_foreign_toplevel_handle_v1 *toplevel);
    static void finished(void *data, zwlr_foreign_toplevel_manager_v1 *manager);

    static constexpr zwlr_foreign_toplevel_manager_v1_listener Listener
    {
        .toplevel = toplevel,
        .finished = finished
    };
};
}
#endif // MWLRFOREIGNTOPLEVELMANAGER_H

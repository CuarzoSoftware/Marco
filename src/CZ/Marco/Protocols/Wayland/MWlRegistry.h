#ifndef MWLREGISTRY_H
#define MWLREGISTRY_H

#include <wayland-client-protocol.h>
#include <CZ/Marco/Marco.h>

namespace CZ
{
struct MWlRegistry
{    
    static void global(void *data, wl_registry *registry, UInt32 name, const char *interface, UInt32 version);
    static void global_remove(void *data, wl_registry *registry, UInt32 name);

    static constexpr wl_registry_listener Listener
    {
        .global = global,
        .global_remove = global_remove
    };
};
}

#endif // MWLREGISTRY_H

#ifndef MLVRPRIVATEHANDLEMANAGER_H
#define MLVRPRIVATEHANDLEMANAGER_H

#include <CZ/Marco/Protocols/LvrPrivateHandle/lvr-private-handle-client.h>
#include <CZ/Marco/Marco.h>

namespace CZ
{
struct MLvrPrivateHandleManager
{
    static void handle(void *data, lvr_private_handle_manager *manager, const char *handle);

    static constexpr lvr_private_handle_manager_listener Listener
    {
        .handle = &handle
    };
};
}

#endif // MLVRPRIVATEHANDLEMANAGER_H

#ifndef MWLSEAT_H
#define MWLSEAT_H

#include <wayland-client-protocol.h>
#include <CZ/Marco/Marco.h>

namespace CZ
{
struct MWlSeat
{
    static void capabilities(void *data, wl_seat *seat, UInt32 capabilities);
    static void name(void *data, wl_seat *seat, const char *name);

    static constexpr wl_seat_listener Listener
    {
        .capabilities = capabilities,
        .name = name
    };
};
}

#endif // MWLSEAT_H

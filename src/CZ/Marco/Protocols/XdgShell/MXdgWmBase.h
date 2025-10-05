#ifndef MXDGWMBASE_H
#define MXDGWMBASE_H

#include <CZ/Marco/Protocols/XdgShell/xdg-shell-client.h>
#include <CZ/Marco/Marco.h>

namespace CZ
{
struct MXdgWmBase
{
    static void ping(void *data, xdg_wm_base *xdgWmBase, UInt32 serial);

    static constexpr xdg_wm_base_listener Listener
    {
        .ping = ping
    };
};
}

#endif // MXDGWMBASE_H

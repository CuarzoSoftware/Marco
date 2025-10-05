#include <CZ/Marco/Protocols/XdgShell/MXdgWmBase.h>

using namespace CZ;

void MXdgWmBase::ping(void *data, xdg_wm_base *xdgWmBase, UInt32 serial)
{
    CZ_UNUSED(data)
    xdg_wm_base_pong(xdgWmBase, serial);
}

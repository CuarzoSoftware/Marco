#include <CZ/Marco/Protocols/Wayland/MWlSeat.h>
#include <CZ/Marco/Protocols/Wayland/MWlPointer.h>
#include <CZ/Marco/Protocols/Wayland/MWlKeyboard.h>
#include <CZ/Marco/MApp.h>

using namespace CZ;

void MWlSeat::capabilities(void *data, wl_seat *seat, UInt32 capabilities)
{
    CZ_UNUSED(data)

    auto app { MApp::Get() };

    if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && !app->wl.pointer)
    {
        app->wl.pointer.set(wl_seat_get_pointer(seat));
        wl_pointer_add_listener(app->wl.pointer, &MWlPointer::Listener, nullptr);
    }

    if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && !app->wl.keyboard)
    {
        app->wl.keyboard.set(wl_seat_get_keyboard(seat));
        wl_keyboard_add_listener(app->wl.keyboard, &MWlKeyboard::Listener, nullptr);
    }

    // TODO: Handle removal
}

void MWlSeat::name(void *data, wl_seat *seat, const char *name)
{
    CZ_UNUSED(data)
    CZ_UNUSED(seat)
    CZ_UNUSED(name)
}

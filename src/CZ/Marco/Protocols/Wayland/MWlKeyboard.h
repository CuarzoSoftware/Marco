#ifndef MWLKEYBOARD_H
#define MWLKEYBOARD_H

#include <wayland-client-protocol.h>
#include <CZ/Marco/Marco.h>

namespace CZ
{
struct MWlKeyboard
{
    static void keymap(void *data, wl_keyboard *keyboard, UInt32 format, Int32 fd, UInt32 size);
    static void enter(void *data, wl_keyboard *keyboard, UInt32 serial, wl_surface *surface, wl_array *keys);
    static void leave(void *data, wl_keyboard *keyboard, UInt32 serial, wl_surface *surface);
    static void key(void *data, wl_keyboard *keyboard, UInt32 serial, UInt32 time, UInt32 key, UInt32 state);
    static void modifiers(void *data, wl_keyboard *keyboard, UInt32 serial, UInt32 depressed, UInt32 latched, UInt32 locked, UInt32 group);
    static void repeat_info(void *data, wl_keyboard *keyboard, Int32 rate, Int32 delay);

    static constexpr wl_keyboard_listener Listener
    {
        .keymap = keymap,
        .enter = enter,
        .leave = leave,
        .key = key,
        .modifiers = modifiers,
        .repeat_info = repeat_info
    };
};
}

#endif // MWLKEYBOARD_H

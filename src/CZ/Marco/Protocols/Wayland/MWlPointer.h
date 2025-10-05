#ifndef MWLPOINTER_H
#define MWLPOINTER_H

#include <wayland-client-protocol.h>
#include <CZ/Marco/Marco.h>

namespace CZ
{
struct MWlPointer
{
    static void enter(void *data, wl_pointer *pointer, UInt32 serial, wl_surface *surface, wl_fixed_t x, wl_fixed_t y);
    static void leave(void *data, wl_pointer *pointer, UInt32 serial, wl_surface *surface);
    static void motion(void *data,wl_pointer *pointer, UInt32 time, wl_fixed_t x, wl_fixed_t y);
    static void button(void *data, wl_pointer *pointer, UInt32 serial, UInt32 time, UInt32 button, UInt32 state);
    static void axis(void *data, wl_pointer *pointer, UInt32 time, UInt32 axis, wl_fixed_t value);
    static void frame(void *data, wl_pointer *pointer);
    static void axis_source(void *data, wl_pointer *pointer, UInt32 axis_source);
    static void axis_stop(void *data, wl_pointer *pointer, UInt32 time, UInt32 axis);
    static void axis_discrete(void *data, wl_pointer *pointer, UInt32 axis, Int32 discrete);
    static void axis_value120(void *data, wl_pointer *pointer, UInt32 axis, Int32 value120);
    static void axis_relative_direction(void *data, wl_pointer *pointer, UInt32 axis, UInt32 direction);

    static void SetCursorFromFocus() noexcept;

    static constexpr wl_pointer_listener Listener
    {
        .enter = enter,
        .leave = leave,
        .motion = motion,
        .button = button,
        .axis = axis,
        .frame = frame,
        .axis_source = axis_source,
        .axis_stop = axis_stop,
        .axis_discrete = axis_discrete,
        .axis_value120 = axis_value120,
        .axis_relative_direction = axis_relative_direction
    };
};
}

#endif // MWLPOINTER_H

#ifndef MWLOUTPUT_H
#define MWLOUTPUT_H

#include <wayland-client-protocol.h>
#include <CZ/Marco/Marco.h>

namespace CZ
{
struct MWlOutput
{
    static void geometry(void *data, wl_output *output, Int32 x, Int32 y, Int32 physicalWidth, Int32 physicalHeight, Int32 subpixel, const char *make, const char *model, Int32 transform);
    static void mode(void *data, wl_output *output, UInt32 flags, Int32 width, Int32 height, Int32 refresh);
    static void done(void *data, wl_output *output);
    static void scale(void *data, wl_output *output, Int32 factor);
    static void name(void *data, wl_output *output, const char *name);
    static void description(void *data, wl_output *output, const char *description);

    static constexpr wl_output_listener Listener
    {
        .geometry = geometry,
        .mode = mode,
        .done = done,
        .scale = scale,
        .name = name,
        .description = description
    };
};
}

#endif // MWLOUTPUT_H

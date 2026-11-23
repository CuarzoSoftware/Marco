#ifndef MXDGOUTPUT_H
#define MXDGOUTPUT_H

#include <CZ/Marco/Protocols/XdgOutput/xdg-output-unstable-v1-client.h>
#include <CZ/Marco/Marco.h>

namespace CZ
{
/// xdg-output: the screens' logical position and size (see MScreen::Props::logicalSize).
struct MXdgOutput
{
    /// Gets the xdg_output of @p screen (once the manager and the screen are both bound).
    static void Create(MScreen &screen) noexcept;

    static void logical_position(void *data, zxdg_output_v1 *output, Int32 x, Int32 y);
    static void logical_size(void *data, zxdg_output_v1 *output, Int32 width, Int32 height);
    static void done(void *data, zxdg_output_v1 *output);
    static void name(void *data, zxdg_output_v1 *output, const char *name);
    static void description(void *data, zxdg_output_v1 *output, const char *description);

    static constexpr zxdg_output_v1_listener Listener
    {
        .logical_position = logical_position,
        .logical_size = logical_size,
        .done = done,
        .name = name,
        .description = description
    };
};
}

#endif // MXDGOUTPUT_H

#ifndef MWLROUTPUTMANAGER_H
#define MWLROUTPUTMANAGER_H

#include <CZ/Marco/Protocols/WlrOutputManagement/wlr-output-management-unstable-v1-client.h>
#include <CZ/Marco/Marco.h>

namespace CZ
{
struct MWlrOutputManager
{
    static void head(void *data, zwlr_output_manager_v1 *manager, zwlr_output_head_v1 *head);
    static void done(void *data, zwlr_output_manager_v1 *manager, UInt32 serial);
    static void finished(void *data, zwlr_output_manager_v1 *manager);

    static constexpr zwlr_output_manager_v1_listener Listener
    {
        .head = head,
        .done = done,
        .finished = finished
    };
};

struct MWlrOutputHead
{
    static void name(void *data, zwlr_output_head_v1 *head, const char *name);
    static void description(void *data, zwlr_output_head_v1 *head, const char *description);
    static void physical_size(void *data, zwlr_output_head_v1 *head, Int32 width, Int32 height);
    static void mode(void *data, zwlr_output_head_v1 *head, zwlr_output_mode_v1 *mode);
    static void enabled(void *data, zwlr_output_head_v1 *head, Int32 enabled);
    static void current_mode(void *data, zwlr_output_head_v1 *head, zwlr_output_mode_v1 *mode);
    static void position(void *data, zwlr_output_head_v1 *head, Int32 x, Int32 y);
    static void transform(void *data, zwlr_output_head_v1 *head, Int32 transform);
    static void scale(void *data, zwlr_output_head_v1 *head, wl_fixed_t scale);
    static void finished(void *data, zwlr_output_head_v1 *head);
    static void make(void *data, zwlr_output_head_v1 *head, const char *make);
    static void model(void *data, zwlr_output_head_v1 *head, const char *model);
    static void serial_number(void *data, zwlr_output_head_v1 *head, const char *serialNumber);
    static void adaptive_sync(void *data, zwlr_output_head_v1 *head, UInt32 state);

    static constexpr zwlr_output_head_v1_listener Listener
    {
        .name = name,
        .description = description,
        .physical_size = physical_size,
        .mode = mode,
        .enabled = enabled,
        .current_mode = current_mode,
        .position = position,
        .transform = transform,
        .scale = scale,
        .finished = finished,
        .make = make,
        .model = model,
        .serial_number = serial_number,
        .adaptive_sync = adaptive_sync
    };
};

struct MWlrOutputMode
{
    static void size(void *data, zwlr_output_mode_v1 *mode, Int32 width, Int32 height);
    static void refresh(void *data, zwlr_output_mode_v1 *mode, Int32 refresh);
    static void preferred(void *data, zwlr_output_mode_v1 *mode);
    static void finished(void *data, zwlr_output_mode_v1 *mode);

    static constexpr zwlr_output_mode_v1_listener Listener
    {
        .size = size,
        .refresh = refresh,
        .preferred = preferred,
        .finished = finished
    };
};

struct MWlrOutputConfiguration
{
    static void succeeded(void *data, zwlr_output_configuration_v1 *configuration);
    static void failed(void *data, zwlr_output_configuration_v1 *configuration);
    static void cancelled(void *data, zwlr_output_configuration_v1 *configuration);

    static constexpr zwlr_output_configuration_v1_listener Listener
    {
        .succeeded = succeeded,
        .failed = failed,
        .cancelled = cancelled
    };
};
}

#endif // MWLROUTPUTMANAGER_H

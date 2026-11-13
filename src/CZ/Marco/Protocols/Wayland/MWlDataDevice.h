#ifndef MWLDATADEVICE_H
#define MWLDATADEVICE_H

#include <wayland-client-protocol.h>
#include <CZ/Marco/Marco.h>

namespace CZ
{
/// wl_data_device events: routes drag & drop offers to the MSurface under the pointer.
struct MWlDataDevice
{
    static void data_offer(void *data, wl_data_device *device, wl_data_offer *offer);
    static void enter(void *data, wl_data_device *device, UInt32 serial, wl_surface *surface, wl_fixed_t x, wl_fixed_t y, wl_data_offer *offer);
    static void leave(void *data, wl_data_device *device);
    static void motion(void *data, wl_data_device *device, UInt32 time, wl_fixed_t x, wl_fixed_t y);
    static void drop(void *data, wl_data_device *device);
    static void selection(void *data, wl_data_device *device, wl_data_offer *offer);

    /// The current selection (clipboard) offer, or nullptr (see MClipboard).
    static MDNDOffer *SelectionOffer() noexcept;

    // MDNDOffer's constructor/destructor are only accessible from here.
    static MDNDOffer *CreateOffer(wl_data_offer *offer) noexcept;
    static void DestroyOffer(MDNDOffer *offer) noexcept;
    static void SetEnterSerial(MDNDOffer &offer, UInt32 serial) noexcept;

    static constexpr wl_data_device_listener Listener
    {
        .data_offer = data_offer,
        .enter = enter,
        .leave = leave,
        .motion = motion,
        .drop = drop,
        .selection = selection
    };
};
}

#endif // MWLDATADEVICE_H

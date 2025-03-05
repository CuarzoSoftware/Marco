#ifndef MTOPLEVELPRIVATE_H
#define MTOPLEVELPRIVATE_H

#include <Marco/roles/MToplevel.h>
#include <Marco/protocols/xdg-shell-client.h>
#include <Marco/protocols/xdg-decoration-unstable-v1-client.h>
#include <Marco/nodes/MCSDShadow.h>
#include <AK/nodes/AKRenderableImage.h>

class AK::MToplevel::Imp
{
public:
    enum Flags
    {
        PendingNullCommit       = 1 << 0,
        PendingFirstConfigure   = 1 << 1,
        PendingConfigureAck     = 1 << 2,
        Mapped                  = 1 << 3,
        ForceUpdate             = 1 << 4,
    };

    Imp(MToplevel &obj) noexcept;
    MToplevel &obj;

    AKBitset<Flags> flags { PendingNullCommit };

    std::string title;
    SkISize minSize, maxSize;

    // From the last xdg_surface_configure
    AKBitset<AKWindowState> currentStates;
    SkISize currentSuggestedSize { 0, 0 };
    UInt32 configureSerial { 0 };

    // From xdg_toplevel_configure but not yet xdg_surface_configure(d)
    AKBitset<AKWindowState> pendingStates;
    SkISize pendingSuggestedSize { 0, 0 };

    // Built-in decorations
    AKRenderableImage borderRadius[4]; // Border radius masks
    SkIRect shadowMargins { 48, 30, 48, 66 }; // L, T, R, B shadow margins
    MCSDShadow shadow; // Shadow node

    // Wayland
    xdg_surface *xdgSurface { nullptr };
    xdg_toplevel *xdgToplevel { nullptr };
    zxdg_toplevel_decoration_v1 *xdgDecoration { nullptr };
    static inline xdg_surface_listener xdgSurfaceListener;
    static inline xdg_toplevel_listener xdgToplevelListener;
    static void xdg_surface_configure(void *data, xdg_surface *xdgSurface, UInt32 serial);
    static void xdg_toplevel_configure(void *data, xdg_toplevel *xdgToplevel, Int32 width, Int32 height, wl_array *states);
    static void xdg_toplevel_close(void *data, xdg_toplevel *xdgToplevel);
    static void xdg_toplevel_configure_bounds(void *data, xdg_toplevel *xdgToplevel,Int32 width, Int32 height);
    static void xdg_toplevel_wm_capabilities(void *data, xdg_toplevel *xdgToplevel, wl_array *capabilities);

    // Root node event filter
    void handleRootPointerButtonEvent(const AKPointerButtonEvent &event) noexcept;
    void handleRootPointerMoveEvent(const AKPointerMoveEvent &event) noexcept;
};

#endif // MTOPLEVELPRIVATE_H

#ifndef MTOPLEVEL_H
#define MTOPLEVEL_H

#include <Marco/nodes/MCSDShadow.h>
#include <Marco/roles/MSurface.h>
#include <Marco/protocols/xdg-shell-client.h>
#include <Marco/protocols/xdg-decoration-unstable-v1-client.h>
#include <AK/nodes/AKRenderableImage.h>

class Marco::MToplevel : public MSurface
{
public:
    enum State
    {
        Maximized   = 1 << XDG_TOPLEVEL_STATE_MAXIMIZED,
        Fullscreen  = 1 << XDG_TOPLEVEL_STATE_FULLSCREEN,
        Resizing    = 1 << XDG_TOPLEVEL_STATE_RESIZING,
        Activated   = 1 << XDG_TOPLEVEL_STATE_ACTIVATED,
        TiledLeft   = 1 << XDG_TOPLEVEL_STATE_TILED_LEFT,
        TiledRight  = 1 << XDG_TOPLEVEL_STATE_TILED_RIGHT,
        TiledTop    = 1 << XDG_TOPLEVEL_STATE_TILED_TOP,
        TiledBottom = 1 << XDG_TOPLEVEL_STATE_TILED_BOTTOM,
        Suspended   = 1 << XDG_TOPLEVEL_STATE_SUSPENDED,
    };

    MToplevel() noexcept;
    ~MToplevel() noexcept;

    void setMaximized(bool maximized) noexcept;
    void setFullscreen(bool fullscreen) noexcept;
    void setMinimized() noexcept;

    virtual void onSuggestedSizeChanged();
    const SkISize &suggestedSize() const noexcept
    {
        return cl.suggestedSize;
    }

    virtual void onStatesChanged();
    AK::AKBitset<State> states() const noexcept
    {
        return cl.states;
    }

    void setTitle(const std::string &title);

    const std::string &title() const noexcept
    {
        return cl.title;
    }

    const SkIRect &csdMargins() const noexcept
    {
        return cl.csdShadowMargins;
    }

    struct
    {
        AK::AKSignal<const std::string &> titleChanged;
        AK::AKSignal<const SkISize &> suggestedSizeChanged;
        AK::AKSignal<AK::AKBitset<State>> statesChanged;
    } on;

protected:

    enum Flags
    {
        PendingNullCommit       = 1 << 0,
        PendingFirstConfigure   = 1 << 1,
        PendingConfigureAck     = 1 << 2,
        Mapped                  = 1 << 3,
        ForceUpdate             = 1 << 4
    };

    void onUpdate() noexcept override;
    void render() noexcept;

    struct {
        xdg_surface *xdgSurface { nullptr };
        xdg_toplevel *xdgToplevel { nullptr };
        zxdg_toplevel_decoration_v1 *xdgDecoration { nullptr };
    } wl;

    struct CL{
        CL(MToplevel *toplevel) : csdShadow(toplevel) {}
        AK::AKBitset<Flags> flags { PendingNullCommit };
        AK::AKBitset<State> states;
        SkISize suggestedSize { 0, 0 };
        std::string title;
        AK::AKRenderableImage csdBorderRadius[4];
        SkIRect csdShadowMargins { 48, 30, 48, 66 };
        MCSDShadow csdShadow;
    } cl{this};

    struct
    {
        UInt32 serial;
        AK::AKBitset<State> states;
        SkISize suggestedSize { 0, 0 };
    } se;

private:
    static void xdg_surface_configure(void *data, xdg_surface *xdgSurface, UInt32 serial);
    static void xdg_toplevel_configure(void *data, xdg_toplevel *xdgToplevel, Int32 width, Int32 height, wl_array *states);
    static void xdg_toplevel_close(void *data, xdg_toplevel *xdgToplevel);
    static void xdg_toplevel_configure_bounds(void *data, xdg_toplevel *xdgToplevel,Int32 width, Int32 height);
    static void xdg_toplevel_wm_capabilities(void *data, xdg_toplevel *xdgToplevel, wl_array *capabilities);
};

#endif // MTOPLEVEL_H

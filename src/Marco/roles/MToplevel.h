#ifndef MTOPLEVEL_H
#define MTOPLEVEL_H

#include <Marco/roles/MSurface.h>
#include <Marco/protocols/xdg-shell-client.h>

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

    void setWindowSize(const SkISize &size) noexcept
    {
        m_windowSize = size;
    }

    void setTitle(const std::string &title)
    {
        if (m_title == title)
            return;

        m_title = title;
        on.titleChanged.notify(m_title);
    }

    const std::string &title() const noexcept
    {
        return m_title;
    }

    struct
    {
        AK::AKSignal<const std::string &> titleChanged;
    } on;

protected:
    enum Changes
    {
        CHWindowSize = MSurface::CHLast,
        CHWindowMargins,
        CHPendingInitialNullAttach,
        CHPendingInitialConfiguration,
        CHConfigurationSerial,
        CHConfigurationSize,
        CHConfigurationState,
        CHLast
    };

    static void xdg_surface_configure(void *data, xdg_surface *xdgSurface, UInt32 serial);
    static void xdg_toplevel_configure(void *data, xdg_toplevel *xdgToplevel, Int32 width, Int32 height, wl_array *states);
    static void xdg_toplevel_close(void *data, xdg_toplevel *xdgToplevel);
    static void xdg_toplevel_configure_bounds(void *data, xdg_toplevel *xdgToplevel,Int32 width, Int32 height);
    static void xdg_toplevel_wm_capabilities(void *data, xdg_toplevel *xdgToplevel, wl_array *capabilities);

    void handleChanges() noexcept override;
    void handleConfigurationChange() noexcept;
    void handleVisibilityChange() noexcept;
    void handleDimensionsChange() noexcept;
    void createSurface() noexcept;
    void destroySurface() noexcept;
    void render() noexcept;
    xdg_surface *m_xdgSurface { nullptr };
    xdg_toplevel *m_xdgToplevel { nullptr };
    SkISize m_windowSize { 0, 0 };
    AK::AKBitset<State> m_states;

    struct
    {
        UInt32 serial;
        AK::AKBitset<State> states;
        SkISize windowSize { 0, 0 };
    }m_conf;

    std::string m_title;
};

#endif // MTOPLEVEL_H

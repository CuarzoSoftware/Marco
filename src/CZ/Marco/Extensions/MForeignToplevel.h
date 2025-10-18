#ifndef MFOREIGNTOPLEVEL_H
#define MFOREIGNTOPLEVEL_H

#include <CZ/Marco/Protocols/WlrForeignToplevel/wlr-foreign-toplevel-management-unstable-v1-client.h>
#include <CZ/skia/core/SkRect.h>
#include <CZ/AK/AKObject.h>
#include <CZ/Marco/Marco.h>
#include <CZ/Marco/MProxy.h>
#include <CZ/Core/CZSignal.h>
#include <CZ/Core/CZBitset.h>
#include <CZ/Core/CZWeak.h>
#include <unordered_set>

class CZ::MForeignToplevel : public AKObject
{
public:
    enum State
    {
        Maximized  = 1 << 0,
        Minimized  = 1 << 1,
        Activated  = 1 << 2,
        Fullscreen = 1 << 3
    };

    enum Changes
    {
        CHState   = 1 << 0,
        CHTitle   = 1 << 1,
        CHAppId   = 1 << 2,
        CHScreens = 1 << 3
    };

    struct Props
    {
        CZBitset<State> state;
        std::string title;
        std::string appId;
        std::unordered_set<MScreen*> screens;
        CZWeak<MForeignToplevel> parent;
    };

    const Props &props() const noexcept { return m_current; }
    MProxy<zwlr_foreign_toplevel_handle_v1> &handle() noexcept { return m_handle; }

    // All these are async, wait for the compositor to notify the change (which may never happen)
    void setMaximized(bool maximized) noexcept;
    void setMinimized(bool minimized) noexcept;
    void activate() noexcept;
    void close() noexcept;
    bool setFullscreen(MScreen *screen = nullptr) noexcept; // Returns false if the compositor doesn't support this request

    // Rect within a surface where the toplevel is represented e.g. as a thumbnail
    void setRect(MSurface &surface, SkIRect rect) noexcept;
    MSurface *surface() const noexcept { return m_surface; }
    SkIRect rect() const noexcept { return m_rect; }

    // Scoped user data
    std::shared_ptr<CZObject> data;

    // Notifies one or more prop changes atomically
    CZSignal<CZBitset<Changes> /*changes*/> onPropsChanged;

private:
    friend class MForeignToplevelManager;
    friend class MWlrForeignToplevelManager;
    friend class MWlrForeignToplevelHandle;
    MForeignToplevel(zwlr_foreign_toplevel_handle_v1 *handle) noexcept :
        m_handle(handle) {}
    ~MForeignToplevel() noexcept;
    MProxy<zwlr_foreign_toplevel_handle_v1> m_handle {};
    Props m_current {};
    Props m_pending {};
    SkIRect m_rect {};
    CZWeak<MSurface> m_surface;
    CZBitset<Changes> m_changes;
    bool m_isComplete {};
};

#endif // MFOREIGNTOPLEVEL_H

#ifndef MFOREIGNTOPLEVEL_H
#define MFOREIGNTOPLEVEL_H

#include <functional>
#include <CZ/Ream/Ream.h>
#include <memory>
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
        CHScreens = 1 << 3,
        CHIdentifier = 1 << 4 ///< props().identifier (Louvre's wlr-foreign-toplevel v4)
    };

    struct Props
    {
        CZBitset<State> state;
        std::string title;
        std::string appId;
        std::unordered_set<MScreen*> screens;
        CZWeak<MForeignToplevel> parent;

        /// Its ext-foreign-toplevel-list identifier (Louvre's wlr-foreign-toplevel v4), empty if unknown.
        std::string identifier;
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

    /**
     * @brief Captures an image of the window (e.g. a thumbnail of a minimized window).
     *
     * Uses ext-image-copy-capture with a source created (ext-image-capture-source) from this
     * toplevel's ext-foreign-toplevel-list handle, found through props().identifier (sent by Louvre's
     * wlr-foreign-toplevel v4). Its size is up to the compositor (Crystals sends docks a small version
     * of the window, with its title bar). The compositor may also ask the user first.
     *
     * @p done is called once, with the image or nullptr (failed, denied, or the window is gone), unless
     * this object is destroyed first.
     *
     * @return `false` (and @p done isn't called) if the compositor doesn't support it.
     */
    bool captureImage(std::function<void(std::shared_ptr<RImage> image)> done) noexcept;

    /// Internal: tracks the ext-foreign-toplevel-list handles (by identifier).
    static void ToplevelListBound(ext_foreign_toplevel_list_v1 *list) noexcept;
    MSurface *surface() const noexcept { return m_surface; }
    SkIRect rect() const noexcept { return m_rect; }

    // Scoped user data
    std::shared_ptr<CZObject> data;

    // Notifies one or more prop changes atomically
    CZSignal<CZBitset<Changes> /*changes*/> onPropsChanged;

private:
    friend class MForeignToplevelManager;
    friend struct MWlrForeignToplevelManager;
    friend struct MWlrForeignToplevelHandle;
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

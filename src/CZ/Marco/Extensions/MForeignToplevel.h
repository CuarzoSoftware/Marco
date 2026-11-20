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

/**
 * @brief A window of another client (wlr-foreign-toplevel-management), e.g. for docks and taskbars.
 *
 * Listed by MForeignToplevelManager (MApp::ext, requires MForeignToplevelManagerExt). Created and
 * destroyed by the manager: it's announced with MForeignToplevelManager::onAdded once its initial
 * props are known, and destroyed right after MForeignToplevelManager::onRemoved.
 */
class CZ::MForeignToplevel : public AKObject
{
public:
    /** @brief Window states, see Props::state. */
    enum State
    {
        Maximized  = 1 << 0, ///< Maximized.
        Minimized  = 1 << 1, ///< Minimized.
        Activated  = 1 << 2, ///< Activated (focused).
        Fullscreen = 1 << 3  ///< Fullscreen.
    };

    /** @brief Props that changed, see onPropsChanged. */
    enum Changes
    {
        CHState   = 1 << 0, ///< props().state
        CHTitle   = 1 << 1, ///< props().title
        CHAppId   = 1 << 2, ///< props().appId
        CHScreens = 1 << 3, ///< props().screens
        CHIdentifier = 1 << 4 ///< props().identifier (Louvre's wlr-foreign-toplevel v4)
    };

    /** @brief The window properties, as sent by the compositor. */
    struct Props
    {
        CZBitset<State> state;                ///< Current states.
        std::string title;                    ///< Title.
        std::string appId;                    ///< App ID.
        std::unordered_set<MScreen*> screens; ///< Screens the window is visible on.
        CZWeak<MForeignToplevel> parent;      ///< Parent window (not implemented yet, always `nullptr`).

        /// Its ext-foreign-toplevel-list identifier (Louvre's wlr-foreign-toplevel v4), empty if unknown.
        std::string identifier;
    };

    /** @brief The current properties. */
    const Props &props() const noexcept { return m_current; }

    /** @brief The zwlr_foreign_toplevel_handle_v1 proxy. */
    MProxy<zwlr_foreign_toplevel_handle_v1> &handle() noexcept { return m_handle; }

    // All these are async, wait for the compositor to notify the change (which may never happen)

    /**
     * @brief Asks the compositor to maximize or unmaximize the window.
     *
     * Like the other requests, it's async: wait for onPropsChanged (which may never come).
     */
    void setMaximized(bool maximized) noexcept;

    /** @brief Asks the compositor to minimize or unminimize the window (async). */
    void setMinimized(bool minimized) noexcept;

    /** @brief Asks the compositor to activate (focus) the window (async). */
    void activate() noexcept;

    /** @brief Asks the window to close (async). */
    void close() noexcept;

    /**
     * @brief Asks the compositor to make the window fullscreen (async).
     *
     * @param screen The screen to use, or `nullptr` to let the compositor choose.
     * @return `false` if the compositor doesn't support this request.
     */
    bool setFullscreen(MScreen *screen = nullptr) noexcept; // Returns false if the compositor doesn't support this request

    // Rect within a surface where the toplevel is represented e.g. as a thumbnail

    /**
     * @brief Tells the compositor where the window is represented within @p surface (e.g. a dock icon).
     *
     * Compositors may use it, e.g. as the target of the minimize animation. @p rect is in
     * @p surface local coordinates.
     */
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

    /// @cond INTERNAL
    /// Internal: tracks the ext-foreign-toplevel-list handles (by identifier).
    static void ToplevelListBound(ext_foreign_toplevel_list_v1 *list) noexcept;
    /// @endcond

    /** @brief The surface passed to setRect(), or `nullptr`. */
    MSurface *surface() const noexcept { return m_surface; }

    /** @brief The rect passed to setRect(). */
    SkIRect rect() const noexcept { return m_rect; }

    // Scoped user data

    /** @brief User data, destroyed along with this object. */
    std::shared_ptr<CZObject> data;

    // Notifies one or more prop changes atomically

    /** @brief Emitted when one or more props change (atomically), with the changed ones. */
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

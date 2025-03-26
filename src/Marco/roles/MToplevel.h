#ifndef MTOPLEVEL_H
#define MTOPLEVEL_H

#include <Marco/roles/MSurface.h>
#include <AK/AKWindowState.h>

/**
 * @brief A toplevel window.
 *
 * The `MToplevel` class represents the `xdg_toplevel` role of the `xdg_shell` protocol.
 * It is the most common type of window and can be maximized, set to fullscreen, minimized, etc.
 */
class AK::MToplevel : public MSurface
{
public:
    /**
     * @brief Default constructor for `MToplevel`.
     */
    MToplevel() noexcept;

    AKCLASS_NO_COPY(MToplevel)

    /**
     * @brief Destructor for `MToplevel`.
     */
    ~MToplevel() noexcept;

    /**
     * @brief Retrieves the current window states.
     *
     * Returns a bitset containing the current window states, such as Maximized, Fullscreen, Activated, etc.
     * Individual states can be checked using helper methods like `maximized()`, `fullscreen()`, etc.
     *
     * @return A bitset representing the current window states.
     * @see windowStateEvent(), onStatesChanged
     */
    AKBitset<AKWindowState> states() const noexcept;

    /**
     * @brief Requests the compositor to maximize or unmaximize the window.
     *
     * The state change is not applied immediately. The compositor will notify the window
     * of the new state via `windowStateEvent()` and `onStatesChanged`.
     *
     * @param maximized If `true`, the window will be maximized, if `false`, it will be unmaximized.
     * @see suggestedSizeChanged(), windowStateEvent(), onStatesChanged
     */
    void setMaximized(bool maximized) noexcept;

    /**
     * @brief Checks if the window is currently maximized.
     *
     * This is equivalent to calling `states().check(AKMaximized)`.
     *
     * @return `true` if the window is maximized, `false` otherwise.
     */
    bool maximized() const noexcept;

    /**
     * @brief Requests the compositor to set the window to fullscreen or exit fullscreen mode.
     *
     * The state change is not applied immediately. The compositor will notify the window
     * of the new state via `windowStateEvent()` and `onStatesChanged`.
     *
     * @param fullscreen If `true`, the window will be set to fullscreen, if `false`, it will exit fullscreen mode.
     * @param screen The screen on which to display the window in fullscreen mode. If `nullptr`, the compositor will choose the screen.
     * @see suggestedSizeChanged(), windowStateEvent(), onStatesChanged
     */
    void setFullscreen(bool fullscreen, MScreen *screen = nullptr) noexcept;

    /**
     * @brief Checks if the window is currently in fullscreen mode.
     *
     * This is equivalent to calling `states().check(AKFullscreen)`.
     *
     * @return `true` if the window is in fullscreen mode, `false` otherwise.
     */
    bool fullscreen() const noexcept;

    /**
     * @brief Requests the compositor to minimize the window.
     *
     * Note that minimizing is not a window state, and there is no way to query
     * whether the compositor has minimized the window.
     */
    void setMinimized() noexcept;

    void setMinSize(const SkISize &size) noexcept;
    const SkISize &minSize() const noexcept;

    void setMaxSize(const SkISize &size) noexcept;
    const SkISize &maxSize() const noexcept;

    /**
     * @brief Retrieves the last window size suggested by the compositor.
     *
     * If either the width or height is `0`, it means the compositor is allowing the application
     * to decide the size for that axis.
     *
     * @return The suggested window size.
     */
    const SkISize &suggestedSize() const noexcept;

    /**
     * @brief Sets the title of the window.
     *
     * @param title The new title for the window.
     * @see onTitleChanged
     */
    void setTitle(const std::string &title);

    /**
     * @brief Retrieves the current window title.
     *
     * The title is set using `setTitle()` and is empty by default.
     *
     * @return The current window title.
     */
    const std::string &title() const noexcept;

    /**
     * @brief Retrieves the built-in client decoration margins.
     *
     * This is primarily informational and has no direct functional use.
     *
     * @return The decoration margins.
     */
    const SkIRect &decorationMargins() const noexcept;

    /**
     * @brief Signal emitted when the window states change.
     *
     * This signal is triggered by the default implementation of `windowStateEvent()`.
     */
    AKSignal<const AKWindowStateEvent&> onStatesChanged;

    /**
     * @brief Signal emitted when the window title changes.
     *
     * This signal is triggered after calling `setTitle()` with a new title.
     */
    AKSignal<> onTitleChanged;

    AKSignal<const AKWindowCloseEvent&> onBeforeClose;
protected:

    /**
     * @brief Notifies the window that the compositor has suggested a new size.
     *
     * This method is triggered before the window is mapped and whenever the compositor
     * suggests a new size, such as after a maximize or fullscreen request.
     *
     * While it is generally advisable to follow the compositor's suggestions, they can be ignored.
     *
     * @see suggestedSize()
     */
    virtual void suggestedSizeChanged();

    /**
     * @brief Handles changes in the window states.
     *
     * This method is called when the window states change. By default, it triggers the
     * `onStatesChanged` signal.
     *
     * @param event The event containing the state changes.
     */
    void windowStateEvent(const AKWindowStateEvent &event) override;
    bool eventFilter(const AKEvent &event, AKObject &object) override;
    bool event(const AKEvent &e) override;
    void onUpdate() noexcept override;

    class Imp;
    Imp *imp() const noexcept;
private:
    std::unique_ptr<Imp> m_imp;
    void render() noexcept;
};

#endif // MTOPLEVEL_H

#ifndef MDND_H
#define MDND_H

#include <CZ/Marco/Roles/MSurface.h>
#include <CZ/Marco/MDNDOffer.h>
#include <CZ/Core/CZBitset.h>
#include <CZ/Core/CZSignal.h>
#include <functional>
#include <string>
#include <vector>

/**
 * @brief A drag & drop session started by this client, and its icon surface.
 *
 * The surface itself is the drag icon (wl_data_device "icon" role): build its content like any
 * other MSurface (e.g. add an image node and size it). The compositor moves it with the pointer;
 * the pointer sits at hotspot() within it.
 *
 * Start a session with start(), passing the button press that initiated it. While dragging, the
 * compositor routes the pointer to the drag & drop: surfaces under it (including this client's)
 * receive the MSurface::onDNDEnter/Motion/Leave/Drop signals instead of pointer events.
 *
 * The session ends with onFinished (dropped and accepted), onCancelled (rejected, or released
 * where nothing accepted it) or cancel(). The icon is unmapped then.
 */
class CZ::MDND : public MSurface
{
public:
    using Action = MDNDOffer::Action;

    MDND() noexcept;
    ~MDND() noexcept;

    /**
     * @brief Starts dragging.
     *
     * @param origin The surface where the drag started (it must have the pointer focus).
     * @param event The pointer button press that started it (its serial is used).
     * @param mimeTypes Data formats offered (see onSend).
     * @param actions Actions supported by the source.
     * @return false if a session is already active or drag & drop isn't supported.
     */
    bool start(MSurface &origin, const CZInputEvent &event, const std::vector<std::string> &mimeTypes,
               CZBitset<Action> actions = CZBitset<Action>(Action::Copy | Action::Move)) noexcept;

    /// Ends the session (the drop target is told it was cancelled).
    void cancel() noexcept;

    bool dragging() const noexcept { return m_source != nullptr; }

    /// Position of the pointer within the icon. Defaults to (0, 0).
    void setHotspot(const SkIPoint &hotspot) noexcept;
    const SkIPoint &hotspot() const noexcept { return m_hotspot; }

    /// MIME type accepted by the target under the pointer (empty if none).
    const std::string &acceptedMimeType() const noexcept { return m_accepted; }

    /// Action selected by the compositor for the current target.
    Action action() const noexcept { return m_action; }

    /// Writes the data in @p mimeType to @p fd (closed afterwards by MDND).
    std::function<void(const std::string &mimeType, Int32 fd)> onSend;

    CZSignal<> onDropPerformed; ///< Released over a target (it may still reject it, see onCancelled)
    CZSignal<> onFinished;      ///< The target took the data: session over
    CZSignal<> onCancelled;     ///< Rejected / released over nothing / cancelled: session over

protected:
    void onUpdate() noexcept override;

private:
    void render() noexcept;
    void end() noexcept;
    static void source_target(void *data, wl_data_source *source, const char *mimeType);
    static void source_send(void *data, wl_data_source *source, const char *mimeType, Int32 fd);
    static void source_cancelled(void *data, wl_data_source *source);
    static void source_dnd_drop_performed(void *data, wl_data_source *source);
    static void source_dnd_finished(void *data, wl_data_source *source);
    static void source_action(void *data, wl_data_source *source, UInt32 action);
    static const wl_data_source_listener Listener;
    wl_data_source *m_source { nullptr };
    SkIPoint m_hotspot {};
    SkIPoint m_sentHotspot {};
    std::string m_accepted;
    Action m_action { Action::NoAction };
};

#endif // MDND_H

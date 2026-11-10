#ifndef MDNDOFFER_H
#define MDNDOFFER_H

#include <CZ/Marco/Marco.h>
#include <CZ/Core/CZObject.h>
#include <CZ/Core/CZBitset.h>
#include <wayland-client-protocol.h>
#include <string>
#include <vector>

/**
 * @brief Data offered to this client by a drag & drop session (wl_data_offer).
 *
 * Received through the MSurface::onDNDEnter / onDNDMotion / onDNDLeave / onDNDDrop signals of the
 * surface under the pointer. Only valid until the leave or drop signal returns.
 *
 * To be a valid drop target, call accept() with one of the mimeTypes() (and setActions()) when the
 * drag enters or moves. On drop, read the data with receive() and call finish().
 */
class CZ::MDNDOffer final : public CZObject
{
public:
    /// Same values as wl_data_device_manager.dnd_action.
    enum Action : UInt32
    {
        NoAction    = 0,
        Copy        = 1,
        Move        = 2,
        Ask         = 4
    };

    /// MIME types offered by the source.
    const std::vector<std::string> &mimeTypes() const noexcept { return m_mimeTypes; }
    bool hasMimeType(const std::string &mimeType) const noexcept;

    /// Actions supported by the source and the one selected by the compositor.
    CZBitset<Action> sourceActions() const noexcept { return m_sourceActions; }
    Action action() const noexcept { return m_action; }

    /// Accepts a MIME type for the drop (an empty string rejects it).
    void accept(const std::string &mimeType) noexcept;

    /// Actions this target supports and prefers (wl_data_device_manager >= 3).
    void setActions(CZBitset<Action> supported, Action preferred) noexcept;

    /**
     * @brief Requests the data in the given MIME type.
     *
     * @return The read end of a pipe the source writes the data to (the caller closes it), or -1.
     */
    Int32 receive(const std::string &mimeType) noexcept;

    /// Tells the source the drop was completed (after receive()).
    void finish() noexcept;

    wl_data_offer *offer() const noexcept { return m_offer; }

private:
    friend struct MWlDataDevice;
    MDNDOffer(wl_data_offer *offer) noexcept;
    ~MDNDOffer() noexcept;
    static void offer_offer(void *data, wl_data_offer *offer, const char *mimeType);
    static void offer_source_actions(void *data, wl_data_offer *offer, UInt32 actions);
    static void offer_action(void *data, wl_data_offer *offer, UInt32 action);
    static const wl_data_offer_listener Listener;
    wl_data_offer *m_offer;
    std::vector<std::string> m_mimeTypes;
    CZBitset<Action> m_sourceActions;
    Action m_action { NoAction };
    UInt32 m_enterSerial { 0 };
};

#endif // MDNDOFFER_H

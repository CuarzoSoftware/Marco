#include <CZ/Marco/MDNDOffer.h>
#include <CZ/Marco/MApp.h>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>

using namespace CZ;

const wl_data_offer_listener MDNDOffer::Listener
{
    .offer = MDNDOffer::offer_offer,
    .source_actions = MDNDOffer::offer_source_actions,
    .action = MDNDOffer::offer_action
};

MDNDOffer::MDNDOffer(wl_data_offer *offer) noexcept : m_offer(offer)
{
    wl_data_offer_add_listener(offer, &Listener, this);
}

MDNDOffer::~MDNDOffer() noexcept
{
    wl_data_offer_destroy(m_offer);
}

bool MDNDOffer::hasMimeType(const std::string &mimeType) const noexcept
{
    return std::find(m_mimeTypes.begin(), m_mimeTypes.end(), mimeType) != m_mimeTypes.end();
}

void MDNDOffer::accept(const std::string &mimeType) noexcept
{
    wl_data_offer_accept(m_offer, m_enterSerial, mimeType.empty() ? nullptr : mimeType.c_str());
}

void MDNDOffer::setActions(CZBitset<Action> supported, Action preferred) noexcept
{
    if (wl_data_offer_get_version(m_offer) >= 3)
        wl_data_offer_set_actions(m_offer, supported.get(), preferred);
}

Int32 MDNDOffer::receive(const std::string &mimeType) noexcept
{
    Int32 fds[2];

    if (pipe2(fds, O_CLOEXEC) != 0)
        return -1;

    wl_data_offer_receive(m_offer, mimeType.c_str(), fds[1]);
    close(fds[1]);
    wl_display_flush(MApp::Get()->wl.display);
    return fds[0];
}

void MDNDOffer::finish() noexcept
{
    if (wl_data_offer_get_version(m_offer) >= 3)
        wl_data_offer_finish(m_offer);
}

void MDNDOffer::offer_offer(void *data, wl_data_offer */*offer*/, const char *mimeType)
{
    static_cast<MDNDOffer*>(data)->m_mimeTypes.emplace_back(mimeType);
}

void MDNDOffer::offer_source_actions(void *data, wl_data_offer */*offer*/, UInt32 actions)
{
    static_cast<MDNDOffer*>(data)->m_sourceActions.set(actions);
}

void MDNDOffer::offer_action(void *data, wl_data_offer */*offer*/, UInt32 action)
{
    static_cast<MDNDOffer*>(data)->m_action = static_cast<Action>(action);
}

#include <CZ/Marco/Protocols/Wayland/MWlDataDevice.h>
#include <CZ/Marco/Roles/MSurface.h>
#include <CZ/Marco/MDNDOffer.h>
#include <CZ/Core/CZWeak.h>
#include <unordered_map>

using namespace CZ;

// Offers announced (data_offer) but not yet used by enter/selection.
static std::unordered_map<wl_data_offer*, MDNDOffer*> s_announced;

// Current drag & drop session over one of our surfaces.
static MDNDOffer *s_dndOffer { nullptr };
static CZWeak<MSurface> s_dndSurface;
static SkPoint s_dndPos {};

// Current selection (clipboard) offer: kept only so it's destroyed when replaced.
static MDNDOffer *s_selectionOffer { nullptr };

static MDNDOffer *TakeAnnounced(wl_data_offer *offer) noexcept
{
    if (!offer)
        return nullptr;

    auto it { s_announced.find(offer) };

    if (it == s_announced.end())
        return nullptr;

    MDNDOffer *ret { it->second };
    s_announced.erase(it);
    return ret;
}

static void EndDND() noexcept
{
    MWlDataDevice::DestroyOffer(s_dndOffer);
    s_dndOffer = nullptr;
    s_dndSurface.reset();
}

void MWlDataDevice::data_offer(void */*data*/, wl_data_device */*device*/, wl_data_offer *offer)
{
    s_announced[offer] = CreateOffer(offer);
}

void MWlDataDevice::enter(void */*data*/, wl_data_device */*device*/, UInt32 serial, wl_surface *surface, wl_fixed_t x, wl_fixed_t y, wl_data_offer *offer)
{
    EndDND();
    s_dndOffer = TakeAnnounced(offer);

    if (!s_dndOffer || !surface)
        return;

    SetEnterSerial(*s_dndOffer, serial);
    s_dndSurface.reset(static_cast<MSurface*>(wl_surface_get_user_data(surface)));
    s_dndPos = SkPoint::Make(wl_fixed_to_double(x), wl_fixed_to_double(y));

    if (s_dndSurface)
        s_dndSurface->onDNDEnter.notify(*s_dndOffer, s_dndPos);
}

void MWlDataDevice::leave(void */*data*/, wl_data_device */*device*/)
{
    if (s_dndOffer && s_dndSurface)
        s_dndSurface->onDNDLeave.notify(*s_dndOffer);

    EndDND();
}

void MWlDataDevice::motion(void */*data*/, wl_data_device */*device*/, UInt32 /*time*/, wl_fixed_t x, wl_fixed_t y)
{
    s_dndPos = SkPoint::Make(wl_fixed_to_double(x), wl_fixed_to_double(y));

    if (s_dndOffer && s_dndSurface)
        s_dndSurface->onDNDMotion.notify(*s_dndOffer, s_dndPos);
}

void MWlDataDevice::drop(void */*data*/, wl_data_device */*device*/)
{
    if (s_dndOffer && s_dndSurface)
        s_dndSurface->onDNDDrop.notify(*s_dndOffer, s_dndPos);

    // No leave follows a drop.
    EndDND();
}

void MWlDataDevice::selection(void */*data*/, wl_data_device */*device*/, wl_data_offer *offer)
{
    DestroyOffer(s_selectionOffer);
    s_selectionOffer = TakeAnnounced(offer);
}

MDNDOffer *MWlDataDevice::CreateOffer(wl_data_offer *offer) noexcept
{
    return new MDNDOffer(offer);
}

void MWlDataDevice::DestroyOffer(MDNDOffer *offer) noexcept
{
    delete offer;
}

void MWlDataDevice::SetEnterSerial(MDNDOffer &offer, UInt32 serial) noexcept
{
    offer.m_enterSerial = serial;
}

#include <CZ/Marco/Protocols/WlrForeignToplevel/MWlrForeignToplevelHandle.h>
#include <CZ/Marco/Extensions/MForeignToplevelManager.h>
#include <CZ/Marco/Extensions/MForeignToplevel.h>
#include <CZ/Marco/MApp.h>

using namespace CZ;

void MForeignToplevelManager::init() noexcept
{
    auto app { MApp::Get() };

    if (!m_screenListener)
    {
        m_screenListener = app->onScreenUnplugged.subscribe(this, [this](MScreen &screen){

            for (auto &handle : m_incomplete)
                handle->m_pending.screens.erase(&screen);

            for (auto &handle : m_toplevels)
            {
                MWlrForeignToplevelHandle::output_leave(nullptr, handle->handle(), screen.wlOutput());
                MWlrForeignToplevelHandle::done(nullptr, handle->handle());
            }
        });
    }
}

void MForeignToplevelManager::unit() noexcept
{
    while (!m_incomplete.empty())
    {
        auto it { m_incomplete.begin() };
        delete *it;
        m_incomplete.erase(it);
    }

    while (!m_toplevels.empty())
    {
        auto it { m_toplevels.begin() };
        onRemoved.notify(*it);
        delete *it;
        m_toplevels.erase(it);
    }
}

#include <CZ/Marco/Protocols/WlrForeignToplevel/MWlrForeignToplevelHandle.h>
#include <CZ/Marco/Extensions/MForeignToplevel.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/MLog.h>
#include <CZ/Core/Utils/CZSetUtils.h>

using namespace CZ;

void MWlrForeignToplevelHandle::title(void */*data*/, zwlr_foreign_toplevel_handle_v1 *handle, const char *title)
{
    auto *res { static_cast<MForeignToplevel*>(zwlr_foreign_toplevel_handle_v1_get_user_data(handle)) };
    res->m_changes.add(MForeignToplevel::CHTitle);
    res->m_pending.title = title;
}

void MWlrForeignToplevelHandle::app_id(void */*data*/, zwlr_foreign_toplevel_handle_v1 *handle, const char *appId)
{
    auto *res { static_cast<MForeignToplevel*>(zwlr_foreign_toplevel_handle_v1_get_user_data(handle)) };
    res->m_changes.add(MForeignToplevel::CHAppId);
    res->m_pending.appId = appId;
}

void MWlrForeignToplevelHandle::output_enter(void */*data*/, zwlr_foreign_toplevel_handle_v1 *handle, wl_output *output)
{
    auto *res { static_cast<MForeignToplevel*>(zwlr_foreign_toplevel_handle_v1_get_user_data(handle)) };
    auto *screen { static_cast<MScreen*>(wl_output_get_user_data(output)) };
    res->m_changes.add(MForeignToplevel::CHScreens);
    res->m_pending.screens.emplace(screen);
}

void MWlrForeignToplevelHandle::output_leave(void */*data*/, zwlr_foreign_toplevel_handle_v1 *handle, wl_output *output)
{
    auto *res { static_cast<MForeignToplevel*>(zwlr_foreign_toplevel_handle_v1_get_user_data(handle)) };
    auto *screen { static_cast<MScreen*>(wl_output_get_user_data(output)) };
    res->m_changes.add(MForeignToplevel::CHScreens);
    res->m_pending.screens.erase(screen);
}

void MWlrForeignToplevelHandle::state(void */*data*/, zwlr_foreign_toplevel_handle_v1 *handle, wl_array *state)
{
    auto *res { static_cast<MForeignToplevel*>(zwlr_foreign_toplevel_handle_v1_get_user_data(handle)) };
    res->m_changes.add(MForeignToplevel::CHState);
    res->m_pending.state.set(0);

    auto *arr { (zwlr_foreign_toplevel_handle_v1_state*) state->data };
    for (size_t i = 0; i < state->size/sizeof(zwlr_foreign_toplevel_handle_v1_state); i++)
        res->m_pending.state.add(1 << arr[i]);
}

void MWlrForeignToplevelHandle::done(void */*data*/, zwlr_foreign_toplevel_handle_v1 *handle)
{
    using CH = MForeignToplevel::Changes;

    auto *res { static_cast<MForeignToplevel*>(zwlr_foreign_toplevel_handle_v1_get_user_data(handle)) };

    if (res->m_isComplete)
    {
        CZBitset<CH> realChanges {};

        if (res->m_changes.has(CH::CHAppId) && res->m_pending.appId != res->m_current.appId)
        {
            realChanges.add(CH::CHAppId);
            res->m_current.appId = res->m_pending.appId;
            MLog(CZTrace, "Foreign toplevel changed appId: {}", res->m_current.appId);
        }

        if (res->m_changes.has(CH::CHTitle) && res->m_pending.title != res->m_current.title)
        {
            realChanges.add(CH::CHTitle);
            res->m_current.title = res->m_pending.title;
            MLog(CZTrace, "Foreign toplevel changed title: {}", res->m_current.title);
        }

        if (res->m_changes.has(CH::CHState) && res->m_pending.state != res->m_current.state)
        {
            realChanges.add(CH::CHState);
            res->m_current.state = res->m_pending.state;
            MLog(CZTrace, "Foreign toplevel changed state: {}", res->m_current.state.get());
        }

        if (res->m_changes.has(CH::CHScreens) && res->m_pending.screens != res->m_current.screens)
        {
            realChanges.add(CH::CHScreens);
            res->m_current.screens = res->m_pending.screens;
            MLog(CZTrace, "Foreign toplevel changed screens: {}", res->m_current.screens.size());
        }

        if (realChanges.get() != 0)
            res->onPropsChanged.notify(realChanges);
    }
    else
    {
        res->m_isComplete = true;
        auto app { MApp::Get() };
        res->m_changes.set(0);
        res->m_current = res->m_pending;
        app->ext.foreignToplevelManager.m_incomplete.erase(res);
        app->ext.foreignToplevelManager.m_toplevels.emplace(res);
        app->ext.foreignToplevelManager.onAdded.notify(res);
        MLog(CZTrace, "Foreign toplevel added Title: {}, AppId: {}",
            res->props().title, res->props().appId);
    }
}

void MWlrForeignToplevelHandle::closed(void */*data*/, zwlr_foreign_toplevel_handle_v1 *handle)
{
    auto app { MApp::Get() };
    auto *res { static_cast<MForeignToplevel*>(zwlr_foreign_toplevel_handle_v1_get_user_data(handle)) };

    if (res->m_isComplete)
    {
        MLog(CZTrace, "Foreign toplevel removed Title: {}, AppId: {}",
             res->props().title, res->props().appId);
        app->ext.foreignToplevelManager.onRemoved.notify(res);
        app->ext.foreignToplevelManager.m_toplevels.erase(res);
        delete res;
    }
    else
    {
        app->ext.foreignToplevelManager.m_incomplete.erase(res);
        delete res;
    }
}

void MWlrForeignToplevelHandle::parent(void *data, zwlr_foreign_toplevel_handle_v1 *handle, zwlr_foreign_toplevel_handle_v1 *parent)
{
    /* TODO */
    CZ_UNUSED(data)
    CZ_UNUSED(handle)
    CZ_UNUSED(parent)
}

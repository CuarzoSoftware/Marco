#include <CZ/Marco/Protocols/WlrForeignToplevel/MWlrForeignToplevelManager.h>
#include <CZ/Marco/Protocols/WlrForeignToplevel/MWlrForeignToplevelHandle.h>
#include <CZ/Marco/Extensions/MForeignToplevelManager.h>
#include <CZ/Marco/Extensions/MForeignToplevel.h>
#include <CZ/Marco/MApp.h>

using namespace CZ;

void MWlrForeignToplevelManager::bound() noexcept
{
    auto app { MApp::Get() };
    app->ext.foreignToplevelManager.init();
}

void MWlrForeignToplevelManager::toplevel(void */*data*/, zwlr_foreign_toplevel_manager_v1 */*manager*/, zwlr_foreign_toplevel_handle_v1 *toplevel)
{
    auto app { MApp::Get() };
    auto *handle { new MForeignToplevel(toplevel) };
    zwlr_foreign_toplevel_handle_v1_add_listener(toplevel, &MWlrForeignToplevelHandle::Listener, handle);
    app->ext.foreignToplevelManager.m_incomplete.emplace(handle);

    // UAPI is notified after the first handle.done event
}

void MWlrForeignToplevelManager::finished(void */*data*/, zwlr_foreign_toplevel_manager_v1 */*manager*/)
{
    auto app { MApp::Get() };
    app->ext.foreignToplevelManager.unit();
}

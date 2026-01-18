#include <CZ/Marco/Protocols/LvrPrivateHandle/MLvrPrivateHandleManager.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/MLog.h>

using namespace CZ;

void MLvrPrivateHandleManager::handle(void *, lvr_private_handle_manager *manager, const char *id)
{
    MLog(CZInfo, "Server App ID: {}", id);
    auto app { MApp::Get() };
    app->m_privateHandle = id;
    app->onPrivateHandleChanged.notify();
    lvr_private_handle_manager_destroy(manager);
}

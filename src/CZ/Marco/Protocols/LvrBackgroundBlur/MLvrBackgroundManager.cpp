#include <CZ/Marco/Protocols/LvrBackgroundBlur/MLvrBackgroundManager.h>
#include <CZ/Marco/MApp.h>

using namespace CZ;

void MLvrBackgroundBlurManager::masking_capabilities(void *data, lvr_background_blur_manager *blurManager, UInt32 caps)
{
    CZ_UNUSED(data)
    CZ_UNUSED(blurManager)
    MApp::Get()->m_maskingcaps.set(caps);
}

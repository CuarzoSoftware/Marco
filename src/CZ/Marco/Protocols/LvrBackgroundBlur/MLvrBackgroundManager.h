#ifndef MLVRBACKGROUNDMANAGER_H
#define MLVRBACKGROUNDMANAGER_H

#include <CZ/Marco/Protocols/LvrBackgroundBlur/lvr-background-blur-client.h>
#include <CZ/Marco/Marco.h>

namespace CZ
{
struct MLvrBackgroundBlurManager
{
    static void masking_capabilities(void *data, lvr_background_blur_manager *blurManager, UInt32 caps);

    static constexpr lvr_background_blur_manager_listener Listener
    {
        .masking_capabilities = masking_capabilities
    };
};
}
#endif // MLVRBACKGROUNDMANAGER_H

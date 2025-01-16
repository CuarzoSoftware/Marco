#ifndef MIMAGELOADER_H
#define MIMAGELOADER_H

#include <Marco/Marco.h>
#include <Marco/MApplication.h>
#include <AK/utils/AKImageLoader.h>

class Marco::MImageLoader
{
public:
    static sk_sp<SkImage> loadFile(const std::filesystem::path &path) noexcept
    {
        assert(app() && "Textures can only be allocated after creating an MApplication instance");
        return AK::AKImageLoader::loadFile(app()->graphics().skContext.get(), path);
    }
};

#endif // MIMAGELOADER_H

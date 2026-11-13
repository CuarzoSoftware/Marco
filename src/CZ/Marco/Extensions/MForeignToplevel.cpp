#include <CZ/Marco/MLog.h>
#include <CZ/Marco/Extensions/MForeignToplevel.h>
#include <CZ/Marco/Roles/MSurface.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/Protocols/ImageCapture/ext-foreign-toplevel-list-v1-client.h>
#include <unordered_map>
#include <CZ/Marco/Protocols/ImageCapture/ext-image-capture-source-v1-client.h>
#include <CZ/Marco/Protocols/ImageCapture/ext-image-copy-capture-v1-client.h>
#include <CZ/Core/CZTimer.h>
#include <CZ/Ream/RCore.h>
#include <CZ/Ream/RDevice.h>
#include <CZ/Ream/RImage.h>
#include <drm_fourcc.h>
#include <sys/mman.h>
#include <unistd.h>
#include <list>

using namespace CZ;

void MForeignToplevel::setMaximized(bool maximized) noexcept
{
    if (maximized)
        zwlr_foreign_toplevel_handle_v1_set_maximized(handle());
    else
        zwlr_foreign_toplevel_handle_v1_unset_maximized(handle());
}

void MForeignToplevel::setMinimized(bool minimized) noexcept
{
    if (minimized)
        zwlr_foreign_toplevel_handle_v1_set_minimized(handle());
    else
        zwlr_foreign_toplevel_handle_v1_unset_minimized(handle());
}

void MForeignToplevel::activate() noexcept
{
    auto app { MApp::Get() };

    if (app->wl.seat)
        zwlr_foreign_toplevel_handle_v1_activate(handle(), app->wl.seat);
}

void MForeignToplevel::close() noexcept
{
    zwlr_foreign_toplevel_handle_v1_close(handle());
}

bool MForeignToplevel::setFullscreen(MScreen *screen) noexcept
{
    if (handle().version() < 2)
        return false;

    zwlr_foreign_toplevel_handle_v1_set_fullscreen(handle(), screen ? screen->wlOutput() : nullptr);
    return true;
}

void MForeignToplevel::setRect(MSurface &surface, SkIRect rect) noexcept
{
    m_surface = &surface;
    m_rect = rect;
    zwlr_foreign_toplevel_handle_v1_set_rectangle(handle(), surface.wlSurface(),
                                                  rect.x(), rect.y(), rect.width(), rect.height());
}

MForeignToplevel::~MForeignToplevel() noexcept
{
    zwlr_foreign_toplevel_handle_v1_destroy(handle());
}

/***************************************** IMAGE CAPTURE *****************************************/

namespace
{
    /**
     * A one-shot capture of a toplevel (see MForeignToplevel::captureImage()): source -> session ->
     * (constraints) -> shm buffer -> frame -> image.
     */
    struct ToplevelCapture
    {
        CZWeak<MForeignToplevel> owner;
        std::function<void(std::shared_ptr<RImage>)> done;
        ext_image_capture_source_v1 *source {};
        ext_image_copy_capture_session_v1 *session {};
        ext_image_copy_capture_frame_v1 *frame {};
        wl_buffer *buffer {};
        int fd { -1 };
        void *data { MAP_FAILED };
        size_t size {};
        UInt32 width {}, height {};
        bool argb {}, xrgb {}, started {}, finished {};

        ~ToplevelCapture() noexcept
        {
            if (frame) ext_image_copy_capture_frame_v1_destroy(frame);
            if (session) ext_image_copy_capture_session_v1_destroy(session);
            if (source) ext_image_capture_source_v1_destroy(source);
            if (buffer) wl_buffer_destroy(buffer);
            if (data != MAP_FAILED) munmap(data, size);
            if (fd >= 0) close(fd);
        }

        void finish(std::shared_ptr<RImage> image) noexcept;
        void captureFrame() noexcept;
        std::shared_ptr<RImage> makeImage() noexcept;
    };

    std::list<std::unique_ptr<ToplevelCapture>> s_captures;

    void ToplevelCapture::finish(std::shared_ptr<RImage> image) noexcept
    {
        if (finished)
            return;

        finished = true;

        if (owner && done)
            done(image);

        // Destroyed after this event (its protocol objects may be in use by the current dispatch)
        auto *self { this };
        CZTimer::OneShot(0, [self](CZTimer*) {
            s_captures.remove_if([self](const auto &c){ return c.get() == self; });
        });
    }

    std::shared_ptr<RImage> ToplevelCapture::makeImage() noexcept
    {
        auto *device { RCore::Get()->mainDevice() };
        const auto &formats { device->textureFormats().formats() };
        const auto fmt { formats.find(DRM_FORMAT_ARGB8888) };

        if (fmt == formats.end())
            return {};

        // XRGB: the X bytes may be anything, make them opaque
        if (!argb)
        {
            auto *px { static_cast<UInt8*>(data) };

            for (size_t i = 3; i < size; i += 4)
                px[i] = 0xFF;
        }

        RPixelBufferInfo info {};
        info.pixels = static_cast<UInt8*>(data);
        info.stride = width * 4;
        info.format = DRM_FORMAT_ARGB8888;
        info.size.set(width, height);

        RImageConstraints cons {};
        cons.allocator = device;
        cons.caps[device] = RImageCap_Src;
        return RImage::MakeFromPixels(info, *fmt, &cons);
    }

    const ext_image_copy_capture_frame_v1_listener FrameListener
    {
        .transform = [](void*, ext_image_copy_capture_frame_v1*, uint32_t) {},
        .damage = [](void*, ext_image_copy_capture_frame_v1*, int32_t, int32_t, int32_t, int32_t) {},
        .presentation_time = [](void*, ext_image_copy_capture_frame_v1*, uint32_t, uint32_t, uint32_t) {},
        .ready = [](void *data, ext_image_copy_capture_frame_v1*) {
            auto *c { static_cast<ToplevelCapture*>(data) };
            c->finish(c->makeImage());
        },
        .failed = [](void *data, ext_image_copy_capture_frame_v1*, uint32_t reason) {
            MLog(CZDebug, "captureImage: frame failed ({})", reason);
            static_cast<ToplevelCapture*>(data)->finish(nullptr);
        }
    };

    void ToplevelCapture::captureFrame() noexcept
    {
        auto app { MApp::Get() };

        if (!app->wl.shm || width == 0 || height == 0 || (!argb && !xrgb))
        {
            finish(nullptr);
            return;
        }

        const UInt32 stride { width * 4 };
        size = size_t(stride) * height;
        fd = memfd_create("marco-toplevel-capture", MFD_CLOEXEC);

        if (fd < 0 || ftruncate(fd, size) < 0 || (data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
        {
            finish(nullptr);
            return;
        }

        auto *pool { wl_shm_create_pool(app->wl.shm, fd, size) };
        buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, argb ? WL_SHM_FORMAT_ARGB8888 : WL_SHM_FORMAT_XRGB8888);
        wl_shm_pool_destroy(pool);

        frame = ext_image_copy_capture_session_v1_create_frame(session);
        ext_image_copy_capture_frame_v1_add_listener(frame, &FrameListener, this);
        ext_image_copy_capture_frame_v1_attach_buffer(frame, buffer);
        ext_image_copy_capture_frame_v1_damage_buffer(frame, 0, 0, width, height);
        ext_image_copy_capture_frame_v1_capture(frame);
    }

    const ext_image_copy_capture_session_v1_listener SessionListener
    {
        .buffer_size = [](void *data, ext_image_copy_capture_session_v1*, uint32_t w, uint32_t h) {
            auto *c { static_cast<ToplevelCapture*>(data) };
            c->width = w;
            c->height = h;
        },
        .shm_format = [](void *data, ext_image_copy_capture_session_v1*, uint32_t format) {
            auto *c { static_cast<ToplevelCapture*>(data) };
            if (format == WL_SHM_FORMAT_ARGB8888) c->argb = true;
            else if (format == WL_SHM_FORMAT_XRGB8888) c->xrgb = true;
        },
        .dmabuf_device = [](void*, ext_image_copy_capture_session_v1*, wl_array*) {},
        .dmabuf_format = [](void*, ext_image_copy_capture_session_v1*, uint32_t, wl_array*) {},
        .done = [](void *data, ext_image_copy_capture_session_v1*) {
            // The first constraints: capture (one frame)
            auto *c { static_cast<ToplevelCapture*>(data) };

            if (!c->started)
            {
                c->started = true;
                c->captureFrame();
            }
        },
        .stopped = [](void *data, ext_image_copy_capture_session_v1*) {
            MLog(CZDebug, "captureImage: session stopped");
            static_cast<ToplevelCapture*>(data)->finish(nullptr);
        }
    };
}

/* ext-foreign-toplevel-list handles by identifier (to create capture sources for wlr handles) */

namespace
{
    struct ExtToplevel
    {
        ext_foreign_toplevel_handle_v1 *handle {};
        std::string identifier;
    };

    std::list<std::unique_ptr<ExtToplevel>> s_extToplevels;

    const ext_foreign_toplevel_handle_v1_listener ExtToplevelListener
    {
        .closed = [](void *data, ext_foreign_toplevel_handle_v1 *handle) {
            ext_foreign_toplevel_handle_v1_destroy(handle);
            s_extToplevels.remove_if([data](const auto &t){ return t.get() == data; });
        },
        .done = [](void*, ext_foreign_toplevel_handle_v1*) {},
        .title = [](void*, ext_foreign_toplevel_handle_v1*, const char*) {},
        .app_id = [](void*, ext_foreign_toplevel_handle_v1*, const char*) {},
        .identifier = [](void *data, ext_foreign_toplevel_handle_v1*, const char *identifier) {
            static_cast<ExtToplevel*>(data)->identifier = identifier ? identifier : "";
        }
    };

    const ext_foreign_toplevel_list_v1_listener ExtToplevelListListener
    {
        .toplevel = [](void*, ext_foreign_toplevel_list_v1*, ext_foreign_toplevel_handle_v1 *handle) {
            auto toplevel { std::make_unique<ExtToplevel>() };
            toplevel->handle = handle;
            ext_foreign_toplevel_handle_v1_add_listener(handle, &ExtToplevelListener, toplevel.get());
            s_extToplevels.emplace_back(std::move(toplevel));
        },
        .finished = [](void*, ext_foreign_toplevel_list_v1*) {}
    };

    ext_foreign_toplevel_handle_v1 *FindExtToplevel(const std::string &identifier) noexcept
    {
        for (const auto &t : s_extToplevels)
            if (!identifier.empty() && t->identifier == identifier)
                return t->handle;

        return nullptr;
    }
}

void MForeignToplevel::ToplevelListBound(ext_foreign_toplevel_list_v1 *list) noexcept
{
    ext_foreign_toplevel_list_v1_add_listener(list, &ExtToplevelListListener, nullptr);
}

bool MForeignToplevel::captureImage(std::function<void(std::shared_ptr<RImage>)> done) noexcept
{
    auto app { MApp::Get() };

    if (!app || !app->wl.toplevelCaptureSourceManager || !app->wl.imageCopyCaptureManager || !app->wl.shm || !m_handle)
    {
        MLog(CZDebug, "captureImage: unsupported (capture source {}, copy capture {}, shm {})",
             bool(app && app->wl.toplevelCaptureSourceManager), bool(app && app->wl.imageCopyCaptureManager), bool(app && app->wl.shm));
        return false;
    }

    // Its ext-foreign-toplevel-list handle (unknown: the compositor doesn't send identifiers)
    auto *extHandle { FindExtToplevel(m_current.identifier) };

    if (!extHandle)
    {
        MLog(CZDebug, "captureImage: no ext-foreign-toplevel-list handle with identifier '{}'", m_current.identifier);
        return false;
    }

    auto capture { std::make_unique<ToplevelCapture>() };
    capture->owner.reset(this);
    capture->done = std::move(done);
    capture->source = ext_foreign_toplevel_image_capture_source_manager_v1_create_source(app->wl.toplevelCaptureSourceManager, extHandle);
    capture->session = ext_image_copy_capture_manager_v1_create_session(app->wl.imageCopyCaptureManager, capture->source, 0);
    ext_image_copy_capture_session_v1_add_listener(capture->session, &SessionListener, capture.get());
    s_captures.emplace_back(std::move(capture));
    return true;
}

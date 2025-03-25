#ifndef MSURFACEPRIVATE_H
#define MSURFACEPRIVATE_H

#include <Marco/roles/MSurface.h>

class AK::MSurface::Imp
{
public:

    enum TmpFlags
    {
        ScaleChanged              = 1 << 0,
        ScreensChanged            = 1 << 1,
        PreferredScaleChanged     = 1 << 2,
    };

    Imp(MSurface &obj) noexcept;
    MSurface &obj;

    // Cleared after onUpdate
    AKBitset<TmpFlags> tmpFlags;

    // Kay
    AKScene scene;
    AKWeak<AKTarget> target;
    AKContainer root;

    // Current wl_surface scale factor
    Int32 scale { 1 };

    // If not set by the compositor the max wl_output scale is used
    Int32 preferredBufferScale { -1 };

    // Intersected screens
    std::set<MScreen*> screens;

    SkISize size { 0, 0 };
    SkISize bufferSize { 0, 0 };
    SkISize viewportSize { -1, -1 };
    bool pendingUpdate { true };

    UInt32 callbackSendMs { 0 };
    wl_callback *wlCallback { nullptr };
    wl_surface *wlSurface { nullptr };
    wp_viewport *wlViewport { nullptr };

    wl_egl_window *eglWindow { nullptr };
    EGLSurface eglSurface { EGL_NO_SURFACE };
    sk_sp<SkSurface> skSurface;

    Role role;
    size_t appLink;

    // Creates a wl_surface and wp_viewporter
    // Prev surface and viewporter are destroyed
    void createSurface() noexcept;
    bool createCallback() noexcept;

    /**
     * @brief Resize the wl_egl_window
     *
     * @return true if resized, false if the size is the same.
     */
    bool resizeBuffer(const SkISize &size) noexcept;

    static void wl_surface_enter(void *data, wl_surface *surface, wl_output *output);
    static void wl_surface_leave(void *data,wl_surface *surface, wl_output *output);
    static void wl_surface_preferred_buffer_scale(void *data, wl_surface *surface, Int32 factor);
    static void wl_surface_preferred_buffer_transform(void *data, wl_surface *surface, UInt32 transform);
    static void wl_callback_done(void *data, wl_callback *callback, UInt32 ms);
};

#endif // MSURFACEPRIVATE_H

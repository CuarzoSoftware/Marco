#include <AK/AKGLContext.h>
#include <Marco/roles/MSurface.h>
#include <Marco/MApplication.h>
#include <AK/AKColors.h>

#include <include/gpu/ganesh/gl/GrGLBackendSurface.h>
#include <include/gpu/ganesh/GrBackendSurface.h>
#include <include/gpu/ganesh/GrRecordingContext.h>
#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/gpu/ganesh/gl/GrGLTypes.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h>
#include <include/core/SkColorSpace.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

using namespace Marco;
using namespace AK;

static wl_surface_listener wlSurfaceListener;
static wl_callback_listener wlCallbackListener;

MSurface::~MSurface()
{
    app()->m_surfaces[m_appLink] = app()->m_surfaces.back();
    app()->m_surfaces.pop_back();
    ak.scene.destroyTarget(ak.target);
    resizeBuffer({0, 0});

    if (wl.callback)
        wl_callback_destroy(wl.callback);

    if (wl.surface)
        wl_surface_destroy(wl.surface);
}

void MSurface::update() noexcept
{
    cl.pendingUpdate = true;
    app()->update();
}

void MSurface::setAutoMinSize() noexcept
{
    layout().setWidthAuto();
    layout().setHeightAuto();
    ak.root.layout().calculate();
    layout().setMinWidth(layout().calculatedWidth());
    layout().setMinHeight(layout().calculatedHeight());
}

MSurface::MSurface(Role role) noexcept : AK::AKSolidColor(AK::AKColor::GrayLighten5), m_role(role)
{
    wlSurfaceListener.enter = wl_surface_enter;
    wlSurfaceListener.leave = wl_surface_leave;
    wlSurfaceListener.preferred_buffer_scale = wl_surface_preferred_buffer_scale;
    wlSurfaceListener.preferred_buffer_transform = wl_surface_preferred_buffer_transform;
    wlCallbackListener.done = wl_callback_done;

    wl.surface = wl_compositor_create_surface(app()->wayland().compositor);
    wl_surface_add_listener(wl.surface, &wlSurfaceListener, this);
    wl_surface_set_user_data(wl.surface, this);

    m_appLink = app()->m_surfaces.size();
    app()->m_surfaces.push_back(this);
    setParent(&ak.root);
    enableChildrenClipping(true);
    ak.target.reset(ak.scene.createTarget());
    ak.scene.setRoot(&ak.root);

    ak.target->on.markedDirty.subscribe(this, [this](AK::AKTarget&){
        update();
    });

    app()->on.screenUnplugged.subscribe(this, [this](MScreen &screen){
        wl_surface_leave(this, wl.surface, screen.wlOutput());
    });
}

void MSurface::wl_surface_enter(void *data, wl_surface */*surface*/, wl_output *output)
{
    MSurface &surface { *static_cast<MSurface*>(data) };
    MScreen *screen { static_cast<MScreen*>(wl_output_get_user_data(output)) };

    if (surface.se.screens.contains(screen))
        return;

    surface.se.screens.insert(screen);
    surface.se.changes.set(Se_Screens);
    surface.on.enteredScreen.notify(*screen);

    if (surface.se.preferredBufferScale != -1)
        surface.update();

    // TODO: Listen for screen prop changes
}

void MSurface::wl_surface_leave(void *data, wl_surface */*surface*/, wl_output *output)
{
    MSurface &surface { *static_cast<MSurface*>(data) };
    MScreen *screen { static_cast<MScreen*>(wl_output_get_user_data(output)) };
    auto it = surface.se.screens.find(screen);

    if (it == surface.se.screens.end())
        return;

    surface.se.screens.erase(it);
    surface.se.changes.set(Se_Screens);
    surface.on.leftScreen.notify(*screen);

    if (surface.se.preferredBufferScale != -1)
        surface.update();
}

void MSurface::wl_surface_preferred_buffer_scale(void *data, wl_surface */*surface*/, Int32 factor)
{
    MSurface &surface { *static_cast<MSurface*>(data) };
    if (factor <= 0)
        factor = 1;

    if (surface.se.preferredBufferScale == factor)
        return;

    surface.se.preferredBufferScale = factor;
    surface.se.changes.set(Se_PrefferredScale);
    surface.update();
}

void MSurface::wl_surface_preferred_buffer_transform(void */*data*/, wl_surface */*surface*/, UInt32 /*transform*/)
{
    // TODO
}

void MSurface::wl_callback_done(void *data, wl_callback *callback, UInt32 ms)
{
    MSurface &surface { *static_cast<MSurface*>(data) };
    wl_callback_destroy(callback);
    surface.wl.callback = nullptr;
    surface.on.presented.notify(ms);

    if (surface.cl.pendingUpdate || surface.ak.target->isDirty())
        surface.update();
}

bool MSurface::createCallback() noexcept
{
    if (wl.callback)
        return false;

    wl.callbackSendMs = AKTime::ms();
    wl.callback = wl_surface_frame(wl.surface);
    wl_callback_add_listener(wl.callback, &wlCallbackListener, this);
    return true;
}

bool MSurface::resizeBuffer(const SkISize &size) noexcept
{

    const SkISize bufferSize { size.width() * cl.scale , size.height() * cl.scale };

    if (bufferSize == cl.bufferSize)
        return false;

    cl.size = size;
    cl.bufferSize = bufferSize;

    eglMakeCurrent(app()->graphics().eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, app()->graphics().eglContext);

    if (size.isEmpty())
    {
        gl.skSurface.reset();

        if (gl.eglSurface != EGL_NO_SURFACE)
        {
            eglDestroySurface(app()->gl.eglDisplay, gl.eglSurface);
            gl.eglSurface = EGL_NO_SURFACE;
        }

        if (gl.eglWindow)
        {
            wl_egl_window_destroy(gl.eglWindow);
            gl.eglWindow = nullptr;
        }

        return true;
    }

    if (gl.eglWindow)
        wl_egl_window_resize(gl.eglWindow, bufferSize.width(), bufferSize.height(), 0, 0);
    else
    {
        gl.eglWindow = wl_egl_window_create(wl.surface, bufferSize.width(), bufferSize.height());
        gl.eglSurface = eglCreateWindowSurface(app()->gl.eglDisplay, app()->gl.eglConfig, (EGLNativeWindowType)gl.eglWindow, NULL);
        assert("Failed to create EGLSurface for MSurface" && gl.eglSurface != EGL_NO_SURFACE);
    }

    static const SkSurfaceProps skSurfaceProps(0, kUnknown_SkPixelGeometry);

    static constexpr GrGLFramebufferInfo fbInfo
    {
        .fFBOID = 0,
        .fFormat = GL_RGBA8_OES
    };

    const GrBackendRenderTarget backendTarget = GrBackendRenderTargets::MakeGL(
        bufferSize.width(),
        bufferSize.height(),
        0, 0,
        fbInfo);

    gl.skSurface = SkSurfaces::WrapBackendRenderTarget(
        AK::AKApp()->glContext()->skContext().get(),
        backendTarget,
        GrSurfaceOrigin::kBottomLeft_GrSurfaceOrigin,
        SkColorType::kRGBA_8888_SkColorType,
        SkColorSpace::MakeSRGB(),
        &skSurfaceProps);

    return true;
}

void MSurface::onUpdate() noexcept
{
    if (se.changes.test(Se_PrefferredScale))
    {
        cl.scale = se.preferredBufferScale;
        cl.changes.set(Cl_Scale);
    }
    else if (se.preferredBufferScale == -1 && se.changes.test(Se_Screens))
    {
        Int32 maxScale { 1 };

        for (const auto &screen : se.screens)
            if (screen->props().scale > maxScale)
                maxScale = screen->props().scale;

        if (maxScale != cl.scale)
        {
            cl.scale = maxScale;
            cl.changes.set(Cl_Scale);
        }
    }
}

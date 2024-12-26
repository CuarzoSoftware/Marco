#include <Marco/roles/MToplevel.h>
#include <Marco/MApplication.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <include/gpu/ganesh/SkSurfaceGanesh.h>
#include <include/core/SkColorSpace.h>
#include <iostream>

using namespace Marco;

static xdg_surface_listener xdgSurfaceListener;
static xdg_toplevel_listener xdgToplevelListener;

static SkSurfaceProps skSurfaceProps(0, kUnknown_SkPixelGeometry);

MToplevel::MToplevel() noexcept : MSurface(Role::Toplevel)
{
    xdgSurfaceListener.configure = xdg_surface_configure;
    xdgToplevelListener.configure = xdg_toplevel_configure;
    xdgToplevelListener.configure_bounds = xdg_toplevel_configure_bounds;
    xdgToplevelListener.close = xdg_toplevel_close;
    xdgToplevelListener.wm_capabilities = xdg_toplevel_wm_capabilities;

    m_xdgSurface = xdg_wm_base_get_xdg_surface(app()->wayland().xdgWmBase, m_wlSurface);
    xdg_surface_add_listener(m_xdgSurface, &xdgSurfaceListener, this);

    m_xdgToplevel = xdg_surface_get_toplevel(m_xdgSurface);
    xdg_toplevel_add_listener(m_xdgToplevel, &xdgToplevelListener, this);
    xdg_toplevel_set_app_id(m_xdgToplevel, app()->appId().c_str());
    app()->on.appIdChanged.subscribe(this, [this](const std::string &appId){
        xdg_toplevel_set_app_id(m_xdgToplevel, appId.c_str());
    });

    m_changes.set(CHPendingInitialNullAttach);
}

MToplevel::~MToplevel() noexcept
{
    if (m_xdgToplevel)
        xdg_toplevel_destroy(m_xdgToplevel);

    if (m_xdgSurface)
        xdg_surface_destroy(m_xdgSurface);

    destroySurface();
}

void MToplevel::xdg_surface_configure(void *data, xdg_surface */*xdgSurface*/, UInt32 serial)
{
    auto &role { *static_cast<MToplevel*>(data) };
    role.m_changes.set(CHConfigurationSerial);
    role.m_changes.set(CHPendingInitialConfiguration, false);
    role.m_conf.serial = serial;
    role.updateLater();
}

void MToplevel::xdg_toplevel_configure(void *data, xdg_toplevel */*xdgToplevel*/, Int32 width, Int32 height, wl_array *states)
{
    auto &role { *static_cast<MToplevel*>(data) };
    role.m_changes.set(CHConfigurationState);
    role.m_changes.set(CHConfigurationSize);
    role.m_conf.states.set(0);
    const UInt32 *stateVals = (UInt32*)states->data;
    for (UInt32 i = 0; i < states->size/sizeof(*stateVals); i++)
        role.m_conf.states.add(1 << stateVals[i]);
    role.m_conf.windowSize.fWidth = width;
    role.m_conf.windowSize.fHeight = height;
}

void MToplevel::xdg_toplevel_close(void *data, xdg_toplevel *xdgToplevel)
{

}

void MToplevel::xdg_toplevel_configure_bounds(void *data, xdg_toplevel *xdgToplevel, Int32 width, Int32 height)
{

}

void MToplevel::xdg_toplevel_wm_capabilities(void *data, xdg_toplevel *xdgToplevel, wl_array *capabilities)
{

}

void MToplevel::handleChanges() noexcept
{
    if (!m_changes.test(CHUpdateLater))
        return;

    m_changes.set(CHUpdateLater, false);

    handleConfigurationChange();
    handleDimensionsChange();
    handleVisibilityChange();

    if (!m_visible || m_changes.test(CHPendingInitialConfiguration))
        return;

    render();
}

void MToplevel::handleConfigurationChange() noexcept
{
    if (m_changes.test(CHConfigurationState))
    {
        m_changes.set(CHConfigurationState, false);
        m_states = m_conf.states;
    }

    if (m_changes.test(CHConfigurationSize))
    {
        m_changes.set(CHConfigurationSize, false);
        m_windowSize = m_conf.windowSize;
    }

    if (m_changes.test(CHConfigurationSerial))
    {
        m_changes.set(CHConfigurationSerial, false);
        xdg_surface_ack_configure(m_xdgSurface, m_conf.serial);
    }
}

void MToplevel::handleVisibilityChange() noexcept
{
    if (!m_changes.test(CHVisibility))
        return;

    m_changes.set(CHVisibility, false);

    if (m_visible)
    {
        if (m_changes.test(CHPendingInitialNullAttach))
        {
            m_changes.set(CHPendingInitialNullAttach, false);

            if (app()->wayland().compositor.version() >= 3)
                wl_surface_set_buffer_scale(m_wlSurface, m_scale);

            wl_surface_attach(m_wlSurface, NULL, 0, 0);
            wl_surface_commit(m_wlSurface);
        }
    }
    else
    {
        if (!m_changes.test(CHPendingInitialNullAttach))
        {
            m_changes.set(CHPendingInitialNullAttach);
            wl_surface_attach(m_wlSurface, NULL, 0, 0);
            wl_surface_commit(m_wlSurface);
        }
    }
}

void MToplevel::handleDimensionsChange() noexcept
{
    const SkISize prevSize { m_bufferSize };

    if (m_changes.test(CHPreferredBufferScale))
    {
        m_changes.set(CHPreferredBufferScale, false);
        m_scale = m_preferredBufferScale;
    }

    if (m_scale <= 0)
        m_scale = 1;

    if (m_windowSize.width() <= 0)
        m_windowSize.fWidth = 512;

    if (m_windowSize.height() <= 0)
        m_windowSize.fHeight = 512;

    if (m_conf.windowSize.fWidth > 0)
        m_windowSize.fWidth = m_conf.windowSize.fWidth;

    if (m_conf.windowSize.fHeight > 0)
        m_windowSize.fHeight = m_conf.windowSize.fHeight;

    m_surfaceSize = m_windowSize;
    m_bufferSize = {m_surfaceSize.fWidth * scale(), m_surfaceSize.fHeight * scale() };

    if (prevSize != m_bufferSize)
        m_changes.set(CHSize);
}

void MToplevel::createSurface() noexcept
{
    if (m_wlSurface)
        return;

    m_wlSurface = wl_compositor_create_surface(app()->wayland().compositor);
}

void MToplevel::destroySurface() noexcept
{
    eglMakeCurrent(app()->graphics().eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, app()->graphics().eglContext);

    if (m_eglSurface != EGL_NO_SURFACE)
    {
        eglDestroySurface(app()->graphics().eglDisplay, m_eglSurface);
        m_eglSurface = EGL_NO_SURFACE;
    }

    if (m_eglWindow)
    {
        wl_egl_window_destroy(m_eglWindow);
        m_eglWindow = nullptr;
    }

    if (m_wlSurface)
    {
        wl_surface_destroy(m_wlSurface);
        m_wlSurface = nullptr;
    }
}

void MToplevel::render() noexcept
{
    if (!createCallback())
        return;

    if (!m_eglWindow)
    {
        m_eglWindow = wl_egl_window_create(m_wlSurface, m_bufferSize.width(), m_bufferSize.height());
        m_eglSurface = eglCreateWindowSurface(app()->graphics().eglDisplay, app()->graphics().eglConfig, m_eglWindow, NULL);
        assert("Failed to create EGLSurface" && m_eglSurface != EGL_NO_SURFACE);
        m_changes.set(CHSize);
    }
    else if (m_changes.test(CHSize))
    {
        wl_egl_window_resize(m_eglWindow, m_bufferSize.width(), m_bufferSize.height(), 0, 0);
    }

    eglMakeCurrent(app()->graphics().eglDisplay, m_eglSurface, m_eglSurface, app()->graphics().eglContext);
    eglSwapInterval(app()->graphics().eglDisplay, 0);

    if (m_changes.test(CHSize))
    {
        const GrGLFramebufferInfo fbInfo
        {
            .fFBOID = 0,
            .fFormat = GL_RGBA8_OES
        };

        const GrBackendRenderTarget backendTarget(
            m_bufferSize.width(),
            m_bufferSize.height(),
            0, 0,
            fbInfo);

        m_skSurface = SkSurfaces::WrapBackendRenderTarget(
            app()->graphics().skContext.get(),
            backendTarget,
            GrSurfaceOrigin::kBottomLeft_GrSurfaceOrigin,
            SkColorType::kRGBA_8888_SkColorType,
            SkColorSpace::MakeSRGB(),
            &skSurfaceProps);

        assert("Failed to create SkSurface" && m_skSurface.get());

        layout().setPosition(YGEdgeLeft, 0.f);
        layout().setPosition(YGEdgeTop, 0.f);
        layout().setWidth(m_surfaceSize.width());
        layout().setHeight(m_surfaceSize.height());
        m_target->setDstRect({ 0, 0, m_bufferSize.width(), m_bufferSize.height() });
        m_target->setViewport(SkRect::MakeWH(m_surfaceSize.width(), m_surfaceSize.height()));
        m_target->setSurface(m_skSurface);
        m_changes.set(CHSize, false);
    }

    if (app()->wayland().compositor.version() >= 3)
        wl_surface_set_buffer_scale(m_wlSurface, m_scale);

    EGLint bufferAge { 0 };
    eglQuerySurface(app()->graphics().eglDisplay, m_eglSurface, EGL_BUFFER_AGE_EXT, &bufferAge);
    m_target->setAge(bufferAge);
    SkRegion skDamage, skOpaque;
    m_target->outDamageRegion = &skDamage;
    m_target->outOpaqueRegion = &skOpaque;
    m_scene.render(m_target);

    if (skDamage.isEmpty())
    {
        wl_surface_commit(m_wlSurface);
        return;
    }
    else
    {
        wl_region *wlOpaqueRegion = wl_compositor_create_region(app()->wayland().compositor);
        SkRegion::Iterator opaqueIt(skOpaque);
        while (!opaqueIt.done())
        {
            wl_region_add(wlOpaqueRegion, opaqueIt.rect().x(), opaqueIt.rect().y(), opaqueIt.rect().width(), opaqueIt.rect().height());
            opaqueIt.next();
        }
        wl_surface_set_opaque_region(m_wlSurface, wlOpaqueRegion);
        wl_region_destroy(wlOpaqueRegion);

        EGLint *damageRects { new EGLint[skDamage.computeRegionComplexity() * 4] };
        EGLint *rectsIt = damageRects;
        SkRegion::Iterator damageIt(skDamage);
        while (!damageIt.done())
        {
            *rectsIt = damageIt.rect().x() * m_scale;
            rectsIt++;
            *rectsIt = (m_surfaceSize.height() - damageIt.rect().height() - damageIt.rect().y()) * m_scale;
            rectsIt++;
            *rectsIt = damageIt.rect().width() * m_scale;
            rectsIt++;
            *rectsIt = damageIt.rect().height() * m_scale;
            rectsIt++;
            damageIt.next();
        }

        assert(app()->graphics().eglSwapBuffersWithDamageKHR(app()->graphics().eglDisplay, m_eglSurface, damageRects, skDamage.computeRegionComplexity()) == EGL_TRUE);
        delete []damageRects;
    }

    //eglSwapBuffers(app()->graphics().eglDisplay, m_eglSurface);
}

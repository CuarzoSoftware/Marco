#include <AK/AKLog.h>
#include <Marco/roles/MToplevel.h>
#include <Marco/MApplication.h>
#include <Marco/MTheme.h>
#include <AK/events/AKStateActivatedEvent.h>
#include <AK/events/AKStateDeactivatedEvent.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h>
#include <include/core/SkColorSpace.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <iostream>

using namespace Marco;
using namespace AK;

static xdg_surface_listener xdgSurfaceListener;
static xdg_toplevel_listener xdgToplevelListener;
/*
static zxdg_toplevel_decoration_v1_listener xdgDecorationListener {
    .configure = [](auto, auto, auto) {}
};*/

MToplevel::MToplevel() noexcept : MSurface(Role::Toplevel)
{
    xdgSurfaceListener.configure = xdg_surface_configure;
    xdgToplevelListener.configure = xdg_toplevel_configure;
    xdgToplevelListener.configure_bounds = xdg_toplevel_configure_bounds;
    xdgToplevelListener.close = xdg_toplevel_close;
    xdgToplevelListener.wm_capabilities = xdg_toplevel_wm_capabilities;

    wl.xdgSurface = xdg_wm_base_get_xdg_surface(app()->wayland().xdgWmBase, MSurface::wl.surface);
    xdg_surface_add_listener(wl.xdgSurface, &xdgSurfaceListener, this);
    wl.xdgToplevel = xdg_surface_get_toplevel(wl.xdgSurface);
    xdg_toplevel_add_listener(wl.xdgToplevel, &xdgToplevelListener, this);
    xdg_toplevel_set_app_id(wl.xdgToplevel, app()->appId().c_str());

    //if (app()->wayland().xdgDecorationManager)
    //    wl.xdgDecoration = zxdg_decoration_manager_v1_get_toplevel_decoration(app()->wayland().xdgDecorationManager, wl.xdgToplevel);

    app()->on.appIdChanged.subscribe(this, [this](const std::string &appId){
        xdg_toplevel_set_app_id(wl.xdgToplevel, appId.c_str());
    });

    ak.root.on.event.subscribe(this, [this](const AKEvent &event){

        if (event.type() == AKEvent::Type::Pointer)
        {
            if (event.subtype() == AKEvent::Subtype::Button)
                handleRootPointerButtonEvent(static_cast<const AKPointerButtonEvent&>(event));
            else if (event.subtype() == AKEvent::Subtype::Move)
                handleRootPointerMoveEvent(static_cast<const AKPointerMoveEvent&>(event));
        }
    });

    /* CSD */

    for (int i = 0; i < 4; i++)
    {
        cl.csdBorderRadius[i].setParent(&MSurface::ak.root);
        cl.csdBorderRadius[i].layout().setPositionType(YGPositionTypeAbsolute);
        cl.csdBorderRadius[i].layout().setWidth(app()->theme()->CSDBorderRadius);
        cl.csdBorderRadius[i].layout().setHeight(app()->theme()->CSDBorderRadius);
        cl.csdBorderRadius[i].enableCustomBlendFunc(true);
        cl.csdBorderRadius[i].enableAutoDamage(false);
        cl.csdBorderRadius[i].setCustomBlendFunc({
            .sRGBFactor = GL_ZERO,
            .dRGBFactor = GL_SRC_ALPHA,
            .sAlphaFactor = GL_ZERO,
            .dAlphaFactor = GL_SRC_ALPHA,
        });

        /*
        cl.csdBorderRadius[i].opaqueRegion.setEmpty();
        cl.csdBorderRadius[i].reactiveRegion.setRect(
            SkIRect::MakeWH(app()->theme()->CSDBorderRadius, app()->theme()->CSDBorderRadius));*/
    }

    // TODO: update only when activated changes

    // TL
    cl.csdBorderRadius[0].setSrcTransform(AKTransform::Normal);
    cl.csdBorderRadius[0].layout().setPosition(YGEdgeLeft, 0);
    cl.csdBorderRadius[0].layout().setPosition(YGEdgeTop, 0);

    // TR
    cl.csdBorderRadius[1].setSrcTransform(AKTransform::Rotated90);
    cl.csdBorderRadius[1].layout().setPosition(YGEdgeRight, 0);
    cl.csdBorderRadius[1].layout().setPosition(YGEdgeTop, 0);

    // BR
    cl.csdBorderRadius[2].setSrcTransform(AKTransform::Rotated180);
    cl.csdBorderRadius[2].layout().setPosition(YGEdgeRight, 0);
    cl.csdBorderRadius[2].layout().setPosition(YGEdgeBottom, 0);

    // BL
    cl.csdBorderRadius[3].setSrcTransform(AKTransform::Rotated270);
    cl.csdBorderRadius[3].layout().setPosition(YGEdgeLeft, 0);
    cl.csdBorderRadius[3].layout().setPosition(YGEdgeBottom, 0);

    cl.csdShadow.setParent(&MSurface::ak.root);
}

MToplevel::~MToplevel() noexcept
{
    xdg_toplevel_destroy(wl.xdgToplevel);
    xdg_surface_destroy(wl.xdgSurface);
}

void MToplevel::setMaximized(bool maximized) noexcept
{
    if (maximized)
        xdg_toplevel_set_maximized(wl.xdgToplevel);
    else
        xdg_toplevel_unset_maximized(wl.xdgToplevel);
}

void MToplevel::setFullscreen(bool fullscreen) noexcept
{
    if (fullscreen)
        xdg_toplevel_set_fullscreen(wl.xdgToplevel, NULL);
    else
        xdg_toplevel_unset_fullscreen(wl.xdgToplevel);
}

void MToplevel::setMinimized() noexcept
{
    xdg_toplevel_set_minimized(wl.xdgToplevel);
}

void MToplevel::onSuggestedSizeChanged()
{
    if (suggestedSize().width() == 0)
    {
        if (globalRect().width() == 0)
            layout().setWidthAuto();
    }
    else
        layout().setWidth(suggestedSize().width());

    if (suggestedSize().height() == 0)
    {
        if (globalRect().height() == 0)
            layout().setHeightAuto();
    }
    else
        layout().setHeight(suggestedSize().height());
}

void MToplevel::onStatesChanged()
{

}

void MToplevel::setTitle(const std::string &title)
{
    if (cl.title == title)
        return;

    xdg_toplevel_set_title(wl.xdgToplevel, cl.title.c_str());
    cl.title = title;
    on.titleChanged.notify(cl.title);
}

void MToplevel::xdg_surface_configure(void *data, xdg_surface */*xdgSurface*/, UInt32 serial)
{
    auto &role { *static_cast<MToplevel*>(data) };
    role.cl.flags.add(PendingConfigureAck);
    role.se.serial = serial;

    bool notifyStates { role.cl.flags.check(PendingFirstConfigure) };
    bool notifySuggestedSize { notifyStates };
    bool activatedChanged { false };
    role.cl.flags.remove(PendingFirstConfigure);

    if (notifyStates)
    {
        xdg_toplevel_set_title(role.wl.xdgToplevel, role.title().c_str());
    }

    if (role.se.states.get() != role.cl.states.get())
    {
        activatedChanged = role.cl.states.check(Activated) != role.se.states.check(Activated);
        role.cl.states = role.se.states;
        notifyStates = true;
    }

    if (role.se.suggestedSize != role.cl.suggestedSize)
    {
        role.cl.suggestedSize = role.se.suggestedSize;
        notifySuggestedSize = true;
    }

    AK::AKWeak<MToplevel> ref { &role };

    if (notifySuggestedSize)
    {
        role.onSuggestedSizeChanged();

        if (ref)
            role.on.suggestedSizeChanged.notify(role.cl.suggestedSize);
    }

    if (ref && notifyStates)
    {
        if (activatedChanged)
        {
            if (role.cl.states.check(Activated))
                role.ak.scene.postEvent(AKStateActivatedEvent());
            else
                role.ak.scene.postEvent(AKStateDeactivatedEvent());
        }

        if (ref)
            role.onStatesChanged();

        if (ref)
            role.on.statesChanged.notify(role.cl.states);
    }

    role.cl.flags.add(ForceUpdate);
    role.update();
}

void MToplevel::xdg_toplevel_configure(void *data, xdg_toplevel */*xdgToplevel*/, Int32 width, Int32 height, wl_array *states)
{
    auto &role { *static_cast<MToplevel*>(data) };
    role.se.states.set(0);
    const UInt32 *stateVals = (UInt32*)states->data;
    for (UInt32 i = 0; i < states->size/sizeof(*stateVals); i++)
        role.se.states.add(1 << stateVals[i]);

    role.se.suggestedSize.fWidth = width < 0 ? 0 : width;
    role.se.suggestedSize.fHeight = height < 0 ? 0 : height;
}

void MToplevel::xdg_toplevel_close(void *data, xdg_toplevel *xdgToplevel)
{
    exit(0);
}

void MToplevel::xdg_toplevel_configure_bounds(void *data, xdg_toplevel *xdgToplevel, Int32 width, Int32 height)
{

}

void MToplevel::xdg_toplevel_wm_capabilities(void *data, xdg_toplevel *xdgToplevel, wl_array *capabilities)
{

}

void MToplevel::onUpdate() noexcept
{
    MSurface::onUpdate();

    if (cl.flags.check(PendingConfigureAck))
    {
        cl.flags.remove(PendingConfigureAck);
        xdg_surface_ack_configure(wl.xdgSurface, se.serial);
    }

    if (visible())
    {
        if (cl.flags.check(PendingNullCommit))
        {
            cl.flags.add(PendingFirstConfigure);
            cl.flags.remove(PendingNullCommit);
            wl_surface_attach(MSurface::wl.surface, nullptr, 0, 0);
            wl_surface_commit(MSurface::wl.surface);
            update();
            return;
        }
    }
    else
    {
        if (!cl.flags.check(PendingNullCommit))
        {
            cl.flags.add(PendingNullCommit);
            wl_surface_attach(MSurface::wl.surface, nullptr, 0, 0);
            wl_surface_commit(MSurface::wl.surface);
        }

        return;
    }

    if (cl.flags.check(PendingFirstConfigure | PendingNullCommit))
        return;

    if (MSurface::se.changes.test(Cl_Scale))
        cl.flags.add(ForceUpdate);

    if (cl.states.check(Maximized | Fullscreen))
    {
        cl.csdShadowMargins = { 0, 0, 0, 0 };
        cl.csdShadow.setVisible(false);
        for (int i = 0; i < 4; i++)
            cl.csdBorderRadius[i].setVisible(false);
    }
    else
    {
        cl.csdShadow.setVisible(true);
        for (int i = 0; i < 4; i++)
            cl.csdBorderRadius[i].setVisible(true);

        if (activated())
        {
            cl.csdShadowMargins = {
                app()->theme()->CSDShadowActiveRadius,
                app()->theme()->CSDShadowActiveRadius - app()->theme()->CSDShadowActiveOffsetY,
                app()->theme()->CSDShadowActiveRadius,
                app()->theme()->CSDShadowActiveRadius + app()->theme()->CSDShadowActiveOffsetY,
            };
        }
        else
        {
            cl.csdShadowMargins = {
                app()->theme()->CSDShadowInactiveRadius,
                app()->theme()->CSDShadowInactiveRadius - app()->theme()->CSDShadowInactiveOffsetY,
                app()->theme()->CSDShadowInactiveRadius,
                app()->theme()->CSDShadowInactiveRadius + app()->theme()->CSDShadowInactiveOffsetY,
            };
        }
    }

    layout().setPosition(YGEdgeLeft, 0.f);
    layout().setPosition(YGEdgeTop, 0.f);
    layout().setMargin(YGEdgeLeft, cl.csdShadowMargins.fLeft);
    layout().setMargin(YGEdgeTop, cl.csdShadowMargins.fTop);
    layout().setMargin(YGEdgeRight, cl.csdShadowMargins.fRight);
    layout().setMargin(YGEdgeBottom, cl.csdShadowMargins.fBottom);
    cl.csdBorderRadius[0].layout().setPosition(YGEdgeLeft, cl.csdShadowMargins.fLeft);
    cl.csdBorderRadius[0].layout().setPosition(YGEdgeTop, cl.csdShadowMargins.fTop);
    cl.csdBorderRadius[1].layout().setPosition(YGEdgeRight, cl.csdShadowMargins.fRight);
    cl.csdBorderRadius[1].layout().setPosition(YGEdgeTop, cl.csdShadowMargins.fTop);
    cl.csdBorderRadius[2].layout().setPosition(YGEdgeRight, cl.csdShadowMargins.fRight);
    cl.csdBorderRadius[2].layout().setPosition(YGEdgeBottom, cl.csdShadowMargins.fBottom);
    cl.csdBorderRadius[3].layout().setPosition(YGEdgeLeft, cl.csdShadowMargins.fLeft);
    cl.csdBorderRadius[3].layout().setPosition(YGEdgeBottom, cl.csdShadowMargins.fBottom);
    render();
}

void MToplevel::render() noexcept
{
    /*
    const Int64 ms = Int64(AKTime::ms()) - Int64(MSurface::wl.callbackSendMs);

    if (ms < 8)
    {
        app()->setTimeout(8 - ms);
        return;
    }*/

    ak.scene.root()->layout().calculate();

    if (MSurface::wl.callback && !cl.flags.check(ForceUpdate))
        return;

    cl.flags.remove(ForceUpdate);

    SkISize size(
        SkScalarFloorToInt(layout().calculatedWidth() + layout().calculatedMargin(YGEdgeLeft) + layout().calculatedMargin(YGEdgeRight)),
        SkScalarFloorToInt(layout().calculatedHeight() + layout().calculatedMargin(YGEdgeTop) + layout().calculatedMargin(YGEdgeBottom)));

    if (size.fWidth <= 0) size.fWidth = 8;
    if (size.fHeight <= 0) size.fHeight = 8;

    const bool sizeChanged { resizeBuffer(size) };

    if (sizeChanged)
    {
        xdg_surface_set_window_geometry(
            wl.xdgSurface,
            cl.csdShadowMargins.fLeft,
            cl.csdShadowMargins.fTop,
            layout().calculatedWidth(),
            layout().calculatedHeight());
    }

    eglMakeCurrent(app()->graphics().eglDisplay, gl.eglSurface, gl.eglSurface, app()->graphics().eglContext);
    eglSwapInterval(app()->graphics().eglDisplay, 0);

    ak.target->setDstRect({ 0, 0, bufferSize().width(), bufferSize().height() });
    ak.target->setViewport(SkRect::MakeWH(surfaceSize().width(), surfaceSize().height()));
    ak.target->setSurface(gl.skSurface);

    EGLint bufferAge { 0 };
    eglQuerySurface(app()->graphics().eglDisplay, gl.eglSurface, EGL_BUFFER_AGE_EXT, &bufferAge);
    ak.target->setBakedComponentsScale(scale());
    ak.target->setRenderCalculatesLayout(false);
    ak.target->setAge(bufferAge);
    SkRegion skDamage, skOpaque;
    ak.target->outDamageRegion = &skDamage;
    ak.target->outOpaqueRegion = &skOpaque;

    /* CSD */
    for (int i = 0; i < 4; i++)
        cl.csdBorderRadius[i].setImage(app()->theme()->csdBorderRadiusMask(scale()));

    /*
    glScissor(0, 0, 1000000, 100000);
    glViewport(0, 0, 1000000, 100000);
    glClear(GL_COLOR_BUFFER_BIT);*/
    ak.scene.render(ak.target);

    for (int i = 0; i < 4; i++)
        ak.target->outOpaqueRegion->op(cl.csdBorderRadius[i].globalRect(), SkRegion::Op::kDifference_Op);

    wl_surface_set_buffer_scale(MSurface::wl.surface, scale());

    /* Input region */
    if (sizeChanged)
    {
        SkIRect inputRect { globalRect() };
        inputRect.outset(6, 6);
        wl_region *wlInputRegion = wl_compositor_create_region(app()->wayland().compositor);
        wl_region_add(wlInputRegion, inputRect.x(), inputRect.y(), inputRect.width(), inputRect.height());
        wl_surface_set_input_region(MSurface::wl.surface, wlInputRegion);
        wl_region_destroy(wlInputRegion);
    }

    /* In some compositors using fractional scaling (without oversampling), the opaque region
     * can leak, causing borders to appear black. This inset prevents that. */
    SkIRect opaqueClip { globalRect() };
    opaqueClip.inset(1, 1);
    skOpaque.op(opaqueClip, SkRegion::Op::kIntersect_Op);
    wl_region *wlOpaqueRegion = wl_compositor_create_region(app()->wayland().compositor);
    SkRegion::Iterator opaqueIt(skOpaque);
    while (!opaqueIt.done())
    {
        wl_region_add(wlOpaqueRegion, opaqueIt.rect().x(), opaqueIt.rect().y(), opaqueIt.rect().width(), opaqueIt.rect().height());
        opaqueIt.next();
    }
    wl_surface_set_opaque_region(MSurface::wl.surface, wlOpaqueRegion);
    wl_region_destroy(wlOpaqueRegion);

    const bool noDamage { skDamage.computeRegionComplexity() == 0 };

    if (noDamage)
        skDamage.setRect(SkIRect(-10, -10, 1, 1));

    EGLint *damageRects { new EGLint[skDamage.computeRegionComplexity() * 4] };
    EGLint *rectsIt = damageRects;
    SkRegion::Iterator damageIt(skDamage);
    while (!damageIt.done())
    {
        *rectsIt = damageIt.rect().x() * scale();
        rectsIt++;
        *rectsIt = (surfaceSize().height() - damageIt.rect().height() - damageIt.rect().y()) * scale();
        rectsIt++;
        *rectsIt = damageIt.rect().width() * scale();
        rectsIt++;
        *rectsIt = damageIt.rect().height() * scale();
        rectsIt++;
        damageIt.next();
    }

    const UInt32 callbackMsA { MSurface::wl.callbackSendMs };
    createCallback();
    const UInt32 callbackMsB { MSurface::wl.callbackSendMs };
    const UInt32 callbackMsDiff { callbackMsB - callbackMsA };

    assert(app()->graphics().eglSwapBuffersWithDamageKHR(app()->graphics().eglDisplay, gl.eglSurface, damageRects, skDamage.computeRegionComplexity()) == EGL_TRUE);
    delete []damageRects;

    if (states().check(Resizing) && callbackMsDiff < 4)
    {
        wl_display_flush(Marco::app()->wayland().display);
        usleep((4 - callbackMsDiff) * 1000);
        AKLog::debug("Sleeping %d ms", (4 - callbackMsDiff));
    }

    //eglSwapBuffers(app()->graphics().eglDisplay, m_eglSurface);
}

void MToplevel::handleRootPointerButtonEvent(const AKPointerButtonEvent &event) noexcept
{
    if (!visible() || event.button() != BTN_LEFT || event.state() != AKPointerButtonEvent::Pressed)
        return;

    const SkPoint &pointerPos { pointer().eventHistory().move.pos() };
    const Int32 resizeMargins { MTheme::CSDResizeOutset };
    const Int32 moveTopMargin { MTheme::CSDMoveOutset };
    UInt32 resizeEdges { 0 };

    if (globalRect().x() - resizeMargins <= pointerPos.x() && globalRect().x() + resizeMargins >= pointerPos.x())
        resizeEdges |= XDG_TOPLEVEL_RESIZE_EDGE_LEFT;
    else if (globalRect().right() - resizeMargins <= pointerPos.x() && globalRect().right() + resizeMargins >= pointerPos.x())
        resizeEdges |= XDG_TOPLEVEL_RESIZE_EDGE_RIGHT;

    if (globalRect().y() - resizeMargins <= pointerPos.y() && globalRect().y() + resizeMargins >= pointerPos.y())
        resizeEdges |= XDG_TOPLEVEL_RESIZE_EDGE_TOP;
    else if (globalRect().bottom() - resizeMargins <= pointerPos.y() && globalRect().bottom() + resizeMargins >= pointerPos.y())
        resizeEdges |= XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;

    if (resizeEdges)
        xdg_toplevel_resize(wl.xdgToplevel, app()->wayland().seat, event.serial(), resizeEdges);
    else if (globalRect().x() <= pointerPos.x() &&
             globalRect().right() >= pointerPos.x() &&
             globalRect().y() <= pointerPos.y() &&
             globalRect().y() + moveTopMargin >= pointerPos.y())
    {
        xdg_toplevel_move(wl.xdgToplevel, app()->wayland().seat, event.serial());
    }
}

void MToplevel::handleRootPointerMoveEvent(const AK::AKPointerMoveEvent &event) noexcept
{
    if (!visible() || states().check(Fullscreen))
    {
        ak.root.setCursor(AKCursor::Default);
        return;
    }

    const SkPoint &pointerPos { event.pos() };
    const Int32 resizeMargins { MTheme::CSDResizeOutset };
    UInt32 resizeEdges { 0 };

    if (globalRect().x() - resizeMargins <= pointerPos.x() && globalRect().x() + resizeMargins >= pointerPos.x())
        resizeEdges |= XDG_TOPLEVEL_RESIZE_EDGE_LEFT;
    else if (globalRect().right() - resizeMargins <= pointerPos.x() && globalRect().right() + resizeMargins >= pointerPos.x())
        resizeEdges |= XDG_TOPLEVEL_RESIZE_EDGE_RIGHT;

    if (globalRect().y() - resizeMargins <= pointerPos.y() && globalRect().y() + resizeMargins >= pointerPos.y())
        resizeEdges |= XDG_TOPLEVEL_RESIZE_EDGE_TOP;
    else if (globalRect().bottom() - resizeMargins <= pointerPos.y() && globalRect().bottom() + resizeMargins >= pointerPos.y())
        resizeEdges |= XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;

    if (resizeEdges == XDG_TOPLEVEL_RESIZE_EDGE_LEFT || resizeEdges == XDG_TOPLEVEL_RESIZE_EDGE_RIGHT)
        ak.root.setCursor(AKCursor::EWResize);
    else if (resizeEdges == XDG_TOPLEVEL_RESIZE_EDGE_TOP || resizeEdges == XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM)
        ak.root.setCursor(AKCursor::NSResize);
    else if (resizeEdges == (XDG_TOPLEVEL_RESIZE_EDGE_TOP | XDG_TOPLEVEL_RESIZE_EDGE_LEFT) ||
             resizeEdges == (XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM | XDG_TOPLEVEL_RESIZE_EDGE_RIGHT))
        ak.root.setCursor(AKCursor::NWSEResize);
    else if (resizeEdges == (XDG_TOPLEVEL_RESIZE_EDGE_TOP | XDG_TOPLEVEL_RESIZE_EDGE_RIGHT) ||
             resizeEdges == (XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM | XDG_TOPLEVEL_RESIZE_EDGE_LEFT))
        ak.root.setCursor(AKCursor::NESWResize);
    else
        ak.root.setCursor(AKCursor::Default);
}

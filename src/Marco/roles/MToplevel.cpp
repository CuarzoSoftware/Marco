#include <Marco/private/MToplevelPrivate.h>
#include <Marco/MApplication.h>
#include <Marco/MTheme.h>

#include <AK/events/AKWindowStateEvent.h>
#include <AK/AKLog.h>

#include <include/gpu/ganesh/SkSurfaceGanesh.h>
#include <include/core/SkColorSpace.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

using namespace Marco;

MToplevel::MToplevel() noexcept : MSurface(Role::Toplevel)
{
    m_imp = std::make_unique<Imp>(*this);
    ak.root.installEventFilter(this);

    imp()->xdgSurface = xdg_wm_base_get_xdg_surface(app()->wayland().xdgWmBase, MSurface::wl.surface);
    xdg_surface_add_listener(imp()->xdgSurface, &Imp::xdgSurfaceListener, this);
    imp()->xdgToplevel = xdg_surface_get_toplevel(imp()->xdgSurface);
    xdg_toplevel_add_listener(imp()->xdgToplevel, &Imp::xdgToplevelListener, this);
    xdg_toplevel_set_app_id(imp()->xdgToplevel, app()->appId().c_str());

    //if (app()->wayland().xdgDecorationManager)
    //    wl.xdgDecoration = zxdg_decoration_manager_v1_get_toplevel_decoration(app()->wayland().xdgDecorationManager, imp()->xdgToplevel);

    app()->on.appIdChanged.subscribe(this, [this](const std::string &appId){
        xdg_toplevel_set_app_id(imp()->xdgToplevel, appId.c_str());
    });

    /* CSD */

    for (int i = 0; i < 4; i++)
    {
        imp()->borderRadius[i].setParent(&MSurface::ak.root);
        imp()->borderRadius[i].layout().setPositionType(YGPositionTypeAbsolute);
        imp()->borderRadius[i].layout().setWidth(app()->theme()->CSDBorderRadius);
        imp()->borderRadius[i].layout().setHeight(app()->theme()->CSDBorderRadius);
        imp()->borderRadius[i].enableCustomBlendFunc(true);
        imp()->borderRadius[i].enableAutoDamage(false);
        imp()->borderRadius[i].setCustomBlendFunc({
            .sRGBFactor = GL_ZERO,
            .dRGBFactor = GL_SRC_ALPHA,
            .sAlphaFactor = GL_ZERO,
            .dAlphaFactor = GL_SRC_ALPHA,
        });

        /*
        imp()->borderRadius[i].opaqueRegion.setEmpty();
        imp()->borderRadius[i].reactiveRegion.setRect(
            SkIRect::MakeWH(app()->theme()->CSDBorderRadius, app()->theme()->CSDBorderRadius));*/
    }

    // TODO: update only when activated changes

    // TL
    imp()->borderRadius[0].setSrcTransform(AKTransform::Normal);
    imp()->borderRadius[0].layout().setPosition(YGEdgeLeft, 0);
    imp()->borderRadius[0].layout().setPosition(YGEdgeTop, 0);

    // TR
    imp()->borderRadius[1].setSrcTransform(AKTransform::Rotated90);
    imp()->borderRadius[1].layout().setPosition(YGEdgeRight, 0);
    imp()->borderRadius[1].layout().setPosition(YGEdgeTop, 0);

    // BR
    imp()->borderRadius[2].setSrcTransform(AKTransform::Rotated180);
    imp()->borderRadius[2].layout().setPosition(YGEdgeRight, 0);
    imp()->borderRadius[2].layout().setPosition(YGEdgeBottom, 0);

    // BL
    imp()->borderRadius[3].setSrcTransform(AKTransform::Rotated270);
    imp()->borderRadius[3].layout().setPosition(YGEdgeLeft, 0);
    imp()->borderRadius[3].layout().setPosition(YGEdgeBottom, 0);

    imp()->shadow.setParent(&MSurface::ak.root);
}

MToplevel::~MToplevel() noexcept
{
    xdg_toplevel_destroy(imp()->xdgToplevel);
    xdg_surface_destroy(imp()->xdgSurface);
}

void MToplevel::setMaximized(bool maximized) noexcept
{
    if (maximized)
        xdg_toplevel_set_maximized(imp()->xdgToplevel);
    else
        xdg_toplevel_unset_maximized(imp()->xdgToplevel);
}

bool MToplevel::maximized() const noexcept
{
    return states().check(AKMaximized);
}

void MToplevel::setFullscreen(bool fullscreen, MScreen *screen) noexcept
{
    if (fullscreen)
        xdg_toplevel_set_fullscreen(imp()->xdgToplevel, screen ? screen->wlOutput() : nullptr);
    else
        xdg_toplevel_unset_fullscreen(imp()->xdgToplevel);
}

bool MToplevel::fullscreen() const noexcept
{
    return states().check(AKFullscreen);
}

void MToplevel::setMinimized() noexcept
{
    xdg_toplevel_set_minimized(imp()->xdgToplevel);
}

void MToplevel::setMinSize(const SkISize &size) noexcept
{
    imp()->minSize = size;

    if (imp()->minSize.fWidth < 0) imp()->minSize.fWidth = 0;
    if (imp()->minSize.fHeight < 0) imp()->minSize.fHeight = 0;

    xdg_toplevel_set_min_size(imp()->xdgToplevel, imp()->minSize.fWidth, imp()->minSize.fHeight);

    layout().setMinWidth(imp()->minSize.fWidth == 0 ? YGUndefined : imp()->minSize.fWidth);
    layout().setMinHeight(imp()->minSize.fHeight == 0 ? YGUndefined : imp()->minSize.fHeight);
}

const SkISize &MToplevel::minSize() const noexcept
{
    return imp()->minSize;
}

void MToplevel::setMaxSize(const SkISize &size) noexcept
{
    imp()->maxSize = size;

    if (imp()->maxSize.fWidth < 0) imp()->maxSize.fWidth = 0;
    if (imp()->maxSize.fHeight < 0) imp()->maxSize.fHeight = 0;

    xdg_toplevel_set_max_size(imp()->xdgToplevel, imp()->maxSize.fWidth, imp()->maxSize.fHeight);

    layout().setMinWidth(imp()->maxSize.fWidth == 0 ? YGUndefined : imp()->maxSize.fWidth);
    layout().setMinHeight(imp()->maxSize.fHeight == 0 ? YGUndefined : imp()->maxSize.fHeight);
}

const SkISize &MToplevel::maxSize() const noexcept
{
    return imp()->maxSize;
}

const SkISize &MToplevel::suggestedSize() const noexcept
{
    return imp()->currentSuggestedSize;
}

void MToplevel::suggestedSizeChanged()
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

AKBitset<AKWindowState> MToplevel::states() const noexcept
{
    return imp()->currentStates;
}

void MToplevel::setTitle(const std::string &title)
{
    if (this->title() == title)
        return;

    xdg_toplevel_set_title(imp()->xdgToplevel, title.c_str());
    imp()->title = title;
    onTitleChanged.notify();
}

const std::string &MToplevel::title() const noexcept
{
    return imp()->title;
}

const SkIRect &MToplevel::decorationMargins() const noexcept
{
    return imp()->shadowMargins;
}

void MToplevel::windowStateEvent(const AKWindowStateEvent &event)
{
    MSurface::windowStateEvent(event);
    onStatesChanged.notify(event);
    event.accept();
}

bool MToplevel::eventFilter(const AKEvent &event, AKObject &object)
{
    if (&ak.root == &object)
    {
        if (event.type() == AKEvent::PointerButton)
            imp()->handleRootPointerButtonEvent(static_cast<const AKPointerButtonEvent&>(event));
        else if (event.type() == AKEvent::PointerMove)
            imp()->handleRootPointerMoveEvent(static_cast<const AKPointerMoveEvent&>(event));
    }

   return MSurface::eventFilter(event, object);
}

void MToplevel::onUpdate() noexcept
{
    MSurface::onUpdate();

    if (imp()->flags.check(Imp::PendingConfigureAck))
    {
        imp()->flags.remove(Imp::PendingConfigureAck);
        xdg_surface_ack_configure(imp()->xdgSurface, imp()->configureSerial);
    }

    if (visible())
    {
        if (imp()->flags.check(Imp::PendingNullCommit))
        {
            imp()->flags.add(Imp::PendingFirstConfigure);
            imp()->flags.remove(Imp::PendingNullCommit);
            wl_surface_attach(MSurface::wl.surface, nullptr, 0, 0);
            wl_surface_commit(MSurface::wl.surface);
            update();
            return;
        }
    }
    else
    {
        if (!imp()->flags.check(Imp::PendingNullCommit))
        {
            imp()->flags.add(Imp::PendingNullCommit);
            wl_surface_attach(MSurface::wl.surface, nullptr, 0, 0);
            wl_surface_commit(MSurface::wl.surface);
        }

        return;
    }

    if (imp()->flags.check(Imp::PendingFirstConfigure | Imp::PendingNullCommit))
        return;

    if (MSurface::se.changes.test(Cl_Scale))
        imp()->flags.add(Imp::ForceUpdate);

    if (states().check(AKMaximized | AKFullscreen))
    {
        imp()->shadowMargins = { 0, 0, 0, 0 };
        imp()->shadow.setVisible(false);
        for (int i = 0; i < 4; i++)
            imp()->borderRadius[i].setVisible(false);
    }
    else
    {
        imp()->shadow.setVisible(true);
        for (int i = 0; i < 4; i++)
            imp()->borderRadius[i].setVisible(true);

        if (activated())
        {
            imp()->shadowMargins = {
                app()->theme()->CSDShadowActiveRadius,
                app()->theme()->CSDShadowActiveRadius - app()->theme()->CSDShadowActiveOffsetY,
                app()->theme()->CSDShadowActiveRadius,
                app()->theme()->CSDShadowActiveRadius + app()->theme()->CSDShadowActiveOffsetY,
            };
        }
        else
        {
            imp()->shadowMargins = {
                app()->theme()->CSDShadowInactiveRadius,
                app()->theme()->CSDShadowInactiveRadius - app()->theme()->CSDShadowInactiveOffsetY,
                app()->theme()->CSDShadowInactiveRadius,
                app()->theme()->CSDShadowInactiveRadius + app()->theme()->CSDShadowInactiveOffsetY,
            };
        }
    }

    layout().setPosition(YGEdgeLeft, 0.f);
    layout().setPosition(YGEdgeTop, 0.f);
    layout().setMargin(YGEdgeLeft, imp()->shadowMargins.fLeft);
    layout().setMargin(YGEdgeTop, imp()->shadowMargins.fTop);
    layout().setMargin(YGEdgeRight, imp()->shadowMargins.fRight);
    layout().setMargin(YGEdgeBottom, imp()->shadowMargins.fBottom);
    imp()->borderRadius[0].layout().setPosition(YGEdgeLeft, imp()->shadowMargins.fLeft);
    imp()->borderRadius[0].layout().setPosition(YGEdgeTop, imp()->shadowMargins.fTop);
    imp()->borderRadius[1].layout().setPosition(YGEdgeRight, imp()->shadowMargins.fRight);
    imp()->borderRadius[1].layout().setPosition(YGEdgeTop, imp()->shadowMargins.fTop);
    imp()->borderRadius[2].layout().setPosition(YGEdgeRight, imp()->shadowMargins.fRight);
    imp()->borderRadius[2].layout().setPosition(YGEdgeBottom, imp()->shadowMargins.fBottom);
    imp()->borderRadius[3].layout().setPosition(YGEdgeLeft, imp()->shadowMargins.fLeft);
    imp()->borderRadius[3].layout().setPosition(YGEdgeBottom, imp()->shadowMargins.fBottom);
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

    if (MSurface::wl.callback && !imp()->flags.check(Imp::ForceUpdate))
        return;

    imp()->flags.remove(Imp::ForceUpdate);

    SkISize size(
        SkScalarFloorToInt(layout().calculatedWidth() + layout().calculatedMargin(YGEdgeLeft) + layout().calculatedMargin(YGEdgeRight)),
        SkScalarFloorToInt(layout().calculatedHeight() + layout().calculatedMargin(YGEdgeTop) + layout().calculatedMargin(YGEdgeBottom)));

    if (size.fWidth <= 0) size.fWidth = 8;
    if (size.fHeight <= 0) size.fHeight = 8;

    const bool sizeChanged { resizeBuffer(size) };

    if (sizeChanged)
    {
        xdg_surface_set_window_geometry(
            imp()->xdgSurface,
            imp()->shadowMargins.fLeft,
            imp()->shadowMargins.fTop,
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
        imp()->borderRadius[i].setImage(app()->theme()->csdBorderRadiusMask(scale()));

    /*
    glScissor(0, 0, 1000000, 100000);
    glViewport(0, 0, 1000000, 100000);
    glClear(GL_COLOR_BUFFER_BIT);*/
    ak.scene.render(ak.target);

    for (int i = 0; i < 4; i++)
        ak.target->outOpaqueRegion->op(imp()->borderRadius[i].globalRect(), SkRegion::Op::kDifference_Op);

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

    /*
    if (states().check(Resizing) && callbackMsDiff < 4)
    {
        wl_display_flush(Marco::app()->wayland().display);
        usleep((4 - callbackMsDiff) * 1000);
        AKLog::debug("Sleeping %d ms", (4 - callbackMsDiff));
    }*/

    //eglSwapBuffers(app()->graphics().eglDisplay, m_eglSurface);
}

MToplevel::Imp *MToplevel::imp() const noexcept
{
    return m_imp.get();
}

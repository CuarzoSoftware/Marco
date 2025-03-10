#include <Marco/private/MLayerSurfacePrivate.h>
#include <Marco/MApplication.h>

using namespace AK;

MLayerSurface::MLayerSurface(Layer layer, AKBitset<AKEdge> anchor, Int32 exclusiveZone, MScreen *screen, const std::string &scope) noexcept :
    MSurface(Role::LayerSurface)
{
    assert("zwlr_layer_shell not supported by the compositor" && app()->wayland().layerShell);
    m_imp = std::make_unique<Imp>(*this);
    imp()->layer = layer;
    imp()->screen = screen;
    imp()->scope = scope;

    imp()->layerSurface = zwlr_layer_shell_v1_get_layer_surface(
        app()->wayland().layerShell,
        wl.surface,
        screen ? screen->wlOutput() : nullptr,
        layer,
        scope.c_str());

    zwlr_layer_surface_v1_add_listener(imp()->layerSurface, &Imp::layerSurfaceListener, this);

    setAnchor(anchor);
    setExclusiveZone(exclusiveZone);
}

MLayerSurface::~MLayerSurface() noexcept
{

}

bool MLayerSurface::setScreen(MScreen *screen) noexcept
{
    return false;
}

MScreen *MLayerSurface::screen() const noexcept
{
    return imp()->screen;
}

bool MLayerSurface::setAnchor(AKBitset<AKEdge> edges) noexcept
{
    if (imp()->anchor.get() == edges.get())
        return false;

    imp()->anchor = edges;
    update();
    return true;
}

AKBitset<AKEdge> MLayerSurface::anchor() const noexcept
{
    return imp()->anchor;
}

bool MLayerSurface::setExclusiveZone(Int32 size) noexcept
{
    if (imp()->exclusiveZone == size)
        return false;

    imp()->exclusiveZone = size;
    update();
    return true;
}

Int32 MLayerSurface::exclusiveZone() const noexcept
{
    return imp()->exclusiveZone;
}

bool MLayerSurface::setMargin(const SkIRect &margin) noexcept
{
    if (imp()->margin == margin)
        return false;

    imp()->margin = margin;
    update();
    return true;
}

const SkIRect &MLayerSurface::margin() const noexcept
{
    return imp()->margin;
}

bool MLayerSurface::setKeyboardInteractivity(KeyboardInteractivity mode) noexcept
{
    if (imp()->keyboardInteractivity == mode)
        return false;

    imp()->keyboardInteractivity = mode;
    update();
    return true;
}

MLayerSurface::KeyboardInteractivity MLayerSurface::keyboardInteractivity() const noexcept
{
    return imp()->keyboardInteractivity;
}

bool MLayerSurface::setLayer(Layer layer) noexcept
{
    if (imp()->layer == layer)
        return false;

    imp()->layer = layer;
    update();
    return true;
}

MLayerSurface::Layer MLayerSurface::layer() const noexcept
{
    return imp()->layer;
}

bool MLayerSurface::setExclusiveEdge(AKEdge edge) noexcept
{
    if (imp()->exclusiveEdge == edge)
        return false;

    imp()->exclusiveEdge = edge;
    update();
    return true;
}

AKEdge MLayerSurface::exclusiveEdge() const noexcept
{
    return imp()->exclusiveEdge;
}

bool MLayerSurface::setScope(const std::string &scope) noexcept
{
    return false;
}

const std::string &MLayerSurface::scope() const noexcept
{
    return imp()->scope;
}

const SkISize &MLayerSurface::suggestedSize() const noexcept
{
    return imp()->suggestedSize;
}

MLayerSurface::Imp *MLayerSurface::imp() const noexcept
{
    return m_imp.get();
}

void MLayerSurface::suggestedSizeChanged()
{
    return;
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

void MLayerSurface::render() noexcept
{
    ak.scene.root()->layout().calculate();

    if (wl.callback && !imp()->flags.check(Imp::ForceUpdate))
        return;

    imp()->flags.remove(Imp::ForceUpdate);

    EGLint bufferAge { -1 };

    SkISize newSize {
        SkScalarFloorToInt(layout().calculatedWidth() + layout().calculatedMargin(YGEdgeLeft) + layout().calculatedMargin(YGEdgeRight)),
        SkScalarFloorToInt(layout().calculatedHeight() + layout().calculatedMargin(YGEdgeTop) + layout().calculatedMargin(YGEdgeBottom))
    };

    if (newSize.fWidth < 8) newSize.fWidth = 8;
    if (newSize.fHeight < 8) newSize.fHeight = 8;

    bool sizeChanged = false;

    if (MSurface::cl.viewportSize != newSize)
    {
        MSurface::cl.viewportSize = newSize;
        bufferAge = 0;
        sizeChanged = true;
    }

    SkISize eglWindowSize { newSize };

    sizeChanged |= resizeBuffer(eglWindowSize);

    if (sizeChanged)
    {
        app()->update();

        wp_viewport_set_source(MSurface::wl.viewport,
                               wl_fixed_from_int(0),
                               wl_fixed_from_int(eglWindowSize.height() - newSize.height()),
                               wl_fixed_from_int(newSize.width()),
                               wl_fixed_from_int(newSize.height()));

        zwlr_layer_surface_v1_set_size(
            imp()->layerSurface,
            layout().calculatedWidth(),
            layout().calculatedHeight());
    }

    eglMakeCurrent(app()->graphics().eglDisplay, gl.eglSurface, gl.eglSurface, app()->graphics().eglContext);
    eglSwapInterval(app()->graphics().eglDisplay, 0);

    ak.target->setDstRect({ 0, 0, surfaceSize().width() * scale(), surfaceSize().height() * scale() });
    ak.target->setViewport(SkRect::MakeWH(surfaceSize().width(), surfaceSize().height()));
    ak.target->setSurface(gl.skSurface);

    if (bufferAge != 0)
        eglQuerySurface(app()->graphics().eglDisplay, gl.eglSurface, EGL_BUFFER_AGE_EXT, &bufferAge);
    ak.target->setBakedComponentsScale(scale());
    ak.target->setRenderCalculatesLayout(false);
    ak.target->setAge(bufferAge);
    SkRegion skDamage, skOpaque;
    ak.target->outDamageRegion = &skDamage;
    ak.target->outOpaqueRegion = &skOpaque;
    ak.scene.render(ak.target);
    wl_surface_set_buffer_scale(MSurface::wl.surface, scale());
    zwlr_layer_surface_v1_set_margin(imp()->layerSurface,
        margin().fTop, margin().fRight, margin().fBottom, margin().fLeft);
    zwlr_layer_surface_v1_set_layer(imp()->layerSurface, layer());
    zwlr_layer_surface_v1_set_anchor(imp()->layerSurface, anchor().get());
    zwlr_layer_surface_v1_set_exclusive_zone(imp()->layerSurface, exclusiveZone());
    zwlr_layer_surface_v1_set_exclusive_edge(imp()->layerSurface, exclusiveEdge());
    zwlr_layer_surface_v1_set_keyboard_interactivity(imp()->layerSurface, keyboardInteractivity());

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
        *rectsIt = bufferSize().height() - ((eglWindowSize.height() - newSize.height()) + damageIt.rect().height() + damageIt.rect().y()) * scale();
        rectsIt++;
        *rectsIt = damageIt.rect().width() * scale();
        rectsIt++;
        *rectsIt = damageIt.rect().height() * scale();
        rectsIt++;
        damageIt.next();
    }

    createCallback();

    assert(app()->graphics().eglSwapBuffersWithDamageKHR(app()->graphics().eglDisplay, gl.eglSurface, damageRects, skDamage.computeRegionComplexity()) == EGL_TRUE);
    delete []damageRects;
}

void MLayerSurface::onUpdate() noexcept
{
    MSurface::onUpdate();

    if (imp()->flags.check(Imp::PendingConfigureAck))
    {
        imp()->flags.remove(Imp::PendingConfigureAck);
        zwlr_layer_surface_v1_ack_configure(imp()->layerSurface, imp()->configureSerial);
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

    render();
}

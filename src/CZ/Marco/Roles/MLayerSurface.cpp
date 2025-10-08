#include <CZ/Marco/Private/MLayerSurfacePrivate.h>
#include <CZ/Marco/Private/MSurfacePrivate.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/MLog.h>
#include <CZ/AK/AKLog.h>
#include <CZ/Ream/WL/RWLSwapchain.h>

using namespace CZ;

MLayerSurface::MLayerSurface() noexcept :
    MSurface(Role::LayerSurface)
{
    assert("zwlr_layer_shell not supported by the compositor" && MApp::Get()->wl.layerShell);
    m_imp = std::make_unique<Imp>(*this);
}

MLayerSurface::~MLayerSurface() noexcept
{
    if (imp()->layerSurface)
    {
        zwlr_layer_surface_v1_destroy(imp()->layerSurface);
        imp()->layerSurface = nullptr;
    }
}

void MLayerSurface::requestAvailableWidth() noexcept
{
    if (imp()->requestAvailableWidth)
        return;

    imp()->requestAvailableWidth = true;
    update(true);
}

void MLayerSurface::requestAvailableHeight() noexcept
{
    if (imp()->requestAvailableHeight)
        return;

    imp()->requestAvailableHeight = true;
    update(true);
}

bool MLayerSurface::setScreen(MScreen *screen) noexcept
{
    if (screen == imp()->pendingScreen)
        return false;

    imp()->pendingScreen = screen;
    update();
    return true;
}

MScreen *MLayerSurface::screen() const noexcept
{
    return imp()->pendingScreen;
}

bool MLayerSurface::setAnchor(CZBitset<CZEdge> edges) noexcept
{
    if (imp()->pendingAnchor.get() == edges.get())
        return false;

    imp()->pendingAnchor = edges;
    update(true);
    return true;
}

CZBitset<CZEdge> MLayerSurface::anchor() const noexcept
{
    return imp()->pendingAnchor;
}

bool MLayerSurface::setExclusiveZone(Int32 size) noexcept
{
    if (size < -1)
    {
        MLog(CZWarning, CZLN, "Invalid value {}. Using -1 instead.", size);
        size = -1;
    }

    if (imp()->pendingExclusiveZone == size)
        return false;

    imp()->pendingExclusiveZone = size;
    update(true);
    return true;
}

Int32 MLayerSurface::exclusiveZone() const noexcept
{
    return imp()->pendingExclusiveZone;
}

bool MLayerSurface::setMargin(const SkIRect &margin) noexcept
{
    if (imp()->pendingMargin == margin)
        return false;

    imp()->pendingMargin = margin;
    update(true);
    return true;
}

const SkIRect &MLayerSurface::margin() const noexcept
{
    return imp()->pendingMargin;
}

bool MLayerSurface::setKeyboardInteractivity(KeyboardInteractivity mode) noexcept
{
    if (imp()->pendingKeyboardInteractivity == mode)
        return false;

    if ((MApp::Get()->wl.layerShell.version() < 4 && mode == OnDemand))
    {
        MLog(CZWarning, CZLN, "OnDemand option not supported by the compositor. Keeping current value");
        return false;
    }

    imp()->pendingKeyboardInteractivity = mode;
    update(true);
    return true;
}

MLayerSurface::KeyboardInteractivity MLayerSurface::keyboardInteractivity() const noexcept
{
    return imp()->pendingKeyboardInteractivity;
}

bool MLayerSurface::setLayer(Layer layer) noexcept
{
    if (imp()->pendingLayer == layer)
        return false;

    if (MApp::Get()->wl.layerShell.version() < 2 && imp()->layerSurface)
        imp()->reset();

    imp()->pendingLayer = layer;
    update(true);
    return true;
}

MLayerSurface::Layer MLayerSurface::layer() const noexcept
{
    return imp()->pendingLayer;
}

bool MLayerSurface::setExclusiveEdge(CZEdge edge) noexcept
{
    if (imp()->pendingExclusiveEdge == edge)
        return false;

    if (MApp::Get()->wl.layerShell.version() < 5)
    {
        MLog(CZWarning, CZLN, "Request not supported by the compositor");
        return false;
    }

    imp()->pendingExclusiveEdge = edge;
    update(true);
    return true;
}

CZEdge MLayerSurface::exclusiveEdge() const noexcept
{
    return imp()->pendingExclusiveEdge;
}

bool MLayerSurface::setScope(const std::string &scope) noexcept
{
    if (imp()->pendingScope == scope)
        return false;

    imp()->pendingScope = scope;
    update();
    return true;
}

const std::string &MLayerSurface::scope() const noexcept
{
    return imp()->pendingScope;
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
    if (suggestedSize().width() != 0)
        layout().setWidth(suggestedSize().width());

    if (suggestedSize().height() != 0)
        layout().setHeight(suggestedSize().height());
}

void MLayerSurface::render() noexcept
{
    scene().root()->layout().calculate();

    if (wlCallback() && !MSurface::imp()->flags.has(MSurface::Imp::ForceUpdate))
        return;

    MSurface::imp()->flags.remove(MSurface::Imp::ForceUpdate);

    bool repaint { false };
    bool anchorChanged = false;

    /* MARGIN CHANGE */

    if (imp()->pendingMargin != imp()->currentMargin)
    {
        imp()->currentMargin = imp()->pendingMargin;

        zwlr_layer_surface_v1_set_margin(imp()->layerSurface,
                                         margin().fTop, margin().fRight, margin().fBottom, margin().fLeft);
    }

    auto app { MApp::Get() };

    if (app->wl.layerShell.version() >= 2)
    {
        /* LAYER CHANGE */

        if (imp()->pendingLayer != imp()->currentLayer)
        {
            imp()->currentLayer = imp()->pendingLayer;
            zwlr_layer_surface_v1_set_layer(imp()->layerSurface, layer());
        }

        if (app->wl.layerShell.version() >= 5)
        {
            /* EXCLUSIVE EDGE CHANGE */

            if (imp()->pendingExclusiveEdge != imp()->currentExclusiveEdge)
            {
                imp()->currentExclusiveEdge = imp()->pendingExclusiveEdge;
                zwlr_layer_surface_v1_set_exclusive_edge(imp()->layerSurface,
                    anchor().has(exclusiveEdge()) ? exclusiveEdge() : CZEdgeNone);
            }
        }
    }

    /* EXCLUSIVE ZONE CHANGE */

    if (imp()->pendingExclusiveZone != imp()->currentExclusiveZone)
    {
        imp()->currentExclusiveZone = imp()->pendingExclusiveZone;
        zwlr_layer_surface_v1_set_exclusive_zone(imp()->layerSurface, exclusiveZone());
    }

    /* KEYBOARD INT CHANGE */

    if (imp()->pendingKeyboardInteractivity != imp()->currentKeyboardInteractivity)
    {
        imp()->currentKeyboardInteractivity = imp()->currentKeyboardInteractivity;
        zwlr_layer_surface_v1_set_keyboard_interactivity(imp()->layerSurface, keyboardInteractivity());
    }

    /* ANCHOR CHANGE */

    if (imp()->pendingAnchor.get() != imp()->currentAnchor.get())
    {
        imp()->currentAnchor = imp()->pendingAnchor;
        zwlr_layer_surface_v1_set_anchor(imp()->layerSurface, anchor().get());
        anchorChanged = true;
    }

    bool fullDamage {};

    SkISize newSize {
        SkScalarFloorToInt(layout().calculatedWidth()),
        SkScalarFloorToInt(layout().calculatedHeight())
    };

    if (newSize.fWidth < 8) newSize.fWidth = 8;
    if (newSize.fHeight < 8) newSize.fHeight = 8;

    bool sizeChanged = false;

    if (MSurface::imp()->viewportSize != newSize)
    {
        MSurface::imp()->viewportSize = newSize;
        fullDamage = true;
        sizeChanged = true;
    }

    SkISize eglWindowSize { newSize };

    sizeChanged |= MSurface::imp()->resizeBuffer(eglWindowSize);

    if (sizeChanged)
    {
        repaint = true;
        wp_viewport_set_source(wlViewport(),
                               wl_fixed_from_int(0),
                               wl_fixed_from_int(eglWindowSize.height() - newSize.height()),
                               wl_fixed_from_int(newSize.width()),
                               wl_fixed_from_int(newSize.height()));
    }

    if (sizeChanged || anchorChanged)
    {
        /* PREVENT 0 SIZE PROTOCOL ERROR */

        const Int32 finalW = imp()->requestAvailableWidth && anchor().hasAll(CZEdgeLeft | CZEdgeRight) ? 0 : newSize.width();
        const Int32 finalH = imp()->requestAvailableHeight && anchor().hasAll(CZEdgeTop | CZEdgeBottom) ? 0 : newSize.height();

        imp()->requestAvailableWidth = false;
        imp()->requestAvailableHeight = false;

        zwlr_layer_surface_v1_set_size(imp()->layerSurface, finalW, finalH);
    }

    repaint |= target()->isDirty() || target()->bakedNodesScale() != scale();

    if (!repaint)
    {
        wl_surface_commit(wlSurface());
        return;
    }

    SkRegion outDamage, outOpaque, outInvisible;
    auto ssImage { MSurface::imp()->swapchain->acquire() };
    PrepareTarget(*this, ssImage.value(), &outDamage, &outOpaque, &outInvisible, fullDamage);
    scene().render(target());
    AttachInputRegion(*this);
    AttachOpaqueRegion(*this, outOpaque);
    AttachInvisibleRegion(*this, outInvisible);
    PresentImage(*this, *ssImage, outDamage);
}

void MLayerSurface::onUpdate() noexcept
{
    auto &flags = MSurface::imp()->flags;
    using SF = MSurface::Imp::Flags;
    auto &tmpFlags = MSurface::imp()->tmpFlags;
    using STF = MSurface::Imp::TmpFlags;

    MSurface::onUpdate();

    /* SCREEN CHANGE */

    if (imp()->currentScreen != imp()->pendingScreen)
    {
        imp()->currentScreen = imp()->pendingScreen;
        imp()->reset();
    }

    /* SCOPE CHANGE */

    if (imp()->currentScope != imp()->pendingScope)
    {
        imp()->currentScope = imp()->pendingScope;
        imp()->reset();
    }

    if (!flags.has(SF::UserMapped))
    {
        if (flags.has(SF::PendingNullCommit))
            return;

        imp()->reset();
        return;
    }

    if (!imp()->layerSurface)
        imp()->createRole();

    if (flags.has(SF::PendingNullCommit))
    {
        flags.add(SF::PendingFirstConfigure);
        flags.remove(SF::PendingNullCommit);
        wl_surface_attach(wlSurface(), nullptr, 0, 0);
        wl_surface_commit(wlSurface());
        update();
        return;
    }

    if (flags.has(SF::PendingConfigureAck))
    {
        flags.remove(SF::PendingConfigureAck);
        zwlr_layer_surface_v1_ack_configure(imp()->layerSurface, imp()->configureSerial);
    }

    if (flags.has(SF::PendingFirstConfigure | SF::PendingNullCommit))
        return;

    if (tmpFlags.has(STF::ScaleChanged))
        flags.add(SF::ForceUpdate);

    render();
}

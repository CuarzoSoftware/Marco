#include "CZ/Marco/MLog.h"
#include <CZ/Marco/Private/MToplevelPrivate.h>
#include <CZ/Marco/Private/MSurfacePrivate.h>
#include <CZ/Marco/Nodes/MVibrancyView.h>
#include <CZ/Marco/Roles/MPopup.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/MTheme.h>

#include <CZ/Core/Events/CZLayoutEvent.h>
#include <CZ/Core/Events/CZWindowStateEvent.h>
#include <CZ/Core/Events/CZPointerButtonEvent.h>
#include <CZ/AK/AKLog.h>

#include <CZ/Ream/RSurface.h>
#include <CZ/Ream/WL/RWLSwapchain.h>

using namespace CZ;

MToplevel::MToplevel() noexcept : MSurface(Role::Toplevel)
{
    m_imp = std::make_unique<Imp>(*this);
    root()->installEventFilter(this);
    auto app { MApp::Get() };

    if (app->wl.xdgWmBase.version() < 5)
        imp()->pendingWMCaps = imp()->currentWMCaps = { WindowMenuCap | MinimizeCap | MaximizeCap | FullscreenCap };

    imp()->xdgSurface = xdg_wm_base_get_xdg_surface(app->wl.xdgWmBase, wlSurface());
    xdg_surface_add_listener(imp()->xdgSurface, &Imp::xdgSurfaceListener, this);
    imp()->xdgToplevel = xdg_surface_get_toplevel(imp()->xdgSurface);
    xdg_toplevel_add_listener(imp()->xdgToplevel, &Imp::xdgToplevelListener, this);
    xdg_toplevel_set_app_id(imp()->xdgToplevel, app->appId().c_str());

    app->onAppIdChanged.subscribe(this, [this](){
        xdg_toplevel_set_app_id(imp()->xdgToplevel, MApp::Get()->appId().c_str());
    });

    onMappedChanged.subscribe(this, [this](){

        if (!mapped())
            return;

        if (!childToplevels().empty())
        {
            MSurface::imp()->flags.add(MSurface::Imp::PendingChildren | MSurface::Imp::ForceUpdate);
            update();

            for (const auto &child : childToplevels())
            {
                child->MSurface::imp()->flags.add(MSurface::Imp::PendingParent | MSurface::Imp::ForceUpdate);
                child->update();
            }
        }

        if (parentToplevel())
        {
            MSurface::imp()->flags.add(MSurface::Imp::PendingParent | MSurface::Imp::ForceUpdate);
            update();
        }
    });

    /* CSD */

    for (int i = 0; i < 4; i++)
    {
        imp()->borderRadius[i].setParent(rootNode());
        imp()->borderRadius[i].layout().setPositionType(YGPositionTypeAbsolute);
        imp()->borderRadius[i].layout().setWidth(app->theme()->CSDBorderRadius);
        imp()->borderRadius[i].layout().setHeight(app->theme()->CSDBorderRadius);
        imp()->borderRadius[i].enableCustomBlendFunc(true);
        imp()->borderRadius[i].enableAutoDamage(false);
        imp()->borderRadius[i].setCustomBlendMode(RBlendMode::DstIn);

        /*
        imp()->borderRadius[i].opaqueRegion.setEmpty();
        imp()->borderRadius[i].reactiveRegion.setRect(
            SkIRect::MakeWH(app()->theme()->CSDBorderRadius, app()->theme()->CSDBorderRadius));*/
    }

    // TODO: update only when activated changes

    // TL
    imp()->borderRadius[0].setSrcTransform(CZTransform::Normal);
    imp()->borderRadius[0].layout().setPosition(YGEdgeLeft, 0);
    imp()->borderRadius[0].layout().setPosition(YGEdgeTop, 0);

    // TR
    imp()->borderRadius[1].setSrcTransform(CZTransform::Rotated90);
    imp()->borderRadius[1].layout().setPosition(YGEdgeRight, 0);
    imp()->borderRadius[1].layout().setPosition(YGEdgeTop, 0);

    // BR
    imp()->borderRadius[2].setSrcTransform(CZTransform::Rotated180);
    imp()->borderRadius[2].layout().setPosition(YGEdgeRight, 0);
    imp()->borderRadius[2].layout().setPosition(YGEdgeBottom, 0);

    // BL
    imp()->borderRadius[3].setSrcTransform(CZTransform::Rotated270);
    imp()->borderRadius[3].layout().setPosition(YGEdgeLeft, 0);
    imp()->borderRadius[3].layout().setPosition(YGEdgeBottom, 0);

    imp()->shadow.setParent(rootNode());
}

MToplevel::~MToplevel() noexcept
{
    while (!imp()->childPopups.empty())
        (*imp()->childPopups.begin())->setParent(nullptr);

    setParentToplevel(nullptr);

    if (imp()->xdgDecoration)
    {
        zxdg_toplevel_decoration_v1_destroy(imp()->xdgDecoration);
        imp()->xdgDecoration = nullptr;
    }

    xdg_toplevel_destroy(imp()->xdgToplevel);
    xdg_surface_destroy(imp()->xdgSurface);
}

void MToplevel::setMaximized(bool maximized) noexcept
{
    if (!wmCapabilities().has(MaximizeCap))
        return;

    if (maximized)
        xdg_toplevel_set_maximized(imp()->xdgToplevel);
    else
        xdg_toplevel_unset_maximized(imp()->xdgToplevel);
}

bool MToplevel::maximized() const noexcept
{
    return states().has(CZWinMaximized);
}

void MToplevel::setFullscreen(bool fullscreen, MScreen *screen) noexcept
{
    if (!wmCapabilities().has(FullscreenCap))
        return;

    if (fullscreen)
        xdg_toplevel_set_fullscreen(imp()->xdgToplevel, screen ? screen->wlOutput() : nullptr);
    else
        xdg_toplevel_unset_fullscreen(imp()->xdgToplevel);
}

bool MToplevel::fullscreen() const noexcept
{
    return states().has(CZWinFullscreen);
}

void MToplevel::setMinimized() noexcept
{
    if (!wmCapabilities().has(MinimizeCap))
        return;

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

const SkISize &MToplevel::suggestedBounds() const noexcept
{
    return imp()->currentSuggestedBounds;
}

void MToplevel::suggestedSizeChanged()
{
    if (suggestedSize().width() == 0)
    {
        if (layout().width().value == 0.f || layout().width().value == YGUndefined)
            layout().setWidthAuto();
    }
    else
        layout().setWidth(suggestedSize().width());

    if (suggestedSize().height() == 0)
    {
        if (layout().height().value == 0.f || layout().height().value == YGUndefined)
            layout().setHeightAuto();
    }
    else
        layout().setHeight(suggestedSize().height());

    onSuggestedSizeChanged.notify();
}

void MToplevel::suggestedBoundsChanged()
{
    onSuggestedSizeChanged.notify();
}

CZBitset<CZWindowState> MToplevel::states() const noexcept
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

const SkIRect &MToplevel::builtinDecorationMargins() const noexcept
{
    return imp()->shadowMargins;
}

const SkIRect &MToplevel::decorationMargins() const noexcept
{
    return imp()->userDecorationMargins;
}

void MToplevel::setDecorationMargins(const SkIRect &margins) noexcept
{
    SkIRect m { margins };

    if (m.fLeft < 0) m.fLeft = 0;
    if (m.fTop < 0) m.fTop = 0;
    if (m.fRight < 0) m.fRight = 0;
    if (m.fBottom < 0) m.fBottom = 0;

    if (imp()->userDecorationMargins == margins)
        return;

    imp()->userDecorationMargins = margins;

    if (decorationMode() == ClientSide && !builtinDecorationsEnabled())
        update();

    decorationMarginsChanged();
}

CZBitset<MToplevel::WMCapabilities> MToplevel::wmCapabilities() const noexcept
{
    return imp()->currentWMCaps;
}

bool MToplevel::showWindowMenu(const CZInputEvent &event, const SkIPoint &pos) noexcept
{
    if (!wmCapabilities().has(WindowMenuCap))
        return false;

    xdg_toplevel_show_window_menu(imp()->xdgToplevel, MApp::Get()->wl.seat, event.serial, pos.x(), pos.y());
    return true;
}

MToplevel::DecorationMode MToplevel::decorationMode() const noexcept
{
    return imp()->currentDecorationMode;
}

void MToplevel::setDecorationMode(DecorationMode mode) noexcept
{
    if (mode == ClientSide)
    {
        if (imp()->xdgDecoration)
        {
            zxdg_toplevel_decoration_v1_destroy(imp()->xdgDecoration);
            imp()->xdgDecoration = nullptr;
            update();
        }

        imp()->pendingDecorationMode = ClientSide;

        if (imp()->pendingDecorationMode != imp()->currentDecorationMode)
        {
            imp()->currentDecorationMode = ClientSide;
            decorationModeChanged();
        }
    }
    else
    {
        auto app { MApp::Get() };

        if (!app->wl.xdgDecorationManager)
            return;

        const bool wasMapped { mapped() };

        if (!imp()->xdgDecoration)
        {
            if (wasMapped)
                imp()->unmap();

            imp()->xdgDecoration = zxdg_decoration_manager_v1_get_toplevel_decoration(app->wl.xdgDecorationManager, imp()->xdgToplevel);
            zxdg_toplevel_decoration_v1_add_listener(imp()->xdgDecoration, &imp()->xdgDecorationListener, this);
            zxdg_toplevel_decoration_v1_set_mode(imp()->xdgDecoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);

            if (wasMapped)
                setMapped(true);
        }
        else
        {
            zxdg_toplevel_decoration_v1_set_mode(imp()->xdgDecoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
        }

        update();
    }
}

bool MToplevel::builtinDecorationsEnabled() const noexcept
{
    return MSurface::imp()->flags.has(MSurface::Imp::BuiltinDecorations);
}

void MToplevel::enableBuiltinDecorations(bool enabled) noexcept
{
    if (builtinDecorationsEnabled() == enabled)
        return;

    MSurface::imp()->flags.setFlag(MSurface::Imp::BuiltinDecorations, enabled);

    if (decorationMode() == ClientSide)
        update(true);
}

bool MToplevel::setParentToplevel(MToplevel *parent) noexcept
{
    if (parent == parentToplevel())
        return true;

    if (parent)
    {
        MToplevel *tl = parent;

        while (tl)
        {
            if (tl == this) return false;
            tl = tl->parentToplevel();
        }

        if (imp()->parentToplevel)
            imp()->parentToplevel->imp()->childToplevels.erase(this);

        // TODO: handle later
        imp()->parentToplevel.reset(parent);
        parent->imp()->childToplevels.insert(this);
        parent->MSurface::imp()->flags.add(MSurface::Imp::PendingChildren);
        MSurface::imp()->flags.add(MSurface::Imp::PendingParent);
        update();
    }
    else
    {
        if (imp()->parentToplevel)
            imp()->parentToplevel->imp()->childToplevels.erase(this);

        imp()->parentToplevel.reset();
        MSurface::imp()->flags.add(MSurface::Imp::PendingParent);
        update();
    }

    return true;
}

MToplevel *MToplevel::parentToplevel() const noexcept
{
    return imp()->parentToplevel;
}

const std::unordered_set<CZ::MToplevel *> &MToplevel::childToplevels() const noexcept
{
    return imp()->childToplevels;
}

const std::unordered_set<MPopup *> &MToplevel::childPopups() const noexcept
{
    return imp()->childPopups;
}

void MToplevel::wmCapabilitiesChanged()
{
    onWMCapabilitiesChanged.notify();
}

void MToplevel::decorationModeChanged()
{
    onDecorationModeChanged.notify();
}

void MToplevel::decorationMarginsChanged()
{

}

void MToplevel::windowStateEvent(const CZWindowStateEvent &event)
{
    MSurface::windowStateEvent(event);
    onStatesChanged.notify(event);
    event.accept();
}

void MToplevel::pointerButtonEvent(const CZPointerButtonEvent &event)
{
    if (event.pressed)
        for (auto &popup : childPopups())
            popup->setMapped(false);

    MSurface::pointerButtonEvent(event);
}

bool MToplevel::eventFilter(const CZEvent &event, CZObject &object) noexcept
{
    if (rootNode() == &object)
    {
        if (event.type() == CZEvent::Type::PointerButton)
            imp()->handleRootPointerButtonEvent(static_cast<const CZPointerButtonEvent&>(event));
        else if (event.type() == CZEvent::Type::PointerMove)
            imp()->handleRootPointerMoveEvent(static_cast<const CZPointerMoveEvent&>(event));
    }

   return MSurface::eventFilter(event, object);
}

bool MToplevel::event(const CZEvent &e) noexcept
{
    if (e.type() == CZEvent::Type::Close)
    {
        onBeforeClose.notify((const CZCloseEvent &)e);
        return e.isAccepted();
    }

    return MSurface::event(e);
}

void MToplevel::onUpdate() noexcept
{
    auto &flags = MSurface::imp()->flags;
    using SF = MSurface::Imp::Flags;
    auto &tmpFlags = MSurface::imp()->tmpFlags;
    using STF = MSurface::Imp::TmpFlags;
    auto app { MApp::Get() };

    MSurface::onUpdate();

    if (!flags.has(SF::UserMapped))
    {
        if (flags.has(SF::PendingNullCommit))
            return;

        if (mapped())
            imp()->unmap();

        return;
    }

    if (flags.has(SF::PendingNullCommit))
    {
        if (fullscreen())
            xdg_toplevel_set_fullscreen(imp()->xdgToplevel, nullptr);

        if (maximized())
            xdg_toplevel_set_maximized(imp()->xdgToplevel);

        xdg_toplevel_set_app_id(imp()->xdgToplevel, app->appId().c_str());
        xdg_toplevel_set_title(imp()->xdgToplevel, imp()->title.c_str());

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
        xdg_surface_ack_configure(imp()->xdgSurface, imp()->configureSerial);
    }

    if (flags.has(SF::PendingFirstConfigure | SF::PendingNullCommit))
        return;

    if (tmpFlags.has(STF::ScaleChanged))
        flags.add(SF::ForceUpdate);

    if (states().has(CZWinFullscreen) || decorationMode() == ServerSide || !builtinDecorationsEnabled())
    {
        if (decorationMode() == ServerSide || builtinDecorationsEnabled())
            imp()->setShadowMargins({ 0, 0, 0, 0 });
        else
            imp()->setShadowMargins(imp()->userDecorationMargins);

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
            imp()->setShadowMargins({
                app->theme()->CSDShadowActiveRadius,
                app->theme()->CSDShadowActiveRadius - app->theme()->CSDShadowActiveOffsetY,
                app->theme()->CSDShadowActiveRadius,
                app->theme()->CSDShadowActiveRadius + app->theme()->CSDShadowActiveOffsetY,
            });
        }
        else
        {
            imp()->setShadowMargins({
                app->theme()->CSDShadowInactiveRadius,
                app->theme()->CSDShadowInactiveRadius - app->theme()->CSDShadowInactiveOffsetY,
                app->theme()->CSDShadowInactiveRadius,
                app->theme()->CSDShadowInactiveRadius + app->theme()->CSDShadowInactiveOffsetY,
            });
        }

        imp()->borderRadius[0].layout().setPosition(YGEdgeLeft, imp()->shadowMargins.fLeft);
        imp()->borderRadius[0].layout().setPosition(YGEdgeTop, imp()->shadowMargins.fTop);
        imp()->borderRadius[1].layout().setPosition(YGEdgeRight, imp()->shadowMargins.fRight);
        imp()->borderRadius[1].layout().setPosition(YGEdgeTop, imp()->shadowMargins.fTop);
        imp()->borderRadius[2].layout().setPosition(YGEdgeRight, imp()->shadowMargins.fRight);
        imp()->borderRadius[2].layout().setPosition(YGEdgeBottom, imp()->shadowMargins.fBottom);
        imp()->borderRadius[3].layout().setPosition(YGEdgeLeft, imp()->shadowMargins.fLeft);
        imp()->borderRadius[3].layout().setPosition(YGEdgeBottom, imp()->shadowMargins.fBottom);
    }

    layout().setPosition(YGEdgeLeft, 0.f);
    layout().setPosition(YGEdgeTop, 0.f);
    layout().setMargin(YGEdgeLeft, imp()->shadowMargins.fLeft);
    layout().setMargin(YGEdgeTop, imp()->shadowMargins.fTop);
    layout().setMargin(YGEdgeRight, imp()->shadowMargins.fRight);
    layout().setMargin(YGEdgeBottom, imp()->shadowMargins.fBottom);

    render();
}

std::vector<std::string> splitString(const std::string& input, size_t chunkSize) {
    std::vector<std::string> chunks;

    for (size_t i = 0; i < input.size(); i += chunkSize) {
        // Create substrings of chunkSize
        chunks.push_back(input.substr(i, chunkSize));
    }

    return chunks;
}

void MToplevel::render() noexcept
{
    scene().root()->layout().calculate();

    auto app { MApp::Get() };
    bool overflow { false };
    SkRect bounds;
    innerBounds(&bounds);

    if (bounds.width() > layout().calculatedWidth())
    {
        overflow = true;
        layout().setWidth(bounds.width());
    }

    if (bounds.height() > layout().calculatedHeight())
    {
        overflow = true;
        layout().setHeight(bounds.height());
    }

    if (overflow)
        scene().root()->layout().calculate();

    if (wlCallback() && !MSurface::imp()->flags.has(MSurface::Imp::ForceUpdate))
        return;

    bool repaint { false };

    MSurface::imp()->flags.remove(MSurface::Imp::ForceUpdate);

    bool fullDamage {};

    SkISize newSize {
        SkScalarFloorToInt(layout().calculatedWidth() + layout().calculatedMargin(YGEdgeLeft) + layout().calculatedMargin(YGEdgeRight)),
        SkScalarFloorToInt(layout().calculatedHeight() + layout().calculatedMargin(YGEdgeTop) + layout().calculatedMargin(YGEdgeBottom))
    };

    if (newSize.fWidth < 128) newSize.fWidth = 128;
    if (newSize.fHeight < 128) newSize.fHeight = 128;

    bool sizeChanged = false;

    if (MSurface::imp()->viewportSize != newSize)
    {
        MSurface::imp()->viewportSize = newSize;
        fullDamage = true;
        sizeChanged = true;
    }

    SkISize eglWindowSize { newSize };

    if (states().has(CZWinResizing))
    {
        eglWindowSize = bufferSize();
        eglWindowSize.fWidth /= scale();
        eglWindowSize.fHeight /= scale();

        if (eglWindowSize.width() < newSize.width())
            eglWindowSize.fWidth = newSize.width() * 1.75f;

        if (eglWindowSize.height() < newSize.height())
            eglWindowSize.fHeight = newSize.height() * 1.75f;
    }

    sizeChanged |= MSurface::imp()->resizeBuffer(eglWindowSize);

    if (sizeChanged)
    {
        repaint = true;
        app->update();

        wp_viewport_set_source(wlViewport(),
            wl_fixed_from_int(0),
            wl_fixed_from_int(0),
            wl_fixed_from_int(newSize.width()),
            wl_fixed_from_int(newSize.height()));

        xdg_surface_set_window_geometry(
            imp()->xdgSurface,
            imp()->shadowMargins.fLeft,
            imp()->shadowMargins.fTop,
            layout().calculatedWidth(),
            layout().calculatedHeight());
    }

    repaint |= target()->isDirty() || target()->bakedNodesScale() != scale();

    if (!repaint)
    {
        imp()->applyPendingParent();
        imp()->applyPendingChildren();
        wl_surface_commit(wlSurface());
        return;
    }

    SkRegion outDamage, outOpaque, outInvisible;
    auto ssImage { MSurface::imp()->swapchain->acquire() };
    PrepareTarget(*this, ssImage.value(), &outDamage, &outOpaque, &outInvisible, fullDamage);

    /* CSD */
    if (decorationMode() == ClientSide && builtinDecorationsEnabled())
        for (int i = 0; i < 4; i++)
            imp()->borderRadius[i].setImage(app->theme()->csdBorderRadiusMask(scale()));

    scene().render(target());

    /* Vibrancy */
    if (MSurface::imp()->backgroundBlur/* && app->wl.svgPathManager*/)
    {
        if (vibrancyState() == AKVibrancyState::Enabled && (SkColorGetA(m_color) < 255 || opacity() < 1.f))
        {
            std::vector<MVibrancyView*> vibrancyViews;
            vibrancyViews.reserve(10);
            FindNodesWithType(this, &vibrancyViews);

            wl_region *region = wl_compositor_create_region(app->wl.compositor);

            for (MVibrancyView *view : vibrancyViews)
                wl_region_add(region, view->worldRect().x(), view->worldRect().y(), view->worldRect().width(), view->worldRect().height());

            lvr_background_blur_set_region(MSurface::imp()->backgroundBlur, region);
            wl_region_destroy(region);

            int r = !builtinDecorationsEnabled() || states().has(CZWinFullscreen) ? 0 : MTheme::CSDBorderRadius;
            int x = imp()->shadowMargins.fLeft;
            int y = imp()->shadowMargins.fTop;
            int w = layout().calculatedWidth();
            int h = layout().calculatedHeight();
            lvr_background_blur_set_round_rect_mask(MSurface::imp()->backgroundBlur,
                                                x, y, w, h,
                                                r, r, r, r);
        }
        else
        {
            wl_region *empty = wl_compositor_create_region(app->wl.compositor);
            lvr_background_blur_set_region(MSurface::imp()->backgroundBlur, empty);
            wl_region_destroy(empty);
        }
    }

    if (true || opacity() == 1.f)
        if (decorationMode() == ClientSide && builtinDecorationsEnabled())
            for (int i = 0; i < 4; i++)
                target()->outOpaque->op(imp()->borderRadius[i].worldRect(), SkRegion::Op::kDifference_Op);

    AttachInputRegion(*this);
    AttachOpaqueRegion(*this, outOpaque);
    AttachInvisibleRegion(*this, outInvisible);
    PresentImage(*this, *ssImage, outDamage);

    imp()->applyPendingParent();
    imp()->applyPendingChildren();
}

MToplevel::Imp *MToplevel::imp() const noexcept
{
    return m_imp.get();
}

#include <CZ/Marco/Private/MSurfacePrivate.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/MTheme.h>
#include <CZ/Marco/Roles/MSubsurface.h>
#include <CZ/Marco/Nodes/MVibrancyView.h>
#include <CZ/Core/Utils/CZRegionUtils.h>
#include <CZ/Core/CZSafeEventQueue.h>
#include <CZ/Ream/RSurface.h>
#include <CZ/Ream/WL/RWLSwapchain.h>
#include <CZ/AK/AKColors.h>
#include <CZ/AK/Events/AKVibrancyEvent.h>

using namespace CZ;

MSurface::MSurface(Role role) noexcept : CZ::AKSolidColor(SK_ColorWHITE)
{
    auto app { MApp::Get() };
    userCaps.add(UCWindowMove);
    m_imp = std::make_unique<Imp>(*this);
    imp()->scene = AKScene::Make();
    imp()->scene->m_win->window = this;
    imp()->role = role;
    imp()->createSurface();

    imp()->appLink = app->m_surfaces.size();
    app->m_surfaces.push_back(this);
    setParent(rootNode());
    enableChildrenClipping(true);

    // Rounded-corner masks: children of root rendered right after the central node (content) and
    // before any decorations, so their DstIn rounds the content without carving the shadow. They
    // are positioned/sized per corner in updateBorderRadiusMasks() and hidden until a radius is set.
    for (int i = 0; i < 4; i++)
    {
        imp()->cornerRadius[i].setParent(rootNode());
        imp()->cornerRadius[i].layout().setPositionType(YGPositionTypeAbsolute);
        imp()->cornerRadius[i].enableCustomBlendFunc(true);
        imp()->cornerRadius[i].enableAutoDamage(false);
        imp()->cornerRadius[i].setCustomBlendMode(RBlendMode::DstIn);
        imp()->cornerRadius[i].setVisible(false);
    }
    imp()->cornerRadius[0].setSrcTransform(CZTransform::Normal);     // TL
    imp()->cornerRadius[1].setSrcTransform(CZTransform::Rotated90);  // TR
    imp()->cornerRadius[2].setSrcTransform(CZTransform::Rotated180); // BR
    imp()->cornerRadius[3].setSrcTransform(CZTransform::Rotated270); // BL

    imp()->target = scene().makeTarget();
    scene().setRoot(rootNode());

    target()->onMarkedDirty.subscribe(this, [this](CZ::AKTarget&){
        update();
    });

    app->onScreenUnplugged.subscribe(this, [this](MScreen &screen){
        Imp::wl_surface_leave(this, wlSurface(), screen.wlOutput());
    });
}

MSurface::~MSurface()
{
    while (!subSurfaces().empty())
        subSurfaces().back()->setParent(nullptr);

    auto app { MApp::Get() };
    app->m_surfaces[imp()->appLink] = app->m_surfaces.back();
    app->m_surfaces.pop_back();
    imp()->resizeBuffer({0, 0});

    if (wlCallback())
    {
        wl_callback_destroy(wlCallback());
        imp()->wlCallback = nullptr;
    }

    if (imp()->invisibleRegion)
    {
        lvr_invisible_region_destroy(imp()->invisibleRegion);
        imp()->invisibleRegion = nullptr;
    }

    if (wlSurface())
    {
        wl_surface_destroy(wlSurface());
        imp()->wlSurface = nullptr;
    }

    notifyDestruction();
}

MSurface::Role MSurface::role() noexcept
{
    return imp()->role;
}

Int32 MSurface::scale() noexcept
{
    return imp()->scale;
}

const SkISize &MSurface::surfaceSize() const noexcept
{
    if (imp()->viewportSize.width() >= 0)
        return imp()->viewportSize;

    return imp()->size;
}

const SkISize &MSurface::bufferSize() const noexcept
{
    return imp()->bufferSize;
}

const std::set<MScreen *> &MSurface::screens() const noexcept
{
    return imp()->screens;
}

void MSurface::setMapped(bool mapped) noexcept
{
    if (imp()->flags.has(Imp::UserMapped) == mapped)
        return;

    imp()->flags.setFlag(Imp::UserMapped, mapped);
    update();
}

bool MSurface::mapped() const noexcept
{
    return imp()->flags.has(Imp::Mapped);
}

void MSurface::update(bool force) noexcept
{
    imp()->flags.add(Imp::PendingUpdate);

    if (force)
        imp()->flags.add(Imp::ForceUpdate);

    MApp::Get()->update();
}

SkISize MSurface::minContentSize() noexcept
{
    const YGValue width { layout().width() };
    const YGValue height { layout().height() };
    layout().setWidthAuto();
    layout().setHeightAuto();
    rootNode()->layout().calculate();
    const SkISize contentSize { SkISize::Make(layout().calculatedWidth(), layout().calculatedHeight()) };
    layout().setWidthYGValue(width);
    layout().setHeightYGValue(height);
    rootNode()->layout().calculate();
    return contentSize;
}

const std::list<MSubsurface *> &MSurface::subSurfaces() const noexcept
{
    return imp()->subSurfaces;
}

AKScene &MSurface::scene() const noexcept
{
    return *imp()->scene;
}

std::shared_ptr<AKTarget> MSurface::target() const noexcept
{
    return imp()->target;
}

AKNode *MSurface::rootNode() const noexcept
{
    return &imp()->root;
}

void MSurface::setDecorations(std::unique_ptr<MDecorations> decorations) noexcept
{
    if (imp()->decorations == decorations) return;

    if (imp()->decorations) // Unbind
        imp()->decorations->setSurface(nullptr);

    imp()->decorations = std::move(decorations);

    if (imp()->decorations) // Bind
        imp()->decorations->setSurface(this);

    syncDecorationsMargins();
    decorationsChanged();
    update(true);
}

void MSurface::decorationsChanged() noexcept {}

MDecorations *MSurface::decorations() const noexcept
{
    return imp()->decorations.get();
}

bool MSurface::decorationsEnabled() const noexcept
{
    return imp()->flags.has(Imp::DecorationsEnabled);
}

void MSurface::enableDecorations(bool enabled) noexcept
{
    if (decorationsEnabled() == enabled)
        return;

    imp()->flags.setFlag(Imp::DecorationsEnabled, enabled);
    syncDecorationsMargins();
    update(true);
}

bool MSurface::decorationsActive() const noexcept
{
    return imp()->decorations && decorationsEnabled();
}

const CZRRect &MSurface::borderRadius() const noexcept
{
    return imp()->borderRadius;
}

void MSurface::setBorderRadius(const CZRRect &borderRadius) noexcept
{
    CZRRect &br { imp()->borderRadius };

    if (br.fRadTL == borderRadius.fRadTL && br.fRadTR == borderRadius.fRadTR &&
        br.fRadBR == borderRadius.fRadBR && br.fRadBL == borderRadius.fRadBL)
        return;

    // Only the corner radii are user-settable; MSurface owns the rect (see updateBorderRadiusMasks).
    br.fRadTL = borderRadius.fRadTL;
    br.fRadTR = borderRadius.fRadTR;
    br.fRadBR = borderRadius.fRadBR;
    br.fRadBL = borderRadius.fRadBL;

    addChange(BorderRadius);
    updateBorderRadiusMasks();
    onBorderRadiusChanged.notify();
    update(true);
}

wl_surface *MSurface::wlSurface() const noexcept
{
    return imp()->wlSurface;
}

wl_callback *MSurface::wlCallback() const noexcept
{
    return imp()->wlCallback;
}

wp_viewport *MSurface::wlViewport() const noexcept
{
    return imp()->wlViewport;
}

lvr_invisible_region *MSurface::wlInvisibleRegion() const noexcept
{
    return imp()->invisibleRegion;
}

AKVibrancyState MSurface::vibrancyState() const noexcept
{
    return imp()->currentVibrancyState;
}

void MSurface::vibrancyEvent(const AKVibrancyEvent &event)
{
    CZSafeEventQueue queue;
    AKNode::Iterator it { bottommostLeftChild() };

    while (!it.done())
    {
        if (it.node() != this)
            queue.addEvent(event.copy(), (AKObject&)(*it.node()));

        it.next();
    }

    queue.dispatch();
    onVibrancyChanged.notify(event);
}

void MSurface::onUpdate() noexcept
{
    if (imp()->tmpFlags.has(Imp::PreferredScaleChanged))
    {
        imp()->scale = imp()->preferredBufferScale;
        imp()->tmpFlags.add(Imp::ScaleChanged);
    }
    else if (imp()->preferredBufferScale == -1 && imp()->tmpFlags.has(Imp::ScreensChanged))
    {
        Int32 maxScale { 1 };

        for (const auto &screen : imp()->screens)
            if (screen->props().scale > maxScale)
                maxScale = screen->props().scale;

        if (maxScale != scale())
        {
            imp()->scale = maxScale;
            imp()->tmpFlags.add(Imp::ScaleChanged);
        }
    }

    // Corner masks are raster images baked at the surface scale; refresh them when it changes.
    if (imp()->tmpFlags.has(Imp::ScaleChanged))
        updateBorderRadiusMasks();
}

bool MSurface::event(const CZEvent &event) noexcept
{
    if (event.type() == CZEvent::Type::Vibrancy)
    {
        vibrancyEvent((const AKVibrancyEvent&)event);
        return true;
    }

    return AKSolidColor::event(event);
}

void MSurface::PrepareTarget(MSurface &window, const RSwapchainImage &ssImage, SkRegion *outDamage, SkRegion *outOpaque, SkRegion *outInvisible, bool forceFullDamage) noexcept
{
    auto surface { RSurface::WrapImage(ssImage.image) };
    RSurfaceGeometry geo {};
    geo.dst.setWH(
        window.surfaceSize().width() * window.scale(),
        window.surfaceSize().height() * window.scale());
    geo.viewport.setWH(
        window.surfaceSize().width(),
        window.surfaceSize().height());
    surface->setGeometry(geo);
    window.target()->surface = surface;
    window.target()->layoutOnRender = false;
    window.target()->age = forceFullDamage ? 0 : ssImage.age;
    window.target()->outDamage = outDamage;
    window.target()->setBakedNodesScale(window.scale());

    wl_surface_set_buffer_scale(window.wlSurface(), window.scale());

    if (true || window.opacity() == 1.f)
        window.target()->outOpaque = outOpaque;
    else
        window.target()->outOpaque = nullptr;

    if (window.wlInvisibleRegion())
        window.target()->outInvisible = outInvisible;
    else
        window.target()->outInvisible = nullptr;
}

void MSurface::AttachInputRegion(MSurface &window) noexcept
{
    // This input region is set to prevent decorations to be considered
    // part of the window when clicked by the compositor.
    // An outset of 6 is added for toplevel resize zones
    auto rect { window.worldRect().makeOutset(6, 6) };
    auto *region { wl_compositor_create_region(MApp::Get()->wl.compositor) };
    wl_region_add(region, rect.x(), rect.y(), rect.width(), rect.height());
    wl_surface_set_input_region(window.wlSurface(), region);
    wl_region_destroy(region);
}

void MSurface::AttachOpaqueRegion(MSurface &window, SkRegion &outOpaque) noexcept
{
    // Rounded corners aren't opaque: carve each visible corner mask out of the opaque region.
    for (int i = 0; i < 4; i++)
        if (window.imp()->cornerRadius[i].visible())
            outOpaque.op(window.imp()->cornerRadius[i].worldRect(), SkRegion::Op::kDifference_Op);

    // Decorations may carve additional non-opaque areas.
    if (window.decorationsActive())
        window.decorations()->subtractOpaque(outOpaque);

    wl_region *wlOpaqueRegion = wl_compositor_create_region(MApp::Get()->wl.compositor);
    SkRegion::Iterator opaqueIt { outOpaque };
    while (!opaqueIt.done())
    {
        wl_region_add(wlOpaqueRegion, opaqueIt.rect().x(), opaqueIt.rect().y(), opaqueIt.rect().width(), opaqueIt.rect().height());
        opaqueIt.next();
    }
    wl_surface_set_opaque_region(window.wlSurface(), wlOpaqueRegion);
    wl_region_destroy(wlOpaqueRegion);
}

void MSurface::AttachInvisibleRegion(MSurface &window, SkRegion &outInvisible) noexcept
{
    if (window.wlInvisibleRegion())
    {
        wl_region *wlRegion = wl_compositor_create_region(MApp::Get()->wl.compositor);
        SkRegion::Iterator invisibleIt { outInvisible };
        while (!invisibleIt.done())
        {
            wl_region_add(wlRegion,
                          invisibleIt.rect().x(), invisibleIt.rect().y(),
                          invisibleIt.rect().width(), invisibleIt.rect().height());
            invisibleIt.next();
        }
        lvr_invisible_region_set_region(window.wlInvisibleRegion(), wlRegion);
        wl_region_destroy(wlRegion);
    }
}

void MSurface::PresentImage(MSurface &window, const RSwapchainImage &ssImage, SkRegion &outDamage) noexcept
{
    CZRegionUtils::Scale(outDamage, window.scale());
    window.MSurface::imp()->createCallback();
    window.MSurface::imp()->swapchain->present(ssImage, &outDamage);
}

void MSurface::syncDecorationsMargins() noexcept
{
    SkIRect m { 0, 0, 0, 0 };

    if (decorationsActive())
    {
        imp()->decorations->setVisible(true);
        m = imp()->decorations->margins();
    }
    else if (imp()->decorations)
        imp()->decorations->setVisible(false);

    layout().setPosition(YGEdgeLeft, 0.f);
    layout().setPosition(YGEdgeTop, 0.f);
    layout().setMargin(YGEdgeLeft, m.fLeft);
    layout().setMargin(YGEdgeTop, m.fTop);
    layout().setMargin(YGEdgeRight, m.fRight);
    layout().setMargin(YGEdgeBottom, m.fBottom);

    // The content corners moved with the margins; re-anchor the rounded-corner masks.
    updateBorderRadiusMasks();
}

void MSurface::updateBorderRadiusMasks() noexcept
{
    // Decoration margins inset the central node within root; the masks sit at its corners.
    SkIRect m { 0, 0, 0, 0 };
    if (decorationsActive())
        m = imp()->decorations->margins();

    auto app { MApp::Get() };
    const CZRRect &br { imp()->borderRadius };
    const Int32 radii[4] { br.fRadTL, br.fRadTR, br.fRadBR, br.fRadBL };

    for (int i = 0; i < 4; i++)
    {
        auto &node { imp()->cornerRadius[i] };
        const Int32 r { std::max(0, radii[i]) };

        if (r == 0)
        {
            node.setVisible(false);
            continue;
        }

        node.setVisible(true);
        node.setImage(app->theme()->csdBorderRadiusMask(scale(), r));
        node.layout().setWidth(r);
        node.layout().setHeight(r);
    }

    // Edge-anchored so the corners follow the content when the surface resizes.
    imp()->cornerRadius[0].layout().setPosition(YGEdgeLeft,   m.fLeft);   // TL
    imp()->cornerRadius[0].layout().setPosition(YGEdgeTop,    m.fTop);
    imp()->cornerRadius[1].layout().setPosition(YGEdgeRight,  m.fRight);  // TR
    imp()->cornerRadius[1].layout().setPosition(YGEdgeTop,    m.fTop);
    imp()->cornerRadius[2].layout().setPosition(YGEdgeRight,  m.fRight);  // BR
    imp()->cornerRadius[2].layout().setPosition(YGEdgeBottom, m.fBottom);
    imp()->cornerRadius[3].layout().setPosition(YGEdgeLeft,   m.fLeft);   // BL
    imp()->cornerRadius[3].layout().setPosition(YGEdgeBottom, m.fBottom);

    // Keep the stored rect equal to the content area (radii are authoritative; rect is best-effort).
    imp()->borderRadius.fLeft   = m.fLeft;
    imp()->borderRadius.fTop    = m.fTop;
    imp()->borderRadius.fRight  = m.fLeft + Int32(layout().calculatedWidth());
    imp()->borderRadius.fBottom = m.fTop  + Int32(layout().calculatedHeight());
}

MSurface::Imp *MSurface::imp() const noexcept
{
    return m_imp.get();
}

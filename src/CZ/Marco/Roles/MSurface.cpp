#include <CZ/Marco/Private/MSurfacePrivate.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/Roles/MSubsurface.h>
#include <CZ/Marco/Nodes/MVibrancyView.h>
#include <CZ/Utils/CZRegionUtils.h>
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

AKVibrancyStyle MSurface::vibrancyStyle() const noexcept
{
    return imp()->currentVibrancyStyle;
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

MSurface::Imp *MSurface::imp() const noexcept
{
    return m_imp.get();
}

#include <CZ/Marco/Private/MSessionLockSurfacePrivate.h>
#include <CZ/Marco/Private/MSurfacePrivate.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/MLog.h>
#include <CZ/Ream/WL/RWLSwapchain.h>

using namespace CZ;

MSessionLockSurface::MSessionLockSurface(MScreen *screen) noexcept :
    MSurface(Role::SessionLockSurface)
{
    m_imp = std::make_unique<Imp>(*this);
    imp()->screen.reset(screen);

    if (auto *manager { MApp::Get()->sessionLockManager() })
    {
        imp()->manager.reset(manager);
        manager->m_surfaces.insert(this);
    }
    else
        MLog(CZWarning, CZLN, "ext_session_lock_manager_v1 not supported by the compositor: the lock surface won't be shown");
}

MSessionLockSurface::~MSessionLockSurface() noexcept
{
    if (imp()->lockSurface)
    {
        ext_session_lock_surface_v1_destroy(imp()->lockSurface);
        imp()->lockSurface = nullptr;
    }

    if (auto *manager { imp()->manager.get() })
    {
        manager->m_surfaces.erase(this);

        // Another one waiting for its screen may take it now
        for (MSessionLockSurface *other : manager->m_surfaces)
            if (!other->imp()->lockSurface)
                other->update();
    }
}

bool MSessionLockSurface::setScreen(MScreen *screen) noexcept
{
    if (screen == imp()->screen)
        return false;

    imp()->screen.reset(screen);
    update();
    return true;
}

MScreen *MSessionLockSurface::screen() const noexcept
{
    return imp()->screen;
}

const SkISize &MSessionLockSurface::suggestedSize() const noexcept
{
    return imp()->suggestedSize;
}

MSessionLockSurface::Imp *MSessionLockSurface::imp() const noexcept
{
    return m_imp.get();
}

void MSessionLockSurface::suggestedSizeChanged()
{
    // The whole surface, decoration margins included (as for layer surfaces)
    if (suggestedSize().width() != 0)
        layout().setWidth(std::max(1.f, suggestedSize().width()
            - layout().calculatedMargin(YGEdgeLeft) - layout().calculatedMargin(YGEdgeRight)));

    if (suggestedSize().height() != 0)
        layout().setHeight(std::max(1.f, suggestedSize().height()
            - layout().calculatedMargin(YGEdgeTop) - layout().calculatedMargin(YGEdgeBottom)));
}

void MSessionLockSurface::render() noexcept
{
    scene().root()->layout().calculate();

    if (!ShouldUpdate(*this)) return;

    // Exactly the configured size (dimensions_mismatch protocol error otherwise), whatever the layout
    const SkISize newSize { suggestedSize().isEmpty() ? CalculatedSize(*this) : suggestedSize() };
    const bool sizeChanged { MSurface::imp()->resizeBuffer(newSize) };
    const bool repaint { !MSurface::imp()->flags.has(MSurface::Imp::HasBufferAttached) || sizeChanged || target()->isDirty() ||
                         target()->bakedNodesScale() != scale() || changes().testAnyOf(CHDecorationMargins) };

    if (!repaint)
    {
        wl_surface_commit(wlSurface());
        return;
    }

    SkRegion outDamage, outOpaque, outInvisible;
    auto ssImage { MSurface::imp()->swapchain->acquire() };
    PrepareTarget(*this, ssImage.value(), &outDamage, &outOpaque, &outInvisible, sizeChanged);
    scene().render(target());
    HandleBackgroundBlur(*this);
    AttachInputRegion(*this);
    AttachOpaqueRegion(*this, outOpaque);
    AttachInvisibleRegion(*this, outInvisible);
    PresentImage(*this, *ssImage, outDamage);
}

void MSessionLockSurface::onUpdate() noexcept
{
    auto &flags = MSurface::imp()->flags;
    using SF = MSurface::Imp::Flags;
    auto &tmpFlags = MSurface::imp()->tmpFlags;
    using STF = MSurface::Imp::TmpFlags;

    MSurface::onUpdate();

    const bool wanted { imp()->wanted() };

    // Not needed anymore, or moved to another screen (a new role for it)
    if (imp()->lockSurface && (!wanted || imp()->roleScreen != imp()->screen))
        imp()->reset();

    if (!wanted)
        return;

    if (!imp()->lockSurface && !imp()->createRole())
        return;

    if (flags.has(SF::PendingConfigureAck))
    {
        flags.remove(SF::PendingConfigureAck);
        ext_session_lock_surface_v1_ack_configure(imp()->lockSurface, imp()->configureSerial);
    }

    // Nothing committed before the first configure is acked
    if (flags.has(SF::PendingFirstConfigure))
        return;

    if (tmpFlags.has(STF::ScaleChanged))
        flags.add(SF::ForceUpdate);

    render();
}

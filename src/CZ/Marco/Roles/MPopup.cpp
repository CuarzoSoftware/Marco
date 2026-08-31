#include <CZ/Marco/Private/MPopupPrivate.h>
#include <CZ/Marco/Private/MToplevelPrivate.h>
#include <CZ/Marco/Private/MLayerSurfacePrivate.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/MLog.h>
#include <CZ/Core/Events/CZInputEvent.h>
#include <CZ/Ream/WL/RWLSwapchain.h>
#include <CZ/Marco/Nodes/MVibrancyView.h>

using namespace CZ;

MPopup::MPopup() noexcept : MSurface(MSurface::Role::Popup)
{
    m_imp = std::make_unique<Imp>(*this);
}

MPopup::~MPopup()
{
    setParent(nullptr);
    imp()->destroy();

    while (!imp()->childPopups.empty())
        (*imp()->childPopups.begin())->setParent(nullptr);

    if (imp()->xdgSurface)
    {
        xdg_surface_destroy(imp()->xdgSurface);
        imp()->xdgSurface = nullptr;
    }
}

bool MPopup::setParent(MSurface *parent) noexcept
{
    if (parent == this->parent())
        return true;

    if (parent)
    {
        if (parent->role() != Role::Toplevel && parent->role() != Role::Popup && parent->role() != Role::LayerSurface)
        {
            MLog(CZError, CZLN, "Popups can only be children of MToplevel, MPopup or MLayerSurface");
            return false;
        }

        // Check circular dependency
        if (parent->role() == Role::Popup)
        {
            MPopup *popupParent { static_cast<MPopup*>(parent) };

            while (popupParent)
            {
                if (popupParent == this)
                    return false;

                if (popupParent->parent() && popupParent->parent()->role() == Role::Popup)
                    popupParent = static_cast<MPopup*>(popupParent->parent());
                else
                    break;
            }
        }

        imp()->setParent(parent);
    }
    else
    {
        imp()->unsetParent();
    }

    imp()->paramsChanged = true;
    update();
    return true;
}

MSurface *MPopup::parent() const noexcept
{
    return imp()->parent;
}

const std::unordered_set<MPopup *> &MPopup::childPopups() const noexcept
{
    return imp()->childPopups;
}

void MPopup::setAnchorRect(const SkIRect &rect) noexcept
{
    if (anchorRect() == rect)
        return;

    imp()->paramsChanged = true;
    imp()->anchorRect = rect;
    update();
}

const SkIRect &MPopup::anchorRect() const noexcept
{
    return imp()->anchorRect;
}

void MPopup::setAnchor(Anchor anchor) noexcept
{
    if (this->anchor() == anchor)
        return;

    imp()->paramsChanged = true;
    imp()->anchor = anchor;
    update();
}

MPopup::Anchor MPopup::anchor() const noexcept
{
    return imp()->anchor;
}

void MPopup::setGravity(Gravity gravity) noexcept
{
    if (this->gravity() == gravity)
        return;

    imp()->paramsChanged = true;
    imp()->gravity = gravity;
    update();
}

MPopup::Gravity MPopup::gravity() const noexcept
{
    return imp()->gravity;
}

void MPopup::setConstraintAdjustment(CZBitset<ConstraintAdjustment> adjustment) noexcept
{
    adjustment &= ConstraintAdjustment::All;

    if (adjustment.get() == constraintAdjustment().get())
        return;

    imp()->paramsChanged = true;
    imp()->constraintAdjustment.set(adjustment.get());
    update();
}

CZBitset<MPopup::ConstraintAdjustment> MPopup::constraintAdjustment() const noexcept
{
    return imp()->constraintAdjustment;
}

void MPopup::setOffset(const SkIPoint &offset) noexcept
{
    if (this->offset() == offset)
        return;

    imp()->paramsChanged = true;
    imp()->offset = offset;
    update();
}

const SkIPoint &MPopup::offset() const noexcept
{
    return imp()->offset;
}

void MPopup::setGrab(const CZInputEvent *event) noexcept
{
    imp()->paramsChanged = true;
    if (event)
        imp()->grab = std::static_pointer_cast<CZInputEvent>(event->copy());
    else
        imp()->grab.reset();
    update();
}

CZInputEvent *MPopup::grab() const noexcept
{
    return imp()->grab.get();
}

const SkIRect &MPopup::assignedRect() const noexcept
{
    return imp()->currentAssignedRect;
}

MPopup::Imp *MPopup::imp() const noexcept
{
    return m_imp.get();
}

void MPopup::closeEvent(const CZCloseEvent &event)
{
    onClose.notify(event);
}

void MPopup::assignedRectChanged()
{
    onAssignedRectChanged.notify();
    layout().setWidth(assignedRect().width());
    layout().setHeight(assignedRect().height());
}

bool MPopup::event(const CZEvent &e) noexcept
{
    if (e.type() == CZEvent::Type::Close)
    {
        closeEvent((const CZCloseEvent&)e);
        return true;
    }

    return MSurface::event(e);
}

void MPopup::onUpdate() noexcept
{
    auto &flags = MSurface::imp()->flags;
    using SF = MSurface::Imp::Flags;
    auto &tmpFlags = MSurface::imp()->tmpFlags;
    using STF = MSurface::Imp::TmpFlags;

    MSurface::onUpdate();

    if (imp()->paramsChanged)
    {
        imp()->paramsChanged = false;
        imp()->destroy();
    }

    if (!flags.has(SF::UserMapped) || !parent() || !parent()->mapped())
    {
        return imp()->destroy();
    }

    if (flags.has(SF::PendingNullCommit))
    {
        flags.add(SF::PendingFirstConfigure);
        flags.remove(SF::PendingNullCommit);
        imp()->create();
        update();
        return;
    }

    if (!imp()->xdgPopup)
        return;

    if (flags.has(SF::PendingConfigureAck))
    {
        flags.remove(SF::PendingConfigureAck);
        xdg_surface_ack_configure(imp()->xdgSurface, imp()->configureSerial);
    }

    if (flags.has(SF::PendingFirstConfigure | SF::PendingNullCommit))
    {
        return;
    }

    MSurface::imp()->setMapped(true);

    if (tmpFlags.has(STF::ScaleChanged))
        flags.add(SF::ForceUpdate);

    layout().setPosition(YGEdgeLeft, 0.f);
    layout().setPosition(YGEdgeTop, 0.f);
    layout().setMargin(YGEdgeAll, 0.f);
    render();
}

void MPopup::render() noexcept
{
    auto app { MApp::Get() };
    scene().root()->layout().calculate();

    if (!ShouldUpdate(*this)) return;

    const SkISize newSize { CalculatedSize(*this) };
    const bool sizeChanged { MSurface::imp()->resizeBuffer(newSize) };
    const bool fullDamage { sizeChanged };
    const bool repaint { !MSurface::imp()->flags.has(MSurface::Imp::HasBufferAttached) || sizeChanged || target()->isDirty() || target()->bakedNodesScale() != scale() || changes().testAnyOf(CHDecorationMargins)};

    if (sizeChanged)
        xdg_surface_set_window_geometry(
            imp()->xdgSurface,
            layout().calculatedMargin(YGEdgeLeft),
            layout().calculatedMargin(YGEdgeTop),
            layout().calculatedWidth(),
            layout().calculatedHeight());

    if (!repaint)
    {
        wl_surface_commit(wlSurface());
        return;
    }

    SkRegion outDamage, outOpaque, outInvisible;
    auto ssImage { MSurface::imp()->swapchain->acquire() };
    PrepareTarget(*this, ssImage.value(), &outDamage, &outOpaque, &outInvisible, fullDamage);
    scene().render(target());
    HandleBackgroundBlur(*this);
    AttachInputRegion(*this);
    AttachOpaqueRegion(*this, outOpaque);
    AttachInvisibleRegion(*this, outInvisible);
    PresentImage(*this, *ssImage, outDamage);
}


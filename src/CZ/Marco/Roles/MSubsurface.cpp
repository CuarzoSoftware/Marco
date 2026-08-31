#include <CZ/Marco/Private/MSubsurfacePrivate.h>
#include <CZ/Marco/Private/MSurfacePrivate.h>
#include <CZ/Marco/Roles/MPopup.h>
#include <CZ/Marco/Roles/MToplevel.h>
#include <CZ/Marco/MApp.h>

#include <CZ/Ream/RSurface.h>
#include <CZ/Ream/WL/RWLSwapchain.h>
#include <CZ/Core/Utils/CZRegionUtils.h>

using namespace CZ;

MSubsurface::MSubsurface(MSurface *parent) noexcept :
    MSurface(MSurface::Role::SubSurface)
{
    assert("wl_subcompositor not supported by the compositor" && MApp::Get()->wl.subCompositor);
    m_imp = std::make_unique<Imp>(*this);
    setParent(parent);
}

MSubsurface::~MSubsurface()
{
    setParent(nullptr);
}

MSurface *MSubsurface::parent() const noexcept
{
    return imp()->parent;
}

bool MSubsurface::setParent(MSurface *surface) noexcept
{
    if (imp()->parent == surface)
        return true;

    // Check if it is a descendant
    if (surface && surface->role() == MSurface::Role::SubSurface)
    {
        MSubsurface *check { (MSubsurface*)surface };

        while (check)
        {
            if (check == this || check->parent() == this)
                return false;

            if (check->parent() && check->parent()->role() == MSurface::Role::SubSurface)
                check = (MSubsurface*)check->parent();
            else
                break;
        }
    }

    if (surface)
    {
        if (imp()->parent)
        {
            imp()->parent->imp()->subSurfaces.erase(imp()->parentLink);
            imp()->parent.reset();

            if (imp()->wlSubSurface)
            {
                wl_subsurface_destroy(imp()->wlSubSurface);
                imp()->wlSubSurface = nullptr;
            }
        }

        imp()->parent.reset(surface);
        imp()->parent->imp()->subSurfaces.push_back(this);
        imp()->parentLink = std::prev(imp()->parent->imp()->subSurfaces.end());
        imp()->wlSubSurface = wl_subcompositor_get_subsurface(MApp::Get()->wl.subCompositor, wlSurface(), parent()->wlSurface());
        update();
        surface->update(true);
    }
    else
    {
        imp()->parent->imp()->subSurfaces.erase(imp()->parentLink);
        imp()->parent.reset();

        wl_subsurface_destroy(imp()->wlSubSurface);
        imp()->wlSubSurface = nullptr;
        MSurface::imp()->setMapped(false);
    }

    return true;
}

const std::list<MSubsurface*>::iterator &MSubsurface::parentLink() const noexcept
{
    return imp()->parentLink;
}

bool MSubsurface::placeAbove(MSubsurface *subSurface) noexcept
{
    if (!parent() || subSurface == this)
        return false;

    if (subSurface)
    {
        if (subSurface->parent() != parent())
            return false;

        if (std::next(subSurface->imp()->parentLink) == imp()->parentLink)
            return true;

        parent()->imp()->subSurfaces.erase(imp()->parentLink);

        if (subSurface == parent()->subSurfaces().back())
        {
            parent()->imp()->subSurfaces.push_back(this);
            imp()->parentLink = std::prev(parent()->imp()->subSurfaces.end());
        }
        else
            imp()->parentLink = parent()->imp()->subSurfaces.insert(std::next(subSurface->imp()->parentLink), this);

        wl_subsurface_place_above(imp()->wlSubSurface, subSurface->wlSurface());
    }
    else
    {
        if (parent()->subSurfaces().front() == this)
            return true;

        parent()->imp()->subSurfaces.erase(imp()->parentLink);
        parent()->imp()->subSurfaces.push_front(this);
        imp()->parentLink = parent()->imp()->subSurfaces.begin();
        wl_subsurface_place_above(imp()->wlSubSurface, parent()->wlSurface());
    }

    update();
    parent()->update(true);
    return true;
}

bool MSubsurface::placeBelow(MSubsurface *subSurface) noexcept
{
    if (!subSurface || !parent() || subSurface->parent() != parent())
        return false;

    if (std::prev(subSurface->imp()->parentLink) == imp()->parentLink)
        return true;

    parent()->imp()->subSurfaces.erase(imp()->parentLink);
    imp()->parentLink = parent()->imp()->subSurfaces.insert(subSurface->imp()->parentLink, this);
    wl_subsurface_place_below(imp()->wlSubSurface, subSurface->wlSurface());
    update();
    parent()->update(true);
    return true;
}

const SkIPoint &MSubsurface::pos() const noexcept
{
    return imp()->pos;
}

bool MSubsurface::isChildOfRole(Role role) const noexcept
{
    MSurface *p { parent() };

    while (p)
    {
        if (p->role() == role)
            return true;

        switch (p->role()) {
        case Role::LayerSurface:
            return false;
            break;
        case Role::Popup:
            p = static_cast<MPopup*>(p)->parent();
            break;
        case Role::Toplevel:
            p = static_cast<MToplevel*>(p)->parentToplevel();
            break;
        default:
            return false;
        }
    }

    return false;
}

void MSubsurface::setPos(const SkIPoint &pos) noexcept
{
    if (pos == imp()->pos)
        return;

    imp()->pos = pos;

    if (parent() && mapped())
    {
        wl_subsurface_set_position(imp()->wlSubSurface, pos.x(), pos.y());
        wl_surface_commit(wlSurface());

        // Protocol: The position is always applied when the parent commits
        parent()->update(true);
        return;
    }

    // From onUpdate() the parent is requested to commit later
    update(true);
    imp()->posChanged = true;
}

MSubsurface::Imp *MSubsurface::imp() const noexcept
{
    return m_imp.get();
}

void MSubsurface::onUpdate() noexcept
{
    MSurface::onUpdate();

    if (!parent())
        return;

    CZWeak<MSubsurface> ref { this };

    if (MSurface::imp()->flags.has(MSurface::Imp::UserMapped))
    {
        if (!mapped())
        {
            parent()->update(true);
            MSurface::imp()->setMapped(true);
        }
    }
    else
    {
        if (mapped())
        {
            parent()->update(true);
            wl_surface_attach(wlSurface(), NULL, 0, 0);
            wl_surface_commit(wlSurface());
            MSurface::imp()->setMapped(false);
        }
    }

    if (!ref || !mapped()) return;

    // A subsurface is always composited together with its parent at the parent's scale. Inherit it
    // directly instead of waiting for preferred_buffer_scale, so the first frame is already at the
    // correct scale. Otherwise the surface starts at scale 1 and later jumps to the parent's scale,
    // which both looks wrong and, on the transition frame, can commit a buffer momentarily smaller
    // than source * buffer_scale (wp_viewport out_of_buffer -> the compositor kills the client).
    if (MSurface::imp()->scale != parent()->scale())
    {
        MSurface::imp()->scale = parent()->scale();
        MSurface::imp()->tmpFlags.add(MSurface::Imp::ScaleChanged);
    }

    if (imp()->posChanged)
    {
        imp()->posChanged = false;
        wl_subsurface_set_position(imp()->wlSubSurface, pos().x(), pos().y());
        update(true);
        parent()->update(true);
    }

    if (MSurface::imp()->tmpFlags.has(MSurface::Imp::ScaleChanged))
        update(true);

    render();
}

void MSubsurface::render() noexcept
{
    auto app { MApp::Get() };
    scene().root()->layout().calculate();

    if (!ShouldUpdate(*this)) return;

    parent()->update(true);

    const SkISize newSize { CalculatedSize(*this) };
    const bool sizeChanged { MSurface::imp()->resizeBuffer(newSize) };
    const bool fullDamage { sizeChanged };
    const bool repaint { !MSurface::imp()->flags.has(MSurface::Imp::HasBufferAttached) || sizeChanged || target()->isDirty() || target()->bakedNodesScale() != scale() || changes().testAnyOf(CHDecorationMargins)};

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

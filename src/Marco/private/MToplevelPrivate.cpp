#include <Marco/private/MToplevelPrivate.h>
#include <Marco/input/MPointer.h>
#include <Marco/MApplication.h>

#include <AK/events/AKWindowStateEvent.h>

using namespace AK;

MToplevel::Imp::Imp(MToplevel &obj) noexcept : obj(obj), shadow(&obj)
{
    xdgSurfaceListener.configure = xdg_surface_configure;
    xdgToplevelListener.configure = xdg_toplevel_configure;
    xdgToplevelListener.configure_bounds = xdg_toplevel_configure_bounds;
    xdgToplevelListener.close = xdg_toplevel_close;
    xdgToplevelListener.wm_capabilities = xdg_toplevel_wm_capabilities;
}

void MToplevel::Imp::xdg_surface_configure(void *data, xdg_surface */*xdgSurface*/, UInt32 serial)
{
    auto &role { *static_cast<MToplevel*>(data) };
    role.imp()->flags.add(PendingConfigureAck);
    role.imp()->configureSerial = serial;

    bool notifyStates { role.imp()->flags.check(PendingFirstConfigure) };
    bool notifySuggestedSize { notifyStates };
    bool activatedChanged { false };
    role.imp()->flags.remove(PendingFirstConfigure);

    if (notifyStates)
    {
        xdg_toplevel_set_title(role.imp()->xdgToplevel, role.title().c_str());
    }

    if (role.imp()->currentStates.get() != role.imp()->pendingStates.get())
    {
        activatedChanged = role.imp()->currentStates.check(AKActivated) != role.imp()->pendingStates.check(AKActivated);
        role.imp()->currentStates = role.imp()->pendingStates;
        notifyStates = true;
    }

    if (role.imp()->pendingSuggestedSize != role.imp()->currentSuggestedSize)
    {
        role.imp()->currentSuggestedSize = role.imp()->pendingSuggestedSize;
        notifySuggestedSize = true;
    }

    AKWeak<MToplevel> ref { &role };

    if (notifySuggestedSize)
        role.suggestedSizeChanged();

    if (ref && notifyStates)
    {
        if (activatedChanged)
            akApp()->postEvent(AKWindowStateEvent(role.ak.scene.windowState().get() ^ role.imp()->currentStates.get()), role.ak.scene);
    }

    role.imp()->flags.add(ForceUpdate);
    role.update();
}

void MToplevel::Imp::xdg_toplevel_configure(void *data, xdg_toplevel *xdgToplevel, Int32 width, Int32 height, wl_array *states)
{
    auto &role { *static_cast<MToplevel*>(data) };
    role.imp()->pendingStates.set(0);
    const UInt32 *stateVals = (UInt32*)states->data;
    for (UInt32 i = 0; i < states->size/sizeof(*stateVals); i++)
        role.imp()->pendingStates.add(1 << stateVals[i]);

    // Just in case the compositor is insane
    role.imp()->pendingSuggestedSize.fWidth = width < 0 ? 0 : width;
    role.imp()->pendingSuggestedSize.fHeight = height < 0 ? 0 : height;
}

void MToplevel::Imp::xdg_toplevel_close(void *data, xdg_toplevel *xdgToplevel)
{

}

void MToplevel::Imp::xdg_toplevel_configure_bounds(void *data, xdg_toplevel *xdgToplevel, Int32 width, Int32 height)
{

}

void MToplevel::Imp::xdg_toplevel_wm_capabilities(void *data, xdg_toplevel *xdgToplevel, wl_array *capabilities)
{

}

void MToplevel::Imp::handleRootPointerButtonEvent(const AKPointerButtonEvent &event) noexcept
{
    if (!obj.visible() || event.button() != BTN_LEFT || event.state() != AKPointerButtonEvent::Pressed)
        return;

    const SkPoint &pointerPos { pointer().eventHistory().move.pos() };
    const Int32 resizeMargins { MTheme::CSDResizeOutset };
    const Int32 moveTopMargin { MTheme::CSDMoveOutset };
    UInt32 resizeEdges { 0 };

    if (obj.globalRect().x() - resizeMargins <= pointerPos.x() && obj.globalRect().x() + resizeMargins >= pointerPos.x())
        resizeEdges |= XDG_TOPLEVEL_RESIZE_EDGE_LEFT;
    else if (obj.globalRect().right() - resizeMargins <= pointerPos.x() && obj.globalRect().right() + resizeMargins >= pointerPos.x())
        resizeEdges |= XDG_TOPLEVEL_RESIZE_EDGE_RIGHT;

    if (obj.globalRect().y() - resizeMargins <= pointerPos.y() && obj.globalRect().y() + resizeMargins >= pointerPos.y())
        resizeEdges |= XDG_TOPLEVEL_RESIZE_EDGE_TOP;
    else if (obj.globalRect().bottom() - resizeMargins <= pointerPos.y() && obj.globalRect().bottom() + resizeMargins >= pointerPos.y())
        resizeEdges |= XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;

    if (resizeEdges)
        xdg_toplevel_resize(xdgToplevel, app()->wayland().seat, event.serial(), resizeEdges);
    else if (obj.globalRect().x() <= pointerPos.x() &&
             obj.globalRect().right() >= pointerPos.x() &&
             obj.globalRect().y() <= pointerPos.y() &&
             obj.globalRect().y() + moveTopMargin >= pointerPos.y())
    {
        xdg_toplevel_move(xdgToplevel, app()->wayland().seat, event.serial());
    }
}

void MToplevel::Imp::handleRootPointerMoveEvent(const AKPointerMoveEvent &event) noexcept
{
    if (!obj.visible() || obj.fullscreen())
    {
        obj.ak.root.setCursor(AKCursor::Default);
        return;
    }

    const SkPoint &pointerPos { event.pos() };
    const Int32 resizeMargins { MTheme::CSDResizeOutset };
    UInt32 resizeEdges { 0 };

    if (obj.globalRect().x() - resizeMargins <= pointerPos.x() && obj.globalRect().x() + resizeMargins >= pointerPos.x())
        resizeEdges |= XDG_TOPLEVEL_RESIZE_EDGE_LEFT;
    else if (obj.globalRect().right() - resizeMargins <= pointerPos.x() && obj.globalRect().right() + resizeMargins >= pointerPos.x())
        resizeEdges |= XDG_TOPLEVEL_RESIZE_EDGE_RIGHT;

    if (obj.globalRect().y() - resizeMargins <= pointerPos.y() && obj.globalRect().y() + resizeMargins >= pointerPos.y())
        resizeEdges |= XDG_TOPLEVEL_RESIZE_EDGE_TOP;
    else if (obj.globalRect().bottom() - resizeMargins <= pointerPos.y() && obj.globalRect().bottom() + resizeMargins >= pointerPos.y())
        resizeEdges |= XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;

    if (resizeEdges == XDG_TOPLEVEL_RESIZE_EDGE_LEFT || resizeEdges == XDG_TOPLEVEL_RESIZE_EDGE_RIGHT)
        obj.ak.root.setCursor(AKCursor::EWResize);
    else if (resizeEdges == XDG_TOPLEVEL_RESIZE_EDGE_TOP || resizeEdges == XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM)
        obj.ak.root.setCursor(AKCursor::NSResize);
    else if (resizeEdges == (XDG_TOPLEVEL_RESIZE_EDGE_TOP | XDG_TOPLEVEL_RESIZE_EDGE_LEFT) ||
             resizeEdges == (XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM | XDG_TOPLEVEL_RESIZE_EDGE_RIGHT))
        obj.ak.root.setCursor(AKCursor::NWSEResize);
    else if (resizeEdges == (XDG_TOPLEVEL_RESIZE_EDGE_TOP | XDG_TOPLEVEL_RESIZE_EDGE_RIGHT) ||
             resizeEdges == (XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM | XDG_TOPLEVEL_RESIZE_EDGE_LEFT))
        obj.ak.root.setCursor(AKCursor::NESWResize);
    else
        obj.ak.root.setCursor(AKCursor::Default);
}

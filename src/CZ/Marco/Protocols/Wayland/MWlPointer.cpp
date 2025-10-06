#include <CZ/Marco/Protocols/Wayland/MWlPointer.h>
#include <CZ/Marco/Protocols/CursorShape/cursor-shape-v1-client.h>
#include <CZ/Marco/Roles/MSurface.h>
#include <CZ/Marco/MApp.h>
#include <CZ/AK/AKApp.h>
#include <CZ/Core/CZCore.h>

using namespace CZ;

void MWlPointer::enter(void *data, wl_pointer *pointer, UInt32 serial, wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
{
    CZ_UNUSED(data)
    CZ_UNUSED(pointer)

    MSurface *surf { static_cast<MSurface*>(wl_surface_get_user_data(surface)) };

    CZPointerEnterEvent e {};
    e.pos.fX = wl_fixed_to_double(x);
    e.pos.fY = wl_fixed_to_double(y);
    e.serial = serial;

    CZCore::Get()->sendEvent(e, surf->scene());
}

void MWlPointer::leave(void *data, wl_pointer *pointer, UInt32 serial, wl_surface *surface)
{
    CZ_UNUSED(data)
    CZ_UNUSED(pointer)

    if (!surface) return;

    CZPointerLeaveEvent e {};
    e.serial = serial;

    CZCore::Get()->sendEvent(e, *AKApp::Get());
}

void MWlPointer::motion(void *data, wl_pointer *pointer, UInt32 time, wl_fixed_t x, wl_fixed_t y)
{
    CZ_UNUSED(data)
    CZ_UNUSED(pointer)
    CZ_UNUSED(time)

    CZPointerMoveEvent e {};
    e.pos.fX = wl_fixed_to_double(x);
    e.pos.fY = wl_fixed_to_double(y);

    CZCore::Get()->sendEvent(e, *AKApp::Get());
}

void MWlPointer::button(void *data, wl_pointer *pointer, UInt32 serial, UInt32 time, UInt32 button, UInt32 state)
{
    CZ_UNUSED(data)
    CZ_UNUSED(pointer)
    CZ_UNUSED(time)

    CZPointerButtonEvent e {};
    e.serial = serial;
    e.button = button;
    e.pressed = state == WL_POINTER_BUTTON_STATE_PRESSED;

    CZCore::Get()->sendEvent(e, *AKApp::Get());

    /*
    auto &p { app()->pointer() };
    if (!p.focus()) return;

    if (p.focus()->role() != MSurface::Role::Popup || (p.focus()->role() == MSurface::Role::SubSurface && !static_cast<MSubsurface*>(p.focus())->isChildOfRole(MSurface::Role::Popup)))
    {
        for (auto *surface : app()->surfaces())
            if (surface->role() == MSurface::Role::Popup)
                surface->setMapped(false);
    }

    if (state == WL_POINTER_BUTTON_STATE_PRESSED)
        p.m_pressedButtons.insert(button);
    else
        p.m_pressedButtons.erase(button);

    p.m_eventHistory.button.setMs(time);
    p.m_eventHistory.button.setUs(AKTime::us());
    p.m_eventHistory.button.setSerial(serial);
    p.m_eventHistory.button.setButton((CZ::CZPointerButtonEvent::Button)button);
    p.m_eventHistory.button.setState((CZ::CZPointerButtonEvent::State)state);
    p.m_eventHistory.button.ignore();
    CZCore::Get()->sendEvent(p.m_eventHistory.button, p.focus()->scene());
*/
}

void MWlPointer::axis(void *data, wl_pointer *pointer, UInt32 time, UInt32 axis, wl_fixed_t value)
{
    CZ_UNUSED(time)
    CZ_UNUSED(data)
    CZ_UNUSED(pointer)

    auto app { MApp::Get() };

    if (wl_pointer_get_version(pointer) >= 5)
    {
        if (!app->m_pendingScrollEvent)
            app->m_pendingScrollEvent = CZPointerScrollEvent();

        auto &e { app->m_pendingScrollEvent };

        if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
        {
            e->hasX = true;
            e->axes.fX = wl_fixed_to_double(value);
        }
        else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
        {
            e->hasY = true;
            e->axes.fY = wl_fixed_to_double(value);
        }
    }

    // Version < 5: No frame and discrete events
    else
    {
        CZPointerScrollEvent e {};
        e.source = CZPointerScrollEvent::Continuous;

        if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
        {
            e.hasX = true;
            e.axes.fX = wl_fixed_to_double(value);
        }
        else
        {
            e.hasY = true;
            e.axes.fY = wl_fixed_to_double(value);
        }

        CZCore::Get()->sendEvent(e, *AKApp::Get());
    }
}

void MWlPointer::frame(void *data, wl_pointer *pointer)
{
    CZ_UNUSED(data)
    CZ_UNUSED(pointer)

    auto app { MApp::Get() };

    if (!app->m_pendingScrollEvent)
        return;

    CZCore::Get()->sendEvent(*app->m_pendingScrollEvent, *AKApp::Get());
    app->m_pendingScrollEvent = std::nullopt;
}

void MWlPointer::axis_source(void *data, wl_pointer *pointer, UInt32 axis_source)
{
    CZ_UNUSED(data)

    auto app { MApp::Get() };

    if (!app->m_pendingScrollEvent)
        app->m_pendingScrollEvent = CZPointerScrollEvent();

    if (wl_pointer_get_version(pointer) < 8 && axis_source == WL_POINTER_AXIS_SOURCE_WHEEL)
        app->m_pendingScrollEvent->source = CZPointerScrollEvent::WheelLegacy;
    else
        app->m_pendingScrollEvent->source = (CZPointerScrollEvent::Source)axis_source;
}

void MWlPointer::axis_stop(void *data, wl_pointer *pointer, UInt32 time, UInt32 axis)
{
    CZ_UNUSED(data)
    CZ_UNUSED(pointer)
    CZ_UNUSED(time)

    auto app { MApp::Get() };

    if (!app->m_pendingScrollEvent)
        app->m_pendingScrollEvent = CZPointerScrollEvent();

    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
    {
        app->m_pendingScrollEvent->hasX = true;
        app->m_pendingScrollEvent->axes.fX = 0;
    }
    else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
    {
        app->m_pendingScrollEvent->hasY = true;
        app->m_pendingScrollEvent->axes.fY = 0;
    }
}

void MWlPointer::axis_discrete(void *data, wl_pointer *pointer, UInt32 axis, Int32 discrete)
{
    CZ_UNUSED(data)
    CZ_UNUSED(pointer)

    auto app { MApp::Get() };

    if (!app->m_pendingScrollEvent)
        app->m_pendingScrollEvent = CZPointerScrollEvent();

    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
    {
        app->m_pendingScrollEvent->hasX = true;
        app->m_pendingScrollEvent->axesDiscrete.fX = discrete;
    }
    else
    {
        app->m_pendingScrollEvent->hasY = true;
        app->m_pendingScrollEvent->axesDiscrete.fY = discrete;
    }
}

void MWlPointer::axis_value120(void *data, wl_pointer *pointer, UInt32 axis, Int32 value120)
{
    CZ_UNUSED(data)
    CZ_UNUSED(pointer)

    auto app { MApp::Get() };

    if (!app->m_pendingScrollEvent)
        app->m_pendingScrollEvent = CZPointerScrollEvent();

    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
    {
        app->m_pendingScrollEvent->hasX = true;
        app->m_pendingScrollEvent->axesDiscrete.fX = value120;
    }
    else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
    {
        app->m_pendingScrollEvent->hasY = true;
        app->m_pendingScrollEvent->axesDiscrete.fY = value120;
    }
}

void MWlPointer::axis_relative_direction(void *data, wl_pointer *pointer, UInt32 axis, UInt32 direction)
{
    CZ_UNUSED(data)
    CZ_UNUSED(pointer)

    auto app { MApp::Get() };

    if (!app->m_pendingScrollEvent)
        app->m_pendingScrollEvent = CZPointerScrollEvent();

    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
        app->m_pendingScrollEvent->relativeDirectionX = (CZPointerScrollEvent::RelativeDirection)direction;
    else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
        app->m_pendingScrollEvent->relativeDirectionY = (CZPointerScrollEvent::RelativeDirection)direction;
}

static std::optional<CZCursorShape> LastSentCursor;

void MWlPointer::SetCursorFromFocus() noexcept
{
    auto app { MApp::Get() };
    auto kay { AKApp::Get() };

    if (!kay->pointer().focus() || !kay->pointer().focus()->pointerFocus())
    {
        if (!LastSentCursor || LastSentCursor.value() != CZCursorShape::Default)
        {
            wp_cursor_shape_device_v1_set_shape(
                app->wl.cursorShapePointer,
                kay->pointer().history().enter.serial,
                (UInt32)CZCursorShape::Default);
            LastSentCursor = CZCursorShape::Default;
        }
        return;
    }

    if (kay->pointer().focus()->pointerFocus()->cursor())
    {
        if (!LastSentCursor || LastSentCursor != kay->pointer().focus()->pointerFocus()->cursor())
        {
            wp_cursor_shape_device_v1_set_shape(
                app->wl.cursorShapePointer,
                kay->pointer().history().enter.serial,
                (UInt32)kay->pointer().focus()->pointerFocus()->cursor().value());
            LastSentCursor = kay->pointer().focus()->pointerFocus()->cursor();
        }
    }
    else
    {
        if (LastSentCursor)
        {
            wl_pointer_set_cursor(
                app->wl.pointer,
                kay->pointer().history().enter.serial,
                NULL, 0, 0);
            LastSentCursor = std::nullopt;
        }
    }
}

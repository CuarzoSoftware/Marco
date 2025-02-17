#ifndef MPOINTER_H
#define MPOINTER_H

#include <Marco/Marco.h>
#include <AK/AKCursor.h>
#include <AK/AKObject.h>
#include <AK/AKWeak.h>
#include <AK/events/AKPointerEnterEvent.h>
#include <AK/events/AKPointerMoveEvent.h>
#include <AK/events/AKPointerLeaveEvent.h>
#include <AK/events/AKPointerButtonEvent.h>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <unordered_set>
#include <unordered_map>

class Marco::MPointer : public AK::AKObject
{
public:
    MPointer() = default;

    struct EventHistory
    {
        AK::AKPointerEnterEvent enter;
        AK::AKPointerMoveEvent move;
        AK::AKPointerLeaveEvent leave;
        AK::AKPointerButtonEvent button;
    };

    AK::AKCursor cursor() const noexcept { return m_cursor; };
    void setCursor(AK::AKCursor cursor) noexcept;
    MSurface *focus() const noexcept { return m_focus; };
    const EventHistory &eventHistory() const noexcept { return m_eventHistory; };
    const std::unordered_set<UInt32> pressedButtons() const noexcept { return m_pressedButtons; };
    bool isButtonPressed(UInt32 button) const noexcept
    {
        return m_pressedButtons.contains(button);
    }

private:
    friend class MApplication;
    AK::AKCursor findNonDefaultCursor(AK::AKNode *node) const noexcept;
    EventHistory m_eventHistory;
    std::unordered_set<UInt32> m_pressedButtons;
    AK::AKWeak<MSurface> m_focus;
    wl_surface *m_cursorSurface { nullptr };
    wl_cursor_theme *m_cursorTheme { nullptr };
    std::unordered_map<AK::AKCursor, wl_cursor*> m_cursors;
    AK::AKCursor m_cursor { AK::AKCursor::Default };
    bool m_forceCursorUpdate { true };
};

#endif // MPOINTER_H

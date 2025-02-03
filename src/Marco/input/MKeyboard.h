#ifndef MKEYBOARD_H
#define MKEYBOARD_H

#include <Marco/Marco.h>
#include <AK/AKObject.h>
#include <AK/AKWeak.h>
#include <AK/events/AKKeyboardKeyEvent.h>
#include <AK/events/AKKeyboardEnterEvent.h>
#include <AK/events/AKKeyboardLeaveEvent.h>
#include <AK/events/AKKeyboardModifiersEvent.h>

class Marco::MKeyboard : public AK::AKObject
{
public:
    MKeyboard() = default;

    struct EventHistory
    {
        AK::AKKeyboardKeyEvent key;
        AK::AKKeyboardEnterEvent enter;
        AK::AKKeyboardLeaveEvent leave;
        AK::AKKeyboardModifiersEvent modifiers;
    };

    MSurface *focus() const noexcept { return m_focus; };
    const EventHistory &eventHistory() const noexcept { return m_eventHistory; };

private:
    friend class MApplication;
    EventHistory m_eventHistory;
    AK::AKWeak<MSurface> m_focus;
};

#endif // MKEYBOARD_H

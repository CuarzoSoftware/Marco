#include <CZ/Events/CZKeyboardEnterEvent.h>
#include <CZ/Events/CZKeyboardKeyEvent.h>
#include <CZ/Marco/Roles/MSurface.h>
#include <CZ/Marco/Protocols/Wayland/MWlKeyboard.h>
#include <CZ/Marco/MLog.h>
#include <CZ/Core/CZKeymap.h>
#include <CZ/Core/CZCore.h>
#include <CZ/AK/AKApp.h>

using namespace CZ;

void MWlKeyboard::keymap(void *data, wl_keyboard *keyboard, UInt32 format, Int32 fd, UInt32 size)
{
    CZ_UNUSED(data)
    CZ_UNUSED(keyboard)

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    {
        MLog(CZError, CZLN, "Unsupported keymap format. Falling back to the default keymap");
        close(fd);
        return;
    }

    auto keymap { CZKeymap::MakeClient(fd, size) };
    close(fd);

    if (!keymap)
    {
        MLog(CZError, CZLN, "Failed to import keymap");
        return;
    }

    MLog(CZTrace, CZLN, "Got a valid keymap from the compositor");
    CZCore::Get()->setKeymap(keymap);
}

void MWlKeyboard::enter(void *data, wl_keyboard *keyboard, UInt32 serial, wl_surface *surface, wl_array *keys)
{
    CZ_UNUSED(data)
    CZ_UNUSED(keyboard)

    MLog(CZTrace, "Enter");

    MSurface *surf { static_cast<MSurface*>(wl_surface_get_user_data(surface)) };
    auto kay { AKApp::Get() };
    CZSafeEventQueue queue;
    auto enter { std::make_shared<CZKeyboardEnterEvent>() };
    enter->serial = serial;
    queue.addEvent(enter, surf->scene());

    auto keymap { CZCore::Get()->keymap() };
    UInt32 *keyCodes { static_cast<UInt32*>(keys->data) };
    for (size_t i = 0; i < keys->size/sizeof(UInt32); i++)
    {
        auto key { std::make_shared<CZKeyboardKeyEvent>() };
        key->serial = serial;
        key->code = keyCodes[i];
        key->isPressed = true;
        keymap->feed(*key);
        queue.addEvent(key, *kay);
    }

    queue.dispatch();
}

void MWlKeyboard::leave(void *data, wl_keyboard *keyboard, UInt32 serial, wl_surface *surface)
{
    CZ_UNUSED(data)
    CZ_UNUSED(keyboard)
    CZ_UNUSED(surface)
    CZKeyboardLeaveEvent e {};
    e.serial = serial;
    CZCore::Get()->sendEvent(e, *AKApp::Get());
}

void MWlKeyboard::key(void *data, wl_keyboard *keyboard, UInt32 serial, UInt32 time, UInt32 key, UInt32 state)
{
    CZ_UNUSED(data)
    CZ_UNUSED(keyboard)
    CZ_UNUSED(time);

    auto keymap { CZCore::Get()->keymap() };
    CZKeyboardKeyEvent e {};
    e.serial = serial;
    e.code = key;
    e.isPressed = state == WL_KEYBOARD_KEY_STATE_PRESSED;
    keymap->feed(e);
    CZCore::Get()->sendEvent(e, *AKApp::Get());
}

void MWlKeyboard::modifiers(void *data, wl_keyboard *keyboard, UInt32 serial, UInt32 depressed, UInt32 latched, UInt32 locked, UInt32 group)
{
    CZ_UNUSED(data)
    CZ_UNUSED(keyboard)
    CZKeyboardModifiersEvent e {};
    e.serial = serial;
    e.modifiers.depressed = depressed;
    e.modifiers.latched = latched;
    e.modifiers.locked = locked;
    e.modifiers.group = group;
    CZCore::Get()->sendEvent(e, *AKApp::Get());
}

void MWlKeyboard::repeat_info(void *data, wl_keyboard *keyboard, Int32 rate, Int32 delay)
{
    CZ_UNUSED(data)
    CZ_UNUSED(keyboard)
    CZCore::Get()->keymap()->setRepeatInfo(delay, rate);
}

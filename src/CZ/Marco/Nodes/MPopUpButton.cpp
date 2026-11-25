#include <CZ/Marco/Nodes/MPopUpButton.h>
#include <CZ/Core/Events/CZPointerButtonEvent.h>

using namespace CZ;

MPopUpButton::MPopUpButton(const std::vector<std::string> &items, AKNode *parent) noexcept :
    AKPopUpButton(items, parent)
{
    // Highlighted while its menu is open
    menu.onMappedChanged.subscribe(this, [this]{ setMenuOpen(menu.mapped()); });
}

void MPopUpButton::openMenu(const CZPointerButtonEvent &event) noexcept
{
    AKPopUpButton::openMenu(event);

    if (items().empty())
        return;

    menu.clear();

    for (size_t i = 0; i < items().size(); i++)
        menu.addAction(items()[i], [this, i]{ select(i); }).setChecked(i == selectedIndex());

    menu.popup(*this, &event);

    // Open right away (the menu is shown a few frames later): the pressed fill stays
    setMenuOpen(true);
}

#include <CZ/Marco/Roles/MMenu.h>
#include <CZ/Marco/Private/MMenuPrivate.h>

using namespace CZ;

MMenu::MMenu() {}

MMenu::Imp *MMenu::imp() const noexcept
{
    return m_imp.get();
}

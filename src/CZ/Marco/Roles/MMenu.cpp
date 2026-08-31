#include <CZ/Marco/Roles/MMenu.h>
#include <CZ/Marco/Private/MMenuPrivate.h>
#include <CZ/Marco/Nodes/MShadowDecorations.h>

using namespace CZ;

MMenu::MMenu() noexcept : MPopup()
{
    setSlot(&m_vibrancy);
    m_vibrancy.layout().setWidthPercent(100);
    m_vibrancy.layout().setHeightPercent(100);
    setColor(CZAdaptiveColor(0));
    setDecorations(std::make_unique<MShadowDecorations>());
    setBorderRadius(CZBorderRadius::Make(8));
}

MMenu::~MMenu()
{

}

MMenu::Imp *MMenu::imp() const noexcept
{
    return m_imp.get();
}

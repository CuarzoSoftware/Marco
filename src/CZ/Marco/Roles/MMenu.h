#ifndef MMENU_H
#define MMENU_H

#include <CZ/Marco/Roles/MPopup.h>
#include <CZ/Marco/Nodes/MVibrancyView.h>

class CZ::MMenu : public MPopup
{
public:
    MMenu() noexcept;
    ~MMenu();

    class Imp;
    Imp *imp() const noexcept;

protected:
    MVibrancyView m_vibrancy { this };
private:
    std::unique_ptr<Imp> m_imp;
};

#endif // MMENU_H

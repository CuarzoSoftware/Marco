#ifndef MMENU_H
#define MMENU_H

#include <CZ/Marco/Roles/MPopup.h>

class CZ::MMenu : public MPopup
{
public:
    MMenu();

    class Imp;
    Imp *imp() const noexcept;
private:
    std::unique_ptr<Imp> m_imp;
};

#endif // MMENU_H

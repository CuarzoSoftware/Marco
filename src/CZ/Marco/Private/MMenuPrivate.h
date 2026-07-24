#ifndef MMENUPRIVATE_H
#define MMENUPRIVATE_H

#include <CZ/Marco/Roles/MMenu.h>

class CZ::MMenu::Imp
{
public:
    Imp(MMenu &obj) noexcept;
    MMenu &obj;
};


#endif // MMENUPRIVATE_H

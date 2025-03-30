#ifndef MSUBSURFACEPRIVATE_H
#define MSUBSURFACEPRIVATE_H

#include <Marco/roles/MSubSurface.h>

class AK::MSubSurface::Imp
{
public:
    Imp(MSubSurface &obj) noexcept;
    MSubSurface &obj;
    SkIPoint pos { 0, 0 };
    AKWeak<MSurface> parent;
    wl_subsurface *wlSubSurface { nullptr };
    std::list<MSubSurface*>::iterator parentLink;
    bool posChanged { false };
};

#endif // MSUBSURFACEPRIVATE_H

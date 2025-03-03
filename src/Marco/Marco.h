#ifndef MARCO_H
#define MARCO_H

#include <AK/AK.h>

using namespace AK;

namespace Marco
{
    class MApplication;
    class MScreen;
    class MSurface;
    class MToplevel;
    class MTheme;
    class MCSDShadow;

    /* Input */
    class MPointer;
    class MKeyboard;

    /* Utils */
    class MImageLoader;

    template <class T> class MProxy;

    inline MApplication *app() noexcept { return (MApplication*)akApp(); };
    MPointer &pointer() noexcept;
    MKeyboard &keyboard() noexcept;
};

#endif // MARCO_H

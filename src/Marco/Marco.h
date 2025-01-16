#ifndef MARCO_H
#define MARCO_H

#include <cstdint>

namespace Marco
{
    class MApplication;
    class MScreen;
    class MSurface;
    class MToplevel;
    class MTheme;
    class MWindowShadow;
    class MCSDShadow;

    /* Utils */
    class MImageLoader;

    template <class T> class MProxy;

    MApplication *app() noexcept;

    /// @brief 64 bits unsigned integer
    typedef uint64_t        UInt64;

    /// @brief 64 bits signed integer
    typedef int64_t         Int64;

    /// @brief 32 bits unsigned integer
    typedef uint32_t        UInt32;

    /// @brief 32 bits signed integer
    typedef int32_t         Int32;

    /// @brief 16 bits unsigned integer
    typedef uint16_t        UInt16;

    /// @brief 16 bits signed integer
    typedef int16_t         Int16;

    /// @brief 8 bits unsigned integer
    typedef uint8_t         UInt8;

    /// @brief 8 bits signed integer
    typedef int8_t          Int8;

    /// @brief 8 bits unsigned integer
    typedef unsigned char   UChar8;

    /// @brief 8 bits signed integer
    typedef char            Char8;

    /// @brief 64 bits float
    typedef double          Float64;

    /// @brief 32 bits float
    typedef float           Float32;

    /// @brief Unsigned integer capable of holding a pointer
    typedef uintptr_t       UIntPtr;
};

#endif // MARCO_H

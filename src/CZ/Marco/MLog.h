#ifndef CZ_MLOG_H
#define CZ_MLOG_H

#include <CZ/Core/CZLogger.h>

#define MLog MarcoLogGet()

const CZ::CZLogger &MarcoLogGet() noexcept;

#endif // CZ_MLOG_H

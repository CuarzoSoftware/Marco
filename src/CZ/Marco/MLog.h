#ifndef CZ_MLOG_H
#define CZ_MLOG_H

#include <CZ/Core/CZLogger.h>

/**
 * @brief Marco's logger, e.g. `MLog(CZInfo, "Hello {}", 1)`.
 *
 * Its level is set with the `CZ_MARCO_LOG_LEVEL` environment variable.
 */
#define MLog MarcoLogGet()

/** @brief Returns Marco's logger (see MLog). */
const CZ::CZLogger &MarcoLogGet() noexcept;

#endif // CZ_MLOG_H

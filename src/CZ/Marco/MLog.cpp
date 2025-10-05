#include <CZ/Marco/MLog.h>

using namespace CZ;

const CZ::CZLogger &MarcoLogGet() noexcept
{
    static CZLogger logger { "Marco", "CZ_MARCO_LOG_LEVEL" };
    return logger;
}

#include "common.h"

#include <stdbool.h>

const KmInt KM_TRUE = -1;
const KmInt KM_FALSE = 0;

int ipow(KmInt base, KmInt exp)
{
    if (exp == 0) return 1;
    int result = 1;
    while (true)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp) break;
        base *= base;
    }
    return result;
}
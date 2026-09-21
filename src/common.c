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

const char* type2str(DataType datatype) {
    switch (datatype) {
    case TYPE_INT: return "INTEGER";
    case TYPE_FLOAT: return "FLOAT";
    case TYPE_STRING: return "STRING";
    case TYPE_INT | TYPE_FLOAT: return "INTEGER or FLOAT";
    case TYPE_REF | TYPE_INT: return "REF INTEGER";
    case TYPE_REF | TYPE_FLOAT: return "REF FLOAT";
    case TYPE_REF | TYPE_STRING: return "REF STRING";
    default: return "UNKNOWN";
    }
}
#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <inttypes.h>

#if INTPTR_MAX == INT64_MAX

typedef int64_t KmInt;
typedef double KmFloat;

#define PRIkmINT PRIi64

#elif INTPTR_MAX == INT32_MAX

typedef int32_t KmInt;
typedef float KmFloat;

#define PRIkmINT PRIu32;

#else
#error "Unknown architecture"
#endif

typedef enum DataType {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING
} DataType;

#endif
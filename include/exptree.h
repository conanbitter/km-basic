#ifndef EXPTREE_H
#define EXPTREE_H

#include <stdint.h>
#include "common.h"

#define EXPTREE_SIZE (256)

#pragma pack(push, 1)

typedef struct ExptBinop {
    uint8_t node_type : 3;
    uint8_t opcode : 5;

    uint8_t left;
    uint8_t right;
} ExptBinop;

typedef struct ExptUnop {
    uint8_t node_type : 3;
    uint8_t : 2;
    uint8_t opcode : 3;

    uint8_t arg;
} ExptUnop;

typedef struct ExptConst {
    uint8_t node_type : 3;
    uint8_t : 3;
    uint8_t valtype : 2;

    KmValue value;
} ExptConst;

typedef struct ExptLoad {
    uint8_t node_type : 3;
    uint8_t : 1;
    uint8_t ref : 1;
    uint8_t local : 1;
    uint8_t valtype : 2;

    KmValue value;
} ExptLoad;

#pragma pack(pop)

uint8_t* expt_init(uint8_t* start);
void* expt_alloc(size_t size);

#define EXPT_NODE(typename) (typename*)expt_alloc(sizeof(typename))

#endif
#ifndef EXPTREE_H
#define EXPTREE_H

#include <stdint.h>
#include "common.h"

#define EXPTREE_SIZE (256)

typedef enum ExptNodeType {
    EXPT_TYPE_ExptBinop,
    EXPT_TYPE_ExptUnop,
    EXPT_TYPE_ExptConst,
    EXPT_TYPE_ExptLoad,
} ExptNodeType;

typedef enum ExptBinopCode {
    EXPT_BINOP_IPOWER,
    EXPT_BINOP_FPOWER,
    EXPT_BINOP_IMUL,
    EXPT_BINOP_FMUL,
    EXPT_BINOP_IDIV,
    EXPT_BINOP_FDIV,
    EXPT_BINOP_MOD,
    EXPT_BINOP_IADD,
    EXPT_BINOP_FADD,
    EXPT_BINOP_AADD,
    EXPT_BINOP_ISUB,
    EXPT_BINOP_FSUB,
    EXPT_BINOP_CONCAT,
    EXPT_BINOP_LSHIFT,
    EXPT_BINOP_RSHIFT,
    EXPT_BINOP_IEQ,
    EXPT_BINOP_FEQ,
    EXPT_BINOP_SEQ,
    EXPT_BINOP_INEQ,
    EXPT_BINOP_FNEQ,
    EXPT_BINOP_SNEQ,
    EXPT_BINOP_IGT,
    EXPT_BINOP_FGT,
    EXPT_BINOP_IGTEQ,
    EXPT_BINOP_FGTEQ,
    EXPT_BINOP_ILS,
    EXPT_BINOP_FLS,
    EXPT_BINOP_ILSEQ,
    EXPT_BINOP_FLSEQ,
    EXPT_BINOP_AND,
    EXPT_BINOP_OR,
    EXPT_BINOP_XOR,
    EXPT_BINOP_ANDSC, // short-circuit
    EXPT_BINOP_ORSC,  // short-circuit
} ExptBinopCode;

typedef enum ExptUnopCode {
    EXPT_UNOP_INEG,
    EXPT_UNOP_FNEG,
    EXPT_UNOP_NOT,
    EXPT_UNOP_ITOF,
    EXPT_UNOP_FTOI
} ExptUnopCode;

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
    uint8_t : 2;
    uint8_t valtype : 3;

    KmValue value;
} ExptConst;

typedef struct ExptLoad {
    uint8_t node_type : 3;
    uint8_t ref : 1;
    uint8_t local : 1;
    uint8_t datatype : 3;

    void* addr;
} ExptLoad;

typedef struct ExptNode {
    uint8_t node_type : 3;
} ExptNode;

#pragma pack(pop)

uint8_t* expt_init(uint8_t* start);
void* expt_alloc(size_t size, ExptNodeType node_type);
void exptr_debug_print(void* root);
uint8_t expt_addr(void* from, void* to);
void expt_codegen(void* root);

#define EXPT_NODE(typename) (typename*)expt_alloc(sizeof(typename), EXPT_TYPE_ ## typename)

#endif
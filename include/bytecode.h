#ifndef BYTECODE_H
#define BYTECODE_H

#include "common.h"
#include "memblock.h"

typedef enum BcOpcode {
    // size: 1 byte
    BC_OPCODE_IPOW,   // Binops
    BC_OPCODE_FPOW,
    BC_OPCODE_IMUL,
    BC_OPCODE_FMUL,
    BC_OPCODE_IDIV,
    BC_OPCODE_FDIV,
    BC_OPCODE_MOD,
    BC_OPCODE_IADD,
    BC_OPCODE_FADD,
    BC_OPCODE_AADD,
    BC_OPCODE_ISUB,
    BC_OPCODE_FSUB,
    BC_OPCODE_CONCAT,
    BC_OPCODE_SHL,
    BC_OPCODE_SHR,
    BC_OPCODE_IEQ,
    BC_OPCODE_FEQ,
    BC_OPCODE_SEQ,
    BC_OPCODE_INEQ,
    BC_OPCODE_FNEQ,
    BC_OPCODE_SNEQ,
    BC_OPCODE_IGT,
    BC_OPCODE_FGT,
    BC_OPCODE_IGTEQ,
    BC_OPCODE_FGTEQ,
    BC_OPCODE_ILS,
    BC_OPCODE_FLS,
    BC_OPCODE_ILSEQ,
    BC_OPCODE_FLSEQ,
    BC_OPCODE_AND,
    BC_OPCODE_OR,
    BC_OPCODE_XOR,
    BC_OPCODE_INEG,   // Unops
    BC_OPCODE_FNEG,
    BC_OPCODE_NOT,
    BC_OPCODE_ITOF,
    BC_OPCODE_FTOI,
    BC_OPCODE_ITOS,
    BC_OPCODE_STOI,
    BC_OPCODE_FTOS,
    BC_OPCODE_STOF,
    BC_OPCODE_ILOADP, // Other
    BC_OPCODE_FLOADP,
    BC_OPCODE_ALOADP,
    BC_OPCODE_ISTOREP,
    BC_OPCODE_FSTOREP,
    BC_OPCODE_ASTOREP,
    BC_OPCODE_STRCOPY,
    BC_OPCODE_DUP,
    BC_OPCODE_DROP,
    BC_OPCODE_RET,
    BC_OPCODE_RETRES,
    // size: 3 bytes
    BC_OPCODE_SYSCALL,
    // size: 5 bytes
    BC_OPCODE_IPUSH,
    BC_OPCODE_FPUSH,
    BC_OPCODE_APUSH,
    BC_OPCODE_ILOAD,
    BC_OPCODE_FLOAD,
    BC_OPCODE_ALOAD,
    BC_OPCODE_ILOADL,
    BC_OPCODE_FLOADL,
    BC_OPCODE_ALOADL,
    BC_OPCODE_ILOADR,
    BC_OPCODE_FLOADR,
    BC_OPCODE_ALOADR,
    BC_OPCODE_ILOADV,
    BC_OPCODE_FLOADV,
    BC_OPCODE_ALOADV,
    BC_OPCODE_ISTORE,
    BC_OPCODE_FSTORE,
    BC_OPCODE_ASTORE,
    BC_OPCODE_ISTOREL,
    BC_OPCODE_FSTOREL,
    BC_OPCODE_ASTOREL,
    BC_OPCODE_ISTORER,
    BC_OPCODE_FSTORER,
    BC_OPCODE_ASTORER,
    BC_OPCODE_ISTOREV,
    BC_OPCODE_FSTOREV,
    BC_OPCODE_ASTOREV,
    BC_OPCODE_LOCALADDR,
    BC_OPCODE_IF,
    BC_OPCODE_IFN,
    BC_OPCODE_JMP,
    BC_OPCODE_CALL,
    BC_OPCODE_FRAME,
    // size: 3-65539 bytes
    BC_OPCODE_STRLIT,
} BcOpcode;

#define _BC_STRLIT_BASE (BYTE_ALIGN + 2)
#define _BC_STRLIT_PADDING (ALIGN_UP(_BC_STRLIT_BASE) - 2 - _BC_STRLIT_BASE)

typedef struct BcStrlit {
    struct BcStrlit* prev;
    uint16_t hash;
#if _BC_STRLIT_PADDING > 0
    char _padding[_BC_STRLIT_PADDING];
#endif
    uint16_t length;
} BcStrlit;

void bc_init();
void bc_emit(BcOpcode opcode);
void bc_arg_int(KmInt value);
void bc_arg_float(KmFloat value);
void bc_arg_addr(void* value);
void bc_arg_u2(uint16_t value);
char* bc_emit_strlit(StrSlice str);

void bc_debug_print();

#endif
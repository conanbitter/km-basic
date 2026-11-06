#include "bytecode.h"
#include "stdio.h"

static const char* opcodestr[] = {
    "ipow   ",
    "fpow   ",
    "imul   ",
    "fmul   ",
    "idiv   ",
    "fdiv   ",
    "mod    ",
    "iadd   ",
    "fadd   ",
    "aadd   ",
    "isub   ",
    "fsub   ",
    "concat ",
    "shl    ",
    "shr    ",
    "ieq    ",
    "feq    ",
    "seq    ",
    "ineq   ",
    "fneq   ",
    "sneq   ",
    "igt    ",
    "fgt    ",
    "igteq  ",
    "fgteq  ",
    "ils    ",
    "fls    ",
    "ilseq  ",
    "flseq  ",
    "and    ",
    "or     ",
    "xor    ",
    "ineg   ",
    "fneg   ",
    "not    ",
    "itof   ",
    "ftoi   ",
    "iloadp ",
    "floadp ",
    "aloadp ",
    "istorep",
    "fstorep",
    "astorep",
    "dup    ",
    "drop   ",
    "ret    ",
    "retres ",
    "syscall",
    "ipush  ",
    "fpush  ",
    "apush  ",
    "iload  ",
    "fload  ",
    "aload  ",
    "iloadl ",
    "floadl ",
    "aloadl ",
    "iloadr ",
    "floadr ",
    "aloadr ",
    "iloadv ",
    "floadv ",
    "aloadv ",
    "istore ",
    "fstore ",
    "astore ",
    "istorel",
    "fstorel",
    "astorel",
    "istorer",
    "fstorer",
    "astorer",
    "istorev",
    "fstorev",
    "astorev",
    "locaddr",
    "if     ",
    "ifn    ",
    "jmp    ",
    "call   ",
    "frame  ",
    "strlit ",
};

static uintptr_t addr;

void bc_init() {
    addr = 0;
}

void bc_emit(BcOpcode opcode) {
    printf("\n %08" PRIXPTR ": %s", addr, opcodestr[opcode]);
    addr++;
}

void bc_arg_int(KmInt value) {
    printf(" %" PRIkmINT, value);
    addr += sizeof(value);
}

void bc_arg_float(KmFloat value) {
    printf(" %f", value);
    addr += sizeof(value);
}

void bc_arg_addr(void* value) {
    printf(" %p", value);
    addr += sizeof(value);
}

void bc_arg_u2(uint16_t value) {
    printf(" %" PRIu16, value);
    addr += sizeof(value);
}
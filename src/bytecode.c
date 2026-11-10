#include <assert.h>
#include <stddef.h>

#include "bytecode.h"
#include "stdio.h"
#include "memblock.h"

static_assert(offsetof(BcStrlit, length) == sizeof(BcStrlit) - sizeof(uint16_t), "length field must remain at the end of BcStrlit!");

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
static MemSegment bc_seg;

void bc_init() {
    addr = 0;
    bc_seg = mem_request();
}

#define BC_APPEND(typename, value) *((typename*)mem_alloc_right(&bc_seg, sizeof(typename))) = value

void bc_emit(BcOpcode opcode) {
    BC_APPEND(uint8_t, opcode);
}

void bc_arg_int(KmInt value) {
    BC_APPEND(KmInt, value);
}

void bc_arg_float(KmFloat value) {
    BC_APPEND(KmFloat, value);
}

void bc_arg_addr(void* value) {
    BC_APPEND(void*, value);
}

void bc_arg_u2(uint16_t value) {
    BC_APPEND(uint16_t, value);
}

void bc_emit_strlit(StrSlice str) {

}

void bc_debug_print() {
    uint8_t* cur = bc_seg.end;
    uint8_t* end = bc_seg.cur_end;

    while (cur >= end)
    {
        cur--;
        BcOpcode opcode = *cur;
        printf("%p: %s", cur, opcodestr[opcode]);

        if (opcode == BC_OPCODE_IPUSH) {
            cur -= sizeof(KmInt);
            KmInt value = *((KmInt*)cur);
            printf(" %" PRIkmINT, value);
        }

        if (opcode == BC_OPCODE_FPUSH) {
            cur -= sizeof(KmFloat);
            KmFloat value = *((KmFloat*)cur);
            printf(" %f", value);
        }

        if (opcode == BC_OPCODE_APUSH || (opcode >= BC_OPCODE_ILOAD && opcode <= BC_OPCODE_CALL)) {
            cur -= sizeof(void*);
            void* value = *((void**)cur);
            printf(" [%p]", value);
        }

        if (opcode == BC_OPCODE_SYSCALL) {
            cur -= sizeof(uint16_t);
            uint16_t value = *((uint16_t*)cur);
            printf(" %" PRIu16, value);
        }

        if (opcode == BC_OPCODE_FRAME) {
            cur -= sizeof(uint16_t);
            uint16_t value1 = *((uint16_t*)cur);
            cur -= sizeof(uint16_t);
            uint16_t value2 = *((uint16_t*)cur);
            printf(" %" PRIu16 ", %" PRIu16, value1, value2);
        }

        printf("\n");
    }

}
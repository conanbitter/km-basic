#include "exptree.h"
#include "lexer.h"
#include "bytecode.h"
#include "memblock.h"

static MemSegment expt_seg;

void expt_init() {
    expt_seg = mem_request();
}

void expt_finish() {
    mem_free(&expt_seg);
}

void* expt_alloc(size_t size, ExptNodeType node_type) {
    ExptNode* body = (ExptNode*)mem_alloc_left(&expt_seg, size);
    body->node_type = node_type;
    return body;
}

uint8_t expt_addr(void* from, void* to) {
    uintptr_t addr = (uintptr_t)from - (uintptr_t)to;
    /*if (addr > 255) {
        printf("[%d:%d] ERROR: expression is to complex",
            token.line,
            token.col
        );
    }*/
    return (uint8_t)addr;
}

void expt_codegen(void* root) {
    ExptNode* cur = (ExptNode*)root;
    switch (cur->node_type)
    {
    case EXPT_TYPE_ExptBinop:
    {
        ExptBinop* node = (ExptBinop*)cur;

        void* left = (char*)root - node->left;
        void* right = (char*)root - node->right;
        expt_codegen(left);
        expt_codegen(right);

        bc_emit(node->opcode + BC_OPCODE_IPOW);
    }
    break;

    case EXPT_TYPE_ExptUnop:
    {
        ExptUnop* node = (ExptUnop*)cur;

        void* arg = (char*)root - node->arg;
        expt_codegen(arg);

        bc_emit(node->opcode + BC_OPCODE_INEG);
    }
    break;

    case EXPT_TYPE_ExptConst:
    {
        ExptConst* node = (ExptConst*)cur;
        switch (node->valtype)
        {
        case TYPE_INT:
            bc_emit(BC_OPCODE_IPUSH);
            bc_arg_int(node->value.int_val);
            break;

        case TYPE_FLOAT:
            bc_emit(BC_OPCODE_FPUSH);
            bc_arg_float(node->value.float_val);
            break;

        case TYPE_STRING:
            bc_emit(BC_OPCODE_APUSH);
            bc_arg_addr(node->value.str_val);
            break;

        default:
            break;
        }
    }
    break;

    case EXPT_TYPE_ExptLoad:
    {
        ExptLoad* node = (ExptLoad*)cur;
        switch (node->datatype)
        {
        case TYPE_INT:
            bc_emit(BC_OPCODE_ILOAD);
            break;

        case TYPE_FLOAT:
            bc_emit(BC_OPCODE_FLOAD);
            break;

        default:
            break;
        }
        bc_arg_addr(node->addr);
    }
    break;
    }
}

#ifndef NDEBUG

static const char* binopstr[] = {
    "I^ ",
    "F^ ",
    "I* ",
    "F* ",
    "\\  ",
    "/  ",
    "MOD",
    "I+ ",
    "F+ ",
    "P+ ",
    "I- ",
    "F- ",
    "&  ",
    "<< ",
    ">> ",
    "I==",
    "F==",
    "$==",
    "I!=",
    "F!=",
    "$!=",
    "I> ",
    "F> ",
    "I>=",
    "F>=",
    "I< ",
    "F< ",
    "I<=",
    "F<=",
    "AND",
    "&& ",
    "OR ",
    "|| ",
    "XOR",
};

static const char* unopstr[] = {
    "XOR",
    "NEG",
    "NOT",
    "I2F",
    "F2I",
    "I2S",
    "S2I",
    "F2S",
    "S2F",
};

#endif

void exptr_debug_print(void* root) {
#ifndef NDEBUG
    ExptNode* end = (ExptNode*)expt_seg.cur_start;
    ExptNode* cur = (ExptNode*)expt_seg.start;
    uintptr_t _start = (uintptr_t)expt_seg.start;

    printf("     root = %" PRIuPTR "\n\n", (uintptr_t)root - _start);

    while (cur != end)
    {
        printf("%4" PRIuPTR " ", (uintptr_t)cur - _start);
        switch (cur->node_type)
        {
        case EXPT_TYPE_ExptBinop:
        {
            ExptBinop* node = (ExptBinop*)cur;
            uintptr_t from = (uintptr_t)cur - (uintptr_t)expt_seg.start;
            printf("binop %s %" PRIuPTR ", %" PRIuPTR "\n", binopstr[node->opcode], from - (uintptr_t)node->left, from - (uintptr_t)node->right);
            cur += sizeof(ExptBinop);
        }
        break;

        case EXPT_TYPE_ExptUnop:
        {
            ExptUnop* node = (ExptUnop*)cur;
            uintptr_t from = (uintptr_t)cur - (uintptr_t)expt_seg.start;
            printf("unop  %s %" PRIuPTR "\n", unopstr[node->opcode], from - (uintptr_t)node->arg);
            cur += sizeof(ExptUnop);
        }
        break;

        case EXPT_TYPE_ExptConst:
        {
            ExptConst* node = (ExptConst*)cur;
            switch (node->valtype)
            {
            case TYPE_INT:
                printf("intlit    %" PRIkmINT "\n", node->value.int_val);
                break;

            case TYPE_FLOAT:
                printf("floatlit  %f\n", node->value.float_val);
                break;

            case TYPE_STRING:
                printf("strlit    %zu\n", node->value.str_val);
                break;

            default:
                break;
            }
            cur += sizeof(ExptConst);
        }
        break;

        case EXPT_TYPE_ExptLoad:
        {
            ExptLoad* node = (ExptLoad*)cur;
            printf("load      %s\n", type2str(node->datatype));
            cur += sizeof(ExptLoad);
        }
        break;
        }
    }
#endif
}
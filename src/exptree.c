#include "exptree.h"
#include "lexer.h"

static uint8_t* tree_start;
static uint8_t* tree_cur;
static uint8_t* tree_end;

uint8_t* expt_init(uint8_t* start) {
    tree_cur = tree_start = start;
    tree_end = tree_start + EXPTREE_SIZE;
    return tree_end;
}

void* expt_alloc(size_t size, ExptNodeType node_type) {
    if ((tree_cur + size) >= tree_end) {
        printf("Run out of expr tree memory");
        exit(1);
    }
    ExptNode* body = (ExptNode*)tree_cur;
    body->node_type = node_type;
    tree_cur += size;
    return body;
}

uint8_t exptr_addr(void* from, void* to) {
    uintptr_t addr = (uintptr_t)from - (uintptr_t)to;
    if (addr > 255) {
        printf("[%d:%d] ERROR: expression is to complex",
            token.line,
            token.col
        );
    }
    return (uint8_t)addr;
}

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
    "F2I"
};

void exptr_debug_print(void* root) {
    ExptNode* end = (ExptNode*)tree_cur;
    ExptNode* cur = (ExptNode*)tree_start;
    uintptr_t _start = (uintptr_t)tree_start;

    printf("     root = %" PRIuPTR "\n\n", (uintptr_t)root - _start);

    while (cur != end)
    {
        printf("%4" PRIuPTR " ", (uintptr_t)cur - _start);
        switch (cur->node_type)
        {
        case EXPT_TYPE_ExptBinop:
        {
            ExptBinop* node = (ExptBinop*)cur;
            uintptr_t from = (uintptr_t)cur - (uintptr_t)tree_start;
            printf("binop %s %" PRIuPTR ", %" PRIuPTR "\n", binopstr[node->opcode], from - (uintptr_t)node->left, from - (uintptr_t)node->right);
            cur += sizeof(ExptBinop);
        }
        break;

        case EXPT_TYPE_ExptUnop:
        {
            ExptUnop* node = (ExptUnop*)cur;
            uintptr_t from = (uintptr_t)cur - (uintptr_t)tree_start;
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
}
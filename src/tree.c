#include "tree.h"
#include "stdio.h"
#include "inttypes.h"

static const char* opstr[] = {
    "I^ ",
    "F^ ",
    "I* ",
    "F* ",
    "\\  ",
    "/  ",
    "MOD",
    "I+ ",
    "F+ ",
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
    "NEG",
    "NOT",
    "I2F",
    "F2I"
};

void debug_print_tree(char* start, char* end) {
    TreeNode* _end = (TreeNode*)end;
    TreeNode* cur = (TreeNode*)start;
    uintptr_t _start = (uintptr_t)start;
    while (cur != _end)
    {
        printf("%4" PRIuPTR " ", (uintptr_t)cur - _start);
        switch (cur->node_type)
        {
        case NODE_EXPROP:
            printf("op %s    [%" PRIuPTR "]", opstr[cur->exprop.op], (uintptr_t)(cur->exprop.left) - _start);
            if (cur->exprop.right != NULL) {
                printf(", [%" PRIuPTR "]\n", (uintptr_t)(cur->exprop.right) - _start);
            } else {
                printf("\n");
            }
            break;

        case NODE_FLOATLIT:
            printf("floatlit  %f\n", cur->floatlit);
            break;

        case NODE_INTLIT:
            printf("intlit    %" PRIkmINT "\n", cur->intlit);
            break;

        case NODE_STRLIT:
            printf("strlit\n");
            break;

        case NODE_LOAD:
            printf("load %s  %" PRIuPTR "\n", cur->load.is_local ? "loc" : "glb", cur->load.offset);
            break;
        }
        cur++;
    }
}
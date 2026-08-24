#include "tree.h"

void debug_print_tree(char* start, char* end) {
    TreeNode* _end = (TreeNode*)end;
    TreeNode* cur = (TreeNode*)start;
    uintptr_t _start = (uintptr_t)start;
    while (cur != _end)
    {
        printf("%4d ", (uintptr_t)cur - _start);
        switch (cur->node_type)
        {
        case NODE_BINOP:
            printf("binop     %d  [%d], [%d]\n", cur->binop.op, (uintptr_t)(cur->binop.left) - _start, (uintptr_t)(cur->binop.right) - _start);
            break;

        case NODE_FLOATLIT:
            printf("floatlit  %f\n", cur->floatlit);
            break;

        case NODE_INTLIT:
            printf("intlit    %d\n", cur->intlit);
            break;

        case NODE_ITOF:
            printf("itof      [%d]\n", (uintptr_t)(cur->child) - _start);
            break;
        }
        cur++;
    }
}
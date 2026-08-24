#include "tree.h"

void debug_print_tree(char* start, char* end) {
    TreeNode* _end = (TreeNode*)end;
    TreeNode* cur = (TreeNode*)start;
    while (cur != _end)
    {
        printf("%p ", (void*)cur);
        switch (cur->node_type)
        {
        case NODE_BINOP:
            printf("binop     %d  %p, %p\n", cur->binop.op, (void*)(cur->binop.left), (void*)(cur->binop.right));
            break;

        case NODE_FLOATLIT:
            printf("floatlit  %f\n", cur->floatlit);
            break;

        case NODE_INTLIT:
            printf("intlit    %d\n", cur->intlit);
            break;

        case NODE_ITOF:
            printf("itof      %p\n", (void*)(cur->child));
            break;
        }
        cur++;
    }
}
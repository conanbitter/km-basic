#include "tree.h"
#include "memblock.h"
#include <stdio.h>
#include <inttypes.h>


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

void tree_append(TreeNode** first_node, TreeNode** last_node, TreeNode* node) {
    TreeNode* item = mem_add_node();
    item->node_type = NODE_LISTITEM;
    item->list.node = node;
    item->list.next = NULL;

    if (*last_node != NULL) (*last_node)->list.next = item;
    *last_node = item;
    if (*first_node == NULL) *first_node = item;
}

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
            char* string = (char*)(cur + 1);
            uint16_t length = cur->strlit.length;
            printf("strlit    \"%.*s\" (hash: %04X)\n", length, string, cur->strlit.hash);
            cur = (TreeNode*)((char*)(cur + 1) + ALIGN_UP(length)) - 1;
            break;

        case NODE_LOAD:
            printf("load %s  %" PRIuPTR "\n", cur->load.is_local ? "loc" : "glb", cur->load.offset);
            break;

        case NODE_STORE:
            printf("store %s [%" PRIuPTR "]=(%s)%" PRIuPTR "\n",
                cur->store.is_local ? "loc" : "glb",
                cur->store.offset,
                type2str(cur->store.value_type),
                (uintptr_t)(cur->store.value) - _start);
            break;

        case NODE_LISTITEM:
            if (cur->list.next == NULL) {
                printf("list      node %" PRIuPTR ", end\n",
                    (uintptr_t)(cur->list.node) - _start);
            } else {
                printf("list      node %" PRIuPTR ", next %" PRIuPTR "\n",
                    (uintptr_t)(cur->list.node) - _start,
                    (uintptr_t)(cur->list.next) - _start);
            }
            break;

        case NODE_DUMMY:
            printf("dummy\n");
            break;
        }
        cur++;
    }
}
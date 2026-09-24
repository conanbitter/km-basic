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

void debug_print_tree(char* start, char* end, TreeNode* root, const char* filename) {
    FILE* fl = fopen(filename, "w");

    TreeNode* _end = (TreeNode*)end;
    TreeNode* cur = (TreeNode*)start;
    uintptr_t _start = (uintptr_t)start;

    fprintf(fl, "flowchart TD\nRoot---id%" PRIuPTR "\n", (uintptr_t)root - (uintptr_t)(mem_free_end));

    while (cur != _end)
    {
        fprintf(fl, "id%" PRIuPTR, (uintptr_t)cur - _start);
        switch (cur->node_type)
        {
        case NODE_EXPROP:
            fprintf(fl, "[[\"%s\"]]\n", opstr[cur->exprop.op]);
            if (cur->exprop.right != NULL) {
                fprintf(fl, "id%" PRIuPTR "---|left|id%" PRIuPTR "\n",
                    (uintptr_t)cur - _start,
                    (uintptr_t)(cur->exprop.left) - _start);
                fprintf(fl, "id%" PRIuPTR "---|right|id%" PRIuPTR "\n",
                    (uintptr_t)cur - _start,
                    (uintptr_t)(cur->exprop.right) - _start);
            } else {
                fprintf(fl, "id%" PRIuPTR "---id%" PRIuPTR "\n",
                    (uintptr_t)cur - _start,
                    (uintptr_t)(cur->exprop.left) - _start);
            }
            break;

        case NODE_FLOATLIT:
            fprintf(fl, "[/\"float lit\\n%f\"/]\n", cur->floatlit);
            break;

        case NODE_INTLIT:
            fprintf(fl, "[/\"int lit\\n%" PRIkmINT "\"/]\n", cur->intlit);
            break;

        case NODE_STRLIT:
            char* string = (char*)(cur + 1);
            uint16_t length = cur->strlit.length;
            fprintf(fl, "[/\"str lit\\n'%.*s'\\n(hash: %04X)\"/]\n", length, string, cur->strlit.hash);
            cur = (TreeNode*)((char*)(cur + 1) + ALIGN_UP(length)) - 1;
            break;

        case NODE_LOAD:
            fprintf(fl, "[\"load %s\\n$%" PRIuPTR "\"]\n", cur->load.is_local ? "loc" : "glb", cur->load.offset);
            break;

        case NODE_STORE:
            fprintf(fl, "[\"store %s\\n(%s)$%" PRIuPTR "\"]\n",
                cur->store.is_local ? "loc" : "glb",
                type2str(cur->store.value_type),
                cur->store.offset);
            fprintf(fl, "id%" PRIuPTR "---id%" PRIuPTR "\n",
                (uintptr_t)cur - _start,
                (uintptr_t)(cur->store.value) - _start);
            break;

        case NODE_LISTITEM:
            fprintf(fl, "(list)\n");
            fprintf(fl, "id%" PRIuPTR "---|node|id%" PRIuPTR "\n",
                (uintptr_t)cur - _start,
                (uintptr_t)(cur->list.node) - _start);
            if (cur->list.next != NULL) {
                fprintf(fl, "id%" PRIuPTR "---|next|id%" PRIuPTR "\n",
                    (uintptr_t)cur - _start,
                    (uintptr_t)(cur->list.next) - _start);
            }
            break;

        case NODE_DUMMY:
            fprintf(fl, "[dummy]\n");
            break;
        }
        cur++;
    }

    fclose(fl);
}
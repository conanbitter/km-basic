#include "exptree.h"

static uint8_t* tree_start;
static uint8_t* tree_cur;
static uint8_t* tree_end;

uint8_t* expt_init(uint8_t* start) {
    tree_cur = tree_start = start;
    tree_end = tree_start + EXPTREE_SIZE;
    return tree_end;
}

void* expt_alloc(size_t size) {
    if ((tree_cur + size) >= tree_end) {
        printf("Run out of expr tree memory");
        exit(1);
    }
    char* body = tree_cur;
    tree_cur += size;
    return body;
}
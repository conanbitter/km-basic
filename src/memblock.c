#include "common.h"
#include "memblock.h"

#include <string.h>

const uintptr_t ptr_alignment = _Alignof(KmInt);

static char* align_ptr(char* ptr) {
    uintptr_t intptr = (uintptr_t)ptr;
    uintptr_t result = (intptr + ptr_alignment - 1) & ~(ptr_alignment - 1);
    return (char*)result;
}

void mem_init(MemBlock* block, char* buffer, char* buffer_end) {
    block->begin = buffer;
    block->end = buffer_end;
    block->current = buffer;
    block->tail = buffer_end;
    block->temp = buffer + sizeof(NameHeader);
    block->prev = NULL;
    block->last_strlit = NULL;
}

void mem_emplace(MemBlock* dict, size_t length, NameEntryType entry_type) {
    NameHeader* entry = (NameHeader*)dict->current;
    dict->current = align_ptr(dict->current + sizeof(NameHeader) + length);
    entry->prev = dict->prev;
    entry->text_len = length;
    entry->padding = dict->current - (char*)entry;
    entry->entry_type = entry_type;
    dict->prev = entry;
    dict->temp = dict->current + sizeof(NameHeader);
}

static char* get_body(NameHeader* header) {
    return (char*)header + header->text_len + header->padding;
}

char* mem_allot(MemBlock* dict, size_t size) {
    if ((dict->current + size) >= dict->tail) {
        printf("Run out of memory");
        exit(1);
    }
    char* body = dict->current;
    dict->current += size;
    return body;
}

TreeNode* mem_add_node(MemBlock* dict) {
    if ((dict->tail - sizeof(TreeNode)) <= dict->current) {
        printf("Run out of memory");
        exit(1);
    }
    dict->tail -= sizeof(TreeNode);
    return (TreeNode*)dict->tail;
}

TreeNode* mem_strlit_node(MemBlock* block, size_t length) {
    uint16_t aligned_length = ALIGN_UP(length);

    if ((block->tail - sizeof(TreeNode) - aligned_length) <= block->current) {
        printf("Run out of memory");
        exit(1);
    }

    char* dest = block->tail - aligned_length;
    memmove(dest, block->temp, length);
    block->tail -= sizeof(TreeNode) + aligned_length;

    TreeNode* node = (TreeNode*)block->tail;
    node->node_type = NODE_STRLIT;
    node->strlit.length = length;
    node->strlit.prev = block->last_strlit;
    node->strlit.hash = 0;
    block->last_strlit = node;

    return node;
}
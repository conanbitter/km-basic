#ifndef MEMBLOCK_H
#define MEMBLOCK_H

#include "tree.h"
#include <stdint.h>

typedef enum NameEntryType {
    NAME_ENTRY_VAR,
    NAME_ENTRY_VAR_LOCAL,
    NAME_ENTRY_ARRAY,
    NAME_ENTRY_ARRAY_LOCAL,
    NAME_ENTRY_FUNC,
    NAME_ENTRY_CONST
} NameEntryType;

typedef struct NameHeader {
    struct NameHeader* prev;
    uint16_t text_len;
    uint8_t padding;
    uint8_t entry_type;
} NameHeader;

typedef struct MemBlock {
    char* begin;
    char* end;

    char* current;
    char* tail;

    // Nametable data
    char* temp;
    NameHeader* prev;

    // Tree data
    TreeNode* last_strlit;
} MemBlock;

typedef struct VarBody {
    uintptr_t offset;
    DataType value_type;
} GlobalVarBody;

void mem_init(MemBlock* dict, char* buffer, char* buffer_end);
void mem_emplace(MemBlock* dict, size_t length, NameEntryType entry_type);
TreeNode* mem_add_node(MemBlock* dict);
TreeNode* mem_strlit_node(MemBlock* dict, size_t length);
char* mem_allot(MemBlock* dict, size_t size);

#endif
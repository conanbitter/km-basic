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

typedef struct VarBody {
    uintptr_t offset;
    DataType value_type;
} GlobalVarBody;

typedef struct ConstBody {
    KmValue value;
    DataType value_type;
} ConstBody;

extern char* mem_end;
extern char* mem_free_end;
extern char* mem_temp;

void mem_init(char* buffer, char* buffer_end);
void mem_emplace(size_t length, NameEntryType entry_type);
TreeNode* mem_add_node();
TreeNode* mem_strlit_node(size_t length);
char* mem_alloc_size(size_t size);

#define MEM_ALLOC(typename) (typename*)mem_alloc_size(sizeof(typename))

#endif
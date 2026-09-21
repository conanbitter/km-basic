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
    uint16_t decl_line;
    uint16_t decl_col;
    uint16_t text_len;
    uint8_t padding;
    uint8_t entry_type;
} NameHeader;

typedef struct VarBody {
    uintptr_t offset;
    DataType value_type;
} VarBody;

typedef struct ConstBody {
    KmValue value;
    DataType value_type;
} ConstBody;

extern char* mem_end;
extern char* mem_free_end;
extern char* mem_temp;

void mem_init(char* buffer, char* buffer_end);

TreeNode* mem_add_node();
TreeNode* mem_strlit_node(size_t length);

void name_emplace(size_t length, NameEntryType entry_type, uint16_t decl_line, uint16_t decl_col);
NameHeader* name_find(size_t length);
void name_check_redecl(size_t length, int line, int col);
void* name_alloc_size(size_t size);
void* name_get_body(NameHeader* header);

#define NAME_ALLOC(typename) (typename*)name_alloc_size(sizeof(typename))

#endif
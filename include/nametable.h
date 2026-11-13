#ifndef NAMETABLE_H
#define NAMETABLE_H

#include <stdint.h>
#include "common.h"
#include "memblock.h"

typedef enum NtEntryType {
    NT_ENTRY_VAR,
    NT_ENTRY_VAR_LOCAL,
    NT_ENTRY_ARRAY,
    NT_ENTRY_ARRAY_LOCAL,
    NT_ENTRY_FUNC,
    NT_ENTRY_CONST
} NtEntryType;

typedef struct NtHeader {
    struct NtHeader* prev;
    uint16_t decl_line;
    uint16_t decl_col;
    uint16_t text_len;
    uint8_t padding;
    uint8_t entry_type;
} NtHeader;

typedef struct NtVarBody {
    uintptr_t offset;
    DataType value_type;
} NtVarBody;

typedef struct NtConstBody {
    KmValue value;
    DataType value_type;
} NtConstBody;

void nt_init();
void nt_free();
void nt_new(StrSlice name, NtEntryType entry_type, uint16_t decl_line, uint16_t decl_col);
NtHeader* nt_find(StrSlice name);
void nt_check_redecl(StrSlice name, int line, int col);
void* nt_get_body(NtHeader* header);
void* nt_alloc(size_t size);

#define NT_ALLOC(typename) ((typename*)nt_alloc(sizeof(typename)))

#endif
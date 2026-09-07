#ifndef DICT_H
#define DICT_H

#include "tree.h"
#include <stdint.h>

typedef enum DictEntryType {
    DICT_STRLIT,
    DICT_VAR,
    DICT_VAR_LOCAL,
    DICT_ARRAY,
    DICT_ARRAY_LOCAL,
    DICT_FUNC,
    DICT_CONST
} DictEntryType;

typedef struct DictHeader {
    struct DictHeader* prev;
    uint16_t text_len;
    uint8_t padding;
    uint8_t entry_type;
} DictHeader;

typedef struct Dictionary {
    char* begin;
    char* end;

    char* current;
    char* tail;

    char* temp;

    DictHeader* prev;
} Dictionary;

typedef struct GlobalVarBody {
    uintptr_t offset;
    DataType value_type;
} GlobalVarBody;

typedef struct LocalVarBody {
    uintptr_t offset;
    DataType value_type;
    TreeNode* fn_id;
    bool is_ref;
} LocalVarBody;

void dict_init(Dictionary* dict, char* buffer, char* buffer_end);
void dict_emplace(Dictionary* dict, size_t length, DictEntryType entry_type);
TreeNode* dict_add_node(Dictionary* dict);
char* dict_allot(Dictionary* dict, size_t size);

#endif
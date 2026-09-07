#ifndef DICT_H
#define DICT_H

#include "tree.h"
#include <stdint.h>

typedef enum DictEntryType {
    DICT_STRLIT,
    DICT_VAR,
    DICT_ARRAY,
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

void dict_init(Dictionary* dict, char* buffer, char* buffer_end);
void dict_emplace(Dictionary* dict, size_t length, DictEntryType entry_type);
TreeNode* dict_add_node(Dictionary* dict);

#endif
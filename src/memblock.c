#include "common.h"
#include "memblock.h"

const uintptr_t ptr_alignment = _Alignof(KmInt);

static char* align_ptr(char* ptr) {
    uintptr_t intptr = (uintptr_t)ptr;
    uintptr_t result = (intptr + ptr_alignment - 1) & ~(ptr_alignment - 1);
    return (char*)result;
}

void dict_init(Dictionary* dict, char* buffer, char* buffer_end) {
    dict->begin = buffer;
    dict->end = buffer_end;
    dict->current = buffer;
    dict->tail = buffer_end;
    dict->temp = buffer + sizeof(DictHeader);
    dict->prev = NULL;
}

void dict_emplace(Dictionary* dict, size_t length, DictEntryType entry_type) {
    DictHeader* entry = (DictHeader*)dict->current;
    dict->current = align_ptr(dict->current + sizeof(DictHeader) + length);
    entry->prev = dict->prev;
    entry->text_len = length;
    entry->padding = dict->current - (char*)entry;
    entry->entry_type = entry_type;
    dict->prev = entry;
    dict->temp = dict->current + sizeof(DictHeader);
}

static char* get_body(DictHeader* header) {
    return (char*)header + header->text_len + header->padding;
}

char* dict_allot(Dictionary* dict, size_t size) {
    if ((dict->current + size) >= dict->tail) {
        printf("Run out of memory");
        exit(1);
    }
    char* body = dict->current;
    dict->current += size;
    return body;
}

TreeNode* dict_add_node(Dictionary* dict) {
    if ((dict->tail - sizeof(TreeNode)) <= dict->current) {
        printf("Run out of memory");
        exit(1);
    }
    dict->tail -= sizeof(TreeNode);
    return (TreeNode*)dict->tail;
}
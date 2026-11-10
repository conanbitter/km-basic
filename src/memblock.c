/*

MEMORY LAYOUT

During parsing:

      [Block start]
  ---------------------
       Name table
  ---------------------
      [Free space]
  ---------------------
       Tree nodes
  (and string literals)
  ---------------------
       [Block end]

During execution:

      [Block start]
  ---------------------
      Strings Arena
  ---------------------
      [Free space]
  ---------------------
       Call frames
  ---------------------
    Global variables
  ---------------------
       Tree nodes
  (and string literals)
  ---------------------
       [Block end]

*/

#include "common.h"
#include "memblock.h"

#include <string.h>
#include <stdio.h>

const uintptr_t ptr_alignment = _Alignof(KmInt);

char* mem_start;
char* mem_end;

char* mem_free_start;
char* mem_free_end;

char* mem_temp;
NameHeader* name_prev;

TreeNode* tree_last_strlit;

static char* align_ptr(char* ptr) {
    uintptr_t intptr = (uintptr_t)ptr;
    uintptr_t result = (intptr + ptr_alignment - 1) & ~(ptr_alignment - 1);
    return (char*)result;
}

void mem_init_old(char* buffer, char* buffer_end) {
    mem_start = buffer;
    mem_end = buffer_end;
    mem_free_start = buffer;
    mem_free_end = buffer_end;
    mem_temp = buffer + sizeof(NameHeader);
    name_prev = NULL;
    //tree_last_strlit = NULL;
}

void name_emplace(size_t length, NameEntryType entry_type, uint16_t decl_line, uint16_t decl_col) {
    NameHeader* entry = (NameHeader*)mem_free_start;
    mem_free_start = align_ptr(mem_free_start + sizeof(NameHeader) + length);
    entry->prev = name_prev;
    entry->text_len = length;
    entry->padding = mem_free_start - (char*)entry - sizeof(NameHeader) - length;
    entry->entry_type = entry_type;
    entry->decl_line = decl_line;
    entry->decl_col = decl_col;
    name_prev = entry;
    mem_temp = mem_free_start + sizeof(NameHeader);
}

void* name_get_body(NameHeader* header) {
    return (char*)(header + 1) + header->text_len + header->padding;
}

void* name_alloc_size(size_t size) {
    if ((mem_free_start + size) >= mem_free_end) {
        printf("Run out of memory");
        exit(1);
    }
    char* body = mem_free_start;
    mem_free_start += size;
    mem_temp = mem_free_start + sizeof(NameHeader);
    return body;
}

NameHeader* name_find(size_t length) {
    NameHeader* cur = name_prev;
    while (cur != NULL) {
        if (cur->text_len == length && memcmp(cur + 1, mem_temp, length) == 0) {
            return cur;
        }
        cur = cur->prev;
    }
    return cur;
}

void name_check_redecl(size_t length, int line, int col) {
    NameHeader* found = name_find(length);
    if (found != NULL) {
        printf("[%d:%d] ERROR: Identifier '%.*s' redeclaration, previously declared at [%"PRIu16":%"PRIu16"]\n",
            line,
            col,
            length,
            mem_temp,
            found->decl_col,
            found->decl_line);
        exit(1);
    }
}

TreeNode* mem_add_node() {
    if ((mem_free_end - sizeof(TreeNode)) <= mem_free_start) {
        printf("Run out of memory");
        exit(1);
    }
    mem_free_end -= sizeof(TreeNode);
    return (TreeNode*)mem_free_end;
}

TreeNode* mem_strlit_node(size_t length) {
    uint16_t new_hash = string_hash(mem_temp, length);

    // duplicate elimination
    TreeNode* cur = tree_last_strlit;
    while (cur != NULL)
    {
        if (cur->strlit.length == length && cur->strlit.hash == new_hash) {
            char* old_string = (char*)(cur + 1);
            if (memcmp(mem_temp, old_string, length) == 0) {
                return cur;
            }
        }
        cur = cur->strlit.prev;
    }

    uint16_t aligned_length = ALIGN_UP(length);

    if ((mem_free_end - sizeof(TreeNode) - aligned_length) <= mem_free_start) {
        printf("Run out of memory");
        exit(1);
    }

    char* dest = mem_free_end - aligned_length;
    memmove(dest, mem_temp, length);
    mem_free_end -= sizeof(TreeNode) + aligned_length;

    TreeNode* node = (TreeNode*)mem_free_end;
    node->node_type = NODE_STRLIT;
    node->strlit.length = length;
    node->strlit.prev = tree_last_strlit;
    node->strlit.hash = new_hash;
    tree_last_strlit = node;

    return node;
}

void mem_init(char* buffer, char* buffer_end) {
    mem_start = buffer;
    mem_end = buffer_end;
    mem_free_start = buffer;
    mem_free_end = buffer_end;
}

MemSegment mem_request() {
    MemSegment result = {
        .start = mem_free_start,
        .end = mem_free_end,
        .cur_start = mem_free_start,
        .cur_end = mem_free_end
    };
    return result;
}

void mem_free(MemSegment* range) {
    if (range->start != range->cur_start) {
#ifndef NDEBUG
        if (range->cur_start < mem_free_start) {
            printf("WARNING: an attempt to free left side of a memory segment that is not the topmost one");
        }
#endif
        mem_free_start = range->start;
    }

    if (range->end != range->cur_end) {
#ifndef NDEBUG
        if (range->cur_end > mem_free_end) {
            printf("WARNING: an attempt to free right side of a memory segment that is not the topmost one");
        }
#endif
        mem_free_end = range->end;
    }
}

void* mem_alloc_left(MemSegment* range, size_t size) {
#ifndef NDEBUG
    if (range->cur_start < mem_free_start) {
        printf("WARNING: an attempt to left expand a memory segment that is not the topmost one");
    }
#endif
    if ((range->cur_start + size) >= mem_free_end) {
        printf("Run out of memory");
        exit(1);
    }
    void* res = range->cur_start;
    range->cur_start += size;
    mem_free_start = range->cur_start;
    return res;
}

void* mem_alloc_right(MemSegment* range, size_t size) {
#ifndef NDEBUG
    if (range->cur_end > mem_free_end) {
        printf("WARNING: an attempt to right expand a memory segment that is not the topmost one");
    }
#endif
    if ((range->cur_end - size) <= mem_free_start) {
        printf("Run out of memory");
        exit(1);
    }
    range->cur_end -= size;
    mem_free_end = range->cur_end;
    return range->cur_end;
}

void mem_add_left(MemSegment* range, char symbol) {
#ifndef NDEBUG
    if (range->cur_start < mem_free_start) {
        printf("WARNING: an attempt to left expand a memory segment that is not the topmost one");
    }
#endif
    if ((range->cur_start + 1) >= mem_free_end) {
        printf("Run out of memory");
        exit(1);
    }
    *range->cur_start = symbol;
    range->cur_start++;
    mem_free_start = range->cur_start;
}
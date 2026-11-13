#include "nametable.h"
#include "string.h"

static MemSegment nt_seg;
static NtHeader* nt_prev;

void nt_init() {
    nt_seg = mem_request();
    nt_prev = NULL;
}

void nt_free() {
    mem_free(&nt_seg);
}

void nt_new(StrSlice name, NtEntryType entry_type, uint16_t decl_line, uint16_t decl_col) {
    NtHeader* entry = MEM_ALLOC_LEFT(&nt_seg, NtHeader);

    char* name_dest = mem_alloc_left(&nt_seg, name.length);
    mem_align_left(&nt_seg);
    if (name_dest != name.addr) {
        printf("Name \"%.*s\" is not emplaced.\n", name.length, name.addr);
        memmove(name_dest, name.addr, name.length);
    }

    entry->prev = nt_prev;
    entry->text_len = name.length;
    entry->padding = nt_seg.cur_start - name_dest - name.length;
    entry->entry_type = entry_type;
    entry->decl_line = decl_line;
    entry->decl_col = decl_col;

    nt_prev = entry;
}

NtHeader* nt_find(StrSlice name) {
    NtHeader* cur = nt_prev;
    while (cur != NULL) {
        if (cur->text_len == name.length && memcmp(cur + 1, name.addr, name.length) == 0) {
            return cur;
        }
        cur = cur->prev;
    }
    return cur;
}

void nt_check_redecl(StrSlice name, int line, int col) {
    NtHeader* found = nt_find(name);
    if (found != NULL) {
        printf("[%d:%d] ERROR: Identifier '%.*s' redeclaration, previously declared at [%"PRIu16":%"PRIu16"]\n",
            line,
            col,
            name.length,
            name.addr,
            found->decl_col,
            found->decl_line);
        exit(1);
    }
}

void* nt_get_body(NtHeader* header) {
    return (char*)(header + 1) + header->text_len + header->padding;
}

void* nt_alloc(size_t size) {
    return mem_alloc_left(&nt_seg, size);
}
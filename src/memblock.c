#include "common.h"
#include "memblock.h"

#include <stdio.h>

char* mem_start;
char* mem_end;

char* mem_free_start;
char* mem_free_end;

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
        printf("WARNING: an attempt to left expand a memory segment that is not the topmost one\n");
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
        printf("WARNING: an attempt to right expand a memory segment that is not the topmost one\n");
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

void mem_align_left(MemSegment* range) {
#ifndef NDEBUG
    if (range->cur_start < mem_free_start) {
        printf("WARNING: an attempt to left expand a memory segment that is not the topmost one\n");
    }
#endif
    char* new_start = (char*)ALIGN_UP((uintptr_t)range->cur_start);
    if (new_start >= mem_free_end) {
        printf("Run out of memory");
        exit(1);
    }
    range->cur_start = new_start;
    mem_free_start = range->cur_start;
}

void* mem_align_right(MemSegment* range) {
#ifndef NDEBUG
    if (range->cur_end > mem_free_end) {
        printf("WARNING: an attempt to right expand a memory segment that is not the topmost one\n");
    }
#endif
    char* new_end = (char*)ALIGN_DOWN((uintptr_t)range->cur_end);
    if (new_end <= mem_free_start) {
        printf("Run out of memory");
        exit(1);
    }
    range->cur_end = new_end;
    mem_free_end = range->cur_end;
    return range->cur_end;
}

void mem_add_left(MemSegment* range, char symbol) {
#ifndef NDEBUG
    if (range->cur_start < mem_free_start) {
        printf("WARNING: an attempt to left expand a memory segment that is not the topmost one\n");
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
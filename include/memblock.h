#ifndef MEMBLOCK_H
#define MEMBLOCK_H

typedef struct MemSegment {
    char* start;
    char* end;
    char* cur_start;
    char* cur_end;
} MemSegment;

typedef struct StrSlice {
    char* addr;
    size_t length;
} StrSlice;

void mem_init(char* buffer, char* buffer_end);

MemSegment mem_request();
void mem_free(MemSegment* segment);

void* mem_alloc_left(MemSegment* segment, size_t size);
void* mem_alloc_right(MemSegment* segment, size_t size);
void mem_add_left(MemSegment* segment, char symbol);

void mem_align_left(MemSegment* range);
void* mem_align_right(MemSegment* range);

#define MEM_ALLOC_LEFT(segment, typename) ((typename*)mem_alloc_left(segment, sizeof(typename)))
#define MEM_ALLOC_RIGHT(segment, typename) ((typename*)mem_alloc_right(segment, sizeof(typename)))

#endif
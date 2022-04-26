
//Autor: Bartosz Mąka

#ifndef PROJEKT_1_7_HEAP_H
#define PROJEKT_1_7_HEAP_H

#include <stdio.h>
struct memory_manager_t
{
    void *memory_start;
    void *memory_end;
    size_t memory_size;
    struct memory_chunk_t *first_memory_chunk;
};
struct memory_chunk_t
{
    struct memory_chunk_t* prev;
    struct memory_chunk_t* next;
    size_t size;
    int checksum;
    int free;
};
enum pointer_type_t
{
    pointer_null,
    pointer_heap_corrupted,
    pointer_control_block,
    pointer_inside_fences,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};
int heap_setup(void);
void heap_clean(void);
void* heap_malloc(size_t size);
void* heap_calloc(size_t number, size_t size);
void* heap_realloc(void* memblock, size_t count);
void  heap_free(void* memblock);
void  heap_free_help(struct memory_chunk_t* p);
size_t   heap_get_largest_used_block_size(void);
enum pointer_type_t get_pointer_type(const void* const pointer);
int heap_validate(void);
void plotka(struct memory_chunk_t*p);
int control_sum(const struct memory_chunk_t *p);
void* heap_malloc_aligned(size_t count);
void* heap_malloc_help(size_t size);
void* malloc_inside_aligned(struct memory_chunk_t *m,struct memory_chunk_t *w,size_t count);
void* heap_calloc_aligned(size_t number, size_t size);
void* heap_realloc_aligned(void* memblock, size_t size);
#endif //PROJEKT_1_7_HEAP_H

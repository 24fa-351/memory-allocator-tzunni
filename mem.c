#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "mem.h"

#define HEAP_SIZE 1024
#define BLOCK_SIZE 16
#define INITIAL_BLOCKS (HEAP_SIZE / BLOCK_SIZE)

static void* heap = NULL;
static struct chunk_on_heap* free_list = NULL;
static int free_list_count = 0;
static int free_list_capacity = INITIAL_BLOCKS;

void* get_me_blocks(size_t how_much) {
    void* ptr = sbrk(0);
    if (sbrk(how_much) == (void*)-1) {
        fprintf(stderr, "sbrk failed\n");
        return NULL;
    }
    return ptr;
}

void initialize_heap() {
    if (!heap) {
        heap = get_me_blocks(HEAP_SIZE);
        if (heap) {
            free_list = (struct chunk_on_heap*)malloc(INITIAL_BLOCKS * sizeof(struct chunk_on_heap));
            if (!free_list) {
                fprintf(stderr, "Free list allocation failed\n");
                return;
            }
            free_list[0].pointer_to_start = heap;
            free_list[0].size = HEAP_SIZE;
            free_list_count = 1;
        } else {
            fprintf(stderr, "Heap initialization failed\n");
        }
    }
}

void expand_free_list() {
    free_list_capacity *= 2;
    struct chunk_on_heap* new_free_list = (struct chunk_on_heap*)realloc(free_list, free_list_capacity * sizeof(struct chunk_on_heap));
    if (!new_free_list) {
        fprintf(stderr, "Failed to expand free list\n");
        return;
    }
    free_list = new_free_list;
}

void heap_insert(struct chunk_on_heap chunk) {
    if (free_list_count >= free_list_capacity) {
        expand_free_list();
    }
    free_list[free_list_count++] = chunk;
    // Maintain min-heap property
    for (int i = free_list_count - 1; i > 0 && free_list[i].size < free_list[(i - 1) / 2].size; i = (i - 1) / 2) {
        struct chunk_on_heap temp = free_list[i];
        free_list[i] = free_list[(i - 1) / 2];
        free_list[(i - 1) / 2] = temp;
    }
}

struct chunk_on_heap heap_extract_min() {
    if (free_list_count == 0) {
        fprintf(stderr, "Heap underflow\n");
        struct chunk_on_heap empty_chunk = {0, NULL};
        return empty_chunk;
    }
    struct chunk_on_heap min_chunk = free_list[0];
    free_list[0] = free_list[--free_list_count];
    // Maintain min-heap property
    for (int i = 0; 2 * i + 1 < free_list_count;) {
        int smallest = i;
        if (free_list[2 * i + 1].size < free_list[smallest].size) {
            smallest = 2 * i + 1;
        }
        if (2 * i + 2 < free_list_count && free_list[2 * i + 2].size < free_list[smallest].size) {
            smallest = 2 * i + 2;
        }
        if (smallest == i) break;
        struct chunk_on_heap temp = free_list[i];
        free_list[i] = free_list[smallest];
        free_list[smallest] = temp;
        i = smallest;
    }
    return min_chunk;
}

void* xmalloc(size_t size) {
    initialize_heap();
    size = (size + (BLOCK_SIZE - 1)) & ~(BLOCK_SIZE - 1); // Align size

    for (int i = 0; i < free_list_count; i++) {
        if (free_list[i].size >= size) {
            struct chunk_on_heap chunk = heap_extract_min();
            if (chunk.size > size) {
                struct chunk_on_heap leftover = { chunk.size - size, chunk.pointer_to_start + size };
                heap_insert(leftover);
            }
            return chunk.pointer_to_start;
        }
    }

    // If no suitable chunk found, get more memory from the system
    void* new_block = get_me_blocks(size > HEAP_SIZE ? size : HEAP_SIZE);
    if (!new_block) {
        fprintf(stderr, "Out of memory\n");
        return NULL;
    }
    struct chunk_on_heap new_chunk = { size > HEAP_SIZE ? size : HEAP_SIZE, new_block };
    heap_insert(new_chunk);
    return xmalloc(size); // Retry allocation
}

void xfree(void* ptr) {
    if (!ptr) {
        return;
    }

    // Find the size of the block to be freed
    size_t block_size = 0;
    for (int i = 0; i < free_list_count; i++) {
        if (free_list[i].pointer_to_start == ptr) {
            block_size = free_list[i].size;
            break;
        }
    }

    // Add the block back to the free list
    struct chunk_on_heap chunk = { block_size, ptr };
    heap_insert(chunk);
}

void* xrealloc(void* ptr, size_t new_size) {
    if (!ptr) return xmalloc(new_size);
    if (new_size == 0) {
        xfree(ptr);
        return NULL;
    }

    void* new_ptr = xmalloc(new_size);
    if (new_ptr) {
        // Copy the minimum of the old and new sizes
        size_t old_size = 0;
        for (int i = 0; i < free_list_count; i++) {
            if (free_list[i].pointer_to_start == ptr) {
                old_size = free_list[i].size;
                break;
            }
        }
        memcpy(new_ptr, ptr, old_size < new_size ? old_size : new_size);
        xfree(ptr);
    }
    return new_ptr;
}

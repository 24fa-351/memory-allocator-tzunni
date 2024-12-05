#include <stddef.h>

struct chunk_on_heap {
    int size;
    char* pointer_to_start;
};

void* xmalloc(size_t size);
void xfree(void* ptr);
void* xrealloc(void* ptr, size_t new_size);
void initialize_heap();
void* get_me_blocks(size_t how_much);
void heap_insert(struct chunk_on_heap chunk);
struct chunk_on_heap heap_extract_min();
void expand_free_list();

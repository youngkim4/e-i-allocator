#include "./allocator.h"
#include "./debug_break.h"
#include <string.h>
#include <stdio.h>

static void *segment_start;
static size_t segment_size;
static size_t nused;

typedef struct header {
    size_t data; 
} header;

size_t roundup(size_t sz, size_t mult);
bool isfree(header *h);

bool myinit(void *heap_start, size_t heap_size) {
    if (heap_size < (2*ALIGNMENT)) {
        return false;
    }

    
    segment_start = heap_start;
    segment_size = heap_size - sizeof(header);
    
    header* first_header = heap_start;
    first_header->data = segment_size;
    
    return true;
}

size_t roundup(size_t sz, size_t mult) {
    return (sz + mult - 1) & ~(mult - 1);
}

bool isfree(header *h) {
    return !(h->data & 0x1);
}

size_t getsize(header *h) {
    return h->data & ~(0x7);
}

void *mymalloc(size_t requested_size) {
    // TODO(you!): remove the line below and implement this!
    if (requested_size > MAX_REQUEST_SIZE || requested_size == 0) {
        return NULL;
    }
    size_t needed = roundup(requested_size, ALIGNMENT);

    header *current_header = segment_start;

    while (true) {
        if (isfree(current_header) && getsize(current_header) >= needed) {
            if (current_header->data - needed >= sizeof(header) + ALIGNMENT) {
                size_t remaining = current_header->data;
                current_header->data = needed + (current_header->data & 0x1);
                header *next = (header*)((char*)current_header + sizeof(header) + needed);
                next->data = remaining - needed - sizeof(header);
            }
            current_header->data += 1;

            return (char*)current_header + sizeof(header);
        }
        current_header = (header*)((char*)current_header + sizeof(header) + getsize(current_header));
    }
    
    return NULL;
}

void myfree(void *ptr) {
    // TODO(you!): implement this!
}

void *myrealloc(void *old_ptr, size_t new_size) {
    // TODO(you!): remove the line below and implement this!
    void *new_ptr = mymalloc(new_size);
    memcpy(new_ptr, old_ptr, new_size);
    myfree(old_ptr);
    return new_ptr;
}

bool validate_heap() {
    /* TODO(you!): remove the line below and implement this to
     * check your internal structures!
     * Return true if all is ok, or false otherwise.
     * This function is called periodically by the test
     * harness to check the state of the heap allocator.
     * You can also use the breakpoint() function to stop
     * in the debugger - e.g. if (something_is_wrong) breakpoint();
     */
    return true;
}

/* Function: dump_heap
 * -------------------
 * This function prints out the the block contents of the heap.  It is not
 * called anywhere, but is a useful helper function to call from gdb when
 * tracing through programs.  It prints out the total range of the heap, and
 * information about each block within it.
 */
void dump_heap() {
    // TODO(you!): Write this function to help debug your heap.
    
}

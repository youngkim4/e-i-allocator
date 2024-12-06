#include "./allocator.h"
#include "./debug_break.h"
#include <string.h>
#include <stdio.h>

typedef struct header {
    size_t data;
} header; 

static void *segment_start;
static void *segment_end;
static size_t segment_size;
static header *first_header;

bool myinit(void *heap_start, size_t heap_size) {
    if (heap_size < (2*ALIGNMENT)) {
        return false;
    }

    
    segment_start = heap_start;
    segment_size = heap_size - sizeof(header);
    
    first_header = heap_start;
    first_header->data = segment_size;

    segment_end = (char*)segment_start + heap_size;
    
    return true;
}

/*
The roundup helper function rounds a size sz up to the nearest multiple
of size mult, which wfill be ALLOCATION (8)
 */
size_t roundup(size_t sz, size_t mult) {
    return (sz + mult - 1) & ~(mult - 1);
}

/*
The isfree helper function checks whether a header indicates whether a block
of memory is free or not by returning the value of its least significant bit.
 */
bool isfree(header *h) {
    return !(h->data & 0x1);
}

/*
The getsize function returns the size of the block of memory the header heads
by returning the value of the header without its 3 least significant bits.
 */
size_t getsize(header *h) {
    return h->data & ~(0x7);
}

/* 
The implicit mymalloc function uses a first-fit approach to identify headers
which both indicate of a block is available and if it has enough space for requested_size.

The function returns a pointer to the allocated data (not the header for it)
 */
void *mymalloc(size_t requested_size) {
    // if requested_size is too big or if it's zero, return NULL
    if (requested_size > MAX_REQUEST_SIZE || requested_size == 0) {
        return NULL;
    }
    
    size_t needed = roundup(requested_size, ALIGNMENT);

    header *current_header = first_header;
    
    // iterate through all headers
    while (true) {
        if (isfree(current_header) && getsize(current_header) >= needed) { // if header indicates free block and there is enough size
            // check if we can create another header after we allocate memory
            if (getsize(current_header) - needed >= sizeof(header) + ALIGNMENT) {  // header needs the space for the size of itself and at least 8 bytes
                size_t surplus = getsize(current_header);
                current_header->data = needed;
                header *next = (header*)((char*)current_header + sizeof(header) + needed); // create new header
                next->data = surplus - needed - sizeof(header);
            }
            
            current_header->data += 1; // change header from indicating free to allocated

            return (char*)current_header + sizeof(header); // return pointer to allocated memory
        }
        current_header = (header*)((char*)current_header + sizeof(header) + getsize(current_header)); // next header
    }
    
    return NULL;
}

/*
The implicit myfree function takes in a pointer to an allocated block and adjusts
its header to indicate that the block is now free.
 */
void myfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    
    header *newheader = (header*)((char*)ptr - sizeof(header));
    newheader->data -= 1;
}

/*
The implicit myrealloc function first checks for edge cases, such as 
the ptr being NULL or the new_size being 0, and then reallocs as normal
by utilizing new_ptr with memcpy, freeing the old memory using myfree, and then
returning new_ptr.
 */
void *myrealloc(void *old_ptr, size_t new_size) {
    if (old_ptr == NULL) {
        // realloc(NULL, x) means to malloc
        if (new_size != 0) {
            return mymalloc(new_size);
        }
        // realloc(NULL, 0) returns NULL
        return NULL;
    }

    // realloc(old_ptr, 0) means to free
    if (new_size == 0) {
        myfree(old_ptr);
        return NULL;
    }

    // otherwise, actually realloc
    myfree(old_ptr);
    void *new_ptr = mymalloc(new_size);
    memcpy(new_ptr, old_ptr, new_size);
    return new_ptr;
}

/*
 */
bool validate_heap() {
    char* check = segment_start;
    while ((check < (char*)segment_end)) {
        size_t payload_size = getsize((header*)check) + sizeof(header);
        if (payload_size > (char*)segment_end - check) {
            return false;
        }
        check += payload_size;
    }
    
    return check == (char*)segment_end;
}

/* Function: dump_heap
 * -------------------
 * This function prints out the the block contents of the heap.  It is not
 * called anywhere, but is a useful helper function to call from gdb when
 * tracing through programs.  It prints out the total range of the heap, and
 * information about each block within it.
 */
void dump_heap() {
    char *iter_ptr = segment_start;
    while (iter_ptr < (char*)segment_end) {
        header *current_header = (header*)iter_ptr;
        printf("Header at %p: size = %zu, allocated = %s\n",
               (void*)current_header, getsize(current_header), isfree(current_header) ? "true" : "false");
        iter_ptr += sizeof(header) + getsize(current_header);
    }
}

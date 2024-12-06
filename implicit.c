/*
 * Young Kim
 * CS107
 * assign6: Heap Allocator
 * ------------------------
 * This implicit.c file implements an implicit heap allocator.
 * It utilizes a typedef to define a "header," which is really just
 * a size_t for clarity and readability, as well as a few helper functions.
 * The header is used to represent if the block of memory following it is
 * either allocated or free, as well as how large the block of memory is.
 * This implicit heap allocator also utilizes a first-fit (address-order) approach for malloc.
 */
#include "./allocator.h"
#include "./debug_break.h"
#include <string.h>
#include <stdio.h>

// header typecast for clarity
typedef size_t header;

// global variables
static void *segment_start;
static void *segment_end;
static size_t segment_size;
static header *first_header;

// main functions
bool myinit(void *heap_start, size_t heap_size);
void *mymalloc(size_t requested_size);
void myfree(void *ptr);
void *myrealloc(void *old_ptr, size_t new_size);
bool validate_heap();
void dump_heap();

// helper functons
size_t roundup(size_t sz, size_t mult);
bool isfree(header *h);
size_t getsize(header *h);

bool myinit(void *heap_start, size_t heap_size) {
    // heap must at least support one header and 8 bytes of data
    if (heap_size < sizeof(header) + ALIGNMENT) {
        return false;
    }

    // initialize segment/heap variables
    segment_start = heap_start;
    segment_size = heap_size - sizeof(header);
    segment_end = (char*)segment_start + heap_size;

    // initialize first header
    first_header = heap_start;
    *first_header = segment_size;
    
    return true;
}

/* Function: mymalloc
 * --------------------
 * This function allocates a size_t requested_size to the heap. 
 * This is done through iterating through the heap header by header
 * (first-fit approach) to identify and choose a header that is both
 * free and has adequate space. The function then returns a pointer to 
 * the new allocated data on the heap, or returns NULL if no such header was found.
 */
void *mymalloc(size_t requested_size) {
    // if requested_size is too big or if it's zero, return NULL
    if (requested_size > MAX_REQUEST_SIZE || requested_size == 0) {
        return NULL;
    }
    
    size_t needed = roundup(requested_size, ALIGNMENT);

    header *current_header = first_header;
    
    // iterate through all headers
    while (current_header != segment_end) {
        if (isfree(current_header) && getsize(current_header) >= needed) { // if header indicates free block and there is enough size
            // check if we can create another header after we allocate memory
            if (getsize(current_header) - needed >= sizeof(header) + ALIGNMENT) {  // header needs the space for the size of itself and at least 8 bytes
                size_t surplus = getsize(current_header);
                *current_header = needed;
                header *next = (header*)((char*)current_header + sizeof(header) + needed); // create new header
                *next = surplus - needed - sizeof(header);
            }
            
            *current_header += 1; // change header from indicating free to allocated

            return (char*)current_header + sizeof(header); // return pointer to allocated memory
        }
        current_header = (header*)((char*)current_header + sizeof(header) + getsize(current_header)); // next header
    }
    
    return NULL;
}

/* Function: myfree
 * ------------------
 * This function frees a block for the specified point in memory void *ptr,
 * adjusting the memory header to indicate that the block is now free.
 */
void myfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    
    header *newheader = (header*)((char*)ptr - sizeof(header));
    *newheader -= 1;    // mark as free
}

/* Function: myrealloc
 * ---------------------
 * This function reallocates a size_t new_size amount of memory from the 
 * location on the heap void *old_ptr. It first checks for edge cases in
 * which realloc turns into malloc or free, and then reallocs as normal
 * by utilizing memcpy with a malloc call using new_size. The function returns 
 * new_ptr, the new location in memory after the reallocation.
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

/* Function: validate_heap
 * -------------------------
 * This function iterates through the heap to check for errors,
 * making sure the heap is 'valid' throughout. It primarily
 * serves to debug any errors.
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

/* Function: roundup 
 * ------------------
 * Helper function to round up size_t sz to 
 * the nearest multiple of size_t mult.
 */
size_t roundup(size_t sz, size_t mult) {
    return (sz + mult - 1) & ~(mult - 1);
}

/* Function: isfree
 * -----------------
 * Helper function to check whether a given header is free.
 */
bool isfree(header *h) {
    return !(*h & 0x1);
}

/* Function: getsize
 * -------------------
 * Helper function to get the size that a given header represents.
 */
size_t getsize(header *h) {
    return *h & ~(0x7);
}

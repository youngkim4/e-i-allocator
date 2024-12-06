/*
 * Young Kim
 * CS107
 * assign6: Heap Allocator
 * --------------------------
 * This explicit.c file implements an explicit heap allocator.
 * It utilizes multiple complex data structures to manage the heap,
 * such as structs to represent headers and freeblocks and a doubly linked list of freeblocks
 * along with a range of helper functions to implement mymalloc, myfree, and myrealloc.
 * The allocator uses a first-fit approach to malloc, as well as LIFO (last-in-first-out) 
 * approach for the doubly linked list of freeblocks.
 * Furthermore, the explicit heap allocator supports a malloc implementation that searches
 * the linked list of freeblocks, coalescing adjacent free blocks and supports in-place realloc.
 */

#include "./allocator.h"
#include "./debug_break.h"
#include <string.h>
#include <stdio.h>

// typedefs
typedef size_t header;
typedef struct freeblock {
    header h;
    struct freeblock *prev;
    struct freeblock *next;
} freeblock;

// global variables
static void *segment_begin;
static size_t segment_size;
static void *segment_end;
static freeblock *first_freeblock;
static size_t freeblocks;

// main functions
bool myinit(void *heap_start, size_t heap_size);
void *mymalloc(size_t requested_size);
void myfree(void *ptr);
void *myrealloc(void *old_ptr, size_t new_size);
bool validate_heap();
void dump_heap();

// helper functions
size_t roundup(size_t sz, size_t mult);
bool isfree(header *h);
size_t getsize(header *h);
void coalesce(freeblock *nf, freeblock *right);
void add_freeblock_to_list(freeblock *nf);
void remove_freeblock_from_list(freeblock *nf);
void split(freeblock *nf, size_t needed);


/* Function: myinit
 * -------------------
 * This function initializes the heap and all of the global variables.
 * It checks whether the heap is big enough for a header and 16 bytes of memory,
 * and returns a boolean based on whether this condition is met.
 */
bool myinit(void *heap_start, size_t heap_size) {
    if (heap_size < sizeof(header) + (2*ALIGNMENT)) {
        return false;
    }

    // initialize segment/heap variables
    segment_begin = heap_start;
    segment_size = heap_size - sizeof(header);
    segment_end = (char*)heap_start + heap_size;

    // initialize first freeblock
    first_freeblock = heap_start;
    first_freeblock->h = segment_size;
    first_freeblock->prev = NULL;
    first_freeblock->next = NULL;
    freeblocks = 1;
    
    return true;
}

/* Function: mymalloc
 * -------------------
 * This function allocates a size_t requested size to the heap.
 * This is done through iterating the list of freeblocks until one
 * block with adequate space is found, and returns the pointer to
 * newly allocated space on the heap. If there is no such freeblock,
 * the function returns NULL.
 */
void *mymalloc(size_t requested_size) {
    // edge cases
    if (requested_size > MAX_REQUEST_SIZE || requested_size == 0) {
        return NULL;
    }

    size_t needed = requested_size <= 2*ALIGNMENT ? 2*ALIGNMENT : roundup(requested_size, ALIGNMENT);   // Only round up to the nearest multiple of 8 if requested_size > 16
    freeblock *cur_fb = first_freeblock;
    
    while (cur_fb != NULL) {
        if (getsize(&cur_fb->h) >= needed) {
            if (getsize(&cur_fb->h) - needed >= sizeof(header) + (2*ALIGNMENT)) {    // If there is enough space for another freeblock, split
                split(cur_fb, needed);
            }
            cur_fb->h += 1;    // mark as "allocated"
            remove_freeblock_from_list(cur_fb);
            return (char*)(cur_fb) + sizeof(header);
        }
        cur_fb = cur_fb->next;  // iterate
    }
    return NULL;
}

/* Function: myfree
 * ------------------
 * This function frees a block for the specified point in memory void* ptr,
 * adding the block to the linked list of freeblocks. It also makes sure to 
 * coalesce any other freeblocks to the right of the newly freed block.
 */
void myfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    freeblock *nf = (freeblock*)((char*)ptr - sizeof(header));
    nf->h -= 1;    // mark as free
    add_freeblock_to_list(nf);
    freeblock *right = (freeblock*)((char*)nf + sizeof(header) + getsize(&nf->h));    
    coalesce(nf, right);    // coalesce adjacent other freeblocks
}

/* Function: myrealloc
 * ----------------------
 * This function reallocates a size_t new_size amount of memory from the
 * location on the heap void *old_ptr. It first checks for edge cases in which
 * realloc turns into malloc or free, then tries to perform an in-place realloc
 * by checking whether or not the current location is suitable for new_size or
 * by checking if there are adjacent freeblocks to coalesce to make space for new_size.
 * If in-place realloc is not possible, then a regular realloc using memcpy takes place.
 * The function returns the location of the realloced memory.
 */
void *myrealloc(void *old_ptr, size_t new_size) {
    // edge cases:
    // if old_ptr == NULL, then realloc = malloc
    if (old_ptr == NULL) {
        return mymalloc(new_size);
    }

    // if old_ptr != NULL and new_size == 0, realloc = free
    if (new_size == 0) {
        myfree(old_ptr);
        return NULL;
    }
    
    new_size = new_size <= 2*ALIGNMENT ? 2*ALIGNMENT : roundup(new_size, ALIGNMENT);
    freeblock *nf = (freeblock*)((char*)old_ptr - sizeof(header));    // block managed by old_ptr
    size_t cur_size = getsize(&nf->h);
    
    // first, check if in-place realloc works:
    if (cur_size >= new_size) {    // if new_size is less than cur_size, we can use the existing location to realloc
        if (getsize(&nf->h) - new_size >= sizeof(header) + (2*ALIGNMENT)) {    // split if possible
            split(nf, new_size);
            nf->h += 1;    // mark as allocated
        }
        return old_ptr;
    }
    else {    // if new_size is larger than cur_size, we need to first check if we can coalesce to make space to use the existing location to realloc
        freeblock *right = (freeblock*)((char*)nf + sizeof(header) + getsize(&nf->h));
        coalesce(nf, right);
        if (getsize(&nf->h) >= new_size) {    // after coalescing, if there is now space for new_size, then we can in-place realloc
            if (getsize(&nf->h) - new_size >= sizeof(header) + (2*ALIGNMENT)) {    // split if possible
                split(nf, new_size);
                nf->h += 1;    // mark as allocated
            }
            return old_ptr;
        }
    }
    
    // if in-place does not work, regular realloc
    void* new_ptr = mymalloc(new_size);
    if (new_ptr == NULL) {
        return NULL;    // have to check this condition or else memcpy might copy into NULL
    }
    memcpy(new_ptr, old_ptr, new_size);
    myfree(old_ptr);
    return new_ptr;
}

/* Function: validate_heap
 * ------------------------
 * This function runs through a series of checks, making sure that the heap contains no 
 * bugs or other inadequacies. It is primarily used to debug errors, as the function 
 * is able to manually catch and describe these errors. By iterating through the heap 
 * both iteratively and via the doubly linked list, we are able to check if the 
 * freeblocks are working as intended, along with other checks.
 */
bool validate_heap() {
    // 1) iterate through the heap's blocks
    size_t freeblocks_iterate = 0;
    char* iter_ptr = segment_begin;
    while ((void*)iter_ptr < segment_end) {
        freeblock* cur_block = (freeblock*)iter_ptr;
        size_t payload_size = getsize(&cur_block->h) + sizeof(header);
        // if the payload_size is bigger than the rest of the heap, return false
        if (payload_size > ((char*)segment_end - iter_ptr)) {
            return false;
        }
        if (isfree(&cur_block->h)) {
            freeblocks_iterate++;
        }

        iter_ptr += payload_size;    // iterate
    }

    // check if we iterated through the entire heap
    if (iter_ptr != (char*)segment_end) {
        return false;
    }
    // check if we correctly counted all freeblocks
    if (freeblocks_iterate != freeblocks) {
        return false;
    }

    // 2) iterate through linked list of freeblockcs
    freeblock *cur_fb = first_freeblock;
    size_t freeblocks_list = 0;
    while (cur_fb != NULL) {
        if (isfree(&cur_fb->h)) {
            freeblocks_list++;
        }
        // check if every freeblock on the list is marked as free
        if (!isfree(&cur_fb->h)) {
            return false;
        }
        cur_fb= cur_fb->next; 
    }

    // check if we correctly counted all freeblocks
    if (freeblocks_list != freeblocks) {
        return false;
    }
    
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
    char *iter_ptr = segment_begin;
    while (iter_ptr < (char*)segment_end) {
        header *cur = (header*)iter_ptr;    // prepare to print headers
        printf("Header at %p: size = %zu, allocated = %s\n",
               (void*)cur, getsize(cur), isfree(cur) ? "false" : "true");
        iter_ptr += sizeof(header) + getsize(cur);    // iterate
    }
}

/* Function: roundup
 * -------------------
 * Helper function to round up size_t sz to 
 * the nearest multiple of size_t mult
 */
size_t roundup(size_t sz, size_t mult) {
    return (sz + mult - 1) & ~(mult - 1);
}

/* Function: isfree
 * ----------------
 * Helper function to check whether a given header is free
 */
bool isfree(header *h) {
    return !(*h & 0x1);
}

/* Function: getsize
 * ------------------
 * Helper function to get the size that a given header represents.
 */
size_t getsize(header *h) {
    return *h & ~(0x7);
}

/* Function: coalesce
 * --------------------
 * Helper function to coalesce adjacent freeblocks to a given freeblock (going right).
 * Runs a loop until all immediate freeblocks going right are all merged into one freeblock.
 */
void coalesce(freeblock *nf, freeblock *right) {
    while ((void*)right != segment_end && isfree(&right->h)) {    // keep checking as long as we haven't hit the end of the heap AND the current block is actually free
        size_t addedsize = getsize(&right->h);
        remove_freeblock_from_list(right);    // make sure to remove the freeblock on the right since we are merging
        nf->h += sizeof(header) + addedsize;    // add the size of the header and the data it represented
        right = (freeblock*)((char*)nf + sizeof(header) + getsize(&nf->h));    // iterate
    }
}

/* Function: add_freeblock_to_list
 * ---------------------------------
 * Helper function to add a given freeblock
 * to the doubly linked list of freeblocks.
 */
void add_freeblock_to_list (freeblock *nf) {
    nf->next = first_freeblock;
    nf->prev = NULL;

    if (first_freeblock) {    // if a first freeblock already exists, need to connect it with nf before we make nf the first freeblock
        first_freeblock->prev = nf;
    }

    first_freeblock = nf;    // LIFO approach
    freeblocks++;
}

/* Function: remove_freeblock_from_list
 * -----------------------------------------
 * Helper function to remove a given freeblock  
 * from the doubly linked list of freeblocks.
 */
void remove_freeblock_from_list (freeblock *nf) {
    if (nf->prev) {    
        (nf->prev)->next = nf->next;
    } else {    // else in this case means that nf is the first freeblock, so we need to set first_freeblock to the next freeblock in the list
        first_freeblock = nf->next;
    }
    if (nf->next) {
        (nf->next)->prev = nf->prev;
    }
    
    freeblocks--;
}

/* Function: split
 * ----------------
 * Helper function to "split" an allocated block
 * to create a new freeblock after it on the heap.
 * Makes sure to add the new freeblock to the linked list.
 */
void split(freeblock *nf, size_t needed) {
    nf->h = needed;
    freeblock *next = (freeblock*)((char*)nf + sizeof(header) + needed);
    (next->h) = getsize(nf->h) - needed - sizeof(header);
    add_freeblock_to_list(next);    // add new freeblock to list
}

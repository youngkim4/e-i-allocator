#include "./allocator.h"
#include "./debug_break.h"
#include <string.h>
#include <stdio.h>

// header struct
typedef struct header {
    size_t data;
} header;

// freeblock struct
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

// helper functions
size_t roundup(size_t sz, size_t mult);
bool isfree(header *h);
size_t getsize(header *h);
void coalesce(freeblock *nf, freeblock *right);
void add_freeblock_to_list(freeblock *nf);
void remove_freeblock_from_list(freeblock *nf);
void split(freeblock *nf, size_t needed);

// main functions
bool myinit(void *heap_start, size_t heap_size);
void *mymalloc(size_t requested_size);
void myfree(void *ptr);
void *myrealloc(void *old_ptr, size_t new_size);
bool validate_heap();
void dump_heap();

/* 
 * The myinit function initializes the heap and all the global variables,
 * fails if the heap is too small (cannot handle one header and 16 bytes of memory)
 */
bool myinit(void *heap_start, size_t heap_size) {
    if (heap_size < sizeof(header) + (2*ALIGNMENT)) {
        return false;
    }

    segment_begin = heap_start;
    segment_size = heap_size - sizeof(header);
    segment_end = (char*)heap_start + heap_size;

    first_freeblock = heap_start;
    (first_freeblock->h).data = segment_size;
    first_freeblock->prev = NULL;
    first_freeblock->next = NULL;
    freeblocks = 1;
    
    return true;
}

/*
Helper function to round up size_t sz to the 
nearest multiple of size_t mult
 */
size_t roundup(size_t sz, size_t mult) {
    return (sz + mult - 1) & ~(mult - 1);
}

// Helper function to check whether a given header is free
bool isfree(header *h) {
    return !(h->data & 0x1);
}

// Helper function to get the size that a given header represents
size_t getsize(header *h) {
    return h->data & ~(0x7);
}

// Helper function to coalesce adjacent freeblocks to a given freeblock (going right)
void coalesce(freeblock *nf, freeblock *right) {
    while ((void*)right != segment_end && isfree(&right->h)) {    // keep checking as long as we haven't hit the end of the heap AND the current block is actually free
        size_t addedsize = getsize(&right->h);
        remove_freeblock_from_list(right);    // make sure to remove the freeblock on the right since we are merging
        (nf->h).data += sizeof(header) + addedsize;    // add the size of the header and the data it represented
        right = (freeblock*)((char*)nf + sizeof(header) + getsize(&nf->h));    // iterate
    }
}

// Helper function to add a given freeblock to the doubly linked list of freeblocks
void add_freeblock_to_list (freeblock *nf) {
    if (first_freeblock) {
        first_freeblock->prev = nf;
        nf->next = first_freeblock;
        nf->prev = NULL;
    }
    first_freeblock = nf; // LIFO approach, so added freeblock goes to end of the list
    freeblocks++;
}

// Helper function to remove a given freeblock from the doubly linked list of freeblocks
void remove_freeblock_from_list (freeblock *nf) {
    if (nf->prev) {
        (nf->prev)->next = nf->next;
    }
    if (nf->next) {
        (nf->next)->prev = nf->prev;
    }

    if (nf == first_freeblock) {
        first_freeblock = nf->next;
    }
    freeblocks--;
}

// Helper function to "split" an allocated block if there is extra space for another free block
void split(freeblock *nf, size_t needed) {
    size_t surplus = getsize(&nf->h);
    (nf->h).data = needed;
    freeblock *next = (freeblock*)((char*)nf + sizeof(header) + needed);
    (next->h).data = surplus - needed - sizeof(header);
    add_freeblock_to_list(next);
}

/* 
 * The mymalloc function allocates the size_t requested size to the heap by iterating through the
 * linked list of freeblocks and allocating based on the first freeblock with enough space.
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
            (cur_fb->h).data += 1;    // mark as "allocated"
            remove_freeblock_from_list(cur_fb);
            return (char*)(cur_fb) + sizeof(header);
        }
        cur_fb = cur_fb->next;  // iterate
    }
    return NULL;
}
/*
 * The myfree function frees a block for the specified point in memory ptr.
 */
void myfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    freeblock *nf = (freeblock*)((char*)ptr - sizeof(header));
    (nf->h).data -= 1;    // mark as free
    add_freeblock_to_list(nf);
    freeblock *right = (freeblock*)((char*)nf + sizeof(header) + getsize(&nf->h));    
    coalesce(nf, right);    // coalesce adjacent other freeblocks
}

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
            (nf->h).data += 1;    // mark as allocated
        }
        return old_ptr;
    }
    else {    // if new_size is larger than cur_size, we need to first check if we can coalesce to make space to use the existing location to realloc
        freeblock *right = (freeblock*)((char*)nf + sizeof(header) + getsize(&nf->h));
        coalesce(nf, right);
        if (getsize(&nf->h) >= new_size) {    // after coalescing, if there is now space for new_size, then we can in-place realloc
            if (getsize(&nf->h) - new_size >= sizeof(header) + (2*ALIGNMENT)) {    // split if possible
                split(nf, new_size);
                (nf->h).data += 1;    // mark as allocated
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

bool validate_heap() {
    // 1) iterate through the heap's blocks
    size_t freeblocks_iterate = 0;
    char* iter_ptr = segment_begin;
    while ((void*)iter_ptr < segment_end) {
        freeblock* cur_block = (freeblock*)iter_ptr;
        size_t payload_size = getsize(&cur_block->h) + sizeof(header);

        if (payload_size > ((char*)segment_end - iter_ptr)) {
            printf("payload bigger than heap");
            return false;
        }
        if (isfree(&cur_block->h)) {
            freeblocks_iterate++;
        }

        iter_ptr += payload_size;
    }

    // check if we iterated through the entire heap
    if (iter_ptr != (char*)segment_end) {
        printf("The entire heap is not allocated for");
        return false;
    }
    // check if we correctly counted all freeblocks
    if (freeblocks_iterate != freeblocks) {
        printf("Freeblocks not linked up: heap iteration");
        return false;
    }

    // 2) iterate through linked list of freeblockcs
    freeblock *cur_fb = first_freeblock;
    size_t freeblocks_list = 0;
    while (cur_fb != NULL) {
        if (isfree(&cur_fb->h)) {
            freeblocks_list++;
        }
        if (((cur_fb->h).data & 0x1) != 1) {
            printf("Freeblock not marked as free");
            return false;
        }
        cur_fb= cur_fb->next; 
    }

    // check if we correctly counted all freeblocks
    if (freeblocks_list != freeblocks) {
        printf("Freeblocks not linked up: linked list");
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

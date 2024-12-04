#include "./allocator.h"
#include "./debug_break.h"
#include <string.h>

typedef struct header {
    size_t data;
} header;

typedef struct freeblock {
    header h;
    struct freeblock *prev;
    struct freeblock *next;
} freeblock;

static void *segment_begin;
static size_t segment_size;
static void *segment_end;
freeblock *first_freeblock;

void coalesce(freeblock *nf, freeblock *right);
void add_freeblock_to_list(freeblock *nf);
void remove_freeblock_from_list(freeblock *nf);


bool myinit(void *heap_start, size_t heap_size) {
    if (heap_size < (ALIGNMENT * 3)) {
        return false;
    }

    segment_begin = heap_start;
    segment_size = heap_size - sizeof(header);
    segment_end = (char*)heap_start + heap_size;

    first_freeblock = segment_begin;
    (first_freeblock->h).data = segment_size;
    first_freeblock->prev = NULL;
    first_freeblock->next = NULL;
    
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

void coalesce (freeblock *nf, freeblock *right) {
    while((void*)right != segment_end && isfree(&right->h)) {
        remove_freeblock_from_list(right);
        size_t addedsize = getsize(&right->h);
        (nf->h).data += sizeof(header) + addedsize;
        right = (freeblock*)((char*)right + sizeof(header) + getsize(&right->h));
    }
}

void add_freeblock_to_list (freeblock *nf) {
    first_freeblock->prev = nf;
    nf->next = first_freeblock;
    nf->prev = NULL;
    first_freeblock = nf;
}

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
}

void *mymalloc(size_t requested_size) {
    if (requested_size > MAX_REQUEST_SIZE || requested_size == 0) {
        return NULL;
    }

    size_t needed = requested_size <= 2*ALIGNMENT ? 2*ALIGNMENT : roundup(requested_size, ALIGNMENT);

    freeblock *cur_fb = first_freeblock;
    while (true) {
        if (getsize(&cur_fb->h) >= needed) {
            if (getsize(&cur_fb->h) - needed >= sizeof(header) + 2*ALIGNMENT) {
                size_t surplus = getsize(&cur_fb->h);
                (cur_fb->h).data = needed;
                freeblock *next = (freeblock*)((char*)cur_fb + sizeof(header) + needed);
                (next->h).data = surplus - needed - sizeof(header);
                add_freeblock_to_list(next);
            }
            (cur_fb->h).data += 1;

            return (char*)(cur_fb) + sizeof(header);
        }
        cur_fb = cur_fb->next;  
    }
    return NULL;
}

void myfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    freeblock *nf = (freeblock*)((char*)ptr - sizeof(header));
    (nf->h).data -= 1;

    add_freeblock_to_list(nf);

    freeblock *right = (freeblock*)((char*)nf + sizeof(header) + getsize(&nf->h));
    coalesce(nf, right);
}

void *myrealloc(void *old_ptr, size_t new_size) {
    if (old_ptr == NULL) {
        if (new_size != 0) {
            return mymalloc(new_size);
        }
        return NULL;
    }

    if (new_size == 0) {
        myfree(old_ptr);
        return NULL;
    }

    // actually realloc
    new_size = new_size <= 2*ALIGNMENT ? 2*ALIGNMENT : roundup(new_size, 2*ALIGNMENT);
    
    freeblock *nf = (freeblock*)((char*)old_ptr - sizeof(header));
    size_t cur_size = getsize(&nf->h);

    if (cur_size >= new_size) {
        if(getsize(&nf->h) - new_size >= sizeof(header) + 2*ALIGNMENT) {
            size_t surplus = getsize(&nf->h);
            (nf->h).data = new_size;
            freeblock *next = (freeblock*)((char*)nf + sizeof(header) + new_size);
            (next->h).data = surplus - new_size - sizeof(header);
            add_freeblock_to_list(next);
        }
        
        return old_ptr;
    }
    else {
        freeblock *right = (freeblock*)((char*)nf + sizeof(header) + getsize(&nf->h));
        coalesce(nf, right);
        if (getsize(&nf->h) >= new_size) {
            
        
            if (getsize(&nf->h) - new_size >= sizeof(header) + 2*ALIGNMENT) {
                size_t surplus = getsize(&nf->h);
                (nf->h).data = new_size;
                freeblock *next = (freeblock*)((char*)nf + sizeof(header) + new_size);
                (next->h).data = surplus - new_size - sizeof(header);
                add_freeblock_to_list(next);
            }
            return old_ptr;
        }
    }

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

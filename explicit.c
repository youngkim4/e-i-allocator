#include "./allocator.h"
#include "./debug_break.h"
#include <string.h>
#include <stdio.h>

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
static freeblock *first_freeblock;
static size_t freeblocks;

void coalesce(freeblock *nf, freeblock *right);
void add_freeblock_to_list(freeblock *nf);
void remove_freeblock_from_list(freeblock *nf);
void dump_heap();


bool myinit(void *heap_start, size_t heap_size) {
    if (heap_size < (ALIGNMENT * 3)) {
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

size_t roundup(size_t sz, size_t mult) {
    return (sz + mult - 1) & ~(mult - 1);
}

bool isfree(header *h) {
    return !(h->data & 0x1);
}

size_t getsize(header *h) {
    return h->data & ~(0x7);
}

void coalesce(freeblock *nf, freeblock *right) {
    while ((void*)right != segment_end && isfree(&right->h)) {
        size_t addedsize = getsize(&right->h);
        remove_freeblock_from_list(right);
        (nf->h).data += sizeof(header) + addedsize;
        right = (freeblock*)((char*)nf + sizeof(header) + getsize(&nf->h));
    }
}

void add_freeblock_to_list (freeblock *nf) {
    if (first_freeblock) {
        first_freeblock->prev = nf;
        nf->next = first_freeblock;
        nf->prev = NULL;
        first_freeblock = nf;
    }
    else {
        first_freeblock = nf;
    }

    freeblocks++;
    
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
    freeblocks--;
}

void split(freeblock *nf, size_t needed) {
    if (getsize(&nf->h) - needed >= sizeof(header) + (2*ALIGNMENT)) {
        size_t surplus = getsize(&nf->h);
        (nf->h).data = needed;
        freeblock *next = (freeblock*)((char*)nf + sizeof(header) + needed);
        (next->h).data = surplus - needed - sizeof(header);
        add_freeblock_to_list(next);
    }
}

void *mymalloc(size_t requested_size) {
    breakpoint();
    if (requested_size > MAX_REQUEST_SIZE || requested_size == 0) {
        return NULL;
    }

    size_t needed = requested_size <= 2*ALIGNMENT ? 2*ALIGNMENT : roundup(requested_size, ALIGNMENT);

    freeblock *cur_fb = first_freeblock;
    
    while (cur_fb != NULL) {
        if (getsize(&cur_fb->h) >= needed) {
            split(cur_fb, needed);
            remove_freeblock_from_list(cur_fb);
            (cur_fb->h).data += 1;
            return (char*)(cur_fb) + sizeof(header);
        }
        cur_fb = cur_fb->next;  
    }
    return NULL;
}

void myfree(void *ptr) {
    breakpoint();
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
    breakpoint();
    if (old_ptr == NULL) {
        return mymalloc(new_size);
    }

    if (new_size == 0) {
        myfree(old_ptr);
        return NULL;
    }

    // actually realloc
    new_size = new_size <= 2*ALIGNMENT ? 2*ALIGNMENT : roundup(new_size, ALIGNMENT);
    
    freeblock *nf = (freeblock*)((char*)old_ptr - sizeof(header)); 
    size_t cur_size = getsize(&nf->h); 

    if (cur_size >= new_size) {
        if (getsize(&nf->h) - new_size >= sizeof(header) + (2*ALIGNMENT)) {
            split(nf, new_size);
            (nf->h).data += 1;
            return old_ptr;
        }
        else {
            memcpy(old_ptr, old_ptr, new_size);
            return old_ptr;
        }
    }
    else {
        freeblock *right = (freeblock*)((char*)nf + sizeof(header) + getsize(&nf->h));
        coalesce(nf, right);
        if (getsize(&nf->h) >= new_size) {
            split(nf, new_size);
            (nf->h).data += 1;
            return old_ptr;
        }
    }
    // in-place does not work
    void* new_ptr = mymalloc(new_size);
    if (new_ptr == NULL) {
        
        return NULL;
    }
    memcpy(new_ptr, old_ptr, new_size);
    myfree(old_ptr);
    return new_ptr;

}

bool validate_heap() {
    /*
    char *iter_ptr = segment_begin;
    while (iter_ptr < (char*)segment_end) {
        size_t payload_size = getsize((header*)iter_ptr) + sizeof(header);
        if (payload_size > ((char*)segment_end - iter_ptr)) {
            printf("payload bigger than heap");
            return false;
        }
        iter_ptr += payload_size;
    }

    if (iter_ptr != (char*)segment_end) {
        printf("implicit test failure");
        breakpoint();
        return false;
    }
    */
    size_t nused = 0;
    size_t free_seq = 0;

    freeblock *seq_iterator = segment_begin;
    while ((void*)seq_iterator < segment_end) {
        if (isfree(&seq_iterator->h)) {
            free_seq++;
        }

        if (((seq_iterator->h).data & 0x7) > 2) {
            printf("header is misaligned, or status bit is invalid");
        }
        nused += sizeof(header) + getsize(&seq_iterator->h);

        seq_iterator = (freeblock*)((char*)segment_begin + nused);
    }

    freeblock *cur_fb = first_freeblock;
    size_t num_free = 0;
    
    while (cur_fb != NULL) {
        if (isfree(&cur_fb->h)) {
            num_free++;
        }

        if (cur_fb == cur_fb->next) {
            printf("free block counted twice\n");
            breakpoint();
            return false;
        }

        cur_fb= cur_fb->next; 
    }

    if (num_free !=  freeblocks) {
        printf("free blocks not linked up");
        breakpoint();
        return false;
    }

    if (free_seq != freeblocks) {
        printf("free blocks not linked up via heap iteration");
        breakpoint();
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
        header *cur = (header*)iter_ptr;
        printf("Header at %p: size = %zu, allocated = %s\n",
               (void*)cur, getsize(cur), isfree(cur) ? "false" : "true");
        iter_ptr += sizeof(header) + getsize(cur);
    }
}

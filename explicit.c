#include "./allocator.h"
#include "./debug_break.h"

bool myinit(void *heap_start, size_t heap_size) {
    /* TODO(you!): remove the line below and implement this!
     * This must be called by a client before making any allocation
     * requests.  The function returns true if initialization was
     * successful, or false otherwise. The myinit function can be
     * called to reset the heap to an empty state. When running
     * against a set of of test scripts, our test harness calls
     * myinit before starting each new script.
     */
    return false;
}

void *mymalloc(size_t requested_size) {
    // TODO(you!): remove the line below and implement this!
    return NULL;
}

void myfree(void *ptr) {
    // TODO(you!): implement this!
}

void *myrealloc(void *old_ptr, size_t new_size) {
    // TODO(you!): remove the line below and implement this!
    return NULL;
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
    return false;
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

File: readme.txt
Author: Young Kim
----------------------

1. For my explicit allocator, I used typedefs and typedef structs to
simplify and make the code a lot more readable and easier to work with.
Knowing that a header is 8 bytes and simply is just a number that represents both
allocation status and memory, I typecasted it as a size_t: typedef size_t header;.
Likewise, I also did this for freeblocks, as I made a struct that held a header, a next pointer
to another freeblock, and a prev pointer to another freeblock. Creating the freeblock struct
was absolutely necessary, as it also allowed me to create a linked-list of freeblocks. Another
design choice I made was to make to implement my linked list of freeblocks with a LIFO approach.
I made this choice because I believe that LIFO implementation is not only the easiest,
but it also allows a constant O(1) free operation.

    Given that I used a LIFO and first-fit approach, rather than something like an address-order approach or best-fit,
there are certain tradeoffs. For example, my LIFO approach benefits a lot in cases where there
are a lot of malloc/free calls and short-lived allocations and deallocations, so that recently freed blocks can
immediately be used again. However, a LIFO approach can fall into some suboptimal efficiency when there are a multitude of
commands such that fragmentation can occur. For example, if we have a lot of realloc calls in which in-place reallocation
cannot occur or a lot of free calls in non-sequential order on the heap, the LIFO approach will scatter free blocks across
the heap. This leads to suboptimal memory utilization. One optimization implementation that I used was in my coalesce helper function.
Instead of just coalescing the immediate free block next to it, I implemented coalesce such that all freeblocks right adjacent to the original
freeblock would be coalesced. In other words, instead of just coalescing once, I used a while loop to coalesce as much as possible. The reason
for this was to have better memory utilization and reduce the amount of adjacent freeblocks throughout a script.

2.

Explicit: 




Tell us about your quarter in CS107!
-----------------------------------




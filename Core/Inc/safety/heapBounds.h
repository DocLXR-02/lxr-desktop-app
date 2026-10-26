#ifndef LXR_HEAP_BOUNDS_H
#define LXR_HEAP_BOUNDS_H
#include <stdint.h>
/* Byte-granular sbrk semantics; malloc owns chunk alignment. No pointer arithmetic. */
static inline int lxr_heap_move(uintptr_t start, uintptr_t limit, uintptr_t *current, int increment)
{
    uintptr_t amount;
    if(start > limit || *current < start || *current > limit) return 0;
    if(increment >= 0) {
        amount = (uintptr_t)increment;
        if(amount > limit - *current) return 0;
        *current += amount;
    } else {
        amount = (uintptr_t)(-(increment + 1)) + 1u; /* also defined for INT_MIN */
        if(amount > *current - start) return 0;
        *current -= amount;
    }
    return 1;
}
#endif

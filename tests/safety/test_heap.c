#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
unsigned char test_heap[256] __attribute__((aligned(8)));
__asm__(".global test_end\n.set test_end,test_heap\n.global test_limit\n.set test_limit,test_heap+256\n");
#define _end test_end
#define _estack test_limit
#include "Core/Src/sysmem.c"
int main(void)
{
    uintptr_t base=(uintptr_t)test_heap;
    assert((uintptr_t)_sbrk(0)==base && !(base&7));
    assert((uintptr_t)_sbrk(1)==base);
    assert((uintptr_t)_sbrk(7)==base+1);
    assert((uintptr_t)_sbrk(248)==base+8);
    errno=0; assert(_sbrk(1)==(caddr_t)-1 && errno==ENOMEM);
    assert((uintptr_t)_sbrk(0)==base+256);
    assert((uintptr_t)_sbrk(-256)==base+256);
    assert((uintptr_t)_sbrk(0)==base);
    assert(_sbrk(-1)==(caddr_t)-1 && errno==ENOMEM);
    assert(_sbrk(INT_MIN)==(caddr_t)-1);
    assert(_sbrk(INT_MAX)==(caddr_t)-1);
    assert((uintptr_t)_sbrk(0)==base);
    uintptr_t current=UINTPTR_MAX-7;
    assert(!lxr_heap_move(UINTPTR_MAX-7,UINTPTR_MAX,&current,8));
    assert(current==UINTPTR_MAX-7);
    assert(lxr_heap_move(UINTPTR_MAX-7,UINTPTR_MAX,&current,7));
    assert(current==UINTPTR_MAX);
    assert(!lxr_heap_move(100,99,&current,0));
    current=99; assert(!lxr_heap_move(100,200,&current,0));
    current=201; assert(!lxr_heap_move(100,200,&current,-1));
    for(int i=-300;i<=300;i++) {
        current=128;
        int valid=lxr_heap_move(0,256,&current,i);
        assert(valid==(i>=-128 && i<=128));
        assert(current==(uintptr_t)(valid?128+i:128));
    }
    puts("PASS bounded _sbrk: ABI wrapper, exact limit, shrink, ENOMEM, alignment, INT_MIN/MAX, uintptr overflow and 601 moves");
}

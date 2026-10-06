// Dolphin OS heap API on top of the C runtime.
// The game asks the OS for one large block at startup (iMemInit) and runs its
// own allocator inside it; a few other places allocate directly from the heap.
#include <malloc.h>
#include <stdlib.h>

extern "C" {

volatile int __OSCurrHeap = -1;

// A nominal arena; only its bounds are used (iMemInit aligns and passes them back).
static unsigned char s_arena[64];

void* OSGetArenaLo() { return s_arena; }
void* OSGetArenaHi() { return s_arena + sizeof(s_arena); }
void* OSInitAlloc(void* arenaStart, void* arenaEnd, int maxHeaps) { return arenaStart; }
int OSCreateHeap(void* start, void* end) { return 0; }

int OSSetCurrentHeap(int heap)
{
    int prev = __OSCurrHeap;
    __OSCurrHeap = heap;
    return prev;
}

// Plain malloc/free so blocks can be released either way (the game frees some
// heap blocks with free()).
void* OSAllocFromHeap(int heap, unsigned int size)
{
    return malloc(size ? size : 1);
}

void OSFreeToHeap(int heap, void* ptr)
{
    free(ptr);
}

}

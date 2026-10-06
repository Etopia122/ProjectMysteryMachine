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

// GameCube DMA and the game's allocator expect 32-byte aligned blocks.
void* OSAllocFromHeap(int heap, unsigned int size)
{
    void* p = _aligned_malloc(size ? size : 1, 32);
    return p;
}

void OSFreeToHeap(int heap, void* ptr)
{
    _aligned_free(ptr);
}

}

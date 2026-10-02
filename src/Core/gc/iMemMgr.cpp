#include "xMemMgr.h"

extern "C" {
void* OSGetArenaHi();
void* OSGetArenaLo();
void* OSInitAlloc(void* arenaStart, void* arenaEnd, int maxHeaps);
int OSCreateHeap(void* start, void* end);
int OSSetCurrentHeap(int heap);
void* OSAllocFromHeap(int heap, unsigned int size);
extern volatile int __OSCurrHeap;
void exit(int status);
void free(void* ptr);
}

extern unsigned char _stack_addr[];
extern unsigned char _stack_end[];

unsigned int mem_top_alloc;
unsigned int mem_base_alloc;
int the_heap;
unsigned int hs;
unsigned int he;

// Functions are in reverse DOL order (-inline deferred emits last to first).

void iMemInit()
{
    unsigned int hi = (unsigned int)OSGetArenaHi() & ~0x1F;
    he = hi;
    hs = ((unsigned int)OSGetArenaLo() + 0x1F) & ~0x1F;
    the_heap = OSCreateHeap(OSInitAlloc((void*)hs, (void*)hi, 1), (void*)he);
    if (the_heap >= 0)
    {
        OSSetCurrentHeap(the_heap);
    }
    else
    {
        exit(-5);
    }

    unsigned int addr = (unsigned int)OSAllocFromHeap(__OSCurrHeap, 0x641000);
    mem_base_alloc = addr;
    mem_top_alloc = addr + 0x641000;
    gMemInfo.system.addr = 0;
    gMemInfo.system.size = 0x100000;
    gMemInfo.system.flags = 0x20;
    gMemInfo.stack.addr = (unsigned int)_stack_addr;
    gMemInfo.stack.size = _stack_end - _stack_addr;
    gMemInfo.stack.flags = 0x820;
    gMemInfo.DRAM.addr = addr;
    gMemInfo.DRAM.size = 0x641000;
    gMemInfo.DRAM.flags = 0x820;
    gMemInfo.SRAM.addr = 0;
    gMemInfo.SRAM.size = 0x200000;
    gMemInfo.SRAM.flags = 0x660;
}

void iMemExit()
{
    free((void*)gMemInfo.DRAM.addr);
    gMemInfo.DRAM.addr = 0;
}

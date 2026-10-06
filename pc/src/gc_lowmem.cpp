// GameCube fixed-address memory the game reads or writes directly:
//  0x80000000  OS globals page (bus/core clock at 0xF8/0xFC, ...)
//  0xCC000000  hardware registers (GX write-gather FIFO at 0xCC008000)
// The executable is large-address-aware, so these 32-bit addresses can be
// mapped as ordinary memory. Hardware register writes land in a dummy page.
#include "pc_platform.h"

#include <windows.h>

static void* map_at(uintptr_t addr, size_t size)
{
    void* p = VirtualAlloc((void*)addr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (p != (void*)addr)
    {
        pc_log("[mem] could not map GameCube address %08X (got %p)", (unsigned)addr, p);
        return NULL;
    }
    return p;
}

void pc_map_gc_memory()
{
    unsigned char* low = (unsigned char*)map_at(0x80000000u, 0x10000);
    if (low)
    {
        *(unsigned int*)(low + 0x28) = 24 * 1024 * 1024; // physical memory size
        *(unsigned int*)(low + 0xF8) = 162000000;        // bus clock
        *(unsigned int*)(low + 0xFC) = 486000000;        // CPU clock
    }
    map_at(0xCC000000u, 0x10000);
}

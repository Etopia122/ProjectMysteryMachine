/*
 * Grouped allocation: callers register pointers with sizes (pushmalloc), then
 * one radmalloc block is carved up for all of them (popmalloc). Each piece is
 * padded so consecutive pieces start in different 32-byte cache banks.
 * Compiled with SN ProDG (GCC 2.95).
 */

#include "bink/rad.h"

static void** defptrs[32];
static U32 defsizes[32];

static U32 total = 0;
static U32 count = 0;
static void*** ptrs = defptrs;
static U32* sizes = defsizes;
static U32 capacity = 32;

void pushmalloc(void* ptr, U32 bytes)
{
    if (capacity == count)
    {
        void*** np = (void***)radmalloc(capacity * 16);
        U32* ns = (U32*)(np + capacity * 2);

        memcpy(np, ptrs, capacity * 4);
        memcpy(ns, sizes, capacity * 4);
        capacity *= 2;
        if (capacity != 64)
            radfree(ptrs);
        ptrs = np;
        sizes = ns;
    }

    bytes = (bytes + 31) & ~31;
    bytes += ((((total >> 5) & 31) - ((bytes >> 5) & 31) + 33) & 31) << 5;
    total += bytes;
    sizes[count] = bytes;
    ptrs[count++] = (void**)ptr;
}

U32 popmalloctotal(void)
{
    return total;
}

void* popmalloc(U32 bytes)
{
    U8* mem;
    U8* p;
    U32 i;

    bytes = (bytes + 31) & ~31;
    mem = (U8*)radmalloc(total + bytes);
    total = 0;
    if (mem)
    {
        p = mem + bytes;
        for (i = 0; i < count; i++)
        {
            *ptrs[i] = p;
            p += sizes[i];
        }
    }
    count = 0;
    return mem;
}

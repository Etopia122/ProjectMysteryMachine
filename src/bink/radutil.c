/*
 * RAD Game Tools utility routines (memory hooks, timers, 64-bit math) from
 * the Bink SDK. Compiled with SN ProDG (GCC 2.95), not MWCC.
 */

#include "bink/rad.h"

static U64 radtimerstart = 0;
static RADMEMALLOC usermalloc = 0;
static RADMEMFREE userfree = 0;
static RADMEMALLOC audiomalloc = 0;
static RADMEMFREE audiofree = 0;

/*
 * GCC emits the out-of-line copies of these inline functions at the end of
 * the file, which is why div64 and ReadTimeBase come last.
 */
/* TODO: same instructions as the original, but hi/q/inv get other registers. */
inline U32 div64(U32 hi, U32 lo, U32 d)
{
    U32 q, inv, top;

    if ((d & (d - 1)) == 0)
    {
        U32 z = radcntlzw(d);
        return (lo >> (31 - z)) | (hi << (z + 1));
    }
    inv = 0xFFFFFFFF / d;
    top = (d + 0xFFFF) >> 16;
    q = 0;
    if (top)
    {
        U32 z = radcntlzw(hi);
        S32 s = 16 - z;
        S32 m = s >> 31;
        q = (((hi << z) / top) >> (-s & m)) << (s & ~m);
        radsubmul64(hi, lo, q, d);
    }
    while (hi)
    {
        U32 t = inv * hi;
        radsubmul64(hi, lo, t, d);
        q += t;
    }
    return q + lo / d;
}

inline void ReadTimeBase(U64* t)
{
    RAD_READ_TIMEBASE(*t);
}

U32 mult64anddiv(U32 a, U32 b, U32 c)
{
    U32 hi, lo;
    radmul64(hi, lo, a, b);
    return div64(hi, lo, c);
}

U32 mult64andshift(U32 a, U32 b, U32 shift)
{
    U64 r = (U64)a * (U64)b;
    return ((U32)r >> shift) | ((U32)(r >> 32) << (32 - shift));
}

void radmemset16(void* dest, U32 value, U32 count)
{
    U32 v = (value << 16) | value;
    U32 n = count >> 1;

    while (n--)
    {
        *(U32*)dest = v;
        dest = (U8*)dest + 4;
    }
    if (count & 1)
        *(U16*)dest = (U16)value;
}

/* TODO: RADTimerRead (div64 inlined with the 40500 ticks/ms divisor) */

void RADCycleTimerStartAddr(U32* addr)
{
    U64 t;
    ReadTimeBase(&t);
    *addr = (U32)t;
}

void RADCycleTimerDeltaAddr(U32* addr)
{
    U64 t;
    ReadTimeBase(&t);
    *addr = (U32)t - *addr;
}

void RADCycleTimerStartAddr64(U64* addr)
{
    ReadTimeBase(addr);
}

void RADCycleTimerDeltaAddr64(U64* addr)
{
    U64 t;
    ReadTimeBase(&t);
    *addr = t - *addr;
}

void RADSetMemory(RADMEMALLOC a, RADMEMFREE f)
{
    usermalloc = a;
    userfree = f;
}

void* radmalloc(U32 bytes)
{
    U8* mem;
    U8 type;
    U32 off;
    U32 size;

    if (bytes == 0 || bytes == 0xFFFFFFFF)
        return 0;

    size = bytes + 64;
    if (usermalloc)
    {
        mem = (U8*)usermalloc(size);
        if (mem)
        {
            if (mem == (U8*)0xFFFFFFFF)
                return 0;
            type = 3;
            goto aligned;
        }
    }

    mem = (U8*)OSAllocFromHeap(__OSCurrHeap, size);
    if (mem == 0)
        return 0;
    type = 0;

aligned:
    off = (U8)(64 - ((U32)mem & 31));
    mem += off;
    mem[-1] = (U8)off;
    mem[-2] = type;
    if (type == 3)
        ((RADMEMFREE*)mem)[-2] = userfree;
    return mem;
}

void radfree(void* ptr)
{
    U8* mem = (U8*)ptr;

    if (mem == 0)
        return;

    if (mem[-2] == 3)
        ((RADMEMFREE*)mem)[-2](mem - mem[-1]);
    else
        OSFreeToHeap(__OSCurrHeap, mem - mem[-1]);
}

void RADSetAudioMemory(RADMEMALLOC a, RADMEMFREE f)
{
    audiomalloc = a;
    audiofree = f;
}

void* radaudiomalloc(U32 bytes)
{
    void* p = 0;
    if (audiomalloc)
        p = audiomalloc(bytes);
    return p;
}

void radaudiofree(void* ptr)
{
    if (audiofree)
        audiofree(ptr);
}


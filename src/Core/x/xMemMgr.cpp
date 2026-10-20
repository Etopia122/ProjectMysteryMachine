#include "xMemMgr.h"

extern "C" {
void* malloc(unsigned int size);
void free(void* ptr);
void* memset(void* dst, int c, unsigned int n);
}

void iMemInit();
void iMemExit();
void xDebugModeRegisterCallback(signed int mode, void (*func)());

xMemInfo_tag gMemInfo;
xMemHeap_tag gxHeap[3];
unsigned int gActiveHeap;
static void (*sMemBaseNotifyFunc)();

#pragma auto_inline on
static void xMemInitHeap(xMemHeap_tag* heap, unsigned int base, unsigned int size,
                         unsigned int flags)
{
    unsigned int old_base = base;
    unsigned int align = 1 << ((flags >> 9) & 0x1F);
    if (flags & 0x100)
    {
        base = base & -align;
        size -= (old_base - base);
    }
    else
    {
        base = (old_base + align - 1) & -align;
        size -= (base - old_base);
    }

    heap->state_idx = 0;
    heap->hard_base = base;
    heap->size = size;
    heap->flags = flags;
    heap->lastblk = 0;
    xHeapState_tag* sp = &heap->state[heap->state_idx];
    sp->curr = base;
    sp->blk_ct = 0;

    if (flags & 0x10000)
    {
        heap->max_blks = 1 << ((flags >> 14) & 0x1);
        heap->blk = (xMemBlock_tag*)malloc(heap->max_blks * sizeof(xMemBlock_tag));
        memset(heap->blk, 0, heap->max_blks * sizeof(xMemBlock_tag));
    }
    else
    {
        heap->max_blks = -1;
        heap->blk = 0;
    }
    heap->opp_heap[0] = -1;
    heap->opp_heap[1] = -1;
}

static unsigned int xMemGetBlockInfo(xMemHeap_tag* heap, unsigned int size, xMemBlkInfo_tag* info)
{
    xHeapState_tag* sp = &heap->state[heap->state_idx];
    unsigned int align = 1 << ((heap->flags >> 9) & 0x1F);
    signed int dir = (heap->flags & 0x100) ? -1 : 1;
    signed int hdrsize = sizeof(xMemBlock_tag);
    hdrsize &= -((heap->flags >> 15) & 1);
    signed int pad = (align - 1) & -align;
    signed int hdr = 0;
    signed int pre = ((align + hdrsize - 1) & -align) + pad;
    signed int block = pre;
    signed int post = block + ((size + align - 1) & -align);
    unsigned int total = post + (align > pad ? align : pad);

    if (heap->flags & 0x100)
    {
        hdr = -hdrsize;
        block = pad - total;
    }

    if (heap->flags & 0x10000)
    {
        info->header = &heap->blk[sp->blk_ct];
    }
    else
    {
        info->header = (xMemBlock_tag*)(sp->curr + hdr);
    }

    info->pre = sp->curr + pre;
    info->block = sp->curr + block;
    info->post = sp->curr + post;
    info->curr = sp->curr + dir * total;
    info->waste = total - (hdrsize + size);
    info->total = total;

    return total;
}

static void xMemPoolAddElements(xMemPool* pool, void* buffer, unsigned int count)
{
    signed int i;
    void* curr;
    void (*initCB)(xMemPool*, void*);
    unsigned int next;
    unsigned int size;

    initCB = pool->InitCB;
    next = pool->NextOffset;
    size = pool->Size;

    curr = buffer;
    for (i = 0; i < (signed int)count - 1; i++)
    {
        *(void**)((unsigned int)curr + next) = (void*)((unsigned int)curr + size);
        if (initCB != 0)
        {
            initCB(pool, curr);
        }
        curr = (void*)((unsigned int)curr + size);
    }

    *(void**)((unsigned int)curr + next) = pool->FreeList;
    if (initCB != 0)
    {
        initCB(pool, curr);
    }
    pool->FreeList = buffer;
    pool->Total += count;
}
#pragma auto_inline off

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xMemDebug_SoakLog(const char*)
{
}

void xMemInit()
{
    iMemInit();
    xMemInitHeap(&gxHeap[0], gMemInfo.DRAM.addr, gMemInfo.DRAM.size, 0x8826);
    xMemInitHeap(&gxHeap[1], gMemInfo.DRAM.addr + gMemInfo.DRAM.size, gMemInfo.DRAM.size, 0x8925);
    xMemInitHeap(&gxHeap[2], gMemInfo.DRAM.addr + gMemInfo.DRAM.size, gMemInfo.DRAM.size, 0x892A);
    gxHeap[0].opp_heap[0] = 1;
    gxHeap[0].opp_heap[1] = 2;
    gxHeap[1].opp_heap[0] = 0;
    gxHeap[1].opp_heap[1] = 2;
    gxHeap[2].opp_heap[0] = 0;
    gxHeap[2].opp_heap[1] = 1;
    gActiveHeap = 0;
    xDebugModeRegisterCallback(0xB, (void (*)())xMemDebug_SoakLog);
}

void xMemExit()
{
    iMemExit();
}

// TODO (87.5%): same instructions, different register assignment (the
// original gives heap r12, hdr r6, sp r7).
void* xMemGrowAlloc(unsigned int heapID, unsigned int size)
{
    unsigned int align;
    unsigned int oldalignsize;
    xMemHeap_tag* heap;
    xMemBlock_tag* hdr;
    xHeapState_tag* sp;
    void* memptr;

    heap = &gxHeap[heapID];
    sp = &heap->state[heap->state_idx];
    hdr = heap->lastblk;
    align = 1 << ((heap->flags >> 9) & 0x1F);
    oldalignsize = hdr->size;

    align = ((oldalignsize + size + align - 1) & -align) - ((oldalignsize + align - 1) & -align);

    if (sp->used + align > heap->size)
    {
        return 0;
    }

    if (heap->flags & 0x100)
    {
        memptr = (void*)(hdr->addr - size);
        hdr->addr = (unsigned int)memptr;
        sp->curr -= align;
    }
    else
    {
        sp->curr += align;
        memptr = (void*)(hdr->addr + oldalignsize);
    }

    sp->used += align;
    hdr->size += size;
    memset(memptr, 0, size);
    return memptr;
}

// TODO (90%): the original keeps flags in r11 and saves only r30/r31; here
// flags lands in a callee-saved register. The empty two-iteration loop is in
// the original too.
void* xMemAlloc(unsigned int heapID, unsigned int size)
{
    unsigned int total;
    xMemBlkInfo_tag info;
    xHeapState_tag* sp;
    xMemBlock_tag* hdr;
    xMemHeap_tag* heap;

    heap = &gxHeap[heapID];
    sp = &heap->state[heap->state_idx];

    if (size == 0)
    {
        return (void*)0xDEADBEEF;
    }

    total = xMemGetBlockInfo(heap, size, &info);
    hdr = info.header;

    if (sp->used + total > heap->size)
    {
        return 0;
    }

    for (signed int i = 0; i < 2; i++)
    {
    }

    sp->curr = info.curr;
    sp->blk_ct++;
    if (heap->flags & 0x10000)
    {
        return 0;
    }

    hdr->addr = info.block;
    hdr->size = size;
    sp->used += total;
    sp->wasted += info.waste;
    memset((void*)hdr->addr, 0, size);
    heap->lastblk = hdr;
    return (void*)hdr->addr;
}

void* xMemPushTemp(unsigned int size)
{
    return malloc(size);
}

void xMemPopTemp(void* addr)
{
    free(addr);
}

signed int xMemPushBase()
{
    xMemHeap_tag* heap = &gxHeap[gActiveHeap];

    heap->state_idx++;
    xHeapState_tag* sp = &heap->state[heap->state_idx];
    *sp = *(sp - 1);

    if (sMemBaseNotifyFunc)
    {
        sMemBaseNotifyFunc();
    }
    return heap->state_idx - 1;
}

signed int xMemPopBase(signed int depth)
{
    xMemHeap_tag* heap = &gxHeap[gActiveHeap];
    if (depth < 0)
    {
        depth = heap->state_idx + depth;
    }

    heap->state_idx = depth;
    if (sMemBaseNotifyFunc)
    {
        sMemBaseNotifyFunc();
    }
    return heap->state_idx;
}

void xMemRegisterBaseNotifyFunc(void (*func)())
{
    sMemBaseNotifyFunc = func;
}

unsigned int xMemGetBase()
{
    return gxHeap[gActiveHeap].state_idx;
}

void xMemPoolSetup(xMemPool* pool, void* buffer, unsigned int nextOffset, unsigned int flags,
                   void (*initCB)(xMemPool*, void*), unsigned int size, unsigned int count,
                   unsigned int numRealloc)
{
    pool->FreeList = 0;
    pool->NextOffset = nextOffset;
    pool->Flags = flags;
    pool->UsedList = 0;
    pool->InitCB = initCB;
    pool->Buffer = buffer;
    pool->Size = size;
    pool->NumRealloc = numRealloc;
    pool->Total = 0;
    xMemPoolAddElements(pool, buffer, count);
}

void* xMemPoolAlloc(xMemPool* pool)
{
    void* retval = pool->FreeList;
    unsigned int next = pool->NextOffset;
    unsigned int flags = pool->Flags;

    if (retval == 0)
    {
        xMemPoolAddElements(pool, xMemAlloc(gActiveHeap, pool->NumRealloc * pool->Size),
                            pool->NumRealloc);
        retval = pool->FreeList;
    }

    pool->FreeList = *(void**)((unsigned int)retval + next);
    if (flags & 1)
    {
        *(void**)((unsigned int)retval + next) = pool->UsedList;
        pool->UsedList = retval;
    }

    return retval;
}

void xMemPoolFree(xMemPool* pool, void* data)
{
    void** prev;
    void* freeList;
    unsigned int next;
    void* curr;

    if (data == 0)
    {
        return;
    }

    freeList = pool->FreeList;
    next = pool->NextOffset;
    if (pool->Flags & 1)
    {
        prev = &pool->UsedList;
        curr = pool->UsedList;

        while (curr != 0 && curr != data)
        {
            prev = (void**)((unsigned int)curr + next);
            curr = *prev;
        }

        if (curr != 0)
        {
            *prev = *(void**)((unsigned int)curr + next);
        }
    }

    *(void**)((unsigned int)data + next) = freeList;
    pool->FreeList = data;
}

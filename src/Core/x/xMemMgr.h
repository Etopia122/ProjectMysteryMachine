#ifndef XMEMMGR_H
#define XMEMMGR_H

class xMemArea_tag
{
public:
    unsigned int addr;
    unsigned int size;
    unsigned int flags;
};

class xMemInfo_tag
{
public:
    xMemArea_tag system; // 0x0
    xMemArea_tag stack;  // 0xC
    xMemArea_tag DRAM;   // 0x18
    xMemArea_tag SRAM;   // 0x24
};

class xMemBlock_tag
{
public:
    unsigned int addr;
    unsigned int size;
};

class xHeapState_tag
{
public:
    unsigned int curr;
    unsigned short blk_ct;
    unsigned short pad;
    unsigned int used;
    unsigned int wasted;
};

class xMemHeap_tag
{
public:
    unsigned int flags;           // 0x0
    unsigned int hard_base;       // 0x4
    unsigned int size;            // 0x8
    signed short opp_heap[2];     // 0xC
    xHeapState_tag state[8];      // 0x10
    unsigned short state_idx;     // 0x90
    unsigned short max_blks;      // 0x92
    xMemBlock_tag* blk;           // 0x94
    xMemBlock_tag* lastblk;       // 0x98
};

class xMemBlkInfo_tag
{
public:
    xMemBlock_tag* header;
    unsigned int pre;
    unsigned int block;
    unsigned int post;
    unsigned int curr;
    unsigned int waste;
    unsigned int total;
};

class xMemPool
{
public:
    void* FreeList;                       // 0x0
    unsigned short NextOffset;            // 0x4
    unsigned short Flags;                 // 0x6
    void* UsedList;                       // 0x8
    void (*InitCB)(xMemPool*, void*);     // 0xC
    void* Buffer;                         // 0x10
    unsigned short Size;                  // 0x14
    unsigned short NumRealloc;            // 0x16
    unsigned int Total;                   // 0x18
};

extern xMemInfo_tag gMemInfo;
extern xMemHeap_tag gxHeap[3];
extern unsigned int gActiveHeap;

void xMemInit();
void xMemExit();
void* xMemGrowAlloc(unsigned int heapID, unsigned int size);
void* xMemAlloc(unsigned int heapID, unsigned int size);
void* xMemPushTemp(unsigned int size);
void xMemPopTemp(void* addr);
signed int xMemPushBase();
signed int xMemPopBase(signed int depth);
void xMemRegisterBaseNotifyFunc(void (*func)());
unsigned int xMemGetBase();
void xMemPoolSetup(xMemPool* pool, void* buffer, unsigned int nextOffset, unsigned int flags,
                   void (*initCB)(xMemPool*, void*), unsigned int size, unsigned int count,
                   unsigned int numRealloc);
void* xMemPoolAlloc(xMemPool* pool);
void xMemPoolFree(xMemPool* pool, void* data);

#endif

/*
 * Memory: the default allocator hooks and free lists, block allocators for
 * fixed-size entries with a stack of free entry pointers.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 1 /* core */

void* malloc(RwUInt32 size);
void free(void* ptr);
void* realloc(void* ptr, RwUInt32 size);
void* memset(void* dst, int c, RwUInt32 n);

static RwLinkList freeListList;

static void* MemoryCalloc(RwUInt32 numObj, RwUInt32 sizeObj) {
    RwUInt32 size = numObj * sizeObj;
    void* mem = malloc(size);

    if (mem) {
        memset(mem, 0, size);
    }

    return mem;
}

RwBool RwFreeListDestroy(RwFreeList* freelist) {
    void* block;

    if (freelist->freeIndices) {
        RwFree(freelist->freeIndices);
    }

    block = freelist->firstBlock;
    while (block) {
        void* next = *(void**)block;

        RwFree(block);
        block = next;
    }

    rwLLLinkRemove(&freelist->link);
    RwFree(freelist);

    return TRUE;
}

static inline RwFreeList* FreeListInitialize(RwFreeList* freelist, RwInt32 entrySize, RwInt32 entriesPerBlock,
                                             RwInt32 alignment) {
    RwUInt32 size;

    freelist->alignment = ((alignment > 1) ? alignment : 1) - 1;
    size = (entrySize + freelist->alignment) & ~freelist->alignment;
    freelist->entrySize = size;
    freelist->entriesPerBlock = entriesPerBlock;
    freelist->blockSize = size * entriesPerBlock + sizeof(void*);
    freelist->blockSize += freelist->alignment;

    freelist->freeIndices = (void**)RwMalloc(sizeof(void*));
    if (freelist->freeIndices) {
        freelist->freeIndexTop = freelist->freeIndices - 1;
        freelist->totalEntries = 0;
        freelist->firstBlock = NULL;

        rwLinkListAddLLLink(&freeListList, &freelist->link);
    } else {
        RWERROR((E_RW_NOMEM, sizeof(void*)));
        RwFree(freelist);
        freelist = NULL;
    }

    return freelist;
}

RwFreeList* RwFreeListCreate(RwInt32 entrySize, RwInt32 entriesPerBlock, RwInt32 alignment) {
    RwFreeList* freelist = (RwFreeList*)RwMalloc(sizeof(RwFreeList));

    if (freelist) {
        freelist = FreeListInitialize(freelist, entrySize, entriesPerBlock, alignment);
    } else {
        RWERROR((E_RW_NOMEM, sizeof(RwFreeList)));
        freelist = NULL;
    }

    return freelist;
}

RwFreeList* _rwFreeListFreeReal(RwFreeList* freelist, void* pData) {
    *++freelist->freeIndexTop = pData;

    return freelist;
}

void* _rwFreeListAllocReal(RwFreeList* freelist) {
    if (freelist->freeIndexTop < freelist->freeIndices) {
        void* block;
        void** indices;
        RwUInt32 size;

        block = RwMalloc(freelist->blockSize);
        if (!block) {
            RWERROR((E_RW_NOMEM, freelist->blockSize));
            return NULL;
        }

        size = (freelist->totalEntries + freelist->entriesPerBlock) * sizeof(void*);
        indices = (void**)RwMalloc(size);
        if (!indices) {
            RWERROR((E_RW_NOMEM, size));
            RwFree(block);
            return NULL;
        }

        RwFree(freelist->freeIndices);
        freelist->freeIndices = indices;
        freelist->totalEntries += freelist->entriesPerBlock;

        *(void**)block = freelist->firstBlock;
        freelist->firstBlock = block;

        {
            RwInt32 count = freelist->entriesPerBlock;
            RwUInt8* entry = (RwUInt8*)(((RwUInt32)block + (freelist->blockSize - freelist->entrySize)) &
                                        ~freelist->alignment);

            while (count--) {
                *indices++ = entry;
                entry -= freelist->entrySize;
            }
        }

        freelist->freeIndexTop = freelist->freeIndices + (freelist->entriesPerBlock - 1);
    }

    return *freelist->freeIndexTop--;
}

RwFreeList* RwFreeListForAllUsed(RwFreeList* freelist, RwFreeListCallBack fpCallBack, void* pData) {
    RwUInt8* entry;
    void* block = freelist->firstBlock;

    while (block) {
        RwInt32 count = freelist->entriesPerBlock;

        entry = (RwUInt8*)((RwUInt32)block + sizeof(void*) + freelist->alignment);
        entry = (RwUInt8*)((RwUInt32)entry & ~freelist->alignment);
        while (count--) {
            void** top = freelist->freeIndexTop;
            void** index = freelist->freeIndices;

            for (; index <= top; index++) {
                if (*index == entry) {
                    break;
                }
            }

            if (index > top) {
                fpCallBack(entry, pData);
            }

            entry += freelist->entrySize;
        }

        block = *(void**)block;
    }

    return freelist;
}

void _rwMemoryClose(void) {
}

RwBool _rwMemoryOpen(const RwMemoryFunctions* memFuncs) {
    rwLinkListInitialize(&freeListList);

    if (memFuncs) {
        RWSRCGLOBAL(memoryFuncs) = *memFuncs;
    } else {
        RWSRCGLOBAL(memoryFuncs).rwmalloc = malloc;
        RWSRCGLOBAL(memoryFuncs).rwfree = free;
        RWSRCGLOBAL(memoryFuncs).rwrealloc = realloc;
        RWSRCGLOBAL(memoryFuncs).rwcalloc = MemoryCalloc;
    }

    return TRUE;
}

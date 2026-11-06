/*
 * PowerPipe heap: a simple block allocator over a chain of super blocks,
 * with a table of free blocks used for first-fit allocation.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 1 /* core */

#define rxHEAPMINSIZE 0x400
#define rxHEAPSUPERBLOCKMINSIZE 0x80
#define rxHEAPFREEBLOCKSGROWBY 32
#define rxHEAPSPLITTHRESHOLD 0x100
#define rxHEAPALIGNMENT 32

typedef struct rxHeapFreeBlock rxHeapFreeBlock;
typedef struct rxHeapBlockHeader rxHeapBlockHeader;
typedef struct rxHeapSuperBlockDescriptor rxHeapSuperBlockDescriptor;

struct rxHeapFreeBlock {
    RwUInt32 size;
    rxHeapBlockHeader* ptr;
};

struct rxHeapBlockHeader {
    /* 0x00 */ rxHeapBlockHeader* prev;
    /* 0x04 */ rxHeapBlockHeader* next;
    /* 0x08 */ RwUInt32 size;
    /* 0x0C */ rxHeapFreeBlock* freeEntry;
    /* 0x10 */ RwUInt32 pad[4];
};

struct rxHeapSuperBlockDescriptor {
    void* start;
    RwUInt32 size;
    rxHeapSuperBlockDescriptor* next;
};

typedef struct RxHeap {
    /* 0x00 */ RwUInt32 superBlockSize;
    /* 0x04 */ rxHeapSuperBlockDescriptor* head;
    /* 0x08 */ rxHeapBlockHeader* headBlock;
    /* 0x0C */ rxHeapFreeBlock* freeBlocks;
    /* 0x10 */ RwUInt32 entriesAlloced;
    /* 0x14 */ RwUInt32 entriesUsed;
    /* 0x18 */ RwBool dirty;
} RxHeap;

#define RwRealloc(_p, _s) ((RWSRCGLOBAL(memoryFuncs).rwrealloc)((_p), (_s)))
#define rwsprintf (RWSRCGLOBAL(stringFuncs).vecSprintf)

#define RxHeapReset(heap) (!(heap)->dirty ? TRUE : _rxHeapReset(heap))

void* memcpy(void* dst, const void* src, unsigned long n);
RwBool _rxHeapReset(RxHeap* heap);
void RxHeapFree(RxHeap* heap, void* block);
void* RxHeapAlloc(RxHeap* heap, RwUInt32 size);

static inline rxHeapSuperBlockDescriptor* HeapSuperBlockCreate(RwUInt32 size) {
    rxHeapSuperBlockDescriptor* superBlock =
        (rxHeapSuperBlockDescriptor*)RwMalloc(size + sizeof(rxHeapSuperBlockDescriptor) + 127);

    if (superBlock) {
        superBlock->start = (void*)(((RwUInt32)superBlock + sizeof(rxHeapSuperBlockDescriptor) + 127) & ~127);
        superBlock->size = size;
        superBlock->next = NULL;
    }

    return superBlock;
}

static inline rxHeapFreeBlock* HeapFreeBlocksAddEntry(RxHeap* heap) {
    rxHeapFreeBlock* freeBlocks;
    RwUInt32 entriesUsed;

    entriesUsed = heap->entriesUsed;
    freeBlocks = heap->freeBlocks;

    if (heap->entriesAlloced <= entriesUsed) {
        freeBlocks = (rxHeapFreeBlock*)RwRealloc(freeBlocks, (heap->entriesAlloced += rxHEAPFREEBLOCKSGROWBY) * sizeof(rxHeapFreeBlock));
        if (!freeBlocks) {
            RWERROR((E_RW_NOMEM, heap->entriesAlloced * sizeof(rxHeapFreeBlock)));
            heap->entriesAlloced -= rxHEAPFREEBLOCKSGROWBY;
        } else {
            if (freeBlocks != heap->freeBlocks && entriesUsed) {
                rxHeapFreeBlock* entry = freeBlocks;

                do {
                    entry->ptr->freeEntry = entry;
                    entry++;
                } while (--entriesUsed);
            }

            heap->freeBlocks = freeBlocks;
        }
    }

    if (freeBlocks) {
        freeBlocks = &freeBlocks[heap->entriesUsed++];
    }

    return freeBlocks;
}

static inline void HeapFreeBlocksRemoveEntry(RxHeap* heap, rxHeapFreeBlock* entry) {
    if (&heap->freeBlocks[heap->entriesUsed - 1] != entry) {
        *entry = heap->freeBlocks[heap->entriesUsed - 1];
        entry->ptr->freeEntry = entry;
    }

    heap->entriesUsed--;
}

static inline RwBool HeapSuperBlockInit(RxHeap* heap, rxHeapSuperBlockDescriptor* superBlock,
                                        rxHeapSuperBlockDescriptor* prevSuperBlock) {
    rxHeapFreeBlock* freeEntry = HeapFreeBlocksAddEntry(heap);

    if (freeEntry) {
        rxHeapBlockHeader* first = (rxHeapBlockHeader*)superBlock->start;
        rxHeapBlockHeader* block = first + 1;
        rxHeapBlockHeader* last = (rxHeapBlockHeader*)((RwUInt8*)first + superBlock->size - sizeof(rxHeapBlockHeader));

        first->prev = NULL;
        first->next = NULL;
        first->size = 0;
        first->freeEntry = NULL;
        *last = *first;

        first->next = block;
        block->prev = first;
        block->next = last;
        last->prev = block;
        block->size = (RwUInt8*)last - (RwUInt8*)(block + 1);
        block->freeEntry = freeEntry;
        freeEntry->ptr = block;
        freeEntry->size = block->size;

        if (prevSuperBlock) {
            rxHeapBlockHeader* prevLast = (rxHeapBlockHeader*)((RwUInt8*)prevSuperBlock->start + prevSuperBlock->size -
                                                               sizeof(rxHeapBlockHeader));

            prevLast->next = first;
            first->prev = prevLast;
        }

        return TRUE;
    }

    return FALSE;
}

static inline void* HeapAllocFrom(RxHeap* heap, rxHeapFreeBlock* freeEntry, RwUInt32 size) {
    RwUInt32 remaining = freeEntry->size - size;

    if (remaining < rxHEAPSPLITTHRESHOLD) {
        rxHeapBlockHeader* block = freeEntry->ptr;

        HeapFreeBlocksRemoveEntry(heap, freeEntry);
        block->freeEntry = NULL;
        return block + 1;
    } else {
        rxHeapBlockHeader* block = freeEntry->ptr;
        rxHeapBlockHeader* newBlock = (rxHeapBlockHeader*)((RwUInt8*)(block + 1) + size);

        newBlock->size = remaining - sizeof(rxHeapBlockHeader);
        newBlock->prev = block;
        newBlock->next = block->next;
        newBlock->freeEntry = freeEntry;
        block->size = size;
        block->next = newBlock;

        if (newBlock->next) {
            newBlock->next->prev = newBlock;
        }

        freeEntry->ptr = newBlock;
        freeEntry->size = newBlock->size;
        block->freeEntry = NULL;
        return block + 1;
    }
}

rxHeapBlockHeader* HeapGrow(RxHeap* heap, RwUInt32 size) {
    rxHeapSuperBlockDescriptor* superBlock;
    RwUInt32 superBlockSize = size + sizeof(rxHeapBlockHeader) * 3;

    if (superBlockSize < heap->superBlockSize) {
        superBlockSize = heap->superBlockSize;
    }

    superBlock = HeapSuperBlockCreate(superBlockSize);
    if (superBlock) {
        if (HeapSuperBlockInit(heap, superBlock, heap->head)) {
            rxHeapSuperBlockDescriptor* cur;
            RwUInt32 totalSize;
            RwChar buffer[256];

            superBlock->next = heap->head;
            heap->head = superBlock;

            totalSize = 0;
            for (cur = heap->head; cur; cur = cur->next) {
                totalSize += cur->size;
            }

            rwsprintf(buffer, "Heap resized from %d to %d bytes", totalSize - superBlockSize, totalSize);

            return (rxHeapBlockHeader*)superBlock->start + 1;
        }

        if (superBlock) {
            RwFree(superBlock);
        }
    }

    return NULL;
}

void* HeapReallocGrow(RxHeap* heap, void* block, RwUInt32 newSize, rxHeapBlockHeader* header, RwInt32 delta) {
    rxHeapBlockHeader* next = header->next;

    if (next && next->freeEntry && next->size + sizeof(rxHeapBlockHeader) >= delta) {
        RwUInt32 total = header->size + (next->size + sizeof(rxHeapBlockHeader));

        if (total - newSize < rxHEAPSPLITTHRESHOLD) {
            HeapFreeBlocksRemoveEntry(heap, next->freeEntry);
            header->size = total;
            header->next = header->next->next;

            if (header->next) {
                header->next->prev = header;
            }
        } else {
            rxHeapBlockHeader* newBlock = (rxHeapBlockHeader*)((RwUInt8*)(header + 1) + newSize);

            newBlock->freeEntry = next->freeEntry;
            newBlock->size = header->next->size - delta;
            newBlock->prev = header;
            newBlock->next = header->next->next;

            if (newBlock->next) {
                newBlock->next->prev = newBlock;
            }

            header->size = newSize;
            header->next = newBlock;
            newBlock->freeEntry->ptr = newBlock;
            newBlock->freeEntry->size = newBlock->size;
        }

        return block;
    }

    {
        void* newBlock = RxHeapAlloc(heap, newSize);

        if (newBlock) {
            memcpy(newBlock, block, header->size);
            RxHeapFree(heap, block);
            return newBlock;
        }
    }

    RWERROR((E_RW_NOMEM, newSize));
    return NULL;
}

void RxHeapFree(RxHeap* heap, void* block) {
    rxHeapBlockHeader* header = (rxHeapBlockHeader*)block - 1;
    RwBool prevFree = (header->prev && header->prev->freeEntry);
    RwBool nextFree = (header->next && header->next->freeEntry);

    if (prevFree) {
        if (nextFree) {
            HeapFreeBlocksRemoveEntry(heap, header->next->freeEntry);
            header->prev->size += header->size + header->next->size + 2 * sizeof(rxHeapBlockHeader);
            header->prev->freeEntry->size = header->prev->size;
            header->prev->next = header->next->next;

            if (header->next->next) {
                header->next->next->prev = header->prev;
            }
        } else {
            header->prev->size += header->size + sizeof(rxHeapBlockHeader);
            header->prev->freeEntry->size = header->prev->size;
            header->prev->next = header->next;

            if (header->next) {
                header->next->prev = header->prev;
            }
        }
    } else if (nextFree) {
        header->size += header->next->size + sizeof(rxHeapBlockHeader);
        header->freeEntry = header->next->freeEntry;
        header->next->freeEntry->ptr = header;
        header->next->freeEntry->size = header->size;
        header->next = header->next->next;

        if (header->next) {
            header->next->prev = header;
        }
    } else {
        rxHeapFreeBlock* freeEntry = HeapFreeBlocksAddEntry(heap);

        if (freeEntry) {
            freeEntry->ptr = header;
            freeEntry->size = header->size;
            header->freeEntry = freeEntry;
        }
    }
}

void* RxHeapAlloc(RxHeap* heap, RwUInt32 size) {
    rxHeapBlockHeader* block;
    RwUInt32 i;
    rxHeapFreeBlock* freeEntry;

    size = (size + (rxHEAPALIGNMENT - 1)) & ~(rxHEAPALIGNMENT - 1);

    freeEntry = &heap->freeBlocks[heap->entriesUsed - 1];
    i = heap->entriesUsed;
    do {
        if (freeEntry->size >= size) {
            heap->dirty = TRUE;
            return HeapAllocFrom(heap, freeEntry, size);
        }

        freeEntry--;
    } while (--i);

    block = HeapGrow(heap, size);
    if (block) {
        heap->dirty = TRUE;
        return HeapAllocFrom(heap, block->freeEntry, size);
    }

    RWERROR((E_RW_NOMEM, size));
    return NULL;
}

void* RxHeapRealloc(RxHeap* heap, void* block, RwUInt32 newSize) {
    RwUInt32 size;
    rxHeapBlockHeader* header;
    rxHeapFreeBlock* freeEntry;
    RwInt32 shrink;
    RwInt32 delta;

    header = (rxHeapBlockHeader*)block - 1;

    size = (newSize + (rxHEAPALIGNMENT - 1)) & ~(rxHEAPALIGNMENT - 1);

    delta = size - header->size;
    if (delta > 0) {
        return HeapReallocGrow(heap, block, size, header, delta);
    }

    shrink = -delta;
    if (shrink < rxHEAPSPLITTHRESHOLD) {
        return block;
    }

    freeEntry = HeapFreeBlocksAddEntry(heap);
    if (!freeEntry) {
        return block;
    }

    {
        rxHeapBlockHeader* newBlock = (rxHeapBlockHeader*)((RwUInt8*)(header + 1) + size);

        newBlock->size = shrink - sizeof(rxHeapBlockHeader);
        newBlock->prev = header;
        newBlock->next = header->next;
        newBlock->freeEntry = freeEntry;
        header->size = size;
        header->next = newBlock;

        if (newBlock->next) {
            newBlock->next->prev = newBlock;
        }

        freeEntry->ptr = newBlock;
        freeEntry->size = newBlock->size;
    }

    return block;
}

RwBool _rxHeapReset(RxHeap* heap) {
    rxHeapSuperBlockDescriptor* prev = NULL;
    rxHeapSuperBlockDescriptor* cur;

    heap->entriesUsed = 0;

    for (cur = heap->head->next; cur; cur = cur->next) {
        if (!HeapSuperBlockInit(heap, cur, prev)) {
            return FALSE;
        }

        if (!prev) {
            heap->headBlock = (rxHeapBlockHeader*)cur->start;
        }

        prev = cur;
    }

    cur = heap->head;
    if (!HeapSuperBlockInit(heap, cur, prev)) {
        return FALSE;
    }

    if (!prev) {
        heap->headBlock = (rxHeapBlockHeader*)cur->start;
    }

    heap->dirty = FALSE;
    return TRUE;
}

void RxHeapDestroy(RxHeap* heap) {
    if (heap) {
        rxHeapSuperBlockDescriptor* cur;

        if (heap->freeBlocks) {
            RwFree(heap->freeBlocks);
            heap->freeBlocks = NULL;
        }

        cur = heap->head;
        while (cur) {
            rxHeapSuperBlockDescriptor* next = cur->next;

            if (cur) {
                RwFree(cur);
            }

            cur = next;
        }

        RwFree(heap);
    }
}

RxHeap* RxHeapCreate(RwUInt32 size) {
    RxHeap* heap;

    if (size < rxHEAPMINSIZE) {
        size = rxHEAPMINSIZE;
    }

    heap = (RxHeap*)RwMalloc(sizeof(RxHeap));
    if (heap) {
        rxHeapSuperBlockDescriptor* superBlock;

        size = (size + (rxHEAPALIGNMENT - 1)) & ~(rxHEAPALIGNMENT - 1);
        if (size < rxHEAPSUPERBLOCKMINSIZE) {
            size = rxHEAPSUPERBLOCKMINSIZE;
        }

        superBlock = HeapSuperBlockCreate(size);
        if (superBlock) {
            heap->superBlockSize = size;
            heap->head = superBlock;
            heap->freeBlocks = NULL;
            heap->entriesAlloced = 0;
            heap->entriesUsed = 0;
            heap->dirty = TRUE;

            if (RxHeapReset(heap)) {
                return heap;
            }

            if (superBlock) {
                RwFree(superBlock);
            }
        }

        RwFree(heap);
    }

    return NULL;
}

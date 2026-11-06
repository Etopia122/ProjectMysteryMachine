/*
 * Resource arena heap: a best-fit allocator over one memory block. Blocks
 * carry a 32-byte header; adjacent free blocks are merged on free.
 */

#include "rwsdk/rwcore.h"

typedef struct RwResHeapBlockHeader RwResHeapBlockHeader;

typedef struct RwResHeap {
    RwResHeapBlockHeader* headBlock;
    RwResHeapBlockHeader* freeBlocks;
} RwResHeap;

struct RwResHeapBlockHeader {
    RwResHeap* heap;
    RwResHeapBlockHeader* next;
    RwResHeapBlockHeader* prev;
    RwUInt32 size;
    RwUInt32 inUse;
    RwUInt32 pad[3];
};

#define BLOCKINUSE(b) ((b)->inUse & 1)
#define BLOCKFREE(b) (~(b)->inUse & 1)

RwBool _rwResHeapInit(void* mem, RwUInt32 size) {
    RwResHeap* heap = (RwResHeap*)mem;
    RwResHeapBlockHeader* header;
    RwInt32 freeSize;

    header = (RwResHeapBlockHeader*)(((RwUInt32)heap + sizeof(RwResHeap) + 31) & ~31);
    freeSize = (((RwUInt32)heap + size) & ~31) - (RwUInt32)header - sizeof(RwResHeapBlockHeader);
    if (freeSize < 32) {
        return FALSE;
    }

    header->heap = heap;
    header->next = NULL;
    header->prev = NULL;
    header->inUse = 0;
    header->size = freeSize;

    heap->headBlock = header;
    heap->freeBlocks = header;

    return TRUE;
}

RwBool _rwResHeapClose(void* heap) {
    return TRUE;
}

void _rwResHeapFree(void* alloc) {
    RwResHeapBlockHeader* header = (RwResHeapBlockHeader*)alloc - 1;
    RwResHeapBlockHeader* prev;
    RwResHeapBlockHeader* next;
    RwResHeap* heap;

    header->inUse = 0;
    heap = header->heap;
    prev = header->prev;
    next = header->next;

    if (!heap->freeBlocks || header < heap->freeBlocks) {
        heap->freeBlocks = header;
    }

    if (prev && BLOCKFREE(prev)) {
        prev->next = next;
        if (next) {
            next->prev = prev;
        }
        prev->size += header->size + sizeof(RwResHeapBlockHeader);
        header = prev;
    }

    if (next && BLOCKFREE(next)) {
        header->next = next->next;
        if (next->next) {
            next->next->prev = header;
        }
        header->size += next->size + sizeof(RwResHeapBlockHeader);
    }
}

void* _rwResHeapAlloc(void* mem, RwUInt32 size) {
    RwResHeap* heap = (RwResHeap*)mem;
    RwResHeapBlockHeader* freeBlock = heap->freeBlocks;
    RwResHeapBlockHeader* best = NULL;

    size = (size + 31) & ~31;

    while (freeBlock) {
        if (BLOCKFREE(freeBlock) && freeBlock->size >= size) {
            if (!best || freeBlock->size < best->size) {
                best = freeBlock;
            }
        }
        freeBlock = freeBlock->next;
    }

    if (!best) {
        return NULL;
    }

    if (best->size > size + 2 * sizeof(RwResHeapBlockHeader)) {
        RwResHeapBlockHeader* newBlock = (RwResHeapBlockHeader*)((RwUInt8*)best + size + sizeof(RwResHeapBlockHeader));
        RwResHeapBlockHeader* next = best->next;

        if (next && BLOCKFREE(next)) {
            newBlock->next = next->next;
            newBlock->size = best->next->size + (best->size - size);
        } else {
            newBlock->next = next;
            newBlock->size = best->size - size - sizeof(RwResHeapBlockHeader);
        }

        best->next = newBlock;
        newBlock->inUse = 0;
        newBlock->prev = best;
        if (newBlock->next) {
            newBlock->next->prev = newBlock;
        }
        best->size = size;
        newBlock->heap = best->heap;
    }

    if (best == heap->freeBlocks) {
        do {
            heap->freeBlocks = heap->freeBlocks->next;
        } while (heap->freeBlocks && BLOCKINUSE(heap->freeBlocks));
    }

    best->inUse = 1;
    return best + 1;
}

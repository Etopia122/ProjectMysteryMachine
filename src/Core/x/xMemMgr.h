#ifndef XMEMMGR_H
#define XMEMMGR_H

extern unsigned int gActiveHeap;

void* xMemAlloc(unsigned int heapID, unsigned int size);

#endif

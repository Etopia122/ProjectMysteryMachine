// GameCube auxiliary RAM (ARAM) and its DMA queue (ARQ) on PC.
// ARAM is a plain buffer; ARAM "addresses" are offsets into it. Transfers are
// done immediately, and completion callbacks run from a small queue so a
// callback that posts the next transfer doesn't recurse.
#include <stdlib.h>
#include <string.h>

#include "pc_platform.h"

extern "C" {

struct ARQRequest
{
    ARQRequest* next;
    unsigned long owner;
    unsigned long type; // 0: main RAM -> ARAM, 1: ARAM -> main RAM
    unsigned long priority;
    unsigned long source;
    unsigned long dest;
    unsigned long length;
    void (*callback)(unsigned long);
};

static const unsigned long ARAM_SIZE = 16 * 1024 * 1024;
static const unsigned long ARAM_BASE = 0x4000; // the first 16 KB are reserved on hardware
static unsigned char* s_aram;
static unsigned long* s_stack;
static unsigned long s_stackMax, s_stackCount;
static unsigned long s_top;

static unsigned char* aram(unsigned long addr) { return s_aram + addr; }

unsigned long ARInit(unsigned long* stack, unsigned long count)
{
    if (!s_aram)
    {
        s_aram = (unsigned char*)calloc(1, ARAM_SIZE);
    }
    s_stack = stack;
    s_stackMax = count;
    s_stackCount = 0;
    s_top = ARAM_BASE;
    return ARAM_BASE;
}

unsigned long ARAlloc(unsigned long length)
{
    unsigned long addr = s_top;
    if (addr + length > ARAM_SIZE || s_stackCount >= s_stackMax)
    {
        return 0;
    }
    s_stack[s_stackCount++] = length;
    s_top += length;
    return addr;
}

// Frees the most recent allocation (ARAM is a stack allocator).
unsigned long ARFree(unsigned long* length)
{
    if (s_stackCount == 0)
    {
        return s_top;
    }
    unsigned long len = s_stack[--s_stackCount];
    s_top -= len;
    if (length)
    {
        *length = len;
    }
    return s_top;
}

unsigned long ARGetBaseAddress() { return ARAM_BASE; }
unsigned long ARGetSize() { return ARAM_SIZE; }

void ARQInit() {}

// The transfer happens immediately; completion is signalled as an interrupt,
// so it can't run while the game has interrupts disabled (as on hardware).
void ARQPostRequest(ARQRequest* task, unsigned long owner, unsigned long type, unsigned long priority,
                    unsigned long source, unsigned long dest, unsigned long length,
                    void (*callback)(unsigned long))
{
    task->next = NULL;
    task->owner = owner;
    task->type = type;
    task->priority = priority;
    task->source = source;
    task->dest = dest;
    task->length = length;
    task->callback = callback;

    if (type == 0)
    {
        memcpy(aram(dest), (void*)source, length);
    }
    else
    {
        memcpy((void*)dest, aram(source), length);
    }
    if (callback)
    {
        pc_irq_post([task, callback]() { callback((unsigned long)task); });
    }
}

void ARQRemoveRequest(ARQRequest* task) {}
void ARQFlushQueue() {}

// Reads/writes ARAM directly; used by the PC audio mixer.
unsigned char* pc_aram_ptr(unsigned long addr) { return aram(addr); }

}

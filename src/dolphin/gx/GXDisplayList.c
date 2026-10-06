/**
 * GXDisplayList.c
 * Description:
 */

#include "dolphin/gx/GXDisplayList.h"
#include "dolphin/gx/GX.h"
#include "dolphin/os/OS.h"
#include "string.h"

#if defined(VERSION_GIHE78)
// The older SDK keeps only the FIFO fields it needs.
typedef struct __GXFifoObj {
    /* 0x00 */ void* base;
    /* 0x04 */ void* end;
    /* 0x08 */ u32 size;
    /* 0x0C */ u32 high_wtrmark;
    /* 0x10 */ u32 low_wtrmark;
    /* 0x14 */ void* read_ptr;
    /* 0x18 */ void* write_ptr;
    /* 0x1C */ s32 rw_dst;
    /* 0x20 */ u8 fifo_wrap;
} __GXFifoObj;

static __GXFifoObj DisplayListFifo;
static GXData __savedGXdata;
static GXFifoObj* OldCPUFifo;

void GXBeginDisplayList(void* list, u32 size) {
    GXFifoObj* CPUFifo = GXGetCPUFifo();

    if (gx->dirtyState != 0) {
        __GXSetDirtyState();
    }

    if (gx->dlSaveContext) {
        memcpy(&__savedGXdata, gx, sizeof(*gx));
    }

    DisplayListFifo.base = list;
    DisplayListFifo.end = (u8*)list + size - 4;
    DisplayListFifo.size = size;
    DisplayListFifo.rw_dst = 0;
    DisplayListFifo.read_ptr = list;
    DisplayListFifo.write_ptr = list;
    gx->inDispList = 1;
    GXSaveCPUFifo(CPUFifo);
    OldCPUFifo = CPUFifo;
    GXSetCPUFifo((GXFifoObj*)&DisplayListFifo);
}

u32 GXEndDisplayList(void) {
    u32 ov;
    BOOL enabled;
    u32 cpEnable;
    u32 reg;

    if (gx->dirtyState != 0) {
        __GXSetDirtyState();
    }

    reg = __piReg[5];
    ov = (reg >> 26) & 1;
    __GXSaveCPUFifoAux((GXFifoObj*)&DisplayListFifo);
    GXSetCPUFifo(OldCPUFifo);

    if (gx->dlSaveContext) {
        enabled = OSDisableInterrupts();
        cpEnable = gx->cpEnable;
        memcpy(gx, &__savedGXdata, sizeof(*gx));
        gx->cpEnable = cpEnable;
        OSRestoreInterrupts(enabled);
    }

    gx->inDispList = 0;

    if (!ov) {
        return DisplayListFifo.rw_dst;
    }
    return 0;
}
#endif

void GXCallDisplayList(void* list, u32 nbytes) {
    if (gx->dirtyState != 0) {
        __GXSetDirtyState();
    }

    if (*(u32*)gx == 0) {
        __GXSendFlushPrim();
    }

    GXFIFO.u8 = 0x40;
    GXFIFO.u32 = (u32)list;
    GXFIFO.u32 = nbytes;
}

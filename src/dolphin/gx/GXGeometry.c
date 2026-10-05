/**
 * GXGeometry.c
 * Description:
 */

#include "dolphin/gx/GXGeometry.h"
#include "dolphin/gx/GX.h"

void __GXSetDirtyState(void) {
    if (gx->dirtyState & GX_DIRTY_SU_TEX) {
        __GXSetSUTexRegs();
    }

    if (gx->dirtyState & GX_DIRTY_BP_MASK) {
        __GXUpdateBPMask();
    }

    if (gx->dirtyState & GX_DIRTY_GEN_MODE) {
        __GXSetGenMode();
    }

    if (gx->dirtyState & GX_DIRTY_VCD) {
        __GXSetVCD();
    }

    if (gx->dirtyState & GX_DIRTY_VAT) {
        __GXSetVAT();
    }

    gx->dirtyState = 0;
}

void GXBegin(GXPrimitive type, GXVtxFmt fmt, u16 vert_num) {
    if (gx->dirtyState) {
        __GXSetDirtyState();
    }

    if (*(u32*)gx == 0) {
        __GXSendFlushPrim();
    }

    GXFIFO.u8 = fmt | type;
    GXFIFO.u16 = vert_num;
}

void __GXSendFlushPrim(void) {
    u32 i;
    u32 sz = gx->vNum * gx->vLim;

    GXFIFO.u8 = 0x98;
    GXFIFO.u16 = gx->vNum;

    for (i = 0; i < sz; i += 4) {
        GXFIFO.s32 = 0;
    }

    gx->bpSentNot = 1;
}

void GXSetLineWidth(u8 width, GXTexOffset offsets) {
    GX_BITFIELD_SET(gx->lpSize, 24, 8, width);
    GX_BITFIELD_SET(gx->lpSize, 13, 3, offsets);
    GXFIFO.u8 = 0x61;
    GXFIFO.u32 = gx->lpSize;
    gx->bpSentNot = 0;
}

void GXSetPointSize(u8 size, GXTexOffset offsets) {
    GX_BITFIELD_SET(gx->lpSize, 16, 8, size);
    GX_BITFIELD_SET(gx->lpSize, 10, 3, offsets);
    GXFIFO.u8 = 0x61;
    GXFIFO.u32 = gx->lpSize;
    gx->bpSentNot = 0;
}

void GXEnableTexOffsets(GXTexCoordID coord, GXBool line, GXBool point) {
    GX_BITFIELD_SET(gx->suTs0[coord], 13, 1, line);
    GX_BITFIELD_SET(gx->suTs0[coord], 12, 1, point);
    GXFIFO.u8 = 0x61;
    GXFIFO.u32 = gx->suTs0[coord];
    gx->bpSentNot = 0;
}

void GXSetCullMode(GXCullMode mode) {
    switch (mode) {
    case GX_CULL_FRONT:
        mode = GX_CULL_BACK;
        break;
    case GX_CULL_BACK:
        mode = GX_CULL_FRONT;
        break;
    }

    GX_BITFIELD_SET(gx->genMode, 16, 2, mode);
    gx->dirtyState |= GX_DIRTY_GEN_MODE;
}

void GXSetCoPlanar(GXBool enable) {
    GX_BITFIELD_SET(gx->genMode, 12, 1, enable);
    GXFIFO.u8 = 0x61;
    GXFIFO.u32 = 0xFE080000;
    GXFIFO.u8 = 0x61;
    GXFIFO.u32 = gx->genMode;
}

void __GXSetGenMode(void) {
    GXFIFO.u8 = 0x61;
    GXFIFO.u32 = gx->genMode;
    gx->bpSentNot = 0;
}

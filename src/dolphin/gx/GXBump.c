/**
 * GXBump.c
 * Description:
 */

#include "dolphin/gx/GXBump.h"
#include "dolphin/gx/GX.h"

void GXSetTevIndirect(GXTevStageID tevStage, GXIndTexStageID texStage, GXIndTexFormat texFmt,
                      GXIndTexBiasSel biasSel, GXIndTexMtxID mtxID, GXIndTexWrap wrapS,
                      GXIndTexWrap wrapT, u8 addPrev, u8 utcLod, GXIndTexAlphaSel alphaSel) {
    u32 field = 0;

    GX_BITFIELD_SET(field, 30, 2, texStage);
    GX_BITFIELD_SET(field, 28, 2, texFmt);
    GX_BITFIELD_SET(field, 25, 3, biasSel);
    GX_BITFIELD_SET(field, 23, 2, alphaSel);
    GX_BITFIELD_SET(field, 19, 4, mtxID);
    GX_BITFIELD_SET(field, 16, 3, wrapS);
    GX_BITFIELD_SET(field, 13, 3, wrapT);
    GX_BITFIELD_SET(field, 12, 1, utcLod);
    GX_BITFIELD_SET(field, 11, 1, addPrev);
    GX_BITFIELD_SET(field, 0, 8, tevStage + 0x10);

    GXFIFO.u8 = 0x61;
    GXFIFO.s32 = field;

    gx->bpSentNot = 0;
}

#define GX_BITFIELD_SET_MASKED(field, pos, size, value) ((field) = ((field) & ~(((1 << (size)) - 1) << (31 - (pos) - (size) + 1))) | (((u32)(value) << (31 - (pos) - (size) + 1)) & (((1 << (size)) - 1) << (31 - (pos) - (size) + 1))))

void GXSetIndTexMtx(GXIndTexMtxID mtxID, f32 offset[6], s8 scale_exp) {
    u32 val;
    u32 field;
    f32 mtx2[6];
    u32 stack_padding[6];

    scale_exp += 17;

    switch (mtxID) {
    case GX_ITM_0:
    case GX_ITM_1:
    case GX_ITM_2:
        val = mtxID - 1;
        break;
    case GX_ITM_S0:
    case GX_ITM_S1:
    case GX_ITM_S2:
        val = mtxID - 5;
        break;
    case GX_ITM_T0:
    case GX_ITM_T1:
    case GX_ITM_T2:
        val = mtxID - 9;
        break;
    case GX_ITM_3:
    case GX_ITM_S3:
    default:
        val = 0;
    }

    field = 0;
    GX_BITFIELD_SET_MASKED(field, 21, 11, (s32)(1024.0f * offset[0]));
    GX_BITFIELD_SET_MASKED(field, 10, 11, (s32)(1024.0f * offset[3]));
    GX_BITFIELD_SET(field, 8, 2, (scale_exp >> 0) & 3);
    GX_BITFIELD_SET(field, 0, 8, val * 3 + 6);
    GXFIFO.u8 = 0x61;
    GXFIFO.s32 = field;

    field = 0;
    GX_BITFIELD_SET_MASKED(field, 21, 11, (s32)(1024.0f * offset[1]));
    GX_BITFIELD_SET_MASKED(field, 10, 11, (s32)(1024.0f * offset[4]));
    GX_BITFIELD_SET(field, 8, 2, (scale_exp >> 2) & 3);
    GX_BITFIELD_SET(field, 0, 8, val * 3 + 7);
    GXFIFO.u8 = 0x61;
    GXFIFO.s32 = field;

    field = 0;
    GX_BITFIELD_SET_MASKED(field, 21, 11, (s32)(1024.0f * offset[2]));
    GX_BITFIELD_SET_MASKED(field, 10, 11, (s32)(1024.0f * offset[5]));
    GX_BITFIELD_SET(field, 8, 2, (scale_exp >> 4) & 3);
    GX_BITFIELD_SET(field, 0, 8, val * 3 + 8);
    GXFIFO.u8 = 0x61;
    GXFIFO.s32 = field;

    gx->bpSentNot = 0;
}

void GXSetIndTexCoordScale(GXIndTexStageID texStage, GXIndTexScale scaleS, GXIndTexScale scaleT) {
    GXData* data;

    switch (texStage) {
    case GX_INDTEXSTAGE0:
        data = gx;
        GX_BITFIELD_SET(gx->IndTexScale0, 28, 4, scaleS);
        GX_BITFIELD_SET(gx->IndTexScale0, 24, 4, scaleT);
        GX_BITFIELD_SET(gx->IndTexScale0, 0, 8, 0x25);
        GXFIFO.u8 = 0x61;
        GXFIFO.s32 = gx->IndTexScale0;
        break;
    case GX_INDTEXSTAGE1:
        data = gx;
        GX_BITFIELD_SET(gx->IndTexScale0, 20, 4, scaleS);
        GX_BITFIELD_SET(gx->IndTexScale0, 16, 4, scaleT);
        GX_BITFIELD_SET(gx->IndTexScale0, 0, 8, 0x25);
        GXFIFO.u8 = 0x61;
        GXFIFO.s32 = gx->IndTexScale0;
        break;
    case GX_INDTEXSTAGE2:
        data = gx;
        GX_BITFIELD_SET(gx->IndTexScale1, 28, 4, scaleS);
        GX_BITFIELD_SET(gx->IndTexScale1, 24, 4, scaleT);
        GX_BITFIELD_SET(gx->IndTexScale1, 0, 8, 0x26);
        GXFIFO.u8 = 0x61;
        GXFIFO.s32 = gx->IndTexScale1;
        break;
    case GX_INDTEXSTAGE3:
        data = gx;
        GX_BITFIELD_SET(gx->IndTexScale1, 20, 4, scaleS);
        GX_BITFIELD_SET(gx->IndTexScale1, 16, 4, scaleT);
        GX_BITFIELD_SET(gx->IndTexScale1, 0, 8, 0x26);
        GXFIFO.u8 = 0x61;
        GXFIFO.s32 = gx->IndTexScale1;
        break;
    }

    gx->bpSentNot = 0;
}

void GXSetIndTexOrder(GXIndTexStageID stage, GXTexCoordID coord, GXTexMapID map) {
    switch (stage) {
    case GX_INDTEXSTAGE0:
        GX_BITFIELD_SET(gx->iref, 29, 3, map);
        GX_BITFIELD_SET(gx->iref, 26, 3, coord);
        break;
    case GX_INDTEXSTAGE1:
        GX_BITFIELD_SET(gx->iref, 23, 3, map);
        GX_BITFIELD_SET(gx->iref, 20, 3, coord);
        break;
    case GX_INDTEXSTAGE2:
        GX_BITFIELD_SET(gx->iref, 17, 3, map);
        GX_BITFIELD_SET(gx->iref, 14, 3, coord);
        break;
    case GX_INDTEXSTAGE3:
        GX_BITFIELD_SET(gx->iref, 11, 3, map);
        GX_BITFIELD_SET(gx->iref, 8, 3, coord);
        break;
    }

    GXFIFO.u8 = 0x61;
    GXFIFO.s32 = gx->iref;
    GXSetWasteFlags();
}

void GXSetNumIndStages(u8 num) {
    GX_BITFIELD_SET(gx->genMode, 13, 3, num);
    gx->dirtyState |= GX_DIRTY_BP_MASK | GX_DIRTY_GEN_MODE;
}

#pragma dont_inline on
void GXSetTevDirect(GXTevStageID stage) {
    GXSetTevIndirect(stage, GX_INDTEXSTAGE0, GX_ITF_8, GX_ITB_NONE, GX_ITM_OFF, GX_ITW_OFF,
                     GX_ITW_OFF, FALSE, FALSE, GX_ITBA_OFF);
}
#pragma dont_inline reset

void __GXUpdateBPMask(void) {
    u32 nIndStages;
    u32 i;
    u32 tmap;
    u32 new_imask = 0;

    nIndStages = (gx->genMode >> 16) & 7;
    for (i = 0; i < nIndStages; i++) {
        switch (i) {
        case 0:
            tmap = gx->iref & 7;
            break;
        case 1:
            tmap = (gx->iref >> 6) & 7;
            break;
        case 2:
            tmap = (gx->iref >> 12) & 7;
            break;
        case 3:
            tmap = (gx->iref >> 18) & 7;
            break;
        }
        new_imask |= 1 << tmap;
    }

    if ((gx->bpMask & 0xFF) != new_imask) {
        SET_REG_FIELD(gx->bpMask, 8, 0, new_imask);
        GX_BP_LOAD_REG(gx->bpMask);
        gx->bpSentNot = 0;
    }
}

void __GXSetIndirectMask(u32 mask) {

    GX_BITFIELD_SET(gx->bpMask, 24, 8, mask);
    GXFIFO.u8 = 0x61;
    GXFIFO.s32 = gx->bpMask;
    gx->bpSentNot = 0;
}

void __GXFlushTextureState(void) {
    GXFIFO.u8 = 0x61;
    GXFIFO.s32 = gx->bpMask;
    gx->bpSentNot = 0;
}

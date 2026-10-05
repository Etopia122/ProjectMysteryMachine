#include "dolphin/gx/GXPixel.h"
#include "dolphin/gx/GX.h"

#define GX_SET_REG_RLWIMI(reg, x, st, end) ((reg) = ((reg) & ~(((1 << ((end) - (st) + 1)) - 1) << (31 - (end)))) | (((u32)(x) << (31 - (end))) & (((1 << ((end) - (st) + 1)) - 1) << (31 - (end)))))

// Older GXSetFog: perspective fog only (no orthographic type bit).
void GXSetFog(GXFogType type, f32 startz, f32 endz, f32 nearz, f32 farz, GXColor color) {
    f32 a, c;
    u32 a_bits, c_bits;
    f32 tmpA, tmpB, tmpC;
    u32 expB, magB, shiftB;

    u32 fogColorReg = 0;
    u32 fogParamReg0 = 0;
    u32 fogParamReg1 = 0;
    u32 fogParamReg2 = 0;
    u32 fogParamReg3 = 0;

    if (farz == nearz || endz == startz) {
        tmpA = 0.0f;
        tmpB = 0.5f;
        tmpC = 0.0f;
    } else {
        tmpA = (farz * nearz) / ((farz - nearz) * (endz - startz));
        tmpB = farz / (farz - nearz);
        tmpC = startz / (endz - startz);
    }

    expB = 0;
    while (tmpB > 1.0) {
        tmpB /= 2.0f;
        expB++;
    }
    while (tmpB > 0.0f && tmpB < 0.5) {
        tmpB *= 2.0f;
        expB--;
    }

    a = tmpA / (1 << expB + 1);
    magB = 8388638.0f * tmpB;
    shiftB = expB + 1;
    c = tmpC;

    fogParamReg1 = 0;
    SET_REG_FIELD(fogParamReg1, 24, 0, magB);
    SET_REG_FIELD(fogParamReg1, 8, 24, 0xEF);

    fogParamReg2 = 0;
    SET_REG_FIELD(fogParamReg2, 24, 0, shiftB);
    SET_REG_FIELD(fogParamReg2, 8, 24, 0xF0);

    a_bits = *(u32*)&a;
    c_bits = *(u32*)&c;

    fogParamReg0 = 0;
    SET_REG_FIELD(fogParamReg0, 11, 0, (a_bits >> 12) & 0x7FF);
    SET_REG_FIELD(fogParamReg0, 8, 11, (a_bits >> 23) & 0xFF);
    SET_REG_FIELD(fogParamReg0, 1, 19, (a_bits >> 31));
    SET_REG_FIELD(fogParamReg0, 8, 24, 0xEE);

    fogParamReg3 = 0;
    SET_REG_FIELD(fogParamReg3, 11, 0, (c_bits >> 12) & 0x7FF);
    SET_REG_FIELD(fogParamReg3, 8, 11, (c_bits >> 23) & 0xFF);
    SET_REG_FIELD(fogParamReg3, 1, 19, (c_bits >> 31));
    SET_REG_FIELD(fogParamReg3, 3, 21, type);
    SET_REG_FIELD(fogParamReg3, 8, 24, 0xF1);

    fogColorReg = 0;
    SET_REG_FIELD(fogColorReg, 8, 0, color.b);
    SET_REG_FIELD(fogColorReg, 8, 8, color.g);
    SET_REG_FIELD(fogColorReg, 8, 16, color.r);
    SET_REG_FIELD(fogColorReg, 8, 24, 0xF2);

    GX_BP_LOAD_REG(fogParamReg0);
    GX_BP_LOAD_REG(fogParamReg1);
    GX_BP_LOAD_REG(fogParamReg2);
    GX_BP_LOAD_REG(fogParamReg3);
    GX_BP_LOAD_REG(fogColorReg);

    gx->bpSentNot = GX_FALSE;
}

void GXSetFogRangeAdj(GXBool enable, u16 center, GXFogAdjTable* table) {
    u32 fogRangeReg;
    u32 fogRangeRegK;
    u32 i;

    if (enable) {
        for (i = 0; i < 10; i += 2) {
            fogRangeRegK = 0;
            GX_SET_REG(fogRangeRegK, table->fogVals[i], GX_BP_FOGRANGEK_HI_ST,
                       GX_BP_FOGRANGEK_HI_END);
            GX_SET_REG(fogRangeRegK, table->fogVals[i + 1], GX_BP_FOGRANGEK_LO_ST,
                       GX_BP_FOGRANGEK_LO_END);
            GX_SET_REG(fogRangeRegK, GX_BP_REG_FOGRANGEK0 + (i / 2), 0, 7);
            GX_BP_LOAD_REG(fogRangeRegK);
        }
    }

    fogRangeReg = 0;
    GX_SET_REG(fogRangeReg, center + 342, GX_BP_FOGRANGE_CENTER_ST, GX_BP_FOGRANGE_CENTER_END);
    GX_SET_REG(fogRangeReg, enable, GX_BP_FOGRANGE_ENABLED_ST, GX_BP_FOGRANGE_ENABLED_END);
    GX_SET_REG(fogRangeReg, GX_BP_REG_FOGRANGE, 0, 7);
    GX_BP_LOAD_REG(fogRangeReg);

    gx->bpSentNot = GX_FALSE;
}

void GXSetBlendMode(GXBlendMode type, GXBlendFactor src_factor, GXBlendFactor dst_factor,
                    GXLogicOp op) {
    SET_REG_FIELD(gx->cmode0, 1, 0, (type == GX_BM_BLEND || type == GX_BM_SUBTRACT));
    SET_REG_FIELD(gx->cmode0, 1, 11, (type == GX_BM_SUBTRACT));
    SET_REG_FIELD(gx->cmode0, 1, 1, (type == GX_BM_LOGIC));
    SET_REG_FIELD(gx->cmode0, 4, 12, op);
    SET_REG_FIELD(gx->cmode0, 3, 8, src_factor);
    SET_REG_FIELD(gx->cmode0, 3, 5, dst_factor);
    SET_REG_FIELD(gx->cmode0, 8, 24, 0x41);
    GX_BP_LOAD_REG(gx->cmode0);
    gx->bpSentNot = GX_FALSE;
}

void GXSetColorUpdate(GXBool updateEnable) {
    SET_REG_FIELD(gx->cmode0, 1, 3, updateEnable);
    GX_BP_LOAD_REG(gx->cmode0);
    gx->bpSentNot = GX_FALSE;
}

void GXSetAlphaUpdate(GXBool updateEnable) {
    SET_REG_FIELD(gx->cmode0, 1, 4, updateEnable);
    GX_BP_LOAD_REG(gx->cmode0);
    gx->bpSentNot = GX_FALSE;
}

void GXSetZMode(GXBool compareEnable, GXCompare func, GXBool updateEnable) {
    SET_REG_FIELD(gx->zmode, 1, 0, compareEnable);
    SET_REG_FIELD(gx->zmode, 3, 1, func);
    SET_REG_FIELD(gx->zmode, 1, 4, updateEnable);
    GX_BP_LOAD_REG(gx->zmode);
    gx->bpSentNot = GX_FALSE;
}

void GXSetZCompLoc(GXBool beforeTex) {
    GX_SET_REG(gx->peCtrl, beforeTex, GX_BP_ZCONTROL_BEFORE_TEX_ST,
               GX_BP_ZCONTROL_BEFORE_TEX_END);
    GX_BP_LOAD_REG(gx->peCtrl);
    gx->bpSentNot = GX_FALSE;
}

void GXSetPixelFmt(GXPixelFmt pixelFmt, GXZFmt16 zFmt) {
    GXBool isZ16;
    static u32 p2f[GX_PF_MAX] = {GX_PF_RGB8_Z24, GX_PF_RGBA6_Z24, GX_PF_RGB565_Z16, GX_PF_Z24,
                                 GX_PF_Y8,       GX_PF_Y8,        GX_PF_Y8,         GX_PF_U8};

    const u32 zControlRegOld = gx->peCtrl;

    GX_SET_REG(gx->peCtrl, p2f[pixelFmt], GX_BP_ZCONTROL_PIXEL_FMT_ST,
               GX_BP_ZCONTROL_PIXEL_FMT_END);
    GX_SET_REG(gx->peCtrl, zFmt, GX_BP_ZCONTROL_Z_FMT_ST, GX_BP_ZCONTROL_Z_FMT_END);

    if (zControlRegOld != gx->peCtrl) {
        GX_BP_LOAD_REG(gx->peCtrl);
        isZ16 = (pixelFmt == GX_PF_RGB565_Z16) ? GX_TRUE : GX_FALSE;
        GX_SET_REG(gx->genMode, isZ16, GX_BP_GENMODE_MULTISAMPLE_ST,
                   GX_BP_GENMODE_MULTISAMPLE_END);
        gx->dirtyState |= GX_DIRTY_GEN_MODE;
    }

    if (p2f[pixelFmt] == GX_PF_Y8) {
        SET_REG_FIELD(gx->cmode1, 2, 9, (pixelFmt - GX_PF_Y8) & 3);
        SET_REG_FIELD(gx->cmode1, 8, 24, GX_BP_REG_DSTALPHA);
        GX_BP_LOAD_REG(gx->cmode1);
    }

    gx->bpSentNot = FALSE;
}

void GXSetDither(GXBool dither) {
    SET_REG_FIELD(gx->cmode0, 1, 2, dither);
    GX_BP_LOAD_REG(gx->cmode0);
    gx->bpSentNot = GX_FALSE;
}

void GXSetDstAlpha(GXBool enable, u8 alpha) {
    SET_REG_FIELD(gx->cmode1, 8, 0, alpha);
    SET_REG_FIELD(gx->cmode1, 1, 8, enable);
    GX_BP_LOAD_REG(gx->cmode1);
    gx->bpSentNot = GX_FALSE;
}

void GXSetFieldMask(GXBool enableEven, GXBool enableOdd) {
    u32 fieldMaskReg = 0;
    GX_SET_REG(fieldMaskReg, enableOdd, GX_BP_FIELDMASK_ODD_ST, GX_BP_FIELDMASK_ODD_END);
    GX_SET_REG(fieldMaskReg, enableEven, GX_BP_FIELDMASK_EVEN_ST, GX_BP_FIELDMASK_EVEN_END);
    GX_SET_REG(fieldMaskReg, GX_BP_REG_FIELDMASK, 0, 7);

    GX_BP_LOAD_REG(fieldMaskReg);
    gx->bpSentNot = GX_FALSE;
}

void GXSetFieldMode(GXBool texLOD, GXBool adjustAR) {
    GX_SET_REG(gx->lpSize, adjustAR, GX_BP_LINEPTWIDTH_ADJUST_ST,
               GX_BP_LINEPTWIDTH_ADJUST_END);
    GX_BP_LOAD_REG(gx->lpSize);

    __GXFlushTextureState();
    GX_BP_LOAD_REG(GX_BP_REG_FIELDMODE << 24 | texLOD);
    __GXFlushTextureState();
}

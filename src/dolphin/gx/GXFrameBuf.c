#include "dolphin/gx/GXFrameBuf.h"
#include "dolphin/gx/GX.h"

GXRenderModeObj GXNtsc480IntDf = {
    VI_TVMODE_NTSC_INT,
    640,
    480,
    480,
    40,
    0,
    640,
    480,
    VI_XFBMODE_DF,
    GX_FALSE,
    GX_FALSE,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    8,
    8,
    10,
    12,
    10,
    8,
    8,
};

GXRenderModeObj GXMpal480IntDf = {
    VI_TVMODE_MPAL_INT,
    640,
    480,
    480,
    40,
    0,
    640,
    480,
    VI_XFBMODE_DF,
    GX_FALSE,
    GX_FALSE,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    8,
    8,
    10,
    12,
    10,
    8,
    8,
};

GXRenderModeObj GXPal528IntDf = {
    VI_TVMODE_PAL_INT,
    640,
    528,
    528,
    40,
    23,
    640,
    528,
    VI_XFBMODE_DF,
    GX_FALSE,
    GX_FALSE,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    8,
    8,
    10,
    12,
    10,
    8,
    8,
};

GXRenderModeObj GXEurgb60Hz480IntDf = {
    VI_TVMODE_EURGB60_INT,
    640,
    480,
    480,
    40,
    0,
    640,
    480,
    VI_XFBMODE_DF,
    GX_FALSE,
    GX_FALSE,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    6,
    8,
    8,
    10,
    12,
    10,
    8,
    8,
};

void GXSetDispCopySrc(u16 left, u16 top, u16 width, u16 height) {
    gx->cpDispSrc = 0;
    GX_BITFIELD_SET(gx->cpDispSrc, 22, 10, left);
    GX_BITFIELD_SET(gx->cpDispSrc, 12, 10, top);
    GX_BITFIELD_SET(gx->cpDispSrc, 0, 8, 73);

    gx->cpDispSize = 0;
    GX_BITFIELD_SET(gx->cpDispSize, 22, 10, width - 1);
    GX_BITFIELD_SET(gx->cpDispSize, 12, 10, height - 1);
    GX_BITFIELD_SET(gx->cpDispSize, 0, 8, 74);
}

void GXSetTexCopySrc(u16 left, u16 top, u16 width, u16 height) {
    gx->cpTexSrc = 0;
    GX_BITFIELD_SET(gx->cpTexSrc, 22, 10, left);
    GX_BITFIELD_SET(gx->cpTexSrc, 12, 10, top);
    GX_BITFIELD_SET(gx->cpTexSrc, 0, 8, 0x49);
    gx->cpTexSize = 0;
    GX_BITFIELD_SET(gx->cpTexSize, 22, 10, width - 1);
    GX_BITFIELD_SET(gx->cpTexSize, 12, 10, height - 1);
    GX_BITFIELD_SET(gx->cpTexSize, 0, 8, 0x4A);
}

void GXSetDispCopyDst(u16 arg0, u16 arg1) {
    s32 val = (s32)((arg0 << 1) & 0xFFFE) >> 5;
    gx->cpDispStride = 0;
    GX_BITFIELD_SET(gx->cpDispStride, 22, 10, val);
    GX_BITFIELD_SET(gx->cpDispStride, 0, 8, 0x4D);
}

void GXSetTexCopyDst(u16 wd, u16 ht, GXTexFmt fmt, GXBool mipmap) {
    u32 rowTiles, colTiles, cmpTiles;
    u32 peTexFmt;
    u32 peTexFmtH;

    gx->cpTexZ = GX_FALSE;

    peTexFmt = fmt & 0xF;
    if (fmt == GX_TF_Z16) {
        peTexFmt = 0xB;
    }

    switch (fmt) {
    case GX_TF_I4:
    case GX_TF_I8:
    case GX_TF_IA4:
    case GX_TF_IA8:
    case GX_CTF_YUVA8:
        SET_REG_FIELD(gx->cpTex, 2, 15, 3);
        break;
    default:
        SET_REG_FIELD(gx->cpTex, 2, 15, 2);
        break;
    }

    gx->cpTexZ = (fmt & 0x10) == 0x10;

    peTexFmtH = (peTexFmt & 8) >> 3;
    SET_REG_FIELD(gx->cpTex, 1, 3, peTexFmtH);
    peTexFmt = peTexFmt & 7;

    __GetImageTileCount(fmt, wd, ht, &rowTiles, &colTiles, &cmpTiles);

    gx->cpTexStride = 0;
    SET_REG_FIELD(gx->cpTexStride, 10, 0, rowTiles * cmpTiles);
    SET_REG_FIELD(gx->cpTexStride, 8, 24, 0x4D);

    SET_REG_FIELD(gx->cpTex, 1, 9, mipmap);
    SET_REG_FIELD(gx->cpTex, 3, 4, peTexFmt);
}

void GXSetDispCopyFrame2Field(GXCopyMode arg0) {
    GX_BITFIELD_SET(gx->cpDisp, 18, 2, arg0);
    GX_BITFIELD_SET(gx->cpTex, 18, 2, 0);
}

// clang-format off
#define INSERT_FIELD(reg, value, nbits, shift)                                 \
    (reg) = ((u32) (reg) & ~(((1 << (nbits)) - 1) << (shift))) |               \
            ((u32) (value) << (shift));
// clang-format on

void GXSetCopyClamp(GXFBClamp clamp) {
    u8 clmpB;
    u8 clmpT;

    clmpT = (clamp & GX_CLAMP_TOP) == GX_CLAMP_TOP;
    clmpB = (clamp & GX_CLAMP_BOTTOM) == GX_CLAMP_BOTTOM;

    SET_REG_FIELD(gx->cpDisp, 1, 0, clmpT);
    SET_REG_FIELD(gx->cpDisp, 1, 1, clmpB);

    SET_REG_FIELD(gx->cpTex, 1, 0, clmpT);
    SET_REG_FIELD(gx->cpTex, 1, 1, clmpB);
}

static u32 __GXGetNumXfbLines(u32 height, u32 scale) {
    u32 numLines;
    u32 actualHeight;
    u32 newScale;

    numLines = (height - 1) * 0x100;
    actualHeight = (numLines / scale) + 1;

    newScale = scale;

    if (newScale > 0x80 && newScale < 0x100) {
        while (newScale % 2 == 0) {
            newScale /= 2;
        }

        if (height % newScale == 0) {
            actualHeight++;
        }
    }

    if (actualHeight > 0x400) {
        actualHeight = 0x400;
    }

    return actualHeight;
}

u32 GXSetDispCopyYScale(f32 vertScale) {
    u32 scale;
    GXBool check;
    u32 height;
    u32 reg;

    scale = (u32)(256.0f / vertScale) & 0x1FF;
    check = (scale != 0x100);

    reg = 0;
    GX_SET_REG(reg, scale, 23, 31);
    GX_SET_REG(reg, 0x4E, 0, 7);
    GX_BP_LOAD_REG(reg);
    gx->bpSentNot = GX_FALSE;

    GX_SET_REG(gx->cpDisp, check, 21, 21);

    height = (gx->cpDispSize >> 10 & 0x3FF) + 1;

    return __GXGetNumXfbLines(height, scale);
}

void GXSetCopyClear(GXColor color, u32 clear_z) {
    u32 r6 = 0;
    GX_BITFIELD_SET(r6, 24, 8, color.r);
    GX_BITFIELD_SET(r6, 16, 8, color.a);
    GX_BITFIELD_SET(r6, 0, 8, 0x4f);
    GXFIFO.u8 = 0x61;
    GXFIFO.u32 = r6;

    r6 = 0;
    GX_BITFIELD_SET(r6, 24, 8, color.b);
    GX_BITFIELD_SET(r6, 16, 8, color.g);
    GX_BITFIELD_SET(r6, 0, 8, 0x50);
    GXFIFO.u8 = 0x61;
    GXFIFO.u32 = r6;

    r6 = 0;
    GX_BITFIELD_SET(r6, 8, 24, clear_z);
    GX_BITFIELD_SET(r6, 0, 8, 0x51);
    GXFIFO.u8 = 0x61;
    GXFIFO.u32 = r6;

    gx->bpSentNot = 0;
}

void GXSetCopyFilter(GXBool useAA, u8 samplePattern[12][2], GXBool doVertFilt, u8 vFilt[7]) {
    u32 msLoc[4];
    u32 coeff0, coeff1;

    if (useAA) {
        msLoc[0] = 0;
        SET_REG_FIELD(msLoc[0], 4, 0, samplePattern[0][0]);
        SET_REG_FIELD(msLoc[0], 4, 4, samplePattern[0][1]);
        SET_REG_FIELD(msLoc[0], 4, 8, samplePattern[1][0]);
        SET_REG_FIELD(msLoc[0], 4, 12, samplePattern[1][1]);
        SET_REG_FIELD(msLoc[0], 4, 16, samplePattern[2][0]);
        SET_REG_FIELD(msLoc[0], 4, 20, samplePattern[2][1]);
        SET_REG_FIELD(msLoc[0], 8, 24, 1);

        msLoc[1] = 0;
        SET_REG_FIELD(msLoc[1], 4, 0, samplePattern[3][0]);
        SET_REG_FIELD(msLoc[1], 4, 4, samplePattern[3][1]);
        SET_REG_FIELD(msLoc[1], 4, 8, samplePattern[4][0]);
        SET_REG_FIELD(msLoc[1], 4, 12, samplePattern[4][1]);
        SET_REG_FIELD(msLoc[1], 4, 16, samplePattern[5][0]);
        SET_REG_FIELD(msLoc[1], 4, 20, samplePattern[5][1]);
        SET_REG_FIELD(msLoc[1], 8, 24, 2);

        msLoc[2] = 0;
        SET_REG_FIELD(msLoc[2], 4, 0, samplePattern[6][0]);
        SET_REG_FIELD(msLoc[2], 4, 4, samplePattern[6][1]);
        SET_REG_FIELD(msLoc[2], 4, 8, samplePattern[7][0]);
        SET_REG_FIELD(msLoc[2], 4, 12, samplePattern[7][1]);
        SET_REG_FIELD(msLoc[2], 4, 16, samplePattern[8][0]);
        SET_REG_FIELD(msLoc[2], 4, 20, samplePattern[8][1]);
        SET_REG_FIELD(msLoc[2], 8, 24, 3);

        msLoc[3] = 0;
        SET_REG_FIELD(msLoc[3], 4, 0, samplePattern[9][0]);
        SET_REG_FIELD(msLoc[3], 4, 4, samplePattern[9][1]);
        SET_REG_FIELD(msLoc[3], 4, 8, samplePattern[10][0]);
        SET_REG_FIELD(msLoc[3], 4, 12, samplePattern[10][1]);
        SET_REG_FIELD(msLoc[3], 4, 16, samplePattern[11][0]);
        SET_REG_FIELD(msLoc[3], 4, 20, samplePattern[11][1]);
        SET_REG_FIELD(msLoc[3], 8, 24, 4);
    } else {
        msLoc[0] = 0x01666666;
        msLoc[1] = 0x02666666;
        msLoc[2] = 0x03666666;
        msLoc[3] = 0x04666666;
    }

    GX_WRITE_RAS_REG(msLoc[0]);
    GX_WRITE_RAS_REG(msLoc[1]);
    GX_WRITE_RAS_REG(msLoc[2]);
    GX_WRITE_RAS_REG(msLoc[3]);

    coeff0 = 0;
    SET_REG_FIELD(coeff0, 8, 24, 0x53);
    coeff1 = 0;
    SET_REG_FIELD(coeff1, 8, 24, 0x54);

    if (doVertFilt) {
        SET_REG_FIELD(coeff0, 6, 0, vFilt[0]);
        SET_REG_FIELD(coeff0, 6, 6, vFilt[1]);
        SET_REG_FIELD(coeff0, 6, 12, vFilt[2]);
        SET_REG_FIELD(coeff0, 6, 18, vFilt[3]);
        SET_REG_FIELD(coeff1, 6, 0, vFilt[4]);
        SET_REG_FIELD(coeff1, 6, 6, vFilt[5]);
        SET_REG_FIELD(coeff1, 6, 12, vFilt[6]);
    } else {
        SET_REG_FIELD(coeff0, 6, 0, 0);
        SET_REG_FIELD(coeff0, 6, 6, 0);
        SET_REG_FIELD(coeff0, 6, 12, 21);
        SET_REG_FIELD(coeff0, 6, 18, 22);
        SET_REG_FIELD(coeff1, 6, 0, 21);
        SET_REG_FIELD(coeff1, 6, 6, 0);
        SET_REG_FIELD(coeff1, 6, 12, 0);
    }

    GX_WRITE_RAS_REG(coeff0);
    GX_WRITE_RAS_REG(coeff1);

    gx->bpSentNot = GX_FALSE;
}

void GXSetDispCopyGamma(GXGamma gamma) {
    GX_BITFIELD_SET(gx->cpDisp, 23, 2, gamma);
}

void GXCopyDisp(void* dest, GXBool doClear) {
    u32 reg;
    u32 tempPeCtrl;
    u32 newDest;
    GXBool check;

    if (doClear) {
        reg = gx->zmode;
        GX_SET_REG(reg, 1, 31, 31);
        GX_SET_REG(reg, 7, 28, 30);
        GX_BP_LOAD_REG(reg);

        reg = gx->cmode0;
        SET_REG_FIELD(reg, 1, 0, 0);
        SET_REG_FIELD(reg, 1, 1, 0);
        GX_BP_LOAD_REG(reg);
    }

    check = GX_FALSE;
    if ((doClear || (gx->peCtrl & 0x7) == 3) && (gx->peCtrl >> 6 & 0x1) == 1) {
        check = GX_TRUE;
        tempPeCtrl = gx->peCtrl;
        SET_REG_FIELD(tempPeCtrl, 1, 6, 0);
        GX_BP_LOAD_REG(tempPeCtrl);
    }

    GX_BP_LOAD_REG(gx->cpDispSrc);
    GX_BP_LOAD_REG(gx->cpDispSize);
    GX_BP_LOAD_REG(gx->cpDispStride);

    newDest = (u32)dest & 0x3FFFFFFF;
    reg = 0;
    GX_SET_REG(reg, newDest >> 5, 11, 31);
    GX_SET_REG(reg, 0x4B, 0, 7);
    GX_BP_LOAD_REG(reg);

    GX_SET_REG(gx->cpDisp, doClear, 20, 20);
    GX_SET_REG(gx->cpDisp, 1, 17, 17);
    GX_SET_REG(gx->cpDisp, 0x52, 0, 7);
    GX_BP_LOAD_REG(gx->cpDisp);

    if (doClear) {
        GX_BP_LOAD_REG(gx->zmode);
        GX_BP_LOAD_REG(gx->cmode0);
    }

    if (check) {
        GX_BP_LOAD_REG(gx->peCtrl);
    }

    gx->bpSentNot = GX_FALSE;
}

void GXCopyTex(void* dest, GXBool doClear) {
    u32 reg;
    u32 reg2;
    u32 newDest;
    GXBool check;

    if (doClear) {
        reg = gx->zmode;
        GX_SET_REG(reg, 1, 31, 31);
        GX_SET_REG(reg, 7, 28, 30);
        GX_BP_LOAD_REG(reg);

        reg = gx->cmode0;
        SET_REG_FIELD(reg, 1, 0, 0);
        SET_REG_FIELD(reg, 1, 1, 0);
        GX_BP_LOAD_REG(reg);
    }

    check = GX_FALSE;
    reg2 = gx->peCtrl;
    if (gx->cpTexZ && (reg2 & 0x7) != 3) {
        check = GX_TRUE;
        GX_SET_REG(reg2, 3, 29, 31);
    }

    if ((doClear || (reg2 & 0x7) == 3) && (reg2 >> 6 & 0x1) == 1) {
        check = GX_TRUE;
        GX_SET_REG(reg2, 0, 25, 25);
    }

    if (check) {
        GX_BP_LOAD_REG(reg2);
    }

    GX_BP_LOAD_REG(gx->cpTexSrc);
    GX_BP_LOAD_REG(gx->cpTexSize);
    GX_BP_LOAD_REG(gx->cpTexStride);

    newDest = (u32)dest & 0x3FFFFFFF;
    reg = 0;
    GX_SET_REG(reg, newDest >> 5, 11, 31);
    GX_SET_REG(reg, 0x4B, 0, 7);
    GX_BP_LOAD_REG(reg);

    GX_SET_REG(gx->cpTex, doClear, 20, 20);
    GX_SET_REG(gx->cpTex, 0, 17, 17);
    GX_SET_REG(gx->cpTex, 0x52, 0, 7);
    GX_BP_LOAD_REG(gx->cpTex);

    if (doClear) {
        GX_BP_LOAD_REG(gx->zmode);
        GX_BP_LOAD_REG(gx->cmode0);
    }

    if (check) {
        GX_BP_LOAD_REG(gx->peCtrl);
    }

    gx->bpSentNot = GX_FALSE;
}

void GXClearBoundingBox(void) {
    GXFIFO.u8 = GX_LOAD_BP_REG;
    GXFIFO.u32 = 0x550003FF;
    GXFIFO.u8 = GX_LOAD_BP_REG;
    GXFIFO.u32 = 0x560003FF;
    set_x2(GX_FALSE);
}

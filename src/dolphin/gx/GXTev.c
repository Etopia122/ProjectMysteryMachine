#include "dolphin/gx/GXTev.h"
#include "dolphin/gx/GX.h"

/* ############################################################################################## */

void GXSetTevOp(GXTevStageID id, GXTevMode mode) {
    GXTevColorArg carg = GX_CC_RASC;
    GXTevAlphaArg aarg = GX_CA_RASA;

    if (id != GX_TEVSTAGE0) {
        carg = GX_CC_CPREV;
        aarg = GX_CA_APREV;
    }

    switch (mode) {
    case GX_MODULATE:
        GXSetTevColorIn(id, GX_CC_ZERO, GX_CC_TEXC, carg, GX_CC_ZERO);
        GXSetTevAlphaIn(id, GX_CA_ZERO, GX_CA_TEXA, aarg, GX_CA_ZERO);
        break;
    case GX_DECAL:
        GXSetTevColorIn(id, carg, GX_CC_TEXC, GX_CC_TEXA, GX_CC_ZERO);
        GXSetTevAlphaIn(id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, aarg);
        break;
    case GX_BLEND:
        GXSetTevColorIn(id, carg, GX_CC_ONE, GX_CC_TEXC, GX_CC_ZERO);
        GXSetTevAlphaIn(id, GX_CA_ZERO, GX_CA_TEXA, aarg, GX_CA_ZERO);
        break;
    case GX_REPLACE:
        GXSetTevColorIn(id, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
        GXSetTevAlphaIn(id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
        break;
    case GX_PASSCLR:
        GXSetTevColorIn(id, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, carg);
        GXSetTevAlphaIn(id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, aarg);
        break;
    }

    GXSetTevColorOp(id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaOp(id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
}

void GXSetTevColorIn(GXTevStageID stage, GXTevColorArg a, GXTevColorArg b, GXTevColorArg c,
                     GXTevColorArg d) {
    u32* tevReg = &gx->tevc[stage];

    SET_REG_FIELD(*tevReg, 4, 12, a);
    SET_REG_FIELD(*tevReg, 4, 8, b);
    SET_REG_FIELD(*tevReg, 4, 4, c);
    SET_REG_FIELD(*tevReg, 4, 0, d);

    GX_BP_LOAD_REG(*tevReg);
    gx->bpSentNot = GX_FALSE;
}

void GXSetTevAlphaIn(GXTevStageID stage, GXTevAlphaArg a, GXTevAlphaArg b, GXTevAlphaArg c,
                     GXTevAlphaArg d) {
    u32* tevReg = &gx->teva[stage];

    SET_REG_FIELD(*tevReg, 3, 13, a);
    SET_REG_FIELD(*tevReg, 3, 10, b);
    SET_REG_FIELD(*tevReg, 3, 7, c);
    SET_REG_FIELD(*tevReg, 3, 4, d);

    GX_BP_LOAD_REG(*tevReg);
    gx->bpSentNot = GX_FALSE;
}

void GXSetTevColorOp(GXTevStageID stage, GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp,
                     GXTevRegID out_reg) {
    u32* tevReg = &gx->tevc[stage];

    SET_REG_FIELD(*tevReg, 1, 18, op & 1);
    if (op <= GX_TEV_SUB) {
        SET_REG_FIELD(*tevReg, 2, 20, scale);
        SET_REG_FIELD(*tevReg, 2, 16, bias);
    } else {
        SET_REG_FIELD(*tevReg, 2, 20, (op >> 1) & 3);
        SET_REG_FIELD(*tevReg, 2, 16, 3);
    }
    SET_REG_FIELD(*tevReg, 1, 19, clamp);
    SET_REG_FIELD(*tevReg, 2, 22, out_reg);

    GX_BP_LOAD_REG(*tevReg);
    gx->bpSentNot = GX_FALSE;
}

void GXSetTevAlphaOp(GXTevStageID stage, GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp,
                     GXTevRegID out_reg) {
    u32* tevReg = &gx->teva[stage];

    SET_REG_FIELD(*tevReg, 1, 18, op & 1);
    if (op <= GX_TEV_SUB) {
        SET_REG_FIELD(*tevReg, 2, 20, scale);
        SET_REG_FIELD(*tevReg, 2, 16, bias);
    } else {
        SET_REG_FIELD(*tevReg, 2, 20, (op >> 1) & 3);
        SET_REG_FIELD(*tevReg, 2, 16, 3);
    }
    SET_REG_FIELD(*tevReg, 1, 19, clamp);
    SET_REG_FIELD(*tevReg, 2, 22, out_reg);

    GX_BP_LOAD_REG(*tevReg);
    gx->bpSentNot = GX_FALSE;
}

void GXSetTevColor(GXTevRegID reg, GXColor color) {
    u32 col = *(u32*)&color;
    u32 ra = (0xE0 + reg * 2) << 24;
    u32 bg = (0xE1 + reg * 2) << 24;

    GX_SET_REG(ra, col >> 24, 24, 31);
    GX_SET_REG(bg, col >> 8, 24, 31);
    GX_SET_REG(bg, col >> 16, 12, 19);
    GX_SET_REG(ra, col & 0xFF, 12, 19);

    GX_BP_LOAD_REG(ra);
    GX_BP_LOAD_REG(bg);
    GX_BP_LOAD_REG(bg);
    GX_BP_LOAD_REG(bg);

    gx->bpSentNot = GX_FALSE;
}

void GXSetTevColorS10(GXTevRegID id, GXColorS10 color) {
    u32 regRA;
    u32 regBG;

    regRA = 0;
    SET_REG_FIELD(regRA, 11, 0, color.r & 0x7FF);
    SET_REG_FIELD(regRA, 11, 12, color.a & 0x7FF);
    SET_REG_FIELD(regRA, 8, 24, 224 + id * 2);

    regBG = 0;
    SET_REG_FIELD(regBG, 11, 0, color.b & 0x7FF);
    SET_REG_FIELD(regBG, 11, 12, color.g & 0x7FF);
    SET_REG_FIELD(regBG, 8, 24, 225 + id * 2);

    GX_BP_LOAD_REG(regRA);
    GX_BP_LOAD_REG(regBG);
    GX_BP_LOAD_REG(regBG);
    GX_BP_LOAD_REG(regBG);

    gx->bpSentNot = GX_FALSE;
}

void GXSetTevKColor(GXTevKColorID id, GXColor color) {
    u32 regRA;
    u32 regBG;

    regRA = 0;
    SET_REG_FIELD(regRA, 8, 0, color.r);
    SET_REG_FIELD(regRA, 8, 12, color.a);
    SET_REG_FIELD(regRA, 4, 20, 8);
    SET_REG_FIELD(regRA, 8, 24, 224 + id * 2);

    regBG = 0;
    SET_REG_FIELD(regBG, 8, 0, color.b);
    SET_REG_FIELD(regBG, 8, 12, color.g);
    SET_REG_FIELD(regBG, 4, 20, 8);
    SET_REG_FIELD(regBG, 8, 24, 225 + id * 2);

    GX_BP_LOAD_REG(regRA);
    GX_BP_LOAD_REG(regBG);
    gx->bpSentNot = GX_FALSE;
}

void GXSetTevKColorSel(GXTevStageID stage, GXTevKColorSel sel) {
    u32* reg;

    reg = &gx->tevKsel[stage >> 1];

    if (stage & 1) {
        GX_SET_REG(*reg, sel, 13, 17);
    } else {
        GX_SET_REG(*reg, sel, 23, 27);
    }

    GX_BP_LOAD_REG(*reg);

    gx->bpSentNot = GX_FALSE;
}

void GXSetTevKAlphaSel(GXTevStageID stage, GXTevKAlphaSel sel) {
    u32* reg;

    reg = &gx->tevKsel[stage >> 1];

    if (stage & 1) {
        GX_SET_REG(*reg, sel, 8, 12);
    } else {
        GX_SET_REG(*reg, sel, 18, 22);
    }

    GX_BP_LOAD_REG(*reg);

    gx->bpSentNot = GX_FALSE;
}

void GXSetTevSwapMode(GXTevStageID stage, GXTevSwapSel rasSel, GXTevSwapSel texSel) {
    u32* reg = &gx->teva[stage];
    GX_SET_REG(*reg, rasSel, 30, 31);
    GX_SET_REG(*reg, texSel, 28, 29);

    GX_BP_LOAD_REG(*reg);

    gx->bpSentNot = GX_FALSE;
}

void GXSetTevSwapModeTable(GXTevSwapSel table, GXTevColorChan red, GXTevColorChan green,
                           GXTevColorChan blue, GXTevColorChan alpha) {
    u32* reg;

    reg = &gx->tevKsel[table << 1];
    GX_SET_REG(*reg, red, 30, 31);
    GX_SET_REG(*reg, green, 28, 29);

    GX_BP_LOAD_REG(*reg);

    reg = &gx->tevKsel[(table << 1) + 1];
    GX_SET_REG(*reg, blue, 30, 31);
    GX_SET_REG(*reg, alpha, 28, 29);

    GX_BP_LOAD_REG(*reg);

    gx->bpSentNot = GX_FALSE;
}

void GXSetAlphaCompare(GXCompare comp0, u8 ref0, GXAlphaOp op, GXCompare comp1, u8 ref1) {
    u32 reg;

    reg = 0;

    SET_REG_FIELD(reg, 8, 0, ref0);
    SET_REG_FIELD(reg, 8, 8, ref1);
    SET_REG_FIELD(reg, 3, 16, comp0);
    SET_REG_FIELD(reg, 3, 19, comp1);
    SET_REG_FIELD(reg, 2, 22, op);
    SET_REG_FIELD(reg, 8, 24, 0xF3);

    GX_BP_LOAD_REG(reg);
    gx->bpSentNot = GX_FALSE;
}

void GXSetZTexture(GXZTexOp op, GXTexFmt format, u32 bias) {
    u32 val1;
    u32 val2;
    u32 val3;

    val1 = 0;
    GX_SET_REG(val1, bias, 8, 31);
    GX_SET_REG(val1, 0xF4, 0, 7);

    val2 = 0;
    switch (format) {
    case GX_TF_Z8:
        val3 = 0;
        break;
    case GX_TF_Z16:
        val3 = 1;
        break;
    case GX_TF_Z24X8:
        val3 = 2;
        break;
    default:
        val3 = 2;
        break;
    }

    GX_SET_REG(val2, val3, 30, 31);
    GX_SET_REG(val2, op, 28, 29);
    GX_SET_REG(val2, 0xF5, 0, 7);

    GX_BP_LOAD_REG(val1);

    GX_BP_LOAD_REG(val2);

    gx->bpSentNot = GX_FALSE;
}

void GXSetTevOrder(GXTevStageID stage, GXTexCoordID coord, GXTexMapID map, GXChannelID color) {
    static int c2r[] = {0, 1, 0, 1, 0, 1, 7, 5, 6};

    u32* reg;
    u32 tempMap;
    u32 tempCoord;

    reg = &gx->tref[stage / 2];
    gx->texmapId[stage] = map;

    tempMap = map & ~0x100;
    tempMap = (tempMap >= GX_MAX_TEXMAP) ? GX_TEXMAP0 : tempMap;

    if (coord >= GX_MAXCOORD) {
        tempCoord = GX_TEXCOORD0;
        gx->tevTcEnab = gx->tevTcEnab & ~(1 << stage);
    } else {
        tempCoord = coord;
        gx->tevTcEnab = gx->tevTcEnab | (1 << stage);
    }

    if (stage & 1) {
        GX_SET_REG(*reg, tempMap, 17, 19);
        GX_SET_REG(*reg, tempCoord, 14, 16);
        GX_SET_REG(*reg, (color == GX_COLOR_NULL ? 7 : c2r[color]), 10, 12);
        GX_SET_REG(*reg, ((map != GX_TEXMAP_NULL) && !(map & 0x100)), 13, 13);

    } else {
        GX_SET_REG(*reg, tempMap, 29, 31);
        GX_SET_REG(*reg, tempCoord, 26, 28);
        GX_SET_REG(*reg, (color == GX_COLOR_NULL ? 7 : c2r[color]), 22, 24);
        GX_SET_REG(*reg, ((map != GX_TEXMAP_NULL) && !(map & 0x100)), 25, 25);
    }

    GX_BP_LOAD_REG(*reg);

    gx->bpSentNot = GX_FALSE;
    gx->dirtyState |= 1;
}

void GXSetNumTevStages(u8 count) {
    GX_SET_REG(gx->genMode, count - 1, 18, 21);

    gx->dirtyState |= 0x4;
}

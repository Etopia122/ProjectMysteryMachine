/**
 * GXLight.c
 * Description:
 */

#include "dolphin/gx/GXLight.h"
#include "dolphin/gx/GX.h"
#include "math.h"

void GXInitLightAttnA(GXLightObj* obj, f32 a0, f32 a1, f32 a2) {
    obj->a0 = a0;
    obj->a1 = a1;
    obj->a2 = a2;
}

void GXInitLightSpot(GXLightObj* obj, f32 cutoff, GXSpotFn spot_fn) {
    f32 a0, a1, a2;
    f32 r;
    f32 d;
    f32 cr;

    if (cutoff <= 0.0f || cutoff > 90.0f) {
        spot_fn = GX_SP_OFF;
    }

    r = (3.1415927f * cutoff) / 180.0f;
    cr = cosf(r);

    switch (spot_fn) {
    case GX_SP_FLAT:
        a0 = -1000.0f * cr;
        a1 = 1000.0f;
        a2 = 0.0f;
        break;
    case GX_SP_COS:
        a0 = -cr / (1.0f - cr);
        a1 = 1.0f / (1.0f - cr);
        a2 = 0.0f;
        break;
    case GX_SP_COS2:
        a0 = 0.0f;
        a1 = -cr / (1.0f - cr);
        a2 = 1.0f / (1.0f - cr);
        break;
    case GX_SP_SHARP:
        d = (1.0f - cr) * (1.0f - cr);
        a0 = (cr * (cr - 2.0f)) / d;
        a1 = 2.0f / d;
        a2 = -1.0f / d;
        break;
    case GX_SP_RING1:
        d = (1.0f - cr) * (1.0f - cr);
        a0 = (-4.0f * cr) / d;
        a1 = (4.0f * (1.0f + cr)) / d;
        a2 = -4.0f / d;
        break;
    case GX_SP_RING2:
        d = (1.0f - cr) * (1.0f - cr);
        a0 = 1.0f - ((2.0f * cr * cr) / d);
        a1 = (4.0f * cr) / d;
        a2 = -2.0f / d;
        break;
    case GX_SP_OFF:
    default:
        a0 = 1.0f;
        a1 = 0.0f;
        a2 = 0.0f;
        break;
    }

    obj->a0 = a0;
    obj->a1 = a1;
    obj->a2 = a2;
}

void GXInitLightDistAttn(GXLightObj* obj, f32 dist, f32 brightness, GXDistAttnFn dist_fn) {
    f32 a, b, c;

    if (dist < 0.0f) {
        dist_fn = GX_DA_OFF;
    }

    if (brightness <= 0.0f || brightness >= 1.0f) {
        dist_fn = GX_DA_OFF;
    }

    switch (dist_fn) {
    case GX_DA_GENTLE:
        a = 1.0f;
        b = (1.0f - brightness) / (brightness * dist);
        c = 0.0f;
        break;
    case GX_DA_MEDIUM:
        a = 1.0f;
        b = (0.5f * (1.0f - brightness)) / (brightness * dist);
        c = (0.5f * (1.0f - brightness)) / (dist * (brightness * dist));
        break;
    case GX_DA_STEEP:
        a = 1.0f;
        b = 0.0f;
        c = (1.0f - brightness) / (dist * (brightness * dist));
        break;
    case GX_DA_OFF:
    default:
        a = 1.0f;
        b = 0.0f;
        c = 0.0f;
        break;
    }

    obj->k0 = a;
    obj->k1 = b;
    obj->k2 = c;
}

void GXInitLightPos(GXLightObj* obj, f32 x, f32 y, f32 z) {
    obj->posX = x;
    obj->posY = y;
    obj->posZ = z;
}

void GXInitLightDir(GXLightObj* obj, f32 x, f32 y, f32 z) {
    obj->dirX = -x;
    obj->dirY = -y;
    obj->dirZ = -z;
}

void GXInitLightColor(GXLightObj* obj, GXColor color) {
    *(u32*)&obj->color = ((u32)color.r << 24) | ((u32)color.g << 16) | ((u32)color.b << 8) | color.a;
}

static inline void PushLight(const __REGISTER GXLightObj* lt_obj, __REGISTER void* dest) {
    __REGISTER u32 zero, color;
    __REGISTER f32 a0_a1, a2_k0, k1_k2;
    __REGISTER f32 px_py, pz_dx, dy_dz;
#ifdef __MWERKS__  // clang-format off
    asm {
        lwz     color, 12(lt_obj)
        xor     zero, zero, zero
        psq_l   a0_a1, 16(lt_obj), 0, 0
        psq_l   a2_k0, 24(lt_obj), 0, 0
        psq_l   k1_k2, 32(lt_obj), 0, 0
        psq_l   px_py, 40(lt_obj), 0, 0
        psq_l   pz_dx, 48(lt_obj), 0, 0
        psq_l   dy_dz, 56(lt_obj), 0, 0

        stw     zero,  0(dest)
        stw     zero,  0(dest)
        stw     zero,  0(dest)
        stw     color, 0(dest)
        psq_st  a0_a1, 0(dest), 0, 0
        psq_st  a2_k0, 0(dest), 0, 0
        psq_st  k1_k2, 0(dest), 0, 0
        psq_st  px_py, 0(dest), 0, 0
        psq_st  pz_dx, 0(dest), 0, 0
        psq_st  dy_dz, 0(dest), 0, 0
    }
#endif  // clang-format on
}

void GXLoadLightObjImm(GXLightObj* obj, GXLightID light) {
    u32 addr;
    u32 idx;

    switch (light) {
    case GX_LIGHT0:
        idx = 0;
        break;
    case GX_LIGHT1:
        idx = 1;
        break;
    case GX_LIGHT2:
        idx = 2;
        break;
    case GX_LIGHT3:
        idx = 3;
        break;
    case GX_LIGHT4:
        idx = 4;
        break;
    case GX_LIGHT5:
        idx = 5;
        break;
    case GX_LIGHT6:
        idx = 6;
        break;
    case GX_LIGHT7:
        idx = 7;
        break;
    default:
        idx = 0;
        break;
    }

    addr = 0x600 + idx * 0x10;

    GXFIFO.u8 = 0x10;
    GXFIFO.u32 = addr | (0x10 - 1) << 16;

    GXFIFO.u32 = 0;
    GXFIFO.u32 = 0;
    GXFIFO.u32 = 0;
    GXFIFO.u32 = *(u32*)&obj->color;
    GXFIFO.f32 = obj->a0;
    GXFIFO.f32 = obj->a1;
    GXFIFO.f32 = obj->a2;
    GXFIFO.f32 = obj->k0;
    GXFIFO.f32 = obj->k1;
    GXFIFO.f32 = obj->k2;
    GXFIFO.f32 = obj->posX;
    GXFIFO.f32 = obj->posY;
    GXFIFO.f32 = obj->posZ;
    GXFIFO.f32 = obj->dirX;
    GXFIFO.f32 = obj->dirY;
    GXFIFO.f32 = obj->dirZ;

    gx->bpSentNot = 1;
}

void GXSetChanAmbColor(GXChannelID chan, GXColor color) {
    u32 reg = 0;
    u32 colIdx;
    u32 alpha;

    switch (chan) {
    case GX_COLOR0:
        alpha = gx->ambColor[0] & 0xFF;
        SET_REG_FIELD(reg, 8, 0, alpha);
        SET_REG_FIELD(reg, 8, 8, color.b);
        SET_REG_FIELD(reg, 8, 16, color.g);
        SET_REG_FIELD(reg, 8, 24, color.r);
        colIdx = 0;
        break;
    case GX_COLOR1:
        alpha = gx->ambColor[1] & 0xFF;
        SET_REG_FIELD(reg, 8, 0, alpha);
        SET_REG_FIELD(reg, 8, 8, color.b);
        SET_REG_FIELD(reg, 8, 16, color.g);
        SET_REG_FIELD(reg, 8, 24, color.r);
        colIdx = 1;
        break;
    case GX_ALPHA0:
        reg = gx->ambColor[0];
        SET_REG_FIELD(reg, 8, 0, color.a);
        colIdx = 0;
        break;
    case GX_ALPHA1:
        reg = gx->ambColor[1];
        SET_REG_FIELD(reg, 8, 0, color.a);
        colIdx = 1;
        break;
    case GX_COLOR0A0:
        SET_REG_FIELD(reg, 8, 0, color.a);
        SET_REG_FIELD(reg, 8, 8, color.b);
        SET_REG_FIELD(reg, 8, 16, color.g);
        SET_REG_FIELD(reg, 8, 24, color.r);
        colIdx = 0;
        break;
    case GX_COLOR1A1:
        SET_REG_FIELD(reg, 8, 0, color.a);
        SET_REG_FIELD(reg, 8, 8, color.b);
        SET_REG_FIELD(reg, 8, 16, color.g);
        SET_REG_FIELD(reg, 8, 24, color.r);
        colIdx = 1;
        break;
    default:
        return;
    }

    GXFIFO.u8 = 0x10;
    GXFIFO.u32 = 0x100a + colIdx;
    GXFIFO.u32 = reg;
    gx->bpSentNot = 1;
    gx->ambColor[colIdx] = reg;
}

void GXSetChanMatColor(GXChannelID chan, GXColor color) {
    u32 reg = 0;
    u32 colIdx;
    u32 alpha;

    switch (chan) {
    case GX_COLOR0:
        alpha = gx->matColor[0] & 0xFF;
        SET_REG_FIELD(reg, 8, 0, alpha);
        SET_REG_FIELD(reg, 8, 8, color.b);
        SET_REG_FIELD(reg, 8, 16, color.g);
        SET_REG_FIELD(reg, 8, 24, color.r);
        colIdx = 0;
        break;
    case GX_COLOR1:
        alpha = gx->matColor[1] & 0xFF;
        SET_REG_FIELD(reg, 8, 0, alpha);
        SET_REG_FIELD(reg, 8, 8, color.b);
        SET_REG_FIELD(reg, 8, 16, color.g);
        SET_REG_FIELD(reg, 8, 24, color.r);
        colIdx = 1;
        break;
    case GX_ALPHA0:
        reg = gx->matColor[0];
        SET_REG_FIELD(reg, 8, 0, color.a);
        colIdx = 0;
        break;
    case GX_ALPHA1:
        reg = gx->matColor[1];
        SET_REG_FIELD(reg, 8, 0, color.a);
        colIdx = 1;
        break;
    case GX_COLOR0A0:
        SET_REG_FIELD(reg, 8, 0, color.a);
        SET_REG_FIELD(reg, 8, 8, color.b);
        SET_REG_FIELD(reg, 8, 16, color.g);
        SET_REG_FIELD(reg, 8, 24, color.r);
        colIdx = 0;
        break;
    case GX_COLOR1A1:
        SET_REG_FIELD(reg, 8, 0, color.a);
        SET_REG_FIELD(reg, 8, 8, color.b);
        SET_REG_FIELD(reg, 8, 16, color.g);
        SET_REG_FIELD(reg, 8, 24, color.r);
        colIdx = 1;
        break;
    default:
        return;
    }

    GXFIFO.u8 = 0x10;
    GXFIFO.u32 = 0x100c + colIdx;
    GXFIFO.u32 = reg;
    gx->bpSentNot = 1;
    gx->matColor[colIdx] = reg;
}

void GXSetNumChans(u8 nChans) {
    SET_REG_FIELD(gx->genMode, 3, 4, nChans);
    GXFIFO.u8 = 0x10;
    GXFIFO.u32 = 0x1009;
    GXFIFO.u32 = nChans;
    gx->dirtyState |= GX_DIRTY_GEN_MODE;
}

void GXSetChanCtrl(GXChannelID chan, GXBool enable, GXColorSrc amb_src, GXColorSrc mat_src,
                   u32 light_mask, GXDiffuseFn diff_fn, GXAttnFn attn_fn) {
    u32 reg;
    u32 idx;

    if (chan == GX_COLOR0A0) {
        idx = 0;
    } else if (chan == GX_COLOR1A1) {
        idx = 1;
    } else {
        idx = chan;
    }

    reg = 0;
    SET_REG_FIELD(reg, 1, 1, enable);
    SET_REG_FIELD(reg, 1, 0, mat_src);
    SET_REG_FIELD(reg, 1, 6, amb_src);
    SET_REG_FIELD(reg, 1, 2, (light_mask & GX_LIGHT0) != 0);
    SET_REG_FIELD(reg, 1, 3, (light_mask & GX_LIGHT1) != 0);
    SET_REG_FIELD(reg, 1, 4, (light_mask & GX_LIGHT2) != 0);
    SET_REG_FIELD(reg, 1, 5, (light_mask & GX_LIGHT3) != 0);
    SET_REG_FIELD(reg, 1, 11, (light_mask & GX_LIGHT4) != 0);
    SET_REG_FIELD(reg, 1, 12, (light_mask & GX_LIGHT5) != 0);
    SET_REG_FIELD(reg, 1, 13, (light_mask & GX_LIGHT6) != 0);
    SET_REG_FIELD(reg, 1, 14, (light_mask & GX_LIGHT7) != 0);

    if (attn_fn == GX_AF_SPEC) {
        diff_fn = GX_DF_NONE;
    }
    SET_REG_FIELD(reg, 2, 7, diff_fn);
    SET_REG_FIELD(reg, 1, 9, attn_fn != GX_AF_NONE);
    SET_REG_FIELD(reg, 1, 10, attn_fn != GX_AF_SPEC);

    GXFIFO.u8 = 0x10;
    GXFIFO.u32 = 0x100e + idx;
    GXFIFO.u32 = reg;
    gx->bpSentNot = GX_TRUE;

    if (chan == GX_COLOR0A0) {
        GXFIFO.u8 = 0x10;
        GXFIFO.u32 = 0x1010;
        GXFIFO.u32 = reg;
    } else if (chan == GX_COLOR1A1) {
        GXFIFO.u8 = 0x10;
        GXFIFO.u32 = 0x1011;
        GXFIFO.u32 = reg;
    }
}

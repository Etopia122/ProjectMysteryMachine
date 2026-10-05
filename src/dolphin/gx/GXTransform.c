#include "dolphin/gx/GXTransform.h"
#include "dolphin/gx/GX.h"
#include "dolphin/gx/GXInit.h"

// Older SDK revision: no GXProject/GXGetViewportv/GXGetScissor, viewport
// computed directly in GXSetViewportJitter, and no z scale/offset.

void __GXSetRange(f32 nearz, f32 fgSideX);

// The original file had an unused static helper here that pooled 0.5f ahead of
// GXGetProjectionv's int->float constant. It is never emitted, but it keeps the
// .sdata2 constant order (0.5, conversion double, 342, 2^24-1) matching.
static f32 __GXHalf(f32 x) {
    return x * 0.5f;
}

void GXSetProjection(const Mtx44 mtx, GXProjectionType type) {
    gx->projType = type;

    gx->projMtx[0] = mtx[0][0];
    gx->projMtx[2] = mtx[1][1];
    gx->projMtx[4] = mtx[2][2];
    gx->projMtx[5] = mtx[2][3];

    if (type == GX_ORTHOGRAPHIC) {
        gx->projMtx[1] = mtx[0][3];
        gx->projMtx[3] = mtx[1][3];
    } else {
        gx->projMtx[1] = mtx[0][2];
        gx->projMtx[3] = mtx[1][2];
    }

    GXFIFO.u8 = 0x10;
    GXFIFO.u32 = 0x00061020;
    GXFIFO.f32 = gx->projMtx[0];
    GXFIFO.f32 = gx->projMtx[1];
    GXFIFO.f32 = gx->projMtx[2];
    GXFIFO.f32 = gx->projMtx[3];
    GXFIFO.f32 = gx->projMtx[4];
    GXFIFO.f32 = gx->projMtx[5];
    GXFIFO.u32 = gx->projType;

    gx->bpSentNot = GX_TRUE;
}

void GXSetProjectionv(f32* ptr) {
    gx->projType = ptr[0];
    gx->projMtx[0] = ptr[1];
    gx->projMtx[1] = ptr[2];
    gx->projMtx[2] = ptr[3];
    gx->projMtx[3] = ptr[4];
    gx->projMtx[4] = ptr[5];
    gx->projMtx[5] = ptr[6];

    GXFIFO.u8 = 0x10;
    GXFIFO.u32 = 0x00061020;
    GXFIFO.f32 = gx->projMtx[0];
    GXFIFO.f32 = gx->projMtx[1];
    GXFIFO.f32 = gx->projMtx[2];
    GXFIFO.f32 = gx->projMtx[3];
    GXFIFO.f32 = gx->projMtx[4];
    GXFIFO.f32 = gx->projMtx[5];
    GXFIFO.u32 = gx->projType;

    gx->bpSentNot = GX_TRUE;
}

void GXGetProjectionv(f32* ptr) {
    ptr[0] = gx->projType;
    ptr[1] = gx->projMtx[0];
    ptr[2] = gx->projMtx[1];
    ptr[3] = gx->projMtx[2];
    ptr[4] = gx->projMtx[3];
    ptr[5] = gx->projMtx[4];
    ptr[6] = gx->projMtx[5];
}

static asm void WriteMTXPS4x3(const __REGISTER Mtx src, __REGISTER volatile void* dst) {
    // clang-format off
    nofralloc
    psq_l  f0,  0(src), 0, 0
    psq_l  f1,  8(src), 0, 0
    psq_l  f2, 16(src), 0, 0
    psq_l  f3, 24(src), 0, 0
    psq_l  f4, 32(src), 0, 0
    psq_l  f5, 40(src), 0, 0
    psq_st f0, 0(dst),  0, 0
    psq_st f1, 0(dst),  0, 0
    psq_st f2, 0(dst),  0, 0
    psq_st f3, 0(dst),  0, 0
    psq_st f4, 0(dst),  0, 0
    psq_st f5, 0(dst),  0, 0
    blr
    // clang-format on
}

static asm void WriteMTXPS3x3from3x4(const __REGISTER Mtx src, __REGISTER volatile void* dst) {
    // clang-format off
    nofralloc
    psq_l  f0,  0(src), 0, 0
    lfs    f1,  8(src)
    psq_l  f2, 16(src), 0, 0
    lfs    f3, 24(src)
    psq_l  f4, 32(src), 0, 0
    lfs    f5, 40(src)
    psq_st f0, 0(dst),  0, 0
    stfs   f1, 0(dst)
    psq_st f2, 0(dst),  0, 0
    stfs   f3, 0(dst)
    psq_st f4, 0(dst),  0, 0
    stfs   f5, 0(dst)
    blr
    // clang-format on
}

static asm void WriteMTXPS4x2(const __REGISTER Mtx src, __REGISTER volatile void* dst) {
    // clang-format off
    nofralloc
    psq_l  f0,  0(src), 0, 0
    psq_l  f1,  8(src), 0, 0
    psq_l  f2, 16(src), 0, 0
    psq_l  f3, 24(src), 0, 0
    psq_st f0, 0(dst),  0, 0
    psq_st f1, 0(dst),  0, 0
    psq_st f2, 0(dst),  0, 0
    psq_st f3, 0(dst),  0, 0
    blr
    // clang-format on
}

void GXLoadPosMtxImm(Mtx mtx, u32 id) {
    u32 reg;
    u32 addr;

    addr = id * 4;
    reg = addr | ((12 - 1) << 16);

    GXFIFO.u8 = 0x10;
    GXFIFO.u32 = reg;
    WriteMTXPS4x3(mtx, &GXFIFO);
}

void GXLoadNrmMtxImm(Mtx mtx, u32 id) {
    u32 reg;
    u32 addr;

    addr = id * 3 + 0x400;
    reg = addr | ((9 - 1) << 16);

    GXFIFO.u8 = 0x10;
    GXFIFO.u32 = reg;
    WriteMTXPS3x3from3x4(mtx, &GXFIFO);
}

void GXSetCurrentMtx(u32 id) {
    SET_REG_FIELD(gx->matIdxA, 6, 0, id);
    __GXSetMatrixIndex(GX_VA_PNMTXIDX);
}

void GXLoadTexMtxImm(const Mtx mtx, u32 id, GXTexMtxType type) {
    u32 reg;
    u32 addr;
    u32 count;

    if (id >= GX_PTTEXMTX0) {
        addr = (id - GX_PTTEXMTX0) * 4 + 0x500;
    } else {
        addr = id * 4;
    }

    if (type == GX_MTX2x4) {
        count = 8;
    } else {
        count = 12;
    }

    reg = addr | ((count - 1) << 16);

    GXFIFO.u8 = 0x10;
    GXFIFO.u32 = reg;

    if (type == GX_MTX3x4) {
        WriteMTXPS4x3(mtx, &GXFIFO);
    } else {
        WriteMTXPS4x2(mtx, &GXFIFO);
    }
}

void GXSetViewportJitter(f32 left, f32 top, f32 wd, f32 ht, f32 nearz, f32 farz, u32 field) {
    f32 sx, sy, sz, ox, oy, oz;
    f32 zmin, zmax;

    if (field == 0) {
        top -= 0.5f;
    }

    sx = wd / 2.0f;
    sy = -ht / 2.0f;
    ox = 342.0f + (left + (wd / 2.0f));
    oy = 342.0f + (top + (ht / 2.0f));
    zmin = 1.6777215e7f * nearz;
    zmax = 1.6777215e7f * farz;
    sz = zmax - zmin;
    oz = zmax;

    gx->vpLeft = left;
    gx->vpTop = top;
    gx->vpWd = wd;
    gx->vpHt = ht;
    gx->vpNearz = nearz;
    gx->vpFarz = farz;

    if (gx->fgRange) {
        __GXSetRange(nearz, gx->fgSideX);
    }

    GXFIFO.u8 = 0x10;
    GXFIFO.u32 = 0x0005101A;
    GXFIFO.f32 = sx;
    GXFIFO.f32 = sy;
    GXFIFO.f32 = sz;
    GXFIFO.f32 = ox;
    GXFIFO.f32 = oy;
    GXFIFO.f32 = oz;

    gx->bpSentNot = GX_TRUE;
}

void GXSetViewport(f32 left, f32 top, f32 wd, f32 ht, f32 nearz, f32 farz) {
    GXSetViewportJitter(left, top, wd, ht, nearz, farz, 1);
}

void GXSetScissor(u32 left, u32 top, u32 wd, u32 ht) {
    u32 tp, lf, bm, rt;

    tp = top + 342;
    lf = left + 342;
    bm = tp + ht - 1;
    rt = lf + wd - 1;

    SET_REG_FIELD(gx->suScis0, 11, 0, tp);
    SET_REG_FIELD(gx->suScis0, 11, 12, lf);
    SET_REG_FIELD(gx->suScis1, 11, 0, bm);
    SET_REG_FIELD(gx->suScis1, 11, 12, rt);

    GX_WRITE_RAS_REG(gx->suScis0);
    GX_WRITE_RAS_REG(gx->suScis1);
    gx->bpSentNot = GX_FALSE;
}

void GXSetScissorBoxOffset(s32 x_off, s32 y_off) {
    u32 reg = 0;
    u32 hx;
    u32 hy;

    hx = (u32)(x_off + 342) >> 1;
    hy = (u32)(y_off + 342) >> 1;

    SET_REG_FIELD(reg, 10, 0, hx);
    SET_REG_FIELD(reg, 10, 10, hy);
    SET_REG_FIELD(reg, 8, 24, 0x59);

    GX_WRITE_RAS_REG(reg);
    gx->bpSentNot = GX_FALSE;
}

void GXSetClipMode(GXClipMode mode) {
    GXFIFO.u8 = 0x10;
    GXFIFO.u32 = 0x1005;
    GXFIFO.u32 = mode;
    gx->bpSentNot = GX_TRUE;
}

void __GXSetMatrixIndex(GXAttr index) {
    if (index < GX_VA_TEX4MTXIDX) {
        GXFIFO.u8 = 0x8;
        GXFIFO.u8 = 0x30;
        GXFIFO.u32 = gx->matIdxA;
        GXFIFO.u8 = 0x10;
        GXFIFO.u32 = 0x1018;
        GXFIFO.u32 = gx->matIdxA;
    } else {
        GXFIFO.u8 = 0x8;
        GXFIFO.u8 = 0x40;
        GXFIFO.u32 = gx->matIdxB;
        GXFIFO.u8 = 0x10;
        GXFIFO.u32 = 0x1019;
        GXFIFO.u32 = gx->matIdxB;
    }

    gx->bpSentNot = GX_TRUE;
}

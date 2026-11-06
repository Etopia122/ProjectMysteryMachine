#ifndef RWSDK_RWGX_H
#define RWSDK_RWGX_H

/*
 * The slice of the Dolphin GX API the RenderWare GameCube driver uses, kept
 * separate from the SDK headers so RenderWare units build with RW's own types.
 */

#include "rwsdk/rwcore.h"

typedef enum {
    GX_POINTS = 0xB8,
    GX_LINES = 0xA8,
    GX_LINESTRIP = 0xB0,
    GX_TRIANGLES = 0x90,
    GX_TRIANGLESTRIP = 0x98,
    GX_TRIANGLEFAN = 0xA0,
    GX_QUADS = 0x80,
} GXPrimitive;

#define GX_VTXFMT0 0

#define GX_NONE 0
#define GX_DIRECT 1

#define GX_VA_POS 9
#define GX_VA_NRM 10
#define GX_VA_CLR0 11
#define GX_VA_TEX0 13

#define GX_POS_XYZ 1
#define GX_CLR_RGBA 1
#define GX_TEX_ST 1
#define GX_F32 4
#define GX_RGBA8 5

#define GX_COLOR0A0 4
#define GX_SRC_REG 0
#define GX_SRC_VTX 1
#define GX_LIGHT_NULL 0
#define GX_DF_NONE 0
#define GX_AF_NONE 2

#define GX_TEVSTAGE0 0
#define GX_MODULATE 0
#define GX_PASSCLR 4
#define GX_TEXCOORD0 0
#define GX_TEXCOORD_NULL 0xFF
#define GX_TEXMAP0 0
#define GX_TEXMAP_NULL 0xFF
#define GX_TG_MTX2x4 1
#define GX_TG_TEX0 4
#define GX_IDENTITY 60
#define GX_PTIDENTITY 125

#define GX_FALSE 0
#define GX_TRUE 1

void GXClearVtxDesc(void);
void GXSetVtxDesc(RwInt32 attr, RwInt32 type);
void GXSetVtxAttrFmt(RwInt32 vtxfmt, RwInt32 attr, RwInt32 cnt, RwInt32 type, RwUInt8 frac);
void GXSetNumTevStages(RwUInt8 nStages);
void GXSetNumChans(RwUInt8 nChans);
void GXSetChanCtrl(RwInt32 chan, RwUInt8 enable, RwInt32 ambSrc, RwInt32 matSrc, RwUInt32 lightMask, RwInt32 diffFn,
                   RwInt32 attnFn);
void GXSetTevOp(RwInt32 id, RwInt32 mode);
void GXSetNumTexGens(RwUInt8 nTexGens);
void GXSetTexCoordGen2(RwInt32 dstCoord, RwInt32 func, RwInt32 srcParam, RwUInt32 mtx, RwUInt8 normalize,
                       RwUInt32 postMtx);
void GXSetTevOrder(RwInt32 stage, RwInt32 coord, RwInt32 map, RwInt32 color);
void GXBegin(GXPrimitive type, RwInt32 vtxfmt, RwUInt16 nverts);

typedef union {
    RwUInt8 u8;
    RwUInt16 u16;
    RwUInt32 u32;
    RwInt8 s8;
    RwInt16 s16;
    RwInt32 s32;
    RwReal f32;
} rwGXWGPipe;

volatile rwGXWGPipe GXWGFifo : 0xCC008000;

static inline void GXPosition3f32(const RwReal x, const RwReal y, const RwReal z)
{
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

static inline void GXColor4u8(const RwUInt8 r, const RwUInt8 g, const RwUInt8 b, const RwUInt8 a)
{
    GXWGFifo.u8 = r;
    GXWGFifo.u8 = g;
    GXWGFifo.u8 = b;
    GXWGFifo.u8 = a;
}

static inline void GXTexCoord2f32(const RwReal s, const RwReal t)
{
    GXWGFifo.f32 = s;
    GXWGFifo.f32 = t;
}

static inline void GXEnd(void)
{
}

#endif

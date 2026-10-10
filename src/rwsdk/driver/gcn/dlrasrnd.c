/*
 * GameCube device layer raster rendering and clearing: raster to raster
 * blits are drawn as a single textured quad with an orthographic projection
 * onto the current context raster, and camera clears draw a flat quad. Plain
 * memory rasters are cleared directly in memory.
 */

#include "rwsdk/rwcore.h"
#include "rwsdk/rwgx.h"

#define rwPLUGIN_ID 1

#define E_RW_DLNOTSUPPORTED 2
#define E_RW_INVRASTERDEPTH 0x8000000C
#define E_RW_INVRASTERFORMAT 0x8000000D

#define rwRASTERTYPENORMAL 0
#define rwRASTERTYPEZBUFFER 1
#define rwRASTERTYPECAMERA 2
#define rwRASTERTYPETEXTURE 4
#define rwRASTERTYPECAMERATEXTURE 5

#define rwRASTERLOCKWRITE 0x01
#define rwRASTERLOCKREAD 0x02
#define rwRASTERLOCKNOFETCH 0x08
#define rwRASTERPIXELLOCKED 0x04

#define rwCAMERACLEARIMAGE 0x1
#define rwCAMERACLEARZ 0x2

#define rwRENDERSTATETEXTURERASTER 1
#define rwRENDERSTATETEXTUREADDRESS 2
#define rwRENDERSTATETEXTUREADDRESSU 3
#define rwRENDERSTATETEXTUREADDRESSV 4
#define rwRENDERSTATETEXTUREFILTER 9
#define rwRENDERSTATEFOGENABLE 14

#define rwFILTERNEAREST 1
#define rwFILTERLINEAR 2
#define rwTEXTUREADDRESSCLAMP 3

#define rwDLRASTERHASALPHA 0x01

#define GX_ORTHOGRAPHIC 1.0f
#define GX_PNMTX0 0

#define GX_ALWAYS 7
#define GX_AOP_AND 0
#define GX_CULL_NONE 0
#define GX_BM_BLEND 1
#define GX_BL_ZERO 0
#define GX_BL_ONE 1
#define GX_BL_SRCALPHA 4
#define GX_BL_INVSRCALPHA 5
#define GX_LO_CLEAR 0
#define GX_REPLACE 3
#define GX_COLOR_NULL 0xFF
#define GX_PF_RGB8_Z24 0
#define GX_PF_RGBA6_Z24 1
#define GX_ZC_LINEAR 0

#define DLRASTERRENDERZ 0.99999994f

typedef struct RwRect {
    /* 0x00 */ RwInt32 x;
    /* 0x04 */ RwInt32 y;
    /* 0x08 */ RwInt32 w;
    /* 0x0C */ RwInt32 h;
} RwRect;

typedef struct GXColor {
    RwUInt8 r;
    RwUInt8 g;
    RwUInt8 b;
    RwUInt8 a;
} GXColor;

typedef struct GXRenderModeObj {
    /* 0x00 */ RwUInt32 viTVmode;
    /* 0x04 */ RwUInt16 fbWidth;
    /* 0x06 */ RwUInt16 efbHeight;
    /* 0x08 */ RwUInt16 xfbHeight;
    /* 0x0A */ RwUInt16 viXOrigin;
    /* 0x0C */ RwUInt16 viYOrigin;
    /* 0x0E */ RwUInt16 viWidth;
    /* 0x10 */ RwUInt16 viHeight;
    /* 0x14 */ RwUInt32 xFBmode;
    /* 0x18 */ RwUInt8 field_rendering;
} GXRenderModeObj;

typedef struct RwDlRasterExt {
    /* 0x00 */ RwUInt32 tlutObj[3];
    /* 0x0C */ RwUInt32 format;
    /* 0x10 */ RwUInt32 tlutFormat;
    /* 0x14 */ RwUInt32 flags;
} RwDlRasterExt;

#define RASTEREXTFROMRASTER(raster) ((RwDlRasterExt*)((RwUInt8*)(raster) + _rwDlRasterExtOffset))

typedef struct RwDlRenderState {
    /* 0x00 */ RwBool zWriteEnable;
    /* 0x04 */ RwBool zTestEnable;
    /* 0x08 */ RwInt32 zFunc;
    /* 0x0C */ RwInt32 pad0C;
    /* 0x10 */ RwInt32 cullMode;
    /* 0x14 */ RwBool fogEnable;
    /* 0x18 */ RwInt32 fogType;
    /* 0x1C */ RwUInt32 fogColor;
    /* 0x20 */ GXColor gxFogColor;
    /* 0x24 */ RwReal fogDensity;
    /* 0x28 */ RwInt32 srcBlend;
    /* 0x2C */ RwInt32 destBlend;
    /* 0x30 */ RwInt32 gxSrcBlend;
    /* 0x34 */ RwInt32 gxDestBlend;
    /* 0x38 */ RwInt32 alphaTestMode;
    /* 0x3C */ RwUInt8 alphaRef0;
    /* 0x3D */ RwUInt8 alphaRef1;
} RwDlRenderState;

extern RwInt32 _rwDlRasterExtOffset;
extern RwDlRenderState lbl_8025E348;
extern GXRenderModeObj* lbl_8025F860;
extern RwInt32 lbl_8025F85C;
extern RwInt32 lbl_8025F808;
extern RwInt32 lbl_8025F800;
extern RwInt32 lbl_8025F7FC;
extern RwInt32 lbl_8025E7F0;

void* memset(void* dst, int c, unsigned long n);
RwUInt8* RwRasterLock(RwRaster* raster, RwUInt8 level, RwInt32 lockMode);
RwRaster* RwRasterUnlock(RwRaster* raster);
RwRGBA* RwRGBASetFromPixel(RwRGBA* rgbOut, RwUInt32 pixelValue, RwInt32 rasterFormat);
RwBool _rwDlGetRenderState(RwInt32 state, void* param);
RwBool _rwDlSetRenderState(RwInt32 state, void* param);
RwBool _rwDlRenderStateFogEnable(RwBool enable);
void _rwDlRenderStateSetZCompLoc(void);
void _rwDlTextureRasterFlush(RwInt32 mode);

RwUInt32 VIGetNextField(void);
void GXSetViewport(RwReal left, RwReal top, RwReal wd, RwReal ht, RwReal nearz, RwReal farz);
void GXSetViewportJitter(RwReal left, RwReal top, RwReal wd, RwReal ht, RwReal nearz, RwReal farz, RwUInt32 field);
void GXSetScissor(RwUInt32 left, RwUInt32 top, RwUInt32 wd, RwUInt32 ht);
void GXSetScissorBoxOffset(RwInt32 xOffset, RwInt32 yOffset);
void GXSetProjectionv(const RwReal* ptr);
void GXLoadPosMtxImm(RwReal mtx[3][4], RwUInt32 id);
void GXSetCurrentMtx(RwUInt32 id);
void GXSetPixelFmt(RwInt32 pixFmt, RwInt32 zFmt);
void GXSetZMode(RwUInt8 compareEnable, RwInt32 func, RwUInt8 updateEnable);
void GXSetZCompLoc(RwUInt8 beforeTex);
void GXSetBlendMode(RwInt32 type, RwInt32 srcFactor, RwInt32 dstFactor, RwInt32 op);
void GXSetCullMode(RwInt32 mode);
void GXSetAlphaCompare(RwInt32 comp0, RwUInt8 ref0, RwInt32 op, RwInt32 comp1, RwUInt8 ref1);
void GXSetChanMatColor(RwInt32 chan, GXColor color);
void GXSetColorUpdate(RwUInt8 updateEnable);

RwInt32 lbl_8025F884;
RwRaster* lbl_8025F880;

static inline void DlRasterSetViewport(void)
{
    GXRenderModeObj* rm = lbl_8025F860;

    if (rm->field_rendering) {
        GXSetViewportJitter(0.0f, 0.0f, rm->fbWidth, rm->xfbHeight, 0.0f, 1.0f, VIGetNextField() ^ 1);
    } else {
        GXSetViewport(0.0f, 0.0f, rm->fbWidth, rm->xfbHeight, 0.0f, 1.0f);
    }
}

static inline void DlRasterRestoreRenderState(void)
{
    GXSetZMode(GX_TRUE, lbl_8025E348.zFunc, lbl_8025E348.zWriteEnable);
    GXSetBlendMode(GX_BM_BLEND, lbl_8025E348.gxSrcBlend, lbl_8025E348.gxDestBlend, GX_LO_CLEAR);
    GXSetCullMode(lbl_8025E348.cullMode - 1);
}

void _rwDlRasterRenderQuadInit(RwRect* rect)
{
    static RwReal projection[7] = { GX_ORTHOGRAPHIC, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f };
    static RwReal posMtx[3][4] = {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
    };

    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXSetNumChans(0);

    DlRasterSetViewport();

    if (!lbl_8025F808) {
        GXSetScissor(lbl_8025F880->nOffsetX + rect->x, lbl_8025F880->nOffsetY + rect->y, rect->w, rect->h);
    } else if (lbl_8025E7F0) {
        RwRaster* dst = lbl_8025F880;
        RwInt32 top = (dst->nOffsetY << 1) + rect->y;

        if (rect->h + top <= lbl_8025F85C + 2) {
            GXSetScissor(dst->nOffsetX + rect->x, top, rect->w, rect->h);
        } else if (top > lbl_8025F85C + 2) {
            GXSetScissor(0, 0, lbl_8025F860->fbWidth, lbl_8025F85C + 2);
        } else {
            GXSetScissor(dst->nOffsetX + rect->x, top, rect->w, (lbl_8025F85C + 2) - top);
        }
        GXSetScissorBoxOffset(0, 0);
    } else {
        RwRaster* dst = lbl_8025F880;
        RwInt32 top = (dst->nOffsetY << 1) + rect->y;

        if (top >= lbl_8025F85C - 2) {
            GXSetScissor(dst->nOffsetX + rect->x, top, rect->w, rect->h);
        } else {
            RwInt32 bottom = rect->h + top;

            if (bottom < lbl_8025F85C - 2) {
                GXSetScissor(0, lbl_8025F85C - 2, lbl_8025F860->fbWidth, lbl_8025F85C + 2);
            } else {
                GXSetScissor(dst->nOffsetX + rect->x, lbl_8025F85C - 2, rect->w, bottom - (lbl_8025F85C - 2));
            }
        }
        GXSetScissorBoxOffset(0, lbl_8025F85C - 2);
    }

    projection[1] = 2.0f / (RwReal)(lbl_8025F860->fbWidth - 1);
    projection[3] = -2.0f / (RwReal)(lbl_8025F860->xfbHeight - 1);
    GXSetProjectionv(projection);
    GXLoadPosMtxImm(posMtx, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
}

void _rwDlRasterRenderQuad(RwRaster* raster, RwRect* rect, RwBool scaled, RwBool blend)
{
    RwRaster* parent = raster->parent;
    RwBool oldFogEnable;
    RwInt32 oldFilter;
    RwInt32 oldAddressU;
    RwInt32 oldAddressV;
    RwRaster* oldRaster;

    _rwDlGetRenderState(rwRENDERSTATEFOGENABLE, &oldFogEnable);
    _rwDlGetRenderState(rwRENDERSTATETEXTUREFILTER, &oldFilter);
    _rwDlGetRenderState(rwRENDERSTATETEXTUREADDRESSU, &oldAddressU);
    _rwDlGetRenderState(rwRENDERSTATETEXTUREADDRESSV, &oldAddressV);
    _rwDlGetRenderState(rwRENDERSTATETEXTURERASTER, &oldRaster);

    GXSetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
    GXSetCullMode(GX_CULL_NONE);
    _rwDlTextureRasterFlush(1);
    if (blend) {
        GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    } else {
        GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
    }

    _rwDlSetRenderState(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    _rwDlSetRenderState(rwRENDERSTATETEXTUREFILTER, scaled ? (void*)rwFILTERLINEAR : (void*)rwFILTERNEAREST);
    _rwDlSetRenderState(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);
    _rwDlSetRenderState(rwRENDERSTATETEXTURERASTER, parent);
    _rwDlRenderStateSetZCompLoc();

    _rwDlRasterRenderQuadInit(rect);

    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    if (scaled) {
        RwRaster* dst = lbl_8025F880;
        RwReal invWidth = 1.0f / parent->width;
        RwReal invHeight = 1.0f / parent->height;

        GXPosition3f32(dst->nOffsetX + rect->x, dst->nOffsetY + rect->y, DLRASTERRENDERZ);
        GXTexCoord2f32(raster->nOffsetX * invWidth, raster->nOffsetY * invHeight);
        GXPosition3f32(dst->nOffsetX + rect->x, dst->nOffsetY + rect->y + rect->h, DLRASTERRENDERZ);
        GXTexCoord2f32(raster->nOffsetX * invWidth, (raster->nOffsetY + raster->height) * invHeight);
        GXPosition3f32(dst->nOffsetX + rect->x + rect->w, dst->nOffsetY + rect->y + rect->h, DLRASTERRENDERZ);
        GXTexCoord2f32((raster->nOffsetX + raster->width) * invWidth, (raster->nOffsetY + raster->height) * invHeight);
        GXPosition3f32(dst->nOffsetX + rect->x + rect->w, dst->nOffsetY + rect->y, DLRASTERRENDERZ);
        GXTexCoord2f32((raster->nOffsetX + raster->width) * invWidth, raster->nOffsetY * invHeight);
    } else {
        RwRaster* dst = lbl_8025F880;
        RwReal invWidth = 1.0f / parent->width;
        RwReal invHeight = 1.0f / parent->height;

        GXPosition3f32(dst->nOffsetX + rect->x, dst->nOffsetY + rect->y, DLRASTERRENDERZ);
        GXTexCoord2f32(raster->nOffsetX * invWidth, raster->nOffsetY * invHeight);
        GXPosition3f32(dst->nOffsetX + rect->x, dst->nOffsetY + rect->y + raster->height, DLRASTERRENDERZ);
        GXTexCoord2f32(raster->nOffsetX * invWidth, (raster->nOffsetY + raster->height) * invHeight);
        GXPosition3f32(dst->nOffsetX + rect->x + raster->width, dst->nOffsetY + rect->y + raster->height,
                       DLRASTERRENDERZ);
        GXTexCoord2f32((raster->nOffsetX + raster->width) * invWidth, (raster->nOffsetY + raster->height) * invHeight);
        GXPosition3f32(dst->nOffsetX + rect->x + raster->width, dst->nOffsetY + rect->y, DLRASTERRENDERZ);
        GXTexCoord2f32((raster->nOffsetX + raster->width) * invWidth, raster->nOffsetY * invHeight);
    }
    GXEnd();

    DlRasterRestoreRenderState();

    _rwDlSetRenderState(rwRENDERSTATEFOGENABLE, (void*)oldFogEnable);
    _rwDlSetRenderState(rwRENDERSTATETEXTUREFILTER, (void*)oldFilter);
    _rwDlSetRenderState(rwRENDERSTATETEXTUREADDRESSU, (void*)oldAddressU);
    _rwDlSetRenderState(rwRENDERSTATETEXTUREADDRESSV, (void*)oldAddressV);
    _rwDlSetRenderState(rwRENDERSTATETEXTURERASTER, oldRaster);
}

RwBool _rwDlRasterRenderGeneric(RwRaster* raster, RwRect* rect, RwBool scaled, RwBool blend)
{
    RwRect dstRect;

    dstRect.x = rect->x;
    dstRect.y = rect->y;
    if (scaled) {
        dstRect.w = rect->w;
        dstRect.h = rect->h;
    } else {
        dstRect.w = raster->width;
        dstRect.h = raster->height;
    }

    switch (lbl_8025F880->cType) {
    case rwRASTERTYPENORMAL:
    case rwRASTERTYPETEXTURE:
    case rwRASTERTYPECAMERATEXTURE:
        switch (raster->cType) {
        case rwRASTERTYPENORMAL:
        case rwRASTERTYPEZBUFFER:
        case rwRASTERTYPECAMERA:
        case rwRASTERTYPETEXTURE:
        case rwRASTERTYPECAMERATEXTURE:
            RWERROR((E_RW_DLNOTSUPPORTED, "SRC, DST Raster render combination not supported"));
            return FALSE;
        case 3:
            break;
        }
        break;
    case rwRASTERTYPECAMERA:
        switch (raster->cType) {
        case rwRASTERTYPENORMAL:
        case rwRASTERTYPETEXTURE:
        case rwRASTERTYPECAMERATEXTURE:
            _rwDlRasterRenderQuad(raster, &dstRect, scaled, blend);
            break;
        case rwRASTERTYPEZBUFFER:
        case rwRASTERTYPECAMERA:
            RWERROR((E_RW_DLNOTSUPPORTED, "SRC, DST Raster render combination not supported"));
            return FALSE;
        case 3:
            break;
        }
        break;
    default:
        RWERROR((E_RW_DLNOTSUPPORTED, "SRC, DST Raster render combination not supported"));
        return FALSE;
    }
    return TRUE;
}

#pragma dont_inline on
RwBool _rwDlRasterRender(void* pRaster, void* pRect, RwInt32 flags)
{
    return _rwDlRasterRenderGeneric((RwRaster*)pRaster, (RwRect*)pRect, FALSE, TRUE);
}

RwBool _rwDlRasterRenderFast(void* pRaster, void* pRect, RwInt32 flags)
{
    return _rwDlRasterRenderGeneric((RwRaster*)pRaster, (RwRect*)pRect, FALSE, FALSE);
}

RwBool _rwDlRasterRenderScaled(void* pRaster, void* pRect, RwInt32 flags)
{
    return _rwDlRasterRenderGeneric((RwRaster*)pRaster, (RwRect*)pRect, TRUE, TRUE);
}
#pragma dont_inline reset

void _rwDlRasterCamera_ZClearRectInit(RwRaster* raster, RwRect* rect)
{
    static RwReal projection[7] = { GX_ORTHOGRAPHIC, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f };
    static RwReal posMtx[3][4] = {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
    };

    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);

    if (raster->cType == rwRASTERTYPECAMERATEXTURE) {
        if (RASTEREXTFROMRASTER(raster)->flags & rwDLRASTERHASALPHA) {
            if (lbl_8025F7FC != GX_PF_RGBA6_Z24) {
                GXSetPixelFmt(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);
                lbl_8025F800 = GX_PF_RGBA6_Z24;
            }
        } else if (lbl_8025F7FC != GX_PF_RGB8_Z24) {
            GXSetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
            lbl_8025F800 = GX_PF_RGB8_Z24;
        }

        GXSetViewport(0.0f, 0.0f, 640.0f, 528.0f, 0.0f, 1.0f);
        GXSetScissor(0, 0, 640, 528);
        projection[1] = 2.0f / 639.0f;
        projection[3] = -2.0f / 527.0f;
    } else {
        if (lbl_8025F800 != lbl_8025F7FC) {
            GXSetPixelFmt(lbl_8025F7FC, GX_ZC_LINEAR);
            lbl_8025F800 = lbl_8025F7FC;
        }

        DlRasterSetViewport();

        if (!lbl_8025F808) {
            GXSetScissor(0, 0, lbl_8025F860->fbWidth, lbl_8025F860->xfbHeight);
        } else if (lbl_8025E7F0) {
            GXSetScissor(0, 0, lbl_8025F860->fbWidth, lbl_8025F85C + 2);
            GXSetScissorBoxOffset(0, 0);
        } else {
            GXSetScissor(0, lbl_8025F85C - 2, lbl_8025F860->fbWidth, lbl_8025F85C + 2);
            GXSetScissorBoxOffset(0, lbl_8025F85C - 2);
        }

        projection[1] = 2.0f / (RwReal)(lbl_8025F860->fbWidth - 1);
        projection[3] = -2.0f / (RwReal)(lbl_8025F860->xfbHeight - 1);
    }

    GXSetProjectionv(projection);
    GXLoadPosMtxImm(posMtx, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
}

void _rwDlRasterCamera_ZClearRect(RwRaster* raster, RwRect* rect, RwRGBA* color, RwInt32 flags)
{
    RwBool fogEnabled = FALSE;
    GXColor clearColor;

    if (flags & rwCAMERACLEARIMAGE) {
        clearColor.r = color->red;
        clearColor.g = color->green;
        clearColor.b = color->blue;
        clearColor.a = color->alpha;
        if (flags & rwCAMERACLEARZ) {
            GXSetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
        } else {
            GXSetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
        }
    } else if (flags & rwCAMERACLEARZ) {
        clearColor.r = 0;
        clearColor.g = 0;
        clearColor.b = 0;
        clearColor.a = 0xFF;
        GXSetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
        GXSetColorUpdate(GX_FALSE);
    } else {
        return;
    }

    GXSetChanMatColor(GX_COLOR0A0, clearColor);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
    GXSetCullMode(GX_CULL_NONE);
    GXSetAlphaCompare(GX_ALWAYS, lbl_8025E348.alphaRef0, GX_AOP_AND, GX_ALWAYS, lbl_8025E348.alphaRef1);
    GXSetZCompLoc(GX_TRUE);

    if (lbl_8025E348.fogEnable) {
        _rwDlRenderStateFogEnable(FALSE);
        fogEnabled = TRUE;
    }
    lbl_8025E348.alphaTestMode = 1;

    _rwDlRasterCamera_ZClearRectInit(raster, rect);

    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(rect->x, rect->y, DLRASTERRENDERZ);
    GXPosition3f32(rect->x, rect->y + rect->h, DLRASTERRENDERZ);
    GXPosition3f32(rect->x + rect->w, rect->y + rect->h, DLRASTERRENDERZ);
    GXPosition3f32(rect->x + rect->w, rect->y, DLRASTERRENDERZ);
    GXEnd();

    if (fogEnabled) {
        _rwDlRenderStateFogEnable(TRUE);
    }

    DlRasterRestoreRenderState();
    GXSetColorUpdate(GX_TRUE);
}

RwBool _rwDlRasterClearGeneric(RwRaster* raster, RwRect* rect, RwInt32 pixelValue)
{
    RwRGBA color;
    RwBool locked;
    RwUInt32 y;

    switch (lbl_8025F880->cType) {
    case rwRASTERTYPECAMERA:
        RwRGBASetFromPixel(&color, pixelValue, RwRasterGetFormat(lbl_8025F880));
        rect->x += lbl_8025F880->nOffsetX;
        rect->y += lbl_8025F880->nOffsetY;
        _rwDlRasterCamera_ZClearRect(raster, rect, &color, rwCAMERACLEARIMAGE);
        break;
    case rwRASTERTYPEZBUFFER:
        rect->x += lbl_8025F880->nOffsetX;
        rect->y += lbl_8025F880->nOffsetY;
        _rwDlRasterCamera_ZClearRect(raster, rect, NULL, rwCAMERACLEARZ);
        break;
    case rwRASTERTYPENORMAL:
    case rwRASTERTYPETEXTURE:
    case rwRASTERTYPECAMERATEXTURE:
        locked = FALSE;
        if (lbl_8025F880->parent == lbl_8025F880 && lbl_8025F880->nOffsetX == 0 && lbl_8025F880->nOffsetY == 0 &&
            lbl_8025F880->width == rect->w && lbl_8025F880->height == rect->h) {
            if (!(lbl_8025F880->privateFlags & rwRASTERPIXELLOCKED)) {
                locked = TRUE;
                RwRasterLock(lbl_8025F880, 0, rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH);
            }
            if (pixelValue == 0 || pixelValue == -1) {
                memset(lbl_8025F880->cpPixels, pixelValue, lbl_8025F880->stride * lbl_8025F880->height);
                if (locked) {
                    RwRasterUnlock(lbl_8025F880);
                }
                return TRUE;
            }
        } else if (!(lbl_8025F880->privateFlags & rwRASTERPIXELLOCKED)) {
            locked = TRUE;
            RwRasterLock(lbl_8025F880, 0, rwRASTERLOCKWRITE | rwRASTERLOCKREAD);
        }

        switch (lbl_8025F880->depth) {
        case 4:
            pixelValue = (((pixelValue << 4) & 0xF0) | (pixelValue & 0xF)) & 0xFF;
            for (y = 0; y < rect->h; y++) {
                memset(lbl_8025F880->cpPixels + lbl_8025F880->stride * (rect->y + y) + (rect->x >> 1), pixelValue,
                       rect->w >> 1);
            }
            break;
        case 8:
            pixelValue = (RwUInt8)pixelValue;
            for (y = 0; y < rect->h; y++) {
                memset(lbl_8025F880->cpPixels + lbl_8025F880->stride * (rect->y + y) + rect->x, pixelValue, rect->w);
            }
            break;
        case 16: {
            RwUInt16 value = (RwUInt16)pixelValue;

            for (y = 0; y < rect->h; y++) {
                RwUInt16* pixels =
                    (RwUInt16*)(lbl_8025F880->cpPixels + lbl_8025F880->stride * (rect->y + y) + (rect->x << 1));
                RwUInt32 x;

                for (x = 0; x < rect->w; x++) {
                    *pixels++ = value;
                }
            }
            break;
        }
        case 32:
            for (y = 0; y < rect->h; y++) {
                RwUInt32* pixels =
                    (RwUInt32*)(lbl_8025F880->cpPixels + lbl_8025F880->stride * (rect->y + y) + (rect->x << 2));
                RwUInt32 x;

                for (x = 0; x < rect->w; x++) {
                    *pixels++ = pixelValue;
                }
            }
            break;
        case 24:
        default:
            RWERROR((E_RW_INVRASTERDEPTH));
            break;
        }

        if (locked) {
            RwRasterUnlock(lbl_8025F880);
        }
        break;
    default:
        RWERROR((E_RW_INVRASTERFORMAT));
        return FALSE;
    }
    return TRUE;
}

RwBool _rwDlRasterClearRect(void* pOut, void* pRect, RwInt32 pixelValue)
{
    return _rwDlRasterClearGeneric(lbl_8025F880, (RwRect*)pRect, pixelValue);
}

RwBool _rwDlRasterClear(void* pOut, void* pInOut, RwInt32 pixelValue)
{
    RwRect rect;

    rect.x = 0;
    rect.y = 0;
    rect.w = lbl_8025F880->width;
    rect.h = lbl_8025F880->height;
    return _rwDlRasterClearGeneric(lbl_8025F880, &rect, pixelValue);
}

RwBool _rwDlSetRasterContext(void* pOut, void* pRaster, RwInt32 flags)
{
    lbl_8025F880 = (RwRaster*)pRaster;
    return TRUE;
}

/*
 * GameCube device layer render states: RenderWare render states are cached
 * in a single block and only pushed to GX when they change. Texture states
 * are kept on a shared render state texture whose raster, filtering and
 * addressing are applied to texture map 0 when it is flushed.
 */

#include "rwsdk/rwcore.h"
#include "rwsdk/rpworld_int.h"

#define rwRENDERSTATETEXTURERASTER 1
#define rwRENDERSTATETEXTUREADDRESS 2
#define rwRENDERSTATETEXTUREADDRESSU 3
#define rwRENDERSTATETEXTUREADDRESSV 4
#define rwRENDERSTATETEXTUREPERSPECTIVE 5
#define rwRENDERSTATEZTESTENABLE 6
#define rwRENDERSTATESHADEMODE 7
#define rwRENDERSTATEZWRITEENABLE 8
#define rwRENDERSTATETEXTUREFILTER 9
#define rwRENDERSTATESRCBLEND 10
#define rwRENDERSTATEDESTBLEND 11
#define rwRENDERSTATEVERTEXALPHAENABLE 12
#define rwRENDERSTATEBORDERCOLOR 13
#define rwRENDERSTATEFOGENABLE 14
#define rwRENDERSTATEFOGCOLOR 15
#define rwRENDERSTATEFOGTYPE 16
#define rwRENDERSTATEFOGDENSITY 17
#define rwRENDERSTATEFOGTABLE 18
#define rwRENDERSTATEALPHAPRIMITIVEBUFFER 19
#define rwRENDERSTATECULLMODE 20

#define rwSHADEMODEGOURAUD 2
#define rwTEXTUREADDRESSWRAP 1
#define rwTEXTUREADDRESSBORDER 4
#define rwFILTERLINEAR 2
#define rwFOGTYPELINEAR 1
#define rwCULLMODECULLBACK 2

#define rwBLENDZERO 1
#define rwBLENDONE 2
#define rwBLENDSRCCOLOR 3
#define rwBLENDINVSRCCOLOR 4
#define rwBLENDSRCALPHA 5
#define rwBLENDINVSRCALPHA 6
#define rwBLENDDESTALPHA 7
#define rwBLENDINVDESTALPHA 8
#define rwBLENDDESTCOLOR 9
#define rwBLENDINVDESTCOLOR 10
#define rwBLENDSRCALPHASAT 11

#define rwRASTERTYPETEXTURE 0x04
#define rwRASTERFORMAT565 0x200
#define rwRASTERLOCKWRITE 0x01
#define rwRASTERLOCKNOFETCH 0x08

#define rwDLRASTERHASALPHA 0x01

#define GX_FALSE 0
#define GX_TRUE 1

#define GX_COLOR0 0
#define GX_COLOR1 1
#define GX_ALPHA0 2
#define GX_ALPHA1 3

#define GX_SRC_REG 0
#define GX_DF_NONE 0
#define GX_AF_NONE 2

#define GX_LEQUAL 3
#define GX_GREATER 4
#define GX_ALWAYS 7

#define GX_AOP_AND 0

#define GX_BM_BLEND 1
#define GX_BL_SRCALPHA 4
#define GX_BL_INVSRCALPHA 5
#define GX_LO_CLEAR 0

#define GX_FOG_NONE 0

#define GX_CULL_FRONT 1

#define GX_MAX_Z24 0x00FFFFFF

typedef struct GXColor {
    RwUInt8 r;
    RwUInt8 g;
    RwUInt8 b;
    RwUInt8 a;
} GXColor;

typedef struct RwDlRasterExt {
    /* 0x00 */ RwUInt32 tlutObj[3];
    /* 0x0C */ RwUInt32 format;
    /* 0x10 */ RwUInt32 tlutFormat;
    /* 0x14 */ RwUInt32 flags;
} RwDlRasterExt;

#define RASTEREXTFROMRASTER(raster) ((RwDlRasterExt*)((RwUInt8*)(raster) + _rwDlRasterExtOffset))

#define RwTextureSetFilterMode(tex, filter) ((tex)->filterAddressing = ((tex)->filterAddressing & ~0xFF) | ((filter) & 0xFF))
#define RwTextureSetAddressing(tex, addressing)                                                                        \
    ((tex)->filterAddressing = ((tex)->filterAddressing & ~0xFF00) |                                                 \
                               ((((addressing) << 8) & 0xF00) | (((addressing) << 12) & 0xF000)))

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

void* memset(void* dst, int c, unsigned long n);
RwRaster* RwRasterCreate(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags);
RwBool RwRasterDestroy(RwRaster* raster);
RwUInt8* RwRasterLock(RwRaster* raster, RwUInt8 level, RwInt32 lockMode);
RwRaster* RwRasterUnlock(RwRaster* raster);
RwTexture* RwTextureCreate(RwRaster* raster);
RwBool RwTextureDestroy(RwTexture* texture);
RwBool _rwDlTextureSetRaster(RwTexture* texture, RwRaster* raster, RwInt32 flags);
void _rwDlTextureSet(RwTexture* texture, RwInt32 map);

void GXSetZMode(RwUInt8 compareEnable, RwInt32 func, RwUInt8 updateEnable);
void GXSetZCompLoc(RwUInt8 beforeTex);
void GXSetBlendMode(RwInt32 type, RwInt32 srcFactor, RwInt32 dstFactor, RwInt32 op);
void GXSetCullMode(RwInt32 mode);
void GXSetAlphaCompare(RwInt32 comp0, RwUInt8 ref0, RwInt32 op, RwInt32 comp1, RwUInt8 ref1);
void GXSetChanCtrl(RwInt32 chan, RwUInt8 enable, RwInt32 ambSrc, RwInt32 matSrc, RwUInt32 lightMask, RwInt32 diffFn,
                   RwInt32 attnFn);
void GXSetChanMatColor(RwInt32 chan, GXColor color);
void GXSetColorUpdate(RwUInt8 updateEnable);
void GXSetAlphaUpdate(RwUInt8 updateEnable);
void GXSetCopyClear(GXColor clearColor, RwUInt32 clearZ);
void GXSetCurrentMtx(RwUInt32 id);
void GXSetFog(RwInt32 type, RwReal startz, RwReal endz, RwReal nearz, RwReal farz, GXColor color);

static RwInt32 DlFogTypeMap[4] = { GX_FOG_NONE, 2, 4, 5 };

static RwInt32 DlBlendMap[12] = { 0, 0, 1, 2, 3, 4, 5, 6, 7, 2, 3, 0 };

RwDlRenderState lbl_8025E348;
RwTexture* lbl_8025F87C;
RwRaster* lbl_8025F878;

void _rwDlRenderStateOpen(void)
{
    GXColor white = { 0xFF, 0xFF, 0xFF, 0xFF };

    lbl_8025E348.fogEnable = FALSE;
    lbl_8025E348.fogType = rwFOGTYPELINEAR;
    lbl_8025E348.fogColor = 0xFFFFFFFF;
    lbl_8025E348.gxFogColor.r = 0xFF;
    lbl_8025E348.gxFogColor.g = 0xFF;
    lbl_8025E348.gxFogColor.b = 0xFF;
    lbl_8025E348.gxFogColor.a = 0xFF;
    lbl_8025E348.fogDensity = -1.0f;

    lbl_8025E348.zWriteEnable = TRUE;
    lbl_8025E348.zTestEnable = TRUE;
    lbl_8025E348.zFunc = GX_LEQUAL;
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetZCompLoc(GX_TRUE);

    lbl_8025E348.srcBlend = rwBLENDSRCALPHA;
    lbl_8025E348.destBlend = rwBLENDINVSRCALPHA;
    lbl_8025E348.gxSrcBlend = GX_BL_SRCALPHA;
    lbl_8025E348.gxDestBlend = GX_BL_INVSRCALPHA;
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);

    lbl_8025E348.cullMode = rwCULLMODECULLBACK;
    GXSetCullMode(GX_CULL_FRONT);

    lbl_8025E348.alphaRef0 = 0;
    lbl_8025E348.alphaRef1 = 0;
    lbl_8025E348.alphaTestMode = 1;
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);

    GXSetChanCtrl(GX_ALPHA0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE, GX_AF_NONE);
    GXSetChanCtrl(GX_ALPHA1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE, GX_AF_NONE);
    GXSetChanCtrl(GX_COLOR0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE, GX_AF_NONE);
    GXSetChanCtrl(GX_COLOR1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE, GX_AF_NONE);
    GXSetChanMatColor(GX_ALPHA0, white);
    GXSetChanMatColor(GX_ALPHA1, white);
    GXSetChanMatColor(GX_COLOR0, white);
    GXSetChanMatColor(GX_COLOR1, white);

    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_TRUE);
    GXSetCopyClear(white, GX_MAX_Z24);
    GXSetCurrentMtx(0);

    lbl_8025F878 = RwRasterCreate(4, 4, 16, rwRASTERTYPETEXTURE | rwRASTERFORMAT565);
    memset(RwRasterLock(lbl_8025F878, 0, rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH), 0xFF, 4 * 4 * 2);
    RwRasterUnlock(lbl_8025F878);

    lbl_8025F87C = RwTextureCreate(NULL);
    RwTextureSetFilterMode(lbl_8025F87C, rwFILTERLINEAR);
    RwTextureSetAddressing(lbl_8025F87C, rwTEXTUREADDRESSWRAP);
}

void _rwDlRenderStateClose(void)
{
    RwTextureDestroy(lbl_8025F87C);
    lbl_8025F87C = NULL;
    RwRasterDestroy(lbl_8025F878);
    lbl_8025F878 = NULL;
}

RwBool _rwDlGetRenderState(RwRenderState state, void* param)
{
    switch (state) {
    case rwRENDERSTATEFOGENABLE:
        *(RwBool*)param = lbl_8025E348.fogEnable;
        return TRUE;
    case rwRENDERSTATEFOGTYPE:
        *(RwInt32*)param = lbl_8025E348.fogType;
        return TRUE;
    case rwRENDERSTATEFOGCOLOR:
        *(RwUInt32*)param = lbl_8025E348.fogColor;
        return TRUE;
    case rwRENDERSTATEFOGDENSITY:
        return FALSE;
    case rwRENDERSTATEFOGTABLE:
        return FALSE;
    case rwRENDERSTATETEXTUREADDRESS: {
        RwInt32 u = (lbl_8025F87C->filterAddressing >> 8) & 0xF;
        RwInt32 v = (lbl_8025F87C->filterAddressing >> 12) & 0xF;

        if (u == v) {
            *(RwInt32*)param = u;
            return TRUE;
        }
        return FALSE;
    }
    case rwRENDERSTATETEXTUREADDRESSU:
        *(RwInt32*)param = (lbl_8025F87C->filterAddressing >> 8) & 0xF;
        return TRUE;
    case rwRENDERSTATETEXTUREADDRESSV:
        *(RwInt32*)param = (lbl_8025F87C->filterAddressing >> 12) & 0xF;
        return TRUE;
    case rwRENDERSTATETEXTUREFILTER:
        *(RwInt32*)param = lbl_8025F87C->filterAddressing & 0xFF;
        return TRUE;
    case rwRENDERSTATETEXTURERASTER:
        *(RwRaster**)param = lbl_8025F87C->raster;
        return TRUE;
    case rwRENDERSTATEZWRITEENABLE:
        *(RwBool*)param = lbl_8025E348.zWriteEnable;
        return TRUE;
    case rwRENDERSTATEZTESTENABLE:
        *(RwBool*)param = lbl_8025E348.zTestEnable;
        return TRUE;
    case rwRENDERSTATESRCBLEND:
        *(RwInt32*)param = lbl_8025E348.srcBlend;
        return TRUE;
    case rwRENDERSTATEDESTBLEND:
        *(RwInt32*)param = lbl_8025E348.destBlend;
        return TRUE;
    case rwRENDERSTATESHADEMODE:
        *(RwInt32*)param = rwSHADEMODEGOURAUD;
        return TRUE;
    case rwRENDERSTATEBORDERCOLOR:
        return FALSE;
    case rwRENDERSTATETEXTUREPERSPECTIVE:
        *(RwBool*)param = TRUE;
        return TRUE;
    case rwRENDERSTATEALPHAPRIMITIVEBUFFER:
        return FALSE;
    case rwRENDERSTATECULLMODE:
        *(RwInt32*)param = lbl_8025E348.cullMode;
        return TRUE;
    }
    return FALSE;
}

RwBool _rwDlRenderStateFogEnable(RwBool enable)
{
    RwCamera* camera = (RwCamera*)RWSRCGLOBAL(curCamera);

    if (enable) {
        if (!lbl_8025E348.fogEnable) {
            RwReal endz;

            if (0.0f > lbl_8025E348.fogDensity) {
                endz = camera->farPlane;
            } else {
                endz = lbl_8025E348.fogDensity;
            }

            GXSetFog(DlFogTypeMap[lbl_8025E348.fogType], camera->fogPlane, endz, camera->nearPlane, camera->farPlane,
                     lbl_8025E348.gxFogColor);
            lbl_8025E348.fogEnable = TRUE;
        }
    } else if (lbl_8025E348.fogEnable) {
        GXSetFog(GX_FOG_NONE, camera->fogPlane, camera->farPlane, camera->nearPlane, camera->farPlane,
                 lbl_8025E348.gxFogColor);
        lbl_8025E348.fogEnable = FALSE;
    }
    return TRUE;
}

void _rwDlTextureRasterFlush(RwInt32 mode);

void _rwDlRenderStateSetZCompLoc(void)
{
    RwInt32 mode = 1;

    if (lbl_8025F87C->raster) {
        _rwDlTextureSet(lbl_8025F87C, 0);
        mode = (RASTEREXTFROMRASTER(lbl_8025F87C->raster)->flags & rwDLRASTERHASALPHA) ? 3 : 1;
    }
    _rwDlTextureRasterFlush(mode);
}

void _rwDlTextureRasterFlush(RwInt32 mode)
{
    if (lbl_8025E348.alphaTestMode != mode) {
        switch (mode) {
        case 1:
            GXSetAlphaCompare(GX_ALWAYS, lbl_8025E348.alphaRef0, GX_AOP_AND, GX_ALWAYS, lbl_8025E348.alphaRef1);
            GXSetZCompLoc(GX_TRUE);
            break;
        case 2:
            GXSetAlphaCompare(GX_GREATER, lbl_8025E348.alphaRef0, GX_AOP_AND, GX_GREATER, lbl_8025E348.alphaRef1);
            GXSetZCompLoc(GX_TRUE);
            break;
        case 3:
            GXSetAlphaCompare(GX_GREATER, lbl_8025E348.alphaRef0, GX_AOP_AND, GX_GREATER, lbl_8025E348.alphaRef1);
            GXSetZCompLoc(GX_FALSE);
            break;
        case 4:
            break;
        }
        lbl_8025E348.alphaTestMode = mode;
    }
}

static inline RwBool DlRenderStateFogColor(RwUInt32 color)
{
    if (color != lbl_8025E348.fogColor) {
        RwCamera* camera = (RwCamera*)RWSRCGLOBAL(curCamera);

        lbl_8025E348.gxFogColor.a = color >> 24;
        lbl_8025E348.gxFogColor.r = color >> 16;
        lbl_8025E348.gxFogColor.g = color >> 8;
        lbl_8025E348.gxFogColor.b = color;
        GXSetFog(DlFogTypeMap[lbl_8025E348.fogType], camera->fogPlane, camera->farPlane, camera->nearPlane,
                 camera->farPlane, lbl_8025E348.gxFogColor);
        lbl_8025E348.fogColor = color;
    }
    return TRUE;
}

static inline RwBool DlRenderStateFogType(RwInt32 type)
{
    if (type != lbl_8025E348.fogType) {
        RwCamera* camera;

        if (type != rwFOGTYPELINEAR) {
            return FALSE;
        }

        camera = (RwCamera*)RWSRCGLOBAL(curCamera);
        GXSetFog(DlFogTypeMap[type], camera->fogPlane, camera->farPlane, camera->nearPlane, camera->farPlane,
                 lbl_8025E348.gxFogColor);
        lbl_8025E348.fogType = type;
    }
    return TRUE;
}

static inline RwBool DlRenderStateTextureAddress(RwInt32 addressing)
{
    if (addressing == rwTEXTUREADDRESSBORDER) {
        return FALSE;
    }
    RwTextureSetAddressing(lbl_8025F87C, addressing);
    return TRUE;
}

static inline RwBool DlRenderStateTextureAddressU(RwInt32 addressing)
{
    if (addressing == rwTEXTUREADDRESSBORDER) {
        return FALSE;
    }
    lbl_8025F87C->filterAddressing = (lbl_8025F87C->filterAddressing & ~0xF00) | ((addressing << 8) & 0xF00);
    return TRUE;
}

static inline RwBool DlRenderStateTextureAddressV(RwInt32 addressing)
{
    if (addressing == rwTEXTUREADDRESSBORDER) {
        return FALSE;
    }
    lbl_8025F87C->filterAddressing = (lbl_8025F87C->filterAddressing & ~0xF000) | ((addressing << 12) & 0xF000);
    return TRUE;
}

static inline RwBool DlRenderStateZWriteEnable(RwBool enable)
{
    if (enable) {
        if (!lbl_8025E348.zWriteEnable) {
            GXSetZMode(GX_TRUE, lbl_8025E348.zFunc, GX_TRUE);
            lbl_8025E348.zWriteEnable = TRUE;
        }
    } else if (lbl_8025E348.zWriteEnable) {
        GXSetZMode(GX_TRUE, lbl_8025E348.zFunc, GX_FALSE);
        lbl_8025E348.zWriteEnable = FALSE;
    }
    return TRUE;
}

static inline RwBool DlRenderStateZTestEnable(RwBool enable)
{
    if (enable) {
        if (!lbl_8025E348.zTestEnable) {
            GXSetZMode(GX_TRUE, GX_LEQUAL, lbl_8025E348.zWriteEnable);
            lbl_8025E348.zTestEnable = TRUE;
            lbl_8025E348.zFunc = GX_LEQUAL;
        }
    } else if (lbl_8025E348.zTestEnable) {
        GXSetZMode(GX_TRUE, GX_ALWAYS, lbl_8025E348.zWriteEnable);
        lbl_8025E348.zTestEnable = FALSE;
        lbl_8025E348.zFunc = GX_ALWAYS;
    }
    return TRUE;
}

static inline RwBool DlRenderStateSrcBlend(RwInt32 blend)
{
    if (blend != lbl_8025E348.srcBlend) {
        RwInt32 gxBlend;

        switch (blend) {
        case rwBLENDZERO:
        case rwBLENDONE:
        case rwBLENDSRCALPHA:
        case rwBLENDINVSRCALPHA:
        case rwBLENDDESTALPHA:
        case rwBLENDINVDESTALPHA:
        case rwBLENDDESTCOLOR:
        case rwBLENDINVDESTCOLOR:
            gxBlend = DlBlendMap[blend];
            GXSetBlendMode(GX_BM_BLEND, gxBlend, lbl_8025E348.gxDestBlend, GX_LO_CLEAR);
            lbl_8025E348.srcBlend = blend;
            lbl_8025E348.gxSrcBlend = gxBlend;
            break;
        case rwBLENDSRCCOLOR:
        case rwBLENDINVSRCCOLOR:
        case rwBLENDSRCALPHASAT:
        default:
            return FALSE;
        }
    }
    return TRUE;
}

static inline RwBool DlRenderStateDestBlend(RwInt32 blend)
{
    if (blend != lbl_8025E348.destBlend) {
        RwInt32 gxBlend;

        switch (blend) {
        case rwBLENDZERO:
        case rwBLENDONE:
        case rwBLENDSRCCOLOR:
        case rwBLENDINVSRCCOLOR:
        case rwBLENDSRCALPHA:
        case rwBLENDINVSRCALPHA:
        case rwBLENDDESTALPHA:
        case rwBLENDINVDESTALPHA:
            gxBlend = DlBlendMap[blend];
            GXSetBlendMode(GX_BM_BLEND, lbl_8025E348.gxSrcBlend, gxBlend, GX_LO_CLEAR);
            lbl_8025E348.destBlend = blend;
            lbl_8025E348.gxDestBlend = gxBlend;
            break;
        case rwBLENDDESTCOLOR:
        case rwBLENDINVDESTCOLOR:
        case rwBLENDSRCALPHASAT:
        default:
            return FALSE;
        }
    }
    return TRUE;
}

static inline RwBool DlRenderStateAlphaPrimitiveBuffer(RwBool enable)
{
    return !enable;
}

static inline RwBool DlRenderStateCullMode(RwInt32 mode)
{
    if (mode != lbl_8025E348.cullMode) {
        GXSetCullMode(mode - 1);
        lbl_8025E348.cullMode = mode;
    }
    return TRUE;
}

RwBool _rwDlSetRenderState(RwRenderState state, void* param)
{
    RwBool result = FALSE;

    switch (state) {
    case rwRENDERSTATEFOGENABLE:
        result = _rwDlRenderStateFogEnable((RwBool)param);
        break;
    case rwRENDERSTATEFOGCOLOR:
        result = DlRenderStateFogColor((RwUInt32)param);
        break;
    case rwRENDERSTATEFOGTYPE:
        result = DlRenderStateFogType((RwInt32)param);
        break;
    case rwRENDERSTATEFOGDENSITY:
        result = FALSE;
        break;
    case rwRENDERSTATEFOGTABLE:
        result = FALSE;
        break;
    case rwRENDERSTATETEXTUREADDRESS:
        result = DlRenderStateTextureAddress((RwInt32)param);
        break;
    case rwRENDERSTATETEXTUREADDRESSU:
        result = DlRenderStateTextureAddressU((RwInt32)param);
        break;
    case rwRENDERSTATETEXTUREADDRESSV:
        result = DlRenderStateTextureAddressV((RwInt32)param);
        break;
    case rwRENDERSTATETEXTUREFILTER:
        RwTextureSetFilterMode(lbl_8025F87C, (RwInt32)param);
        result = TRUE;
        break;
    case rwRENDERSTATETEXTURERASTER:
        if ((RwRaster*)param != lbl_8025F87C->raster) {
            _rwDlTextureSetRaster(lbl_8025F87C, (RwRaster*)param, 0);
        }
        result = TRUE;
        break;
    case rwRENDERSTATEZWRITEENABLE:
        result = DlRenderStateZWriteEnable((RwBool)param);
        break;
    case rwRENDERSTATEZTESTENABLE:
        result = DlRenderStateZTestEnable((RwBool)param);
        break;
    case rwRENDERSTATESRCBLEND:
        result = DlRenderStateSrcBlend((RwInt32)param);
        break;
    case rwRENDERSTATEDESTBLEND:
        result = DlRenderStateDestBlend((RwInt32)param);
        break;
    case rwRENDERSTATESHADEMODE:
        result = ((RwInt32)param == rwSHADEMODEGOURAUD);
        break;
    case rwRENDERSTATEBORDERCOLOR:
        result = FALSE;
        break;
    case rwRENDERSTATETEXTUREPERSPECTIVE:
        result = (RwBool)param;
        break;
    case rwRENDERSTATEALPHAPRIMITIVEBUFFER:
        result = DlRenderStateAlphaPrimitiveBuffer((RwBool)param);
        break;
    case rwRENDERSTATECULLMODE:
        result = DlRenderStateCullMode((RwInt32)param);
        break;
    }
    return result;
}

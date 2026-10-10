/*
 * GameCube device layer: the device system handler (video modes, frame
 * buffer and FIFO allocation, VI setup, the standard function table), the
 * camera begin/end/clear standards, and the triple buffered display copy.
 * Each shown frame is queued with its FIFO write pointer; a GX breakpoint
 * and the VI retrace callbacks advance the queue and flip the VI frame buffer
 * once the frame's draw sync token has been reached.
 */

#include "rwsdk/rwcore.h"
#include "rwsdk/rpworld_int.h"

#define rwPLUGIN_ID 1

#define E_RW_DLNOTSUPPORTED 2

#define rwDEVICESYSTEMOPEN 0
#define rwDEVICESYSTEMCLOSE 1
#define rwDEVICESYSTEMSTART 2
#define rwDEVICESYSTEMSTOP 3
#define rwDEVICESYSTEMREGISTER 4
#define rwDEVICESYSTEMGETNUMMODES 5
#define rwDEVICESYSTEMGETMODEINFO 6
#define rwDEVICESYSTEMUSEMODE 7
#define rwDEVICESYSTEMFOCUS 8
#define rwDEVICESYSTEMINITPIPELINE 9
#define rwDEVICESYSTEMGETMODE 10
#define rwDEVICESYSTEMSTANDARDS 11
#define rwDEVICESYSTEMGETTEXMEMSIZE 12
#define rwDEVICESYSTEMGETNUMSUBSYSTEMS 13
#define rwDEVICESYSTEMGETSUBSYSTEMINFO 14
#define rwDEVICESYSTEMGETCURRENTSUBSYSTEM 15
#define rwDEVICESYSTEMSETSUBSYSTEM 16
#define rwDEVICESYSTEMFINALIZESTART 17
#define rwDEVICESYSTEMINITIATESTOP 18
#define rwDEVICESYSTEMGETMAXTEXTURESIZE 19

#define rwSTANDARDNASTANDARD 0
#define rwSTANDARDCAMERABEGINUPDATE 1
#define rwSTANDARDRGBTOPIXEL 2
#define rwSTANDARDPIXELTORGB 3
#define rwSTANDARDRASTERCREATE 4
#define rwSTANDARDRASTERDESTROY 5
#define rwSTANDARDIMAGEGETRASTER 6
#define rwSTANDARDRASTERSETIMAGE 7
#define rwSTANDARDTEXTURESETRASTER 8
#define rwSTANDARDIMAGEFINDRASTERFORMAT 9
#define rwSTANDARDCAMERAENDUPDATE 10
#define rwSTANDARDSETRASTERCONTEXT 11
#define rwSTANDARDRASTERSUBRASTER 12
#define rwSTANDARDRASTERCLEARRECT 13
#define rwSTANDARDRASTERCLEAR 14
#define rwSTANDARDRASTERLOCK 15
#define rwSTANDARDRASTERUNLOCK 16
#define rwSTANDARDRASTERRENDER 17
#define rwSTANDARDRASTERRENDERSCALED 18
#define rwSTANDARDRASTERRENDERFAST 19
#define rwSTANDARDRASTERSHOWRASTER 20
#define rwSTANDARDCAMERACLEAR 21
#define rwSTANDARDHINTRENDERF2B 22
#define rwSTANDARDRASTERLOCKPALETTE 23
#define rwSTANDARDRASTERUNLOCKPALETTE 24
#define rwSTANDARDNATIVETEXTUREGETSIZE 25
#define rwSTANDARDNATIVETEXTUREREAD 26
#define rwSTANDARDNATIVETEXTUREWRITE 27
#define rwSTANDARDRASTERGETMIPLEVELS 28
#define rwSTANDARDNUMOFSTANDARD 29

#define rwRASTERTYPECAMERATEXTURE 5
#define rwCAMERACLEARIMAGE 0x1
#define rwPARALLEL 2

#define rwDLRASTERHASALPHA 0x01

#define rwDLVIDEOMODECUSTOM 42

#define GX_ORTHOGRAPHIC 1.0f
#define GX_PERSPECTIVE 0.0f
#define GX_PNMTX0 0
#define GX_TRUE 1
#define GX_FALSE 0
#define GX_PF_RGB8_Z24 0
#define GX_PF_RGBA6_Z24 1
#define GX_PF_RGB565_Z16 2
#define GX_ZC_LINEAR 0
#define GX_CLAMP_TOP 1
#define GX_CLAMP_BOTTOM 2
#define GX_MAX_Z24 0x00FFFFFF

#define VI_NTSC 0
#define VI_PAL 1
#define VI_MPAL 2
#define VI_EURGB60 5

#define DLNUMFRAMES 3

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
    /* 0x19 */ RwUInt8 aa;
    /* 0x1A */ RwUInt8 sample_pattern[12][2];
    /* 0x32 */ RwUInt8 vfilter[7];
} GXRenderModeObj;

typedef struct RwDlRasterExt {
    /* 0x00 */ RwUInt32 tlutObj[3];
    /* 0x0C */ RwUInt32 format;
    /* 0x10 */ RwUInt32 tlutFormat;
    /* 0x14 */ RwUInt32 flags;
} RwDlRasterExt;

#define RASTEREXTFROMRASTER(raster) ((RwDlRasterExt*)((RwUInt8*)(raster) + _rwDlRasterExtOffset))

typedef struct RwRect {
    /* 0x00 */ RwInt32 x;
    /* 0x04 */ RwInt32 y;
    /* 0x08 */ RwInt32 w;
    /* 0x0C */ RwInt32 h;
} RwRect;

typedef struct RwVideoMode {
    /* 0x00 */ RwInt32 width;
    /* 0x04 */ RwInt32 height;
    /* 0x08 */ RwInt32 depth;
    /* 0x0C */ RwInt32 flags;
} RwVideoMode;

typedef struct RwStandard {
    /* 0x00 */ RwInt32 nStandard;
    /* 0x04 */ RwStandardFunc fpStandard;
} RwStandard;

typedef struct RwGameCubeDeviceConfig {
    /* 0x00 */ GXRenderModeObj* renderMode;
    /* 0x04 */ RwInt32 pixelFormat;
    /* 0x08 */ RwUInt32 fifoSize;
} RwGameCubeDeviceConfig;

typedef struct RwDlFrame {
    /* 0x00 */ void* frameBuffer;
    /* 0x04 */ void* writePtr;
} RwDlFrame;

typedef struct RwDlGlobals {
    /* 0x00 */ RwCamera* curCamera;
    /* 0x04 */ RwMemoryFunctions* memFuncs;
} RwDlGlobals;

extern RwInt32 _rwDlRasterExtOffset;
extern GXRenderModeObj GXNtsc480IntDf;
extern GXRenderModeObj GXPal528IntDf;
extern GXRenderModeObj GXMpal480IntDf;
extern volatile RwUInt16 lbl_8025E800;
extern volatile RwUInt16 lbl_8025F890;

RwBool _rwDlGetRenderState(RwInt32 state, void* param);
RwBool _rwDlSetRenderState(RwInt32 state, void* param);
RwBool _rwDlIm2DRenderLine(void* vertices, RwInt32 numVertices, RwInt32 vert1, RwInt32 vert2);
RwBool _rwDlIm2DRenderTriangle(void* vertices, RwInt32 numVertices, RwInt32 vert1, RwInt32 vert2, RwInt32 vert3);
RwBool _rwDlIm2DRenderPrimitive(RwInt32 primType, void* vertices, RwInt32 numVertices);
RwBool _rwDlIm2DRenderIndexedPrimitive(RwInt32 primType, void* vertices, RwInt32 numVertices, RwUInt16* indices,
                                       RwInt32 numIndices);
void _rwDlRenderStateOpen(void);
void _rwDlRenderStateClose(void);
void _rwDlRasterCamera_ZClearRect(RwRaster* raster, RwRect* rect, RwRGBA* color, RwInt32 flags);
RwBool _rwDlTokenDone(RwUInt16 token);
RwBool _rwDlRasterPluginAttach(void);
RwBool _rwDlTexturePluginAttach(void);

RwBool _rwDlRGBToPixel(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlPixelToRGB(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlRasterSetFromImage(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlImageGetFromRaster(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlRasterDestroy(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlRasterCreate(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlImageFindRasterFormat(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlTextureSetRaster(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlRasterLock(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlRasterUnlock(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlRasterLockPalette(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlRasterUnlockPalette(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlRasterClear(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlRasterClearRect(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlRasterRender(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlRasterRenderScaled(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlRasterRenderFast(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlSetRasterContext(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlRasterSubRaster(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlNativeTextureGetSize(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlNativeTextureWrite(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlNativeTextureRead(void* pOut, void* pInOut, RwInt32 nI);
RwBool _rwDlRasterGetMipLevels(void* pOut, void* pInOut, RwInt32 nI);

void _rwDlMatrixMultiply(RwMatrix* dst, const RwMatrix* matA, const RwMatrix* matB);
void _rwDlV3dTransformPoints(RwV3d* pointsOut, const RwV3d* pointsIn, RwInt32 numPoints, const RwMatrix* matrix);
void _rwDlV3dTransformVectors(RwV3d* vectorsOut, const RwV3d* vectorsIn, RwInt32 numPoints, const RwMatrix* matrix);
RwBool _rwMatrixSetMultFn(void* multMat);
RwBool _rwVectorSetMultFn(void* multPoint, void* multVector);

RwMatrix* RwFrameGetLTM(RwFrame* frame);
RwMatrix* RwMatrixInvert(RwMatrix* dst, const RwMatrix* src);
RwMatrix* RwMatrixMultiply(RwMatrix* matrixOut, const RwMatrix* matrixIn1, const RwMatrix* matrixIn2);

RwInt32 OSDisableInterrupts(void);
RwInt32 OSRestoreInterrupts(RwInt32 level);
void OSInitThreadQueue(void* queue);
void OSSleepThread(void* queue);
void OSWakeupThread(void* queue);
void DCInvalidateRange(void* addr, RwUInt32 nBytes);

RwUInt32 VIGetTvFormat(void);
RwUInt32 VIGetNextField(void);
void VIConfigure(GXRenderModeObj* rm);
void VISetBlack(RwInt32 black);
void VIFlush(void);
void VIWaitForRetrace(void);
void VISetNextFrameBuffer(void* fb);
void* VISetPreRetraceCallback(void* callback);
void* VISetPostRetraceCallback(void* callback);

void* GXInit(void* base, RwUInt32 size);
void* GXGetCPUFifo(void);
void GXGetFifoPtrs(void* fifo, void** readPtr, void** writePtr);
void GXFlush(void);
void GXSetCurrentGXThread(void);
void GXEnableBreakPt(void* breakPtr);
void GXDisableBreakPt(void);
void* GXSetBreakPtCallback(void* callback);
void GXSetDrawSync(RwUInt16 token);
RwUInt16 GXReadDrawSync(void);
void GXCopyDisp(void* dest, RwUInt8 clear);
void GXSetCopyClamp(RwInt32 clamp);
void GXSetCopyClear(GXColor clearColor, RwUInt32 clearZ);
void GXSetDispCopySrc(RwUInt16 left, RwUInt16 top, RwUInt16 wd, RwUInt16 ht);
void GXSetDispCopyDst(RwUInt16 wd, RwUInt16 ht);
RwUInt32 GXSetDispCopyYScale(RwReal vscale);
void GXSetDispCopyGamma(RwInt32 gamma);
void GXSetCopyFilter(RwUInt8 aa, RwUInt8 samplePattern[12][2], RwUInt8 vf, RwUInt8 vfilter[7]);
void GXSetFieldMode(RwUInt8 fieldMode, RwUInt8 halfAspectRatio);
void GXSetPixelFmt(RwInt32 pixFmt, RwInt32 zFmt);
void GXSetViewport(RwReal left, RwReal top, RwReal wd, RwReal ht, RwReal nearz, RwReal farz);
void GXSetViewportJitter(RwReal left, RwReal top, RwReal wd, RwReal ht, RwReal nearz, RwReal farz, RwUInt32 field);
void GXSetScissor(RwUInt32 left, RwUInt32 top, RwUInt32 wd, RwUInt32 ht);
void GXSetScissorBoxOffset(RwInt32 xOffset, RwInt32 yOffset);
void GXSetProjectionv(const RwReal* ptr);
void GXLoadPosMtxImm(RwReal mtx[3][4], RwUInt32 id);
void GXLoadNrmMtxImm(RwReal mtx[3][4], RwUInt32 id);
void GXSetCurrentMtx(RwUInt32 id);

RwDlFrame lbl_8025E2B0[DLNUMFRAMES];
GXRenderModeObj lbl_8025E2C8;
RwMatrix lbl_8025E304;

RwInt32 lbl_8025E7F0 = 1;
RwUInt32 lbl_8025E7F4 = 0x40000;
RwInt32 lbl_8025E7F8 = 1;
RwInt32 lbl_8025E7FC = 0;

RwInt32 lbl_8025F7F8;
RwInt32 lbl_8025F7FC;
RwInt32 lbl_8025F800;
RwInt32 lbl_8025F804;
RwInt32 lbl_8025F808;
void* lbl_8025F80C;
volatile RwInt32 lbl_8025F810;
volatile RwInt32 lbl_8025F814;
volatile RwInt32 lbl_8025F818;
volatile RwInt32 lbl_8025F81C;
volatile RwInt32 lbl_8025F820;
volatile RwInt32 lbl_8025F824;
volatile RwInt32 lbl_8025F828;
RwInt32 lbl_8025F82C;
volatile RwInt32 lbl_8025F830;
RwUInt32 lbl_8025F834[2];
RwUInt16 lbl_8025F83C[4];
void* lbl_8025F844;
void* lbl_8025F848;
void* lbl_8025F84C;
void* lbl_8025F850;
void* lbl_8025F854;
void* lbl_8025F858;
RwInt32 lbl_8025F85C;
GXRenderModeObj* lbl_8025F860;
RwDlGlobals lbl_8025F864;
RwInt32 lbl_8025F86C;

static RwVideoMode DlVideoModes[4] = {
    { 640, 480, 24, 1 },
    { 640, 528, 24, 1 },
    { 640, 480, 24, 1 },
    { 0, 0, 0, 1 },
};

static inline void DlAdvanceBreakPt(void)
{
    RwUInt32 next = (lbl_8025F810 + 1) % DLNUMFRAMES;

    if (next == lbl_8025F814) {
        GXDisableBreakPt();
        lbl_8025F820 = FALSE;
    } else {
        GXEnableBreakPt(lbl_8025E2B0[next].writePtr);
    }
    lbl_8025F810 = next;
    OSWakeupThread(lbl_8025F834);

    if (!lbl_8025F808) {
        lbl_8025F824 = TRUE;
    } else if (lbl_8025F830 == TRUE) {
        lbl_8025F824 = TRUE;
        lbl_8025F830 = FALSE;
    } else {
        lbl_8025F830 = TRUE;
    }
}

void _rwDlBreakPtCallback(void)
{
    if (lbl_8025E2B0[lbl_8025F810].frameBuffer != lbl_8025F844) {
        DlAdvanceBreakPt();
    } else {
        lbl_8025F828 = TRUE;
    }
}

void _rwDlVIPreRetraceCallback(RwUInt32 retraceCount)
{
    RwInt16 token = lbl_8025F83C[lbl_8025F81C];

    if (lbl_8025F824 == TRUE && _rwDlTokenDone(token)) {
        lbl_8025F844 = (lbl_8025F844 == lbl_8025F850) ? lbl_8025F84C : lbl_8025F850;
        VISetNextFrameBuffer(lbl_8025F844);
        if (lbl_8025E7F8) {
            VISetBlack(FALSE);
            lbl_8025E7F8 = FALSE;
        }
        VIFlush();
        lbl_8025F81C = (lbl_8025F81C + 1) % DLNUMFRAMES;
        lbl_8025F824 = FALSE;
        lbl_8025F82C = TRUE;
    }
}

void _rwDlVIPostRetraceCallback(RwUInt32 retraceCount)
{
    if (lbl_8025F828 && lbl_8025F82C) {
        DlAdvanceBreakPt();
        lbl_8025F828 = FALSE;
    }
    lbl_8025F82C = FALSE;
}

RwBool _rwDlNullStandard(void* pOut, void* pInOut, RwInt32 nI)
{
    return FALSE;
}

void _rwDlRenderModeSelect(GXRenderModeObj* renderMode, RwInt32 pixelFormat)
{
    if (renderMode) {
        GXRenderModeObj* rm = &lbl_8025E2C8;
        RwInt32 format;

        *rm = *renderMode;
        lbl_8025F860 = rm;
        lbl_8025F804 = rwDLVIDEOMODECUSTOM;

        format = pixelFormat;
        if (lbl_8025F860->aa) {
            format = GX_PF_RGB565_Z16;
        }
        lbl_8025F7FC = format;
        lbl_8025F800 = format;

        DlVideoModes[3].width = lbl_8025F860->fbWidth;
        DlVideoModes[3].height = lbl_8025F860->efbHeight;
        switch (pixelFormat) {
        case GX_PF_RGB8_Z24:
        case GX_PF_RGBA6_Z24:
            DlVideoModes[3].depth = 24;
            break;
        case GX_PF_RGB565_Z16:
            DlVideoModes[3].depth = 16;
            break;
        default:
            RWERROR((E_RW_DLNOTSUPPORTED, "Invalid pixel format"));
            break;
        }
    } else {
        switch (VIGetTvFormat()) {
        case VI_NTSC:
            lbl_8025F804 = 0;
            lbl_8025F860 = &GXNtsc480IntDf;
            break;
        case VI_PAL:
        case VI_EURGB60:
            lbl_8025F804 = 1;
            lbl_8025F860 = &GXPal528IntDf;
            break;
        case VI_MPAL:
            lbl_8025F804 = 2;
            lbl_8025F860 = &GXMpal480IntDf;
            break;
        default:
            RWERROR((E_RW_DLNOTSUPPORTED, "Invalid TV format"));
            break;
        }
    }
}

RwDevice* _rwDeviceGetHandle(void);
RwBool _rwDlCameraBeginUpdate(void* pOut, void* pCamera, RwInt32 flags);
RwBool _rwDlCameraEndUpdate(void* pOut, void* pCamera, RwInt32 flags);
RwBool _rwDlCameraClear(void* pCamera, void* pColor, RwInt32 flags);
RwBool _rwDlRasterShowRaster(void* pRaster, void* pInOut, RwInt32 flags);

static inline RwBool DlDeviceStandards(RwStandardFunc* standardFunctions, RwInt32 numStandardsFunctions)
{
    RwInt32 i;
    RwInt32 numDriverFunctions;
    RwStandard majorFunctions[] = {
        { rwSTANDARDCAMERABEGINUPDATE, _rwDlCameraBeginUpdate },
        { rwSTANDARDCAMERAENDUPDATE, _rwDlCameraEndUpdate },
        { rwSTANDARDCAMERACLEAR, _rwDlCameraClear },
        { rwSTANDARDRASTERSHOWRASTER, _rwDlRasterShowRaster },
        { rwSTANDARDRGBTOPIXEL, _rwDlRGBToPixel },
        { rwSTANDARDPIXELTORGB, _rwDlPixelToRGB },
        { rwSTANDARDRASTERSETIMAGE, _rwDlRasterSetFromImage },
        { rwSTANDARDIMAGEGETRASTER, _rwDlImageGetFromRaster },
        { rwSTANDARDRASTERDESTROY, _rwDlRasterDestroy },
        { rwSTANDARDRASTERCREATE, _rwDlRasterCreate },
        { rwSTANDARDIMAGEFINDRASTERFORMAT, _rwDlImageFindRasterFormat },
        { rwSTANDARDTEXTURESETRASTER, _rwDlTextureSetRaster },
        { rwSTANDARDRASTERLOCK, _rwDlRasterLock },
        { rwSTANDARDRASTERUNLOCK, _rwDlRasterUnlock },
        { rwSTANDARDRASTERLOCKPALETTE, _rwDlRasterLockPalette },
        { rwSTANDARDRASTERUNLOCKPALETTE, _rwDlRasterUnlockPalette },
        { rwSTANDARDRASTERCLEAR, _rwDlRasterClear },
        { rwSTANDARDRASTERCLEARRECT, _rwDlRasterClearRect },
        { rwSTANDARDRASTERRENDER, _rwDlRasterRender },
        { rwSTANDARDRASTERRENDERSCALED, _rwDlRasterRenderScaled },
        { rwSTANDARDRASTERRENDERFAST, _rwDlRasterRenderFast },
        { rwSTANDARDSETRASTERCONTEXT, _rwDlSetRasterContext },
        { rwSTANDARDRASTERSUBRASTER, _rwDlRasterSubRaster },
        { rwSTANDARDNATIVETEXTUREGETSIZE, _rwDlNativeTextureGetSize },
        { rwSTANDARDNATIVETEXTUREWRITE, _rwDlNativeTextureWrite },
        { rwSTANDARDNATIVETEXTUREREAD, _rwDlNativeTextureRead },
        { rwSTANDARDRASTERGETMIPLEVELS, _rwDlRasterGetMipLevels },
    };

    numDriverFunctions = sizeof(majorFunctions) / sizeof(RwStandard);

    for (i = 0; i < numStandardsFunctions; i++) {
        standardFunctions[i] = _rwDlNullStandard;
    }

    while (numDriverFunctions--) {
        if (majorFunctions->nStandard < numStandardsFunctions && majorFunctions->nStandard >= 0) {
            standardFunctions[majorFunctions[numDriverFunctions].nStandard] =
                majorFunctions[numDriverFunctions].fpStandard;
        }
    }
    return TRUE;
}

static inline RwBool DlDeviceStart(void)
{
    RwInt32 pixelFormat = lbl_8025F7FC;
    GXRenderModeObj* rm = lbl_8025F860;

    VIConfigure(rm);
    VISetBlack(TRUE);
    VIFlush();

    if (rm->field_rendering) {
        GXSetViewportJitter(0.0f, 0.0f, rm->fbWidth, rm->xfbHeight, 0.0f, 1.0f, VIGetNextField() ^ 1);
    } else {
        GXSetViewport(0.0f, 0.0f, rm->fbWidth, rm->xfbHeight, 0.0f, 1.0f);
    }

    GXSetScissor(0, 0, rm->fbWidth, rm->efbHeight);
    GXSetScissorBoxOffset(0, 0);
    GXSetDispCopySrc(0, 0, rm->fbWidth, rm->efbHeight);
    GXSetDispCopyDst(rm->fbWidth, rm->xfbHeight);
    GXSetCopyFilter(rm->aa, rm->sample_pattern, GX_TRUE, rm->vfilter);

    if (rm->aa && rm->xfbHeight == rm->viHeight) {
        GXSetDispCopyYScale(1.0f);
        lbl_8025F85C = rm->xfbHeight / 2;
        lbl_8025F808 = TRUE;
        lbl_8025E7F0 = TRUE;
    } else {
        GXSetDispCopyYScale((RwReal)rm->xfbHeight / (RwReal)rm->efbHeight);
        lbl_8025F808 = FALSE;
    }

    GXSetFieldMode(rm->field_rendering, rm->xfbHeight < rm->viHeight);
    GXSetPixelFmt(pixelFormat, GX_ZC_LINEAR);
    lbl_8025E7F8 = TRUE;

    VIWaitForRetrace();
    if (lbl_8025F860->viTVmode & 1) {
        VIWaitForRetrace();
    }

    GXSetDispCopyGamma(0);
    VISetPreRetraceCallback(_rwDlVIPreRetraceCallback);
    VISetPostRetraceCallback(_rwDlVIPostRetraceCallback);
    GXSetBreakPtCallback(_rwDlBreakPtCallback);
    OSInitThreadQueue(lbl_8025F834);
    return TRUE;
}

RwBool _rwDlSystem(RwInt32 request, void* pOut, void* pInOut, RwInt32 nIn)
{
    switch (request) {
    case rwDEVICESYSTEMUSEMODE:
        return nIn < 1;

    case rwDEVICESYSTEMGETNUMMODES:
        *(RwInt32*)pOut = 1;
        return TRUE;

    case rwDEVICESYSTEMGETMODEINFO:
        if (nIn < 1) {
            switch (lbl_8025F804) {
            case 0:
                *(RwVideoMode*)pOut = DlVideoModes[0];
                break;
            case 1:
            case 5:
                *(RwVideoMode*)pOut = DlVideoModes[1];
                break;
            case 2:
                *(RwVideoMode*)pOut = DlVideoModes[2];
                break;
            case rwDLVIDEOMODECUSTOM:
                *(RwVideoMode*)pOut = DlVideoModes[3];
                break;
            }
            return TRUE;
        }
        return FALSE;

    case rwDEVICESYSTEMGETMODE:
        *(RwInt32*)pOut = 0;
        return TRUE;

    case rwDEVICESYSTEMFOCUS:
        return TRUE;

    case rwDEVICESYSTEMREGISTER:
        *(RwDevice*)pOut = *_rwDeviceGetHandle();
        lbl_8025F864.memFuncs = (RwMemoryFunctions*)pInOut;
        _rwDlRasterPluginAttach();
        _rwDlTexturePluginAttach();
        return TRUE;

    case rwDEVICESYSTEMOPEN: {
        RwGameCubeDeviceConfig* config = *(RwGameCubeDeviceConfig**)pInOut;
        RwUInt32 fbSize;

        if (config) {
            _rwDlRenderModeSelect(config->renderMode, config->pixelFormat);
            lbl_8025E7F4 = config->fifoSize;
        } else {
            _rwDlRenderModeSelect(NULL, lbl_8025F7FC);
        }

        fbSize = ((lbl_8025F860->fbWidth + 15) & 0xFFF0) * lbl_8025F860->xfbHeight * 2;
        lbl_8025F80C = RwMalloc(lbl_8025E7F4 + fbSize * 2 + 31);
        lbl_8025F854 = (void*)(((RwUInt32)lbl_8025F80C + 31) & ~31);
        DCInvalidateRange(lbl_8025F854, lbl_8025E7F4);
        lbl_8025F850 = (RwUInt8*)lbl_8025F854 + lbl_8025E7F4;
        lbl_8025F84C = (RwUInt8*)lbl_8025F850 + fbSize;
        lbl_8025F844 = lbl_8025F850;
        lbl_8025F848 = lbl_8025F84C;
        lbl_8025F858 = GXInit(lbl_8025F854, lbl_8025E7F4);

        while (lbl_8025F890 != GXReadDrawSync()) {
            GXSetDrawSync(lbl_8025F890);
        }
        return TRUE;
    }

    case rwDEVICESYSTEMCLOSE:
        RwFree(lbl_8025F80C);
        lbl_8025F84C = NULL;
        lbl_8025F850 = NULL;
        lbl_8025F854 = NULL;
        lbl_8025F80C = NULL;
        return TRUE;

    case rwDEVICESYSTEMSTART:
        return DlDeviceStart();

    case rwDEVICESYSTEMFINALIZESTART:
        _rwMatrixSetMultFn(_rwDlMatrixMultiply);
        _rwVectorSetMultFn(_rwDlV3dTransformPoints, _rwDlV3dTransformVectors);
        _rwDlRenderStateOpen();
        return TRUE;

    case rwDEVICESYSTEMINITIATESTOP:
        _rwDlRenderStateClose();
        return TRUE;

    case rwDEVICESYSTEMSTOP:
        return TRUE;

    case rwDEVICESYSTEMINITPIPELINE:
        return TRUE;

    case rwDEVICESYSTEMSTANDARDS:
        return DlDeviceStandards((RwStandardFunc*)pOut, nIn);

    case rwDEVICESYSTEMGETMAXTEXTURESIZE:
        *(RwInt32*)pOut = 1024;
        return TRUE;
    }
    return FALSE;
}

RwBool _rwDlCameraClear(void* pCamera, void* pColor, RwInt32 flags)
{
    if (!lbl_8025F7F8) {
        RwRaster* raster = ((RwCamera*)pCamera)->frameBuffer;
        RwRect rect;

        if (!lbl_8025F808) {
            rect.x = raster->nOffsetX;
            rect.y = raster->nOffsetY;
            rect.w = raster->width;
            rect.h = raster->height;
        } else {
            rect.x = raster->nOffsetX;
            rect.y = raster->nOffsetY << 1;
            rect.w = raster->width;
            rect.h = raster->height << 1;
        }
        _rwDlRasterCamera_ZClearRect(raster, &rect, (RwRGBA*)pColor, flags);
    } else if (flags & rwCAMERACLEARIMAGE) {
        GXSetCopyClear(*(GXColor*)pColor, GX_MAX_Z24);
    }
    return TRUE;
}

RwBool _rwDlCameraBeginUpdate(void* pOut, void* pCamera, RwInt32 flags)
{
    static RwReal projection[7] = { GX_PERSPECTIVE, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
    RwCamera* camera = (RwCamera*)pCamera;
    RwMatrix viewOffset;
    RwRaster* raster;

    GXSetCurrentGXThread();
    lbl_8025F864.curCamera = camera;

    viewOffset.flags = 0;
    viewOffset.right.x = 1.0f;
    viewOffset.right.y = 0.0f;
    viewOffset.right.z = 0.0f;
    viewOffset.up.x = 0.0f;
    viewOffset.up.y = 1.0f;
    viewOffset.up.z = 0.0f;
    viewOffset.at.x = -camera->viewOffset.x;
    viewOffset.at.y = camera->viewOffset.y;
    viewOffset.at.z = 1.0f;
    viewOffset.pos.x = camera->viewOffset.x;
    viewOffset.pos.y = -camera->viewOffset.y;
    viewOffset.pos.z = 0.0f;

    lbl_8025E304.flags = rwMATRIXTYPEORTHONORMAL | rwMATRIXINTERNALIDENTITY;
    RwMatrixInvert(&lbl_8025E304, RwFrameGetLTM((RwFrame*)camera->object.object.parent));
    RwMatrixTransform(&lbl_8025E304, &viewOffset, rwCOMBINEPOSTCONCAT);

    projection[1] = camera->recipViewWindow.x;
    projection[3] = camera->recipViewWindow.y;
    if (camera->projectionType == rwPARALLEL) {
        projection[0] = GX_ORTHOGRAPHIC;
        projection[5] = -1.0f / (camera->farPlane - camera->nearPlane);
        projection[6] = camera->farPlane * projection[5];
    } else {
        projection[0] = GX_PERSPECTIVE;
        projection[5] = -camera->nearPlane / (camera->farPlane - camera->nearPlane);
        projection[6] = camera->farPlane * projection[5];
    }
    GXSetProjectionv(projection);

    raster = camera->frameBuffer;
    if (raster->cType & rwRASTERTYPECAMERATEXTURE) {
        if (RASTEREXTFROMRASTER(raster)->flags & rwDLRASTERHASALPHA) {
            if (lbl_8025F800 != GX_PF_RGBA6_Z24) {
                GXSetPixelFmt(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);
                lbl_8025F800 = GX_PF_RGBA6_Z24;
            }
        } else if (lbl_8025F800 != GX_PF_RGB8_Z24) {
            GXSetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
            lbl_8025F800 = GX_PF_RGB8_Z24;
        }

        GXSetViewport(raster->nOffsetX, raster->nOffsetY, raster->width, raster->height, 0.0f, 1.0f);
        GXSetScissor(raster->nOffsetX, raster->nOffsetY, raster->width, raster->height);
        GXSetScissorBoxOffset(0, 0);
    } else {
        if (lbl_8025F800 != lbl_8025F7FC) {
            GXSetPixelFmt(lbl_8025F7FC, GX_ZC_LINEAR);
            lbl_8025F800 = lbl_8025F7FC;
        }

        if (!lbl_8025F808) {
            if (lbl_8025F860->field_rendering) {
                GXSetViewportJitter(raster->nOffsetX, raster->nOffsetY, raster->width, raster->height, 0.0f, 1.0f,
                                    VIGetNextField() ^ 1);
            } else {
                GXSetViewport(raster->nOffsetX, raster->nOffsetY, raster->width, raster->height, 0.0f, 1.0f);
            }
            GXSetScissor(raster->nOffsetX, raster->nOffsetY, raster->width, raster->height);
        } else {
            if (lbl_8025F860->field_rendering) {
                GXSetViewportJitter(raster->nOffsetX, raster->nOffsetY, raster->width, raster->height, 0.0f, 1.0f,
                                    VIGetNextField() ^ 1);
            } else {
                GXSetViewport(raster->nOffsetX, raster->nOffsetY << 1, raster->width, raster->height << 1, 0.0f,
                              1.0f);
            }

            if (lbl_8025E7F0) {
                if ((raster->nOffsetY + raster->height) << 1 <= lbl_8025F85C + 2) {
                    GXSetScissor(raster->nOffsetX, raster->nOffsetY << 1, lbl_8025F860->fbWidth, raster->height << 1);
                } else if (raster->nOffsetY << 1 > lbl_8025F85C + 2) {
                    GXSetScissor(0, 0, lbl_8025F860->fbWidth, lbl_8025F85C + 2);
                } else {
                    GXSetScissor(raster->nOffsetX, raster->nOffsetY << 1, lbl_8025F860->fbWidth, lbl_8025F85C + 2);
                }
                GXSetScissorBoxOffset(0, 0);
            } else {
                if (raster->nOffsetY << 1 >= lbl_8025F85C - 2) {
                    GXSetScissor(raster->nOffsetX, raster->nOffsetY << 1, lbl_8025F860->fbWidth, raster->height << 1);
                } else if ((raster->nOffsetY + raster->height) << 1 < lbl_8025F85C + 2) {
                    GXSetScissor(0, lbl_8025F85C - 2, lbl_8025F860->fbWidth, lbl_8025F85C + 2);
                } else {
                    GXSetScissor(raster->nOffsetX, lbl_8025F85C - 2, lbl_8025F860->fbWidth, raster->height << 1);
                }
                GXSetScissorBoxOffset(0, lbl_8025F85C - 2);
            }
        }
    }
    return TRUE;
}

RwBool _rwDlCameraEndUpdate(void* pOut, void* pCamera, RwInt32 flags)
{
    lbl_8025F864.curCamera = NULL;
    return TRUE;
}

static inline void DlQueueFrame(void* writePtr)
{
    lbl_8025E2B0[lbl_8025F814].writePtr = writePtr;
    lbl_8025E2B0[lbl_8025F814].frameBuffer = lbl_8025F848;
    lbl_8025F814 = (lbl_8025F814 + 1) % DLNUMFRAMES;
}

static inline void DlQueueFrameToken(void* writePtr)
{
    lbl_8025E2B0[lbl_8025F814].writePtr = writePtr;
    lbl_8025E2B0[lbl_8025F814].frameBuffer = lbl_8025F848;
    lbl_8025F83C[lbl_8025F818] = lbl_8025E800;
    lbl_8025F814 = (lbl_8025F814 + 1) % DLNUMFRAMES;
    lbl_8025F818 = (lbl_8025F818 + 1) % DLNUMFRAMES;
}

static inline void DlNextToken(void)
{
    GXSetDrawSync(lbl_8025E800);
    lbl_8025E800 = (lbl_8025E800 + 1) % 0xE000;
}

static inline void DlSwapFrameBuffer(void)
{
    lbl_8025F848 = (lbl_8025F848 == lbl_8025F850) ? lbl_8025F84C : lbl_8025F850;
}

RwBool _rwDlRasterShowRaster(void* pRaster, void* pInOut, RwInt32 flags)
{
    void* readPtr;
    void* writePtr;
    RwInt32 level;
    volatile RwBool full;

    level = OSDisableInterrupts();
    full = (lbl_8025F814 - lbl_8025F810 == -1 || lbl_8025F814 - lbl_8025F810 == 2) ? TRUE : FALSE;
    OSRestoreInterrupts(level);
    if (full == TRUE) {
        OSSleepThread(lbl_8025F834);
    }

    if (!lbl_8025F808) {
        GXFlush();
        GXGetFifoPtrs(GXGetCPUFifo(), &readPtr, &writePtr);
        level = OSDisableInterrupts();
        DlQueueFrameToken(writePtr);
        OSRestoreInterrupts(level);
        if (!lbl_8025F820) {
            lbl_8025F820 = TRUE;
            GXEnableBreakPt(writePtr);
        }

        GXCopyDisp(lbl_8025F848, lbl_8025F7F8);
        DlNextToken();
        GXFlush();
        DlSwapFrameBuffer();
    } else if (lbl_8025E7F0) {
        GXFlush();
        GXGetFifoPtrs(GXGetCPUFifo(), &readPtr, &writePtr);
        level = OSDisableInterrupts();
        DlQueueFrame(writePtr);
        OSRestoreInterrupts(level);
        if (!lbl_8025F820) {
            lbl_8025F820 = TRUE;
            GXEnableBreakPt(writePtr);
        }

        GXCopyDisp(lbl_8025F848, lbl_8025F7F8);
        DlNextToken();
        GXFlush();
        lbl_8025E7F0 = FALSE;
    } else {
        GXFlush();
        GXGetFifoPtrs(GXGetCPUFifo(), &readPtr, &writePtr);
        level = OSDisableInterrupts();
        DlQueueFrameToken(writePtr);
        OSRestoreInterrupts(level);
        if (!lbl_8025F820) {
            lbl_8025F820 = TRUE;
            GXEnableBreakPt(writePtr);
        }

        GXSetCopyClamp(GX_CLAMP_BOTTOM);
        GXSetDispCopySrc(0, 2, lbl_8025F860->fbWidth, lbl_8025F860->efbHeight - 2);
        GXCopyDisp((RwUInt8*)lbl_8025F848 +
                       ((lbl_8025F860->fbWidth + 15) & 0xFFF0) * (lbl_8025F860->efbHeight - 2) * 2,
                   lbl_8025F7F8);
        DlNextToken();
        GXSetCopyClamp(GX_CLAMP_TOP | GX_CLAMP_BOTTOM);
        GXSetDispCopySrc(0, 0, lbl_8025F860->fbWidth, lbl_8025F860->efbHeight);
        GXFlush();
        DlSwapFrameBuffer();
        lbl_8025E7F0 = TRUE;
    }
    return TRUE;
}

static RwDevice DlDevice = {
    1.0f,
    _rwDlSystem,
    0.0f,
    0.99999994f,
    _rwDlSetRenderState,
    _rwDlGetRenderState,
    _rwDlIm2DRenderLine,
    _rwDlIm2DRenderTriangle,
    _rwDlIm2DRenderPrimitive,
    _rwDlIm2DRenderIndexedPrimitive,
    NULL,
    NULL,
    NULL,
    NULL,
};

RwDevice* _rwDeviceGetHandle(void)
{
    return &DlDevice;
}

void _rwDlTransformSetup(RwMatrix* matrix, RwInt32 flags)
{
    RwMatrix* view;
    RwMatrix combined;
    RwReal mtx[3][4];

    if (matrix) {
        combined.flags = rwMATRIXTYPEORTHONORMAL | rwMATRIXINTERNALIDENTITY;
        RwMatrixMultiply(&combined, matrix, &lbl_8025E304);
        view = &combined;
    } else {
        view = &lbl_8025E304;
    }

    mtx[0][0] = -view->right.x;
    mtx[0][1] = -view->up.x;
    mtx[0][2] = -view->at.x;
    mtx[0][3] = -view->pos.x;
    mtx[1][0] = view->right.y;
    mtx[1][1] = view->up.y;
    mtx[1][2] = view->at.y;
    mtx[1][3] = view->pos.y;
    mtx[2][0] = -view->right.z;
    mtx[2][1] = -view->up.z;
    mtx[2][2] = -view->at.z;
    mtx[2][3] = -view->pos.z;

    GXLoadPosMtxImm(mtx, GX_PNMTX0);
    if (flags) {
        GXLoadNrmMtxImm(mtx, GX_PNMTX0);
    }
    GXSetCurrentMtx(GX_PNMTX0);
}

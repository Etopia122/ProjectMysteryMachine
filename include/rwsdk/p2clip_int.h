#ifndef RWSDK_P2CLIP_INT_H
#define RWSDK_P2CLIP_INT_H

/*
 * Camera and screen space vertices shared by the generic Transform and clip
 * nodes, the camera parameters they capture per packet, and the per plane
 * clipping helpers.
 */

#include "rwsdk/baim3d_int.h"

#define rwXLOCLIP 0x01
#define rwXHICLIP 0x02
#define rwYLOCLIP 0x04
#define rwYHICLIP 0x08
#define rwZLOCLIP 0x10
#define rwZHICLIP 0x20

typedef struct RwRGBAReal {
    RwReal red;
    RwReal green;
    RwReal blue;
    RwReal alpha;
} RwRGBAReal;

typedef struct RxCamSpace3DVertex {
    /* 0x00 */ RwV3d cameraVertex;
    /* 0x0C */ RwUInt8 clipFlags;
    /* 0x0D */ RwUInt8 pad[3];
    /* 0x10 */ RwRGBAReal col;
    /* 0x20 */ RwReal u;
    /* 0x24 */ RwReal v;
} RxCamSpace3DVertex;

typedef struct RxScrSpace2DVertex {
    /* 0x00 */ RwReal x;
    /* 0x04 */ RwReal y;
    /* 0x08 */ RwReal z;
    /* 0x0C */ RwRGBA color;
    /* 0x10 */ RwReal u;
    /* 0x14 */ RwReal v;
    /* 0x18 */ RwReal recipZ;
} RxScrSpace2DVertex;

/* The camera's viewport and clip planes, captured once per packet. */
typedef struct rwTransformCamera {
    /* 0x00 */ RwReal zScale;
    /* 0x04 */ RwReal zShift;
    /* 0x08 */ RwReal nearClip;
    /* 0x0C */ RwReal farClip;
    /* 0x10 */ RwReal xScale;
    /* 0x14 */ RwReal yScale;
    /* 0x18 */ RwReal xOffset;
    /* 0x1C */ RwReal yOffset;
    /* 0x20 */ RwReal nearScreenZ;
    /* 0x24 */ RwReal farScreenZ;
} rwTransformCamera;

extern rwTransformCamera _rxTransformCamera;

/* A vertex made by clipping, and the two it was interpolated between. */
typedef struct RxInterp {
    /* 0x00 */ RxVertexIndex originalVert;
    /* 0x02 */ RxVertexIndex parentVert1;
    /* 0x04 */ RxVertexIndex parentVert2;
    /* 0x08 */ RwReal interp;
} RxInterp;

typedef RwReal (*RxClipPlaneFn)(RxCamSpace3DVertex* out, RxCamSpace3DVertex* in1, RxCamSpace3DVertex* in2,
                                RxScrSpace2DVertex* devOut, RxScrSpace2DVertex* devIn1, RxScrSpace2DVertex* devIn2);

#define rxCLIPRINGSIZE 16
#define rxCLIPRINGNEXT(i) (((i) + 1) & (rxCLIPRINGSIZE - 1))

RwInt32 _rxClipPolygonPlane(RxCluster* camVerts, RxCluster* devVerts, RxCluster* interps, RxVertexIndex* ring,
                             RwUInt32* start, RwUInt32* end, RwUInt32 plane, RxClipPlaneFn clipFn);
RwInt32 _rxClipLinePlane(RxCluster* camVerts, RxCluster* devVerts, RxVertexIndex* ring, RwUInt32* start,
                          RwUInt32* end, RwUInt32 plane, RxClipPlaneFn clipFn);

RwReal _rxClipZLoPersp(RxCamSpace3DVertex* out, RxCamSpace3DVertex* in1, RxCamSpace3DVertex* in2,
                       RxScrSpace2DVertex* devOut, RxScrSpace2DVertex* devIn1, RxScrSpace2DVertex* devIn2);
RwReal _rxClipZHiPersp(RxCamSpace3DVertex* out, RxCamSpace3DVertex* in1, RxCamSpace3DVertex* in2,
                       RxScrSpace2DVertex* devOut, RxScrSpace2DVertex* devIn1, RxScrSpace2DVertex* devIn2);
RwReal _rxClipYLoPersp(RxCamSpace3DVertex* out, RxCamSpace3DVertex* in1, RxCamSpace3DVertex* in2,
                       RxScrSpace2DVertex* devOut, RxScrSpace2DVertex* devIn1, RxScrSpace2DVertex* devIn2);
RwReal _rxClipYHiPersp(RxCamSpace3DVertex* out, RxCamSpace3DVertex* in1, RxCamSpace3DVertex* in2,
                       RxScrSpace2DVertex* devOut, RxScrSpace2DVertex* devIn1, RxScrSpace2DVertex* devIn2);
RwReal _rxClipXLoPersp(RxCamSpace3DVertex* out, RxCamSpace3DVertex* in1, RxCamSpace3DVertex* in2,
                       RxScrSpace2DVertex* devOut, RxScrSpace2DVertex* devIn1, RxScrSpace2DVertex* devIn2);
RwReal _rxClipXHiPersp(RxCamSpace3DVertex* out, RxCamSpace3DVertex* in1, RxCamSpace3DVertex* in2,
                       RxScrSpace2DVertex* devOut, RxScrSpace2DVertex* devIn1, RxScrSpace2DVertex* devIn2);
RwReal _rxClipZLoPara(RxCamSpace3DVertex* out, RxCamSpace3DVertex* in1, RxCamSpace3DVertex* in2,
                      RxScrSpace2DVertex* devOut, RxScrSpace2DVertex* devIn1, RxScrSpace2DVertex* devIn2);
RwReal _rxClipZHiPara(RxCamSpace3DVertex* out, RxCamSpace3DVertex* in1, RxCamSpace3DVertex* in2,
                      RxScrSpace2DVertex* devOut, RxScrSpace2DVertex* devIn1, RxScrSpace2DVertex* devIn2);
RwReal _rxClipYLoPara(RxCamSpace3DVertex* out, RxCamSpace3DVertex* in1, RxCamSpace3DVertex* in2,
                      RxScrSpace2DVertex* devOut, RxScrSpace2DVertex* devIn1, RxScrSpace2DVertex* devIn2);
RwReal _rxClipYHiPara(RxCamSpace3DVertex* out, RxCamSpace3DVertex* in1, RxCamSpace3DVertex* in2,
                      RxScrSpace2DVertex* devOut, RxScrSpace2DVertex* devIn1, RxScrSpace2DVertex* devIn2);
RwReal _rxClipXLoPara(RxCamSpace3DVertex* out, RxCamSpace3DVertex* in1, RxCamSpace3DVertex* in2,
                      RxScrSpace2DVertex* devOut, RxScrSpace2DVertex* devIn1, RxScrSpace2DVertex* devIn2);
RwReal _rxClipXHiPara(RxCamSpace3DVertex* out, RxCamSpace3DVertex* in1, RxCamSpace3DVertex* in2,
                      RxScrSpace2DVertex* devOut, RxScrSpace2DVertex* devIn1, RxScrSpace2DVertex* devIn2);

#endif

/*
 * Transform.csl: transforms object space vertices into camera space, works
 * out their clip flags and projects the unclipped ones to screen space. A
 * packet whose vertices are all clipped by one plane leaves through the
 * second output.
 */

#include "rwsdk/baim3d_int.h"
#include "rwsdk/rpworld_int.h"

extern RxClusterDefinition RxClObjSpace3DVertices;
extern RxClusterDefinition RxClCamSpace3DVertices;
extern RxClusterDefinition RxClScrSpace2DVertices;
extern RxClusterDefinition RxClMeshState;

#define rwPERSPECTIVE 1

#define rxGEOMETRY_TEXTURED 0x04
#define rxGEOMETRY_PRELIT 0x08
#define rxGEOMETRY_COLORED 0x100

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
    /* 0x20 */ RwUInt8 pad[8];
} rwTransformCamera;

rwTransformCamera _rxTransformCamera;

#define RxClusterResetCursor(_cl) ((_cl)->currentData = (_cl)->data)

#define RxClusterLockRead(_packet, _clusterIndex)                                                                      \
    (((RwInt32)((_packet)->inputToClusterSlot[(_clusterIndex)]) == -1)                                                 \
         ? (RxCluster*)NULL                                                                                            \
         : (RxClusterResetCursor(&(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]),               \
            &(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]))

#define OBJV RxClusterGetCursorData(objVerts, RxObjSpace3DVertex)
#define CAMV RxClusterGetCursorData(camVerts, RxCamSpace3DVertex)
#define DEVV RxClusterGetCursorData(devVerts, RxScrSpace2DVertex)

static inline void TransformSetupCamera(RwCamera* camera)
{
    RwRaster* raster = camera->frameBuffer;

    _rxTransformCamera.xOffset = raster->nOffsetX;
    _rxTransformCamera.yOffset = raster->nOffsetY;
    _rxTransformCamera.xScale = raster->width;
    _rxTransformCamera.yScale = raster->height;
    _rxTransformCamera.farClip = camera->farPlane;
    _rxTransformCamera.nearClip = camera->nearPlane;
    _rxTransformCamera.zScale = camera->zScale;
    _rxTransformCamera.zShift = camera->zShift;
}

static RwBool TransformParallel(RxPipelineNodeInstance* self, RwCamera* camera)
{
    RxCluster* devVerts;
    RwBool colored;
    RwRGBA color;
    RwBool prelit;
    RwUInt32 clipOr;
    RwBool textured;
    RwInt32 clipAnd;
    RwUInt32 numVerts;
    RwUInt32 objStride;
    RwUInt32 camStride;
    RxPacket* packet;
    RxCluster* objVerts;
    RwUInt32 devStride;
    RxCluster* meshState;
    RxCluster* camVerts;
    RxMeshStateVector* meshData;

    TransformSetupCamera(camera);

    packet = RxPacketFetch(self);
    colored = FALSE;
    prelit = FALSE;
    textured = FALSE;
    objVerts = RxClusterLockRead(packet, 0);
    camVerts = RxClusterLockWrite(packet, 1, self);
    devVerts = RxClusterLockWrite(packet, 2, self);
    meshState = RxClusterLockWrite(packet, 3, self);
    meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);
    if (!meshData->NumVertices) {
        return TRUE;
    }

    camVerts = RxClusterInitializeData(camVerts, meshData->NumVertices, sizeof(RxCamSpace3DVertex));
    devVerts = RxClusterInitializeData(devVerts, meshData->NumVertices, sizeof(RxScrSpace2DVertex));

    numVerts = meshData->NumVertices;
    if (meshData->Flags & rxGEOMETRY_COLORED) {
        colored = TRUE;
    } else if (meshData->Flags & rxGEOMETRY_PRELIT) {
        prelit = TRUE;
    } else {
        color = meshData->MatCol;
    }
    if (meshData->Flags & rxGEOMETRY_TEXTURED) {
        textured = TRUE;
    }

    objStride = objVerts->stride;
    camStride = camVerts->stride;
    devStride = devVerts->stride;
    clipOr = 0;
    clipAnd = -1;

    while (numVerts--) {
        RwMatrix* m = &meshData->Obj2Cam;
        RwReal x;
        RwReal y;
        RwReal z;
        RwUInt32 xClip;
        RwUInt32 zClip;
        RwUInt32 yClip;
        RwUInt32 clip;
        RwReal ox;
        RwReal oy;
        RwReal oz;

        ox = OBJV->objVertex.x;
        oy = OBJV->objVertex.y;
        oz = OBJV->objVertex.z;
        x = ox * m->right.x + oy * m->up.x + oz * m->at.x + m->pos.x;
        y = ox * m->right.y + oy * m->up.y + oz * m->at.y + m->pos.y;
        z = ox * m->right.z + oy * m->up.z + oz * m->at.z + m->pos.z;
        CAMV->cameraVertex.x = x;
        CAMV->cameraVertex.y = y;
        CAMV->cameraVertex.z = z;

        if (prelit) {
            RwUInt8 alpha;
            RwUInt8 blue;
            RwUInt8 green;
            RwUInt8 red;

            red = OBJV->color.red;
            green = OBJV->color.green;
            blue = OBJV->color.blue;
            alpha = OBJV->color.alpha;

            CAMV->col.red = red;
            CAMV->col.green = green;
            CAMV->col.blue = blue;
            color.red = red;
            color.green = green;
            color.blue = blue;
            color.alpha = alpha;
            CAMV->col.alpha = alpha;
        } else if (colored) {
            RwUInt8 alpha;
            RwUInt8 blue;
            RwUInt8 green;
            RwUInt8 red;

            red = OBJV->color.red;
            green = OBJV->color.green;
            blue = OBJV->color.blue;
            alpha = OBJV->color.alpha;

            CAMV->col.red = red;
            CAMV->col.green = green;
            CAMV->col.blue = blue;
            color.red = red;
            color.green = green;
            color.blue = blue;
            color.alpha = alpha;
            CAMV->col.alpha = alpha;
        } else {
            CAMV->col.red = 0.0f;
            CAMV->col.green = 0.0f;
            CAMV->col.blue = 0.0f;
            CAMV->col.alpha = 255.0f;
        }
        DEVV->color.red = color.red;
        DEVV->color.green = color.green;
        DEVV->color.blue = color.blue;
        DEVV->color.alpha = color.alpha;

        if (z < _rxTransformCamera.nearClip) {
            zClip = rwZLOCLIP;
        } else if (z > _rxTransformCamera.farClip) {
            zClip = rwZHICLIP;
        } else {
            zClip = 0;
        }
        if (y < 0.0f) {
            yClip = rwYLOCLIP;
        } else if (y > 1.0f) {
            yClip = rwYHICLIP;
        } else {
            yClip = 0;
        }
        if (x < 0.0f) {
            xClip = rwXLOCLIP;
        } else if (x > 1.0f) {
            xClip = rwXHICLIP;
        } else {
            xClip = 0;
        }
        CAMV->clipFlags = xClip | (yClip | zClip);

        clip = CAMV->clipFlags;
        clipOr |= clip;
        clipAnd &= clip;
        if (!clip) {
            DEVV->x = _rxTransformCamera.xOffset + x * _rxTransformCamera.xScale;
            DEVV->y = _rxTransformCamera.yOffset + y * _rxTransformCamera.yScale;
            DEVV->z = _rxTransformCamera.zShift + z * _rxTransformCamera.zScale;
            if (textured) {
                RwReal u = OBJV->u;
                RwReal v = OBJV->v;

                CAMV->u = u;
                CAMV->v = v;
                DEVV->u = u;
                DEVV->v = v;
            }
        } else {
            if (textured) {
                RwReal v = OBJV->v;

                CAMV->u = OBJV->u;
                CAMV->v = v;
            }
            DEVV->z = 0.0f;
        }

        objVerts->currentData = (RwUInt8*)objVerts->currentData + objStride;
        camVerts->currentData = (RwUInt8*)camVerts->currentData + camStride;
        devVerts->currentData = (RwUInt8*)devVerts->currentData + devStride;
    }

    devVerts->numUsed = meshData->NumVertices;
    camVerts->numUsed = meshData->NumVertices;
    meshData->ClipFlagsOr = clipOr;
    meshData->ClipFlagsAnd = clipAnd;

    if (clipAnd) {
        RxPacketDispatch(packet, 1, self);
    } else {
        RxPacketDispatch(packet, 0, self);
    }

    return TRUE;
}

static RwBool TransformPerspective(RxPipelineNodeInstance* self, RwCamera* camera)
{
    RxCluster* devVerts;
    RwBool colored;
    RwRGBA color;
    RwBool prelit;
    RwUInt32 clipOr;
    RwBool textured;
    RwInt32 clipAnd;
    RwUInt32 numVerts;
    RwUInt32 objStride;
    RwUInt32 camStride;
    RxPacket* packet;
    RxCluster* objVerts;
    RwUInt32 devStride;
    RxCluster* meshState;
    RxCluster* camVerts;
    RxMeshStateVector* meshData;

    TransformSetupCamera(camera);

    packet = RxPacketFetch(self);
    colored = FALSE;
    prelit = FALSE;
    textured = FALSE;
    objVerts = RxClusterLockRead(packet, 0);
    camVerts = RxClusterLockWrite(packet, 1, self);
    devVerts = RxClusterLockWrite(packet, 2, self);
    meshState = RxClusterLockWrite(packet, 3, self);
    meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);
    if (!meshData->NumVertices) {
        return TRUE;
    }

    camVerts = RxClusterInitializeData(camVerts, meshData->NumVertices, sizeof(RxCamSpace3DVertex));
    devVerts = RxClusterInitializeData(devVerts, meshData->NumVertices, sizeof(RxScrSpace2DVertex));

    numVerts = meshData->NumVertices;
    if (meshData->Flags & rxGEOMETRY_COLORED) {
        colored = TRUE;
    } else if (meshData->Flags & rxGEOMETRY_PRELIT) {
        prelit = TRUE;
    } else {
        color = meshData->MatCol;
    }
    if (meshData->Flags & rxGEOMETRY_TEXTURED) {
        textured = TRUE;
    }

    objStride = objVerts->stride;
    camStride = camVerts->stride;
    devStride = devVerts->stride;
    clipOr = 0;
    clipAnd = -1;

    while (numVerts--) {
        RwMatrix* m = &meshData->Obj2Cam;
        RwReal x;
        RwReal y;
        RwReal z;
        RwUInt32 xClip;
        RwUInt32 zClip;
        RwUInt32 yClip;
        RwUInt32 clip;
        RwReal ox;
        RwReal oy;
        RwReal oz;

        ox = OBJV->objVertex.x;
        oy = OBJV->objVertex.y;
        oz = OBJV->objVertex.z;
        x = ox * m->right.x + oy * m->up.x + oz * m->at.x + m->pos.x;
        y = ox * m->right.y + oy * m->up.y + oz * m->at.y + m->pos.y;
        z = ox * m->right.z + oy * m->up.z + oz * m->at.z + m->pos.z;
        CAMV->cameraVertex.x = x;
        CAMV->cameraVertex.y = y;
        CAMV->cameraVertex.z = z;

        if (prelit) {
            RwUInt8 alpha;
            RwUInt8 blue;
            RwUInt8 green;
            RwUInt8 red;

            red = OBJV->color.red;
            green = OBJV->color.green;
            blue = OBJV->color.blue;
            alpha = OBJV->color.alpha;

            CAMV->col.red = red;
            CAMV->col.green = green;
            CAMV->col.blue = blue;
            color.red = red;
            color.green = green;
            color.blue = blue;
            color.alpha = alpha;
            CAMV->col.alpha = alpha;
        } else if (colored) {
            RwUInt8 alpha;
            RwUInt8 blue;
            RwUInt8 green;
            RwUInt8 red;

            red = OBJV->color.red;
            green = OBJV->color.green;
            blue = OBJV->color.blue;
            alpha = OBJV->color.alpha;

            CAMV->col.red = red;
            CAMV->col.green = green;
            CAMV->col.blue = blue;
            color.red = red;
            color.green = green;
            color.blue = blue;
            color.alpha = alpha;
            CAMV->col.alpha = alpha;
        } else {
            CAMV->col.red = 0.0f;
            CAMV->col.green = 0.0f;
            CAMV->col.blue = 0.0f;
            CAMV->col.alpha = 255.0f;
        }
        DEVV->color.red = color.red;
        DEVV->color.green = color.green;
        DEVV->color.blue = color.blue;
        DEVV->color.alpha = color.alpha;

        if (z < _rxTransformCamera.nearClip) {
            zClip = rwZLOCLIP;
        } else if (z > _rxTransformCamera.farClip) {
            zClip = rwZHICLIP;
        } else {
            zClip = 0;
        }
        if (y < 0.0f) {
            yClip = rwYLOCLIP;
        } else if (y > z) {
            yClip = rwYHICLIP;
        } else {
            yClip = 0;
        }
        if (x < 0.0f) {
            xClip = rwXLOCLIP;
        } else if (x > z) {
            xClip = rwXHICLIP;
        } else {
            xClip = 0;
        }
        CAMV->clipFlags = xClip | (yClip | zClip);

        clip = CAMV->clipFlags;
        clipOr |= clip;
        clipAnd &= clip;
        if (!clip) {
            RwReal recipZ = 1.0f / z;

            DEVV->x = _rxTransformCamera.xOffset + _rxTransformCamera.xScale * (x * recipZ);
            DEVV->y = _rxTransformCamera.yOffset + _rxTransformCamera.yScale * (y * recipZ);
            DEVV->z = _rxTransformCamera.zShift + recipZ * _rxTransformCamera.zScale;
            if (textured) {
                RwReal u = OBJV->u;
                RwReal v = OBJV->v;

                CAMV->u = u;
                CAMV->v = v;
                DEVV->u = u;
                DEVV->v = v;
            }
        } else {
            if (textured) {
                RwReal v = OBJV->v;

                CAMV->u = OBJV->u;
                CAMV->v = v;
            }
            DEVV->z = 0.0f;
        }

        objVerts->currentData = (RwUInt8*)objVerts->currentData + objStride;
        camVerts->currentData = (RwUInt8*)camVerts->currentData + camStride;
        devVerts->currentData = (RwUInt8*)devVerts->currentData + devStride;
    }

    devVerts->numUsed = meshData->NumVertices;
    camVerts->numUsed = meshData->NumVertices;
    meshData->ClipFlagsOr = clipOr;
    meshData->ClipFlagsAnd = clipAnd;

    if (clipAnd) {
        RxPacketDispatch(packet, 1, self);
    } else {
        RxPacketDispatch(packet, 0, self);
    }

    return TRUE;
}

RwBool TransformNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    RwCamera* camera = (RwCamera*)RWSRCGLOBAL(curCamera);

    if (camera->projectionType != rwPERSPECTIVE) {
        return TransformParallel(self, camera);
    }
    return TransformPerspective(self, camera);
}

RxNodeDefinition* RxNodeDefinitionGetTransform(void)
{
    static RxClusterRef N1clofinterest[] = {
        { &RxClObjSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClCamSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClScrSpace2DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED },
    };

    static RxClusterValidityReq N1inputreqs[] = {
        rxCLREQ_REQUIRED, rxCLREQ_DONTWANT, rxCLREQ_DONTWANT, rxCLREQ_REQUIRED,
    };

#pragma explicit_zero_data on
    static RxClusterValid N1outcl1[] = { rxCLVALID_NOCHANGE, rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_VALID };
    static RxClusterValid N1outcl2[] = { rxCLVALID_NOCHANGE, rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_VALID };
#pragma explicit_zero_data reset

    static RwChar _TransformOut[] = "TransformOut";
    static RwChar _TransformOut2[] = "TransformOut2";

    static RxOutputSpec N1outputs[] = {
        { _TransformOut, N1outcl1, rxCLVALID_NOCHANGE },
        { _TransformOut2, N1outcl2, rxCLVALID_NOCHANGE },
    };

    static RwChar _Transform_csl[] = "Transform.csl";

    static RxNodeDefinition nodeTransformCSL = {
        _Transform_csl,
        { TransformNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 4, N1clofinterest, N1inputreqs, 2, N1outputs },
        0,
        0,
        0,
    };

    return &nodeTransformCSL;
}

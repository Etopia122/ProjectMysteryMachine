/*
 * ClipLine.csl: clips a packet's line list against the view frustum. Lines
 * wholly inside pass untouched, lines wholly outside one plane are dropped,
 * and the rest are clipped plane by plane, the new vertices projected to the
 * screen edge they were clipped to.
 */

#include "rwsdk/p2clip_int.h"
#include "rwsdk/rpworld_int.h"

extern RxClusterDefinition RxClCamSpace3DVertices;
extern RxClusterDefinition RxClScrSpace2DVertices;
extern RxClusterDefinition RxClIndices;
extern RxClusterDefinition RxClMeshState;
extern RxClusterDefinition RxClRenderState;

#define rwPERSPECTIVE 1
#define rwSHADEMODEGOURAUD 2

RwReal RwIm2DGetNearScreenZ(void);
RwReal RwIm2DGetFarScreenZ(void);
RxCluster* RxClusterResize(RxCluster* cluster, RwUInt32 minSize);
void _rxPacketDestroy(RxPacket* packet);

typedef RwBool (*ClipLineFn)(RxPipelineNodeInstance* self, RxPacket* packet);

#define RxClusterResetCursor(_cl) ((_cl)->currentData = (_cl)->data)

#define RxClusterLockRead(_packet, _clusterIndex)                                                                      \
    (((RwInt32)((_packet)->inputToClusterSlot[(_clusterIndex)]) == -1)                                                 \
         ? (RxCluster*)NULL                                                                                            \
         : (RxClusterResetCursor(&(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]),               \
            &(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]))

#define CLIPVERT(cl, T, i) ((T*)((RwUInt8*)(cl)->data + (cl)->stride * (i)))

static const RxClipPlaneFn perspClipFns[] = {
    _rxClipZLoPersp, _rxClipZHiPersp, _rxClipYLoPersp, _rxClipYHiPersp, _rxClipXLoPersp, _rxClipXHiPersp,
};

static const RxClipPlaneFn paraClipFns[] = {
    _rxClipZLoPara, _rxClipZHiPara, _rxClipYLoPara, _rxClipYHiPara, _rxClipXLoPara, _rxClipXHiPara,
};

/* The clip flags of the line now at the front of the ring. */
#define RINGFLAGS()                                                              \
    (CLIPVERT(camVerts, RxCamSpace3DVertex, ring[start])->clipFlags |            \
     CLIPVERT(camVerts, RxCamSpace3DVertex, ring[rxCLIPRINGNEXT(start)])->clipFlags)

#define CLIPPLANE(_flag, _fn)                                                                  \
    if (clipFlags & (_flag)) {                                                                 \
        if (_rxClipLinePlane(camVerts, devVerts, ring, &start, &end, (_flag), (_fn)) < 2) {     \
            goto next;                                                                         \
        }                                                                                      \
    }

/* @CLIPLINE_BEGIN@ */
static RwBool ClipLinePerspective(RxPipelineNodeInstance* self, RxPacket* packet)
{
    RxMeshStateVector* meshData;
    RxCluster* indices;
    RxCluster* meshState;
    RxCluster* camVerts;
    RxCluster* devVerts;
    RwUInt32 lineStride;
    RxRenderStateVector* rsvp;
    RwBool ok = TRUE;
    RwUInt32 numLines;
    RwUInt32 i;
    RxCluster* renderState;

    camVerts = RxClusterLockWrite(packet, 0, self);
    devVerts = RxClusterLockWrite(packet, 1, self);
    indices = RxClusterLockWrite(packet, 2, self);
    meshState = RxClusterLockWrite(packet, 3, self);
    renderState = RxClusterLockRead(packet, 4);
    if (renderState && renderState->data) {
        rsvp = RxClusterGetCursorData(renderState, RxRenderStateVector);
    } else {
        rsvp = &RXPIPELINEGLOBAL(defaultRenderState);
    }
    meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);
    lineStride = indices->stride * 2;

    if (meshData->ClipFlagsOr) {
        numLines = meshData->NumElements;
        for (i = 0; i < numLines; i++) {
            RxVertexIndex i0;
            RwUInt8 flags0;
            RwUInt32 numVerts;
            RxVertexIndex i1;
            RwUInt8 andFlags;
            RxVertexIndex* line;
            RwUInt32 keep;
            RwUInt8 clipFlags;
            RwUInt8 flags1;

            line = (RxVertexIndex*)indices->currentData;
            keep = FALSE;
            i0 = line[0];
            i1 = line[1];
            numVerts = camVerts->numUsed;
            flags0 = CLIPVERT(camVerts, RxCamSpace3DVertex, i0)->clipFlags;
            flags1 = CLIPVERT(camVerts, RxCamSpace3DVertex, i1)->clipFlags;
            clipFlags = flags0 | flags1;
            andFlags = flags0 & flags1;

            if (clipFlags) {
                if (!andFlags) {
                    RxVertexIndex ring[rxCLIPRINGSIZE];
                    RwUInt32 start;
                    RwUInt32 end;
                    RwInt32 k;

                    if (camVerts->numUsed + 12 > camVerts->numAlloced) {
                        ok = RxClusterResize(camVerts, camVerts->numAlloced + (numLines - i) + 12) != NULL;
                        if (!ok) {
                            goto check;
                        }
                    }
                    if (devVerts->numUsed + 12 > devVerts->numAlloced) {
                        ok = RxClusterResize(devVerts, devVerts->numAlloced + (numLines - i) + 12) != NULL;
                        if (!ok) {
                            goto check;
                        }
                    }

                    line = (RxVertexIndex*)indices->currentData;
                    start = 0;
                    ring[0] = i0;
                    ring[1] = i1;
                    end = 2;

                    if (clipFlags & (rwZLOCLIP | rwZHICLIP)) {
                        CLIPPLANE(rwZLOCLIP, perspClipFns[0]);
                        CLIPPLANE(rwZHICLIP, perspClipFns[1]);
                        clipFlags = RINGFLAGS();
                    }
                    if (clipFlags & (rwYLOCLIP | rwYHICLIP)) {
                        CLIPPLANE(rwYLOCLIP, perspClipFns[2]);
                        CLIPPLANE(rwYHICLIP, perspClipFns[3]);
                        clipFlags = RINGFLAGS();
                    }
                    if (clipFlags & (rwXLOCLIP | rwXHICLIP)) {
                        CLIPPLANE(rwXLOCLIP, perspClipFns[4]);
                        CLIPPLANE(rwXHICLIP, perspClipFns[5]);
                    }

                    k = start;
                    do {
                        RxVertexIndex v = ring[k];
                        RxCamSpace3DVertex* cam = CLIPVERT(camVerts, RxCamSpace3DVertex, v);
                        RwUInt8 f = cam->clipFlags;

                        if (f & 0x3F) {
                            RwReal x = cam->cameraVertex.x;
                            RwReal y = cam->cameraVertex.y;
                            RxScrSpace2DVertex* dev = CLIPVERT(devVerts, RxScrSpace2DVertex, v);
                            RwReal recipZ = 1.0f / cam->cameraVertex.z;

                            switch (f & 0x03) {
                            case rwXHICLIP:
                                dev->x = _rxTransformCamera.xScale + _rxTransformCamera.xOffset;
                                break;
                            case rwXLOCLIP:
                                dev->x = _rxTransformCamera.xOffset;
                                break;
                            default:
                                dev->x = _rxTransformCamera.xOffset + recipZ * (_rxTransformCamera.xScale * x);
                                break;
                            }
                            switch (f & 0x0F) {
                            case rwYHICLIP:
                                dev->y = _rxTransformCamera.yScale + _rxTransformCamera.yOffset;
                                break;
                            case rwYLOCLIP:
                                dev->y = _rxTransformCamera.yOffset;
                                break;
                            default:
                                dev->y = _rxTransformCamera.yOffset + recipZ * (_rxTransformCamera.yScale * y);
                                break;
                            }
                            switch (f & 0x3F) {
                            case rwZHICLIP:
                                dev->z = _rxTransformCamera.farScreenZ;
                                break;
                            case rwZLOCLIP:
                                dev->z = _rxTransformCamera.nearScreenZ;
                                break;
                            default:
                                dev->z = _rxTransformCamera.zShift + _rxTransformCamera.zScale * recipZ;
                                break;
                            }
                            dev->u = cam->u;
                            dev->v = cam->v;
                        }
                        k = rxCLIPRINGNEXT(k);
                    } while (k != (RwInt32)end);

                    if (!(rsvp->ShadeMode & rwSHADEMODEGOURAUD)) {
                        while (i0 != ring[start]) {
                            ring[end] = ring[start];
                            end = rxCLIPRINGNEXT(end);
                            start = rxCLIPRINGNEXT(start);
                        }
                        if (i0 != ring[start]) {
                            CLIPVERT(devVerts, RxScrSpace2DVertex, ring[start])->color.red =
                                CLIPVERT(devVerts, RxScrSpace2DVertex, i0)->color.red;
                            CLIPVERT(devVerts, RxScrSpace2DVertex, ring[start])->color.green =
                                CLIPVERT(devVerts, RxScrSpace2DVertex, i0)->color.green;
                            CLIPVERT(devVerts, RxScrSpace2DVertex, ring[start])->color.blue =
                                CLIPVERT(devVerts, RxScrSpace2DVertex, i0)->color.blue;
                            CLIPVERT(devVerts, RxScrSpace2DVertex, ring[start])->color.alpha =
                                CLIPVERT(devVerts, RxScrSpace2DVertex, i0)->color.alpha;
                        }
                    }

                    keep = TRUE;
                    {
                        RwUInt32 s = start;

                        line[0] = ring[s];
                        line[1] = ring[rxCLIPRINGNEXT(s)];
                    }
                }
            } else {
                keep = TRUE;
            }

        next:
            if (!keep) {
                RxVertexIndex* last = (RxVertexIndex*)((RwUInt8*)indices->data + indices->stride * ((numLines - 1) * 2));

                numLines--;
                i--;
                ((RxVertexIndex*)indices->currentData)[0] = last[0];
                ((RxVertexIndex*)indices->currentData)[1] = last[1];
                indices->numUsed -= 2;
                camVerts->numUsed = numVerts;
                devVerts->numUsed = numVerts;
            } else {
                indices->currentData = (RwUInt8*)indices->currentData + lineStride;
            }

        check:
            if (!ok) {
                _rxPacketDestroy(packet);
                return ok;
            }
        }

        meshData->NumElements = indices->numUsed >> 1;
        meshData->NumVertices = camVerts->numUsed;
    }

    if (!meshData->NumElements) {
        RxPacketDispatch(packet, 1, self);
    } else {
        RxPacketDispatch(packet, 0, self);
    }

    return ok;
}

static RwBool ClipLineParallel(RxPipelineNodeInstance* self, RxPacket* packet)
{
    RxMeshStateVector* meshData;
    RxCluster* indices;
    RxCluster* meshState;
    RxCluster* camVerts;
    RxCluster* devVerts;
    RwUInt32 lineStride;
    RxRenderStateVector* rsvp;
    RwBool ok = TRUE;
    RwUInt32 numLines;
    RwUInt32 i;
    RxCluster* renderState;

    camVerts = RxClusterLockWrite(packet, 0, self);
    devVerts = RxClusterLockWrite(packet, 1, self);
    indices = RxClusterLockWrite(packet, 2, self);
    meshState = RxClusterLockWrite(packet, 3, self);
    renderState = RxClusterLockRead(packet, 4);
    if (renderState && renderState->data) {
        rsvp = RxClusterGetCursorData(renderState, RxRenderStateVector);
    } else {
        rsvp = &RXPIPELINEGLOBAL(defaultRenderState);
    }
    meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);
    lineStride = indices->stride * 2;

    if (meshData->ClipFlagsOr) {
        numLines = meshData->NumElements;
        for (i = 0; i < numLines; i++) {
            RxVertexIndex i0;
            RwUInt8 flags0;
            RwUInt32 numVerts;
            RxVertexIndex i1;
            RwUInt8 andFlags;
            RxVertexIndex* line;
            RwUInt32 keep;
            RwUInt8 clipFlags;
            RwUInt8 flags1;

            line = (RxVertexIndex*)indices->currentData;
            keep = FALSE;
            i0 = line[0];
            i1 = line[1];
            numVerts = camVerts->numUsed;
            flags0 = CLIPVERT(camVerts, RxCamSpace3DVertex, i0)->clipFlags;
            flags1 = CLIPVERT(camVerts, RxCamSpace3DVertex, i1)->clipFlags;
            clipFlags = flags0 | flags1;
            andFlags = flags0 & flags1;

            if (clipFlags) {
                if (!andFlags) {
                    RxVertexIndex ring[rxCLIPRINGSIZE];
                    RwUInt32 start;
                    RwUInt32 end;
                    RwInt32 k;

                    if (camVerts->numUsed + 12 > camVerts->numAlloced) {
                        ok = RxClusterResize(camVerts, camVerts->numAlloced + (numLines - i) + 12) != NULL;
                        if (!ok) {
                            goto check;
                        }
                    }
                    if (devVerts->numUsed + 12 > devVerts->numAlloced) {
                        ok = RxClusterResize(devVerts, devVerts->numAlloced + (numLines - i) + 12) != NULL;
                        if (!ok) {
                            goto check;
                        }
                    }

                    line = (RxVertexIndex*)indices->currentData;
                    start = 0;
                    ring[0] = i0;
                    ring[1] = i1;
                    end = 2;

                    if (clipFlags & (rwZLOCLIP | rwZHICLIP)) {
                        CLIPPLANE(rwZLOCLIP, paraClipFns[0]);
                        CLIPPLANE(rwZHICLIP, paraClipFns[1]);
                        clipFlags = RINGFLAGS();
                    }
                    if (clipFlags & (rwYLOCLIP | rwYHICLIP)) {
                        CLIPPLANE(rwYLOCLIP, paraClipFns[2]);
                        CLIPPLANE(rwYHICLIP, paraClipFns[3]);
                        clipFlags = RINGFLAGS();
                    }
                    if (clipFlags & (rwXLOCLIP | rwXHICLIP)) {
                        CLIPPLANE(rwXLOCLIP, paraClipFns[4]);
                        CLIPPLANE(rwXHICLIP, paraClipFns[5]);
                    }

                    k = start;
                    do {
                        RxVertexIndex v = ring[k];
                        RxCamSpace3DVertex* cam = CLIPVERT(camVerts, RxCamSpace3DVertex, v);
                        RwUInt8 f = cam->clipFlags;

                        if (f & 0x3F) {
                            RwReal x = cam->cameraVertex.x;
                            RwReal y = cam->cameraVertex.y;
                            RxScrSpace2DVertex* dev = CLIPVERT(devVerts, RxScrSpace2DVertex, v);
                            RwReal recipZ = cam->cameraVertex.z;

                            switch (f & 0x03) {
                            case rwXHICLIP:
                                dev->x = _rxTransformCamera.xScale + _rxTransformCamera.xOffset;
                                break;
                            case rwXLOCLIP:
                                dev->x = _rxTransformCamera.xOffset;
                                break;
                            default:
                                dev->x = _rxTransformCamera.xOffset + _rxTransformCamera.xScale * x;
                                break;
                            }
                            switch (f & 0x0F) {
                            case rwYHICLIP:
                                dev->y = _rxTransformCamera.yScale + _rxTransformCamera.yOffset;
                                break;
                            case rwYLOCLIP:
                                dev->y = _rxTransformCamera.yOffset;
                                break;
                            default:
                                dev->y = _rxTransformCamera.yOffset + _rxTransformCamera.yScale * y;
                                break;
                            }
                            switch (f & 0x3F) {
                            case rwZHICLIP:
                                dev->z = _rxTransformCamera.farScreenZ;
                                break;
                            case rwZLOCLIP:
                                dev->z = _rxTransformCamera.nearScreenZ;
                                break;
                            default:
                                dev->z = _rxTransformCamera.zShift + _rxTransformCamera.zScale * recipZ;
                                break;
                            }
                            dev->u = cam->u;
                            dev->v = cam->v;
                        }
                        k = rxCLIPRINGNEXT(k);
                    } while (k != (RwInt32)end);

                    if (!(rsvp->ShadeMode & rwSHADEMODEGOURAUD)) {
                        while (i0 != ring[start]) {
                            ring[end] = ring[start];
                            end = rxCLIPRINGNEXT(end);
                            start = rxCLIPRINGNEXT(start);
                        }
                        if (i0 != ring[start]) {
                            CLIPVERT(devVerts, RxScrSpace2DVertex, ring[start])->color.red =
                                CLIPVERT(devVerts, RxScrSpace2DVertex, i0)->color.red;
                            CLIPVERT(devVerts, RxScrSpace2DVertex, ring[start])->color.green =
                                CLIPVERT(devVerts, RxScrSpace2DVertex, i0)->color.green;
                            CLIPVERT(devVerts, RxScrSpace2DVertex, ring[start])->color.blue =
                                CLIPVERT(devVerts, RxScrSpace2DVertex, i0)->color.blue;
                            CLIPVERT(devVerts, RxScrSpace2DVertex, ring[start])->color.alpha =
                                CLIPVERT(devVerts, RxScrSpace2DVertex, i0)->color.alpha;
                        }
                    }

                    keep = TRUE;
                    {
                        RwUInt32 s = start;

                        line[0] = ring[s];
                        line[1] = ring[rxCLIPRINGNEXT(s)];
                    }
                }
            } else {
                keep = TRUE;
            }

        next:
            if (!keep) {
                RxVertexIndex* last = (RxVertexIndex*)((RwUInt8*)indices->data + indices->stride * ((numLines - 1) * 2));

                numLines--;
                i--;
                ((RxVertexIndex*)indices->currentData)[0] = last[0];
                ((RxVertexIndex*)indices->currentData)[1] = last[1];
                indices->numUsed -= 2;
                camVerts->numUsed = numVerts;
                devVerts->numUsed = numVerts;
            } else {
                indices->currentData = (RwUInt8*)indices->currentData + lineStride;
            }

        check:
            if (!ok) {
                _rxPacketDestroy(packet);
                return ok;
            }
        }

        meshData->NumElements = indices->numUsed >> 1;
        meshData->NumVertices = camVerts->numUsed;
    }

    if (!meshData->NumElements) {
        RxPacketDispatch(packet, 1, self);
    } else {
        RxPacketDispatch(packet, 0, self);
    }

    return ok;
}
/* @CLIPLINE_END@ */

static ClipLineFn clipLineFns[] = { ClipLineParallel, ClipLinePerspective };

RwBool ClipLineNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    RwCamera* camera = (RwCamera*)RWSRCGLOBAL(curCamera);
    RwRaster* raster = camera->frameBuffer;
    ClipLineFn fn = clipLineFns[camera->projectionType == rwPERSPECTIVE];

    _rxTransformCamera.zScale = camera->zScale;
    _rxTransformCamera.zShift = camera->zShift;
    _rxTransformCamera.xScale = raster->width;
    _rxTransformCamera.yScale = raster->height;
    _rxTransformCamera.xOffset = raster->nOffsetX;
    _rxTransformCamera.yOffset = raster->nOffsetY;
    _rxTransformCamera.nearClip = camera->nearPlane;
    _rxTransformCamera.farClip = camera->farPlane;
    _rxTransformCamera.nearScreenZ = RwIm2DGetNearScreenZ();
    _rxTransformCamera.farScreenZ = RwIm2DGetFarScreenZ();

    return fn(self, RxPacketFetch(self));
}

RxNodeDefinition* RxNodeDefinitionGetClipLine(void)
{
    static RxClusterRef N1clofinterest[] = {
        { &RxClCamSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClScrSpace2DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClIndices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClRenderState, rxCLALLOWABSENT, rxCLRESERVED },
    };

    static RxClusterValidityReq N1inputreqs[] = {
        rxCLREQ_REQUIRED, rxCLREQ_REQUIRED, rxCLREQ_REQUIRED, rxCLREQ_REQUIRED, rxCLREQ_OPTIONAL,
    };

#pragma explicit_zero_data on
    static RxClusterValid N1outcl1[] = {
        rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_NOCHANGE,
    };
    static RxClusterValid N1outcl2[] = {
        rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_INVALID, rxCLVALID_VALID, rxCLVALID_NOCHANGE,
    };
#pragma explicit_zero_data reset

    static RwChar _ClipOut[] = "ClipOut";
    static RwChar _ClipAllClipped[] = "ClipAllClipped";

    static RxOutputSpec N1outputs[] = {
        { _ClipOut, N1outcl1, rxCLVALID_NOCHANGE },
        { _ClipAllClipped, N1outcl2, rxCLVALID_NOCHANGE },
    };

    static RwChar _ClipLine_csl[] = "ClipLine.csl";

    static RxNodeDefinition nodeClipLineCSL = {
        _ClipLine_csl,
        { ClipLineNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 5, N1clofinterest, N1inputreqs, 2, N1outputs },
        0,
        0,
        0,
    };

    return &nodeClipLineCSL;
}

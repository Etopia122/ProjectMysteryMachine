/*
 * ClipTriangle.csl: clips a packet's triangle list against the view
 * frustum. Clipped triangles become fans of new triangles appended to the
 * index list, and an optional interpolants cluster records how each new
 * vertex was made so later nodes can interpolate their own data.
 */

#include "rwsdk/p2clip_int.h"
#include "rwsdk/rpworld_int.h"

extern RxClusterDefinition RxClCamSpace3DVertices;
extern RxClusterDefinition RxClScrSpace2DVertices;
extern RxClusterDefinition RxClIndices;
extern RxClusterDefinition RxClMeshState;
extern RxClusterDefinition RxClRenderState;
extern RxClusterDefinition RxClInterpolants;

#define rwPERSPECTIVE 1
#define rwSHADEMODEGOURAUD 2

RwReal RwIm2DGetNearScreenZ(void);
RwReal RwIm2DGetFarScreenZ(void);
RxCluster* RxClusterResize(RxCluster* cluster, RwUInt32 minSize);
void _rxPacketDestroy(RxPacket* packet);

typedef RwBool (*ClipTriangleFn)(RxPipelineNodeInstance* self, RxPacket* packet);

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

/* The combined clip flags of the polygon now in the ring. */
#define RINGFLAGS()                                                                  \
    do {                                                                             \
        RwInt32 _k = start;                                                          \
                                                                                     \
        clipFlags = CLIPVERT(camVerts, RxCamSpace3DVertex, ring[_k])->clipFlags;     \
        _k = rxCLIPRINGNEXT(_k);                                                     \
        do {                                                                         \
            clipFlags |= CLIPVERT(camVerts, RxCamSpace3DVertex, ring[_k])->clipFlags; \
            _k = rxCLIPRINGNEXT(_k);                                                 \
        } while (_k != (RwInt32)end);                                                \
    } while (0)

#define CLIPPLANE(_flag, _fn)                                                                                 \
    if (clipFlags & (_flag)) {                                                                                \
        if (_rxClipPolygonPlane(camVerts, devVerts, interps, ring, &start, &end, (_flag), (_fn)) < 3) {        \
            goto check;                                                                                       \
        }                                                                                                     \
    }

static RwBool ClipTrianglePerspective(RxPipelineNodeInstance* self, RxPacket* packet)
{
    RxCluster* meshState;
    RwUInt32 i;

    RxCluster* devVerts;
    RxCluster* renderState;
    RxMeshStateVector* meshData;
    RwUInt32 triStride;
    RxCluster* indices;
    RxCluster* interps;
    RxRenderStateVector* rsvp;
    RwBool ok = TRUE;
    RwUInt32 numTris;
    RxCluster* camVerts;
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
    interps = RxClusterLockWrite(packet, 5, self);
    meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);
    triStride = indices->stride;
    if (interps) {
        interps->stride = sizeof(RxInterp);
        interps->numUsed = 0;
    }

    if (meshData->ClipFlagsOr) {
        triStride *= 3;
        numTris = meshData->NumElements;
        for (i = 0; i < numTris; i++) {
            RxVertexIndex i0;
            RwUInt32 numKept;
            RxVertexIndex i1;
            RxVertexIndex i2;
            RwUInt8 flags2;
            RwUInt8 andFlags;
            RwUInt32 numVerts;
            RwUInt8 flags1;
            RwUInt8 flags0;
            RxVertexIndex* tri;
            RwUInt8 clipFlags;
            RxVertexIndex numVertsIdx;

            tri = (RxVertexIndex*)indices->currentData;
            numKept = 0;
            numVerts = camVerts->numUsed;
            numVertsIdx = numVerts;
            i0 = tri[0];
            i1 = tri[1];
            i2 = tri[2];
            flags0 = CLIPVERT(camVerts, RxCamSpace3DVertex, i0)->clipFlags;
            flags1 = CLIPVERT(camVerts, RxCamSpace3DVertex, i1)->clipFlags;
            flags2 = CLIPVERT(camVerts, RxCamSpace3DVertex, i2)->clipFlags;
            clipFlags = flags2 | (flags0 | flags1);
            andFlags = flags2 & (flags0 & flags1);

            if (clipFlags) {
                if (!andFlags) {
                    RxVertexIndex ring[rxCLIPRINGSIZE];
                    RwUInt32 start;
                    RwUInt32 end;
                    RwInt32 k;
                    RxVertexIndex pivot;

                    if (indices->numUsed + 24 > indices->numAlloced) {
                        ok = RxClusterResize(indices, indices->numAlloced + (numTris - i) + 24) != NULL;
                        if (!ok) {
                            goto check;
                        }
                        indices->currentData = (RwUInt8*)indices->data + indices->stride * (i * 3);
                    }
                    if (camVerts->numUsed + 12 > camVerts->numAlloced) {
                        ok = RxClusterResize(camVerts, camVerts->numAlloced + (numTris - i) + 12) != NULL;
                        if (!ok) {
                            goto check;
                        }
                    }
                    if (devVerts->numUsed + 12 > devVerts->numAlloced) {
                        ok = RxClusterResize(devVerts, devVerts->numAlloced + (numTris - i) + 12) != NULL;
                        if (!ok) {
                            goto check;
                        }
                    }
                    if (interps && interps->numUsed + 12 > interps->numAlloced) {
                        RwUInt32 size = interps->numAlloced + (numTris - i) + 12;
                        RxCluster* cl;

                        cl = !interps->numAlloced ? RxClusterInitializeData(interps, size, sizeof(RxInterp))
                                                  : RxClusterResize(interps, size);
                        if (!cl) {
                            ok = FALSE;
                            goto check;
                        }
                        interps->currentData = (RwUInt8*)interps->data + interps->stride * interps->numUsed;
                    }

                    tri = (RxVertexIndex*)indices->currentData;
                    start = 0;
                    ring[0] = i0;
                    ring[1] = i1;
                    ring[2] = i2;
                    end = 3;

                    if (clipFlags & (rwZLOCLIP | rwZHICLIP)) {
                        CLIPPLANE(rwZLOCLIP, perspClipFns[0]);
                        CLIPPLANE(rwZHICLIP, perspClipFns[1]);
                        RINGFLAGS();
                    }
                    if (clipFlags & (rwYLOCLIP | rwYHICLIP)) {
                        CLIPPLANE(rwYLOCLIP, perspClipFns[2]);
                        CLIPPLANE(rwYHICLIP, perspClipFns[3]);
                        RINGFLAGS();
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

                    pivot = ring[start];
                    if (!(rsvp->ShadeMode & rwSHADEMODEGOURAUD)) {
                        while (pivot != i2 && pivot < numVertsIdx) {
                            ring[end] = ring[start];
                            end = rxCLIPRINGNEXT(end);
                            start = rxCLIPRINGNEXT(start);
                            pivot = ring[start];
                        }
                        if (pivot != i2) {
                            CLIPVERT(devVerts, RxScrSpace2DVertex, pivot)->color.red =
                                CLIPVERT(devVerts, RxScrSpace2DVertex, i2)->color.red;
                            CLIPVERT(devVerts, RxScrSpace2DVertex, pivot)->color.green =
                                CLIPVERT(devVerts, RxScrSpace2DVertex, i2)->color.green;
                            CLIPVERT(devVerts, RxScrSpace2DVertex, pivot)->color.blue =
                                CLIPVERT(devVerts, RxScrSpace2DVertex, i2)->color.blue;
                            CLIPVERT(devVerts, RxScrSpace2DVertex, pivot)->color.alpha =
                                CLIPVERT(devVerts, RxScrSpace2DVertex, i2)->color.alpha;
                        }
                    }

                    numKept = 1;
                    {
                        RwInt32 prev = (start + 2) & (rxCLIPRINGSIZE - 1);
                        RwInt32 cur = rxCLIPRINGNEXT(prev);

                        tri[0] = ring[rxCLIPRINGNEXT(start)];
                        tri[1] = ring[prev];
                        tri[2] = pivot;
                        indices->currentData = (RwUInt8*)indices->data + indices->stride * (indices->numUsed - 3);
                        while (cur != (RwInt32)end) {
                            RxVertexIndex* fan;

                            indices->currentData = (RwUInt8*)indices->currentData + triStride;
                            numKept++;
                            fan = (RxVertexIndex*)indices->currentData;
                            fan[0] = ring[prev];
                            fan[1] = ring[cur];
                            fan[2] = pivot;
                            prev = cur;
                            cur = rxCLIPRINGNEXT(cur);
                        }
                        indices->currentData = tri;
                    }
                }
            } else {
                numKept = 1;
            }

        check:
            if (!numKept) {
                RwUInt32 numNew = camVerts->numUsed - numVerts;
                RxVertexIndex* end3 = (RxVertexIndex*)((RwUInt8*)indices->data + indices->stride * (indices->numUsed - 3));
                RxVertexIndex* last = (RxVertexIndex*)((RwUInt8*)indices->data + indices->stride * ((numTris - 1) * 3));

                numTris--;
                i--;
                ((RxVertexIndex*)indices->currentData)[0] = last[0];
                ((RxVertexIndex*)indices->currentData)[1] = last[1];
                ((RxVertexIndex*)indices->currentData)[2] = last[2];
                last[0] = end3[0];
                last[1] = end3[1];
                last[2] = end3[2];
                indices->numUsed -= 3;
                camVerts->numUsed = numVerts;
                devVerts->numUsed = numVerts;
                if (interps) {
                    RwUInt32 stride = interps->stride;

                    interps->numUsed -= numNew;
                    while (numNew--) {
                        interps->currentData = (RwUInt8*)interps->currentData - stride;
                    }
                }
            } else {
                indices->numUsed += (numKept - 1) * 3;
                indices->currentData = (RwUInt8*)indices->currentData + triStride;
            }

            if (!ok) {
                _rxPacketDestroy(packet);
                return ok;
            }
        }

        meshData->NumElements = indices->numUsed / 3;
        meshData->NumVertices = camVerts->numUsed;
    }

    if (!meshData->NumElements) {
        RxPacketDispatch(packet, 1, self);
    } else {
        RxPacketDispatch(packet, 0, self);
    }

    return ok;
}

static RwBool ClipTriangleParallel(RxPipelineNodeInstance* self, RxPacket* packet)
{
    RxCluster* meshState;
    RwUInt32 i;

    RxCluster* devVerts;
    RxCluster* renderState;
    RxMeshStateVector* meshData;
    RwUInt32 triStride;
    RxCluster* indices;
    RxCluster* interps;
    RxRenderStateVector* rsvp;
    RwBool ok = TRUE;
    RwUInt32 numTris;
    RxCluster* camVerts;
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
    interps = RxClusterLockWrite(packet, 5, self);
    meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);
    triStride = indices->stride;
    if (interps) {
        interps->stride = sizeof(RxInterp);
        interps->numUsed = 0;
    }

    if (meshData->ClipFlagsOr) {
        triStride *= 3;
        numTris = meshData->NumElements;
        for (i = 0; i < numTris; i++) {
            RxVertexIndex i0;
            RwUInt32 numKept;
            RxVertexIndex i1;
            RxVertexIndex i2;
            RwUInt8 flags2;
            RwUInt8 andFlags;
            RwUInt32 numVerts;
            RwUInt8 flags1;
            RwUInt8 flags0;
            RxVertexIndex* tri;
            RwUInt8 clipFlags;
            RxVertexIndex numVertsIdx;

            tri = (RxVertexIndex*)indices->currentData;
            numKept = 0;
            numVerts = camVerts->numUsed;
            numVertsIdx = numVerts;
            i0 = tri[0];
            i1 = tri[1];
            i2 = tri[2];
            flags0 = CLIPVERT(camVerts, RxCamSpace3DVertex, i0)->clipFlags;
            flags1 = CLIPVERT(camVerts, RxCamSpace3DVertex, i1)->clipFlags;
            flags2 = CLIPVERT(camVerts, RxCamSpace3DVertex, i2)->clipFlags;
            clipFlags = flags2 | (flags0 | flags1);
            andFlags = flags2 & (flags0 & flags1);

            if (clipFlags) {
                if (!andFlags) {
                    RxVertexIndex ring[rxCLIPRINGSIZE];
                    RwUInt32 start;
                    RwUInt32 end;
                    RwInt32 k;
                    RxVertexIndex pivot;

                    if (indices->numUsed + 24 > indices->numAlloced) {
                        ok = RxClusterResize(indices, indices->numAlloced + (numTris - i) + 24) != NULL;
                        if (!ok) {
                            goto check;
                        }
                        indices->currentData = (RwUInt8*)indices->data + indices->stride * (i * 3);
                    }
                    if (camVerts->numUsed + 12 > camVerts->numAlloced) {
                        ok = RxClusterResize(camVerts, camVerts->numAlloced + (numTris - i) + 12) != NULL;
                        if (!ok) {
                            goto check;
                        }
                    }
                    if (devVerts->numUsed + 12 > devVerts->numAlloced) {
                        ok = RxClusterResize(devVerts, devVerts->numAlloced + (numTris - i) + 12) != NULL;
                        if (!ok) {
                            goto check;
                        }
                    }
                    if (interps && interps->numUsed + 12 > interps->numAlloced) {
                        RwUInt32 size = interps->numAlloced + (numTris - i) + 12;
                        RxCluster* cl;

                        cl = !interps->numAlloced ? RxClusterInitializeData(interps, size, sizeof(RxInterp))
                                                  : RxClusterResize(interps, size);
                        if (!cl) {
                            ok = FALSE;
                            goto check;
                        }
                        interps->currentData = (RwUInt8*)interps->data + interps->stride * interps->numUsed;
                    }

                    tri = (RxVertexIndex*)indices->currentData;
                    start = 0;
                    ring[0] = i0;
                    ring[1] = i1;
                    ring[2] = i2;
                    end = 3;

                    if (clipFlags & (rwZLOCLIP | rwZHICLIP)) {
                        CLIPPLANE(rwZLOCLIP, paraClipFns[0]);
                        CLIPPLANE(rwZHICLIP, paraClipFns[1]);
                        RINGFLAGS();
                    }
                    if (clipFlags & (rwYLOCLIP | rwYHICLIP)) {
                        CLIPPLANE(rwYLOCLIP, paraClipFns[2]);
                        CLIPPLANE(rwYHICLIP, paraClipFns[3]);
                        RINGFLAGS();
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

                    pivot = ring[start];
                    if (!(rsvp->ShadeMode & rwSHADEMODEGOURAUD)) {
                        while (pivot != i2 && pivot < numVertsIdx) {
                            ring[end] = ring[start];
                            end = rxCLIPRINGNEXT(end);
                            start = rxCLIPRINGNEXT(start);
                            pivot = ring[start];
                        }
                        if (pivot != i2) {
                            CLIPVERT(devVerts, RxScrSpace2DVertex, pivot)->color.red =
                                CLIPVERT(devVerts, RxScrSpace2DVertex, i2)->color.red;
                            CLIPVERT(devVerts, RxScrSpace2DVertex, pivot)->color.green =
                                CLIPVERT(devVerts, RxScrSpace2DVertex, i2)->color.green;
                            CLIPVERT(devVerts, RxScrSpace2DVertex, pivot)->color.blue =
                                CLIPVERT(devVerts, RxScrSpace2DVertex, i2)->color.blue;
                            CLIPVERT(devVerts, RxScrSpace2DVertex, pivot)->color.alpha =
                                CLIPVERT(devVerts, RxScrSpace2DVertex, i2)->color.alpha;
                        }
                    }

                    numKept = 1;
                    {
                        RwInt32 prev = (start + 2) & (rxCLIPRINGSIZE - 1);
                        RwInt32 cur = rxCLIPRINGNEXT(prev);

                        tri[0] = ring[rxCLIPRINGNEXT(start)];
                        tri[1] = ring[prev];
                        tri[2] = pivot;
                        indices->currentData = (RwUInt8*)indices->data + indices->stride * (indices->numUsed - 3);
                        while (cur != (RwInt32)end) {
                            RxVertexIndex* fan;

                            indices->currentData = (RwUInt8*)indices->currentData + triStride;
                            numKept++;
                            fan = (RxVertexIndex*)indices->currentData;
                            fan[0] = ring[prev];
                            fan[1] = ring[cur];
                            fan[2] = pivot;
                            prev = cur;
                            cur = rxCLIPRINGNEXT(cur);
                        }
                        indices->currentData = tri;
                    }
                }
            } else {
                numKept = 1;
            }

        check:
            if (!numKept) {
                RwUInt32 numNew = camVerts->numUsed - numVerts;
                RxVertexIndex* end3 = (RxVertexIndex*)((RwUInt8*)indices->data + indices->stride * (indices->numUsed - 3));
                RxVertexIndex* last = (RxVertexIndex*)((RwUInt8*)indices->data + indices->stride * ((numTris - 1) * 3));

                numTris--;
                i--;
                ((RxVertexIndex*)indices->currentData)[0] = last[0];
                ((RxVertexIndex*)indices->currentData)[1] = last[1];
                ((RxVertexIndex*)indices->currentData)[2] = last[2];
                last[0] = end3[0];
                last[1] = end3[1];
                last[2] = end3[2];
                indices->numUsed -= 3;
                camVerts->numUsed = numVerts;
                devVerts->numUsed = numVerts;
                if (interps) {
                    RwUInt32 stride = interps->stride;

                    interps->numUsed -= numNew;
                    while (numNew--) {
                        interps->currentData = (RwUInt8*)interps->currentData - stride;
                    }
                }
            } else {
                indices->numUsed += (numKept - 1) * 3;
                indices->currentData = (RwUInt8*)indices->currentData + triStride;
            }

            if (!ok) {
                _rxPacketDestroy(packet);
                return ok;
            }
        }

        meshData->NumElements = indices->numUsed / 3;
        meshData->NumVertices = camVerts->numUsed;
    }

    if (!meshData->NumElements) {
        RxPacketDispatch(packet, 1, self);
    } else {
        RxPacketDispatch(packet, 0, self);
    }

    return ok;
}

static ClipTriangleFn clipTriangleFns[] = { ClipTriangleParallel, ClipTrianglePerspective };

RwBool ClipTriangleNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    RwCamera* camera = (RwCamera*)RWSRCGLOBAL(curCamera);
    RwRaster* raster = camera->frameBuffer;
    ClipTriangleFn fn = clipTriangleFns[camera->projectionType == rwPERSPECTIVE];

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

RxNodeDefinition* RxNodeDefinitionGetClipTriangle(void)
{
    static RxClusterRef N1clofinterest[] = {
        { &RxClCamSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClScrSpace2DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClIndices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClRenderState, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClInterpolants, rxCLALLOWABSENT, rxCLRESERVED },
    };

    static RxClusterValidityReq N1inputreqs[] = {
        rxCLREQ_REQUIRED, rxCLREQ_REQUIRED, rxCLREQ_REQUIRED, rxCLREQ_REQUIRED, rxCLREQ_OPTIONAL, rxCLREQ_OPTIONAL,
    };

#pragma explicit_zero_data on
    static RxClusterValid N1outcl1[] = {
        rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_NOCHANGE, rxCLVALID_VALID,
    };
    static RxClusterValid N1outcl2[] = {
        rxCLVALID_VALID, rxCLVALID_VALID, rxCLVALID_INVALID, rxCLVALID_VALID, rxCLVALID_NOCHANGE, rxCLVALID_INVALID,
    };
#pragma explicit_zero_data reset

    static RwChar _ClipOut[] = "ClipOut";
    static RwChar _ClipAllClipped[] = "ClipAllClipped";

    static RxOutputSpec N1outputs[] = {
        { _ClipOut, N1outcl1, rxCLVALID_NOCHANGE },
        { _ClipAllClipped, N1outcl2, rxCLVALID_NOCHANGE },
    };

    static RwChar _ClipTriangle_csl[] = "ClipTriangle.csl";

    static RxNodeDefinition nodeClipTriangleCSL = {
        _ClipTriangle_csl,
        { ClipTriangleNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 6, N1clofinterest, N1inputreqs, 2, N1outputs },
        0,
        0,
        0,
    };

    return &nodeClipTriangleCSL;
}

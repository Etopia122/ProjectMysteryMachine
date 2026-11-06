/*
 * Clipping helpers for the generic clip nodes: Sutherland-Hodgman clipping
 * of a ring of vertex indices against one frustum plane, and the per plane
 * functions that make the new vertex where an edge crosses it.
 */

#include "rwsdk/p2clip_int.h"

#define CLIPVERT(cl, T, i) ((T*)((RwUInt8*)(cl)->data + (cl)->stride * (i)))

static RwUInt32 ClipPolygonPlaneInterp(RxCluster* camVerts, RxCluster* devVerts, RxCluster* interps,
                                       RxVertexIndex* ring, RwUInt32* start, RwUInt32* end, RwUInt32 plane,
                                       RxClipPlaneFn clipFn)
{
    RxScrSpace2DVertex* prevDev;
    RwInt32 cur;
    RwUInt32 out;
    RxCamSpace3DVertex* curCam;
    RwInt32 last;
    RxScrSpace2DVertex* curDev;
    RxCamSpace3DVertex* prevCam;
    RxVertexIndex prevIdx;
    RxVertexIndex curIdx;

    out = *end;
    cur = *start;
    last = out;
    prevIdx = ring[(out - 1) & (rxCLIPRINGSIZE - 1)];
    curIdx = ring[cur];
    prevCam = CLIPVERT(camVerts, RxCamSpace3DVertex, prevIdx);
    prevDev = CLIPVERT(devVerts, RxScrSpace2DVertex, prevIdx);
    curCam = CLIPVERT(camVerts, RxCamSpace3DVertex, curIdx);
    curDev = CLIPVERT(devVerts, RxScrSpace2DVertex, curIdx);

    do {
        if (!(prevCam->clipFlags & plane)) {
            ring[out] = prevIdx;
            out = rxCLIPRINGNEXT(out);
        }

        if ((prevCam->clipFlags ^ curCam->clipFlags) & plane) {
            RxVertexIndex newIdx;
            RxInterp* interp;
            RxCamSpace3DVertex* newCam;
            RxScrSpace2DVertex* newDev;

            interp = (RxInterp*)interps->currentData;
            newIdx = camVerts->numUsed++;
            devVerts->numUsed++;
            ring[out] = newIdx;
            out = rxCLIPRINGNEXT(out);
            newCam = CLIPVERT(camVerts, RxCamSpace3DVertex, newIdx);
            newDev = CLIPVERT(devVerts, RxScrSpace2DVertex, newIdx);
            if (prevCam->clipFlags & plane) {
                interp->interp = clipFn(newCam, prevCam, curCam, newDev, prevDev, curDev);
                interp->originalVert = newIdx;
                interp->parentVert1 = prevIdx;
                interp->parentVert2 = curIdx;
            } else {
                interp->interp = clipFn(newCam, curCam, prevCam, newDev, curDev, prevDev);
                interp->originalVert = newIdx;
                interp->parentVert1 = curIdx;
                interp->parentVert2 = prevIdx;
            }
            interps->numUsed++;
            interps->currentData = (RwUInt8*)interps->currentData + interps->stride;
        }

        cur = rxCLIPRINGNEXT(cur);
        prevIdx = curIdx;
        curIdx = ring[cur];
        prevCam = curCam;
        prevDev = curDev;
        curCam = CLIPVERT(camVerts, RxCamSpace3DVertex, curIdx);
        curDev = CLIPVERT(devVerts, RxScrSpace2DVertex, curIdx);
    } while (cur != last);

    *start = cur;
    *end = out;
    return (out - cur) & (rxCLIPRINGSIZE - 1);
}

static RwUInt32 ClipPolygonPlaneSimple(RxCluster* camVerts, RxCluster* devVerts, RxCluster* interps,
                                       RxVertexIndex* ring, RwUInt32* start, RwUInt32* end, RwUInt32 plane,
                                       RxClipPlaneFn clipFn)
{
    RwInt32 last;
    RwInt32 cur;
    RxCamSpace3DVertex* prevCam;
    RwUInt32 out;
    RxCamSpace3DVertex* curCam;
    RxScrSpace2DVertex* prevDev;
    RxVertexIndex prevIdx;
    RxScrSpace2DVertex* curDev;
    RxVertexIndex curIdx;

    out = *end;
    cur = *start;
    last = out;
    prevIdx = ring[(out - 1) & (rxCLIPRINGSIZE - 1)];
    curIdx = ring[cur];
    prevCam = CLIPVERT(camVerts, RxCamSpace3DVertex, prevIdx);
    prevDev = CLIPVERT(devVerts, RxScrSpace2DVertex, prevIdx);
    curCam = CLIPVERT(camVerts, RxCamSpace3DVertex, curIdx);
    curDev = CLIPVERT(devVerts, RxScrSpace2DVertex, curIdx);

    do {
        if (!(prevCam->clipFlags & plane)) {
            ring[out] = prevIdx;
            out = rxCLIPRINGNEXT(out);
        }

        if ((prevCam->clipFlags ^ curCam->clipFlags) & plane) {
            RxVertexIndex newIdx = camVerts->numUsed++;
            RxCamSpace3DVertex* newCam;
            RxScrSpace2DVertex* newDev;

            devVerts->numUsed++;
            ring[out] = newIdx;
            out = rxCLIPRINGNEXT(out);
            newCam = CLIPVERT(camVerts, RxCamSpace3DVertex, newIdx);
            newDev = CLIPVERT(devVerts, RxScrSpace2DVertex, newIdx);
            if (prevCam->clipFlags & plane) {
                clipFn(newCam, prevCam, curCam, newDev, prevDev, curDev);
            } else {
                clipFn(newCam, curCam, prevCam, newDev, curDev, prevDev);
            }
        }

        cur = rxCLIPRINGNEXT(cur);
        prevIdx = curIdx;
        curIdx = ring[cur];
        prevCam = curCam;
        prevDev = curDev;
        curCam = CLIPVERT(camVerts, RxCamSpace3DVertex, curIdx);
        curDev = CLIPVERT(devVerts, RxScrSpace2DVertex, curIdx);
    } while (cur != last);

    *start = cur;
    *end = out;
    return (out - cur) & (rxCLIPRINGSIZE - 1);
}

RwInt32 _rxClipPolygonPlane(RxCluster* camVerts, RxCluster* devVerts, RxCluster* interps, RxVertexIndex* ring,
                             RwUInt32* start, RwUInt32* end, RwUInt32 plane, RxClipPlaneFn clipFn)
{
    return (interps ? ClipPolygonPlaneInterp : ClipPolygonPlaneSimple)(camVerts, devVerts, interps, ring, start, end,
                                                                      plane, clipFn);
}

RwInt32 _rxClipLinePlane(RxCluster* camVerts, RxCluster* devVerts, RxVertexIndex* ring, RwUInt32* start,
                          RwUInt32* end, RwUInt32 plane, RxClipPlaneFn clipFn)
{
    RxScrSpace2DVertex* dev1;
    RwUInt32 out;
    RxCamSpace3DVertex* cam0;
    RxCamSpace3DVertex* cam1;
    RxVertexIndex idx0;
    RwUInt32 last;
    RxVertexIndex idx1;
    RxScrSpace2DVertex* dev0;
    RwUInt32 first;

    first = *start;
    out = *end;
    last = out;
    idx1 = ring[rxCLIPRINGNEXT(first)];
    idx0 = ring[first];
    cam1 = CLIPVERT(camVerts, RxCamSpace3DVertex, idx1);
    dev1 = CLIPVERT(devVerts, RxScrSpace2DVertex, idx1);
    cam0 = CLIPVERT(camVerts, RxCamSpace3DVertex, idx0);
    dev0 = CLIPVERT(devVerts, RxScrSpace2DVertex, idx0);

    if (!(cam1->clipFlags & plane)) {
        ring[out] = idx1;
        out = rxCLIPRINGNEXT(out);
    }

    if ((cam1->clipFlags ^ cam0->clipFlags) & plane) {
        RxVertexIndex newIdx = camVerts->numUsed++;
        RxCamSpace3DVertex* newCam;
        RxScrSpace2DVertex* newDev;

        devVerts->numUsed++;
        ring[out] = newIdx;
        out = rxCLIPRINGNEXT(out);
        newCam = CLIPVERT(camVerts, RxCamSpace3DVertex, newIdx);
        newDev = CLIPVERT(devVerts, RxScrSpace2DVertex, newIdx);
        if (cam1->clipFlags & plane) {
            clipFn(newCam, cam1, cam0, newDev, dev1, dev0);
        } else {
            clipFn(newCam, cam0, cam1, newDev, dev0, dev1);
        }
    }

    if (!(cam0->clipFlags & plane)) {
        ring[out] = idx0;
        out = rxCLIPRINGNEXT(out);
    }

    *start = *end;
    *end = out;
    return (out - last) & (rxCLIPRINGSIZE - 1);
}

/* Interpolates UVs and the screen vertex colour along the clipped edge. */
#define CLIPLERP(a, b) ((a) + t * ((b) - (a)))

#define CLIPCOLOR(c) \
    devOut->color.c = (RwReal)devIn1->color.c + t * (RwReal)(RwInt32)(devIn2->color.c - devIn1->color.c)

#define CLIPUVCOLOR()                                     \
    out->u = CLIPLERP(in1->u, in2->u);                    \
    out->v = CLIPLERP(in1->v, in2->v);                    \
    CLIPCOLOR(red);                                       \
    CLIPCOLOR(green);                                     \
    CLIPCOLOR(blue);                                      \
    CLIPCOLOR(alpha)

/* Clip flags of a new vertex against the perspective and parallel frusta. */
#define CLIPYFLAGSPERSP()                                                                                   \
    out->clipFlags |= (out->cameraVertex.y < 0.0f)                ? rwYLOCLIP                               \
                      : (out->cameraVertex.y > out->cameraVertex.z) ? rwYHICLIP                             \
                                                                    : 0
#define CLIPXFLAGSPERSP()                                                                                   \
    out->clipFlags |= (out->cameraVertex.x < 0.0f)                ? rwXLOCLIP                               \
                      : (out->cameraVertex.x > out->cameraVertex.z) ? rwXHICLIP                             \
                                                                    : 0
#define CLIPYFLAGSPARA()                                                                                    \
    out->clipFlags |= (out->cameraVertex.y < 0.0f) ? rwYLOCLIP : (out->cameraVertex.y > 1.0f) ? rwYHICLIP : 0
#define CLIPXFLAGSPARA()                                                                                    \
    out->clipFlags |= (out->cameraVertex.x < 0.0f) ? rwXLOCLIP : (out->cameraVertex.x > 1.0f) ? rwXHICLIP : 0

#define CLIPARGS                                                                                            \
    RxCamSpace3DVertex *out, RxCamSpace3DVertex *in1, RxCamSpace3DVertex *in2, RxScrSpace2DVertex *devOut, \
        RxScrSpace2DVertex *devIn1, RxScrSpace2DVertex *devIn2

RwReal _rxClipZLoPersp(CLIPARGS)
{
    RwReal t = (_rxTransformCamera.nearClip - in1->cameraVertex.z) / (in2->cameraVertex.z - in1->cameraVertex.z);

    out->cameraVertex.x = CLIPLERP(in1->cameraVertex.x, in2->cameraVertex.x);
    out->cameraVertex.y = CLIPLERP(in1->cameraVertex.y, in2->cameraVertex.y);
    out->cameraVertex.z = _rxTransformCamera.nearClip;
    CLIPUVCOLOR();
    out->clipFlags = rwZLOCLIP;
    CLIPYFLAGSPERSP();
    CLIPXFLAGSPERSP();
    return t;
}

RwReal _rxClipZHiPersp(CLIPARGS)
{
    RwReal t = (_rxTransformCamera.farClip - in1->cameraVertex.z) / (in2->cameraVertex.z - in1->cameraVertex.z);

    out->cameraVertex.x = CLIPLERP(in1->cameraVertex.x, in2->cameraVertex.x);
    out->cameraVertex.y = CLIPLERP(in1->cameraVertex.y, in2->cameraVertex.y);
    out->cameraVertex.z = _rxTransformCamera.farClip;
    CLIPUVCOLOR();
    out->clipFlags = rwZHICLIP;
    CLIPYFLAGSPERSP();
    CLIPXFLAGSPERSP();
    return t;
}

RwReal _rxClipYLoPersp(CLIPARGS)
{
    RwReal t = in1->cameraVertex.y / (in1->cameraVertex.y - in2->cameraVertex.y);

    out->cameraVertex.x = CLIPLERP(in1->cameraVertex.x, in2->cameraVertex.x);
    out->cameraVertex.z = CLIPLERP(in1->cameraVertex.z, in2->cameraVertex.z);
    out->cameraVertex.y = 0.0f;
    CLIPUVCOLOR();
    out->clipFlags = (in1->clipFlags & ~0xF) | rwYLOCLIP;
    CLIPXFLAGSPERSP();
    return t;
}

RwReal _rxClipYHiPersp(CLIPARGS)
{
    RwReal d1 = in1->cameraVertex.y - in1->cameraVertex.z;
    RwReal t = d1 / (d1 - (in2->cameraVertex.y - in2->cameraVertex.z));

    out->cameraVertex.x = CLIPLERP(in1->cameraVertex.x, in2->cameraVertex.x);
    out->cameraVertex.z = CLIPLERP(in1->cameraVertex.z, in2->cameraVertex.z);
    out->cameraVertex.y = out->cameraVertex.z;
    CLIPUVCOLOR();
    out->clipFlags = (in1->clipFlags & ~0xF) | rwYHICLIP;
    CLIPXFLAGSPERSP();
    return t;
}

RwReal _rxClipXLoPersp(CLIPARGS)
{
    RwReal t = in1->cameraVertex.x / (in1->cameraVertex.x - in2->cameraVertex.x);

    out->cameraVertex.y = CLIPLERP(in1->cameraVertex.y, in2->cameraVertex.y);
    out->cameraVertex.z = CLIPLERP(in1->cameraVertex.z, in2->cameraVertex.z);
    out->cameraVertex.x = 0.0f;
    CLIPUVCOLOR();
    out->clipFlags = (in1->clipFlags & ~0x3) | rwXLOCLIP;
    return t;
}

RwReal _rxClipXHiPersp(CLIPARGS)
{
    RwReal d1 = in1->cameraVertex.x - in1->cameraVertex.z;
    RwReal t = d1 / (d1 - (in2->cameraVertex.x - in2->cameraVertex.z));

    out->cameraVertex.y = CLIPLERP(in1->cameraVertex.y, in2->cameraVertex.y);
    out->cameraVertex.z = CLIPLERP(in1->cameraVertex.z, in2->cameraVertex.z);
    out->cameraVertex.x = out->cameraVertex.z;
    CLIPUVCOLOR();
    out->clipFlags = (in1->clipFlags & ~0x3) | rwXHICLIP;
    return t;
}

RwReal _rxClipZLoPara(CLIPARGS)
{
    RwReal t = (_rxTransformCamera.nearClip - in1->cameraVertex.z) / (in2->cameraVertex.z - in1->cameraVertex.z);

    out->cameraVertex.x = CLIPLERP(in1->cameraVertex.x, in2->cameraVertex.x);
    out->cameraVertex.y = CLIPLERP(in1->cameraVertex.y, in2->cameraVertex.y);
    out->cameraVertex.z = _rxTransformCamera.nearClip;
    CLIPUVCOLOR();
    out->clipFlags = rwZLOCLIP;
    CLIPYFLAGSPARA();
    CLIPXFLAGSPARA();
    return t;
}

RwReal _rxClipZHiPara(CLIPARGS)
{
    RwReal t = (_rxTransformCamera.farClip - in1->cameraVertex.z) / (in2->cameraVertex.z - in1->cameraVertex.z);

    out->cameraVertex.x = CLIPLERP(in1->cameraVertex.x, in2->cameraVertex.x);
    out->cameraVertex.y = CLIPLERP(in1->cameraVertex.y, in2->cameraVertex.y);
    out->cameraVertex.z = _rxTransformCamera.farClip;
    CLIPUVCOLOR();
    out->clipFlags = rwZHICLIP;
    CLIPYFLAGSPARA();
    CLIPXFLAGSPARA();
    return t;
}

RwReal _rxClipYLoPara(CLIPARGS)
{
    RwReal t = in1->cameraVertex.y / (in1->cameraVertex.y - in2->cameraVertex.y);

    out->cameraVertex.x = CLIPLERP(in1->cameraVertex.x, in2->cameraVertex.x);
    out->cameraVertex.z = CLIPLERP(in1->cameraVertex.z, in2->cameraVertex.z);
    out->cameraVertex.y = 0.0f;
    CLIPUVCOLOR();
    out->clipFlags = (in1->clipFlags & ~0xF) | rwYLOCLIP;
    CLIPXFLAGSPARA();
    return t;
}

RwReal _rxClipYHiPara(CLIPARGS)
{
    RwReal t = (1.0f - in1->cameraVertex.y) / (in2->cameraVertex.y - in1->cameraVertex.y);

    out->cameraVertex.x = CLIPLERP(in1->cameraVertex.x, in2->cameraVertex.x);
    out->cameraVertex.z = CLIPLERP(in1->cameraVertex.z, in2->cameraVertex.z);
    out->cameraVertex.y = 1.0f;
    CLIPUVCOLOR();
    out->clipFlags = (in1->clipFlags & ~0xF) | rwYHICLIP;
    CLIPXFLAGSPARA();
    return t;
}

RwReal _rxClipXLoPara(CLIPARGS)
{
    RwReal t = in1->cameraVertex.x / (in1->cameraVertex.x - in2->cameraVertex.x);

    out->cameraVertex.y = CLIPLERP(in1->cameraVertex.y, in2->cameraVertex.y);
    out->cameraVertex.z = CLIPLERP(in1->cameraVertex.z, in2->cameraVertex.z);
    out->cameraVertex.x = 0.0f;
    CLIPUVCOLOR();
    out->clipFlags = (in1->clipFlags & ~0x3) | rwXLOCLIP;
    return t;
}

RwReal _rxClipXHiPara(CLIPARGS)
{
    RwReal t = (1.0f - in1->cameraVertex.x) / (in2->cameraVertex.x - in1->cameraVertex.x);

    out->cameraVertex.y = CLIPLERP(in1->cameraVertex.y, in2->cameraVertex.y);
    out->cameraVertex.z = CLIPLERP(in1->cameraVertex.z, in2->cameraVertex.z);
    out->cameraVertex.x = 1.0f;
    CLIPUVCOLOR();
    out->clipFlags = (in1->clipFlags & ~0x3) | rwXHICLIP;
    return t;
}

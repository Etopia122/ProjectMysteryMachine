/*
 * Collision plugin: line and sphere intersection tests against an atomic's
 * geometry. Geometries with collision data walk their BSP tree, others test
 * every triangle; morphing atomics test triangles interpolated between the
 * current pair of morph targets.
 */

#include "rwsdk/rpworld_int.h"
#include "rwsdk/rpcollis_int.h"
#include "rwsdk/ctintsec_int.h"

#define rwPLUGIN_ID 0x11D /* rwID_COLLISPLUGIN */

#define E_RW_NULLP 0x80000016
#define E_RP_COLLIS_INV_INTERSECTION 0

#define rwMATRIXTYPENORMAL 0x00000001

typedef struct GeomCallBack {
    /* 0x00 */ RpIntersection* intersection;
    /* 0x04 */ RpIntersectionCallBackGeometryTriangle func;
    /* 0x08 */ void* data;
} GeomCallBack;

typedef struct LineCollParam {
    /* 0x00 */ RpGeometry* geometry;
    /* 0x04 */ GeomCallBack* callBack;
    /* 0x08 */ RwLine* line;
    /* 0x0C */ RwV3d delta;
} LineCollParam;

typedef struct SphereCollParam {
    /* 0x00 */ RpGeometry* geometry;
    /* 0x04 */ GeomCallBack* callBack;
    /* 0x08 */ RwSphere* sphere;
    /* 0x0C */ RwReal recipRadius;
} SphereCollParam;

extern RwInt32 _rpCollisionGeometryDataOffset;

#define RPCOLLISIONGEOMETRYGETDATA(geometry)                                                           \
    (*(RpCollisionData**)((RwUInt8*)(geometry) + _rpCollisionGeometryDataOffset))

RwMatrix* RwFrameGetLTM(RwFrame* frame);
RwMatrix* RwMatrixInvert(RwMatrix* matrixOut, const RwMatrix* matrixIn);
RwV3d* RwV3dTransformPoints(RwV3d* pointsOut, const RwV3d* pointsIn, RwInt32 numPoints, const RwMatrix* matrix);
RwReal _rwInvSqrt(RwReal num);
RwReal _rwSqrt(RwReal num);
RwSphere* RpAtomicGetWorldBoundingSphere(RpAtomic* atomic);
RwBool RtIntersectionSphereTriangle(RwSphere* sphere, RwV3d* v0, RwV3d* v1, RwV3d* v2, RwV3d* normal,
                                    RwReal* distance);
RpCollBSPTree* _rpCollBSPTreeForAllLineLeafNodeIntersections(RpCollBSPTree* tree, RwLine* line, RpV3dGradient* grad,
                                                             RpCollBSPLeafNodeCallBack callBack, void* data);
RpCollBSPTree* _rpCollBSPTreeForAllBoxLeafNodeIntersections(RpCollBSPTree* tree, RwBBox* box,
                                                            RpCollBSPLeafNodeCallBack callBack, void* data);

static RwInt32 GeomLeafNodeForAllLineIntersections(RwInt32 numTriangles, RwInt32 triOffset, void* data)
{
    LineCollParam* isData;
    RpGeometry* geometry;
    RwV3d* vertices;
    RpTriangle* triangles;
    GeomCallBack* cb;
    RwUInt16* triIndex;
    RpTriangle* tri;
    RwV3d* v0;
    RwV3d* v1;
    RwV3d* v2;
    RwReal distance;
    RwInt32 result;

    isData = (LineCollParam*)data;
    geometry = isData->geometry;
    cb = isData->callBack;
    vertices = geometry->morphTarget->verts;
    triangles = geometry->triangles;
    triIndex = &RPCOLLISIONGEOMETRYGETDATA(geometry)->triangleMap[triOffset];

    while (numTriangles--) {
        tri = &triangles[*triIndex];
        v0 = &vertices[tri->vertIndex[0]];
        v2 = &vertices[tri->vertIndex[2]];
        v1 = &vertices[tri->vertIndex[1]];
        LINETRIANGLE(result, &isData->line->start, &isData->delta, v0, v1, v2, &distance);
        if (result) {
            RpCollisionTriangle collisionTri;

            collisionTri.index = *triIndex;
            collisionTri.point = *v0;
            TRIANGLENORMAL(&collisionTri.normal, v0, v1, v2);
            collisionTri.vertices[0] = v0;
            collisionTri.vertices[1] = v1;
            collisionTri.vertices[2] = v2;
            if (!cb->func(cb->intersection, &collisionTri, distance, cb->data)) {
                return FALSE;
            }
        }
        triIndex++;
    }
    return TRUE;
}

static RwInt32 GeomLeafNodeForAllSphereIntersections(RwInt32 numTriangles, RwInt32 triOffset, void* data)
{
    SphereCollParam* isData;
    RpGeometry* geometry;
    RwV3d* vertices;
    RpTriangle* triangles;
    GeomCallBack* cb;
    RwUInt16* triIndex;
    RpTriangle* tri;
    RwV3d* v0;
    RwV3d* v1;
    RwV3d* v2;
    RwReal distance;
    RpCollisionTriangle collisionTri;

    isData = (SphereCollParam*)data;
    geometry = isData->geometry;
    cb = isData->callBack;
    vertices = geometry->morphTarget->verts;
    triangles = geometry->triangles;
    triIndex = &RPCOLLISIONGEOMETRYGETDATA(geometry)->triangleMap[triOffset];

    while (numTriangles--) {
        tri = &triangles[*triIndex];
        v0 = &vertices[tri->vertIndex[0]];
        v1 = &vertices[tri->vertIndex[1]];
        v2 = &vertices[tri->vertIndex[2]];
        if (RtIntersectionSphereTriangle(isData->sphere, v0, v1, v2, &collisionTri.normal, &distance)) {
            collisionTri.point = *v0;
            collisionTri.index = *triIndex;
            collisionTri.vertices[0] = v0;
            collisionTri.vertices[1] = v1;
            collisionTri.vertices[2] = v2;
            distance *= isData->recipRadius;
            if (!cb->func(cb->intersection, &collisionTri, distance, cb->data)) {
                return FALSE;
            }
        }
        triIndex++;
    }
    return TRUE;
}

static RpGeometry* GeometryForAllLineIntersections(RpGeometry* geometry, RwLine* testLine, GeomCallBack* cb)
{
    RpCollisionData* collData;
    RpV3dGradient grad;
    LineCollParam isData;
    RwV3d* vertices;
    RpTriangle* triangle;
    RwInt32 iTri;
    RwV3d* v0;
    RwV3d* v1;
    RwV3d* v2;
    RwReal distance;
    RwInt32 result;
    RpCollisionTriangle collisionTri;
    RwV3d delta;

    collData = RPCOLLISIONGEOMETRYGETDATA(geometry);
    if (collData) {
        isData.callBack = cb;
        isData.geometry = geometry;
        isData.line = testLine;
        isData.delta.x = testLine->end.x - testLine->start.x;
        isData.delta.y = testLine->end.y - testLine->start.y;
        isData.delta.z = testLine->end.z - testLine->start.z;

        {
            RwReal recip = 0.0f;

            if (isData.delta.x != 0.0f) {
                recip = 1.0f / isData.delta.x;
            }
            grad.dydx = recip * isData.delta.y;
            grad.dzdx = isData.delta.z * recip;
        }
        {
            RwReal recip = 0.0f;

            if (isData.delta.y != 0.0f) {
                recip = 1.0f / isData.delta.y;
            }
            grad.dxdy = isData.delta.x * recip;
            grad.dzdy = isData.delta.z * recip;
        }
        {
            RwReal recip = 0.0f;

            if (isData.delta.z != 0.0f) {
                recip = 1.0f / isData.delta.z;
            }
            grad.dxdz = isData.delta.x * recip;
            grad.dydz = isData.delta.y * recip;
        }

        _rpCollBSPTreeForAllLineLeafNodeIntersections(collData->tree, testLine, &grad,
                                                      GeomLeafNodeForAllLineIntersections, &isData);
    } else {
        delta.x = testLine->end.x - testLine->start.x;
        delta.y = testLine->end.y - testLine->start.y;
        delta.z = testLine->end.z - testLine->start.z;
        vertices = geometry->morphTarget->verts;
        triangle = geometry->triangles;

        for (iTri = 0; iTri < geometry->numTriangles; iTri++, triangle++) {
            v0 = &vertices[triangle->vertIndex[0]];
            v2 = &vertices[triangle->vertIndex[2]];
            v1 = &vertices[triangle->vertIndex[1]];
            LINETRIANGLE(result, &testLine->start, &delta, v0, v1, v2, &distance);
            if (result) {
                collisionTri.point = *v0;
                collisionTri.index = iTri;
                TRIANGLENORMAL(&collisionTri.normal, v0, v1, v2);
                collisionTri.vertices[0] = v0;
                collisionTri.vertices[1] = v1;
                collisionTri.vertices[2] = v2;
                if (!cb->func(cb->intersection, &collisionTri, distance, cb->data)) {
                    return NULL;
                }
            }
        }
    }
    return geometry;
}

static RpGeometry* GeometryForAllSphereIntersections(RpGeometry* geometry, RwSphere* testSphere, GeomCallBack* cb)
{
    RpCollisionData* collData;
    RwBBox bbox;
    SphereCollParam isData;
    RwV3d* vertices;
    RpTriangle* triangle;
    RwReal recipRadius;
    RwInt32 iTri;
    RwV3d* v0;
    RwV3d* v1;
    RwV3d* v2;
    RwReal distance;
    RpCollisionTriangle collisionTri;

    collData = RPCOLLISIONGEOMETRYGETDATA(geometry);
    if (collData) {
        isData.callBack = cb;
        isData.geometry = geometry;
        isData.sphere = testSphere;
        isData.recipRadius = 1.0f / testSphere->radius;

        bbox.sup = testSphere->center;
        bbox.inf = testSphere->center;
        bbox.inf.x -= testSphere->radius;
        bbox.inf.y -= testSphere->radius;
        bbox.inf.z -= testSphere->radius;
        bbox.sup.x += testSphere->radius;
        bbox.sup.y += testSphere->radius;
        bbox.sup.z += testSphere->radius;

        _rpCollBSPTreeForAllBoxLeafNodeIntersections(collData->tree, &bbox, GeomLeafNodeForAllSphereIntersections,
                                                     &isData);
    } else {
        recipRadius = 1.0f / testSphere->radius;
        vertices = geometry->morphTarget->verts;
        triangle = geometry->triangles;

        for (iTri = 0; iTri < geometry->numTriangles; iTri++, triangle++) {
            v0 = &vertices[triangle->vertIndex[0]];
            v1 = &vertices[triangle->vertIndex[1]];
            v2 = &vertices[triangle->vertIndex[2]];
            if (RtIntersectionSphereTriangle(testSphere, v0, v1, v2, &collisionTri.normal, &distance)) {
                collisionTri.point = *v0;
                collisionTri.index = iTri;
                collisionTri.vertices[0] = v0;
                collisionTri.vertices[1] = v1;
                collisionTri.vertices[2] = v2;
                distance *= recipRadius;
                if (!cb->func(cb->intersection, &collisionTri, distance, cb->data)) {
                    return NULL;
                }
            }
        }
    }
    return geometry;
}

/* Interpolates a morphing triangle's vertex between two morph targets. */
#define MORPHVERTEX(_out, _index)                                                                          do {                                                                                                       startVert = &startFramePoints[_index];                                                                 endVert = &endFramePoints[_index];                                                                     (_out).x = endVert->x - startVert->x;                                                                  (_out).y = endVert->y - startVert->y;                                                                  (_out).z = endVert->z - startVert->z;                                                                  (_out).x = (_out).x * blendFactor;                                                                     (_out).y = (_out).y * blendFactor;                                                                     (_out).z = (_out).z * blendFactor;                                                                     (_out).x = (_out).x + startVert->x;                                                                    (_out).y = (_out).y + startVert->y;                                                                    (_out).z = (_out).z + startVert->z;                                                                } while (0)

static RpAtomic* AtomicForAllLineIntersections(RpAtomic* atomic, RwLine* testLine, GeomCallBack* cb)
{
    RpGeometry* geom = atomic->geometry;
    RwLine localLine;
    RwMatrix inverseLTM;

    inverseLTM.flags = rwMATRIXTYPENORMAL;
    RwMatrixInvert(&inverseLTM, RwFrameGetLTM((RwFrame*)atomic->object.object.parent));
    RwV3dTransformPoints(&localLine.start, &testLine->start, 2, &inverseLTM);

    if (geom->numMorphTargets == 1) {
        if (!GeometryForAllLineIntersections(geom, &localLine, cb)) {
            return NULL;
        }
    } else {
        RwV3d delta;
        RwV3d v0;
        RwV3d v1;
        RwV3d v2;
        RwV3d* vp0 = NULL;
        RwV3d* vp1 = NULL;
        RwV3d* vp2 = NULL;
        RpInterpolator* interpolator = &atomic->interpolator;
        RwV3d* startFramePoints;
        RwV3d* endFramePoints;
        RwReal blendFactor = 0.0f;
        RwInt32 iTri;
        RpTriangle* triangle;

        delta.x = localLine.end.x - localLine.start.x;
        delta.y = localLine.end.y - localLine.start.y;
        delta.z = localLine.end.z - localLine.start.z;

        if (interpolator->startMorphTarget == interpolator->endMorphTarget ||
            interpolator->startMorphTarget >= geom->numMorphTargets ||
            interpolator->endMorphTarget >= geom->numMorphTargets) {
            if (interpolator->startMorphTarget < geom->numMorphTargets &&
                interpolator->endMorphTarget < geom->numMorphTargets) {
                startFramePoints = geom->morphTarget[interpolator->startMorphTarget].verts;
            } else {
                startFramePoints = geom->morphTarget->verts;
            }
            endFramePoints = startFramePoints;
        } else {
            blendFactor = interpolator->recipTime * interpolator->position;
            vp0 = &v0;
            vp1 = &v1;
            vp2 = &v2;
            startFramePoints = geom->morphTarget[interpolator->startMorphTarget].verts;
            endFramePoints = geom->morphTarget[interpolator->endMorphTarget].verts;
        }

        triangle = geom->triangles;
        for (iTri = 0; iTri < geom->numTriangles; triangle++, iTri++) {
            RwInt32 result;
            RwReal distance;

            if (startFramePoints == endFramePoints) {
                vp0 = &startFramePoints[triangle->vertIndex[0]];
                vp1 = &startFramePoints[triangle->vertIndex[1]];
                vp2 = &startFramePoints[triangle->vertIndex[2]];
            } else {
                RwV3d* startVert;
                RwV3d* endVert;

                MORPHVERTEX(v0, triangle->vertIndex[0]);
                MORPHVERTEX(v1, triangle->vertIndex[1]);
                MORPHVERTEX(v2, triangle->vertIndex[2]);
            }

            LINETRIANGLE(result, &localLine.start, &delta, vp0, vp1, vp2, &distance);
            if (result) {
                RpCollisionTriangle collisionTri;

                TRIANGLENORMAL(&collisionTri.normal, vp0, vp1, vp2);
                collisionTri.point = *vp0;
                collisionTri.index = iTri;
                collisionTri.vertices[0] = vp0;
                collisionTri.vertices[1] = vp1;
                collisionTri.vertices[2] = vp2;
                if (!cb->func(cb->intersection, &collisionTri, distance, cb->data)) {
                    return atomic;
                }
            }
        }
    }
    return atomic;
}

static RpAtomic* AtomicForAllSphereIntersections(RpAtomic* atomic, RwSphere* testSphere, GeomCallBack* cb)
{
    RpGeometry* geom = atomic->geometry;
    RwSphere localSphere;
    RwMatrix inverseLTM;
    RwReal scaleSq;
    RwReal _result;

    inverseLTM.flags = rwMATRIXTYPENORMAL;
    RwMatrixInvert(&inverseLTM, RwFrameGetLTM((RwFrame*)atomic->object.object.parent));
    RwV3dTransformPoints(&localSphere.center, &testSphere->center, 1, &inverseLTM);
    scaleSq = inverseLTM.at.x * inverseLTM.at.x + inverseLTM.at.y * inverseLTM.at.y +
              inverseLTM.at.z * inverseLTM.at.z;
    _result = _rwSqrt(scaleSq);
    localSphere.radius = _result * testSphere->radius;

    if (geom->numMorphTargets == 1) {
        if (!GeometryForAllSphereIntersections(geom, &localSphere, cb)) {
            return NULL;
        }
    } else {
        RwV3d v0;
        RwV3d v1;
        RwV3d v2;
        RwV3d* vp0 = NULL;
        RwV3d* vp1 = NULL;
        RwV3d* vp2 = NULL;
        RpInterpolator* interpolator = &atomic->interpolator;
        RwV3d* startFramePoints;
        RwV3d* endFramePoints;
        RwReal blendFactor = 0.0f;
        RwReal recipSphereRad = 1.0f / localSphere.radius;
        RwInt32 iTri;
        RpTriangle* triangle;

        if (interpolator->startMorphTarget == interpolator->endMorphTarget ||
            interpolator->startMorphTarget >= geom->numMorphTargets ||
            interpolator->endMorphTarget >= geom->numMorphTargets) {
            if (interpolator->startMorphTarget < geom->numMorphTargets &&
                interpolator->endMorphTarget < geom->numMorphTargets) {
                startFramePoints = geom->morphTarget[interpolator->startMorphTarget].verts;
            } else {
                startFramePoints = geom->morphTarget->verts;
            }
            endFramePoints = startFramePoints;
        } else {
            blendFactor = interpolator->recipTime * interpolator->position;
            vp0 = &v0;
            vp1 = &v1;
            vp2 = &v2;
            startFramePoints = geom->morphTarget[interpolator->startMorphTarget].verts;
            endFramePoints = geom->morphTarget[interpolator->endMorphTarget].verts;
        }

        triangle = geom->triangles;
        for (iTri = 0; iTri < geom->numTriangles; triangle++, iTri++) {
            RpCollisionTriangle collisionTri;
            RwReal distance;

            if (startFramePoints == endFramePoints) {
                vp0 = &startFramePoints[triangle->vertIndex[0]];
                vp1 = &startFramePoints[triangle->vertIndex[1]];
                vp2 = &startFramePoints[triangle->vertIndex[2]];
            } else {
                RwV3d* startVert;
                RwV3d* endVert;

                MORPHVERTEX(v0, triangle->vertIndex[0]);
                MORPHVERTEX(v1, triangle->vertIndex[1]);
                MORPHVERTEX(v2, triangle->vertIndex[2]);
            }

            if (RtIntersectionSphereTriangle(&localSphere, vp0, vp1, vp2, &collisionTri.normal, &distance)) {
                collisionTri.point = *vp0;
                collisionTri.index = iTri;
                collisionTri.vertices[0] = vp0;
                collisionTri.vertices[1] = vp1;
                collisionTri.vertices[2] = vp2;
                distance *= recipSphereRad;
                if (!cb->func(cb->intersection, &collisionTri, distance, cb->data)) {
                    return atomic;
                }
            }
        }
    }
    return atomic;
}

RpAtomic* RpAtomicForAllIntersections(RpAtomic* atomic, RpIntersection* intersection,
                                      RpIntersectionCallBackGeometryTriangle callBack, void* data)
{
    GeomCallBack cb;

    cb.func = callBack;
    cb.data = data;
    cb.intersection = intersection;

    switch (intersection->type) {
    case rpINTERSECTLINE:
        AtomicForAllLineIntersections(atomic, &intersection->t.line, &cb);
        return atomic;
    case rpINTERSECTSPHERE:
        AtomicForAllSphereIntersections(atomic, &intersection->t.sphere, &cb);
        return atomic;
    case rpINTERSECTATOMIC: {
        RpAtomic* intersectionAtomic = (RpAtomic*)intersection->t.object;

        if (intersectionAtomic) {
            RwSphere* boundingSphere = RpAtomicGetWorldBoundingSphere(intersectionAtomic);

            AtomicForAllSphereIntersections(atomic, boundingSphere, &cb);
            return atomic;
        }
        RWERROR((E_RW_NULLP));
        return NULL;
    }
    default:
        RWERROR((E_RP_COLLIS_INV_INTERSECTION));
        return NULL;
    }
}

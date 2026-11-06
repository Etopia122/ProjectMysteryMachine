/*
 * Collision plugin: intersection queries against a world. The world's
 * sector BSP is walked to find the sectors a line or box touches, and each
 * sector's triangles are then tested through its collision BSP tree, or for
 * sectors without collision data through the older per-sector split tree.
 */

#include "rwsdk/rpworld_int.h"
#include "rwsdk/rpcollis_int.h"
#include "rwsdk/ctintsec_int.h"

#define rwPLUGIN_ID 0x11D /* rwID_COLLISPLUGIN */

#define E_RP_COLLIS_INV_INTERSECTION 0

#define rpCOLLSECTORSPLIT 0x80

typedef RpCollisionTriangle* (*RpIntersectionCallBackWorldTriangle)(RpIntersection* intersection,
                                                                    RpWorldSector* sector,
                                                                    RpCollisionTriangle* collTriangle,
                                                                    RwReal distance, void* data);

typedef union IntersectionCallBack {
    RpIntersectionCallBackWorldTriangle worldCB;
} IntersectionCallBack;

typedef struct CallBackParam {
    /* 0x00 */ RpIntersection* intersection;
    /* 0x04 */ IntersectionCallBack u;
    /* 0x08 */ void* data;
} CallBackParam;

typedef struct TestSphere {
    /* 0x00 */ RwSphere* sphere;
    /* 0x04 */ RwReal recipRadius;
} TestSphere;

typedef struct PolyTestParam {
    /* 0x00 */ RwBBox bbox;
    /* 0x18 */ RpWorldSector* worldSector;
    /* 0x1C */ RpCollBSPLeafNodeCallBack leafTest;
    /* 0x20 */ RpCollBSPLeafNodeCallBack v303leafTest;
    /* 0x24 */ void* leafTestData;
    /* 0x28 */ CallBackParam* cbParam;
} PolyTestParam;

typedef struct PolyLineTestParam {
    /* 0x00 */ RpV3dGradient grad;
    /* 0x18 */ RwLine clippedLine;
    /* 0x30 */ RpWorldSector* worldSector;
    /* 0x34 */ RwV3d* start;
    /* 0x38 */ RwV3d delta;
    /* 0x44 */ CallBackParam* cbParam;
} PolyLineTestParam;

/* Older collision data: an implicit binary tree of splits and polygon runs. */
typedef struct RpCollSector {
    /* 0x00 */ RwUInt8 cType;
    /* 0x01 */ RwUInt8 vertex;
    /* 0x02 */ RwUInt8 start;
} RpCollSector;

/* The integer views are volatile so the float is always reread from memory. */
typedef union RwSplitBits {
    RwReal nReal;
    volatile RwInt32 nInt;
    volatile RwUInt32 nUInt;
} RwSplitBits;

extern RwInt32 _rpCollisionWorldSectorDataOffset;

#define RPCOLLISIONWORLDSECTORGETDATA(sector)                                                          \
    (*(RpCollisionData**)((RwUInt8*)(sector) + _rpCollisionWorldSectorDataOffset))

#define COLLSECTORROOT(sector) ((RpCollSector*)(sector)->unk80)


RwReal _rwInvSqrt(RwReal num);
RwSphere* RpAtomicGetWorldBoundingSphere(RpAtomic* atomic);
RpAtomic* RpAtomicForAllWorldSectors(RpAtomic* atomic, RpWorldSectorCallBack callback, void* data);
RwBool RtIntersectionSphereTriangle(RwSphere* sphere, RwV3d* v0, RwV3d* v1, RwV3d* v2, RwV3d* normal,
                                    RwReal* distance);
RwBool RtIntersectionBBoxTriangle(RwBBox* bbox, RwV3d* v0, RwV3d* v1, RwV3d* v2);
RpCollBSPTree* _rpCollBSPTreeForAllLineLeafNodeIntersections(RpCollBSPTree* tree, RwLine* line, RpV3dGradient* grad,
                                                             RpCollBSPLeafNodeCallBack callBack, void* data);
RpCollBSPTree* _rpCollBSPTreeForAllBoxLeafNodeIntersections(RpCollBSPTree* tree, RwBBox* box,
                                                            RpCollBSPLeafNodeCallBack callBack, void* data);

/* Point where a line crosses the plane coord[axis] == value. */
#define GETINTERSECTION(vec, ln, value, axis, grad)                                                    \
    switch (axis) {                                                                                    \
    case 0: {                                                                                          \
        RwReal delta = (value) - (ln).start.x;                                                         \
        (vec).x = (value);                                                                             \
        (vec).y = (ln).start.y + (grad)->dydx * delta;                                                 \
        (vec).z = (ln).start.z + (grad)->dzdx * delta;                                                 \
        break;                                                                                         \
    }                                                                                                  \
    case 4: {                                                                                          \
        RwReal delta = (value) - (ln).start.y;                                                         \
        (vec).x = (ln).start.x + (grad)->dxdy * delta;                                                 \
        (vec).y = (value);                                                                             \
        (vec).z = (ln).start.z + (grad)->dzdy * delta;                                                 \
        break;                                                                                         \
    }                                                                                                  \
    case 8: {                                                                                          \
        RwReal delta = (value) - (ln).start.z;                                                         \
        (vec).x = (ln).start.x + (grad)->dxdz * delta;                                                 \
        (vec).y = (ln).start.y + (grad)->dydz * delta;                                                 \
        (vec).z = (value);                                                                             \
        break;                                                                                         \
    }                                                                                                  \
    }

#define PUSHSECTOR(lnStart, lnEnd, sect)                                                                   nStack++;                                                                                              sectorStack[nStack] = (sect);                                                                          lineStack[nStack].start = (lnStart);                                                                   lineStack[nStack].end = (lnEnd)

static RpWorld* WorldForAllLineWorldSectorIntersections(RpWorld* world, RwLine* testLine, RpV3dGradient* grad,
                                                        RpWorldSectorCallBack callBack, void* data,
                                                        RwLine* clippedLineOut)
{
    RwInt32 nStack;
    RpSector* sector;
    RpSector* sectorStack[64];
    RwLine line;
    RwLine lineStack[64];
    RpPlaneSector* plane;
    RwSplitBits lStart;
    RwSplitBits lEnd;
    RwSplitBits rStart;
    RwSplitBits rEnd;

    line = *testLine;
    nStack = 0;
    sector = world->rootSector;
    while (nStack >= 0) {
        if (sector->type < 0) {
            if (clippedLineOut) {
                *clippedLineOut = line;
            }
            if (!callBack((RpWorldSector*)sector, data)) {
                return world;
            }
            sector = sectorStack[nStack];
            line = lineStack[nStack];
            nStack--;
        } else {
            plane = (RpPlaneSector*)sector;

            rStart.nReal = GETCOORD(line.start, sector->type) - plane->rightValue;
            lStart.nReal = GETCOORD(line.start, sector->type) - plane->leftValue;
            lEnd.nReal = GETCOORD(line.end, sector->type) - plane->leftValue;
            rEnd.nReal = GETCOORD(line.end, sector->type) - plane->rightValue;

            if (rStart.nInt < 0 && rEnd.nInt < 0) {
                sector = plane->leftSubTree;
            } else if (lStart.nInt >= 0 && lEnd.nInt >= 0) {
                sector = plane->rightSubTree;
            } else if (!((lStart.nInt ^ lEnd.nInt) & 0x80000000) && !((rStart.nInt ^ rEnd.nInt) & 0x80000000)) {
                if (rStart.nInt < rEnd.nInt) {
                    PUSHSECTOR(line.start, line.end, plane->rightSubTree);
                    sector = plane->leftSubTree;
                } else {
                    PUSHSECTOR(line.start, line.end, plane->leftSubTree);
                    sector = plane->rightSubTree;
                }
            } else if (((lStart.nInt ^ lEnd.nInt) & 0x80000000) && rStart.nInt >= 0 && rEnd.nInt >= 0) {
                RwV3d vLeft;

                GETINTERSECTION(vLeft, line, plane->leftValue, sector->type, grad);
                if (lStart.nInt < 0) {
                    PUSHSECTOR(line.start, line.end, plane->rightSubTree);
                    sector = plane->leftSubTree;
                    line.end = vLeft;
                } else {
                    PUSHSECTOR(vLeft, line.end, plane->leftSubTree);
                    sector = plane->rightSubTree;
                }
            } else if (((rStart.nInt ^ rEnd.nInt) & 0x80000000) && lStart.nInt < 0 && lEnd.nInt < 0) {
                RwV3d vRight;

                GETINTERSECTION(vRight, line, plane->rightValue, sector->type, grad);
                if (rStart.nInt < 0) {
                    PUSHSECTOR(vRight, line.end, plane->rightSubTree);
                    sector = plane->leftSubTree;
                } else {
                    PUSHSECTOR(line.start, line.end, plane->leftSubTree);
                    sector = plane->rightSubTree;
                    line.end = vRight;
                }
            } else {
                RwV3d vLeft;
                RwV3d vRight;

                GETINTERSECTION(vRight, line, plane->rightValue, sector->type, grad);
                GETINTERSECTION(vLeft, line, plane->leftValue, sector->type, grad);
                if (lStart.nInt < 0) {
                    PUSHSECTOR(vRight, line.end, plane->rightSubTree);
                    sector = plane->leftSubTree;
                    line.end = vLeft;
                } else {
                    PUSHSECTOR(vLeft, line.end, plane->leftSubTree);
                    sector = plane->rightSubTree;
                    line.end = vRight;
                }
            }
        }
    }
    return world;
}

static inline RpWorld* WorldForAllBoxWorldSectorIntersections(RpWorld* world, RwBBox* box, RpWorldSectorCallBack callBack,
                                                       void* data)
{
    RpSector* sectorStack[64];
    RpPlaneSector* plane;
    RpSector* sector;
    RwInt32 nStack;

    nStack = 0;
    sector = world->rootSector;
    while (nStack >= 0) {
        if (sector->type < 0) {
            if (!callBack((RpWorldSector*)sector, data)) {
                return world;
            }
            sector = sectorStack[nStack--];
        } else {
            plane = (RpPlaneSector*)sector;
            if (GETCOORD(box->inf, sector->type) < plane->leftValue) {
                sector = plane->leftSubTree;
                if (GETCOORD(box->sup, plane->type) >= plane->rightValue) {
                    sectorStack[++nStack] = plane->rightSubTree;
                }
            } else {
                sector = plane->rightSubTree;
            }
        }
    }
    return world;
}

static RpWorldSector* OldWorldSectorForAllLinePolyIntersections(RpWorldSector* worldSector, PolyLineTestParam* isData)
{
    RwLine lineStack[16];
    RpCollSector* collSector;
    CallBackParam* cbParam;
    RwV3d* vertices;
    RwInt32 nStack;
    RwInt32 cut;
    RwSplitBits start;
    RwReal distance;
    RwInt32 cutStack[16];
    RwInt32 numPolygons;
    RwInt32 plane;
    RwSplitBits end;
    RpCollisionTriangle collTri;
    RwReal value;
    RwInt32 polyOffset;
    RpPolygon* poly;
    RwInt32 result;
    RwLine line;
    RwV3d* v0;
    RwV3d* v1;
    RwV3d* v2;

    collSector = COLLSECTORROOT(worldSector);
    if (collSector) {
        line = isData->clippedLine;
        cbParam = isData->cbParam;
        vertices = worldSector->vertices;
        cut = 1;
        nStack = 0;
        while (nStack >= 0) {
            if (collSector[cut].cType & rpCOLLSECTORSPLIT) {
                plane = (collSector[cut].cType >> 3) & 0xC;
                value = GETCOORD(vertices[collSector[cut].vertex], plane);
                numPolygons = collSector[cut].cType & 0x1F;
                polyOffset = collSector[cut].start;

                start.nReal = GETCOORD(line.start, plane) - value;
                end.nReal = GETCOORD(line.end, plane) - value;
                if ((start.nInt ^ end.nInt) & 0x80000000) {
                    RwV3d vPlane;

                    GETINTERSECTION(vPlane, line, value, plane, &isData->grad);
                    nStack++;
                    cutStack[nStack] = (start.nInt < 0) ? cut + cut + 1 : cut + cut;
                    lineStack[nStack].start = vPlane;
                    lineStack[nStack].end = line.end;
                    line.end = vPlane;
                }
                cut = (start.nInt < 0) ? cut + cut : cut + cut + 1;
            } else {
                numPolygons = collSector[cut].cType;
                polyOffset = collSector[cut].start;
                line = lineStack[nStack];
                cut = cutStack[nStack];
                nStack--;
            }

            poly = &worldSector->polygons[polyOffset];
            while (numPolygons--) {
                v0 = &vertices[poly->vertIndex[0]];
                v2 = &vertices[poly->vertIndex[2]];
                v1 = &vertices[poly->vertIndex[1]];
                LINETRIANGLE(result, isData->start, &isData->delta, v0, v1, v2, &distance);
                if (result) {
                    collTri.point = *v0;
                    collTri.index = polyOffset;
                    TRIANGLENORMAL(&collTri.normal, v0, v1, v2);
                    collTri.vertices[0] = v0;
                    collTri.vertices[1] = v1;
                    collTri.vertices[2] = v2;
                    if (!cbParam->u.worldCB(cbParam->intersection, worldSector, &collTri, distance, cbParam->data)) {
                        return NULL;
                    }
                }
                polyOffset++;
                poly++;
            }
        }
    }
    return worldSector;
}

static RwInt32 OldLeafNodeForAllBoxPolyIntersections(RwInt32 numPolygons, RwInt32 polyOffset, void* data)
{
    PolyTestParam* isData;
    CallBackParam* cbParam;
    RpWorldSector* worldSector;
    RwV3d* vertices;
    RpPolygon* poly;
    RwV3d* v0;
    RwV3d* v1;
    RwV3d* v2;
    RpCollisionTriangle collTri;

    isData = (PolyTestParam*)data;
    worldSector = isData->worldSector;
    cbParam = isData->cbParam;
    vertices = worldSector->vertices;
    poly = &worldSector->polygons[polyOffset];
    while (numPolygons--) {
        v0 = &vertices[poly->vertIndex[0]];
        v1 = &vertices[poly->vertIndex[1]];
        v2 = &vertices[poly->vertIndex[2]];
        if (RtIntersectionBBoxTriangle(&isData->bbox, v0, v1, v2)) {
            collTri.point = *v0;
            collTri.index = polyOffset;
            TRIANGLENORMAL(&collTri.normal, v0, v1, v2);
            collTri.vertices[0] = v0;
            collTri.vertices[1] = v1;
            collTri.vertices[2] = v2;
            if (!cbParam->u.worldCB(cbParam->intersection, worldSector, &collTri, 0.0f, cbParam->data)) {
                return FALSE;
            }
        }
        polyOffset++;
        poly++;
    }
    return TRUE;
}

static RwInt32 OldLeafNodeForAllSpherePolyIntersections(RwInt32 numPolygons, RwInt32 polyOffset, void* data)
{
    PolyTestParam* isData;
    TestSphere* testSphere;
    CallBackParam* cbParam;
    RpWorldSector* worldSector;
    RwV3d* vertices;
    RpPolygon* poly;
    RwV3d* v0;
    RwV3d* v1;
    RwV3d* v2;
    RwReal distance;
    RpCollisionTriangle collTri;

    isData = (PolyTestParam*)data;
    worldSector = isData->worldSector;
    testSphere = (TestSphere*)isData->leafTestData;
    cbParam = isData->cbParam;
    vertices = worldSector->vertices;
    poly = &worldSector->polygons[polyOffset];
    while (numPolygons--) {
        v0 = &vertices[poly->vertIndex[0]];
        v1 = &vertices[poly->vertIndex[1]];
        v2 = &vertices[poly->vertIndex[2]];
        if (RtIntersectionSphereTriangle(testSphere->sphere, v0, v1, v2, &collTri.normal, &distance)) {
            collTri.point = *v0;
            collTri.index = polyOffset;
            collTri.vertices[0] = v0;
            collTri.vertices[1] = v1;
            collTri.vertices[2] = v2;
            distance *= testSphere->recipRadius;
            if (!cbParam->u.worldCB(cbParam->intersection, worldSector, &collTri, distance, cbParam->data)) {
                return FALSE;
            }
        }
        polyOffset++;
        poly++;
    }
    return TRUE;
}

static inline RpWorldSector* OldWorldSectorForAllBoxedPrimitivePolyIntersections(RpWorldSector* worldSector,
                                                                          PolyTestParam* isData)
{
    RpCollSector* collSector;
    RwV3d* vertices;
    RwInt32 nStack;
    RwInt32 cut;
    RwInt32 cutStack[16];
    RwInt32 numPolygons;
    RwInt32 polyOffset;
    RwInt32 plane;
    RwReal value;
    RwSplitBits right;
    RwSplitBits left;

    collSector = COLLSECTORROOT(worldSector);
    if (collSector) {
        vertices = worldSector->vertices;
        cut = 1;
        nStack = 0;
        while (nStack >= 0) {
            if (collSector[cut].cType & rpCOLLSECTORSPLIT) {
                plane = (collSector[cut].cType >> 3) & 0xC;
                value = GETCOORD(vertices[collSector[cut].vertex], plane);
                numPolygons = collSector[cut].cType & 0x1F;
                polyOffset = collSector[cut].start;

                right.nReal = GETCOORD(isData->bbox.inf, plane) - value;
                left.nReal = value - GETCOORD(isData->bbox.sup, plane);
                if (right.nInt < 0) {
                    if (left.nInt < 0) {
                        cutStack[++nStack] = cut + cut + 1;
                    }
                    cut = cut + cut;
                } else if (left.nInt < 0) {
                    cut = cut + cut + 1;
                }
            } else {
                numPolygons = collSector[cut].cType;
                polyOffset = collSector[cut].start;
                cut = cutStack[nStack--];
            }

            if (!isData->v303leafTest(numPolygons, polyOffset, isData)) {
                return NULL;
            }
        }
    }
    return worldSector;
}

static RwInt32 LeafNodeForAllLinePolyIntersections(RwInt32 numPolygons, RwInt32 polyOffset, void* data)
{
    PolyLineTestParam* isData;
    CallBackParam* cbParam;
    RpWorldSector* worldSector;
    RwV3d* vertices;
    RpPolygon* polygons;
    RwUInt16* polyIndex;
    RpPolygon* poly;
    RwV3d* v0;
    RwV3d* v1;
    RwV3d* v2;
    RwReal distance;
    RwInt32 result;
    RpCollisionTriangle collisionTri;

    isData = (PolyLineTestParam*)data;
    worldSector = isData->worldSector;
    cbParam = isData->cbParam;
    vertices = worldSector->vertices;
    polygons = worldSector->polygons;
    polyIndex = &RPCOLLISIONWORLDSECTORGETDATA(worldSector)->triangleMap[polyOffset];

    while (numPolygons--) {
        poly = &polygons[*polyIndex];
        v0 = &vertices[poly->vertIndex[0]];
        v2 = &vertices[poly->vertIndex[2]];
        v1 = &vertices[poly->vertIndex[1]];
        LINETRIANGLE(result, isData->start, &isData->delta, v0, v1, v2, &distance);
        if (result) {
            collisionTri.index = *polyIndex;
            collisionTri.point = *v0;
            TRIANGLENORMAL(&collisionTri.normal, v0, v1, v2);
            collisionTri.vertices[0] = v0;
            collisionTri.vertices[1] = v1;
            collisionTri.vertices[2] = v2;
            if (!cbParam->u.worldCB(cbParam->intersection, worldSector, &collisionTri, distance, cbParam->data)) {
                return FALSE;
            }
        }
        polyIndex++;
    }
    return TRUE;
}

static RwInt32 LeafNodeForAllSpherePolyIntersections(RwInt32 numPolygons, RwInt32 polyOffset, void* data)
{
    PolyTestParam* isData;
    CallBackParam* cbParam;
    TestSphere* testSphere;
    RpWorldSector* worldSector;
    RwV3d* vertices;
    RpPolygon* polygons;
    RwUInt16* polyIndex;
    RpPolygon* poly;
    RwV3d* v0;
    RwV3d* v1;
    RwV3d* v2;
    RwReal distance;
    RpCollisionTriangle collisionTri;

    isData = (PolyTestParam*)data;
    worldSector = isData->worldSector;
    cbParam = isData->cbParam;
    testSphere = (TestSphere*)isData->leafTestData;
    vertices = worldSector->vertices;
    polygons = worldSector->polygons;
    polyIndex = &RPCOLLISIONWORLDSECTORGETDATA(worldSector)->triangleMap[polyOffset];

    while (numPolygons--) {
        poly = &polygons[*polyIndex];
        v0 = &vertices[poly->vertIndex[0]];
        v1 = &vertices[poly->vertIndex[1]];
        v2 = &vertices[poly->vertIndex[2]];
        if (RtIntersectionSphereTriangle(testSphere->sphere, v0, v1, v2, &collisionTri.normal, &distance)) {
            collisionTri.point = *v0;
            collisionTri.index = *polyIndex;
            collisionTri.vertices[0] = v0;
            collisionTri.vertices[1] = v1;
            collisionTri.vertices[2] = v2;
            distance *= testSphere->recipRadius;
            if (!cbParam->u.worldCB(cbParam->intersection, worldSector, &collisionTri, distance, cbParam->data)) {
                return FALSE;
            }
        }
        polyIndex++;
    }
    return TRUE;
}

static RwInt32 LeafNodeForAllBoxPolyIntersections(RwInt32 numPolygons, RwInt32 polyOffset, void* data)
{
    PolyTestParam* isData;
    CallBackParam* cbParam;
    RpWorldSector* worldSector;
    RwV3d* vertices;
    RpPolygon* polygons;
    RwUInt16* polyIndex;
    RpPolygon* poly;
    RwV3d* v0;
    RwV3d* v1;
    RwV3d* v2;
    RpCollisionTriangle collisionTri;

    isData = (PolyTestParam*)data;
    worldSector = isData->worldSector;
    cbParam = isData->cbParam;
    vertices = worldSector->vertices;
    polygons = worldSector->polygons;
    polyIndex = &RPCOLLISIONWORLDSECTORGETDATA(worldSector)->triangleMap[polyOffset];

    while (numPolygons--) {
        poly = &polygons[*polyIndex];
        v0 = &vertices[poly->vertIndex[0]];
        v1 = &vertices[poly->vertIndex[1]];
        v2 = &vertices[poly->vertIndex[2]];
        if (RtIntersectionBBoxTriangle(&isData->bbox, v0, v1, v2)) {
            collisionTri.point = *v0;
            collisionTri.index = *polyIndex;
            TRIANGLENORMAL(&collisionTri.normal, v0, v1, v2);
            collisionTri.vertices[0] = v0;
            collisionTri.vertices[1] = v1;
            collisionTri.vertices[2] = v2;
            if (!cbParam->u.worldCB(cbParam->intersection, worldSector, &collisionTri, 0.0f, cbParam->data)) {
                return FALSE;
            }
        }
        polyIndex++;
    }
    return TRUE;
}

static RpWorldSector* WorldSectorForAllLinePolyIntersections(RpWorldSector* worldSector, void* data)
{
    PolyLineTestParam* isData = (PolyLineTestParam*)data;
    RpCollisionData* collData = RPCOLLISIONWORLDSECTORGETDATA(worldSector);

    isData->worldSector = worldSector;
    if (!collData) {
        return OldWorldSectorForAllLinePolyIntersections(worldSector, isData);
    }
    if (!_rpCollBSPTreeForAllLineLeafNodeIntersections(collData->tree, &isData->clippedLine, &isData->grad,
                                                       LeafNodeForAllLinePolyIntersections, isData)) {
        return NULL;
    }
    return worldSector;
}

static RpWorldSector* WorldSectorForAllBoxedPrimitivePolyIntersections(RpWorldSector* worldSector, void* data)
{
    PolyTestParam* isData = (PolyTestParam*)data;
    RpCollisionData* collData = RPCOLLISIONWORLDSECTORGETDATA(worldSector);

    isData->worldSector = worldSector;
    if (!collData) {
        return OldWorldSectorForAllBoxedPrimitivePolyIntersections(worldSector, isData);
    }
    if (!_rpCollBSPTreeForAllBoxLeafNodeIntersections(collData->tree, &isData->bbox, isData->leafTest, isData)) {
        return NULL;
    }
    return worldSector;
}

RpWorld* RpCollisionWorldForAllIntersections(RpWorld* world, RpIntersection* intersection,
                                             RpIntersectionCallBackWorldTriangle callBack, void* data)
{
    CallBackParam cbParam;

    cbParam.u.worldCB = callBack;
    cbParam.intersection = intersection;
    cbParam.data = data;

    switch (intersection->type) {
    case rpINTERSECTPOINT:
        RWERROR((E_RP_COLLIS_INV_INTERSECTION));
        return NULL;
    case rpINTERSECTLINE: {
        PolyLineTestParam isData;

        isData.start = &intersection->t.line.start;
        isData.delta.x = intersection->t.line.end.x - intersection->t.line.start.x;
        isData.delta.y = intersection->t.line.end.y - intersection->t.line.start.y;
        isData.delta.z = intersection->t.line.end.z - intersection->t.line.start.z;
        isData.cbParam = &cbParam;
        {
            RwReal recip = 0.0f;

            if (isData.delta.x != 0.0f) {
                recip = 1.0f / isData.delta.x;
            }
            isData.grad.dydx = isData.delta.y * recip;
            isData.grad.dzdx = isData.delta.z * recip;
        }
        {
            RwReal recip = 0.0f;

            if (isData.delta.y != 0.0f) {
                recip = 1.0f / isData.delta.y;
            }
            isData.grad.dxdy = isData.delta.x * recip;
            isData.grad.dzdy = isData.delta.z * recip;
        }
        {
            RwReal recip = 0.0f;

            if (isData.delta.z != 0.0f) {
                recip = 1.0f / isData.delta.z;
            }
            isData.grad.dxdz = isData.delta.x * recip;
            isData.grad.dydz = isData.delta.y * recip;
        }
        WorldForAllLineWorldSectorIntersections(world, &intersection->t.line, &isData.grad,
                                                WorldSectorForAllLinePolyIntersections, &isData,
                                                &isData.clippedLine);
        return world;
    }
    case rpINTERSECTSPHERE: {
        TestSphere testSphere;
        PolyTestParam isData;

        isData.bbox.sup = intersection->t.sphere.center;
        isData.bbox.inf = intersection->t.sphere.center;
        isData.bbox.inf.x -= intersection->t.sphere.radius;
        isData.bbox.inf.y -= intersection->t.sphere.radius;
        isData.bbox.inf.z -= intersection->t.sphere.radius;
        isData.bbox.sup.x += intersection->t.sphere.radius;
        isData.bbox.sup.y += intersection->t.sphere.radius;
        isData.bbox.sup.z += intersection->t.sphere.radius;
        testSphere.sphere = &intersection->t.sphere;
        testSphere.recipRadius = 1.0f / intersection->t.sphere.radius;
        isData.leafTest = LeafNodeForAllSpherePolyIntersections;
        isData.v303leafTest = OldLeafNodeForAllSpherePolyIntersections;
        isData.leafTestData = &testSphere;
        isData.cbParam = &cbParam;
        WorldForAllBoxWorldSectorIntersections(world, &isData.bbox, WorldSectorForAllBoxedPrimitivePolyIntersections,
                                               &isData);
        return world;
    }
    case rpINTERSECTBOX: {
        PolyTestParam isData;

        isData.bbox = intersection->t.box;
        isData.leafTest = LeafNodeForAllBoxPolyIntersections;
        isData.v303leafTest = OldLeafNodeForAllBoxPolyIntersections;
        isData.cbParam = &cbParam;
        WorldForAllBoxWorldSectorIntersections(world, &isData.bbox, WorldSectorForAllBoxedPrimitivePolyIntersections,
                                               &isData);
        return world;
    }
    case rpINTERSECTATOMIC: {
        RpAtomic* atomic = (RpAtomic*)intersection->t.object;
        RwSphere sphere;
        TestSphere testSphere;
        PolyTestParam isData;

        sphere = *RpAtomicGetWorldBoundingSphere(atomic);
        testSphere.sphere = &sphere;
        testSphere.recipRadius = 1.0f / sphere.radius;
        isData.bbox.sup = testSphere.sphere->center;
        isData.bbox.inf = testSphere.sphere->center;
        isData.bbox.inf.x -= sphere.radius;
        isData.bbox.inf.y -= sphere.radius;
        isData.bbox.inf.z -= sphere.radius;
        isData.bbox.sup.x += sphere.radius;
        isData.bbox.sup.y += sphere.radius;
        isData.bbox.sup.z += sphere.radius;
        isData.leafTest = LeafNodeForAllSpherePolyIntersections;
        isData.v303leafTest = OldLeafNodeForAllSpherePolyIntersections;
        isData.leafTestData = &testSphere;
        isData.cbParam = &cbParam;
        RpAtomicForAllWorldSectors(atomic, WorldSectorForAllBoxedPrimitivePolyIntersections, &isData);
        return world;
    }
    default:
        RWERROR((E_RP_COLLIS_INV_INTERSECTION));
        return NULL;
    }
}

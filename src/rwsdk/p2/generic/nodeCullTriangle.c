/*
 * CullTriangle.csl: removes back facing triangles from an indexed triangle
 * list by moving the last triangle into the culled one's slot. Parallel
 * cameras test the camera space winding; perspective cameras test screen
 * space winding, or the camera space plane for triangles that need clipping.
 */

#include "rwsdk/baim3d_int.h"
#include "rwsdk/rpworld_int.h"

extern RxClusterDefinition RxClCamSpace3DVertices;
extern RxClusterDefinition RxClScrSpace2DVertices;
extern RxClusterDefinition RxClIndices;
extern RxClusterDefinition RxClMeshState;

#define rwPERSPECTIVE 1

typedef struct RxCamSpace3DVertex {
    /* 0x00 */ RwV3d cameraVertex;
    /* 0x0C */ RwUInt8 clipFlags;
    /* 0x0D */ RwUInt8 pad[3];
    /* 0x10 */ RwRGBA col;
    /* 0x14 */ RwReal u;
    /* 0x18 */ RwReal v;
} RxCamSpace3DVertex;

typedef struct RxScrSpace2DVertex {
    /* 0x00 */ RwReal x;
    /* 0x04 */ RwReal y;
    /* 0x08 */ RwReal z;
} RxScrSpace2DVertex;

typedef RwBool (*CullTriangleFn)(RxPipelineNodeInstance* self, RxPacket* packet);

#define RxClusterResetCursor(_cl) ((_cl)->currentData = (_cl)->data)

#define RxClusterLockRead(_packet, _clusterIndex)                                                                      \
    (((RwInt32)((_packet)->inputToClusterSlot[(_clusterIndex)]) == -1)                                                 \
         ? (RxCluster*)NULL                                                                                            \
         : (RxClusterResetCursor(&(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]),               \
            &(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]))

#define RxClusterGetIndexedData(_cl, _type, _index) ((_type*)((RwUInt8*)(_cl)->data + (_cl)->stride * (_index)))

#define RxClusterIncCursorByStride(_cl, _stride) ((_cl)->currentData = (RwUInt8*)(_cl)->currentData + (_stride))

#define RwV3dCrossProduct(_o, _a, _b)                                                                                  \
    do {                                                                                                               \
        (_o)->x = (_a)->y * (_b)->z - (_a)->z * (_b)->y;                                                               \
        (_o)->y = (_a)->z * (_b)->x - (_a)->x * (_b)->z;                                                               \
        (_o)->z = (_a)->x * (_b)->y - (_a)->y * (_b)->x;                                                               \
    } while (0)

#define RwV3dDotProduct(_a, _b) ((_a)->x * (_b)->x + (_a)->y * (_b)->y + (_a)->z * (_b)->z)

/* Replaces the current triangle with the last one and shortens the list. */
#define CULLCURRENTTRIANGLE(_indices, _meshData, _i)                                                                   \
    do {                                                                                                               \
        RxVertexIndex* _last = &((RxVertexIndex*)(_indices)->data)[(_meshData)->NumElements * 3 - 3];               \
                                                                                                                       \
        RxClusterGetCursorData(_indices, RxVertexIndex)[0] = _last[0];                                               \
        RxClusterGetCursorData(_indices, RxVertexIndex)[1] = _last[1];                                               \
        RxClusterGetCursorData(_indices, RxVertexIndex)[2] = _last[2];                                               \
        (_meshData)->NumElements--;                                                                                    \
        (_i)--;                                                                                                        \
    } while (0)

static RwBool CullTriangleParallel(RxPipelineNodeInstance* self, RxPacket* packet)
{
    RxCluster* camVerts = RxClusterLockRead(packet, 0);
    RxCluster* devVerts = RxClusterLockRead(packet, 1);
    RxCluster* indices = RxClusterLockWrite(packet, 2, self);
    RxCluster* meshState = RxClusterLockWrite(packet, 3, self);
    RxMeshStateVector* meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);
    RwUInt32 triStride = indices->stride * 3;
    RwUInt32 i;

    for (i = 0; i < meshData->NumElements; i++) {
        RxVertexIndex* idx = RxClusterGetCursorData(indices, RxVertexIndex);
        RxCamSpace3DVertex* v0 = RxClusterGetIndexedData(camVerts, RxCamSpace3DVertex, idx[0]);
        RxCamSpace3DVertex* v1 = RxClusterGetIndexedData(camVerts, RxCamSpace3DVertex, idx[1]);
        RxCamSpace3DVertex* v2 = RxClusterGetIndexedData(camVerts, RxCamSpace3DVertex, idx[2]);
        RwReal cross = (v0->cameraVertex.x - v1->cameraVertex.x) * (v2->cameraVertex.y - v1->cameraVertex.y) -
                       (v0->cameraVertex.y - v1->cameraVertex.y) * (v2->cameraVertex.x - v1->cameraVertex.x);

        if (*(RwInt32*)&cross < 0) {
            CULLCURRENTTRIANGLE(indices, meshData, i);
        } else {
            RxClusterIncCursorByStride(indices, triStride);
        }
    }

    if (meshData->NumElements < indices->numUsed * 3) {
        indices->numUsed = meshData->NumElements * 3;
    }

    if (!meshData->NumElements) {
        RxPacketDispatch(packet, 1, self);
    } else {
        RxPacketDispatch(packet, 0, self);
    }

    return TRUE;
}

static RwBool CullTrianglePerspective(RxPipelineNodeInstance* self, RxPacket* packet)
{
    RxCluster* camVerts = RxClusterLockRead(packet, 0);
    RxCluster* devVerts = RxClusterLockRead(packet, 1);
    RxCluster* indices = RxClusterLockWrite(packet, 2, self);
    RxCluster* meshState = RxClusterLockWrite(packet, 3, self);
    RxMeshStateVector* meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);
    RwUInt32 i;

    if (meshData->ClipFlagsOr == 0) {
        RwUInt32 triStride = indices->stride * 3;

        for (i = 0; i < meshData->NumElements; i++) {
            RxVertexIndex* idx = RxClusterGetCursorData(indices, RxVertexIndex);
            RxScrSpace2DVertex* v0 = RxClusterGetIndexedData(devVerts, RxScrSpace2DVertex, idx[0]);
            RxScrSpace2DVertex* v1 = RxClusterGetIndexedData(devVerts, RxScrSpace2DVertex, idx[1]);
            RxScrSpace2DVertex* v2 = RxClusterGetIndexedData(devVerts, RxScrSpace2DVertex, idx[2]);
            RwReal cross = (v0->x - v1->x) * (v2->y - v1->y) - (v0->y - v1->y) * (v2->x - v1->x);

            if (*(RwInt32*)&cross <= 0) {
                CULLCURRENTTRIANGLE(indices, meshData, i);
            } else {
                RxClusterIncCursorByStride(indices, triStride);
            }
        }
    } else {
        RwUInt32 triStride = indices->stride * 3;

        for (i = 0; i < meshData->NumElements; i++) {
            RxVertexIndex* idx = RxClusterGetCursorData(indices, RxVertexIndex);
            RxCamSpace3DVertex* c0 = RxClusterGetIndexedData(camVerts, RxCamSpace3DVertex, idx[0]);
            RxCamSpace3DVertex* c1 = RxClusterGetIndexedData(camVerts, RxCamSpace3DVertex, idx[1]);
            RxCamSpace3DVertex* c2 = RxClusterGetIndexedData(camVerts, RxCamSpace3DVertex, idx[2]);

            if (c0->clipFlags & c1->clipFlags & c2->clipFlags) {
                CULLCURRENTTRIANGLE(indices, meshData, i);
            } else if (c0->clipFlags | c1->clipFlags | c2->clipFlags) {
                RwV3d edge0;
                RwV3d edge1;
                RwV3d normal;
                RwReal dot;

                edge0.x = c0->cameraVertex.x - c1->cameraVertex.x;
                edge0.y = c0->cameraVertex.y - c1->cameraVertex.y;
                edge0.z = c0->cameraVertex.z - c1->cameraVertex.z;
                edge1.x = c2->cameraVertex.x - c1->cameraVertex.x;
                edge1.y = c2->cameraVertex.y - c1->cameraVertex.y;
                edge1.z = c2->cameraVertex.z - c1->cameraVertex.z;
                RwV3dCrossProduct(&normal, &edge0, &edge1);
                dot = RwV3dDotProduct(&normal, &c0->cameraVertex);

                if (*(RwInt32*)&dot <= 0) {
                    CULLCURRENTTRIANGLE(indices, meshData, i);
                } else {
                    RxClusterIncCursorByStride(indices, triStride);
                }
            } else {
                RxScrSpace2DVertex* v0 = RxClusterGetIndexedData(devVerts, RxScrSpace2DVertex, idx[0]);
                RxScrSpace2DVertex* v1 = RxClusterGetIndexedData(devVerts, RxScrSpace2DVertex, idx[1]);
                RxScrSpace2DVertex* v2 = RxClusterGetIndexedData(devVerts, RxScrSpace2DVertex, idx[2]);
                RwReal cross = (v0->x - v1->x) * (v2->y - v1->y) - (v0->y - v1->y) * (v2->x - v1->x);

                if (*(RwInt32*)&cross <= 0) {
                    CULLCURRENTTRIANGLE(indices, meshData, i);
                } else {
                    RxClusterIncCursorByStride(indices, triStride);
                }
            }
        }
    }

    if (meshData->NumElements < indices->numUsed * 3) {
        indices->numUsed = meshData->NumElements * 3;
    }

    if (!meshData->NumElements) {
        RxPacketDispatch(packet, 1, self);
    } else {
        RxPacketDispatch(packet, 0, self);
    }

    return TRUE;
}

static CullTriangleFn CullTriangleFunctions[] = { CullTriangleParallel, CullTrianglePerspective };

RwBool CullTriangleNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    CullTriangleFn cullFn =
        CullTriangleFunctions[((RwCamera*)RWSRCGLOBAL(curCamera))->projectionType == rwPERSPECTIVE];

    return cullFn(self, RxPacketFetch(self));
}

RxNodeDefinition* RxNodeDefinitionGetCullTriangle(void)
{
    static RxClusterRef N1clofinterest[] = {
        { &RxClCamSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClScrSpace2DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClIndices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED },
    };

    static RxClusterValidityReq N1inputreqs[] = {
        rxCLREQ_REQUIRED, rxCLREQ_REQUIRED, rxCLREQ_REQUIRED, rxCLREQ_REQUIRED,
    };

    static RxClusterValid N1outcl1[] = {
        rxCLVALID_NOCHANGE, rxCLVALID_NOCHANGE, rxCLVALID_VALID, rxCLVALID_VALID,
    };

    static RxClusterValid N1outcl2[] = {
        rxCLVALID_NOCHANGE, rxCLVALID_NOCHANGE, rxCLVALID_INVALID, rxCLVALID_VALID,
    };

    static RwChar _CullTriangleOut[] = "CullTriangleOut";

    static RwChar _CullTriangleAllCulled[] = "CullTriangleAllCulled";

    static RxOutputSpec N1outputs[] = {
        { _CullTriangleOut, N1outcl1, rxCLVALID_NOCHANGE },
        { _CullTriangleAllCulled, N1outcl2, rxCLVALID_NOCHANGE },
    };

    static RwChar _CullTriangle_csl[] = "CullTriangle.csl";

    static RxNodeDefinition nodeCullTriangleCSL = {
        _CullTriangle_csl,
        { CullTriangleNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 4, N1clofinterest, N1inputreqs, 2, N1outputs },
        0,
        0,
        0,
    };

    return &nodeCullTriangleCSL;
}

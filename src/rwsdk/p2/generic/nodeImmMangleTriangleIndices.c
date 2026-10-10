/*
 * ImmMangleTriangleIndices.csl: makes sure an immediate mode triangle packet
 * has an index cluster in triangle list form, generating indices for
 * unindexed primitives and expanding fan and strip indices in place.
 */

#include "rwsdk/baim3d_int.h"

extern RxClusterDefinition RxClMeshState;
extern RxClusterDefinition RxClIndices;

void* RxHeapAlloc(void* heap, RwUInt32 size);
RxCluster* RxClusterSetData(RxCluster* cluster, void* data, RwUInt32 stride, RwUInt32 numElements);

#define RxClusterResetCursor(_cl) ((_cl)->currentData = (_cl)->data)

#define RxClusterLockRead(_packet, _clusterIndex)                                                                      \
    (((RwInt32)((_packet)->inputToClusterSlot[(_clusterIndex)]) == -1)                                                 \
         ? (RxCluster*)NULL                                                                                            \
         : (RxClusterResetCursor(&(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]),               \
            &(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]))

RwBool ImmMangleTriangleIndicesNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    RxPacket* packet;
    RxCluster* meshState;
    RxCluster* indices;
    RxMeshStateVector* meshData;

    packet = RxPacketFetch(self);
    meshState = RxClusterLockWrite(packet, 0, self);
    meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);
    indices = RxClusterLockRead(packet, 1);

    if (indices->numUsed == 0) {
        RxCluster* outIndices = RxClusterLockWrite(packet, 1, self);
        RwUInt32 numIndices;
        RxVertexIndex* idx;
        RwUInt32 i;

        switch (meshData->PrimType) {
        case rwPRIMTYPETRILIST:
            numIndices = meshData->NumElements * 3;
            outIndices = RxClusterInitializeData(outIndices, numIndices, sizeof(RxVertexIndex));
            idx = RxClusterGetCursorData(outIndices, RxVertexIndex);
            for (i = 0; i < numIndices; i++) {
                *idx++ = (RxVertexIndex)i;
            }
            outIndices->numUsed = numIndices;
            break;
        case rwPRIMTYPETRIFAN:
            numIndices = meshData->NumElements * 3;
            outIndices = RxClusterInitializeData(outIndices, numIndices, sizeof(RxVertexIndex));
            idx = RxClusterGetCursorData(outIndices, RxVertexIndex);
            for (i = 0; i < meshData->NumElements; i++) {
                idx[0] = 0;
                idx[1] = (RxVertexIndex)(i + 1);
                idx[2] = (RxVertexIndex)(i + 2);
                idx += 3;
            }
            outIndices->numUsed = numIndices;
            meshData->PrimType = rwPRIMTYPETRILIST;
            break;
        case rwPRIMTYPETRISTRIP:
            numIndices = meshData->NumElements * 3;
            outIndices = RxClusterInitializeData(outIndices, numIndices, sizeof(RxVertexIndex));
            idx = RxClusterGetCursorData(outIndices, RxVertexIndex);
            for (i = 0; i < meshData->NumElements; i++) {
                RwUInt32 odd = i & 1;

                idx[0] = (RxVertexIndex)(i + odd);
                idx[1] = (RxVertexIndex)(i + (odd ^ 1));
                idx[2] = (RxVertexIndex)(i + 2);
                idx += 3;
            }
            outIndices->numUsed = numIndices;
            meshData->PrimType = rwPRIMTYPETRILIST;
            break;
        default:
            RxPacketDispatch(packet, 0, self);
            return FALSE;
        }
    } else {
        switch (meshData->PrimType) {
        case rwPRIMTYPETRILIST:
            break;
        case rwPRIMTYPETRIFAN: {
            RwUInt32 numTris = indices->numUsed - 2;
            RxVertexIndex first = *RxClusterGetCursorData(indices, RxVertexIndex);
            RxVertexIndex* src = (RxVertexIndex*)((RwUInt8*)indices->data + indices->stride * numTris);
            RxVertexIndex* dst;
            RwUInt32 numIndices;
            RwUInt32 i;
            void* mem;

            mem = RxHeapAlloc(params->heap, numTris * 6);
            numIndices = numTris * 3;
            indices = RxClusterSetData(indices, mem, sizeof(RxVertexIndex), numIndices);
            dst = (RxVertexIndex*)((RwUInt8*)indices->data + indices->stride * ((numTris - 1) * 3));
            for (i = 0; i < numTris; i++) {
                dst[2] = src[1];
                dst[1] = src[0];
                dst[0] = first;
                src--;
                dst -= 3;
            }
            indices->numUsed = numIndices;
            meshData->PrimType = rwPRIMTYPETRILIST;
            break;
        }
        case rwPRIMTYPETRISTRIP: {
            RwUInt32 numTris = indices->numUsed - 2;
            RxVertexIndex* src = (RxVertexIndex*)((RwUInt8*)indices->data + indices->stride * (indices->numUsed - 3));
            RxVertexIndex* dst;
            RwUInt32 numIndices;
            RwUInt32 odd;
            RwUInt32 i;
            void* mem;

            mem = RxHeapAlloc(params->heap, numTris * 6);
            numIndices = numTris * 3;
            indices = RxClusterSetData(indices, mem, sizeof(RxVertexIndex), numIndices);
            dst = (RxVertexIndex*)((RwUInt8*)indices->data + indices->stride * ((numTris - 1) * 3));
            odd = numTris & 1;
            for (i = 0; i < numTris; i++) {
                dst[2] = src[2];
                dst[1] = src[odd];
                dst[0] = src[odd ^ 1];
                odd ^= 1;
                src--;
                dst -= 3;
            }
            indices->numUsed = numIndices;
            meshData->PrimType = rwPRIMTYPETRILIST;
            break;
        }
        default:
            RxPacketDispatch(packet, 0, self);
            return FALSE;
        }
    }

    RxPacketDispatch(packet, 0, self);

    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetImmMangleTriangleIndices(void)
{
    static RxClusterRef N1clofinterest[] = {
        { &RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClIndices, rxCLALLOWABSENT, rxCLRESERVED },
    };

    static RxClusterValidityReq N1inputreqs[] = { rxCLREQ_REQUIRED, rxCLREQ_OPTIONAL };

    static RxClusterValid N1outcl1[] = { rxCLVALID_VALID, rxCLVALID_VALID };

    static RwChar _ImmRenderSetupOut[] = "ImmRenderSetupOut";

    static RxOutputSpec N1outputs[] = { { _ImmRenderSetupOut, N1outcl1, rxCLVALID_NOCHANGE } };

    static RwChar _ImmMangleTriangleIndices_csl[] = "ImmMangleTriangleIndices.csl";

    static RxNodeDefinition nodeImmMangleTriangleIndicesCSL = {
        _ImmMangleTriangleIndices_csl,
        { ImmMangleTriangleIndicesNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 2, N1clofinterest, N1inputreqs, 1, N1outputs },
        0,
        0,
        0,
    };

    return &nodeImmMangleTriangleIndicesCSL;
}

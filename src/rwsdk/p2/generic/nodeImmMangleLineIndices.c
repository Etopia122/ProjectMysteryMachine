/*
 * ImmMangleLineIndices.csl: makes sure an immediate mode line packet has an
 * index cluster in line list form, generating indices for unindexed
 * primitives and expanding poly line indices into line pairs in place.
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

RwBool ImmMangleLineIndicesNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    RxPacket* packet;
    RxCluster* meshState;
    RxCluster* indices;
    RxMeshStateVector* meshData;

    if (_rxExecCtxGlobal.pipeline->embeddedPacketState == rxPKST_PENDING) {
        _rxExecCtxGlobal.pipeline->embeddedPacketState = rxPKST_INUSE;
        packet = _rxExecCtxGlobal.pipeline->embeddedPacket;
    } else {
        packet = NULL;
    }
    meshState = RxClusterLockWrite(packet, 0, self);
    meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);
    indices = RxClusterLockRead(packet, 1);

    if (indices->numUsed == 0) {
        indices = RxClusterLockWrite(packet, 1, self);

        if (meshData->PrimType == rwPRIMTYPELINELIST) {
            RwUInt32 numIndices = meshData->NumElements * 2;
            RxVertexIndex* idx;
            RwUInt32 i;

            indices = RxClusterInitializeData(indices, numIndices, sizeof(RxVertexIndex));
            idx = RxClusterGetCursorData(indices, RxVertexIndex);
            for (i = 0; i < numIndices; i++) {
                *idx++ = (RxVertexIndex)i;
            }
            indices->numUsed = numIndices;
        } else if (meshData->PrimType == rwPRIMTYPEPOLYLINE) {
            RwUInt32 numIndices = meshData->NumElements * 2;
            RxVertexIndex* idx;
            RwUInt32 i;

            indices = RxClusterInitializeData(indices, numIndices, sizeof(RxVertexIndex));
            idx = RxClusterGetCursorData(indices, RxVertexIndex);
            for (i = 0; i < meshData->NumElements; i++) {
                *idx++ = (RxVertexIndex)i;
                *idx++ = (RxVertexIndex)(i + 1);
            }
            indices->numUsed = numIndices;
            meshData->PrimType = rwPRIMTYPELINELIST;
        } else {
            RxPacketDispatch(packet, 0, self);
            return FALSE;
        }
    } else if (meshData->PrimType != rwPRIMTYPELINELIST) {
        if (meshData->PrimType == rwPRIMTYPEPOLYLINE) {
            RwUInt32 numLines = indices->numUsed - 1;
            RxVertexIndex* src;
            RxVertexIndex* dst;
            RwUInt32 i;
            void* mem;

            src = (RxVertexIndex*)((RwUInt8*)indices->data + indices->stride * (numLines - 1));
            mem = RxHeapAlloc(params->heap, numLines * 4);
            indices = RxClusterSetData(indices, mem, sizeof(RxVertexIndex), numLines * 2);
            dst = (RxVertexIndex*)((RwUInt8*)indices->data + indices->stride * ((numLines - 1) * 2));
            for (i = 0; i < numLines; i++) {
                dst[1] = src[1];
                dst[0] = src[0];
                src--;
                dst -= 2;
            }
            indices->numUsed = numLines << 1;
            meshData->PrimType = rwPRIMTYPELINELIST;
        } else {
            RxPacketDispatch(packet, 0, self);
            return FALSE;
        }
    }

    RxPacketDispatch(packet, 0, self);

    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetImmMangleLineIndices(void)
{
    static RxClusterRef N1clofinterest[] = {
        { &RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClIndices, rxCLALLOWABSENT, rxCLRESERVED },
    };

    static RxClusterValidityReq N1inputreqs[] = { rxCLREQ_REQUIRED, rxCLREQ_OPTIONAL };

    static RxClusterValid N1outcl1[] = { rxCLVALID_VALID, rxCLVALID_VALID };

    static RwChar _ImmRenderSetupOut[] = "ImmRenderSetupOut";

    static RxOutputSpec N1outputs[] = { { _ImmRenderSetupOut, N1outcl1, rxCLVALID_NOCHANGE } };

    static RwChar _ImmMangleLineIndices_csl[] = "ImmMangleLineIndices.csl";

    static RxNodeDefinition nodeImmMangleLineIndicesCSL = {
        _ImmMangleLineIndices_csl,
        { ImmMangleLineIndicesNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 2, N1clofinterest, N1inputreqs, 1, N1outputs },
        0,
        0,
        0,
    };

    return &nodeImmMangleLineIndicesCSL;
}

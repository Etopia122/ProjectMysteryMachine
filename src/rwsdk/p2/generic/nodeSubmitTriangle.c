/*
 * SubmitTriangle.csl: applies the packet's texture and vertex alpha state and
 * submits its screen space vertices as a 2D triangle list, indexed when the
 * packet has indices.
 */

#include "rwsdk/baim3d_int.h"

extern RxClusterDefinition RxClScrSpace2DVertices;
extern RxClusterDefinition RxClIndices;
extern RxClusterDefinition RxClMeshState;
extern RxClusterDefinition RxClRenderState;

#define rwRENDERSTATETEXTURERASTER 1
#define rwRENDERSTATETEXTUREADDRESS 2
#define rwRENDERSTATETEXTUREADDRESSU 3
#define rwRENDERSTATETEXTUREADDRESSV 4
#define rwRENDERSTATEZTESTENABLE 6
#define rwRENDERSTATESHADEMODE 7
#define rwRENDERSTATEZWRITEENABLE 8
#define rwRENDERSTATETEXTUREFILTER 9
#define rwRENDERSTATESRCBLEND 10
#define rwRENDERSTATEDESTBLEND 11
#define rwRENDERSTATEVERTEXALPHAENABLE 12

RwBool RwRenderStateSet(RwInt32 state, void* value);
RwBool RwIm2DRenderPrimitive(RwInt32 primType, void* vertices, RwInt32 numVertices);
RwBool RwIm2DRenderIndexedPrimitive(RwInt32 primType, void* vertices, RwInt32 numVertices, RxVertexIndex* indices,
                                    RwInt32 numIndices);

#define RxClusterResetCursor(_cl) ((_cl)->currentData = (_cl)->data)

#define RxClusterLockRead(_packet, _clusterIndex)                                                                      \
    (((RwInt32)((_packet)->inputToClusterSlot[(_clusterIndex)]) == -1)                                                 \
         ? (RxCluster*)NULL                                                                                            \
         : (RxClusterResetCursor(&(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]),               \
            &(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]))

RwBool SubmitTriangleNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    RxPacket* packet;
    RxCluster* devVerts;
    RxCluster* indices;
    RxCluster* meshState;
    RxCluster* renderState;
    RxMeshStateVector* meshData;
    RxRenderStateVector* rsvp;

    packet = RxPacketFetch(self);

    devVerts = RxClusterLockRead(packet, 0);
    indices = RxClusterLockRead(packet, 1);
    meshState = RxClusterLockRead(packet, 2);
    renderState = RxClusterLockRead(packet, 3);

    meshData = RxClusterGetCursorData(meshState, RxMeshStateVector);
    if (renderState && renderState->data) {
        rsvp = RxClusterGetCursorData(renderState, RxRenderStateVector);
    } else {
        rsvp = &RXPIPELINEGLOBAL(defaultRenderState);
    }

    if (rsvp->TextureRaster) {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, rsvp->TextureRaster);
        RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rsvp->FilterMode);
        if (rsvp->AddressModeU == rsvp->AddressModeV) {
            RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rsvp->AddressModeU);
        } else {
            RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSU, (void*)rsvp->AddressModeU);
            RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSV, (void*)rsvp->AddressModeV);
        }
    } else {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    }
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,
                     (void*)((rsvp->Flags & rxRENDERSTATEFLAG_VERTEXALPHAENABLE) != 0));

    if (indices && indices->numAlloced) {
        RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, RxClusterGetCursorData(devVerts, void), meshData->NumVertices,
                                     RxClusterGetCursorData(indices, RxVertexIndex), indices->numUsed);
    } else {
        RwIm2DRenderPrimitive(rwPRIMTYPETRILIST, RxClusterGetCursorData(devVerts, void), meshData->NumVertices);
    }

    RxPacketDispatch(packet, 0, self);

    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetSubmitTriangle(void)
{
    static RxClusterRef N1clofinterest[] = {
        { &RxClScrSpace2DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClIndices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClRenderState, rxCLALLOWABSENT, rxCLRESERVED },
    };

    static RxClusterValidityReq N1inputreqs[] = {
        rxCLREQ_REQUIRED, rxCLREQ_OPTIONAL, rxCLREQ_REQUIRED, rxCLREQ_OPTIONAL,
    };

    static RxClusterValid N1outcl1[] = {
        rxCLVALID_VALID, rxCLVALID_NOCHANGE, rxCLVALID_VALID, rxCLVALID_NOCHANGE,
    };

    static RwChar _SubmitOut[] = "SubmitOut";

    static RxOutputSpec N1outputs[] = { { _SubmitOut, N1outcl1, rxCLVALID_NOCHANGE } };

    static RwChar _SubmitTriangle_csl[] = "SubmitTriangle.csl";

    static RxNodeDefinition nodeSubmitTriangleCSL = {
        _SubmitTriangle_csl,
        { SubmitTriangleNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 4, N1clofinterest, N1inputreqs, 1, N1outputs },
        0,
        0,
        0,
    };

    return &nodeSubmitTriangleCSL;
}

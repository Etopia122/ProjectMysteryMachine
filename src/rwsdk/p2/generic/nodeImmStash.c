/*
 * ImmStash.csl: the end of the immediate mode transform pipeline. Records
 * the transformed packet's clusters in the vertex pool's stash, marking them
 * external so they survive the packet, for the render pipelines to use.
 */

#include "rwsdk/baim3d_int.h"

extern RxClusterDefinition RxClObjSpace3DVertices;
extern RxClusterDefinition RxClCamSpace3DVertices;
extern RxClusterDefinition RxClScrSpace2DVertices;
extern RxClusterDefinition RxClMeshState;
extern RxClusterDefinition RxClRenderState;

void* memset(void* dst, int c, unsigned long n);

#define rxCLFLAGS_EXTERNAL 0x02

#define RxClusterResetCursor(_cl) ((_cl)->currentData = (_cl)->data)

#define RxClusterLockRead(_packet, _clusterIndex)                                                                      \
    (((RwInt32)((_packet)->inputToClusterSlot[(_clusterIndex)]) == -1)                                                 \
         ? (RxCluster*)NULL                                                                                            \
         : (RxClusterResetCursor(&(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]),               \
            &(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]))

RwBool ImmStashNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    rwIm3DPoolStash* stash = &((rwIm3DPool*)params->dataParam)->stash;
    RxPacket* packet;
    RxCluster* cl;

    if (_rxExecCtxGlobal.pipeline->embeddedPacketState == rxPKST_PENDING) {
        _rxExecCtxGlobal.pipeline->embeddedPacketState = rxPKST_INUSE;
        packet = _rxExecCtxGlobal.pipeline->embeddedPacket;
    } else {
        packet = NULL;
    }

    memset(stash, 0, sizeof(rwIm3DPoolStash));

    cl = RxClusterLockRead(packet, 0);
    stash->objVerts = RxClusterGetCursorData(cl, RxObjSpace3DVertex);
    cl->flags |= rxCLFLAGS_EXTERNAL;

    cl = RxClusterLockRead(packet, 1);
    stash->camVerts = RxClusterGetCursorData(cl, void);
    cl->flags |= rxCLFLAGS_EXTERNAL;

    cl = RxClusterLockRead(packet, 2);
    stash->devVerts = RxClusterGetCursorData(cl, void);
    cl->flags |= rxCLFLAGS_EXTERNAL;

    cl = RxClusterLockRead(packet, 3);
    stash->meshState = RxClusterGetCursorData(cl, void);
    stash->numVerts = ((RxMeshStateVector*)stash->meshState)->NumVertices;
    cl->flags |= rxCLFLAGS_EXTERNAL;

    cl = RxClusterLockRead(packet, 4);
    stash->renderState = RxClusterGetCursorData(cl, void);
    cl->flags |= rxCLFLAGS_EXTERNAL;

    _rxPacketDestroy(packet);

    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetImmStash(void)
{
    static RxClusterRef N1clofinterest[] = {
        { &RxClObjSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClCamSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClScrSpace2DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClRenderState, rxCLALLOWABSENT, rxCLRESERVED },
    };

    static RxClusterValidityReq N1inputreqs[] = {
        rxCLREQ_OPTIONAL, rxCLREQ_OPTIONAL, rxCLREQ_OPTIONAL, rxCLREQ_OPTIONAL, rxCLREQ_OPTIONAL,
    };

    static RwChar _ImmStash_csl[] = "ImmStash.csl";

    static RxNodeDefinition nodeImmStashCSL = {
        _ImmStash_csl,
        { ImmStashNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 5, N1clofinterest, N1inputreqs, 0, NULL },
        0,
        0,
        0,
    };

    return &nodeImmStashCSL;
}

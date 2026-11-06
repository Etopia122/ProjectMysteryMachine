/*
 * MaterialScatter.csl: sends each mesh packet on to its material's
 * pipeline (or the current default material pipeline).
 */

#include "rwsdk/rpworld_int.h"
#include "rwsdk/p2core_int.h"

extern RxClusterDefinition RxClMeshState;
extern RxClusterDefinition RxClObjSpace3DVertices;
extern RxClusterDefinition RxClIndices;
extern RxClusterDefinition RxClRenderState;
extern RxClusterDefinition RxClLights;

void _rxEmbeddedPacketBetweenPipelines(RxPipeline* fromPipeline, RxPipeline* toPipeline);

#define RxClusterResetCursor(_cl) ((_cl)->currentData = (_cl)->data)

#define RxClusterLockRead(_packet, _clusterIndex)                                                                      \
    (((RwInt32)((_packet)->inputToClusterSlot[(_clusterIndex)]) == -1)                                                 \
         ? (RxCluster*)NULL                                                                                            \
         : (RxClusterResetCursor(&(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]),               \
            &(_packet)->clusters[(_packet)->inputToClusterSlot[(_clusterIndex)]]))

#define RxPacketDispatchToPipeline(_packet, _dest, _self)                                                              \
    do {                                                                                                               \
        RxPipeline* _toPipeline = (_dest);                                                                             \
                                                                                                                       \
        if (_rxExecCtxGlobal.exitCode != FALSE) {                                                                      \
            RxPipeline* _fromPipeline = _rxExecCtxGlobal.pipeline;                                                     \
            RxPipelineNode* _nodeTo;                                                                                   \
            RwUInt32 _exitCode;                                                                                        \
                                                                                                                       \
            _rxEmbeddedPacketBetweenPipelines(_fromPipeline, _toPipeline);                                             \
            _rxExecCtxGlobal.pipeline = _toPipeline;                                                                   \
            _nodeTo = &_toPipeline->nodes[0];                                                                          \
            _exitCode = _nodeTo->nodeDef->nodeMethods.nodeBody(_nodeTo, &_rxExecCtxGlobal.params);                     \
            if (!_exitCode) {                                                                                          \
                _rxExecCtxGlobal.exitCode = _exitCode;                                                                 \
            }                                                                                                          \
            _rxExecCtxGlobal.pipeline = _fromPipeline;                                                                 \
        }                                                                                                              \
                                                                                                                       \
        if (_toPipeline->embeddedPacketState > rxPKST_UNUSED) {                                                        \
            _toPipeline->embeddedPacketState = rxPKST_INUSE;                                                           \
            _rxPacketDestroy(_toPipeline->embeddedPacket);                                                             \
        }                                                                                                              \
    } while (0)

RwBool MaterialScatterNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    RxPacket* packet = RxPacketFetch(self);
    RxCluster* meshState = RxClusterLockRead(packet, 0);
    RxPipeline* pipeline = RxClusterGetCursorData(meshState, RxMeshStateVector)->Pipeline;

    if (!pipeline) {
        pipeline = RXPIPELINEGLOBAL(currentMaterialPipeline);
    }

    RxPacketDispatchToPipeline(packet, pipeline, self);

    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetMaterialScatter(void)
{
    static RxClusterRef N1clofinterest[] = {
        { &RxClMeshState, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClObjSpace3DVertices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClIndices, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClRenderState, rxCLALLOWABSENT, rxCLRESERVED },
        { &RxClLights, rxCLALLOWABSENT, rxCLRESERVED },
    };

    static RxClusterValidityReq N1inputreqs[] = {
        rxCLREQ_REQUIRED, rxCLREQ_OPTIONAL, rxCLREQ_OPTIONAL, rxCLREQ_OPTIONAL, rxCLREQ_OPTIONAL,
    };

    static RwChar _MaterialScatter_csl[] = "MaterialScatter.csl";

    static RxNodeDefinition nodeMaterialScatterCSL = {
        _MaterialScatter_csl,
        { MaterialScatterNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 5, N1clofinterest, N1inputreqs, 0, NULL },
        0,
        0,
        0,
    };

    return &nodeMaterialScatterCSL;
}

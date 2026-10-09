/*
 * PowerPipe core: the global packet heap, pipeline creation and destruction,
 * cluster data management and pipeline execution.
 */

#include "rwsdk/p2core_int.h"

#define rwPLUGIN_ID 1 /* core */

#define rxCLFLAGS_CLUSTERVALID 0x01
#define rxCLFLAGS_EXTERNAL 0x02
#define rxCLFLAGS_EXTERNALMODIFIABLE 0x04
#define rxCLFLAGS_MODIFIED 0x08

#define rxPIPELINEMAXOUTPUTS 32

typedef struct RxHeap {
    /* 0x00 */ RwUInt32 pad00[6];
    /* 0x18 */ RwBool dirty;
} RxHeap;

#define RxHeapReset(heap) (!(heap)->dirty ? TRUE : _rxHeapReset(heap))

void* memset(void* dst, int c, unsigned long n);
void* memcpy(void* dst, const void* src, unsigned long n);
RxHeap* RxHeapCreate(RwUInt32 size);
void RxHeapDestroy(RxHeap* heap);
void* RxHeapAlloc(RxHeap* heap, RwUInt32 size);
void RxHeapFree(RxHeap* heap, void* block);
void* RxHeapRealloc(RxHeap* heap, void* block, RwUInt32 newSize, RwBool allowCopy);
RwBool _rxHeapReset(RxHeap* heap);
RxRenderStateVector* RxRenderStateVectorSetDefaultRenderStateVector(RxRenderStateVector* rsvp);

RwUInt32 _rxHeapInitialSize = 0x1000;
RwUInt32 _rxPipelineMaxNodes = 64;

static RwBool PipelineGlobalsOpen;
RxHeap* _rxHeapGlobal;

RwBool _rxPipelineClose(void)
{
    if (PipelineGlobalsOpen) {
        RwFreeListDestroy(RXPIPELINEGLOBAL(pipesFreeList));
        RXPIPELINEGLOBAL(pipesFreeList) = NULL;

        RxHeapDestroy(_rxHeapGlobal);
        _rxHeapGlobal = NULL;
        PipelineGlobalsOpen = FALSE;
    }
    return TRUE;
}

RwBool _rxPipelineOpen(void)
{
    if (!PipelineGlobalsOpen) {
        _rxHeapGlobal = RxHeapCreate(_rxHeapInitialSize);
        if (_rxHeapGlobal == NULL) {
            return FALSE;
        }

        RXPIPELINEGLOBAL(pipesFreeList) = RwFreeListCreate(sizeof(RxPipeline), 64, 4);
        if (RXPIPELINEGLOBAL(pipesFreeList) == NULL) {
            RxHeapDestroy(_rxHeapGlobal);
            _rxHeapGlobal = NULL;
            return FALSE;
        }

        RXPIPELINEGLOBAL(maxNodesPerPipe) = _rxPipelineMaxNodes;
        RxRenderStateVectorSetDefaultRenderStateVector(&RXPIPELINEGLOBAL(defaultRenderState));
        RXPIPELINEGLOBAL(unk38) = 0;
        RXPIPELINEGLOBAL(unk34) = 0;

        PipelineGlobalsOpen = TRUE;
        return TRUE;
    }
    return FALSE;
}

RxPipelineNode* PipelineNodeDestroy(RxPipelineNode* node, RxPipeline* pipeline)
{
    if (!pipeline->locked) {
        if (node->nodeDef->nodeMethods.pipelineNodeTerm) {
            ((void (*)(RxPipelineNode*))node->nodeDef->nodeMethods.pipelineNodeTerm)(node);
        }

        node->nodeDef->InputPipesCnt--;
        if (node->nodeDef->InputPipesCnt == 0) {
            if (node->nodeDef->nodeMethods.nodeTerm) {
                ((void (*)(RxNodeDefinition*))node->nodeDef->nodeMethods.nodeTerm)(node->nodeDef);
            }
            if (node->nodeDef->editable) {
                RwFree(node->nodeDef);
                node->nodeDef = NULL;
            }
        }

        if (node->initializationData) {
            RwFree(node->initializationData);
            node->initializationData = NULL;
            node->initializationDataSize = 0;
        }

        memset(node, 0, sizeof(RxPipelineNode));
    } else {
        RwUInt32 nodeIndex;

        if (node->initializationData) {
            RwFree(node->initializationData);
            node->initializationData = NULL;
            node->initializationDataSize = 0;
        }

        if (node->nodeDef->InputPipesCnt == 0 && node->nodeDef->editable) {
            RwFree(node->nodeDef);
            node->nodeDef = NULL;
        }

        if ((nodeIndex = node - pipeline->nodes) < pipeline->numNodes - 1) {
            RwUInt32* srcOutputs;
            RxPipelineNodeTopSortData* dstTopSort;
            RxPipelineNodeTopSortData* srcTopSort;
            RwUInt32 i;
            RwUInt32* dstOutputs;

            dstOutputs = (RwUInt32*)(pipeline->nodes + RXPIPELINEGLOBAL(maxNodesPerPipe));
            dstOutputs += nodeIndex * rxPIPELINEMAXOUTPUTS;
            srcOutputs = dstOutputs + rxPIPELINEMAXOUTPUTS;
            for (i = nodeIndex; i < pipeline->numNodes - 1; i++) {
                memcpy(dstOutputs, srcOutputs, rxPIPELINEMAXOUTPUTS * sizeof(RwUInt32));
                dstOutputs = srcOutputs;
                srcOutputs += rxPIPELINEMAXOUTPUTS;
            }

            dstOutputs = (RwUInt32*)(pipeline->nodes + RXPIPELINEGLOBAL(maxNodesPerPipe));
            dstOutputs += RXPIPELINEGLOBAL(maxNodesPerPipe) * rxPIPELINEMAXOUTPUTS;
            dstTopSort = (RxPipelineNodeTopSortData*)dstOutputs;
            srcTopSort = dstTopSort + 1;
            for (i = nodeIndex; i < pipeline->numNodes - 1; i++) {
                memcpy(dstTopSort, srcTopSort, sizeof(RxPipelineNodeTopSortData));
                dstTopSort = srcTopSort;
                srcTopSort++;
            }

            for (i = nodeIndex; i < pipeline->numNodes - 1; i++) {
                memcpy(&pipeline->nodes[i], &pipeline->nodes[i + 1], sizeof(RxPipelineNode));
                pipeline->nodes[i].outputs -= rxPIPELINEMAXOUTPUTS;
                pipeline->nodes[i].topSortData--;
            }

            {
                RwUInt32 output;
                RwUInt32 n;
                RwUInt32 j;

                for (n = 0; n < pipeline->numNodes - 1; n++) {
                    for (j = 0; j < pipeline->nodes[n].numOutputs; j++) {
                        output = pipeline->nodes[n].outputs[j];

                        if (output >= nodeIndex) {
                            if (nodeIndex == output) {
                                pipeline->nodes[n].outputs[j] = (RwUInt32)-1;
                            } else {
                                pipeline->nodes[n].outputs[j] = output - 1;
                            }
                        }
                    }
                }
            }
        }
    }

    pipeline->numNodes--;
    return node;
}

RxHeap* RxHeapGetGlobalHeap(void)
{
    return _rxHeapGlobal;
}

RxPacket* RxPacketCreate(RxPipelineNode* node)
{
    RxPacket* packet;

    _rxExecCtxGlobal.pipeline->embeddedPacketState = rxPKST_INUSE;
    packet = _rxExecCtxGlobal.pipeline->embeddedPacket;

    packet->inputToClusterSlot = node->inputToClusterSlot;
    packet->slotsContinue = node->slotsContinue;
    packet->slotClusterRefs = node->slotClusterRefs;
    return packet;
}

RxCluster* RxClusterSetExternalData(RxCluster* cluster, void* data, RwInt32 stride, RwInt32 numElements)
{
    if (cluster->data && !(cluster->flags & rxCLFLAGS_EXTERNAL) && cluster->data != data) {
        RxHeapFree(_rxHeapGlobal, cluster->data);
        cluster->data = NULL;
    }

    cluster->data = data;
    cluster->currentData = data;
    cluster->stride = stride;
    cluster->flags |= rxCLFLAGS_CLUSTERVALID | rxCLFLAGS_EXTERNAL;
    cluster->numAlloced = numElements;
    cluster->numUsed = numElements;
    return cluster;
}

RxCluster* RxClusterSetData(RxCluster* cluster, void* data, RwInt32 stride, RwInt32 numElements)
{
    if (cluster->data && !(cluster->flags & rxCLFLAGS_EXTERNAL) && cluster->data != data) {
        RxHeapFree(_rxHeapGlobal, cluster->data);
        cluster->data = NULL;
    }

    cluster->data = data;
    cluster->currentData = data;
    cluster->stride = stride;
    cluster->flags |= rxCLFLAGS_CLUSTERVALID;
    cluster->numAlloced = numElements;
    cluster->numUsed = numElements;
    return cluster;
}

RxCluster* RxClusterInitializeData(RxCluster* cluster, RwUInt32 numElements, RwUInt16 stride)
{
    RwUInt32 size = numElements * stride;

    if (cluster->data && !(cluster->flags & rxCLFLAGS_EXTERNAL)) {
        RxHeapFree(_rxHeapGlobal, cluster->data);
    }

    cluster->currentData = cluster->data = RxHeapAlloc(_rxHeapGlobal, size);
    cluster->numAlloced = numElements;
    cluster->numUsed = 0;
    cluster->flags |= rxCLFLAGS_CLUSTERVALID;
    cluster->stride = stride;
    return cluster;
}

RxCluster* RxClusterResize(RxCluster* cluster, RwUInt32 minSize)
{
    cluster->currentData = cluster->data = RxHeapRealloc(_rxHeapGlobal, cluster->data, minSize * cluster->stride, TRUE);
    cluster->flags |= rxCLFLAGS_CLUSTERVALID;

    if (cluster->numUsed > minSize) {
        cluster->numUsed = minSize;
    }
    cluster->numAlloced = minSize;
    return cluster;
}

RxCluster* RxClusterLockWrite(RxPacket* packet, RwUInt32 clusterIndex, RxPipelineNodeInstance* node)
{
    RwUInt32 slot = packet->inputToClusterSlot[clusterIndex];

    if (slot != (RwUInt32)-1) {
        RxCluster* cluster = &packet->clusters[slot];

        if (cluster->clusterRef == NULL) {
            RxPipelineCluster* clusterRef = packet->slotClusterRefs[slot];

            cluster->clusterRef = clusterRef;
            cluster->attributes = clusterRef->creationAttributes;
        }

        cluster->flags |= rxCLFLAGS_MODIFIED;

        if ((RwUInt32)(cluster->flags & (rxCLFLAGS_EXTERNAL | rxCLFLAGS_EXTERNALMODIFIABLE)) == rxCLFLAGS_EXTERNAL) {
            void* data = cluster->data;
            RwUInt32 numAlloced = cluster->numAlloced;
            RwUInt32 numUsed = cluster->numUsed;

            cluster->flags &= ~rxCLFLAGS_EXTERNAL;
            cluster->data = NULL;

            if (!RxClusterInitializeData(cluster, numAlloced, cluster->stride)) {
                cluster->flags |= rxCLFLAGS_EXTERNAL;
                cluster->data = data;
                return NULL;
            }

            cluster->numUsed = numUsed;
            memcpy(cluster->data, data, numUsed * cluster->stride);
        }

        cluster->currentData = cluster->data;
        return cluster;
    }
    return NULL;
}

RxPipeline* RxPipelineExecute(RxPipeline* pipeline, void* data, RwBool heapReset)
{
    if (heapReset) {
        RxHeapReset(_rxHeapGlobal);
    }

    _rxExecCtxGlobal.exitCode = TRUE;
    _rxExecCtxGlobal.pipeline = pipeline;
    _rxExecCtxGlobal.params.dataParam = data;
    _rxExecCtxGlobal.params.heap = _rxHeapGlobal;

    pipeline->embeddedPacketState = rxPKST_PACKETLESS;

    {
        RwUInt32 result = ((RwUInt32(*)(RxPipelineNode*, const RxPipelineNodeParam*))pipeline->nodes[0].nodeDef->nodeMethods.nodeBody)(
            &pipeline->nodes[0], &_rxExecCtxGlobal.params);

        if (!result) {
            _rxExecCtxGlobal.exitCode = result;
        }
    }

    if (pipeline->embeddedPacketState > rxPKST_UNUSED) {
        pipeline->embeddedPacketState = rxPKST_INUSE;
        _rxPacketDestroy(pipeline->embeddedPacket);
    }

    _rxExecCtxGlobal.params.dataParam = NULL;
    _rxExecCtxGlobal.pipeline = NULL;
    _rxExecCtxGlobal.params.heap = NULL;

    return _rxExecCtxGlobal.exitCode ? pipeline : NULL;
}

RxPipeline* RxPipelineCreate(void)
{
    RxPipeline* pipeline = (RxPipeline*)RwFreeListAlloc(RXPIPELINEGLOBAL(pipesFreeList));

    if (pipeline) {
        memset(pipeline, 0, sizeof(RxPipeline));
        pipeline->locked = FALSE;
        return pipeline;
    }

    RWERROR((E_RW_NOMEM, sizeof(RxPipeline)));
    return NULL;
}

void _rxPipelineDestroy(RxPipeline* pipeline)
{
    if (pipeline) {
        RwUInt32 numNodes;
        RwUInt32 i;
        RxPipelineNode* node;

        node = pipeline->nodes;
        numNodes = pipeline->numNodes;

        for (i = 0; i < numNodes; i++) {
            PipelineNodeDestroy(node, pipeline);
            node++;
        }
        pipeline->nodes = NULL;

        if (pipeline->superBlock) {
            RwFree(pipeline->superBlock);
            pipeline->superBlock = NULL;
            pipeline->superBlockSize = 0;
        }

        RwFreeListFree(RXPIPELINEGLOBAL(pipesFreeList), pipeline);
    }
}

/*
 * PowerPipe editing: locking a pipeline expands its nodes into an editable
 * layout inside the pipeline's super block, fragments and paths can then be
 * added, and unlocking validates, topologically sorts and compacts it.
 */

#include "rwsdk/p2core_int.h"

/* Variable arguments (the MSL definitions, without its headers). */
typedef struct __va_list_struct {
    char gpr;
    char fpr;
    char reserved[2];
    char* input_arg_area;
    char* reg_save_area;
} __va_list[1];

void* __va_arg(struct __va_list_struct* list, int type);

#define va_list __va_list
#define va_start(list, fmt) __builtin_va_info(&list)
#define va_arg(list, type) (*((type*)__va_arg(list, _var_arg_typeof(type))))
#define va_end(list) ((void)0)

#define rwPLUGIN_ID 1 /* core */

#define E_RW_INVALIDPARAMETER 0x80000016
#define E_RX_CYCLICPIPELINE 0x1C
#define E_RX_DEP_DEPENDENCIESMISMATCH 0x22
#define E_RX_INVALIDENTRYPOINT 0x24
#define E_RX_NODETOOMANYCLUSTERSOFINTEREST 0x28
#define E_RX_NODETOOMANYOUTPUTS 0x29
#define E_RX_PIPELINETOOMANYNODES 0x2A
#define E_RX_UNLOCKEDPIPE 0x34

#define rxPIPELINEMAXOUTPUTS 32
#define rxPIPELINEMAXCLUSTERSOFINTEREST 32

typedef struct rxStalacStack {
    RwUInt8* top;
    RwUInt8* bottom;
} rxStalacStack;

typedef struct rxPipelineTopSortState {
    RxPipeline* pipeline;
    RwUInt32 nodesArranged;
} rxPipelineTopSortState;

typedef RwBool (*rxNodeInitFn)(RxNodeDefinition* nodeDef);
typedef void (*rxNodeTermFn)(RxNodeDefinition* nodeDef);
typedef RwBool (*rxPipelineNodeInitFn)(RxPipelineNode* node);
typedef void (*rxPipelineNodeTermFn)(RxPipelineNode* node);
typedef RwBool (*rxPipelineNodeConfigFn)(RxPipelineNode* node, RxPipeline* pipeline);

void* memset(void* dst, int c, unsigned long n);
void* memcpy(void* dst, const void* src, unsigned long n);
RxPipelineNode* PipelineNodeDestroy(RxPipelineNode* node, RxPipeline* pipeline);
RwBool _rxChaseDependencies(RxPipeline* pipeline);
static inline RwUInt32 PipelineNodeIndex(RxPipeline* pipeline, RxPipelineNode* node)
{
    RwUInt32 index = node - pipeline->nodes;

    if (&pipeline->nodes[index] == node && index < pipeline->numNodes) {
        return index;
    }
    return (RwUInt32)-1;
}

RxPipeline* RxLockedPipeAddPath(RxLockedPipe* pipeline, RwUInt32* out, RxPipelineNode* in);

#define RwRealloc(_p, _s) ((RWSRCGLOBAL(memoryFuncs).rwrealloc)((_p), (_s)))

#pragma explicit_zero_data on
static rxStalacStack Stalac = { NULL, NULL };
#pragma explicit_zero_data reset

void* StalacTiteAlloc(RwUInt32 size)
{
    size = (size + 3) & ~3;

    Stalac.top -= size;
    if (Stalac.top < Stalac.bottom) {
        Stalac.top += size;
        RWERROR((E_RW_NOMEM));
        return NULL;
    }
    return Stalac.top;
}

void* StalacMiteAlloc(RwUInt32 size)
{
    size = (size + 3) & ~3;

    Stalac.bottom += size;
    if (Stalac.bottom > Stalac.top) {
        Stalac.bottom -= size;
        RWERROR((E_RW_NOMEM));
        return NULL;
    }
    return Stalac.bottom - size;
}

RwUInt32 PipelineCalcNumUniqueClusters(RxPipeline* pipeline)
{
    RwUInt32 prevCluster;
    RwUInt32 minCluster;
    RwUInt32 numClusters;
    RwUInt32 j;
    RwUInt32 i;
    RxNodeDefinition* nodeDef;

    numClusters = 0;
    minCluster = 0;
    for (;;) {
        prevCluster = minCluster;
        minCluster = (RwUInt32)-1;
        for (i = 0; i < pipeline->numNodes; i++) {
            nodeDef = pipeline->nodes[i].nodeDef;
            for (j = 0; j < nodeDef->io.numClustersOfInterest; j++) {
                RwUInt32 cluster = (RwUInt32)nodeDef->io.clustersOfInterest[j].clusterDef;

                if (cluster > prevCluster && cluster < minCluster) {
                    minCluster = cluster;
                }
            }
        }

        if (minCluster == (RwUInt32)-1) {
            break;
        }
        numClusters++;
    }
    return numClusters;
}

static inline void TopSortDataSet(RxPipelineNodeTopSortData* topSortData, RwUInt32 numIns, RwUInt32 numInsVisited,
                                  void* req)
{
    topSortData->numIns = numIns;
    topSortData->numInsVisited = numInsVisited;
    topSortData->req = req;
}

RwBool LockPipelineExpandData(RxPipeline* dst, RxPipeline* src)
{
    RwUInt32* outputs;
    RxPipelineNodeTopSortData* topSortData;
    RxPipelineNode* nodesEnd;
    RwInt32 i;
    RwUInt32 n;

    if (dst != src) {
        for (i = src->numNodes - 1; i >= 0; i--) {
            memcpy(&dst->nodes[i], &src->nodes[i], sizeof(RxPipelineNode));
            dst->nodes[i].slotClusterRefs = NULL;
            dst->nodes[i].slotsContinue = NULL;
            dst->nodes[i].privateData = NULL;
            dst->nodes[i].inputToClusterSlot = NULL;

            if (dst->nodes[i].initializationDataSize) {
                dst->nodes[i].initializationData = RwMalloc(dst->nodes[i].initializationDataSize);
                if (dst->nodes[i].initializationData == NULL) {
                    RWERROR((E_RW_NOMEM, dst->nodes[i].initializationDataSize));
                    return FALSE;
                }
                memcpy(dst->nodes[i].initializationData, src->nodes[i].initializationData,
                       dst->nodes[i].initializationDataSize);
            }
        }
        dst->numNodes = src->numNodes;
    }

    nodesEnd = dst->nodes + RXPIPELINEGLOBAL(maxNodesPerPipe);
    outputs = (RwUInt32*)nodesEnd + (src->numNodes - 1) * rxPIPELINEMAXOUTPUTS;
    for (i = src->numNodes - 1; i >= 0; i--) {
        dst->nodes[i].outputs = outputs;
        if (src->nodes[i].outputs) {
            memcpy(dst->nodes[i].outputs, src->nodes[i].outputs, rxPIPELINEMAXOUTPUTS * sizeof(RwUInt32));
        }
        outputs -= rxPIPELINEMAXOUTPUTS;
    }

    topSortData = (RxPipelineNodeTopSortData*)((RwUInt32*)nodesEnd +
                                               RXPIPELINEGLOBAL(maxNodesPerPipe) * rxPIPELINEMAXOUTPUTS);
    for (n = 0; n < src->numNodes; n++) {
        TopSortDataSet(topSortData, 0, 0, NULL);
        dst->nodes[n].topSortData = topSortData;
        topSortData++;
    }
    return TRUE;
}

RwBool _NodeCreate(RxPipeline* pipeline, RxPipelineNode* node, RxNodeDefinition* nodeDef)
{
    RwBool result = TRUE;
    RwUInt32 numOutputs = nodeDef->io.numOutputs;

    memset(node, 0, sizeof(RxPipelineNode));

    if (numOutputs > rxPIPELINEMAXOUTPUTS) {
        RWERROR((E_RX_NODETOOMANYOUTPUTS));
        result = FALSE;
    }
    if (nodeDef->io.numClustersOfInterest > rxPIPELINEMAXCLUSTERSOFINTEREST) {
        RWERROR((E_RX_NODETOOMANYCLUSTERSOFINTEREST));
        result = FALSE;
    }
    if (numOutputs >= RXPIPELINEGLOBAL(maxNodesPerPipe)) {
        RWERROR((E_RX_PIPELINETOOMANYNODES));
        result = FALSE;
    }

    if (result) {
        RwUInt32 i;
        RwUInt32* outputs;
        RxPipelineNodeTopSortData* topSortData;

        outputs = (RwUInt32*)(pipeline->nodes + RXPIPELINEGLOBAL(maxNodesPerPipe));
        outputs += pipeline->numNodes * rxPIPELINEMAXOUTPUTS;
        node->outputs = outputs;
        node->numOutputs = numOutputs;
        for (i = 0; i < node->numOutputs; i++) {
            *outputs++ = (RwUInt32)-1;
        }

        outputs = (RwUInt32*)(pipeline->nodes + RXPIPELINEGLOBAL(maxNodesPerPipe));
        topSortData = (RxPipelineNodeTopSortData*)(outputs + RXPIPELINEGLOBAL(maxNodesPerPipe) * rxPIPELINEMAXOUTPUTS);
        topSortData += pipeline->numNodes;
        topSortData->numIns = 0;
        topSortData->numInsVisited = 0;
        topSortData->req = NULL;
        node->topSortData = topSortData;

        node->initializationData = NULL;
        node->initializationDataSize = 0;
        node->nodeDef = nodeDef;

        pipeline->numNodes++;
    }
    return result;
}

void PipelineTopSort(rxPipelineTopSortState* state, RwUInt32 nodeIndex)
{
    RwUInt32 i;
    RwUInt32 nodesArranged = state->nodesArranged;
    RxPipelineNode* node;

    if (nodesArranged != nodeIndex) {
        RxPipelineNode* nodes;
        RwUInt32* outputsA = state->pipeline->nodes[nodesArranged].outputs;
        RwUInt32* outputsB = state->pipeline->nodes[nodeIndex].outputs;
        RxPipelineNodeTopSortData* topSortA;
        RxPipelineNodeTopSortData* topSortB;
        RxPipelineNodeTopSortData topSortTemp;
        RxPipelineNode nodeTemp;
        RwUInt32 k;

        for (k = 0; k < rxPIPELINEMAXOUTPUTS; k++) {
            RwUInt32 temp = outputsA[k];

            outputsA[k] = outputsB[k];
            outputsB[k] = temp;
        }
        state->pipeline->nodes[nodesArranged].outputs = outputsB;
        state->pipeline->nodes[nodeIndex].outputs = outputsA;

        topSortA = state->pipeline->nodes[nodesArranged].topSortData;
        topSortB = state->pipeline->nodes[nodeIndex].topSortData;
        topSortTemp = *topSortA;
        *topSortA = *topSortB;
        *topSortB = topSortTemp;
        state->pipeline->nodes[nodesArranged].topSortData = topSortB;
        state->pipeline->nodes[nodeIndex].topSortData = topSortA;

        nodes = state->pipeline->nodes;
        nodeTemp = nodes[nodesArranged];
        nodes[nodesArranged] = nodes[nodeIndex];
        state->pipeline->nodes[nodeIndex] = nodeTemp;

        for (i = 0; i < state->pipeline->numNodes; i++) {
            RwUInt32 j;
            RxPipelineNode* curNode = &state->pipeline->nodes[i];

            for (j = 0; j < curNode->numOutputs; j++) {
                if (curNode->outputs[j] == nodesArranged) {
                    curNode->outputs[j] = nodeIndex;
                } else if (curNode->outputs[j] == nodeIndex) {
                    curNode->outputs[j] = nodesArranged;
                }
            }
        }
    }

    node = &state->pipeline->nodes[state->nodesArranged];
    state->nodesArranged++;
    if (node->numOutputs) {
        for (i = 0; i < node->numOutputs; i++) {
            RwUInt32 output = node->outputs[i];

            if (output != (RwUInt32)-1) {
                RxPipelineNode* outNode = &state->pipeline->nodes[output];

                outNode->topSortData->numInsVisited++;
                if (outNode->topSortData->numIns == outNode->topSortData->numInsVisited) {
                    PipelineTopSort(state, output);
                }
            }
        }
    }
}

RwUInt32* RxPipelineNodeFindOutputByName(RxPipelineNode* node, const RwChar* outputName)
{
    if (node && node->nodeDef && outputName) {
        RwInt32 i;
        RwInt32 numOutputs = node->nodeDef->io.numOutputs;
        RxOutputSpec* outputs = node->nodeDef->io.outputs;

        for (i = 0; i < numOutputs; i++, outputs++) {
            if (!RWSRCGLOBAL(stringFuncs).rwstrcmp(outputs->name, outputName)) {
                return &node->outputs[i];
            }
        }
    }
    return NULL;
}

static inline RwUInt32* RxPipelineNodeFindOutputByIndex(RxPipelineNode* node, RwUInt32 outputIndex)
{
    if (node && node->nodeDef && outputIndex < node->numOutputs) {
        return &node->outputs[outputIndex];
    }
    return NULL;
}

RxPipelineNode* RxPipelineNodeFindInput(RxPipelineNode* node)
{
    if (node && node->nodeDef) {
        return node;
    }
    return NULL;
}

static inline RwBool PipelineSuperBlockResize(RxPipeline* pipeline, RwUInt32 size)
{
    void* oldBlock = pipeline->superBlock;
    void* newBlock = RwRealloc(oldBlock, size);

    if (newBlock) {
        RwUInt32 numNodes = pipeline->numNodes;
        RwInt32 delta = (RwUInt8*)newBlock - (RwUInt8*)oldBlock;
        RwUInt32 i;

        pipeline->superBlock = newBlock;
        pipeline->superBlockSize = size;
        pipeline->nodes = (RxPipelineNode*)pipeline->superBlock;

        if (pipeline->embeddedPacket) {
            pipeline->embeddedPacket = (RxPacket*)((RwUInt8*)pipeline->embeddedPacket + delta);
        }
        if (pipeline->inputRequirements) {
            pipeline->inputRequirements = (RwUInt8*)pipeline->inputRequirements + delta;
        }

        for (i = 0; i < numNodes; i++) {
            if (pipeline->nodes[i].outputs) {
                pipeline->nodes[i].outputs = (RwUInt32*)((RwUInt8*)pipeline->nodes[i].outputs + delta);
            }
            if (pipeline->nodes[i].slotClusterRefs) {
                pipeline->nodes[i].slotClusterRefs =
                    (RxPipelineCluster**)((RwUInt8*)pipeline->nodes[i].slotClusterRefs + delta);
            }
            if (pipeline->nodes[i].slotsContinue) {
                pipeline->nodes[i].slotsContinue = (RwUInt32*)((RwUInt8*)pipeline->nodes[i].slotsContinue + delta);
            }
            if (pipeline->nodes[i].privateData) {
                pipeline->nodes[i].privateData = (RwUInt8*)pipeline->nodes[i].privateData + delta;
            }
            if (pipeline->nodes[i].inputToClusterSlot) {
                pipeline->nodes[i].inputToClusterSlot =
                    (RwUInt32*)((RwUInt8*)pipeline->nodes[i].inputToClusterSlot + delta);
            }
            if (pipeline->nodes[i].topSortData) {
                pipeline->nodes[i].topSortData =
                    (RxPipelineNodeTopSortData*)((RwUInt8*)pipeline->nodes[i].topSortData + delta);
            }
        }
    } else {
        RWERROR((E_RW_NOMEM, size));
        return FALSE;
    }
    return TRUE;
}

RxPipeline* RxLockedPipeUnlock(RxLockedPipe* pipeline)
{
    if (pipeline && pipeline->locked) {
        if (pipeline->numNodes) {
            RwUInt32 numOutputsTotal = 0;
            RwBool failed = FALSE;
            RwUInt32 numFailed = 0;
            RwUInt32 numNodes = pipeline->numNodes;
            RxPipelineNode* nodes;
            RwUInt32 numUniqueClusters;
            RwUInt32 nodesAreaSize;
            RwUInt32 dataSize;
            RwUInt32 extraSize;
            RwUInt32 size;
            RwUInt32 i;
            RwInt32 j;
            rxPipelineTopSortState state;

            if (pipeline->entryPoint >= numNodes || pipeline->nodes[pipeline->entryPoint].nodeDef == NULL) {
                RWERROR((E_RX_INVALIDENTRYPOINT));
                return NULL;
            }

            nodes = pipeline->nodes;
            numUniqueClusters = PipelineCalcNumUniqueClusters(pipeline);

            nodesAreaSize = RXPIPELINEGLOBAL(maxNodesPerPipe) *
                            (sizeof(RxPipelineNode) + rxPIPELINEMAXOUTPUTS * sizeof(RwUInt32) +
                             sizeof(RxPipelineNodeTopSortData));

            dataSize = numNodes * sizeof(RxPipelineNode);
            for (i = 0; i < numNodes; i++) {
                dataSize += nodes->numOutputs * sizeof(RwUInt32);
            }
            dataSize += RXPIPELINEGLOBAL(maxNodesPerPipe) * sizeof(RxPipelineNodeTopSortData);
            dataSize += numNodes * numUniqueClusters * (0x24 + 0x10) + numNodes * 0x14;

            extraSize = numUniqueClusters * 8 + numUniqueClusters * 0xC + numNodes * numUniqueClusters * 4 +
                        numNodes * (numUniqueClusters + 1) * 4;
            for (i = 0; i < numNodes; i++) {
                RxNodeDefinition* nodeDef = nodes[i].nodeDef;

                if (nodeDef->pipelineNodePrivateDataSize) {
                    extraSize += nodeDef->pipelineNodePrivateDataSize;
                }
                extraSize += nodeDef->io.numClustersOfInterest * sizeof(RwUInt32);
            }
            extraSize += (numUniqueClusters - 1) * sizeof(RxCluster) + sizeof(RxPacket) + 0x14;
            dataSize += extraSize;

            size = nodesAreaSize;
            if (dataSize > nodesAreaSize) {
                size = dataSize;
            }

            if (size > pipeline->superBlockSize) {
                if (!PipelineSuperBlockResize(pipeline, size)) {
                    return NULL;
                }
            }

            Stalac.bottom = NULL;
            Stalac.top = (RwUInt8*)pipeline->superBlock + size;

            state.pipeline = pipeline;
            state.nodesArranged = 0;

            for (i = 0; i < pipeline->numNodes; i++) {
                if (pipeline->nodes[i].nodeDef) {
                    pipeline->nodes[i].topSortData->numInsVisited = 0;
                    pipeline->nodes[i].topSortData->numIns = 0;
                }
            }

            for (i = 0; i < pipeline->numNodes; i++) {
                RxPipelineNode* node = &pipeline->nodes[i];

                if (node->nodeDef && node->numOutputs) {
                    RwUInt32 count = node->numOutputs;
                    RwUInt32* out = node->outputs;

                    do {
                        if (*out != (RwUInt32)-1) {
                            pipeline->nodes[*out].topSortData->numIns++;
                        }
                        out++;
                    } while (--count);
                }
            }

            if (pipeline->nodes[pipeline->entryPoint].topSortData->numIns != 0) {
                RWERROR((E_RX_INVALIDENTRYPOINT));
                pipeline = NULL;
            } else {
                for (i = 0; i < pipeline->numNodes; i++) {
                    if (i != pipeline->entryPoint && pipeline->nodes[i].topSortData->numIns == 0) {
                        RWERROR((E_RX_DEP_DEPENDENCIESMISMATCH));
                        pipeline = NULL;
                        break;
                    }
                }

                if (pipeline) {
                    PipelineTopSort(&state, pipeline->entryPoint);

                    for (i = 0; i < pipeline->numNodes; i++) {
                        if (pipeline->nodes[i].topSortData->numIns != pipeline->nodes[i].topSortData->numInsVisited) {
                            RWERROR((E_RX_CYCLICPIPELINE));
                            pipeline = NULL;
                            break;
                        }
                    }

                    if (pipeline) {
                        pipeline->entryPoint = 0;
                    }
                }
            }

            if (pipeline == NULL) {
                return NULL;
            }

            {
                RxPipelineNode* nodesEnd =
                    pipeline->nodes + RXPIPELINEGLOBAL(maxNodesPerPipe);
                RxPipelineNodeTopSortData* src = (RxPipelineNodeTopSortData*)((RwUInt32*)nodesEnd +
                                                                              RXPIPELINEGLOBAL(maxNodesPerPipe) *
                                                                                  rxPIPELINEMAXOUTPUTS) +
                                                 (pipeline->numNodes - 1);
                RxPipelineNodeTopSortData* dst =
                    (RxPipelineNodeTopSortData*)((RwUInt8*)pipeline->superBlock + size) - 1;
                RwUInt32* outputsSrc = (RwUInt32*)nodesEnd;
                RwUInt32* outputsDst;

                for (j = pipeline->numNodes - 1; j >= 0; j--) {
                    memcpy(dst, src, sizeof(RxPipelineNodeTopSortData));
                    pipeline->nodes[j].topSortData = dst;
                    src--;
                    dst--;
                }

                outputsDst = (RwUInt32*)(pipeline->nodes + pipeline->numNodes);
                for (i = 0; i < pipeline->numNodes; i++) {
                    if (pipeline->nodes[i].numOutputs == 0) {
                        pipeline->nodes[i].outputs = NULL;
                    } else {
                        memcpy(outputsDst, outputsSrc, pipeline->nodes[i].numOutputs * sizeof(RwUInt32));
                        pipeline->nodes[i].outputs = outputsDst;
                    }
                    numOutputsTotal += pipeline->nodes[i].numOutputs;
                    outputsSrc += rxPIPELINEMAXOUTPUTS;
                    outputsDst += pipeline->nodes[i].numOutputs;
                }

                Stalac.top = (RwUInt8*)dst;
                Stalac.bottom = (RwUInt8*)(outputsSrc - rxPIPELINEMAXOUTPUTS) + numOutputsTotal * sizeof(RwUInt32);
            }

            if (_rxChaseDependencies(pipeline)) {
                return NULL;
            }

            if (PipelineSuperBlockResize(pipeline, Stalac.bottom - (RwUInt8*)pipeline->superBlock)) {
                for (i = 0; i < pipeline->numNodes; i++) {
                    pipeline->nodes[i].topSortData = NULL;
                }

                for (j = pipeline->numNodes - 1; j >= 0; j--) {
                    RxPipelineNode* node = &pipeline->nodes[j];
                    RxNodeDefinition* nodeDef = node->nodeDef;

                    if (nodeDef->InputPipesCnt++ == 0 && nodeDef->nodeMethods.nodeInit &&
                        !((rxNodeInitFn)nodeDef->nodeMethods.nodeInit)(nodeDef)) {
                        failed = TRUE;
                        numFailed = pipeline->numNodes - 1 - j;
                        break;
                    }

                    if (nodeDef->nodeMethods.pipelineNodeInit &&
                        !((rxPipelineNodeInitFn)nodeDef->nodeMethods.pipelineNodeInit)(node)) {
                        nodeDef->InputPipesCnt--;
                        if (nodeDef->InputPipesCnt == 0 && nodeDef->nodeMethods.nodeTerm) {
                            ((rxNodeTermFn)nodeDef->nodeMethods.nodeTerm)(nodeDef);
                        }
                        failed = TRUE;
                        numFailed = pipeline->numNodes - 1 - j;
                        break;
                    }
                }

                if (!failed) {
                    for (j = pipeline->numNodes - 1; j >= 0; j--) {
                        RxPipelineNode* node = &pipeline->nodes[j];

                        if (node->nodeDef->nodeMethods.pipelineNodeConfig &&
                            !((rxPipelineNodeConfigFn)node->nodeDef->nodeMethods.pipelineNodeConfig)(node, pipeline)) {
                            failed = TRUE;
                            numFailed = pipeline->numNodes;
                            break;
                        }
                    }
                }

                if (failed) {
                    for (i = pipeline->numNodes - numFailed; i < pipeline->numNodes; i++) {
                        RxNodeDefinition* nodeDef = pipeline->nodes[i].nodeDef;

                        if (nodeDef->nodeMethods.pipelineNodeTerm) {
                            ((rxPipelineNodeTermFn)nodeDef->nodeMethods.pipelineNodeTerm)(&pipeline->nodes[i]);
                        }
                        nodeDef->InputPipesCnt--;
                        if (nodeDef->InputPipesCnt == 0 && nodeDef->nodeMethods.nodeTerm) {
                            ((rxNodeTermFn)nodeDef->nodeMethods.nodeTerm)(nodeDef);
                        }
                    }
                } else {
                    pipeline->locked = FALSE;
                    return pipeline;
                }
            }

            LockPipelineExpandData(pipeline, pipeline);
            return NULL;
        }

        pipeline->locked = FALSE;
        return pipeline;
    }

    if (pipeline == NULL) {
        RWERROR((E_RW_INVALIDPARAMETER));
    } else {
        RWERROR((E_RX_UNLOCKEDPIPE));
    }
    return NULL;
}

RxPipeline* RxPipelineLock(RxPipeline* pipeline)
{
    RwUInt32 size;
    RxNodeDefinition* nodeDef;
    RwUInt32 i;

    if (!pipeline->locked) {
        size = RXPIPELINEGLOBAL(maxNodesPerPipe) * sizeof(RxPipelineNode) +
                        RXPIPELINEGLOBAL(maxNodesPerPipe) * rxPIPELINEMAXOUTPUTS * sizeof(RwUInt32) +
                        RXPIPELINEGLOBAL(maxNodesPerPipe) * sizeof(RxPipelineNodeTopSortData);

        if (pipeline->nodes) {
            if (size > pipeline->superBlockSize) {
                if (!PipelineSuperBlockResize(pipeline, size)) {
                    return NULL;
                }
            }
            if (!LockPipelineExpandData(pipeline, pipeline)) {
                return NULL;
            }
        } else {
            pipeline->superBlock = RwMalloc(size);
            if (pipeline->superBlock == NULL) {
                RWERROR((E_RW_NOMEM, size));
                return NULL;
            }
            pipeline->superBlockSize = size;
            pipeline->nodes = (RxPipelineNode*)pipeline->superBlock;
        }

        pipeline->locked = TRUE;

        if (pipeline->nodes) {
            for (i = 0; i < pipeline->numNodes; i++) {
                nodeDef = pipeline->nodes[i].nodeDef;

                if (nodeDef->nodeMethods.pipelineNodeTerm) {
                    ((rxPipelineNodeTermFn)nodeDef->nodeMethods.pipelineNodeTerm)(&pipeline->nodes[i]);
                }
                if (--pipeline->nodes[i].nodeDef->InputPipesCnt == 0 && nodeDef->nodeMethods.nodeTerm) {
                    ((rxNodeTermFn)nodeDef->nodeMethods.nodeTerm)(pipeline->nodes[i].nodeDef);
                }
                pipeline->nodes[i].slotClusterRefs = NULL;
            }
        }
    }
    return pipeline;
}

RxPipelineNode* RxPipelineFindNodeByName(RxPipeline* pipeline, const RwChar* name, RxPipelineNode* start,
                                         RwInt32* nodeIndex)
{
    RwBool valid = FALSE;

    if (pipeline && name && pipeline->numNodes) {
        valid = TRUE;
    }

    if (valid) {
        RxPipelineNode* node = pipeline->nodes;
        RwInt32 remaining = pipeline->numNodes;

        if (start) {
            while (node != start && remaining > 0) {
                node++;
                remaining--;
            }
            node++;
            remaining--;
        }

        while (remaining > 0) {
            if (node->nodeDef && !RWSRCGLOBAL(stringFuncs).rwstrcmp(node->nodeDef->name, name)) {
                if (nodeIndex) {
                    *nodeIndex = remaining;
                }
                return node;
            }
            node++;
            remaining--;
        }
    }

    if (nodeIndex) {
        *nodeIndex = -1;
    }
    return NULL;
}

RxLockedPipe* RxLockedPipeAddFragment(RxLockedPipe* pipeline, RwUInt32* firstIndex, RxNodeDefinition* nodeDef0, ...)
{
    if (pipeline && pipeline->locked) {
        va_list va;
        RxNodeDefinition* nodeDef;
        RwUInt32 numNodes = 0;

        va_start(va, nodeDef0);
        for (nodeDef = nodeDef0; nodeDef; nodeDef = va_arg(va, RxNodeDefinition*)) {
            numNodes++;
        }
        va_end(va);

        if (numNodes) {
            RxPipelineNode* prev = NULL;
            RwUInt32 base = pipeline->numNodes;
            RwUInt32 n;

            if (base + numNodes > RXPIPELINEGLOBAL(maxNodesPerPipe)) {
                RWERROR((E_RX_PIPELINETOOMANYNODES));
                return NULL;
            }

            va_start(va, nodeDef0);
            for (n = 0, nodeDef = nodeDef0; nodeDef; n++, nodeDef = va_arg(va, RxNodeDefinition*)) {
                RxPipelineNode* node = &pipeline->nodes[base + n];

                if (!_NodeCreate(pipeline, node, nodeDef)) {
                    break;
                }

                if (prev) {
                    if (!RxLockedPipeAddPath(pipeline, RxPipelineNodeFindOutputByIndex(prev, 0),
                                             RxPipelineNodeFindInput(node))) {
                        PipelineNodeDestroy(node, pipeline);
                        break;
                    }
                }
                prev = node;
            }
            va_end(va);

            if (n == numNodes) {
                if (firstIndex) {
                    *firstIndex = base;
                }
                return pipeline;
            }

            while (n--) {
                PipelineNodeDestroy(&pipeline->nodes[base + n], pipeline);
            }
        }
        return NULL;
    }

    if (pipeline == NULL) {
        RWERROR((E_RW_INVALIDPARAMETER));
    } else {
        RWERROR((E_RX_UNLOCKEDPIPE));
    }
    return NULL;
}

RxPipeline* RxLockedPipeAddPath(RxLockedPipe* pipeline, RwUInt32* out, RxPipelineNode* in)
{
    if (pipeline && pipeline->locked && out && *out == (RwUInt32)-1 && in && in->nodeDef) {
        RwUInt32 inIndex = PipelineNodeIndex(pipeline, in);

        if (inIndex != (RwUInt32)-1) {
            *out = inIndex;
            return pipeline;
        }
    }
    return NULL;
}

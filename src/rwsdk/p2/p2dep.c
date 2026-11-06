/*
 * PowerPipe dependency chasing, run when a pipeline is unlocked: each node's
 * cluster requirements are propagated back up the pipeline, clusters that
 * nothing downstream wants are dropped, clusters whose lifetimes overlap are
 * grouped into scopes, scopes are packed into packet cluster slots, and the
 * per-node slot tables are written into the pipeline's super block.
 */

#include "rwsdk/p2core_int.h"

#define rwPLUGIN_ID 1 /* core */

#define E_RX_DEP_DEPENDENCIESMISMATCH 0x1D
#define E_RX_DEP_DUPLICATECLUSTERDEFS 0x1E
#define E_RX_DEP_NULLCLUSTERDEF 0x1F
#define E_RX_DEP_OUTOFMEMORY 0x20

typedef struct rxReq rxReq;
typedef struct rxReqEntry rxReqEntry;
typedef struct rxScope rxScope;

/* A node's cluster requirements. */
struct rxReq {
    /* 0x00 */ RwUInt32 numEntries;
    /* 0x04 */ RwUInt32 maxEntries;
    /* 0x08 */ RwUInt32 slotsUsed;
    /* 0x0C */ rxReqEntry* entries;
    /* 0x10 */ RxPipelineNode* node;
};

struct rxReqEntry {
    /* 0x00 */ RxClusterDefinition* clusterDef;
    /* 0x04 */ RxClusterValidityReq rqdOrOpt;
    /* 0x08 */ RwUInt32 numIns;
    /* 0x0C */ rxScope* scope;
    /* 0x10 */ rxReqEntry* nextInScope;
    /* 0x14 */ rxReq* req;
    /* 0x18 */ RwUInt32 continuity;
    /* 0x1C */ RwUInt32 slot;
    /* 0x20 */ RxPipelineNode* source;
};

/* Requirement entries that share one cluster's lifetime. */
struct rxScope {
    /* 0x00 */ rxReqEntry* entries;
    /* 0x04 */ rxScope* sibling;
    /* 0x08 */ rxScope* next;
    /* 0x0C */ rxScope* parent;
};

typedef struct RxPipelineRequiresCluster {
    /* 0x00 */ RxClusterDefinition* clusterDef;
    /* 0x04 */ RxClusterValidityReq rqdOrOpt;
    /* 0x08 */ RwUInt32 slotIndex;
} RxPipelineRequiresCluster;

struct RxClusterDefinition {
    /* 0x00 */ RwChar* name;
    /* 0x04 */ RwUInt32 defaultStride;
    /* 0x08 */ RwUInt32 defaultAttributes;
    /* 0x0C */ const RwChar* attributeSet;
};

typedef void (*rxPipelineClusterCallBack)(RxClusterDefinition* clusterDef, RwUInt32 index, void* data);

void* memset(void* dst, int c, unsigned long n);
void* StalacTiteAlloc(RwUInt32 size);
void* StalacMiteAlloc(RwUInt32 size);
RwUInt32 PipelineCalcNumUniqueClusters(RxPipeline* pipeline);
void _rx_rxRadixExchangeSort(void* array, RwUInt32 numElements, RwUInt32 elementSize, RwUInt32 keyOffset,
                             RwUInt32 keyBit, RwUInt32 keyMask);

#define NODEREQ(node) ((rxReq*)(node)->topSortData->req)
#define REQENTRY(req, i) (((i) < (req)->numEntries) ? &(req)->entries[i] : NULL)

rxReqEntry* _ReqSearch4Cluster(rxReq* req, RxClusterDefinition* clusterDef)
{
    rxReqEntry* entry;
    RwUInt32 i;

    if (req->numEntries) {
        entry = req->entries;
        for (i = 0; i < req->numEntries; i++) {
            if (entry->clusterDef == clusterDef) {
                return entry;
            }
            entry++;
        }
    }
    return NULL;
}

void _ReqDeleteEntry(rxReq* req, rxReqEntry* entry)
{
    if (entry != &req->entries[req->numEntries - 1]) {
        *entry = req->entries[req->numEntries - 1];
    }
    req->numEntries--;
}

RwInt32 _IoSpecSearch4Cluster(RxIoSpec* io, RxClusterDefinition* clusterDef)
{
    RwUInt32 i;

    for (i = 0; i < io->numClustersOfInterest; i++) {
        if (io->clustersOfInterest[i].clusterDef == clusterDef) {
            return i;
        }
    }
    return -1;
}

static inline rxReqEntry* ReqAddEntry(rxReq* req, RxClusterDefinition* clusterDef, RxClusterValidityReq rqdOrOpt,
                                      RwUInt32 numIns, RxPipelineNode* source)
{
    rxReqEntry* entry = &req->entries[req->numEntries++];

    entry->clusterDef = clusterDef;
    entry->rqdOrOpt = rqdOrOpt;
    entry->numIns = numIns;
    entry->nextInScope = NULL;
    entry->req = req;
    entry->scope = NULL;
    entry->continuity = 0;
    entry->slot = (RwUInt32)-1;
    entry->source = source;
    return entry;
}

#define OUTPUTCLUSTERVALID(_valid, _nodeDef, _output, _clusterDef)                                           do {                                                                                                       RwInt32 _index = _IoSpecSearch4Cluster(&(_nodeDef)->io, (_clusterDef));                                                                                                                                       (_valid) = (_index == -1) ? (_output)->allOtherClusters : (_output)->outputClusters[_index];        } while (0)

/* Drops a cluster from a node and from everything downstream it reached unchanged. */
static void _PropDownElimPath(RxPipeline* pipeline, RxPipelineNode* node, RxClusterDefinition* clusterDef)
{
    RxNodeDefinition* nodeDef = node->nodeDef;
    rxReqEntry* entry = _ReqSearch4Cluster(NODEREQ(node), clusterDef);

    if (entry && --entry->numIns == 0) {
        RwUInt32 i;

        _ReqDeleteEntry(NODEREQ(node), entry);
        for (i = 0; i < node->numOutputs; i++) {
            if (node->outputs[i] != (RwUInt32)-1) {
                RxClusterValid valid;

                OUTPUTCLUSTERVALID(valid, nodeDef, &node->nodeDef->io.outputs[i], clusterDef);
                if (valid == rxCLVALID_NOCHANGE) {
                    _PropDownElimPath(pipeline, &pipeline->nodes[node->outputs[i]], clusterDef);
                }
            }
        }
    }
}

static RwUInt32 _PropagateDependenciesAndKillDeadPaths(RxPipeline* pipeline)
{
    RwUInt32 numNodes = pipeline->numNodes;
    RxPipelineNode* node = &pipeline->nodes[numNodes - 1];

    do {
        RxNodeDefinition* nodeDef = node->nodeDef;
        RwUInt32 numUniqueClusters;
        rxReq* req;
        RwUInt32 i;
        RwUInt32 j;

        for (i = 0; i < nodeDef->io.numClustersOfInterest; i++) {
            RxClusterDefinition* clusterDef = nodeDef->io.clustersOfInterest[i].clusterDef;

            if (!clusterDef) {
                RWERROR((E_RX_DEP_NULLCLUSTERDEF, node->nodeDef->name));
                return E_RX_DEP_NULLCLUSTERDEF;
            }
            for (j = i + 1; j < nodeDef->io.numClustersOfInterest; j++) {
                if (clusterDef == nodeDef->io.clustersOfInterest[j].clusterDef) {
                    RWERROR((E_RX_DEP_DUPLICATECLUSTERDEFS, node->nodeDef->name,
                             nodeDef->io.clustersOfInterest[j].clusterDef->name));
                    return E_RX_DEP_DUPLICATECLUSTERDEFS;
                }
            }
        }

        numUniqueClusters = PipelineCalcNumUniqueClusters(pipeline);
        req = (rxReq*)StalacTiteAlloc(sizeof(rxReq));
        if (req) {
            req->entries = (rxReqEntry*)StalacTiteAlloc(numUniqueClusters * sizeof(rxReqEntry));
            if (req->entries) {
                req->maxEntries = numUniqueClusters;
                req->numEntries = 0;
                req->node = node;
                req->slotsUsed = 0;
            } else {
                req = NULL;
            }
        }
        node->topSortData->req = req;
        if (!req) {
            RWERROR((E_RX_DEP_OUTOFMEMORY));
            return E_RX_DEP_OUTOFMEMORY;
        }

        for (i = 0; i < node->numOutputs; i++) {
            if (node->outputs[i] != (RwUInt32)-1) {
                RxPipelineNode* next = &pipeline->nodes[node->outputs[i]];
                RxOutputSpec* output = &node->nodeDef->io.outputs[i];

                for (j = 0; j < NODEREQ(next)->numEntries; j++) {
                    rxReqEntry* nextEntry = REQENTRY(NODEREQ(next), j);
                    RxClusterDefinition* clusterDef = nextEntry->clusterDef;
                    RxClusterValid valid;

                    OUTPUTCLUSTERVALID(valid, nodeDef, output, clusterDef);

                    if (valid == rxCLVALID_NOCHANGE) {
                        rxReqEntry* entry = _ReqSearch4Cluster(NODEREQ(node), clusterDef);

                        if (entry) {
                            if (nextEntry->rqdOrOpt == rxCLREQ_REQUIRED) {
                                entry->rqdOrOpt = rxCLREQ_REQUIRED;
                                entry->source = nextEntry->source;
                            }
                        } else {
                            entry = ReqAddEntry(NODEREQ(node), clusterDef, nextEntry->rqdOrOpt,
                                                node->topSortData->numIns, nextEntry->source);
                        }
                        if (!entry) {
                            RWERROR((E_RX_DEP_OUTOFMEMORY));
                            return E_RX_DEP_OUTOFMEMORY;
                        }
                    } else if (nextEntry->rqdOrOpt == rxCLREQ_REQUIRED) {
                        if (valid != rxCLVALID_VALID) {
                            RWERROR((E_RX_DEP_DEPENDENCIESMISMATCH, nextEntry->clusterDef->name,
                                     nextEntry->source->nodeDef->name, node->nodeDef->name, i, output->name));
                            return E_RX_DEP_DEPENDENCIESMISMATCH;
                        }
                    } else if (valid == rxCLVALID_INVALID) {
                        _PropDownElimPath(pipeline, next, clusterDef);
                    }
                }
            }
        }

        for (i = 0; i < nodeDef->io.numClustersOfInterest; i++) {
            RxClusterValidityReq rqdOrOpt = nodeDef->io.inputRequirements[i];

            if (rqdOrOpt != rxCLREQ_DONTWANT) {
                RxClusterDefinition* clusterDef = nodeDef->io.clustersOfInterest[i].clusterDef;
                rxReqEntry* entry = _ReqSearch4Cluster(NODEREQ(node), clusterDef);

                if (entry) {
                    if (rqdOrOpt == rxCLREQ_REQUIRED) {
                        entry->rqdOrOpt = rxCLREQ_REQUIRED;
                        entry->source = node;
                    }
                } else {
                    entry = ReqAddEntry(NODEREQ(node), clusterDef, rqdOrOpt, node->topSortData->numIns, node);
                }
                if (!entry) {
                    RWERROR((E_RX_DEP_OUTOFMEMORY));
                    return E_RX_DEP_OUTOFMEMORY;
                }
            }
        }

        node--;
    } while (--numNodes);

    return 0;
}

static RwUInt32 _ForAllNodeReqsAddOutputClustersAndBuildContinuityBitfields(RxPipeline* pipeline)
{
    RwUInt32 numNodes = pipeline->numNodes;
    RxPipelineNode* node = pipeline->nodes;

    do {
        RxNodeDefinition* nodeDef = node->nodeDef;
        RwUInt32 i;
        RwUInt32 j;

        for (i = 0; i < nodeDef->io.numClustersOfInterest; i++) {
            RxClusterRef* ref = &nodeDef->io.clustersOfInterest[i];

            if (ref->forcePresent != rxCLALLOWABSENT) {
                RxClusterDefinition* clusterDef = ref->clusterDef;
                rxReqEntry* entry = _ReqSearch4Cluster(NODEREQ(node), clusterDef);

                if (!entry) {
                    entry = ReqAddEntry(NODEREQ(node), clusterDef, rxCLREQ_DONTWANT, 1, node);
                }
                if (!entry) {
                    RWERROR((E_RX_DEP_OUTOFMEMORY));
                    return E_RX_DEP_OUTOFMEMORY;
                }
            }
        }

        for (i = 0; i < node->numOutputs; i++) {
            if (node->outputs[i] != (RwUInt32)-1) {
                RwUInt32 bit = 1 << i;
                RxPipelineNode* next = &pipeline->nodes[node->outputs[i]];
                RxOutputSpec* output = &node->nodeDef->io.outputs[i];

                for (j = 0; j < NODEREQ(next)->numEntries; j++) {
                    RxClusterDefinition* clusterDef = REQENTRY(NODEREQ(next), j)->clusterDef;
                    RxClusterValid valid;

                    OUTPUTCLUSTERVALID(valid, nodeDef, output, clusterDef);

                    if (valid != rxCLVALID_INVALID) {
                        rxReqEntry* entry;

                        if (valid != rxCLVALID_NOCHANGE) {
                            entry = _ReqSearch4Cluster(NODEREQ(node), clusterDef);
                            if (!entry) {
                                entry = ReqAddEntry(NODEREQ(node), clusterDef, rxCLREQ_DONTWANT, 1, node);
                            }
                            if (!entry) {
                                RWERROR((E_RX_DEP_OUTOFMEMORY));
                                return E_RX_DEP_OUTOFMEMORY;
                            }
                        } else {
                            entry = _ReqSearch4Cluster(NODEREQ(node), clusterDef);
                        }
                        if (entry) {
                            entry->continuity |= bit;
                        }
                    }
                }
            }
        }

        node++;
    } while (--numNodes);

    return 0;
}

static inline rxScope* ScopeCreate(rxScope** scopes)
{
    rxScope* scope = (rxScope*)StalacTiteAlloc(sizeof(rxScope));

    if (scope) {
        scope->sibling = NULL;
        scope->entries = NULL;
        scope->parent = NULL;
        scope->next = *scopes;
        *scopes = scope;
    }
    return scope;
}

static RwUInt32 _TraceClusterScopes(RxPipeline* pipeline, rxScope** scopes)
{
    RwUInt32 numNodes = pipeline->numNodes;
    RxPipelineNode* node = pipeline->nodes;

    do {
        RwUInt32 i;
        RwUInt32 j;

        for (i = 0; i < NODEREQ(node)->numEntries; i++) {
            rxReqEntry* entry = REQENTRY(NODEREQ(node), i);

            if (!entry->scope) {
                entry->scope = ScopeCreate(scopes);
                if (!entry->scope) {
                    RWERROR((E_RX_DEP_OUTOFMEMORY));
                    return E_RX_DEP_OUTOFMEMORY;
                }
                entry->nextInScope = entry->scope->entries;
                entry->scope->entries = entry;
            }
        }

        for (i = 0; i < node->numOutputs; i++) {
            if (node->outputs[i] != (RwUInt32)-1) {
                RxPipelineNode* next = &pipeline->nodes[node->outputs[i]];
                RwUInt32 bit = 1 << i;

                for (j = 0; j < NODEREQ(next)->numEntries; j++) {
                    rxReqEntry* nextEntry = REQENTRY(NODEREQ(next), j);

                    if (nextEntry->rqdOrOpt != rxCLREQ_DONTWANT) {
                        rxReqEntry* entry = _ReqSearch4Cluster(NODEREQ(node), nextEntry->clusterDef);

                        if (entry && (entry->continuity & bit)) {
                            if (!nextEntry->scope) {
                                nextEntry->nextInScope = entry->scope->entries;
                                entry->scope->entries = nextEntry;
                                nextEntry->scope = entry->scope;
                            } else {
                                rxScope* scopeA = nextEntry->scope;
                                rxScope* scopeB = entry->scope;
                                rxScope** link = scopes;

                                while (scopeA->parent) {
                                    scopeA = scopeA->parent;
                                }
                                while (scopeB->parent) {
                                    scopeB = scopeB->parent;
                                }
                                if (scopeA != scopeB) {
                                    rxScope* last = scopeA;

                                    while (last->sibling) {
                                        last = last->sibling;
                                    }
                                    last->sibling = scopeB;
                                    scopeB->parent = scopeA;
                                    while (*link != scopeB) {
                                        link = &(*link)->next;
                                    }
                                    *link = scopeB->next;
                                }
                            }
                        }
                    }
                }
            }
        }

        node++;
    } while (--numNodes);

    return 0;
}

static RwUInt32 _AssignClusterSlots(RxPipeline* pipeline, rxScope** scopes)
{
    RwUInt32 numSlots = 0;
    rxScope* scope;

    for (scope = *scopes; scope; scope = scope->next) {
        rxScope* sub;
        RwUInt32 used = 0;
        RwUInt32 slot;
        rxReqEntry* entry;
        RwUInt32 bit;

        sub = scope;
        do {
            for (entry = sub->entries; entry; entry = entry->nextInScope) {
                used |= entry->req->slotsUsed;
            }
        } while ((sub = sub->sibling));

        slot = 0;
        while (used & 1) {
            used >>= 1;
            slot++;
        }
        if (slot >= numSlots) {
            numSlots = slot + 1;
        }

        bit = 1 << slot;
        sub = scope;
        do {
            for (entry = sub->entries; entry; entry = entry->nextInScope) {
                entry->slot = slot;
                entry->req->slotsUsed |= bit;
            }
        } while ((sub = sub->sibling));
    }

    pipeline->packetNumClusterSlots = numSlots;
    return 0;
}

static void _MyEnumPipelineClustersCallBack(RxClusterDefinition* clusterDef, RwUInt32 index, void* data)
{
    RxPipelineCluster* cluster = (RxPipelineCluster*)StalacMiteAlloc(sizeof(RxPipelineCluster));

    if (!*(RxPipelineCluster**)data) {
        *(RxPipelineCluster**)data = cluster;
    }
    cluster->clusterRef = clusterDef;
    cluster->creationAttributes = clusterDef->defaultAttributes;
}

/* Calls back once for each distinct cluster among the scopes. */
static inline void ScopesForAllUniqueClusters(rxScope* scopes, rxPipelineClusterCallBack callBack, void* data)
{
    rxScope* scope;

    for (scope = scopes; scope; scope = scope->next) {
        rxScope* prev;

        for (prev = scopes; prev != scope; prev = prev->next) {
            if (prev->entries->clusterDef == scope->entries->clusterDef) {
                break;
            }
        }
        if (prev == scope && callBack) {
            callBack(scope->entries->clusterDef, 0, data);
        }
    }
}

static RwUInt32 _ForAllNodesWriteClusterAllocations(RxPipeline* pipeline, rxScope* scopes)
{
    RwUInt32 numInputRequirements = 0;
    RwUInt32 numUniqueClusters;
    RxPipelineCluster* clusters;
    rxReq* req;
    rxScope* scope;
    RwUInt32 i;
    RwUInt8* top;

    req = NODEREQ(&pipeline->nodes[0]);
    for (i = 0; i < req->numEntries; i++) {
        rxReqEntry* entry = REQENTRY(req, i);

        if (entry->rqdOrOpt == rxCLREQ_REQUIRED || entry->rqdOrOpt == rxCLREQ_OPTIONAL) {
            numInputRequirements++;
        }
    }

    numUniqueClusters = 0;
    for (scope = scopes; scope; scope = scope->next) {
        rxScope* prev;

        for (prev = scopes; scope != prev; prev = prev->next) {
            if (scope->entries->clusterDef == prev->entries->clusterDef) {
                break;
            }
        }
        if (prev == scope) {
            numUniqueClusters++;
        }
    }

    top = (RwUInt8*)StalacTiteAlloc(0);
    {
        RwUInt8* bottom = (RwUInt8*)StalacMiteAlloc(0);

        memset(bottom, 0, top - bottom);
    }

    pipeline->embeddedPacket = (RxPacket*)StalacMiteAlloc((pipeline->packetNumClusterSlots - 1) * sizeof(RxCluster) +
                                                          sizeof(RxPacket));
    clusters = NULL;
    pipeline->embeddedPacket->numClusters = pipeline->packetNumClusterSlots;
    pipeline->embeddedPacket->pipeline = pipeline;
    pipeline->embeddedPacketState = rxPKST_PACKETLESS;

    ScopesForAllUniqueClusters(scopes, _MyEnumPipelineClustersCallBack, &clusters);

    for (i = 0; i < pipeline->numNodes; i++) {
        RxPipelineNode* node = &pipeline->nodes[i];
        RwUInt32 continuity;
        RwUInt32 j;

        if (pipeline->packetNumClusterSlots) {
            node->slotClusterRefs =
                (RxPipelineCluster**)StalacMiteAlloc(pipeline->packetNumClusterSlots * sizeof(RxPipelineCluster*));
        }
        node->slotsContinue = (RwUInt32*)StalacMiteAlloc((pipeline->packetNumClusterSlots + 1) * sizeof(RwUInt32));
        if (node->nodeDef->io.numClustersOfInterest) {
            node->inputToClusterSlot =
                (RwUInt32*)StalacMiteAlloc(node->nodeDef->io.numClustersOfInterest * sizeof(RwUInt32));
        }
        if (node->nodeDef->pipelineNodePrivateDataSize) {
            node->privateData = StalacMiteAlloc((node->nodeDef->pipelineNodePrivateDataSize + 3) & ~3);
        }

        continuity = (RwUInt32)-1;
        for (j = 0; j < NODEREQ(node)->numEntries; j++) {
            rxReqEntry* entry = REQENTRY(NODEREQ(node), j);
            RxPipelineCluster* cluster = NULL;
            RwUInt32 k;

            for (k = 0; k < numUniqueClusters; k++) {
                if (clusters[k].clusterRef == entry->clusterDef) {
                    cluster = &clusters[k];
                    break;
                }
            }
            node->slotClusterRefs[entry->slot] = cluster;
            node->slotsContinue[entry->slot + 1] = entry->continuity;
            continuity &= entry->continuity;
        }
        node->slotsContinue[0] = continuity;

        for (j = 0; j < node->nodeDef->io.numClustersOfInterest; j++) {
            RwUInt32 k;

            node->inputToClusterSlot[j] = (RwUInt32)-1;
            for (k = 0; k < pipeline->packetNumClusterSlots; k++) {
                if (node->slotClusterRefs[k] &&
                    node->slotClusterRefs[k]->clusterRef == node->nodeDef->io.clustersOfInterest[j].clusterDef) {
                    node->inputToClusterSlot[j] = k;
                    break;
                }
            }
        }
    }

    if (numInputRequirements) {
        pipeline->inputRequirements = StalacMiteAlloc(numInputRequirements * sizeof(RxPipelineRequiresCluster));
    }

    {
        RwUInt32 count = 0;

        req = NODEREQ(&pipeline->nodes[0]);
        for (i = 0; i < req->numEntries; i++) {
            rxReqEntry* entry = REQENTRY(req, i);

            if (entry->rqdOrOpt == rxCLREQ_REQUIRED || entry->rqdOrOpt == rxCLREQ_OPTIONAL) {
                RxPipelineRequiresCluster* inputReq = &((RxPipelineRequiresCluster*)pipeline->inputRequirements)[count++];

                inputReq->clusterDef = entry->clusterDef;
                inputReq->rqdOrOpt = entry->rqdOrOpt;
                inputReq->slotIndex = entry->slot;
            }
        }
        pipeline->numInputRequirements = count;
    }

    _rx_rxRadixExchangeSort(pipeline->inputRequirements, pipeline->numInputRequirements,
                            sizeof(RxPipelineRequiresCluster), 0, 0, (RwUInt32)-1);
    return 0;
}

RwUInt32 _rxChaseDependencies(RxPipeline* pipeline)
{
    rxScope* scopes = NULL;
    RwUInt32 result;

    if ((result = _PropagateDependenciesAndKillDeadPaths(pipeline)) == 0 &&
        (result = _ForAllNodeReqsAddOutputClustersAndBuildContinuityBitfields(pipeline)) == 0 &&
        (result = _TraceClusterScopes(pipeline, &scopes)) == 0 &&
        (result = _AssignClusterSlots(pipeline, &scopes)) == 0) {
        result = _ForAllNodesWriteClusterAllocations(pipeline, scopes);
    }
    return result;
}

/*
 * Embedded packet hand-off: passing a pipeline's packet on to the next node
 * or into another pipeline, dropping the clusters the receiver does not
 * want, and destroying a packet's cluster data.
 */

#include "rwsdk/p2core_int.h"

#define rxCLFLAGS_EXTERNAL 0x02

typedef struct RxHeap RxHeap;

typedef struct RxPipelineRequiresCluster {
    /* 0x00 */ RxClusterDefinition* clusterDef;
    /* 0x04 */ RxClusterValidityReq rqdOrOpt;
    /* 0x08 */ RwUInt32 slotIndex;
} RxPipelineRequiresCluster;

extern RxHeap* _rxHeapGlobal;

void RxHeapFree(RxHeap* heap, void* block);

/* Forget a cluster's data without touching its cursor or attributes. */
#define RxClusterClear(_cl)                \
    do {                                   \
        *(RwUInt32*)&(_cl)->flags = 0;     \
        (_cl)->data = NULL;                \
        (_cl)->numAlloced = 0;             \
        (_cl)->numUsed = 0;                \
        (_cl)->clusterRef = NULL;          \
    } while (0)

void _rxPacketDestroy(RxPacket* packet);

void _rxEmbeddedPacketBetweenPipelines(RxPipeline* fromPipeline, RxPipeline* toPipeline)
{
    RwUInt32 i;
    RxPacket* toPacket;
    RxPacket* fromPacket;

    if (fromPipeline->embeddedPacketState > rxPKST_UNUSED) {
        fromPacket = fromPipeline->embeddedPacket;
        toPacket = toPipeline->embeddedPacket;
        for (i = 0; i < toPipeline->numInputRequirements; i++) {
            RwUInt32 j;
            RxPipelineRequiresCluster* req;

            req = &((RxPipelineRequiresCluster*)toPipeline->inputRequirements)[i];

            for (j = 0; j < fromPacket->numClusters; j++) {
                if (fromPacket->clusters[j].clusterRef &&
                    fromPacket->clusters[j].clusterRef->clusterRef == req->clusterDef) {
                    break;
                }
            }

            if (j != fromPacket->numClusters && fromPacket->clusters[j].data) {
                toPacket->clusters[req->slotIndex] = fromPacket->clusters[j];
                toPacket->clusters[req->slotIndex].clusterRef = toPipeline->nodes->slotClusterRefs[req->slotIndex];
                fromPacket->clusters[j].data = NULL;
            }
        }

        _rxPacketDestroy(fromPacket);
        toPacket->slotClusterRefs = toPipeline->nodes->slotClusterRefs;
        toPacket->inputToClusterSlot = toPipeline->nodes->inputToClusterSlot;
        toPacket->slotsContinue = toPipeline->nodes->slotsContinue;
        toPipeline->embeddedPacketState = rxPKST_PENDING;
    }
}

RxPipelineNode* _rxEmbeddedPacketBetweenNodes(RxPipeline* pipeline, RxPipelineNode* nodeFrom, RwUInt32 whichOutput)
{
    RwUInt32 nodeIndex = nodeFrom->outputs[whichOutput];
    RxPacket* packet;
    RxPipelineNode* nodeTo;

    if (nodeIndex != (RwUInt32)-1) {
        nodeTo = &pipeline->nodes[nodeIndex];

        if (pipeline->embeddedPacketState > rxPKST_UNUSED) {
            RwUInt32 outputBit;

            packet = pipeline->embeddedPacket;
            outputBit = 1 << whichOutput;

            if (!(packet->slotsContinue[0] & outputBit)) {
                RwUInt32 i = packet->numClusters;

                do {
                    if (!(outputBit & packet->slotsContinue[i]) && packet->clusters[i - 1].clusterRef) {
                        if (packet->clusters[i - 1].data && !(packet->clusters[i - 1].flags & rxCLFLAGS_EXTERNAL)) {
                            RxHeapFree(_rxHeapGlobal, packet->clusters[i - 1].data);
                        }
                        RxClusterClear(&packet->clusters[i - 1]);
                    }
                } while (--i);
            }

            packet->inputToClusterSlot = nodeTo->inputToClusterSlot;
            packet->slotsContinue = nodeTo->slotsContinue;
            packet->slotClusterRefs = nodeTo->slotClusterRefs;
            pipeline->embeddedPacketState = rxPKST_PENDING;
        }

        return nodeTo;
    }

    return NULL;
}

void _rxPacketDestroy(RxPacket* packet)
{
    RwUInt32 i;
    RxCluster* cl = packet->clusters;

    packet->pipeline->embeddedPacketState = rxPKST_UNUSED;
    i = packet->numClusters;
    do {
        if (cl->clusterRef) {
            if (cl->data && !(cl->flags & rxCLFLAGS_EXTERNAL)) {
                RxHeapFree(_rxHeapGlobal, cl->data);
            }
            RxClusterClear(cl);
        }
        cl++;
    } while (--i);

    packet->flags = 0;
}

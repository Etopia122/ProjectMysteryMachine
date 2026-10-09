#ifndef RWSDK_P2CORE_INT_H
#define RWSDK_P2CORE_INT_H

/* PowerPipe (p2) layouts used by the world pipelines and nodes. */

#include "rwsdk/rwcore.h"

typedef struct RxPipeline RxPipeline;
typedef struct RxPipeline RxLockedPipe;
typedef struct RxPipelineNode RxPipelineNode;
typedef struct RxPipelineNode RxPipelineNodeInstance;
typedef struct RxNodeDefinition RxNodeDefinition;
typedef struct RxClusterDefinition RxClusterDefinition;
typedef struct RxPacket RxPacket;

typedef enum RxClusterValidityReq {
    rxCLREQ_DONTWANT = 0,
    rxCLREQ_REQUIRED = 1,
    rxCLREQ_OPTIONAL = 2,
} RxClusterValidityReq;

typedef enum RxClusterValid {
    rxCLVALID_NOCHANGE = 0,
    rxCLVALID_VALID = 1,
    rxCLVALID_INVALID = 2,
} RxClusterValid;

typedef enum RxClusterForcePresent {
    rxCLALLOWABSENT = 0,
    rxCLFORCEPRESENT = 1,
} RxClusterForcePresent;

#define rxCLRESERVED 0

/* Embedded packet states. */
#define rxPKST_PACKETLESS 0
#define rxPKST_UNUSED 1
#define rxPKST_INUSE 2
#define rxPKST_PENDING 3

typedef struct RxCluster {
    /* 0x00 */ RwUInt16 flags;
    /* 0x02 */ RwUInt16 stride;
    /* 0x04 */ void* data;
    /* 0x08 */ void* currentData;
    /* 0x0C */ RwUInt32 numAlloced;
    /* 0x10 */ RwUInt32 numUsed;
    /* 0x14 */ void* clusterRef;
    /* 0x18 */ RwUInt32 attributes;
} RxCluster;

typedef struct RxPipelineNodeParam {
    void* dataParam;
    void* heap;
} RxPipelineNodeParam;

typedef RwBool (*RxNodeBodyFn)(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params);

typedef struct RxNodeMethods {
    RxNodeBodyFn nodeBody;
    void* nodeInit;
    void* nodeTerm;
    void* pipelineNodeInit;
    void* pipelineNodeTerm;
    void* pipelineNodeConfig;
    void* configMsgHandler;
} RxNodeMethods;

typedef struct RxClusterRef {
    RxClusterDefinition* clusterDef;
    RxClusterForcePresent forcePresent;
    RwUInt32 reserved;
} RxClusterRef;

typedef struct RxOutputSpec {
    RwChar* name;
    RxClusterValid* outputClusters;
    RxClusterValid allOtherClusters;
} RxOutputSpec;

typedef struct RxIoSpec {
    RwUInt32 numClustersOfInterest;
    RxClusterRef* clustersOfInterest;
    RxClusterValidityReq* inputRequirements;
    RwUInt32 numOutputs;
    RxOutputSpec* outputs;
} RxIoSpec;

struct RxNodeDefinition {
    RwChar* name;
    RxNodeMethods nodeMethods;
    RxIoSpec io;
    RwUInt32 pipelineNodePrivateDataSize;
    RwInt32 editable;
    RwInt32 InputPipesCnt;
};

struct RxPipelineNode {
    RxNodeDefinition* nodeDef;
};

struct RxPipeline {
    /* 0x00 */ RwUInt8 pad00[0x10];
    /* 0x10 */ RwInt32 embeddedPacketState;
    /* 0x14 */ RxPacket* embeddedPacket;
    /* 0x18 */ RwUInt8 pad18[0x2C - 0x18];
    /* 0x2C */ RwUInt32 pluginId;
    /* 0x30 */ RwUInt32 pluginData;
};

typedef struct RxExecutionContext {
    /* 0x00 */ RxPipeline* pipeline;
    /* 0x04 */ RxPipelineNode* currentNode;
    /* 0x08 */ RwInt32 exitCode;
    /* 0x0C */ RwUInt32 pad;
    /* 0x10 */ RxPipelineNodeParam params;
} RxExecutionContext;

typedef struct RxRenderStateVector {
    /* 0x00 */ RwUInt32 Flags;
    /* 0x04 */ RwInt32 ShadeMode;
    /* 0x08 */ RwInt32 SrcBlend;
    /* 0x0C */ RwInt32 DestBlend;
    /* 0x10 */ RwRaster* TextureRaster;
    /* 0x14 */ RwInt32 AddressModeU;
    /* 0x18 */ RwInt32 AddressModeV;
    /* 0x1C */ RwInt32 FilterMode;
    /* 0x20 */ RwRGBA BorderColor;
    /* 0x24 */ RwInt32 FogType;
    /* 0x28 */ RwRGBA FogColor;
    /* 0x2C */ RwUInt8* FogTable;
} RxRenderStateVector;

typedef struct rwPipeGlobals {
    /* 0x00 */ RwUInt32 pad00;
    /* 0x04 */ RxRenderStateVector defaultRenderState;
    /* 0x34 */ RwUInt8 pad34[0xC];
    /* 0x40 */ RxPipeline* currentAtomicPipeline;
    /* 0x44 */ RxPipeline* currentWorldSectorPipeline;
    /* 0x48 */ RxPipeline* currentMaterialPipeline;
    /* 0x4C */ RxPipeline* genericAtomicPipeline;
    /* 0x50 */ RxPipeline* genericWorldSectorPipeline;
    /* 0x54 */ RxPipeline* genericMaterialPipeline;
    /* 0x58 */ RxPipeline* platformAtomicPipeline;
    /* 0x5C */ RxPipeline* platformWorldSectorPipeline;
    /* 0x60 */ RxPipeline* platformMaterialPipeline;
} rwPipeGlobals;

extern RwInt32 _rxPipelineGlobalsOffset;
extern RxExecutionContext _rxExecCtxGlobal;

#define RXPIPELINEGLOBAL(var) (RWPLUGINOFFSET(rwPipeGlobals, RwEngineInstance, _rxPipelineGlobalsOffset)->var)

#define RxPacketFetch(node)                                                    \
    ((_rxExecCtxGlobal.pipeline->embeddedPacketState == rxPKST_PENDING)        \
         ? (_rxExecCtxGlobal.pipeline->embeddedPacketState = rxPKST_INUSE,     \
            _rxExecCtxGlobal.pipeline->embeddedPacket)                         \
         : NULL)

RxPipeline* RxPipelineCreate(void);
RxLockedPipe* RxPipelineLock(RxPipeline* pipeline);
RxLockedPipe* RxLockedPipeAddFragment(RxLockedPipe* pipeline, RwUInt32* firstIndex, RxNodeDefinition* nodeDef0,
                                      ...);
RxPipeline* RxLockedPipeUnlock(RxLockedPipe* pipeline);
void _rxPipelineDestroy(RxPipeline* pipeline);
RxPipeline* RxPipelineExecute(RxPipeline* pipeline, void* data, RwBool heapReset);
RxCluster* RxClusterResize(RxCluster* cluster, RwUInt32 minSize);
RxCluster* RxClusterLockWrite(RxPacket* packet, RwUInt32 clusterIndex, RxPipelineNodeInstance* node);
RxCluster* RxClusterInitializeData(RxCluster* cluster, RwUInt32 numElements, RwUInt16 stride);
RxPipelineNode* _rxEmbeddedPacketBetweenNodes(RxPipeline* pipeline, RxPipelineNode* nodeFrom, RwUInt32 whichOutput);
void _rxPacketDestroy(RxPacket* packet);

#endif

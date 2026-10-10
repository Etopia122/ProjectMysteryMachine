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
typedef struct RxPipelineCluster RxPipelineCluster;

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
    /* 0x14 */ RxPipelineCluster* clusterRef;
    /* 0x18 */ RwUInt32 attributes;
} RxCluster;

struct RxPipelineCluster {
    /* 0x00 */ RxClusterDefinition* clusterRef;
    /* 0x04 */ RwUInt32 creationAttributes;
};

struct RxPacket {
    /* 0x00 */ RwUInt16 flags;
    /* 0x02 */ RwUInt16 numClusters;
    /* 0x04 */ RxPipeline* pipeline;
    /* 0x08 */ RwUInt32* inputToClusterSlot;
    /* 0x0C */ RwUInt32* slotsContinue;
    /* 0x10 */ RxPipelineCluster** slotClusterRefs;
    /* 0x14 */ RxCluster clusters[1];
};

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

typedef struct RxPipelineNodeTopSortData {
    /* 0x00 */ RwUInt32 numIns;
    /* 0x04 */ RwUInt32 numInsVisited;
    /* 0x08 */ void* req;
} RxPipelineNodeTopSortData;

struct RxPipelineNode {
    /* 0x00 */ RxNodeDefinition* nodeDef;
    /* 0x04 */ RwUInt32 numOutputs;
    /* 0x08 */ RwUInt32* outputs;
    /* 0x0C */ RxPipelineCluster** slotClusterRefs;
    /* 0x10 */ RwUInt32* slotsContinue;
    /* 0x14 */ void* privateData;
    /* 0x18 */ RwUInt32* inputToClusterSlot;
    /* 0x1C */ RxPipelineNodeTopSortData* topSortData;
    /* 0x20 */ void* initializationData;
    /* 0x24 */ RwUInt32 initializationDataSize;
};

struct RxPipeline {
    /* 0x00 */ RwBool locked;
    /* 0x04 */ RwUInt32 numNodes;
    /* 0x08 */ RxPipelineNode* nodes;
    /* 0x0C */ RwUInt32 packetNumClusterSlots;
    /* 0x10 */ RwInt32 embeddedPacketState;
    /* 0x14 */ RxPacket* embeddedPacket;
    /* 0x18 */ RwUInt32 numInputRequirements;
    /* 0x1C */ void* inputRequirements;
    /* 0x20 */ void* superBlock;
    /* 0x24 */ RwUInt32 superBlockSize;
    /* 0x28 */ RwUInt32 entryPoint;
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

typedef struct RxMeshSurfaceProperties {
    /* 0x00 */ RwReal ambient;
    /* 0x04 */ RwReal specular;
    /* 0x08 */ RwReal diffuse;
} RxMeshSurfaceProperties;

typedef struct RxMeshStateVector {
    /* 0x00 */ RwInt32 Flags;
    /* 0x04 */ void* SourceObject;
    /* 0x08 */ RwMatrix Obj2World;
    /* 0x48 */ RwMatrix Obj2Cam;
    /* 0x88 */ RxMeshSurfaceProperties SurfaceProperties;
    /* 0x94 */ RwTexture* Texture;
    /* 0x98 */ RwRGBA MatCol;
    /* 0x9C */ RxPipeline* Pipeline;
    /* 0xA0 */ RwInt32 PrimType;
    /* 0xA4 */ RwUInt32 NumElements;
    /* 0xA8 */ RwUInt32 NumVertices;
    /* 0xAC */ RwInt32 ClipFlagsOr;
    /* 0xB0 */ RwInt32 ClipFlagsAnd;
    /* 0xB4 */ void* SourceMesh;
    /* 0xB8 */ void* DataObject;
} RxMeshStateVector;

#define rxGEOMETRY_TRISTRIP 0x01
#define rxGEOMETRY_POSITIONS 0x02
#define rxGEOMETRY_TEXTURED 0x04
#define rxGEOMETRY_PRELIT 0x08
#define rxGEOMETRY_NORMALS 0x10
#define rxGEOMETRY_LIGHT 0x20
#define rxGEOMETRY_MODULATE 0x40
#define rxGEOMETRY_TEXTURED2 0x80
#define rxGEOMETRY_COLORED 0x100

#define rxRENDERSTATEFLAG_TEXTUREPERSPECTIVE 0x01
#define rxRENDERSTATEFLAG_ZTESTENABLE 0x02
#define rxRENDERSTATEFLAG_ZWRITEENABLE 0x04
#define rxRENDERSTATEFLAG_VERTEXALPHAENABLE 0x08
#define rxRENDERSTATEFLAG_FOGENABLE 0x10
#define rxRENDERSTATEFLAG_ALPHAPRIMITIVEBUFFER 0x20

#define RxClusterGetCursorData(_cl, _type) ((_type*)(_cl)->currentData)

/* Hands the packet on to the node connected to the given output, then frees
 * the embedded packet if nothing claimed it. */
#define RxPacketDispatch(_packet, _output, _self)                                                                         do {                                                                                                                       RxPipeline* _curPipeline = _rxExecCtxGlobal.pipeline;                                                                                                                                                                                         if (_rxExecCtxGlobal.exitCode != FALSE) {                                                                                  RxPipelineNode* _nextNode = _rxEmbeddedPacketBetweenNodes(_curPipeline, (_self), (_output));                                                                                                                                                  if (_nextNode) {                                                                                                           RwUInt32 _exitCode = _nextNode->nodeDef->nodeMethods.nodeBody(_nextNode, &_rxExecCtxGlobal.params);                                                                                                                                           if (!_exitCode) {                                                                                                          _rxExecCtxGlobal.exitCode = _exitCode;                                                                             }                                                                                                                  }                                                                                                                  }                                                                                                                                                                                                                                             if (_curPipeline->embeddedPacketState > rxPKST_UNUSED) {                                                                   _curPipeline->embeddedPacketState = rxPKST_INUSE;                                                                      _rxPacketDestroy(_curPipeline->embeddedPacket);                                                                    }                                                                                                                  } while (0)

typedef struct rwPipeGlobals {
    /* 0x00 */ RwFreeList* pipesFreeList;
    /* 0x04 */ RxRenderStateVector defaultRenderState;
    /* 0x34 */ RwUInt32 unk34;
    /* 0x38 */ RwUInt32 unk38;
    /* 0x3C */ RwUInt32 maxNodesPerPipe;
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
RxPacket* RxPacketCreate(RxPipelineNode* node);
RxCluster* RxClusterSetExternalData(RxCluster* cluster, void* data, RwInt32 stride, RwInt32 numElements);

#endif

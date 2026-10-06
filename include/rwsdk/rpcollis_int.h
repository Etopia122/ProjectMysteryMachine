#ifndef RWSDK_RPCOLLIS_INT_H
#define RWSDK_RPCOLLIS_INT_H

// Collision plugin internals (layouts from the PS2 DWARF of the p2rwcoll units).

#include "rwsdk/rwcore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RpCollBSPLeafNode {
    RwUInt16 numPolygons;
    RwUInt16 firstPolygon;
} RpCollBSPLeafNode;

typedef struct RpCollBSPBranchNode {
    RwUInt16 type; /* split axis as a byte offset into an RwV3d */
    RwUInt8 leftType;
    RwUInt8 rightType;
    RwUInt16 leftNode;
    RwUInt16 rightNode;
    RwReal leftValue;
    RwReal rightValue;
} RpCollBSPBranchNode;

typedef struct RpCollBSPTree {
    RwUInt32 numLeafNodes;
    RpCollBSPBranchNode* branchNodes;
    RpCollBSPLeafNode* leafNodes;
    RwUInt8 pad[4];
} RpCollBSPTree;

typedef struct RpCollisionData {
    RwInt32 flags;
    RpCollBSPTree* tree;
    RwInt32 numTriangles;
    RwUInt16* triangleMap;
} RpCollisionData;

typedef struct RpV3dGradient {
    RwReal dydx;
    RwReal dzdx;
    RwReal dxdy;
    RwReal dzdy;
    RwReal dxdz;
    RwReal dydz;
} RpV3dGradient;

typedef RwInt32 (*RpCollBSPLeafNodeCallBack)(RwInt32 numPolygons, RwInt32 polyOffset, void* data);

RpCollBSPTree* _rpCollBSPTreeInit(RpCollBSPTree* tree, RwInt32 numLeafNodes);
RwInt32 _rpCollBSPTreeMemGetSize(RwInt32 numLeafNodes);
void _rpCollBSPTreeDestroy(RpCollBSPTree* tree);
RpCollBSPTree* _rpCollBSPTreeStreamWrite(RpCollBSPTree* tree, RwStream* stream);
RpCollBSPTree* _rpCollBSPTreeStreamRead(RpCollBSPTree* tree, RwStream* stream);
RwInt32 _rpCollBSPTreeStreamGetSize(RpCollBSPTree* tree);

#ifdef __cplusplus
}
#endif

#endif

/*
 * Collision plugin: BSP tree over a geometry's or world sector's triangles,
 * built offline and streamed with the collision data.
 */

#include "rwsdk/rwcore.h"
#include "rwsdk/rpcollis_int.h"

#define rpCOLLBSP_LEAF_NODE 1
#define rpCOLLBSP_BRANCH_NODE 2

#define GETCOORD(vec, axis) (*(RwReal*)((RwUInt8*)&(vec) + (axis)))

typedef struct nodeInfo {
    RwUInt32 type;
    RwUInt32 index;
} nodeInfo;

RpCollBSPTree* _rpCollBSPTreeInit(RpCollBSPTree* tree, RwInt32 numLeafNodes) {
    RwInt32 numBranchNodes = numLeafNodes - 1;

    tree->numLeafNodes = numLeafNodes;
    if (numBranchNodes > 0) {
        tree->branchNodes = (RpCollBSPBranchNode*)(tree + 1);
        tree->leafNodes = (RpCollBSPLeafNode*)(tree->branchNodes + numBranchNodes);
    } else {
        tree->branchNodes = NULL;
        tree->leafNodes = (RpCollBSPLeafNode*)(tree + 1);
    }

    return tree;
}

RwInt32 _rpCollBSPTreeMemGetSize(RwInt32 numLeafNodes) {
    return sizeof(RpCollBSPTree) + (numLeafNodes - 1) * sizeof(RpCollBSPBranchNode) +
           numLeafNodes * sizeof(RpCollBSPLeafNode);
}

void _rpCollBSPTreeDestroy(RpCollBSPTree* tree) {
    if (tree) {
        RwFree(tree);
    }
}

RpCollBSPTree* _rpCollBSPTreeStreamWrite(RpCollBSPTree* tree, RwStream* stream) {
    RwUInt32 numLeaves;
    RwUInt32 numBranches;
    RpCollBSPBranchNode* branch;
    RpCollBSPLeafNode* leaf;
    RwUInt32 types;
    RwUInt32 indices;
    RwUInt32 data;

    numBranches = tree->numLeafNodes - 1;
    branch = tree->branchNodes;
    while (numBranches--) {
        types = (branch->type << 16) | (branch->leftType << 8) | branch->rightType;
        indices = (branch->leftNode << 16) | branch->rightNode;

        if (!RwStreamWriteInt32(stream, (RwInt32*)&types, sizeof(RwUInt32)) ||
            !RwStreamWriteInt32(stream, (RwInt32*)&indices, sizeof(RwUInt32)) ||
            !RwStreamWriteReal(stream, &branch->leftValue, sizeof(RwReal)) ||
            !RwStreamWriteReal(stream, &branch->rightValue, sizeof(RwReal))) {
            return NULL;
        }

        branch++;
    }

    numLeaves = tree->numLeafNodes;
    leaf = tree->leafNodes;
    while (numLeaves--) {
        data = (leaf->numPolygons << 16) | leaf->firstPolygon;

        if (!RwStreamWriteInt32(stream, (RwInt32*)&data, sizeof(RwUInt32))) {
            return NULL;
        }

        leaf++;
    }

    return tree;
}

RpCollBSPTree* _rpCollBSPTreeStreamRead(RpCollBSPTree* tree, RwStream* stream) {
    RwUInt32 numLeaves;
    RwUInt32 numBranches;
    RpCollBSPBranchNode* branch;
    RpCollBSPLeafNode* leaf;
    RwUInt32 types;
    RwUInt32 indices;
    RwUInt32 data;

    numBranches = tree->numLeafNodes - 1;
    branch = tree->branchNodes;
    while (numBranches--) {
        if (!RwStreamReadInt32(stream, (RwInt32*)&types, sizeof(RwUInt32)) ||
            !RwStreamReadInt32(stream, (RwInt32*)&indices, sizeof(RwUInt32)) ||
            !RwStreamReadReal(stream, &branch->leftValue, sizeof(RwReal)) ||
            !RwStreamReadReal(stream, &branch->rightValue, sizeof(RwReal))) {
            return NULL;
        }

        branch->type = (RwUInt16)(types >> 16);
        branch->leftType = (RwUInt8)(types >> 8);
        branch->rightType = (RwUInt8)types;
        branch->leftNode = (RwUInt16)(indices >> 16);
        branch->rightNode = (RwUInt16)indices;

        branch++;
    }

    numLeaves = tree->numLeafNodes;
    leaf = tree->leafNodes;
    while (numLeaves--) {
        if (!RwStreamReadInt32(stream, (RwInt32*)&data, sizeof(RwUInt32))) {
            _rpCollBSPTreeDestroy(tree);
            return NULL;
        }

        leaf->numPolygons = (RwUInt16)(data >> 16);
        leaf->firstPolygon = (RwUInt16)data;

        leaf++;
    }

    return tree;
}

RwInt32 _rpCollBSPTreeStreamGetSize(RpCollBSPTree* tree) {
    RwInt32 size;

    size = (tree->numLeafNodes - 1) * (sizeof(RwUInt32) * 2 + sizeof(RwReal) * 2) +
           tree->numLeafNodes * sizeof(RwUInt32);

    return size;
}

/* The integer views are volatile so the float is always reread from memory. */
typedef union RwSplitBits {
    RwReal nReal;
    volatile RwInt32 nInt;
    volatile RwUInt32 nUInt;
} RwSplitBits;

/* Point where a line crosses the plane coord[axis] == value. */
#define GETINTERSECTION(vec, ln, value, axis, grad)                     \
    switch (axis) {                                                     \
    case 0: {                                                           \
        RwReal delta = (value) - (ln).start.x;                          \
        (vec).x = (value);                                              \
        (vec).y = (ln).start.y + (grad)->dydx * delta;                  \
        (vec).z = (ln).start.z + (grad)->dzdx * delta;                  \
        break;                                                          \
    }                                                                   \
    case 4: {                                                           \
        RwReal delta = (value) - (ln).start.y;                          \
        (vec).x = (ln).start.x + (grad)->dxdy * delta;                  \
        (vec).y = (value);                                              \
        (vec).z = (ln).start.z + (grad)->dzdy * delta;                  \
        break;                                                          \
    }                                                                   \
    case 8: {                                                           \
        RwReal delta = (value) - (ln).start.z;                          \
        (vec).x = (ln).start.x + (grad)->dxdz * delta;                  \
        (vec).y = (ln).start.y + (grad)->dydz * delta;                  \
        (vec).z = (value);                                              \
        break;                                                          \
    }                                                                   \
    }

#define PUSHNODE(lnStart, lnEnd, ntype, nindex) \
    nStack++;                                   \
    nodeStack[nStack].type = (ntype);           \
    nodeStack[nStack].index = (nindex);         \
    lineStack[nStack].start = (lnStart);        \
    lineStack[nStack].end = (lnEnd)

RpCollBSPTree* _rpCollBSPTreeForAllLineLeafNodeIntersections(RpCollBSPTree* tree, RwLine* line, RpV3dGradient* grad,
                                                             RpCollBSPLeafNodeCallBack callBack, void* data) {
    RwLine lineStack[33];
    nodeInfo nodeStack[33];
    RwLine currLine;
    nodeInfo node;
    RwSplitBits lStart;
    RwSplitBits lEnd;
    RwSplitBits rStart;
    RwSplitBits rEnd;
    RpCollBSPLeafNode* leaf;
    RpCollBSPBranchNode* branch;
    RwInt32 nStack;
    RwUInt32 axis;
    RwUInt32 leftType;
    RwUInt32 rightType;
    RwUInt32 leftNode;
    RwUInt32 rightNode;

    node.type = tree->branchNodes ? rpCOLLBSP_BRANCH_NODE : rpCOLLBSP_LEAF_NODE;
    node.index = 0;
    nStack = 0;
    currLine = *line;

    while (nStack >= 0) {
        if (node.type == rpCOLLBSP_LEAF_NODE) {
            leaf = &tree->leafNodes[node.index];

            if (!callBack(leaf->numPolygons, leaf->firstPolygon, data)) {
                return NULL;
            }

            node = nodeStack[nStack];
            currLine = lineStack[nStack];
            nStack--;
        } else {
            branch = &tree->branchNodes[node.index];
            axis = branch->type;
            leftType = branch->leftType;
            rightType = branch->rightType;
            leftNode = branch->leftNode;
            rightNode = branch->rightNode;

            rStart.nReal = GETCOORD(currLine.start, axis) - branch->rightValue;
            lStart.nReal = GETCOORD(currLine.start, axis) - branch->leftValue;
            lEnd.nReal = GETCOORD(currLine.end, axis) - branch->leftValue;
            rEnd.nReal = GETCOORD(currLine.end, axis) - branch->rightValue;

            if (rStart.nInt < 0 && rEnd.nInt < 0) {
                /* entirely before the right plane: left only */
                node.type = leftType;
                node.index = leftNode;
            } else if (lStart.nInt >= 0 && lEnd.nInt >= 0) {
                /* entirely past the left plane: right only */
                node.type = rightType;
                node.index = rightNode;
            } else if (!((lStart.nInt ^ lEnd.nInt) & 0x80000000) && !((rStart.nInt ^ rEnd.nInt) & 0x80000000)) {
                /* inside the overlap: visit both, nearer first */
                if (rStart.nInt < rEnd.nInt) {
                    PUSHNODE(currLine.start, currLine.end, rightType, rightNode);
                    node.type = leftType;
                    node.index = leftNode;
                } else {
                    PUSHNODE(currLine.start, currLine.end, leftType, leftNode);
                    node.type = rightType;
                    node.index = rightNode;
                }
            } else if (((lStart.nInt ^ lEnd.nInt) & 0x80000000) && rStart.nInt >= 0 && rEnd.nInt >= 0) {
                /* crosses the left plane only */
                RwV3d vLeft;

                GETINTERSECTION(vLeft, currLine, branch->leftValue, axis, grad);
                if (lStart.nInt < 0) {
                    PUSHNODE(currLine.start, currLine.end, rightType, rightNode);
                    node.type = leftType;
                    node.index = leftNode;
                    currLine.end = vLeft;
                } else {
                    PUSHNODE(vLeft, currLine.end, leftType, leftNode);
                    node.type = rightType;
                    node.index = rightNode;
                }
            } else if (((rStart.nInt ^ rEnd.nInt) & 0x80000000) && lStart.nInt < 0 && lEnd.nInt < 0) {
                /* crosses the right plane only */
                RwV3d vRight;

                GETINTERSECTION(vRight, currLine, branch->rightValue, axis, grad);
                if (rStart.nInt < 0) {
                    PUSHNODE(vRight, currLine.end, rightType, rightNode);
                    node.type = leftType;
                    node.index = leftNode;
                } else {
                    PUSHNODE(currLine.start, currLine.end, leftType, leftNode);
                    node.type = rightType;
                    node.index = rightNode;
                    currLine.end = vRight;
                }
            } else {
                /* crosses both planes */
                RwV3d vLeft;
                RwV3d vRight;

                GETINTERSECTION(vLeft, currLine, branch->leftValue, axis, grad);
                GETINTERSECTION(vRight, currLine, branch->rightValue, axis, grad);
                if (lStart.nInt < 0) {
                    PUSHNODE(vRight, currLine.end, rightType, rightNode);
                    node.type = leftType;
                    node.index = leftNode;
                    currLine.end = vLeft;
                } else {
                    PUSHNODE(vLeft, currLine.end, leftType, leftNode);
                    node.type = rightType;
                    node.index = rightNode;
                    currLine.end = vRight;
                }
            }
        }
    }

    return tree;
}

RpCollBSPTree* _rpCollBSPTreeForAllBoxLeafNodeIntersections(RpCollBSPTree* tree, RwBBox* box,
                                                            RpCollBSPLeafNodeCallBack callBack, void* data) {
    RwInt32 nStack;
    nodeInfo nodeStack[33];
    RpCollBSPLeafNode* leaf;
    RpCollBSPBranchNode* branch;
    nodeInfo node;

    node.index = 0;
    node.type = tree->branchNodes ? rpCOLLBSP_BRANCH_NODE : rpCOLLBSP_LEAF_NODE;
    nStack = 0;

    while (nStack >= 0) {
        if (node.type == rpCOLLBSP_LEAF_NODE) {
            leaf = &tree->leafNodes[node.index];

            if (!callBack(leaf->numPolygons, leaf->firstPolygon, data)) {
                return NULL;
            }

            node = nodeStack[nStack--];
        } else {
            branch = &tree->branchNodes[node.index];

            if (GETCOORD(box->inf, branch->type) < branch->leftValue) {
                node.type = branch->leftType;
                node.index = branch->leftNode;

                if (GETCOORD(box->sup, branch->type) >= branch->rightValue) {
                    nStack++;
                    nodeStack[nStack].index = branch->rightNode;
                    nodeStack[nStack].type = branch->rightType;
                }
            } else {
                node.type = branch->rightType;
                node.index = branch->rightNode;
            }
        }
    }

    return tree;
}

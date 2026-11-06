/*
 * Hierarchical animation plugin: per-frame hierarchy extension, stream
 * support, interpolation scheme registry and hierarchy creation.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID rwID_HANIMPLUGIN
#define rwID_HANIMPLUGIN 0x11E

#define rpHANIMSTREAMCURRENTVERSION 0x100
#define rpHANIMMAXINTERPOLATORS 16
#define rpHANIMSTDKEYFRAMETYPEID 1

#define E_RP_HANIM_INTERP_IDINUSE 0
#define E_RP_HANIM_INTERP_BLOCKFULL 1

typedef enum RpHAnimHierarchyFlag {
    rpHANIMHIERARCHYSUBHIERARCHY = 0x01,
    rpHANIMHIERARCHYNOMATRICES = 0x02,
} RpHAnimHierarchyFlag;

typedef struct RpHAnimHierarchy RpHAnimHierarchy;

typedef struct RpHAnimNodeInfo {
    RwInt32 nodeID;
    RwInt32 nodeIndex;
    RwInt32 flags;
    RwFrame* pFrame;
} RpHAnimNodeInfo;

struct RpHAnimHierarchy {
    /* 0x00 */ RwInt32 flags;
    /* 0x04 */ RwInt32 numNodes;
    /* 0x08 */ void* pCurrentAnim;
    /* 0x0C */ RwReal currentTime;
    /* 0x10 */ void* pNextFrame;
    /* 0x14 */ void* pAnimCallBack;
    /* 0x18 */ void* pAnimCallBackData;
    /* 0x1C */ RwReal animCallBackTime;
    /* 0x20 */ void* pAnimLoopCallBack;
    /* 0x24 */ void* pAnimLoopCallBackData;
    /* 0x28 */ RwMatrix* pMatrixArray;
    /* 0x2C */ void* pMatrixArrayUnaligned;
    /* 0x30 */ RpHAnimNodeInfo* pNodeInfo;
    /* 0x34 */ RwFrame* parentFrame;
    /* 0x38 */ RwInt32 maxKeyFrameSize;
    /* 0x3C */ RwInt32 currentKeyFrameSize;
    /* 0x40 */ void* keyFrameToMatrixCB;
    /* 0x44 */ void* keyFrameBlendCB;
    /* 0x48 */ void* keyFrameInterpolateCB;
    /* 0x4C */ void* keyFrameAddCB;
    /* 0x50 */ RpHAnimHierarchy* parentHierarchy;
    /* 0x54 */ RwInt32 offsetInParent;
    /* 0x58 */ RwInt32 rootParentOffset;
};

typedef struct RpHAnimFrameExtension {
    RwInt32 id;
    RpHAnimHierarchy* hierarchy;
} RpHAnimFrameExtension;

typedef struct RpHAnimInterpolatorInfo {
    RwInt32 typeID;
    RwInt32 keyFrameSize;
    void* keyFrameToMatrixCB;
    void* keyFrameBlendCB;
    void* keyFrameInterpolateCB;
    void* keyFrameAddCB;
    void* keyFrameMulRecipCB;
    void* keyFrameStreamReadCB;
    void* keyFrameStreamWriteCB;
    void* keyFrameStreamGetSizeCB;
} RpHAnimInterpolatorInfo;

typedef struct RpHAnimAtomicGlobalVars {
    RwInt32 engineOffset;
    RwFreeList* HAnimAnimationFreeList;
    RwFreeList* HAnimFreeList;
} RpHAnimAtomicGlobalVars;

#define RPHANIMFRAMEGETEXT(frame) ((RpHAnimFrameExtension*)((RwUInt8*)(frame) + RpHAnimAtomicGlobals.engineOffset))

RpHAnimAtomicGlobalVars RpHAnimAtomicGlobals;
RpHAnimInterpolatorInfo RpHAnimInterpolatorInfoBlock[rpHANIMMAXINTERPOLATORS];
RwInt32 RpHAnimNumInterpolatorTypes;

void RpHAnimStdKeyFrameToMatrix(void);
void RpHAnimStdKeyFrameBlend(void);
void RpHAnimStdKeyFrameInterpolate(void);
void RpHAnimStdKeyFrameAdd(void);
void RpHAnimStdKeyFrameMulRecip(void);
void RpHAnimStdKeyFrameStreamRead(void);
void RpHAnimStdKeyFrameStreamWrite(void);
void RpHAnimStdKeyFrameStreamGetSize(void);

void* memset(void* dst, int val, unsigned long n);
RwInt32 RwFrameRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RwFrameRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                    RwPluginDataChunkWriteCallBack writeCB,
                                    RwPluginDataChunkGetSizeCallBack getSizeCB);

RwBool RpHAnimRegisterInterpolationScheme(RpHAnimInterpolatorInfo* interpolatorInfo);
RpHAnimHierarchy* RpHAnimHierarchyCreate(RwInt32 numNodes, RwUInt32* nodeFlags, RwInt32* nodeIDs,
                                         RpHAnimHierarchyFlag flags, RwInt32 maxKeyFrameSize);
RpHAnimHierarchy* RpHAnimHierarchyDestroy(RpHAnimHierarchy* hierarchy);

void* HAnimOpen(void* instance, RwInt32 offset, RwInt32 size) {
    RpHAnimInterpolatorInfo interpolatorInfo;

    RpHAnimAtomicGlobals.HAnimAnimationFreeList = NULL;
    RpHAnimAtomicGlobals.HAnimFreeList = RwFreeListCreate(0x14, 0x14, 0);
    if (!RpHAnimAtomicGlobals.HAnimFreeList) {
        instance = NULL;
    }

    interpolatorInfo.typeID = rpHANIMSTDKEYFRAMETYPEID;
    interpolatorInfo.keyFrameSize = 0x24;
    interpolatorInfo.keyFrameToMatrixCB = RpHAnimStdKeyFrameToMatrix;
    interpolatorInfo.keyFrameBlendCB = RpHAnimStdKeyFrameBlend;
    interpolatorInfo.keyFrameInterpolateCB = RpHAnimStdKeyFrameInterpolate;
    interpolatorInfo.keyFrameAddCB = RpHAnimStdKeyFrameAdd;
    interpolatorInfo.keyFrameMulRecipCB = RpHAnimStdKeyFrameMulRecip;
    interpolatorInfo.keyFrameStreamReadCB = RpHAnimStdKeyFrameStreamRead;
    interpolatorInfo.keyFrameStreamWriteCB = RpHAnimStdKeyFrameStreamWrite;
    interpolatorInfo.keyFrameStreamGetSizeCB = RpHAnimStdKeyFrameStreamGetSize;
    RpHAnimRegisterInterpolationScheme(&interpolatorInfo);

    return instance;
}

void* HAnimClose(void* instance, RwInt32 offset, RwInt32 size) {
    RpHAnimNumInterpolatorTypes = 0;

    if (RpHAnimAtomicGlobals.HAnimFreeList) {
        RwFreeListDestroy(RpHAnimAtomicGlobals.HAnimFreeList);
        RpHAnimAtomicGlobals.HAnimFreeList = NULL;
    }

    if (RpHAnimAtomicGlobals.HAnimAnimationFreeList) {
        RwFreeListDestroy(RpHAnimAtomicGlobals.HAnimAnimationFreeList);
        RpHAnimAtomicGlobals.HAnimAnimationFreeList = NULL;
    }

    return instance;
}

void* HAnimConstructor(void* object, RwInt32 offset, RwInt32 size) {
    RpHAnimFrameExtension* frameExt = RPHANIMFRAMEGETEXT(object);

    frameExt->hierarchy = NULL;
    frameExt->id = -1;

    return object;
}

void* HAnimDestructor(void* object, RwInt32 offset, RwInt32 size) {
    RpHAnimFrameExtension* frameExt = RPHANIMFRAMEGETEXT(object);
    RpHAnimHierarchy* hierarchy = frameExt->hierarchy;

    if (hierarchy) {
        RwInt32 i;

        for (i = 0; i < hierarchy->numNodes; i++) {
            hierarchy->pNodeInfo[i].pFrame = NULL;
        }

        if (hierarchy->parentFrame == (RwFrame*)object) {
            RpHAnimHierarchyDestroy(hierarchy);
        }

        frameExt->hierarchy = NULL;
    }

    frameExt->id = -1;

    return object;
}

void* HAnimCopy(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size) {
    RpHAnimFrameExtension* srcExt;
    RpHAnimHierarchy* srcHierarchy;
    RpHAnimFrameExtension* dstExt;

    srcExt = RPHANIMFRAMEGETEXT(srcObject);
    dstExt = RPHANIMFRAMEGETEXT(dstObject);

    dstExt->id = srcExt->id;

    srcHierarchy = srcExt->hierarchy;
    if (srcHierarchy && !(srcHierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY)) {
        RpHAnimHierarchy* dstHierarchy;
        RwInt32 i;

        dstHierarchy = RpHAnimHierarchyCreate(srcHierarchy->numNodes, NULL, NULL,
                                              (RpHAnimHierarchyFlag)srcHierarchy->flags,
                                              srcHierarchy->maxKeyFrameSize);

        for (i = 0; i < dstHierarchy->numNodes; i++) {
            dstHierarchy->pNodeInfo[i].pFrame = NULL;
            dstHierarchy->pNodeInfo[i].flags = srcHierarchy->pNodeInfo[i].flags;
            dstHierarchy->pNodeInfo[i].nodeIndex = srcHierarchy->pNodeInfo[i].nodeIndex;
            dstHierarchy->pNodeInfo[i].nodeID = srcHierarchy->pNodeInfo[i].nodeID;
        }

        dstExt->hierarchy = dstHierarchy;
        dstHierarchy->parentFrame = (RwFrame*)dstObject;
    }

    return dstObject;
}

RwStream* HAnimWrite(RwStream* stream, RwInt32 binaryLength, const void* object, RwInt32 offsetInObject,
                     RwInt32 sizeInObject) {
    RwInt32 version = rpHANIMSTREAMCURRENTVERSION;
    RpHAnimFrameExtension* frameExt;
    RwInt32 i;
    RpHAnimNodeInfo* nodeInfo;
    RpHAnimHierarchy* hierarchy;

    if (!RwStreamWriteInt32(stream, &version, sizeof(RwInt32))) {
        return NULL;
    }

    frameExt = RPHANIMFRAMEGETEXT(object);
    if (!RwStreamWriteInt32(stream, &frameExt->id, sizeof(RwInt32))) {
        return NULL;
    }

    hierarchy = frameExt->hierarchy;
    if (hierarchy && !(hierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY)) {
        if (!RwStreamWriteInt32(stream, &hierarchy->numNodes, sizeof(RwInt32))) {
            return NULL;
        }

        if (!RwStreamWriteInt32(stream, &hierarchy->flags, sizeof(RwInt32))) {
            return NULL;
        }

        if (!RwStreamWriteInt32(stream, &hierarchy->maxKeyFrameSize, sizeof(RwInt32))) {
            return NULL;
        }

        nodeInfo = hierarchy->pNodeInfo;
        for (i = 0; i < hierarchy->numNodes; i++) {
            if (!RwStreamWriteInt32(stream, &nodeInfo->nodeID, sizeof(RwInt32))) {
                return NULL;
            }

            if (!RwStreamWriteInt32(stream, &nodeInfo->nodeIndex, sizeof(RwInt32))) {
                return NULL;
            }

            if (!RwStreamWriteInt32(stream, &nodeInfo->flags, sizeof(RwInt32))) {
                return NULL;
            }

            nodeInfo++;
        }
    } else {
        RwInt32 numNodes = 0;

        if (!RwStreamWriteInt32(stream, &numNodes, sizeof(RwInt32))) {
            return NULL;
        }
    }

    return stream;
}

RwStream* HAnimRead(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offsetInObject,
                    RwInt32 sizeInObject) {
    RwInt32 numNodes;
    RwInt32 version;
    RwInt32 flags;
    RwInt32 maxKeyFrameSize;
    RpHAnimFrameExtension* frameExt = RPHANIMFRAMEGETEXT(object);

    if (!RwStreamReadInt32(stream, &version, sizeof(RwInt32))) {
        return NULL;
    }

    if (version != rpHANIMSTREAMCURRENTVERSION) {
        return NULL;
    }

    if (!RwStreamReadInt32(stream, &frameExt->id, sizeof(RwInt32))) {
        return NULL;
    }

    if (!RwStreamReadInt32(stream, &numNodes, sizeof(RwInt32))) {
        return NULL;
    }

    if (numNodes > 0) {
        RpHAnimHierarchy* hierarchy;

        if (!RwStreamReadInt32(stream, &flags, sizeof(RwInt32))) {
            return NULL;
        }

        if (!RwStreamReadInt32(stream, &maxKeyFrameSize, sizeof(RwInt32))) {
            return NULL;
        }

        hierarchy = (RpHAnimHierarchy*)RwMalloc(sizeof(RpHAnimHierarchy) + numNodes * maxKeyFrameSize * 3);
        memset(hierarchy, 0, sizeof(RpHAnimHierarchy) + numNodes * maxKeyFrameSize * 3);

        if (hierarchy) {
            RwInt32 i;
            RpHAnimNodeInfo* nodeInfo;

            hierarchy->flags = flags;
            hierarchy->parentFrame = (RwFrame*)object;
            hierarchy->numNodes = numNodes;
            hierarchy->currentKeyFrameSize = maxKeyFrameSize;
            hierarchy->maxKeyFrameSize = maxKeyFrameSize;
            hierarchy->parentHierarchy = hierarchy;
            hierarchy->offsetInParent = 0;

            if (hierarchy->flags & rpHANIMHIERARCHYNOMATRICES) {
                hierarchy->pMatrixArray = NULL;
                hierarchy->pMatrixArrayUnaligned = NULL;
            } else {
                void* unaligned = RwMalloc(numNodes * sizeof(RwMatrix) + 15);

                hierarchy->pMatrixArray = (RwMatrix*)(((RwUInt32)unaligned + 15) & ~15);
                hierarchy->pMatrixArrayUnaligned = unaligned;
            }

            hierarchy->pNodeInfo = (RpHAnimNodeInfo*)RwMalloc(numNodes * sizeof(RpHAnimNodeInfo));

            nodeInfo = hierarchy->pNodeInfo;
            for (i = 0; i < hierarchy->numNodes; i++) {
                if (!RwStreamReadInt32(stream, &nodeInfo->nodeID, sizeof(RwInt32))) {
                    return NULL;
                }

                if (!RwStreamReadInt32(stream, &nodeInfo->nodeIndex, sizeof(RwInt32))) {
                    return NULL;
                }

                if (!RwStreamReadInt32(stream, &nodeInfo->flags, sizeof(RwInt32))) {
                    return NULL;
                }

                nodeInfo->pFrame = NULL;
                nodeInfo++;
            }
        }

        frameExt->hierarchy = hierarchy;
    }

    return stream;
}

RwInt32 HAnimSize(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject) {
    RwInt32 size = sizeof(RwInt32) * 3;
    RpHAnimHierarchy* hierarchy = RPHANIMFRAMEGETEXT(object)->hierarchy;

    if (hierarchy && !(hierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY)) {
        size += sizeof(RwInt32) * 2 + hierarchy->numNodes * sizeof(RwInt32) * 3;
    }

    return size;
}

RwBool RpHAnimRegisterInterpolationScheme(RpHAnimInterpolatorInfo* interpolatorInfo) {
    RwInt32 i;

    if (RpHAnimNumInterpolatorTypes < rpHANIMMAXINTERPOLATORS) {
        for (i = 0; i < RpHAnimNumInterpolatorTypes; i++) {
            if (interpolatorInfo->typeID == RpHAnimInterpolatorInfoBlock[i].typeID) {
                RWERROR((E_RP_HANIM_INTERP_IDINUSE));
                return FALSE;
            }
        }

        RpHAnimInterpolatorInfoBlock[RpHAnimNumInterpolatorTypes] = *interpolatorInfo;
        RpHAnimNumInterpolatorTypes++;
        return TRUE;
    }

    RWERROR((E_RP_HANIM_INTERP_BLOCKFULL));
    return FALSE;
}

RpHAnimHierarchy* RpHAnimHierarchyCreate(RwInt32 numNodes, RwUInt32* nodeFlags, RwInt32* nodeIDs,
                                         RpHAnimHierarchyFlag flags, RwInt32 maxKeyFrameSize) {
    RpHAnimHierarchy* hierarchy;
    RwInt32 i;

    hierarchy = (RpHAnimHierarchy*)RwMalloc(sizeof(RpHAnimHierarchy) + numNodes * maxKeyFrameSize * 3);

    hierarchy->flags = flags;
    hierarchy->numNodes = numNodes;
    hierarchy->pCurrentAnim = NULL;
    hierarchy->pNextFrame = NULL;
    hierarchy->currentTime = 0.0f;
    hierarchy->parentFrame = NULL;
    hierarchy->pAnimCallBack = NULL;
    hierarchy->animCallBackTime = -1.0f;
    hierarchy->pAnimCallBackData = NULL;
    hierarchy->pAnimLoopCallBack = NULL;
    hierarchy->pAnimLoopCallBackData = NULL;

    if (!(flags & rpHANIMHIERARCHYNOMATRICES)) {
        void* unaligned = RwMalloc(numNodes * sizeof(RwMatrix) + 15);

        hierarchy->pMatrixArray = (RwMatrix*)(((RwUInt32)unaligned + 15) & ~15);
        hierarchy->pMatrixArrayUnaligned = unaligned;
    }

    hierarchy->pNodeInfo = (RpHAnimNodeInfo*)RwMalloc(numNodes * sizeof(RpHAnimNodeInfo));

    for (i = 0; i < numNodes; i++) {
        hierarchy->pNodeInfo[i].pFrame = NULL;

        if (nodeIDs) {
            hierarchy->pNodeInfo[i].nodeID = nodeIDs[i];
        }

        hierarchy->pNodeInfo[i].nodeIndex = i;

        if (nodeFlags) {
            hierarchy->pNodeInfo[i].flags = nodeFlags[i];
        }
    }

    hierarchy->currentKeyFrameSize = maxKeyFrameSize;
    hierarchy->maxKeyFrameSize = maxKeyFrameSize;
    hierarchy->keyFrameToMatrixCB = NULL;
    hierarchy->keyFrameInterpolateCB = NULL;
    hierarchy->keyFrameBlendCB = NULL;
    hierarchy->keyFrameAddCB = NULL;
    hierarchy->parentHierarchy = hierarchy;
    hierarchy->offsetInParent = 0;

    return hierarchy;
}

RpHAnimHierarchy* RpHAnimHierarchyDestroy(RpHAnimHierarchy* hierarchy) {
    RwFrame* parentFrame = hierarchy->parentFrame;

    if (!(hierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY)) {
        if (!(hierarchy->flags & rpHANIMHIERARCHYNOMATRICES)) {
            RwFree(hierarchy->pMatrixArrayUnaligned);
        }

        RwFree(hierarchy->pNodeInfo);
    }

    hierarchy->pMatrixArrayUnaligned = NULL;
    hierarchy->pMatrixArray = NULL;
    hierarchy->pNodeInfo = NULL;

    RwFree(hierarchy);

    if (parentFrame) {
        RPHANIMFRAMEGETEXT(parentFrame)->hierarchy = NULL;
    }

    return NULL;
}

RwBool RpHAnimFrameSetHierarchy(RwFrame* frame, RpHAnimHierarchy* hierarchy) {
    RpHAnimFrameExtension* frameExt = RPHANIMFRAMEGETEXT(frame);

    if (frameExt->hierarchy) {
        frameExt->hierarchy->parentFrame = NULL;
    }

    frameExt->hierarchy = hierarchy;

    if (hierarchy) {
        hierarchy->parentFrame = frame;
    }

    return TRUE;
}

RpHAnimHierarchy* RpHAnimFrameGetHierarchy(RwFrame* frame) {
    return RPHANIMFRAMEGETEXT(frame)->hierarchy;
}

RwBool RpHAnimPluginAttach(void) {
    RwInt32 offset;

    if (RwEngineRegisterPlugin(0, rwID_HANIMPLUGIN, HAnimOpen, HAnimClose) < 0) {
        return FALSE;
    }

    RpHAnimAtomicGlobals.engineOffset = RwFrameRegisterPlugin(sizeof(RpHAnimFrameExtension), rwID_HANIMPLUGIN,
                                                              HAnimConstructor, HAnimDestructor, HAnimCopy);

    offset = RwFrameRegisterPluginStream(rwID_HANIMPLUGIN, HAnimRead, HAnimWrite, HAnimSize);

    return (offset >= 0 && RpHAnimAtomicGlobals.engineOffset >= 0);
}

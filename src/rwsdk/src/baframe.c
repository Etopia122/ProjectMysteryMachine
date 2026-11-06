/*
 * Frames: the transform hierarchy objects hang off. Changes mark the
 * hierarchy root dirty (it joins the engine's dirty-frame list) so matrices
 * and attached objects are synchronized lazily.
 */

#include "rwsdk/rwcore.h"

#define rwFRAME 0

/* privateFlags */
#define rwFRAMEPRIVATEHIERARCHYSYNCLTM 0x01
#define rwFRAMEPRIVATEHIERARCHYSYNCOBJ 0x02
#define rwFRAMEPRIVATESUBTREESYNCLTM 0x04
#define rwFRAMEPRIVATESUBTREESYNCOBJ 0x08

typedef struct rwFrameGlobals {
    RwFreeList* frameFreeList;
} rwFrameGlobals;

RwPluginRegistry frameTKList = { sizeof(RwFrame), sizeof(RwFrame), 0, 0, NULL, NULL };

RwModuleInfo frameModule;

#define FRAMEGLOBAL(var) (RWPLUGINOFFSET(rwFrameGlobals, RwEngineInstance, frameModule.globalsOffset)->var)

#define RwFrameGetParent(frame) ((RwFrame*)rwObjectGetParent(frame))

#define rwMatrixInitializeIdentity(m, t)                                \
    do {                                                                \
        (m)->flags = (t);                                               \
        (m)->right.x = (m)->up.y = (m)->at.z = (RwReal)1.0;             \
        (m)->right.y = (m)->right.z = (m)->up.x = (RwReal)0.0;          \
        (m)->up.z = (m)->at.x = (m)->at.y = (RwReal)0.0;                \
        (m)->pos.x = (m)->pos.y = (m)->pos.z = (RwReal)0.0;             \
        (m)->flags |= (rwMATRIXINTERNALIDENTITY | rwMATRIXTYPEORTHONORMAL); \
    } while (0)

/* Mark a frame (and its hierarchy) as needing synchronization. */
#define rwFrameUpdateObjects(frame)                                                         \
    do {                                                                                    \
        RwUInt8 oldFlags = rwObjectGetPrivateFlags((frame)->root);                          \
        if (!(oldFlags & (rwFRAMEPRIVATEHIERARCHYSYNCLTM | rwFRAMEPRIVATEHIERARCHYSYNCOBJ))) { \
            rwLinkListAddLLLink(&RWSRCGLOBAL(dirtyFrameList), &(frame)->root->inDirtyListLink); \
        }                                                                                   \
        rwObjectSetPrivateFlags((frame)->root, oldFlags | (rwFRAMEPRIVATEHIERARCHYSYNCLTM | \
                                                           rwFRAMEPRIVATEHIERARCHYSYNCOBJ)); \
        rwObjectSetPrivateFlags((frame), rwObjectGetPrivateFlags(frame) |                    \
                                             (rwFRAMEPRIVATESUBTREESYNCLTM |                 \
                                              rwFRAMEPRIVATESUBTREESYNCOBJ));                \
    } while (0)

void* _rwFrameOpen(void* instance, RwInt32 offset, RwInt32 size) {
    frameModule.globalsOffset = offset;

    FRAMEGLOBAL(frameFreeList) = RwFreeListCreate(frameTKList.sizeOfStruct, 50, sizeof(RwUInt32));
    if (!FRAMEGLOBAL(frameFreeList)) {
        return NULL;
    }

    rwLinkListInitialize(&RWSRCGLOBAL(dirtyFrameList));

    frameModule.numInstances++;

    return instance;
}

void* _rwFrameClose(void* instance, RwInt32 offset, RwInt32 size) {
    if (FRAMEGLOBAL(frameFreeList)) {
        RwFreeListDestroy(FRAMEGLOBAL(frameFreeList));
        FRAMEGLOBAL(frameFreeList) = NULL;
    }

    frameModule.numInstances--;

    return instance;
}

static void rwSetHierarchyRoot(RwFrame* frame, RwFrame* root) {
    frame->root = root;

    frame = frame->child;
    while (frame) {
        rwSetHierarchyRoot(frame, root);
        frame = frame->next;
    }
}

RwBool RwFrameDirty(const RwFrame* frame) {
    return rwObjectGetPrivateFlags(frame->root) & (rwFRAMEPRIVATEHIERARCHYSYNCLTM | rwFRAMEPRIVATEHIERARCHYSYNCOBJ);
}

RwFrame* RwFrameCreate(void) {
    RwFrame* frame;

    frame = (RwFrame*)RwFreeListAlloc(FRAMEGLOBAL(frameFreeList));
    if (!frame) {
        return NULL;
    }

    rwObjectInitialize(frame, rwFRAME, 0);
    rwLinkListInitialize(&frame->objectList);

    rwMatrixInitializeIdentity(&frame->modelling, rwMATRIXTYPEORTHONORMAL);
    rwMatrixInitializeIdentity(&frame->ltm, rwMATRIXTYPEORTHONORMAL);

    frame->child = NULL;
    frame->next = NULL;
    frame->root = frame;

    _rwPluginRegistryInitObject(&frameTKList, frame);

    return frame;
}

RwBool RwFrameDestroy(RwFrame* frame) {
    RwFrame* childFrame;

    _rwPluginRegistryDeInitObject(&frameTKList, frame);

    if (RwFrameGetParent(frame)) {
        RwFrameRemoveChild(frame);
    }

    if (rwObjectGetPrivateFlags(frame) & (rwFRAMEPRIVATEHIERARCHYSYNCLTM | rwFRAMEPRIVATEHIERARCHYSYNCOBJ)) {
        rwLLLinkRemove(&frame->inDirtyListLink);
    }

    childFrame = frame->child;
    while (childFrame) {
        rwObjectSetParent(childFrame, NULL);
        childFrame = childFrame->next;
    }

    RwFreeListFree(FRAMEGLOBAL(frameFreeList), frame);

    return TRUE;
}

static void FrameDestroyRecurseDeInitLeaf(RwFrame* frame) {
    _rwPluginRegistryDeInitObject(&frameTKList, frame);

    if (rwObjectGetPrivateFlags(frame) & (rwFRAMEPRIVATEHIERARCHYSYNCLTM | rwFRAMEPRIVATEHIERARCHYSYNCOBJ)) {
        rwLLLinkRemove(&frame->inDirtyListLink);
    }
}

static void rwFrameDestroyRecurseDestroyLeaf(RwFrame* frame) {
    FrameDestroyRecurseDeInitLeaf(frame);
    RwFreeListFree(FRAMEGLOBAL(frameFreeList), frame);
}

static void rwFrameDestroyRecurse(RwFrame* frame) {
    if (frame) {
        RwFrame* child = frame->child;

        while (child) {
            RwFrame* next = child->next;

            rwFrameDestroyRecurse(child);
            child = next;
        }

        rwFrameDestroyRecurseDestroyLeaf(frame);
    }
}

RwBool RwFrameDestroyHierarchy(RwFrame* frame) {
    rwFrameDestroyRecurse(frame);
    return TRUE;
}

RwFrame* RwFrameUpdateObjects(RwFrame* frame) {
    rwFrameUpdateObjects(frame);
    return frame;
}

RwMatrix* RwFrameGetLTM(RwFrame* frame) {
    if (rwObjectGetPrivateFlags(frame->root) & rwFRAMEPRIVATEHIERARCHYSYNCLTM) {
        _rwFrameSyncHierarchyLTM(frame->root);
    }

    return &frame->ltm;
}

RwFrame* RwFrameGetRoot(const RwFrame* frame) {
    return frame->root;
}

RwFrame* RwFrameAddChild(RwFrame* parent, RwFrame* child) {
    if (RwFrameGetParent(child)) {
        RwFrameRemoveChild(child);
    }

    child->next = parent->child;
    parent->child = child;
    rwObjectSetParent(child, parent);

    rwSetHierarchyRoot(child, parent->root);

    if (rwObjectGetPrivateFlags(child) & (rwFRAMEPRIVATEHIERARCHYSYNCLTM | rwFRAMEPRIVATEHIERARCHYSYNCOBJ)) {
        rwLLLinkRemove(&child->inDirtyListLink);
        rwObjectSetPrivateFlags(child, rwObjectGetPrivateFlags(child) &
                                           ~(rwFRAMEPRIVATEHIERARCHYSYNCLTM | rwFRAMEPRIVATEHIERARCHYSYNCOBJ));
    }

    rwFrameUpdateObjects(child);

    return parent;
}

RwFrame* RwFrameRemoveChild(RwFrame* child) {
    RwFrame* curFrame;

    curFrame = RwFrameGetParent(child)->child;
    if (curFrame == child) {
        RwFrameGetParent(child)->child = child->next;
    } else {
        while (curFrame->next != child) {
            curFrame = curFrame->next;
        }
        curFrame->next = child->next;
    }

    rwObjectSetParent(child, NULL);
    child->next = NULL;

    rwSetHierarchyRoot(child, child);

    rwFrameUpdateObjects(child);

    return child;
}

RwFrame* RwFrameForAllChildren(RwFrame* frame, RwFrameCallBack callBack, void* data) {
    RwFrame* curFrame = frame->child;

    while (curFrame) {
        RwFrame* next = curFrame->next;

        if (!callBack(curFrame, data)) {
            return frame;
        }

        curFrame = next;
    }

    return frame;
}

RwFrame* RwFrameTransform(RwFrame* frame, const RwMatrix* m, RwOpCombineType combine) {
    RwMatrixTransform(&frame->modelling, m, combine);
    rwFrameUpdateObjects(frame);
    return frame;
}

RwFrame* RwFrameRotate(RwFrame* frame, const RwV3d* axis, RwReal angle, RwOpCombineType combine) {
    RwMatrixRotate(&frame->modelling, axis, angle, combine);
    rwFrameUpdateObjects(frame);
    return frame;
}

RwFrame* RwFrameOrthoNormalize(RwFrame* frame) {
    RwMatrixOrthoNormalize(&frame->modelling, &frame->modelling);
    rwFrameUpdateObjects(frame);
    return frame;
}

RwFrame* RwFrameForAllObjects(RwFrame* frame, RwObjectCallBack callBack, void* data) {
    RwLLLink* current;
    RwLLLink* next;
    RwLLLink* end;

    current = rwLinkListGetFirstLLLink(&frame->objectList);
    end = rwLinkListGetTerminator(&frame->objectList);

    while (current != end) {
        RwObject* object = (RwObject*)rwLLLinkGetData(current, RwObjectHasFrame, lFrame);

        next = rwLLLinkGetNext(current);
        if (!callBack(object, data)) {
            return frame;
        }
        current = next;
    }

    return frame;
}

RwInt32 RwFrameRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return _rwPluginRegistryAddPlugin(&frameTKList, size, pluginID, constructCB, destructCB, copyCB);
}

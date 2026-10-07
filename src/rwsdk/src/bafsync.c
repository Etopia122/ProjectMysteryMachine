/*
 * Frame synchronisation: walk the dirty frame list, rebuild the LTMs of
 * changed hierarchies and tell the attached objects.
 */

#include "rwsdk/rwcore.h"

#define rwFRAMEPRIVATEHIERARCHYSYNCLTM 0x01
#define rwFRAMEPRIVATEHIERARCHYSYNCOBJ 0x02
#define rwFRAMEPRIVATESUBTREESYNCLTM 0x04
#define rwFRAMEPRIVATESUBTREESYNCOBJ 0x08

RwMatrix* RwMatrixMultiply(RwMatrix* matrixOut, const RwMatrix* matrixIn1, const RwMatrix* matrixIn2);

static RwObject* FrameSyncObject(RwObject* object, void* data) {
    ((RwObjectHasFrame*)object)->sync((RwObjectHasFrame*)object);

    return object;
}

static void FrameSyncHierarchyRecurse(RwFrame* frame, RwInt32 flags) {
    while (frame) {
        RwInt32 accumflags = flags | rwObjectGetPrivateFlags(frame);

        if (accumflags & rwFRAMEPRIVATESUBTREESYNCLTM) {
            RwMatrixMultiply(&frame->ltm, &frame->modelling, &((RwFrame*)rwObjectGetParent(frame))->ltm);
        }

        if (rwLinkListGetFirstLLLink(&frame->objectList) != rwLinkListGetTerminator(&frame->objectList)) {
            RwFrameForAllObjects(frame, FrameSyncObject, NULL);
        }

        rwObjectSetPrivateFlags(frame, rwObjectGetPrivateFlags(frame) &
                                           ~(rwFRAMEPRIVATESUBTREESYNCLTM | rwFRAMEPRIVATESUBTREESYNCOBJ));

        FrameSyncHierarchyRecurse(frame->child, accumflags);

        frame = frame->next;
    }
}

static void FrameSyncHierarchyRecurseNoLTM(RwFrame* frame) {
    while (frame) {
        if (rwLinkListGetFirstLLLink(&frame->objectList) != rwLinkListGetTerminator(&frame->objectList)) {
            RwFrameForAllObjects(frame, FrameSyncObject, NULL);
        }

        rwObjectSetPrivateFlags(frame, rwObjectGetPrivateFlags(frame) & ~rwFRAMEPRIVATESUBTREESYNCOBJ);

        FrameSyncHierarchyRecurseNoLTM(frame->child);

        frame = frame->next;
    }
}

static void FrameSyncDirtyRoot(RwFrame* frame) {
    RwUInt8 flags = rwObjectGetPrivateFlags(frame);

    if (flags & rwFRAMEPRIVATEHIERARCHYSYNCLTM) {
        RwUInt8 syncLTM = flags & rwFRAMEPRIVATESUBTREESYNCLTM;

        if (syncLTM) {
            frame->ltm = frame->modelling;
        }

        if (rwLinkListGetFirstLLLink(&frame->objectList) != rwLinkListGetTerminator(&frame->objectList)) {
            RwFrameForAllObjects(frame, FrameSyncObject, NULL);
        }

        FrameSyncHierarchyRecurse(frame->child, syncLTM);
    } else {
        if (rwLinkListGetFirstLLLink(&frame->objectList) != rwLinkListGetTerminator(&frame->objectList)) {
            RwFrameForAllObjects(frame, FrameSyncObject, NULL);
        }

        FrameSyncHierarchyRecurseNoLTM(frame->child);
    }

    rwObjectSetPrivateFlags(frame, flags & 0xF0);
}

RwBool _rwFrameSyncDirty(void) {
    RwLLLink* cur = rwLinkListGetFirstLLLink(&RWSRCGLOBAL(dirtyFrameList));
    RwLLLink* end = rwLinkListGetTerminator(&RWSRCGLOBAL(dirtyFrameList));

    while (cur != end) {
        FrameSyncDirtyRoot(rwLLLinkGetData(cur, RwFrame, inDirtyListLink));
        cur = rwLLLinkGetNext(cur);
    }

    rwLinkListInitialize(&RWSRCGLOBAL(dirtyFrameList));

    return TRUE;
}

static void FrameSyncHierarchyLTMRecurse(RwFrame* frame, RwInt32 flags) {
    while (frame) {
        RwInt32 accumflags = flags | rwObjectGetPrivateFlags(frame);

        if (accumflags & rwFRAMEPRIVATESUBTREESYNCLTM) {
            RwMatrixMultiply(&frame->ltm, &frame->modelling, &((RwFrame*)rwObjectGetParent(frame))->ltm);
            rwObjectSetPrivateFlags(frame, rwObjectGetPrivateFlags(frame) & ~rwFRAMEPRIVATESUBTREESYNCLTM);
        }

        FrameSyncHierarchyLTMRecurse(frame->child, accumflags);

        frame = frame->next;
    }
}

void _rwFrameSyncHierarchyLTM(RwFrame* frame) {
    RwUInt8 flags = rwObjectGetPrivateFlags(frame);

    if (flags & rwFRAMEPRIVATESUBTREESYNCLTM) {
        frame->ltm = frame->modelling;
    }

    FrameSyncHierarchyLTMRecurse(frame->child, flags);

    rwObjectSetPrivateFlags(frame, flags & ~(rwFRAMEPRIVATEHIERARCHYSYNCLTM | rwFRAMEPRIVATESUBTREESYNCLTM));
}

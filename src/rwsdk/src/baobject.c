/*
 * Objects that hang off a frame: attaching to and detaching from its list.
 */

#include "rwsdk/rwcore.h"

RwFrame* RwFrameUpdateObjects(RwFrame* frame);

void _rwObjectHasFrameSetFrame(void* object, RwFrame* frame) {
    RwObjectHasFrame* ohf = (RwObjectHasFrame*)object;

    if (rwObjectGetParent(ohf)) {
        rwLLLinkRemove(&ohf->lFrame);
    }

    rwObjectSetParent(ohf, frame);

    if (frame) {
        rwLinkListAddLLLink(&frame->objectList, &ohf->lFrame);
        RwFrameUpdateObjects(frame);
    }
}

void _rwObjectHasFrameReleaseFrame(void* object) {
    RwObjectHasFrame* ohf = (RwObjectHasFrame*)object;

    if (rwObjectGetParent(ohf)) {
        rwLLLinkRemove(&ohf->lFrame);
    }
}

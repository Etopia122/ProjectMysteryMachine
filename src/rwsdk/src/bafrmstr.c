/*
 * Frame streaming: a frame list chunk read back into a hierarchy.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 1 /* core */

typedef struct rwFrameList {
    RwFrame** frames;
    RwInt32 numFrames;
} rwFrameList;

typedef struct rwStreamFrame {
    RwV3d right;
    RwV3d up;
    RwV3d at;
    RwV3d pos;
    RwInt32 parentIndex;
    RwUInt32 data;
} rwStreamFrame;

extern RwPluginRegistry frameTKList;

RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length);
void* RwMemNative32(void* mem, RwUInt32 size);
void* RwMemFloat32ToReal(void* mem, RwUInt32 size);
RwInt32 _rwPluginRegistryAddPluginStream(RwPluginRegistry* reg, RwUInt32 pluginID,
                                         RwPluginDataChunkReadCallBack readCB,
                                         RwPluginDataChunkWriteCallBack writeCB,
                                         RwPluginDataChunkGetSizeCallBack getSizeCB);
const RwPluginRegistry* _rwPluginRegistryReadDataChunks(const RwPluginRegistry* reg, RwStream* stream,
                                                        void* object);
RwFrame* RwFrameCreate(void);
RwFrame* RwFrameAddChild(RwFrame* parent, RwFrame* child);
RwReal _rwMatrixNormalError(const RwMatrix* matrix);
RwReal _rwMatrixOrthogonalError(const RwMatrix* matrix);
RwReal _rwMatrixDeterminant(const RwMatrix* matrix);

RwInt32 RwFrameRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                    RwPluginDataChunkWriteCallBack writeCB,
                                    RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return _rwPluginRegistryAddPluginStream(&frameTKList, pluginID, readCB, writeCB, getSizeCB);
}

rwFrameList* _rwFrameListDeinitialize(rwFrameList* frameList) {
    if (frameList->numFrames) {
        RwFree(frameList->frames);
    }

    return frameList;
}

rwFrameList* _rwFrameListStreamRead(RwStream* stream, rwFrameList* frameList) {
    RwInt32 numFrames;
    RwUInt32 size;
    RwUInt32 version;
    RwInt32 i;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version)) {
        return NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION) {
        if (RwStreamRead(stream, &numFrames, sizeof(numFrames)) != sizeof(numFrames)) {
            return NULL;
        }

        RwMemNative32(&numFrames, sizeof(numFrames));
        frameList->numFrames = numFrames;

        frameList->frames = (RwFrame**)RwMalloc(numFrames * sizeof(RwFrame*));
        if (!frameList->frames) {
            RWERROR((E_RW_NOMEM, numFrames * sizeof(RwFrame*)));
            return NULL;
        }

        for (i = 0; i < numFrames; i++) {
            rwStreamFrame frameChunk;
            RwFrame* frame;
            RwMatrix* matrix;

            if (RwStreamRead(stream, &frameChunk, sizeof(frameChunk)) != sizeof(frameChunk)) {
                RwFree(frameList->frames);
                return NULL;
            }

            RwMemNative32(&frameChunk, sizeof(frameChunk));
            RwMemFloat32ToReal(&frameChunk.right, sizeof(frameChunk.right));
            RwMemFloat32ToReal(&frameChunk.up, sizeof(frameChunk.up));
            RwMemFloat32ToReal(&frameChunk.at, sizeof(frameChunk.at));
            RwMemFloat32ToReal(&frameChunk.pos, sizeof(frameChunk.pos));

            frame = RwFrameCreate();
            if (!frame) {
                RwFree(frameList->frames);
                return NULL;
            }

            matrix = &frame->modelling;
            matrix->right = frameChunk.right;
            matrix->up = frameChunk.up;
            matrix->at = frameChunk.at;
            matrix->pos = frameChunk.pos;

            if ((RwReal)0.01 >= _rwMatrixNormalError(matrix) && (RwReal)0.01 >= _rwMatrixOrthogonalError(matrix) &&
                (RwReal)0.99 <= _rwMatrixDeterminant(matrix)) {
                matrix->flags &= ~rwMATRIXINTERNALIDENTITY;
            } else {
                matrix->flags &= ~(rwMATRIXTYPEORTHONORMAL | rwMATRIXINTERNALIDENTITY);
            }

            frameList->frames[i] = frame;

            if (frameChunk.parentIndex >= 0) {
                RwFrameAddChild(frameList->frames[frameChunk.parentIndex], frame);
            }
        }
    } else {
        RWERROR((E_RW_BADVERSION));
        return NULL;
    }

    for (i = 0; i < numFrames; i++) {
        if (!_rwPluginRegistryReadDataChunks(&frameTKList, stream, frameList->frames[i])) {
            return NULL;
        }
    }

    return frameList;
}

/*
 * Camera streaming: the camera chunk read back into a new camera.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 1 /* core */

typedef struct RwCamera RwCamera;

typedef struct RwV2d {
    RwReal x;
    RwReal y;
} RwV2d;

typedef struct rwStreamCamera {
    RwV2d viewWindow;
    RwV2d viewOffset;
    RwReal nearPlane;
    RwReal farPlane;
    RwReal fogPlane;
    RwUInt32 projection;
} rwStreamCamera;

/* Only the fog plane is touched directly; everything else goes through the API. */
#define RwCameraSetFogDistance(camera, distance) (*(RwReal*)((RwUInt8*)(camera) + 0x88) = (distance))

extern RwPluginRegistry cameraTKList;

void* memset(void* dst, int c, unsigned long n);
RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length);
void* RwMemNative32(void* mem, RwUInt32 size);
void* RwMemFloat32ToReal(void* mem, RwUInt32 size);
RwInt32 _rwPluginRegistryAddPluginStream(RwPluginRegistry* reg, RwUInt32 pluginID,
                                         RwPluginDataChunkReadCallBack readCB,
                                         RwPluginDataChunkWriteCallBack writeCB,
                                         RwPluginDataChunkGetSizeCallBack getSizeCB);
const RwPluginRegistry* _rwPluginRegistryReadDataChunks(const RwPluginRegistry* reg, RwStream* stream,
                                                        void* object);
RwCamera* RwCameraCreate(void);
RwCamera* RwCameraSetViewWindow(RwCamera* camera, const RwV2d* viewWindow);
RwCamera* RwCameraSetViewOffset(RwCamera* camera, const RwV2d* offset);
RwCamera* RwCameraSetNearClipPlane(RwCamera* camera, RwReal nearClip);
RwCamera* RwCameraSetFarClipPlane(RwCamera* camera, RwReal farClip);
RwCamera* RwCameraSetProjection(RwCamera* camera, RwUInt32 projection);

RwInt32 RwCameraRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                     RwPluginDataChunkWriteCallBack writeCB,
                                     RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return _rwPluginRegistryAddPluginStream(&cameraTKList, pluginID, readCB, writeCB, getSizeCB);
}

RwCamera* RwCameraStreamRead(RwStream* stream) {
    RwUInt32 size;
    RwUInt32 version;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version)) {
        return NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION) {
        rwStreamCamera cam;
        RwCamera* camera;

        memset(&cam, 0, sizeof(cam));
        if (RwStreamRead(stream, &cam, size) != size) {
            return NULL;
        }

        RwMemNative32(&cam, sizeof(cam));
        RwMemFloat32ToReal(&cam.viewWindow, sizeof(cam.viewWindow));
        RwMemFloat32ToReal(&cam.viewOffset, sizeof(cam.viewOffset));
        RwMemFloat32ToReal(&cam.nearPlane, sizeof(cam.nearPlane));
        RwMemFloat32ToReal(&cam.farPlane, sizeof(cam.farPlane));
        RwMemFloat32ToReal(&cam.fogPlane, sizeof(cam.fogPlane));

        camera = RwCameraCreate();
        if (!camera) {
            return NULL;
        }

        if (!_rwPluginRegistryReadDataChunks(&cameraTKList, stream, camera)) {
            return NULL;
        }

        RwCameraSetViewWindow(camera, &cam.viewWindow);
        RwCameraSetViewOffset(camera, &cam.viewOffset);
        RwCameraSetNearClipPlane(camera, cam.nearPlane);
        RwCameraSetFarClipPlane(camera, cam.farPlane);
        RwCameraSetFogDistance(camera, cam.fogPlane);
        RwCameraSetProjection(camera, cam.projection);

        return camera;
    }

    RWERROR((E_RW_BADVERSION));
    return NULL;
}

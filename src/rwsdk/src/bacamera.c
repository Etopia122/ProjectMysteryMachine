/*
 * Cameras: projection setup, view matrices and the world space view frustum
 * (corners, clip planes and bounding box) rebuilt whenever the frame syncs.
 */

#include "rwsdk/rpworld_int.h"

#define rwPLUGIN_ID 1 /* core */

#define E_RW_BADPARAM 0x80000003

#define rwCAMERA 4

#define rwPERSPECTIVE 1
#define rwPARALLEL 2

#define rwSPHEREOUTSIDE 0
#define rwSPHEREBOUNDARY 1
#define rwSPHEREINSIDE 2

#define rwSTANDARDCAMERABEGINUPDATE 1
#define rwSTANDARDCAMERAENDUPDATE 10
#define rwSTANDARDCAMERACLEAR 21

typedef struct rwCameraGlobals {
    RwFreeList* cameraFreeList;
} rwCameraGlobals;

#define RWCAMERAGLOBAL(var) (RWPLUGINOFFSET(rwCameraGlobals, RwEngineInstance, cameraModule.globalsOffset)->var)

/* 1 when the float's sign bit is clear, 0 when set. */
#define FLOATPOSITIVE(f) ((RwUInt8)((*(RwInt32*)&(f) >> 31) + 1))

#define FRUSTUMPLANECLOSEST(fp)                                                                                        \
    do {                                                                                                               \
        (fp)->closestX = FLOATPOSITIVE((fp)->plane.normal.x);                                                          \
        (fp)->closestY = FLOATPOSITIVE((fp)->plane.normal.y);                                                          \
        (fp)->closestZ = FLOATPOSITIVE((fp)->plane.normal.z);                                                          \
    } while (0)

#define V3DDOT(a, b) ((a)->x * (b)->x + (a)->y * (b)->y + (a)->z * (b)->z)

#define V3DCROSS(o, a, b)                                                                                              \
    do {                                                                                                               \
        (o)->x = (a)->y * (b)->z - (a)->z * (b)->y;                                                                    \
        (o)->y = (a)->z * (b)->x - (a)->x * (b)->z;                                                                    \
        (o)->z = (a)->x * (b)->y - (a)->y * (b)->x;                                                                    \
    } while (0)

#define V3DSUB(o, a, b)                                                                                                \
    do {                                                                                                               \
        (o)->x = (a)->x - (b)->x;                                                                                      \
        (o)->y = (a)->y - (b)->y;                                                                                      \
        (o)->z = (a)->z - (b)->z;                                                                                      \
    } while (0)

#define V3DADD(o, a, b)                                                                                                \
    do {                                                                                                               \
        (o)->x = (a)->x + (b)->x;                                                                                      \
        (o)->y = (a)->y + (b)->y;                                                                                      \
        (o)->z = (a)->z + (b)->z;                                                                                      \
    } while (0)

#define V3DSCALE(o, a, s)                                                                                              \
    do {                                                                                                               \
        (o)->x = (a)->x * (s);                                                                                         \
        (o)->y = (a)->y * (s);                                                                                         \
        (o)->z = (a)->z * (s);                                                                                         \
    } while (0)

#define V3DINCREMENTSCALED(o, a, s)                                                                                    \
    do {                                                                                                               \
        (o)->x += (a)->x * (s);                                                                                        \
        (o)->y += (a)->y * (s);                                                                                        \
        (o)->z += (a)->z * (s);                                                                                        \
    } while (0)

#define V3DNEGATE(o, a)                                                                                                \
    do {                                                                                                               \
        (o)->x = -(a)->x;                                                                                              \
        (o)->y = -(a)->y;                                                                                              \
        (o)->z = -(a)->z;                                                                                              \
    } while (0)

#define V3DNORMALIZE(v)                                                                                                \
    do {                                                                                                               \
        RwReal _recip = _rwInvSqrt(V3DDOT(v, v));                                                                      \
        (v)->x *= _recip;                                                                                              \
        (v)->y *= _recip;                                                                                              \
        (v)->z *= _recip;                                                                                              \
    } while (0)

RwReal _rwInvSqrt(const RwReal num);
RwV3d* RwV3dTransformPoints(RwV3d* pointsOut, const RwV3d* pointsIn, RwInt32 numPoints, const RwMatrix* matrix);
void RwMatrixOptimize(RwMatrix* matrix, const void* tolerance);
RwBBox* RwBBoxCalculate(RwBBox* boundBox, const RwV3d* verts, RwInt32 numVerts);
RwFrame* RwFrameUpdateObjects(RwFrame* frame);
void _rwFrameSyncDirty(void);
void _rwPipeInitForCamera(RwCamera* camera);
void _rwObjectHasFrameReleaseFrame(void* object);
RwRaster* RwRasterShowRaster(RwRaster* raster, void* dev, RwUInt32 flags);
RwReal RwIm2DGetNearScreenZ(void);
RwReal RwIm2DGetFarScreenZ(void);

RwPluginRegistry cameraTKList = { sizeof(RwCamera), sizeof(RwCamera), 0, 0, NULL, NULL };

RwModuleInfo cameraModule;

#define RwCameraGetFrame(camera) ((RwFrame*)rwObjectGetParent(camera))

static inline void CameraUpdateZShiftScale(RwCamera* camera)
{
    RwReal nearScreenZ = RwIm2DGetNearScreenZ();
    RwReal farScreenZ = RwIm2DGetFarScreenZ();
    RwReal nearZ, farZ;
    RwReal zEpsilon;
    RwReal zScale;

    switch (camera->projectionType) {
    case rwPARALLEL:
        farZ = camera->farPlane;
        nearZ = camera->nearPlane;
        break;
    case rwPERSPECTIVE:
    default:
        farZ = 1.0f / camera->farPlane;
        nearZ = 1.0f / camera->nearPlane;
        break;
    }

    zEpsilon = 0.0001f * (farScreenZ - nearScreenZ);
    farScreenZ -= zEpsilon;
    nearScreenZ += zEpsilon;

    zScale = (farScreenZ - nearScreenZ) / (farZ - nearZ);
    camera->zScale = zScale;
    camera->zShift = 0.5f * ((farScreenZ + nearScreenZ) - zScale * (farZ + nearZ));
}

void CameraBuildPerspClipPlanes(RwCamera* camera)
{
    const RwMatrix* cameraLTM = &RwCameraGetFrame(camera)->ltm;
    RwV3d* corners = camera->frustumCorners;
    RwFrustumPlane* frustumPlane;
    RwV3d vTemp;
    RwV3d vRight;
    RwV3d vUp;
    RwV3d vRight2;
    RwV3d vUp2;
    RwV3d vOffset;
    RwV3d vTemp2;
    RwV3d* nearCorner;
    RwV3d* farCorner;
    RwInt32 i;

    V3DSCALE(&vRight, &cameraLTM->right, camera->viewWindow.x);
    V3DSCALE(&vUp, &cameraLTM->up, camera->viewWindow.y);
    V3DSCALE(&vRight2, &vRight, 2.0f);
    V3DSCALE(&vUp2, &vUp, 2.0f);

    V3DSCALE(&vOffset, &cameraLTM->right, -camera->viewOffset.x);
    V3DINCREMENTSCALED(&vOffset, &cameraLTM->up, camera->viewOffset.y);

    V3DADD(&vTemp, &cameraLTM->at, &vRight);
    V3DADD(&vTemp, &vTemp, &vUp);
    corners[0] = vTemp;
    V3DSUB(&vTemp, &vTemp, &vRight2);
    corners[1] = vTemp;
    V3DSUB(&vTemp, &vTemp, &vUp2);
    corners[2] = vTemp;
    V3DADD(&vTemp, &vTemp, &vRight2);
    corners[3] = vTemp;

    nearCorner = corners;
    for (i = 0; i < 4; i++) {
        farCorner = &corners[i + 4];

        V3DSUB(&vTemp, nearCorner, &vOffset);
        V3DADD(nearCorner, &vOffset, &cameraLTM->pos);
        V3DINCREMENTSCALED(nearCorner, &vTemp, camera->nearPlane);
        nearCorner++;
        V3DADD(farCorner, &vOffset, &cameraLTM->pos);
        V3DINCREMENTSCALED(farCorner, &vTemp, camera->farPlane);
    }

    /* Far plane */
    frustumPlane = &camera->frustumPlanes[0];
    frustumPlane->plane.normal = cameraLTM->at;
    frustumPlane->plane.distance = V3DDOT(&corners[4], &cameraLTM->at);
    FRUSTUMPLANECLOSEST(frustumPlane);

    /* Near plane */
    frustumPlane++;
    V3DNEGATE(&frustumPlane->plane.normal, &camera->frustumPlanes[0].plane.normal);
    frustumPlane->plane.distance = V3DDOT(&corners[0], &frustumPlane->plane.normal);
    FRUSTUMPLANECLOSEST(frustumPlane);

    frustumPlane++;
    V3DSUB(&vTemp, &corners[1], &corners[5]);
    V3DSUB(&vTemp2, &corners[6], &corners[5]);
    V3DCROSS(&frustumPlane->plane.normal, &vTemp, &vTemp2);
    V3DNORMALIZE(&frustumPlane->plane.normal);
    frustumPlane->plane.distance = V3DDOT(&corners[1], &frustumPlane->plane.normal);
    FRUSTUMPLANECLOSEST(frustumPlane);

    frustumPlane++;
    V3DSUB(&vTemp2, &corners[4], &corners[5]);
    V3DCROSS(&frustumPlane->plane.normal, &vTemp2, &vTemp);
    V3DNORMALIZE(&frustumPlane->plane.normal);
    frustumPlane->plane.distance = V3DDOT(&corners[1], &frustumPlane->plane.normal);
    FRUSTUMPLANECLOSEST(frustumPlane);

    frustumPlane++;
    V3DSUB(&vTemp, &corners[3], &corners[7]);
    V3DSUB(&vTemp2, &corners[4], &corners[7]);
    V3DCROSS(&frustumPlane->plane.normal, &vTemp, &vTemp2);
    V3DNORMALIZE(&frustumPlane->plane.normal);
    frustumPlane->plane.distance = V3DDOT(&corners[3], &frustumPlane->plane.normal);
    FRUSTUMPLANECLOSEST(frustumPlane);

    frustumPlane++;
    V3DSUB(&vTemp2, &corners[6], &corners[7]);
    V3DCROSS(&frustumPlane->plane.normal, &vTemp2, &vTemp);
    V3DNORMALIZE(&frustumPlane->plane.normal);
    frustumPlane->plane.distance = V3DDOT(&corners[3], &frustumPlane->plane.normal);
    FRUSTUMPLANECLOSEST(frustumPlane);
}

RwCamera* CameraBuildPerspViewMatrix(RwCamera* camera)
{
    RwMatrix* viewMatrix = &camera->viewMatrix;
    const RwMatrix* cameraLTM = &RwCameraGetFrame(camera)->ltm;
    RwReal scale;
    RwReal offset;
    RwV3d col;

    scale = -0.5f * camera->recipViewWindow.x;
    V3DSCALE(&col, &cameraLTM->right, scale);
    offset = 0.5f - scale * camera->viewOffset.x;
    V3DINCREMENTSCALED(&col, &cameraLTM->at, offset);
    viewMatrix->right.x = col.x;
    viewMatrix->up.x = col.y;
    viewMatrix->at.x = col.z;
    viewMatrix->pos.x = 0.5f - (V3DDOT(&cameraLTM->pos, &col) + offset);

    scale = -0.5f * camera->recipViewWindow.y;
    V3DSCALE(&col, &cameraLTM->up, scale);
    offset = 0.5f + scale * camera->viewOffset.y;
    V3DINCREMENTSCALED(&col, &cameraLTM->at, offset);
    viewMatrix->right.y = col.x;
    viewMatrix->up.y = col.y;
    viewMatrix->at.y = col.z;
    viewMatrix->pos.y = 0.5f - (V3DDOT(&cameraLTM->pos, &col) + offset);

    viewMatrix->right.z = cameraLTM->at.x;
    viewMatrix->up.z = cameraLTM->at.y;
    viewMatrix->at.z = cameraLTM->at.z;
    viewMatrix->pos.z = -V3DDOT(&cameraLTM->pos, &cameraLTM->at);

    RwMatrixOptimize(viewMatrix, NULL);
    return camera;
}

void CameraBuildParallelClipPlanes(RwCamera* camera)
{
    const RwMatrix* cameraLTM = &RwCameraGetFrame(camera)->ltm;
    RwV3d* corners = camera->frustumCorners;
    RwFrustumPlane* frustumPlane;
    RwV3d vTemp1, vTemp2;
    RwReal vwY;
    RwReal farZ;
    RwReal nearOffsetX;
    RwReal farOffsetY;
    RwReal nearZ;
    RwReal vwX;
    RwReal nearOffsetY;
    RwReal farOffsetX;

    vwX = camera->viewWindow.x;
    vwY = camera->viewWindow.y;
    nearZ = camera->nearPlane;
    farZ = camera->farPlane;
    nearOffsetX = -camera->viewOffset.x * (1.0f - nearZ);
    nearOffsetY = (1.0f - nearZ) * camera->viewOffset.y;
    farOffsetX = (1.0f - farZ) * -camera->viewOffset.x;
    farOffsetY = (1.0f - farZ) * camera->viewOffset.y;

    corners[0].z = corners[1].z = corners[2].z = corners[3].z = nearZ;
    corners[4].z = corners[5].z = corners[6].z = corners[7].z = farZ;

    corners[0].x = corners[3].x = vwX + nearOffsetX;
    corners[1].x = corners[2].x = -vwX + nearOffsetX;
    corners[4].x = corners[7].x = vwX + farOffsetX;
    corners[5].x = corners[6].x = -vwX + farOffsetX;

    corners[0].y = corners[1].y = vwY + nearOffsetY;
    corners[2].y = corners[3].y = -vwY + nearOffsetY;
    corners[4].y = corners[5].y = vwY + farOffsetY;
    corners[6].y = corners[7].y = -vwY + farOffsetY;

    RwV3dTransformPoints(corners, corners, 8, cameraLTM);

    /* Far plane */
    frustumPlane = &camera->frustumPlanes[0];
    frustumPlane->plane.normal = cameraLTM->at;
    frustumPlane->plane.distance = V3DDOT(&corners[4], &frustumPlane->plane.normal);
    FRUSTUMPLANECLOSEST(frustumPlane);

    /* Near plane */
    frustumPlane++;
    V3DNEGATE(&frustumPlane->plane.normal, &camera->frustumPlanes[0].plane.normal);
    frustumPlane->plane.distance = V3DDOT(&corners[0], &frustumPlane->plane.normal);
    FRUSTUMPLANECLOSEST(frustumPlane);

    frustumPlane++;
    V3DSUB(&vTemp1, &corners[1], &corners[5]);
    V3DSUB(&vTemp2, &corners[6], &corners[5]);
    V3DCROSS(&frustumPlane->plane.normal, &vTemp1, &vTemp2);
    V3DNORMALIZE(&frustumPlane->plane.normal);
    frustumPlane->plane.distance = V3DDOT(&corners[1], &frustumPlane->plane.normal);
    FRUSTUMPLANECLOSEST(frustumPlane);

    frustumPlane++;
    V3DSUB(&vTemp2, &corners[4], &corners[5]);
    V3DCROSS(&frustumPlane->plane.normal, &vTemp2, &vTemp1);
    V3DNORMALIZE(&frustumPlane->plane.normal);
    frustumPlane->plane.distance = V3DDOT(&corners[1], &frustumPlane->plane.normal);
    FRUSTUMPLANECLOSEST(frustumPlane);

    frustumPlane++;
    V3DNEGATE(&frustumPlane->plane.normal, &camera->frustumPlanes[2].plane.normal);
    frustumPlane->plane.distance = V3DDOT(&corners[3], &frustumPlane->plane.normal);
    FRUSTUMPLANECLOSEST(frustumPlane);

    frustumPlane++;
    V3DNEGATE(&frustumPlane->plane.normal, &camera->frustumPlanes[3].plane.normal);
    frustumPlane->plane.distance = V3DDOT(&corners[3], &frustumPlane->plane.normal);
    FRUSTUMPLANECLOSEST(frustumPlane);
}

RwCamera* CameraBuildParallelViewMatrix(RwCamera* camera)
{
    RwMatrix* viewMatrix = &camera->viewMatrix;
    const RwMatrix* cameraLTM = &RwCameraGetFrame(camera)->ltm;
    RwReal scale;
    RwReal offset;
    RwV3d col;

    scale = -0.5f * camera->recipViewWindow.x;
    V3DSCALE(&col, &cameraLTM->right, scale);
    offset = -(scale * camera->viewOffset.x);
    V3DINCREMENTSCALED(&col, &cameraLTM->at, offset);
    viewMatrix->right.x = col.x;
    viewMatrix->up.x = col.y;
    viewMatrix->at.x = col.z;
    viewMatrix->pos.x = 0.5f - (V3DDOT(&cameraLTM->pos, &col) + offset);

    scale = -0.5f * camera->recipViewWindow.y;
    V3DSCALE(&col, &cameraLTM->up, scale);
    offset = scale * camera->viewOffset.y;
    V3DINCREMENTSCALED(&col, &cameraLTM->at, offset);
    viewMatrix->right.y = col.x;
    viewMatrix->up.y = col.y;
    viewMatrix->at.y = col.z;
    viewMatrix->pos.y = 0.5f - (V3DDOT(&cameraLTM->pos, &col) + offset);

    viewMatrix->right.z = cameraLTM->at.x;
    viewMatrix->up.z = cameraLTM->at.y;
    viewMatrix->at.z = cameraLTM->at.z;
    viewMatrix->pos.z = -V3DDOT(&cameraLTM->pos, &cameraLTM->at);

    RwMatrixOptimize(viewMatrix, NULL);
    return camera;
}

RwObjectHasFrame* CameraSync(RwObjectHasFrame* object)
{
    RwCamera* camera = (RwCamera*)object;

    if (camera->projectionType == rwPERSPECTIVE) {
        CameraBuildPerspViewMatrix(camera);
        CameraBuildPerspClipPlanes(camera);
    } else {
        CameraBuildParallelViewMatrix(camera);
        CameraBuildParallelClipPlanes(camera);
    }

    RwBBoxCalculate(&camera->frustumBoundBox, camera->frustumCorners, 8);
    return object;
}

RwCamera* CameraEndUpdate(RwCamera* camera)
{
    if (RWSRCGLOBAL(stdFunc[rwSTANDARDCAMERAENDUPDATE])(NULL, camera, 0)) {
        RWSRCGLOBAL(curCamera) = NULL;
        return camera;
    }
    return NULL;
}

RwCamera* CameraBeginUpdate(RwCamera* camera)
{
    RWSRCGLOBAL(curCamera) = camera;
    _rwFrameSyncDirty();

    if (RWSRCGLOBAL(stdFunc[rwSTANDARDCAMERABEGINUPDATE])(NULL, camera, 0)) {
        _rwPipeInitForCamera(camera);
        return camera;
    }
    return NULL;
}

void* _rwCameraClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (RWCAMERAGLOBAL(cameraFreeList)) {
        RwFreeListDestroy(RWCAMERAGLOBAL(cameraFreeList));
        RWCAMERAGLOBAL(cameraFreeList) = NULL;
    }

    cameraModule.numInstances--;
    return instance;
}

void* _rwCameraOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    cameraModule.globalsOffset = offset;

    RWCAMERAGLOBAL(cameraFreeList) = RwFreeListCreate(cameraTKList.sizeOfStruct, 4, 4);
    if (!RWCAMERAGLOBAL(cameraFreeList)) {
        return NULL;
    }

    cameraModule.numInstances++;
    return instance;
}

RwCamera* RwCameraEndUpdate(RwCamera* camera)
{
    return camera->endUpdate(camera);
}

RwCamera* RwCameraBeginUpdate(RwCamera* camera)
{
    return camera->beginUpdate(camera);
}

RwCamera* RwCameraSetViewOffset(RwCamera* camera, const RwV2d* offset)
{
    camera->viewOffset = *offset;

    if (RwCameraGetFrame(camera)) {
        RwFrameUpdateObjects(RwCameraGetFrame(camera));
    }
    return camera;
}

RwCamera* RwCameraSetNearClipPlane(RwCamera* camera, RwReal nearClip)
{
    camera->nearPlane = nearClip;
    CameraUpdateZShiftScale(camera);

    if (RwCameraGetFrame(camera)) {
        RwFrameUpdateObjects(RwCameraGetFrame(camera));
    }
    return camera;
}

RwCamera* RwCameraSetFarClipPlane(RwCamera* camera, RwReal farClip)
{
    camera->farPlane = farClip;
    CameraUpdateZShiftScale(camera);

    if (RwCameraGetFrame(camera)) {
        RwFrameUpdateObjects(RwCameraGetFrame(camera));
    }
    return camera;
}

RwInt32 RwCameraFrustumTestSphere(const RwCamera* camera, const RwSphere* sphere)
{
    RwInt32 result = rwSPHEREINSIDE;
    const RwFrustumPlane* frustumPlane = camera->frustumPlanes;
    RwInt32 numPlanes = 6;

    while (numPlanes--) {
        RwReal dot = V3DDOT(&sphere->center, &frustumPlane->plane.normal) - frustumPlane->plane.distance;

        if (dot > sphere->radius) {
            return rwSPHEREOUTSIDE;
        }
        if (dot > -sphere->radius) {
            result = rwSPHEREBOUNDARY;
        }
        frustumPlane++;
    }
    return result;
}

RwCamera* RwCameraClear(RwCamera* camera, RwRGBA* colour, RwInt32 clearMode)
{
    if (RWSRCGLOBAL(stdFunc[rwSTANDARDCAMERACLEAR])(camera, colour, clearMode)) {
        return camera;
    }
    return NULL;
}

RwCamera* RwCameraShowRaster(RwCamera* camera, void* dev, RwUInt32 flags)
{
    if (RwRasterShowRaster(camera->frameBuffer, dev, flags)) {
        return camera;
    }
    return NULL;
}

RwCamera* RwCameraSetProjection(RwCamera* camera, RwInt32 projection)
{
    switch (projection) {
    case rwPERSPECTIVE:
    case rwPARALLEL:
        camera->projectionType = projection;
        if (RwCameraGetFrame(camera)) {
            RwFrameUpdateObjects(RwCameraGetFrame(camera));
        }
        CameraUpdateZShiftScale(camera);
        return camera;
    }

    RWERROR((E_RW_BADPARAM, "Invalid projection type specified"));
    return NULL;
}

RwCamera* RwCameraSetViewWindow(RwCamera* camera, const RwV2d* viewWindow)
{
    camera->viewWindow = *viewWindow;
    camera->recipViewWindow.x = 1.0f / camera->viewWindow.x;
    camera->recipViewWindow.y = 1.0f / camera->viewWindow.y;

    if (RwCameraGetFrame(camera)) {
        RwFrameUpdateObjects(RwCameraGetFrame(camera));
    }
    return camera;
}

RwInt32 RwCameraRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                               RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    return _rwPluginRegistryAddPlugin(&cameraTKList, size, pluginID, constructCB, destructCB, copyCB);
}

RwBool RwCameraDestroy(RwCamera* camera)
{
    _rwPluginRegistryDeInitObject(&cameraTKList, camera);
    _rwObjectHasFrameReleaseFrame(camera);
    RwFreeListFree(RWCAMERAGLOBAL(cameraFreeList), camera);
    return TRUE;
}

RwCamera* RwCameraCreate(void)
{
    RwCamera* camera = (RwCamera*)RwFreeListAlloc(RWCAMERAGLOBAL(cameraFreeList));

    if (camera == NULL) {
        return NULL;
    }

    rwObjectInitialize(camera, rwCAMERA, 0);
    camera->object.sync = CameraSync;
    camera->beginUpdate = CameraBeginUpdate;
    camera->endUpdate = CameraEndUpdate;

    camera->viewWindow.x = camera->viewWindow.y = 1.0f;
    camera->recipViewWindow.x = camera->recipViewWindow.y = 1.0f;
    camera->viewOffset.x = camera->viewOffset.y = 0.0f;
    camera->nearPlane = 0.05f;
    camera->farPlane = 10.0f;
    camera->fogPlane = 5.0f;
    camera->frameBuffer = NULL;
    camera->zBuffer = NULL;
    camera->projectionType = rwPERSPECTIVE;

    CameraUpdateZShiftScale(camera);

    camera->viewMatrix.flags = 0;

    _rwPluginRegistryInitObject(&cameraTKList, camera);
    return camera;
}

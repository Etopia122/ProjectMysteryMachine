#include "iCamera.h"
#include "iTime.h"
#include "zGlobals.h"
#include "rwtypes.h"

extern "C" void* memcpy(void* dst, const void* src, unsigned int n);

class RwRaster;
class RpWorld;

class RwV2d
{
public:
    float x; // 0x0
    float y; // 0x4
};

class RwCamera
{
public:
    RwObject object;                     // 0x0
    unsigned char pad8[0x60 - 0x8];
    RwRaster* frameBuffer;               // 0x60
    RwRaster* zBuffer;                   // 0x64
    unsigned char pad68[0x88 - 0x68];
    float fogPlane;                      // 0x88
    unsigned char pad8C[0x98 - 0x8C];
    float frustumPlanes[6][5];           // 0x98
};

class iEnv
{
public:
    RpWorld* world; // 0x0
};

extern "C" {
signed int RwRenderStateSet(signed int state, void* value);
RwCamera* RwCameraCreate();
signed int RwCameraDestroy(RwCamera* camera);
RwCamera* RwCameraClear(RwCamera* camera, RwRGBA* colour, signed int clearMode);
RwCamera* RwCameraBeginUpdate(RwCamera* camera);
RwCamera* RwCameraEndUpdate(RwCamera* camera);
RwCamera* RwCameraShowRaster(RwCamera* camera, void* dev, unsigned int flags);
RwCamera* RwCameraSetNearClipPlane(RwCamera* camera, float nearClip);
RwCamera* RwCameraSetFarClipPlane(RwCamera* camera, float farClip);
RwCamera* RwCameraSetViewWindow(RwCamera* camera, const RwV2d* viewWindow);
RwRaster* RwRasterCreate(signed int width, signed int height, signed int depth, signed int flags);
signed int RwRasterDestroy(RwRaster* raster);
RwFrame* RwFrameCreate();
signed int RwFrameDestroy(RwFrame* frame);
RwFrame* RwFrameOrthoNormalize(RwFrame* frame);
RwFrame* RwFrameUpdateObjects(RwFrame* frame);
RwMatrixTag* RwMatrixUpdate(RwMatrixTag* matrix);
void* _rwObjectHasFrameSetFrame(void* object, RwFrame* frame);
signed int _rwFrameSyncDirty();
RpWorld* RpWorldAddCamera(RpWorld* world, RwCamera* camera);
RpWorld* RpWorldRemoveCamera(RpWorld* world, RwCamera* camera);
double tan(double x);
// TODO: unidentified (iScrFX: draws pending black frames)
void fn_800149F0();
}

void xShadowSetWorld(RpWorld* world);
float iTimeDiffSec(iTime t0, iTime t1);

extern RwCamera* globalCamera;

#ifdef PC_PORT
// The GameCube OS keeps the bus clock in its globals page; ports can't map it.
#define GET_BUS_FREQUENCY() (162000000u)
#else
#define GET_BUS_FREQUENCY() (*(unsigned int*)0x800000F8)
#endif
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define RWRGBALONG(r, g, b, a)                                                                  \
    ((unsigned int)(((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))

float sCameraNearClip = 0.2f;
float sCameraFarClip = 400.0f;

// Functions are in reverse DOL order (-inline deferred emits last to first).

RwCamera* iCameraCreate(signed int width, signed int height)
{
    RwV2d vw;
    RwCamera* camera = RwCameraCreate();

    _rwObjectHasFrameSetFrame(camera, RwFrameCreate());
    camera->frameBuffer = RwRasterCreate(width, height, 0, 2);
    camera->zBuffer = RwRasterCreate(width, height, 0, 1);
    RwCameraSetFarClipPlane(camera, sCameraFarClip);
    RwCameraSetNearClipPlane(camera, sCameraNearClip);
    vw.x = 1.0f;
    vw.y = 1.0f;
    RwCameraSetViewWindow(camera, &vw);
    return camera;
}

void iCameraDestroy(RwCamera* camera)
{
    _rwFrameSyncDirty();
    if (camera)
    {
        RwFrame* frame = (RwFrame*)camera->object.parent;
        if (frame)
        {
            _rwObjectHasFrameSetFrame(camera, 0);
            RwFrameDestroy(frame);
        }
        if (camera->frameBuffer)
        {
            RwRasterDestroy(camera->frameBuffer);
            camera->frameBuffer = 0;
        }
        if (camera->zBuffer)
        {
            RwRasterDestroy(camera->zBuffer);
            camera->zBuffer = 0;
        }
        RwCameraDestroy(camera);
    }
}

void iCameraBegin(RwCamera* cam, signed int clear)
{
    if (clear)
    {
        if (globals.fog.type)
        {
            RwCameraClear(cam, &globals.fog.bgcolor, 3);
        }
        else
        {
            RwCameraClear(cam, 0, 2);
        }
    }
    RwCameraSetNearClipPlane(cam, sCameraNearClip);
    RwCameraBeginUpdate(cam);
}

void iCameraEnd(RwCamera* cam)
{
    fn_800149F0();
    RwCameraEndUpdate(cam);
}

void iCameraShowRaster(RwCamera* cam)
{
    RwCameraShowRaster(cam, 0, 0);
}

void iCameraFrustumPlanes(RwCamera* cam, _xVec4* frustplane)
{
    frustplane[0].x = cam->frustumPlanes[2][0];
    frustplane[1].x = cam->frustumPlanes[2][1];
    frustplane[2].x = cam->frustumPlanes[2][2];
    frustplane[3].x = cam->frustumPlanes[2][3];
    frustplane[0].y = cam->frustumPlanes[4][0];
    frustplane[1].y = cam->frustumPlanes[4][1];
    frustplane[2].y = cam->frustumPlanes[4][2];
    frustplane[3].y = cam->frustumPlanes[4][3];
    frustplane[0].z = cam->frustumPlanes[5][0];
    frustplane[1].z = cam->frustumPlanes[5][1];
    frustplane[2].z = cam->frustumPlanes[5][2];
    frustplane[3].z = cam->frustumPlanes[5][3];
    frustplane[0].w = cam->frustumPlanes[3][0];
    frustplane[1].w = cam->frustumPlanes[3][1];
    frustplane[2].w = cam->frustumPlanes[3][2];
    frustplane[3].w = cam->frustumPlanes[3][3];
    frustplane[4].x = cam->frustumPlanes[1][0];
    frustplane[5].x = cam->frustumPlanes[1][1];
    frustplane[6].x = cam->frustumPlanes[1][2];
    frustplane[7].x = cam->frustumPlanes[1][3];
    frustplane[4].y = cam->frustumPlanes[0][0];
    frustplane[5].y = cam->frustumPlanes[0][1];
    frustplane[6].y = cam->frustumPlanes[0][2];
    frustplane[7].y = cam->frustumPlanes[0][3];
    frustplane[4].z = cam->frustumPlanes[2][0];
    frustplane[5].z = cam->frustumPlanes[2][1];
    frustplane[6].z = cam->frustumPlanes[2][2];
    frustplane[7].z = cam->frustumPlanes[2][3];
    frustplane[4].w = cam->frustumPlanes[4][0];
    frustplane[5].w = cam->frustumPlanes[4][1];
    frustplane[6].w = cam->frustumPlanes[4][2];
    frustplane[7].w = cam->frustumPlanes[4][3];
}

void iCameraUpdatePos(RwCamera* cam, _xMat4x3* pos)
{
    RwFrame* frame = (RwFrame*)cam->object.parent;
    RwMatrixTag* mat = &frame->modelling;

    memcpy(mat, pos, sizeof(_xMat4x3));
    RwFrameOrthoNormalize(frame);
    RwMatrixUpdate(mat);
    RwFrameUpdateObjects(frame);
}

void iCameraSetFOV(RwCamera* cam, float fov)
{
    RwV2d vw;
    vw.x = tan(3.1415927f * (0.5f * fov) / 180.0f);
    vw.y = 480.0f * vw.x / 640.0f;
    RwCameraSetViewWindow(cam, &vw);
}

void iCameraAssignEnv(RwCamera* camera, iEnv* env_geom)
{
    globalCamera = camera;
    RpWorldAddCamera(env_geom->world, camera);
    xShadowSetWorld(env_geom->world);
}

void iCameraRemoveEnv(RwCamera* camera, iEnv* env_geom)
{
    RpWorldRemoveCamera(env_geom->world, camera);
}

void iCameraSetNearFarClip(float nearPlane, float farPlane)
{
    if (nearPlane <= 0.0f)
    {
        nearPlane = 0.2f;
    }
    sCameraNearClip = nearPlane;
    if (farPlane <= 0.0f)
    {
        farPlane = 400.0f;
    }
    sCameraFarClip = farPlane;
}

void iCameraSetFogParams(iFogParams* fp, float time)
{
    if (!fp || fp->type == rwFOGTYPENAFOGTYPE)
    {
        globals.fog.type = rwFOGTYPENAFOGTYPE;
        globals.fogA.type = rwFOGTYPENAFOGTYPE;
        return;
    }

    if (0.0f == time || fp->type != globals.fogA.type)
    {
        globals.fogA = globals.fog = *fp;
        globals.fog_t0 = 0;
        return;
    }

    globals.fogA = globals.fog;
    globals.fogB = *fp;
    globals.fog_t0 = iTimeGet();
    globals.fog_t1 = globals.fog_t0 + (iTime)(time * (GET_BUS_FREQUENCY() / 4));
}

// TODO (73.6%): the original frame is 0x10 bytes larger and the colour lerps are
// scheduled differently; the logic is the same.
void iCameraUpdateFog(RwCamera* cam, iTime t)
{
    iFogParams* fp = &globals.fog;
    iFogParams* fa = &globals.fogA;

    if (globals.fog.type == rwFOGTYPENAFOGTYPE)
    {
        RwRenderStateSet(14, (void*)0);
        RwCameraSetFarClipPlane(cam, sCameraFarClip);
        return;
    }

    if (globals.fog_t0 == 0)
    {
        fp = fa;
    }
    else
    {
        iTime now = iTimeGet();
        float total = iTimeDiffSec(globals.fog_t0, globals.fog_t1);
        float lerp = iTimeDiffSec(globals.fog_t0, now) / total;
        lerp = MAX(0.0f, MIN(lerp, 1.0f));

        iFogParams* fb = &globals.fogB;
        fp->type = fb->type;
        fp->table = fb->table;
        fp->start = lerp * (fb->start - fa->start) + fa->start;
        fp->stop = lerp * (fb->stop - fa->stop) + fa->stop;
        fp->density = lerp * (fb->density - fa->density) + fa->density;
        fp->fogcolor.red = lerp * (fb->fogcolor.red - fa->fogcolor.red) + fa->fogcolor.red;
        fp->fogcolor.green =
            lerp * (fb->fogcolor.green - fa->fogcolor.green) + fa->fogcolor.green;
        fp->fogcolor.blue = lerp * (fb->fogcolor.blue - fa->fogcolor.blue) + fa->fogcolor.blue;
        fp->fogcolor.alpha =
            lerp * (fb->fogcolor.alpha - fa->fogcolor.alpha) + fa->fogcolor.alpha;
        fp->bgcolor.red = lerp * (fb->bgcolor.red - fa->bgcolor.red) + fa->bgcolor.red;
        fp->bgcolor.green = lerp * (fb->bgcolor.green - fa->bgcolor.green) + fa->bgcolor.green;
        fp->bgcolor.blue = lerp * (fb->bgcolor.blue - fa->bgcolor.blue) + fa->bgcolor.blue;
        fp->bgcolor.alpha = lerp * (fb->bgcolor.alpha - fa->bgcolor.alpha) + fa->bgcolor.alpha;

        if (1.0f == lerp)
        {
            globals.fog_t0 = 0;
            globals.fogA = globals.fogB;
        }
    }

    RwRenderStateSet(14, (void*)1);
    RwRenderStateSet(16, (void*)fp->type);
    RwRenderStateSet(15, (void*)RWRGBALONG(fp->fogcolor.red, fp->fogcolor.green,
                                            fp->fogcolor.blue, fp->fogcolor.alpha));
    RwRenderStateSet(17, &fp->density);
    RwRenderStateSet(18, fp->table);
    cam->fogPlane = fp->start;
    RwCameraSetFarClipPlane(cam, fp->stop);
}

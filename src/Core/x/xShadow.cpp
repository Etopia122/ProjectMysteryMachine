#include "xShadow.h"
#include "xEnt.h"
#include "xCollide.h"
#include "xEnv.h"
#include "xMath.h"
#include "zGlobals.h"
#include "zScene.h"
#include "rwtypes.h"

#ifndef NULL
#define NULL 0
#endif

#define SCENE_ID(s) ((((s)[0] & ~0x20) << 24) | ((s)[1] << 16) | ((s)[2] << 8) | (s)[3])
#define SHADOW_MAX(a, b) ((a) >= (b) ? (a) : (b))

class RwRaster
{
public:
    unsigned char pad0[0xC];
    signed int width;  // 0xC
    signed int height; // 0x10
};

class RwV2d
{
public:
    float x; // 0x0
    float y; // 0x4
};

class RwCamera
{
public:
    RwObject object;           // 0x0
    unsigned char pad8[0x58];
    RwRaster* frameBuffer;     // 0x60
    RwRaster* zBuffer;         // 0x64
    RwV2d viewWindow;          // 0x68
    RwV2d recipViewWindow;     // 0x70
    RwV2d viewOffset;          // 0x78
    float nearPlane;           // 0x80
    float farPlane;            // 0x84
};

class RwSphere
{
public:
    RwV3d center; // 0x0
    float radius; // 0xC
};

class RpIntersection
{
public:
    union
    {
        RwSphere sphere;
        unsigned char data[0x18];
    } t;                 // 0x0
    signed int type;     // 0x18
};

class RpCollisionTriangle
{
public:
    RwV3d normal;        // 0x0
    RwV3d point;         // 0xC
    signed int index;    // 0x18
    RwV3d* vertices[3];  // 0x1C
};

class RpTriangle
{
public:
    unsigned short vertIndex[3]; // 0x0
    unsigned short matIndex;     // 0x6
};

class RpGeometry
{
public:
    unsigned char pad0[0x10];
    signed int numTriangles; // 0x10
    signed int numVertices;  // 0x14
    unsigned char pad18[0x14];
    RpTriangle* triangles;   // 0x2C
};

class RpAtomic
{
public:
    unsigned char pad0[0x18];
    RpGeometry* geometry;    // 0x18
    RwSphere boundingSphere; // 0x1C
};

class RxObjSpace3DVertex
{
public:
    RwV3d objVertex;          // 0x0
    RwV3d objNormal;          // 0xC
    RwRGBA color;             // 0x18
    float u;                  // 0x1C
    float v;                  // 0x20
};

class RwIm2DVertex
{
public:
    float x;           // 0x0
    float y;           // 0x4
    float z;           // 0x8
    RwRGBA emissiveColor; // 0xC
    float u;           // 0x10
    float v;           // 0x14
    float pad;         // 0x18
};

class tagiRenderInput
{
public:
    unsigned short* m_index;            // 0x0
    RxObjSpace3DVertex* m_vertex;       // 0x4
    unsigned char pad8[0x78];
};

class RwEngineInstanceType
{
public:
    RwCamera* curCamera; // 0x0
};

class RpWorldSector;

class ShadowRenderParam
{
public:
    RwV3d dir;               // 0x0
    RwMatrixTag invMatrix;   // 0xC
    unsigned char shadowValue; // 0x4C
    signed int fade;         // 0x50
    signed int numIm3DBatch; // 0x54
};

extern "C" {
void* memcpy(void* dst, const void* src, unsigned int n);
signed int RwRenderStateSet(signed int state, void* value);
signed int RwRenderStateGet(signed int state, void* value);
RwCamera* RwCameraCreate();
signed int RwCameraDestroy(RwCamera* camera);
RwCamera* RwCameraClear(RwCamera* camera, RwRGBA* colour, signed int clearMode);
RwCamera* RwCameraBeginUpdate(RwCamera* camera);
RwCamera* RwCameraEndUpdate(RwCamera* camera);
RwCamera* RwCameraSetNearClipPlane(RwCamera* camera, float nearClip);
RwCamera* RwCameraSetFarClipPlane(RwCamera* camera, float farClip);
RwCamera* RwCameraSetViewWindow(RwCamera* camera, const RwV2d* viewWindow);
RwCamera* RwCameraSetProjection(RwCamera* camera, signed int projection);
signed int RwCameraFrustumTestSphere(const RwCamera* camera, const RwSphere* sphere);
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
RwV3d* RwV3dTransformPoints(RwV3d* pointsOut, const RwV3d* pointsIn, signed int numPoints,
                            const RwMatrixTag* matrix);
float RwV3dLength(const RwV3d* in);
float RwIm2DGetNearScreenZ();
float RwIm2DGetFarScreenZ();
signed int RwIm2DRenderPrimitive(signed int primType, RwIm2DVertex* vertices, signed int numVertices);
void* RwIm3DTransform(RxObjSpace3DVertex* pVerts, unsigned int numVerts, RwMatrixTag* ltm,
                      unsigned int flags);
signed int RwIm3DEnd();
signed int RwIm3DRenderPrimitive(signed int primType);
RpWorld* RpCollisionWorldForAllIntersections(
    RpWorld* world, RpIntersection* intersection,
    RpCollisionTriangle* (*callBack)(RpIntersection*, RpWorldSector*, RpCollisionTriangle*, float,
                                     void*),
    void* data);
// TODO: unidentified RenderWare matrix functions (invert, scale, translate)
RwMatrixTag* RwMatrixInvert(RwMatrixTag* dst, const RwMatrixTag* src);
RwMatrixTag* RwMatrixScale(RwMatrixTag* matrix, const RwV3d* scale, signed int combineOp);
RwMatrixTag* RwMatrixTranslate(RwMatrixTag* matrix, const RwV3d* translation, signed int combineOp);
// TODO: unidentified GameCube RenderWare function (copies the frame buffer into a raster)
void fn_80178A3C(RwRaster* raster);
extern RwEngineInstanceType* RwEngineInstance;
}

void zEntGetShadowParams(_xEnt* ent, _xVec3* center, float* radius);
void xModelRender(xModelInstance* modelInst);
void iModelVertEval(RpAtomic* model, unsigned int index, unsigned int count, RwMatrixTag* mat,
                    _xVec3* vel, _xVec3* pos);
void xDrawSetColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void xDrawLine(const _xVec3* a, const _xVec3* b);
void* xMemPushTemp(unsigned int size);
void xMemPopTemp(void* addr);
float xVec3Normalize(_xVec3* o, _xVec3* v);
void xMat3x3LookVec(_xMat3x3* m, _xVec3* at);
void xRayHitsScene(_xScene* sc, _xRay3* r, _xCollis* coll);
void iRayHitsEnv(const _xRay3* r, const _xEnv* env, _xCollis* coll);
signed int zMenuRunning();
extern "C" double fabs(double);

extern tagiRenderInput gRenderBuffer;

float ShadowThreshold = 144.0f;
float ShadowZoneRadius = 2.25f;
float ShadowStrength = 0.3f;

static float SHADOW_BF_DOT;
static float SHADOW_BOTH;
static RxObjSpace3DVertex* Im3DBuffer;
static unsigned int Im3DBufferPos;
static unsigned int ShadowNumTriangles;
static RwCamera* ShadowCamera;
static RwRaster* ShadowCameraRaster;
static RwRaster* ShadowRenderRaster;
signed int ShadowFade;
static unsigned int sShadowFlags;
static RwRaster* gc_saveraster;
static _xSphere sSphere;

static inline void RwIm3DVertexSetPos(RxObjSpace3DVertex* vert, float x, float y, float z)
{
    vert->objVertex.x = x;
    vert->objVertex.y = y;
    vert->objVertex.z = z;
}

static inline void modelRenderCB(void* model)
{
    xModelRender((xModelInstance*)model);
}

static inline void ShadowCameraUpdate(RwCamera* shadowCamera, void* model)
{
    RwRGBA bgColor = { 255, 255, 255, 0 };
    signed int fogstate;
    RwCamera* camera = RwEngineInstance->curCamera;

    RwRenderStateGet(14, &fogstate);
    RwRenderStateSet(14, (void*)0);
    if (camera != NULL)
    {
        RwCameraEndUpdate(camera);
    }
    fn_80178A3C(gc_saveraster);
    RwCameraClear(shadowCamera, &bgColor, 1);
    if (RwCameraBeginUpdate(shadowCamera))
    {
        RwRenderStateSet(1, (void*)0);
        RwRenderStateSet(8, (void*)0);
        RwRenderStateSet(6, (void*)0);
        modelRenderCB(model);
        InvertRaster(shadowCamera);
        RwCameraEndUpdate(shadowCamera);
        fn_80178A3C(shadowCamera->frameBuffer);
    }
    if (camera != NULL)
    {
        RwCameraBeginUpdate(camera);
    }

    float recipCamZ = 1.0f / RwEngineInstance->curCamera->farPlane;
    RwRenderStateSet(10, (void*)2);
    RwRenderStateSet(11, (void*)1);
    RwRenderStateSet(6, (void*)0);
    RwRenderStateSet(8, (void*)0);
    RwRenderStateSet(9, (void*)1);
    RwRenderStateSet(1, gc_saveraster);
    Im2DRenderQuad(0.0f, 0.0f, 256.0f, 256.0f, RwIm2DGetFarScreenZ(), recipCamZ, 0.0f);
    RwRenderStateSet(6, (void*)1);
    RwRenderStateSet(8, (void*)1);
    RwRenderStateSet(10, (void*)5);
    RwRenderStateSet(11, (void*)6);
    RwRenderStateSet(14, (void*)fogstate);
}

static inline void xShadowCameraUpdate(void* model, _xVec3* center, float radius)
{
    RwCamera* cam = ShadowCamera;
    RwCameraSetFarClipPlane(cam, 2.0f * radius);
    RwCameraSetNearClipPlane(cam, 0.001f * radius);
    RwV2d vw;
    vw.x = radius;
    vw.y = radius;
    RwCameraSetViewWindow(cam, &vw);

    RwCamera* camera = ShadowCamera;
    RwFrame* camFrame = (RwFrame*)camera->object.parent;
    RwMatrixTag* camMatrix = &camFrame->modelling;
    camMatrix->pos.x = center->x;
    camMatrix->pos.y = center->y;
    camMatrix->pos.z = center->z;
    camMatrix->pos.x += camMatrix->at.x * (-0.5f * camera->farPlane);
    camMatrix->pos.y += camMatrix->at.y * (-0.5f * camera->farPlane);
    camMatrix->pos.z += camMatrix->at.z * (-0.5f * camera->farPlane);
    RwMatrixUpdate(camMatrix);
    RwFrameUpdateObjects(camFrame);

    ShadowCameraUpdate(ShadowCamera, model);
    ShadowRenderRaster = ShadowCameraRaster;
}

static inline void ShadowCameraDestroy(RwCamera* shadowCamera)
{
    if (shadowCamera != NULL)
    {
        _rwFrameSyncDirty();
        RwFrame* frame = (RwFrame*)shadowCamera->object.parent;
        if (frame != NULL)
        {
            _rwObjectHasFrameSetFrame(shadowCamera, NULL);
            RwFrameDestroy(frame);
        }
        if (shadowCamera->zBuffer != NULL)
        {
            RwRaster* raster = shadowCamera->zBuffer;
            shadowCamera->zBuffer = NULL;
            RwRasterDestroy(raster);
        }
        if (shadowCamera->frameBuffer != NULL)
        {
            RwRaster* raster = shadowCamera->frameBuffer;
            shadowCamera->frameBuffer = NULL;
            RwRasterDestroy(raster);
        }
        RwCameraDestroy(shadowCamera);
    }
}

static inline RwCamera* ShadowCameraCreatePersp(signed int res)
{
    RwCamera* cam = RwCameraCreate();
    if (cam != NULL)
    {
        _rwObjectHasFrameSetFrame(cam, RwFrameCreate());
        if (cam->object.parent != NULL)
        {
            RwRaster* raster = RwRasterCreate(res, res, 0, 1);
            if (raster != NULL)
            {
                cam->zBuffer = raster;
                RwCameraSetProjection(cam, 2);
                return cam;
            }
        }
    }
    ShadowCameraDestroy(cam);
    return NULL;
}

static inline unsigned int xShadowEntIsOutsideFrustum(_xEnt* ent)
{
    xModelInstance* model;
    RpAtomic* atomic;
    if (ent == NULL || (model = ent->model) == NULL || (atomic = model->Data) == NULL)
    {
        return 1;
    }

    RwSphere worldsph;
    RwCamera* cam = ShadowCamera;
    RwMatrixTag* mat = model->Mat;
    RwV3dTransformPoints(&worldsph.center, &atomic->boundingSphere.center, 1, mat);
    float xScale2 = mat->right.x * mat->right.x + mat->right.y * mat->right.y +
                    mat->right.z * mat->right.z;
    float yScale2 = mat->up.x * mat->up.x + mat->up.y * mat->up.y + mat->up.z * mat->up.z;
    float zScale2 = mat->at.x * mat->at.x + mat->at.y * mat->at.y + mat->at.z * mat->at.z;
    float atomicScale = SHADOW_MAX(xScale2, SHADOW_MAX(yScale2, zScale2));
    atomicScale = xsqrt(atomicScale);
    worldsph.radius = atomic->boundingSphere.radius * atomicScale;
    unsigned int test = (unsigned char)(RwCameraFrustumTestSphere(cam, &worldsph) == 0);
    return test;
}

static inline void ShadowFlushIm3D()
{
    if (RwIm3DTransform(Im3DBuffer, Im3DBufferPos, NULL, 0x19))
    {
        RwIm3DRenderPrimitive(3);
        RwIm3DEnd();
    }
}

static inline void xShadowSceneTweaks()
{
    unsigned int sceneID = globals.sceneCur->sceneID;
    _xVec3* pos = &((_xMat4x3*)globals.player.ent.model->Mat)->pos;
    if (sceneID == SCENE_ID("I001"))
    {
        _xVec3 p = { -7.0f, 10.0f, -6.0f };
        float dx = pos->x - p.x;
        float dy = pos->y - p.y;
        float dz = pos->z - p.z;
        SHADOW_BOTH = dx * dx + dy * dy + dz * dz < 16.0f ? 0.1f : 2.0f;
    }
    else if (sceneID == SCENE_ID("I005"))
    {
        _xVec3 p = { -5.0f, 11.0f, -179.0f };
        float dx = pos->x - p.x;
        float dy = pos->y - p.y;
        float dz = pos->z - p.z;
        SHADOW_BOTH = dx * dx + dy * dy + dz * dz < 1.0f ? 0.05f : 2.0f;
    }
}

// TODO: NonMatching. .sdata2 order: the target pools bgColor, 1.0f, 0.0f and 256.0f
// first, as if an out-of-line copy of the camera-update code (a dead-stripped
// xShadowCameraUpdate?) was compiled before everything else. The remaining
// function diffs are register numbering and stack layout.

// Functions are in reverse DOL order (-inline deferred emits last to first).

void SetupShadow()
{
    signed int res = 256;
    while (res > 640 || res > 480)
    {
        res >>= 1;
    }

    ShadowCamera = ShadowCameraCreatePersp(res);
    if (ShadowCamera != NULL)
    {
        ShadowCameraRaster = RwRasterCreate(res, res, 0, 5);
        if (ShadowCameraRaster != NULL)
        {
            ShadowCamera->frameBuffer = ShadowCameraRaster;
        }
    }
    gc_saveraster = RwRasterCreate(256, 256, 32, 0x504);
}

void xShadowSetWorld(RpWorld* world)
{
    unsigned int sceneID = globals.sceneCur->sceneID;
    RpWorldAddCamera(world, ShadowCamera);
    SHADOW_BOTH = 2.0f;
    SHADOW_BF_DOT = 0.0f;
    if (sceneID == SCENE_ID("E001") || sceneID == SCENE_ID("E002") ||
        sceneID == SCENE_ID("F003") || sceneID == SCENE_ID("G002") ||
        sceneID == SCENE_ID("I020") || sceneID == SCENE_ID("I021") ||
        sceneID == SCENE_ID("I003") || sceneID == SCENE_ID("L011") ||
        sceneID == SCENE_ID("O004"))
    {
        SHADOW_BF_DOT = -0.5f;
    }
    ShadowThreshold = 144.0f;
    if (sceneID == SCENE_ID("W028"))
    {
        ShadowThreshold = 225.0f;
    }
}

void xShadowSetLight(_xVec3* target_pos, _xVec3* in_vec, float shadow_depth)
{
    _xVec3 zvec;
    _xMat4x3 matrix;

    xVec3Normalize(&zvec, in_vec);
    xMat3x3LookVec((_xMat3x3*)&matrix, &zvec);
    matrix.pos = *target_pos;
    RwFrame* camFrame = (RwFrame*)ShadowCamera->object.parent;
    RwMatrixTag* camMatrix = &camFrame->modelling;
    memcpy(camMatrix, &matrix, sizeof(_xMat4x3));
    RwFrameOrthoNormalize(camFrame);
    RwMatrixUpdate(camMatrix);
    RwFrameUpdateObjects(camFrame);
}

void xShadowRenderWorld(_xVec3* center, float radius)
{
    RwFrame* camFrame = (RwFrame*)ShadowCamera->object.parent;
    _xVec3* rt = (_xVec3*)&camFrame->modelling.right;
    _xVec3* up = (_xVec3*)&camFrame->modelling.up;
    _xVec3* at = (_xVec3*)&camFrame->modelling.at;
    float sf[3][2] = { { 0.0f, 0.0f }, { 0.0f, 0.5f }, { 0.0f, -0.5f } };
    _xRay3 R[3];
    _xCollis crec[3];
    _xCollis ecrec;
    RpIntersection shadowZone;
    _xSphere zone;
    _xVec3 hit;
    signed int i;
    float env_dist = 100.0f;
    float ent_dist = 100.0f;
    unsigned int hit_ent = 0;
    unsigned int hit_env = 0;

    float hx = 0.0f;
    float hy = 0.0f;
    float hz = 0.0f;
    sShadowFlags = 0;
    for (i = 0; i < 3; i++)
    {
        R[i].dir = *at;
        R[i].origin = *center;
        xVec3AddScaled(&R[i].origin, rt, sf[i][0]);
        xVec3AddScaled(&R[i].origin, up, sf[i][1]);
        R[i].min_t = 0.0f;
        R[i].max_t = 10.0f;
        R[i].flags = 0xC00;
        xRayHitsScene((_xScene*)globals.sceneCur, &R[i], &crec[i]);

        xVec3Copy(&hit, &R[i].origin);
        xVec3AddScaled(&hit, &R[i].dir, crec[i].dist);
        if (crec[i].flags & 1)
        {
            if (crec[i].optr != NULL)
            {
                hit_ent++;
                xDrawSetColor(255, 0, 0, 255);
                if (crec[i].dist < ent_dist)
                {
                    ent_dist = crec[i].dist;
                }
                iRayHitsEnv(&R[i], globals.sceneCur->env, &ecrec);
                if (ecrec.flags & 1)
                {
                    hit_env++;
                    float ex = R[i].dir.x * ecrec.dist + R[i].origin.x;
                    float ey = R[i].dir.y * ecrec.dist + R[i].origin.y;
                    float ez = R[i].dir.z * ecrec.dist + R[i].origin.z;
                    if (ecrec.dist < env_dist)
                    {
                        env_dist = ecrec.dist;
                        hx = ex;
                        hy = ey;
                        hz = ez;
                    }
                }
            }
            else
            {
                xDrawSetColor(0, 255, 0, 255);
                if (0)
                {
                    xDrawLine(&R[i].origin, &hit);
                }
                hit_env++;
                if (crec[i].dist < env_dist)
                {
                    env_dist = crec[i].dist;
                    hx = hit.x;
                    hy = hit.y;
                    hz = hit.z;
                }
            }
        }
    }

    if (hit_env && hit_ent && ent_dist > 0.0f && env_dist > 0.0f &&
        (float)__fabs(ent_dist - env_dist) < SHADOW_BOTH)
    {
        sShadowFlags |= 3;
    }
    if (hit_env && env_dist > 0.0f && env_dist < ent_dist)
    {
        sShadowFlags |= 1;
    }
    if (hit_ent && ent_dist > 0.0f && ent_dist < env_dist)
    {
        sShadowFlags |= 2;
    }

    if (sShadowFlags & 1)
    {
        sSphere.center.x = hx;
        zone.center.x = hx;
        zone.center.y = hy;
        zone.center.z = hz;
        zone.r = radius;
        sSphere.center.y = hy;
        sSphere.center.z = hz;
        sSphere.r = radius;
    }
    else
    {
        sSphere.r = 0.0f;
        return;
    }

    shadowZone.type = 3;
    shadowZone.t.sphere.center.x = zone.center.x;
    shadowZone.t.sphere.center.y = zone.center.y;
    shadowZone.t.sphere.center.z = zone.center.z;
    shadowZone.t.sphere.radius = zone.r;
    ShadowRender(ShadowCamera, ShadowRenderRaster, globals.sceneCur->env->geom->world, &shadowZone,
                 ShadowStrength, ShadowFade ? ShadowZoneRadius : 0.0f);
}

unsigned int xShadowReceiveShadowSetup(_xEnt* ent)
{
    unsigned int sceneID = globals.sceneCur->sceneID;
    if (zMenuRunning())
    {
        return 0;
    }
    if (!(sShadowFlags & 2))
    {
        return 0;
    }
    if (ent->model == NULL || !xEntIsVisible(ent))
    {
        return 0;
    }

    unsigned int rsf_valid = 0;
    if (sceneID == SCENE_ID("w027"))
    {
        rsf_valid = 1;
    }
    if (rsf_valid && !(ent->baseFlags & 0x10))
    {
        return 0;
    }
    if (xShadowEntIsOutsideFrustum(ent))
    {
        return 0;
    }
    if (ent->id == 0xFC1DFD0B && sceneID == SCENE_ID("c004"))
    {
        _xVec3 p = { -458.0f, -17.0f, 121.0f };
        _xVec3* pos = &((_xMat4x3*)globals.player.ent.model->Mat)->pos;
        float dx = pos->x - p.x;
        float dy = pos->y - p.y;
        float dz = pos->z - p.z;
        if (dx * dx + dy * dy + dz * dz < 9.0f)
        {
            return 0;
        }
    }
    return 1;
}

void xShadowReceiveShadow(_xEnt* ent)
{
    RwCamera* shadowCamera = ShadowCamera;
    if (!xShadowEntIsOutsideFrustum(ent) && !(ent->flags & 4) && !(ent->material_alpha < 255.0f))
    {
        if (ent->moreFlags & 1)
        {
            return;
        }

        signed int fogstate;
        RwRenderStateSet(1, ShadowCamera->frameBuffer);
        RwRenderStateSet(2, (void*)3);
        RwRenderStateSet(12, (void*)1);
        RwRenderStateGet(14, &fogstate);
        RwRenderStateSet(14, (void*)0);
        RwRenderStateSet(10, (void*)1);

        float shadowFactor = ShadowStrength;
        if (ShadowStrength < 0.0f)
        {
            shadowFactor = -ShadowStrength;
            RwRenderStateSet(11, (void*)3);
        }
        else
        {
            RwRenderStateSet(11, (void*)4);
        }

        float fadeDist = 0.0f;
        float radius = shadowCamera->viewWindow.x;
        RwMatrixTag* shadowMatrix = &((RwFrame*)shadowCamera->object.parent)->modelling;
        float atx = shadowMatrix->at.x;
        float aty = shadowMatrix->at.y;
        float atz = shadowMatrix->at.z;
        RwMatrixTag invMatrix;
        RwMatrixInvert(&invMatrix, shadowMatrix);
        RwV3d scl;
        scl.x = scl.y = -0.5f / radius;
        scl.z = 1.0f / (fadeDist + radius);
        RwMatrixScale(&invMatrix, &scl, 2);
        RwV3d tr;
        tr.x = tr.y = 0.5f;
        tr.z = 0.0f;
        RwMatrixTranslate(&invMatrix, &tr, 2);

        unsigned int max_verts = 0;
        unsigned char val = 255.0f * shadowFactor;
        for (xModelInstance* model = ent->model; model != NULL; model = model->Next)
        {
            unsigned int numverts = model->Data->geometry->numVertices;
            if (numverts > max_verts)
            {
                max_verts = numverts;
            }
        }

        _xVec3* xvert = (_xVec3*)xMemPushTemp(max_verts * sizeof(_xVec3));
        if (xvert == NULL)
        {
            return;
        }

        Im3DBuffer = gRenderBuffer.m_vertex;
        for (xModelInstance* model = ent->model; model != NULL; model = model->Next)
        {
            RpAtomic* atomic = model->Data;
            RpGeometry* geom = atomic->geometry;
            iModelVertEval(atomic, 0, geom->numVertices, model->Mat, NULL, xvert);
            xDrawSetColor(0, 0, 255, 255);
            RpTriangle* tri = geom->triangles;
            for (unsigned int i = 0; i < geom->numTriangles; i++, tri++)
            {
                if (Im3DBufferPos > 477)
                {
                    ShadowFlushIm3D();
                    Im3DBufferPos = 0;
                }

                _xVec3* v0 = &xvert[tri->vertIndex[0]];
                _xVec3* v1 = &xvert[tri->vertIndex[1]];
                _xVec3* v2 = &xvert[tri->vertIndex[2]];
                RxObjSpace3DVertex* imv = &Im3DBuffer[Im3DBufferPos];
                RwV3d vIn[3];
                RwV3d vShad[3];
                vIn[0].x = v0->x;
                vIn[0].y = v0->y;
                vIn[0].z = v0->z;
                vIn[1].x = v1->x;
                vIn[1].y = v1->y;
                vIn[1].z = v1->z;
                vIn[2].x = v2->x;
                vIn[2].y = v2->y;
                vIn[2].z = v2->z;
                RwV3dTransformPoints(vShad, vIn, 3, &invMatrix);

                if ((vShad[0].z < 0.0f && vShad[1].z < 0.0f && vShad[2].z < 0.0f) ||
                    (vShad[0].x < 0.0f && vShad[1].x < 0.0f && vShad[2].x < 0.0f) ||
                    (vShad[0].x > 1.0f && vShad[1].x > 1.0f && vShad[2].x > 1.0f) ||
                    (vShad[0].y < 0.0f && vShad[1].y < 0.0f && vShad[2].y < 0.0f) ||
                    (vShad[0].y > 1.0f && vShad[1].y > 1.0f && vShad[2].y > 1.0f))
                {
                    continue;
                }

                RwV3d a;
                RwV3d b;
                RwV3d c;
                a.x = v1->x - v0->x;
                a.y = v1->y - v0->y;
                a.z = v1->z - v0->z;
                b.x = v2->x - v0->x;
                b.y = v2->y - v0->y;
                b.z = v2->z - v0->z;
                c.x = a.y * b.z - a.z * b.y;
                c.y = a.z * b.x - a.x * b.z;
                c.z = a.x * b.y - a.y * b.x;
                float len = 0.001f / RwV3dLength(&c);
                c.x *= len;
                c.y *= len;
                c.z *= len;
                if (c.x * atx + c.y * aty + c.z * atz > 0.0f)
                {
                    continue;
                }

                RwIm3DVertexSetPos(&imv[0], v0->x + c.x, v0->y + c.y, v0->z + c.z);
                RwIm3DVertexSetPos(&imv[1], v1->x + c.x, v1->y + c.y, v1->z + c.z);
                RwIm3DVertexSetPos(&imv[2], v2->x + c.x, v2->y + c.y, v2->z + c.z);
                imv[0].u = vShad[0].x;
                imv[1].u = vShad[1].x;
                imv[2].u = vShad[2].x;
                imv[0].v = vShad[0].y;
                imv[1].v = vShad[1].y;
                imv[2].v = vShad[2].y;
                imv[0].color.red = val;
                imv[0].color.green = val;
                imv[0].color.blue = val;
                imv[0].color.alpha = val;
                imv[1].color.red = val;
                imv[1].color.green = val;
                imv[1].color.blue = val;
                imv[1].color.alpha = val;
                imv[2].color.red = val;
                imv[2].color.green = val;
                imv[2].color.blue = val;
                imv[2].color.alpha = val;
                Im3DBufferPos += 3;
            }

            if (Im3DBufferPos != 0)
            {
                ShadowFlushIm3D();
                Im3DBufferPos = 0;
            }
        }

        xMemPopTemp(xvert);
        RwRenderStateSet(11, (void*)6);
        RwRenderStateSet(10, (void*)5);
        RwRenderStateSet(14, &fogstate);
    }
}

void xShadowRender(_xEnt* ent)
{
    _xVec3 center;
    float radius;

    zEntGetShadowParams(ent, &center, &radius);
    xShadowCameraUpdate(ent->model, &center, radius);

    xShadowSceneTweaks();

    xShadowRenderWorld(&center, radius);
}

signed int Im2DRenderQuad(float x1, float y1, float x2, float y2, float z, float recipCamZ,
                          float uvOffset)
{
    RwIm2DVertex vx[4];

    vx[0].x = x1;
    vx[0].y = y1;
    vx[0].z = z;
    vx[0].emissiveColor.red = 255;
    vx[0].emissiveColor.green = 255;
    vx[0].emissiveColor.blue = 255;
    vx[0].emissiveColor.alpha = 255;
    vx[0].u = uvOffset;
    vx[0].v = uvOffset;

    vx[1].x = x1;
    vx[1].y = y2;
    vx[1].z = z;
    vx[1].emissiveColor.red = 255;
    vx[1].emissiveColor.green = 255;
    vx[1].emissiveColor.blue = 255;
    vx[1].emissiveColor.alpha = 255;
    vx[1].u = uvOffset;
    vx[1].v = 1.0f + uvOffset;

    vx[2].x = x2;
    vx[2].y = y1;
    vx[2].z = z;
    vx[2].emissiveColor.red = 255;
    vx[2].emissiveColor.green = 255;
    vx[2].emissiveColor.blue = 255;
    vx[2].emissiveColor.alpha = 255;
    vx[2].u = 1.0f + uvOffset;
    vx[2].v = uvOffset;

    vx[3].x = x2;
    vx[3].y = y2;
    vx[3].z = z;
    vx[3].emissiveColor.red = 255;
    vx[3].emissiveColor.green = 255;
    vx[3].emissiveColor.blue = 255;
    vx[3].emissiveColor.alpha = 255;
    vx[3].u = 1.0f + uvOffset;
    vx[3].v = 1.0f + uvOffset;

    RwIm2DRenderPrimitive(4, vx, 4);
    return 1;
}

void InvertRaster(RwCamera* cam)
{
    RwIm2DVertex vx[4];
    float cam_width = cam->frameBuffer->width;
    float cam_height = cam->frameBuffer->height;

    vx[0].x = 0.0f;
    vx[0].y = 0.0f;
    vx[0].z = RwIm2DGetNearScreenZ();
    vx[0].emissiveColor.red = 255;
    vx[0].emissiveColor.green = 255;
    vx[0].emissiveColor.blue = 255;
    vx[0].emissiveColor.alpha = 255;
    vx[0].u = 0.0f;
    vx[0].v = 0.0f;

    vx[1].x = 0.0f;
    vx[1].y = cam_height;
    vx[1].z = RwIm2DGetNearScreenZ();
    vx[1].emissiveColor.red = 255;
    vx[1].emissiveColor.green = 255;
    vx[1].emissiveColor.blue = 255;
    vx[1].emissiveColor.alpha = 255;
    vx[1].u = 0.0f;
    vx[1].v = 1.0f;

    vx[2].x = cam_width;
    vx[2].y = 0.0f;
    vx[2].z = RwIm2DGetNearScreenZ();
    vx[2].emissiveColor.red = 255;
    vx[2].emissiveColor.green = 255;
    vx[2].emissiveColor.blue = 255;
    vx[2].emissiveColor.alpha = 255;
    vx[2].u = 1.0f;
    vx[2].v = 0.0f;

    vx[3].x = cam_width;
    vx[3].y = cam_height;
    vx[3].z = RwIm2DGetNearScreenZ();
    vx[3].emissiveColor.red = 255;
    vx[3].emissiveColor.green = 255;
    vx[3].emissiveColor.blue = 255;
    vx[3].emissiveColor.alpha = 255;
    vx[3].u = 1.0f;
    vx[3].v = 1.0f;

    RwRenderStateSet(6, (void*)0);
    RwRenderStateSet(1, (void*)0);
    RwRenderStateSet(12, (void*)1);
    RwRenderStateSet(11, (void*)1);
    RwRenderStateSet(10, (void*)10);
    RwIm2DRenderPrimitive(4, vx, 4);
    RwRenderStateSet(6, (void*)1);
    RwRenderStateSet(11, (void*)6);
    RwRenderStateSet(10, (void*)5);
}

RpCollisionTriangle* ShadowRenderTriangleCB(RpIntersection* intersection, RpWorldSector* sector,
                                            RpCollisionTriangle* collTriangle, float distance,
                                            void* data)
{
    ShadowRenderParam* param = (ShadowRenderParam*)data;
    RwV3d vIn[3];
    RwV3d vShad[3];

    if (collTriangle->normal.x * param->dir.x + collTriangle->normal.y * param->dir.y +
            collTriangle->normal.z * param->dir.z >
        SHADOW_BF_DOT)
    {
        return collTriangle;
    }

    vIn[0] = *collTriangle->vertices[0];
    vIn[1] = *collTriangle->vertices[1];
    vIn[2] = *collTriangle->vertices[2];
    RwV3dTransformPoints(vShad, vIn, 3, &param->invMatrix);

    if ((vShad[0].z < 0.0f && vShad[1].z < 0.0f && vShad[2].z < 0.0f) ||
        (vShad[0].x < 0.0f && vShad[1].x < 0.0f && vShad[2].x < 0.0f) ||
        (vShad[0].x > 1.0f && vShad[1].x > 1.0f && vShad[2].x > 1.0f) ||
        (vShad[0].y < 0.0f && vShad[1].y < 0.0f && vShad[2].y < 0.0f) ||
        (vShad[0].y > 1.0f && vShad[1].y > 1.0f && vShad[2].y > 1.0f))
    {
        return collTriangle;
    }

    if (Im3DBufferPos > 477)
    {
        ShadowFlushIm3D();
        param->numIm3DBatch++;
        Im3DBufferPos = 0;
    }

    RxObjSpace3DVertex* imv = &Im3DBuffer[Im3DBufferPos];
    float nx = 0.001f * collTriangle->normal.x;
    float ny = 0.001f * collTriangle->normal.y;
    float nz = 0.001f * collTriangle->normal.z;
    RwIm3DVertexSetPos(&imv[0], vIn[0].x + nx, vIn[0].y + ny, vIn[0].z + nz);
    RwIm3DVertexSetPos(&imv[1], vIn[1].x + nx, vIn[1].y + ny, vIn[1].z + nz);
    RwIm3DVertexSetPos(&imv[2], vIn[2].x + nx, vIn[2].y + ny, vIn[2].z + nz);
    imv[0].u = vShad[0].x;
    imv[1].u = vShad[1].x;
    imv[2].u = vShad[2].x;
    imv[0].v = vShad[0].y;
    imv[1].v = vShad[1].y;
    imv[2].v = vShad[2].y;
    unsigned char val = param->shadowValue;
    imv[0].color.red = val;
    imv[0].color.green = val;
    imv[0].color.blue = val;
    imv[0].color.alpha = val;
    imv[1].color.red = val;
    imv[1].color.green = val;
    imv[1].color.blue = val;
    imv[1].color.alpha = val;
    imv[2].color.red = val;
    imv[2].color.green = val;
    imv[2].color.blue = val;
    imv[2].color.alpha = val;
    Im3DBufferPos += 3;
    ShadowNumTriangles++;
    return collTriangle;
}

signed int ShadowRender(RwCamera* shadowCamera, RwRaster* shadow, RpWorld* world,
                        RpIntersection* shadowZone, float shadowStrength, float fadeDist)
{
    ShadowRenderParam param;
    signed int fogstate;

    RwRenderStateSet(1, shadowCamera->frameBuffer);
    RwRenderStateSet(2, (void*)3);
    RwRenderStateSet(12, (void*)1);
    RwRenderStateSet(10, (void*)1);
    if (shadowStrength < 0.0f)
    {
        shadowStrength = -shadowStrength;
        RwRenderStateSet(11, (void*)3);
    }
    else
    {
        RwRenderStateSet(11, (void*)4);
    }

    RwMatrixTag* shadowMatrix = &((RwFrame*)shadowCamera->object.parent)->modelling;
    param.dir = shadowMatrix->at;
    float radius = shadowCamera->viewWindow.x;
    RwMatrixInvert(&param.invMatrix, shadowMatrix);
    RwV3d scl;
    scl.x = scl.y = -0.5f / radius;
    scl.z = 1.0f / (fadeDist + radius);
    RwMatrixScale(&param.invMatrix, &scl, 2);

    param.fade = fadeDist > 0.0f ? 1 : 0;
    param.numIm3DBatch = 0;
    Im3DBuffer = gRenderBuffer.m_vertex;
    Im3DBufferPos = 0;
    ShadowNumTriangles = 0;
    param.shadowValue = 255.0f * shadowStrength;

    RwV3d tr2;
    RwV3d vtmp;
    RwV3d tr;
    vtmp.x = shadowMatrix->pos.x + shadowMatrix->at.x;
    vtmp.y = shadowMatrix->pos.y + shadowMatrix->at.y;
    vtmp.z = shadowMatrix->pos.z + shadowMatrix->at.z;
    RwV3dTransformPoints(&tr, &vtmp, 1, &param.invMatrix);
    tr2.x = tr2.y = 0.5f;
    tr2.z = -1.5f * tr.z;
    RwMatrixTranslate(&param.invMatrix, &tr2, 2);

    RwRenderStateGet(14, &fogstate);
    RwRenderStateSet(14, (void*)0);
    RpCollisionWorldForAllIntersections(world, shadowZone, ShadowRenderTriangleCB, &param);
    if (Im3DBufferPos != 0)
    {
        ShadowFlushIm3D();
        Im3DBufferPos = 0;
    }
    RwRenderStateSet(14, &fogstate);
    RwRenderStateSet(11, (void*)6);
    RwRenderStateSet(10, (void*)5);
    return 1;
}

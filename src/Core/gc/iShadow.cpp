#include "xEnt.h"
#include "xCollide.h"
#include "xstransvc.h"
#include "xString.h"
#include "xMath.h"
#include "zGlobals.h"
#include "zScene.h"
#include "rwtypes.h"

extern "C" void* memset(void* dst, int c, unsigned int n);

class RwRaster;
class _xEnv;

class RwTexture
{
public:
    RwRaster* raster; // 0x0
};

class RxObjSpace3DVertex
{
public:
    RwV3d objVertex; // 0x0
    RwV3d objNormal; // 0xC
    RwRGBA color;    // 0x18
    float u;         // 0x1C
    float v;         // 0x20
};

class RxRenderStateVector
{
public:
    unsigned int Flags;          // 0x0
    signed int ShadeMode;        // 0x4
    signed int SrcBlend;         // 0x8
    signed int DestBlend;        // 0xC
    RwRaster* TextureRaster;     // 0x10
    unsigned char pad14[0x30 - 0x14];
};

// TODO: unidentified RenderWare GameCube state block; bytes 0x3C/0x3D are
// the alpha test settings (see iEnv.cpp).
class RwGameCubeAlphaState
{
public:
    unsigned char pad0[0x3C];
    unsigned char enable; // 0x3C
    unsigned char ref;    // 0x3D
};
extern volatile RwGameCubeAlphaState lbl_8025E348;

extern "C" {
signed int RwRenderStateSet(signed int state, void* value);
void* RwIm3DTransform(RxObjSpace3DVertex* pVerts, unsigned int numVerts, RwMatrixTag* ltm,
                      unsigned int flags);
signed int RwIm3DRenderPrimitive(signed int primType);
signed int RwIm3DEnd();
// TODO: misnamed RenderWare GameCube function (mode, state block)
void _rwDlTextureRasterFlush(signed int mode, volatile void* data);
// TODO: unidentified RenderWare functions
RxRenderStateVector* RxRenderStateVectorLoadDriverState(RxRenderStateVector* rsv);
RwV3d* RwV3dTransformPoints(_xVec3* pointsOut, const _xVec3* pointsIn, signed int numPoints,
                   const RwMatrixTag* matrix);
}

void iRayHitsEnv(const _xRay3* r, const _xEnv* env, _xCollis* coll);
void xprintf(char* fmt, ...);

static RwRaster* sShadowRaster;
static RxRenderStateVector rsv;
static _xRay3 sShadowRay;
static RxObjSpace3DVertex sShadowVert[384];
static unsigned int sShadowVertCount;
// TODO: never set in this build; forces the shadow to be recollided.
static signed int sShadowForce;

#define RwIm3DVertexSetPos(vert, imx, imy, imz)                                                    \
    {                                                                                              \
        (vert)->objVertex.x = (imx);                                                               \
        (vert)->objVertex.y = (imy);                                                               \
        (vert)->objVertex.z = (imz);                                                               \
    }

// Functions are in reverse DOL order (-inline deferred emits last to first).

void iShadowInit()
{
    unsigned int i;
    RwTexture* tex = (RwTexture*)xSTFindAsset(xStrHash("shadow"), 0);

    sShadowRaster = tex ? tex->raster : 0;
    sShadowRay.dir.x = 0.0f;
    sShadowRay.dir.y = -1.0f;
    sShadowRay.dir.z = 0.0f;
    sShadowRay.min_t = 0.0f;
    sShadowRay.max_t = 10.0f;
    sShadowRay.flags = 0xC00;
    sShadowVertCount = 0;
    memset(sShadowVert, 0, sizeof(sShadowVert));
    for (i = 0; i < 384; i += 6)
    {
        sShadowVert[i + 1].u = 1.0f;
        sShadowVert[i + 2].v = 1.0f;
        sShadowVert[i + 3].u = 1.0f;
        sShadowVert[i + 4].v = 1.0f;
        sShadowVert[i + 5].u = 1.0f;
        sShadowVert[i + 5].v = 1.0f;
        sShadowVert[i + 0].objNormal.y = 1.0f;
        sShadowVert[i + 1].objNormal.y = 1.0f;
        sShadowVert[i + 2].objNormal.y = 1.0f;
        sShadowVert[i + 3].objNormal.y = 1.0f;
        sShadowVert[i + 4].objNormal.y = 1.0f;
        sShadowVert[i + 5].objNormal.y = 1.0f;
    }
}

// TODO (99.9%): two float registers swapped in the corner refit.
void iShadowCollide(iShadowCache* cache, _xEnt* ent, float radius, float ecc)
{
    _xCollis crec;
    _xVec3 tmppos[4];
    float height;
    float mag;
    float dydx;
    float dydz;
    float centerx;
    float centery;
    float centerz;

    if (!ent->model)
    {
        return;
    }

    if (!sShadowForce)
    {
        if (ecc == 1.0f)
        {
            RwMatrixTag* mat = ent->model->Mat;
            if (cache->pos.x == mat->pos.x && cache->pos.y == mat->pos.y &&
                cache->pos.z == mat->pos.z)
            {
                return;
            }
        }
        else
        {
            RwMatrixTag* mat = ent->model->Mat;
            if (cache->pos.x == mat->pos.x && cache->pos.y == mat->pos.y &&
                cache->pos.z == mat->pos.z && cache->right.x == mat->right.x &&
                cache->right.y == mat->right.y && cache->right.z == mat->right.z)
            {
                return;
            }
        }
    }

    crec.flags = 0x300;
    xVec3Copy(&sShadowRay.origin, (_xVec3*)&ent->model->Mat->pos);
    xVec3Copy(&cache->pos, &sShadowRay.origin);
    xVec3Copy(&cache->right, (_xVec3*)&ent->model->Mat->right);
    iRayHitsEnv(&sShadowRay, globals.sceneCur->env, &crec);

    if (crec.flags & 1)
    {
        height = 0.05f - crec.dist;
    }
    else
    {
        cache->corner[0].x = -1e10f;
        return;
    }

    {
        tmppos[0].x = ecc * radius;
        tmppos[0].y = 0.0f;
        tmppos[0].z = 0.0f;
        tmppos[1].x = 0.0f;
        tmppos[1].y = 0.0f;
        tmppos[1].z = radius;
        tmppos[2].x = 0.0f;
        tmppos[2].y = 0.0f;
        tmppos[2].z = -radius;
        tmppos[3].x = -ecc * radius;
        tmppos[3].y = 0.0f;
        tmppos[3].z = 0.0f;
        RwV3dTransformPoints(cache->corner, tmppos, 4, ent->model->Mat);
        cache->corner[0].y += height;
        cache->corner[1].y += height;
        cache->corner[2].y += height;
        cache->corner[3].y += height;

        if (crec.norm.y < 0.0f)
        {
            crec.norm.x = -crec.norm.x;
            crec.norm.y = -crec.norm.y;
            crec.norm.z = -crec.norm.z;
        }
        if (crec.norm.y < 0.8660254f)
        {
            mag = crec.norm.x * crec.norm.x + crec.norm.z * crec.norm.z;
            if (mag < 1e-6f)
            {
                xprintf("bonk!\n");
                mag = 1e-6f;
            }
            mag = 0.5f / xsqrt(mag);
            crec.norm.x *= mag;
            crec.norm.y = 0.8660254f;
            crec.norm.z *= mag;
        }

        dydx = -crec.norm.x / crec.norm.y;
        dydz = -crec.norm.z / crec.norm.y;
        centerx = 0.5f * (cache->corner[0].x + cache->corner[3].x);
        centery = 0.5f * (cache->corner[0].y + cache->corner[3].y);
        centerz = 0.5f * (cache->corner[0].z + cache->corner[3].z);
        cache->corner[0].y = dydz * (cache->corner[0].z - centerz) +
                             (dydx * (cache->corner[0].x - centerx) + centery);
        cache->corner[1].y = dydz * (cache->corner[1].z - centerz) +
                             (dydx * (cache->corner[1].x - centerx) + centery);
        cache->corner[2].y = dydz * (cache->corner[2].z - centerz) +
                             (dydx * (cache->corner[2].x - centerx) + centery);
        cache->corner[3].y = dydz * (cache->corner[3].z - centerz) +
                             (dydx * (cache->corner[3].x - centerx) + centery);
    }
}

// TODO (88%): the original branches around a lone blr for the vertex limit
// and schedules the six vertex addresses differently.
void iShadowAdd(iShadowCache* cache, unsigned char alpha)
{
    if (cache->corner[0].x != -1e10f)
    {
        if (sShadowVertCount < 384)
        {
            RxObjSpace3DVertex* v0 = &sShadowVert[sShadowVertCount];
            RxObjSpace3DVertex* v1 = &sShadowVert[sShadowVertCount + 1];
            RxObjSpace3DVertex* v2 = &sShadowVert[sShadowVertCount + 2];
            RxObjSpace3DVertex* v3 = &sShadowVert[sShadowVertCount + 3];
            RxObjSpace3DVertex* v4 = &sShadowVert[sShadowVertCount + 4];
            RxObjSpace3DVertex* v5 = &sShadowVert[sShadowVertCount + 5];
            RwIm3DVertexSetPos(v0, cache->corner[0].x, cache->corner[0].y, cache->corner[0].z);
            RwIm3DVertexSetPos(v1, cache->corner[1].x, cache->corner[1].y, cache->corner[1].z);
            RwIm3DVertexSetPos(v2, cache->corner[2].x, cache->corner[2].y, cache->corner[2].z);
            RwIm3DVertexSetPos(v3, cache->corner[1].x, cache->corner[1].y, cache->corner[1].z);
            RwIm3DVertexSetPos(v4, cache->corner[2].x, cache->corner[2].y, cache->corner[2].z);
            RwIm3DVertexSetPos(v5, cache->corner[3].x, cache->corner[3].y, cache->corner[3].z);
            v5->color.alpha = alpha;
            v4->color.alpha = alpha;
            v3->color.alpha = alpha;
            v2->color.alpha = alpha;
            v1->color.alpha = alpha;
            v0->color.alpha = alpha;
            sShadowVertCount += 6;
        }
    }
}

// TODO (97.3%): the original loads the flush mode (li r3, 1) before the two
// alpha state stores (same as fn_8000A138 in iEnv.cpp).
void iShadowRender()
{
    if (sShadowVertCount)
    {
        RxRenderStateVectorLoadDriverState(&rsv);
        RwRenderStateSet(10, (void*)4);
        RwRenderStateSet(11, (void*)9);
        RwRenderStateSet(12, (void*)1);
        RwRenderStateSet(1, sShadowRaster);
        RwRenderStateSet(7, (void*)2);
        RwRenderStateSet(8, (void*)0);
        lbl_8025E348.enable = 0;
        lbl_8025E348.ref = 0;
        _rwDlTextureRasterFlush(1, &lbl_8025E348);
        RwIm3DTransform(sShadowVert, sShadowVertCount, 0, 0x19);
        RwIm3DRenderPrimitive(3);
        RwIm3DEnd();
        RwRenderStateSet(10, (void*)rsv.SrcBlend);
        RwRenderStateSet(11, (void*)rsv.DestBlend);
        RwRenderStateSet(7, (void*)rsv.ShadeMode);
        RwRenderStateSet(8, (void*)((rsv.Flags >> 2) & 1));
        RwRenderStateSet(12, (void*)((rsv.Flags >> 3) & 1));
        sShadowVertCount = 0;
    }
}

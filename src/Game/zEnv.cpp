#include "xEnv.h"
#include "iFX.h"
#include "xstransvc.h"
#include "xCamera.h"
#include "iEnv.h"
#include "rwtypes.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zSurface.h"

class p2EnvAsset : public p2BaseAsset
{
public:
    unsigned int bspAssetID;           // 0x8
    unsigned int startCameraAssetID;   // 0xC
    unsigned int climateFlags;         // 0x10
    float climateStrengthMin;          // 0x14
    float climateStrengthMax;          // 0x18
    float padF1;                       // 0x1C
    float padF2;                       // 0x20
    float padF3;                       // 0x24
    unsigned int bspCollisionAssetID;  // 0x28
    unsigned int bspFXAssetID;         // 0x2C
    unsigned int bspCameraAssetID;     // 0x30
    unsigned int bspMapperID;          // 0x34
    unsigned int bspMapperCollisionID; // 0x38
    unsigned int bspMapperFXID;        // 0x3C
};

class _zEnv : public xBase
{
public:
    p2EnvAsset* easset; // 0x10
};

class _zGroup;
class RpMaterial;
class RwTexture;

class RpMaterialList
{
public:
    RpMaterial** materials;
    signed int numMaterials;
    signed int space;
};

class RpWorldLayout
{
public:
    unsigned char pad0[0x10];
    RpMaterialList matList; // 0x10
};

extern "C" RpMaterial* RpMaterialSetTexture(RpMaterial* material, RwTexture* texture);

unsigned int zGroupGetItem(_zGroup* g, unsigned int index);
void zLightResetAll(_xEnv* env);
void iLightInit(RpWorld* world);
void xCameraPickInitial(_tagxCamFrame* cam, _tagxCam* initial);
void zEntPlayer_StoreCheckPoint(float* pos);
void xClimateSetSnow(float stre);
void xClimateSetRain(float stre);

signed int zEnvEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                       xBase* toParamWidget);

static inline void zEnvReset(_zEnv* env)
{
    xBaseReset(env, env->easset);
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zEnvInit(_zEnv* env, p2EnvAsset* easset)
{
    unsigned int bufsize;
    void* buf;

    xBaseInit(env, easset);
    env->easset = easset;
    env->eventFunc = zEnvEventCB;
    if (env->linkCount)
    {
        env->link = (p2LinkAsset*)(env->easset + 1);
    }

    xSTAssetCountByType('BSP ');

    buf = xSTFindAsset(easset->bspAssetID, &bufsize);
    if (buf)
    {
        xEnvLoadBsp(globals.sceneCur->env, buf, bufsize, 0);
    }

    zSurfaceRegisterMapper(easset->bspMapperID);

    if (easset->bspCollisionAssetID)
    {
        buf = xSTFindAsset(easset->bspCollisionAssetID, &bufsize);
        if (buf)
        {
            xEnvLoadBsp(globals.sceneCur->env, buf, bufsize, 1);
        }
    }

    if (easset->bspFXAssetID)
    {
        buf = xSTFindAsset(easset->bspFXAssetID, &bufsize);
        if (buf)
        {
            xEnvLoadBsp(globals.sceneCur->env, buf, bufsize, 2);
        }
    }

    if (easset->bspCameraAssetID)
    {
        buf = xSTFindAsset(easset->bspCameraAssetID, &bufsize);
        if (buf)
        {
            xEnvLoadBsp(globals.sceneCur->env, buf, bufsize, 3);
        }
    }

    if (globals.sceneCur->env && globals.sceneCur->env->geom)
    {
        zLightResetAll(globals.sceneCur->env);
        iLightInit(globals.sceneCur->env->geom->world);
    }
}

void zEnvSetup(_zEnv* env)
{
    xBaseSetup();
    iEnvLightingBasics(globals.sceneCur->env->geom, (xEnvAsset*)env->easset);
}

void zEnvStartingCamera(_zEnv* env)
{
    _tagxCam* cam = (_tagxCam*)zSceneFindObject(env->easset->startCameraAssetID);
    if (cam)
    {
        xCameraPickInitial(&globals.camera, cam);
        zEntPlayer_StoreCheckPoint(0);
    }
}

void zEnvRender(_xEnv* env)
{
    RpWorld* world = env->geom->world;
    signed int num = ((RpWorldLayout*)world)->matList.numMaterials;

    for (signed int i = 0; i < num; i++)
    {
        xSurface* sp = zSurfaceGetSurface(i);
        zSurfaceProps* pp = (zSurfaceProps*)sp->moprops;

        if (pp && pp->asset && (pp->texanim_flags & 1))
        {
            RpMaterial* mp = ((RpWorldLayout*)world)->matList.materials[i];
            if (mp)
            {
                _zGroup* g = (_zGroup*)pp->texanim[0].group_ptr;
                if (g)
                {
                    unsigned int texid = zGroupGetItem(g, pp->texanim[0].group_idx);
                    RwTexture* texptr = (RwTexture*)xSTFindAsset(texid, 0);
                    if (texptr)
                    {
                        RpMaterialSetTexture(mp, texptr);
                    }
                }
            }
        }
    }

    xEnvRender(env);
}

void zEnvSave(_zEnv* ent, xSerial* s)
{
    xBaseSave(ent, s);
}

void zEnvLoad(_zEnv* ent, xSerial* s)
{
    xBaseLoad(ent, s);
}

signed int zEnvEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                       xBase* toParamWidget)
{
    switch (toEvent)
    {
    case 0xA:
        zEnvReset((_zEnv*)to);
        break;
    case 0x10A:
        iFXWarbSetParams(1, toParam);
        break;
    case 0x10B:
        iFXWarbSetParams(2, toParam);
        break;
    case 0x122:
        xClimateSetSnow(toParam[0]);
        break;
    case 0x121:
        xClimateSetRain(toParam[0]);
        break;
    }
    return 1;
}

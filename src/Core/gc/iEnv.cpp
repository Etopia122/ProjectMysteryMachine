#include "iEnv.h"
#include "iDraw.h"
#include "xMemMgr.h"

class RwCamera;

extern "C" {
void _rwFrameSyncDirty();
RpWorld* RpWorldDestroy(RpWorld* world);
RpWorld* RpWorldRender(RpWorld* world);
RpWorld* RpWorldAddCamera(RpWorld* world, RwCamera* camera);
RpWorld* RpWorldRemoveCamera(RpWorld* world, RwCamera* camera);
signed int RwRenderStateSet(signed int state, void* value);
// TODO: unidentified RenderWare world function; stores the vector at world+0x68
void fn_80151CA4(RpWorld* world, RwV3d* v);
}

// These live in .sbss interleaved with other units' variables, so they aren't
// defined here yet.
extern RwCamera* globalCamera;
extern signed int sBeginDrawFX;
extern iEnv* lastEnv;

static inline void iEnvSetBSP(iEnv* env, signed int envDataType, RpWorld* bsp)
{
    if (envDataType == 0)
    {
        env->world = bsp;
    }
    else if (envDataType == 1)
    {
        env->collision = bsp;
    }
    else if (envDataType == 2)
    {
        env->fx = bsp;
    }
    else if (envDataType == 3)
    {
        env->camera = bsp;
    }
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void iEnvLoad(iEnv* env, void* data, unsigned int datasize, signed int dataType)
{
    RwV3d v;

    iEnvSetBSP(env, dataType, (RpWorld*)data);
    v.x = 1.0f;
    v.z = 1.0f;
    v.y = 0.0f;
    fn_80151CA4((RpWorld*)data, &v);
    if (dataType == 0)
    {
        env->memlvl = xMemGetBase();
    }
}

void iEnvFree(iEnv* env)
{
    _rwFrameSyncDirty();
    RpWorldDestroy(env->world);
    env->world = 0;
    if (env->fx)
    {
        RpWorldDestroy(env->fx);
        env->fx = 0;
    }
    if (env->collision)
    {
        RpWorldDestroy(env->collision);
        env->collision = 0;
    }
}

void iEnvDefaultLighting(iEnv* env)
{
}

void iEnvLightingBasics(iEnv* env, xEnvAsset* asset)
{
}

void iEnvRender(iEnv* env)
{
    RwRenderStateSet(10, (void*)5);
    RwRenderStateSet(11, (void*)6);
    RpWorldRender(env->world);
    lastEnv = env;
}

signed int iEnvBeginRenderFX(iEnv* env)
{
    env = lastEnv;
    if (env->fx && globalCamera && !sBeginDrawFX)
    {
        RpWorldRemoveCamera(env->world, globalCamera);
        RpWorldAddCamera(env->fx, globalCamera);
        sBeginDrawFX = 1;
    }
    return sBeginDrawFX;
}

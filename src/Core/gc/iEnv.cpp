#include "iEnv.h"
#include "iDraw.h"
#include "xMemMgr.h"



extern "C" {
void _rwFrameSyncDirty();
RpWorld* RpWorldDestroy(RpWorld* world);
RpWorld* RpWorldRender(RpWorld* world);
RpWorld* RpWorldAddCamera(RpWorld* world, RwCamera* camera);
RpWorld* RpWorldRemoveCamera(RpWorld* world, RwCamera* camera);
signed int RwRenderStateSet(signed int state, void* value);
// TODO: unidentified RenderWare world function; stores the vector at world+0x68
void fn_80151CA4(RpWorld* world, RwV3d* v);
// TODO: unidentified GameCube render state functions (fn_80101D64 is called
// with the same kind of flag)
void fn_80101C6C(signed int enable);
void fn_80101CAC(signed int enable);
void fn_80101D64(signed int enable);
// TODO: misnamed RenderWare GameCube function (mode, optional state block)
void _rwDlTextureRasterFlush(signed int mode, volatile void* data);
void GXSetAlphaCompare(signed int comp0, unsigned char ref0, signed int op, signed int comp1,
                       unsigned char ref1);
}

// The same function is also called with only the mode.
namespace rwgcn1
{
extern "C" void _rwDlTextureRasterFlush(signed int mode);
}

// TODO: unidentified RenderWare GameCube state block; bytes 0x3C/0x3D are
// the alpha test settings.
class RwGameCubeAlphaState
{
public:
    unsigned char pad0[0x3C];
    unsigned char enable; // 0x3C
    unsigned char ref;    // 0x3D
};
extern volatile RwGameCubeAlphaState lbl_8025E348;

// These live in .sbss interleaved with other units' variables, so they aren't
// defined here yet.
extern RwCamera* globalCamera;
extern signed int sBeginDrawFX;
extern iEnv* lastEnv;
extern unsigned int lbl_8025E8A0;

static unsigned int sEnvFXEnabled = 1;
static unsigned int sEnvFXVisible = 1;
static unsigned char sEnvFXAlpha = 255;
static unsigned char sEnvFXFlags = 2;

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

void iEnvRenderFX(iEnv* env)
{
    env = lastEnv;
    if (env->fx && globalCamera)
    {
        RwRenderStateSet(10, (void*)5);
        RwRenderStateSet(11, (void*)6);
        RpWorldRender(env->fx);
    }
}

// TODO (97.5%): before the final flush the original loads the mode (li r3, 1)
// ahead of the two state stores; here it comes between them.
extern "C" void fn_8000A138()
{
    if (sEnvFXEnabled)
    {
        iEnv* env = lastEnv;
        if (sEnvFXVisible && env->fx && globalCamera)
        {
            if (lbl_8025E8A0)
            {
                RwRenderStateSet(10, (void*)5);
                RwRenderStateSet(11, (void*)2);
            }
            else
            {
                RwRenderStateSet(10, (void*)5);
                RwRenderStateSet(11, (void*)6);
            }
            RwRenderStateSet(6, (void*)1);
            RwRenderStateSet(8, (void*)0);
            if (sEnvFXFlags & 2)
            {
                rwgcn1::_rwDlTextureRasterFlush(4);
                GXSetAlphaCompare(6, 0, 0, 3, sEnvFXAlpha);
                fn_80101D64(0);
            }
            else
            {
                lbl_8025E348.enable = 0;
                lbl_8025E348.ref = sEnvFXAlpha;
                _rwDlTextureRasterFlush(3, &lbl_8025E348);
            }
            RpWorldRender(env->fx);
            lbl_8025E348.enable = 0;
            lbl_8025E348.ref = 0;
            _rwDlTextureRasterFlush(1, &lbl_8025E348);
            RwRenderStateSet(6, (void*)1);
            RwRenderStateSet(8, (void*)1);
        }
    }
}

void iEnvEndRenderFX(iEnv* env)
{
    env = lastEnv;
    if (env->fx && globalCamera && sBeginDrawFX)
    {
        RpWorldRemoveCamera(env->fx, globalCamera);
        RpWorldAddCamera(env->world, globalCamera);
        sBeginDrawFX = 0;
    }
}

extern "C" void fn_8000A080(unsigned int mode)
{
    if (mode == -1)
    {
        fn_80101C6C(0);
        fn_80101CAC(0);
    }
    else if (mode == 0)
    {
        fn_80101C6C(1);
        fn_80101CAC(1);
    }
}

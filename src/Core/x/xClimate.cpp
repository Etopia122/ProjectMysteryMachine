#include "xMath3.h"
#include "xString.h"
#include "iMath.h"
#include "zGlobals.h"
#include "zEnt.h"
#include "zParEmitter.h"
#include "rwtypes.h"

extern "C" void* memcpy(void* dst, const void* src, unsigned int n);

class p2EnvAsset : public p2BaseAsset
{
public:
    unsigned int bspAssetID;     // 0x8
    unsigned int startCameraAssetID; // 0xC
    unsigned int climateFlags;   // 0x10
    float climateStrengthMin;    // 0x14
    float climateStrengthMax;    // 0x18
};

class _tagRain
{
public:
    signed int rain;
    float strength;
    _zParEmitter* rain_emitter;
    _zParEmitter* snow_emitter;
};

class _tagWind
{
public:
    float strength;
    float angle;
    _xVec3 dir;
};

class _tagClimate
{
public:
    _tagRain rain;
    _tagWind wind;
};

class zScene
{
public:
    unsigned char pad0[0x6C];
    unsigned int sceneID; // 0x6C
};

#define CLIMATE_SCENE_ID(s) ((((s)[0] & ~0x20) << 24) | ((s)[1] << 16) | ((s)[2] << 8) | (s)[3])

static _tagClimate* sClimate;

static inline void xMat3x3Copy(_xMat3x3* m1, const _xMat3x3* m2)
{
    memcpy(m1, m2, sizeof(_xMat3x3));
}

static inline void xMat3x3Identity(_xMat3x3* m)
{
    xMat3x3Copy(m, &g_I3);
}

static inline void xVec3Init(_xVec3* v, float x, float y, float z)
{
    v->x = x;
    v->y = y;
    v->z = z;
}

static inline void xClimateVecFromAngle(float angleDegrees, _xVec3* vec)
{
    _xMat3x3 Mat;

    xMat3x3Identity(&Mat);
    xMat3x3Euler(&Mat, (3.1415927f * angleDegrees) / 180.0f, 0.0f, 0.0f);
    xVec3Init(vec, 0.0f, 0.0f, 1.0f);
    xMat3x3LMulVec(vec, &Mat, vec);
}

static inline void GetPosBigDogWhattupFool(_xVec3* vec)
{
    _xMat4x3* mat = &globals.camera.mat;
    vec->x = 10.0f * mat->at.x + mat->pos.x;
    vec->y = 10.0f * mat->at.y + mat->pos.y;
    vec->z = 10.0f * mat->at.z + mat->pos.z;
}

static inline void UpdateWind(_tagClimate* climate, float seconds)
{
}

void UpdateRain(_tagClimate* climate, float seconds);

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xClimateInit(_tagClimate* climate)
{
    climate->rain.strength = 0.0f;

    climate->rain.rain_emitter = zParEmitterFind(xStrHash("PAREMIT_RAIN"));
    climate->rain.rain_emitter->emit_flags &= 0xFE;

    climate->rain.snow_emitter = zParEmitterFind(xStrHash("PAREMIT_SNOW"));
    climate->rain.snow_emitter->emit_flags &= 0xFE;
}

void xClimateInitAsset(_tagClimate* climate, p2EnvAsset* asset)
{
    sClimate = climate;
    climate->wind.strength = 0.0f;
    xClimateVecFromAngle(climate->wind.angle, &climate->wind.dir);

    if (asset->climateFlags == 0)
    {
        climate->wind.strength = 0.0f;
        climate->rain.strength = 0.0f;
        return;
    }
    if (asset->climateFlags & 1)
    {
        climate->rain.rain = 1;
        climate->rain.strength = 0.5f * (asset->climateStrengthMax - asset->climateStrengthMin);
        climate->rain.strength += asset->climateStrengthMin;
    }
    else if (asset->climateFlags & 2)
    {
        climate->rain.rain = 0;
        climate->rain.strength = 0.5f * (asset->climateStrengthMax - asset->climateStrengthMin);
        climate->rain.strength += asset->climateStrengthMin;
    }
}

void xClimateSetSnow(float stre)
{
    sClimate->rain.rain = 0;
    sClimate->rain.strength = stre;
}

void xClimateSetRain(float stre)
{
    sClimate->rain.rain = 1;
    sClimate->rain.strength = stre;
}

// TODO (99.2%): float registers are off by one. The original gives 22.5f
// f31 and 1.0f f23 in the snow loop; here 1.0f takes f31.
void UpdateRain(_tagClimate* climate, float seconds)
{
    _tagRain* r = &climate->rain;
    _zParEmitterCustomSettings info;
    info.custom_flags = 0x20;

    if (r->rain)
    {
        signed int total_rain_drops = 25.0f * r->strength;
        if (globals.sceneCur->sceneID == CLIMATE_SCENE_ID("R004"))
        {
            for (signed int i = 0; i < 5; i++)
            {
                info.pos.x = 99.9643f;
                info.pos.y = 32.8615f;
                info.pos.z = -177.306f;
                info.pos.x += 25.0f * iurand() - 12.5f;
                info.pos.y += 10.0f;
                info.pos.z += 25.0f * iurand() - 12.5f;
                zParEmitterEmitCustom(r->rain_emitter, seconds, &info);
            }
            for (signed int i = 0; i < 5; i++)
            {
                info.pos = *(_xVec3*)&globals.player.ent.model->Mat->pos;
                info.pos.x += 25.0f * iurand() - 12.5f;
                info.pos.y += 10.0f;
                info.pos.z += 25.0f * iurand() - 12.5f;
                zParEmitterEmitCustom(r->rain_emitter, seconds, &info);
            }
            for (signed int i = 0; i < total_rain_drops; i++)
            {
                GetPosBigDogWhattupFool(&info.pos);
                info.pos.x += 25.0f * iurand() - 12.5f;
                info.pos.y += 10.0f;
                info.pos.z += 25.0f * iurand() - 12.5f;
                zParEmitterEmitCustom(r->rain_emitter, seconds, &info);
            }
        }
        else
        {
            for (signed int i = 0; i < total_rain_drops; i++)
            {
                GetPosBigDogWhattupFool(&info.pos);
                info.pos.x += 25.0f * iurand() - 12.5f;
                info.pos.y += 10.0f;
                info.pos.z += 25.0f * iurand() - 12.5f;
                zParEmitterEmitCustom(r->rain_emitter, seconds, &info);
            }
        }
        return;
    }

    _xVec3 fool;
    signed int total_snow_flakes = 25.0f * r->strength;
    info.custom_flags |= 1;
    GetPosBigDogWhattupFool(&fool);
    for (signed int i = 0; i < total_snow_flakes; i++)
    {
        info.pos = fool;
        info.pos.x += 45.0f * iurand() - 22.5f;
        info.pos.z += 45.0f * iurand() - 22.5f;

        float xx = info.pos.x - fool.x;
        float zz = info.pos.z - fool.z;
        float perc = 1.0f - xx * zz / 506.25f;
        info.pos.y += 5.0f * perc + 5.0f;
        info.life = 2.5f * perc + 2.5f;
        zParEmitterEmitCustom(r->snow_emitter, seconds, &info);
    }
}

void xClimateUpdate(_tagClimate* climate, float seconds)
{
    UpdateRain(climate, seconds);
    UpdateWind(climate, seconds);
}

void xClimateGetRealWindDir(_tagClimate* climate, _xVec3* dir)
{
    dir->x = climate->wind.dir.x * climate->wind.strength;
    dir->y = 0.0f;
    dir->z = climate->wind.dir.z * climate->wind.strength;
}

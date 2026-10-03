#include "zGlobals.h"
#include "iMath.h"

class _xFColor
{
public:
    float r; // 0x0
    float g; // 0x4
    float b; // 0x8
    float a; // 0xC
};

class iLight
{
public:
    unsigned int type;  // 0x0
    void* hw;           // 0x4
    _xSphere sph;       // 0x8
    float radius_sq;    // 0x18
    _xFColor color;     // 0x1C
    _xVec3 dir;         // 0x2C
    float coneangle;    // 0x38
};

class zLightAsset : public p2BaseAsset
{
public:
    unsigned char lightType;   // 0x8
    unsigned char lightEffect; // 0x9
    unsigned char lightPad[2]; // 0xA
    unsigned int lightFlags;   // 0xC
    float lightColor[4];       // 0x10
    _xVec3 lightDir;           // 0x20
    float lightConeAngle;      // 0x2C
    _xSphere lightSphere;      // 0x30
    unsigned int attachID;     // 0x40
};

class _zLight : public xBase
{
public:
    unsigned int flags;    // 0x10
    zLightAsset* tasset;   // 0x14
    iLight light;          // 0x18
    xBase* attached_to;    // 0x54
    signed int true_idx;   // 0x58
    float* reg;            // 0x5C
    signed int effect_idx; // 0x60
};

void iLightSetColor(iLight* light, _xFColor* col);
void zLightOn(_zLight* zl, signed int on);

static inline float leGetRandom()
{
    float ret = iurand();
    return ret;
}

void zLightEffectInitStrobe(_zLight* zlight)
{
}

void zLightEffectInitDim(_zLight* zlight)
{
}

void zLightEffectInitHalfDim(_zLight* zlight)
{
}

void zLightEffectInitRandomCol(_zLight* zlight)
{
}

void zLightEffectInitFlicker(_zLight* zlight)
{
    *zlight->reg = 0.0f;
    zLightOn(zlight, 1);
}

static inline void EffectFlicker(_zLight* zlight, float seconds, float min, float rnd)
{
    iLight* l = &zlight->light;
    float* reg = zlight->reg;

    *reg -= seconds;

    if (*reg <= 0.0f)
    {
        *reg = rnd * leGetRandom();
        *reg += min;

        l->color.r = zlight->tasset->lightColor[0] - (leGetRandom() * 0.2f + 0.1f);
        l->color.g = zlight->tasset->lightColor[1] - (leGetRandom() * 0.2f + 0.1f);
        l->color.b = zlight->tasset->lightColor[2] - (leGetRandom() * 0.2f + 0.1f);

        if (l->color.r > 1.0f)
        {
            l->color.r = 1.0f;
        }
        else if (l->color.r < 0.0f)
        {
            l->color.r = 0.0f;
        }

        if (l->color.g > 1.0f)
        {
            l->color.g = 1.0f;
        }
        else if (l->color.g < 0.0f)
        {
            l->color.g = 0.0f;
        }

        if (l->color.b > 1.0f)
        {
            l->color.b = 1.0f;
        }
        else if (l->color.b < 0.0f)
        {
            l->color.b = 0.0f;
        }

        iLightSetColor(l, &l->color);
    }
}

void zLightEffectFlicker(_zLight* zlight, float seconds)
{
    EffectFlicker(zlight, seconds, 0.05f, 0.2f);
}

void zLightEffectFlickerSlow(_zLight* zlight, float seconds)
{
    EffectFlicker(zlight, seconds, 0.2f, 0.3f);
}

void zLightEffectFlickerErratic(_zLight* zlight, float seconds)
{
    EffectFlicker(zlight, seconds, 0.0f, 0.1f);
}

void zLightEffectStrobeSlow(_zLight*, float)
{
}

void zLightEffectStrobe(_zLight*, float)
{
}

void zLightEffectStrobeFast(_zLight*, float)
{
}

void zLightEffectDimSlow(_zLight*, float)
{
}

void zLightEffectDim(_zLight*, float)
{
}

void zLightEffectDimFast(_zLight*, float)
{
}

void zLightEffectHalfDimSlow(_zLight*, float)
{
}

void zLightEffectHalfDim(_zLight*, float)
{
}

void zLightEffectHalfDimFast(_zLight*, float)
{
}

void zLightEffectRandomColSlow(_zLight*, float)
{
}

void zLightEffectRandomCol(_zLight*, float)
{
}

void zLightEffectRandomColFast(_zLight*, float)
{
}

void zLightEffectInitCauldron(_zLight* zlight)
{
    *zlight->reg = 0.0f;
    zLightOn(zlight, 1);
}

static inline float leBlendToCol(float f1, float f2, float f3)
{
    if (f1 > f2)
    {
        f1 -= f3;
        if (f1 < f2)
        {
            f1 = f2;
        }
    }
    else if (f1 < f2)
    {
        f1 += f3;
        if (f1 > f2)
        {
            f1 = f2;
        }
    }
    return f1;
}

void zLightEffectCauldron(_zLight* zlight, float seconds)
{
    iLight* l = &zlight->light;
    float* reg = zlight->reg;

    *reg -= seconds;

    if (*reg <= 0.0f)
    {
        reg[0] = 0.25f;
        reg[1] = leGetRandom() * 0.5f;
        reg[2] = leGetRandom() * 0.4f + 0.6f;
        reg[3] = leGetRandom() * 0.5f;
    }

    float amount = seconds * 3.0f;
    l->color.r = leBlendToCol(l->color.r, reg[1], amount);
    l->color.g = leBlendToCol(l->color.g, reg[2], amount);
    l->color.b = leBlendToCol(l->color.b, reg[3], amount);

    iLightSetColor(l, &l->color);
}

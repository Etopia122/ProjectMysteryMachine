#include "xMath3.h"
#include "xString.h"
#include "xSnd.h"
#include "iMath.h"
#include "iCamera.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zParEmitter.h"

extern "C" {
void* memset(void* dst, int c, unsigned int n);
int sprintf(char* s, const char* format, ...);
}

class p2EnvAsset : public p2BaseAsset
{
public:
    unsigned int bspAssetID;         // 0x8
    unsigned int startCameraAssetID; // 0xC
    unsigned int climateFlags;       // 0x10
    float climateStrengthMin;        // 0x14
    float climateStrengthMax;        // 0x18
};

class _zEnv : public xBase
{
public:
    p2EnvAsset* easset; // 0x10
};

class zFogAsset : public p2BaseAsset
{
public:
    unsigned char bkgndColor[4]; // 0x8
    unsigned char fogColor[4];   // 0xC
    float fogDensity;            // 0x10
    float fogStart;              // 0x14
    float fogStop;               // 0x18
};

class _zFog : public xBase
{
public:
    zFogAsset* tasset; // 0x10
};

class _tagFirework
{
public:
    signed int state; // 0x0
    float timer;      // 0x4
    _xVec3 vel;       // 0x8
    _xVec3 pos;       // 0x14
    float fuel;       // 0x20
};

class _tagClimate;

signed int iGetDay();
signed int iGetMonth();
signed int zGameIsPaused();
xBase* zSceneGetObject(signed int type, signed int idx);
unsigned int iSndIsPlayingByID(unsigned int id);
void zRumbleStartDistance(signed int type, float dist, float maxdist);
void xClimateInitAsset(_tagClimate* climate, p2EnvAsset* asset);

extern _zEnv* gCurEnv;
extern _tagClimate gClimate;

#define SCENE_ID(s) ((((s)[0] & ~0x20) << 24) | ((s)[1] << 16) | ((s)[2] << 8) | (s)[3])

static signed int sGameExtrasOn;
static signed int sDay;
static signed int sMonth;
static signed int sFireworksOn;
static signed int sColoredFountain;
static _zParEmitter* sFireworkTrailEmit;
static _zParEmitter* sFirework1Emit;
static _zParEmitter* sFirework2Emit;
static _zFog* sFog;
static float sFogIntensity;
static float sFogPrevIntensity = 1.0f;
static signed int sThunderOn;
static unsigned int sThunderFar1;
static unsigned int sThunderFar2;
static unsigned int sThunderNear1;
static unsigned int sThunderNear2;
static unsigned int sThunderNear3;
static unsigned int sThunderNear4;
static _tagFirework sFirework[10];

static inline unsigned char zGameExtrasFogByte(unsigned char c, float intensity)
{
    float f = c * intensity;
    if (f > 255.0f)
    {
        f = 255.0f;
    }
    else if (f < 0.0f)
    {
        f = 0.0f;
    }
    return f;
}

static inline void zGameExtrasSetFog(iFogParams* fog, float intensity)
{
    fog->type = (RwFogType)1;
    fog->start = sFog->tasset->fogStart;
    fog->stop = sFog->tasset->fogStop;
    fog->density = sFog->tasset->fogDensity;
    fog->fogcolor.red = zGameExtrasFogByte(sFog->tasset->fogColor[0], intensity);
    fog->fogcolor.green = zGameExtrasFogByte(sFog->tasset->fogColor[1], intensity);
    fog->fogcolor.blue = zGameExtrasFogByte(sFog->tasset->fogColor[2], intensity);
    fog->fogcolor.alpha = zGameExtrasFogByte(sFog->tasset->fogColor[3], intensity);
    fog->bgcolor.red = zGameExtrasFogByte(sFog->tasset->bkgndColor[0], intensity);
    fog->bgcolor.green = zGameExtrasFogByte(sFog->tasset->bkgndColor[1], intensity);
    fog->bgcolor.blue = zGameExtrasFogByte(sFog->tasset->bkgndColor[2], intensity);
    fog->bgcolor.alpha = zGameExtrasFogByte(sFog->tasset->bkgndColor[3], intensity);
    fog->table = 0;
    iCameraSetFogParams(fog, 0.0f);
}

static inline unsigned char zGameExtrasRandByte()
{
    return 127.0f * iurand();
}

static inline void zGameExtrasSetColor(zParEmitterAsset* a, unsigned char r, unsigned char g,
                                       unsigned char b)
{
    a->color_death[0] = r;
    a->color_birth[0] = r;
    a->color_death[1] = g;
    a->color_birth[1] = g;
    a->color_death[2] = b;
    a->color_birth[2] = b;
}

static void UpdateFireworks(float dt);
static void UpdateThunder(float dt);
static void UpdateColoredFountain(float dt);
static void Init4thJuly();
static void InitStPatricksDay();
static void InitThunderSounds();
static void StartFireworks();
static void InitHalloween();
static void InitSnow();

void zGameExtrasInit()
{
    sFireworkTrailEmit = 0;
    sFirework1Emit = 0;
    sFirework2Emit = 0;
    sFireworksOn = 0;
    sColoredFountain = 0;
    sGameExtrasOn = 0;
    sDay = 0;
    sMonth = 0;
    sFog = 0;
    sFogIntensity = 0.0f;
    sFogPrevIntensity = 1.0f;
    sThunderOn = 0;
    InitThunderSounds();

    sFog = (_zFog*)zSceneGetObject(0x24, 0);
    sThunderOn = sFog != 0;
    if (sGameExtrasOn == 0)
    {
        sGameExtrasOn = sThunderOn;
    }

    sDay = iGetDay();
    sMonth = iGetMonth();

    if (globals.sceneCur->sceneID == SCENE_ID("H001") ||
        globals.sceneCur->sceneID == SCENE_ID("H002"))
    {
        switch (sMonth)
        {
        case 1:
            if (sDay == 1)
            {
                StartFireworks();
                return;
            }
            break;
        case 2:
            if (sDay == 2)
            {
                StartFireworks();
                return;
            }
            break;
        case 3:
            if (sDay == 17)
            {
                InitStPatricksDay();
            }
            if (sDay == 22)
            {
                StartFireworks();
                return;
            }
            break;
        case 5:
            if (sDay == 30)
            {
                StartFireworks();
                return;
            }
            break;
        case 7:
            if (sDay == 4)
            {
                Init4thJuly();
                StartFireworks();
                return;
            }
            break;
        case 8:
            if (sDay == 28)
            {
                StartFireworks();
                sColoredFountain = 1;
                sGameExtrasOn = 1;
                return;
            }
            break;
        case 9:
            if (sDay == 8)
            {
                StartFireworks();
                return;
            }
            break;
        case 10:
            if (sDay == 31)
            {
                InitHalloween();
                return;
            }
            break;
        case 11:
            if (sDay == 5)
            {
                StartFireworks();
                return;
            }
            break;
        case 12:
            if (sDay == 31)
            {
                StartFireworks();
                return;
            }
            if (sDay == 20)
            {
                StartFireworks();
                return;
            }
            InitSnow();
            break;
        }
    }
}

#pragma auto_inline on
static void InitSnow()
{
    if (gCurEnv != 0)
    {
        gCurEnv->easset->climateFlags = 2;
        gCurEnv->easset->climateStrengthMin = 0.45f;
        gCurEnv->easset->climateStrengthMax = 0.55f;
        if (sDay == 25)
        {
            gCurEnv->easset->climateStrengthMin = 0.75f;
            gCurEnv->easset->climateStrengthMax = 1.0f;
            xClimateInitAsset(&gClimate, gCurEnv->easset);
        }
        else if (sDay == 14)
        {
            gCurEnv->easset->climateStrengthMin = 0.99f;
            gCurEnv->easset->climateStrengthMax = 1.0f;
            xClimateInitAsset(&gClimate, gCurEnv->easset);
        }
    }
}
#pragma auto_inline reset

void zGameExtras(float dt)
{
    if (sGameExtrasOn && globals.cmgr == 0 && !zGameIsPaused())
    {
        UpdateColoredFountain(dt);
        UpdateFireworks(dt);
        UpdateThunder(dt);
    }
}

void zGameExtrasReset()
{
    sGameExtrasOn = 0;
    sDay = 0;
    sMonth = 0;
    sFireworksOn = 0;
    sColoredFountain = 0;
}

static void InitStPatricksDay()
{
    char emitter_txt[64];
    signed int i;
    zParEmitterAsset* a;
    _zParEmitter* fe;

    fe = zParEmitterFind("FOUNTAIN EMITTER");
    if (fe == 0)
    {
        return;
    }

    zGameExtrasSetColor(fe->tasset, 9, 110, 21);
    zGameExtrasSetColor(zParEmitterFind("FOUNTAIN EMITTER TOP")->tasset, 9, 110, 21);
    zGameExtrasSetColor(zParEmitterFind("FOUNTAIN EMITTER MID")->tasset, 9, 110, 21);
    zGameExtrasSetColor(zParEmitterFind("FOUNTAIN DOG ON TOP")->tasset, 9, 110, 21);

    for (i = 1; i <= 7; i++)
    {
        sprintf(emitter_txt, "CHIMNEY EMITTER %d", i);
        fe = zParEmitterFind(emitter_txt);
        if (fe != 0)
        {
            a = fe->tasset;
            if (a != 0)
            {
                zGameExtrasSetColor(a, 9, 110, 21);
            }
        }
    }
}

static void Init4thJuly()
{
    char emitter_txt[64];
    signed int idx;
    signed int i;
    zParEmitterAsset* a;
    _zParEmitter* fe;

    fe = zParEmitterFind("FOUNTAIN EMITTER");
    if (fe == 0)
    {
        return;
    }

    zGameExtrasSetColor(fe->tasset, 32, 172, 252);
    zGameExtrasSetColor(zParEmitterFind("FOUNTAIN EMITTER MID")->tasset, 255, 255, 255);
    zGameExtrasSetColor(zParEmitterFind("FOUNTAIN DOG ON TOP")->tasset, 212, 0, 25);
    zGameExtrasSetColor(zParEmitterFind("FOUNTAIN EMITTER TOP")->tasset, 212, 0, 25);

    idx = 0;
    for (i = 1; i <= 7; i++)
    {
        sprintf(emitter_txt, "CHIMNEY EMITTER %d", i);
        fe = zParEmitterFind(emitter_txt);
        if (fe != 0)
        {
            a = fe->tasset;
            if (a != 0)
            {
                if (idx == 0)
                {
                    zGameExtrasSetColor(a, 255, 255, 255);
                }
                else if (idx == 1)
                {
                    zGameExtrasSetColor(a, 212, 0, 25);
                }
                else
                {
                    zGameExtrasSetColor(a, 32, 172, 252);
                }
            }
            idx++;
            if (idx > 2)
            {
                idx = 0;
            }
        }
    }
}

#pragma auto_inline on
static void InitHalloween()
{
    char name[64];
    for (signed int i = 0; i < 20; i++)
    {
        sprintf(name, "BAT HANG %d", i);
        _xEnt* bat = (_xEnt*)zSceneFindObject(xStrHash(name));
        if (bat != 0)
        {
            bat->flags |= 1;
            bat->baseFlags |= 8;
        }
    }
}

static void StartFireworks()
{
    sThunderOn = 0;
    sFireworkTrailEmit = zParEmitterFind("PAREMIT_FIREWORKS_TRAIL");
    sFirework1Emit = zParEmitterFind("PAREMIT_FIREWORKS1");
    sFirework2Emit = zParEmitterFind("PAREMIT_FIREWORKS2");
    memset(sFirework, 0, sizeof(sFirework));
    for (signed int i = 0; i < 10; i++)
    {
        sFirework[i].state = 0;
        sFirework[i].timer = 5.0f * iurand();
    }
    sFireworksOn = 1;
    sGameExtrasOn = 1;
    sFog = (_zFog*)zSceneFindObject(xStrHash("LIGHT_FOG"));
}
#pragma auto_inline reset

static void UpdateColoredFountain(float dt)
{
    static float ftimer = 0.0f;
    zParEmitterAsset* a;
    _zParEmitter* fe;

    if (sColoredFountain)
    {
        ftimer -= dt;
        if (ftimer <= 0.0f)
        {
            ftimer = 3.0f;
            fe = zParEmitterFind("FOUNTAIN EMITTER");
            if (fe != 0)
            {
                a = fe->tasset;
                a->color_birth[0] = a->color_death[0] = 127.0f * iurand() + 128.0f;
                a->color_birth[1] = a->color_death[1] = 127.0f * iurand() + 128.0f;
                a->color_birth[2] = a->color_death[2] = 127.0f * iurand() + 128.0f;
                a = zParEmitterFind("FOUNTAIN EMITTER TOP")->tasset;
                a->color_birth[0] = a->color_death[0] = 127.0f * iurand() + 128.0f;
                a->color_birth[1] = a->color_death[1] = 127.0f * iurand() + 128.0f;
                a->color_birth[2] = a->color_death[2] = 127.0f * iurand() + 128.0f;
                a = zParEmitterFind("FOUNTAIN EMITTER MID")->tasset;
                a->color_birth[0] = a->color_death[0] = 127.0f * iurand() + 128.0f;
                a->color_birth[1] = a->color_death[1] = 127.0f * iurand() + 128.0f;
                a->color_birth[2] = a->color_death[2] = 127.0f * iurand() + 128.0f;
                a = zParEmitterFind("FOUNTAIN DOG ON TOP")->tasset;
                a->color_birth[0] = a->color_death[0] = 127.0f * iurand() + 128.0f;
                a->color_birth[1] = a->color_death[1] = 127.0f * iurand() + 128.0f;
                a->color_birth[2] = a->color_death[2] = 127.0f * iurand() + 128.0f;
            }
        }
    }
}

static void UpdateThunder(float dt)
{
    iFogParams fog;

    if (!sThunderOn)
    {
        return;
    }

    if (sFog != 0)
    {
        float intensity = sFogIntensity;
        if (intensity != sFogPrevIntensity)
        {
            if (1.0f != intensity && irand() % 10 == 8)
            {
                sFogIntensity += iurand();
            }
            zGameExtrasSetFog(&fog, intensity);
            sFogPrevIntensity = sFogIntensity;
        }
    }

    sFogIntensity *= 0.9f;
    if (sFogIntensity < 1.0f)
    {
        sFogIntensity = 1.0f;
    }
    if (iSndIsPlayingByID(sThunderFar1) || iSndIsPlayingByID(sThunderFar2))
    {
        sFogIntensity += 0.1f;
    }
    if (iSndIsPlayingByID(sThunderNear1) || iSndIsPlayingByID(sThunderNear2) ||
        iSndIsPlayingByID(sThunderNear3) || iSndIsPlayingByID(sThunderNear4))
    {
        sFogIntensity += 0.3f;
    }
    if (sFogIntensity > 8.0f)
    {
        sFogIntensity = 8.0f;
    }
}

#pragma auto_inline on
static void InitThunderSounds()
{
    sThunderFar1 = xStrHash("thunder_distant1");
    sThunderFar2 = xStrHash("thunder_distant4");
    sThunderNear1 = xStrHash("thunder1");
    sThunderNear2 = xStrHash("thunder2");
    sThunderNear3 = xStrHash("thunder3");
    sThunderNear4 = xStrHash("thunder4");
}
#pragma auto_inline reset

// TODO: 99.0%. The original zero-extends each random colour byte (clrlwi)
// before adding 128.
static void UpdateFireworks(float dt)
{
    static signed int july_4_rgb = 0.0f;
    static _xVec3 h001_launch[5] = {
        { -29.14f, 2.30486f, 47.4601f },  { -27.533f, 2.30486f, 64.1456f },
        { -11.4041f, 2.30486f, 64.6424f }, { -40.0325f, 2.30486f, 46.0661f },
        { -40.0091f, 2.30486f, 48.8285f },
    };
    static _xVec3 h002_launch[5] = {
        { -51.8615f, 2.0f, -7.29103f }, { -48.333f, 2.0f, -18.33f },
        { -51.533f, 2.0f, -5.72145f },  { -57.5795f, 2.0f, 9.16543f },
        { -53.623f, 2.0f, 21.6936f },
    };
    iFogParams fog;
    signed int i;
    _zParEmitterCustomSettings trail_info;
    _zParEmitter* femit;
    _zParEmitterCustomSettings xplo_info;

    if (!sFireworksOn)
    {
        return;
    }

    if (sFog != 0 && sFogIntensity != sFogPrevIntensity)
    {
        fog.type = (RwFogType)1;
        fog.start = sFog->tasset->fogStart;
        fog.stop = sFog->tasset->fogStop -
                   0.4f * sFog->tasset->fogStop * ((4.5f - sFogIntensity) / 4.5f);
        fog.density = sFog->tasset->fogDensity;
        fog.fogcolor.red = zGameExtrasFogByte(sFog->tasset->fogColor[0], sFogIntensity);
        fog.fogcolor.green = zGameExtrasFogByte(sFog->tasset->fogColor[1], sFogIntensity);
        fog.fogcolor.blue = zGameExtrasFogByte(sFog->tasset->fogColor[2], sFogIntensity);
        fog.fogcolor.alpha = zGameExtrasFogByte(sFog->tasset->fogColor[3], sFogIntensity);
        fog.bgcolor.red = zGameExtrasFogByte(sFog->tasset->bkgndColor[0], sFogIntensity);
        fog.bgcolor.green = zGameExtrasFogByte(sFog->tasset->bkgndColor[1], sFogIntensity);
        fog.bgcolor.blue = zGameExtrasFogByte(sFog->tasset->bkgndColor[2], sFogIntensity);
        fog.bgcolor.alpha = zGameExtrasFogByte(sFog->tasset->bkgndColor[3], sFogIntensity);
        fog.table = 0;
        iCameraSetFogParams(&fog, 0.0f);
        sFogPrevIntensity = sFogIntensity;
    }

    sFogIntensity *= 0.97f;
    if (sFogIntensity < 0.5f)
    {
        sFogIntensity = 0.5f;
    }

    for (i = 0; i < 10; i++)
    {
        _tagFirework* f = &sFirework[i];
        if (f->state == 0)
        {
            f->timer -= dt;
            if (f->timer <= 0.0f)
            {
                signed int n = 5.0f * iurand();
                _xVec3* launch;
                if (n < 0)
                {
                    n = 0;
                }
                else if (n >= 5)
                {
                    n = 4;
                }
                if (globals.sceneCur->sceneID == SCENE_ID("H001"))
                {
                    launch = &h001_launch[n];
                }
                else
                {
                    launch = &h002_launch[n];
                }
                if (launch != 0)
                {
                    f->pos.x = launch->x;
                    f->pos.y = launch->y;
                    f->pos.z = launch->z;
                }
                else
                {
                    f->pos.x = -29.14f;
                    f->pos.y = 2.30486f;
                    f->pos.z = 47.4601f;
                }
                f->vel.x = 13.0f * iurand() + -6.5f;
                f->vel.y = 0.0f;
                f->vel.z = 13.0f * iurand() + -6.5f;
                f->fuel = 0.5f * iurand() + 1.5f;
                f->state = 1;
            }
        }
        else
        {
            f->fuel -= dt;
            if (f->fuel > 0.0f)
            {
                f->vel.y += 15.0f * dt;
            }
            trail_info.custom_flags = 0x20;
            f->pos.x += f->vel.x * dt;
            f->pos.y += f->vel.y * dt;
            f->pos.z += f->vel.z * dt;
            trail_info.pos = f->pos;
            zParEmitterEmitCustom(sFireworkTrailEmit, dt, &trail_info);

            if (f->fuel <= 0.0f)
            {
                f->state = 0;
                f->timer = 5.0f * iurand() + 2.5f;
                femit = sFirework1Emit;
                if (iurand() < 0.75f)
                {
                    femit = sFirework2Emit;
                }
                xplo_info.custom_flags = 0x1A0;
                xplo_info.pos = f->pos;
                if (sDay == 4 && sMonth == 7)
                {
                    if (july_4_rgb == 0)
                    {
                        xplo_info.color_birth[0] = xplo_info.color_death[0] = 212;
                        xplo_info.color_birth[1] = xplo_info.color_death[1] = 0;
                        xplo_info.color_birth[2] = xplo_info.color_death[2] = 25;
                    }
                    else if (july_4_rgb == 1)
                    {
                        xplo_info.color_birth[0] = xplo_info.color_death[0] = 255;
                        xplo_info.color_birth[1] = xplo_info.color_death[1] = 255;
                        xplo_info.color_birth[2] = xplo_info.color_death[2] = 255;
                    }
                    else
                    {
                        xplo_info.color_birth[0] = xplo_info.color_death[0] = 32;
                        xplo_info.color_birth[1] = xplo_info.color_death[1] = 172;
                        xplo_info.color_birth[2] = xplo_info.color_death[2] = 252;
                    }
                    july_4_rgb++;
                    if (july_4_rgb > 2)
                    {
                        july_4_rgb = 0;
                    }
                }
                else
                {
                    xplo_info.color_birth[0] = xplo_info.color_death[0] =
                        zGameExtrasRandByte() + 128;
                    xplo_info.color_birth[1] = xplo_info.color_death[1] =
                        zGameExtrasRandByte() + 128;
                    xplo_info.color_birth[2] = xplo_info.color_death[2] =
                        zGameExtrasRandByte() + 128;
                    f->timer += 5.0f * iurand();
                }
                xplo_info.color_birth[3] = 255;
                xplo_info.color_death[3] = 0;
                zParEmitterEmitCustom(femit, dt, &xplo_info);

                float dx = globals.player.ent.model->Mat->pos.x - f->pos.x;
                float dz = globals.player.ent.model->Mat->pos.z - f->pos.z;
                zRumbleStartDistance(0, dx * dx + dz * dz, 2300.0f);
                f->pos.y = globals.player.ent.model->Mat->pos.y;
                if (irand() & 1)
                {
                    xSndPlay(xStrHash("cannon_fire1"), 0.5f, 0.5f, 0.0f, 0x80, 2, 0);
                }
                else
                {
                    xSndPlay(xStrHash("cannon_fire2"), 0.5f, 0.5f, 0.0f, 0x80, 2, 0);
                }
                sFogIntensity += f->timer;
                if (sFogIntensity > 4.5f)
                {
                    sFogIntensity = 4.5f;
                }
            }
        }
    }
}

signed int zGameExtraIsValentines()
{
    if (sDay == 14 && sMonth == 2)
    {
        return 1;
    }
    return 0;
}

signed int zGameExtraIsShirazBday()
{
    if (sDay == 2 && sMonth == 2)
    {
        return 1;
    }
    return 0;
}

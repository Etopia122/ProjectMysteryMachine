#include "zGlobals.h"
#include "zScene.h"
#include "zParEmitter.h"
#include "zSurface.h"
#include "xMath.h"
#include "xMath3.h"
#include "xString.h"
#include "xstransvc.h"
#include "xAnim.h"
#include "iModel.h"
#include "iMath.h"
#include "xCollide.h"
#include "xSnd.h"

#ifndef NULL
#define NULL 0
#endif

class RwMatrixTag;
class RpClump;
class xAnimActiveEffect;
class xAnimEffect;
class _zEntHangable;
class _zEntPickup;
class _zEntButton;
class _zEntDestructObj;
class st_zEntVillain;

class RwSphere
{
public:
    float x, y, z;  // 0x0
    float radius;   // 0xC
};

class RpMorphTarget
{
public:
    void* parentGeom;          // 0x0
    RwSphere boundingSphere;   // 0x4
};

class RpGeometry
{
public:
    unsigned char pad0[0x68];
    RpMorphTarget* morphTarget; // 0x68
};

class RpAtomic
{
public:
    unsigned char pad0[0x18];
    RpGeometry* geometry;         // 0x18
    RwSphere boundingSphere;      // 0x1C
    RwSphere worldBoundingSphere; // 0x2C
    RpClump* clump;               // 0x3C
};

class _xEntCollis
{
public:
    unsigned char chk;      // 0x0
    unsigned char pen;      // 0x1
    unsigned char pad2[0xA];
    _xCollis colls[18];     // 0xC
};

class xAnimFile
{
public:
    xAnimFile* Next;          // 0x0
    char* Name;               // 0x4
    unsigned int ID;          // 0x8
    unsigned int FileFlags;   // 0xC
    float Duration;           // 0x10
    unsigned char pad14[0x3C - 0x14];
    void* RawData[1];         // 0x3C
};

class xAnimState
{
public:
    xAnimState* Next;       // 0x0
    char* Name;             // 0x4
    unsigned int ID;        // 0x8
    unsigned int Flags;     // 0xC
    unsigned int UserFlags; // 0x10
    float Speed;            // 0x14
    xAnimFile* Data;        // 0x18
};

class xAnimTransition
{
public:
    xAnimTransition* Next;  // 0x0
    xAnimState* Dest;       // 0x4
    void* Conditional;      // 0x8
    void* Callback;         // 0xC
    unsigned int Flags;     // 0x10
    unsigned int UserFlags; // 0x14
};

class xAnimSingle
{
public:
    unsigned int SingleFlags; // 0x0
    xAnimState* State;        // 0x4
    float Time;               // 0x8
    float CurrentSpeed;       // 0xC
    unsigned char pad10[0x58 - 0x10];
    xAnimSingle* Blend;       // 0x58
};

class xAnimTable
{
public:
    xAnimTable* Next;                // 0x0
    char* Name;                      // 0x4
    xAnimTransition* TransitionList; // 0x8
    xAnimState* StateList;           // 0xC
};

class xAnimPlay
{
public:
    xAnimPlay* Next;          // 0x0
    unsigned short NumSingle; // 0x4
    unsigned short BoneCount; // 0x6
    xAnimSingle* Single;      // 0x8
    void* Object;             // 0xC
    xAnimTable* Table;        // 0x10
    _xVec3 Translate[2];      // 0x14
};

class _tagSkidEdge;

class _tagFeetSkidMark
{
public:
    signed int total_feet;   // 0x0
    _tagSkidEdge* edge[4];   // 0x4
    signed int first_idx;    // 0x14
    signed int total_edges;  // 0x18
    float sample_timer;      // 0x1C
    float remove_timer;      // 0x20
};

class tagzFeet
{
public:
    signed int total;          // 0x0
    _xEnt* owner;              // 0x4
    signed short entity_type;  // 0x8
    signed short pitch;        // 0xA
};

class _tagPlayerFX
{
public:
    _zParEmitter* emit_land[23];  // 0x0
    _zParEmitter* emit_stop[23];  // 0x5C
    _zParEmitter* emit_scratch;   // 0xB8
    _zParEmitter* emit_digup[2];  // 0xBC
    float timer_stop;             // 0xC4
    float timer_scratch;          // 0xC8
    float timer_digup;            // 0xCC
};

class _tagPlayerSndFX
{
public:
    signed short ammo_pitch;           // 0x0
    signed short special_pitch;        // 0x2
    signed short token_pitch;          // 0x4
    signed short bite_pitch;           // 0x6
    signed short snackcounter_pitch;   // 0x8
    signed short save_pitch;           // 0xA
    signed short key_pitch;            // 0xC
    signed short info_pitch;           // 0xE
    signed short error_pitch;          // 0x10
    unsigned int ammo_id;              // 0x14
    unsigned int special_id;           // 0x18
    unsigned int token_id;             // 0x1C
    unsigned int bite_id;              // 0x20
    unsigned int snackcounter_id;      // 0x24
    unsigned int save_id;              // 0x28
    unsigned int key_id;               // 0x2C
    unsigned int info_id;              // 0x30
    unsigned int error_id;             // 0x34
    unsigned int van_id;               // 0x38
    signed short butt_pitch;           // 0x3C
    signed short head_pitch;           // 0x3E
    signed short butt_snd;             // 0x40
    signed short head_snd;             // 0x42
    unsigned int butt_id[3];           // 0x44
    unsigned int head_id[3];           // 0x50
    unsigned int buttmiss_id;          // 0x5C
    unsigned int skid_id[3];           // 0x60
    signed int vo_snd;                 // 0x6C
    unsigned int dig_id[2];            // 0x70
    unsigned int scare_id[2];          // 0x78
    unsigned int excite_id[13];        // 0x80
    unsigned int danger_id[9];         // 0xB4
    unsigned int leap_id[12];          // 0xD8
    unsigned int see_geronimo_id[1];   // 0x108
    unsigned int see_shaggy_id[5];     // 0x10C
    unsigned int see_holly_id[3];      // 0x120
    unsigned int see_monster_id[5];    // 0x12C
    unsigned int see_groundskeeper_id[1]; // 0x140
    unsigned int see_warpgate_id[1];   // 0x144
    unsigned int need_more_snacks_id[3]; // 0x148
    unsigned int success_id[1];        // 0x154
    unsigned int see_token_id[3];      // 0x158
    unsigned int see_crab_id[2];       // 0x164
    unsigned int see_rat_id[4];        // 0x16C
    unsigned int see_gargoyle_id[2];   // 0x17C
    unsigned int see_creeper_id[2];    // 0x184
    unsigned int see_ghost_diver_id[2]; // 0x18C
    unsigned int see_sea_creature_id[2]; // 0x194
    unsigned int see_headless_specter_id[2]; // 0x19C
    unsigned int see_scarecrow_id[3];  // 0x1A4
    unsigned int see_space_kook_id[2]; // 0x1B0
    unsigned int see_tar_monster_id[4]; // 0x1B8
    unsigned int see_witch_id[2];      // 0x1C8
    unsigned int see_robot_id[2];      // 0x1D0
    unsigned int see_wolfman_id[2];    // 0x1D8
    unsigned int see_ghost_id[4];      // 0x1E0
    unsigned int see_zombie_id[2];     // 0x1F0
    unsigned int sorry_id[4];          // 0x1F8
    unsigned int collect_all_snacks_id[1]; // 0x208
    unsigned int eat_sandwich_id[2];   // 0x20C
    unsigned int eat_turkey_id[2];     // 0x214
    unsigned int eat_cake_id[2];       // 0x21C
    unsigned int eat_snacks_id[3];     // 0x224
    unsigned char pad230[0x18];
};

class zPlayerSituation
{
public:
    unsigned int priority;  // 0x0
    unsigned int countMax;  // 0x4
    float delay;            // 0x8
    float punchDelay;       // 0xC
    float elapsedTime;      // 0x10
    unsigned int count;     // 0x14
    unsigned int type;      // 0x18
};

enum _LostChanceFX
{
    LCFX_None = 0,
    LCFX_StdShock = 1,
    LCFX_Water = 2,
    LCFX_Total = 3
};

enum _tagSDRumbleType
{
    SDR_None = 0
};

xSurface* zSurfaceGetSurface(_xCollis* coll);
void zRumbleStart(signed int idx, _tagSDRumbleType type);
signed short fn_800C751C(signed short old, float* pitch, signed int range, float scale);
void zLaughNotify(signed int laugh);
void zFeetAddSkidMark(_xEnt* ent, _tagFeetSkidMark* mark, float dt);
void zFeetUpdateSkidMark(_tagFeetSkidMark* mark, float dt);
unsigned int iSndIsPlayingByID(unsigned int id);
void RpClumpRemoveAtomic(RpClump* clump, RpAtomic* atomic);
void RpClumpAddAtomic(RpClump* clump, RpAtomic* atomic);

extern "C" {
signed int fn_800ED288();
}

signed int sPlayerSurfaceType;
static float sPlayerWaterOffset;
_xVec3 gPlayerWaterPos;
signed int gPlayerHitWater;
signed int gDeadScooby;
_xVec3 gDeadScoobyPos;
static _zParEmitter* sEmitterStars;
static _zParEmitter* sEmitterShockLeft;
static _zParEmitter* sEmitterShockRight;
static _zParEmitter* sEmitterWaterSplash;
static _zParEmitter* sEmitterBounceMiss;
static _zParEmitter* sEmitterSprings;
static _zParEmitter* sEmitterSpringBoard;
static _zParEmitter* sEmitterBounceOnVillain;
static _zParEmitter* sEmitterHeadButt;
static _zParEmitter* sEmitterHeadButt2;
static unsigned int sSoundWaterSplash;
_tagPlayerFX gPlayerfx;
tagzFeet sPlayerFeet[10];
_tagFeetSkidMark sPlayerSkidMark[5];
_tagFeetSkidMark* sPlayerCurSkidMark;
unsigned int sSnackDecrement;
_zEntPickup* sDecrementSnackGateEnt;
float sDecrementDelay;
float sDecrementAmount;
static _tagPlayerSndFX sPlayerSndfx;

static unsigned int cchkButtbounce;
static unsigned int cchkSquish;
static unsigned int sVO;
static unsigned int sVOAssetID;
static unsigned int sVOPriority;
extern zPlayerSituation sPlayerInfo[37];
static zPlayerSituation* sVONextSituation;
static float sVOTimer;
static RpAtomic* sPlayerBodyAtomic;
static RpAtomic* sPlayerHeadAtomic;
static RpClump* sPlayerBodyClump;
static RpClump* sPlayerHeadClump;

#define SND_INVALID 0xFFFEFFFF

void zEntPlayer_InitCutsceneModelHack()
{
    sPlayerBodyAtomic = (RpAtomic*)xSTFindAsset(0xF2D545DF, NULL);
    sPlayerHeadAtomic = (RpAtomic*)xSTFindAsset(0xB2E286A1, NULL);
    sPlayerBodyClump = sPlayerBodyAtomic->clump;
    sPlayerHeadClump = sPlayerHeadAtomic->clump;
    RpClumpRemoveAtomic(sPlayerHeadClump, sPlayerHeadAtomic);
    RpClumpAddAtomic(sPlayerBodyClump, sPlayerHeadAtomic);
    sPlayerBodyAtomic->worldBoundingSphere.radius = 1.9f;
    sPlayerBodyAtomic->boundingSphere.radius = 1.9f;
    sPlayerBodyAtomic->geometry->morphTarget->boundingSphere.radius = 1.9f;
    sPlayerHeadAtomic->worldBoundingSphere.radius = 1.9f;
    sPlayerHeadAtomic->boundingSphere.radius = 1.9f;
    sPlayerHeadAtomic->geometry->morphTarget->boundingSphere.radius = 1.9f;
}

static unsigned int zEntPlayer_SNDSkidCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*)
{
    static signed int snd = 0.0f;
    static signed short pitch_angle = 0.0f;
    float pitch;
    signed int newsnd;

    pitch_angle = fn_800C751C(pitch_angle, &pitch, 0x1E, 1.5f);
    xSndPlay(sPlayerSndfx.skid_id[snd], 0.5f, 0.5f, pitch, 0x80, 2, 0);
    newsnd = snd;
    while (newsnd == snd)
    {
        newsnd = fn_800ED288() % 3;
    }
    snd = newsnd;

    signed int type = 0;
    xSurface* xsurf = zSurfaceGetSurface(&globals.player.ent.collis->colls[0]);
    if (xsurf != NULL && xsurf->state == 0)
    {
        zSurfaceProps* surface = (zSurfaceProps*)xsurf->moprops;
        if (surface != NULL)
        {
            zSurfAsset* surface_asset = surface->asset;
            if (surface_asset != NULL)
            {
                type = surface_asset->surf_type;
            }
        }
    }

    if (type == 0x16)
    {
        gPlayerfx.timer_stop = 0.0f;
    }
    else
    {
        gPlayerfx.timer_stop = 0.5f;
    }
    zRumbleStart(0, (_tagSDRumbleType)6);
    return 0;
}

static unsigned int zEntPlayer_FXOpenBrollyCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*)
{
    return 0;
}

static unsigned int zEntPlayer_FXSpringCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*)
{
    _zParEmitterCustomSettings info;
    _xEnt* ent;

    if (sEmitterSprings != NULL)
    {
        info.custom_flags = 0x20;
        ent = &globals.player.ent;
        iModelTagEval(ent->model->Data, &globals.player.HangPawTag[2], ent->model->Mat, &info.pos);
        info.pos.y += 0.5f;
        zParEmitterEmitCustom(sEmitterSprings, 1.0f / 30.0f, &info);
        iModelTagEval(ent->model->Data, &globals.player.HangPawTag[3], ent->model->Mat, &info.pos);
        info.pos.y += 0.5f;
        zParEmitterEmitCustom(sEmitterSprings, 1.0f / 30.0f, &info);
    }
    return 0;
}

static unsigned int zEntPlayer_FXDigupCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*)
{
    gPlayerfx.timer_digup = 0.3f;
    zRumbleStart(0, (_tagSDRumbleType)0);
    return 0;
}

static unsigned int zEntPlayer_FXScratchCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*)
{
    gPlayerfx.timer_scratch = 1.65f;
    zRumbleStart(0, (_tagSDRumbleType)5);
    return 0;
}

static void zEntPlayer_FXUpdate(_xEnt* ent, float seconds)
{
    signed int i;
    zParEmitterAsset* a;
    _xVec3 paw;

    sPlayerSurfaceType = 0;
    xSurface* xsurf = zSurfaceGetSurface(&ent->collis->colls[0]);
    if (xsurf != NULL && xsurf->state == 0)
    {
        zSurfaceProps* surface = (zSurfaceProps*)xsurf->moprops;
        if (surface != NULL)
        {
            zSurfAsset* surface_asset = surface->asset;
            if (surface_asset != NULL)
            {
                sPlayerSurfaceType = surface_asset->surf_type;
            }
        }
    }

    for (i = 0; i < 23; i++)
    {
        if (gPlayerfx.emit_land[i] != NULL)
        {
            gPlayerfx.emit_land[i]->emit_flags &= 0xFE;
        }
        if (gPlayerfx.emit_stop[i] != NULL)
        {
            gPlayerfx.emit_stop[i]->emit_flags &= 0xFE;
        }
    }
    gPlayerfx.emit_scratch->emit_flags &= 0xFE;
    gPlayerfx.emit_digup[0]->emit_flags &= 0xFE;
    gPlayerfx.emit_digup[1]->emit_flags &= 0xFE;

    if (gPlayerfx.emit_stop[sPlayerSurfaceType] != NULL && gPlayerfx.timer_stop > 0.0f)
    {
        if (xEntIsVisible(ent))
        {
            if (sPlayerCurSkidMark == NULL)
            {
                signed int least_edges = 16;
                for (i = 0; i < 5; i++)
                {
                    if (least_edges > sPlayerSkidMark[i].total_edges - sPlayerSkidMark[i].first_idx)
                    {
                        sPlayerCurSkidMark = &sPlayerSkidMark[i];
                        least_edges = sPlayerSkidMark[i].total_edges - sPlayerSkidMark[i].first_idx;
                    }
                }
                if (sPlayerCurSkidMark == NULL)
                {
                    sPlayerCurSkidMark = &sPlayerSkidMark[0];
                }
                sPlayerCurSkidMark->total_edges = 0;
                sPlayerCurSkidMark->first_idx = 0;
                sPlayerCurSkidMark->sample_timer = 0.0f;
                sPlayerCurSkidMark->remove_timer = 0.0f;
            }
            zFeetAddSkidMark(ent, sPlayerCurSkidMark, seconds);
            zLaughNotify(6);
            gPlayerfx.emit_stop[sPlayerSurfaceType]->emit_flags |= 1;
            a = gPlayerfx.emit_stop[sPlayerSurfaceType]->tasset;
            a->pos = *(_xVec3*)&ent->frame->mat.pos;
            a->pos.y += 0.1f;
            gPlayerfx.timer_stop -= seconds;
        }
        else
        {
            gPlayerfx.timer_stop = 0.0f;
        }
    }
    else
    {
        sPlayerCurSkidMark = NULL;
    }

    for (i = 0; i < 5; i++)
    {
        zFeetUpdateSkidMark(&sPlayerSkidMark[i], seconds);
    }

    xAnimState* state = globals.player.ent.model->Anim->Single->State;
    if (state != NULL && state->ID != xStrHash("Inactive_SitFlea"))
    {
        gPlayerfx.timer_scratch = 0.0f;
    }

    if (gPlayerfx.timer_scratch > 0.0f && xEntIsVisible(ent))
    {
        gPlayerfx.emit_scratch->emit_flags |= 1;
        a = gPlayerfx.emit_scratch->tasset;
        iModelTagEval(ent->model->Data, &globals.player.HangPawTag[2], ent->model->Mat, &paw);
        a->pos = paw;
        gPlayerfx.timer_scratch -= seconds;
    }

    if (gPlayerfx.timer_digup > 0.0f && xEntIsVisible(ent))
    {
        zLaughNotify(5);
        gPlayerfx.emit_digup[0]->emit_flags |= 1;
        a = gPlayerfx.emit_digup[0]->tasset;
        a->pos = *(_xVec3*)&((_xMat4x3*)ent->model->Mat)->pos;
        gPlayerfx.emit_digup[1]->emit_flags |= 1;
        a = gPlayerfx.emit_digup[1]->tasset;
        a->pos = *(_xVec3*)&((_xMat4x3*)ent->model->Mat)->pos;
        gPlayerfx.timer_digup -= seconds;
    }
}

static void zEntPlayer_FXLostChance(_xEnt* ent)
{
    _LostChanceFX fx_to_play = LCFX_StdShock;
    signed int sd_surface = -1;

    xSurface* fsurf = zSurfaceGetSurface(&ent->collis->colls[0]);
    if (fsurf != NULL && fsurf->state == 0)
    {
        zSurfaceProps* prop = (zSurfaceProps*)fsurf->moprops;
        sd_surface = prop->asset->surf_type;
        if (prop != NULL && prop->asset != NULL)
        {
            switch (prop->asset->game_damage_type)
            {
            case 3:
                fx_to_play = LCFX_Water;
                break;
            default:
                fx_to_play = LCFX_StdShock;
                break;
            }
        }
    }

    if (fx_to_play == LCFX_StdShock)
    {
        if (!gPlayerHitWater)
        {
            _xVec3* center = xBoundCenter(&ent->bound);
            float dx = 0.5f * globals.camera.hi_cam.mat.right.x;
            float dz = 0.5f * globals.camera.hi_cam.mat.right.z;
            if (sEmitterShockLeft != NULL)
            {
                _zParEmitterCustomSettings info;
                info.custom_flags = 0x20;
                info.pos = *center;
                info.pos.x += dx;
                info.pos.y += 1.0f;
                info.pos.z += dz;
                zParEmitterEmitCustom(sEmitterShockLeft, 1.0f / 30.0f, &info);
            }
            if (sEmitterShockRight != NULL)
            {
                _zParEmitterCustomSettings info;
                info.custom_flags = 0x20;
                info.pos = *center;
                info.pos.x -= dx;
                info.pos.y += 1.0f;
                info.pos.z -= dz;
                zParEmitterEmitCustom(sEmitterShockRight, 1.0f / 30.0f, &info);
            }
        }
    }
    else if (fx_to_play == LCFX_Water)
    {
        _xVec3* center = xBoundCenter(&ent->bound);
        _zParEmitterCustomSettings info;
        info.custom_flags = 0x20;
        info.pos = *center;
        info.pos.y += 0.1f;
        zParEmitterEmitCustom(sEmitterWaterSplash, 1.0f / 30.0f, &info);
        xSndPlay3D(sSoundWaterSplash, 0.5f, 0.0f, 0, 2, &info.pos, 100.0f);
        if (globals.player.Inv_Chances)
        {
            gPlayerWaterPos = *center;
            sPlayerWaterOffset = 0.0f;
            gPlayerHitWater = 1;
        }
    }

    if (sd_surface == 0x15 && globals.player.Inv_Chances)
    {
        gPlayerWaterPos = *xBoundCenter(&ent->bound);
        sPlayerWaterOffset = 0.0f;
        gPlayerHitWater = 1;
    }
}

#pragma auto_inline on
void zEntPlayer_Notify(signed int situation)
{
    if (globals.player.ControlOn)
    {
        zPlayerSituation* s = &sPlayerInfo[situation];
        if ((sVONextSituation == NULL || sVONextSituation->priority < s->priority) &&
            (!iSndIsPlayingByID(sVOAssetID) || s->priority > sVOPriority) &&
            (s->countMax == 0 || s->count < s->countMax) && !(s->delay > s->elapsedTime))
        {
            sVONextSituation = s;
            sVOTimer = s->punchDelay;
        }
    }
}
#pragma auto_inline reset

void zEntPlayer_Notify(unsigned char characterSubType)
{
    if (globals.player.ControlOn)
    {
        switch (characterSubType)
        {
        case 2:
            zEntPlayer_Notify(12);
            break;
        case 21:
            zEntPlayer_Notify(16);
            break;
        case 22:
            zEntPlayer_Notify(17);
            break;
        case 5:
            zEntPlayer_Notify(18);
            break;
        case 3:
            zEntPlayer_Notify(19);
            break;
        case 7:
            zEntPlayer_Notify(20);
            break;
        case 4:
            zEntPlayer_Notify(21);
            break;
        case 9:
            zEntPlayer_Notify(22);
            break;
        case 13:
            zEntPlayer_Notify(23);
            break;
        case 15:
            zEntPlayer_Notify(24);
            break;
        case 16:
            zEntPlayer_Notify(25);
            break;
        case 17:
            zEntPlayer_Notify(26);
            break;
        case 12:
            zEntPlayer_Notify(27);
            break;
        case 19:
            zEntPlayer_Notify(28);
            break;
        case 6:
            zEntPlayer_Notify(29);
            break;
        case 20:
            zEntPlayer_Notify(30);
            break;
        case 47:
            zEntPlayer_Notify(11);
            break;
        case 48:
            zEntPlayer_Notify(13);
            break;
        case 28:
        case 29:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
            zEntPlayer_Notify(2);
            break;
        default:
            zEntPlayer_Notify(10);
            break;
        }
    }
}

static inline void zEntPlayer_SNDPlayRandom(unsigned int* array, unsigned int max)
{
    unsigned int i = 0;
    unsigned int idx = irand() % max;
    sVOAssetID = array[idx];
    while (true)
    {
        sVO = xSndPlay(sVOAssetID, 1.0f, 1.0f, 0.0f, 0x80, 0x202, 0);
        if (sVO != 0 && sVO != SND_INVALID)
        {
            break;
        }
        idx++;
        if (idx >= max)
        {
            idx = 0;
        }
        i++;
        sVOAssetID = array[idx];
        if (i >= max)
        {
            break;
        }
    }
}

void zEntPlayer_SituationDo()
{
    if (sVONextSituation != NULL && globals.player.Health)
    {
        switch (sVONextSituation->type)
        {
        case 33:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.eat_cake_id, 2);
            break;
        case 34:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.eat_sandwich_id, 2);
            break;
        case 35:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.eat_turkey_id, 2);
            break;
        case 36:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.eat_snacks_id, 3);
            break;
        case 31:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.sorry_id, 4);
            break;
        case 3:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_token_id, 3);
            break;
        case 5:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.success_id, 1);
            break;
        case 15:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.need_more_snacks_id, 3);
            break;
        case 14:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_warpgate_id, 1);
            break;
        case 9:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.leap_id, 12);
            break;
        case 0:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.dig_id, 2);
            break;
        case 8:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.danger_id, 9);
            break;
        case 1:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.scare_id, 2);
            break;
        case 7:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.excite_id, 13);
            break;
        case 12:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_geronimo_id, 1);
            break;
        case 16:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_crab_id, 2);
            break;
        case 17:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_rat_id, 4);
            break;
        case 18:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_gargoyle_id, 2);
            break;
        case 19:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_creeper_id, 2);
            break;
        case 20:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_ghost_diver_id, 2);
            break;
        case 21:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_sea_creature_id, 2);
            break;
        case 22:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_headless_specter_id, 2);
            break;
        case 23:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_scarecrow_id, 3);
            break;
        case 24:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_space_kook_id, 2);
            break;
        case 25:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_tar_monster_id, 4);
            break;
        case 26:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_witch_id, 2);
            break;
        case 27:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_robot_id, 2);
            break;
        case 28:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_wolfman_id, 2);
            break;
        case 29:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_ghost_id, 4);
            break;
        case 30:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_zombie_id, 2);
            break;
        case 2:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_shaggy_id, 5);
            break;
        case 10:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_monster_id, 5);
            break;
        case 11:
            if (globals.sceneCur->sceneID != 'S005')
            {
                zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_holly_id, 3);
            }
            break;
        case 13:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.see_groundskeeper_id, 1);
            break;
        case 32:
            zEntPlayer_SNDPlayRandom(sPlayerSndfx.collect_all_snacks_id, 1);
            break;
        }
        sVONextSituation->elapsedTime = 0.0f;
        sVONextSituation->count++;
        sVONextSituation = NULL;
    }
}

void zEntPlayer_SNDVan(_xEnt*)
{
    xSndPlay(sPlayerSndfx.van_id, 0.5f, 0.5f, 0.0f, 0x80, 2, 0);
}

void zEntPlayer_SNDSnackCounter(_xEnt*)
{
    xSndPlay(sPlayerSndfx.snackcounter_id, 0.5f, 0.5f, 0.0f, 0x80, 2, 0);
}

void zEntPlayer_SNDKeyPickup(_xEnt*)
{
    float pitch;
    sPlayerSndfx.key_pitch = fn_800C751C(sPlayerSndfx.key_pitch, &pitch, 0x1E, 0.25f);
    xSndPlay(sPlayerSndfx.key_id, 0.5f, 0.5f, pitch, 0x80, 2, 0);
}

void zEntPlayer_SNDErrorPickup(_xEnt*)
{
    float pitch;
    sPlayerSndfx.error_pitch = fn_800C751C(sPlayerSndfx.error_pitch, &pitch, 0x1E, 0.25f);
    xSndPlay(sPlayerSndfx.error_id, 0.5f, 0.5f, pitch, 0x80, 2, 0);
}

void zEntPlayer_SNDInfoPickup(_xEnt*)
{
    float pitch;
    sPlayerSndfx.info_pitch = fn_800C751C(sPlayerSndfx.info_pitch, &pitch, 0x1E, 0.25f);
    xSndPlay(sPlayerSndfx.info_id, 0.5f, 0.5f, pitch, 0x80, 2, 0);
}

void zEntPlayer_SNDSavePickup(_xEnt*)
{
    float pitch;
    sPlayerSndfx.save_pitch = fn_800C751C(sPlayerSndfx.save_pitch, &pitch, 0x1E, 0.25f);
    xSndPlay(sPlayerSndfx.save_id, 0.5f, 0.5f, pitch, 0x80, 2, 0);
}

void zEntPlayer_SNDTokenPickup(_xEnt*)
{
    float pitch;
    sPlayerSndfx.token_pitch = fn_800C751C(sPlayerSndfx.token_pitch, &pitch, 0x1E, 0.25f);
    xSndPlay(sPlayerSndfx.token_id, 0.5f, 0.5f, pitch, 0x80, 2, 0);
}

void zEntPlayer_SNDSpecialPickup(_xEnt*)
{
    float pitch;
    sPlayerSndfx.special_pitch = fn_800C751C(sPlayerSndfx.special_pitch, &pitch, 0x1E, 0.25f);
    xSndPlay(sPlayerSndfx.special_id, 0.5f, 0.5f, pitch, 0x80, 2, 0);
}

void zEntPlayer_SNDAmmoPickup(_xEnt*)
{
    float pitch;
    sPlayerSndfx.ammo_pitch = fn_800C751C(sPlayerSndfx.ammo_pitch, &pitch, 0x1E, 0.25f);
    xSndPlay(sPlayerSndfx.ammo_id, 0.5f, 0.5f, pitch, 0x80, 2, 0);
}

void zEntPlayer_SNDBitePickup(_xEnt*)
{
    float pitch;
    sPlayerSndfx.bite_pitch = fn_800C751C(sPlayerSndfx.bite_pitch, &pitch, 0x1E, 0.25f);
    xSndPlay(sPlayerSndfx.bite_id, 0.5f, 0.5f, pitch, 0x80, 2, 0);
}

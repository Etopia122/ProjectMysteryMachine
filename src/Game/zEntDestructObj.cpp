#include "zEnt.h"
#include "zGlobals.h"
#include "xserializer.h"
#include "rwtypes.h"

enum _tagSDRumbleType
{
    SDR_Digging = 0,
    SDR_Headbutt = 1,
    SDR_ButtbounceJump = 2,
    SDR_Buttbounce = 3,
    SDR_VillainCollideScare = 4,
    SDR_FleaScratch = 5,
    SDR_ScreechingStop = 6,
    SDR_GotScared = 7,
    SDR_WebDestroyed = 8,
    SDR_XploDestroyedObj = 9,
    SDR_DustDestroyedObj = 10,
    SDR_EventLight = 11,
    SDR_EventMedium = 12,
    SDR_EventHeavy = 13,
    SDR_Test = 14,
    SDR_FireworkBang = 15,
    SDR_Total = 16,
};

void zRumbleStart(signed int pad_id, _tagSDRumbleType rumble_type);

class _zParEmitter;

class _zParEmitterCustomSettings
{
public:
    unsigned int custom_flags;
    unsigned char count;
    unsigned char count_variation;
    float interval;
    unsigned int attachToID;
    _xVec3 pos;
    _xVec3 vel;
    float vel_angle_variation;
    unsigned char color_birth[4];
    unsigned char color_death[4];
    float size_birth;
    float size_birth_variation;
    float size_death;
    float life;
    float life_variation;
    unsigned char rot[3];
    float xzcircle_radius;
    float emit_interval_current;
};

_zParEmitter* zParEmitterFind(char* asset_name);
void zParEmitterEmitCustom(_zParEmitter* p, float dt, _zParEmitterCustomSettings* info);

class zEntDestructObjAsset
{
public:
    float animSpeed;
    unsigned int initAnimState;
    unsigned int health;
    unsigned int spawnItemID;
    unsigned int dflags;
    unsigned char collType;
    unsigned char fxType;
    unsigned char pad[2];
    float blast_radius;
    float blast_strength;
};

class _zEntDestructObj : public _zEnt
{
public:
    zEntDestructObjAsset* dasset; // 0xEC
    unsigned int state;           // 0xF0
    unsigned int healthCnt;       // 0xF4
    float fx_timer;               // 0xF8
    _zParEmitter* fx_emitter;     // 0xFC
};

_zParEmitter* sEmitDust;
_zParEmitter* sEmitXplo;
_zParEmitter* sEmitWeb;

signed int zEntDestructObjEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                                  xBase* toParamWidget);
void zEntDestructObj_Update(_zEntDestructObj* ent, _xScene* sc, float dt);
void zEntDextructObj_Move(_zEntDestructObj* ent, _xScene* sc, float dt, _xEntFrame* frame);

static inline float xVec3Dist2(const _xVec3* a, const _xVec3* b)
{
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float dz = a->z - b->z;
    return dx * dx + dy * dy + dz * dz;
}

static inline void zEntDestructObj_DestroyFX(_zEntDestructObj* o)
{
    _tagSDRumbleType rt = SDR_Total;

    o->chkby &= 0xE7;
    o->flags &= ~1;
    o->fx_timer = 0.33f;
    switch (o->dasset->fxType)
    {
    case 0:
        o->fx_emitter = 0;
        break;
    case 1:
        o->fx_emitter = sEmitDust;
        rt = SDR_DustDestroyedObj;
        break;
    case 2:
        o->fx_emitter = sEmitXplo;
        rt = SDR_XploDestroyedObj;
        break;
    case 3:
        o->fx_emitter = sEmitWeb;
        rt = SDR_WebDestroyed;
        break;
    }
    if (rt != SDR_Total)
    {
        if (xVec3Dist2((_xVec3*)&o->model->Mat->pos,
                       (_xVec3*)&globals.player.ent.model->Mat->pos) <= 25.0f)
        {
            zRumbleStart(0, rt);
        }
    }
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zEntDestructObj_FindFX()
{
    sEmitDust = zParEmitterFind("PAREMIT_DOBJ_DUST");
    sEmitXplo = zParEmitterFind("PAREMIT_DOBJ_XPLO");
    sEmitWeb = zParEmitterFind("PAREMIT_DOBJ_WEB");
}

void zEntDestructObj_Init(_zEntDestructObj* ent, p2EntAsset* asset)
{
    zEntDestructObjAsset* dasset;

    zEntInit(ent, asset, 'DSTR');
    dasset = (zEntDestructObjAsset*)(asset + 1);
    ent->dasset = dasset;
    ent->healthCnt = dasset->health;
    ent->penby |= 0x10;
    ent->state = 0;
    if (ent->dasset->collType & 2)
    {
        ent->chkby |= 0x18;
    }
    else
    {
        ent->chkby = 0;
    }
    ent->move = (void (*)(_xEnt*, _xScene*, float, _xEntFrame*))zEntDextructObj_Move;
    ent->eventFunc = zEntDestructObjEventCB;
    ent->update = (void (*)(_xEnt*, _xScene*, float))zEntDestructObj_Update;
    if (ent->linkCount)
    {
        ent->link = (p2LinkAsset*)((zEntDestructObjAsset*)(ent->asset + 1) + 1);
    }
    else
    {
        ent->link = 0;
    }
    ent->eventFunc = zEntDestructObjEventCB;
    xEntReset(ent);
}

void zEntDextructObj_Move(_zEntDestructObj* ent, _xScene* sc, float dt, _xEntFrame* frame)
{
}

void zEntDestructObj_Update(_zEntDestructObj* ent, _xScene* sc, float dt)
{
    _zParEmitterCustomSettings info;

    xEntUpdate(ent, sc, dt);
    if (ent->healthCnt > 1)
    {
        ent->state = 0;
    }
    else
    {
        ent->state = 1;
    }
    if (ent->fx_emitter && ent->fx_timer > 0.0f)
    {
        ent->fx_timer -= dt;
        info.custom_flags = 0x20;
        info.pos = *xBoundCenter(&ent->bound);
        zParEmitterEmitCustom(ent->fx_emitter, dt, &info);
    }
}

unsigned int zEntDestructObj_ButtBounce(_zEntDestructObj* ent)
{
    return (ent->dasset->dflags & 2) ? 3 : 0;
}

unsigned int zEntDestructObj_Headbutt(_zEntDestructObj* ent)
{
    if (ent->dasset->dflags & 1)
    {
        return (ent->dasset->dflags & 8) ? 5 : 4;
    }
    return 0;
}

void zEntDestructObj_Save(_zEntDestructObj* ent, xSerial* s)
{
    zEntSave(ent, s);
    s->Write(ent->state);
}

void zEntDestructObj_Load(_zEntDestructObj* ent, xSerial* s)
{
    zEntLoad(ent, s);
    s->Read(&ent->state);
}

unsigned int zEntDestructObj_CheckState(_zEntDestructObj* ent)
{
    return ent->state;
}

signed int zEntDestructObjEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                                  xBase* toParamWidget)
{
    _zEntDestructObj* s = (_zEntDestructObj*)to;

    switch (toEvent)
    {
    case 0x3:
        s->flags |= 1;
        break;
    case 0x4:
        s->flags &= ~1;
        break;
    case 0x53:
        s->chkby |= 0x18;
        break;
    case 0x54:
        s->chkby &= 0xE7;
        break;
    case 0x55:
        s->chkby |= 0x18;
        s->flags |= 1;
        break;
    case 0x56:
        s->chkby &= 0xE7;
        s->flags &= ~1;
        break;
    case 0x16:
        zEntDestructObj_DestroyFX(s);
        break;
    case 0xA:
        xEntReset(s);
        s->healthCnt = s->dasset->health;
        s->pflags = 0;
        s->penby |= 0x10;
        s->flags |= 1;
        if (s->dasset->collType & 2)
        {
            s->chkby |= 0x18;
        }
        else
        {
            s->chkby = 0;
        }
        s->fx_timer = 0.0f;
        s->fx_emitter = 0;
        break;
    case 0xC3:
    case 0xC4:
    case 0xC5:
    case 0xC6:
    case 0xC7:
    case 0xC8:
    case 0xC9:
    case 0xCA:
        zEntAnimEvent(s, toEvent, toParam);
        break;
    case 0xFC:
        s->moreFlags |= 1;
        break;
    case 0xFD:
        s->moreFlags &= ~1;
        s->flags &= 0xFB;
        s->fade_tmr = 0.0f;
        break;
    case 0xFE:
        s->moreFlags ^= 1;
        if (!(s->moreFlags & 1))
        {
            s->flags &= 0xFB;
            s->fade_tmr = 0.0f;
        }
        break;
    case 0x3A:
        s->healthCnt--;
        if (s->healthCnt == 0)
        {
            zEntEvent(s, s, 0x16);
        }
        break;
    }
    return 1;
}

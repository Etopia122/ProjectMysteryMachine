// TODO: 13 of 25 functions match (98.8%). Most of what is left is register
// allocation (Update's idx/type registers, Give's specials bit, the two
// UpdateFlyToInterface loops) and scheduling (Drop's xsqrt, lastSnackInArea's
// epilogue, UnlockSnackGate's prologue). objdiff still scores .bss at 50%;
// the DOL itself is identical.
#include "zEnt.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zParEmitter.h"
#include "zCounter.h"
#include "xMath.h"
#include "rwtypes.h"

#ifndef NULL
#define NULL 0
#endif

class xSerial;
class xPar;
class xParGroup;
class RpAtomic;
class _xQuat;
class _tagxCamFrame;
class _zUI;
class xEntPickupAsset;

class zAssetPickup
{
public:
    unsigned int pickupHash;    // 0x0
    unsigned char pickupType;   // 0x4
    unsigned char pickupIndex;  // 0x5
    unsigned short pickupFlags; // 0x6
    unsigned int quantity;      // 0x8
    unsigned int modelID;       // 0xC
    unsigned int animID;        // 0x10
};

class zAssetPickupTable
{
public:
    unsigned int Magic; // 0x0
    unsigned int Count; // 0x4
    zAssetPickup entries[1]; // 0x8
};

class zEntPickupAsset : public p2EntAsset
{
public:
    unsigned int pickupHash;    // 0x50
    unsigned short pickupFlags; // 0x54
    unsigned short pickupValue; // 0x56
};

class _zEntPickup : public _zEnt
{
public:
    unsigned int state;              // 0xEC
    zAssetPickup* p;                 // 0xF0
    void* anim;                      // 0xF4
    float animTime;                  // 0xF8
    float timer;                     // 0xFC
    float shake_timer;               // 0x100
    _xVec3 shake_pos;                // 0x104
    signed int snackGateInfoDisplayed; // 0x110
    float fx_timer;                  // 0x114
    xPar* fx_par;                    // 0x118
    _zParEmitter* fx_emit;           // 0x11C
    float fx_scale;                  // 0x120
    _xVec3 grab_pos;                 // 0x124
    unsigned int flyflags;           // 0x130
    signed int flg_opts;             // 0x134
    _xVec3 vel;                      // 0x138
    _xVec3 droppos;                  // 0x144
    xBase* followTarget;             // 0x150
    _xVec3 followOffset;             // 0x154
    xBase* dropParent;               // 0x160
    _xEnt* useThisEntPos;            // 0x164
};

class _tagKeyShake
{
public:
    _xVec3 orig_pos;   // 0x0
    float shake_timer; // 0xC
    float shake_it;    // 0x10
    _zUI* ui;          // 0x14
};

class zEntPickupWarpPoint
{
public:
    signed int id;        // 0x0
    unsigned int status;  // 0x4
    unsigned int sceneID; // 0x8
};

class xParLike
{
public:
    unsigned char pad0[0x10];
    _xVec3 pos; // 0x10
};

class _zUILike
{
public:
    unsigned char pad0[0xEC];
    class _zUIAsset
    {
    public:
        unsigned char pad0[0x1C];
        float x; // 0x1C
        float y; // 0x20
        float z; // 0x24
    }* uiasset; // 0xEC
};

class xParEmitterLike
{
public:
    unsigned char pad0[0x48];
    class
    {
    public:
        unsigned char pad0[0x20];
        xParGroup* group; // 0x20
    }* parSys; // 0x48
};

class _xEntCollis
{
public:
    unsigned char chk;      // 0x0
    unsigned char pen;      // 0x1
    unsigned char pad2[0xA];
    unsigned int colls0flags; // 0xC
    unsigned char pad10[0x4FC - 0x10];
};

void zEntPlayer_Notify(signed int situation);
void zEntPlayer_SNDSnackCounter(_xEnt* ent);

extern "C" {
signed int sprintf(char* s, const char* format, ...);
RwMatrixTag* RwMatrixUpdate(RwMatrixTag* matrix);
// TODO: unidentified RenderWare matrix scale
RwMatrixTag* fn_80162B60(RwMatrixTag* matrix, const RwV3d* scale, signed int combineOp);
// TODO: unidentified functions
void fn_80015EAC(const char* fmt, ...);
float fn_800294A0(float t);
}

void zHud_showSnack();

unsigned int xStrHash(const char* str);
unsigned int xSndPlay(unsigned int id, float volL, float volR, float pitch, unsigned int priority,
                      unsigned int flags, unsigned int parentID);
unsigned int xSndPlay3D(unsigned int id, float vol, float pitch, unsigned int priority,
                        unsigned int flags, _xEnt* parent, float radius);
_xVec3* xBoundCenter(_xBound* b);
unsigned int xEntIsVisible(_xEnt* ent);
void xEntCollide(_xEnt* ent, _xScene* sc, float dt);
float xMat3x3LookVec(_xMat3x3* m, _xVec3* at);
void xMat3x3RMulRotY(_xMat3x3* o, _xMat3x3* m, float t);
void xMat4x3MoveLocalRight(_xMat4x3* m, float mag);
void xMat4x3MoveLocalUp(_xMat4x3* m, float mag);
void xMat4x3MoveLocalAt(_xMat4x3* m, float mag);
_xMat4x3* xCameraGetCurMatrix(_tagxCamFrame* cam);
signed int irand();
float iurand();
void xParGroupKillPar(xParGroup* ps, xPar* p);
void xParGroupSetStealing(xParGroup* group, signed int stealing);
float iAnimDuration(void* raw);
void iAnimEval(void* raw, float time, unsigned int flags, _xVec3* tran, _xQuat* quat);
unsigned int iAnimBoneCount(void* raw);
void iModelAnimMatrices(RpAtomic* model, _xQuat* quat, _xVec3* tran, RwMatrixTag* mat);
signed int iModelCull(RpAtomic* model, RwMatrixTag* mat);
unsigned int iModelNumBones(RpAtomic* model);
void iShadowCollide(iShadowCache* cache, _xEnt* ent, float a, float b);
void iShadowAdd(iShadowCache* cache, unsigned char alpha);
xModelInstance* xModelInstanceAlloc(RpAtomic* data, void* object, unsigned short flags,
                                    unsigned char boneIndex, unsigned char* boneRemap);
void* xSTFindAsset(unsigned int aid, unsigned int* size);
xBase* zSceneFindObject(unsigned int gameID);
signed int zSceneGetAreaConstTotalSnacks(unsigned int sceneID);
signed int zSceneGetAreaSnacksCollected(unsigned int sceneID);
void zSceneGameStatsTally_clue();
void zSceneGameStatsTally_token();
void zVarSetSnackGateValue(unsigned int amount);
void zGameStateSwitchEvent(signed int event);
void zMusicNotify(signed int event);
void zHud_ShakeSnackCounter();
void zHud_showHealthandStatus();
signed int zGameExtraIsValentines();
signed int zGameExtraIsShirazBday();
signed int zGameExtraIsAnnaBday();
void zEntPlayer_GiveKey(unsigned int num);
void zEntPlayer_GiveClue(unsigned int num);
void zEntPlayer_GiveHealth(unsigned int num);
void zEntPlayer_GiveChance(unsigned int num);
void zEntPlayer_GiveHotsauce(unsigned int num);
void zEntPlayer_GiveScoobySnack(unsigned int num);
signed int zEntPlayer_HasScoobySnack(unsigned int num);
void zEntPlayer_TakeScoobySnack(_xEnt* ent, unsigned int num);
void zEntPlayer_SNDAmmoPickup(_xEnt* ent);
void zEntPlayer_SNDSpecialPickup(_xEnt* ent);
void zEntPlayer_SNDTokenPickup(_xEnt* ent);
void zEntPlayer_SNDSavePickup(_xEnt* ent);
void zEntPlayer_SNDInfoPickup(_xEnt* ent);
void zEntPlayer_SNDKeyPickup(_xEnt* ent);
void zEntPlayer_SNDErrorPickup(_xEnt* ent);
void zEntPlayer_SNDBitePickup(_xEnt* ent);
void Write_b7(xSerial* s, unsigned int bits);
void Read_b7(xSerial* s, unsigned int* bits);

extern unsigned char* giAnimScratch;
extern _zParEmitter* sEmitDust;
// TODO: unidentified zEntPlayer flag (blocks the snack gate prompt)
extern unsigned int lbl_8025ECFC;

void zEntPickup_Update(_zEntPickup* ent, _xScene* sc, float dt);
signed int zEntPickupEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                             xBase* toParamWidget);
void zEntPickup_GiveFX(_zEntPickup* ent);
void zEntPickup_UpdateFX(_zEntPickup* ent, _xScene* sc, float dt);
void zEntPickup_FlyToInterface(_zEntPickup* ent, float dt);
signed int zPickup_Give(_zEntPickup* ent, zAssetPickup* p, unsigned short value);
void zEntPickup_Reset(_zEntPickup* ent);
void zEntPickup_Drop(_zEntPickup* ent);

static float sKeysOffTimer = -10.1f;
signed int zHudShowWarpGateInfo = -1;

static signed int sKeyCounterEnabledBeforeGive;
static _zParEmitter* sEmitGlow;
static _zParEmitter* sEmitMmm;
static _zParEmitter* sEmitQMark;
static _zParEmitter* sEmitBubble;
static _zParEmitter* sEmitGum;
static _zParEmitter* sEmitOdour;
static _zParEmitter* sEmitGrabPowerup;
static _zParEmitter* sEmitGrabCrumb;
static _zParEmitter* sEmitGrabSnackBox;
static _zParEmitter* sEmitGrabMmm;
static _zParEmitter* sEmitGrabQMark;
static _zParEmitter* sEmitGrabGum;
static _zParEmitter* sEmitGrabBubbles;
static _zParEmitter* sEmitLoveHart;
static _zParEmitter* sEmitAkmalDaisy;
static _zParEmitter* sEmitGrabGhost;
static _zParEmitter* sEmitGrabSDLogo;
static _zParEmitter* sEmitWarpGate;
static _zParEmitter* sEmitMiniVanPuff;
static _zCounter* keyCounter;

static _tagKeyShake sKeyShake[4] = {};
static zEntPickupWarpPoint zWarpPointTable[27] = {
    { 0, 0, 'B004' },  { 1, 0, 'C004' },  { 2, 0, 'E004' },  { 3, 0, 'E006' },
    { 4, 0, 'E009' },  { 5, 0, 'F003' },  { 6, 0, 'F007' },  { 7, 0, 'F010' },
    { 8, 0, 'G005' },  { 9, 0, 'G008' },  { 10, 1, 'H001' }, { 11, 0, 'I003' },
    { 12, 0, 'I006' }, { 13, 0, 'L014' }, { 14, 0, 'L018' }, { 15, 0, 'O004' },
    { 16, 0, 'O006' }, { 17, 0, 'P003' }, { 18, 0, 'P005' }, { 19, 0, 'R003' },
    { 20, 0, 'S002' }, { 21, 0, 'W022' }, { 22, 0, 'W026' }, { 23, 0, 'L015' },
    { 24, 0, 'G001' }, { 25, 0, 'H003' }, { 0, 0, 0 },
};

static _xEntFrame gPickupFrame;
static _xEntCollis gPickupCollis;

static void zEntPickup_ShowKeyCnt(unsigned int collected, unsigned int total);

static inline void zEntPickup_PlaySound(_zEntPickup* ent)
{
    if (ent->p->pickupType == 1)
    {
        switch (ent->p->pickupIndex)
        {
        case 0:
        case 1:
        case 3:
        case 4:
            zEntPlayer_SNDBitePickup(ent);
            break;
        case 7:
            zEntPlayer_SNDInfoPickup(ent);
            break;
        case 5:
            zEntPlayer_SNDKeyPickup(ent);
            break;
        case 6:
            zEntPlayer_SNDSavePickup(ent);
            break;
        case 2:
        default:
            zEntPlayer_SNDSpecialPickup(ent);
            break;
        }
    }
    else if (ent->p->pickupType == 2)
    {
        zEntPlayer_SNDSpecialPickup(ent);
        zMusicNotify(2);
    }
    else if (ent->p->pickupType == 3 || ent->p->pickupType == 5)
    {
        zEntPlayer_SNDAmmoPickup(ent);
    }
    else
    {
        zEntPlayer_SNDTokenPickup(ent);
    }
}

static inline void zEntPickup_TallyClue(_zEntPickup* ent)
{
    if (ent->p->pickupType == 1)
    {
        switch (ent->p->pickupIndex)
        {
        case 6:
            break;
        case 7:
            zSceneGameStatsTally_clue();
            break;
        }
    }
}

static inline void zEntPickup_MakeDroppableInline(_zEntPickup* ent, _xVec3* pos, xBase* parent)
{
    ent->droppos = *(_xVec3*)&ent->model->Mat->pos;
    *(_xVec3*)&ent->model->Mat->pos = *pos;
    ent->state = 0x10;
    ent->flags &= ~1;
    ent->dropParent = parent;
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zEntPickupInit(void* ent, void* asset)
{
    zEntPickupInit((_zEntPickup*)ent, (p2EntAsset*)asset);
}

void zEntPickupInit(_zEntPickup* ent, p2EntAsset* asset)
{
    unsigned int size;
    unsigned int i;

    zEntInit(ent, asset, 'PKUP');
    if (ent->linkCount != 0)
    {
        ent->link = (p2LinkAsset*)((zEntPickupAsset*)ent->asset + 1);
    }
    else
    {
        ent->link = NULL;
    }
    for (i = 0; i < ent->linkCount; i++)
    {
        p2LinkAsset* link = &((p2LinkAsset*)ent->link)[i];
        if (link->dstEvent == 0x99)
        {
            link->dstAssetID = ent->id;
        }
    }

    ent->pflags = 0;
    ent->penby = 0;
    ent->chkby = 0;
    ent->update = (void (*)(_xEnt*, _xScene*, float))zEntPickup_Update;
    ent->move = NULL;
    ent->eventFunc = zEntPickupEventCB;
    ent->followTarget = NULL;
    ent->followOffset.x = 0.0f;
    ent->followOffset.y = 0.0f;
    ent->followOffset.z = 0.0f;
    ent->shake_timer = 0.0f;
    ent->snackGateInfoDisplayed = 0;
    ent->dropParent = NULL;

    zAssetPickupTable* table = (zAssetPickupTable*)globals.pickupTable;
    zAssetPickup* p = table->entries;
    for (i = 0; i < table->Count; i++)
    {
        if (((zEntPickupAsset*)asset)->pickupHash == p[i].pickupHash)
        {
            p += i;
            break;
        }
    }
    if (i == table->Count)
    {
        fn_80015EAC("ERROR:  Pickup table entry not found for hash 0x%x\n",
                    ((zEntPickupAsset*)asset)->pickupHash);
    }

    unsigned int animID = p->animID;
    RpAtomic* model = (RpAtomic*)xSTFindAsset(p->modelID, &size);
    void* anim = NULL;
    if (animID != 0)
    {
        anim = xSTFindAsset(animID, &size);
        if (iAnimBoneCount(anim) != iModelNumBones(model))
        {
            anim = NULL;
        }
    }

    ent->model = xModelInstanceAlloc(model, ent, 0, 0, NULL);
    ent->model->Flags &= ~2;
    ent->state = 0x41;
    ent->p = p;
    if (ent->p->pickupType == 1 && ent->p->pickupIndex == 2)
    {
        ent->p->pickupIndex = 1;
    }
    if (ent->p->pickupType == 1 && (ent->p->pickupIndex == 4 || ent->p->pickupIndex == 0))
    {
        ent->asset->baseFlags |= 2;
        ent->baseFlags |= 2;
    }
    ent->anim = anim;
    ent->animTime = 0.0f;
    ent->timer = 0.0f;
    ent->fx_timer = 0.0f;
    ent->fx_par = NULL;
    ent->fx_emit = NULL;
    ent->fx_scale = 1.0f;
    ent->flyflags = 0;
    zEntReset(ent);
    ent->bound.sph.r = 1.0f;
}

void zEntPickup_Setup(_zEntPickup* p)
{
    zEntSetup(p);
    keyCounter = (_zCounter*)zSceneFindObject(xStrHash("KEY_COUNTER"));
}

void zEntPickup_Setup()
{
    zScene* sc = globals.sceneCur;
    unsigned int i;
    unsigned int j;

    for (i = 0; i < sc->num_base; i++)
    {
        xBase* b = sc->base[i];
        if (b == NULL)
        {
            continue;
        }
        for (j = 0; j < b->linkCount; j++)
        {
            p2LinkAsset* link = &((p2LinkAsset*)b->link)[j];
            if (link->dstEvent != 0xB2)
            {
                continue;
            }
            _zEntPickup* pickup = (_zEntPickup*)zSceneFindObject(link->dstAssetID);
            if (pickup == NULL || pickup->baseType != 4 || pickup->state == 0x10)
            {
                continue;
            }
            _xVec3* pos = &pickup->asset->pos;
            switch (b->baseType)
            {
            case 2:
                pickup->flg_opts |= 1;
                pickup->flg_opts |= 2;
                pickup->useThisEntPos = (_xEnt*)b;
            case 11:
            case 12:
            case 24:
            case 27:
            case 35:
                pos = &((_xEnt*)b)->asset->pos;
                break;
            }
            zEntPickup_MakeDroppableInline(pickup, pos, NULL);
        }
    }

    sEmitGlow = zParEmitterFind("PAREMIT_PICKUP_GLOW");
    if (sEmitGlow != NULL && sEmitGlow->group != NULL)
    {
        xParGroupSetStealing(sEmitGlow->group, 0);
    }
    sEmitMmm = zParEmitterFind("PAREMIT_PICKUP_MMM");
    sEmitQMark = zParEmitterFind("PAREMIT_PICKUP_QMARK");
    sEmitBubble = zParEmitterFind("PAREMIT_PICKUP_BUBBLE");
    if (sEmitBubble != NULL && sEmitBubble->group != NULL)
    {
        xParGroupSetStealing(sEmitBubble->group, 0);
    }
    sEmitGum = zParEmitterFind("PAREMIT_PICKUP_GUM");
    if (sEmitGum != NULL && sEmitGum->group != NULL)
    {
        xParGroupSetStealing(sEmitGum->group, 0);
    }
    sEmitOdour = NULL;
    sEmitGrabPowerup = zParEmitterFind("PAREMIT_PICKUP_SUPER");
    sEmitGrabCrumb = zParEmitterFind("PAREMIT_GRAB_CRUMB");
    sEmitGrabSnackBox = NULL;
    sEmitGrabMmm = zParEmitterFind("PAREMIT_GRAB_MMM");
    sEmitGrabQMark = zParEmitterFind("PAREMIT_GRAB_QMARK");
    sEmitGrabGum = zParEmitterFind("PAREMIT_GRAB_GUM");
    sEmitGrabBubbles = zParEmitterFind("PAREMIT_GRAB_BUBBLES");
    sEmitGrabGhost = zParEmitterFind("PAREMIT_GRAB_GHOST");
    sEmitGrabSDLogo = NULL;
    sEmitWarpGate = zParEmitterFind("PAREMIT_WARP");
    sEmitMiniVanPuff = zParEmitterFind("PAREMIT_SMALL_VAN_PUFF");
    sEmitLoveHart = zParEmitterFind("PAREMIT_LOVEHART");
    sEmitAkmalDaisy = zParEmitterFind("PAREMIT_DAISY");

    for (i = 0; i < 4; i++)
    {
        sKeyShake[i].shake_timer = 0.0f;
        sKeyShake[i].ui = NULL;
    }
    sKeyCounterEnabledBeforeGive = 0;
}

signed int zEntPickupEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                             xBase* toParamWidget)
{
    _zEntPickup* ent = (_zEntPickup*)to;

    switch (toEvent)
    {
    case 3:
        if (!(ent->state & 8))
        {
            ent->flags |= 1;
        }
        break;
    case 4:
        ent->flags &= ~1;
        break;
    case 10:
        zEntPickup_Reset(ent);
        break;
    case 0x23:
        if (ent->p->pickupType == 2)
        {
            unsigned int bit = 1 << ent->p->pickupIndex;
            if (!(globals.player.Inv_Specials & bit))
            {
                globals.player.SpecialReceived = ent->p->pickupIndex;
                globals.player.Inv_Specials |= bit;
            }
        }
        break;
    case 0xB2:
        if (ent->state == 0x10)
        {
            zEntPickup_Drop(ent);
        }
        break;
    case 0xD9:
        ent->followTarget = toParamWidget;
        ent->followOffset.x = toParam[0];
        ent->followOffset.y = toParam[1];
        ent->followOffset.z = toParam[2];
        break;
    }
    return 1;
}

#define PICKUP_MIN(a, b) ((a) < (b) ? (a) : (b))
#define PICKUP_MAX(a, b) ((a) < (b) ? (b) : (a))

signed int zPickup_Give(_zEntPickup* ent, zAssetPickup* p, unsigned short value)
{
    switch (p->pickupType)
    {
    case 1:
        switch (p->pickupIndex)
        {
        case 7:
            zEntPlayer_GiveClue(p->quantity);
            return 0;
        case 5:
            if (keyCounter != NULL && (keyCounter->baseFlags & 1))
            {
                zEntPickup_ShowKeyCnt(keyCounter->asset->count - keyCounter->count,
                                      keyCounter->asset->count);
            }
            break;
        case 4:
            zHud_showSnack();
            zEntPlayer_GiveScoobySnack(5);
            zEntPlayer_Notify(0x24);
            break;
        case 0:
            zEntPlayer_GiveScoobySnack(p->quantity);
            break;
        case 1:
            if (globals.player.Health >= 5)
            {
                zHud_showHealthandStatus();
                return 0;
            }
            zEntPlayer_GiveHealth(p->quantity);
            zEntPlayer_Notify(0x22);
            break;
        case 2:
            zEntPlayer_GiveChance(p->quantity);
            break;
        case 3:
            zEntPlayer_GiveHotsauce(value);
            break;
        case 9:
            if (!zEntPlayer_HasScoobySnack(value))
            {
                return 0;
            }
            zEntPlayer_TakeScoobySnack(ent, value);
            break;
        }
        break;
    case 2:
        globals.player.SpecialReceived = p->pickupIndex;
        globals.player.Inv_Specials |= 1 << p->pickupIndex;
        break;
    case 3:
    {
        if (globals.player.Inv_Ammo[p->pickupIndex] >= globals.player.Inv_AmmoMax[p->pickupIndex])
        {
            return 0;
        }
        globals.player.Inv_Ammo[p->pickupIndex] =
            PICKUP_MAX(PICKUP_MIN(globals.player.Inv_Ammo[p->pickupIndex] + (signed int)p->quantity,
                                  globals.player.Inv_AmmoMax[p->pickupIndex]),
                       0);
        break;
    }
    case 5:
    {
        globals.player.Inv_AmmoMax[p->pickupIndex] += p->quantity;
        globals.player.Inv_Ammo[p->pickupIndex] =
            PICKUP_MAX(PICKUP_MIN(globals.player.Inv_Ammo[p->pickupIndex] + (signed int)p->quantity,
                                  globals.player.Inv_AmmoMax[p->pickupIndex]),
                       0);
        break;
    }
    case 4:
        zSceneGameStatsTally_token();
        globals.player.Inv_Mtokens[p->pickupIndex >> 5] |= 1 << (p->pickupIndex & 0x1F);
        break;
    }
    zEntPickup_GiveFX(ent);
    return 1;
}

void zEntPickup_FlyToInterface(_zEntPickup* ent, float dt)
{
    _xMat4x3 mat;
    float t;
    float s;
    _xVec3 d;
    mat = *xCameraGetCurMatrix((_tagxCamFrame*)&globals.camera);

    if (ent->p->pickupIndex == 5)
    {
        xMat4x3MoveLocalUp(&mat, 0.225f);
        switch (keyCounter->asset->count - keyCounter->count)
        {
        case 1:
            xMat4x3MoveLocalRight(&mat, 0.17f);
            break;
        case 2:
            xMat4x3MoveLocalRight(&mat, 0.08f);
            break;
        case 3:
            xMat4x3MoveLocalRight(&mat, -0.08f);
            break;
        case 4:
        default:
            xMat4x3MoveLocalRight(&mat, -0.17f);
            break;
        }
        xMat4x3MoveLocalAt(&mat, 0.5f);
    }
    else
    {
        xMat4x3MoveLocalUp(&mat, 0.225f);
        xMat4x3MoveLocalRight(&mat, -0.25f);
        xMat4x3MoveLocalAt(&mat, 0.5f);
    }

    t = 1.0f - ent->timer / 0.4f;
    if (t < 0.0f)
    {
        t = 0.0f;
    }
    else if (t > 1.0f)
    {
        t = 1.0f;
    }
    t = fn_800294A0(t);
    xVec3Sub(&d, &mat.pos, &ent->grab_pos);
    s = 0.9f * t;
    ent->model->Mat->pos.x = d.x * t + ent->grab_pos.x;
    ent->model->Mat->pos.y = d.y * t + ent->grab_pos.y;
    ent->model->Mat->pos.z = d.z * t + ent->grab_pos.z;
    ent->fx_scale = 1.0f - s;
    ent->vel.x = 0.0f;
    ent->vel.y = 0.0f;
    ent->vel.z = 0.0f;
}

void zEntPickup_Update(_zEntPickup* ent, _xScene* sc, float dt)
{
    if (ent->state & 8)
    {
        if (!(ent->p->pickupFlags & 1))
        {
            return;
        }
        if (ent->timer > 0.0f)
        {
            ent->timer -= dt;
            if (ent->timer <= 0.0f)
            {
                ent->timer = 0.5f;
                ent->state = (ent->state & ~0x3F) | 1;
                ent->flags |= 1;
            }
            else
            {
                return;
            }
        }
        else
        {
            return;
        }
    }

    if (!xEntIsVisible(ent))
    {
        if (ent->fx_par != NULL && !(ent->flyflags & 2))
        {
            ent->flyflags |= 2;
            ((xParLike*)ent->fx_par)->pos = *(_xVec3*)&ent->model->Mat->pos;
            ((xParLike*)ent->fx_par)->pos.y += 10000.0f;
        }
        return;
    }

    if ((ent->state & 0x40) && !(ent->state & 4) && !(ent->state & 0x20) &&
        globals.player.Health != 0)
    {
        ent->fx_scale = 1.0f;
        if (ent->timer > 0.0f)
        {
            ent->timer -= dt;
            if (ent->timer <= 0.0f)
            {
                ent->timer = 0.0f;
            }
        }
        else
        {
            zAssetPickup* p;
            float dz;
            float dy;
            float dx;
            float d1;
            _xVec3* pos;
            float hdz;
            float hdx;
            float d2;
            p = ent->p;
            pos = (_xVec3*)&ent->model->Mat->pos;
            dx = globals.player.ent.bound.sph.center.x - pos->x;
            hdx = globals.player.head_sph.center.x - pos->x;
            dy = globals.player.ent.bound.sph.center.y - pos->y;
            dx *= dx;
            hdx *= hdx;
            dy *= dy;
            dz = globals.player.ent.bound.sph.center.z - pos->z;
            hdz = globals.player.head_sph.center.z - pos->z;
            dz *= dz;
            hdz *= hdz;
            d1 = dz + (dx + dy);
            d2 = hdz + (hdx + dy);

            if (p->pickupType == 1 && p->pickupIndex == 8)
            {
                if (globals.sceneCur->pendingPortal == NULL &&
                    (d1 <= (2.5f * globals.player.ent.bound.sph.r) * (2.5f * globals.player.ent.bound.sph.r) ||
                     d2 <= (2.5f * globals.player.ent.bound.sph.r) * (2.5f * globals.player.ent.bound.sph.r)))
                {
                    if (zHudShowWarpGateInfo == -1)
                    {
                        zHudShowWarpGateInfo = p->quantity;
                        zEntEvent("MNU4 WARP PROMPT", 3);
                        zEntEvent("MNU4 WARP NAME", 3);
                        zWarpPointTable[((zEntPickupAsset*)ent->asset)->pickupValue].status = 1;
                        xSndPlay(xStrHash("warpgate_activate"), 0.5f, 0.5f, 0.0f, 0x7F, 0, 0);
                        xSndPlay3D(xStrHash("warpgate_idle_loop"), 0.5f, 0.5f, 0x7F, 0x200, ent,
                                   20.0f);
                        zEntPlayer_Notify(0xE);
                    }
                    if (globals.pad0 != NULL && (globals.pad0->pressed & 0x20000))
                    {
                        xBase* sel = zSceneFindObject(xStrHash("SELECT X UIF"));
                        if (sel != NULL)
                        {
                            if (zHudShowWarpGateInfo != -1)
                            {
                                zEntEvent("MNU4 WARP PROMPT", 4);
                                zEntEvent("MNU4 WARP NAME", 4);
                                zHudShowWarpGateInfo = -1;
                            }
                            zGameStateSwitchEvent(0x9B);
                            zEntEvent(sel, 0x51);
                            zEntEvent(sel, 0x4F);
                        }
                    }
                }
                else if (zHudShowWarpGateInfo != -1)
                {
                    zEntEvent("MNU4 WARP PROMPT", 4);
                    zEntEvent("MNU4 WARP NAME", 4);
                    zHudShowWarpGateInfo = -1;
                }
            }
            else if (p->pickupType == 1 && p->pickupIndex == 9)
            {
                if (lbl_8025ECFC == 0)
                {
                    float r = 2.5f * globals.player.ent.bound.sph.r;
                    if ((d1 <= r * r || d2 <= r * r))
                    {
                        if (ent->snackGateInfoDisplayed == -1)
                        {
                            ent->snackGateInfoDisplayed =
                                ((zEntPickupAsset*)ent->asset)->pickupValue;
                            zVarSetSnackGateValue(ent->snackGateInfoDisplayed);
                            zEntEvent("MNU4 SNACKGATE PROMPT", 0x51);
                        }
                        if (zPickup_Give(ent, ent->p, ((zEntPickupAsset*)ent->asset)->pickupValue))
                        {
                            zEntPickup_TallyClue(ent);
                            ent->grab_pos = *(_xVec3*)&ent->model->Mat->pos;
                            ent->timer = 0.0f;
                            ent->vel.x = 0.0f;
                            ent->vel.y = 0.0f;
                            ent->vel.z = 0.0f;
                        }
                        else if (ent->shake_timer <= 0.0f)
                        {
                            ent->shake_pos = *(_xVec3*)&ent->model->Mat->pos;
                            ent->shake_timer = 3.0f;
                            xSndPlay(xStrHash("snackgate_error1"), 0.5f, 0.5f, 0.0f, 0x7F, 0, 0);
                            xSndPlay(xStrHash("snackgate_spin3"), 0.5f, 0.5f, 0.0f, 0x7F, 0x200,
                                     0);
                            zEntPlayer_Notify(0xF);
                            zHud_showSnack();
                        }
                    }
                    else if (ent->snackGateInfoDisplayed != -1)
                    {
                        zEntEvent("MNU4 SNACKGATE PROMPT", 0x52);
                        ent->snackGateInfoDisplayed = -1;
                    }
                }
            }
            else if (p->pickupType == 1 && p->pickupIndex == 7)
            {
                float r = 1.5f * globals.player.ent.bound.sph.r;
                if ((d1 <= r * r || d2 <= r * r))
                {
                    zEntPickup_GiveFX(ent);
                    ent->state = (ent->state & ~0x3F) | 0x20;
                    zEntPickup_PlaySound(ent);
                    zEntPickup_TallyClue(ent);
                    ent->timer = 10.0f;
                    ent->vel.x = 0.0f;
                    ent->vel.y = 0.0f;
                    ent->vel.z = 0.0f;
                    zEntEvent(ent, ent, 0x23);
                }
            }
            else if (p->pickupType == 1 && p->pickupIndex == 6)
            {
                float r = 1.5f * globals.player.ent.bound.sph.r;
                if ((d1 <= r * r || d2 <= r * r))
                {
                    zEntPickup_GiveFX(ent);
                    ent->state = (ent->state & ~0x3F) | 0x20;
                    zEntPickup_TallyClue(ent);
                    ent->timer = 20.0f;
                    ent->vel.x = 0.0f;
                    ent->vel.y = 0.0f;
                    ent->vel.z = 0.0f;
                    zEntEvent(ent, ent, 0x23);
                    zGameStateSwitchEvent(0x99);
                }
            }
            else
            {
                float r = 1.5f * globals.player.ent.bound.sph.r;
                if ((d1 <= r * r || d2 <= r * r))
                {
                    if (p->pickupType == 1 && (p->pickupIndex == 0 || p->pickupIndex == 4))
                    {
                        ent->state = (ent->state & ~0x3F) | 4;
                        zEntPickup_PlaySound(ent);
                        zHud_showSnack();
                        ent->grab_pos = *(_xVec3*)&ent->model->Mat->pos;
                        ent->timer = 0.4f;
                        ent->vel.x = 0.0f;
                        ent->vel.y = 0.0f;
                        ent->vel.z = 0.0f;
                        ent->flyflags |= 1;
                        if (zGameExtraIsValentines())
                        {
                            _zParEmitterCustomSettings info;
                            info.custom_flags = 0x20;
                            info.pos = *(_xVec3*)&ent->model->Mat->pos;
                            zParEmitterEmitCustom(sEmitLoveHart, 1.0f / 30.0f, &info);
                        }
                        else if (zGameExtraIsShirazBday())
                        {
                            _zParEmitterCustomSettings info;
                            info.custom_flags = 0x20;
                            info.pos = *(_xVec3*)&ent->model->Mat->pos;
                            zParEmitterEmitCustom(sEmitAkmalDaisy, 1.0f / 30.0f, &info);
                        }
                        else if (zGameExtraIsAnnaBday())
                        {
                            _zParEmitterCustomSettings info;
                            info.custom_flags = 0x20;
                            info.pos = *(_xVec3*)&ent->model->Mat->pos;
                            zParEmitterEmitCustom(sEmitLoveHart, 1.0f / 30.0f, &info);
                        }
                        else if (ent->p->pickupIndex == 0)
                        {
                            _zParEmitterCustomSettings info;
                            info.custom_flags = 0x20;
                            info.pos = *(_xVec3*)&ent->model->Mat->pos;
                            zParEmitterEmitCustom(sEmitGrabCrumb, 1.0f / 30.0f, &info);
                        }
                    }
                    else
                    {
                        sKeyCounterEnabledBeforeGive = 0;
                        if (keyCounter != NULL && (keyCounter->baseFlags & 1))
                        {
                            sKeyCounterEnabledBeforeGive = 1;
                        }
                        if (zPickup_Give(ent, ent->p, ((zEntPickupAsset*)ent->asset)->pickupValue))
                        {
                            ent->state = (ent->state & ~0x3F) | 4;
                            zEntPickup_PlaySound(ent);
                            zEntPickup_TallyClue(ent);
                            ent->grab_pos = *(_xVec3*)&ent->model->Mat->pos;
                            if (ent->p->pickupIndex == 5 && keyCounter != NULL &&
                                sKeyCounterEnabledBeforeGive)
                            {
                                ent->timer = 0.4f;
                                ent->vel.x = 0.0f;
                                ent->vel.y = 0.0f;
                                ent->vel.z = 0.0f;
                                ent->flyflags |= 1;
                            }
                            else
                            {
                                ent->timer = 3.0f;
                                ent->vel.x = 0.0f;
                                ent->vel.y = 15.0f;
                                ent->vel.z = 0.0f;
                                ent->flyflags = 0;
                                zEntPlayer_GiveKey(ent->p->quantity);
                            }
                            zEntEvent(ent, ent, 0x23);
                        }
                        else if (ent->shake_timer <= 0.0f)
                        {
                            zEntPlayer_SNDErrorPickup(ent);
                            ent->shake_pos = *(_xVec3*)&ent->model->Mat->pos;
                            ent->shake_timer = 3.0f;
                        }
                    }
                }
                else
                {
                    float r8 = 8.0f * globals.player.ent.bound.sph.r;
                    if ((d1 <= r8 * r8 || d2 <= r8 * r8) && p->pickupType == 4)
                    {
                        zEntPlayer_Notify(3);
                    }
                }
            }
        }
    }

    if (ent->anim != NULL)
    {
        float duration = iAnimDuration(ent->anim);
        ent->animTime += dt;
        if (ent->animTime >= duration)
        {
            ent->animTime -= duration;
        }
        _xQuat* quat = (_xQuat*)giAnimScratch;
        _xVec3* tran = (_xVec3*)((unsigned char*)quat + 0x410);
        iAnimEval(ent->anim, ent->animTime, 0, tran, quat);
        iModelAnimMatrices(ent->model->Data, quat, tran, ent->model->Mat + 1);
    }

    if (ent->shake_timer != 0.0f)
    {
        ent->shake_timer -= dt;
        if (ent->shake_timer < 0.0f)
        {
            *(_xVec3*)&ent->model->Mat->pos = ent->shake_pos;
            ent->shake_timer = 0.0f;
        }
        else
        {
            xMat3x3RMulRotY((_xMat3x3*)ent->model->Mat, (_xMat3x3*)ent->model->Mat,
                            18.849556f * dt * (ent->shake_timer / 3.0f));
        }
    }

    if (ent->anim == NULL)
    {
        if (ent->p->pickupType == 1 && ent->p->pickupIndex == 8)
        {
            _xVec3* ppos = (_xVec3*)&globals.player.ent.model->Mat->pos;
            float px = ppos->x;
            float py = 1.0f + ppos->y;
            float pz = ppos->z;
            float dist = 0.0f;
            if (zWarpPointTable[((zEntPickupAsset*)ent->asset)->pickupValue].status)
            {
                _xVec3 dir;
                dir.x = px - ent->model->Mat->pos.x;
                dir.y = py - ent->model->Mat->pos.y;
                dir.z = pz - ent->model->Mat->pos.z;
                dist = xMat3x3LookVec((_xMat3x3*)ent->model->Mat, &dir);
            }
            if (zWarpPointTable[((zEntPickupAsset*)ent->asset)->pickupValue].status &&
                sEmitWarpGate != NULL && dist < 7.5f)
            {
                _zParEmitterCustomSettings info;
                info.custom_flags = 0x20;
                info.pos = *(_xVec3*)&ent->model->Mat->pos;
                zParEmitterEmitCustom(sEmitWarpGate, dt, &info);
            }
        }
        else
        {
            xMat3x3RMulRotY((_xMat3x3*)ent->model->Mat, (_xMat3x3*)ent->model->Mat,
                            3.1415927f * dt);
        }
    }

    switch (ent->state & 0x3F)
    {
    case 1:
        if (ent->followTarget != NULL)
        {
            _xVec3* tpos = (_xVec3*)&((_xEnt*)ent->followTarget)->model->Mat->pos;
            if (tpos != NULL)
            {
                *(_xVec3*)&ent->model->Mat->pos = *tpos;
                xVec3AddTo((_xVec3*)&ent->model->Mat->pos, &ent->followOffset);
            }
        }
        ent->fx_scale = 1.0f;
        zEntPickup_UpdateFX(ent, sc, dt);
        break;
    case 2:
    {
        gPickupFrame.mat = *(_xMat4x3*)ent->model->Mat;
        gPickupFrame.mat.pos.y -= 0.8f;
        gPickupCollis.chk = 0x22;
        gPickupFrame.oldpos = gPickupFrame.mat.pos;
        gPickupCollis.pen = 0x22;
        ent->vel.y = -(15.0f * dt - ent->vel.y);
        if (ent->vel.y < -20.0f)
        {
            ent->vel.y = -20.0f;
        }
        gPickupFrame.mat.pos.x = ent->vel.x * dt + gPickupFrame.mat.pos.x;
        gPickupFrame.mat.pos.y = ent->vel.y * dt + gPickupFrame.mat.pos.y;
        gPickupFrame.mat.pos.z = ent->vel.z * dt + gPickupFrame.mat.pos.z;
        if (!(ent->state & 0x80) || ent->vel.y < 0.0f)
        {
            ent->frame = &gPickupFrame;
            ent->collis = (::_xEntCollis*)&gPickupCollis;
            xEntCollide(ent, sc, dt);
            ent->frame = NULL;
            ent->collis = NULL;
        }
        gPickupFrame.mat.pos.y += 0.8f;
        *(_xMat4x3*)ent->model->Mat = gPickupFrame.mat;
        if ((gPickupCollis.colls0flags & 1) && ent->vel.y < 0.0f)
        {
            ent->vel.x = 0.0f;
            ent->vel.y = 0.0f;
            ent->vel.z = 0.0f;
            ent->state &= ~0x3F;
            ent->state |= 1;
        }
        break;
    }
    case 0x20:
        if (globals.player.ControlOn)
        {
            ent->timer -= dt;
        }
        if (ent->timer <= 0.0f)
        {
            ent->state = (ent->state & ~0x3F) | 1;
        }
        else
        {
            float deg;
            float total;
            if (ent->p->pickupType == 1 && ent->p->pickupIndex == 7)
            {
                deg = 1080.0f;
                total = 10.0f;
            }
            else
            {
                deg = 720.0f;
                total = 20.0f;
            }
            xMat3x3RMulRotY((_xMat3x3*)ent->model->Mat, (_xMat3x3*)ent->model->Mat,
                            ((3.1415927f * deg) / 180.0f) * dt * (ent->timer / total));
        }
        zEntPickup_UpdateFX(ent, sc, dt);
        break;
    case 4:
        ent->timer -= dt;
        if (ent->timer <= 0.0f)
        {
            if (ent->p->pickupType == 1)
            {
                if (ent->p->pickupIndex == 0 || ent->p->pickupIndex == 4)
                {
                    zHud_ShakeSnackCounter();
                    if (zPickup_Give(ent, ent->p, ((zEntPickupAsset*)ent->asset)->pickupValue))
                    {
                        zEntPlayer_SNDSnackCounter(ent);
                        zEntEvent(ent, ent, 0x23);
                    }
                }
                else if (ent->p->pickupIndex == 5)
                {
                    if (keyCounter != NULL && sKeyCounterEnabledBeforeGive)
                    {
                        zEntPlayer_GiveKey(ent->p->quantity);
                        zEntPlayer_SNDSnackCounter(ent);
                        if (keyCounter != NULL)
                        {
                            char s[64];
                            signed short count = keyCounter->count;
                            signed int idx = keyCounter->asset->count - count;
                            sprintf(s, "KEY ON %d UI", idx, count);
                            xBase* ui = zSceneFindObject(xStrHash(s));
                            if (ui != NULL && idx - 1 >= 0 && idx - 1 < 4)
                            {
                                _tagKeyShake* ks = &sKeyShake[idx - 1];
                                ks->shake_timer = 1.0f;
                                ks->ui = (_zUI*)ui;
                                _zUILike::_zUIAsset* a = ((_zUILike*)ks->ui)->uiasset;
                                ks->orig_pos.x = a->x;
                                ks->orig_pos.y = a->y;
                                ks->orig_pos.z = a->z;
                                ks->shake_it = 10.0f;
                            }
                        }
                        zEntPickup_ShowKeyCnt(keyCounter->asset->count - keyCounter->count,
                                              keyCounter->asset->count);
                    }
                }
            }
            ent->state = (ent->state & ~0x3F) | 8;
            ent->flags &= ~1;
            ent->timer = 10.0f;
            ent->model->Mat->pos.x = ent->asset->pos.x;
            ent->model->Mat->pos.y = ent->asset->pos.y;
            ent->model->Mat->pos.z = ent->asset->pos.z;
        }
        else
        {
            ent->model->Mat->pos.x += ent->vel.x * dt;
            ent->model->Mat->pos.y += ent->vel.y * dt;
            ent->model->Mat->pos.z += ent->vel.z * dt;
        }
        break;
    }

    if (sKeysOffTimer > -10.0f && sKeysOffTimer < 0.0f)
    {
        sKeysOffTimer = -10.1f;
        zEntEvent("ALL KEYS GROUP", 4);
    }
}

#pragma auto_inline on
void zEntPickup_hideSnackGateInfo(_zEntPickup* ent)
{
    if (ent->snackGateInfoDisplayed != -1)
    {
        zEntEvent("MNU4 SNACKGATE PROMPT", 0x52);
        ent->snackGateInfoDisplayed = -1;
    }
}
#pragma auto_inline reset

#pragma auto_inline on
void zEntPickup_Reset(_zEntPickup* ent)
{
    zEntReset(ent);
    ent->state = 0x41;
    ent->animTime = 0.0f;
    ent->timer = 0.0f;
    ent->vel.z = 0.0f;
    ent->vel.y = 0.0f;
    ent->vel.x = 0.0f;
    ent->bound.type = 1;
    ent->shake_timer = 0.0f;
    ent->fx_timer = 0.0f;
    ent->fx_par = NULL;
    ent->fx_emit = NULL;
    ent->fx_scale = 1.0f;
    ent->flyflags = 0;
    if (zHudShowWarpGateInfo != -1)
    {
        zEntEvent("MNU4 WARP PROMPT", 4);
        zEntEvent("MNU4 WARP NAME", 4);
        zHudShowWarpGateInfo = -1;
    }
    zEntPickup_hideSnackGateInfo(ent);
    for (signed int i = 0; i < 4; i++)
    {
        sKeyShake[i].shake_timer = 0.0f;
        sKeyShake[i].ui = NULL;
    }
    sKeyCounterEnabledBeforeGive = 0;
}
#pragma auto_inline reset

void zEntPickup_Save(_zEntPickup* ent, xSerial* s)
{
    zEntSave(ent, s);
    Write_b7(s, ent->state);
}

void zEntPickup_Load(_zEntPickup* ent, xSerial* s)
{
    signed int wasDroppable = 0;
    zEntLoad(ent, s);
    if (ent->state & 0x10)
    {
        wasDroppable = 1;
    }
    Read_b7(s, &ent->state);
    if (wasDroppable && (ent->state & 0x23))
    {
        _zEntPickup* parent = (_zEntPickup*)ent->dropParent;
        if (parent != NULL && parent->baseType == 0xB && !(parent->baseFlags & 2) &&
            (*(unsigned int*)((unsigned char*)parent + 0xF0) & 1))
        {
            _xVec3* pos = (_xVec3*)&ent->model->Mat->pos;
            fn_80015EAC("Put back  %f %f %f    %f %f %f\n", pos->x, pos->y, pos->z, ent->droppos.x,
                        ent->droppos.y, ent->droppos.z);
            ent->state = 0x10;
            ent->flags &= ~1;
            return;
        }
        *(_xVec3*)&ent->model->Mat->pos = ent->droppos;
    }
}

void zEntPickup_MakeDroppable(_zEntPickup* ent, _xVec3* pos, xBase* parent)
{
    zEntPickup_MakeDroppableInline(ent, pos, parent);
}

#pragma auto_inline on
void zEntPickup_Drop(_zEntPickup* ent)
{
    ent->state = 0xC2;
    ent->flags |= 1;
    if (ent->flg_opts & 2)
    {
        if (ent->useThisEntPos != NULL)
        {
            ent->droppos = *(_xVec3*)&ent->useThisEntPos->model->Mat->pos;
            *(_xVec3*)&ent->model->Mat->pos = *(_xVec3*)&ent->useThisEntPos->model->Mat->pos;
        }
    }
    float h = ent->model->Mat->pos.y - ent->droppos.y;
    float vy = xsqrt(75.0f);
    ent->vel.x = 0.0f;
    ent->vel.y = vy;
    ent->vel.z = 0.0f;
    if (!(ent->flg_opts & 1))
    {
        float t2;
        float t1;
        unsigned int n = xMathSolveQuadratic(-7.5f, vy, h, &t1, &t2);
        if (n != 0)
        {
            if (n == 2 && t2 > t1)
            {
                t1 = t2;
            }
            if (t1 > 0.0001f)
            {
                t1 = 1.0f / t1;
                ent->vel.x = t1 * (ent->droppos.x - ent->model->Mat->pos.x);
                ent->vel.z = t1 * (ent->droppos.z - ent->model->Mat->pos.z);
            }
        }
    }
    ent->timer = 1.2f;
}
#pragma auto_inline reset

void zEntPickup_UpdateFX(_zEntPickup* ent, _xScene* sc, float dt)
{
    signed int doGlow = 0;

    if (sEmitMiniVanPuff != NULL && ent->p->pickupType == 1 && ent->p->pickupIndex == 6)
    {
        signed int visible;
        if (globals.cmgr == NULL)
        {
            visible = 1;
        }
        else if (!(ent->flyflags & 2))
        {
            visible = 1;
        }
        else
        {
            visible = 0;
        }
        if (visible)
        {
            _zParEmitterCustomSettings info;
            info.custom_flags = 0x64;
            _xMat4x3* m = (_xMat4x3*)ent->model->Mat;
            info.pos = m->pos;
            info.pos.x = -0.35f * m->right.x + info.pos.x;
            info.pos.y += 0.05f;
            info.pos.z = -0.35f * m->right.z + info.pos.z;
            info.pos.x = 0.4f * m->at.x + info.pos.x;
            info.pos.z = 0.4f * m->at.z + info.pos.z;
            if (ent->state & 0x20)
            {
                info.count = 5;
            }
            else
            {
                info.count = 2;
            }
            m = (_xMat4x3*)ent->model->Mat;
            info.vel.x = -0.2f * -m->right.x;
            info.vel.y = 0.2f;
            info.vel.z = -0.2f * -m->right.z;
            zParEmitterEmitCustom(sEmitMiniVanPuff, dt, &info);
        }
    }
    else
    {
        signed int visible;
        if (globals.cmgr == NULL)
        {
            visible = 1;
        }
        else if (!(ent->flyflags & 2))
        {
            visible = 1;
        }
        else
        {
            visible = 0;
        }
        if (!visible && ent->fx_par != NULL)
        {
            if (!(ent->flyflags & 2))
            {
                ent->flyflags |= 2;
                ((xParLike*)ent->fx_par)->pos = *(_xVec3*)&ent->model->Mat->pos;
                ((xParLike*)ent->fx_par)->pos.y += 10000.0f;
                return;
            }
        }
        else
        {
            ent->flyflags &= ~2;
        }
    }

    if (ent->fx_timer == 999.0f)
    {
        if (ent->fx_par != NULL)
        {
            ((xParLike*)ent->fx_par)->pos = *(_xVec3*)&ent->model->Mat->pos;
        }
        return;
    }

    ent->fx_timer -= dt;
    if (ent->fx_timer > 0.0f)
    {
        return;
    }

    unsigned char r = 0x42;
    unsigned char g = 0xB8;
    unsigned char b = 0xFC;
    _zParEmitterCustomSettings info;
    info.custom_flags = 0x1A0;
    info.pos = *(_xVec3*)&ent->model->Mat->pos;
    if (ent->p->pickupType == 1)
    {
        switch (ent->p->pickupIndex)
        {
        case 2:
        default:
            doGlow = 1;
            break;
        case 5:
        case 6:
        case 7:
            doGlow = 1;
            r = 0xFF;
            g = 0;
            b = 0xFF;
            break;
        case 0:
            ent->fx_timer = 999.0f;
            return;
        case 3:
        case 4:
            doGlow = 1;
            r = 0xFF;
            g = 0xFF;
            b = 0;
            break;
        case 1:
            doGlow = 1;
            r = 0xFF;
            g = 0xFF;
            b = 0;
            break;
        case 8:
        case 9:
            doGlow = 0;
            break;
        }
    }
    else if (ent->p->pickupType == 2)
    {
        doGlow = 1;
    }
    else if (ent->p->pickupType == 3 || ent->p->pickupType == 5)
    {
        if (ent->p->pickupIndex == 1)
        {
            ent->fx_timer = 0.3f * iurand() + 0.1f;
            info.pos.y += 0.1f;
            ent->fx_par = zParEmitterEmitCustom(sEmitBubble, 1.0f / 30.0f, &info);
            ent->fx_emit = sEmitBubble;
        }
        else
        {
            ent->fx_timer = 1.0f;
            ent->fx_par = zParEmitterEmitCustom(sEmitGum, 1.0f / 30.0f, &info);
            ent->fx_emit = sEmitGum;
        }
    }
    else if (ent->p->pickupType == 4)
    {
        info.custom_flags |= 0x600;
        doGlow = 1;
        info.size_death = 5.0f;
        info.size_birth = 5.0f;
    }
    else
    {
        doGlow = 1;
    }

    if (doGlow && ent->fx_timer != 0.0f && sEmitGlow != NULL)
    {
        info.color_birth[0] = r;
        info.color_birth[1] = g;
        info.color_birth[2] = b;
        info.color_death[0] = r;
        info.color_death[1] = g;
        info.color_death[2] = b;
        ent->fx_timer = 999.0f;
        ent->fx_par = zParEmitterEmitCustom(sEmitGlow, 1.0f / 30.0f, &info);
        ent->fx_emit = sEmitGlow;
        if (ent->fx_par == NULL)
        {
            ent->fx_timer = 0.0f;
            ent->fx_emit = NULL;
        }
    }
}

void zEntPickup_RenderList(_zEntPickup* plist, unsigned int pcount)
{
    for (signed int i = 0; i < (signed int)pcount; i++, plist++)
    {
        if (!xEntIsVisible(plist))
        {
            continue;
        }

        RpAtomic* model = plist->model->Data;
        RwMatrixTag* mat = plist->model->Mat;
        if (!iModelCull(model, mat))
        {
            RwFrame* frame = (RwFrame*)((RwObject*)model)->parent;
            frame->ltm = *mat;
            if (plist->fx_scale != 1.0f)
            {
                RwV3d scale;
                scale.x = plist->fx_scale;
                scale.y = plist->fx_scale;
                scale.z = plist->fx_scale;
                fn_80162B60(&frame->ltm, &scale, 1);
            }
            RwMatrixUpdate(&frame->ltm);
            (*(void (**)(RpAtomic*))((unsigned char*)model + 0x48))(model);
        }

        if (plist->state & 0x23)
        {
            _xVec3* ppos = (_xVec3*)&globals.player.ent.model->Mat->pos;
            _xVec3* pos = (_xVec3*)&plist->model->Mat->pos;
            float dx = pos->x - ppos->x;
            float dz = pos->z - ppos->z;
            float d2 = dx * dx + dz * dz;
            if (d2 < 100.0f)
            {
                signed int alpha;
                if (d2 > 64.0f)
                {
                    alpha = 0.5f * (80.0f * (10.0f - xsqrt(d2)));
                    if (alpha > 0x50)
                    {
                        alpha = 0x50;
                    }
                    if (alpha < 0)
                    {
                        alpha = 1;
                    }
                }
                else
                {
                    alpha = 0x50;
                }
                iShadowCollide(&plist->ishadow, plist, 0.667f, 1.0f);
                iShadowAdd(&plist->ishadow, alpha);
            }
        }
    }
}

void zEntPickup_GiveFX(_zEntPickup* ent)
{
    signed int killPar = 1;
    signed int type = ent->p->pickupType;
    signed int index = ent->p->pickupIndex;
    signed int fx;

    if (type == 2 || type == 4)
    {
        xSndPlay(xStrHash("Scooby_Doobie_Doo"), 0.5f, 0.5f, 0.0f, 0x7F, 0, 0);
    }

    fx = 0;
    _xMat4x3* m = (_xMat4x3*)ent->model->Mat;
    if (type == 1)
    {
        switch (index)
        {
        case 0:
        case 3:
            fx = 2;
            break;
        case 4:
            fx = 1;
            break;
        case 1:
            fx = 3;
            break;
        case 2:
            fx = 8;
            break;
        case 6:
        case 7:
            killPar = 0;
            fx = 4;
            break;
        case 5:
        case 8:
            fx = 4;
            break;
        case 9:
            fx = 4;
            break;
        }
    }
    else if (type == 2)
    {
        fx = 9;
    }
    else if (type == 3 || type == 5)
    {
        if (index == 0)
        {
            fx = 5;
        }
        else
        {
            fx = 6;
        }
    }
    else
    {
        fx = 7;
    }

    _zParEmitterCustomSettings info;
    info.custom_flags = 0x20;
    info.pos = m->pos;
    if (zGameExtraIsAnnaBday())
    {
        if (fx != 1)
        {
            _zParEmitterCustomSettings info2;
            info2.custom_flags = 0x20;
            info2.pos = *(_xVec3*)&ent->model->Mat->pos;
            zParEmitterEmitCustom(sEmitAkmalDaisy, 1.0f / 30.0f, &info2);
        }
    }
    else
    {
        switch (fx)
        {
        case 0:
        case 1:
        case 2:
            break;
        case 3:
            if (sEmitGrabMmm != NULL)
            {
                zParEmitterEmitCustom(sEmitGrabMmm, 1.0f / 30.0f, &info);
            }
            break;
        case 4:
            if (sEmitGrabQMark != NULL)
            {
                zParEmitterEmitCustom(sEmitGrabQMark, 1.0f / 30.0f, &info);
            }
            break;
        case 5:
            if (sEmitGrabGum != NULL)
            {
                zParEmitterEmitCustom(sEmitGrabGum, 1.0f / 30.0f, &info);
            }
            break;
        case 6:
            if (sEmitGrabBubbles != NULL)
            {
                zParEmitterEmitCustom(sEmitGrabBubbles, 1.0f / 30.0f, &info);
            }
            break;
        case 7:
            if (sEmitGrabGhost != NULL)
            {
                zParEmitterEmitCustom(sEmitGrabGhost, 1.0f / 30.0f, &info);
            }
            break;
        case 8:
            if (sEmitGrabSDLogo != NULL)
            {
                zParEmitterEmitCustom(sEmitGrabSDLogo, 1.0f / 30.0f, &info);
            }
            break;
        case 9:
            if (sEmitGrabPowerup != NULL)
            {
                zParEmitterEmitCustom(sEmitGrabPowerup, 1.0f / 30.0f, &info);
            }
            break;
        }
    }

    if (ent->fx_par != NULL && killPar)
    {
        xParGroupKillPar(((xParEmitterLike*)ent->fx_emit)->parSys->group, ent->fx_par);
    }
}

void enableWarpPoint(unsigned int index)
{
    zWarpPointTable[index].status = 1;
}

unsigned int zEntPickup_WarpIsOpen(unsigned int index)
{
    return zWarpPointTable[index].status;
}

void zEntPickup_UpdateFlyToInterface(_zEntPickup* ent, unsigned int pcount, float dt)
{
    _zUILike::_zUIAsset* a;
    signed int i;
    float sx;
    float sy;
    for (i = 0; i < (signed int)pcount; ent++, i++)
    {
        if ((ent->state & 4) && (ent->flyflags & 1))
        {
            zEntPickup_FlyToInterface(ent, dt);
        }
    }

    for (i = 0; i < 4; i++)
    {
        if (sKeyShake[i].shake_timer > 0.0f)
        {
            sKeyShake[i].shake_timer -= dt;
            if (sKeyShake[i].shake_timer <= 0.0f)
            {
                sKeyShake[i].shake_timer = 0.0f;
            }
            if (sKeyShake[i].ui != NULL)
            {
                a = ((_zUILike*)sKeyShake[i].ui)->uiasset;
                sKeyShake[i].shake_it = -0.85f * sKeyShake[i].shake_it;
                sx = sKeyShake[i].shake_it;
                sy = sx;
                if (irand() & 1)
                {
                    sy = -sx;
                }
                a->x = sx + sKeyShake[i].orig_pos.x;
                a->y = sy + sKeyShake[i].orig_pos.y;
            }
        }
    }
}

static inline unsigned int zEntPickup_snacksLeftInArea()
{
    signed int total = zSceneGetAreaConstTotalSnacks(globals.sceneCur->sceneID);
    return total - zSceneGetAreaSnacksCollected(globals.sceneCur->sceneID) != 0;
}

unsigned int zEntPickup_lastSnackInArea()
{
    return zEntPickup_snacksLeftInArea() == 0;
}

void zEntPickup_UnlockSnackGate(_xEnt* ent)
{
    _zEntPickup* pickup = (_zEntPickup*)ent;
    pickup->state = (pickup->state & ~0x3F) | 4;
    zEntEvent(pickup, pickup, 0x23);
    zEntPickup_hideSnackGateInfo(pickup);
    xSndPlay(xStrHash("snackgate_activate"), 0.5f, 0.5f, 0.0f, 0x7F, 0, 0);
    if (sEmitDust != NULL)
    {
        _zParEmitterCustomSettings info;
        info.custom_flags = 0x20;
        info.pos = *xBoundCenter(&pickup->bound);
        zParEmitterEmitCustom(sEmitDust, 1.0f / 30.0f, &info);
    }
    zEntEvent("MNU4 SNACKGATE UNLOCKED PROMPT", 0x51);
    zEntPlayer_Notify(5);
}

void zEntPickupCheat_allWarps()
{
    for (unsigned int i = 0; i < 26; i++)
    {
        zWarpPointTable[i].status = 1;
    }
}

void zEntPickupResetWarps()
{
    for (unsigned int i = 0; i < 26; i++)
    {
        zWarpPointTable[i].status = 0;
    }
    zWarpPointTable[10].status = 1;
}

void zEntPickup_SceneUpdate(float dt)
{
    sKeysOffTimer -= dt;
}

#pragma auto_inline on
static void zEntPickup_ShowKeyCnt(unsigned int collected, unsigned int total)
{
    char s[64];
    for (signed int i = 1; i <= (signed int)total; i++)
    {
        if (i > (signed int)collected)
        {
            sprintf(s, "KEY OFF %d UI", i);
            zEntEvent(s, 3);
            sprintf(s, "KEY ON %d UI", i);
            zEntEvent(s, 4);
        }
        else
        {
            sprintf(s, "KEY OFF %d UI", i);
            zEntEvent(s, 4);
            sprintf(s, "KEY ON %d UI", i);
            zEntEvent(s, 3);
        }
    }
    if (collected == total)
    {
        sKeysOffTimer = 1.5f;
    }
}
#pragma auto_inline reset

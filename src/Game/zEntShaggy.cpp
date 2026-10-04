#include "zGlobals.h"
#include "zScene.h"
#include "zParEmitter.h"
#include "xMath.h"
#include "xMath3.h"
#include "xString.h"
#include "xstransvc.h"
#include "xEntMotion.h"
#include "xAnim.h"
#include "iModel.h"
#include "xCollide.h"
#include "xSnd.h"

#ifndef NULL
#define NULL 0
#endif

#define SCENE_ID(s) ((((s)[0] & ~0x20) << 24) | ((s)[1] << 16) | ((s)[2] << 8) | (s)[3])

class RwMatrixTag;
class _zMovePoint;
class _zEntHangable;
class xAnimActiveEffect;
class xAnimEffect;
class iAnimSKBHeader;

class _xEntCollis
{
public:
    unsigned char chk;      // 0x0
    unsigned char pen;      // 0x1
    unsigned char pad2[0xA];
    _xCollis colls[18];     // 0xC
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
};

class _zPlatform : public _zEnt
{
public:
    void* passet;              // 0xEC
    _xEntMotion motion;        // 0xF0
    unsigned short state;      // 0x15C
    unsigned short plat_flags; // 0x15E
    float tmr;                 // 0x160
    signed int ctr;            // 0x164
    void* src;                 // 0x168
    xModelInstance* am;        // 0x16C
    xModelInstance* bm;        // 0x170
    signed int moving;         // 0x174
};

class ShaggyGuts
{
public:
    _zEnt* ent;                 // 0x0
    unsigned int shaggyType;    // 0x4
    _zEnt* swap;                // 0x8
    float oldY;                 // 0xC
    unsigned int prefIdle;      // 0x10
    unsigned int prefMove;      // 0x14
    unsigned int actionIndex;   // 0x18
    float idleTimer;            // 0x1C
    float talkTimer;            // 0x20
    unsigned int talkFlag;      // 0x24
    unsigned int talkRand;      // 0x28
    float talkSwitch;           // 0x2C
    _zEntHangable* hangme;      // 0x30
    float hangspin;             // 0x34
    float facerot;              // 0x38
    _zMovePoint* facept;        // 0x3C
    unsigned short chimneyState; // 0x40
    unsigned short chimneyRand; // 0x42
    float chimneyTimer;         // 0x44
    float chimneyHeight;        // 0x48
    float chimneyVel;           // 0x4C
    float chimneyGrav;          // 0x50
    float timeLaunch;           // 0x54
    float timeTop;              // 0x58
    float timeEvent;            // 0x5C
    _zPlatform* chimneyPlat;    // 0x60
    _xVec3 oldAt;               // 0x64
    _xVec3 oldAt2;              // 0x70
    unsigned int stopMode;      // 0x7C
    float stopParam;            // 0x80
    unsigned int stopEvent;     // 0x84
    unsigned int runEvent;      // 0x88
    _zEnt* witchFollow;         // 0x8C
    _zEnt* witchDrop;           // 0x90
    _xVec3 dropDiff;            // 0x94
    float eatTimer;             // 0xA0
    _zEnt* geronimoChase;       // 0xA4
    unsigned int barrelButted;  // 0xA8
    unsigned int barrelBounced; // 0xAC
    unsigned int barrelFellOver; // 0xB0
    float swapTimer;            // 0xB4
};

class ShaggyMagnet
{
public:
    _zEnt* shaggy;  // 0x0
    xBase* target;  // 0x4
    float radius;   // 0x8
};

class st_zEntVillain;

extern "C" {
void* memset(void* dst, int c, unsigned int n);
void* memcpy(void* dst, const void* src, unsigned int n);
int strcmp(const char* a, const char* b);
int strncmp(const char* a, const char* b, unsigned int n);
signed int fn_80015EAC(char* fmt, ...);
}

unsigned int irand();
float iurand();
signed int iModelVertCount(RpAtomic* model);
void iModelTagIndex(iModelTag* tag, RpAtomic* model, signed int index);
xModelInstance* xModelInstanceAlloc(RpAtomic* data, void* object, unsigned short flags,
                                    unsigned char boneIndex, unsigned char* boneRemap);
void xModelInstanceFree(xModelInstance* model);
void xModelInstanceAttach(xModelInstance* inst, xModelInstance* parent);
void xAnimPlaySetState(xAnimSingle* single, xAnimState* state, float startTime);
signed short fn_800C751C(signed short old, float* pitch, signed int range, float scale);
void xModelRender(xModelInstance* model);
void xEntUpdate(_xEnt* ent, _xScene* sc, float dt);

void xEntBeginUpdate(_xEnt* ent, _xScene* sc, float dt);
void xEntEndUpdate(_xEnt* ent, _xScene* sc, float dt);
enum _tagSDRumbleType
{
    SDR_None = 0
};
void zRumbleStart(signed int idx, _tagSDRumbleType type);
xAnimState* xAnimTableGetState(xAnimTable* table, char* name);
void xAnimStateNewEffect(xAnimState* state, unsigned int flags, float startTime, float endTime,
                         unsigned int (*callback)(unsigned int, xAnimActiveEffect*, xAnimSingle*,
                                                  void*),
                         unsigned int userDataSize);
unsigned int Shaggy8_BarrelDestroy(unsigned int, xAnimActiveEffect*, xAnimSingle*, void* object);
unsigned int Shaggy8_Getup(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy8_BounceCB(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy8_Bounce(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy8_Crash(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy8_Stopped(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy8_FlipBackCB(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy8_FlipBack(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy8_FlipOverCB(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy8_FlipOver(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Witch_DropShaggyCB(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Witch_GrabShaggyCB(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Witch_DropShaggyIdle(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Witch_DropShaggyMove(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Witch_HasShaggy(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy5_GargRunPick(xAnimTransition* tran, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy5_EatTimeUp(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy5_EatStopCB(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy5_EatStop(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy5_EatMoveCB(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy5_Action(xAnimTransition* tran, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy5_WitchDropped(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy5_WitchAttached(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy5_IdleTran(xAnimTransition* tran, xAnimSingle*, _zEnt* ent);
void xAnimTableAddTransition(xAnimTable* table, xAnimTransition* tran, char* source);
unsigned int Shaggy5_MoveTran(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy5_ActionCB(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy1_IdlePick(xAnimTransition*, xAnimSingle*, _zEnt* ent);
unsigned int Shaggy1_IdleResetCB(xAnimTransition*, xAnimSingle*, _zEnt* ent);

unsigned int Shaggy3_Center(xAnimTransition* tran, xAnimSingle* single, _zPlatform* ent);
unsigned int Shaggy3_Right(xAnimTransition* tran, xAnimSingle* single, _zPlatform* ent);
unsigned int Shaggy3_Left(xAnimTransition* tran, xAnimSingle* single, _zPlatform* ent);
unsigned int Shaggy3_StopScare(xAnimTransition* tran, xAnimSingle* single, _zPlatform* ent);
unsigned int Shaggy3_Stop(xAnimTransition* tran, xAnimSingle* single, _zPlatform* ent);
unsigned int Shaggy3_MoveDead(xAnimTransition* tran, xAnimSingle* single, _zPlatform* ent);
unsigned int Shaggy3_Move(xAnimTransition* tran, xAnimSingle* single, _zPlatform* ent);
unsigned int Shaggy0_CarryHitCB(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy0_PlayerCaught(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy0_FlipCB(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy0_Throw(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_NotalkCB(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_Notalk(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_FallDeathCB(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_FallDeath(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_DeathReact(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_ScareReact(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_Noturn(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_TurnCB(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_Turn(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_EndSetDownCB(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_PlyrSet(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_SetDown(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_EventCB(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_IdleLeave(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_ScareCB(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_Scare(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_LetGoCB(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_LetGoShark(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_LetGo(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_HangNoSwing(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_HangSwing(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_Hang(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_FallCB(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_Fall(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_CatchCB(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_Catch(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_LandCB(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_ThrowEndCB(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_ThrowCB(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_Stop(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_Walk(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);

signed int zPlatformEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                            xBase* toParamWidget);
void zPlatform_Update(_xEnt* ent, _xScene* sc, float dt);
unsigned int Shaggy3_MowerStopCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void* object);

unsigned int Shaggy2_RandCheck(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy2_Apex(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy2_StateChange(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy0_PlayerLanded(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy0_CatchCB(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy0_Catch(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_TalkChooseCB(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_TalkChoose(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_TalkFlag(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
unsigned int Shaggy1_Talk(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);

typedef unsigned int (*ShaggyTranCB)(xAnimTransition*, xAnimSingle*, void*);
#define xAnimTableNewTransition(t, s, d, c, cb, f, u, st, dt, p, q, b, o) \
    xAnimTableNewTransition(t, s, d, (ShaggyTranCB)(c), (ShaggyTranCB)(cb), f, u, st, dt, p, q, b, o)
void Shaggy1_BonusAnims(_zEnt* ent);
signed int Shaggy2Plat_EventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                               xBase* toParamWidget);
signed int Shaggy2_EventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                           xBase* toParamWidget);
void Shaggy2_Render(_xEnt* ent);
void Shaggy2_Update(_zEnt* ent, _xScene* sc, float dt);
signed int Shaggy3_EventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                           xBase* toParamWidget);
void Shaggy3_Update(_zEnt* ent, _xScene* sc, float dt);
signed int Shaggy5_EventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                           xBase* toParamWidget);
void Shaggy5_Update(_zEnt* ent, _xScene* sc, float dt);
void Shaggy5_Move(_xEnt* ent, _xScene* s, float dt, _xEntFrame* frame);
signed int Shaggy8_EventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                           xBase* toParamWidget);
void Shaggy8_Update(_zEnt* ent, _xScene* sc, float dt);
signed int Shaggy0_EventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                           xBase* toParamWidget);
signed int Shaggy1_EventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                           xBase* toParamWidget);
void Shaggy1_Update(_zEnt* ent, _xScene* sc, float dt);
void Shaggy1_Move(_xEnt* ent, _xScene* s, float dt, _xEntFrame* frame);
void ShaggySwap(_zEnt* src, _zEnt* dest);

unsigned int gNumShaggy;
ShaggyGuts gShaggy[16];
static float gShaggy3TurnAngle;
static _zParEmitter* gTractorEmitter;
static _zParEmitter* gBurningManEmitter;
static signed int (*sVilEventFunc)(xBase*, xBase*, unsigned int, float*, xBase*);
static void (*sVilUpdateFunc)(_xEnt*, _xScene*, float);
static void (*sVilMoveFunc)(_xEnt*, _xScene*, float, _xEntFrame*);
float gACCEL_START;
float gACCEL_END;
float THROW_MINHEIGHT = 1.0f;
float THROW_DEFHEIGHT = 2.0f;
float sHangVirtualPeak;
float sDefHangTime;
static unsigned int ChimneyTagIndex = 0;
static iModelTag ChimneyTag;
static unsigned int ShaggyTagIndex = 0;
static iModelTag ShaggyTag;
static signed int sMagnetCount;
static ShaggyMagnet sMagnetList[64];
static signed short sSndCallPitch;

static inline ShaggyGuts* ShaggyGetGuts(_zEnt* ent)
{
    for (unsigned int i = 0; i < gNumShaggy; i++)
    {
        if (gShaggy[i].ent == ent)
        {
            return &gShaggy[i];
        }
    }
    return NULL;
}

static inline unsigned int ShaggyIsShaggy(_zEnt* ent)
{
    return ent->baseType == 2 && ent->subType >= 0x1C && ent->subType <= 0x25;
}

static inline unsigned int ShaggyHasTable(xModelInstance* minst, char* name)
{
    while (minst != NULL)
    {
        if (minst->Anim != NULL && xStricmp(minst->Anim->Table->Name, name) == 0)
        {
            return 1;
        }
        minst = minst->Next;
    }
    return 0;
}

static inline void ShaggyVil_HandleVisible(_zEnt* ent, unsigned int toEvent)
{
    ShaggyGuts* guts = ShaggyGetGuts(ent);
    _xMat4x3* mat = (_xMat4x3*)ent->model->Mat;

    switch (toEvent)
    {
    case 4:
    case 0x56:
        if (guts->oldY == 1e10f)
        {
            guts->oldY = mat->pos.y;
            ((_xMat4x3*)ent->model->Mat)->pos.y = 1e10f;
            ent->frame->mat.pos.y = ((_xMat4x3*)ent->model->Mat)->pos.y;
            ent->frame->oldpos.y = ((_xMat4x3*)ent->model->Mat)->pos.y;
            ent->bound.sph.center.y =
                ((_xMat4x3*)ent->model->Mat)->pos.y + ent->bound.sph.r;
        }
        ent->flags &= ~1;
        break;
    case 3:
    case 0x55:
        if (guts->oldY != 1e10f)
        {
            ((_xMat4x3*)ent->model->Mat)->pos.y = guts->oldY;
            guts->oldY = 1e10f;
            ent->frame->mat.pos.y = ((_xMat4x3*)ent->model->Mat)->pos.y;
            ent->frame->oldpos.y = ((_xMat4x3*)ent->model->Mat)->pos.y;
            ent->bound.sph.center.y =
                ((_xMat4x3*)ent->model->Mat)->pos.y + ent->bound.sph.r;
        }
        ent->flags |= 1;
        break;
    }
}

// ---- DOL order: zEntShaggy_SceneReset ----

void zEntShaggy_SceneReset()
{
    for (unsigned int i = 0; i < gNumShaggy; i++)
    {
        ShaggyGuts* guts = &gShaggy[i];
        unsigned int isGeronimo = 0;

        guts->hangme = NULL;
        guts->hangspin = 0.0f;
        guts->chimneyState = 0;
        _xMat4x3* mat = (_xMat4x3*)guts->ent->model->Mat;
        guts->oldAt.x = mat->at.x;
        guts->oldAt.y = mat->at.y;
        guts->oldAt.z = mat->at.z;
        guts->oldAt2.x = guts->oldAt.x;
        guts->oldAt2.y = guts->oldAt.y;
        guts->oldAt2.z = guts->oldAt.z;
        guts->witchFollow = NULL;
        guts->witchDrop = NULL;
        guts->barrelButted = 0;
        guts->barrelBounced = 0;
        guts->swap = NULL;
        guts->actionIndex = 0;
        guts->eatTimer = 0.0f;
        guts->facerot = 0.0f;
        guts->facept = NULL;
        guts->talkTimer = 0.0f;
        guts->talkSwitch = -1.0f;
        guts->stopMode = 1;
        guts->stopParam = 5.0f;
        guts->stopEvent = 0;
        guts->runEvent = 0;

        if (globals.sceneCur->sceneID == SCENE_ID("o006") &&
            guts->ent->id == xStrHash("SHAGGY5"))
        {
            isGeronimo = 1;
        }
        guts->geronimoChase = isGeronimo ? (_zEnt*)zSceneFindObject(xStrHash("GERONIMO")) : NULL;

        if (guts->ent->baseType == 2)
        {
            guts->ent->pflags |= 4;
        }

        if (guts->ent->baseType == 6 && ShaggyHasTable(guts->ent->model, "ShaggyChimney"))
        {
            xModelInstance* minst = guts->ent->model->Next;
            if (minst != NULL)
            {
                minst = minst->Next;
                if (minst != NULL)
                {
                    ((_xMat4x3*)minst->Mat)[minst->BoneCount + 1].pos.y = -0.6f;
                    minst->Flags &= ~7;
                }
            }
        }

        if (guts->ent->baseType == 2 && guts->ent->subType == 0x24)
        {
            xModelInstance* minst = guts->ent->model->Next;
            if (minst != NULL)
            {
                minst->Flags |= 3;
            }
        }

        if (guts->ent->baseType == 6 && ShaggyHasTable(guts->ent->model, "Shaggy3"))
        {
            xAnimPlay* play = guts->ent->model->Anim;
            xAnimPlaySetState(play->Single, play->Table->StateList, 0.0f);
        }
    }

    sMagnetCount = 0;

    for (signed int i = 0; i < (signed int)gNumShaggy; i++)
    {
        _zEnt* ent = gShaggy[i].ent;
        if (ent->baseType == 2)
        {
            ShaggyVil_HandleVisible(ent, (ent->asset->flags & 1) ? 3 : 4);
        }
    }
}

void zEntShaggy_SceneStart(zScene* sc)
{
    sSndCallPitch = 359.0f * iurand();
    gTractorEmitter = zParEmitterFind("THORN CLIPPINGS EMITTER");
    gBurningManEmitter = zParEmitterFind("SHAGGY BURNNING EMITTER");
    gNumShaggy = 0;
    memset(gShaggy, 0, sizeof(gShaggy));

    unsigned int j = 0;
    unsigned int hangHashID = xStrHash("SHAGGY_HANG0");
    memset(globals.player.Shaggy1.Hangable, 0, sizeof(globals.player.Shaggy1.Hangable));

    for (unsigned int i = 0; i < sc->num_ents; i++)
    {
        if (ShaggyIsShaggy(sc->ents[i]))
        {
            _zEnt* ent = sc->ents[i];
            switch (ent->subType)
            {
            case 0x1C:
            case 0x1D:
                Shaggy1_BonusAnims(ent);
                break;
            case 0x24:
            {
                xModelInstance* inst = ent->model;
                xModelInstance* inst2 = inst->Next;
                if (inst != NULL && inst2 != NULL)
                {
                    if (iModelVertCount(inst2->Data) > iModelVertCount(inst->Data))
                    {
                        RpAtomic* data1 = inst->Data;
                        inst->Data = inst2->Data;
                        inst2->Data = data1;
                    }
                }
                break;
            }
            }

            ShaggyGuts* guts = &gShaggy[gNumShaggy];
            guts->ent = sc->ents[i];
            guts->shaggyType = sc->ents[i]->subType - 0x1C;
            guts->hangme = NULL;
            _xMat4x3* mat = (_xMat4x3*)sc->ents[i]->model->Mat;
            guts->oldAt = mat->at;
            gShaggy[gNumShaggy].oldAt2 = gShaggy[gNumShaggy].oldAt;
            gShaggy[gNumShaggy].oldY = 1e10f;
            gShaggy[gNumShaggy].talkTimer = 0.0f;
            gShaggy[gNumShaggy].talkSwitch = -1.0f;
            gShaggy[gNumShaggy].talkRand = irand() >> 16;

            unsigned int isGeronimo = 0;
            if (globals.sceneCur->sceneID == SCENE_ID("o006") &&
                gShaggy[gNumShaggy].ent->id == xStrHash("SHAGGY5"))
            {
                isGeronimo = 1;
            }
            gShaggy[gNumShaggy++].geronimoChase =
                isGeronimo ? (_zEnt*)zSceneFindObject(xStrHash("GERONIMO")) : NULL;
        }

        if (sc->ents[i]->baseType == 0x17)
        {
            unsigned int id = sc->ents[i]->id;
            if (id >= hangHashID && id <= hangHashID + 9)
            {
                globals.player.Shaggy1.Hangable[j++] = sc->ents[i];
            }
        }

        if (sc->ents[i]->baseType == 6)
        {
            xAnimPlay* play = sc->ents[i]->model->Anim;
            if (play != NULL && strcmp(play->Table->Name, "Shaggy3") == 0)
            {
                ShaggyGuts* guts = &gShaggy[gNumShaggy];
                guts->ent = sc->ents[i];
                guts->shaggyType = 3;
                guts->hangme = NULL;
                _xMat4x3* mat = (_xMat4x3*)sc->ents[i]->model->Mat;
                guts->oldAt = mat->at;
                gNumShaggy++;
                guts->oldAt2 = guts->oldAt;
                guts->oldY = 1e10f;
                guts->stopMode = 1;
                guts->stopParam = 5.0f;
                guts->stopEvent = 0;
                guts->runEvent = 0;
                sc->ents[i]->update = (void (*)(_xEnt*, _xScene*, float))Shaggy3_Update;
                sc->ents[i]->eventFunc = Shaggy3_EventCB;
            }
        }

        if (sc->ents[i]->baseType == 6 && ShaggyHasTable(sc->ents[i]->model, "ShaggyChimney"))
        {
            xModelInstance* minst = sc->ents[i]->model;
            minst->Flags = 0;
            xModelInstance* inst = minst->Next;
            inst->Flags = (inst->Flags & ~8) | 0x20;
            xModelInstance* tmpinst = inst->Next;
            if (tmpinst != NULL)
            {
                xModelInstance* newminst = xModelInstanceAlloc(
                    tmpinst->Data, tmpinst->Object, (tmpinst->Flags & ~8) | 0x60, 0, NULL);
                newminst->Anim = inst->Next->Anim;
                *(_xMat4x3*)newminst->Mat = *(_xMat4x3*)inst->Next->Mat;
                newminst->Anim->Object = newminst;
                inst->Next->Anim = NULL;
                xModelInstanceFree(inst->Next);
                xModelInstanceAttach(newminst, inst);
                ((_xMat4x3*)newminst->Mat)[newminst->BoneCount + 1].pos.y = -0.6f;
                newminst->Flags &= ~7;
                iModelTagIndex(&ChimneyTag, inst->Data, ChimneyTagIndex);
                iModelTagIndex(&ShaggyTag, inst->Next->Data, ShaggyTagIndex);
            }

            ShaggyGuts* guts = &gShaggy[gNumShaggy];
            guts->ent = sc->ents[i];
            guts->shaggyType = 2;
            guts->chimneyState = 0;
            guts->chimneyTimer = 0.0f;
            sc->ents[i]->update = (void (*)(_xEnt*, _xScene*, float))Shaggy2_Update;
            sc->ents[i]->render = Shaggy2_Render;
            sc->ents[i]->eventFunc = Shaggy2_EventCB;
            sc->ents[i]->chkby |= 0x10;
            sc->ents[i]->penby |= 0x10;

            _zPlatform* plat = (_zPlatform*)sc->ents[i];
            guts = &gShaggy[gNumShaggy];
            guts->chimneyHeight = *(float*)((char*)plat->motion.asset + 0x14);
            guts->timeLaunch = *(float*)((char*)plat->motion.asset + 0x28);
            guts->timeTop = *(float*)((char*)plat->motion.asset + 0x20);
            guts->chimneyGrav = *(float*)((char*)plat->motion.asset + 0x1C);
            guts->timeEvent = 0.0f;
            guts->oldY = 1e10f;
            if (guts->chimneyHeight == 0.0f)
            {
                guts->chimneyHeight = 4.0f;
            }
            if (guts->timeLaunch == 0.0f)
            {
                guts->timeLaunch = 3.0f;
            }
            guts->chimneyPlat = NULL;

            for (unsigned int k = 0; k < sc->num_ents; k++)
            {
                if (i != k && sc->ents[k]->baseType == 6)
                {
                    _xVec3* a = (_xVec3*)((char*)sc->ents[k]->asset + 0x1C);
                    _xVec3* b = (_xVec3*)((char*)sc->ents[i]->asset + 0x1C);
                    float dx = a->y - b->y;
                    float dy = a->x - b->x;
                    float dz = a->z - b->z;
                    if (xsqrt(dz * dz + (dy * dy + dx * dx)) < 0.1f)
                    {
                        gShaggy[gNumShaggy].chimneyPlat = (_zPlatform*)sc->ents[k];
                        sc->ents[k]->flags &= ~1;
                        sc->ents[k]->model->Flags = 0;
                        sc->ents[k]->eventFunc = Shaggy2Plat_EventCB;
                        break;
                    }
                }
            }
            gNumShaggy++;
        }

        if (sc->ents[i]->baseType == 0xB && ShaggyHasTable(sc->ents[i]->model, "ShaggyChimney"))
        {
            for (xModelInstance* minst = sc->ents[i]->model; minst != NULL; minst = minst->Next)
            {
                minst->Flags = 0;
            }
        }
    }

    for (signed int i = 0; i < (signed int)gNumShaggy; i++)
    {
        _zEnt* ent = gShaggy[i].ent;
        if (ent->baseType == 2)
        {
            ShaggyVil_HandleVisible(ent, (ent->asset->flags & 1) ? 3 : 4);
        }
    }
}

void zEntShaggy_Init(st_zEntVillain* vil)
{
    _zEnt* shaggy = (_zEnt*)vil;

    for (unsigned int i = 0; i < 16; i++)
    {
        gShaggy[i].hangme = NULL;
        gShaggy[i].hangspin = 0.0f;
    }

    if (sVilUpdateFunc == NULL)
    {
        sVilUpdateFunc = shaggy->update;
    }
    if (sVilMoveFunc == NULL)
    {
        sVilMoveFunc = shaggy->move;
    }
    if (sVilEventFunc == NULL)
    {
        sVilEventFunc = shaggy->eventFunc;
    }

    switch (shaggy->subType)
    {
    case 0x1C:
    case 0x1D:
        shaggy->update = (void (*)(_xEnt*, _xScene*, float))Shaggy1_Update;
        shaggy->move = Shaggy1_Move;
        if (shaggy->subType == 0x1C)
        {
            shaggy->eventFunc = Shaggy0_EventCB;
        }
        if (shaggy->subType == 0x1D)
        {
            shaggy->eventFunc = Shaggy1_EventCB;
        }
        break;
    case 0x21:
        shaggy->update = (void (*)(_xEnt*, _xScene*, float))Shaggy5_Update;
        shaggy->move = Shaggy5_Move;
        shaggy->eventFunc = Shaggy5_EventCB;
        break;
    case 0x24:
        shaggy->update = (void (*)(_xEnt*, _xScene*, float))Shaggy8_Update;
        shaggy->eventFunc = Shaggy8_EventCB;
        break;
    }

    shaggy->collis->chk = 0x26;
    shaggy->collis->pen = 0x66;
    shaggy->chkby = 0x1C;
    shaggy->penby = 0x1C;
}

void zEntShaggy_UpdateDep()
{
    for (signed int i = 0; i < (signed int)gNumShaggy; i++)
    {
        if (gShaggy[i].witchFollow != NULL)
        {
            *(_xMat4x3*)gShaggy[i].ent->model->Mat = *(_xMat4x3*)gShaggy[i].witchFollow->model->Mat;
        }
    }
}

signed int Shaggy8_CallToPlayer()
{
    _zEnt* shaggy;
    _xVec3* pos;

    if (globals.player.Shaggy8CallTimer > 0.0f || (shaggy = globals.player.Shaggy8Call) == NULL ||
        !(shaggy->baseFlags & 1))
    {
        return 0;
    }

    xModelInstance* minst = shaggy->model;
    if (!(minst->Anim->Single->State->UserFlags & 0x10000))
    {
        return 0;
    }

    zPlayerGlobals* p = &globals.player;
    _xMat4x3* smat = (_xMat4x3*)minst->Mat;
    _xMat4x3* pmat = (_xMat4x3*)globals.player.ent.model->Mat;
    float dz = pmat->pos.z - smat->pos.z;
    float dx = pmat->pos.x - smat->pos.x;
    float dist = xsqrt(dx * dx + dz * dz);
    if (dist < 0.1f)
    {
        return 0;
    }
    float inv = 1.0f / dist;
    dx *= inv;
    dz *= inv;

    float pitch;
    sSndCallPitch = fn_800C751C(sSndCallPitch, &pitch, 30, 0.15f);
    unsigned int snd = 0x0176632D - ((irand() >> 16) & 1);
    if (xEntIsVisible(shaggy))
    {
        xSndPlay(snd, 0.5f, 0.5f, pitch, 0x80, 2, 0);
    }

    float best = 1000000.0f;
    signed int bestIdx = -1;
    for (signed int i = 0; i < sMagnetCount; i++)
    {
        ShaggyMagnet* mag = &sMagnetList[i];
        if (mag->shaggy == shaggy)
        {
            xBase* tgt = mag->target;
            switch (tgt->baseType)
            {
            case 13:
                pos = (_xVec3*)((char*)tgt + 0x14);
                break;
            case 2:
            case 3:
            case 4:
            case 6:
            case 8:
            case 11:
            case 12:
            case 18:
            case 23:
            case 24:
            case 27:
                pos = &((_xMat4x3*)((_xEnt*)tgt)->model->Mat)->pos;
                break;
            }

            float r = shaggy->bound.sph.r;
            _xMat4x3* m = (_xMat4x3*)shaggy->model->Mat;
            float tz = pos->z - m->pos.z;
            float tx = pos->x - m->pos.x;
            if (!(tx * tx + tz * tz < r * r))
            {
                float along = tx * dx + tz * dz;
                if (along >= r && along < best && __fabs(tx * dz - tz * dx) < mag->radius)
                {
                    bestIdx = i;
                    best = along;
                }
            }
        }
    }

    globals.player.Shaggy8CallTimer = 1.3333f;
    float param[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (bestIdx != -1)
    {
        zEntEvent(NULL, 0, shaggy, 0xCD, param, sMagnetList[bestIdx].target, 0);
    }
    else
    {
        float oldx = ((_xMat4x3*)p->ent.model->Mat)->pos.x;
        float oldz = ((_xMat4x3*)p->ent.model->Mat)->pos.z;
        ((_xMat4x3*)p->ent.model->Mat)->pos.x = 20.0f * dx + oldx;
        ((_xMat4x3*)p->ent.model->Mat)->pos.z = 20.0f * dz + ((_xMat4x3*)p->ent.model->Mat)->pos.z;
        zEntEvent(NULL, 0, shaggy, 0xCD, param, &p->ent, 0);
        ((_xMat4x3*)p->ent.model->Mat)->pos.x = oldx;
        ((_xMat4x3*)p->ent.model->Mat)->pos.z = oldz;
    }
    return 1;
}

signed int Shaggy8_EventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                           xBase* toParamWidget)
{
    _zEnt* ent = (_zEnt*)to;
    ShaggyGuts* guts = ShaggyGetGuts(ent);
    signed int ret = 0;

    switch (toEvent)
    {
    case 3:
    case 4:
    case 0x55:
    case 0x56:
        ShaggyVil_HandleVisible(ent, toEvent);
        break;
    case 0xD1:
        if (toParamWidget != NULL && toParamWidget->baseType == 2)
        {
            ShaggySwap(ent, (_zEnt*)toParamWidget);
        }
        break;
    case 0xD2:
        guts->prefIdle = toParam[0];
        guts->prefMove = toParam[1];
        break;
    case 0xD3:
        guts->actionIndex = 1;
        if (toParamWidget != NULL && toParamWidget->baseType == 2 &&
            ShaggyGetGuts((_zEnt*)toParamWidget) != NULL)
        {
            guts->swap = (_zEnt*)toParamWidget;
            guts->swapTimer = 0.0f;
            ((_xMat4x3*)guts->swap->model->Mat)->pos.y = 1000000.0f;
            zEntEvent(ent, toParamWidget, 3);
        }
        break;
    case 0xDE:
        globals.player.Shaggy8Call = ent;
        globals.player.Shaggy8CallTimer = toParam[0];
        break;
    case 0xDF:
        globals.player.Shaggy8Call = NULL;
        break;
    case 0xE0:
        if (toParamWidget != NULL)
        {
            signed int i;
            for (i = 0; i < sMagnetCount; i++)
            {
                if (sMagnetList[i].shaggy == ent && sMagnetList[i].target == toParamWidget)
                {
                    sMagnetList[i].radius = toParam[0];
                    break;
                }
            }
            if (i == sMagnetCount)
            {
                sMagnetList[sMagnetCount].shaggy = ent;
                sMagnetList[sMagnetCount].target = toParamWidget;
                sMagnetList[sMagnetCount].radius = toParam[0];
                sMagnetCount++;
            }
        }
        break;
    case 0xE1:
        if (toParamWidget != NULL)
        {
            for (signed int i = 0; i < sMagnetCount; i++)
            {
                if (sMagnetList[i].shaggy == ent && sMagnetList[i].target == toParamWidget)
                {
                    sMagnetCount--;
                    sMagnetList[i] = sMagnetList[sMagnetCount];
                    break;
                }
            }
        }
        break;
    default:
        ret = sVilEventFunc(from, to, toEvent, toParam, toParamWidget);
        break;
    }
    return ret;
}

void Shaggy8_Update(_zEnt* ent, _xScene* sc, float dt)
{
    if (ent == globals.player.Shaggy8Call)
    {
        globals.player.Shaggy8CallTimer -= dt;
    }

    if (xEntIsVisible(ent))
    {
        xAnimSingle* single = ent->model->Anim->Single;
        xAnimState* blendState = single->Blend != NULL ? single->Blend->State : NULL;
        ShaggyGuts* guts = ShaggyGetGuts(ent);

        if (guts->swap == NULL)
        {
            guts->idleTimer += dt;
        }
        if (guts->swap != NULL)
        {
            guts->swapTimer += dt;
        }

        if (single->State->UserFlags == 0 || (blendState != NULL && blendState->UserFlags == 0))
        {
            xEntBeginUpdate(ent, sc, dt);
            xEntEndUpdate(ent, sc, dt);
        }

        sVilUpdateFunc(ent, sc, dt);

        for (signed int i = 6; i < 18; i++)
        {
            if (!(ent->frame->mode & 0x20) || __fabs(ent->frame->drot.angle) < 0.01308997f)
            {
                _xCollis* coll = &ent->collis->colls[i];
                if ((coll->flags & 1) && !(coll->flags & 0xC) && coll->optr == NULL)
                {
                    if (coll->depen.x * coll->depen.x + coll->depen.z * coll->depen.z > 0.001f)
                    {
                        guts->barrelFellOver = 1;
                        zEntEvent(ent, 0x29);
                    }
                }
            }
        }
    }
}

signed int Shaggy8_BBCallback(_xCollis* coll)
{
    _zEnt* ent = (_zEnt*)coll->optr;
    if (ent->baseType == 2 && ent->subType == 0x24)
    {
        char* name = ent->model->Anim->Single->State->Name;
        if (strcmp(name, "Stand_idle") == 0 || strcmp(name, "Stand_walk") == 0 ||
            strcmp(name, "Stand_run") == 0)
        {
            ShaggyGuts* guts = ShaggyGetGuts(ent);
            if (guts == NULL)
            {
                return 0;
            }
            guts->barrelBounced = 1;
            return 1;
        }
    }
    return 0;
}

signed int Shaggy8_HBCallback(_zEnt* plyr, _xCollis* coll)
{
    _zEnt* ent = (_zEnt*)coll->optr;
    if (ent->baseType == 2 && ent->subType == 0x24 &&
        strncmp(ent->model->Anim->Single->State->Name, "Idle", 4) == 0)
    {
        ShaggyGuts* guts = ShaggyGetGuts(ent);
        if (guts == NULL)
        {
            return 0;
        }
        guts->barrelButted = 1;
        plyr->frame->mat.pos = ((_xMat4x3*)plyr->model->Mat)->pos;
        globals.player.HeadbuttHit = 1;
        globals.player.DecelRunSpeed = 0.25f * globals.player.DecelRunSpeed;
        zRumbleStart(0, (_tagSDRumbleType)1);
        return 1;
    }
    return 0;
}

xAnimTable* zEntShaggy_Anim8()
{
    xAnimTable* table;

    table = xAnimTableNew("Shaggy8", NULL);
    xAnimTableNewState(table, "Idle", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle2", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle3", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Stand_idle", 0x10, 0x10001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Stand_walk", 0x10, 0x02010004, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Stand_run", 0x10, 0x02010006, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Stand_bounce", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Crash", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Sit", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GetUp", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "NoBarrelIdle", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "FlipOver", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "FlipBack", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewTransition(table, "Crash", "Sit", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "GetUp", "NoBarrelIdle", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Idle Idle2 Idle3", "FlipOver", Shaggy8_FlipOver, Shaggy8_FlipOverCB, 0, 0, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Stand_idle Stand_walk Stand_run", "FlipBack", Shaggy8_FlipBack, Shaggy8_FlipBackCB, 0, 0, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Stand_idle Stand_run", "Stand_walk", Shaggy5_MoveTran, NULL, 0, 0, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Stand_idle Stand_run", "Stand_walk", Shaggy5_MoveTran, NULL, 0, 3, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Stand_idle Stand_walk", "Stand_run", Shaggy5_MoveTran, NULL, 0, 1, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Stand_walk Stand_run", "Stand_idle", Shaggy8_Stopped, NULL, 0, 0, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Stand_idle Stand_walk Stand_run", "Crash", Shaggy8_Crash, Shaggy5_ActionCB, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Sit", "GetUp", Shaggy8_Getup, NULL, 0, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Stand_idle Stand_walk Stand_run Stand_bounce", "Stand_bounce", Shaggy8_Bounce, Shaggy8_BounceCB, 0, 0, 0.0f, 0.0f, 5, 0, 0.08f, NULL);
    xAnimTableNewTransition(table, "Stand_bounce", "Stand_walk", Shaggy5_MoveTran, NULL, 0x10, 0, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Stand_bounce", "Stand_walk", Shaggy5_MoveTran, NULL, 0x10, 3, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Stand_bounce", "Stand_run", Shaggy5_MoveTran, NULL, 0x10, 1, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Stand_bounce", "Stand_idle", Shaggy8_Stopped, NULL, 0x10, 0, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "FlipOver", "Stand_idle", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "FlipBack", "Idle", NULL, Shaggy1_IdleResetCB, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Idle", "Idle2", Shaggy1_IdlePick, NULL, 0, 4, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle2", "Idle", Shaggy1_IdlePick, NULL, 0, 7, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle", "Idle2", Shaggy1_IdlePick, NULL, 0, 0xC, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle2", "Idle", Shaggy1_IdlePick, NULL, 0, 0xE, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle", "Idle3", Shaggy1_IdlePick, NULL, 0, 0x11, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle3", "Idle", Shaggy1_IdlePick, Shaggy1_IdleResetCB, 0, 0x13, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimStateNewEffect(xAnimTableGetState(table, "Crash"), 1, 0.2f, 0.0f, Shaggy8_BarrelDestroy, 0);
    return table;
}

unsigned int Shaggy8_BarrelDestroy(unsigned int, xAnimActiveEffect*, xAnimSingle*, void* object)
{
    ShaggyGetGuts((_zEnt*)object);
    xModelInstance* minst = ((_zEnt*)object)->model->Next;
    if (minst != NULL)
    {
        minst->Flags &= ~3;
    }
    return 0;
}

unsigned int Shaggy8_Getup(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    ShaggyGuts* guts = ShaggyGetGuts(ent);
    return guts->swapTimer >= 3.0f;
}

unsigned int Shaggy8_BounceCB(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    ShaggyGetGuts(ent)->barrelBounced = 0;
    return 0;
}

unsigned int Shaggy8_Bounce(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    return ShaggyGetGuts(ent)->barrelBounced != 0;
}

unsigned int Shaggy8_Crash(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    return ShaggyGetGuts(ent)->actionIndex != 0;
}

unsigned int Shaggy8_Stopped(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    signed int goal = *(signed int*)((char*)ent + 0x114);
    return goal != 6 && goal != 8;
}

unsigned int Shaggy8_FlipBackCB(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    ShaggyGetGuts(ent)->barrelFellOver = 0;
    return 0;
}

unsigned int Shaggy8_FlipBack(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    ShaggyGuts* guts = ShaggyGetGuts(ent);
    return guts != NULL && guts->barrelFellOver != 0;
}

unsigned int Shaggy8_FlipOverCB(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    ShaggyGetGuts(ent)->barrelButted = 0;
    return 0;
}

unsigned int Shaggy8_FlipOver(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    ShaggyGuts* guts = ShaggyGetGuts(ent);
    return guts != NULL && guts->barrelButted != 0;
}


signed int Shaggy5_EventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                           xBase* toParamWidget)
{
    _zEnt* ent = (_zEnt*)to;
    ShaggyGuts* guts = ShaggyGetGuts(ent);
    signed int ret = 0;

    switch (toEvent)
    {
    case 3:
    case 4:
    case 0x55:
    case 0x56:
        ShaggyVil_HandleVisible(ent, toEvent);
        break;
    case 0xCF:
        if (toParamWidget != NULL && toParamWidget->baseType == 2)
        {
            guts->witchFollow = (_zEnt*)toParamWidget;
            ent->collis->chk = 0;
            ent->chkby = 0;
            ent->frame->vel.y = 0.0f;
        }
        ret = 1;
        break;
    case 0xD0:
        if (toParamWidget != NULL && toParamWidget->baseType == 6 && guts->witchFollow != NULL)
        {
            guts->witchFollow = NULL;
            guts->witchDrop = (_zEnt*)toParamWidget;
            ent->frame->vel.y = 0.0f;
            guts->dropDiff.x = ((_xMat4x3*)guts->witchDrop->model->Mat)->pos.x -
                               ((_xMat4x3*)ent->model->Mat)->pos.x;
            guts->dropDiff.y = ((_xMat4x3*)guts->witchDrop->model->Mat)->pos.y -
                               ((_xMat4x3*)ent->model->Mat)->pos.y;
            guts->dropDiff.z = ((_xMat4x3*)guts->witchDrop->model->Mat)->pos.z -
                               ((_xMat4x3*)ent->model->Mat)->pos.z;
        }
        ret = 1;
        break;
    case 0xD1:
        if (toParamWidget != NULL && toParamWidget->baseType == 2)
        {
            ShaggySwap(ent, (_zEnt*)toParamWidget);
        }
        break;
    case 0xD2:
        guts->prefIdle = toParam[0];
        guts->prefMove = toParam[1];
        if (guts->prefIdle > 3)
        {
            guts->prefIdle = 3;
        }
        if (guts->prefMove > 3)
        {
            guts->prefMove = 3;
        }
        break;
    case 0xD3:
        guts->actionIndex = toParam[0];
        if (guts->actionIndex == 1 && toParamWidget != NULL)
        {
            float param[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            ret = sVilEventFunc(from, to, 0xCD, param, toParamWidget);
        }
        break;
    default:
        ret = sVilEventFunc(from, to, toEvent, toParam, toParamWidget);
        break;
    }
    return ret;
}

void ShaggySwap(_zEnt* src, _zEnt* dest)
{
    ShaggyGuts* sguts = ShaggyGetGuts(src);
    ShaggyGuts* dguts = ShaggyGetGuts(dest);

    if (sguts->swap == NULL)
    {
        sguts->swap = dest;
    }

    if (!xEntIsVisible(src) || src->model->Anim->Single->Blend->State != NULL)
    {
        return;
    }

    char* idle = "Idle";
    char* name = src->model->Anim->Single->State->Name;
    if (sguts->shaggyType == 5)
    {
        switch (dguts->shaggyType)
        {
        case 0:
        case 5:
            sguts->prefIdle = 0;
            if (strcmp(name, "Idle") != 0)
            {
                return;
            }
            break;
        case 1:
            sguts->prefIdle = 1;
            if (strcmp(name, "Idle_scared") != 0)
            {
                return;
            }
            break;
        case 7:
            sguts->prefIdle = 2;
            if (strcmp(name, "Idle_hungry") != 0)
            {
                return;
            }
            break;
        }
    }
    else
    {
        if (strcmp(name, "Idle") != 0)
        {
            return;
        }
        dguts->prefIdle = 0;
        switch (sguts->shaggyType)
        {
        case 1:
            dguts->prefIdle = 1;
            idle = "Idle_scared";
            break;
        case 7:
            dguts->prefIdle = 2;
            idle = "Idle_hungry";
            break;
        }
    }

    sguts->swap = NULL;
    zEntEvent(src, dest, 3);
    *(_xMat4x3*)dest->model->Mat = *(_xMat4x3*)src->model->Mat;
    dest->frame->mat = src->frame->mat;
    dest->frame->oldpos.x = src->frame->oldpos.x;
    dest->frame->oldpos.y = src->frame->oldpos.y;
    dest->frame->oldpos.z = src->frame->oldpos.z;
    dest->bound.sph.center.x = src->bound.sph.center.x;
    dest->bound.sph.center.y = src->bound.sph.center.y;
    dest->bound.sph.center.z = src->bound.sph.center.z;
    xAnimPlaySetState(dest->model->Anim->Single, xAnimTableGetState(dest->model->Anim->Table, idle),
                      0.0f);
    dest->model->Anim->Single->Time = src->model->Anim->Single->Time;
    zEntEvent(src, 4);
    memcpy((_xMat4x3*)dest->model->Mat + 1, (_xMat4x3*)src->model->Mat + 1,
           dest->model->BoneCount << 6);
}

void Shaggy5_Update(_zEnt* ent, _xScene* sc, float dt)
{
    if (!xEntIsVisible(ent))
    {
        return;
    }

    if (gBurningManEmitter != NULL)
    {
        ShaggyGuts* guts = ShaggyGetGuts(ent);
        if (guts->prefIdle == 3)
        {
            _zParEmitterCustomSettings info;
            info.custom_flags = 0x20;
            _xMat4x3* mat = (_xMat4x3*)ent->model->Mat;
            info.pos.x = -0.15f * mat->at.x + mat->pos.x;
            info.pos.y = -0.15f * mat->at.y + mat->pos.y;
            info.pos.y = info.pos.y + 0.9f;
            info.pos.z = -0.15f * mat->at.z + mat->pos.z;
            zParEmitterEmitCustom(gBurningManEmitter, dt, &info);
        }
    }

    ShaggyGuts* guts = ShaggyGetGuts(ent);

    if (guts->witchFollow != NULL)
    {
        xEntBeginUpdate(ent, sc, dt);
        xEntEndUpdate(ent, sc, dt);
        return;
    }

    if (guts->swap == NULL)
    {
        guts->idleTimer += dt;
    }

    if (guts->talkTimer != 0.0f)
    {
        guts->talkTimer -= dt;
        guts->talkSwitch -= dt;
        if (guts->talkTimer < 0.0f)
        {
            guts->talkTimer = 0.0f;
        }
    }

    if (guts->witchDrop != NULL)
    {
        _xMat4x3* mat = (_xMat4x3*)ent->model->Mat;
        mat->pos.y = ent->frame->vel.y * dt + mat->pos.y;
        ent->frame->vel.y = -(10.0f * dt - ent->frame->vel.y);
        if (((_xMat4x3*)ent->model->Mat)->pos.y <
            -2.0f + ((_xMat4x3*)guts->witchDrop->model->Mat)->pos.y)
        {
            guts->witchDrop = NULL;
            ent->collis->chk = 0x26;
            ent->chkby = 0x1C;
            ShaggyVil_HandleVisible(ent, 4);
            zEntEvent(ent, 2);
        }
        xEntBeginUpdate(ent, sc, dt);
        xEntEndUpdate(ent, sc, dt);
        return;
    }

    guts->eatTimer -= dt;

    if (guts->geronimoChase != NULL)
    {
        _xMat4x3* mat = (_xMat4x3*)ent->model->Mat;
        _xMat4x3* gmat = (_xMat4x3*)guts->geronimoChase->model->Mat;
        float dz = mat->pos.z - gmat->pos.z;
        float dx = mat->pos.x - gmat->pos.x;
        float d2 = dx * dx + dz * dz;
        float* spd_move = (float*)((char*)ent + 0x154);
        float* spd_chase = (float*)((char*)ent + 0x158);
        if (d2 < 16.0f)
        {
            float d = 0.5f * dt;
            *spd_move += d;
            *spd_chase += d;
            if (*spd_move > 4.1f)
            {
                *spd_move = 4.1f;
            }
            if (*spd_chase > 4.1f)
            {
                *spd_chase = 4.1f;
            }
        }
        if (d2 > 36.0f)
        {
            float d = 0.5f * dt;
            *spd_move -= d;
            *spd_chase -= d;
            if (*spd_move < 3.6f)
            {
                *spd_move = 3.6f;
            }
            if (*spd_chase < 3.6f)
            {
                *spd_chase = 3.6f;
            }
        }
    }

    sVilUpdateFunc(ent, sc, dt);

    if (guts->swap != NULL)
    {
        ShaggySwap(ent, guts->swap);
    }
}

void Shaggy5_Move(_xEnt* ent, _xScene* s, float dt, _xEntFrame* frame)
{
    sVilMoveFunc(ent, s, dt, frame);
}

xAnimTable* zEntShaggy_Anim5()
{
    xAnimTable* table;

    table = xAnimTableNew("Shaggy5", NULL);
    xAnimTableNewState(table, "Idle", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle2", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle3", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_scared", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_scared2", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_scared3", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_hungry", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_hungry2", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_garg", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_garg2", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_garg3", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_witch", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Move_walk", 0x10, 0x02000004, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Move_run", 0x10, 0x02000006, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Move_hungry", 0x10, 0x02000004, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Move_sneak", 0x10, 0x02000004, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Move_turn", 0x10, 0x02000000, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Chimney_intro", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Fire_hit", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Eat_tomove", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Eat_tostop", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Eat_loop", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "React_scare", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "React_death", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Talk", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Talk2", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Talk3", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Talk4", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewTransition(table, "Idle Idle2 Idle3 Idle_scared Idle_scared2 Idle_scared3 Idle_hungry Idle_hungry2 Idle_garg Idle_garg2 Idle_garg3 Move_sneak Move_run Move_hungry", "Move_walk", Shaggy5_MoveTran, NULL, 0, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle Idle2 Idle3 Idle_scared Idle_scared2 Idle_scared3 Idle_hungry Idle_hungry2 Idle_garg Idle_garg2 Idle_garg3 Move_walk Move_run Move_hungry", "Move_sneak", Shaggy5_MoveTran, NULL, 0, 3, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle Idle2 Idle3 Idle_scared Idle_scared2 Idle_scared3 Idle_hungry Idle_hungry2 Idle_garg Idle_garg2 Idle_garg3 Move_walk Move_sneak Move_hungry", "Move_run", Shaggy5_MoveTran, Shaggy1_IdleResetCB, 0, 1, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle Idle2 Idle3 Idle_scared Idle_scared2 Idle_scared3 Idle_hungry Idle_hungry2 Idle_garg Idle_garg2 Idle_garg3 Move_walk Move_sneak Move_run", "Move_hungry", Shaggy5_MoveTran, NULL, 0, 2, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Move_walk Move_sneak Move_run Move_hungry Idle_scared Idle_scared2 Idle_scared3 Idle_hungry Idle_hungry2 Idle_garg Idle_garg2 Idle_garg3 ", "Idle", Shaggy5_IdleTran, Shaggy1_IdleResetCB, 0, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Move_walk Move_sneak Move_run Move_hungry Idle Idle2 Idle3 Idle_hungry Idle_hungry2 Idle_garg Idle_garg2 Idle_garg3 ", "Idle_scared", Shaggy5_IdleTran, Shaggy1_IdleResetCB, 0, 1, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Move_walk Move_sneak Move_run Move_hungry Idle Idle2 Idle3 Idle_scared Idle_scared2 Idle_scared3 Idle_garg Idle_garg2 Idle_garg3 ", "Idle_hungry", Shaggy5_IdleTran, Shaggy1_IdleResetCB, 0, 2, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Move_walk Move_sneak Move_run Move_hungry Idle Idle2 Idle3 Idle_scared Idle_scared2 Idle_scared3 Idle_hungry Idle_hungry2 ", "Idle_garg", Shaggy5_IdleTran, Shaggy1_IdleResetCB, 0, 3, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableAddTransition(table, xAnimTableNewTransition(table, "Idle Idle2 Idle3 Idle_scared Idle_scared2 Idle_scared3 Idle_hungry Idle_hungry2 Idle_garg Idle_garg2 Idle_garg3 ", "Idle_witch", Shaggy5_WitchAttached, NULL, 0, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL), "Move_walk Move_sneak Move_run Move_turn Fire_hit");
    xAnimTableNewTransition(table, "Idle_witch", "Chimney_intro", Shaggy5_WitchDropped, NULL, 0, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle Idle2 Idle3 Idle_scared Idle_scared2 Idle_scared3 Idle_hungry Idle_hungry2 Idle_garg Idle_garg2 Idle_garg3 Move_walk Move_sneak Move_run Move_hungry ", "Eat_tomove", Shaggy5_Action, Shaggy5_EatMoveCB, 0, 1, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Eat_tomove", "Move_hungry", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Move_hungry", "Eat_tostop", Shaggy5_EatStop, Shaggy5_EatStopCB, 0, 0, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Eat_tostop", "Eat_loop", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Eat_loop", "Idle_hungry", Shaggy5_EatTimeUp, NULL, 0, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "React_scare", "Idle", Shaggy5_IdleTran, Shaggy1_IdleResetCB, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "React_scare", "Idle_scared", Shaggy5_IdleTran, Shaggy1_IdleResetCB, 0x10, 1, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "React_scare", "Idle_hungry", Shaggy5_IdleTran, Shaggy1_IdleResetCB, 0x10, 2, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "React_scare", "Idle_garg", Shaggy5_IdleTran, Shaggy1_IdleResetCB, 0x10, 3, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "React_scare", "Move_walk", Shaggy5_MoveTran, NULL, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "React_scare", "Move_sneak", Shaggy5_MoveTran, NULL, 0x10, 3, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "React_scare", "Move_run", Shaggy5_MoveTran, Shaggy1_IdleResetCB, 0x10, 1, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "React_scare", "Move_hungry", Shaggy5_MoveTran, NULL, 0x10, 2, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle", "Idle2", Shaggy1_IdlePick, NULL, 0, 4, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle2", "Idle", Shaggy1_IdlePick, NULL, 0, 7, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle", "Idle2", Shaggy1_IdlePick, NULL, 0, 0xC, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle2", "Idle", Shaggy1_IdlePick, NULL, 0, 0xE, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle", "Idle3", Shaggy1_IdlePick, NULL, 0, 0x11, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle3", "Idle", Shaggy1_IdlePick, Shaggy1_IdleResetCB, 0, 0x13, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle2 Idle3", "Idle", Shaggy1_IdleLeave, Shaggy1_IdleResetCB, 0, 0, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle_scared", "Idle_scared2", Shaggy1_IdlePick, NULL, 0, 4, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_scared2", "Idle_scared", Shaggy1_IdlePick, NULL, 0, 7, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_scared", "Idle_scared2", Shaggy1_IdlePick, NULL, 0, 0xC, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_scared2", "Idle_scared", Shaggy1_IdlePick, NULL, 0, 0xE, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_scared", "Idle_scared3", Shaggy1_IdlePick, NULL, 0, 0x11, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_scared3", "Idle_scared", Shaggy1_IdlePick, Shaggy1_IdleResetCB, 0, 0x13, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_scared2 Idle_scared3", "Idle_scared", Shaggy1_IdleLeave, Shaggy1_IdleResetCB, 0, 0, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle_hungry", "Idle_hungry2", Shaggy1_IdlePick, NULL, 0, 4, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_hungry2", "Idle_hungry", Shaggy1_IdlePick, NULL, 0, 7, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_hungry", "Idle_hungry2", Shaggy1_IdlePick, NULL, 0, 0xC, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_hungry2", "Idle_hungry", Shaggy1_IdlePick, NULL, 0, 0xE, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_hungry", "Idle_hungry2", Shaggy1_IdlePick, NULL, 0, 0x11, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_hungry2", "Idle_hungry", Shaggy1_IdlePick, Shaggy1_IdleResetCB, 0, 0x13, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_hungry2", "Idle_hungry", Shaggy1_IdleLeave, Shaggy1_IdleResetCB, 0, 0, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle_garg", "Idle_garg2", Shaggy1_IdlePick, NULL, 0, 4, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_garg2", "Idle_garg", Shaggy1_IdlePick, NULL, 0, 7, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_garg", "Idle_garg2", Shaggy1_IdlePick, NULL, 0, 0xC, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_garg2", "Idle_garg", Shaggy1_IdlePick, NULL, 0, 0xE, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_garg", "Idle_garg3", Shaggy1_IdlePick, NULL, 0, 0x11, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_garg3", "Idle_garg", Shaggy1_IdlePick, Shaggy1_IdleResetCB, 0, 0x13, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_garg2 Idle_garg3", "Idle_garg", Shaggy1_IdleLeave, Shaggy1_IdleResetCB, 0, 0, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Move_run", "Fire_hit", Shaggy5_GargRunPick, NULL, 0, 0x46, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Move_run", "Fire_hit", Shaggy5_GargRunPick, Shaggy1_IdleResetCB, 0, 0x7B, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Fire_hit", "Move_run", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle Idle2 Idle3 Idle_scared Idle_scared2 Idle_scared3 Idle_hungry Idle_hungry2 Idle_garg Idle_garg2 Idle_garg3 Move_walk Move_sneak Move_run", "React_scare", Shaggy1_ScareReact, NULL, 0, 0, 0.0f, 0.0f, 0xB, 0, 0.15f, NULL);
    xAnimTableAddTransition(table, xAnimTableNewTransition(table, "Idle Idle2 Idle3 Idle_scared Idle_scared2 Idle_scared3 Idle_hungry Idle_hungry2 Idle_garg Idle_garg2 Idle_garg3 Move_walk Move_sneak Move_run Move_hungry React_scare", "React_death", Shaggy1_DeathReact, NULL, 0, 0, 0.0f, 0.0f, 0xC, 0, 0.15f, NULL), "Chimney_intro Fire_hit Eat_tomove Eat_tostop Eat_loop");
    return table;
}

void Shaggy5_WitchTransitions(xAnimTable* table)
{
    xAnimTransition* tran;

    tran = xAnimTableNewTransition(table, "Idle Idle_Fidget_1 Idle_Fidget_2", "Shaggy_hold", Witch_HasShaggy, Witch_GrabShaggyCB, 0, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableAddTransition(table, tran, "Bored Bored_Fidget_1 Bored_Fidget_1 Move Chase Surprize");
    xAnimTableAddTransition(table, tran, "Taunt Taunt_1 Taunt_2 Hear_Start Hear_Loop Look_Start Look_Loop");
    xAnimTableAddTransition(table, tran, "Investigate_Start Investigate AttackIdle Attack Attack_1 Attack_2");
    xAnimTableAddTransition(table, tran, "Combo Combo_1 Combo_2 Lob_Attack Lob_Combo Extend_Attack_Start Extend_Attack_Loop");
    xAnimTableAddTransition(table, tran, "Extend_Combo Hit_By_Gum Gummed Gummed_Resist Gummed_BreakFree Hit_When_Gummed");
    xAnimTableAddTransition(table, tran, "Bounced_When_Gummed Hit_By_Soap Soaped Soaped_Resist Soaped_BreakFree");
    xAnimTableAddTransition(table, tran, "Hit_When_Soaped Bounced_When_Soaped Dazed_Begin Dazed Dazed_Recovery Death");
    xAnimTableAddTransition(table, tran, "Death_From_Above Death_From_Dazed Death_Idle Jump_Windup Jump_In_Air Jump_Landing");
    xAnimTableNewTransition(table, "Shaggy_hold", "Idle", Witch_DropShaggyIdle, Witch_DropShaggyCB, 0, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Shaggy_hold", "Move", Witch_DropShaggyMove, Witch_DropShaggyCB, 0, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
}

unsigned int Witch_DropShaggyCB(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    ent->collis->chk |= 0x20;
    return 0;
}

unsigned int Witch_GrabShaggyCB(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    ent->collis->chk &= ~0x20;
    return 0;
}

static inline unsigned int WitchHasShaggy(_zEnt* ent)
{
    for (signed int i = 0; i < (signed int)gNumShaggy; i++)
    {
        if (ent == gShaggy[i].witchFollow)
        {
            return 1;
        }
    }
    return 0;
}

unsigned int Witch_DropShaggyIdle(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    signed int goal = *(signed int*)((char*)ent + 0x114);
    return goal != 6 && goal != 8 && !WitchHasShaggy(ent);
}

unsigned int Witch_DropShaggyMove(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    signed int goal = *(signed int*)((char*)ent + 0x114);
    return (goal == 6 || goal == 8) && !WitchHasShaggy(ent);
}

unsigned int Witch_HasShaggy(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    for (signed int i = 0; i < (signed int)gNumShaggy; i++)
    {
        if (ent == gShaggy[i].witchFollow)
        {
            return 1;
        }
    }
    return 0;
}

unsigned int Shaggy5_GargRunPick(xAnimTransition* tran, xAnimSingle*, _zEnt* ent)
{
    ShaggyGuts* guts = ShaggyGetGuts(ent);
    return guts->prefIdle == 3 && guts->swap == NULL &&
           guts->idleTimer >= 0.0625f * tran->UserFlags &&
           guts->idleTimer - 0.5f < 0.0625f * tran->UserFlags;
}

unsigned int Shaggy5_EatTimeUp(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    return ShaggyGetGuts(ent)->eatTimer <= 0.0f;
}

unsigned int Shaggy5_EatStopCB(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    ShaggyGetGuts(ent)->eatTimer = 2.0f;
    return 0;
}

unsigned int Shaggy5_EatStop(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    ShaggyGuts* guts = ShaggyGetGuts(ent);
    signed int goal = *(signed int*)((char*)ent + 0x114);
    return goal != 6 && goal != 8 && guts->eatTimer > 0.0f;
}

unsigned int Shaggy5_EatMoveCB(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    ShaggyGuts* guts = ShaggyGetGuts(ent);
    guts->eatTimer = 1000.0f;
    guts->actionIndex = 0;
    return 0;
}

unsigned int Shaggy5_ActionCB(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    ShaggyGetGuts(ent)->actionIndex = 0;
    return 0;
}

unsigned int Shaggy5_Action(xAnimTransition* tran, xAnimSingle*, _zEnt* ent)
{
    return ShaggyGetGuts(ent)->actionIndex == tran->UserFlags;
}

unsigned int Shaggy5_WitchDropped(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    return ShaggyGetGuts(ent)->witchDrop != NULL;
}

unsigned int Shaggy5_WitchAttached(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    return ShaggyGetGuts(ent)->witchFollow != NULL;
}

unsigned int Shaggy5_IdleTran(xAnimTransition* tran, xAnimSingle*, _zEnt* ent)
{
    ShaggyGuts* guts = ShaggyGetGuts(ent);
    signed int goal = *(signed int*)((char*)ent + 0x114);
    return goal != 6 && goal != 8 && guts->prefIdle == tran->UserFlags;
}

unsigned int Shaggy5_MoveTran(xAnimTransition* tran, xAnimSingle*, _zEnt* ent)
{
    ShaggyGuts* guts = ShaggyGetGuts(ent);
    signed int goal = *(signed int*)((char*)ent + 0x114);
    return (goal == 6 || goal == 8) && guts->prefMove == tran->UserFlags;
}

signed int Shaggy3_EventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                           xBase* toParamWidget)
{
    _zEnt* ent = (_zEnt*)to;
    xAnimState* state = ent->model->Anim->Single->State;
    ShaggyGuts* guts = ShaggyGetGuts(ent);

    switch (toEvent)
    {
    case 0x20:
        if (guts->stopMode == 3)
        {
            guts->runEvent = 1;
            guts->stopEvent = 0;
        }
        break;
    case 0x12:
        guts->stopEvent = 0;
        if (xStricmp(state->Name, "Talk") == 0 || xStricmp(state->Name, "StopReact") == 0)
        {
            guts->runEvent = 1;
        }
        break;
    case 0x13:
        if (guts->stopMode != 0)
        {
            guts->runEvent = 0;
            if (guts->stopMode == 2 || guts->stopMode == 3)
            {
                guts->stopEvent = 1;
            }
        }
        break;
    case 0x123:
        if (toParam != NULL)
        {
            guts->stopMode = toParam[0];
            guts->stopParam = toParam[1];
            if (guts->stopMode == 0 && strcmp(state->Name, "Drive") != 0 &&
                strcmp(state->Name, "Drive2") != 0 && strcmp(state->Name, "TurnLeft") != 0 &&
                strcmp(state->Name, "TurnRight") != 0)
            {
                guts->runEvent = 1;
                guts->stopEvent = 0;
            }
        }
        else
        {
            guts->stopMode = 1;
        }
        break;
    }
    return zPlatformEventCB(from, to, toEvent, toParam, toParamWidget);
}

void Shaggy3_Update(_zEnt* ent, _xScene* sc, float dt)
{
    if (gTractorEmitter != NULL && ((_zPlatform*)ent)->moving != 0)
    {
        _zParEmitterCustomSettings info;
        info.custom_flags = 0x60;
        _xMat4x3* mat = (_xMat4x3*)ent->model->Mat;
        info.pos.x = -1.45f * mat->at.x + mat->pos.x;
        info.pos.y = -1.45f * mat->at.y + mat->pos.y;
        info.vel.x = -1.25f * mat->at.x;
        info.vel.y = 1.0f;
        info.pos.z = -1.45f * mat->at.z + mat->pos.z;
        info.vel.z = -1.25f * mat->at.z;
        zParEmitterEmitCustom(gTractorEmitter, dt, &info);
    }

    ShaggyGuts* guts = ShaggyGetGuts(ent);
    guts->oldAt2 = guts->oldAt;
    guts->oldAt = ((_xMat4x3*)ent->model->Mat)->at;

    float oldAng = atan2(guts->oldAt2.x, guts->oldAt2.z);
    float newAng = atan2(guts->oldAt.x, guts->oldAt.z);
    oldAng = xAngleClamp(oldAng);
    float diff = xAngleClamp(newAng) - oldAng;
    gShaggy3TurnAngle = diff;
    while (diff > 3.1415927f)
    {
        diff -= 6.2831855f;
    }
    while (diff < -3.1415927f)
    {
        diff += 6.2831855f;
    }
    gShaggy3TurnAngle = diff;

    if (globals.player.Health != 0)
    {
        guts->idleTimer += dt;
    }

    if (guts->runEvent != 0)
    {
        xAnimState* state = ent->model->Anim->Single->State;
        if (xStricmp(state->Name, "Talk") != 0 && xStricmp(state->Name, "StopReact") != 0)
        {
            zPlatformEventCB(NULL, ent, 0x12, NULL, NULL);
            guts->runEvent = 0;
        }
    }

    _xMat4x3* mat = (_xMat4x3*)ent->model->Mat;
    _xMat4x3* pmat = (_xMat4x3*)globals.player.ent.model->Mat;
    float dx = pmat->pos.x - mat->pos.x;
    float dz = pmat->pos.z - mat->pos.z;
    if (guts->stopMode == 3)
    {
        if (dx * dx + dz * dz > guts->stopParam * guts->stopParam)
        {
            guts->stopEvent = 1;
            guts->runEvent = 0;
        }
        else if (globals.player.drv.driver != NULL && globals.player.drv.driver == ent)
        {
            guts->stopEvent = 0;
            guts->runEvent = 1;
        }
    }

    zPlatform_Update(ent, sc, dt);
}

xAnimTable* zEntShaggy_Anim3()
{
    xAnimTable* table;

    table = xAnimTableNew("Shaggy3", NULL);
    xAnimTableNewState(table, "Idle", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Drive", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Drive2", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "TurnLeft", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "TurnRight", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "StopReact", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Talk", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewTransition(table, "Idle", "Drive", Shaggy3_Move, Shaggy1_IdleResetCB, 0, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Drive Drive2 TurnLeft TurnRight", "Idle", Shaggy3_Stop, NULL, 0, 0, 0.0f, 0.0f, 2, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Drive Drive2 TurnLeft TurnRight", "StopReact", Shaggy3_StopScare, NULL, 0, 0, 0.0f, 0.0f, 3, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Drive Drive2 TurnRight", "TurnLeft", Shaggy3_Left, NULL, 0, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Drive Drive2 TurnLeft", "TurnRight", Shaggy3_Right, NULL, 0, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "TurnLeft TurnRight", "Drive", Shaggy3_Center, Shaggy1_IdleResetCB, 0, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "StopReact", "Idle", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Idle", "Talk", Shaggy1_Talk, NULL, 0, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Talk", "Idle", Shaggy1_Notalk, NULL, 0, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Drive", "Drive2", Shaggy1_IdlePick, NULL, 0, 3, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Drive2", "Drive", Shaggy1_IdlePick, Shaggy1_IdleResetCB, 0, 5, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle Drive TurnLeft TurnRight", "Drive2", Shaggy3_MoveDead, Shaggy1_IdleResetCB, 0, 0, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    xAnimStateNewEffect(xAnimTableGetState(table, "StopReact"), 9, 0.5f, 0.0f, Shaggy3_MowerStopCB, 0);
    return table;
}

unsigned int Shaggy3_MowerStopCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void* object)
{
    zPlatformEventCB(NULL, (xBase*)object, 0x13, NULL, NULL);
    return 0;
}

static inline unsigned int Shaggy3FrameMoving(_xEntFrame* frame)
{
    return (frame->mode & 2) &&
           frame->dpos.x * frame->dpos.x + frame->dpos.y * frame->dpos.y +
                   frame->dpos.z * frame->dpos.z >=
               0.0005f;
}

unsigned int Shaggy3_Center(xAnimTransition*, xAnimSingle* single, _zPlatform*)
{
    return __fabs(gShaggy3TurnAngle) <= 0.0015f && single->Blend->State == NULL;
}

unsigned int Shaggy3_Right(xAnimTransition*, xAnimSingle* single, _zPlatform*)
{
    return globals.player.Health != 0 && gShaggy3TurnAngle < -0.0015f &&
           single->Blend->State == NULL;
}

unsigned int Shaggy3_Left(xAnimTransition*, xAnimSingle* single, _zPlatform*)
{
    return globals.player.Health != 0 && gShaggy3TurnAngle > 0.0015f &&
           single->Blend->State == NULL;
}

unsigned int Shaggy3_StopScare(xAnimTransition*, xAnimSingle*, _zPlatform* ent)
{
    return ShaggyGetGuts(ent)->stopEvent;
}

unsigned int Shaggy3_Stop(xAnimTransition*, xAnimSingle*, _zPlatform* ent)
{
    _xEntFrame* frame = ent->frame;
    if (frame == NULL)
    {
        return 0;
    }
    unsigned char ret = 0;
    unsigned char stopped = 1;
    if (frame->mode & 2)
    {
        if (!(frame->dpos.x * frame->dpos.x + frame->dpos.y * frame->dpos.y +
                  frame->dpos.z * frame->dpos.z <
              0.0005f))
        {
            stopped = 0;
        }
    }
    if (stopped && globals.player.DamageTimer < 1.25f)
    {
        ret = 1;
    }
    return ret;
}

unsigned int Shaggy3_MoveDead(xAnimTransition*, xAnimSingle*, _zPlatform* ent)
{
    _xEntFrame* frame = ent->frame;
    if (frame == NULL)
    {
        return 0;
    }
    unsigned char moving = globals.player.Health == 0 && (frame->mode & 2);
    return moving && frame->dpos.x * frame->dpos.x + frame->dpos.y * frame->dpos.y +
                             frame->dpos.z * frame->dpos.z >=
                         0.0005f;
}

unsigned int Shaggy3_Move(xAnimTransition*, xAnimSingle*, _zPlatform* ent)
{
    _xEntFrame* frame = ent->frame;
    if (frame == NULL)
    {
        return 0;
    }
    return Shaggy3FrameMoving(frame);
}

void Shaggy2_Render(_xEnt* ent)
{
    if (xEntIsVisible(ent))
    {
        xModelRender(ent->model);
    }
}

signed int Shaggy2Plat_EventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                               xBase* toParamWidget)
{
    for (unsigned int i = 0; i < gNumShaggy; i++)
    {
        if ((xBase*)gShaggy[i].chimneyPlat == to &&
            (toEvent == 0x12 || toEvent == 0x15 || toEvent == 0x20))
        {
            zEntEvent(from, 0, gShaggy[i].ent, toEvent, toParam, toParamWidget, 0);
        }
    }
    return zPlatformEventCB(from, to, toEvent, toParam, toParamWidget);
}

signed int Shaggy2_EventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                           xBase* toParamWidget)
{
    signed int ret = 1;

    switch (toEvent)
    {
    case 0x12:
    case 0x15:
    {
        ShaggyGuts* guts = ShaggyGetGuts((_zEnt*)to);
        if (guts != NULL && guts->chimneyState == 0)
        {
            guts->chimneyState = 1;
            guts->chimneyTimer = 0.0f;
            guts->timeEvent = *toParam;
            guts->chimneyRand = (irand() >> 15) & 1;
        }
    }
    case 0x14:
    default:
        ret = zPlatformEventCB(from, to, toEvent, toParam, toParamWidget);
        break;
    }
    return ret;
}

void Shaggy2_Update(_zEnt* ent, _xScene* sc, float dt)
{
    ShaggyGuts* guts = ShaggyGetGuts(ent);
    xModelInstance* minst = ent->model;
    xModelInstance* inst = minst->Next;
    xModelInstance* inst2 = inst->Next;
    _xVec3* pos = &((_xMat4x3*)inst2->Mat)[inst2->BoneCount + 1].pos;

restart:
    switch (guts->chimneyState)
    {
    case 1:
    {
        unsigned int i;
        for (i = 0; i < gNumShaggy; i++)
        {
            if (gShaggy[i].chimneyState > 1)
            {
                break;
            }
        }
        if (i != gNumShaggy)
        {
            break;
        }
        guts->chimneyState = 2;
        guts->chimneyTimer = 0.0f;
    }
    case 2:
        if (guts->chimneyTimer == 0.0f)
        {
            inst2->Flags |= 6;
            pos->y = -0.6f + ((_xMat4x3*)minst->Mat)->pos.y;
        }
        else
        {
            inst2->Flags |= 1;
            pos->y = 3.0f * dt + pos->y;
            if (pos->y > 0.0f)
            {
                pos->y = 0.0f;
            }
            if (guts->chimneyTimer + dt >= guts->timeLaunch - 0.9f &&
                guts->chimneyTimer < guts->timeLaunch - 0.9f)
            {
                xSndPlay3D(0x0F03423C, 0.5f, 0.0f, 0, 0x14, &((_xMat4x3*)inst2->Mat)->pos, 12.0f);
            }
            if (guts->chimneyTimer >= guts->timeLaunch)
            {
                guts->chimneyState = 3;
                guts->chimneyTimer = 0.0f;
                float h = guts->chimneyHeight - pos->y;
                if (h < 0.0f)
                {
                    h = 0.0f;
                }
                guts->chimneyVel = xsqrt(2.0f * guts->chimneyGrav * h);
            }
        }
        break;
    case 3:
        if (guts->chimneyTimer >= guts->timeTop || guts->chimneyVel != 0.0f)
        {
            float vel = guts->chimneyVel;
            pos->y = vel * dt + pos->y;
            guts->chimneyVel = -(guts->chimneyGrav * dt - guts->chimneyVel);
            if (vel > 0.0f && guts->chimneyVel <= 0.0f)
            {
                guts->chimneyVel = 0.0f;
                guts->chimneyTimer = 0.0f;
                xSndPlay3D(0x0F0343C3, 0.5f, 0.0f, 0, 0x14, &((_xMat4x3*)inst2->Mat)->pos, 12.0f);
            }
        }
        if (pos->y <= -0.6f)
        {
            inst2->Flags &= ~7;
            guts->chimneyState = 4;
            guts->chimneyTimer = 0.0f;
        }
        break;
    case 4:
        if (guts->chimneyTimer >= guts->timeEvent)
        {
            guts->chimneyState = 0;
            guts->chimneyTimer = 0.0f;
            zEntEvent(ent, 0x13);
            if (guts->chimneyState != 0)
            {
                goto restart;
            }
        }
        break;
    }

    xEntUpdate(ent, sc, dt);

    if (guts != NULL)
    {
        guts->chimneyTimer += dt;
    }

    ent->chkby |= 0x10;
    ent->penby |= 0x10;

    if (guts != NULL && guts->chimneyPlat != NULL)
    {
        _xVec3 chimneyPos;
        _xVec3 shaggyPos;
        iModelTagEval(inst->Data, &ChimneyTag, inst->Mat, &chimneyPos);
        iModelTagEval(inst2->Data, &ShaggyTag, inst2->Mat, &shaggyPos);
        if (chimneyPos.y < shaggyPos.y && (inst2->Flags & 1))
        {
            chimneyPos.y = shaggyPos.y;
        }
        _zPlatform* plat = guts->chimneyPlat;
        plat->frame->oldpos = ((_xMat4x3*)plat->model->Mat)->pos;
        ((_xMat4x3*)guts->chimneyPlat->model->Mat)->pos.x = ((_xMat4x3*)inst->Mat)->pos.x;
        ((_xMat4x3*)guts->chimneyPlat->model->Mat)->pos.y = chimneyPos.y - 2.5f;
        ((_xMat4x3*)guts->chimneyPlat->model->Mat)->pos.z = ((_xMat4x3*)inst->Mat)->pos.z;
        plat = guts->chimneyPlat;
        plat->frame->mat.pos = ((_xMat4x3*)plat->model->Mat)->pos;
    }
}

xAnimTable* zEntShaggy_Chimney()
{
    xAnimTable* table;

    table = xAnimTableNew("ShaggyChimney", NULL);
    xAnimTableNewState(table, "Idle", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Shoot", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Land", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewTransition(table, "Idle", "Shoot", Shaggy2_StateChange, NULL, 0, 2, 0.0f, 0.0f, 1, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Idle", "Land", Shaggy2_StateChange, NULL, 0, 4, 0.0f, 0.0f, 1, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Shoot Land", "Idle", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.0f, NULL);
    return table;
}

xAnimTable* zEntShaggy_Anim2()
{
    xAnimTable* table;

    table = xAnimTableNew("Shaggy2", NULL);
    xAnimTableNewState(table, "Idle", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle2", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Shoot", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Apex", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Fall", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Intro", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewTransition(table, "Idle Idle2 Fall Apex", "Intro", Shaggy2_StateChange, NULL, 0, 2, 0.0f, 0.0f, 1, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Idle Idle2 Intro", "Shoot", Shaggy2_StateChange, NULL, 0, 3, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Shoot", "Apex", Shaggy2_Apex, NULL, 0, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Apex", "Fall", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Intro", "Idle", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Intro", "Idle2", Shaggy2_RandCheck, NULL, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    return table;
}

unsigned int Shaggy2_RandCheck(xAnimTransition*, xAnimSingle*, _zEnt*)
{
    return irand() > 0;
}

unsigned int Shaggy2_Apex(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    ShaggyGuts* guts = ShaggyGetGuts(ent);
    return guts != NULL && guts->chimneyVel < 1.0f;
}

unsigned int Shaggy2_StateChange(xAnimTransition* tran, xAnimSingle*, _zEnt* ent)
{
    ShaggyGuts* guts = ShaggyGetGuts(ent);
    return guts != NULL && guts->chimneyState == tran->UserFlags && guts->chimneyTimer == 0.0f;
}

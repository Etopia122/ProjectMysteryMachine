// TODO: 97.8% (NonMatching). Most remaining differences are register
// allocation: the inlined HealthCheck swaps the health and constant registers
// in every transition check, and the large update functions (zEntPlayer_Update,
// PlayerHangCheck/HangUpdate, HeadcollUpdate, CalcJumpImpulse_Smooth) are close
// but not exact. A few .sdata2 constants (0.35f from zEntPlayer_Damage and
// -2.0f from CalcJumpImpulse_Smooth) sit earlier in the target pool than the
// reverse-order model puts them. zEntPlayer_FXSetup, zEntPlayer_ButtonHBFX and
// zEntPlayer_ButtonBBFX are auto-inlined helpers kept at their PS2 positions so
// their literals land in the right place; the linker must strip them once this
// unit is Matching.
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
#include "xFFX.h"
#include "iMath3.h"
#include "iDraw.h"
#include "xEntMotion.h"
#include "xShadow.h"
#include "xTRC.h"
#include "xEnv.h"
#include "iEnv.h"

#ifndef NULL
#define NULL 0
#endif

class RwMatrixTag;
class RpClump;
class xAnimActiveEffect;
class xAnimEffect;
class _zEntButton;
class _zEntDestructObj;

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
    unsigned char chk;        // 0x0
    unsigned char pen;        // 0x1
    unsigned char env_sidx;   // 0x2
    unsigned char env_eidx;   // 0x3
    unsigned char npc_sidx;   // 0x4
    unsigned char npc_eidx;   // 0x5
    unsigned char dyn_sidx;   // 0x6
    unsigned char dyn_eidx;   // 0x7
    unsigned char stat_sidx;  // 0x8
    unsigned char stat_eidx;  // 0x9
    unsigned char trig_idx;   // 0xA
    unsigned char idx;        // 0xB
    _xCollis colls[18];       // 0xC
    void (*post)(_xEnt*, _xScene*, float, _xEntCollis*);                 // 0x4D4
    unsigned int (*depenq)(_xEnt*, _xEnt*, _xScene*, float, _xCollis*); // 0x4D8
    unsigned char trigs_cur[8];     // 0x4DC
    unsigned char trigs_prev[8];    // 0x4E4
    unsigned char trigs_entered[8]; // 0x4EC
    unsigned char trigs_exited[8];  // 0x4F4
};

class p2EntTriggerAsset : public p2EntAsset
{
public:
    _xVec3 p[4];       // 0x50
    _xVec3 direction;  // 0x80
    unsigned int flags; // 0x8C
};

class xAnimFile
{
public:
    xAnimFile* Next;          // 0x0
    char* Name;               // 0x4
    unsigned int ID;          // 0x8
    unsigned int FileFlags;   // 0xC
    float Duration;           // 0x10
    float TimeOffset;         // 0x14
    float PhysBase[4];        // 0x18
    float PhysDelta[4];       // 0x28
    unsigned short BoneCount; // 0x38
    unsigned char NumAnims[2]; // 0x3A
    void** RawData;           // 0x3C
    void* Physics;            // 0x40
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
    float SrcTime;          // 0x18
    float DestTime;         // 0x1C
    unsigned short Priority;      // 0x20
    unsigned short QueuePriority; // 0x22
    float BlendRecip;       // 0x24
    unsigned short* BlendOffset;  // 0x28
};

class xAnimPlay;

class xAnimSingle
{
public:
    unsigned int SingleFlags; // 0x0
    xAnimState* State;        // 0x4
    float Time;               // 0x8
    float CurrentSpeed;       // 0xC
    float BilinearLerp[2];    // 0x10
    unsigned char pad18[0x24 - 0x18];
    xAnimPlay* Play;          // 0x24
    unsigned char pad28[0x54 - 0x28];
    xAnimTransition* Tran;    // 0x54
    xAnimSingle* Blend;       // 0x58
    float BlendFactor;        // 0x5C
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
    unsigned int pad1;        // 0x2C
    unsigned char Quat[0x20]; // 0x30
    float Yaw;                // 0x50
    void* Pool;               // 0x54
    xModelInstance* ModelInst; // 0x58
};

class p2EntHangableAsset
{
public:
    unsigned int flags; // 0x0
    float pivotOffset;  // 0x4
    float leverArm;     // 0x8
    float gravity;      // 0xC
    float accel;        // 0x10
    float decay;        // 0x14
    float grabDelay;    // 0x18
    float stopDecel;    // 0x1C
};

class _zEntHangable : public _zEnt
{
public:
    p2EntHangableAsset* hangInfo; // 0xEC
    _xVec3 pivot;                 // 0xF0
    _xVec3 endpos;                // 0xFC
    _xVec3 vel;                   // 0x108
    _xVec3 swingplane;            // 0x114
    float grabTimer;              // 0x120
    float spin;                   // 0x124
    unsigned int state;           // 0x128
    _zEnt* shaggy;                // 0x12C
    signed int enabled;           // 0x130
    _zEnt* follow;                // 0x134
    signed int moving;            // 0x138
    float candle_timer;           // 0x13C
    signed int candle_state;      // 0x140
};

class _zEntPickup : public _zEnt
{
public:
    unsigned int state; // 0xEC
};

class _zGust;

class zGustData
{
public:
    unsigned int gust_on; // 0x0
    _zGust* g[4];         // 0x4
    float lerp[4];        // 0x14
};

class _tagLightning;

class zEGenerator : public _zEnt
{
public:
    unsigned short flags;    // 0xEC
    unsigned short num_dsts; // 0xEE
    float tmr;               // 0xF0
    xAnimFile* afile;        // 0xF4
    _xVec3 src_pos;          // 0xF8
    _xVec3 dst_pos;          // 0x104
    xBase* dst;              // 0x110
    _tagLightning* lfx[2];   // 0x114
};

class p2PlatformSpringboardData
{
public:
    float jmph[3];          // 0x0
    unsigned int animID[3]; // 0xC
    _xVec3 jmpdir;          // 0x18
};

class p2PlatformAsset
{
public:
    unsigned char type;   // 0x0
    unsigned char pad;    // 0x1
    unsigned short flags; // 0x2
    union
    {
        p2PlatformSpringboardData sb; // 0x4
    };
};

class _zPlatform : public _zEnt
{
public:
    p2PlatformAsset* passet;   // 0xEC
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

class p2SimpleObjAsset;

class _zEntSimpleObj : public _zEnt
{
public:
    p2SimpleObjAsset* sasset; // 0xEC
    unsigned int sflags;      // 0xF0
};

class xPar;

class _zEntBubble : public _zEnt
{
public:
    unsigned int state;   // 0xEC
    _zEnt* villain;       // 0xF0
};

class _tagSkidEdge;
class zSFX;

class tagzFeet
{
public:
    signed int total;          // 0x0
    _xEnt* owner;              // 0x4
    signed short entity_type;  // 0x8
    signed short pitch;        // 0xA
};

class st_zEntVillain : public _zEnt
{
public:
    unsigned char pad0EC[0x338 - 0xEC];
    tagzFeet feet;     // 0x338
    zSFX* sfx_curTalk; // 0x344
    _xBound bound2;    // 0x348
};

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
signed int zPitchChange(signed int angle, float* offset, signed int range, float max);
void zLaughNotify(signed int laugh);
void zFeetAddSkidMark(_xEnt* ent, _tagFeetSkidMark* mark, float dt);
void zFeetUpdateSkidMark(_tagFeetSkidMark* mark, float dt);
unsigned int iSndIsPlayingByID(unsigned int id);
extern "C" void RpClumpRemoveAtomic(RpClump* clump, RpAtomic* atomic);
extern "C" void RpClumpAddAtomic(RpClump* clump, RpAtomic* atomic);
unsigned int zSurfaceGetMatchOrient(xSurface* surf);
void xMat4x3Rot(_xMat4x3* m, _xVec3* a, float t, _xVec3* p);
void xMat3x3RMulVec(_xVec3* o, _xMat3x3* m, _xVec3* v);
float xVec3Normalize(_xVec3* o, _xVec3* v);
void zEntPickup_Drop(_zEntPickup* ent);
void zEntHangable_FollowUpdate(_zEntHangable* ent);
void zEntHangable_SetMatrix(_zEntHangable* ent, float dt);
float iurand();
unsigned int zEntDestructObj_Headbutt(_zEntDestructObj* ent);
unsigned int zEntDestructObj_ButtBounce(_zEntDestructObj* ent);
unsigned int zEntDestructObj_CheckState(_zEntDestructObj* ent);
unsigned int zEntButton_Headbutt(_zEntButton* ent);
unsigned int zEntButton_ButtBounce(_zEntButton* ent);
unsigned int zEntButton_Stand(_zEntButton* ent);
unsigned int zEntButton_CheckState(_zEntButton* ent);
signed int Shaggy8_HBCallback(_zEnt* ent, _xCollis* coll);
void xMat3x3GetEuler(_xMat3x3* m, _xVec3* a);
static signed int GetHeadcollAnimState();
void Shaggy0_PlayerStates(xAnimTable* table);
unsigned int Shaggy0_FlipCB(xAnimTransition* tran, xAnimSingle* single, _zEnt* ent);
void Shaggy0_PlayerTransitions(xAnimTable* table);
void Shaggy1_PlayerStates(xAnimTable* table);
void Shaggy1_PlayerTransitions(xAnimTable* table);
void zEntBubble_Launch(float speed);
void zEntShaggy_SetSpin(_zEnt* ent, float spin);
void xAnimTableAddTransition(xAnimTable* table, xAnimTransition* tran, char* source);
static unsigned int FaceIdle(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FaceSwing(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FaceSneak(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FaceTeeter(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FacePant(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FaceRun(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FaceEvent(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FaceScare(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FaceClearScareCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FaceMad(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FaceBlow(xAnimTransition*, xAnimSingle*, void*);
void zHud_showSnack();
void zHud_showSnackSpecial();
void zEntPickup_hideSnackGateInfo(_zEntPickup* ent);
void zEntPickup_UnlockSnackGate(_xEnt* ent);
void zSurfaceGetName(signed int type, char* buffer);
void zFeetInit(signed int, _xEnt* ent, tagzFeet* feet, signed int idx, xAnimTable* table);
void zFeetInitSkidMark(_tagFeetSkidMark* mark, signed int num);
void xAnimStateNewEffect(xAnimState* state, unsigned int flags, float startTime, float endTime,
                         unsigned int (*callback)(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*),
                         unsigned int userDataSize);
signed int zEntPlayerEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                             xBase* toParamWidget);
void zEntPlayer_Update(_xEnt* ent, _xScene* sc, float dt);
void zEntPlayer_Move(_xEnt* ent, _xScene* sc, float dt, _xEntFrame* frame);
void zEntPlayer_Render(_zEnt* ent);
void zEntPlayerReset(_xEnt* ent);
void zEntVillain_Stun(st_zEntVillain* vil, float tym);
unsigned int zEntVillain_Bounce(st_zEntVillain* vil);
signed int zEntVillain_Headbutt(st_zEntVillain* vil, _xVec3* dir);
void zEntBubble_EnableCollision();
void zEntBubble_DisableCollision();
void xEntApplyPhysics(_xEnt* ent, _xScene* sc, float dt);
void xEntMove(_xEnt* ent, _xScene* sc, float dt);
void xEntBeginUpdate(_xEnt* ent, _xScene* sc, float dt);
void xEntEndUpdate(_xEnt* ent, _xScene* sc, float dt);
void xEntDriveUpdate(_xEntDrive* drv, float dt);
void zGustUpdateEnt(_xEnt* ent, _xScene* sc, float dt, void* fdata);
unsigned int zSurfaceGetStep(xSurface* surf);
void zSceneGameStatsTally_death();
void zGameStateSwitch(signed int state);
void Shaggy1_ChooseThrowTarget(_zEnt* ent);
void Shaggy1_PlayerUpdate(_zEnt* ent, _xScene* sc, float dt);
void Shaggy8_CallToPlayer();
signed int Shaggy8_BBCallback(_xEnt* ent, _xCollis* coll);
unsigned int zEntShaggy_GetSwap(_zEnt* ent);
signed int zEntVillain_Bumped(st_zEntVillain* ent, _xCollis* colrec);
static void zEntPlayer_SnackDecrement(float dt);
static void PlayerFaceUpdate(float dt);
static void HeadcollUpdate(float dt);
static void HeadbuttTest(_xCollis* coll, _xCollis* endcoll, float angle_cosine,
                         signed int (*callback)(_zEnt*, _xCollis*));
static signed int VillHBCallback(_zEnt* ent, _xCollis* coll);
void DestructObjBBCheck(_xEnt* ent);
void ButtonBBCheck(_xEnt* ent);
static unsigned int PlayerFacing(_xEnt* ent, _xEnt* target, float cosine);
void zLightAddLocal(_xEnt* ent);
void zLightRemoveLocal();
signed int iModelCull(RpAtomic* model, RwMatrixTag* mat);
void iModelRender(RpAtomic* model, RwMatrixTag* mat);
void iModelSetMaterialAlpha(RpAtomic* model, signed int alpha);
void xEntRender(_xEnt* ent);
void xModelRender(xModelInstance* model);
void xMat3x3RMulRotY(_xMat3x3* o, _xMat3x3* m, float t);
extern float sTimeElapsed;
void zEntEventAll(xBase* from, unsigned int fromEvent, unsigned int toEvent, float* toParam);
void zSceneGameStatsTally_scare();
void xCameraSetController(_tagxCamFrame* cam, float x, float y, float dt);
static void PlayerAbsControl(_xEnt* ent, float x, float z, float dt);
void xLine3VecDist2(_xVec3* p1, _xVec3* p2, _xVec3* v, _xIsect* isx);
unsigned int zSurfaceGetSlide(xSurface* surf);
float zSurfaceGetSlideStartAngle(xSurface* surf);
float zSurfaceGetSlideStopAngle(xSurface* surf);
unsigned int zSurfaceGetSticky(xSurface* surf);
float zSurfaceGetFriction(xSurface* surf);
signed int zSurfaceGetSlickness(xSurface* surf);
float zSurfaceGetDamping(xSurface* surf, float min_vel);
void xRayHitsScene(_xScene* sc, _xRay3* r, _xCollis* coll);
void iRayHitsModel(const _xRay3* r, const xModelInstance* m, _xCollis* coll);
void iRayHitsEnv(const _xRay3* r, const _xEnv* env, _xCollis* coll);
void xMat3x3RotC(_xMat3x3* m, float _x, float _y, float _z, float t);
signed int Shaggy2_EventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam, xBase* toParamWidget);
signed int Shaggy2Plat_EventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                               xBase* toParamWidget);
void zPlatform_Dismount();
void zPlatform_Shake(_zPlatform* plat, float, float ampl, float freq);
void zPlatform_Tremble(_zPlatform* plat, float ampl, float freq, float dur);
static float CalcJumpImpulse_Smooth(float g, float j, float h, float T, float s);
void _iAnimSKBFixTransPop(void* data, void* data2, unsigned int bone);
xAnimState* xAnimTableGetState(xAnimTable* table, char* name);
signed int zGameModeGet();
signed int zGameStateGet();
void iModelTagSetup(iModelTag* tag, RpAtomic* model, float x, float y, float z);
signed int iModelVertCount(RpAtomic* model);
void iModelTagIndex(iModelTag* tag, RpAtomic* model, signed int index);
void xScrFxFade(iColor_tag* base, iColor_tag* dest, float seconds, void (*callback)(), signed int hold,
                signed int unk);
void xAnimTableRemoveEffectByCB(xAnimTable* table,
                                unsigned int (*callback)(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*));
void zFeetExit(_xEnt* ent, xAnimTable* table);
void xEntSetNostepNormAngle(float angle);
void xAnimPlaySetState(xAnimSingle* single, xAnimState* state, float startTime);
void xEntDefaultBoundUpdate(_xEnt* ent, _xVec3* pos);
void zEntPlayer_SNDInit();
static void PlayerBoundUpdate(_xEnt* ent, _xVec3* pos);
static unsigned int PlayerDepenQuery(_xEnt* ent, _xEnt* other, _xScene* sc, float dt, _xCollis* coll);
unsigned int iSphereHitsModel3(const _xSphere* b, const xModelInstance* m, _xCollis* coll,
                               unsigned char ncolls, float sth);
unsigned int xCollideGetCollsIdx(_xVec3* hdng, _xMat3x3* mat);
signed int zSurfaceGetDamageType(xSurface* surf);
unsigned int zSurfaceGetDamagePassthrough(xSurface* surf);
unsigned int zSurfaceGetStandOn(xSurface* surf);
void xScrFxLetterbox(signed int on);
void zSceneGameStatsTally_snack(unsigned int n);
unsigned int zEntPickup_lastSnackInArea();
void zHudShowAllSnacksArea();
void xEntBeginCollCheckTrigs(_xEnt* ent);
void xEntEndCollCheckTrigs(_xEnt* ent);
void xSceneForAllTriggers(_xScene* sc, _xEnt* (*func)(_xEnt*, _xScene*, void*), void* data);
void xEntBeginCollide(_xEnt* ent, _xScene* sc, float dt);
void xEntEndCollide(_xEnt* ent, _xScene* sc, float dt);
void iSphereHitsEnv4(const _xSphere* b, const _xEnv* env, const _xMat3x3* mat, _xCollis* colls);
void xEntCollCheckNPCs(_xEnt* p, _xScene* sc, _xEnt* (*hitIt)(_xEnt*, _xScene*, void*));
void xEntCollCheckDyns(_xEnt* p, _xScene* sc, _xEnt* (*hitIt)(_xEnt*, _xScene*, void*));
void xEntCollCheckStats(_xEnt* p, _xScene* sc, _xEnt* (*hitIt)(_xEnt*, _xScene*, void*));

extern _xVec3 g_O3;
extern _xVec3 g_Y3;

signed int zEntPlayer_Damage(xBase* src, unsigned int damage);
static void PlayerCollsSelectDepen(_xEnt* p);
static _xEnt* PlayerCollCheckOneVillain(_xEnt* ent, _xScene* sc, void* data);
static _xEnt* PlayerCollCheckOneEnt(_xEnt* ent, _xScene* sc, void* data);

static inline void xVec3Cross(_xVec3* o, _xVec3* a, _xVec3* b)
{
    float x = a->y * b->z - b->y * a->z;
    float y = a->z * b->x - b->z * a->x;
    float z = a->x * b->y - b->x * a->y;
    o->x = x;
    o->y = y;
    o->z = z;
}

extern "C" {
signed int rand();
char* strcpy(char* dst, const char* src);
char* strcat(char* dst, const char* src);
signed int xScrFxIsFading();
void* memset(void* dst, int c, unsigned int n);
long labs(long x);
void fn_8000A080(signed int);
int strcmp(const char* a, const char* b);
void* memcpy(void* dst, const void* src, unsigned int n);
double acos(double);
signed int sprintf(char* s, const char* format, ...);
}
void zEntVillain_HeroWasHurt();

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
float startJump;
float startDouble;
float startBounce;
float umbrellaFudgeX = -0.15f;
float umbrellaFudgeY = 0.95f;
float umbrellaFudgeRot = 1.5707964f;
static float minVelmag = 0.01f;
static float maxVelmag = 10.0f;
static float curVelmag;
static float curVelangle;
static signed int surfSlickness = 1;
static float surfFriction = 1.0f;
static float surfDamping = minVelmag;
static signed int lastSlickness = 1;
static _xVec3 lastDeltaPos;
static _xVec3 lastFloorNorm;
static _xEnt* lastFloorEnt;
static unsigned int surfSticky;
static float surfSlideStart = 0.34906587f;
static float surfSlideStop = 0.17453294f;
static float surfSlickRatio;
static float surfSlickTimer;
static float surfPeakRatio = 1.25f;
static float surfAccelWalk = 4.0f;
static float surfAccelRun = 8.0f;
static float surfDecelIdle = 2.0f;
static float surfDecelSkid = 8.0f;
static float surfMaxSpeed;
static float surfSlipTimer;
static float update_dt;
static _xVec3 update_motion;
static _xVec3 req_motion;
_xVec3 vHangBefore;
_xVec3 vHangFirst;
_xVec3 vHangLast;
zGustData gust_data;
_xMat4x3 gPlayerAbsMat;
static _xVec3 headcoll_oses[12] = {
    { 0.0f, 0.0f, 0.15f },   { 0.0f, 0.0f, 0.15f }, { 0.0f, 0.0f, 0.4f },   { 0.0f, 0.0f, 0.3f },
    { 0.0f, 1.15f, -0.15f }, { 0.0f, 1.0f, -0.2f }, { 0.0f, 0.9f, 0.45f },  { 0.0f, 0.75f, 0.5f },
    { 0.0f, 0.9f, -0.25f },  { 0.0f, 0.0f, -0.15f }, { 0.0f, 0.0f, 0.45f }, { 0.0f, 0.0f, 0.5f },
};
static float headcoll_bltimes[12] = { 0.15f, 0.15f, 0.1f,  0.1f,  0.15f, 0.15f,
                                      0.15f, 0.15f, 0.15f, 0.15f, 0.15f, 0.15f };
signed int headcoll_anim;
float headcoll_tmr;
float headcoll_bltime;
_xVec3 headcoll_fromos = { 0.0f, 0.0f, 0.0f };
_xVec3 headcoll_os = { 0.0f, 0.0f, 0.0f };
_xMat4x3 rendermat;
_xVec3 floor_supp[4];
float floor_dist[4];
float floor_tmr[4];
_zParEmitterCustomSettings g_info;
extern zPlayerSituation sPlayerInfo[37];


#define SND_INVALID 0xFFFFFFFF

void zEntPlayer_InitCutsceneModelHack();
static unsigned int zEntPlayer_SNDSkidCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*);
static unsigned int zEntPlayer_FXOpenBrollyCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*);
static unsigned int zEntPlayer_FXSpringCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*);
static unsigned int zEntPlayer_FXDigupCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*);
static unsigned int zEntPlayer_FXScratchCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*);
static void zEntPlayer_FXUpdate(_xEnt* ent, float seconds);
void zEntPlayer_FXSetup(xAnimTable* table);
void zEntPlayer_ButtonHBFX();
void zEntPlayer_ButtonBBFX(_xEnt* ent);
static void zEntPlayer_FXLostChance(_xEnt* ent);
void zEntPlayer_Notify(signed int situation);
void zEntPlayer_Notify(unsigned char characterSubType);
void zEntPlayer_SituationDo();
void zEntPlayer_SNDVan(_xEnt*);
void zEntPlayer_SNDSnackCounter(_xEnt*);
void zEntPlayer_SNDKeyPickup(_xEnt*);
void zEntPlayer_SNDErrorPickup(_xEnt*);
void zEntPlayer_SNDInfoPickup(_xEnt*);
void zEntPlayer_SNDSavePickup(_xEnt*);
void zEntPlayer_SNDTokenPickup(_xEnt*);
void zEntPlayer_SNDSpecialPickup(_xEnt*);
void zEntPlayer_SNDAmmoPickup(_xEnt*);
void zEntPlayer_SNDBitePickup(_xEnt*);
void zEntPlayer_SNDInit();
void zEntPlayer_LoadCheckPoint();
void zEntPlayer_StoreCheckPoint(float* toParam);
static void PlayerRotMatchUpdateEnt(_xEnt* ent, _xScene*, float dt, void* fdata);
static void PlayerDigUpdate(float dt);
static void PlayerTeeterCheck(float dt);
static void PlayerHangCheck(_xEnt* ent, _xScene* sc);
static void PlayerHangUpdate(_xEnt* ent, float mag, float angle, float dt);
signed int zEntPlayerEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam, xBase* toParamWidget);
static _xVec3* GetPosVec(xBase* target);
_xEnt* zEntPlayerCollTriggerCB(_xEnt* ent, _xScene*, void* data);
void zEntPlayerCollTrigger(_xEnt* ent, _xScene* sc);
void zEntPlayerCollide(_xEnt* ent, _xScene* sc, float dt);
static void PlayerCollsSelectDepen(_xEnt* p);
static void PlayerCollsSideTranslate(_xCollis* colls, float x, float y, float z);
static _xEnt* PlayerCollCheckOneVillain(_xEnt* ent, _xScene*, void* data);
static _xEnt* PlayerCollCheckOneEnt(_xEnt* ent, _xScene*, void* data);
void zEntPlayerReset(_xEnt* ent);
void zEntPlayerExit(_xEnt* ent);
void zEntPlayerDriveUpdate(_xEnt* p, float dt);
void zEntPlayerSurfDamageUpdate(_xEnt* p);
static void PlayerBoundUpdate(_xEnt* ent, _xVec3* pos);
static unsigned int PlayerDepenQuery(_xEnt*, _xEnt*, _xScene*, float, _xCollis* coll);
void zEntPlayerFloorUpdate(_xEnt* p, float dt);
void zEntPlayerVelUpdate(_xEnt* ent, float dt);
static void PlayerEGenCheck(_xEnt* ent, zScene* sc, float dt);
void zEntPlayerJumpUpdate(_xEnt* ent, float dt);
void zEntPlayerJumpStart(_xEnt* ent, zJumpParam* jump);
void CalcJumpImpulse(zJumpParam* param);
static float CalcJumpImpulse_Smooth(float g, float j, float h, float Tgc, float Tgs);
void zEntPlayer_GiveHotsauce(unsigned int value);
void zEntPlayer_GiveHealth(unsigned int quantity);
void zEntPlayer_GiveChance(unsigned int quantity);
void zEntPlayer_GiveKey(unsigned int quantity);
void zEntPlayer_GiveClue(unsigned int quantity);
void zEntPlayer_TakeScoobySnack(_xEnt* ent, unsigned int quantity);
signed int zEntPlayer_HasScoobySnack(unsigned int quantity);
void zEntPlayer_GiveScoobySnack(unsigned int quantity);
unsigned int zEntPlayer_MoveInfo();
signed int zEntPlayer_Damage(xBase* src, unsigned int damage);
signed int zEntPlayer_Damage(xBase* src, unsigned int damage, _xVec3* knockback);
void zEntPlayer_Move(_xEnt* ent, _xScene*, float dt, _xEntFrame* frame);
void zEntPlayer_Render(_zEnt* ent);
void zEntPlayer_Update(_xEnt* ent, _xScene* sc, float dt);
static void zEntPlayer_SnackDecrement(float dt);
void zEntPlayer_Load(_xEnt*, xSerial*);
void zEntPlayer_Init(_xEnt* ent, p2EntAsset* asset);
xAnimTable* zEntPlayer_LampshadeAnimTable();
static unsigned int LampFixbugCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int LampbackCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int LampoffCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int LamponCheck(xAnimTransition*, xAnimSingle* anim, void*);
static unsigned int LampDrawCheck(xAnimTransition*, xAnimSingle* anim, void*);
xAnimTable* zEntPlayer_UmbrellaAnimTable();
xAnimTable* zEntPlayer_TongueAnimTable();
xAnimTable* zEntPlayer_HeadAnimTable();
static void PlayerFaceUpdate(float dt);
static unsigned int FaceIdle(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FaceSwing(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FaceSneak(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FaceTeeter(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FacePant(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FaceRun(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FaceEvent(xAnimTransition* tran, xAnimSingle*, void*);
static unsigned int FaceScare(xAnimTransition* tran, xAnimSingle*, void*);
static unsigned int FaceClearScareCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FaceMad(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FaceBlow(xAnimTransition*, xAnimSingle*, void*);
xAnimTable* zEntPlayer_AnimTable();
static inline signed int DestructObjHBCallback(_zEnt* ent, _xCollis* coll);
void DestructObjBBCheck(_xEnt* ent);
void ButtonBBCheck(_xEnt* ent);
static inline signed int ButtonHBCallback(_zEnt* ent, _xCollis* coll);
static void PlayerAddToAllStates(xAnimTable* table, xAnimTransition* tran);
static unsigned int SD0_TossCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int ClearHelmetCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FallToDeathDropCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FallToDeathDrop(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FallToDeathStart(xAnimTransition*, xAnimSingle*, void*);
static unsigned int DeathCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int HangNoSwingCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int HangSwingCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int HangCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int PowerupCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int PowerupCheck(xAnimTransition* tran, xAnimSingle*, void*);
static unsigned int NoHangCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int HangCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int NotFloatCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int FloatCheck(xAnimTransition* tran, xAnimSingle*, void*);
static unsigned int SlideCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int ButtSmashStateCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int ButtSmashCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int ButtSmashCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int HeadHitCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int HeadHitCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int AttackFinishedHotsauce(xAnimTransition*, xAnimSingle*, void*);
static unsigned int AttackFinishedRun(xAnimTransition* tran, xAnimSingle* anim, void* data);
static unsigned int AttackFinished(xAnimTransition* tran, xAnimSingle* anim, void* data);
static unsigned int AttackCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int AttackCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int ProjectileCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int ProjectileRepeat(xAnimTransition* tran, xAnimSingle* anim, void* data);
static unsigned int ProjectileRepeatSync(xAnimTransition* tran, xAnimSingle* anim, void* data);
static unsigned int ProjectileCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int StopDigCheck(xAnimTransition* tran, xAnimSingle* anim, void* data);
static unsigned int DigCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int DigCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int NospringCheck(xAnimTransition*, xAnimSingle*, _zEnt* ent);
static unsigned int SpringCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int ButtSmashLandCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int BounceCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int BounceCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int DblJumpCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int DblJumpCheckTime(xAnimTransition*, xAnimSingle* anim, void*);
static unsigned int DblJumpCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int JumpHangCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int JumpCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int JumpCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int ActiveCheck(xAnimTransition* tran, xAnimSingle* anim, void* data);
static unsigned int IdleGetupCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int IdleSitCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int IdleMajorCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int IdleMinorCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int IdleGetupCheck(xAnimTransition*, xAnimSingle* anim, void*);
static unsigned int IdleSitCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int IdleMajorCheck(xAnimTransition* tran, xAnimSingle*, void*);
static unsigned int IdleMinorCheck(xAnimTransition* tran, xAnimSingle*, void*);
static unsigned int FallingCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int LandHealthCheck(xAnimTransition* tran, xAnimSingle* anim, void* data);
static unsigned int LandFastCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int LandWalkCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int LandSlowCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int ScareLandCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int LandCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int ScareKnockbackCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int ScareCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int ScareCheckCritter(xAnimTransition*, xAnimSingle*, void*);
static unsigned int ScareCheckNoHealth(xAnimTransition*, xAnimSingle*, void*);
static unsigned int UncowerCheck(xAnimTransition* tran, xAnimSingle* anim, void* data);
static unsigned int CowerCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int ScareCheck(xAnimTransition* tran, xAnimSingle* anim, void* data);
static unsigned int NosneakPlunger(xAnimTransition* tran, xAnimSingle* anim, void* data);
static unsigned int NosneakSlippery(xAnimTransition* tran, xAnimSingle* anim, void* data);
static unsigned int NosneakTeeter(xAnimTransition* tran, xAnimSingle* anim, void* data);
static unsigned int NosneakCheck(xAnimTransition* tran, xAnimSingle* anim, void* data);
static unsigned int SneakIdleCheck(xAnimTransition* tran, xAnimSingle*, void*);
static unsigned int SneakMoveCheck(xAnimTransition* tran, xAnimSingle*, void*);
static unsigned int SkidStopCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int IdleCheck(xAnimTransition* tran, xAnimSingle* anim, void* data);
static unsigned int WalkPlungeCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int StickNoBootsWalk(xAnimTransition*, xAnimSingle*, void*);
static unsigned int StickNoBootsIdle(xAnimTransition*, xAnimSingle*, void*);
static unsigned int StickySurfWalk(xAnimTransition*, xAnimSingle*, void*);
static unsigned int StickySurfIdle(xAnimTransition*, xAnimSingle*, void*);
static unsigned int WalkCheck(xAnimTransition* tran, xAnimSingle*, void*);
static unsigned int NotRunCB(xAnimTransition*, xAnimSingle*, void*);
static unsigned int NotRunCheck(xAnimTransition* tran, xAnimSingle*, void*);
static unsigned int RunCheck(xAnimTransition* tran, xAnimSingle*, void*);
static unsigned int HotsauceCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int SlipperyCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int TeeterCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int PlungerCheck(xAnimTransition*, xAnimSingle*, void*);
static unsigned int HealthCheck(xAnimTransition* tran, xAnimSingle*, void*);
static signed int VillHBCallback(_zEnt* ent, _xCollis* coll);
static void HeadbuttTest(_xCollis* coll, _xCollis* endcoll, float angle_cosine, signed int (*headbuttCB)(_zEnt*, _xCollis*));
static void HeadcollUpdate(float dt);
static signed int GetHeadcollAnimState();
static void PlayerAbsControl(_xEnt* ent, float x, float z, float dt);
static unsigned int PlayerFacing(_xEnt* ent, _xEnt* target, float cosine);



static inline void zEntPlayer_SNDButtBounce(signed int hit_villain)
{
    float pitch;
    if (hit_villain)
    {
        sPlayerSndfx.head_pitch = zPitchChange(sPlayerSndfx.head_pitch, &pitch, 0x1E, 1.5f);
        xSndPlay(sPlayerSndfx.head_id[sPlayerSndfx.head_snd], 0.5f, 0.5f, pitch, 0x80, 2, 0);
        signed int newsnd = sPlayerSndfx.head_snd;
        while (newsnd == sPlayerSndfx.head_snd)
        {
            newsnd = rand() % 3;
        }
        sPlayerSndfx.head_snd = newsnd;
    }
    else
    {
        sPlayerSndfx.butt_pitch = zPitchChange(sPlayerSndfx.butt_pitch, &pitch, 0x1E, 1.5f);
        xSndPlay(sPlayerSndfx.butt_id[sPlayerSndfx.butt_snd], 0.5f, 0.5f, pitch, 0x80, 2, 0);
        signed int newsnd = sPlayerSndfx.butt_snd;
        while (newsnd == sPlayerSndfx.butt_snd)
        {
            newsnd = rand() % 3;
        }
        sPlayerSndfx.butt_snd = newsnd;
    }
}

static inline void PlayerCollisTranslate(_xCollis* c, float x, float y, float z)
{
    if (c->dist < 1e19f && (c->depen.x != 0.0f || c->depen.y != 0.0f || c->depen.z != 0.0f))
    {
        float dx = c->tohit.x - x;
        float dy = c->tohit.y - y;
        float dz = c->tohit.z - z;
        c->tohit.x = dx;
        c->tohit.y = dy;
        c->tohit.z = dz;
        float dist = xsqrt(dx * dx + dy * dy + dz * dz);
        float s = 1.0f / dist;
        c->dist = dist;
        c->hdng.x = dx * s;
        c->hdng.y = dy * s;
        c->hdng.z = dz * s;
        s = s * (dist - 0.7f);
        s = (s > 0.0f) ? 0.0f : s;
        c->depen.x = dx * s;
        c->depen.y = dy * s;
        c->depen.z = dz * s;
    }
}

static inline void PlayerCollsWallsTranslate(_xCollis* colls, float x, float y, float z)
{
    _xCollis* c;
    _xCollis* cend;
    for (c = colls + 2, cend = colls + 6; c < cend; c++)
    {
        PlayerCollisTranslate(c, x, y, z);
    }
}

static inline void PlayerCollsAllTranslate(_xCollis* colls, float x, float y, float z)
{
    _xCollis* c;
    _xCollis* cend;
    for (c = colls, cend = colls + 6; c < cend; c++)
    {
        PlayerCollisTranslate(c, x, y, z);
    }
}

static inline void PlayerMinstShow(xModelInstance* m)
{
    if (m->Flags & 2)
    {
        m->Flags |= 1;
    }
    else
    {
        m->Flags |= 2;
    }
}

static inline signed int PlayerFpClassify(float value)
{
    unsigned long integer = *(unsigned long*)&value;
    switch (integer & 0x7F800000)
    {
    case 0x7F800000:
        if ((integer & 0x7FFFFF) != 0)
        {
            return 1;
        }
        return 2;
    case 0:
        if ((integer & 0x7FFFFF) != 0)
        {
            return 5;
        }
        return 3;
    }
    return 4;
}

static inline _zParEmitter* zEntPlayerFindSurfEmitter(char* kind, signed int surftype)
{
    char emitname[128];
    char surfname[128];
    strcpy(emitname, "PAREMIT_SD_");
    strcat(emitname, kind);
    zSurfaceGetName(surftype, surfname);
    strcat(emitname, surfname);
    _zParEmitter* emitter = zParEmitterFind(xStrHash(emitname));
    if (emitter != NULL && (emitter->emit_flags & 1))
    {
        emitter->emit_flags ^= 1;
    }
    return emitter;
}

static inline void zEntPlayerFindEmitters()
{
    signed int i;
    for (i = 0; i < 23; i++)
    {
        gPlayerfx.emit_land[i] = zEntPlayerFindSurfEmitter("LAND_", i);
        gPlayerfx.emit_stop[i] = zEntPlayerFindSurfEmitter("STOP_", i);
    }
    gPlayerfx.emit_scratch = zParEmitterFind("PAREMIT_SD_FLEASCRATCH");
    gPlayerfx.emit_digup[0] = zParEmitterFind("PAREMIT_SD_DIGUPDUST");
    gPlayerfx.emit_digup[1] = zParEmitterFind("PAREMIT_SD_DIGUPDIRT");
    sEmitterSprings = zParEmitterFind("PAREMIT_SPRING_THING");
    sEmitterShockLeft = zParEmitterFind("PAREMIT_SD_SHOCKLEFT");
    sEmitterShockRight = zParEmitterFind("PAREMIT_SD_SHOCKRIGHT");
    sEmitterWaterSplash = zParEmitterFind("PAREMIT_SD_WATERSPLASH");
    sEmitterBounceMiss = zParEmitterFind("PAREMIT_SD_BOUNCEMISS");
    sEmitterSpringBoard = zParEmitterFind("PAREMIT_PLAT_SPRINGBOARD");
    sEmitterBounceOnVillain = zParEmitterFind("PAREMIT_SD_BOUNCEONVILLAIN");
    sEmitterHeadButt = zParEmitterFind("PAREMIT_SD_HEADBUTT");
    sEmitterHeadButt2 = zParEmitterFind("PAREMIT_SD_HEADBUTT2");
    sEmitterStars = zParEmitterFind("PAREMIT_STARS");
    sSoundWaterSplash = xStrHash("scooby_splash");
}

static inline void PlayerHeadAddTransition(xAnimTable* table, char* dest, xAnimTransitionConditionalCallback cond,
                                           xAnimTransitionCallback cb, unsigned int flags, unsigned int userFlags,
                                           unsigned short priority)
{
    xAnimState* state;
    xAnimTransition* tran =
        xAnimTableNewTransition(table, "", dest, cond, cb, flags, userFlags, 0.0f, 0.0f, priority, 0, 0.0f, NULL);
    state = table->StateList;
    while (state != NULL)
    {
        if (xStricmp(state->Name, tran->Dest->Name) != 0)
        {
            xAnimTableAddTransition(table, tran, state->Name);
        }
        state = state->Next;
    }
}

static inline unsigned char PlayerIsDigging()
{
    return globals.player.DigTimer != 0.0f;
}

static inline unsigned int PlayerSurfaceAnimCheck(xAnimTransition* tran)
{
    unsigned int bit;
    if (globals.player.Slide == 2)
    {
        bit = tran->UserFlags & 0x20;
    }
    else if (surfSlickTimer != 0.0f)
    {
        bit = tran->UserFlags & 0x10;
    }
    else if (surfSticky)
    {
        if (globals.player.Inv_Specials & 4)
        {
            bit = tran->UserFlags & 0x80;
        }
        else
        {
            bit = tran->UserFlags & 0x40;
        }
    }
    else
    {
        bit = HealthCheck(tran, NULL, NULL);
    }
    return bit != 0;
}

static inline void PlayerScareReset()
{
    globals.player.ScareTimer = 0.0f;
    globals.player.CowerTimer = 0.0f;
    if (globals.player.JumpState != 0)
    {
        globals.player.JumpState = 2;
        globals.player.Jump_CanFloat = 0;
        globals.player.Jump_CanDouble = 0;
    }
    zEntPlayer_FXLostChance(&globals.player.ent);
    zRumbleStart(0, (_tagSDRumbleType)7);
}

static inline void DampenControls(float* angle, float* mag, float x, float y)
{
    if (x > -(signed int)globals.player.s.AnalogMin && x < globals.player.s.AnalogMin)
    {
        x = 0.0f;
    }
    if (y > -(signed int)globals.player.s.AnalogMin && y < globals.player.s.AnalogMin)
    {
        y = 0.0f;
    }
    *angle = 0.0f;
    if (x == 0.0f && y == 0.0f)
    {
        *mag = 0.0f;
        return;
    }
    float ax = (float)__fabs(x);
    float ay = (float)__fabs(y);
    float m = (ax > ay) ? ax : ay;
    *angle = 0.0f;
    *mag = (m - globals.player.s.AnalogMin) / (signed int)(globals.player.s.AnalogMax - globals.player.s.AnalogMin);
    if (*mag < 0.0f)
    {
        *mag = 0.0f;
        return;
    }
    if (*mag > 1.0f)
    {
        *mag = 1.0f;
    }
    *angle = xAngleClamp(xatan2(x, y));
}

static inline float PlayerLeanRange(float x, float lo, float hi)
{
    if (x < lo)
    {
        return 0.0f;
    }
    if (x > hi)
    {
        return 1.0f;
    }
    return (x - lo) * (1.0f / (hi - lo));
}

static inline void PlayerAutoMoveDone(_xEnt* ent)
{
    globals.player.AutoMoveSpeed = 0;
    xBase* obj = globals.player.AutoMoveObject;
    zEntEvent(obj, ent, 0x1F);
    if (obj->baseType == 0xD)
    {
        zEntEvent(ent, obj, 0x1F);
    }
}

static unsigned int PlayerFacing(_xEnt* ent, _xEnt* target, float cosine)
{
    float fx = ent->frame->mat.at.x;
    float fz = ent->frame->mat.at.z;
    float flen = xsqrt(fx * fx + fz * fz);
    float dx = target->frame->mat.pos.x - ((_xMat4x3*)ent->model->Mat)->pos.x;
    float dz = target->frame->mat.pos.z - ((_xMat4x3*)ent->model->Mat)->pos.z;
    float dlen = xsqrt(dx * dx + dz * dz);
    return fx * dx + fz * dz > cosine * flen * dlen;
}

static void PlayerAbsControl(_xEnt* ent, float x, float z, float dt)
{
    float diffAngle = 0.0f;
    float autodist2d = 0.0f;
    float turnfactor;
    float mag;
    float angle;
    _xVec3 euler;

    if (gTrcPad[0].state != 2)
    {
        x = 0.0f;
        z = 0.0f;
    }
    if (!globals.player.ControlOn)
    {
        x = 0.0f;
        z = 0.0f;
    }

    xMat3x3GetEuler((_xMat3x3*)ent->frame, &euler);
    if (euler.x >= 0.0f)
    {
    }
    else
    {
        euler.x += 6.2831855f;
    }
    ent->frame->rot.angle = euler.x;
    surfMaxSpeed = 0.0f;

    xAnimSingle* single = ent->model->Anim->Single;
    unsigned int animUserFlag = (single->State != NULL) ? single->State->UserFlags : 0;
    xAnimSingle* blend = single->Blend;
    unsigned char hasblend = 0;
    if (blend != NULL && blend->State != NULL)
    {
        hasblend = 1;
    }
    unsigned int blendUserFlag = hasblend ? (blend->State->UserFlags | 0x80000000) : 0;
    unsigned int moveFlag = animUserFlag;
    unsigned int blendFlag = blendUserFlag;
    if ((animUserFlag & 0x1E) == 0xE && !(animUserFlag & 0x800))
    {
        moveFlag &= ~0x1E;
    }
    if ((blendUserFlag & 0x1E) == 0xE && !(blendUserFlag & 0x800))
    {
        blendFlag &= ~0x1E;
    }
    float tmpdrot = ((moveFlag & 0x1E) == 0xC) ? globals.player.s.HeadbuttTurn : 7.0f;
    memset(&ent->frame->dpos, 0, sizeof(_xVec3));

    if (moveFlag & 0x100)
    {
        return;
    }

    DampenControls(&angle, &mag, x, z);
    memcpy(&gPlayerAbsMat, &globals.camera.hi_cam.mat, sizeof(_xMat4x3));

    if (globals.player.Slide == 1 && (globals.player.JumpState == 0 || globals.player.JumpState == 1))
    {
        _xVec3 X;
        _xVec3 D;
        _xVec3* N = &globals.player.floor_norm;
        zLaughNotify(8);
        X.y = 0.0f;
        X.z = N->x;
        X.x = -N->z;
        xVec3Normalize(&X, &X);
        xVec3Cross(&D, N, &X);
        xVec3Normalize(&D, &D);
        if (globals.player.SlideTimer >= 0.3f)
        {
            _xMat3x3 N_to_Up;
            _xVec3 D_flat;
            xMat3x3RotC(&N_to_Up, X.x, X.y, X.z, acos(N->y));
            xMat3x3RMulVec(&D_flat, &N_to_Up, &D);
            float downAngle = xAngleClamp(xatan2(D_flat.x, D_flat.z)) - ent->frame->rot.angle;
            if (downAngle > 3.1415927f)
            {
                downAngle -= 6.2831855f;
            }
            else if (downAngle < -3.1415927f)
            {
                downAngle += 6.2831855f;
            }
            if ((float)__fabs(downAngle) > 0.08726647f)
            {
                ent->frame->drot.angle = 5.0f * downAngle * dt;
                ent->frame->mode |= 0x20;
            }
            float peakLerp = 0.0f;
            if (mag != 0.0f)
            {
                float camAngle = angle - xAngleClamp(xatan2(gPlayerAbsMat.right.z, gPlayerAbsMat.right.x));
                if (camAngle > 6.2831855f)
                {
                    camAngle -= 6.2831855f;
                }
                else if (camAngle < 0.0f)
                {
                    camAngle += 6.2831855f;
                }
                float accelMag = 5.0f * dt;
                float s = mag * accelMag;
                float dir_dp = X.x * (s * isin(camAngle)) + X.y * 0.0f + X.z * (s * icos(camAngle));
                ent->frame->dpos.x = X.x * dir_dp;
                ent->frame->dpos.y = X.y * dir_dp;
                ent->frame->dpos.z = X.z * dir_dp;
                ent->frame->mode |= 2;
                peakLerp = PlayerLeanRange((float)__fabs(dir_dp), dt, accelMag);
                if (dir_dp < 0.0f)
                {
                    peakLerp = -peakLerp;
                }
                globals.player.LeanLerp += 6.0f * ((peakLerp + 1.0f) - globals.player.LeanLerp) * dt;
            }
            else
            {
                peakLerp = PlayerLeanRange((float)__fabs(0.0), dt, 5.0f * dt);
                globals.player.LeanLerp += 6.0f * ((peakLerp + 1.0f) - globals.player.LeanLerp) * dt;
            }
        }
        else
        {
            float peakLerp = PlayerLeanRange((float)__fabs(0.0), dt, 5.0f * dt);
            globals.player.LeanLerp += 6.0f * ((peakLerp + 1.0f) - globals.player.LeanLerp) * dt;
        }
        ent->model->Anim->Single->BilinearLerp[0] = globals.player.LeanLerp;
        ent->model->Anim->Single->Blend->BilinearLerp[0] = globals.player.LeanLerp;
        _xVec3* vel = &ent->frame->vel;
        if (vel->x * vel->x + vel->y * vel->y + vel->z * vel->z < 49.0f)
        {
            float s = dt * (0.45f * globals.sceneCur->gravity * D.y);
            ent->frame->dvel.x = D.x * s;
            ent->frame->dvel.y = D.y * s;
            ent->frame->dvel.z = D.z * s;
            ent->frame->mode |= 8;
        }
        return;
    }

    if (globals.player.HotsauceTimer != 0.0f)
    {
        globals.player.HotsauceTimer -= dt;
        if (globals.player.HotsauceTimer <= 0.0f)
        {
            globals.player.HotsauceTimer = 0.0f;
        }
        else
        {
            moveFlag = 8;
            if (mag < globals.player.s.SpeedHotsauce[2])
            {
                mag = globals.player.s.SpeedHotsauce[2];
                turnfactor = 1.0f;
                angle = ent->frame->rot.angle;
                globals.player.Speed = 3;
                goto domove;
            }
        }
    }

    if (globals.player.Shaggy1.GrabTimer != 0.0f)
    {
        diffAngle = globals.player.Shaggy1.GrabRot - ent->frame->rot.angle;
        if (diffAngle > 3.1415927f)
        {
            diffAngle -= 6.2831855f;
        }
        else if (diffAngle < -3.1415927f)
        {
            diffAngle += 6.2831855f;
        }
        ent->frame->drot.angle = 4.0f * diffAngle * dt;
        ent->frame->mode |= 0x20;
        if ((float)__fabs(ent->frame->drot.angle) < 0.006f)
        {
            if ((float)__fabs(diffAngle) > 0.006f)
            {
                if (diffAngle > 0.0f)
                {
                    diffAngle = 0.006f;
                }
                else
                {
                    diffAngle = -0.006f;
                }
            }
            ent->frame->drot.angle = diffAngle;
        }
        goto lean;
    }

    if (globals.player.FireTarget != NULL)
    {
        _xVec3 delta;
        delta.x = globals.player.FireTarget->bound.sph.center.x - ent->bound.sph.center.x;
        delta.y = globals.player.FireTarget->bound.sph.center.y - ent->bound.sph.center.y;
        delta.z = globals.player.FireTarget->bound.sph.center.z - ent->bound.sph.center.z;
        xVec3Normalize(&delta, &delta);
        float dx = isin(ent->frame->rot.angle);
        float dz = icos(ent->frame->rot.angle);
        if (dx * delta.x + 0.0f * delta.y + dz * delta.z < 0.34202f)
        {
            globals.player.FireTarget = NULL;
        }
        else
        {
            angle = xAngleClamp(xatan2(delta.x, delta.z));
            if (angle > 6.2831855f)
            {
                angle -= 6.2831855f;
            }
            else if (angle < 0.0f)
            {
                angle += 6.2831855f;
            }
            if (mag != 0.0f)
            {
                goto turn;
            }
            diffAngle = angle - ent->frame->rot.angle;
            if (diffAngle > 3.1415927f)
            {
                diffAngle -= 6.2831855f;
            }
            else if (diffAngle < -3.1415927f)
            {
                diffAngle += 6.2831855f;
            }
            ent->frame->drot.angle = dt * (diffAngle * tmpdrot);
            ent->frame->mode |= 0x20;
        }
    }

    if (globals.player.Shaggy1.Grab != NULL &&
        (globals.player.Shaggy1.ThrowState == 1 || globals.player.Shaggy1.ThrowState == 2))
    {
        _xVec3 delta;
        delta.x = globals.player.Shaggy1.ThrowTgtVec.x - ent->bound.sph.center.x;
        delta.y = globals.player.Shaggy1.ThrowTgtVec.y - ent->bound.sph.center.y;
        delta.z = globals.player.Shaggy1.ThrowTgtVec.z - ent->bound.sph.center.z;
        xVec3Normalize(&delta, &delta);
        float dx = isin(ent->frame->rot.angle);
        float dz = icos(ent->frame->rot.angle);
        float turnang = xAngleClamp(xatan2(delta.x, delta.z));
        if (turnang > 6.2831855f)
        {
            turnang -= 6.2831855f;
        }
        else if (turnang < 0.0f)
        {
            turnang += 6.2831855f;
        }
        diffAngle = turnang - ent->frame->rot.angle;
        if (diffAngle > 3.1415927f)
        {
            diffAngle -= 6.2831855f;
        }
        else if (diffAngle < -3.1415927f)
        {
            diffAngle += 6.2831855f;
        }
        float drot = diffAngle;
        float maxrot = 12.566371f * dt;
        if ((float)__fabs(diffAngle) <= maxrot)
        {
            globals.player.Shaggy1.ThrowState = 2;
        }
        else
        {
            drot = diffAngle * tmpdrot;
            if ((float)__fabs(drot) < 12.566371f)
            {
                if (drot > 0.0f)
                {
                    drot = maxrot;
                }
                else
                {
                    drot = 12.566371f * -dt;
                }
            }
            else
            {
                drot = drot * dt;
            }
        }
        ent->frame->drot.angle = drot;
        ent->frame->mode |= 0x20;
        globals.player.Speed = 0;
        goto lean;
    }

    if (moveFlag & 0x80)
    {
        angle = ent->frame->rot.angle;
        if (mag != 0.0f)
        {
            goto turn;
        }
    }

    if (!globals.player.ControlOn && globals.player.AutoMoveSpeed)
    {
        float dx = globals.player.AutoMoveTarget.x - ((_xMat4x3*)ent->model->Mat)->pos.x;
        float dz = globals.player.AutoMoveTarget.z - ((_xMat4x3*)ent->model->Mat)->pos.z;
        autodist2d = xsqrt(dx * dx + dz * dz);
        if (autodist2d < 0.001f)
        {
            PlayerAutoMoveDone(ent);
        }
        else
        {
            angle = xAngleClamp(xatan2(dx, dz));
            if (angle > 6.2831855f)
            {
                angle -= 6.2831855f;
            }
            else if (angle < 0.0f)
            {
                angle += 6.2831855f;
            }
            diffAngle = angle - ent->frame->rot.angle;
            if (diffAngle > 3.1415927f)
            {
                diffAngle -= 6.2831855f;
            }
            else if (diffAngle < -3.1415927f)
            {
                diffAngle += 6.2831855f;
            }
            float drot = diffAngle * tmpdrot;
            if ((float)__fabs(drot) < 0.19634955f)
            {
                if (drot < 0.0f)
                {
                    drot = -0.19634955f;
                }
                else
                {
                    drot = 0.19634955f;
                }
            }
            drot *= dt;
            if ((float)__fabs(drot) > (float)__fabs(diffAngle))
            {
                drot = diffAngle;
            }
            ent->frame->drot.angle = drot;
            ent->frame->mode |= 0x20;
            if (globals.player.AutoMoveSpeed == 4)
            {
                if (drot == diffAngle)
                {
                    PlayerAutoMoveDone(ent);
                }
                turnfactor = 0.0f;
                mag = 0.0f;
            }
            else if (autodist2d <= globals.player.AutoMoveDist)
            {
                PlayerAutoMoveDone(ent);
            }
            else
            {
                mag = 1.0f;
                turnfactor = icos(diffAngle);
            }
            goto speed;
        }
    }

    if (mag == 0.0f)
    {
        turnfactor = 0.0f;
        globals.player.Speed = 0;
        goto domove;
    }
    angle -= xAngleClamp(xatan2(gPlayerAbsMat.right.z, gPlayerAbsMat.right.x));
    if (angle > 6.2831855f)
    {
        angle -= 6.2831855f;
    }
    else if (angle < 0.0f)
    {
        angle += 6.2831855f;
    }

turn:
    if (!(moveFlag & 0x800))
    {
        diffAngle = angle - ent->frame->rot.angle;
        if (diffAngle > 3.1415927f)
        {
            diffAngle -= 6.2831855f;
        }
        else if (diffAngle < -3.1415927f)
        {
            diffAngle += 6.2831855f;
        }
        turnfactor = icos(diffAngle);
        ent->frame->drot.angle = dt * (diffAngle * tmpdrot);
        ent->frame->mode |= 0x20;
    }

speed:
    if (moveFlag & 0x800)
    {
        turnfactor = 1.0f;
    }
    if (surfSticky && !(globals.player.Inv_Specials & 4))
    {
        if (turnfactor > 0.2f)
        {
            globals.player.Speed = 1;
        }
        else
        {
            globals.player.Speed = 0;
        }
    }
    else if (globals.player.HotsauceTimer != 0.0f)
    {
        globals.player.Speed = 3;
    }
    else if (((globals.player.ControlOn && (globals.pad0->on & 0x1000)) || globals.player.AutoMoveSpeed == 2) &&
             globals.player.Sneak == 0)
    {
        globals.player.Speed = 2;
    }
    else if (turnfactor > 0.2f || globals.player.Speed != 0)
    {
        globals.player.Speed = 1;
    }

domove:
    ent->frame->mode &= ~2;
    if (globals.player.HangEnt != NULL)
    {
        zLaughNotify(7);
        PlayerHangUpdate(ent, mag, angle, dt);
        return;
    }

    if ((moveFlag & 0x1E) || (blendFlag & 0x1E))
    {
        unsigned int type = moveFlag & 0x1E;
        if (type == 0)
        {
            type = blendFlag & 0x1E;
        }
        switch (type)
        {
        case 0:
            break;
        case 2:
            if (turnfactor <= 0.0f || mag == 0.0f)
            {
                goto lean;
            }
            maxVelmag = globals.player.s.SpeedSneak[1];
            mag *= maxVelmag * turnfactor;
            break;
        case 4:
            if (turnfactor <= 0.0f)
            {
                goto lean;
            }
            maxVelmag = globals.player.s.SpeedWalk[1];
            mag *= maxVelmag * turnfactor;
            if (surfSticky && !(globals.player.Inv_Specials & 4))
            {
                mag *= 0.667f;
            }
            break;
        case 6:
            if (mag != 0.0f)
            {
                if ((float)__fabs(diffAngle) >= 0.5235988f)
                {
                    if (diffAngle > 0.0f)
                    {
                        angle = 0.5235988f + ent->frame->rot.angle;
                    }
                    else
                    {
                        angle = ent->frame->rot.angle - 0.5235988f;
                    }
                }
            }
            else
            {
                angle = ent->frame->rot.angle;
            }
            if (moveFlag & 0x20)
            {
                if (surfSlickRatio != 0.0f)
                {
                    goto lean;
                }
                mag = globals.player.DecelRunSpeed;
                globals.player.DecelRunSpeed = mag - 1.3333334f * (globals.player.DecelRun * dt);
                if (globals.player.DecelRunSpeed < 0.0f)
                {
                    globals.player.DecelRunSpeed = 0.0f;
                }
            }
            else
            {
                if (mag == 0.0f)
                {
                    goto lean;
                }
                float speed;
                maxVelmag = globals.player.s.SpeedRun[1];
                if (mag >= globals.player.s.SpeedRun[3])
                {
                    speed = maxVelmag;
                }
                else if (mag <= globals.player.s.SpeedRun[2])
                {
                    speed = globals.player.s.SpeedRun[0];
                }
                else
                {
                    speed = ((mag - globals.player.s.SpeedRun[2]) /
                             (globals.player.s.SpeedRun[3] - globals.player.s.SpeedRun[2])) *
                                (maxVelmag - globals.player.s.SpeedRun[0]) +
                            globals.player.s.SpeedRun[0];
                }
                mag = speed;
                globals.player.DecelRunSpeed = speed;
                globals.player.DecelRun = speed;
            }
            break;
        case 8:
        {
            float speed;
            maxVelmag = globals.player.s.SpeedHotsauce[1];
            if (mag >= globals.player.s.SpeedHotsauce[3])
            {
                speed = maxVelmag;
            }
            else if (mag <= globals.player.s.SpeedHotsauce[2])
            {
                speed = globals.player.s.SpeedHotsauce[0];
            }
            else
            {
                speed = ((mag - globals.player.s.SpeedHotsauce[2]) /
                         (globals.player.s.SpeedHotsauce[3] - globals.player.s.SpeedHotsauce[2])) *
                            (maxVelmag - globals.player.s.SpeedHotsauce[0]) +
                        globals.player.s.SpeedHotsauce[0];
            }
            mag = speed;
            globals.player.DecelRunSpeed = speed;
            globals.player.DecelRun = speed;
            break;
        }
        case 10:
        case 14:
            if (turnfactor <= 0.0f && globals.player.cheat_mode == 0)
            {
                goto lean;
            }
            maxVelmag = globals.player.s.SpeedAir[1];
            mag *= maxVelmag * turnfactor;
            break;
        case 12:
            angle = ent->frame->rot.angle;
            mag = globals.player.HeadbuttVel;
            globals.player.DecelRunSpeed = mag;
            globals.player.DecelRun = mag;
            break;
        }

        switch (moveFlag & 0x1E)
        {
        case 2:
            if ((strcmp(ent->model->Anim->Single->State->Name, "Walk_sneak") == 0 ||
                 strcmp(ent->model->Anim->Single->State->Name, "Walk_blackknight") == 0) &&
                globals.player.s.AnimSneak[0] != 0.0f)
            {
                float s = mag / globals.player.s.AnimSneak[0];
                if (s < globals.player.s.AnimSneak[1])
                {
                    s = globals.player.s.AnimSneak[1];
                }
                else if (s > globals.player.s.AnimSneak[2])
                {
                    s = globals.player.s.AnimSneak[2];
                }
                ent->model->Anim->Single->CurrentSpeed = s;
            }
            break;
        case 3:
            break;
        case 4:
            if (globals.player.s.AnimWalk[0] != 0.0f)
            {
                float s = mag / globals.player.s.AnimWalk[0];
                if (s < globals.player.s.AnimWalk[1])
                {
                    s = globals.player.s.AnimWalk[1];
                }
                else if (s > globals.player.s.AnimWalk[2])
                {
                    s = globals.player.s.AnimWalk[2];
                }
                ent->model->Anim->Single->CurrentSpeed = s;
            }
            if (ent->model->Anim->Single->State->Speed != 1.0f)
            {
                ent->model->Anim->Single->CurrentSpeed = ent->model->Anim->Single->State->Speed;
            }
            break;
        }

        if (surfSlickRatio != 0.0f && globals.player.ControlOn)
        {
            if (mag == 0.0f)
            {
                goto lean;
            }
            float slickLerp = 1.3333334f * surfSlipTimer;
            float accelMag = (1.0f - slickLerp) * surfSlickRatio + 20.0f * slickLerp * surfSlickRatio;
            float accel;
            if (type == 4 || type == 2)
            {
                accel = surfAccelWalk * accelMag;
                surfMaxSpeed = globals.player.s.SpeedWalk[1] * surfPeakRatio;
            }
            else
            {
                accel = surfAccelRun * accelMag;
                surfMaxSpeed = globals.player.s.SpeedRun[1] * surfPeakRatio;
            }
            ent->frame->vel.x += dt * (accel * isin(angle));
            ent->frame->vel.z += dt * (accel * icos(angle));
            float peakLerp = 2.5f * surfSlipTimer;
            if (peakLerp >= 1.0f)
            {
                surfMaxSpeed = mag;
            }
            else
            {
                surfMaxSpeed = (1.0f - peakLerp) * surfMaxSpeed + peakLerp * mag;
            }
        }
        else
        {
            mag *= dt;
            if (globals.player.cheat_mode)
            {
                mag *= 3.0f;
            }
            ent->frame->dpos.x = mag * isin(angle);
            ent->frame->dpos.z = mag * icos(angle);
            ent->frame->mode |= 2;
            if (!globals.player.ControlOn && globals.player.AutoMoveSpeed && globals.player.AutoMoveSpeed != 4 &&
                ent->frame->dpos.x * ent->frame->dpos.x + ent->frame->dpos.z * ent->frame->dpos.z >=
                    autodist2d * autodist2d)
            {
                ent->frame->dpos.x = globals.player.AutoMoveTarget.x - ((_xMat4x3*)ent->model->Mat)->pos.x;
                ent->frame->dpos.z = globals.player.AutoMoveTarget.z - ((_xMat4x3*)ent->model->Mat)->pos.z;
                PlayerAutoMoveDone(ent);
            }
        }
    }

lean:
    float peakLerp = PlayerLeanRange((float)__fabs(diffAngle), 0.17453294f, 0.9599311f);
    if (diffAngle > 0.0f)
    {
        peakLerp = -peakLerp;
    }
    globals.player.LeanLerp = (6.0f * ((peakLerp + 1.0f) - globals.player.LeanLerp)) * dt + globals.player.LeanLerp;
    if (moveFlag & 0x40)
    {
        ent->model->Anim->Single->BilinearLerp[0] = globals.player.LeanLerp;
    }
    if (blendFlag & 0x40)
    {
        ent->model->Anim->Single->Blend->BilinearLerp[0] = globals.player.LeanLerp;
    }
    if (globals.player.Shaggy1.GrabTimer != 0.0f)
    {
        ent->frame->mode &= ~6;
    }
}

static signed int GetHeadcollAnimState()
{
    xAnimState* state = globals.player.ent.model->Anim->Single->State;
    if ((state->UserFlags & 0x1E) == 0xC)
    {
        return 1;
    }
    if ((state->UserFlags & 0x1E) == 4)
    {
        return 2;
    }
    if ((state->UserFlags & 0x1E) == 6 && (state->UserFlags & 0x1E) != 0x20)
    {
        return 3;
    }

    static unsigned int JNORMAL_ID = xStrHash("Jump_normal");
    static unsigned int JDOUBLE_ID = xStrHash("Jump_double");
    static unsigned int JWALK_ID = xStrHash("Jump_walk");
    static unsigned int JRUN_ID = xStrHash("Jump_run");
    static unsigned int JSB_ID = xStrHash("Jump_springboard");
    static unsigned int FNORMAL_ID = xStrHash("Fall_normal");
    static unsigned int FWALK_ID = xStrHash("Fall_walk");
    static unsigned int FRUN_ID = xStrHash("Fall_run");

    if (state->ID == JNORMAL_ID)
    {
        return (globals.player.ent.frame->vel.y <= 0.0f) ? 9 : 4;
    }
    if (state->ID == JDOUBLE_ID)
    {
        return (globals.player.ent.frame->vel.y <= 0.0f) ? 9 : 5;
    }
    if (state->ID == JWALK_ID)
    {
        return (globals.player.ent.frame->vel.y <= 0.0f) ? 10 : 6;
    }
    if (state->ID == JRUN_ID)
    {
        return (globals.player.ent.frame->vel.y <= 0.0f) ? 11 : 7;
    }
    if (state->ID == JSB_ID)
    {
        return (globals.player.ent.frame->vel.y <= 0.0f) ? 9 : 8;
    }
    if (state->ID == FNORMAL_ID)
    {
        return 9;
    }
    if (state->ID == FWALK_ID)
    {
        return 10;
    }
    if (state->ID == FRUN_ID)
    {
        return 11;
    }
    return 0;
}

static unsigned int cchkButtbounce;
static unsigned int cchkSquish;
static unsigned int sVO;
static unsigned int sVOAssetID;
static unsigned int sVOPriority;
static zPlayerSituation* sVONextSituation;
static float sVOTimer;

static void HeadcollUpdate(float dt)
{
    signed int headcoll_animto = GetHeadcollAnimState();
    if (headcoll_animto != headcoll_anim)
    {
        headcoll_fromos = headcoll_os;
        if (headcoll_animto != 0)
        {
            headcoll_bltime = headcoll_bltimes[headcoll_animto];
        }
        else
        {
            headcoll_bltime = headcoll_bltimes[headcoll_anim];
        }
        headcoll_anim = headcoll_animto;
        headcoll_tmr = headcoll_bltime;
    }
    if (headcoll_tmr > 0.0f)
    {
        float t = headcoll_tmr - dt;
        headcoll_tmr = t;
        if (t <= 0.0f)
        {
            headcoll_tmr = 0.0f;
            headcoll_os = headcoll_oses[headcoll_anim];
        }
        t = t / headcoll_bltime;
        float s = 1.0f - t;
        headcoll_os.x = headcoll_fromos.x * t + headcoll_oses[headcoll_anim].x * s;
        headcoll_os.y = headcoll_fromos.y * t + headcoll_oses[headcoll_anim].y * s;
        headcoll_os.z = headcoll_fromos.z * t + headcoll_oses[headcoll_anim].z * s;
    }
}

#pragma auto_inline on
static void HeadbuttTest(_xCollis* coll, _xCollis* endcoll, float angle_cosine,
                         signed int (*headbuttCB)(_zEnt*, _xCollis*))
{
    _zEnt* ent = (_zEnt*)&globals.player.ent;
    if ((globals.player.ent.model->Anim->Single->State->UserFlags & 0x1E) == 0xC)
    {
        float dx2 = isin(globals.player.ent.frame->rot.angle);
        float dz2 = icos(globals.player.ent.frame->rot.angle);
        for (; coll < endcoll; coll++)
        {
            if ((float)__fabs(coll->hdng.y) < 0.7071f)
            {
                _zEnt* villent = (_zEnt*)coll->optr;
                float dx = villent->frame->mat.pos.x - ((_xMat4x3*)globals.player.ent.model->Mat)->pos.x;
                float dz = villent->frame->mat.pos.z - ((_xMat4x3*)globals.player.ent.model->Mat)->pos.z;
                float mag = xsqrt(dx * dx + dz * dz);
                if (mag > 1e-7f && dx2 * dx + dz2 * dz > angle_cosine * mag && headbuttCB(ent, coll))
                {
                    return;
                }
            }
        }
    }
}
#pragma auto_inline reset

static signed int VillHBCallback(_zEnt* ent, _xCollis* coll)
{
    _zEnt* villent = (_zEnt*)coll->optr;
    if (villent->subType == 0x24)
    {
        return Shaggy8_HBCallback(ent, coll);
    }
    if (zEntVillain_Headbutt((st_zEntVillain*)villent, &coll->hdng))
    {
        ent->frame->mat.pos.x = ((_xMat4x3*)ent->model->Mat)->pos.x;
        ent->frame->mat.pos.y = ((_xMat4x3*)ent->model->Mat)->pos.y;
        ent->frame->mat.pos.z = ((_xMat4x3*)ent->model->Mat)->pos.z;
        globals.player.HeadbuttHit = 1;
        globals.player.DecelRunSpeed = 0.25f * globals.player.DecelRunSpeed;
        zEntPlayer_SNDButtBounce(1);
        zRumbleStart(0, (_tagSDRumbleType)1);
        return 1;
    }
    return 0;
}

#pragma auto_inline on
static unsigned int HealthCheck(xAnimTransition* tran, xAnimSingle*, void*)
{
    unsigned int health = 3;
    if (globals.player.Health < 3)
    {
        health = globals.player.Health;
    }
    return tran->UserFlags & (1 << health);
}
#pragma auto_inline reset

#pragma auto_inline on
static unsigned int PlungerCheck(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.Speed == 0 && globals.player.Slide == 2)
    {
        ret = 1;
    }
    return ret;
}

#pragma auto_inline on
static unsigned int TeeterCheck(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.Speed == 0 && globals.player.Teeter && globals.player.Slide != 2 &&
        !(globals.player.SlideTimer > 0.0f) && !surfSlickTimer)
    {
        ret = 1;
    }
    return ret;
}
#pragma auto_inline reset
#pragma auto_inline reset

#pragma auto_inline on
static unsigned int SlipperyCheck(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.Speed == 0 && globals.player.Slide == 0 && surfSlickTimer != 0.0f)
    {
        ret = 1;
    }
    return ret;
}
#pragma auto_inline reset

static unsigned int HotsauceCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.Speed == 3;
}

static unsigned int RunCheck(xAnimTransition* tran, xAnimSingle*, void*)
{
    unsigned char ret = globals.player.Speed == 2;
    if (ret)
    {
        ret = globals.player.Sneak == 0;
    }
    if (ret)
    {
        ret = PlayerSurfaceAnimCheck(tran);
    }
    if (ret)
    {
        ret = globals.player.Slide == 0;
    }
    return ret;
}

static unsigned int NotRunCheck(xAnimTransition* tran, xAnimSingle*, void*)
{
    unsigned char ret = globals.player.Speed < 2;
    if (!ret)
    {
        ret = globals.player.Sneak != 0;
    }
    if (ret)
    {
        ret = PlayerSurfaceAnimCheck(tran);
    }
    return ret;
}

static unsigned int NotRunCB(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.Face_PantTimer = 2.0f;
    return 0;
}

static unsigned int WalkCheck(xAnimTransition* tran, xAnimSingle*, void*)
{
    unsigned char ret = globals.player.Speed == 1;
    if (ret)
    {
        ret = PlayerSurfaceAnimCheck(tran);
    }
    if (ret)
    {
        ret = globals.player.Slide == 0;
        if (!ret)
        {
            ret = globals.player.Slide == 3;
        }
    }
    return ret;
}

static unsigned int StickySurfIdle(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.Speed == 0 && surfSticky)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int StickySurfWalk(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.Speed == 1 && surfSticky)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int StickNoBootsIdle(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if ((globals.player.Speed == 0 && surfSticky && !(globals.player.Inv_Specials & 4)) || globals.player.Slide == 1 ||
        globals.player.Slide == 3)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int StickNoBootsWalk(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if ((globals.player.Speed == 1 && surfSticky && !(globals.player.Inv_Specials & 4)) || globals.player.Slide == 1 ||
        globals.player.Slide == 3)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int WalkPlungeCheck(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.Slide == 2 && (globals.player.Speed == 1 || globals.player.Speed == 2))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int IdleCheck(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    unsigned char ret = 0;
    unsigned char idle = 0;
    if (globals.player.Speed == 0 && globals.player.Slide == 0 && surfSlickTimer == 0.0f && !globals.player.Teeter)
    {
        idle = 1;
    }
    if (idle && HealthCheck(tran, anim, data))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int SkidStopCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.ent.frame->vel.x * globals.player.ent.frame->vel.x +
               globals.player.ent.frame->vel.z * globals.player.ent.frame->vel.z <
           0.5f;
}

static unsigned int SneakMoveCheck(xAnimTransition* tran, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (((1 << globals.player.Sneak) & tran->UserFlags) && (globals.player.Speed == 1 || globals.player.Speed == 2))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int SneakIdleCheck(xAnimTransition* tran, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (((1 << globals.player.Sneak) & tran->UserFlags) && globals.player.Speed == 0)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int NosneakCheck(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    unsigned char ret = 0;
    if (globals.player.Sneak == 0 && HealthCheck(tran, anim, data))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int NosneakTeeter(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    unsigned char ret = 0;
    if (globals.player.Sneak == 0 && TeeterCheck(tran, anim, data))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int NosneakSlippery(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    unsigned char ret = 0;
    if (globals.player.Sneak == 0 && SlipperyCheck(tran, anim, data))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int NosneakPlunger(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    unsigned char ret = 0;
    if (globals.player.Sneak == 0 && PlungerCheck(tran, anim, data))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int ScareCheck(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    unsigned char ret = 0;
    if (globals.player.ScareTimer && HealthCheck(tran, anim, data))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int CowerCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.CowerTimer != 0.0f;
}

static unsigned int UncowerCheck(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    unsigned char ret = 0;
    if (globals.player.CowerTimer == 0.0f && HealthCheck(tran, anim, data))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int ScareCheckNoHealth(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.ScareTimer != 0.0f;
}

static unsigned int ScareCheckCritter(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.ScareTimer != 0.0f && globals.player.ScareSource != NULL &&
        globals.player.ScareSource->baseType == 2 &&
        (((_xEnt*)globals.player.ScareSource)->subType == 0x15 || ((_xEnt*)globals.player.ScareSource)->subType == 0x16 ||
         ((_xEnt*)globals.player.ScareSource)->subType == 0x18))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int ScareCB(xAnimTransition*, xAnimSingle*, void*)
{
    PlayerScareReset();
    return 0;
}

static unsigned int ScareKnockbackCB(xAnimTransition*, xAnimSingle*, void*)
{
    _xVec3 knock;
    _xMat4x3* mat = (_xMat4x3*)globals.player.ent.model->Mat;
    knock.x = mat->at.x;
    knock.y = 0.0f;
    knock.z = mat->at.z;
    xVec3Normalize(&knock, &knock);
    knock.x *= -4.0f;
    knock.z *= -4.0f;
    knock.y = 15.0f;
    globals.player.ent.frame->vel = knock;
    PlayerScareReset();
    return 0;
}

static unsigned int LandCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.JumpState == 0;
}

static unsigned int ScareLandCheck(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.JumpState == 0 || globals.player.DamageTimer < 0.5f)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int LandSlowCheck(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.JumpState == 0 && globals.player.Speed <= 0)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int LandWalkCheck(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.JumpState == 0 && globals.player.Speed == 1)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int LandFastCheck(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.JumpState == 0 && globals.player.Speed >= 2)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int LandHealthCheck(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    unsigned char ret = 0;
    if (globals.player.JumpState == 0 && HealthCheck(tran, anim, data))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int FallingCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.JumpState == 2;
}

static unsigned int IdleMinorCheck(xAnimTransition* tran, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.IdleMinorTimer > 3.0f &&
        (unsigned short)tran->UserFlags == globals.player.IdleRand % (tran->UserFlags >> 16))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int IdleMajorCheck(xAnimTransition* tran, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.IdleMajorTimer > 9.0f &&
        (unsigned short)tran->UserFlags == globals.player.IdleRand % (tran->UserFlags >> 16))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int IdleSitCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.IdleSitTimer > 11.0f;
}

static unsigned int IdleGetupCheck(xAnimTransition*, xAnimSingle* anim, void*)
{
    unsigned char ret = 0;
    if (globals.player.IdleSitTimer > 11.0f && anim->Blend->State == NULL)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int IdleMinorCB(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.IdleMinorTimer = -1.5f;
    globals.player.IdleRand = irand() >> 16;
    return 0;
}

static unsigned int IdleMajorCB(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.IdleMinorTimer = 0.0001f;
    globals.player.IdleMajorTimer = 0.0f;
    globals.player.IdleRand = irand() >> 16;
    return 0;
}

static unsigned int IdleSitCB(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.IdleMinorTimer = 0.0001f;
    globals.player.IdleMajorTimer = 0.0f;
    globals.player.IdleSitTimer = -20.0f;
    return 0;
}

static unsigned int IdleGetupCB(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.IdleMinorTimer = 0.0001f;
    globals.player.IdleMajorTimer = 0.0f;
    globals.player.IdleSitTimer = -15.0f;
    return 0;
}

static unsigned int ActiveCheck(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    unsigned char ret = 0;
    if (globals.player.IdleMinorTimer == 0.0f && HealthCheck(tran, anim, data))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int JumpCheck(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    _xEnt* fent;
    if ((globals.player.Slide != 1 || !(globals.player.SlideTimer < 0.5f) ||
         ((fent = (_xEnt*)globals.player.ent.collis->colls[0].optr) != NULL && fent->baseType == 6 &&
          fent->subType == 0xB)) &&
        globals.player.Slide != 3 && (!surfSticky || (globals.player.Inv_Specials & 4)) && globals.player.ControlOn &&
        (globals.pad0->pressed & 0x10000))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int JumpCB(xAnimTransition*, xAnimSingle*, void*)
{
    zEntPlayerJumpStart(&globals.player.ent, &globals.player.s.Jump);
    startJump = startDouble = globals.player.ent.frame->mat.pos.y;
    return 0;
}

static unsigned int JumpHangCB(xAnimTransition*, xAnimSingle*, void*)
{
    if (globals.player.HangEnt != NULL)
    {
        globals.player.HangEnt->grabTimer = globals.player.HangEnt->hangInfo->grabDelay;
        globals.player.HangEnt = NULL;
        globals.player.RootUpTarget.x = 0.0f;
        globals.player.RootUpTarget.y = 1.0f;
        globals.player.RootUpTarget.z = 0.0f;
        globals.player.HangElapsed = 0.0f;
        zEntPlayer_Notify(9);
    }
    globals.player.Jump_CanDouble = 1;
    globals.player.Jump_CanFloat = 1;
    zEntPlayerJumpStart(&globals.player.ent, &globals.player.s.Jump);
    startJump = startDouble = globals.player.ent.frame->mat.pos.y;
    return 0;
}

static unsigned int DblJumpCheck(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.ControlOn && (globals.pad0->pressed & 0x10000) && (globals.player.Inv_Specials & 0x200) &&
        globals.player.Jump_CanDouble)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int DblJumpCheckTime(xAnimTransition*, xAnimSingle* anim, void*)
{
    unsigned char ret = 0;
    if (anim->Time > 0.1f && globals.player.ControlOn && (globals.pad0->pressed & 0x10000) &&
        (globals.player.Inv_Specials & 0x200) && globals.player.Jump_CanDouble)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int DblJumpCB(xAnimTransition*, xAnimSingle*, void*)
{
    zEntPlayerJumpStart(&globals.player.ent, &globals.player.s.Double);
    globals.player.Jump_CanDouble = 0;
    startDouble = globals.player.ent.frame->mat.pos.y;
    return 0;
}

static unsigned int BounceCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.Bounced == 1;
}

static unsigned int BounceCB(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.Bounced = 0;
    globals.player.Jump_CanDouble = 1;
    globals.player.Jump_CanFloat = 1;
    return 0;
}

static unsigned int ButtSmashLandCB(xAnimTransition*, xAnimSingle*, void*)
{
    if (globals.player.Inv_Specials & 0x4000)
    {
        if (sEmitterStars != NULL)
        {
            g_info.custom_flags = 0x205E1;
            _xMat4x3* mat = (_xMat4x3*)globals.player.ent.model->Mat;
            _xCollis* coll = &globals.player.ent.collis->colls[0];
            g_info.pos.x = coll->dist * coll->tohit.x + mat->pos.x;
            g_info.pos.y = 0.5f + (coll->dist * coll->tohit.y + mat->pos.y);
            g_info.size_birth = 0.0f;
            g_info.size_death = 0.5f;
            g_info.pos.z = coll->dist * coll->tohit.z + mat->pos.z;
            g_info.color_death[0] = 0x40;
            g_info.color_death[1] = 0x40;
            g_info.color_death[2] = 0x40;
            g_info.color_death[3] = 0;
            g_info.life = 0.25f;
            g_info.vel.x = 0.0f;
            g_info.vel.y = 0.02f;
            g_info.vel.z = 0.0f;
            g_info.xzcircle_radius = 0.2f;
            for (signed int i = 0; i < 2; i++)
            {
                if (i == 0)
                {
                    g_info.color_birth[0] = 0x80;
                    g_info.color_birth[1] = 0x80;
                    g_info.color_birth[2] = 0x80;
                    g_info.color_birth[3] = 0x40;
                }
                else
                {
                    g_info.color_birth[0] = 0x80;
                    g_info.color_birth[1] = 0x80;
                    g_info.color_birth[2] = 0x40;
                    g_info.color_birth[3] = 0x60;
                }
                zParEmitterEmitCustom(sEmitterStars, 1.0f, &g_info);
            }
        }
        globals.player.ShockRadius = 0.6f;
        globals.player.ShockRadiusOld = 0.0f;
        zLaughNotify(3);
    }
    else
    {
        zLaughNotify(2);
    }
    return 0;
}

static unsigned int SpringCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.Bounced == 2;
}

static unsigned int NospringCheck(xAnimTransition*, xAnimSingle*, _zEnt* ent)
{
    return ent->frame->vel.y < 10.0f;
}

static unsigned int DigCheck(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.Diggable != NULL && globals.player.JumpState == 0 && globals.player.ControlOn &&
        (globals.pad0->pressed & 0x20000) && (globals.player.Inv_Specials & 0x2000))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int DigCB(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.DigTimer = 0.00001f;
    zEntEvent(globals.player.Diggable, 0xC1);
    return 0;
}

static unsigned int StopDigCheck(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    unsigned char ret = 0;
    if (!PlayerIsDigging() && HealthCheck(tran, anim, data))
    {
        ret = 1;
    }
    return ret;
}

#pragma auto_inline on
static unsigned int ProjectileCheck(xAnimTransition*, xAnimSingle*, void*)
{
    if (globals.player.JumpState == 0)
    {
        if (globals.player.ControlOn && (globals.pad0->pressed & 0x200) && (globals.player.Inv_Specials & 2) &&
            globals.player.Inv_Ammo[1])
        {
            globals.player.LastProjectile = 1;
            return 1;
        }
        if (globals.player.ControlOn && (globals.pad0->pressed & 0x100) && (globals.player.Inv_Specials & 1) &&
            globals.player.Inv_Ammo[0])
        {
            globals.player.LastProjectile = 0;
            return 1;
        }
    }
    return 0;
}
#pragma auto_inline reset

static unsigned int ProjectileRepeatSync(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    if (anim->Time >= globals.player.s.FirePress && anim->Time < globals.player.s.FireRepeat)
    {
        if (ProjectileCheck(tran, anim, data))
        {
            return 1;
        }
        return 0;
    }
    return 0;
}

static unsigned int ProjectileRepeat(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    if (anim->Time >= globals.player.s.FireRepeat)
    {
        if (ProjectileCheck(tran, anim, data))
        {
            return 1;
        }
        return 0;
    }
    return 0;
}

static unsigned int ProjectileCB(xAnimTransition*, xAnimSingle*, void*)
{
    zEntBubble_Launch(globals.player.s.FireDelay);
    return 0;
}

static unsigned int AttackCheck(xAnimTransition*, xAnimSingle*, void*)
{
    if (globals.player.ControlOn && (globals.pad0->pressed & 0x80000) && globals.player.JumpState == 0 &&
        (!surfSticky || (globals.player.Inv_Specials & 4)) && globals.player.Slide != 1 && globals.player.Slide != 3)
    {
        return 1;
    }
    return 0;
}

static unsigned int AttackCB(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.HeadbuttTimer = 1e-8f;
    globals.player.HeadbuttVel = globals.player.s.HeadbuttSpeedMin;
    return 0;
}

static unsigned int AttackFinished(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    unsigned char ret = 0;
    if (globals.player.HeadbuttTimer == 0.0f && HealthCheck(tran, anim, data))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int AttackFinishedRun(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    unsigned char ret = 0;
    unsigned char run = 0;
    if (globals.player.HeadbuttTimer == 0.0f && globals.player.Speed == 2)
    {
        run = 1;
    }
    if (run && HealthCheck(tran, anim, data))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int AttackFinishedHotsauce(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.HeadbuttTimer == 0.0f && globals.player.Speed == 3)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int HeadHitCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.HeadbuttHit;
}

static unsigned int HeadHitCB(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.HeadbuttHit = 0;
    return 0;
}

static unsigned int ButtSmashCheck(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if ((globals.player.JumpState == 2 || globals.player.JumpState == 3) && (globals.player.Inv_Specials & 0x400) &&
        globals.player.ControlOn && (globals.pad0->pressed & 0x80000))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int ButtSmashCB(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.JumpState = 4;
    globals.player.Jump_CurrGravity = globals.player.s.Gravity;
    globals.player.Jump_HoldTimer = 0.0f;
    globals.player.Jump_ChangeTimer = 0.0f;
    globals.player.ent.frame->vel.y = 0.0f;
    globals.player.ent.frame->dpos.y = 0.0f;
    globals.player.Bounced = 0;
    return 0;
}

static unsigned int ButtSmashStateCB(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.JumpState = 5;
    return 0;
}

static unsigned int SlideCheck(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.Slide == 1 && globals.player.SlideTimer > 0.3f &&
        (globals.player.JumpState == 0 || globals.player.JumpState == 1))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int FloatCheck(xAnimTransition* tran, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.JumpState == 3 &&
        ((tran->UserFlags & 4) ||
         ((tran->UserFlags & 1) && globals.player.ent.frame->mat.pos.y - globals.player.ent.frame->oldpos.y <= 0.0f) ||
         ((tran->UserFlags & 2) && globals.player.ent.frame->mat.pos.y - globals.player.ent.frame->oldpos.y > 0.0f)))
    {
        ret = 1;
    }
    return ret;
}

static unsigned int NotFloatCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.JumpState != 3;
}

static unsigned int HangCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.HangFound != NULL;
}

static unsigned int NoHangCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.HangEnt == NULL;
}

static unsigned int PowerupCheck(xAnimTransition* tran, xAnimSingle*, void*)
{
    return globals.player.SpecialReceived == tran->UserFlags;
}

static unsigned int PowerupCB(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.SpecialReceived = 0;
    return 0;
}

static unsigned int HangCB(xAnimTransition*, xAnimSingle*, void*)
{
    _xVec3 unitHang;

    globals.player.HangEnt = globals.player.HangFound;
    globals.player.HangPivot = globals.player.HangFound->pivot;
    globals.player.HangVel.x = 0.0f;
    globals.player.HangVel.y = 0.0f;
    globals.player.HangVel.z = 0.0f;
    globals.player.HangLength = globals.player.HangFound->hangInfo->leverArm;
    globals.player.HangStartPos = *(_xVec3*)&((_xMat4x3*)globals.player.ent.model->Mat)->pos;
    globals.player.HangStartLerp = 1.0f;
    unitHang.x = globals.player.ent.bound.sph.center.x - globals.player.HangPivot.x;
    unitHang.y = globals.player.ent.bound.sph.center.y - globals.player.HangPivot.y;
    unitHang.z = globals.player.ent.bound.sph.center.z - globals.player.HangPivot.z;
    xVec3Normalize(&unitHang, &unitHang);
    unitHang.x *= globals.player.HangFound->hangInfo->leverArm;
    unitHang.y *= globals.player.HangFound->hangInfo->leverArm;
    unitHang.z *= globals.player.HangFound->hangInfo->leverArm;
    unitHang.x += globals.player.HangFound->pivot.x;
    unitHang.y += globals.player.HangFound->pivot.y;
    unitHang.z += globals.player.HangFound->pivot.z;
    vHangBefore = *(_xVec3*)&((_xMat4x3*)globals.player.ent.model->Mat)->pos;
    ((_xMat4x3*)globals.player.ent.model->Mat)->pos.x = unitHang.x;
    ((_xMat4x3*)globals.player.ent.model->Mat)->pos.y = unitHang.y - globals.player.ent.bound.sph.r;
    ((_xMat4x3*)globals.player.ent.model->Mat)->pos.z = unitHang.z;
    vHangFirst = *(_xVec3*)&((_xMat4x3*)globals.player.ent.model->Mat)->pos;
    if (globals.player.HangEnt->shaggy != NULL)
    {
        _xMat4x3* hmat = (_xMat4x3*)globals.player.HangEnt->model->Mat;
        float hang_angle = xAngleClamp(xatan2(hmat->at.x, hmat->at.z));
        _xMat4x3* pmat = (_xMat4x3*)globals.player.ent.model->Mat;
        float spin = xAngleClamp(xatan2(pmat->at.x, pmat->at.z)) - hang_angle;
        if (spin < 0.0f)
        {
            spin += 6.2831855f;
        }
        if (spin > 6.2831855f)
        {
            spin -= 6.2831855f;
        }
        zEntShaggy_SetSpin(globals.player.HangEnt->shaggy, spin);
    }
    return 0;
}

static unsigned int HangSwingCheck(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (labs(globals.pad0->analog1.x) > globals.player.s.AnalogMin ||
        labs(globals.pad0->analog1.y) > globals.player.s.AnalogMin)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int HangNoSwingCheck(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (labs(globals.pad0->analog1.x) <= globals.player.s.AnalogMin &&
        labs(globals.pad0->analog1.y) <= globals.player.s.AnalogMin)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int DeathCB(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned int snd;
    if ((irand() >> 16) & 1)
    {
        snd = xStrHash("Scooby_Help");
    }
    else
    {
        snd = xStrHash("Scooby_Oh_No");
    }
    xSndPlay(snd, 0.5f, 0.5f, 0.0f, 0xFF, 0, 0);
    PlayerScareReset();
    globals.player.DamageTimer = 3.0f;
    return 0;
}

#pragma auto_inline on
static unsigned int FallToDeathStart(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.FallDeathTimer != 0.0f;
}
#pragma auto_inline reset

static unsigned int FallToDeathDrop(xAnimTransition* tran, xAnimSingle* anim, void* data)
{
    return !globals.player.FallDeathTimer;
}

static unsigned int FallToDeathDropCB(xAnimTransition*, xAnimSingle*, void*)
{
    PlayerScareReset();
    globals.player.DamageTimer = 1.0f;
    globals.player.ent.frame->vel.y = -4.0f;
    return 0;
}

static unsigned int ClearHelmetCB(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.HelmetTimer = 0.0f;
    return 0;
}

static unsigned int SD0_TossCB(xAnimTransition*, xAnimSingle*, void*)
{
    zEntPlayerJumpStart(&globals.player.ent, &globals.player.s.Toss);
    globals.player.ent.frame->vel.x = 0.3f * globals.player.ent.frame->mat.at.x + globals.player.ent.frame->vel.x;
    globals.player.ent.frame->vel.z = 0.3f * globals.player.ent.frame->mat.at.z + globals.player.ent.frame->vel.z;
    return 0;
}

static void PlayerAddToAllStates(xAnimTable* table, xAnimTransition* tran)
{
    xAnimTableAddTransition(table, tran, "Idle_normal Idle_scared Idle_brave Walk_normal Walk_scared Walk_brave");
    xAnimTableAddTransition(table, tran, "Walk_slippery Walk_plunger Run_normal Run_scared Run_brave Run_slippery");
    xAnimTableAddTransition(table, tran, "RunToIdle_normal RunToIdle_scared RunToIdle_brave RunToSkid");
    xAnimTableAddTransition(table, tran,
                            "Idle_sneak Idle_lampshade Idle_blackknight Idle_teeter Idle_slippery Idle_plunger Slide");
    xAnimTableAddTransition(table, tran,
                            "Skid SkidToIdle Walk_sneak Walk_blackknight Walk_sticky Walk_boots Run_boots Run_hotsauce");
    xAnimTableAddTransition(table, tran,
                            "IdleToSneak IdleToLampshade IdleToBlackknight LampshadeToSneak SneakToLampshade");
    xAnimTableAddTransition(table, tran,
                            "Land_normal Land_walkidle Land_running Land_walk Buttsmash_land Buttsmash_hit");
    xAnimTableAddTransition(table, tran,
                            "Action_attack Action_attackrun Action_attack Action_attackrun Action_attackhit");
    xAnimTableAddTransition(table, tran,
                            "Action_projectile Action_dig PowerUpReward Inactive_1 Inactive_2 Inactive_3 Inactive_4");
    xAnimTableAddTransition(
        table, tran, "Inactive_SitStart Inactive_SitLoop Inactive_SitEnd Inactive_SitEndSlow Idle_LookLeft Idle_LookRight");
    xAnimTableAddTransition(table, tran,
                            "Inactive_SitFlea Inactive_SitSniff Inactive_SitHungry Inactive_SitLookLeft "
                            "Inactive_SniffAir Inactive_SniffGround");
    xAnimTableAddTransition(table, tran, "Inactive_Magnify Inactive_Dizzy Inactive_BlowFeet Inactive_LookAround");
}

static inline signed int ButtonHBCallback(_zEnt* ent, _xCollis* coll)
{
    _zEnt* button = (_zEnt*)coll->optr;
    if (button->baseType != 0x18)
    {
        return 0;
    }
    unsigned int headResponse = zEntButton_Headbutt((_zEntButton*)button);
    unsigned int buttonState = zEntButton_CheckState((_zEntButton*)coll->optr);
    if (headResponse == 4 && buttonState == 0)
    {
        if (!(globals.player.Inv_Specials & 0x800))
        {
            globals.player.ent.frame->mat.pos = *(_xVec3*)&((_xMat4x3*)globals.player.ent.model->Mat)->pos;
            globals.player.HeadbuttHit = 1;
            globals.player.DecelRunSpeed = 0.25f * globals.player.DecelRunSpeed;
        }
        else
        {
            zEntEvent((xBase*)coll->optr, 0x37);
            zEntPlayer_ButtonHBFX();
            globals.player.ent.frame->mat.pos = *(_xVec3*)&((_xMat4x3*)globals.player.ent.model->Mat)->pos;
            globals.player.HeadbuttHit = 1;
            globals.player.DecelRunSpeed = 0.25f * globals.player.DecelRunSpeed;
        }
    }
    return 1;
}

void ButtonBBCheck(_xEnt* ent)
{
    if (ent->collis->dyn_eidx > ent->collis->dyn_sidx)
    {
        _xCollis* coll = &ent->collis->colls[ent->collis->dyn_sidx];
        if (coll->optr != NULL && ((xBase*)coll->optr)->baseType == 0x18)
        {
            if ((globals.player.Inv_Specials & 0x800) && ((xBase*)coll->optr)->baseType == 0x18 &&
                (globals.player.ent.model->Anim->Single->State->UserFlags & 0x1E) == 0xC)
            {
                HeadbuttTest(coll, &ent->collis->colls[ent->collis->dyn_eidx], 0.1f, ButtonHBCallback);
            }
            coll = &ent->collis->colls[ent->collis->dyn_sidx];
            _xCollis* endcoll = &ent->collis->colls[ent->collis->dyn_eidx];
            if (((xBase*)coll->optr)->baseType == 0x18 && globals.player.Health &&
                (ent->model->Anim->Single->State->UserFlags & 0x1E) == 0xE)
            {
                for (; coll < endcoll; coll++)
                {
                    if (coll->hdng.y < -0.866025f)
                    {
                        unsigned int bounceResponse = zEntButton_ButtBounce((_zEntButton*)coll->optr);
                        unsigned int buttonState = zEntButton_CheckState((_zEntButton*)coll->optr);
                        if ((bounceResponse == 2 || bounceResponse == 3) && buttonState == 0)
                        {
                            zEntEvent((xBase*)coll->optr, 0x37);
                            zEntPlayer_ButtonBBFX(ent);
                        }
                        else if (coll->hdng.x == 0.0f && coll->hdng.z == 0.0f)
                        {
                            ent->frame->vel.y = 0.0f;
                        }
                    }
                }
            }

            _xEntCollis* collis = ent->collis;
            _zEnt* bn = (_zEnt*)collis->colls[0].optr;
            if (bn != NULL && bn->baseType == 0x18 && collis->colls[0].norm.x == 0.0f && collis->colls[0].norm.z == 0.0f)
            {
                unsigned int response = zEntButton_Stand((_zEntButton*)bn);
                unsigned int buttonState = zEntButton_CheckState((_zEntButton*)collis->colls[0].optr);
                if (response == 1 && buttonState == 0)
                {
                    zEntEvent((xBase*)collis->colls[0].optr, 0x37);
                }
            }
        }
    }
}

void DestructObjBBCheck(_xEnt* ent)
{
    if (ent->collis->dyn_eidx > ent->collis->dyn_sidx)
    {
        _xCollis* coll = &ent->collis->colls[ent->collis->dyn_sidx];
        if (coll->optr != NULL && ((xBase*)coll->optr)->baseType == 0x1B)
        {
            HeadbuttTest(coll, &ent->collis->colls[ent->collis->dyn_eidx], 0.1f, DestructObjHBCallback);
            coll = &ent->collis->colls[ent->collis->dyn_sidx];
            _xCollis* endcoll = &ent->collis->colls[ent->collis->dyn_eidx];
            if (globals.player.Health && (ent->model->Anim->Single->State->UserFlags & 0x1E) == 0xE)
            {
                for (; coll < endcoll; coll++)
                {
                    if (coll->hdng.y < -0.5f)
                    {
                        unsigned int bounceResponse = zEntDestructObj_ButtBounce((_zEntDestructObj*)coll->optr);
                        unsigned int buttonState = zEntDestructObj_CheckState((_zEntDestructObj*)coll->optr);
                        if (bounceResponse == 3 && buttonState != 2)
                        {
                            zEntEvent((xBase*)coll->optr, 0x3A);
                            ent->frame->mat.pos = *(_xVec3*)&((_xMat4x3*)ent->model->Mat)->pos;
                            startBounce = ent->frame->mat.pos.y;
                            zEntPlayerJumpStart(ent, &globals.player.s.Bounce);
                            globals.player.Jump_CanDouble = 1;
                            globals.player.Jump_CanFloat = 1;
                        }
                        if (coll->hdng.x == 0.0f && coll->hdng.z == 0.0f)
                        {
                            ent->frame->vel.y = 0.0f;
                        }
                    }
                }
            }
        }
    }
}

static inline signed int DestructObjHBCallback(_zEnt* ent, _xCollis* coll)
{
    _zEnt* dobj = (_zEnt*)coll->optr;
    if (dobj->baseType != 0x1B)
    {
        return 0;
    }
    unsigned int headResponse = zEntDestructObj_Headbutt((_zEntDestructObj*)dobj);
    unsigned int dobjState = zEntDestructObj_CheckState((_zEntDestructObj*)coll->optr);
    if ((headResponse == 4 || headResponse == 5) && dobjState != 2)
    {
        if (!(globals.player.Inv_Specials & 0x800))
        {
            globals.player.ent.frame->mat.pos = *(_xVec3*)&((_xMat4x3*)globals.player.ent.model->Mat)->pos;
            globals.player.HeadbuttHit = 1;
            globals.player.DecelRunSpeed = 0.25f * globals.player.DecelRunSpeed;
        }
        else
        {
            zEntEvent((xBase*)coll->optr, 0x3A);
            globals.player.ent.frame->mat.pos = *(_xVec3*)&((_xMat4x3*)globals.player.ent.model->Mat)->pos;
            if (dobjState != 1 || headResponse != 5)
            {
                globals.player.HeadbuttHit = 1;
            }
            globals.player.DecelRunSpeed = 0.25f * globals.player.DecelRunSpeed;
        }
    }
    return 1;
}

xAnimTable* zEntPlayer_AnimTable()
{
    xAnimTransition* tran;
    xAnimTable* table;

    table = xAnimTableNew("Player", NULL);
    xAnimTableNewState(table, "Idle_normal", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_scared", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_brave", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Walk_normal", 0x10, 0x444, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Walk_scared", 0x10, 0x444, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Walk_brave", 0x10, 0x444, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Walk_slippery", 0x10, 0x44, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Walk_plunger", 0x10, 0x44, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Walk_sticky", 0x10, 4, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Walk_boots", 0x10, 4, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Run_normal", 0x10, 0x46, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Run_scared", 0x10, 0x46, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Run_brave", 0x10, 0x46, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Run_slippery", 0x10, 0x46, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Run_boots", 0x10, 0x46, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RunToIdle_normal", 0x20, 0x26, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RunToIdle_scared", 0x20, 0x26, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RunToIdle_brave", 0x20, 0x26, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RunToSkid", 0x20, 0x26, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_sneak", 0x10, 3, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_lampshade", 0x10, 3, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_blackknight", 0x10, 3, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_teeter", 0x10, 0x40000, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_slippery", 0x10, 0x40000, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_plunger", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Skid", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "SkidToIdle", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Walk_sneak", 0x10, 2, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Walk_blackknight", 0x10, 2, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Run_hotsauce", 0x10, 8, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "IdleToSneak", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "IdleToLampshade", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "IdleToBlackknight", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "LampshadeToSneak", 0x20, 2, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "SneakToLampshade", 0x20, 2, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Scare_normal", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Scare_losechance", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Scare_loseloop", 0x10, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Scare_brave", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Scare_airhit", 0x20, 0x100, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Scare_airfall", 0x10, 0x80A, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Scare_airland", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Scare_airlose", 0x20, 0x180, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Scare_losefall", 0x10, 0x180, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Scare_loseland", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Scare_norm_cower", 0x10, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Scare_norm_hit", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Scare_critter", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Scare_thorns", 0x20, 0x80A, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Jump_normal", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Jump_run", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Jump_walk", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Jump_double", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Jump_buttbounce", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Jump_hang", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Jump_springboard", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Fall_normal", 0, 0xA, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Fall_run", 0, 0xA, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Fall_walk", 0, 0xA, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Fall_floatup", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Fall_floatdown", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Fall_floatopen", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Fall_spring", 0x10, 0xA, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Fall_fromspring", 0x20, 0xA, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Fall_losechance", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Fall_losedrop", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Land_normal", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Land_running", 0x20, 6, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Land_walk", 0x20, 4, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Land_walkidle", 0x20, 4, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "HangGrab", 0x20, 0x20000, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Hang", 0x10, 0x20000, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "HangToSwing", 0, 0x20000, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "SwingToHang", 0x20, 0x20000, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Buttsmash_start", 0x20, 0x1080E, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Buttsmash_fall", 0x10, 0x1080E, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Buttsmash_land", 0x20, 0x10080, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Buttsmash_hit", 0x20, 0x1080A, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Slide", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Action_attack", 0x10, 0x1000C, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Action_attackrun", 0x10, 0x1000C, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Action_attackhit", 0x20, 0x100A6, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Action_projectile", 0x20, 0x80000, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Action_dig", 0x10, 0x12000, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "PowerUpReward", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_1", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_2", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_3", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_4", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_SitStart", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_SitLoop", 0x10, 0x81, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_SitEnd", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_SitEndSlow", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_LookLeft", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_LookRight", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_LookAround", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_SitFlea", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_SitSniff", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_SitHungry", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_SitLookLeft", 0x20, 0x81, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_SniffAir", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_SniffGround", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_Magnify", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_Dizzy", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Inactive_BlowFeet", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Powerup_helmet", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Powerup_plunger", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Powerup_soap", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Powerup_lamp", 0x20, 0x80, 1.0f, NULL, NULL, 0.0f, NULL);
    Shaggy1_PlayerStates(table);
    Shaggy1_PlayerTransitions(table);
    Shaggy0_PlayerStates(table);
    Shaggy0_PlayerTransitions(table);
    xAnimTableNewTransition(table, "SD0_Throw", "Fall_normal", NULL, SD0_TossCB, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "SD1_HangReact SD1_LandReact", "Idle_scared", HealthCheck, NULL, 0x10, 2, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SD1_HangReact SD1_LandReact", "Idle_normal", HealthCheck, NULL, 0x10, 4, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SD1_HangReact SD1_LandReact", "Idle_brave", HealthCheck, NULL, 0x10, 8, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SD0_SetDown", "Idle_scared", HealthCheck, (xAnimTransitionCallback)Shaggy0_FlipCB, 0x10, 2, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SD0_SetDown", "Idle_normal", HealthCheck, (xAnimTransitionCallback)Shaggy0_FlipCB, 0x10, 4, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SD0_SetDown", "Idle_brave", HealthCheck, (xAnimTransitionCallback)Shaggy0_FlipCB, 0x10, 8, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SD1_SetDown", "Idle_scared", HealthCheck, NULL, 0x10, 2, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SD1_SetDown", "Idle_normal", HealthCheck, NULL, 0x10, 4, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SD1_SetDown", "Idle_brave", HealthCheck, NULL, 0x10, 8, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SD1_Hold SD1_HoldWalk SD1_CatchJump SD1_CatchThrow SD1_Throw SD1_ThrowIdle SD1_ThrowWalk SD1_HangReact SD1_LandReact", "Scare_norm_hit", ScareCheckNoHealth, ScareCB, 0, 0, 0.0f, 0.0f, 5, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SD1_Hold SD1_HoldWalk SD1_CatchJump SD1_CatchThrow SD1_Throw SD1_ThrowIdle SD1_ThrowWalk SD1_HangReact SD1_LandReact", "Scare_critter", ScareCheckCritter, ScareCB, 0, 0, 0.0f, 0.0f, 6, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SD1_Hold SD1_HoldWalk SD1_CatchJump SD1_CatchThrow SD1_Throw SD1_ThrowIdle SD1_ThrowWalk SD1_HangReact SD1_LandReact", "Scare_loseloop", ScareCheck, DeathCB, 0, 1, 0.0f, 0.0f, 0xA, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SD0_Hold SD0_HoldWalk SD0_CatchJump SD0_CatchThrow SD0_Throw", "Scare_airhit", ScareCheckNoHealth, ScareKnockbackCB, 0, 0, 0.0f, 0.0f, 5, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attack Action_projectile Action_dig PowerUpReward", "Idle_scared", HealthCheck, NULL, 0x10, 2, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attack Action_projectile Action_dig PowerUpReward", "Idle_normal", HealthCheck, NULL, 0x10, 4, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attack Action_projectile Action_dig PowerUpReward", "Idle_brave", HealthCheck, NULL, 0x10, 8, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attack Action_projectile Action_dig PowerUpReward", "Idle_teeter", TeeterCheck, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attack Action_projectile Action_dig PowerUpReward", "Idle_slippery", SlipperyCheck, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attack Action_projectile Action_dig PowerUpReward", "Idle_plunger", PlungerCheck, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attackrun", "Run_scared", HealthCheck, NULL, 0x10, 2, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attackrun", "Run_normal", HealthCheck, NULL, 0x10, 4, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attackrun", "Run_brave", HealthCheck, NULL, 0x10, 8, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attackrun", "Run_hotsauce", HotsauceCheck, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Scare_airhit", "Scare_airfall", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Scare_airhit", "Idle_scared", LandHealthCheck, NULL, 0x10, 2, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Scare_airhit", "Idle_normal", LandHealthCheck, NULL, 0x10, 4, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Scare_airhit", "Idle_brave", LandHealthCheck, NULL, 0x10, 8, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Scare_brave", "Idle_normal", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Scare_normal", "Idle_scared", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Scare_norm_hit Scare_critter", "Idle_scared", HealthCheck, NULL, 0x10, 2, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Scare_norm_hit Scare_critter", "Idle_normal", HealthCheck, NULL, 0x10, 4, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Scare_norm_hit Scare_critter", "Idle_brave", HealthCheck, NULL, 0x10, 8, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Inactive_1 Inactive_2 Inactive_3 Inactive_4 Inactive_SitEnd Inactive_SitEndSlow", "Idle_scared", HealthCheck, NULL, 0x10, 2, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Inactive_1 Inactive_2 Inactive_3 Inactive_4 Inactive_SitEnd Inactive_SitEndSlow", "Idle_normal", HealthCheck, NULL, 0x10, 4, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Inactive_1 Inactive_2 Inactive_3 Inactive_4 Inactive_SitEnd Inactive_SitEndSlow", "Idle_brave", HealthCheck, NULL, 0x10, 8, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Jump_normal Jump_run Jump_double Jump_buttbounce Jump_hang", "Fall_normal", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Jump_walk", "Fall_walk", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Jump_run", "Fall_run", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Jump_springboard", "Fall_spring", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Fall_fromspring", "Fall_normal", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Land_normal Land_walk Land_walkidle Land_running", "Idle_scared", HealthCheck, NULL, 0x10, 2, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Land_normal Land_walk Land_walkidle Land_running", "Idle_normal", HealthCheck, NULL, 0x10, 4, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Land_normal Land_walk Land_walkidle Land_running", "Idle_brave", HealthCheck, NULL, 0x10, 8, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Land_normal Land_walk Land_walkidle Land_running", "Walk_scared", WalkCheck, NULL, 0x10, 2, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Land_normal Land_walk Land_walkidle Land_running", "Walk_normal", WalkCheck, NULL, 0x10, 4, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Land_normal Land_walk Land_walkidle Land_running", "Walk_brave", WalkCheck, NULL, 0x10, 8, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Land_running", "Run_scared", RunCheck, NULL, 0x10, 2, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Land_running", "Run_normal", RunCheck, NULL, 0x10, 4, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Land_running", "Run_brave", RunCheck, NULL, 0x10, 8, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Land_running", "Run_slippery", RunCheck, NULL, 0x10, 0x10, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Land_normal Land_walk Land_walkidle Land_running", "Walk_slippery", WalkCheck, NULL, 0x10, 0x10, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Land_normal Land_walk Land_walkidle Land_running", "Walk_sticky", WalkCheck, NULL, 0x10, 0x40, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Land_normal Land_walk Land_walkidle Land_running", "Walk_boots", WalkCheck, NULL, 0x10, 0x80, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Land_normal Land_walk Land_walkidle Land_running", "Walk_plunger", WalkPlungeCheck, NULL, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Land_normal Land_walk Land_walkidle Land_running", "Idle_plunger", PlungerCheck, NULL, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Land_normal Land_walk Land_walkidle Land_running", "Slide", SlideCheck, NULL, 0x10, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "RunToSkid", "Skid", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "SkidToIdle", "Idle_scared", HealthCheck, NULL, 0x10, 2, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SkidToIdle", "Idle_normal", HealthCheck, NULL, 0x10, 4, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SkidToIdle", "Idle_brave", HealthCheck, NULL, 0x10, 8, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SkidToIdle", "Idle_teeter", TeeterCheck, NULL, 0x10, 0, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SkidToIdle", "Idle_slippery", SlipperyCheck, NULL, 0x10, 0, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SkidToIdle", "Idle_plunger", PlungerCheck, NULL, 0x10, 0, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "SkidToIdle", "Slide", SlideCheck, NULL, 0x10, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "HangGrab", "Hang", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "SwingToHang", "Hang", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "IdleToSneak", "Walk_sneak", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "IdleToLampshade", "Idle_lampshade", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "IdleToBlackknight", "Idle_blackknight", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "LampshadeToSneak", "Walk_sneak", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "SneakToLampshade", "Idle_lampshade", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Idle_normal Idle_brave  Idle_slippery Idle_plunger Run_boots Slide Walk_normal Walk_scared Walk_brave Walk_slippery Walk_plunger Walk_sticky Walk_boots", "Idle_scared", IdleCheck, NULL, 0, 2, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle_scared Idle_brave  Idle_slippery Idle_plunger Run_boots Slide Walk_normal Walk_scared Walk_brave Walk_slippery Walk_plunger Walk_sticky Walk_boots", "Idle_normal", IdleCheck, NULL, 0, 4, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle_scared Idle_normal Idle_slippery Idle_plunger Run_boots Slide Walk_normal Walk_scared Walk_brave Walk_slippery Walk_plunger Walk_sticky Walk_boots", "Idle_brave", IdleCheck, NULL, 0, 8, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle_scared Idle_normal Idle_brave Idle_LookLeft Idle_LookRight Inactive_LookAround Idle_plunger Idle_slippery Run_boots Slide Inactive_SniffAir Inactive_SniffGround Inactive_Magnify Walk_normal Walk_scared Walk_brave Walk_slippery Walk_plunger Walk_sticky Walk_boots", "Idle_teeter", TeeterCheck, NULL, 0, 0, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle_scared Idle_normal Idle_brave Idle_LookLeft Idle_LookRight Inactive_LookAround Idle_plunger Idle_teeter Run_boots Slide Inactive_SniffAir Inactive_SniffGround Inactive_Magnify Walk_normal Walk_scared Walk_brave Walk_slippery Walk_plunger Walk_sticky Walk_boots", "Idle_slippery", SlipperyCheck, NULL, 0, 0, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle_scared Idle_normal Idle_brave Idle_LookLeft Idle_LookRight Inactive_LookAround Idle_teeter Idle_slippery Run_boots Slide Inactive_SniffAir Inactive_SniffGround Inactive_Magnify Walk_normal Walk_scared Walk_brave Walk_slippery Walk_plunger Walk_sticky Walk_boots", "Idle_plunger", PlungerCheck, NULL, 0, 0, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle_scared Idle_normal Idle_brave Idle_LookLeft Idle_LookRight Inactive_LookAround Idle_plunger Idle_teeter Idle_slippery Inactive_SniffAir Inactive_SniffGround Inactive_Magnify Walk_normal Walk_scared Walk_brave Walk_slippery Walk_plunger Walk_sticky Walk_boots", "Slide", SlideCheck, NULL, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Skid", "SkidToIdle", SkidStopCheck, NULL, 0, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Slide Walk_normal Walk_brave Run_boots Walk_slippery Walk_plunger Walk_sticky Walk_boots Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify", "Walk_scared", WalkCheck, NULL, 0, 2, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Slide Walk_scared Walk_brave Run_boots Walk_slippery Walk_plunger Walk_sticky Walk_boots Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify", "Walk_normal", WalkCheck, NULL, 0, 4, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Slide Walk_scared Walk_normal Run_boots Walk_slippery Walk_plunger Walk_sticky Walk_boots Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify", "Walk_brave", WalkCheck, NULL, 0, 8, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Slide Walk_plunger  Walk_sticky  Walk_boots Run_boots Walk_scared Walk_normal Walk_brave Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify", "Walk_slippery", WalkCheck, NULL, 0, 0x10, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Slide Walk_slippery Walk_sticky  Walk_boots Run_boots Walk_scared Walk_normal Walk_brave Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify", "Walk_plunger", WalkCheck, NULL, 0, 0x20, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Slide Walk_slippery Walk_plunger Walk_boots Run_boots Walk_scared Walk_normal Walk_brave Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify", "Walk_sticky", WalkCheck, NULL, 0, 0x40, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Slide Walk_slippery Walk_plunger Walk_sticky Run_boots Walk_scared Walk_normal Walk_brave Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify", "Walk_boots", WalkCheck, NULL, 0, 0x80, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    tran = xAnimTableNewTransition(table, "Run_normal Run_scared Run_brave Run_slippery Run_boots", "Walk_plunger", WalkPlungeCheck, NULL, 0, 0, 0.0f, 0.0f, 5, 0, 0.15f, NULL);
    xAnimTableAddTransition(table, tran, "Walk_normal Walk_scared Walk_brave Walk_slippery Walk_sticky Walk_boots");
    xAnimTableAddTransition(table, tran, "RunToIdle_normal RunToIdle_scared RunToIdle_brave RunToSkid");
    xAnimTableAddTransition(table, tran, "Idle_sneak Idle_lampshade Idle_blackknight Idle_teeter Idle_slippery Idle_plunger");
    xAnimTableAddTransition(table, tran, "Skid SkidToIdle Walk_sneak Walk_blackknight");
    xAnimTableAddTransition(table, tran, "IdleToSneak IdleToLampshade IdleToBlackknight LampshadeToSneak SneakToLampshade");
    xAnimTableAddTransition(table, tran, "Inactive_1 Inactive_2 Inactive_3 Inactive_4 Inactive_SitFlea");
    xAnimTableAddTransition(table, tran, "Inactive_SitStart Inactive_SitLoop Inactive_SitEnd Inactive_SitEndSlow Idle_LookLeft Idle_LookRight");
    xAnimTableAddTransition(table, tran, "Inactive_SitSniff Inactive_SitHungry Inactive_SitLookLeft Inactive_SniffAir Inactive_SniffGround");
    xAnimTableAddTransition(table, tran, "Inactive_Magnify Inactive_Dizzy Inactive_BlowFeet Inactive_LookAround");
    tran = xAnimTableNewTransition(table, "Run_normal Run_scared Run_brave Run_slippery", "Slide", SlideCheck, NULL, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableAddTransition(table, tran, "Walk_normal Walk_scared Walk_brave Walk_slippery Walk_sticky Walk_boots");
    xAnimTableAddTransition(table, tran, "RunToIdle_normal RunToIdle_scared RunToIdle_brave RunToSkid");
    xAnimTableAddTransition(table, tran, "Idle_sneak Idle_lampshade Idle_blackknight Idle_teeter Idle_slippery Idle_plunger");
    xAnimTableAddTransition(table, tran, "Skid SkidToIdle Walk_sneak Walk_blackknight Run_slippery");
    xAnimTableAddTransition(table, tran, "IdleToSneak IdleToLampshade IdleToBlackknight LampshadeToSneak SneakToLampshade");
    xAnimTableAddTransition(table, tran, "Inactive_1 Inactive_2 Inactive_3 Inactive_4 Inactive_SitFlea");
    xAnimTableAddTransition(table, tran, "Inactive_SitStart Inactive_SitLoop Inactive_SitEnd Inactive_SitEndSlow Idle_LookLeft Idle_LookRight");
    xAnimTableAddTransition(table, tran, "Inactive_SitSniff Inactive_SitHungry Inactive_SitLookLeft Inactive_SniffAir Inactive_SniffGround");
    xAnimTableAddTransition(table, tran, "Inactive_Magnify Inactive_Dizzy Inactive_BlowFeet Inactive_LookAround");
    xAnimTableNewTransition(table, "Slide Run_normal Run_brave  Run_hotsauce Run_slippery Run_boots Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify Walk_normal Walk_scared Walk_brave Walk_slippery Walk_plunger Walk_sticky Walk_boots", "Run_scared", RunCheck, NULL, 0, 2, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Slide Run_scared Run_brave  Run_hotsauce Run_slippery Run_boots Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify Walk_normal Walk_scared Walk_brave Walk_slippery Walk_plunger Walk_sticky Walk_boots", "Run_normal", RunCheck, NULL, 0, 4, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Slide Run_scared Run_normal Run_hotsauce Run_slippery Run_boots Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify Walk_normal Walk_scared Walk_brave Walk_slippery Walk_plunger Walk_sticky Walk_boots", "Run_brave", RunCheck, NULL, 0, 8, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Slide Run_scared Run_normal Run_hotsauce Run_brave    Run_boots Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify Walk_normal Walk_scared Walk_brave Walk_slippery Walk_plunger Walk_sticky Walk_boots", "Run_slippery", RunCheck, NULL, 0, 0x10, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Slide Run_scared Run_normal Run_hotsauce Run_brave Run_slippery Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify Walk_normal Walk_scared Walk_brave Walk_slippery Walk_plunger Walk_sticky Walk_boots", "Run_boots", RunCheck, NULL, 0, 0x80, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Run_scared Run_normal Run_brave Run_hotsauce Run_slippery Run_boots", "Walk_sticky", StickySurfWalk, ClearHelmetCB, 0, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Run_scared Run_normal Run_brave Run_hotsauce Run_slippery Run_boots", "Idle_normal", StickySurfIdle, ClearHelmetCB, 0, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attackrun Action_attack", "Walk_sticky", StickNoBootsWalk, ClearHelmetCB, 0, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attackrun Action_attack", "Idle_normal", StickNoBootsIdle, ClearHelmetCB, 0, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Run_scared", "RunToIdle_scared", NotRunCheck, NotRunCB, 0, 0x1F, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Run_normal", "RunToIdle_normal", NotRunCheck, NotRunCB, 0, 0x1F, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Run_brave", "RunToIdle_brave", NotRunCheck, NotRunCB, 0, 0x1F, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Run_slippery", "RunToSkid", NotRunCheck, NULL, 0, 0x1F, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Run_hotsauce", "RunToIdle_scared", NotRunCheck, NotRunCB, 0, 2, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Run_hotsauce", "RunToIdle_normal", NotRunCheck, NotRunCB, 0, 2, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Run_hotsauce", "RunToIdle_brave", NotRunCheck, NotRunCB, 0, 2, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Run_hotsauce", "RunToSkid", NotRunCheck, NULL, 0, 0x10, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "RunToIdle_scared", "Idle_scared", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "RunToIdle_normal", "Idle_normal", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "RunToIdle_brave", "Idle_brave", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    tran = xAnimTableNewTransition(table, "Walk_scared Walk_normal Walk_brave Run_scared Run_normal Run_brave", "Walk_sneak", SneakMoveCheck, NULL, 0, 6, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableAddTransition(table, tran, "Idle_sneak Idle_blackknight Walk_blackknight");
    xAnimTableAddTransition(table, tran, " Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify");
    tran = xAnimTableNewTransition(table, "Walk_scared Walk_normal Walk_brave Run_scared Run_normal Run_brave", "Walk_blackknight", SneakMoveCheck, NULL, 0, 8, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableAddTransition(table, tran, "Idle_sneak Idle_blackknight Walk_sneak");
    xAnimTableAddTransition(table, tran, " Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify");
    xAnimTableNewTransition(table, " Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify", "IdleToLampshade", SneakIdleCheck, NULL, 0, 4, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Walk_sneak", "SneakToLampshade", SneakIdleCheck, NULL, 0, 4, 0.0f, 0.0f, 1, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Idle_lampshade", "LampshadeToSneak", SneakMoveCheck, NULL, 0, 4, 0.0f, 0.0f, 1, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "SneakToLampshade", "LampshadeToSneak", SneakMoveCheck, NULL, 0x20, 4, 0.0f, 0.0f, 2, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "LampshadeToSneak", "SneakToLampshade", SneakIdleCheck, NULL, 0x20, 4, 0.0f, 0.0f, 2, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Walk_sneak Walk_blackknight Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify", "Idle_sneak", SneakIdleCheck, NULL, 0, 2, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Walk_sneak Walk_blackknight Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify", "Idle_blackknight", SneakIdleCheck, NULL, 0, 8, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle_sneak Idle_lampshade Idle_blackknight", "Idle_scared", NosneakCheck, NULL, 0, 2, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle_sneak Idle_lampshade Idle_blackknight", "Idle_normal", NosneakCheck, NULL, 0, 4, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle_sneak Idle_lampshade Idle_blackknight", "Idle_brave", NosneakCheck, NULL, 0, 8, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle_sneak Idle_lampshade Idle_blackknight", "Idle_teeter", NosneakTeeter, NULL, 0, 0, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle_sneak Idle_lampshade Idle_blackknight", "Idle_slippery", NosneakSlippery, NULL, 0, 0, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Idle_sneak Idle_lampshade Idle_blackknight", "Idle_plunger", NosneakPlunger, NULL, 0, 0, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Walk_sneak Walk_blackknight", "Walk_scared", NosneakCheck, NULL, 0, 2, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Walk_sneak Walk_blackknight", "Walk_normal", NosneakCheck, NULL, 0, 4, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Walk_sneak Walk_blackknight", "Walk_brave", NosneakCheck, NULL, 0, 8, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    tran = xAnimTableNewTransition(table, " Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify", "Run_hotsauce", HotsauceCheck, NULL, 0, 0, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    xAnimTableAddTransition(table, tran, "Walk_scared Walk_normal Walk_brave Walk_sneak Walk_blackknight");
    xAnimTableAddTransition(table, tran, "Idle_sneak Idle_lampshade Idle_blackknight Idle_teeter Idle_slippery Idle_plunger");
    xAnimTableAddTransition(table, tran, "Run_scared Run_normal Run_brave");
    xAnimTableAddTransition(table, tran, "Inactive_1 Inactive_2 Inactive_3 Inactive_4");
    xAnimTableAddTransition(table, tran, "Inactive_SitStart Inactive_SitLoop Inactive_SitEnd Inactive_SitEndSlow");
    xAnimTableAddTransition(table, tran, "Idle_LookLeft Idle_LookRight Inactive_SitFlea");
    xAnimTableAddTransition(table, tran, "Inactive_SitSniff Inactive_SitHungry Inactive_SitLookLeft Inactive_SniffAir Inactive_SniffGround");
    xAnimTableAddTransition(table, tran, "Inactive_Magnify Inactive_Dizzy Inactive_BlowFeet Inactive_LookAround");
    PlayerAddToAllStates(table, xAnimTableNewTransition(table, "", "Scare_norm_cower", CowerCheck, NULL, 0, 0, 0.0f, 0.0f, 5, 0, 0.15f, NULL));
    xAnimTableNewTransition(table, "Scare_norm_cower", "Idle_scared", UncowerCheck, NULL, 0, 2, 0.0f, 0.0f, 5, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Scare_norm_cower", "Idle_normal", UncowerCheck, NULL, 0, 4, 0.0f, 0.0f, 5, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Scare_norm_cower", "Idle_brave", UncowerCheck, NULL, 0, 8, 0.0f, 0.0f, 5, 0, 0.15f, NULL);
    PlayerAddToAllStates(table, xAnimTableNewTransition(table, "", "Scare_norm_hit", ScareCheckNoHealth, ScareCB, 0, 0, 0.0f, 0.0f, 5, 0, 0.15f, NULL));
    PlayerAddToAllStates(table, xAnimTableNewTransition(table, "", "Scare_critter", ScareCheckCritter, ScareCB, 0, 0, 0.0f, 0.0f, 6, 0, 0.15f, NULL));
    tran = xAnimTableNewTransition(table, "", "Scare_loseloop", ScareCheck, DeathCB, 0, 1, 0.0f, 0.0f, 0xA, 0, 0.15f, NULL);
    PlayerAddToAllStates(table, tran);
    xAnimTableAddTransition(table, tran, "Powerup_helmet Powerup_lamp Powerup_plunger Powerup_soap Scare_brave");
    xAnimTableAddTransition(table, tran, "Scare_critter Scare_norm_cower Scare_norm_hit Scare_normal Scare_thorns");
    xAnimTableNewTransition(table, "Scare_airlose", "Scare_losefall", NULL, DeathCB, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Scare_losefall", "Scare_loseland", LandCheck, NULL, 0, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    tran = xAnimTableNewTransition(table, "", "Fall_losechance", FallToDeathStart, ButtSmashCB, 0, 0, 0.0f, 0.0f, 0x14, 0, 0.15f, NULL);
    for (xAnimState* state = table->StateList; state != NULL; state = state->Next)
    {
        if (xStricmp(state->Name, "Fall_losechance") != 0 && xStricmp(state->Name, "Fall_losedrop") != 0)
        {
            xAnimTableAddTransition(table, tran, state->Name);
        }
    }
    xAnimTableNewTransition(table, "Fall_losechance", "Fall_losedrop", FallToDeathDrop, FallToDeathDropCB, 0, 0, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    tran = xAnimTableNewTransition(table, "Jump_normal Jump_run Jump_walk Jump_double Jump_buttbounce Jump_hang Jump_springboard", "Scare_airhit", ScareCheck, ScareCB, 0, 0xF, 0.0f, 0.0f, 5, 0, 0.15f, NULL);
    xAnimTableAddTransition(table, tran, "Fall_normal Fall_run Fall_walk Fall_high Fall_floatup");
    xAnimTableAddTransition(table, tran, "Fall_floatdown Fall_floatopen HangGrab Hang HangToSwing SwingToHang");
    xAnimTableAddTransition(table, tran, "Buttsmash_start Buttsmash_fall Fall_spring Fall_fromspring");
    tran = xAnimTableNewTransition(table, "Jump_normal Jump_run Jump_walk Jump_double Jump_buttbounce Jump_hang Jump_springboard", "Scare_airlose", ScareCheck, ScareKnockbackCB, 0, 1, 0.0f, 0.0f, 7, 0, 0.15f, NULL);
    xAnimTableAddTransition(table, tran, "Fall_normal Fall_run Fall_walk Fall_high Fall_floatup");
    xAnimTableAddTransition(table, tran, "Fall_floatdown Fall_floatopen HangGrab Hang HangToSwing SwingToHang");
    xAnimTableAddTransition(table, tran, "Buttsmash_start Buttsmash_fall Fall_spring Fall_fromspring");
    xAnimTableAddTransition(table, tran, "Scare_airhit Scare_airfall Scare_airland");
    tran = xAnimTableNewTransition(table, " Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify", "Jump_normal", JumpCheck, JumpCB, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableAddTransition(table, tran, "Idle_sneak Idle_lampshade Idle_blackknight Walk_sneak Walk_blackknight");
    xAnimTableAddTransition(table, tran, "Idle_teeter Idle_slippery Idle_plunger Slide Inactive_1 Inactive_2 Inactive_3 Inactive_4");
    xAnimTableAddTransition(table, tran, "Inactive_SitStart Inactive_SitLoop Inactive_SitEnd Inactive_SitEndSlow");
    xAnimTableAddTransition(table, tran, "Idle_LookLeft Idle_LookRight Inactive_SitFlea");
    xAnimTableAddTransition(table, tran, "Land_normal Land_walkidle");
    xAnimTableAddTransition(table, tran, "RunToIdle_scared RunToIdle_normal RunToIdle_brave");
    xAnimTableAddTransition(table, tran, "RunToSkid Skid SkidToIdle Walk_slippery Walk_plunger Walk_boots Idle_plunger");
    xAnimTableAddTransition(table, tran, "Inactive_SitSniff Inactive_SitHungry Inactive_SitLookLeft Inactive_SniffAir Inactive_SniffGround");
    xAnimTableAddTransition(table, tran, "Inactive_Magnify Inactive_Dizzy Inactive_BlowFeet Inactive_LookAround");
    xAnimTableNewTransition(table, "Fall_spring", "Fall_fromspring", (xAnimTransitionConditionalCallback)NospringCheck, NULL, 0, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Walk_normal Walk_scared Walk_brave Land_walk", "Jump_walk", JumpCheck, JumpCB, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Run_normal Run_scared Run_brave Run_hotsauce Land_running Run_slippery Run_boots", "Jump_run", JumpCheck, JumpCB, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Fall_normal Fall_run Fall_walk Fall_floatup Fall_floatdown Fall_floatopen Fall_high Fall_spring Fall_fromspring Jump_hang", "Jump_double", DblJumpCheck, DblJumpCB, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Jump_normal Jump_run Jump_walk Jump_springboard", "Jump_double", DblJumpCheckTime, DblJumpCB, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    tran = xAnimTableNewTransition(table, "Buttsmash_start Scare_airhit Scare_airfall", "Jump_buttbounce", BounceCheck, BounceCB, 0, 0, 0.0f, 0.0f, 5, 0, 0.15f, NULL);
    xAnimTableAddTransition(table, tran, "Fall_normal Fall_run Fall_walk Fall_floatup Fall_floatdown Fall_floatopen Fall_high");
    xAnimTableAddTransition(table, tran, "Jump_normal Jump_run Jump_walk");
    xAnimTableAddTransition(table, tran, "Jump_double Jump_buttbounce Jump_hang Jump_springboard");
    xAnimTableAddTransition(table, tran, "Fall_spring Fall_fromspring");
    tran = xAnimTableNewTransition(table, "Buttsmash_start Scare_airhit Scare_airfall", "Jump_springboard", SpringCheck, BounceCB, 0, 0, 0.0f, 0.0f, 5, 0, 0.15f, NULL);
    xAnimTableAddTransition(table, tran, "Fall_normal Fall_run Fall_walk Fall_floatup Fall_floatdown Fall_floatopen Fall_high");
    xAnimTableAddTransition(table, tran, "Jump_normal Jump_run Jump_walk");
    xAnimTableAddTransition(table, tran, "Jump_double Jump_buttbounce Jump_hang Jump_springboard");
    xAnimTableAddTransition(table, tran, "Fall_spring Fall_fromspring");
    tran = xAnimTableNewTransition(table, "Fall_normal Fall_run Fall_walk Fall_floatup Fall_floatdown Fall_floatopen Fall_high", "Hang", HangCheck, HangCB, 0, 0, 0.0f, 0.0f, 5, 0, 0.1f, NULL);
    xAnimTableAddTransition(table, tran, "Jump_normal Jump_walk Jump_run Jump_double");
    xAnimTableAddTransition(table, tran, "Fall_spring Fall_fromspring Jump_springboard");
    xAnimTableNewTransition(table, "Hang HangGrab SwingToHang HangToSwing", "Jump_hang", JumpCheck, JumpHangCB, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Hang HangGrab SwingToHang HangToSwing", "Fall_normal", NoHangCheck, NULL, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Hang", "HangToSwing", HangSwingCheck, NULL, 0, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "HangToSwing", "SwingToHang", HangNoSwingCheck, NULL, 0, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    tran = xAnimTableNewTransition(table, "Walk_scared Walk_normal Walk_brave Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify", "Fall_normal", FallingCheck, NULL, 0, 0, 0.0f, 0.0f, 5, 0, 0.15f, NULL);
    xAnimTableAddTransition(table, tran, "Idle_sneak Idle_lampshade Idle_blackknight Idle_plunger Idle_slippery Idle_teeter Slide");
    xAnimTableAddTransition(table, tran, "Walk_sneak Walk_blackknight Land_normal Land_walk Land_walkidle Land_running");
    xAnimTableAddTransition(table, tran, "Inactive_1 Inactive_2 Inactive_3 Inactive_4");
    xAnimTableAddTransition(table, tran, "Inactive_SitStart Inactive_SitLoop Inactive_SitEnd Inactive_SitEndSlow");
    xAnimTableAddTransition(table, tran, "Idle_LookLeft Idle_LookRight Inactive_SitFlea");
    xAnimTableAddTransition(table, tran, "Walk_slippery Walk_plunger Walk_sticky Walk_boots");
    xAnimTableAddTransition(table, tran, "Inactive_SitSniff Inactive_SitHungry Inactive_SitLookLeft Inactive_SniffAir Inactive_SniffGround");
    xAnimTableAddTransition(table, tran, "Inactive_Magnify Inactive_Dizzy Inactive_BlowFeet Inactive_LookAround");
    xAnimTableNewTransition(table, "Run_normal Run_scared Run_brave Run_hotsauce", "Fall_run", FallingCheck, NULL, 0, 0, 0.0f, 0.0f, 5, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Fall_normal Fall_run Fall_walk Fall_high Fall_spring Fall_fromspring", "Fall_floatopen", FloatCheck, NULL, 0, 4, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Fall_floatopen", "Fall_floatdown", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Fall_floatopen", "Fall_floatup", FloatCheck, NULL, 0x10, 2, 0.0f, 0.0f, 2, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Fall_floatdown", "Fall_floatup", FloatCheck, NULL, 0, 2, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Fall_floatup", "Fall_floatdown", FloatCheck, NULL, 0, 1, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Fall_floatdown Fall_floatup", "Fall_normal", NotFloatCheck, NULL, 0, 0, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Fall_normal Fall_run Fall_walk Fall_floatup Fall_floatdown Fall_floatopen Fall_high Jump_double Jump_walk Jump_run Jump_normal Jump_hang Fall_spring Fall_fromspring", "Buttsmash_start", ButtSmashCheck, ButtSmashCB, 0, 0, 0.0f, 0.0f, 5, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Buttsmash_start", "Buttsmash_fall", NULL, ButtSmashStateCB, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Buttsmash_fall", "Buttsmash_land", LandCheck, ButtSmashLandCB, 0, 0, 0.0f, 0.0f, 5, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Buttsmash_fall", "Jump_buttbounce", BounceCheck, BounceCB, 0, 0, 0.0f, 0.0f, 5, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Buttsmash_fall", "Jump_springboard", SpringCheck, BounceCB, 0, 0, 0.0f, 0.0f, 5, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Buttsmash_land", "Idle_scared", HealthCheck, NULL, 0x10, 2, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Buttsmash_land", "Idle_normal", HealthCheck, NULL, 0x10, 4, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Buttsmash_land", "Idle_brave", HealthCheck, NULL, 0x10, 8, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Buttsmash_hit", "Idle_scared", HealthCheck, NULL, 0x10, 2, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Buttsmash_hit", "Idle_normal", HealthCheck, NULL, 0x10, 4, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Buttsmash_hit", "Idle_brave", HealthCheck, NULL, 0x10, 8, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Fall_normal Fall_run Fall_floatup Fall_floatdown Fall_floatopen Fall_high", "Land_normal", LandSlowCheck, NULL, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Fall_walk", "Land_walkidle", LandSlowCheck, NULL, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Fall_normal Fall_run Fall_walk Fall_floatup Fall_floatdown Fall_floatopen Fall_high", "Land_walk", LandWalkCheck, NULL, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Fall_normal Fall_run Fall_walk Fall_floatup Fall_floatdown Fall_floatopen Fall_high", "Land_running", LandFastCheck, NULL, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Fall_spring Fall_fromspring", "Land_normal", LandCheck, NULL, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Scare_airfall", "Scare_airland", ScareLandCheck, NULL, 0, 0, 0.0f, 0.0f, 5, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Scare_airland", "Idle_scared", HealthCheck, NULL, 0x10, 2, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Scare_airland", "Idle_normal", HealthCheck, NULL, 0x10, 4, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Scare_airland", "Idle_brave", HealthCheck, NULL, 0x10, 8, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Idle_scared Idle_normal Idle_brave", "Inactive_SitStart", IdleSitCheck, NULL, 0, 0, 0.0f, 0.0f, 2, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Inactive_SitStart", "Inactive_SitLoop", NULL, IdleSitCB, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Inactive_SitLoop Inactive_SitFlea Inactive_SitSniff Inactive_SitHungry Inactive_SitLookLeft", "Inactive_SitEnd", ActiveCheck, NULL, 0, 0xE, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Inactive_SitEndSlow", "Inactive_SitEnd", ActiveCheck, NULL, 0x20, 0, 0.0f, 0.0f, 2, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Inactive_SitLoop", "Inactive_SitEndSlow", IdleGetupCheck, IdleGetupCB, 0, 0, 0.0f, 0.0f, 2, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Inactive_SitEnd Inactive_SitEndSlow", "Idle_scared", HealthCheck, NULL, 0x10, 2, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Inactive_SitEnd Inactive_SitEndSlow", "Idle_normal", HealthCheck, NULL, 0x10, 4, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Inactive_SitEnd Inactive_SitEndSlow", "Idle_brave", HealthCheck, NULL, 0x10, 8, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Inactive_SitLoop", "Inactive_SitSniff", IdleMinorCheck, IdleMinorCB, 0, 0x20000, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Inactive_SitLoop", "Inactive_SitLookLeft", IdleMinorCheck, IdleMinorCB, 0, 0x20001, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Inactive_SitLoop", "Inactive_SitFlea", IdleMajorCheck, IdleMajorCB, 0, 0x30000, 0.0f, 0.0f, 2, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Inactive_SitLoop", "Inactive_SitHungry", IdleMajorCheck, IdleMajorCB, 0, 0x30001, 0.0f, 0.0f, 2, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Inactive_SitLoop", "Inactive_Magnify", IdleMajorCheck, IdleMajorCB, 0, 0x30002, 0.0f, 0.0f, 2, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Inactive_SitFlea Inactive_SitSniff Inactive_SitHungry Inactive_Magnify Inactive_SitLookLeft", "Inactive_SitLoop", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Idle_normal Idle_brave", "Idle_LookLeft", IdleMinorCheck, IdleMinorCB, 0, 0x40000, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_normal Idle_brave", "Idle_LookRight", IdleMinorCheck, IdleMinorCB, 0, 0x40001, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_normal Idle_brave", "Inactive_SniffAir", IdleMinorCheck, IdleMinorCB, 0, 0x40002, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_normal Idle_brave", "Inactive_SniffGround", IdleMinorCheck, IdleMinorCB, 0, 0x40003, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_normal Idle_brave", "Inactive_LookAround", IdleMajorCheck, IdleMajorCB, 0, 0x10000, 0.0f, 0.0f, 2, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Idle_LookLeft Idle_LookRight Inactive_SniffAir Inactive_SniffGround Inactive_LookAround", "Idle_scared", HealthCheck, NULL, 0x10, 2, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_LookLeft Idle_LookRight Inactive_SniffAir Inactive_SniffGround Inactive_LookAround", "Idle_normal", HealthCheck, NULL, 0x10, 4, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Idle_LookLeft Idle_LookRight Inactive_SniffAir Inactive_SniffGround Inactive_LookAround", "Idle_brave", HealthCheck, NULL, 0x10, 8, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, " Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify Walk_normal Walk_scared Walk_brave Walk_slippery Walk_plunger Walk_sticky Walk_boots", "Powerup_helmet", PowerupCheck, PowerupCB, 0, 0xB, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, " Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify Walk_normal Walk_scared Walk_brave Walk_slippery Walk_plunger Walk_sticky Walk_boots", "Powerup_plunger", PowerupCheck, PowerupCB, 0, 3, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, " Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify Walk_normal Walk_scared Walk_brave Walk_slippery Walk_plunger Walk_sticky Walk_boots", "Powerup_soap", PowerupCheck, PowerupCB, 0, 1, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, " Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify Walk_normal Walk_scared Walk_brave Walk_slippery Walk_plunger Walk_sticky Walk_boots", "Powerup_lamp", PowerupCheck, PowerupCB, 0, 5, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Powerup_helmet Powerup_plunger Powerup_soap Powerup_lamp", "Idle_scared", HealthCheck, NULL, 0x10, 2, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Powerup_helmet Powerup_plunger Powerup_soap Powerup_lamp", "Idle_normal", HealthCheck, NULL, 0x10, 4, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "Powerup_helmet Powerup_plunger Powerup_soap Powerup_lamp", "Idle_brave", HealthCheck, NULL, 0x10, 8, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    tran = xAnimTableNewTransition(table, " Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify", "Action_projectile", ProjectileCheck, ProjectileCB, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableAddTransition(table, tran, "Walk_normal Walk_scared Walk_brave");
    xAnimTableAddTransition(table, tran, "Idle_sneak Idle_lampshade Idle_blackknight Walk_sneak Walk_blackknight");
    xAnimTableAddTransition(table, tran, "Idle_teeter Idle_slippery Idle_plunger Inactive_1 Inactive_2 Inactive_3 Inactive_4");
    xAnimTableAddTransition(table, tran, "Run_normal Run_scared Run_brave");
    xAnimTableNewTransition(table, "Action_projectile", "Action_projectile", ProjectileRepeatSync, ProjectileCB, 0, 0, globals.player.s.FireRepeat, 0.0f, 4, 5, 10.0f, NULL);
    xAnimTableNewTransition(table, "Action_projectile", "Action_projectile", ProjectileRepeat, ProjectileCB, 0, 0, 0.0f, 0.0f, 4, 0, 10.0f, NULL);
    tran = xAnimTableNewTransition(table, " Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify", "Action_attackrun", AttackCheck, AttackCB, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableAddTransition(table, tran, "Walk_normal Walk_scared Walk_brave");
    xAnimTableAddTransition(table, tran, "Idle_sneak Idle_lampshade Idle_blackknight Walk_sneak Walk_blackknight");
    xAnimTableAddTransition(table, tran, "Idle_teeter Idle_slippery Idle_plunger Inactive_1 Inactive_2 Inactive_3 Inactive_4");
    xAnimTableAddTransition(table, tran, "Run_normal Run_scared Run_brave Run_hotsauce");
    xAnimTableAddTransition(table, tran, "Land_normal Land_walk Land_walkidle Land_running");
    xAnimTableAddTransition(table, tran, "Walk_boots Run_boots");
    xAnimTableNewTransition(table, "Action_Attack Action_AttackRun", "Action_attackhit", HeadHitCheck, HeadHitCB, 0, 0, 0.0f, 0.0f, 2, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Action_attackhit", "Idle_scared", HealthCheck, NULL, 0x10, 2, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Action_attackhit", "Idle_normal", HealthCheck, NULL, 0x10, 4, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Action_attackhit", "Idle_brave", HealthCheck, NULL, 0x10, 8, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Action_attackrun", "RunToIdle_scared", AttackFinished, NotRunCB, 0, 2, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attackrun", "RunToIdle_normal", AttackFinished, NotRunCB, 0, 4, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attackrun", "RunToIdle_brave", AttackFinished, NotRunCB, 0, 8, 0.0f, 0.0f, 1, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attackrun", "Run_scared", AttackFinishedRun, NULL, 0, 2, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attackrun", "Run_normal", AttackFinishedRun, NULL, 0, 4, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attackrun", "Run_brave", AttackFinishedRun, NULL, 0, 8, 0.0f, 0.0f, 2, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_attackrun", "Run_hotsauce", AttackFinishedHotsauce, NULL, 0, 1, 0.0f, 0.0f, 3, 0, 0.15f, NULL);
    tran = xAnimTableNewTransition(table, " Idle_scared Idle_normal Idle_brave Idle_teeter Idle_slippery Idle_plunger Idle_LookLeft Idle_LookRight Inactive_LookAround Inactive_SniffAir Inactive_SniffGround Inactive_Magnify", "Action_dig", DigCheck, DigCB, 0, 0, 0.0f, 0.0f, 4, 0, 0.15f, NULL);
    xAnimTableAddTransition(table, tran, "Walk_normal Walk_scared Walk_brave");
    xAnimTableAddTransition(table, tran, "Idle_sneak Idle_lampshade Idle_blackknight Walk_sneak Walk_blackknight");
    xAnimTableAddTransition(table, tran, "Inactive_1 Inactive_2 Inactive_3 Inactive_4");
    xAnimTableNewTransition(table, "Action_dig", "Idle_scared", StopDigCheck, NULL, 0, 2, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_dig", "Idle_normal", StopDigCheck, NULL, 0, 4, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    xAnimTableNewTransition(table, "Action_dig", "Idle_brave", StopDigCheck, NULL, 0, 8, 0.0f, 0.0f, 0, 0, 0.15f, NULL);
    return table;
}

static unsigned int FaceBlow(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.Face_AnimSpecific == 6;
}

static unsigned int FaceMad(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.Face_AnimSpecific == 5;
}

static unsigned int FaceClearScareCB(xAnimTransition*, xAnimSingle*, void*)
{
    globals.player.Face_ScareTimer = 0.0f;
    return 0;
}

static unsigned int FaceScare(xAnimTransition* tran, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.Face_ScareTimer != 0.0f && globals.player.Face_ScareRandom == tran->UserFlags)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int FaceEvent(xAnimTransition* tran, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.Face_EventTimer != 0.0f && globals.player.Face_Event == tran->UserFlags &&
        globals.player.Face_ScareTimer == 0.0f && globals.player.Face_AnimSpecific < 5)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int FaceRun(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.Face_AnimSpecific == 4 && globals.player.Face_ScareTimer == 0.0f &&
        globals.player.Face_EventTimer == 0.0f)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int FacePant(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.Face_PantTimer != 0.0f && globals.player.Face_AnimSpecific < 4 &&
        globals.player.Face_ScareTimer == 0.0f && globals.player.Face_EventTimer == 0.0f)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int FaceTeeter(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.Face_AnimSpecific == 1 && globals.player.Face_ScareTimer == 0.0f &&
        globals.player.Face_EventTimer == 0.0f && globals.player.Face_PantTimer == 0.0f)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int FaceSneak(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.Face_AnimSpecific == 2 && globals.player.Face_ScareTimer == 0.0f &&
        globals.player.Face_EventTimer == 0.0f && globals.player.Face_PantTimer == 0.0f)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int FaceSwing(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.Face_AnimSpecific == 3 && globals.player.Face_ScareTimer == 0.0f &&
        globals.player.Face_EventTimer == 0.0f && globals.player.Face_PantTimer == 0.0f)
    {
        ret = 1;
    }
    return ret;
}

static unsigned int FaceIdle(xAnimTransition*, xAnimSingle*, void*)
{
    unsigned char ret = 0;
    if (globals.player.Face_AnimSpecific == 0 && globals.player.Face_ScareTimer == 0.0f &&
        globals.player.Face_EventTimer == 0.0f && globals.player.Face_PantTimer == 0.0f)
    {
        ret = 1;
    }
    return ret;
}

static void PlayerFaceUpdate(float dt)
{
    if (globals.player.Face_ScareTimer > 0.0f)
    {
        globals.player.Face_ScareTimer -= dt;
        if (globals.player.Face_ScareTimer < 0.0f)
        {
            globals.player.Face_ScareTimer = 0.0f;
        }
    }
    if (globals.player.Face_EventTimer > 0.0f)
    {
        globals.player.Face_EventTimer -= dt;
        if (globals.player.Face_EventTimer < 0.0f)
        {
            globals.player.Face_EventTimer = 0.0f;
        }
    }
    if (globals.player.Face_PantTimer > 0.0f)
    {
        globals.player.Face_PantTimer -= dt;
        if (globals.player.Face_PantTimer < 0.0f)
        {
            globals.player.Face_PantTimer = 0.0f;
        }
    }
    globals.player.Face_AnimSpecific = 0;
    xAnimState* state = globals.player.ent.model->Anim->Single->State;
    if (state->UserFlags & 0x40000)
    {
        globals.player.Face_AnimSpecific = 1;
    }
    if ((state->UserFlags & 0x1E) == 2)
    {
        globals.player.Face_AnimSpecific = 2;
    }
    if ((state->UserFlags & 0x1E) == 6 && !(state->UserFlags & 0x20))
    {
        globals.player.Face_AnimSpecific = 4;
    }
    if (state->UserFlags & 0x20000)
    {
        globals.player.Face_AnimSpecific = 3;
    }
    if (state->UserFlags & 0x10000)
    {
        globals.player.Face_AnimSpecific = 5;
    }
    if (state->UserFlags & 0x80000)
    {
        globals.player.Face_AnimSpecific = 6;
    }
}

xAnimTable* zEntPlayer_HeadAnimTable()
{
    xAnimTable* table = xAnimTableNew("PlayerHead", NULL);
    xAnimTableNewState(table, "Idle", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "IdleA", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "IdleB", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "IdleC", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "ScareA", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "ScareB", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "ScareC", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "ScareD", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "ScareE", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "ScareF", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Blink", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Blow", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Dizzy", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Grin", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Happy", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Mad", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Sneak", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Swing", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Teeter", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Run", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Pant", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "LickFace", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "WinkGrin", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewTransition(table, "LickFace WinkGrin", "Idle", NULL, NULL, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
    PlayerHeadAddTransition(table, "ScareA", FaceScare, NULL, 0, 0, 6);
    PlayerHeadAddTransition(table, "ScareB", FaceScare, NULL, 0, 1, 6);
    PlayerHeadAddTransition(table, "ScareC", FaceScare, NULL, 0, 2, 6);
    PlayerHeadAddTransition(table, "ScareD", FaceScare, NULL, 0, 3, 6);
    PlayerHeadAddTransition(table, "ScareE", FaceScare, NULL, 0, 4, 6);
    PlayerHeadAddTransition(table, "ScareF", FaceScare, NULL, 0, 5, 6);
    PlayerHeadAddTransition(table, "Mad", FaceMad, FaceClearScareCB, 0, 0, 5);
    PlayerHeadAddTransition(table, "Blow", FaceBlow, FaceClearScareCB, 0, 0, 5);
    PlayerHeadAddTransition(table, "Happy", FaceEvent, NULL, 0, 0, 4);
    PlayerHeadAddTransition(table, "Grin", FaceEvent, NULL, 0, 1, 4);
    PlayerHeadAddTransition(table, "Dizzy", FaceEvent, NULL, 0, 2, 4);
    PlayerHeadAddTransition(table, "Run", FaceRun, NULL, 0, 0, 3);
    PlayerHeadAddTransition(table, "Pant", FacePant, NULL, 0, 0, 3);
    PlayerHeadAddTransition(table, "Teeter", FaceTeeter, NULL, 0, 0, 3);
    PlayerHeadAddTransition(table, "Sneak", FaceSneak, NULL, 0, 0, 3);
    PlayerHeadAddTransition(table, "Swing", FaceSwing, NULL, 0, 0, 3);
    PlayerHeadAddTransition(table, "Idle", FaceIdle, NULL, 0, 0, 1);
    return table;
}

xAnimTable* zEntPlayer_TongueAnimTable()
{
    xAnimTable* table = xAnimTableNew("PlayerHead", NULL);
    xAnimTableNewState(table, "Idle", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    return table;
}

xAnimTable* zEntPlayer_UmbrellaAnimTable()
{
    xAnimTable* table = xAnimTableNew("PlayerUmbrella", NULL);
    xAnimTableNewState(table, "Idle", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    return table;
}

static unsigned int LampDrawCheck(xAnimTransition*, xAnimSingle* anim, void*)
{
    return !(anim->Play->ModelInst->Flags & 1);
}

static unsigned int LamponCheck(xAnimTransition*, xAnimSingle* anim, void*)
{
    if (anim->Play->ModelInst->Flags & 1)
    {
        return 0;
    }
    return strcmp(globals.player.ent.model->Anim->Single->State->Name, "IdleToLampshade") == 0;
}

static unsigned int LampoffCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return strcmp(globals.player.ent.model->Anim->Single->State->Name, "LampshadeToSneak") == 0;
}

static unsigned int LampbackCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return strcmp(globals.player.ent.model->Anim->Single->State->Name, "SneakToLampshade") == 0;
}

static unsigned int LampFixbugCheck(xAnimTransition*, xAnimSingle*, void*)
{
    return strcmp(globals.player.ent.model->Anim->Single->State->Name, "Idle_lampshade") == 0;
}

xAnimTable* zEntPlayer_LampshadeAnimTable()
{
    xAnimTable* table = xAnimTableNew("PlayerLampshade", NULL);
    xAnimTableNewState(table, "Idle", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "TurnOff", 0, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "TurnBackOn", 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewTransition(table, "Idle TurnOff", "Idle", LamponCheck, NULL, 0, 0, 0.0f, 0.0f, 2, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "Idle", "TurnOff", LampoffCheck, NULL, 0, 0, 0.0f, 0.0f, 2, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "TurnOff Idle", "TurnBackOn", LampbackCheck, NULL, 0, 0, 0.0f, 0.0f, 2, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "TurnBackOn", "TurnOff", LampoffCheck, NULL, 0x20, 0, 0.0f, 0.0f, 2, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "TurnOff", "TurnBackOn", LampbackCheck, NULL, 0x20, 0, 0.0f, 0.0f, 2, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "TurnOff", "TurnBackOn", LampFixbugCheck, NULL, 0x20, 0, 0.0f, 0.0f, 2, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "TurnOff", "Idle", LampDrawCheck, NULL, 0, 0, 0.0f, 10.0f, 1, 0, 0.0f, NULL);
    xAnimTableNewTransition(table, "TurnBackOn", "Idle", NULL, NULL, 0x10, 0, 0.0f, 10.0f, 1, 0, 0.0f, NULL);
    return table;
}

// Stand-in for a function the linker strips; it keeps "PARSYS_SD_" between
// zEntPlayer_Init's and zEntPlayer_LampshadeAnimTable's literals.
void zEntPlayerParSysDeadStripped(char* name)
{
    strcpy(name, "PARSYS_SD_");
}

void zEntPlayer_Init(_xEnt* ent, p2EntAsset* asset)
{
    zEntInit((_zEnt*)ent, asset, 'PLYR');
    ent->baseFlags |= 1;
    ent->flags |= 1;
    globals.player.Visible = 1;
    globals.player.AutoMoveSpeed = 0;
    ent->pflags &= 0xFB;
    ent->collis->chk &= ~1;
    ent->update = zEntPlayer_Update;
    ent->move = zEntPlayer_Move;
    ent->render = (void (*)(_xEnt*))zEntPlayer_Render;
    ent->eventFunc = zEntPlayerEventCB;
    if (ent->linkCount)
    {
        ent->link = (p2LinkAsset*)(asset + 1);
    }
    else
    {
        ent->link = NULL;
    }
    if (globals.sceneCur->baseCount[28])
    {
        xFFXAddEffect(ent, zGustUpdateEnt, &gust_data);
    }
    xFFXRotMatchState* rms = xFFXRotMatchAlloc();
    if (rms != NULL)
    {
        rms->max_decl = globals.player.s.RotMatchMaxAngle;
        rms->tmatch = globals.player.s.RotMatchMatchTime;
        rms->trelax = globals.player.s.RotMatchRelaxTime;
        rms->tmr = 0.0f;
        xFFXAddEffect(ent, PlayerRotMatchUpdateEnt, rms);
    }
    memset(&gPlayerfx, 0, sizeof(_tagPlayerFX));
    zEntPlayerFindEmitters();
    zFeetInit(-1, ent, &sPlayerFeet[0], 0, ent->model->Anim->Table);
    zFeetInit(-1, ent, &sPlayerFeet[1], 1, ent->model->Anim->Table);
    zFeetInit(-1, ent, &sPlayerFeet[2], 2, ent->model->Anim->Table);
    zFeetInit(-1, ent, &sPlayerFeet[3], 3, ent->model->Anim->Table);
    zFeetInit(-1, ent, &sPlayerFeet[4], 4, ent->model->Anim->Table);
    zFeetInit(-1, ent, &sPlayerFeet[5], 5, ent->model->Anim->Table);
    zFeetInit(-1, ent, &sPlayerFeet[6], 6, ent->model->Anim->Table);
    zFeetInit(-1, ent, &sPlayerFeet[7], 7, ent->model->Anim->Table);
    zFeetInit(-1, ent, &sPlayerFeet[8], 8, ent->model->Anim->Table);
    zFeetInit(-1, ent, &sPlayerFeet[9], 9, ent->model->Anim->Table);
    zEntPlayer_FXSetup(ent->model->Anim->Table);
    for (signed int i = 0; i < 5; i++)
    {
        zFeetInitSkidMark(&sPlayerSkidMark[i], 4);
    }
    sSnackDecrement = 0;
    sDecrementSnackGateEnt = NULL;
    sDecrementDelay = 0.0f;
    sDecrementAmount = 2.0f;
    sVONextSituation = NULL;
    sVOTimer = 0.0f;
    for (signed int i = 0; i < 15; i++)
    {
        sPlayerInfo[i].elapsedTime = sPlayerInfo[i].delay;
        sPlayerInfo[i].count = 0;
    }
    zEntPlayerReset(ent);
}

void zEntPlayer_Load(_xEnt*, xSerial*)
{
}

static void zEntPlayer_SnackDecrement(float dt)
{
    if (sSnackDecrement)
    {
        zEntEvent(&globals.player.ent, 8);
        zHud_showSnack();
        sDecrementDelay -= dt;
        if (sDecrementDelay <= 0.0f)
        {
            sDecrementDelay = 0.0f;
            zEntPickup_hideSnackGateInfo(sDecrementSnackGateEnt);
            xSndPlay(sPlayerSndfx.snackcounter_id, 0.5f, 0.5f, 0.0f, 0x80, 2, 0);
            sDecrementAmount += 2.5f * dt;
            unsigned int amount = sDecrementAmount;
            if (amount > sSnackDecrement)
            {
                amount = sSnackDecrement;
            }
            if ((signed int)(globals.player.Inv_ScoobySnacks - amount) < 0)
            {
                globals.player.Inv_ScoobySnacks = 0;
                sSnackDecrement = 0;
            }
            else
            {
                globals.player.Inv_ScoobySnacks -= amount;
                sSnackDecrement -= amount;
            }
            if (sSnackDecrement == 0)
            {
                zEntEvent(&globals.player.ent, 9);
                zEntPickup_UnlockSnackGate(sDecrementSnackGateEnt);
                zHud_showSnackSpecial();
                sDecrementSnackGateEnt = NULL;
            }
        }
    }
}

void zEntPlayer_Update(_xEnt* ent, _xScene* sc, float dt)
{
    _xCollis* coll;
    _xCollis* endcoll;
    unsigned int i;

    zEntPlayer_SnackDecrement(dt);
    if (!globals.player.ControlOn || globals.cmgr != NULL)
    {
        globals.player.ControlOffTimer = 1.0f;
    }

    globals.player.Sneak = 0;
    if (((globals.player.ControlOn && (globals.pad0->on & 0x2000)) ||
         (!globals.player.ControlOn && globals.player.AutoMoveSpeed == 3)) &&
        !surfSticky)
    {
        if (globals.player.Inv_Specials & 0x40)
        {
            globals.player.Sneak = 3;
            zLaughNotify(0);
        }
        else if (globals.player.Inv_Specials & 0x10)
        {
            globals.player.Sneak = ((globals.player.Inv_Specials >> 5) & 1) + 1;
            zLaughNotify(0);
        }
    }

    if (globals.player.ScareShaggy)
    {
        globals.player.ScareShaggy--;
    }

    xAnimState* astate = globals.player.ent.model->Anim->Single->State;
    if ((strcmp(astate->Name, "SD0_Hold") == 0 || strcmp(astate->Name, "SD0_HoldWalk") == 0 ||
         strcmp(astate->Name, "SD1_Hold") == 0 || strcmp(astate->Name, "SD1_HoldWalk") == 0) &&
        ((globals.pad0->pressed & 0x30000) ||
         ((globals.pad0->pressed & 0xC0000) && globals.player.cheat_mode == 0)) &&
        globals.player.Shaggy1.SetDown == 0 && globals.player.Shaggy1.ThrowState == 0)
    {
        zLaughNotify(0xE);
        if (astate->Name[2] == '1')
        {
            Shaggy1_ChooseThrowTarget(globals.player.Shaggy1.Grab);
        }
        else
        {
            globals.player.Shaggy1.ThrowState = 3;
        }
    }
    if (globals.player.Shaggy1.ThrowState == 2)
    {
        globals.player.Shaggy1.ThrowState = 3;
    }

    if (globals.player.ControlOn && globals.pad0->on == 0 &&
        labs(globals.pad0->analog1.x) <= globals.player.s.AnalogMin &&
        labs(globals.pad0->analog1.y) <= globals.player.s.AnalogMin)
    {
        xAnimState* state = globals.player.ent.model->Anim->Single->State;
        if (state != NULL && (state->UserFlags & 1))
        {
            globals.player.IdleMinorTimer += dt;
            globals.player.IdleMajorTimer += dt;
            if (!surfSticky)
            {
                globals.player.IdleSitTimer += dt;
            }
            if (globals.player.IdleMinorTimer == 0.0f)
            {
                globals.player.IdleMinorTimer = 0.0001f;
            }
        }
    }
    else
    {
        globals.player.IdleMinorTimer = 0.0f;
        globals.player.IdleMajorTimer = -3.0f;
        globals.player.IdleSitTimer = 0.0f;
    }

    if (globals.player.Inv_Specials & 0x40)
    {
        globals.player.WorldDisguise = 6;
    }
    else
    {
        globals.player.WorldDisguise = 5;
    }

    if (globals.player.HeadbuttTimer != 0.0f)
    {
        zLaughNotify(1);
        globals.player.HeadbuttTimer += dt;
        if (!globals.player.ControlOn || globals.player.HeadbuttTimer > globals.player.s.HeadbuttTimeMax ||
            (globals.player.HeadbuttTimer > globals.player.s.HeadbuttTimeMin && !(globals.pad0->on & 0x80000)))
        {
            globals.player.HeadbuttTimer = 0.0f;
        }
        else
        {
            globals.player.HeadbuttVel += globals.player.s.HeadbuttAccel * dt;
            if (globals.player.HeadbuttVel > globals.player.s.HeadbuttSpeedMax)
            {
                globals.player.HeadbuttVel = globals.player.s.HeadbuttSpeedMax;
            }
        }
    }

    for (i = 0; i < 37; i++)
    {
        sPlayerInfo[i].elapsedTime += dt;
    }

    if (sVOTimer != 0.0f || sVONextSituation != NULL)
    {
        if (sVOTimer > 0.0f)
        {
            sVOTimer -= dt;
            if (sVOTimer < 0.0f)
            {
                sVOTimer = 0.0f;
            }
        }
        if (sVOTimer == 0.0f && sVONextSituation != NULL)
        {
            zEntPlayer_SituationDo();
        }
    }

    if (globals.player.ScareTimer > 0.0f)
    {
        globals.player.ScareTimer -= dt;
        if (globals.player.ScareTimer < 0.0f)
        {
            globals.player.ScareTimer = 0.0f;
        }
    }
    if (globals.player.DamageTimer > 0.0f)
    {
        globals.player.DamageTimer -= dt;
        if (globals.player.DamageTimer < 0.0f)
        {
            globals.player.DamageTimer = 0.0f;
            globals.player.DamageSource = NULL;
        }
    }
    if (globals.player.ControlOffTimer > 0.0f)
    {
        globals.player.ControlOffTimer -= dt;
        if (globals.player.ControlOffTimer < 0.0f)
        {
            globals.player.ControlOffTimer = 0.0f;
        }
    }
    if (globals.player.DamageTimer != 0.0f && globals.player.Health)
    {
        globals.player.Transparent++;
        if (globals.player.Transparent == 4)
        {
            globals.player.Transparent = -2;
        }
    }
    else
    {
        globals.player.Transparent = 0;
    }
    if (globals.player.CowerTimer > 0.0f)
    {
        globals.player.CowerTimer -= dt;
        if (globals.player.CowerTimer < 0.0f)
        {
            globals.player.CowerTimer = 0.0f;
        }
    }
    if (globals.player.FallDeathTimer > 0.0f)
    {
        globals.player.FallDeathTimer -= dt;
        if (globals.player.FallDeathTimer < 0.0f)
        {
            globals.player.FallDeathTimer = 0.0f;
        }
    }
    if (globals.player.HelmetTimer > 0.0f)
    {
        globals.player.HelmetTimer -= dt;
        if (globals.player.HelmetTimer < 0.0f)
        {
            globals.player.HelmetTimer = 0.0f;
        }
    }

    PlayerFaceUpdate(dt);

    iColor_tag clear = { 0, 0, 0, 0xFF };
    iColor_tag black = { 0 };
    if (globals.player.Health == 0)
    {
        if (globals.player.DamageTimer <= 0.3f && xScrFxIsFading() == 0)
        {
            xScrFxFade(&black, &clear, 0.3f, NULL, 1, 1);
        }
        if (globals.player.DamageTimer == 0.0f)
        {
            zSceneGameStatsTally_death();
            zGameStateSwitch(2);
            gPlayerHitWater = 0;
            globals.player.Health = 3;
            sPlayerWaterOffset = 0.0f;
            gDeadScooby = 0;
        }
    }

    PlayerHangCheck(ent, sc);
    update_dt = dt;
    xEntBeginUpdate(ent, sc, dt);

    if (globals.player.ShockRadius != 0.0f)
    {
        globals.player.ShockRadius = 6.0f * dt + globals.player.ShockRadius;
        if (globals.player.ShockRadius > 4.0f)
        {
            globals.player.ShockRadius = 4.0f;
        }
        float rad2 = globals.player.ShockRadius * globals.player.ShockRadius;
        float radold2 = globals.player.ShockRadiusOld * globals.player.ShockRadiusOld;
        for (i = 0; i < sc->num_npcs; i++)
        {
            _xEnt* vill = sc->npcs[i];
            float dx = vill->frame->mat.pos.x - ent->frame->mat.pos.x;
            float dz = vill->frame->mat.pos.z - ent->frame->mat.pos.z;
            float dxo = vill->frame->oldpos.x - ent->frame->mat.pos.x;
            float dzo = vill->frame->oldpos.z - ent->frame->mat.pos.z;
            float sdist2 = dx * dx + dz * dz;
            float sdistold2 = dxo * dxo + dzo * dzo;
            float ydist = (float)__fabs(vill->frame->mat.pos.y - ent->frame->mat.pos.y);
            if (((rad2 > sdist2 && radold2 <= sdist2) || (rad2 > sdistold2 && radold2 <= sdistold2)) &&
                ydist < 0.5f)
            {
                zEntVillain_Stun((st_zEntVillain*)vill, (4.0f - xsqrt(rad2)) / 3.4f);
            }
        }
        if (globals.player.ShockRadius == 4.0f)
        {
            globals.player.ShockRadius = 0.0f;
        }
        else
        {
            globals.player.ShockRadiusOld = globals.player.ShockRadius;
        }
    }

    zEntBubble_EnableCollision();
    signed int num_updates = 1;
    if (globals.player.HangEnt == NULL)
    {
        if (dt > 0.1f)
        {
            num_updates = 6;
        }
        else if (dt > 1.0f / 12.0f)
        {
            num_updates = 5;
        }
        else if (dt > 1.0f / 15.0f)
        {
            num_updates = 4;
        }
        else if (dt > 0.05f)
        {
            num_updates = 3;
        }
        else if (dt > 1.0f / 30.0f)
        {
            num_updates = 2;
        }
    }

    float sdt = dt / num_updates;
    float fg = 0.5f * sdt;
    float sett2 = 13.0f * sdt;
    for (signed int updidx = 0; updidx < num_updates; updidx++)
    {
        _xVec3 suboldpos = ent->frame->mat.pos;
        HeadcollUpdate(sdt);
        xEntApplyPhysics(ent, sc, sdt);
        xEntMove(ent, sc, sdt);
        req_motion = ent->frame->dpos;
        xEntDriveUpdate(&globals.player.drv, sdt);
        if (globals.player.MountChimney != NULL && globals.player.MountChimney->eventFunc == Shaggy2Plat_EventCB)
        {
            globals.player.ent.frame->mat.pos.y +=
                ((_xMat4x3*)globals.player.MountChimney->model->Mat)->pos.y - globals.player.MountChimOldY;
        }
        if (globals.sceneCur->baseCount[28])
        {
            for (xFFX* gust_fkt = ent->ffx; gust_fkt != NULL; gust_fkt = gust_fkt->next)
            {
                if (gust_fkt->doEffect == zGustUpdateEnt)
                {
                    if (globals.player.JumpState == 3 || surfSlickRatio != 0.0f)
                    {
                        xFFXTurnOn(gust_fkt);
                    }
                    else
                    {
                        xFFXTurnOff(gust_fkt);
                        if (gust_data.gust_on)
                        {
                            memset(&gust_data, 0, sizeof(zGustData));
                        }
                    }
                }
            }
        }
        else
        {
            gust_data.gust_on = 0;
        }
        xFFXApply(ent, sc, sdt);

        float dx = ent->frame->mat.pos.x - suboldpos.x;
        float dy = ent->frame->mat.pos.y - suboldpos.y;
        float dz = ent->frame->mat.pos.z - suboldpos.z;
        if (globals.player.cheat_mode == 0 && (globals.player.JumpState == 0 || globals.player.JumpState == 1) &&
            globals.player.FallDeathTimer == 0.0f)
        {
            ent->frame->mat.pos.y -= fg;
            float ndotm = globals.player.floor_norm.x * dx + globals.player.floor_norm.z * dz;
            if (ndotm > 0.0f && (req_motion.x != 0.0f || req_motion.z != 0.0f || surfSlickRatio != 0.0f))
            {
                globals.player.slope = -1;
                float m = xsqrt(ndotm);
                float sdy = (dy - fg) - m;
                ent->frame->mat.pos.y -= m;
                float s = xsqrt((dz * dz + (dy * dy + dx * dx)) / (dz * dz + (sdy * sdy + dx * dx)));
                s = 0.25f * (s - 1.0f);
                ent->frame->mat.pos.x += s * dx;
                ent->frame->mat.pos.y += s * sdy;
                ent->frame->mat.pos.z += s * dz;
            }
            else if (ndotm < 0.0f)
            {
                globals.player.slope = 1;
            }
            else
            {
                globals.player.slope = 0;
            }
        }

        if (lastFloorEnt != NULL)
        {
            if (surfSlickRatio != 0.0f)
            {
                float mag = globals.player.floor_norm.x * globals.player.floor_norm.x +
                            globals.player.floor_norm.z * globals.player.floor_norm.z;
                ent->frame->mat.pos.x = sett2 * globals.player.floor_norm.x + ent->frame->mat.pos.x;
                ent->frame->mat.pos.y = ent->frame->mat.pos.y - sett2 * xsqrt(mag);
                ent->frame->mat.pos.z = sett2 * globals.player.floor_norm.z + ent->frame->mat.pos.z;
            }
            else if (globals.player.SlipFadeTimer > 0.0f)
            {
                float lerpspeed = 30.0f * ((globals.player.SlipFadeTimer / 0.8f) * sdt);
                ent->frame->mat.pos.x = lerpspeed * lastFloorNorm.x + ent->frame->mat.pos.x;
                ent->frame->mat.pos.z = lerpspeed * lastFloorNorm.z + ent->frame->mat.pos.z;
            }
        }
        zEntPlayerCollide(ent, sc, sdt);
    }

    update_motion.x = ent->frame->mat.pos.x - ent->frame->oldpos.x;
    update_motion.y = ent->frame->mat.pos.y - ent->frame->oldpos.y;
    update_motion.z = ent->frame->mat.pos.z - ent->frame->oldpos.z;
    dt = update_dt;

    if (globals.player.cheat_mode == 0)
    {
        _xCollis bbc;
        bbc.flags = 0;
        _xBox bbox = *(_xBox*)((char*)globals.sceneCur->env->geom->world + 0x50);
        bbox.upper.y += 10.0f;
        xSphereHitsBox(&ent->bound.sph, &bbox, &bbc);
        if (!(bbc.flags & 0x11))
        {
            zEntEvent(ent, 0x11);
        }
    }

    if (globals.player.cheat_mode == 0)
    {
        _xRay3 ctr;
        _xCollis ctc;
        ctr.origin = ent->frame->oldpos;
        ctr.origin.y += 0.7f;
        ctr.max_t = xVec3Normalize(&ctr.dir, &update_motion);
        ctr.min_t = 0.0f;
        ctr.flags = 0xC00;
        ctc.flags = 0;
        iRayHitsEnv(&ctr, globals.sceneCur->env, &ctc);
        if (ctc.flags & 1)
        {
            ent->frame->mat.pos = ent->frame->oldpos;
        }
        else
        {
            _xCollis* cend = &ent->collis->colls[ent->collis->idx];
            for (_xCollis* c = &ent->collis->colls[6]; c < cend; c++)
            {
                _xEnt* cent = (_xEnt*)c->optr;
                if (cent != NULL &&
                    (cent->id == xStrHash("slipdeck01") || cent->id == xStrHash("slipdeck02") ||
                     cent->id == xStrHash("slipdeck03") || cent->id == xStrHash("slipdeck04")) &&
                    (iRayHitsModel(&ctr, cent->model, &ctc), ctc.flags & 1))
                {
                    ent->frame->mat.pos = ent->frame->oldpos;
                    break;
                }
            }
        }
    }

    for (_xCollis* wcoll = &ent->collis->colls[2]; wcoll < &ent->collis->colls[6]; wcoll++)
    {
        if (wcoll->flags & 1)
        {
            float hdotm = wcoll->hdng.x * req_motion.x + wcoll->hdng.z * req_motion.z;
            xSurface* wsurf = zSurfaceGetSurface(wcoll);
            if (hdotm > 0.0f && wcoll->hdng.y < 0.0f && zSurfaceGetStep(wsurf))
            {
                ent->frame->mat.pos.y += 1.5f * dt;
                _xCollis* gcoll = &ent->collis->colls[0];
                *gcoll = *wcoll;
                wcoll->flags = 0;
            }
        }
    }

    zEntBubble_DisableCollision();
    PlayerEGenCheck(ent, (zScene*)sc, dt);
    globals.player.HeadbuttHit = 0;
    ButtonBBCheck(ent);
    DestructObjBBCheck(ent);

    if (globals.player.Shaggy1.GrabOn)
    {
        if (ent->collis->npc_eidx > ent->collis->npc_sidx &&
            (ent->model->Anim->Single->State->UserFlags & 0x400))
        {
            endcoll = &ent->collis->colls[ent->collis->npc_eidx];
            for (coll = &ent->collis->colls[ent->collis->npc_sidx]; coll < endcoll; coll++)
            {
                _zEnt* shagent = (_zEnt*)coll->optr;
                if ((shagent->subType == 0x1C || shagent->subType == 0x1D) &&
                    (float)__fabs(((_xMat4x3*)shagent->model->Mat)->pos.y - ((_xMat4x3*)ent->model->Mat)->pos.y) <
                        0.5f &&
                    PlayerFacing(ent, shagent, 0.86602f))
                {
                    globals.player.Shaggy1.GrabTry = shagent;
                    break;
                }
            }
        }
    }

    if (ent->collis->npc_eidx > ent->collis->npc_sidx && globals.player.Shaggy1.Grab != NULL &&
        globals.player.Shaggy1.GrabOn && !zEntShaggy_GetSwap(globals.player.Shaggy1.Grab) &&
        globals.player.Shaggy1.Grab->subType == 0x1C)
    {
        endcoll = &ent->collis->colls[ent->collis->npc_eidx];
        for (coll = &ent->collis->colls[ent->collis->npc_sidx]; coll < endcoll; coll++)
        {
            _zEnt* shagent = (_zEnt*)coll->optr;
            if (shagent == globals.player.Shaggy1.Grab && coll->hdng.y < -0.7071f)
            {
                ent->frame->mat.pos.x += 0.001f;
                globals.player.Shaggy1.CatchYou = 1;
                globals.player.Shaggy1.CatchHeight = ent->frame->oldpos.y - shagent->frame->mat.pos.y;
                ent->frame->mat.pos = shagent->frame->mat.pos;
                *(_xVec3*)&((_xMat4x3*)ent->model->Mat)->pos = *(_xVec3*)&((_xMat4x3*)shagent->model->Mat)->pos;
                break;
            }
        }
    }

    if (ent->collis->npc_eidx > ent->collis->npc_sidx)
    {
        coll = &ent->collis->colls[ent->collis->npc_sidx];
        endcoll = &ent->collis->colls[ent->collis->npc_eidx];
        unsigned int type = ent->model->Anim->Single->State->UserFlags & 0x1E;
        if (type == 0xC)
        {
            HeadbuttTest(coll, endcoll, 0.7071f, VillHBCallback);
        }
        else if (globals.player.Health &&
                 (type == 0xE ||
                  (type == 0xA && ent->frame->mat.pos.y < ent->frame->oldpos.y &&
                   (((_xEnt*)coll->optr)->subType == 0x16 || ((_xEnt*)coll->optr)->subType == 0x18 ||
                    ((_xEnt*)coll->optr)->subType == 0 || ((_xEnt*)coll->optr)->subType == 0x15 ||
                    ((_xEnt*)coll->optr)->subType == 0x24))))
        {
            for (; coll < endcoll; coll++)
            {
                if (coll->hdng.y < -0.5f)
                {
                    signed int bounceResponse = 0;
                    if (((_xEnt*)coll->optr)->subType == 0x24)
                    {
                        if (Shaggy8_BBCallback(ent, coll))
                        {
                            bounceResponse = 2;
                        }
                    }
                    else
                    {
                        bounceResponse = zEntVillain_Bounce((st_zEntVillain*)coll->optr);
                    }
                    switch (bounceResponse)
                    {
                    case 1:
                    case 2:
                    {
                        zJumpParam* jump = &globals.player.s.Spring;
                        ent->frame->mat.pos = *(_xVec3*)&((_xMat4x3*)ent->model->Mat)->pos;
                        startBounce = ent->frame->mat.pos.y;
                        if (bounceResponse == 1)
                        {
                            jump = &globals.player.s.Bounce;
                        }
                        zEntPlayerJumpStart(ent, jump);
                        unsigned char st = ((_xEnt*)coll->optr)->subType;
                        if (st == 0x16 || st == 0x18 || st == 0x15 || st == 0)
                        {
                            globals.player.Bounced = 1;
                        }
                        if (((_xEnt*)coll->optr)->subType == 0x24)
                        {
                            globals.player.Bounced = 2;
                        }
                        globals.player.Jump_CanDouble = 1;
                        globals.player.Jump_CanFloat = 1;
                        zRumbleStart(0, (_tagSDRumbleType)2);
                        zEntPlayer_SNDButtBounce(0);
                    }
                    case 4:
                        if (coll->hdng.x == 0.0f && coll->hdng.z == 0.0f)
                        {
                            ent->frame->vel.y = 0.0f;
                        }
                        break;
                    }
                    break;
                }
            }
        }
        else
        {
            signed int took_damage = 0;
            for (; coll < endcoll; coll++)
            {
                switch (zEntVillain_Bumped((st_zEntVillain*)coll->optr, coll))
                {
                case 2:
                    if (coll->hdng.y < -0.5f)
                    {
                        startBounce = ent->frame->mat.pos.y;
                        zEntPlayerJumpStart(ent, &globals.player.s.Spring);
                        globals.player.Bounced = 2;
                        globals.player.Jump_CanDouble = 1;
                        globals.player.Jump_CanFloat = 1;
                        zRumbleStart(0, (_tagSDRumbleType)2);
                    }
                    break;
                case 3:
                    if (zEntPlayer_Damage((xBase*)coll->optr, 1) != 0)
                    {
                        zRumbleStart(0, (_tagSDRumbleType)4);
                    }
                    took_damage = 1;
                    break;
                case 4:
                    if (zEntPlayer_Damage((xBase*)coll->optr, 1) != 0)
                    {
                        zRumbleStart(0, (_tagSDRumbleType)4);
                    }
                    took_damage = 1;
                    break;
                }
                if (took_damage)
                {
                    break;
                }
            }
        }
    }

    if (globals.player.Health)
    {
        if (ent->collis->dyn_eidx > ent->collis->dyn_sidx)
        {
            endcoll = &ent->collis->colls[ent->collis->dyn_eidx];
            for (coll = &ent->collis->colls[ent->collis->dyn_sidx]; coll < endcoll; coll++)
            {
                _zEntBubble* bub = (_zEntBubble*)coll->optr;
                if (bub == NULL)
                {
                    continue;
                }
                if (bub->baseType == 0xF)
                {
                    if (coll->hdng.y < -0.866025f)
                    {
                        if (bub->subType == 0)
                        {
                            startBounce = ent->frame->mat.pos.y;
                            zEntPlayerJumpStart(ent, &globals.player.s.Spring);
                            globals.player.Jump_CanDouble = 1;
                            globals.player.Jump_CanFloat = 1;
                        }
                        else if (bub->subType == 1)
                        {
                            st_zEntVillain* vill = (st_zEntVillain*)bub->villain;
                            if (vill != NULL && (ent->model->Anim->Single->State->UserFlags & 0x1E) == 0xE)
                            {
                                zEntVillain_Bounce(vill);
                            }
                        }
                    }
                    else if (bub->subType == 1)
                    {
                        st_zEntVillain* vill = (st_zEntVillain*)bub->villain;
                        if (vill != NULL && (ent->model->Anim->Single->State->UserFlags & 0x1E) == 0xC)
                        {
                            zEntVillain_Headbutt(vill, &coll->hdng);
                        }
                    }
                    break;
                }
            }
        }
    }

    zEntPlayerFloorUpdate(ent, dt);
    PlayerTeeterCheck(dt);
    zEntPlayerSurfDamageUpdate(ent);
    zEntPlayerDriveUpdate(ent, dt);
    zEntPlayerJumpUpdate(ent, dt);
    zEntPlayerCollTrigger(ent, sc);

    if (globals.player.RootUp.y != globals.player.RootUpTarget.y || globals.player.RootUp.y != 1.0f)
    {
        float dx = globals.player.RootUpTarget.x - globals.player.RootUp.x;
        float dy = globals.player.RootUpTarget.y - globals.player.RootUp.y;
        float dz = globals.player.RootUpTarget.z - globals.player.RootUp.z;
        float mag = dx * dx + dy * dy + dz * dz;
        float lerpspeed;
        if (globals.player.HangElapsed <= 0.25f)
        {
            lerpspeed = 0.07f;
        }
        else if (globals.player.HangElapsed >= 0.5f)
        {
            lerpspeed = 0.5f;
        }
        else
        {
            lerpspeed = 1.72f * (globals.player.HangElapsed - 0.25f);
        }
        if (mag < lerpspeed * lerpspeed)
        {
            globals.player.RootUp = globals.player.RootUpTarget;
        }
        else
        {
            float s = (1.0f / xsqrt(mag)) * lerpspeed;
            globals.player.RootUp.x += dx * s;
            globals.player.RootUp.y += dy * s;
            globals.player.RootUp.z += dz * s;
            xVec3Normalize(&globals.player.RootUp, &globals.player.RootUp);
        }
        if (PlayerFpClassify(globals.player.RootUp.y) == 1 || PlayerFpClassify(globals.player.RootUpTarget.y) == 1)
        {
            globals.player.RootUp.x = 0.0f;
            globals.player.RootUp.y = 1.0f;
            globals.player.RootUp.z = 0.0f;
            globals.player.RootUpTarget.x = 0.0f;
            globals.player.RootUpTarget.y = 1.0f;
            globals.player.RootUpTarget.z = 0.0f;
            globals.player.HangElapsed = 0.0f;
        }
    }

    zEntPlayerVelUpdate(ent, dt);
    xEntEndUpdate(ent, sc, dt);

    if (globals.player.HangEnt != NULL && globals.player.HangEnt != globals.player.HangEntLast)
    {
        vHangLast = *(_xVec3*)&((_xMat4x3*)globals.player.ent.model->Mat)->pos;
        ent->frame->vel.x = 0.0f;
        ent->frame->vel.y = 0.0f;
        ent->frame->vel.z = 0.0f;
    }
    if (globals.player.HangEntLast != globals.player.HangEnt)
    {
        if (globals.player.HangEnt != NULL)
        {
            zEntEvent(globals.player.HangEnt, 0x20);
            if (globals.player.HangEnt->follow != NULL)
            {
                zEntEvent(globals.player.HangEnt->follow, 0x20);
            }
        }
        if (globals.player.HangEntLast != NULL)
        {
            zEntEvent(globals.player.HangEntLast, 0x21);
            if (globals.player.HangEntLast->follow != NULL)
            {
                zEntEvent(globals.player.HangEntLast->follow, 0x21);
            }
        }
    }
    globals.player.HangEntLast = globals.player.HangEnt;

    lastDeltaPos.x = ent->frame->mat.pos.x - ent->frame->oldpos.x;
    lastDeltaPos.y = ent->frame->mat.pos.y - ent->frame->oldpos.y;
    lastDeltaPos.z = ent->frame->mat.pos.z - ent->frame->oldpos.z;
    if (surfSlickRatio != 0.0f)
    {
        lastFloorNorm = globals.player.floor_norm;
        lastFloorEnt = (_xEnt*)ent->collis->colls[0].optr;
        if (lastFloorEnt != NULL && lastFloorEnt->id != xStrHash("slipdeck01") &&
            lastFloorEnt->id != xStrHash("slipdeck02") && lastFloorEnt->id != xStrHash("slipdeck03") &&
            lastFloorEnt->id != xStrHash("slipdeck04"))
        {
            lastFloorEnt = NULL;
        }
    }

    if (globals.player.DigTimer != 0.0f)
    {
        PlayerDigUpdate(dt);
    }
    else
    {
        globals.player.Diggable = NULL;
        for (i = 0; i < sc->num_stats; i++)
        {
            if (sc->stats[i] != NULL && (sc->stats[i]->baseFlags & 1) && xEntIsVisible(sc->stats[i]))
            {
                _zEntSimpleObj* simpent = (_zEntSimpleObj*)sc->stats[i];
                unsigned int sflags = simpent->sflags;
                if (sflags & 1)
                {
                    _xMat4x3* mat = (_xMat4x3*)simpent->model->Mat;
                    float dx = globals.player.ent.bound.sph.center.x - mat->pos.x;
                    float dy = globals.player.ent.bound.sph.center.y - mat->pos.y;
                    float dz = globals.player.ent.bound.sph.center.z - mat->pos.z;
                    float r = 1.25f * globals.player.ent.bound.sph.r;
                    if (dx * dx + dy * dy + dz * dz < r * r)
                    {
                        globals.player.Diggable = simpent;
                        if (!(sflags & 2))
                        {
                            float parm[4];
                            zEntPlayer_Notify(0);
                            parm[0] = 2.0f;
                            zEntEvent(simpent, 0xC3, parm);
                            simpent->sflags |= 2;
                        }
                    }
                    else if (sflags & 2)
                    {
                        simpent->sflags = sflags ^ 2;
                    }
                }
            }
        }
        if (globals.player.Diggable == NULL && globals.player.JumpState == 0 && globals.player.ControlOn &&
            (globals.pad0->pressed & 0x20000))
        {
            Shaggy8_CallToPlayer();
        }
    }

    if (globals.player.Shaggy1.Grab != NULL)
    {
        Shaggy1_PlayerUpdate(globals.player.Shaggy1.Grab, sc, dt);
    }
    if (globals.player.JumpState == 0)
    {
        xCameraTargetOnGround(1);
    }
    else
    {
        xCameraTargetOnGround(0);
    }
    zEntPlayer_FXUpdate(ent, dt);
    if (!globals.player.ControlOn && globals.player.ControlOnEvent)
    {
        globals.player.ControlOn = 1;
        globals.player.ControlOnEvent = 0;
    }
}

void zEntPlayer_Render(_zEnt* ent)
{
    if (globals.player.Visible)
    {
        xModelInstance* minst;
        float rootZadjust = 0.0f;
        _xMat4x3 rootOldMat = *(_xMat4x3*)ent->model->Mat;
        xAnimSingle* asingle = ent->model->Anim->Single;
        xAnimSingle* ablend = asingle->Blend;

        if (strcmp(asingle->State->Name, "SD0_SetDown") == 0 || strcmp(asingle->State->Name, "SD1_SetDown") == 0)
        {
            if (asingle->Time >= 0.85f)
            {
                rootZadjust = -0.6f;
            }
            else if (asingle->Time > 0.65f)
            {
                rootZadjust = (-0.6f * (asingle->Time - 0.65f)) / (0.85f - 0.65f);
            }
        }
        else if (ablend->State != NULL &&
                 (strcmp(ablend->State->Name, "SD0_SetDown") == 0 || strcmp(ablend->State->Name, "SD1_SetDown") == 0))
        {
            rootZadjust = -0.6f * (1.0f - asingle->BlendFactor * asingle->Tran->BlendRecip);
        }

        if (rootZadjust)
        {
            _xVec3 rootZvec = { 0.0f, 0.0f, 0.0f };
            rootZvec.z = rootZadjust;
            xMat3x3RMulVec(&rootZvec, (_xMat3x3*)ent->model->Mat, &rootZvec);
            ((_xMat4x3*)ent->model->Mat)->pos.x += rootZvec.x;
            ((_xMat4x3*)ent->model->Mat)->pos.z += rootZvec.z;
        }

        if (globals.player.RootUp.y != 1.0f)
        {
            _xMat4x3 tmpMat;
            _xMat4x3* mat = (_xMat4x3*)ent->model->Mat;
            tmpMat.up = globals.player.RootUp;
            xVec3Cross(&tmpMat.right, &globals.player.RootUp, &mat->at);
            xVec3Normalize(&tmpMat.right, &tmpMat.right);
            xVec3Cross(&tmpMat.at, &tmpMat.right, &tmpMat.up);
            tmpMat.pos.x = 0.0f;
            if (globals.player.HangEnt != NULL)
            {
                tmpMat.pos.y = -ent->bound.sph.r;
            }
            else
            {
                tmpMat.pos.y = 0.0f;
            }
            tmpMat.pos.z = 0.0f;
            xMat3x3RMulVec(&tmpMat.pos, (_xMat3x3*)&tmpMat, &tmpMat.pos);
            mat = (_xMat4x3*)ent->model->Mat;
            tmpMat.pos.x += mat->pos.x;
            if (globals.player.HangEnt != NULL)
            {
                tmpMat.pos.y += mat->pos.y + ent->bound.sph.r;
            }
            else
            {
                tmpMat.pos.y += mat->pos.y;
            }
            tmpMat.pos.z += ((_xMat4x3*)ent->model->Mat)->pos.z;
            if (globals.player.HangEnt != NULL && globals.player.HangStartLerp < 1.0f)
            {
                tmpMat.pos.z = globals.player.HangStartLerp * (tmpMat.pos.z - globals.player.HangStartPos.z) +
                               globals.player.HangStartPos.z;
                tmpMat.pos.x = globals.player.HangStartLerp * (tmpMat.pos.x - globals.player.HangStartPos.x) +
                               globals.player.HangStartPos.x;
                tmpMat.pos.y = globals.player.HangStartLerp * (tmpMat.pos.y - globals.player.HangStartPos.y) +
                               globals.player.HangStartPos.y;
            }
            tmpMat.flags = 0;
            *(_xMat4x3*)ent->model->Mat = tmpMat;
            rendermat = tmpMat;
        }
        else
        {
            rendermat = *(_xMat4x3*)ent->model->Mat;
        }

        if (globals.player.RootUp.y != 1.0f || rootZadjust)
        {
            for (minst = ent->model->Next; minst != NULL; minst = minst->Next)
            {
                *(_xMat4x3*)minst->Mat = *(_xMat4x3*)ent->model->Mat;
            }
        }

        unsigned int index = 0;
        asingle = ent->model->Anim->Single;
        for (minst = ent->model; minst != NULL; minst = minst->Next, index++)
        {
            switch (index)
            {
            case 0:
            case 1:
                break;
            case 2:
                if (((asingle->State->UserFlags & 0x1E) == 0xC && (globals.player.Inv_Specials & 0x800)) ||
                    strcmp(asingle->State->Name, "Powerup_helmet") == 0)
                {
                    globals.player.HelmetTimer = 0.3f;
                    PlayerMinstShow(minst);
                }
                else if (globals.player.HelmetTimer == 0.0f)
                {
                    minst->Flags &= ~3;
                }
                break;
            case 3:
                if ((asingle->State->UserFlags & 0x1E) == 2 && globals.player.WorldDisguise != 6)
                {
                    PlayerMinstShow(minst);
                }
                else
                {
                    minst->Flags &= ~3;
                }
                break;
            case 4:
                if (xStricmp(asingle->State->Name, "Action_dig") == 0)
                {
                    PlayerMinstShow(minst);
                }
                else
                {
                    minst->Flags &= ~3;
                }
                break;
            case 5:
                if (globals.player.WorldDisguise == 5 &&
                    (xStricmp(asingle->State->Name, "Idle_lampshade") == 0 ||
                     xStricmp(asingle->State->Name, "Idle_blackknight") == 0 ||
                     xStricmp(asingle->State->Name, "IdleToLampshade") == 0 ||
                     xStricmp(asingle->State->Name, "LampshadeToSneak") == 0 ||
                     xStricmp(asingle->State->Name, "SneakToLampshade") == 0 ||
                     xStricmp(asingle->State->Name, "Powerup_lamp") == 0))
                {
                    if (minst->BoneCount <= 10)
                    {
                        minst->Flags &= ~8;
                        minst->Flags |= 0x20;
                        minst->BoneIndex = 0;
                    }
                    if (minst->Anim != NULL)
                    {
                        minst->Flags |= 4;
                    }
                    PlayerMinstShow(minst);
                }
                else
                {
                    minst->Flags &= ~7;
                }
                break;
            case 6:
                if (globals.player.WorldDisguise == 6 &&
                    (xStricmp(asingle->State->Name, "Idle_lampshade") == 0 ||
                     xStricmp(asingle->State->Name, "Idle_blackknight") == 0 ||
                     xStricmp(asingle->State->Name, "Walk_blackknight") == 0))
                {
                    if (minst->Flags & 2)
                    {
                        ent->model->Flags &= ~1;
                        minst->Flags |= 1;
                    }
                    else
                    {
                        minst->Flags |= 2;
                    }
                }
                else
                {
                    ent->model->Flags |= 1;
                    minst->Flags &= ~7;
                }
                break;
            case 7:
                if (surfSticky && (globals.player.Inv_Specials & 4))
                {
                    PlayerMinstShow(minst);
                }
                else
                {
                    minst->Flags &= ~7;
                }
                break;
            case 8:
                if (strcmp(asingle->State->Name, "Powerup_plunger") == 0 ||
                    (globals.player.Slide == 2 && (strcmp(asingle->State->Name, "Land_normal") == 0 ||
                                                   strcmp(asingle->State->Name, "Land_walk") == 0 ||
                                                   strcmp(asingle->State->Name, "Land_walkidle") == 0 ||
                                                   strcmp(asingle->State->Name, "Land_running") == 0 ||
                                                   strcmp(asingle->State->Name, "Walk_plunger") == 0 ||
                                                   strcmp(asingle->State->Name, "Idle_plunger") == 0)))
                {
                    PlayerMinstShow(minst);
                }
                else
                {
                    minst->Flags &= ~3;
                }
                break;
            case 9:
                if (xStricmp(asingle->State->Name, "Fall_floatup") == 0 ||
                    xStricmp(asingle->State->Name, "Fall_floatdown") == 0 ||
                    xStricmp(asingle->State->Name, "Fall_floatopen") == 0)
                {
                    if (!(minst->Flags & 1))
                    {
                        if (minst->Anim != NULL)
                        {
                            minst->Anim->Single->Time = 0.0f;
                            minst->Anim->Single->CurrentSpeed = minst->Anim->Single->State->Speed;
                        }
                    }
                    minst->Flags |= 1;
                    memcpy(minst->Mat, ent->model->Mat, 0x30);
                    _xVec3 pos0;
                    _xVec3 pos1;
                    iModelTagEval(ent->model->Data, &globals.player.HangPawTag[0], ent->model->Mat, &pos0);
                    iModelTagEval(ent->model->Data, &globals.player.HangPawTag[1], ent->model->Mat, &pos1);
                    ((_xMat4x3*)minst->Mat)->pos.x = 0.5f * (pos1.x - pos0.x) + pos0.x;
                    ((_xMat4x3*)minst->Mat)->pos.y = 0.5f * (pos1.y - pos0.y) + pos0.y;
                    ((_xMat4x3*)minst->Mat)->pos.z = 0.5f * (pos1.z - pos0.z) + pos0.z;
                    xMat3x3RMulRotY((_xMat3x3*)minst->Mat, (_xMat3x3*)minst->Mat, umbrellaFudgeRot);
                    ((_xMat4x3*)minst->Mat)->pos.y += umbrellaFudgeY;
                    ((_xMat4x3*)minst->Mat)->pos.x += umbrellaFudgeX * ((_xMat4x3*)minst->Mat)->right.x;
                    ((_xMat4x3*)minst->Mat)->pos.z += umbrellaFudgeX * ((_xMat4x3*)minst->Mat)->right.z;
                }
                else
                {
                    minst->Flags &= ~1;
                }
                break;
            case 10:
                minst->Flags &= ~8;
                minst->Flags |= 0x20;
                minst->Flags &= ~1;
                break;
            case 11:
                if (strcmp(asingle->State->Name, "Inactive_Magnify") == 0 && asingle->Time > 0.55f &&
                    asingle->Time < 7.3666673f)
                {
                    PlayerMinstShow(minst);
                }
                else
                {
                    minst->Flags &= ~3;
                }
                break;
            }
        }

        signed int player_trans = 0x50;
        if (globals.player.s.TakeDamage && gPlayerHitWater)
        {
            globals.player.Transparent = 1;
            player_trans = 255.0f - 0.5f * (255.0f * sPlayerWaterOffset);
            if (player_trans < 0)
            {
                player_trans = 0;
            }
            sPlayerWaterOffset += 2.0f * sTimeElapsed;
            for (minst = globals.player.ent.model; minst != NULL; minst = minst->Next)
            {
                if (minst->Flags & 1)
                {
                    ((_xMat4x3*)minst->Mat)->pos.x = gPlayerWaterPos.x;
                    globals.player.ent.frame->mat.pos.x = gPlayerWaterPos.x;
                    float y = gPlayerWaterPos.y - sPlayerWaterOffset;
                    ((_xMat4x3*)minst->Mat)->pos.y = y;
                    globals.player.ent.frame->mat.pos.y = y;
                    ((_xMat4x3*)minst->Mat)->pos.z = gPlayerWaterPos.z;
                    globals.player.ent.frame->mat.pos.z = gPlayerWaterPos.z;
                }
            }
        }

        if (globals.player.Transparent > 0)
        {
            if (!iModelCull(ent->model->Data, ent->model->Mat))
            {
                zLightAddLocal(ent);
                xShadowRender(ent);
                fn_8000A080(-1);
                xModelRender(ent->model);
                fn_8000A080(0);
                for (minst = globals.player.ent.model; minst != NULL; minst = minst->Next)
                {
                    if (minst->Flags & 1)
                    {
                        iModelSetMaterialAlpha(minst->Data, player_trans);
                        iModelRender(minst->Data, minst->Mat);
                        iModelSetMaterialAlpha(minst->Data, 0xFF);
                    }
                }
                zLightRemoveLocal();
            }
        }
        else
        {
            xEntRender(&globals.player.ent);
        }
        *(_xMat4x3*)ent->model->Mat = rootOldMat;
    }
}

void zEntPlayer_Move(_xEnt* ent, _xScene*, float dt, _xEntFrame* frame)
{
    frame->mode = 0;
    frame->mode &= ~0x400;
    frame->mode &= ~0x800;
    if (globals.player.Health)
    {
        unsigned int on = globals.pad0->on;
        signed int x = -globals.pad0->analog1.x;
        signed int z = -globals.pad0->analog1.y;
        if (on & 0x80)
        {
            x = globals.player.s.AnalogMax;
        }
        else if (on & 0x20)
        {
            x = -globals.player.s.AnalogMax;
        }
        if (on & 0x10)
        {
            z = globals.player.s.AnalogMax;
        }
        else if (on & 0x40)
        {
            z = -globals.player.s.AnalogMax;
        }
        PlayerAbsControl(ent, x, z, dt);
        if (globals.player.ControlOn)
        {
            xCameraSetController(&globals.camera, globals.pad0->analog1.x, globals.pad0->analog1.y, dt);
        }
    }
}

signed int zEntPlayer_Damage(xBase* src, unsigned int damage, _xVec3* knockback)
{
    if (zEntPlayer_Damage(src, damage) == 0)
    {
        return 0;
    }
    if (knockback != NULL)
    {
        globals.player.ent.frame->vel = *knockback;
    }
    return 1;
}

signed int zEntPlayer_Damage(xBase* src, unsigned int damage)
{
    if (globals.player.Shaggy1.Grab != NULL &&
        (strcmp(globals.player.Shaggy1.Grab->model->Anim->Single->State->Name, "CatchJump") == 0 ||
         strcmp(globals.player.Shaggy1.Grab->model->Anim->Single->State->Name, "CatchThrow") == 0 ||
         strcmp(globals.player.Shaggy1.Grab->model->Anim->Single->State->Name, "SetDown") == 0 ||
         strcmp(globals.player.Shaggy1.Grab->model->Anim->Single->State->Name, "SD1_SetDown") == 0))
    {
        return 0;
    }
    if (globals.player.DamageTimer > 0.0f || globals.player.ControlOffTimer > 0.0f)
    {
        return 0;
    }

    globals.player.ScareSource = src;
    if (damage > globals.player.Health)
    {
        damage = globals.player.Health;
    }
    if (globals.player.s.TakeDamage)
    {
        globals.player.Health -= damage;
    }
    zEntVillain_HeroWasHurt();
    globals.player.ScareTimer = 0.35f;
    if (globals.player.Health)
    {
        globals.player.Face_ScareTimer = 3.0f;
        globals.player.Face_ScareRandom = (irand() >> 16) % 6;
    }
    globals.player.ScareShaggy = 2;

    if (globals.player.Health == 0)
    {
        globals.player.DamageTimer = 10.0f;
        if (globals.player.ent.driver != NULL)
        {
            globals.player.ent.driver = NULL;
            globals.player.ent.frame->vel = g_O3;
            globals.player.ent.frame->oldvel = g_O3;
        }
        zEntEventAll(NULL, 0, 0x33, NULL);
        if (globals.player.s.TakeDamage)
        {
            gDeadScooby = 1;
            gDeadScoobyPos = *(_xVec3*)&((_xMat4x3*)globals.player.ent.model->Mat)->pos;
        }
    }
    else
    {
        globals.player.DamageTimer = 2.0f;
    }

    if (globals.player.HangEnt != NULL)
    {
        globals.player.HangEnt->grabTimer = globals.player.HangEnt->hangInfo->grabDelay;
        globals.player.HangEnt = NULL;
        globals.player.RootUpTarget.x = 0.0f;
        globals.player.RootUpTarget.y = 1.0f;
        globals.player.RootUpTarget.z = 0.0f;
        globals.player.HangElapsed = 0.0f;
    }
    zSceneGameStatsTally_scare();
    return 1;
}

unsigned int zEntPlayer_MoveInfo()
{
    unsigned int retval = 0;
    xAnimState* state = globals.player.ent.model->Anim->Single->State;
    unsigned int flags = state->UserFlags;
    unsigned int type = flags & 0x1E;
    if (type == 0 || (flags & 1))
    {
        retval = 1;
    }
    if (type == 4)
    {
        retval |= 2;
    }
    if (type == 6 || type == 8)
    {
        retval |= 4;
    }
    if (type == 0xE)
    {
        retval |= 8;
    }
    if (type == 0xC)
    {
        retval |= 0x10;
    }
    if (strcmp(state->Name, "Idle_lampshade") == 0)
    {
        retval |= 0x800;
    }
    else if (strcmp(globals.player.ent.model->Anim->Single->State->Name, "Walk_sneak") == 0)
    {
        retval |= 0x400;
    }
    else if (strcmp(globals.player.ent.model->Anim->Single->State->Name, "Idle_blackknight") == 0)
    {
        retval |= 0x1000;
    }
    else if (strcmp(globals.player.ent.model->Anim->Single->State->Name, "Walk_blackknight") == 0)
    {
        retval |= 0x1000;
    }
    if (type == 2)
    {
        retval |= 0x100;
    }
    if (retval & 0x1800)
    {
        retval |= 0x200;
    }
    return retval;
}

void zEntPlayer_GiveScoobySnack(unsigned int quantity)
{
    globals.player.Inv_ScoobySnacks += quantity;
    zSceneGameStatsTally_snack(quantity);
    if (zEntPickup_lastSnackInArea())
    {
        zHudShowAllSnacksArea();
    }
}

signed int zEntPlayer_HasScoobySnack(unsigned int quantity)
{
    return quantity <= globals.player.Inv_ScoobySnacks;
}

void zEntPlayer_TakeScoobySnack(_xEnt* ent, unsigned int quantity)
{
    sDecrementSnackGateEnt = (_zEntPickup*)ent;
    sDecrementDelay = 1.1f;
    sDecrementAmount = 2.0f;
    sSnackDecrement += quantity;
}

void zEntPlayer_GiveClue(unsigned int quantity)
{
    globals.player.Inv_Clue += quantity;
}

void zEntPlayer_GiveKey(unsigned int quantity)
{
    globals.player.Inv_Key += quantity;
    if (globals.player.Inv_Key > 99)
    {
        globals.player.Inv_Key = 99;
    }
}

void zEntPlayer_GiveChance(unsigned int quantity)
{
    globals.player.Inv_Chances += quantity;
    if (globals.player.Inv_Chances > 99)
    {
        globals.player.Inv_Chances = 99;
    }
}

void zEntPlayer_GiveHealth(unsigned int quantity)
{
    globals.player.Health += quantity;
    if (globals.player.Health > 5)
    {
        globals.player.Health = 5;
    }
}

void zEntPlayer_GiveHotsauce(unsigned int value)
{
    if (value == 0)
    {
        globals.player.HotsauceTimer = 10.0f;
    }
    else
    {
        globals.player.HotsauceTimer = value;
    }
}

static float CalcJumpImpulse_Smooth(float g, float j, float h, float Tgc, float Tgs)
{
    float Tm[3];
    float A = (j - g) / (6.0f * Tgs);
    float Tf = Tgc + Tgs;
    float B = ((g * Tgc - j * Tgc) - j * Tgs) / (2.0f * Tgs);
    float AS = -j * 0.5f;
    float Kc = -(2.0f * B * Tgc + (3.0f * A * (Tgc * Tgc) + j * Tgc));
    float AF = -g * 0.5f;
    float D = -(Kc * Tgc + (B * (Tgc * Tgc) - (AS * (Tgc * Tgc) - A * (Tgc * (Tgc * Tgc)))));
    float Kbf = g * Tf + (2.0f * B * Tf + 3.0f * A * (Tf * Tf));
    float Ts = xsqrt((0.0f - h) / AS);
    float TF = xsqrt((-(Kbf * Tf + (AF * (Tf * Tf) - (D + (A * (Tf * (Tf * Tf)) + B * (Tf * Tf))))) - h) / AF);
    unsigned int solcnt = xMathSolveCubic(-2.0f * A, -B, 0.0f, D - h, &Tm[0], &Tm[1], &Tm[2]);

    if (Ts <= Tgc && Ts >= 0.0f)
    {
        return -(2.0f * AS * Ts + 0.0f);
    }
    if (TF >= Tf)
    {
        return -(2.0f * AF * TF + (Kbf + Kc));
    }

    float Tmfound = -1.0f;
    for (unsigned int i = 0; i < solcnt; i++)
    {
        if (Tm[i] >= Tgc && Tm[i] <= Tf && (Tmfound < 0.0f || Tm[i] < Tmfound))
        {
            Tmfound = Tm[i];
        }
    }
    if (Tmfound != -1.0f)
    {
        return -(3.0f * A * (Tmfound * Tmfound) + (2.0f * B * Tmfound + Kc));
    }
    return 1.0f;
}

void CalcJumpImpulse(zJumpParam* param)
{
    param->ImpulseVel = CalcJumpImpulse_Smooth(globals.player.s.Gravity, globals.player.s.JumpGravity,
                                               param->PeakHeight, param->TimeGravChange, globals.player.s.GravSmooth);
}

#pragma auto_inline on
void zEntPlayerJumpStart(_xEnt* ent, zJumpParam* jump)
{
    globals.player.Jump_CurrGravity = globals.player.s.JumpGravity;
    globals.player.Jump_HoldTimer = jump->TimeHold;
    globals.player.Jump_ChangeTimer = jump->TimeGravChange;
    if (ent->frame->vel.y > 0.0f && globals.player.JumpState == 2)
    {
        ent->frame->vel.y = jump->ImpulseVel;
    }
    else
    {
        ent->frame->vel.y = jump->ImpulseVel;
    }
    globals.player.JumpState = 2;
    unsigned int flags = ent->model->Anim->Single->State->UserFlags;
    if ((flags & 0x1E) == 0xE || (flags & 0x100))
    {
        globals.player.Bounced = 1;
    }
}
#pragma auto_inline reset

void zEntPlayerJumpUpdate(_xEnt* ent, float dt)
{
    globals.player.LastJumpState = globals.player.JumpState;
    if (globals.player.JumpState == 4)
    {
        return;
    }
    if (globals.player.JumpState == 5)
    {
        ent->frame->vel.y = -globals.player.s.ButtsmashSpeed;
    }

    if (globals.player.Jump_HoldTimer)
    {
        globals.player.Jump_HoldTimer -= dt;
        if (globals.player.Jump_HoldTimer <= 0.0f)
        {
            globals.player.Jump_HoldTimer = 0.0f;
        }
        else if (!globals.player.ControlOn || !(globals.pad0->on & 0x10000))
        {
            globals.player.Jump_HoldTimer = 0.0f;
            globals.player.Jump_ChangeTimer = 0.0f;
            globals.player.Jump_CurrGravity = globals.player.s.Gravity;
        }
    }

    if (globals.player.Jump_ChangeTimer)
    {
        globals.player.Jump_ChangeTimer -= dt;
        if (globals.player.Jump_ChangeTimer == 0.0f)
        {
            globals.player.Jump_ChangeTimer = 1e-7f;
        }
        if (globals.player.Jump_ChangeTimer <= -globals.player.s.GravSmooth)
        {
            globals.player.Jump_HoldTimer = 0.0f;
            globals.player.Jump_ChangeTimer = 0.0f;
            globals.player.Jump_CurrGravity = globals.player.s.Gravity;
        }
        else if (globals.player.Jump_ChangeTimer < 0.0f)
        {
            float lerp = -globals.player.Jump_ChangeTimer / globals.player.s.GravSmooth;
            globals.player.Jump_CurrGravity =
                (1.0f - lerp) * globals.player.s.JumpGravity + lerp * globals.player.s.Gravity;
        }
    }

    _xCollis* cfloor = &ent->collis->colls[0];
    if ((cfloor->flags & 1) && (cfloor->optr == NULL || ((xBase*)cfloor->optr)->baseType != 2) &&
        ent->frame->vel.y <= 0.0f)
    {
        if (globals.player.JumpState != 0)
        {
            if (globals.player.JumpState == 5)
            {
                if (sEmitterBounceMiss != NULL)
                {
                    _zParEmitterCustomSettings info;
                    info.custom_flags = 0x20;
                    _xMat4x3* mat = (_xMat4x3*)ent->model->Mat;
                    info.pos.x = cfloor->dist * cfloor->tohit.x + mat->pos.x;
                    info.pos.y = cfloor->dist * cfloor->tohit.y + mat->pos.y;
                    info.pos.z = cfloor->dist * cfloor->tohit.z + mat->pos.z;
                    for (signed int i = 0; i < 25; i++)
                    {
                        zParEmitterEmitCustom(sEmitterBounceMiss, 1.0f, &info);
                    }
                }
                zRumbleStart(0, (_tagSDRumbleType)3);
                float pitch;
                sPlayerSndfx.butt_pitch = zPitchChange(sPlayerSndfx.butt_pitch, &pitch, 0x1E, 1.5f);
                xSndPlay(sPlayerSndfx.buttmiss_id, 0.5f, 0.5f, 0.0f, 0x80, 2, 0);
            }
            globals.player.JumpState = 0;
            globals.player.Jump_CanDouble = 1;
            globals.player.Jump_CanFloat = 1;
            globals.player.Jump_Springboard = NULL;
            globals.player.Jump_SpringboardStart = 0;
        }
    }
    else
    {
        switch (globals.player.JumpState)
        {
        case 0:
            globals.player.JumpState = 1;
            globals.player.JumpTimer = 0.0f;
            break;
        case 1:
            globals.player.JumpTimer += dt;
            if (globals.player.JumpTimer >= 0.25f)
            {
                globals.player.JumpState = 2;
            }
            break;
        case 2:
            if (ent->frame->vel.y < 16.0f && globals.player.Jump_SpringboardStart)
            {
                if (sEmitterSpringBoard != NULL)
                {
                    _zParEmitterCustomSettings info;
                    info.custom_flags = 0x20;
                    info.pos = *xBoundCenter(&globals.player.Jump_Springboard->bound);
                    for (signed int i = 0; i < 25; i++)
                    {
                        zParEmitterEmitCustom(sEmitterSpringBoard, 1.0f / 30.0f, &info);
                    }
                }
                globals.player.Jump_SpringboardStart = 0;
                p2PlatformAsset* passet = globals.player.Jump_Springboard->passet;
                if (passet->sb.jmpdir.x == 0.0f && passet->sb.jmpdir.z == 0.0f)
                {
                    globals.player.Jump_CanDouble = 1;
                }
            }
            break;
        }
        if (globals.player.JumpState == 3)
        {
            ent->frame->vel.x *= 0.96f;
            ent->frame->vel.z *= 0.96f;
        }
    }

    if (globals.player.Jump_CurrGravity != 0.0f)
    {
        if (globals.player.JumpState == 2 || globals.player.JumpState == 1)
        {
            ent->frame->vel.y -= globals.player.Jump_CurrGravity * dt;
        }
        if (globals.player.JumpState == 2 || globals.player.JumpState == 3)
        {
            if ((globals.player.Inv_Specials & 0x1000) && globals.player.ControlOn &&
                (globals.pad0->on & 0x10000) && globals.player.Jump_CanFloat && !globals.player.Jump_CanDouble &&
                (ent->frame->vel.y <= -globals.player.s.FloatSpeed || gust_data.gust_on))
            {
                zLaughNotify(4);
                globals.player.JumpState = 3;
            }
            if (globals.player.JumpState == 3)
            {
                ent->frame->vel.y = -globals.player.s.FloatSpeed;
                if (!globals.player.ControlOn || !(globals.pad0->on & 0x10000))
                {
                    globals.player.JumpState = 2;
                }
            }
        }
    }
}

static void PlayerEGenCheck(_xEnt* ent, zScene* sc, float dt)
{
    signed int i;
    globals.player.earc_coll.flags &= ~1;
    for (i = 0; i < (signed int)sc->baseCount[0x29]; i++)
    {
        zEGenerator* egen = (zEGenerator*)sc->baseList[0x29] + i;
        if (egen->flags & 1)
        {
            _xIsect isect;
            xLine3VecDist2(&egen->src_pos, &egen->dst_pos, &ent->bound.sph.center, &isect);
            float r = ent->bound.sph.r;
            if (isect.dist < r * r + 0.1f * (2.0f * r) + 0.1f * 0.1f)
            {
                globals.player.earc_coll.flags |= 1;
                if (zEntPlayer_Damage(egen, 1) != 0)
                {
                    globals.player.DamageTimer = 5.0f;
                }
            }
        }
    }
}

void zEntPlayerVelUpdate(_xEnt* ent, float dt)
{
    if (!(ent->model->Anim->Single->State->UserFlags & 0x100))
    {
        _xVec3* v = &ent->frame->vel;
        if (!(globals.player.SlideTimer > 0.0f) && surfSlickRatio == 0.0f && !(globals.player.SlipFadeTimer > 0.0f))
        {
            if (globals.player.JumpState == 0)
            {
                v->x = 0.0f;
                v->y = 0.0f;
                v->z = 0.0f;
            }
            return;
        }

        if (globals.player.Slide == 0 && globals.player.SlideTimer > 0.0f)
        {
            float sft = 2.15f - globals.player.SlideTimer;
            if (globals.player.JumpState == 0 || globals.player.JumpState == 1)
            {
                if (globals.player.JumpState == 0)
                {
                    v->y = 0.0f;
                }
                if (sft < 0.4f)
                {
                    float gft = 0.4f - sft;
                    float s = gft / (gft + dt);
                    v->x *= s;
                    v->z *= s;
                }
                else
                {
                    v->x = 0.0f;
                    v->z = 0.0f;
                }
            }
            else if (sft < 2.15f)
            {
                float aft = 2.15f - sft;
                float s = aft / (aft + dt);
                v->x *= s;
                v->z *= s;
            }
            else
            {
                v->x = 0.0f;
                v->z = 0.0f;
            }
        }

        float velen2 = v->x * v->x + v->y * v->y + v->z * v->z;
        if ((float)__fabs(velen2) < 0.0001f)
        {
            v->x = 0.0f;
            v->y = 0.0f;
            v->z = 0.0f;
        }

        signed int i = 0;
        _xCollis* coll = &ent->collis->colls[0];
        if (surfSlickRatio != 0.0f)
        {
            if (coll->optr != NULL)
            {
                if (ent->frame->mat.pos.y - ent->frame->oldpos.y > 0.0f)
                {
                    v->x *= 0.97f;
                    v->z *= 0.97f;
                }
            }
            else
            {
                v->y = 0.0f;
            }
            coll += 2;
            i = 2;
        }
        else if (!(globals.player.SlideTimer > 0.0f))
        {
            if (globals.player.SlipFadeTimer > 0.0f)
            {
                float rft = globals.player.SlipFadeTimer;
                float s = rft / (rft + dt);
                v->x *= s;
                v->z *= s;
            }
            else
            {
                v->x = 0.0f;
                v->z = 0.0f;
            }
        }

        for (; i < 6; i++, coll++)
        {
            if ((coll->flags & 1) && coll->dist < 0.7f &&
                (coll->optr == NULL || ((xBase*)coll->optr)->baseType != 2))
            {
                if (!(v->x * coll->hdng.x + v->y * coll->hdng.y + v->z * coll->hdng.z < 0.0f))
                {
                    float d = v->x * coll->norm.x + v->y * coll->norm.y + v->z * coll->norm.z;
                    if (!(d > 0.0f))
                    {
                        d = -d;
                        if (d >= 0.965926f * velen2)
                        {
                            v->x = 0.0f;
                            v->y = 0.0f;
                            v->z = 0.0f;
                        }
                        else
                        {
                            float s = 0.5f * d;
                            v->x += coll->norm.x * s;
                            v->y += coll->norm.y * s;
                            v->z += coll->norm.z * s;
                            velen2 = v->x * v->x + v->y * v->y + v->z * v->z;
                        }
                    }
                }
            }
        }
    }
}

void zEntPlayerFloorUpdate(_xEnt* p, float dt)
{
    static _xVec3 lorigins[6] = {
        { -0.3f, 0.0f, 0.3f },  { 0.3f, 0.0f, 0.3f }, { 0.3f, 0.0f, -0.3f },
        { -0.3f, 0.0f, -0.3f }, { 0.0f, 0.0f, 0.3f }, { 0.0f, 0.0f, -0.3f },
    };
    _xCollis* fcoll = &p->collis->colls[0];
    xSurface* fsurf = zSurfaceGetSurface(fcoll);
    globals.player.floor_surf = fsurf;

    surfSlickTimer -= dt;
    if (surfSlickTimer < 0.0f)
    {
        surfSlickTimer = 0.0f;
    }
    surfSticky = 0;
    surfSlickRatio = 0.0f;

    if (globals.player.CanSlip)
    {
        if ((globals.player.JumpState == 0 || globals.player.JumpState == 1) && fsurf != NULL && fsurf->state == 0)
        {
            surfSticky = zSurfaceGetSticky(fsurf);
        }
        if (fsurf != NULL && fsurf->state == 0)
        {
            surfFriction = zSurfaceGetFriction(fsurf);
            surfSlickness = zSurfaceGetSlickness(fsurf);
            surfDamping = zSurfaceGetDamping(fsurf, minVelmag);
            surfSlipTimer = 0.00001f;
        }
        else
        {
            surfFriction = 1.0f;
            surfDamping = minVelmag;
            if (globals.player.JumpState != 0)
            {
                surfSlipTimer = 0.0f;
            }
            if (surfSlipTimer)
            {
                surfSlipTimer += dt;
                if (surfSlipTimer >= 0.75f)
                {
                    surfSlipTimer = 0.0f;
                }
            }
            if (surfSlipTimer == 0.0f)
            {
                surfSlickness = 1;
            }
        }

        if (surfSlickness != 1)
        {
            surfSlickRatio = (surfSlickness > 1) ? 150.0f / surfSlickness : 1.0f;
            surfSlickTimer = 0.1f;
        }
        else
        {
            surfSlickRatio = 0.0f;
        }

        _xVec3* vel = &p->frame->vel;
        if (lastSlickness != surfSlickness)
        {
            if (surfSlickness == 1)
            {
                if (lastFloorEnt != NULL)
                {
                    globals.player.SlipFadeTimer = 0.8f;
                }
                else
                {
                    vel->x = 0.0f;
                    vel->z = 0.0f;
                }
            }
            else
            {
                float s = 1.0f / dt;
                vel->x = lastDeltaPos.x * s;
                vel->z = lastDeltaPos.z * s;
            }
        }

        if (surfSlickRatio)
        {
            float velmag = vel->x * vel->x + vel->z * vel->z;
            if (surfMaxSpeed)
            {
                if (velmag > surfMaxSpeed * surfMaxSpeed)
                {
                    float s = surfMaxSpeed / xsqrt(velmag);
                    vel->x *= s;
                    vel->z *= s;
                }
            }
            else
            {
                float decelmag;
                if (xStricmp(p->model->Anim->Single->State->Name, "Skid") != 0)
                {
                    decelmag = surfDecelIdle * dt;
                }
                else
                {
                    decelmag = surfDecelSkid * dt;
                }
                float slickLerp = 1.3333334f * surfSlipTimer;
                decelmag = (1.0f - slickLerp) * decelmag + 20.0f * slickLerp * decelmag;
                velmag = xsqrt(velmag);
                if (velmag <= decelmag)
                {
                    vel->x = 0.0f;
                    vel->z = 0.0f;
                }
                else
                {
                    float s = (velmag - decelmag) / velmag;
                    vel->x *= s;
                    vel->z *= s;
                }
            }
        }
        else if (globals.player.SlipFadeTimer > 0.0f)
        {
            globals.player.SlipFadeTimer -= dt;
            if (globals.player.SlipFadeTimer < 0.0f)
            {
                globals.player.SlipFadeTimer = 0.0f;
            }
        }
        lastSlickness = surfSlickness;
    }

    _xRay3 ray;
    _xCollis rcoll;
    if (p->collis->colls[0].flags & 1)
    {
        globals.player.floor_norm = fcoll->norm;
    }
    else
    {
        globals.player.floor_norm = g_Y3;
        ray.origin = *xBoundCenter(&p->bound);
        ray.dir.x = 0.0f;
        ray.dir.y = -1.0f;
        ray.dir.z = 0.0f;
        ray.max_t = 10.0f;
        ray.flags = 0x800;
        rcoll.flags = 0;
        rcoll.optr = NULL;
        rcoll.dist = 1e19f;
        xRayHitsScene(globals.sceneCur, &ray, &rcoll);
        fcoll->dist = rcoll.dist;
    }

    float dave = 0.0f;
    ray.dir.x = 0.0f;
    ray.dir.y = -1.0f;
    ray.dir.z = 0.0f;
    ray.min_t = -0.7f;
    ray.max_t = 1.0f;
    ray.flags = 0xC00;
    _xEnt* fent = (_xEnt*)fcoll->optr;
    signed int i;
    for (i = 0; i < 4; i++)
    {
        xMat3x3RMulVec(&ray.origin, (_xMat3x3*)p->frame, &lorigins[i]);
        ray.origin.x += p->frame->mat.pos.x;
        ray.origin.y += p->frame->mat.pos.y;
        ray.origin.z += p->frame->mat.pos.z;
        rcoll.flags = 0;
        rcoll.optr = NULL;
        rcoll.dist = 1e19f;
        if (fent != NULL)
        {
            if (fent->collLev == 5)
            {
                iRayHitsModel(&ray, fent->model, &rcoll);
            }
            else
            {
                xRayHitsBound(&ray, &fent->bound, &rcoll);
            }
        }
        else
        {
            iRayHitsEnv(&ray, globals.sceneCur->env, &rcoll);
        }
        floor_supp[i] = ray.origin;
        floor_dist[i] = rcoll.dist;
        if (rcoll.flags & 1)
        {
            dave += rcoll.dist;
        }
    }
    dave *= 0.25f;
    float dmin = dave - 0.17320508f;
    float dmax = dave + 0.17320508f;
    for (i = 0; i < 4; i++)
    {
        float d = floor_dist[i];
        d = (dmin > ((d < dmax) ? d : dmax)) ? dmin : ((d < dmax) ? d : dmax);
        floor_supp[i].y -= d;
    }

    float ax = floor_supp[1].x - floor_supp[0].x;
    float bx = floor_supp[2].x - floor_supp[0].x;
    float cx = floor_supp[3].x - floor_supp[0].x;
    float ay = floor_supp[1].y - floor_supp[0].y;
    float by = floor_supp[2].y - floor_supp[0].y;
    float cy = floor_supp[3].y - floor_supp[0].y;
    float az = floor_supp[1].z - floor_supp[0].z;
    float bz = floor_supp[2].z - floor_supp[0].z;
    float cz = floor_supp[3].z - floor_supp[0].z;
    globals.player.floor_norm.x = (ay * bz - by * az) + (by * cz - cy * bz);
    globals.player.floor_norm.y = (az * bx - bz * ax) + (bz * cx - cz * bx);
    globals.player.floor_norm.z = (ax * by - bx * ay) + (bx * cy - cx * by);
    xVec3Normalize(&globals.player.floor_norm, &globals.player.floor_norm);

    if (globals.player.Slide == 1 || globals.player.Slide == 2)
    {
        if (((p->collis->colls[0].flags & 1) &&
             (fcoll->norm.y > (float)cos(surfSlideStop) ||
              (fsurf != NULL && (fsurf->state != 0 || !zSurfaceGetSlide(fsurf))))) ||
            p->frame->vel.y > 0.0f || surfSlickRatio != 0.0f)
        {
            if (!(globals.player.Inv_Specials & 8))
            {
                p->pflags &= 0x7F;
                globals.player.SlideTimer = 2.15f;
            }
            if (surfSlickRatio != 0.0f)
            {
                globals.player.SlideTimer = 0.0f;
            }
            globals.player.Slide = 0;
        }
    }
    else if ((p->collis->colls[0].flags & 1) && fcoll->norm.y < (float)cos(zSurfaceGetSlideStartAngle(fsurf)) &&
             fsurf != NULL && fsurf->state == 0 && zSurfaceGetSlide(fsurf))
    {
        if ((globals.player.Inv_Specials & 8) &&
            (fcoll->optr == NULL || ((xBase*)fcoll->optr)->baseType != 6 || ((_xEnt*)fcoll->optr)->subType != 0xB))
        {
            globals.player.Slide = 2;
        }
        else
        {
            _xVec3 X;
            _xVec3 D;
            _xMat3x3 N_to_Up;
            _xVec3 D_flat;
            X.x = -fcoll->norm.z;
            X.y = 0.0f;
            X.z = fcoll->norm.x;
            xVec3Normalize(&X, &X);
            xVec3Cross(&D, &fcoll->norm, &X);
            xVec3Normalize(&D, &D);
            xMat3x3RotC(&N_to_Up, X.x, X.y, X.z, acos(fcoll->norm.y));
            xMat3x3RMulVec(&D_flat, &N_to_Up, &D);
            float ddotm = D_flat.x * req_motion.x + D_flat.z * req_motion.z;
            if (ddotm < 0.0f && (globals.player.LastJumpState == 0 || globals.player.LastJumpState == 1))
            {
                globals.player.Slide = 3;
                float s = ddotm / xsqrt(D_flat.x * D_flat.x + D_flat.z * D_flat.z);
                p->frame->mat.pos.x -= D_flat.x * s;
                p->frame->mat.pos.z -= D_flat.z * s;
                if (update_motion.y > 0.0f)
                {
                    p->frame->mat.pos.y = p->frame->oldpos.y;
                }
            }
            else
            {
                globals.player.Slide = 1;
                surfSlideStart = zSurfaceGetSlideStartAngle(fsurf);
                surfSlideStop = zSurfaceGetSlideStopAngle(fsurf);
                p->pflags |= 0x80;
                float s = 0.5f / update_dt;
                p->frame->vel.x += req_motion.x * s;
                p->frame->vel.z += req_motion.z * s;
                p->frame->vel.y = -4.0f;
            }
            globals.player.SlideTimer = 0.0f;
        }
    }
    else
    {
        globals.player.Slide = 0;
    }

    switch (globals.player.Slide)
    {
    case 1:
        globals.player.SlideTimer += dt;
        break;
    case 0:
        if (globals.player.SlideTimer > 0.0f)
        {
            globals.player.SlideTimer -= dt;
            if (globals.player.SlideTimer < 0.0f)
            {
                globals.player.SlideTimer = 0.0f;
            }
        }
        break;
    }
}

zPlayerSituation sPlayerInfo[37] = {
    { 0, 0, 120.0f, 0.0f, 0.0f, 0, 0 },
    { 1, 0, 2.0f, 0.0f, 0.0f, 0, 1 },
    { 0, 0, 60.0f, 0.0f, 0.0f, 0, 2 },
    { 0, 0, 60.0f, 0.0f, 0.0f, 0, 3 },
    { 0, 0, 120.0f, 1.0f, 0.0f, 0, 4 },
    { 0, 0, 2.0f, 0.5f, 0.0f, 0, 5 },
    { 0, 0, 120.0f, 1.0f, 0.0f, 0, 6 },
    { 0, 0, 2.0f, 0.0f, 0.0f, 0, 7 },
    { 1, 0, 20.0f, 0.5f, 0.0f, 0, 8 },
    { 0, 0, 1.0f, 0.0f, 0.0f, 0, 9 },
    { 0, 0, 60.0f, 0.0f, 0.0f, 0, 10 },
    { 0, 0, 60.0f, 1.0f, 0.0f, 0, 11 },
    { 0, 0, 30.0f, 0.0f, 0.0f, 0, 12 },
    { 0, 0, 60.0f, 0.0f, 0.0f, 0, 13 },
    { 5, 0, 60.0f, 0.5f, 0.0f, 0, 14 },
    { 5, 0, 5.0f, 1.0f, 0.0f, 0, 15 },
    { 1, 0, 30.0f, 0.0f, 0.0f, 0, 16 },
    { 1, 0, 30.0f, 0.0f, 0.0f, 0, 17 },
    { 1, 0, 30.0f, 0.0f, 0.0f, 0, 18 },
    { 1, 0, 30.0f, 0.0f, 0.0f, 0, 19 },
    { 1, 0, 30.0f, 0.0f, 0.0f, 0, 20 },
    { 1, 0, 30.0f, 0.0f, 0.0f, 0, 21 },
    { 1, 0, 30.0f, 0.0f, 0.0f, 0, 22 },
    { 1, 0, 30.0f, 0.0f, 0.0f, 0, 23 },
    { 1, 0, 30.0f, 0.0f, 0.0f, 0, 24 },
    { 1, 0, 30.0f, 0.0f, 0.0f, 0, 25 },
    { 1, 0, 30.0f, 0.0f, 0.0f, 0, 26 },
    { 1, 0, 30.0f, 0.0f, 0.0f, 0, 27 },
    { 1, 0, 30.0f, 0.0f, 0.0f, 0, 28 },
    { 1, 0, 30.0f, 0.0f, 0.0f, 0, 29 },
    { 1, 0, 30.0f, 0.0f, 0.0f, 0, 30 },
    { 1, 0, 2.0f, 1.0f, 0.0f, 0, 31 },
    { 1, 0, 30.0f, 0.2f, 0.0f, 0, 32 },
    { 1, 0, 30.0f, 0.5f, 0.0f, 0, 33 },
    { 1, 0, 30.0f, 0.5f, 0.0f, 0, 34 },
    { 1, 0, 30.0f, 0.5f, 0.0f, 0, 35 },
    { 1, 0, 30.0f, 0.5f, 0.0f, 0, 36 },
};

static unsigned int PlayerDepenQuery(_xEnt*, _xEnt*, _xScene*, float, _xCollis* coll)
{
    if (globals.player.DamageTimer > 0.0f)
    {
        xSurface* surf = zSurfaceGetSurface(coll);
        if (surf != NULL && surf->state == 0)
        {
            zSurfaceProps* prop = (zSurfaceProps*)surf->moprops;
            if (prop != NULL && prop->asset != NULL)
            {
                switch (prop->asset->game_damage_type)
                {
                case 0:
                    return 1;
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                    if (prop->asset->game_damage_flags & 1)
                    {
                        return 0;
                    }
                    break;
                }
            }
        }
    }
    return 1;
}

static void PlayerBoundUpdate(_xEnt* ent, _xVec3* pos)
{
    _xVec3 dpos;

    xEntDefaultBoundUpdate(ent, pos);
    if ((headcoll_anim >= 4 && headcoll_anim <= 7) || headcoll_anim == 8)
    {
        dpos.x = pos->x - ((_xMat4x3*)ent->model->Mat)->pos.x;
        dpos.y = pos->y - ((_xMat4x3*)ent->model->Mat)->pos.y;
        dpos.z = pos->z - ((_xMat4x3*)ent->model->Mat)->pos.z;
        xModelInstance* head = globals.player.ent.model->Next;
        iModelTagEval(head->Data, &globals.player.head_tag, head->Mat, &globals.player.head_sph.center);
        globals.player.head_sph.center.x += dpos.x;
        globals.player.head_sph.center.y += dpos.y;
        globals.player.head_sph.center.z += dpos.z;
        _xVec3* c = xBoundCenter(&ent->bound);
        dpos.x = c->x - globals.player.head_sph.center.x;
        dpos.y = c->y - globals.player.head_sph.center.y;
        dpos.z = c->z - globals.player.head_sph.center.z;
        xVec3Normalize(&dpos, &dpos);
        dpos.x *= 0.4f;
        dpos.y *= 0.4f;
        dpos.z *= 0.4f;
        globals.player.head_sph.center.x += dpos.x;
        globals.player.head_sph.center.y += dpos.y;
        globals.player.head_sph.center.z += dpos.z;
        globals.player.head_sph.r = 0.7f;
        if (globals.player.head_sph.center.y < ent->bound.sph.center.y)
        {
            globals.player.head_sph.center.y = ent->bound.sph.center.y;
        }
    }
    else
    {
        xMat3x3RMulVec(&dpos, (_xMat3x3*)&rendermat, &headcoll_os);
        _xVec3* c = xBoundCenter(&ent->bound);
        globals.player.head_sph.center.x = c->x + dpos.x;
        globals.player.head_sph.center.y = c->y + dpos.y;
        globals.player.head_sph.r = 0.7f;
        globals.player.head_sph.center.z = c->z + dpos.z;
    }
}

void zEntPlayerSurfDamageUpdate(_xEnt* p)
{
    if (!(globals.player.DamageTimer > 0.0f))
    {
        _xCollis* coll = &p->collis->colls[0];
        _xCollis* cend = coll + p->collis->idx;
        for (; coll < cend; coll++)
        {
            xSurface* surf;
            zSurfaceProps* prop;
            if (!(coll->flags & 1))
            {
                continue;
            }
            surf = zSurfaceGetSurface(coll);
            if (surf == NULL || surf->state != 0)
            {
                continue;
            }
            signed int damaged = 0;
            prop = (zSurfaceProps*)surf->moprops;
            if (prop == NULL || prop->asset == NULL)
            {
                continue;
            }
            switch (prop->asset->game_damage_type)
            {
            case 4:
            case 6:
                if (zEntPlayer_Damage(surf, 1) != 0)
                {
                    globals.player.DamageTimer = 5.0f;
                }
                zRumbleStart(0, (_tagSDRumbleType)7);
                damaged = 1;
                break;
            case 1:
            {
                _xEnt* cent = (_xEnt*)coll->optr;
                if (cent != NULL && cent->baseType == 0x29 && !(((zEGenerator*)cent)->flags & 1))
                {
                    break;
                }
            }
            case 5:
                if (globals.player.Health)
                {
                    globals.player.DamageTimer = 0.0f;
                    zEntPlayer_Damage(surf, globals.player.Health);
                    damaged = 1;
                }
                break;
            }
            if (damaged)
            {
                if (coll->optr != NULL)
                {
                    globals.player.DamageSource = (_zEnt*)coll->optr;
                }
                return;
            }
        }
    }
}

void zEntPlayerDriveUpdate(_xEnt* p, float dt)
{
    _xEntDrive* drv = &globals.player.drv;
    _zPlatform* plat;

    if (globals.player.drv.odriver != NULL)
    {
        _zPlatform* oplat = (_zPlatform*)globals.player.drv.odriver;
        if (drv->os == 0.0f)
        {
            zEntEvent(globals.player.drv.odriver, 0x21);
            drv->odriver = NULL;
            drv->otm = 0.0f;
            if (oplat->subType == 9)
            {
                zPlatform_Dismount();
            }
            if (oplat->passet->flags & 1)
            {
                zPlatform_Shake(oplat, dt, 0.2f, 37.699112f);
            }
        }
    }

    _xCollis* coll = &p->collis->colls[0];
    _xEnt* cent;
    if (p->collis->colls[0].flags & 1)
    {
        cent = (_xEnt*)coll->optr;
    }
    else
    {
        cent = NULL;
    }

    if (cent != NULL && (cent->eventFunc == Shaggy2_EventCB || cent->eventFunc == Shaggy2Plat_EventCB))
    {
        globals.player.MountChimney = cent;
        globals.player.MountChimOldY = ((_xMat4x3*)cent->model->Mat)->pos.y;
    }
    else
    {
        globals.player.MountChimney = NULL;
    }

    if (cent == NULL)
    {
        if (drv->driver != NULL)
        {
            if (globals.player.JumpState != 0 && globals.player.JumpState != 1)
            {
                xEntDriveDismount(drv, 1.0f);
            }
            else
            {
                xEntDriveDismount(drv, 0.3f);
            }
        }
        return;
    }

    xSurface* surf = zSurfaceGetSurface(coll);
    if (surf != NULL && surf->state == 0 && zSurfaceGetDamageType(surf))
    {
        return;
    }

    if (cent != drv->driver)
    {
        if (drv->driver != NULL)
        {
            xEntDriveDismount(drv, 0.3f);
        }
        if (cent->baseType != 6)
        {
            return;
        }
        if (cent->eventFunc == Shaggy2_EventCB || cent->eventFunc == Shaggy2Plat_EventCB)
        {
            zEntEvent(cent, 0x20);
        }
        plat = (_zPlatform*)cent;
        if (!(plat->plat_flags & 1))
        {
            return;
        }
        if (!gPlayerHitWater && cent != drv->odriver)
        {
            zEntEvent(cent, 0x20);
        }
        if (plat->subType != 0xA)
        {
            float ampl;
            float freq;
            float dur;
            if (plat->subType == 0xB)
            {
                xEntDriveMount(drv, plat, 0.5f);
            }
            else
            {
                xEntDriveMount(drv, plat, 0.15f);
            }
            if (plat->subType == 9)
            {
                zPlatform_Tremble(plat, ampl, freq, dur);
            }
            if (plat->passet->flags & 1)
            {
                zPlatform_Shake(plat, dt, 0.2f, 37.699112f);
            }
        }
    }
    else
    {
        plat = (_zPlatform*)drv->driver;
    }

    if (plat->subType == 0xB || plat->subType == 9)
    {
        zEntPlayer_Notify(8);
    }

    if (plat->subType == 0xA && globals.player.Health)
    {
        if (!(plat->passet->flags & 2) || globals.player.JumpState == 0 || globals.player.JumpState == 1)
        {
            if (plat->tmr <= 0.0f)
            {
                plat->ctr = 0;
            }
            else if (plat->ctr < 2)
            {
                plat->ctr++;
            }

            float* jmphs = plat->passet->sb.jmph;
            float h = (((jmphs[0] > jmphs[1]) ? jmphs[0] : jmphs[1]) > jmphs[2])
                          ? ((jmphs[0] > jmphs[1]) ? jmphs[0] : jmphs[1])
                          : jmphs[2];
            float v = CalcJumpImpulse_Smooth(globals.player.s.Gravity, globals.player.s.JumpGravity, h, 0.3f,
                                             globals.player.s.GravSmooth);
            startBounce = p->frame->mat.pos.y;
            globals.player.Jump_CurrGravity = globals.player.s.JumpGravity;
            globals.player.Jump_HoldTimer = 0.0f;
            globals.player.Jump_ChangeTimer = 0.3f;
            if (p->frame->vel.y > 0.0f && globals.player.JumpState == 2)
            {
                p->frame->vel.y = v;
            }
            else
            {
                p->frame->vel.y = v;
            }
            globals.player.JumpState = 2;
            unsigned int flags = p->model->Anim->Single->State->UserFlags;
            if ((flags & 0x1E) == 0xE || (flags & 0x100))
            {
                globals.player.Bounced = 1;
            }
            _xVec3* jmpdir = &plat->passet->sb.jmpdir;
            p->frame->vel.x = jmpdir->x * p->frame->vel.y;
            p->frame->vel.y = jmpdir->y * p->frame->vel.y;
            p->frame->vel.z = jmpdir->z * p->frame->vel.y;
            globals.player.Jump_CanDouble = 0;
            globals.player.Jump_CanFloat = 1;
            globals.player.Jump_Springboard = plat;
            globals.player.Jump_SpringboardStart = 1;
            xAnimPlay* aplay = plat->model->Anim;
            if (aplay != NULL)
            {
                xAnimSingle* asing = aplay->Single;
                asing->Time = 0.0f;
                asing->CurrentSpeed = 1.0f;
            }
            globals.player.Bounced = 2;
            zEntPlayer_Notify(9);
        }
    }
}

void zEntPlayerExit(_xEnt* ent)
{
    xAnimTableRemoveEffectByCB(ent->model->Anim->Table, zEntPlayer_FXScratchCB);
    xAnimTableRemoveEffectByCB(ent->model->Anim->Table, zEntPlayer_SNDSkidCB);
    xAnimTableRemoveEffectByCB(ent->model->Anim->Table, zEntPlayer_FXDigupCB);
    xAnimTableRemoveEffectByCB(ent->model->Anim->Table, zEntPlayer_FXOpenBrollyCB);
    xAnimTableRemoveEffectByCB(ent->model->Anim->Table, zEntPlayer_FXSpringCB);
    zFeetExit(ent, ent->model->Anim->Table);
}

void zEntPlayerReset(_xEnt* ent)
{
    xModelInstance* minst;
    xAnimState* curr;
    unsigned int index;

    _iAnimSKBFixTransPop(xAnimTableGetState(ent->model->Anim->Table, "RunToIdle_normal")->Data->RawData[0],
                         xAnimTableGetState(ent->model->Anim->Table, "Idle_normal")->Data->RawData[0], 0);
    _iAnimSKBFixTransPop(xAnimTableGetState(ent->model->Anim->Table, "SkidToIdle")->Data->RawData[0],
                         xAnimTableGetState(ent->model->Anim->Table, "Idle_normal")->Data->RawData[0], 0);

    if (zGameModeGet() != 0xB || (zGameStateGet() != 5 && zGameStateGet() != 1 && zGameStateGet() != 4))
    {
        if (globals.player.Inv_Ammo[0] < globals.player.s.AmmoSceneGum)
        {
            globals.player.Inv_Ammo[0] = globals.player.s.AmmoSceneGum;
        }
        if (globals.player.Inv_Ammo[1] < globals.player.s.AmmoSceneSoap)
        {
            globals.player.Inv_Ammo[1] = globals.player.s.AmmoSceneSoap;
        }
        if (zGameModeGet() == 0xB && zGameStateGet() == 2)
        {
            gPlayerHitWater = 0;
            globals.player.Health = 3;
            sPlayerWaterOffset = 0.0f;
            gDeadScooby = 0;
        }
        else
        {
            globals.player.Health = 3;
            globals.player.Inv_Chances = 3;
            globals.player.Inv_ScoobySnacks = 0;
            globals.player.Inv_Specials = globals.player.s.Initial_Specials;
            globals.player.Inv_Mtokens[0] = 0;
            gPlayerHitWater = 0;
            sPlayerWaterOffset = 0.0f;
            gDeadScooby = 0;
            globals.player.Inv_Ammo[0] = globals.player.s.AmmoSceneGum;
            globals.player.Inv_Ammo[1] = globals.player.s.AmmoSceneSoap;
            globals.player.Inv_Key = 0;
            globals.player.Inv_Clue = 0;
        }
    }

    globals.player.ControlOnEvent = 0;
    globals.player.ControlOn = 1;
    globals.player.Speed = 0;
    globals.player.Sneak = 0;
    globals.player.Teeter = 0;
    globals.player.CanSlip = globals.sceneCur->baseCount[26];
    globals.player.Slide = 0;
    globals.player.SlideTimer = 0.0f;
    globals.player.Stepping = 0;
    globals.player.JumpState = 0;
    globals.player.LastJumpState = 0;
    globals.player.IdleMinorTimer = 0.0f;
    globals.player.IdleMajorTimer = 0.0f;
    globals.player.IdleSitTimer = 0.0f;
    globals.player.cheat_mode = 0;
    globals.player.FireTarget = NULL;
    globals.player.HeadbuttHit = 0;
    globals.player.LeanLerp = 1.0f;
    globals.player.ScareTimer = 0.0f;
    globals.player.DamageTimer = 0.0f;
    globals.player.ControlOffTimer = 0.0f;
    globals.player.HelmetTimer = 0.0f;
    globals.player.Bounced = 0;
    globals.player.FallDeathTimer = 0.0f;
    globals.player.Diggable = NULL;
    globals.player.Shaggy8Call = NULL;
    globals.player.Visible = 1;
    globals.player.AutoMoveSpeed = 0;
    globals.player.Shaggy1.SetDown = 0;
    globals.player.Shaggy1.GrabOn = 1;
    globals.player.Face_ScareTimer = 0.0f;
    globals.player.Face_EventTimer = 0.0f;
    globals.player.Face_PantTimer = 0.0f;
    globals.player.SpecialReceived = 0;
    globals.player.Jump_CanFloat = 1;
    globals.player.Jump_CanDouble = 1;
    globals.player.DamageSource = NULL;
    globals.player.Jump_Springboard = NULL;
    globals.player.Jump_SpringboardStart = 0;
    gPlayerHitWater = 0;
    sPlayerWaterOffset = 0.0f;
    gDeadScooby = 0;

    ent->collis->depenq = PlayerDepenQuery;
    ent->bupdate = PlayerBoundUpdate;
    surfSlickRatio = 0.0f;
    surfSlickTimer = 0.0f;
    globals.player.floor_surf = NULL;
    globals.player.floor_norm = g_Y3;
    ent->collis->chk = 0x2E;
    xEntReset(ent);
    ent->frame->rot.axis = g_Y3;
    ent->frame->rot.angle = ent->asset->ang.x;
    xEntDriveInit(&globals.player.drv, ent);
    globals.player.drv.flags = 1;
    xCameraSetTarget(&globals.camera, (_xVec3*)&((_xMat4x3*)globals.player.ent.model->Mat)->pos);
    xCameraSetTarget(&globals.camera, (_xMat4x3*)globals.player.ent.model->Mat);
    memcpy(&gPlayerAbsMat, &globals.camera.hi_cam.mat, sizeof(_xMat4x3));
    globals.player.Jump_CurrGravity = globals.player.s.Gravity;

    iModelTagSetup(&globals.player.BubbleTag, globals.player.ent.model->Next->Data, 0.0f, 1.334f, 0.475f);
    iModelTagSetup(&globals.player.HangPawTag[0], globals.player.ent.model->Data, 0.12f, 0.015f, 0.175f);
    iModelTagSetup(&globals.player.HangPawTag[1], globals.player.ent.model->Data, -0.12f, 0.015f, 0.195f);
    iModelTagSetup(&globals.player.HangPawTag[2], globals.player.ent.model->Data, 0.2f, 0.018f, -0.62f);
    iModelTagSetup(&globals.player.HangPawTag[3], globals.player.ent.model->Data, -0.2f, 0.018f, -0.62f);
    globals.player.HangPawOffset = 1.5f;
    iModelTagSetup(&globals.player.center_tag, globals.player.ent.model->Data, 0.0f, 0.465f, 0.099f);
    iModelTagSetup(&globals.player.head_tag, globals.player.ent.model->Next->Data, 0.0f, 1.251f, 0.293f);

    xModelInstance* tongue = globals.player.ent.model;
    unsigned int tongueIdx = 0;
    globals.player.TongueFlags[0] = 0;
    globals.player.TongueFlags[1] = 0;
    while (tongue != NULL)
    {
        if (tongueIdx == 10)
        {
            unsigned int tngV = iModelVertCount(tongue->Data);
            for (unsigned int numV = 0; numV < tngV; numV++)
            {
                iModelTag tmpTag;
                iModelTagIndex(&tmpTag, tongue->Data, numV);
                for (unsigned int wnum = 0; wnum < 4; wnum++)
                {
                    if (tmpTag.wt[wnum])
                    {
                        unsigned char widx = tmpTag.matidx >> (wnum * 8);
                        globals.player.TongueFlags[widx >> 5] |= 1 << (widx & 0x1F);
                    }
                }
            }
            break;
        }
        tongue = tongue->Next;
        tongueIdx++;
    }

    for (curr = globals.player.ent.model->Anim->Table->StateList; curr != NULL; curr = curr->Next)
    {
        if ((curr->UserFlags & 0x40) && (curr->Data->NumAnims[0] != 3 || curr->Data->NumAnims[1] != 1))
        {
            curr->UserFlags &= ~0x40;
        }
    }

    for (minst = globals.player.ent.model, index = 0; minst != NULL; minst = minst->Next, index++)
    {
        if (index >= 2)
        {
            minst->Flags &= ~3;
        }
    }

    globals.player.RootUp.x = 0.0f;
    globals.player.RootUp.y = 1.0f;
    globals.player.RootUp.z = 0.0f;
    globals.player.RootUpTarget.x = 0.0f;
    globals.player.RootUpTarget.y = 1.0f;
    globals.player.RootUpTarget.z = 0.0f;
    globals.player.HangElapsed = 0.0f;
    globals.player.WorldDisguise = 5;
    globals.player.HangEnt = NULL;
    globals.player.HangEntLast = NULL;
    globals.player.Shaggy1.Grab = NULL;
    globals.player.Shaggy1.GrabTry = NULL;
    globals.player.Shaggy1.GrabTimer = 0.0f;
    globals.player.Shaggy1.HangTarget = NULL;
    globals.player.Shaggy1.ThrowTimer = -1.0f;
    globals.player.Shaggy1.ThrowTgt = NULL;
    xEntBeginCollide(&globals.player.ent, globals.sceneCur, 0.0f);
    xEntSetNostepNormAngle(surfSlideStart);
    curVelmag = 0.0f;
    curVelangle = 0.0f;
    surfSlickness = 1;
    surfFriction = 1.0f;
    surfDamping = minVelmag;
    lastSlickness = 1;
    surfSticky = 0;
    zEntPlayer_SNDInit();
    sVONextSituation = NULL;
    sVOTimer = 0.0f;
    for (signed int i = 0; i < 15; i++)
    {
        sPlayerInfo[i].elapsedTime = sPlayerInfo[i].delay;
        sPlayerInfo[i].count = 0;
    }
    xAnimPlaySetState(ent->model->Anim->Single, ent->model->Anim->Table->StateList, 0.0f);
    if (globals.sceneCur->sceneID == 'MNU3')
    {
        globals.player.ControlOn = 0;
        globals.player.ControlOnEvent = 0;
    }
    iColor_tag clear = { 0, 0, 0, 0xFF };
    iColor_tag black = { 0 };
    xScrFxFade(&clear, &black, 1.0f, NULL, 0, 1);
}

static _xEnt* PlayerCollCheckOneEnt(_xEnt* ent, _xScene*, void* data)
{
    _xEnt* p = (_xEnt*)data;
    unsigned int modl_coll = 0;
    unsigned int head_hit = 0;

    if (p->collis->idx >= 15)
    {
        return NULL;
    }
    if (!(ent->chkby & p->collType))
    {
        return ent;
    }
    if (ent->id == p->id)
    {
        return ent;
    }

    _xCollis* coll = &p->collis->colls[p->collis->idx];
    if (ent->collLev == 5 && (p->collType & 0x18))
    {
        modl_coll = 1;
    }
    if (modl_coll)
    {
        coll->flags = 0;
    }
    else
    {
        coll->flags = 0x1F00;
    }

    xBoundHitsBound(&p->bound, &ent->bound, coll);

    _xCollis coll2;
    coll2.flags = 0x1F00;
    coll2.optr = NULL;
    coll2.dist = 1e19f;
    _xSphere temp = p->bound.sph;
    p->bound.sph = globals.player.head_sph;
    xBoundHitsBound(&p->bound, &ent->bound, &coll2);
    p->bound.sph = temp;
    if ((coll2.flags & 1) && (!(coll->flags & 1) || !(coll->dist < coll2.dist)))
    {
        *coll = coll2;
        head_hit = 1;
    }

    if (coll->flags & 1)
    {
        if (modl_coll)
        {
            coll->flags = 0x1F00;
            unsigned char ncolls =
                iSphereHitsModel3(head_hit ? &globals.player.head_sph : &p->bound.sph, ent->model, coll,
                                  15 - p->collis->idx, 0.7853982f);
            for (unsigned char idx = 0; idx < ncolls; idx++)
            {
                coll[idx].optr = ent;
                p->collis->idx++;
            }
            return ent;
        }

        coll->oid = 0;
        coll->optr = ent;
        p->collis->idx++;
        if (coll->flags & 0x10)
        {
            _xVec3* pc = xBoundCenter(&p->bound);
            _xVec3* ec = xBoundCenter(&ent->bound);
            coll->tohit.x = ec->x - pc->x;
            coll->tohit.y = ec->y - pc->y;
            coll->tohit.z = ec->z - pc->z;
            float len = xsqrt(coll->tohit.x * coll->tohit.x + coll->tohit.y * coll->tohit.y +
                              coll->tohit.z * coll->tohit.z);
            float s = -0.35f * (1.0f / len);
            coll->depen.x = coll->tohit.x * s;
            coll->depen.y = coll->tohit.y * s;
            coll->depen.z = coll->tohit.z * s;
            coll->depen.y = 0.0f;
            len = xsqrt(coll->depen.x * coll->depen.x + coll->depen.y * coll->depen.y +
                        coll->depen.z * coll->depen.z);
            s = -1.0f / len;
            coll->hdng.x = coll->depen.x * s;
            coll->hdng.y = coll->depen.y * s;
            coll->hdng.z = coll->depen.z * s;
            coll->dist = 0.7f - len;
            coll->tohit.x = coll->hdng.x * coll->dist;
            coll->tohit.y = coll->hdng.y * coll->dist;
            coll->tohit.z = coll->hdng.z * coll->dist;
        }
        if ((ent->pflags & 0x20) && ent->bound.type == 1 && p->bound.type == 1 && coll->hdng.y < -0.866025f)
        {
            float dx = p->bound.sph.center.x - ent->bound.sph.center.x;
            float dz = p->bound.sph.center.z - ent->bound.sph.center.z;
            float rsum = p->bound.sph.r + ent->bound.sph.r;
            float hsqr = rsum * rsum - (dx * dx + dz * dz);
            if (hsqr >= 0.0f)
            {
                coll->depen.x = 0.0f;
                coll->depen.y = xsqrt(hsqr) - (p->bound.sph.center.y - ent->bound.sph.center.y);
                coll->depen.z = 0.0f;
                coll->dist = p->bound.sph.r - coll->depen.y;
                coll->hdng.x = 0.0f;
                coll->hdng.y = -1.0f;
                coll->hdng.z = 0.0f;
            }
        }
    }
    return ent;
}

static _xEnt* PlayerCollCheckOneVillain(_xEnt* ent, _xScene*, void* data)
{
    _xEnt* p = (_xEnt*)data;
    unsigned int modl_coll = 0;

    if (p->collis->idx >= 15)
    {
        return NULL;
    }
    if (!(ent->chkby & p->collType))
    {
        return ent;
    }
    if (ent->id == p->id)
    {
        return ent;
    }

    _xCollis* coll = &p->collis->colls[p->collis->idx];
    if (ent->collLev == 5 && (p->collType & 0x18))
    {
        modl_coll = 1;
    }
    if (modl_coll)
    {
        coll->flags = 0;
    }
    else
    {
        coll->flags = 0x1F00;
    }

    if ((cchkButtbounce || (cchkSquish && (ent->subType == 0x16 || ent->subType == 0x18 ||
                                           ent->subType == 0x15 || ent->subType == 0))) &&
        (p->frame->oldpos.y >= ent->frame->mat.pos.y + ent->bound.sph.r ||
         p->frame->oldpos.y >= ent->frame->oldpos.y + ent->bound.sph.r) &&
        p->frame->mat.pos.y <= ent->frame->mat.pos.y + ent->bound.sph.r)
    {
        float playerOldRad = p->bound.sph.r;
        p->bound.sph.r = playerOldRad * 1.5f;
        xBoundHitsBound(&p->bound, &ent->bound, coll);
        p->bound.sph.r = playerOldRad;
        if (coll->flags & 1)
        {
            coll->norm.x = 0.0f;
            coll->norm.y = 1.0f;
            coll->norm.z = 0.0f;
            coll->depen.x = 0.0f;
            coll->depen.y = (ent->frame->mat.pos.y + ent->bound.sph.r) - p->frame->mat.pos.y;
            coll->depen.z = 0.0f;
            coll->dist = p->bound.sph.r - coll->depen.y;
            coll->hdng.x = 0.0f;
            coll->hdng.y = -1.0f;
            coll->hdng.z = 0.0f;
            coll->tohit.x = coll->hdng.x * coll->dist;
            coll->tohit.y = coll->hdng.y * coll->dist;
            coll->tohit.z = coll->hdng.z * coll->dist;
            coll->oid = ent->id;
            coll->optr = ent;
            p->collis->idx++;
            return ent;
        }
    }

    xBoundHitsBound(&p->bound, &ent->bound, coll);

    st_zEntVillain* vill = (st_zEntVillain*)ent;
    if (vill->bound2.sph.r)
    {
        _xCollis coll2;
        coll2.flags = 0x1F00;
        coll2.optr = NULL;
        coll2.dist = 1e19f;
        xBoundHitsBound(&p->bound, &vill->bound2, &coll2);
        if ((coll2.flags & 1) && (!(coll->flags & 1) || !(coll->dist < coll2.dist)))
        {
            *coll = coll2;
        }
    }

    if (coll->flags & 1)
    {
        coll->oid = 0;
        coll->optr = ent;
        p->collis->idx++;
        if (coll->flags & 0x10)
        {
            _xVec3* pc = xBoundCenter(&p->bound);
            _xVec3* ec = xBoundCenter(&ent->bound);
            coll->tohit.x = ec->x - pc->x;
            coll->tohit.y = ec->y - pc->y;
            coll->tohit.z = ec->z - pc->z;
            float len = xsqrt(coll->tohit.x * coll->tohit.x + coll->tohit.y * coll->tohit.y +
                              coll->tohit.z * coll->tohit.z);
            float s = -0.35f * (1.0f / len);
            coll->depen.x = coll->tohit.x * s;
            coll->depen.y = coll->tohit.y * s;
            coll->depen.z = coll->tohit.z * s;
            coll->depen.y = 0.0f;
            len = xsqrt(coll->depen.x * coll->depen.x + coll->depen.y * coll->depen.y +
                        coll->depen.z * coll->depen.z);
            s = -1.0f / len;
            coll->hdng.x = coll->depen.x * s;
            coll->hdng.y = coll->depen.y * s;
            coll->hdng.z = coll->depen.z * s;
            coll->dist = 0.7f - len;
            coll->tohit.x = coll->hdng.x * coll->dist;
            coll->tohit.y = coll->hdng.y * coll->dist;
            coll->tohit.z = coll->hdng.z * coll->dist;
        }
        if ((ent->pflags & 0x20) && ent->bound.type == 1 && p->bound.type == 1 && coll->hdng.y < -0.866025f)
        {
            float dx = p->bound.sph.center.x - ent->bound.sph.center.x;
            float dz = p->bound.sph.center.z - ent->bound.sph.center.z;
            float rsum = p->bound.sph.r + ent->bound.sph.r;
            float hsqr = rsum * rsum - (dx * dx + dz * dz);
            if (hsqr >= 0.0f)
            {
                coll->depen.x = 0.0f;
                coll->depen.y = xsqrt(hsqr) - (p->bound.sph.center.y - ent->bound.sph.center.y);
                coll->depen.z = 0.0f;
                coll->dist = p->bound.sph.r - coll->depen.y;
                coll->hdng.x = 0.0f;
                coll->hdng.y = -1.0f;
                coll->hdng.z = 0.0f;
            }
        }
    }
    return ent;
}

static void PlayerCollsSideTranslate(_xCollis* colls, float x, float y, float z)
{
    PlayerCollisTranslate(&colls[3], x, y, z);
    PlayerCollisTranslate(&colls[5], x, y, z);
}

static void PlayerCollsSelectDepen(_xEnt* p)
{
    _xCollis* colls = &p->collis->colls[0];
    _xMat4x3* mat = (_xMat4x3*)p->frame;
    _xCollis* c;
    _xCollis* cend;

    for (c = colls + 6, cend = colls + p->collis->idx; c < cend; c++)
    {
        _xCollis* curr = &colls[xCollideGetCollsIdx(&c->tohit, (_xMat3x3*)mat)];
        if (c->dist < curr->dist)
        {
            *curr = *c;
        }
    }

    if (p->pflags & 0x80)
    {
        _xCollis* coll = &p->collis->colls[0];
        if ((coll->flags & 1) && coll->dist < 0.7f)
        {
            float h_dot_n = coll->hdng.x * coll->norm.x + coll->hdng.y * coll->norm.y + coll->hdng.z * coll->norm.z;
            if (h_dot_n > 0.0f)
            {
                h_dot_n = -h_dot_n;
                coll->norm.x *= -1.0f;
                coll->norm.y *= -1.0f;
                coll->norm.z *= -1.0f;
            }
            if ((float)__fabs((float)acos(coll->norm.x * update_motion.x + coll->norm.y * update_motion.y +
                                          coll->norm.z * update_motion.z)) < 0.7853982f)
            {
                float depen_len = h_dot_n * coll->dist + 0.7f;
                if (depen_len < 0.0f || depen_len > 0.7f)
                {
                    depen_len = (((depen_len < 0.7f) ? depen_len : 0.7f) < 0.0f)
                                    ? 0.0f
                                    : ((depen_len < 0.7f) ? depen_len : 0.7f);
                }
                coll->depen.x = coll->norm.x * depen_len;
                coll->depen.y = coll->norm.y * depen_len;
                coll->depen.z = coll->norm.z * depen_len;
            }
        }
    }

    for (c = colls; c < colls + 6; c++)
    {
        _xEnt* cent = (_xEnt*)c->optr;
        if (cent != NULL)
        {
            if (!(cent->penby & p->collType) || !(p->collis->pen & cent->collType))
            {
                c->depen.x = 0.0f;
                c->depen.y = 0.0f;
                c->depen.z = 0.0f;
            }
            else
            {
                xSurface* surf;
                if (globals.player.DamageTimer > 0.0f && (surf = zSurfaceGetSurface(c), surf != NULL) &&
                    surf->state == 0 && zSurfaceGetDamageType(surf) && zSurfaceGetDamagePassthrough(surf))
                {
                    c->depen.x = 0.0f;
                    c->depen.y = 0.0f;
                    c->depen.z = 0.0f;
                }
                else if (cent->baseType == 2)
                {
                    if (c == colls)
                    {
                        c->depen.y = 0.0f;
                    }
                    else if (c != &colls[2])
                    {
                        c->depen.x = 0.0f;
                        c->depen.y = 0.0f;
                        c->depen.z = 0.0f;
                    }
                }
            }
        }
    }

    xSurface* sfloor = zSurfaceGetSurface(colls);
    if (p->frame->vel.y > 0.0f)
    {
        p->collis->colls[0].flags &= ~1;
    }

    if ((p->collis->colls[0].flags & 1) && colls[0].dist < 0.7f)
    {
        if (sfloor != NULL && (!zSurfaceGetStandOn(sfloor) || globals.player.Slide == 1 || globals.player.Slide == 3))
        {
            mat->pos.x += colls[0].depen.x;
            mat->pos.y += colls[0].depen.y;
            mat->pos.z += colls[0].depen.z;
            PlayerCollsWallsTranslate(colls, colls[0].depen.x, colls[0].depen.y, colls[0].depen.z);
        }
        else
        {
            mat->pos.y += colls[0].depen.y;
            PlayerCollsWallsTranslate(colls, 0.0f, colls[0].depen.y, 0.0f);
        }
    }
    else if ((colls[1].flags & 1) && colls[1].dist < 0.7f)
    {
        _xCollis* cceil = &colls[1];
        if (cceil->hdng.y < (float)cos(0.5235987901687622))
        {
            mat->pos.x += cceil->depen.x;
            mat->pos.y += cceil->depen.y;
            mat->pos.z += cceil->depen.z;
            PlayerCollsWallsTranslate(colls, cceil->depen.x, cceil->depen.y, cceil->depen.z);
        }
        else
        {
            mat->pos.y += cceil->depen.y;
            PlayerCollsWallsTranslate(colls, 0.0f, cceil->depen.y, 0.0f);
            p->frame->vel.y = 0.0f;
        }
    }

    signed int num_walls = 0;
    _xCollis* first_wall = NULL;
    _xCollis* inside_wall = NULL;
    for (c = colls + 2, cend = colls + 6; c < cend; c++)
    {
        if (c->flags & 0x10)
        {
            inside_wall = c;
        }
        if (c->flags & 1)
        {
            if (c->optr != NULL)
            {
                num_walls++;
                if (first_wall == NULL)
                {
                    first_wall = c;
                }
            }
            else
            {
                num_walls++;
                if (first_wall == NULL)
                {
                    first_wall = c;
                }
            }
        }
    }
    if (inside_wall != NULL)
    {
        inside_wall->flags &= ~1;
    }

    if (num_walls)
    {
        if (num_walls == 1)
        {
            mat->pos.x += first_wall->depen.x;
            mat->pos.z += first_wall->depen.z;
        }

        _xCollis* cfront = &colls[2];
        _xCollis* crear = &colls[4];
        if ((cfront->flags & 1) && cfront->dist < 0.7f)
        {
            if (cfront->optr != NULL)
            {
                if ((crear->flags & 1) && crear->dist < 0.7f)
                {
                    if (crear->optr != NULL)
                    {
                        float dx = 0.5f * (crear->depen.x + cfront->depen.x);
                        float dz = 0.5f * (crear->depen.z + cfront->depen.z);
                        mat->pos.x += dx;
                        mat->pos.z += dz;
                        PlayerCollsSideTranslate(colls, dx, 0.0f, dz);
                    }
                    else
                    {
                        mat->pos.x += crear->depen.x;
                        mat->pos.z += crear->depen.z;
                        PlayerCollsSideTranslate(colls, crear->depen.x, 0.0f, crear->depen.z);
                    }
                }
                else
                {
                    mat->pos.x += cfront->depen.x;
                    mat->pos.z += cfront->depen.z;
                    PlayerCollsSideTranslate(colls, cfront->depen.x, 0.0f, cfront->depen.z);
                }
            }
            else if ((crear->flags & 1) && crear->dist < 0.7f)
            {
                if (crear->optr != NULL)
                {
                    mat->pos.x += cfront->depen.x;
                    mat->pos.z += cfront->depen.z;
                    PlayerCollsSideTranslate(colls, cfront->depen.x, 0.0f, cfront->depen.z);
                }
                else
                {
                    float dx = 0.5f * (crear->depen.x + cfront->depen.x);
                    float dz = 0.5f * (crear->depen.z + cfront->depen.z);
                    mat->pos.x += dx;
                    mat->pos.z += dz;
                    PlayerCollsSideTranslate(colls, dx, 0.0f, dz);
                }
            }
            else
            {
                mat->pos.x += cfront->depen.x;
                mat->pos.z += cfront->depen.z;
                PlayerCollsSideTranslate(colls, cfront->depen.x, 0.0f, cfront->depen.z);
            }
        }
        else if ((crear->flags & 1) && crear->dist < 0.7f)
        {
            if (crear->optr != NULL)
            {
                mat->pos.x += crear->depen.x;
                mat->pos.z += crear->depen.z;
                PlayerCollsSideTranslate(colls, crear->depen.x, 0.0f, crear->depen.z);
            }
            else
            {
                mat->pos.x += crear->depen.x;
                mat->pos.z += crear->depen.z;
                PlayerCollsSideTranslate(colls, crear->depen.x, 0.0f, crear->depen.z);
            }
        }

        _xCollis* cleft = &colls[3];
        _xCollis* cright = &colls[5];
        if ((cleft->flags & 1) && cleft->dist < 0.7f)
        {
            if (cleft->optr != NULL)
            {
                if ((cright->flags & 1) && cright->dist < 0.7f)
                {
                    if (cright->optr != NULL)
                    {
                        mat->pos.x += 0.5f * (cright->depen.x + cleft->depen.x);
                        mat->pos.z += 0.5f * (cright->depen.z + cleft->depen.z);
                    }
                    else
                    {
                        mat->pos.x += cright->depen.x;
                        mat->pos.z += cright->depen.z;
                    }
                }
                else
                {
                    mat->pos.x += cleft->depen.x;
                    mat->pos.z += cleft->depen.z;
                }
            }
            else if ((cright->flags & 1) && cright->dist < 0.7f)
            {
                if (cright->optr != NULL)
                {
                    mat->pos.x += cleft->depen.x;
                    mat->pos.z += cleft->depen.z;
                }
                else
                {
                    mat->pos.x += 0.5f * (cright->depen.x + cleft->depen.x);
                    mat->pos.z += 0.5f * (cright->depen.z + cleft->depen.z);
                }
            }
            else
            {
                mat->pos.x += cleft->depen.x;
                mat->pos.z += cleft->depen.z;
            }
        }
        else if ((cright->flags & 1) && cright->dist < 0.7f)
        {
            if (cright->optr != NULL)
            {
                mat->pos.x += cright->depen.x;
                mat->pos.z += cright->depen.z;
            }
            else
            {
                mat->pos.x += cright->depen.x;
                mat->pos.z += cright->depen.z;
            }
        }
    }
}

void zEntPlayerCollide(_xEnt* ent, _xScene* sc, float dt)
{
    _xCollis* c;
    _xCollis* cend;

    cchkButtbounce = globals.player.Health && (ent->model->Anim->Single->State->UserFlags & 0x1E) == 0xE;
    cchkSquish = globals.player.Health && (ent->model->Anim->Single->State->UserFlags & 0x1E) == 0xA &&
                 ent->frame->mat.pos.y < ent->frame->oldpos.y;

    xEntBeginCollide(ent, sc, dt);

    if (ent->collis->chk & 0x20)
    {
        ent->collis->env_sidx = ent->collis->idx;
        ent->bound.sph.r = 0.90999997f;
        iSphereHitsEnv4(&ent->bound.sph, sc->env, (_xMat3x3*)ent->frame, &ent->collis->colls[0]);
        ent->bound.sph.r = 0.7f;
        globals.player.head_sph.r = 0.90999997f;
        iSphereHitsEnv4(&globals.player.head_sph, sc->env, (_xMat3x3*)ent->frame, &ent->collis->colls[0]);
        globals.player.head_sph.r = 0.7f;

        PlayerCollsAllTranslate(&ent->collis->colls[0], 0.0f, 0.0f, 0.0f);
        ent->collis->env_eidx = ent->collis->idx;
    }

    if (ent->collis->chk & 8)
    {
        xEntCollCheckNPCs(ent, sc, PlayerCollCheckOneVillain);
    }
    if (ent->collis->chk & 4)
    {
        xEntCollCheckDyns(ent, sc, PlayerCollCheckOneEnt);
    }
    if (ent->collis->chk & 2)
    {
        xEntCollCheckStats(ent, sc, PlayerCollCheckOneEnt);
    }
    if (ent->collis->chk & 0x2E)
    {
        PlayerCollsSelectDepen(ent);
        _xCollis* colls = &ent->collis->colls[0];
        for (c = colls + 2, cend = colls + ent->collis->idx; c < cend; c++)
        {
            if (c->dist > 0.7f)
            {
                c->flags &= ~1;
            }
        }
        xEntEndCollide(ent, sc, dt);
    }
}

void zEntPlayerCollTrigger(_xEnt* ent, _xScene* sc)
{
    unsigned short i;

    xEntBeginCollCheckTrigs(ent);
    xSceneForAllTriggers(sc, zEntPlayerCollTriggerCB, ent);
    xEntEndCollCheckTrigs(ent);

    for (i = 0; i < sc->num_trigs; i++)
    {
        if (ent->baseFlags & 1)
        {
            if (ent->collis->trigs_entered[i >> 3] & (1 << (i % 8)))
            {
                p2EntTriggerAsset* asset = (p2EntTriggerAsset*)sc->trigs[i]->asset;
                if (asset->flags & 1)
                {
                    _xMat4x3* mat = (_xMat4x3*)ent->model->Mat;
                    if (asset->direction.x * mat->at.x + asset->direction.y * mat->at.y +
                            asset->direction.z * mat->at.z <=
                        0.0f)
                    {
                        zEntEvent(sc->trigs[i], 5, NULL);
                    }
                }
                else
                {
                    zEntEvent(sc->trigs[i], 5, NULL);
                }
            }
            if (ent->collis->trigs_exited[i >> 3] & (1 << (i % 8)))
            {
                p2EntTriggerAsset* asset = (p2EntTriggerAsset*)sc->trigs[i]->asset;
                if (asset->flags & 1)
                {
                    _xMat4x3* mat = (_xMat4x3*)ent->model->Mat;
                    if (asset->direction.x * mat->at.x + asset->direction.y * mat->at.y +
                            asset->direction.z * mat->at.z <=
                        0.0f)
                    {
                        zEntEvent(sc->trigs[i], 6, NULL);
                    }
                }
                else
                {
                    zEntEvent(sc->trigs[i], 6, NULL);
                }
            }
            if ((ent->collis->trigs_cur[i >> 3] & (1 << (i % 8))) && globals.player.ControlOn &&
                (globals.pad0->pressed & 0x20000))
            {
                zEntEvent(sc->trigs[i], 0x3B);
            }
        }
    }
}

_xEnt* zEntPlayerCollTriggerCB(_xEnt* ent, _xScene*, void* data)
{
    _zEnt* p = (_zEnt*)data;
    _zEnt* t = (_zEnt*)ent;
    if (t->baseFlags & 1)
    {
        p2EntTriggerAsset* tasset = (p2EntTriggerAsset*)t->asset;
        switch (t->subType)
        {
        case 0:
        {
            _xBox box;
            _xIsect isect;
            box.upper = tasset->p[1];
            box.lower = tasset->p[0];
            iBoxIsectVec(&box, &p->bound.sph.center, &isect);
            if (isect.penned <= 0.0f)
            {
                p->collis->trigs_cur[(unsigned short)p->collis->trig_idx >> 3] |= (unsigned char)(1 << ((unsigned short)p->collis->trig_idx % 8));
            }
            else
            {
                p->collis->trigs_cur[(unsigned short)p->collis->trig_idx >> 3] &= (unsigned char)~(1 << ((unsigned short)p->collis->trig_idx % 8));
            }
            break;
        }
        case 1:
        {
            _xSphere sphere;
            _xIsect isect;
            sphere.center = tasset->p[0];
            sphere.r = tasset->p[1].x;
            iSphereIsectVec(&sphere, &p->bound.sph.center, &isect);
            if (isect.penned <= 0.0f)
            {
                p->collis->trigs_cur[(unsigned short)p->collis->trig_idx >> 3] |= (unsigned char)(1 << ((unsigned short)p->collis->trig_idx % 8));
            }
            else
            {
                p->collis->trigs_cur[(unsigned short)p->collis->trig_idx >> 3] &= (unsigned char)~(1 << ((unsigned short)p->collis->trig_idx % 8));
            }
            break;
        }
        case 2:
        {
            _xCylinder cylinder;
            _xIsect isect;
            cylinder.center = tasset->p[0];
            cylinder.r = tasset->p[1].x;
            cylinder.h = tasset->p[1].y;
            iCylinderIsectVec(&cylinder, &p->bound.sph.center, &isect);
            if (isect.penned <= 0.0f)
            {
                p->collis->trigs_cur[(unsigned short)p->collis->trig_idx >> 3] |= (unsigned char)(1 << ((unsigned short)p->collis->trig_idx % 8));
            }
            else
            {
                p->collis->trigs_cur[(unsigned short)p->collis->trig_idx >> 3] &= (unsigned char)~(1 << ((unsigned short)p->collis->trig_idx % 8));
            }
            break;
        }
        case 3:
        {
            _xSphere sphere;
            _xIsect isect;
            sphere.center = tasset->p[0];
            sphere.r = tasset->p[1].x;
            iSphereIsectVec(&sphere, &p->bound.sph.center, &isect);
            if (isect.penned <= 0.0f)
            {
                p->collis->trigs_cur[(unsigned short)p->collis->trig_idx >> 3] |= (unsigned char)(1 << ((unsigned short)p->collis->trig_idx % 8));
            }
            else
            {
                p->collis->trigs_cur[(unsigned short)p->collis->trig_idx >> 3] &= (unsigned char)~(1 << ((unsigned short)p->collis->trig_idx % 8));
            }
            break;
        }
        case 4:
        case 5:
            break;
        }
    }
    p->collis->trig_idx++;
    return ent;
}

#pragma auto_inline on
static _xVec3* GetPosVec(xBase* target)
{
    _xVec3* pos = &g_O3;
    switch (target->baseType)
    {
    case 13:
        pos = *(_xVec3**)((char*)target + 0x14); // _xMovePoint::pos
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
        pos = (_xVec3*)&((_xMat4x3*)((_xEnt*)target)->model->Mat)->pos;
        break;
    }
    return pos;
}
#pragma auto_inline reset

signed int zEntPlayerEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                             xBase* toParamWidget)
{
    switch (toEvent)
    {
    case 0x111:
        zEntPlayer_Notify(1);
        break;
    case 0x113:
        zEntPlayer_Notify(8);
        break;
    case 0x116:
        zEntPlayer_Notify(2);
        break;
    case 0x118:
        zEntPlayer_Notify(3);
        break;
    case 0x11B:
        zEntPlayer_Notify(10);
        break;
    case 0x11C:
        zEntPlayer_Notify(5);
        break;
    case 0x8:
        globals.player.ControlOn = 0;
        globals.player.ControlOnEvent = 0;
        globals.player.ControlOffTimer = 1.0f;
        globals.player.AutoMoveSpeed = 0;
        xScrFxLetterbox(1);
        break;
    case 0x9:
        globals.player.ControlOnEvent = 1;
        globals.player.AutoMoveSpeed = 0;
        xScrFxLetterbox(0);
        break;
    case 0x36:
        globals.player.Health++;
        if (globals.player.Health > 5)
        {
            globals.player.Health = 5;
        }
        break;
    case 0x34:
        globals.player.Inv_Chances++;
        if (globals.player.Inv_Chances > 99)
        {
            globals.player.Inv_Chances = 99;
        }
        break;
    case 0x35:
        globals.player.Inv_ScoobySnacks++;
        zSceneGameStatsTally_snack(1);
        if (zEntPickup_lastSnackInArea())
        {
            zHudShowAllSnacksArea();
        }
        break;
    case 0x25:
        if (globals.player.Health)
        {
            globals.player.DamageTimer = 0.0f;
            zEntPlayer_Damage(from, globals.player.Health);
        }
        break;
    case 0x11:
        if (((_xEnt*)to)->asset != NULL)
        {
            ((_xEnt*)to)->frame->mat.pos = ((_xEnt*)to)->asset->pos;
            *(_xVec3*)&((_xMat4x3*)((_xEnt*)to)->model->Mat)->pos = ((_xEnt*)to)->asset->pos;
            ((_xEnt*)to)->frame->vel = g_O3;
            ((_xEnt*)to)->frame->rot.axis = g_Y3;
            ((_xEnt*)to)->frame->rot.angle = 0.0f;
        }
        break;
    case 0x5D:
    {
        char* astatename = ((_xEnt*)to)->model->Anim->Single->State->Name;
        if (xStricmp(astatename, "Fall_losechance") != 0 && xStricmp(astatename, "Fall_losedrop") != 0)
        {
            globals.player.FallDeathTimer = 1.2f;
            globals.player.Health = 0;
            globals.player.DamageTimer = 15.0f;
            globals.camera.hi_cam.flags |= 2;
            if (globals.player.s.TakeDamage)
            {
                gDeadScooby = 1;
                gDeadScoobyPos = *(_xVec3*)&((_xMat4x3*)globals.player.ent.model->Mat)->pos;
            }
            globals.player.ent.frame->vel.x = 0.0f;
            globals.player.ent.frame->vel.z = 0.0f;
        }
        break;
    }
    case 0x3:
        globals.player.Visible = 1;
        break;
    case 0x4:
        globals.player.Visible = 0;
        break;
    case 0xCD:
        if (!globals.player.ControlOn && toParamWidget != NULL)
        {
            globals.player.AutoMoveDist = toParam[0];
            globals.player.AutoMoveSpeed = (unsigned int)toParam[1] + 1;
            globals.player.AutoMoveTarget = *GetPosVec(toParamWidget);
            globals.player.AutoMoveObject = toParamWidget;
            if (globals.player.AutoMoveSpeed > 3 || globals.player.AutoMoveSpeed < 1)
            {
                globals.player.AutoMoveSpeed = 1;
            }
        }
        break;
    case 0xDA:
        if (!globals.player.ControlOn && toParamWidget != NULL)
        {
            globals.player.AutoMoveSpeed = 4;
            globals.player.AutoMoveTarget = *GetPosVec(toParamWidget);
            globals.player.AutoMoveObject = toParamWidget;
        }
        break;
    case 0x29:
        globals.player.AutoMoveSpeed = 0;
        break;
    case 0xE5:
        globals.player.Shaggy1.SetDown = 1;
        break;
    case 0xE6:
        globals.player.Shaggy1.GrabOn = 1;
        break;
    case 0xE7:
        globals.player.Shaggy1.GrabOn = 0;
        break;
    case 0xDD:
        globals.player.Shaggy1.ThrowTgt = toParamWidget;
        globals.player.Shaggy1.ThrowTgtAngle = (toParam[0] != 0.0f) ? toParam[0] : 45.0f;
        globals.player.Shaggy1.ThrowTgtPeak = (toParam[1] != 0.0f) ? toParam[1] : 6.0f;
        globals.player.Shaggy1.ThrowTgtOffset = toParam[2];
        break;
    case 0x12C:
        zRumbleStart(0, (_tagSDRumbleType)0xE);
        break;
    case 0x12D:
        zRumbleStart(0, (_tagSDRumbleType)0xB);
        break;
    case 0x12E:
        zRumbleStart(0, (_tagSDRumbleType)0xC);
        break;
    case 0x12F:
        zRumbleStart(0, (_tagSDRumbleType)0xD);
        break;
    }
    return 1;
}

static void PlayerHangUpdate(_xEnt* ent, float mag, float angle, float dt)
{
    zEntHangable_FollowUpdate(globals.player.HangEnt);
    xModelInstance* model = ent->model;
    if (globals.player.HangEnt->enabled <= 0 || !(model->Anim->Single->State->UserFlags & 0x20000))
    {
        globals.player.HangEnt->grabTimer = globals.player.HangEnt->hangInfo->grabDelay;
        globals.player.HangEnt = NULL;
        globals.player.RootUpTarget.x = 0.0f;
        globals.player.RootUpTarget.y = 1.0f;
        globals.player.RootUpTarget.z = 0.0f;
        globals.player.HangElapsed = 0.0f;
        globals.player.Jump_CanDouble = 1;
        globals.player.Jump_CanFloat = 1;
        ent->frame->vel.y = globals.player.HangVel.y;
    }

    float px = ((_xMat4x3*)model->Mat)->pos.x;
    float py = ((_xMat4x3*)model->Mat)->pos.y + ent->bound.sph.r;
    float pz = ((_xMat4x3*)model->Mat)->pos.z;
    float fx = mag * isin(angle);
    float fy = -globals.player.HangEnt->hangInfo->gravity;
    float fz = mag * icos(angle);
    if (globals.player.HangEnt->hangInfo->flags & 1)
    {
        fz *= 1.2f;
        fx *= 1.2f;
        float d = fx * globals.player.HangEnt->swingplane.x + fz * globals.player.HangEnt->swingplane.z;
        fz -= d * globals.player.HangEnt->swingplane.z;
        fx -= d * globals.player.HangEnt->swingplane.x;
        float m2 = fx * fx + fz * fz;
        if (m2 > 1.0f)
        {
            float inv = 1.0f / m2;
            fx *= inv;
            fz *= inv;
        }
    }
    fx *= globals.player.HangEnt->hangInfo->accel;
    fz *= globals.player.HangEnt->hangInfo->accel;

    _xVec3 unitHang;
    unitHang.x = px - globals.player.HangPivot.x;
    unitHang.y = py - globals.player.HangPivot.y;
    unitHang.z = pz - globals.player.HangPivot.z;
    xVec3Normalize(&unitHang, &unitHang);

    float removedot = unitHang.x * fx + unitHang.y * fy + unitHang.z * fz;
    globals.player.HangVel.y += (fy - removedot * unitHang.y) * dt;
    globals.player.HangVel.x += (fx - removedot * unitHang.x) * dt;
    globals.player.HangVel.z += (fz - removedot * unitHang.z) * dt;

    removedot = unitHang.x * globals.player.HangVel.x + unitHang.y * globals.player.HangVel.y +
                unitHang.z * globals.player.HangVel.z;
    globals.player.HangVel.x -= removedot * unitHang.x;
    globals.player.HangVel.y -= removedot * unitHang.y;
    globals.player.HangVel.z -= removedot * unitHang.z;

    float decay = 1.0f - globals.player.HangEnt->hangInfo->decay;
    globals.player.HangVel.x *= decay;
    globals.player.HangVel.y *= decay;
    globals.player.HangVel.z *= decay;

    unitHang.x = (globals.player.HangVel.x * dt + px) - globals.player.HangPivot.x;
    unitHang.y = (globals.player.HangVel.y * dt + py) - globals.player.HangPivot.y;
    unitHang.z = (globals.player.HangVel.z * dt + pz) - globals.player.HangPivot.z;
    xVec3Normalize(&unitHang, &unitHang);

    globals.player.RootUpTarget = unitHang;
    globals.player.RootUpTarget.x *= -1.0f;
    globals.player.RootUpTarget.y *= -1.0f;
    globals.player.RootUpTarget.z *= -1.0f;
    globals.player.HangElapsed += dt;

    unitHang.x *= globals.player.HangLength;
    unitHang.y *= globals.player.HangLength;
    unitHang.z *= globals.player.HangLength;
    ent->frame->mat.pos.x = globals.player.HangPivot.x + unitHang.x;
    ent->frame->mat.pos.y = globals.player.HangPivot.y + unitHang.y;
    ent->frame->mat.pos.z = globals.player.HangPivot.z + unitHang.z;
    ent->frame->mat.pos.y -= ent->bound.sph.r;
    ent->frame->mode |= 1;

    if (globals.player.HangStartLerp < 1.0f)
    {
        float lerp = 5.0f * dt + globals.player.HangStartLerp;
        globals.player.HangStartLerp = (lerp > 1.0f) ? 1.0f : lerp;
    }

    if (globals.player.HangEnt != NULL && globals.player.HangEnt->baseType == 0x17)
    {
        globals.player.HangEnt->endpos.x = ent->frame->mat.pos.x;
        globals.player.HangEnt->endpos.y = ent->frame->mat.pos.y + ent->bound.sph.r;
        globals.player.HangEnt->endpos.z = ent->frame->mat.pos.z;
        globals.player.HangEnt->vel = globals.player.HangVel;
        if (ent->frame->mode & 0x20)
        {
            globals.player.HangEnt->spin += ent->frame->drot.angle;
        }
        zEntHangable_SetMatrix(globals.player.HangEnt, dt);
    }
}

static void PlayerHangCheck(_xEnt* ent, _xScene* sc)
{
    unsigned int i;
    _zEntHangable* hangclose;
    float closedist;

    globals.player.HangFound = NULL;
    if (globals.player.HangEnt == NULL)
    {
        hangclose = NULL;
        closedist = 1e7f;
        for (i = 0; i < sc->num_dyns; i++)
        {
            _zEntHangable* hangme = (_zEntHangable*)sc->dyns[i];
            if (hangme->baseType == 0x17 && hangme->enabled > 0 && hangme->grabTimer == 0.0f &&
                (hangme->state != 1 || hangme->shaggy != NULL))
            {
                float dx = ent->bound.sph.center.x - hangme->endpos.x;
                float dy = ent->bound.sph.center.y - hangme->endpos.y;
                float dz = ent->bound.sph.center.z - hangme->endpos.z;
                float d = dx * dx + dy * dy + dz * dz;
                if (d < closedist)
                {
                    closedist = d;
                    hangclose = hangme;
                }
            }
        }

        if (hangclose != NULL)
        {
            if (closedist > 16.0f)
            {
                return;
            }
            _xVec3 oldpos = ent->frame->oldpos;
            oldpos.y += ent->bound.sph.r;

            float dx = hangclose->pivot.x - oldpos.x;
            float dy = hangclose->pivot.y - oldpos.y;
            float dz = hangclose->pivot.z - oldpos.z;
            float d1 = xsqrt(dx * dx + dy * dy + dz * dz);

            dx = hangclose->pivot.x - ent->bound.sph.center.x;
            dy = hangclose->pivot.y - ent->bound.sph.center.y;
            dz = hangclose->pivot.z - ent->bound.sph.center.z;
            float d2 = xsqrt(dx * dx + dy * dy + dz * dz);

            unsigned int sphereresult =
                ((d1 - hangclose->hangInfo->leverArm) * (d2 - hangclose->hangInfo->leverArm) <= 0.0f) ||
                ((float)__fabs(d2 - hangclose->hangInfo->leverArm) < 0.15f);

            dx = hangclose->endpos.x - ent->bound.sph.center.x;
            dy = hangclose->endpos.y - ent->bound.sph.center.y;
            dz = hangclose->endpos.z - ent->bound.sph.center.z;
            float dot = dx * gPlayerAbsMat.right.x + dy * gPlayerAbsMat.right.y + dz * gPlayerAbsMat.right.z;
            dx -= dot * gPlayerAbsMat.right.x;
            dy -= dot * gPlayerAbsMat.right.y;
            dz -= dot * gPlayerAbsMat.right.z;
            unsigned int inplane = (dx * dx + dy * dy + dz * dz < 2.25f) && ((float)__fabs(dot) < 0.65f);

            if (inplane && sphereresult)
            {
                globals.player.HangFound = hangclose;
            }
        }
    }
}

static void PlayerTeeterCheck(float dt)
{
    signed int i;

    if (globals.player.MountChimney)
    {
        globals.player.Teeter = 4;
    }
    if (globals.player.Teeter)
    {
        globals.player.Teeter--;
    }
    if (globals.player.JumpState == 0)
    {
        for (i = 0; i < 4; i++)
        {
            if (floor_dist[i] > 0.424264f)
            {
                if (floor_tmr[i] >= 0.2f)
                {
                    floor_tmr[i] = 0.2f;
                    globals.player.Teeter = 1;
                }
                else
                {
                    floor_tmr[i] += dt;
                }
            }
            else
            {
                floor_tmr[i] = 0.0f;
            }
        }
    }
}

static void PlayerDigUpdate(float dt)
{
    unsigned int i;
    p2LinkAsset* curlink;
    float digT = globals.player.DigTimer;
    float digDelta = digT + dt;

    if (digT < 0.8f && digDelta >= 0.8f)
    {
        curlink = globals.player.Diggable->link;
        for (i = 0; i < globals.player.Diggable->linkCount; i++, curlink++)
        {
            xBase* tgt = zSceneFindObject(curlink->dstAssetID);
            if (tgt->baseType == 4 && ((_zEntPickup*)tgt)->state == 0x10)
            {
                zEntPickup_Drop((_zEntPickup*)tgt);
                zEntPlayer_Notify(5);
            }
        }
    }

    if (digT < 0.4f && digDelta >= 0.4f)
    {
        curlink = globals.player.Diggable->link;
        for (i = 0; i < globals.player.Diggable->linkCount; i++, curlink++)
        {
            xBase* tgt = zSceneFindObject(curlink->dstAssetID);
            if (tgt->baseType == 6)
            {
                _xEnt* tgtEnt = (_xEnt*)tgt;
                tgtEnt->flags |= 1;
                tgtEnt->chkby = 0x18;
                if (tgtEnt->model->Anim != NULL)
                {
                    tgtEnt->model->Anim->Single->CurrentSpeed = tgtEnt->model->Anim->Single->State->Speed;
                    tgtEnt->model->Anim->Single->Time = 0.0f;
                }
            }
        }
    }

    if (digT < 0.8f && digDelta >= 0.8f)
    {
        globals.player.Diggable->flags &= 0xFE;
        globals.player.Diggable->chkby = 0;
    }

    if (digDelta > 2.0f)
    {
        digDelta = 0.0f;
    }
    globals.player.DigTimer = digDelta;
}

static void PlayerRotMatchUpdateEnt(_xEnt* ent, _xScene*, float dt, void* fdata)
{
    xFFXRotMatchState* rms = (xFFXRotMatchState*)fdata;
    if (ent->collis != NULL && ent->frame != NULL)
    {
        signed int hit_it = ent->collis->colls[0].flags & 1;
        xSurface* surf = zSurfaceGetSurface(&ent->collis->colls[0]);
        signed int grounded = 0;
        if (hit_it && surf != NULL && surf->state == 0 && zSurfaceGetMatchOrient(surf))
        {
            grounded = 1;
        }

        if (grounded)
        {
            if (!rms->lgrounded)
            {
                rms->tmr = 0.0f;
            }
            _xVec3* fup = &globals.player.floor_norm;
            _xVec3* eup = &globals.player.RootUpTarget;
            _xVec3 nfup;
            _xVec3 neup;
            xVec3Normalize(&nfup, fup);
            xVec3Normalize(&neup, eup);
            float fdecl = acos(nfup.y);
            float edecl = acos(neup.y);
            if (edecl < rms->max_decl || fdecl < rms->max_decl)
            {
                _xVec3 raxis;
                xVec3Cross(&raxis, eup, fup);
                xVec3Normalize(&raxis, &raxis);
                float dang = acos(nfup.x * neup.x + nfup.y * neup.y + nfup.z * neup.z);
                if (dang)
                {
                    float s = dt / rms->tmatch;
                    s = (1.0f < s) ? 1.0f : s;
                    if (fdecl >= rms->max_decl)
                    {
                        float rang = (rms->max_decl - edecl) / (fdecl - edecl);
                        s = (s < rang) ? s : rang;
                    }
                    if (s != 0.0f)
                    {
                        _xMat4x3 rot;
                        xMat4x3Rot(&rot, &raxis, s * dang - 0.001f,
                                   (_xVec3*)&((_xMat4x3*)ent->model->Mat)->pos);
                        xMat3x3RMulVec(eup, (_xMat3x3*)&rot, &neup);
                        globals.player.HangElapsed = 0.0f;
                    }
                }
            }
        }
        else if (globals.player.HangEnt == NULL)
        {
            if (rms->lgrounded)
            {
                rms->tmr = -0.1f;
            }
            if (rms->tmr > 0.0f)
            {
                _xVec3* eup = &globals.player.RootUpTarget;
                _xVec3 neup;
                _xVec3 raxis;
                xVec3Normalize(&neup, eup);
                raxis.x = -neup.z;
                raxis.y = 0.0f;
                raxis.z = neup.x;
                float rang = acos(neup.y);
                if (rang != 0.0f)
                {
                    float s = dt / rms->trelax;
                    s = (s > 1.0f) ? 1.0f : s;
                    _xMat4x3 rot;
                    xMat4x3Rot(&rot, &raxis, s * rang, (_xVec3*)&((_xMat4x3*)ent->model->Mat)->pos);
                    xMat3x3RMulVec(eup, (_xMat3x3*)&rot, &neup);
                    globals.player.HangElapsed = 0.0f;
                }
            }
        }

        if ((float)__fabs(globals.player.RootUpTarget.y - 1.0f) < 0.00001f)
        {
            globals.player.RootUpTarget.y = 1.0f;
            globals.player.RootUpTarget.x = 0.0f;
            globals.player.RootUpTarget.z = 0.0f;
            globals.player.HangElapsed = 0.0f;
        }
        rms->tmr += dt;
        rms->lgrounded = grounded;
    }
}

void zEntPlayer_StoreCheckPoint(float* toParam)
{
    _xVec3 p = globals.player.ent.frame->mat.pos;
    float r = globals.player.ent.frame->rot.angle;
    globals.player.cp.pos = p;
    if (toParam != NULL && *toParam)
    {
        globals.player.cp.rot = *toParam;
    }
    else
    {
        globals.player.cp.rot = r;
    }
    globals.player.cp.initCamID = globals.camera.hi_cam.id;
}

void zEntPlayer_LoadCheckPoint()
{
    globals.player.ent.frame->mat.pos = globals.player.cp.pos;
    *(_xVec3*)&((_xMat4x3*)globals.player.ent.model->Mat)->pos = globals.player.cp.pos;
    if (globals.player.cp.rot != 0.0f)
    {
        globals.player.ent.frame->rot.angle = globals.player.cp.rot;
    }
    xCameraSetTarget(&globals.camera, &globals.player.ent.frame->mat.pos);
    xCameraSetTarget(&globals.camera, (_xMat4x3*)globals.player.ent.model->Mat);
    xCameraPickInitial(&globals.camera, globals.player.cp.initCamID);
}

void zEntPlayer_SNDInit()
{
    char name[256];
    signed int i;

    i = 0;
    sPlayerSndfx.butt_id[i++] = xStrHash("sndfx_butt_smash1");
    sPlayerSndfx.butt_id[i++] = xStrHash("sndfx_butt_smash2");
    sPlayerSndfx.butt_id[i++] = xStrHash("sndfx_butt_smash3");
    sPlayerSndfx.butt_pitch = 359.0f * iurand();
    sPlayerSndfx.butt_snd = rand() % 3;
    i = 0;
    sPlayerSndfx.head_id[i++] = xStrHash("sndfx_head_butt1");
    sPlayerSndfx.head_id[i++] = xStrHash("sndfx_head_butt2");
    sPlayerSndfx.head_id[i++] = xStrHash("sndfx_head_butt3");
    sPlayerSndfx.head_pitch = 359.0f * iurand();
    sPlayerSndfx.head_snd = rand() % 3;
    sPlayerSndfx.buttmiss_id = xStrHash("sndfx_butt_miss1");
    i = 0;
    sPlayerSndfx.skid_id[i++] = xStrHash("sndfx_skid1");
    sPlayerSndfx.skid_id[i++] = xStrHash("sndfx_skid2");
    sPlayerSndfx.skid_id[i++] = xStrHash("sndfx_skid3");
    sPlayerSndfx.ammo_pitch = 359.0f * iurand();
    sPlayerSndfx.special_pitch = 359.0f * iurand();
    sPlayerSndfx.token_pitch = 359.0f * iurand();
    sPlayerSndfx.save_pitch = 359.0f * iurand();
    sPlayerSndfx.key_pitch = 359.0f * iurand();
    sPlayerSndfx.info_pitch = 359.0f * iurand();
    sPlayerSndfx.error_pitch = 359.0f * iurand();
    sPlayerSndfx.bite_pitch = 359.0f * iurand();
    sPlayerSndfx.snackcounter_pitch = 359.0f * iurand();
    sPlayerSndfx.ammo_id = xStrHash("sndfx_pu_ammo");
    sPlayerSndfx.special_id = xStrHash("sndfx_pu_special");
    sPlayerSndfx.token_id = xStrHash("sndfx_pu_token");
    sPlayerSndfx.save_id = xStrHash("sndfx_pu_save");
    sPlayerSndfx.info_id = xStrHash("sndfx_pu_info");
    sPlayerSndfx.key_id = xStrHash("sndfx_pu_key");
    sPlayerSndfx.error_id = xStrHash("sndfx_pu_error");
    sPlayerSndfx.bite_id = xStrHash("sndfx_pu_bite");
    sPlayerSndfx.snackcounter_id = xStrHash("sndfx_pu_counter");
    sPlayerSndfx.van_id = xStrHash("sndfx_pu_van");
    i = 0;
    sPlayerSndfx.dig_id[i++] = xStrHash("scooby_one_30");
    sPlayerSndfx.dig_id[i++] = xStrHash("scooby_one_29");
    i = 0;
    sPlayerSndfx.scare_id[i++] = xStrHash("Scooby_Help");
    sPlayerSndfx.scare_id[i++] = xStrHash("Scooby_Oh_No");
    i = 0;
    sPlayerSndfx.danger_id[i++] = xStrHash("scooby_gulp_01");
    sPlayerSndfx.danger_id[i++] = xStrHash("scooby_gulp_02");
    sPlayerSndfx.danger_id[i++] = xStrHash("scooby_gulp_03");
    sPlayerSndfx.danger_id[i++] = xStrHash("scooby_gulp_04");
    sPlayerSndfx.danger_id[i++] = xStrHash("scooby_gulp_05");
    sPlayerSndfx.danger_id[i++] = xStrHash("scooby_gulp_06");
    sPlayerSndfx.danger_id[i++] = xStrHash("scooby_gulp_07");
    sPlayerSndfx.danger_id[i++] = xStrHash("scooby_gulp_08");
    sPlayerSndfx.danger_id[i++] = xStrHash("scooby_one_26");
    i = 0;
    sPlayerSndfx.leap_id[i++] = xStrHash("scooby_excite_09");
    sPlayerSndfx.leap_id[i++] = xStrHash("scooby_excite_10");
    sPlayerSndfx.leap_id[i++] = xStrHash("scooby_bounce_01");
    sPlayerSndfx.leap_id[i++] = xStrHash("scooby_bounce_02");
    sPlayerSndfx.leap_id[i++] = xStrHash("scooby_bounce_03");
    sPlayerSndfx.leap_id[i++] = xStrHash("scooby_bounce_09");
    sPlayerSndfx.leap_id[i++] = xStrHash("scooby_bounce_10");
    sPlayerSndfx.leap_id[i++] = xStrHash("scooby_bounce_15");
    sPlayerSndfx.leap_id[i++] = xStrHash("scooby_bounce_16");
    sPlayerSndfx.leap_id[i++] = xStrHash("scooby_bounce_18");
    sPlayerSndfx.leap_id[i++] = xStrHash("scooby_bounce_19");
    sPlayerSndfx.leap_id[i++] = xStrHash("scooby_bounce_20");
    for (signed int n = 0; n < 13; n++)
    {
        sprintf(name, "scooby_excite_%02d", n + 1);
        sPlayerSndfx.excite_id[n] = xStrHash(name);
    }
    sPlayerSndfx.see_geronimo_id[0] = xStrHash("scooby_one_42b");
    i = 0;
    sPlayerSndfx.see_shaggy_id[i++] = xStrHash("scooby_one_19a");
    sPlayerSndfx.see_shaggy_id[i++] = xStrHash("scooby_one_19b");
    sPlayerSndfx.see_shaggy_id[i++] = xStrHash("scooby_one_20a");
    sPlayerSndfx.see_shaggy_id[i++] = xStrHash("scooby_one_20b");
    sPlayerSndfx.see_shaggy_id[i++] = xStrHash("scooby_one_20c");
    i = 0;
    sPlayerSndfx.see_monster_id[i++] = xStrHash("scooby_one_53c");
    sPlayerSndfx.see_monster_id[i++] = xStrHash("scooby_one_21a");
    sPlayerSndfx.see_monster_id[i++] = xStrHash("scooby_one_21b");
    sPlayerSndfx.see_monster_id[i++] = xStrHash("scooby_one_22a");
    sPlayerSndfx.see_monster_id[i++] = xStrHash("scooby_one_22b");
    i = 0;
    sPlayerSndfx.see_holly_id[i++] = xStrHash("scooby_holly1a");
    sPlayerSndfx.see_holly_id[i++] = xStrHash("scooby_holly1b");
    sPlayerSndfx.see_holly_id[i++] = xStrHash("scooby_holly2");
    sPlayerSndfx.see_groundskeeper_id[0] = xStrHash("scooby_holly1a");
    sPlayerSndfx.see_warpgate_id[0] = xStrHash("scooby_one_25");
    i = 0;
    sPlayerSndfx.need_more_snacks_id[i++] = xStrHash("scooby_one_23a");
    sPlayerSndfx.need_more_snacks_id[i++] = xStrHash("scooby_one_23b");
    sPlayerSndfx.need_more_snacks_id[i++] = xStrHash("scooby_one_23c");
    sPlayerSndfx.success_id[0] = xStrHash("scooby_one_07");
    i = 0;
    sPlayerSndfx.see_token_id[i++] = xStrHash("scooby_one_24a");
    sPlayerSndfx.see_token_id[i++] = xStrHash("scooby_one_24b");
    sPlayerSndfx.see_token_id[i++] = xStrHash("scooby_one_24c");
    i = 0;
    sPlayerSndfx.see_crab_id[i++] = xStrHash("scooby_one_53c");
    sPlayerSndfx.see_crab_id[i++] = xStrHash("scooby_one_31");
    i = 0;
    sPlayerSndfx.see_rat_id[i++] = xStrHash("scooby_one_53c");
    sPlayerSndfx.see_rat_id[i++] = xStrHash("scooby_one_32");
    sPlayerSndfx.see_rat_id[i++] = xStrHash("scooby_one_32b");
    sPlayerSndfx.see_rat_id[i++] = xStrHash("scooby_one_32c");
    i = 0;
    sPlayerSndfx.see_gargoyle_id[i++] = xStrHash("scooby_one_53c");
    sPlayerSndfx.see_gargoyle_id[i++] = xStrHash("scooby_one_41");
    i = 0;
    sPlayerSndfx.see_creeper_id[i++] = xStrHash("scooby_one_53c");
    sPlayerSndfx.see_creeper_id[i++] = xStrHash("scooby_one_44");
    i = 0;
    sPlayerSndfx.see_ghost_diver_id[i++] = xStrHash("scooby_one_53c");
    sPlayerSndfx.see_ghost_diver_id[i++] = xStrHash("scooby_one_45");
    i = 0;
    sPlayerSndfx.see_sea_creature_id[i++] = xStrHash("scooby_one_53c");
    sPlayerSndfx.see_sea_creature_id[i++] = xStrHash("scooby_one_46");
    i = 0;
    sPlayerSndfx.see_headless_specter_id[i++] = xStrHash("scooby_one_53c");
    sPlayerSndfx.see_headless_specter_id[i++] = xStrHash("scooby_one_47");
    i = 0;
    sPlayerSndfx.see_scarecrow_id[i++] = xStrHash("scooby_one_53c");
    sPlayerSndfx.see_scarecrow_id[i++] = xStrHash("scooby_one_49");
    sPlayerSndfx.see_scarecrow_id[i++] = xStrHash("scooby_one_49b");
    i = 0;
    sPlayerSndfx.see_space_kook_id[i++] = xStrHash("scooby_one_53c");
    sPlayerSndfx.see_space_kook_id[i++] = xStrHash("scooby_one_50");
    i = 0;
    sPlayerSndfx.see_tar_monster_id[i++] = xStrHash("scooby_one_53c");
    sPlayerSndfx.see_tar_monster_id[i++] = xStrHash("scooby_one_51");
    sPlayerSndfx.see_tar_monster_id[i++] = xStrHash("scooby_one_51b");
    sPlayerSndfx.see_tar_monster_id[i++] = xStrHash("scooby_one_51c");
    i = 0;
    sPlayerSndfx.see_witch_id[i++] = xStrHash("scooby_one_53c");
    sPlayerSndfx.see_witch_id[i++] = xStrHash("scooby_one_52");
    i = 0;
    sPlayerSndfx.see_robot_id[i++] = xStrHash("scooby_one_53c");
    sPlayerSndfx.see_robot_id[i++] = xStrHash("scooby_one_48");
    i = 0;
    sPlayerSndfx.see_wolfman_id[i++] = xStrHash("scooby_one_53c");
    sPlayerSndfx.see_wolfman_id[i++] = xStrHash("scooby_one_53");
    i = 0;
    sPlayerSndfx.see_ghost_id[i++] = xStrHash("scooby_one_53c");
    sPlayerSndfx.see_ghost_id[i++] = xStrHash("scooby_one_57a");
    sPlayerSndfx.see_ghost_id[i++] = xStrHash("scooby_one_57b");
    sPlayerSndfx.see_ghost_id[i++] = xStrHash("scooby_one_57c");
    i = 0;
    sPlayerSndfx.see_zombie_id[i++] = xStrHash("scooby_one_53c");
    sPlayerSndfx.see_zombie_id[i++] = xStrHash("scooby_one_54");
    i = 0;
    sPlayerSndfx.sorry_id[i++] = xStrHash("scooby_one_14a");
    sPlayerSndfx.sorry_id[i++] = xStrHash("scooby_one_14b");
    sPlayerSndfx.sorry_id[i++] = xStrHash("scooby_one_15a");
    sPlayerSndfx.sorry_id[i++] = xStrHash("scooby_one_14b");
    sPlayerSndfx.collect_all_snacks_id[0] = xStrHash("scooby_one_07");
    i = 0;
    sPlayerSndfx.eat_sandwich_id[i++] = xStrHash("scooby_one_09");
    sPlayerSndfx.eat_sandwich_id[i++] = xStrHash("scooby_one_06b");
    i = 0;
    sPlayerSndfx.eat_turkey_id[i++] = xStrHash("scooby_one_10");
    sPlayerSndfx.eat_turkey_id[i++] = xStrHash("scooby_one_06b");
    i = 0;
    sPlayerSndfx.eat_cake_id[i++] = xStrHash("scooby_one_11");
    sPlayerSndfx.eat_cake_id[i++] = xStrHash("scooby_one_06b");
    i = 0;
    sPlayerSndfx.eat_snacks_id[i++] = xStrHash("scooby_one_03a");
    sPlayerSndfx.eat_snacks_id[i++] = xStrHash("scooby_one_03b");
    sPlayerSndfx.eat_snacks_id[i++] = xStrHash("scooby_one_03c");
}

void zEntPlayer_SNDBitePickup(_xEnt*)
{
    float pitch;
    sPlayerSndfx.bite_pitch = zPitchChange(sPlayerSndfx.bite_pitch, &pitch, 0x1E, 0.25f);
    xSndPlay(sPlayerSndfx.bite_id, 0.5f, 0.5f, pitch, 0x80, 2, 0);
}

void zEntPlayer_SNDAmmoPickup(_xEnt*)
{
    float pitch;
    sPlayerSndfx.ammo_pitch = zPitchChange(sPlayerSndfx.ammo_pitch, &pitch, 0x1E, 0.25f);
    xSndPlay(sPlayerSndfx.ammo_id, 0.5f, 0.5f, pitch, 0x80, 2, 0);
}

void zEntPlayer_SNDSpecialPickup(_xEnt*)
{
    float pitch;
    sPlayerSndfx.special_pitch = zPitchChange(sPlayerSndfx.special_pitch, &pitch, 0x1E, 0.25f);
    xSndPlay(sPlayerSndfx.special_id, 0.5f, 0.5f, pitch, 0x80, 2, 0);
}

void zEntPlayer_SNDTokenPickup(_xEnt*)
{
    float pitch;
    sPlayerSndfx.token_pitch = zPitchChange(sPlayerSndfx.token_pitch, &pitch, 0x1E, 0.25f);
    xSndPlay(sPlayerSndfx.token_id, 0.5f, 0.5f, pitch, 0x80, 2, 0);
}

void zEntPlayer_SNDSavePickup(_xEnt*)
{
    float pitch;
    sPlayerSndfx.save_pitch = zPitchChange(sPlayerSndfx.save_pitch, &pitch, 0x1E, 0.25f);
    xSndPlay(sPlayerSndfx.save_id, 0.5f, 0.5f, pitch, 0x80, 2, 0);
}

void zEntPlayer_SNDInfoPickup(_xEnt*)
{
    float pitch;
    sPlayerSndfx.info_pitch = zPitchChange(sPlayerSndfx.info_pitch, &pitch, 0x1E, 0.25f);
    xSndPlay(sPlayerSndfx.info_id, 0.5f, 0.5f, pitch, 0x80, 2, 0);
}

void zEntPlayer_SNDErrorPickup(_xEnt*)
{
    float pitch;
    sPlayerSndfx.error_pitch = zPitchChange(sPlayerSndfx.error_pitch, &pitch, 0x1E, 0.25f);
    xSndPlay(sPlayerSndfx.error_id, 0.5f, 0.5f, pitch, 0x80, 2, 0);
}

void zEntPlayer_SNDKeyPickup(_xEnt*)
{
    float pitch;
    sPlayerSndfx.key_pitch = zPitchChange(sPlayerSndfx.key_pitch, &pitch, 0x1E, 0.25f);
    xSndPlay(sPlayerSndfx.key_id, 0.5f, 0.5f, pitch, 0x80, 2, 0);
}

void zEntPlayer_SNDSnackCounter(_xEnt*)
{
    xSndPlay(sPlayerSndfx.snackcounter_id, 0.5f, 0.5f, 0.0f, 0x80, 2, 0);
}

void zEntPlayer_SNDVan(_xEnt*)
{
    xSndPlay(sPlayerSndfx.van_id, 0.5f, 0.5f, 0.0f, 0x80, 2, 0);
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
        if (globals.player.s.TakeDamage)
        {
            gPlayerWaterPos = *center;
            sPlayerWaterOffset = 0.0f;
            gPlayerHitWater = 1;
        }
    }

    if (sd_surface == 0x15 && globals.player.s.TakeDamage)
    {
        gPlayerWaterPos = *xBoundCenter(&ent->bound);
        sPlayerWaterOffset = 0.0f;
        gPlayerHitWater = 1;
    }
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

#pragma auto_inline on
void zEntPlayer_FXSetup(xAnimTable* table)
{
    xAnimStateNewEffect(xAnimTableGetState(table, "Inactive_SitFlea"), 1, 0.05f, 0.0f, zEntPlayer_FXScratchCB, 0);
    gPlayerfx.timer_scratch = 0.0f;
    xAnimStateNewEffect(xAnimTableGetState(table, "Fall_floatopen"), 1, 0.05f, 0.0f, zEntPlayer_FXOpenBrollyCB, 0);
    xAnimStateNewEffect(xAnimTableGetState(table, "Action_dig"), 1, 0.05f, 0.0f, zEntPlayer_FXDigupCB, 0);
    gPlayerfx.timer_digup = 0.0f;
    xAnimStateNewEffect(xAnimTableGetState(table, "Jump_double"), 1, 0.0f, 0.0f, zEntPlayer_FXSpringCB, 0);
    xAnimStateNewEffect(xAnimTableGetState(table, "RunToIdle_normal"), 1, 0.05f, 0.0f, zEntPlayer_SNDSkidCB, 0);
    xAnimStateNewEffect(xAnimTableGetState(table, "RunToIdle_scared"), 1, 0.05f, 0.0f, zEntPlayer_SNDSkidCB, 0);
    xAnimStateNewEffect(xAnimTableGetState(table, "RunToIdle_brave"), 1, 0.05f, 0.0f, zEntPlayer_SNDSkidCB, 0);
    xAnimStateNewEffect(xAnimTableGetState(table, "RunToSkid"), 1, 0.05f, 0.0f, zEntPlayer_SNDSkidCB, 0);
}
#pragma auto_inline reset

static unsigned int zEntPlayer_FXScratchCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*)
{
    gPlayerfx.timer_scratch = 1.65f;
    zRumbleStart(0, (_tagSDRumbleType)5);
    return 0;
}

static unsigned int zEntPlayer_FXDigupCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*)
{
    gPlayerfx.timer_digup = 0.3f;
    zRumbleStart(0, (_tagSDRumbleType)0);
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

static unsigned int zEntPlayer_FXOpenBrollyCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*)
{
    return 0;
}

static unsigned int zEntPlayer_SNDSkidCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*)
{
    static signed int snd = 0.0f;
    static signed short pitch_angle = 0.0f;
    float pitch;
    signed int newsnd;

    pitch_angle = zPitchChange(pitch_angle, &pitch, 0x1E, 1.5f);
    xSndPlay(sPlayerSndfx.skid_id[snd], 0.5f, 0.5f, pitch, 0x80, 2, 0);
    newsnd = snd;
    while (newsnd == snd)
    {
        newsnd = rand() % 3;
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

static RpAtomic* sPlayerBodyAtomic;
static RpAtomic* sPlayerHeadAtomic;
static RpClump* sPlayerBodyClump;
static RpClump* sPlayerHeadClump;

#pragma auto_inline on
void zEntPlayer_ButtonBBFX(_xEnt* ent)
{
    if (sEmitterBounceOnVillain != NULL)
    {
        _zParEmitterCustomSettings info;
        info.custom_flags = 0x20;
        _xMat4x3* mat = (_xMat4x3*)ent->model->Mat;
        info.pos.x = mat->pos.x;
        float y = mat->pos.y - 1.0f;
        info.pos.y = y;
        info.pos.z = mat->pos.z;
        switch (ent->bound.type)
        {
        case 4:
        case 0:
        case 1:
            info.pos = ent->bound.sph.center;
            break;
        case 2:
            info.pos.y = y + (ent->bound.box.box.upper.y - ent->bound.box.box.lower.y);
            break;
        case 3:
            info.pos.y = y + ent->bound.cyl.h;
            break;
        }
        zParEmitterEmitCustom(sEmitterBounceOnVillain, 1.0f / 30.0f, &info);
    }
}
#pragma auto_inline reset

#pragma auto_inline on
void zEntPlayer_ButtonHBFX()
{
    if (sEmitterHeadButt != NULL && sEmitterHeadButt2 != NULL)
    {
        _zParEmitterCustomSettings info;
        _xVec3 angles;
        _xMat4x3* mat = (_xMat4x3*)globals.player.ent.model->Mat;
        info.custom_flags = 0x20;
        info.pos.x = mat->pos.x + mat->at.x;
        info.pos.y = 1.1f + mat->pos.y;
        info.custom_flags = 0x10020;
        info.pos.z = mat->pos.z + mat->at.z;
        xMat3x3GetEuler((_xMat3x3*)mat, &angles);
        info.rot[1] = 0x40;
        info.rot[0] = 255.0f * (angles.x / 6.2831855f);
        info.rot[2] = 255.0f * (angles.z / 6.2831855f);
        zParEmitterEmitCustom(sEmitterHeadButt, 1.0f / 30.0f, &info);
        info.rot[0] += 0x40;
        zParEmitterEmitCustom(sEmitterHeadButt2, 1.0f / 30.0f, &info);
    }
}
#pragma auto_inline reset


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

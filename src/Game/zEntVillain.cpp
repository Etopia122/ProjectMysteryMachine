// Lets VILN_holoDeath (with the hologram emitter inlined into it) be inlined
// into the aware handlers, as in the original.
#pragma inline_max_size(1000)

#include "zGlobals.h"
#include "zScene.h"
#include "zParEmitter.h"
#include "zSurface.h"
#include "zEntProjectile.h"
#include "xMath.h"
#include "xMath3.h"
#include "xString.h"
#include "xstransvc.h"
#include "xAnim.h"
#include "iModel.h"
#include "xCollide.h"
#include "xSnd.h"
#include "xFFX.h"
#include "xordarray.h"
#include "rwtypes.h"
#include "iMath.h"
#include "iSnd.h"
#include "xMemMgr.h"
#include "iDraw.h"
#include "xMovePoint.h"
#include "xserializer.h"
#include "zEnt.h"

#ifndef NULL
#define NULL 0
#endif

class RwMatrixTag;
class xAnimActiveEffect;
class xAnimEffect;
class xAnimTransitionList;
class _zEntBubble;
class _zEntPickup;
class _zMovePoint;
class _tagLightning;
class zSFX;
class st_ZVIL_PRECALC;
class st_NPC_SOUND_QUEUE;
class st_ZVIL_CONFIG_REFERENCE;
class st_STIMULUS_CONTEXT_DATA;
class st_ZVIL_NPC_HANDLERFUNCS;
class st_ZVIL_NPCTYPE_LOOKUP;
class st_NPCTYPE_CONFIG;
class st_NPC_BONUS;
class st_NPC_TALLY_RECORD;
class st_zEntVillain_ParEmitRecord;
class st_zEntVillain_ParEmitInfo;
class st_BKAXE_DATA;
class st_BGG_QUAKEBALL;
class st_REDBEARD_SWORD;
class st_ZVMM_FLASK_DATA;
class st_NPC_SOUND_PROPERTIES;
class st_NPC_SOUND_RECORDS;
class st_NPC_SOUND_ENTRY;
class st_zEntVillain;
class zAnimFxSound;
class p2EntVillainAsset;
class zAssetPickup;
class xPar;
class xMemPool;
class xAnimPlay;
class xAnimTable;

enum _tagLightningType
{
    eLT_Line = 0,
    eLT_Rotating = 1,
    eLT_Total = 2,
};

class xAnimFile
{
public:
    xAnimFile* Next; // 0x0
    char* Name; // 0x4
    unsigned int ID; // 0x8
    unsigned int FileFlags; // 0xC
    float Duration; // 0x10
    float TimeOffset; // 0x14
    float PhysBase[4]; // 0x18
    float PhysDelta[4]; // 0x28
    unsigned short BoneCount; // 0x38
    unsigned char NumAnims[2]; // 0x3A
    void** RawData; // 0x3C
    void* Physics; // 0x40
};

class xAnimState
{
public:
    xAnimState* Next; // 0x0
    char* Name; // 0x4
    unsigned int ID; // 0x8
    unsigned int Flags; // 0xC
    unsigned int UserFlags; // 0x10
    float Speed; // 0x14
    xAnimFile* Data; // 0x18
    xAnimEffect* Effects; // 0x1C
    xAnimTransitionList* Default; // 0x20
    xAnimTransitionList* List; // 0x24
    float* BoneBlend; // 0x28
    float* TimeSnap; // 0x2C
    float FadeRecip; // 0x30
    unsigned short* FadeOffset; // 0x34
};

class xAnimTransition
{
public:
    xAnimTransition* Next; // 0x0
    xAnimState* Dest; // 0x4
    unsigned int (*Conditional)(xAnimTransition*, xAnimSingle*, void*); // 0x8
    unsigned int (*Callback)(xAnimTransition*, xAnimSingle*, void*); // 0xC
    unsigned int Flags; // 0x10
    unsigned int UserFlags; // 0x14
    float SrcTime; // 0x18
    float DestTime; // 0x1C
    unsigned short Priority; // 0x20
    unsigned short QueuePriority; // 0x22
    float BlendRecip; // 0x24
    unsigned short* BlendOffset; // 0x28
};

class xAnimTransitionList
{
public:
    xAnimTransitionList* Next; // 0x0
    xAnimTransition* T; // 0x4
};

class xAnimSingle
{
public:
    unsigned int SingleFlags; // 0x0
    xAnimState* State; // 0x4
    float Time; // 0x8
    float CurrentSpeed; // 0xC
    float BilinearLerp[2]; // 0x10
    xAnimEffect* Effect; // 0x18
    unsigned int ActiveCount; // 0x1C
    xAnimActiveEffect* ActiveList; // 0x20
    xAnimPlay* Play; // 0x24
    float LastTime; // 0x28
    _xVec3 LastTranslation; // 0x2C
    float LastYaw; // 0x38
    unsigned int pad; // 0x3C
    _xQuat LastQuat; // 0x40
    xAnimTransition* Sync; // 0x50
    xAnimTransition* Tran; // 0x54
    xAnimSingle* Blend; // 0x58
    float BlendFactor; // 0x5C
};

class xAnimTable
{
public:
    xAnimTable* Next; // 0x0
    char* Name; // 0x4
    xAnimTransition* TransitionList; // 0x8
    xAnimState* StateList; // 0xC
    unsigned int AnimIndex; // 0x10
    unsigned int MorphIndex; // 0x14
};

class xAnimPlay
{
public:
    xAnimPlay* Next; // 0x0
    unsigned short NumSingle; // 0x4
    unsigned short BoneCount; // 0x6
    xAnimSingle* Single; // 0x8
    void* Object; // 0xC
    xAnimTable* Table; // 0x10
    _xVec3 Translate[2]; // 0x14
    unsigned int pad1; // 0x2C
    _xQuat Quat[2]; // 0x30
    float Yaw; // 0x50
    xMemPool* Pool; // 0x54
    xModelInstance* ModelInst; // 0x58
    unsigned int pad2; // 0x5C
};

class _zEntPickup : public _zEnt
{
public:
    unsigned int state; // 0xEC
    zAssetPickup* p; // 0xF0
    void* anim; // 0xF4
    float animTime; // 0xF8
    float timer; // 0xFC
    float shake_timer; // 0x100
    _xVec3 shake_pos; // 0x104
    signed int snackGateInfoDisplayed; // 0x110
    float fx_timer; // 0x114
    xPar* fx_par; // 0x118
    _zParEmitter* fx_emit; // 0x11C
    float fx_scale; // 0x120
    _xVec3 grab_pos; // 0x124
    unsigned int flyflags; // 0x130
    signed int flg_opts; // 0x134
    _xVec3 vel; // 0x138
    _xVec3 droppos; // 0x144
    xBase* followTarget; // 0x150
    _xVec3 followOffset; // 0x154
    xBase* dropParent; // 0x160
    _xEnt* useThisEntPos; // 0x164
};

class _tagLightningAdd
{
public:
    _tagLightningType type; // 0x0
    float setup_degrees; // 0x4
    float move_degrees; // 0x8
    float rot_radius; // 0xC
    _xVec3* start; // 0x10
    _xVec3* end; // 0x14
    signed int total_points; // 0x18
    float time; // 0x1C
    float arc_height; // 0x20
    float thickness; // 0x24
    unsigned char r; // 0x28
    unsigned char g; // 0x29
    unsigned char b; // 0x2A
    unsigned char alpha_pad; // 0x2B
    float rand_radius; // 0x2C
    unsigned int flags; // 0x30
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

class tagzFeet
{
public:
    signed int total;          // 0x0
    _xEnt* owner;              // 0x4
    signed short entity_type;  // 0x8
    signed short pitch;        // 0xA
};

enum en_NPC_ANIMSTATES
{
    ZVIL_ANIM_UNKNOWN = 0,
    ZVIL_ANIM_IDLE = 1,
    ZVIL_ANIM_FIDGET_IDLE1 = 2,
    ZVIL_ANIM_FIDGET_IDLE2 = 3,
    ZVIL_ANIM_BORED = 4,
    ZVIL_ANIM_FIDGET_BORED1 = 5,
    ZVIL_ANIM_FIDGET_BORED2 = 6,
    ZVIL_ANIM_MOVE = 7,
    ZVIL_ANIM_POSE = 8,
    ZVIL_ANIM_TURN = 9,
    ZVIL_ANIM_CHASE = 10,
    ZVIL_ANIM_ACTION_NOMOVE = 11,
    ZVIL_ANIM_ACTION_MOVING = 12,
    ZVIL_ANIM_SURPRIZE = 13,
    ZVIL_ANIM_TAUNT = 14,
    ZVIL_ANIM_TAUNT1 = 15,
    ZVIL_ANIM_TAUNT2 = 16,
    ZVIL_ANIM_HEAR_START = 17,
    ZVIL_ANIM_HEAR_LOOP = 18,
    ZVIL_ANIM_LOOK_START = 19,
    ZVIL_ANIM_LOOK_LOOP = 20,
    ZVIL_ANIM_INVESTIGATE_START = 21,
    ZVIL_ANIM_INVESTIGATE = 22,
    ZVIL_ANIM_ATTACKIDLE = 23,
    ZVIL_ANIM_ATTACK = 24,
    ZVIL_ANIM_ATTACK1 = 25,
    ZVIL_ANIM_ATTACK2 = 26,
    ZVIL_ANIM_COMBO = 27,
    ZVIL_ANIM_COMBO1 = 28,
    ZVIL_ANIM_COMBO2 = 29,
    ZVIL_ANIM_LOB_ATTACK = 30,
    ZVIL_ANIM_LOB_COMBO = 31,
    ZVIL_ANIM_EXT_ATTACK_START = 32,
    ZVIL_ANIM_EXT_ATTACK_LOOP = 33,
    ZVIL_ANIM_EXT_COMBO = 34,
    ZVIL_ANIM_HIT_BY_GUM = 35,
    ZVIL_ANIM_GUMMED = 36,
    ZVIL_ANIM_GUMMED_RESIST = 37,
    ZVIL_ANIM_GUMMED_BREAKFREE = 38,
    ZVIL_ANIM_HIT_WHEN_GUMMED = 39,
    ZVIL_ANIM_BOUNCED_WHEN_GUMMED = 40,
    ZVIL_ANIM_HIT_BY_SOAP = 41,
    ZVIL_ANIM_SOAPED = 42,
    ZVIL_ANIM_SOAPED_RESIST = 43,
    ZVIL_ANIM_SOAPED_BREAKFREE = 44,
    ZVIL_ANIM_HIT_WHEN_SOAPED = 45,
    ZVIL_ANIM_BOUNCED_WHEN_SOAPED = 46,
    ZVIL_ANIM_DAZED_BEGIN = 47,
    ZVIL_ANIM_DAZED = 48,
    ZVIL_ANIM_DAZED_RECOVER = 49,
    ZVIL_ANIM_DEATH = 50,
    ZVIL_ANIM_DEATH_ABOVE = 51,
    ZVIL_ANIM_DEATH_DAZED = 52,
    ZVIL_ANIM_DEATH_IDLE = 53,
    ZVIL_ANIM_JUMP_BEGIN = 54,
    ZVIL_ANIM_JUMP_LOOP = 55,
    ZVIL_ANIM_JUMP_LAND = 56,
    ZVIL_ANIM_FALL_LOOP = 57,
    ZVIL_ANIM_FALL_SPLAT = 58,
    ZVIL_ANIM_BK_SCARYIDLE = 59,
    ZVIL_ANIM_BK_WALK = 60,
    ZVIL_ANIM_BK_TURN = 61,
    ZVIL_ANIM_BK_PATROL = 62,
    ZVIL_ANIM_BK_PATROLFLIP = 63,
    ZVIL_ANIM_BK_ATTACK = 64,
    ZVIL_ANIM_BK_SHOCK = 65,
    ZVIL_ANIM_BK_SHOCKSKEL = 66,
    ZVIL_ANIM_BK_GLOAT = 67,
    ZVIL_ANIM_BK_GLOAT1 = 68,
    ZVIL_ANIM_BK_GLOAT2 = 69,
    ZVIL_ANIM_BK_DEATH = 70,
    ZVIL_ANIM_GG_CHASE = 71,
    ZVIL_ANIM_GG_CHASELEFT = 72,
    ZVIL_ANIM_GG_CHASERIGHT = 73,
    ZVIL_ANIM_GG_BUILDBEGIN = 74,
    ZVIL_ANIM_GG_BUILDLOOP = 75,
    ZVIL_ANIM_GG_ATTACK = 76,
    ZVIL_ANIM_GG_TAUNT = 77,
    ZVIL_ANIM_GG_TAUNT1 = 78,
    ZVIL_ANIM_GG_HURT = 79,
    ZVIL_ANIM_GG_HURT1 = 80,
    ZVIL_ANIM_GG_HURT2 = 81,
    ZVIL_ANIM_RB_IDLEWIND = 82,
    ZVIL_ANIM_RB_PATROL = 83,
    ZVIL_ANIM_RB_TURN = 84,
    ZVIL_ANIM_RB_CHASE = 85,
    ZVIL_ANIM_RB_TAUNTFEEL = 86,
    ZVIL_ANIM_RB_TAUNTTHROAT = 87,
    ZVIL_ANIM_RB_TAUNTLAUGH = 88,
    ZVIL_ANIM_RB_ATTACKPUNCH = 89,
    ZVIL_ANIM_RB_ATTACKTHROW = 90,
    ZVIL_ANIM_RB_ATTACKWAIT = 91,
    ZVIL_ANIM_RB_ATTACKCATCH = 92,
    ZVIL_ANIM_RB_REACT = 93,
    ZVIL_ANIM_RB_LOOK_BEGIN = 94,
    ZVIL_ANIM_RB_LOOK_LOOP = 95,
    ZVIL_ANIM_RB_LOOK_END = 96,
    ZVIL_ANIM_RB_DEATH_LOOP = 97,
    ZVIL_ANIM_MM_FP_MOVE = 98,
    ZVIL_ANIM_MM_FP_TAUNT = 99,
    ZVIL_ANIM_MM_FP_ACTION = 100,
    ZVIL_ANIM_MM_FP_JUMP = 101,
    ZVIL_ANIM_MM_BA_IDLE = 102,
    ZVIL_ANIM_MM_BA_MOVE = 103,
    ZVIL_ANIM_MM_BA_TURN = 104,
    ZVIL_ANIM_MM_BA_BACKSTEP = 105,
    ZVIL_ANIM_MM_BA_TAUNT = 106,
    ZVIL_ANIM_MM_BA_REACT = 107,
    ZVIL_ANIM_MM_BA_ATTACK_BEGIN = 108,
    ZVIL_ANIM_MM_BA_ATTACK_LOOP = 109,
    ZVIL_ANIM_MM_BA_ATTACK = 110,
    ZVIL_ANIM_MM_BA_SHIELD_BEGIN = 111,
    ZVIL_ANIM_MM_BA_SHIELD_LOOP = 112,
    ZVIL_ANIM_MM_BA_SHIELD_END = 113,
    ZVIL_ANIM_MM_BA_STUNNED_BEGIN = 114,
    ZVIL_ANIM_MM_BA_STUNNED_LOOP = 115,
    ZVIL_ANIM_MM_BA_STUNNED_END = 116,
    ZVIL_ANIM_MM_BA_KNOCKED_BEGIN = 117,
    ZVIL_ANIM_MM_BA_KNOCKED_LOOP = 118,
    ZVIL_ANIM_MM_BA_KNOCKED_END = 119,
    ZVIL_ANIM_MM_BA_DEATH_BEGIN = 120,
    ZVIL_ANIM_MM_BA_DEATH = 121,
    ZVIL_ANIM_GANG_ALTIDLE1 = 122,
    ZVIL_ANIM_GANG_ALTIDLE2 = 123,
    ZVIL_ANIM_GANG_ALTIDLE3 = 124,
    ZVIL_ANIM_GANG_FIDGET = 125,
    ZVIL_ANIM_GANG_FIDGET1 = 126,
    ZVIL_ANIM_GANG_FIDGET2 = 127,
    ZVIL_ANIM_GANG_FIDGET3 = 128,
    ZVIL_ANIM_GANG_TURN = 129,
    ZVIL_ANIM_GANG_WALK = 130,
    ZVIL_ANIM_GANG_RUN = 131,
    ZVIL_ANIM_GANG_RUNCHASED = 132,
    ZVIL_ANIM_GANG_ACTION = 133,
    ZVIL_ANIM_GANG_BOSSACTION = 134,
    ZVIL_ANIM_GANG_TALK = 135,
    ZVIL_ANIM_GANG_TALK1 = 136,
    ZVIL_ANIM_GANG_TALK2 = 137,
    ZVIL_ANIM_GANG_TALK3 = 138,
    ZVIL_ANIM_GANG_TALKANDTURN = 139,
    ZVIL_ANIM_SHAM_MOVE = 140,
    ZVIL_ANIM_SHAM_TAUNT = 141,
    ZVIL_ANIM_CRIT_MOVE = 142,
    ZVIL_ANIM_CRIT_BATSLEEP = 143,
    ZVIL_ANIM_CRIT_SPIDERDROP = 144,
    ZVIL_ANIM_CRIT_SMASHED = 145,
    ZVIL_ANIM_SIMP_ACTION = 146,
    ZVIL_ANIM_NOMORE = 147,
};

enum en_BEHAVIOUR_GOAL
{
    ZVIL_GOAL_NONE = -1,
    ZVIL_GOAL_AFTERLIFE = 0,
    ZVIL_GOAL_BASEINIT = 1,
    ZVIL_GOAL_STARTUP = 2,
    ZVIL_GOAL_GOONLINE = 3,
    ZVIL_GOAL_IDLE = 4,
    ZVIL_GOAL_BORED = 5,
    ZVIL_GOAL_PATROL = 6,
    ZVIL_GOAL_MINIPATROL = 7,
    ZVIL_GOAL_FOLLOW = 8,
    ZVIL_GOAL_GUARD = 9,
    ZVIL_GOAL_AMBUSH = 10,
    ZVIL_GOAL_LEAP = 11,
    ZVIL_GOAL_TALK = 12,
    ZVIL_GOAL_LOOKFORHERO = 13,
    ZVIL_GOAL_LISTEN = 14,
    ZVIL_GOAL_SURPRIZE = 15,
    ZVIL_GOAL_INVESTIGATE = 16,
    ZVIL_GOAL_ATTACKREADY = 17,
    ZVIL_GOAL_BULLREADY = 18,
    ZVIL_GOAL_CHASE = 19,
    ZVIL_GOAL_CQCATTACK = 20,
    ZVIL_GOAL_LOBBING = 21,
    ZVIL_GOAL_SPELLCAST = 22,
    ZVIL_GOAL_TAUNT = 23,
    ZVIL_GOAL_DAZED = 24,
    ZVIL_GOAL_SOAPED = 25,
    ZVIL_GOAL_GUMMED = 26,
    ZVIL_GOAL_HURT = 27,
    ZVIL_GOAL_BLOCKHEAD = 28,
    ZVIL_GOAL_TELEPORT = 29,
    ZVIL_GOAL_KNOCKBACK = 30,
    ZVIL_GOAL_ACTION = 31,
    ZVIL_GOAL_DIE = 32,
    ZVIL_GOAL_NOMORE = 33,
};

enum en_NPC_MODE
{
    ZVIL_MODE_NOCHANGE = 0,
    ZVIL_MODE_SHUTDOWN = 1,
    ZVIL_MODE_READY = 2,
    ZVIL_MODE_NORMAL = 3,
    ZVIL_MODE_ALERT = 4,
    ZVIL_MODE_BATTLE = 5,
    ZVIL_MODE_RETREAT = 6,
    ZVIL_MODE_DEATH = 7,
    ZVIL_MODE_NOMORE = 8,
};

enum en_zEntVillainSubTypeID
{
    eEntVillainType_Undefined = -1,
    eEntVillainTypeBat = 0,
    eEntVillainTypeBlackKnight = 1,
    eEntVillainTypeBuckMasters = 2,
    eEntVillainTypeCreeper = 3,
    eEntVillainTypeSeaCreature = 4,
    eEntVillainTypeGargoyle = 5,
    eEntVillainTypeGhost = 6,
    eEntVillainTypeGhostDiver = 7,
    eEntVillainTypeGreenGhost = 8,
    eEntVillainTypeHeadlessSpecter = 9,
    eEntVillainTypePhantom = 10,
    eEntVillainTypeRedbeard = 11,
    eEntVillainTypeRobot = 12,
    eEntVillainTypeScarecrow = 13,
    eEntVillainTypeShark = 14,
    eEntVillainTypeSpaceKook = 15,
    eEntVillainTypeTarMonster = 16,
    eEntVillainTypeWitch = 17,
    eEntVillainTypeWitchDoctor = 18,
    eEntVillainTypeWolfman = 19,
    eEntVillainTypeZombie = 20,
    eEntVillainTypeCrab = 21,
    eEntVillainTypeRat = 22,
    eEntVillainTypeFlyingFish = 23,
    eEntVillainTypeSpider = 24,
    eEntVillainTypeHand = 25,
    eEntVillainTypeKillerPlant = 26,
    eEntVillainTypeBird = 27,
    eEntVillainTypeShaggy0 = 28,
    eEntVillainTypeShaggy1 = 29,
    eEntVillainTypeShaggy2 = 30,
    eEntVillainTypeShaggy3 = 31,
    eEntVillainTypeShaggy4 = 32,
    eEntVillainTypeShaggy5 = 33,
    eEntVillainTypeShaggy6 = 34,
    eEntVillainTypeShaggy7 = 35,
    eEntVillainTypeShaggy8 = 36,
    eEntVillainTypeShaggy9 = 37,
    eEntVillainTypeFred = 38,
    eEntVillainTypeDaphne = 39,
    eEntVillainTypeVelma = 40,
    eEntVillainBossBlackKnight = 41,
    eEntVillainBossGreenGhost = 42,
    eEntVillainBossRedbeard = 43,
    eEntVillainBossMastermind = 44,
    eEntVillainCaptainMoody = 45,
    eEntVillainCaveMan = 46,
    eEntVillainHolly = 47,
    eEntVillainGroundsKeeper = 48,
    eEntVillainProfessor = 49,
    eEntVillainType_NoMore = 50,
};

enum en_NPC_PERSONALITIES
{
    ZVIL_PERSON_BOGUS = -1,
    ZVIL_PERSON_NORMAL = 0,
    ZVIL_PERSON_WIMP = 1,
    ZVIL_PERSON_WIRED = 2,
    ZVIL_PERSON_BULLY = 3,
    ZVIL_PERSON_LEADER = 4,
    ZVIL_PERSON_AGGRESSIVE = 5,
    ZVIL_PERSON_BOSS = 6,
    ZVIL_PERSON_SCOOBYGANG = 7,
};

enum en_NPC_SOUND
{
    VSND_TYP_BOGUS = -1,
    VSND_TYP_ENCOUNTER = 0,
    VSND_TYP_SPOOKYROOM = 1,
    VSND_TYP_DIALOGUE = 2,
    VSND_TYP_MOVE = 3,
    VSND_TYP_MOVE_ALT = 4,
    VSND_TYP_IDLESLOP = 5,
    VSND_TYP_SNORE = 6,
    VSND_TYP_MUMBLE = 7,
    VSND_TYP_SPLISH = 8,
    VSND_TYP_SPLASH = 9,
    VSND_TYP_CLANKING = 10,
    VSND_TYP_LOOK = 11,
    VSND_TYP_LISTEN = 12,
    VSND_TYP_CURIOUS = 13,
    VSND_TYP_EXCLAIM = 14,
    VSND_TYP_AMBUSH = 15,
    VSND_TYP_ATTACK = 16,
    VSND_TYP_ATTACKSWIPE = 17,
    VSND_TYP_KICK = 18,
    VSND_TYP_PUNCH = 19,
    VSND_TYP_WEPLAUNCH = 20,
    VSND_TYP_WEPFLIGHT = 21,
    VSND_TYP_WEPEXPLODE = 22,
    VSND_TYP_LAUGH = 23,
    VSND_TYP_DIZZY = 24,
    VSND_TYP_SOAPBEGIN = 25,
    VSND_TYP_SOAPSTOP = 26,
    VSND_TYP_SOAPED = 27,
    VSND_TYP_GUMBEGIN = 28,
    VSND_TYP_GUMSTOP = 29,
    VSND_TYP_GUMMED = 30,
    VSND_TYP_GUMBOUNCE = 31,
    VSND_TYP_GUMSTRETCH = 32,
    VSND_TYP_OUCH = 33,
    VSND_TYP_OUCHCRASH = 34,
    VSND_TYP_FALL = 35,
    VSND_TYP_DIE = 36,
    VSND_TYP_HOLOFADE = 37,
    VSND_TYP_PLAYERDEATH = 38,
    VSND_TYP_PRJ_LAUNCH = 39,
    VSND_TYP_PRJ_FLIGHT = 40,
    VSND_TYP_PRJ_EXPLODE = 41,
    VSND_TYP_PRJ_SPLAT = 42,
    VSND_TYP_SPELL_CAST = 43,
    VSND_TYP_SPELL_PLAY = 44,
    VSND_TYP_SPELL_LOOP = 45,
    VSND_TYP_ZSFXTALK = 46,
    VSND_TYP_CHEERING = 47,
    VSND_TYP_ONELINER = 48,
    VSND_TYP_ONELINERTOO = 49,
    VSND_TYP_NEEDTARBOOTS = 50,
    VSND_TYP_EXTRASPRING = 51,
    VSND_TYP_NEEDFLOAT = 52,
    VSND_TYP_HAVEFLOAT = 53,
    VSND_TYP_BKZAPPED = 54,
    VSND_TYP_BKDIE = 55,
    VSND_TYP_BLOCKING = 56,
    VSND_TYP_ESCAPING = 57,
    VSND_TYP_TRASHTALK = 58,
    VSND_TYP_FOREPLAYHURT = 59,
    VSND_TYP_BUTTONPRESS = 60,
    VSND_TYP_ATTACKBUILD = 61,
    VSND_TYP_ALMOSTKILLED = 62,
    VSND_TYP_DISAPPEAR = 63,
    VSND_TYP_REAPPEAR = 64,
    VSND_TYP_MMDIE = 65,
    VSND_TYP_MMLAUGH = 66,
    ZVIL_SND_NOMORE = 67,
    VSND_LISTEND = 32768,
};

enum en_NPC_STIMULUS
{
    ZVIL_STIM_NONE = 0,
    ZVIL_STIM_OFTHEGODS = 1,
    ZVIL_STIM_MAKEREADY = 2,
    ZVIL_STIM_EXITGOAL = 3,
    ZVIL_STIM_ENTERGOAL = 4,
    ZVIL_STIM_TRANGOAL = 5,
    ZVIL_STIM_WAKEUP = 6,
    ZVIL_STIM_GOTOSLEEP = 7,
    ZVIL_STIM_GOTOPOS = 8,
    ZVIL_STIM_GOTOWIDGET = 9,
    ZVIL_STIM_FOLLOWWIDGET = 10,
    ZVIL_STIM_STARTPATROL = 11,
    ZVIL_STIM_STOPPATROL = 12,
    ZVIL_STIM_STARTGUARD = 13,
    ZVIL_STIM_STOPGUARD = 14,
    ZVIL_STIM_TRESPASS = 15,
    ZVIL_STIM_ENDTRESPASS = 16,
    ZVIL_STIM_STARTTALK = 17,
    ZVIL_STIM_STOPTALK = 18,
    ZVIL_STIM_LOOKFORHERO = 19,
    ZVIL_STIM_SAWHERO = 20,
    ZVIL_STIM_HEARDHERO = 21,
    ZVIL_STIM_SHOCKED = 22,
    ZVIL_STIM_ATTACKHERO = 23,
    ZVIL_STIM_DOACTION = 24,
    ZVIL_STIM_DOTAUNT = 25,
    ZVIL_STIM_COLLIDE = 26,
    ZVIL_STIM_HEADBUTT = 27,
    ZVIL_STIM_BOUNCED = 28,
    ZVIL_STIM_BUMPED = 29,
    ZVIL_STIM_STUNNING = 30,
    ZVIL_STIM_KILLED = 31,
    ZVIL_STIM_SOAPED = 32,
    ZVIL_STIM_GUMMED = 33,
    ZVIL_STIM_KILLSELF = 34,
    ZVIL_STIM_GOTOAFTERLIFE = 35,
    ZVIL_STIM_GUARDWIDGET = 36,
    ZVIL_STIM_GANGBOSSACTION = 37,
    ZVIL_STIM_HURTBOSS = 38,
    ZVIL_STIM_MMPUSHBUTTON = 39,
    ZVIL_STIM_NOMORE = 40,
};

enum en_NPC_QUADRANT
{
    ZVIL_QUAD_UNKNOWN = 0,
    ZVIL_QUAD_FRONT = 1,
    ZVIL_QUAD_BACK = 2,
    ZVIL_QUAD_LEFT = 3,
    ZVIL_QUAD_RIGHT = 4,
};

enum en_ZVIL_FLAG_GROUPS
{
    ZVIL_FGRP_PHYS = 0,
    ZVIL_FGRP_CHK = 1,
    ZVIL_FGRP_PEN = 2,
    ZVIL_FGRP_CHKBY = 3,
    ZVIL_FGRP_PENBY = 4,
    ZVIL_FGRP_ABLE = 5,
    ZVIL_FGRP_ATAK = 6,
    ZVIL_FGRP_DTCT = 7,
    ZVIL_FGRP_MOVE = 8,
    ZVIL_FGRP_SPYK = 9,
    ZVIL_FGRP_VULN = 10,
    ZVIL_FGRP_ALOW = 11,
    ZVIL_FGRP_NOMORE = 12,
};

enum en_ZVIL_QUERY
{
    ZVIL_Q_DOUPDATE = 0,
    ZVIL_Q_ALIVE = 1,
    ZVIL_Q_ASLEEP = 2,
    ZVIL_Q_PITFALL = 3,
    ZVIL_Q_HAVEANCHOR = 4,
    ZVIL_Q_HAVEGUARDPOS = 5,
    ZVIL_Q_DOFOLLOW = 6,
    ZVIL_Q_NEEDNAV = 7,
    ZVIL_Q_HAVENAV = 8,
    ZVIL_Q_CANGUARD = 9,
    ZVIL_Q_CANPATROL = 10,
    ZVIL_Q_CANCHASE = 11,
    ZVIL_Q_RUNAWAY = 12,
    ZVIL_Q_CANPURSUE = 13,
    ZVIL_Q_CANATTACK = 14,
    ZVIL_Q_SHOULDATTACK = 15,
    ZVIL_Q_CANDETECT = 16,
    ZVIL_Q_PLYRDETECTED = 17,
    ZVIL_Q_PLYRTOOCLOSE = 18,
    ZVIL_Q_PLYRNEAR = 19,
    ZVIL_Q_PLYRINFOV = 20,
    ZVIL_Q_PLYRINLOS = 21,
    ZVIL_Q_CANSEEPLYR = 22,
    ZVIL_Q_CANHEARPLYR = 23,
    ZVIL_Q_PLYRSEEN = 24,
    ZVIL_Q_PLYRHEARD = 25,
    ZVIL_Q_NOMORE = 26,
};

enum en_ZVIL_NOTICE
{
    ZVIL_NOTE_PREPARE = 0,
    ZVIL_NOTE_PRECALC = 1,
    ZVIL_NOTE_OLDTIME = 2,
    ZVIL_NOTE_NOMORE = 3,
};

enum en_ZVIL_GOAL_SPOT
{
    ZVIL_GSPOT_START = 0,
    ZVIL_GSPOT_RESUME = 1,
    ZVIL_GSPOT_FIDGET = 2,
    ZVIL_GSPOT_ATTACKLINGER = 3,
    ZVIL_GSPOT_KICKATTACK = 4,
    ZVIL_GSPOT_BUBHIT = 5,
    ZVIL_GSPOT_BUBBOUNCE = 6,
    ZVIL_GSPOT_BUBRESIST = 7,
    ZVIL_GSPOT_BUBBREAKFREE = 8,
    ZVIL_GSPOT_STATERECOVER = 9,
    ZVIL_GSPOT_ACTION_ALTA = 10,
    ZVIL_GSPOT_MOVELEFT = 11,
    ZVIL_GSPOT_MOVERIGHT = 12,
    ZVIL_GSPOT_TAUNT_ALTA = 13,
    ZVIL_GSPOT_TAUNT_ALTB = 14,
    ZVIL_GSPOT_HURT_ALTA = 15,
    ZVIL_GSPOT_HURT_ALTB = 16,
};

enum en_VPAR_EMITTER_ROLE
{
    VPAR_EMIT_BOGUS = -1,
    VPAR_EMIT_WALLPRINT = 0,
    VPAR_EMIT_EXTATTACK = 1,
    VPAR_EMIT_SOAPY = 2,
    VPAR_EMIT_POPSOAP = 3,
    VPAR_EMIT_GUMMY = 4,
    VPAR_EMIT_POPGUM = 5,
    VPAR_EMIT_BUTTBOUNCE = 6,
    VPAR_EMIT_HEADBUTT = 7,
    VPAR_EMIT_STUN = 8,
    VPAR_EMIT_DYING = 9,
    VPAR_EMIT_DECAY = 10,
    VPAR_EMIT_POOF = 11,
    VPAR_EMIT_RESPAWN = 12,
    VPAR_EMIT_TELEPORT = 13,
    VPAR_EMIT_BORED = 14,
    VPAR_EMIT_GUARD = 15,
    VPAR_EMIT_QUESTMARK = 16,
    VPAR_EMIT_EXCLAIM = 17,
};

enum en_ZVILLAIN_BOUNCE_RESPONSE
{
    ZVIL_BOUNCE_NONE = 0,
    ZVIL_BOUNCE_NORMAL = 1,
    ZVIL_BOUNCE_GUM = 2,
    ZVIL_BOUNCE_DAMAGE = 3,
    ZVIL_BOUNCE_LAND = 4,
};

enum en_ZVILLAIN_BUBBLE_RESPONSE
{
    ZVIL_BUBBLE_NONE = 0,
    ZVIL_BUBBLE_ENVELOP = 1,
    ZVIL_BUBBLE_BREAK = 2,
    ZVIL_BUBBLE_BOUNCE = 3,
};

enum en_ZVILLAIN_HEADBUTT_RESPONSE
{
    ZVIL_HEADBUTT_NONE = 0,
    ZVIL_HEADBUTT_NORMAL = 1,
    ZVIL_HEADBUTT_CONTINUE = 2,
    ZVIL_HEADBUTT_HITGANG = 3,
    ZVIL_HEADBUTT_DAMAGE = 4,
};

enum en_ZVILLAIN_BUMPED_RESPONSE
{
    ZVIL_BUMPED_NONE = 0,
    ZVIL_BUMPED_SOAP = 1,
    ZVIL_BUMPED_GUM = 2,
    ZVIL_BUMPED_DAMAGE = 3,
    ZVIL_BUMPED_ATTACK = 4,
};

enum en_NPC_TIMERS
{
    ZVIL_TTIL_FIDGET = 0,
    ZVIL_TTIL_BORED = 1,
    ZVIL_TTIL_TAUNT = 2,
    ZVIL_TTIL_CHKFORNAV = 3,
    ZVIL_TTIL_RECOVERY = 4,
    ZVIL_TTIL_NEXTCHASE = 5,
    ZVIL_TTIL_ANIMSOUND = 6,
    ZVIL_TTIL_NEXTVOICE = 7,
    ZVIL_TTIL_QUIP_OK = 8,
    ZVIL_TDUR_STDBY = 9,
    ZVIL_TDUR_ALERT = 10,
    ZVIL_TDUR_TAUNT = 11,
    ZVIL_TDUR_DAZED = 12,
    ZVIL_TDUR_GUM = 13,
    ZVIL_TDUR_SOAP = 14,
    ZVIL_TDUR_LOOK = 15,
    ZVIL_TDUR_HEAR = 16,
    ZVIL_TDUR_SEARCH = 17,
    ZVIL_TDUR_EXTEND = 18,
    ZVIL_TDUR_LOBRELOAD = 19,
    ZVIL_TDUR_EXTRELOAD = 20,
    ZVIL_TDUR_BUILDUP = 21,
    ZVIL_TDUR_HURT = 22,
    ZVIL_TDUR_DECAY = 23,
    ZVIL_TDUR_NAVDELAY = 24,
    ZVIL_TWIN_MINGUM = 25,
    ZVIL_TWIN_POSTATTACK = 26,
    ZVIL_TWIN_IGNORESNEAK = 27,
    ZVIL_TYMR_MINANIMPLAY = 28,
    ZVIL_TYMR_CURGOAL = 29,
    ZVIL_TYMR_GENERIC = 30,
    ZVIL_TYMR_WEPLIFE = 31,
    ZVIL_TYMR_NOMORE = 32,
};

enum en_ZVMM_FLASK_STAGE
{
    MAST_STAGE_UNKNOWN = 0,
    MAST_STAGE_TEASE = 1,
    MAST_STAGE_TOSS = 2,
    MAST_STAGE_AFTERMATH = 3,
};

enum en_ZVMM_FLASK_TYPE
{
    MAST_FLSK_FIRE = 0,
    MAST_FLSK_ACID = 1,
    MAST_FLSK_SMOG = 2,
    MAST_FLSK_NOMORE = 3,
};

enum en_BGG_PHASE
{
    BGG_PHAZ_UNKNOWN = 0,
    BGG_PHAZ_BUILDUP = 1,
    BGG_PHAZ_FLY = 2,
    BGG_PHAZ_FALL = 3,
    BGG_PHAZ_SHOCKWAVE = 4,
};

enum en_RBS_STAGE
{
    RBS_STAGE_PRELIGHT = 4000,
    RBS_STAGE_OUTBOUND = 4001,
    RBS_STAGE_INBOUND = 4002,
};

enum en_zEntVillain_StartBehaviour
{
    ZVIL_START_PREBIRTH = 0,
    ZVIL_START_DEFAULT = 1,
    ZVIL_START_SLEEPING = 2,
    ZVIL_START_AMBUSH = 3,
};

enum en_ZVIL_MODEL_VERTEX
{
    ZVIL_MDLVERT_GUM_LFOOT = 0,
    ZVIL_MDLVERT_GUM_RFOOT = 1,
    ZVIL_MDLVERT_GUM_ELBOW = 2,
    ZVIL_MDLVERT_GUM_CUSTA = 3,
    ZVIL_MDLVERT_GUM_CUSTB = 4,
    ZVIL_MDLVERT_WEAPON = 5,
    ZVIL_MDLVERT_ATTACK = 6,
    ZVIL_MDLVERT_ATTACKALT = 7,
    ZVIL_MDLVERT_DMG_BOUNCE = 8,
    ZVIL_MDLVERT_LOSEYEBALL = 9,
    ZVIL_MDLVERT_SWORDHANDLE = 10,
    ZVIL_MDLVERT_SWORDPIVOT = 11,
    ZVIL_MDLVERT_NOMORE = 12,
};

enum en_NPC_INI_PARAMETERS
{
    ZVIL_PARM_NONE = -1,
    ZVIL_PARM_HIT_POINTS = 0,
    ZVIL_PARM_BUBBLED_TIME = 1,
    ZVIL_PARM_GUMMED_TIME = 2,
    ZVIL_PARM_DAZED_TIME = 3,
    ZVIL_PARM_EXTEND_RANGE = 4,
    ZVIL_PARM_EXTEND_RATE = 5,
    ZVIL_PARM_EXTEND_WIDTH = 6,
    ZVIL_PARM_EXTEND_TIME = 7,
    ZVIL_PARM_FOV_ANGLE = 8,
    ZVIL_PARM_GLOAT_TIME = 9,
    ZVIL_PARM_LOB_RANGE = 10,
    ZVIL_PARM_LOB_SPEED = 11,
    ZVIL_PARM_LOB_SALVO = 12,
    ZVIL_PARM_LOB_PRJTYPE = 13,
    ZVIL_PARM_LOB_RELOAD_TIME = 14,
    ZVIL_PARM_LOB_ARC = 15,
    ZVIL_PARM_LOB_HEAVY = 16,
    ZVIL_PARM_DETECT_RADIUS = 17,
    ZVIL_PARM_DETECT_HEIGHT = 18,
    ZVIL_PARM_DETECT_HYTOFF = 19,
    ZVIL_PARM_PURSUE_RADIUS = 20,
    ZVIL_PARM_MOVE_RATE = 21,
    ZVIL_PARM_CHASE_RATE = 22,
    ZVIL_PARM_TURN_RATE = 23,
    ZVIL_PARM_MDLSCL_BASE = 24,
    ZVIL_PARM_SANIM_IDLE = 25,
    ZVIL_PARM_SANIM_MOVE = 26,
    ZVIL_PARM_SANIM_CHASE = 27,
    ZVIL_PARM_WANDER_ARC = 28,
    ZVIL_PARM_VERT_GUM_LFOOT = 29,
    ZVIL_PARM_VERT_GUM_RFOOT = 30,
    ZVIL_PARM_VERT_GUM_ELBOW = 31,
    ZVIL_PARM_VERT_GUM_CUSTA = 32,
    ZVIL_PARM_VERT_GUM_CUSTB = 33,
    ZVIL_PARM_VERT_WEAPON = 34,
    ZVIL_PARM_VERT_ATTACK = 35,
    ZVIL_PARM_VERT_ATTACKALT = 36,
    ZVIL_PARM_VERT_DMG_BOUNCE = 37,
    ZVIL_PARM_VERT_LOSEYEBALL = 38,
    ZVIL_PARM_VERT_RBSWORDHANDLE = 39,
    ZVIL_PARM_VERT_RBSWORDPIVOT = 40,
    ZVIL_PARM_BND_SPH_CENTER = 41,
    ZVIL_PARM_BND_SPH_RADIUS = 42,
    ZVIL_PARM_BND2_SPH_CENTER = 43,
    ZVIL_PARM_BND2_SPH_RADIUS = 44,
    ZVIL_PARM_SPAWNMASK = 45,
    ZVIL_PARM_MINDIFFICULTY = 46,
    ZVIL_PARM_DMG_BEGTIME = 47,
    ZVIL_PARM_DMG_ENDTIME = 48,
    ZVIL_PARM_NOMORE = 49,
};


class st_ZVIL_PRECALC
{
public:
    signed int flg_status; // 0x0
    float ds2_VtoP; // 0x4
    float dst_VtoP; // 0x8
    float ang_VtoP; // 0xC
    _xVec3 dir_VtoP; // 0x10
    _xVec3 vec_VtoP; // 0x1C
    unsigned int plyr_mvinf; // 0x28
    signed int flg_plyr; // 0x2C
    signed int flg_xtra; // 0x30
    float dif_plyrCtr; // 0x34
    float dif_vilCtr; // 0x38
    float hyt_vil; // 0x3C
    float hyt_plyr; // 0x40
    float ds2_anchor; // 0x44
    float ds2_grdpos; // 0x48
    float cos_faceang; // 0x4C
    float sin_faceang; // 0x50
    float ds2_plyrToAnchor; // 0x54
    _xVec3 pos_intercept; // 0x58
};

class st_NPC_SOUND_QUEUE
{
public:
    en_NPC_SOUND sndEvent; // 0x0
    unsigned int sndDirect; // 0x4
    signed int flg_frolic; // 0x8
    float tmr_delay; // 0xC
    signed int cnt_repeat; // 0x10
    float radius; // 0x14
    float tym_repeatDelay; // 0x18
};

class st_ZVIL_CONFIG_REFERENCE
{
public:
    p2EntVillainAsset* vilp2ass; // 0x0
    p2EntAsset* asset; // 0x4
    float spd_move; // 0x8
    float spd_chase; // 0xC
    float spd_turn; // 0x10
    float ds2_detect; // 0x14
    float hyt_detect; // 0x18
    float hyt_dtctoff; // 0x1C
    float fov_detect; // 0x20
    float ds2_pursue; // 0x24
    float ds2_fromhome; // 0x28
    float spd_lob; // 0x2C
    float dst_lob; // 0x30
    signed int cnt_lobSalvo; // 0x34
    unsigned int prjTypeID; // 0x38
    float dst_extend; // 0x3C
    float wid_extend; // 0x40
    float spd_extend; // 0x44
    float ang_wander; // 0x48
    float fac_lobarc; // 0x4C
    float fac_lobheavy; // 0x50
    float tymref_list[32]; // 0x54
    signed int msk_condspawn; // 0xD4
    signed int minDifficulty; // 0xD8
    signed int hitpts; // 0xDC
    signed int misc_flags; // 0xE0
    _xVec3 scl_modelBase; // 0xE4
    _xVec3 sanim_idle; // 0xF0
    _xVec3 sanim_move; // 0xFC
    _xVec3 sanim_chase; // 0x108
    float rad_bound; // 0x114
    _xVec3 vec_bndoffset; // 0x118
    float rad_bound2; // 0x124
    _xVec3 vec_bnd2offset; // 0x128
    float tym_attack_begin; // 0x134
    float tym_attack_end; // 0x138
    _xVec3 pos_verts[12]; // 0x13C
};

class st_STIMULUS_CONTEXT_DATA
{
public:
    en_NPC_STIMULUS stim; // 0x0
    _zEnt* tgt_ent;       // 0x4
    union
    {
        en_BEHAVIOUR_GOAL gods_goal;
        _zEntBubble* bubble;
        _xVec3 dir_headbutt;
        _xCollis* colrec;
        _xVec3 pos_guard;
        _xVec3 gotopos;
        xBase* widge;
        signed int use_alt_action;
        signed int mm_teleport_hurtz_doggie;
        zSFX* talkSFX;
    }; // 0x8
    union
    {
        float off_goto;
        float mag_headbutt;
        float amt_stun;
    }; // 0x14
    union
    {
        signed int res_gods;
        en_ZVILLAIN_BUBBLE_RESPONSE res_bubble;
        en_ZVILLAIN_HEADBUTT_RESPONSE res_headbutt;
        en_ZVILLAIN_BOUNCE_RESPONSE res_buttbounce;
        en_ZVILLAIN_BUMPED_RESPONSE res_bumped;
    }; // 0x18
    signed int res_bogus;       // 0x1C
    en_BEHAVIOUR_GOAL trangoal; // 0x20
    en_NPC_MODE tranmode;       // 0x24
};

class st_ZVIL_NPC_HANDLERFUNCS
{
public:
    signed int flg_funcs; // 0x0
    signed int (*flags)(st_zEntVillain*, en_ZVIL_FLAG_GROUPS, en_BEHAVIOUR_GOAL); // 0x4
    signed int (*query)(st_zEntVillain*, en_ZVIL_QUERY); // 0x8
    signed int (*sysevent)(st_zEntVillain*, xBase*, unsigned int, float*, xBase*,
                           st_STIMULUS_CONTEXT_DATA*); // 0xC
    signed int (*notice)(st_zEntVillain*, en_ZVIL_NOTICE, st_STIMULUS_CONTEXT_DATA*); // 0x10
    signed int (*aware)(st_zEntVillain*, _xScene*, float, st_STIMULUS_CONTEXT_DATA*); // 0x14
    signed int (*stimulus)(st_zEntVillain*, st_STIMULUS_CONTEXT_DATA*, st_STIMULUS_CONTEXT_DATA*); // 0x18
    signed int (*trangoal)(st_zEntVillain*, st_STIMULUS_CONTEXT_DATA*, float); // 0x1C
    signed int (*calcmove)(st_zEntVillain*, _xScene*, float, _xEntFrame*); // 0x20
    signed int (*respond)(st_zEntVillain*, st_STIMULUS_CONTEXT_DATA*); // 0x24
    signed int (*pickanim)(st_zEntVillain*, en_BEHAVIOUR_GOAL, en_ZVIL_GOAL_SPOT); // 0x28
    signed int (*animtran)(st_zEntVillain*, xAnimTransition*, xAnimSingle*); // 0x2C
    signed int (*xtrarend)(st_zEntVillain*); // 0x30
};

class st_ZVIL_NPCTYPE_LOOKUP
{
public:
    signed int enabled; // 0x0
    float decay; // 0x4
    float ds2_atpos; // 0x8
    float strength; // 0xC
    float inv_mass; // 0x10
    signed int dmg_give; // 0x14
    float dst_noisy; // 0x18
    float rad_bound; // 0x1C
    _xVec3 vec_bndoff; // 0x20
    float rad_bound2; // 0x2C
    _xVec3 vec_bnd2off; // 0x30
    signed int ability; // 0x3C
    signed int abl_atak; // 0x40
    signed int abl_dtct; // 0x44
    signed int abl_move; // 0x48
    signed int abl_spyk; // 0x4C
    signed int abl_vuln; // 0x50
    en_NPC_PERSONALITIES typ_person; // 0x54
    unsigned char flg_physics; // 0x58
    unsigned char pad[3]; // 0x59
    unsigned char flg_chk; // 0x5C
    unsigned char flg_pen; // 0x5D
    unsigned char flg_chkby; // 0x5E
    unsigned char flg_penby; // 0x5F
};

class st_NPCTYPE_CONFIG
{
public:
    signed int hitpts; // 0x0
    signed int misc_flags; // 0x4
    _xVec3 scl_modelBase; // 0x8
    _xVec3 sanim_idle; // 0x14
    _xVec3 sanim_move; // 0x20
    _xVec3 sanim_chase; // 0x2C
    float rad_bound; // 0x38
    _xVec3 vec_bndoffset; // 0x3C
    float rad_bound2; // 0x48
    _xVec3 vec_bnd2offset; // 0x4C
    float tym_attack_begin; // 0x58
    float tym_attack_end; // 0x5C
    _xVec3 pos_verts[12]; // 0x60
};

class st_NPC_BONUS
{
public:
    signed int flg_info; // 0x0
    _zEntPickup* pickup; // 0x4
};

class st_NPC_TALLY_RECORD
{
public:
    signed int cnt_hitgum; // 0x0
    signed int cnt_hitsoap; // 0x4
    signed int cnt_killed; // 0x8
    signed int cnt_mortals; // 0xC
};

class st_zEntVillain_ParEmitRecord
{
public:
    en_VPAR_EMITTER_ROLE emitrole; // 0x0
    en_zEntVillainSubTypeID viltype; // 0x4
    signed int priority; // 0x8
    unsigned int vilowner; // 0xC
    unsigned int emitterID; // 0x10
    _zParEmitter* emitctxt; // 0x14
    signed int flg_need; // 0x18
    signed int flg_omit; // 0x1C
};

class st_zEntVillain_ParEmitInfo
{
public:
    char* emitname; // 0x0
    en_VPAR_EMITTER_ROLE emitrole; // 0x4
    en_zEntVillainSubTypeID viltype; // 0x8
    signed int flg_need; // 0xC
    signed int flg_omit; // 0x10
};

class st_BKAXE_DATA
{
public:
    signed int flg_stat; // 0x0
    _tagLightning* strk[8]; // 0x4
    float rad_zap; // 0x24
    float dst_zapdrop; // 0x28
    float tym_zapdur; // 0x2C
    _xVec3 planes; // 0x30
};

class st_BGG_QUAKEBALL
{
public:
    signed int flg_stat; // 0x0
    en_BGG_PHASE phase; // 0x4
    float rad_build; // 0x8
    float spd_flight; // 0xC
    _xVec3 pos_tgt; // 0x10
    float tym_flightMax; // 0x1C
    float rad_shock; // 0x20
    float hyt_shock; // 0x24
    float tym_shock; // 0x28
};

class st_REDBEARD_SWORD : public _xEnt
{
public:
    signed int flg_stat; // 0xE8
    en_RBS_STAGE stage; // 0xEC
    unsigned int aid_mdlFile; // 0xF0
    RpAtomic* mdlFile; // 0xF4
    st_zEntVillain* rbvil; // 0xF8
    xModelInstance* mdl_animsword; // 0xFC
    _xEntFrame raw_frame; // 0x100
    _xEntCollis raw_collis; // 0x1F0
    _xEnt* ent_colchk[5]; // 0x6EC
    float tym_age; // 0x700
    float dst_far; // 0x704
    float spd_fly; // 0x708
    _xVec3 dir_end; // 0x70C
    _xVec3 pos_end; // 0x718
};

class st_ZVMM_FLASK_DATA
{
public:
    signed int flg_stat; // 0x0
    en_ZVMM_FLASK_STAGE stage; // 0x4
    en_ZVMM_FLASK_TYPE typ_flask; // 0x8
    _xVec3 pos_tgt; // 0xC
    float spd_flight; // 0x18
    float arcness; // 0x1C
    float heavy; // 0x20
    float tym_aftermath; // 0x24
    float hyt_aftermath; // 0x28
    float rad_aftermath; // 0x2C
};

class st_NPC_SOUND_PROPERTIES
{
public:
    en_NPC_SOUND sndEvent; // 0x0
    signed int flg_vsnd; // 0x4
    float tym_delayStart; // 0x8
};

class st_NPC_SOUND_RECORDS
{
public:
    en_NPC_SOUND typ_sound; // 0x0
    char* nam_sound; // 0x4
    unsigned int aid_sound; // 0x8
};

class st_NPC_SOUND_ENTRY
{
public:
    en_zEntVillainSubTypeID viltyp; // 0x0
    st_NPC_SOUND_RECORDS* sndList; // 0x4
};

class st_zEntVillain : public _zEnt
{
public:
    st_ZVIL_NPC_HANDLERFUNCS* vhf; // 0xEC
    signed int ability; // 0xF0
    signed int abl_atak; // 0xF4
    signed int abl_dtct; // 0xF8
    signed int abl_move; // 0xFC
    signed int abl_spyk; // 0x100
    signed int abl_vuln; // 0x104
    en_NPC_PERSONALITIES typ_person; // 0x108
    signed int flg_misc; // 0x10C
    en_NPC_MODE mode; // 0x110
    en_BEHAVIOUR_GOAL goal; // 0x114
    en_BEHAVIOUR_GOAL goaldflt; // 0x118
    signed int standby; // 0x11C
    signed int anim_playing; // 0x120
    signed int anim_playsync; // 0x124
    en_NPC_ANIMSTATES anim_playnext; // 0x128
    float anim_loopmult; // 0x12C
    _xVec3 sanim_idle; // 0x130
    _xVec3 sanim_move; // 0x13C
    _xVec3 sanim_chase; // 0x148
    float spd_move; // 0x154
    float spd_chase; // 0x158
    float spd_turn; // 0x15C
    float ds2_detect; // 0x160
    float fov_detect; // 0x164
    float hyt_detect; // 0x168
    float hyt_dtctoff; // 0x16C
    float ds2_pursue; // 0x170
    float spd_lob; // 0x174
    float dst_lob; // 0x178
    signed int cnt_lobSalvo; // 0x17C
    unsigned int prjTypeID; // 0x180
    _xVec3 vec_lobSpin; // 0x184
    float dst_extend; // 0x190
    float wid_extend; // 0x194
    float spd_extend; // 0x198
    _xVec3 pos_exttgt; // 0x19C
    float ang_wander; // 0x1A8
    float fac_lobarc; // 0x1AC
    float fac_lobheavy; // 0x1B0
    iModelTag tag_vert[12]; // 0x1B4
    signed int flg_vert; // 0x334
    tagzFeet feet; // 0x338
    zSFX* sfx_curTalk; // 0x344
    _xBound bound2; // 0x348
    signed int allow; // 0x380
    _zEntBubble* bubble; // 0x384
    float scl_nombub; // 0x388
    xModelInstance* mdl_gumBlob; // 0x38C
    signed int hitpts; // 0x390
    unsigned int flinch; // 0x394
    signed int goodattack; // 0x398
    _zMovePoint* nav_mvptlist; // 0x39C
    _zMovePoint* nav_next; // 0x3A0
    _zMovePoint* nav_prev; // 0x3A4
    signed int need_navpt; // 0x3A8
    float dst_navTween; // 0x3AC
    signed int domove; // 0x3B0
    _xVec3 pos_moveto; // 0x3B4
    _xVec3 dlt_moveto; // 0x3C0
    _xVec3 dir_moveto; // 0x3CC
    float ang_moveto; // 0x3D8
    _xVec3 vel_moveto; // 0x3DC
    float dlt_faceang; // 0x3E8
    float ang_mdlface; // 0x3EC
    float spd_moveto; // 0x3F0
    float wiz_moveto; // 0x3F4
    float ang_spiral; // 0x3F8
    float rad_spiral; // 0x3FC
    _xVec3 pos_vortex; // 0x400
    _xVec3 vec_knocked; // 0x40C
    _xVec3 pos_bowling; // 0x418
    _xVec3 dir_bowling; // 0x424
    _xVec3 pos_pursue_home; // 0x430
    _xVec3 pos_guard_home; // 0x43C
    _xVec3 pos_goto; // 0x448
    float ds2_goto_keepaway; // 0x454
    _xEnt* tgt_follow; // 0x458
    _zMovePoint* tgt_bullseye; // 0x45C
    _zEnt* tgt_protect; // 0x460
    signed int tgt_stat; // 0x464
    float ang_listen; // 0x468
    zEntProjectile* prj_reserved; // 0x46C
    float tmr_list[32]; // 0x470
    st_ZVIL_PRECALC precalc; // 0x4F0
    st_NPC_SOUND_QUEUE snd_queue[4]; // 0x554
    st_ZVIL_CONFIG_REFERENCE vilcfg; // 0x5C4
};

class zAnimFxSound
{
public:
    unsigned int ID; // 0x0
    float vol; // 0x4
    float pitch; // 0x8
    unsigned int priority; // 0xC
    unsigned int flags; // 0x10
    float radius; // 0x14
};

class p2EntVillainAsset
{
public:
    float activateRadius; // 0x0
    float activateFOV; // 0x4
    float detectHeight; // 0x8
    float detectHeightOffset; // 0xC
    float speedMovement; // 0x10
    float speedPursue; // 0x14
    float speedTurn; // 0x18
    float pursuitRange; // 0x1C
    unsigned short durDazedState; // 0x20
    unsigned short durGloatState; // 0x22
    unsigned short durGummedState; // 0x24
    unsigned short durBubbleState; // 0x26
    unsigned char hitpoints; // 0x28
    unsigned char behaviorState; // 0x29
    unsigned short pad; // 0x2A
    unsigned int villFlags; // 0x2C
    float lobSpeed; // 0x30
    float lobDurReload; // 0x34
    float lobRange; // 0x38
    unsigned int lobSalvo; // 0x3C
    unsigned int projectileTypeID; // 0x40
    unsigned int mvptBullseyeID; // 0x44
    float lobArcness; // 0x48
    float lobHeavy; // 0x4C
    float extenderRange; // 0x50
    float extenderWidth; // 0x54
    float extenderDuration; // 0x58
    float extenderRate; // 0x5C
    float extenderReloadTime; // 0x60
    unsigned int movePointAssetID; // 0x64
    unsigned int pathAssetID; // 0x68
    signed int minPlayerPowerups; // 0x6C
    signed int minGameDifficulty; // 0x70
};



extern "C" void* memset(void* dst, int val, unsigned long n);

_zMovePoint* zMovePointFindNearest(signed int flags, _xVec3* pos, void* context,
                                   signed int (*cb)(void*, _xVec3*, _zMovePoint*));
signed int xUtil_yesno(float wt_yes);
float xVec3Normalize(_xVec3* o, _xVec3* v);
static inline float xVec3Length2(const _xVec3* v)
{
    return v->x * v->x + v->y * v->y + v->z * v->z;
}
void zEntPickup_Reset(_zEntPickup* ent);
void zEntPickup_MakeDroppable(_zEntPickup* ent, _xVec3* pos, xBase* dropParent);
void zEntPickup_Drop(_zEntPickup* ent);
xAnimState* xAnimTableGetStateID(xAnimTable* table, unsigned int ID);
void xAnimPlayStartTransition(xAnimPlay* play, xAnimTransition* tran);
xAnimTransition* xAnimTempTransitionAlloc();
char* xSceneID2Name(_xScene* sc, unsigned int id);
void xAnimTableAddTransition(xAnimTable* table, xAnimTransition* tran, char* source);
void Shaggy5_WitchTransitions(xAnimTable* table);


_tagLightning* zLightningAdd(_tagLightningAdd* add);
void zLightningKill(_tagLightning* l);
void xMat3x3LookVec(_xMat3x3* m, _xVec3* at);
extern "C" void* memcpy(void* dst, const void* src, unsigned long n);
extern const _xVec3 g_Y3;
extern const _xVec3 g_O3;
extern const _xVec3 g_Z3;
extern const _xVec3 g_NY3;
extern "C" double asin(double);
extern "C" void fn_800A8F88(xBase* b, zScene* s); // called from zSceneSetup
void xDrawSetColor(iColor_tag color);
unsigned int xRayHitsSceneFast(_xRay3* r, _xScene* sc);
unsigned int zEntPlayer_MoveInfo();
void zEntPlayer_Notify(unsigned char characterSubType);
void zEntPlayer_Notify(signed int situation);
unsigned int zEntIsShaggy(_zEnt* ent);
void zLaughNotify(signed int situation);
void zEntBubble_Pop(_zEntBubble* bub);
void* iSndLookup(unsigned int id);
void zHologramAdd(_xEnt* ent);
void xAnimPlaySetState(xAnimSingle* single, xAnimState* state, float startTime);
void zEntShaggy_Init(st_zEntVillain* vil);
void xModelRender(xModelInstance* modelInst);
void xEntRender(_xEnt* ent);
unsigned int iModelTagSetup(iModelTag* tag, RpAtomic* model, float x, float y, float z);
void xEntBeginUpdate(_xEnt* ent, _xScene* sc, float dt);
void xEntEndUpdate(_xEnt* ent, _xScene* sc, float dt);
void xEntApplyPhysics(_xEnt* ent, _xScene* sc, float dt);
void xEntMove(_xEnt* ent, _xScene* sc, float dt);
void xEntCollide(_xEnt* ent, _xScene* sc, float dt);
void xEntMotionToMatrix(_xEnt* ent, _xEntFrame* frame);
_xVec3* zMovePointGetPos(_zMovePoint* m);
unsigned char zMovePointIsOn(_zMovePoint* m);
float zMovePointGetDelay(_zMovePoint* m);
_zMovePoint* zMovePoint_From_xAssetID(unsigned int aid);
float zMovePointGetNext(_zMovePoint* current, _zMovePoint* prev, _zMovePoint** next, _xVec3* hdng);
class zAssetModelParam;
zAssetModelParam* zEntGetModelParams(unsigned int assetID, unsigned int* size);
signed int zParamGetInt(zAssetModelParam* param, unsigned int size, char* tok, signed int def);
float zParamGetFloat(zAssetModelParam* param, unsigned int size, char* tok, float def);
signed int zParamGetVector(zAssetModelParam* param, unsigned int size, char* tok, _xVec3 def,
                           _xVec3* result);
void xEntBeginCollide(_xEnt* ent, _xScene* sc, float dt);
void xEntEndCollide(_xEnt* ent, _xScene* sc, float dt);
xModelInstance* xModelInstanceAlloc(RpAtomic* data, void* object, unsigned short flags, unsigned char boneIndex,
                                    unsigned char* boneRemap);
void xModelInstanceFree(xModelInstance* inst);
static void VIRM_initVilDflt(st_zEntVillain* ent);
static signed int zVIL_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* sc, float dt);
static float VNAV_get_movespeed(st_zEntVillain* vil, float rnd);
void xMat4x3RMul3x3(_xMat4x3* o, _xMat3x3* a, _xMat4x3* b);
signed int zEntPlayer_Damage(xBase* src, unsigned int damage);
signed int zEntPlayer_Damage(xBase* src, unsigned int damage, _xVec3* knockback);
class RwTexture;
extern "C" signed int RwRenderStateSet(signed int state, void* value);
class RxObjSpace3DVertex
{
public:
    _xVec3 objVertex; // 0x0
    _xVec3 objNormal; // 0xC
    RwRGBA color;     // 0x18
    float u;          // 0x1C
    float v;          // 0x20
};
extern "C" void* RwIm3DTransform(RxObjSpace3DVertex* pVerts, unsigned int numVerts, void* ltm, unsigned int flags);
extern "C" signed int RwIm3DRenderPrimitive(signed int primType);
extern "C" signed int RwIm3DEnd();
static void SuperVillainGumDraw(_xVec3* from, _xVec3* to, float top_radius, float bot_radius);

class ScoobRGBA
{
public:
    unsigned char r, g, b, a;
};
// Defined in zEntVillainHolo.cpp.
extern ScoobRGBA g_holoPoofBirth[50];
extern ScoobRGBA g_holoPoofDeath[50];
extern ScoobRGBA g_holoDotsBirth[50];
extern ScoobRGBA g_holoDotsDeath[50];
static en_ZVILLAIN_HEADBUTT_RESPONSE VOUT_spikey_header(st_zEntVillain* vil);
static signed int VNAV_chklos_timz(void* usrdata, _xVec3* mypos, _zMovePoint* mvpt);
static en_NPC_ANIMSTATES VANM_set(st_zEntVillain* ent, en_NPC_ANIMSTATES* best, signed int cnt, signed int sync);

static _zParEmitter* sEmitHoloPoof;
static _zParEmitter* sEmitHoloDots;
static _zParEmitter* sEmitRBFlames;
static _zParEmitter* sEmitGGVortex1;
static _zParEmitter* sEmitGGVortex2;
static _zParEmitter* sEmitGGVortex3;
static _zParEmitter* sEmitGGVortexHurt;
static _zParEmitter* sEmitGreenBall;
static _zParEmitter* sEmitGreenBallSmack;
static _zParEmitter* sEmitGreenBallShockwave;
static _zParEmitter* sEmitWitchDying;
static _zParEmitter* sEmitKillerPlant;
static _zParEmitter* sEmitMMRedFlask;
static _zParEmitter* sEmitMMGreenFlask;
static _zParEmitter* sEmitMMBlackFlask;
static _zParEmitter* sEmitMMTeleportHide;
static _zParEmitter* sEmitMMTeleportShow;
static signed int g_NPCSoundStreamLock;
static signed int g_npcinit;
static signed int g_npccnt;
static st_zEntVillain* g_npcpool;
static en_BEHAVIOUR_GOAL g_pgol;
static en_NPC_STIMULUS g_pstm;
static RpAtomic* g_gumModelAsset;
static signed int g_bonusTableReady;
static signed int g_stattotvil;
static st_zEntVillain_ParEmitRecord* g_emitpool;
static signed int g_emitnext;
static signed int g_emitready;
static signed int g_scene_is_a_boss_battle;
static char* g_datablock;
static _tagLightning* g_bk_lytstrk[5];
static st_REDBEARD_SWORD g_rbsword[4];
static _xVec3 g_bgg_prj_pos;
static _xVec3 g_mm_telepos; // where the Mastermind teleports out from
static _tagLightning* g_mm_lytstrk[5];
static RxObjSpace3DVertex sStripVert[32];

class st_NPC_NOBONUS
{
public:
    unsigned int sceneID; // 0x0
    unsigned int aid_vil; // 0x4
};

static signed int VILN_BMM_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_BMM_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BMM_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_BMM_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_BMM_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BMM_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BMM_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BMM_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BMM_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_BMM_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_BRB_xtrarend(st_zEntVillain* vil);
static signed int VILN_BRB_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_BRB_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BRB_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_BRB_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_BRB_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BRB_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BRB_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BRB_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BRB_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_BRB_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_BGG_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_BGG_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BGG_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_BGG_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_BGG_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BGG_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BGG_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BGG_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BGG_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_BGG_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_BBK_animtran(st_zEntVillain* vil, xAnimTransition* tran, xAnimSingle* single);
static signed int VILN_BBK_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_BBK_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BBK_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_BBK_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_BBK_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BBK_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BBK_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BBK_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BBK_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_BBK_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_sham_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_sham_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_sham_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_sham_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_sham_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_sham_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_sham_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_sham_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_sham_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_sham_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_gang_animtran(st_zEntVillain* vil, xAnimTransition* tran, xAnimSingle* single);
static signed int VILN_gang_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_gang_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_gang_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_gang_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_gang_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_gang_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_gang_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_gang_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_gang_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_gang_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_shagM_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_shagM_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shagM_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_shagM_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_shagM_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shagM_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shagM_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shagM_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shagM_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_shagM_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_shag_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_shag_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shag_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_shag_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_shag_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shag_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shag_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shag_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shag_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_shag_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_shark_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_shark_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shark_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_shark_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_shark_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shark_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shark_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shark_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shark_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_shark_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_crit_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_crit_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_crit_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_crit_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_crit_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_crit_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_crit_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_crit_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_crit_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_crit_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_kp_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_kp_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_std_xtrarend(st_zEntVillain* vil);
static signed int VILN_std_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_std_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_std_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_std_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_std_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_std_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_std_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_std_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_std_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_std_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);

static st_NPC_SOUND_PROPERTIES g_sndProps[36] = {
    { VSND_TYP_ENCOUNTER, 0x4003, 0.0f },
    { VSND_TYP_SPOOKYROOM, 0x4003, 0.0f },
    { VSND_TYP_IDLESLOP, 0xC001, -1.0f },
    { VSND_TYP_MOVE, 0x4000, 0.2f },
    { VSND_TYP_MOVE_ALT, 0x4000, 0.5f },
    { VSND_TYP_CLANKING, 0x4000, 0.2f },
    { VSND_TYP_SPLISH, 0x4000, 0.1f },
    { VSND_TYP_SPLASH, 0x4000, 0.1f },
    { VSND_TYP_SNORE, 0x4000, 0.4f },
    { VSND_TYP_CHEERING, 0x1202, -1.0f },
    { VSND_TYP_ZSFXTALK, 0x1202, -1.0f },
    { VSND_TYP_ONELINER, 0x1202, -1.0f },
    { VSND_TYP_ONELINERTOO, 0x1202, -1.0f },
    { VSND_TYP_NEEDTARBOOTS, 0x1202, 0.1f },
    { VSND_TYP_EXTRASPRING, 0x1202, 0.1f },
    { VSND_TYP_NEEDFLOAT, 0x1202, 0.1f },
    { VSND_TYP_HAVEFLOAT, 0x1202, 0.1f },
    { VSND_TYP_BLOCKING, 0x9201, 0.1f },
    { VSND_TYP_ESCAPING, 0x9201, 0.1f },
    { VSND_TYP_TRASHTALK, 0x9201, 0.1f },
    { VSND_TYP_FOREPLAYHURT, 0x9201, 0.1f },
    { VSND_TYP_BUTTONPRESS, 0x9201, 0.1f },
    { VSND_TYP_ATTACKBUILD, 0x9201, 0.1f },
    { VSND_TYP_ALMOSTKILLED, 0x9201, 0.1f },
    { VSND_TYP_MMDIE, 0x9201, 5.1f },
    { VSND_TYP_MMLAUGH, 0x9201, 0.1f },
    { VSND_TYP_BKZAPPED, 0x1201, -1.0f },
    { VSND_TYP_BKDIE, 0x1201, -1.0f },
    { VSND_TYP_LOOK, 0xA001, -1.0f },
    { VSND_TYP_LISTEN, 0xA001, -1.0f },
    { VSND_TYP_CURIOUS, 0xA001, -1.0f },
    { VSND_TYP_HOLOFADE, 0x1000A001, 0.1f },
    { VSND_TYP_SPELL_CAST, 0x1000, 0.0f },
    { VSND_TYP_SPELL_PLAY, 0x2000, 0.0f },
    { VSND_TYP_SPELL_LOOP, 0x2803, 0.0f },
    { VSND_LISTEND, 0x2000, 0.1f },
};

static st_NPC_SOUND_RECORDS g_sndTrax_General[65] = {
    { VSND_TYP_SPOOKYROOM, "theramin 01", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin 02", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin 03", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin 04", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin 05", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin 06", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin 07", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin 08", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin 09", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin 10", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin 11", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin 12", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin loop 01", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin loop 02", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin loop 03", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin loop 04", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin loop 05", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin loop 06", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin loop 07", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin loop 08", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin loop 09", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin loop 10", 0 },
    { VSND_TYP_SPOOKYROOM, "theramin loop 11", 0 },
    { VSND_TYP_ATTACKSWIPE, "gen_attackswipe1", 0 },
    { VSND_TYP_OUCHCRASH, "gen_crash01", 0 },
    { VSND_TYP_OUCHCRASH, "gen_crash02", 0 },
    { VSND_TYP_OUCHCRASH, "gen_crash03", 0 },
    { VSND_TYP_OUCHCRASH, "gen_crash04", 0 },
    { VSND_TYP_OUCHCRASH, "gen_crash06", 0 },
    { VSND_TYP_OUCHCRASH, "gen_crash08", 0 },
    { VSND_TYP_OUCHCRASH, "gen_crash09", 0 },
    { VSND_TYP_OUCHCRASH, "gen_crash10", 0 },
    { VSND_TYP_SOAPBEGIN, "gen_bubble_form1", 0 },
    { VSND_TYP_SOAPBEGIN, "gen_bubble_form2", 0 },
    { VSND_TYP_SOAPSTOP, "soapbubble_pop", 0 },
    { VSND_TYP_SOAPSTOP, "soapbubble_pop2", 0 },
    { VSND_TYP_SOAPSTOP, "soapbubble_pop3", 0 },
    { VSND_TYP_GUMBEGIN, "gen_gummed1", 0 },
    { VSND_TYP_GUMBEGIN, "gen_gummed2", 0 },
    { VSND_TYP_GUMBEGIN, "gen_gummed3", 0 },
    { VSND_TYP_GUMBEGIN, "gen_gummed4", 0 },
    { VSND_TYP_GUMBEGIN, "gen_gummed5", 0 },
    { VSND_TYP_GUMSTOP, "gum_pop", 0 },
    { VSND_TYP_GUMBOUNCE, "gen_gumbounce1", 0 },
    { VSND_TYP_GUMBOUNCE, "gen_gumbounce2", 0 },
    { VSND_TYP_GUMBOUNCE, "gen_gumbounce05", 0 },
    { VSND_TYP_GUMSTRETCH, "gen_gumstretch1", 0 },
    { VSND_TYP_GUMSTRETCH, "gen_gumstretch2", 0 },
    { VSND_TYP_GUMSTRETCH, "gen_gumstretch3", 0 },
    { VSND_TYP_IDLESLOP, "gen_runstep1", 0 },
    { VSND_TYP_IDLESLOP, "gen_runstep2", 0 },
    { VSND_TYP_IDLESLOP, "gen_runstep3", 0 },
    { VSND_TYP_IDLESLOP, "gen_runstep4", 0 },
    { VSND_TYP_IDLESLOP, "gen_runstep5", 0 },
    { VSND_TYP_IDLESLOP, "gen_runstep6", 0 },
    { VSND_TYP_IDLESLOP, "gen_runstep7", 0 },
    { VSND_TYP_IDLESLOP, "gen_runstep8", 0 },
    { VSND_TYP_FALL, "gen_bodyfall1", 0 },
    { VSND_TYP_FALL, "gen_bodyfall2", 0 },
    { VSND_TYP_FALL, "gen_bodyfall3", 0 },
    { VSND_TYP_FALL, "gen_bodyfall4", 0 },
    { VSND_TYP_FALL, "gen_bodyfall5", 0 },
    { VSND_TYP_FALL, "gen_bodyfall6", 0 },
    { VSND_TYP_HOLOFADE, "life_essence_fade", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_BlackKnight[49] = {
    { VSND_TYP_CLANKING, "bk_armour1", 0 },
    { VSND_TYP_CLANKING, "bk_armour2", 0 },
    { VSND_TYP_CLANKING, "bk_armour3", 0 },
    { VSND_TYP_CLANKING, "bk_armour4", 0 },
    { VSND_TYP_CLANKING, "bk_armour5", 0 },
    { VSND_TYP_CLANKING, "bk_armour6", 0 },
    { VSND_TYP_CLANKING, "bk_armour7", 0 },
    { VSND_TYP_CLANKING, "bk_armour10", 0 },
    { VSND_TYP_CLANKING, "bk_armour11", 0 },
    { VSND_TYP_CLANKING, "bk_armour12", 0 },
    { VSND_TYP_CLANKING, "bk_armour13", 0 },
    { VSND_TYP_CLANKING, "bk_armour14", 0 },
    { VSND_TYP_CLANKING, "bk_armour15", 0 },
    { VSND_TYP_CLANKING, "bk_armour16", 0 },
    { VSND_TYP_MOVE, "bk_step1", 0 },
    { VSND_TYP_MOVE, "bk_step2", 0 },
    { VSND_TYP_MOVE_ALT, "bk_step3", 0 },
    { VSND_TYP_MOVE_ALT, "bk_step4", 0 },
    { VSND_TYP_ATTACK, "bk_attack1", 0 },
    { VSND_TYP_ATTACK, "bk_attack2", 0 },
    { VSND_TYP_ATTACK, "bk_attack3", 0 },
    { VSND_TYP_ATTACK, "bk_attack4", 0 },
    { VSND_TYP_ATTACK, "bk_attack5", 0 },
    { VSND_TYP_ATTACK, "bk_attack6", 0 },
    { VSND_TYP_OUCH, "bk_death3", 0 },
    { VSND_TYP_OUCH, "bk_death4", 0 },
    { VSND_TYP_OUCH, "bk_death5", 0 },
    { VSND_TYP_OUCH, "bk_death6", 0 },
    { VSND_TYP_BKZAPPED, "bk_shock", 0 },
    { VSND_TYP_BKDIE, "bk_break1", 0 },
    { VSND_TYP_BKDIE, "bk_break2", 0 },
    { VSND_TYP_BKDIE, "bk_break3", 0 },
    { VSND_TYP_BKDIE, "bk_break4", 0 },
    { VSND_TYP_BKDIE, "bk_break5", 0 },
    { VSND_TYP_LAUGH, "bk_gloat1", 0 },
    { VSND_TYP_LAUGH, "bk_gloat2", 0 },
    { VSND_TYP_LAUGH, "bk_gloat3", 0 },
    { VSND_TYP_LAUGH, "bk_gloat4", 0 },
    { VSND_TYP_LAUGH, "bk_gloat5", 0 },
    { VSND_TYP_LAUGH, "bk_gloat6", 0 },
    { VSND_TYP_LAUGH, "bk_gloat7", 0 },
    { VSND_TYP_LAUGH, "bk_gloat8", 0 },
    { VSND_TYP_LAUGH, "bk_gloat9", 0 },
    { VSND_TYP_PRJ_FLIGHT, "rb_sword_loop1", 0 },
    { VSND_TYP_PRJ_FLIGHT, "rb_sword_loop2", 0 },
    { VSND_TYP_PRJ_SPLAT, "bk_axehit1", 0 },
    { VSND_TYP_PRJ_SPLAT, "bk_knifehit1", 0 },
    { VSND_TYP_PRJ_SPLAT, "bk_knifehit2", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_GreenGhost[35] = {
    { VSND_TYP_CLANKING, "chain_rattle", 0 },
    { VSND_TYP_CLANKING, "chain_rattle2", 0 },
    { VSND_TYP_CLANKING, "chain_rattle3", 0 },
    { VSND_TYP_CLANKING, "chain_rattle4", 0 },
    { VSND_TYP_MMDIE, "pit_moan_single", 0 },
    { VSND_TYP_DIE, "gr_death_scream01", 0 },
    { VSND_TYP_LAUGH, "gr_laugh01", 0 },
    { VSND_TYP_LAUGH, "gr_laugh02", 0 },
    { VSND_TYP_LAUGH, "gr_laugh03", 0 },
    { VSND_TYP_LAUGH, "gr_laugh04", 0 },
    { VSND_TYP_LAUGH, "gr_laugh05", 0 },
    { VSND_TYP_LAUGH, "gr_laugh06", 0 },
    { VSND_TYP_LAUGH, "gr_laugh07", 0 },
    { VSND_TYP_LAUGH, "gr_laugh08", 0 },
    { VSND_TYP_DIZZY, "gr_scare_scream01", 0 },
    { VSND_TYP_DIZZY, "gr_scare_scream02", 0 },
    { VSND_TYP_DIZZY, "gr_scare_scream03", 0 },
    { VSND_TYP_DIZZY, "gr_scare_scream04", 0 },
    { VSND_TYP_DIALOGUE, "gr_suck_scream01", 0 },
    { VSND_TYP_DIALOGUE, "gr_suck_scream02", 0 },
    { VSND_TYP_DIALOGUE, "gr_suck_scream03", 0 },
    { VSND_TYP_DIALOGUE, "gr_suck_scream04", 0 },
    { VSND_TYP_DIALOGUE, "gr_suck_scream05", 0 },
    { VSND_TYP_DIALOGUE, "gr_suck_scream06", 0 },
    { VSND_TYP_DIALOGUE, "gr_suck_scream07", 0 },
    { VSND_TYP_DIALOGUE, "gr_suck_scream08", 0 },
    { VSND_TYP_DIALOGUE, "gr_suck_scream09", 0 },
    { VSND_TYP_DIALOGUE, "gr_vortex1", 0 },
    { VSND_TYP_DIALOGUE, "gr_vortex2", 0 },
    { VSND_TYP_PRJ_LAUNCH, "gr_energyball_throw", 0 },
    { VSND_TYP_PRJ_FLIGHT, "gr_energyball_loop", 0 },
    { VSND_TYP_PRJ_EXPLODE, "gr_energyball_hit1", 0 },
    { VSND_TYP_PRJ_EXPLODE, "gr_energyball_hit2", 0 },
    { VSND_TYP_PRJ_SPLAT, "gr_energyball_form", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_RedBeard[23] = {
    { VSND_TYP_PUNCH, "punch1", 0 },
    { VSND_TYP_PUNCH, "punch2", 0 },
    { VSND_TYP_PUNCH, "punch3", 0 },
    { VSND_TYP_DIE, "rb_death01", 0 },
    { VSND_TYP_DIE, "rb_death02", 0 },
    { VSND_TYP_DIE, "rb_death03", 0 },
    { VSND_TYP_OUCH, "rb_hurt01", 0 },
    { VSND_TYP_OUCH, "rb_hurt02", 0 },
    { VSND_TYP_OUCH, "rb_hurt03", 0 },
    { VSND_TYP_IDLESLOP, "rb_idle01", 0 },
    { VSND_TYP_IDLESLOP, "rb_idle02", 0 },
    { VSND_TYP_ATTACK, "rb_laugh01", 0 },
    { VSND_TYP_LAUGH, "rb_taunt01", 0 },
    { VSND_TYP_LAUGH, "rb_taunt02", 0 },
    { VSND_TYP_LAUGH, "rb_taunt03", 0 },
    { VSND_TYP_LAUGH, "rb_taunt04", 0 },
    { VSND_TYP_LAUGH, "rb_taunt05", 0 },
    { VSND_TYP_LAUGH, "rb_taunt06", 0 },
    { VSND_TYP_LAUGH, "rb_taunt07", 0 },
    { VSND_TYP_LAUGH, "rb_taunt08", 0 },
    { VSND_TYP_PRJ_FLIGHT, "rb_sword_loop1", 0 },
    { VSND_TYP_PRJ_FLIGHT, "rb_sword_loop2", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_MasterMind[77] = {
    { VSND_TYP_ATTACKBUILD, "mastermind_01", 0 },
    { VSND_TYP_TRASHTALK, "mastermind_02", 0 },
    { VSND_TYP_ESCAPING, "mastermind_03", 0 },
    { VSND_TYP_ATTACKBUILD, "mastermind_04", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_06", 0 },
    { VSND_TYP_BLOCKING, "mastermind_07", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_08", 0 },
    { VSND_TYP_BLOCKING, "mastermind_09", 0 },
    { VSND_TYP_TRASHTALK, "mastermind_10", 0 },
    { VSND_TYP_TRASHTALK, "mastermind_11", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_12", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_13", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_14", 0 },
    { VSND_TYP_ALMOSTKILLED, "mastermind_15", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_16", 0 },
    { VSND_TYP_DIZZY, "mastermind_17a", 0 },
    { VSND_TYP_DIZZY, "mastermind_17b", 0 },
    { VSND_TYP_BUTTONPRESS, "mastermind_18", 0 },
    { VSND_TYP_BUTTONPRESS, "mastermind_19", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_20", 0 },
    { VSND_TYP_BUTTONPRESS, "mastermind_21", 0 },
    { VSND_TYP_ATTACKBUILD, "mastermind_22", 0 },
    { VSND_TYP_FOREPLAYHURT, "mastermind_23a", 0 },
    { VSND_TYP_MMDIE, "mastermind_05", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_26", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_27", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_28", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_29", 0 },
    { VSND_TYP_BUTTONPRESS, "mastermind_30", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_31", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_32", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_33", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_34", 0 },
    { VSND_TYP_BUTTONPRESS, "mastermind_35", 0 },
    { VSND_TYP_BUTTONPRESS, "mastermind_36", 0 },
    { VSND_TYP_FOREPLAYHURT, "mastermind_37", 0 },
    { VSND_TYP_ATTACKBUILD, "mastermind_38", 0 },
    { VSND_TYP_BLOCKING, "mastermind_39", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_40", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_41", 0 },
    { VSND_TYP_MMLAUGH, "mastermind_42", 0 },
    { VSND_TYP_ATTACKBUILD, "mastermind_43", 0 },
    { VSND_TYP_DIALOGUE, "mastermind_44", 0 },
    { VSND_TYP_MMLAUGH, "mastermind_laugh1", 0 },
    { VSND_TYP_MMLAUGH, "mastermind_laugh2", 0 },
    { VSND_TYP_MMLAUGH, "mastermind_laugh3", 0 },
    { VSND_TYP_MMLAUGH, "mastermind_laugh4", 0 },
    { VSND_TYP_MMLAUGH, "mastermind_laugh5", 0 },
    { VSND_TYP_ATTACKBUILD, "mastermind_laugh1", 0 },
    { VSND_TYP_ATTACKBUILD, "mastermind_laugh2", 0 },
    { VSND_TYP_ATTACKBUILD, "mastermind_laugh3", 0 },
    { VSND_TYP_ATTACKBUILD, "mastermind_laugh4", 0 },
    { VSND_TYP_ATTACKBUILD, "mastermind_laugh5", 0 },
    { VSND_TYP_DISAPPEAR, "magic_wand1", 0 },
    { VSND_TYP_DISAPPEAR, "magic_wand2", 0 },
    { VSND_TYP_REAPPEAR, "magic_wand3", 0 },
    { VSND_TYP_REAPPEAR, "magic_wand4", 0 },
    { VSND_TYP_REAPPEAR, "magic_wand5", 0 },
    { VSND_TYP_REAPPEAR, "magic_wand6", 0 },
    { VSND_TYP_OUCH, "oldsk_death3", 0 },
    { VSND_TYP_OUCH, "oldsk_death4", 0 },
    { VSND_TYP_OUCH, "oldsk_death5", 0 },
    { VSND_TYP_OUCH, "oldsk_death6", 0 },
    { VSND_TYP_CLANKING, "oldsk_gumbounce3", 0 },
    { VSND_TYP_CLANKING, "oldsk_gumbounce4", 0 },
    { VSND_TYP_DIZZY, "oldsk_stunned1", 0 },
    { VSND_TYP_DIZZY, "oldsk_stunned3", 0 },
    { VSND_TYP_PRJ_LAUNCH, "magic_potion", 0 },
    { VSND_TYP_PRJ_EXPLODE, "potion_smash", 0 },
    { VSND_TYP_PRJ_EXPLODE, "mm_bottle_break1", 0 },
    { VSND_TYP_PRJ_EXPLODE, "mm_bottle_break2", 0 },
    { VSND_TYP_PRJ_EXPLODE, "mm_bottle_break3", 0 },
    { VSND_TYP_PRJ_SPLAT, "potion_smash", 0 },
    { VSND_TYP_PRJ_SPLAT, "mm_bottle_break1", 0 },
    { VSND_TYP_PRJ_SPLAT, "mm_bottle_break2", 0 },
    { VSND_TYP_PRJ_SPLAT, "mm_bottle_break3", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_Fred[1] = {
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_Daphne[3] = {
    { VSND_TYP_DIALOGUE, "daphne_01", 0 },
    { VSND_TYP_DIALOGUE, "daphne_02", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_Velma[1] = {
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_Shaggy[1] = {
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_Holly[41] = {
    { VSND_TYP_DIALOGUE, "holly_25", 0 },
    { VSND_TYP_DIALOGUE, "holly_26", 0 },
    { VSND_TYP_DIALOGUE, "holly_27a", 0 },
    { VSND_TYP_DIALOGUE, "holly_27b", 0 },
    { VSND_TYP_ONELINER, "holly_30", 0 },
    { VSND_TYP_DIALOGUE, "holly_32", 0 },
    { VSND_TYP_ONELINER, "holly_33", 0 },
    { VSND_TYP_DIALOGUE, "holly_40", 0 },
    { VSND_TYP_DIALOGUE, "holly_41", 0 },
    { VSND_TYP_DIALOGUE, "holly_42", 0 },
    { VSND_TYP_ONELINER, "holly_43a", 0 },
    { VSND_TYP_ONELINER, "holly_43b", 0 },
    { VSND_TYP_DIALOGUE, "holly_44a", 0 },
    { VSND_TYP_DIALOGUE, "holly_44b", 0 },
    { VSND_TYP_DIALOGUE, "holly_45", 0 },
    { VSND_TYP_DIALOGUE, "holly_46", 0 },
    { VSND_TYP_DIALOGUE, "holly_47a", 0 },
    { VSND_TYP_DIALOGUE, "holly_47b", 0 },
    { VSND_TYP_DIALOGUE, "holly_48", 0 },
    { VSND_TYP_ONELINER, "holly_49", 0 },
    { VSND_TYP_DIALOGUE, "holly_50", 0 },
    { VSND_TYP_ONELINER, "holly_51a", 0 },
    { VSND_TYP_ONELINER, "holly_51b", 0 },
    { VSND_TYP_ONELINER, "holly_52a", 0 },
    { VSND_TYP_ONELINER, "holly_52b", 0 },
    { VSND_TYP_LAUGH, "holly_laugh_01", 0 },
    { VSND_TYP_LAUGH, "holly_laugh_01b", 0 },
    { VSND_TYP_LAUGH, "holly_laugh_02", 0 },
    { VSND_TYP_LAUGH, "holly_laugh_02b", 0 },
    { VSND_TYP_LAUGH, "holly_laugh_03", 0 },
    { VSND_TYP_LAUGH, "holly_laugh_03b", 0 },
    { VSND_TYP_LAUGH, "holly_laugh_04", 0 },
    { VSND_TYP_LAUGH, "holly_laugh_04b", 0 },
    { VSND_TYP_LAUGH, "holly_laugh_05", 0 },
    { VSND_TYP_LAUGH, "holly_laugh_05b", 0 },
    { VSND_TYP_LAUGH, "holly_laugh_06", 0 },
    { VSND_TYP_LAUGH, "holly_laugh_06b", 0 },
    { VSND_TYP_OUCH, "holly_yell1", 0 },
    { VSND_TYP_OUCH, "holly_yell2", 0 },
    { VSND_TYP_OUCH, "holly_yell3", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_GroundsKeeper[67] = {
    { VSND_TYP_ONELINER, "groundskeeper_01", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_02", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_04", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_05", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_06a", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_06b", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_07", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_08", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_09", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_10", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_11", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_12", 0 },
    { VSND_TYP_DIALOGUE, "groundskeeper_13", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_15", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_16", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_17", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_18", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_19", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_20", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_21", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_22", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_23", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_24", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_25", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_26", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_27", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_28", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_29", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_30", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_31", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_32", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_33", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_34", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_35", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_36", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_37", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_38", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_39", 0 },
    { VSND_TYP_DIALOGUE, "groundskeeper_40", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_41", 0 },
    { VSND_TYP_NEEDTARBOOTS, "groundskeeper_42", 0 },
    { VSND_TYP_EXTRASPRING, "groundskeeper_43", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_44", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_45", 0 },
    { VSND_TYP_DIALOGUE, "groundskeeper_46", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_47", 0 },
    { VSND_TYP_NEEDFLOAT, "groundskeeper_48", 0 },
    { VSND_TYP_DIALOGUE, "groundskeeper_49", 0 },
    { VSND_TYP_DIALOGUE, "groundskeeper_50", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_51", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_51b", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_51c", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_51d", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_52", 0 },
    { VSND_TYP_HAVEFLOAT, "groundskeeper_53", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_54", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_55", 0 },
    { VSND_TYP_ONELINER, "groundskeeper_56", 0 },
    { VSND_TYP_DIALOGUE, "groundskeeper_57", 0 },
    { VSND_TYP_DIALOGUE, "groundskeeper_58a", 0 },
    { VSND_TYP_DIALOGUE, "groundskeeper_58b", 0 },
    { VSND_TYP_DIALOGUE, "groundskeeper_59a", 0 },
    { VSND_TYP_DIALOGUE, "groundskeeper_59b", 0 },
    { VSND_TYP_DIALOGUE, "groundskeeper_60a", 0 },
    { VSND_TYP_DIALOGUE, "groundskeeper_60b", 0 },
    { VSND_TYP_DIALOGUE, "groundskeeper_61", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_Professor[1] = {
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_TimConway[44] = {
    { VSND_TYP_ONELINER, "Tim Conway U95_29", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_30", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_31", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_34", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_35", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_36", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_38", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_39", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_40", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_41", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_42", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_43", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_44", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_46", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_49", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_51", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_52", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_53", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_54", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_56", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_57", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_58", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_58b", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_59", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_59b", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_60", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_61", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_63", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_64", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_65", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_66", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_67", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_67b", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_69", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_71", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_73", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_74", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_75", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_76", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_77", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_78", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_80", 0 },
    { VSND_TYP_ONELINER, "Tim Conway U95_81", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_Bat[9] = {
    { VSND_TYP_MOVE, "bat_wing1", 0 },
    { VSND_TYP_MOVE, "bat_wing2", 0 },
    { VSND_TYP_MOVE, "bat_wing3", 0 },
    { VSND_TYP_CLANKING, "bat_cry1", 0 },
    { VSND_TYP_CLANKING, "bat_cry2", 0 },
    { VSND_TYP_CLANKING, "bat_cry3", 0 },
    { VSND_TYP_CLANKING, "bigbat_cry1", 0 },
    { VSND_TYP_CLANKING, "bigbat_cry2", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_BuckMaster[54] = {
    { VSND_TYP_SOAPED, "bm_bubble1", 0 },
    { VSND_TYP_SOAPED, "bm_bubble2", 0 },
    { VSND_TYP_GUMMED, "bm_bubble3", 0 },
    { VSND_TYP_GUMMED, "bm_bubble4", 0 },
    { VSND_TYP_SOAPED, "bm_bubble5", 0 },
    { VSND_TYP_GUMMED, "bm_bubble6", 0 },
    { VSND_TYP_SOAPED, "bm_bubble7", 0 },
    { VSND_TYP_SOAPED, "bm_bubble8", 0 },
    { VSND_TYP_IDLESLOP, "bm_idle1", 0 },
    { VSND_TYP_IDLESLOP, "bm_idle2", 0 },
    { VSND_TYP_IDLESLOP, "bm_idle3", 0 },
    { VSND_TYP_IDLESLOP, "bm_idle4", 0 },
    { VSND_TYP_IDLESLOP, "bm_idle5", 0 },
    { VSND_TYP_EXCLAIM, "bm_surprise1", 0 },
    { VSND_TYP_EXCLAIM, "bm_surprise2", 0 },
    { VSND_TYP_EXCLAIM, "bm_surprise3", 0 },
    { VSND_TYP_EXCLAIM, "bm_surprise4", 0 },
    { VSND_TYP_EXCLAIM, "bm_surprised1", 0 },
    { VSND_TYP_EXCLAIM, "bm_surprised2", 0 },
    { VSND_TYP_EXCLAIM, "bm_surprised3", 0 },
    { VSND_TYP_EXCLAIM, "bm_surprised4", 0 },
    { VSND_TYP_ATTACK, "bm_attack01", 0 },
    { VSND_TYP_ATTACK, "bm_attack02", 0 },
    { VSND_TYP_ATTACK, "bm_attack03", 0 },
    { VSND_TYP_ATTACK, "bm_attack04", 0 },
    { VSND_TYP_ATTACK, "bm_attack05", 0 },
    { VSND_TYP_ATTACK, "bm_attack06", 0 },
    { VSND_TYP_ATTACK, "bm_attack07", 0 },
    { VSND_TYP_KICK, "bm_kick01", 0 },
    { VSND_TYP_KICK, "bm_kick02", 0 },
    { VSND_TYP_KICK, "bm_kick03", 0 },
    { VSND_TYP_KICK, "bm_kick04", 0 },
    { VSND_TYP_KICK, "bm_kick05", 0 },
    { VSND_TYP_KICK, "bm_kick06", 0 },
    { VSND_TYP_KICK, "bm_kick07", 0 },
    { VSND_TYP_CURIOUS, "bm_look1", 0 },
    { VSND_TYP_CURIOUS, "bm_look2", 0 },
    { VSND_TYP_CURIOUS, "bm_look3", 0 },
    { VSND_TYP_LOOK, "bm_look4", 0 },
    { VSND_TYP_CURIOUS, "bm_investigate", 0 },
    { VSND_TYP_CURIOUS, "bm_investigate2", 0 },
    { VSND_TYP_CURIOUS, "bm_investigate3", 0 },
    { VSND_TYP_LOOK, "bm_investigate4", 0 },
    { VSND_TYP_LOOK, "bm_investigate5", 0 },
    { VSND_TYP_LOOK, "bm_investigate6", 0 },
    { VSND_TYP_LOOK, "bm_investigate7", 0 },
    { VSND_TYP_DIE, "bm_death1", 0 },
    { VSND_TYP_OUCH, "bm_death2", 0 },
    { VSND_TYP_OUCH, "bm_death3", 0 },
    { VSND_TYP_OUCH, "bm_death4", 0 },
    { VSND_TYP_OUCH, "bm_death5", 0 },
    { VSND_TYP_OUCH, "bm_death6", 0 },
    { VSND_TYP_FALL, "bm_fall1", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_CaptainMoody[27] = {
    { VSND_TYP_ATTACK, "cm_attack1", 0 },
    { VSND_TYP_ATTACK, "cm_attack2", 0 },
    { VSND_TYP_ATTACK, "cm_attack3", 0 },
    { VSND_TYP_ATTACK, "cm_attack4", 0 },
    { VSND_TYP_CLANKING, "cm_chase", 0 },
    { VSND_TYP_CLANKING, "cm_chase2", 0 },
    { VSND_TYP_CLANKING, "cm_chase3", 0 },
    { VSND_TYP_CLANKING, "cm_chase4", 0 },
    { VSND_TYP_DIE, "cm_death1", 0 },
    { VSND_TYP_DIE, "cm_death2", 0 },
    { VSND_TYP_MOVE, "cm_fly1", 0 },
    { VSND_TYP_LAUGH, "cm_gloat1", 0 },
    { VSND_TYP_LAUGH, "cm_gloat2", 0 },
    { VSND_TYP_LAUGH, "cm_gloat3", 0 },
    { VSND_TYP_LAUGH, "cm_gloat4", 0 },
    { VSND_TYP_GUMMED, "cm_hit1", 0 },
    { VSND_TYP_GUMMED, "cm_hit2", 0 },
    { VSND_TYP_GUMMED, "cm_hit3", 0 },
    { VSND_TYP_LISTEN, "cm_listen1", 0 },
    { VSND_TYP_LISTEN, "cm_listen2", 0 },
    { VSND_TYP_LOOK, "cm_look1", 0 },
    { VSND_TYP_EXCLAIM, "cm_surprise1", 0 },
    { VSND_TYP_EXCLAIM, "cm_surprise2", 0 },
    { VSND_TYP_EXCLAIM, "cm_surprise3", 0 },
    { VSND_TYP_EXCLAIM, "cm_surprise4", 0 },
    { VSND_TYP_EXCLAIM, "cm_surprised1", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_Caveman[43] = {
    { VSND_TYP_ATTACK, "cv_attack1", 0 },
    { VSND_TYP_ATTACK, "cv_attack2", 0 },
    { VSND_TYP_ATTACK, "cv_attack3", 0 },
    { VSND_TYP_SOAPED, "cv_bubble1", 0 },
    { VSND_TYP_SOAPED, "cv_bubble2", 0 },
    { VSND_TYP_SOAPED, "cv_bubble3", 0 },
    { VSND_TYP_MOVE, "cv_chase1", 0 },
    { VSND_TYP_MOVE, "cv_chase2", 0 },
    { VSND_TYP_MOVE, "cv_chase3", 0 },
    { VSND_TYP_MOVE, "cv_chase4", 0 },
    { VSND_TYP_DIE, "cv_death1", 0 },
    { VSND_TYP_DIE, "cv_death2", 0 },
    { VSND_TYP_DIE, "cv_death3", 0 },
    { VSND_TYP_DIE, "cv_death4", 0 },
    { VSND_TYP_GUMMED, "cv_gummed1", 0 },
    { VSND_TYP_GUMMED, "cv_gummed2", 0 },
    { VSND_TYP_CLANKING, "cv_headbonk1", 0 },
    { VSND_TYP_CLANKING, "cv_headbonk2", 0 },
    { VSND_TYP_LISTEN, "cv_hear1", 0 },
    { VSND_TYP_LISTEN, "cv_hear2", 0 },
    { VSND_TYP_LISTEN, "cv_hear3", 0 },
    { VSND_TYP_OUCH, "cv_hit1", 0 },
    { VSND_TYP_OUCH, "cv_hit2", 0 },
    { VSND_TYP_OUCH, "cv_hit3", 0 },
    { VSND_TYP_IDLESLOP, "cv_idle1", 0 },
    { VSND_TYP_IDLESLOP, "cv_idle2", 0 },
    { VSND_TYP_IDLESLOP, "cv_idle3", 0 },
    { VSND_TYP_IDLESLOP, "cv_idle4", 0 },
    { VSND_TYP_IDLESLOP, "cv_idle5", 0 },
    { VSND_TYP_LISTEN, "cv_listen1", 0 },
    { VSND_TYP_LISTEN, "cv_listen2", 0 },
    { VSND_TYP_LISTEN, "cv_listen3", 0 },
    { VSND_TYP_LISTEN, "cv_listen4", 0 },
    { VSND_TYP_LOOK, "cv_look1", 0 },
    { VSND_TYP_EXCLAIM, "cv_see1", 0 },
    { VSND_TYP_EXCLAIM, "cv_see2", 0 },
    { VSND_TYP_EXCLAIM, "cv_surprise1", 0 },
    { VSND_TYP_EXCLAIM, "cv_surprise2", 0 },
    { VSND_TYP_LAUGH, "cv_taunt1", 0 },
    { VSND_TYP_LAUGH, "cv_taunt2", 0 },
    { VSND_TYP_LAUGH, "cv_taunt3", 0 },
    { VSND_TYP_LAUGH, "cv_taunt4", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_Creeper[45] = {
    { VSND_TYP_ATTACK, "cr_attack1", 0 },
    { VSND_TYP_ATTACK, "cr_attack2", 0 },
    { VSND_TYP_ATTACK, "cr_attack3", 0 },
    { VSND_TYP_ATTACK, "cr_attack4", 0 },
    { VSND_TYP_ATTACK, "cr_attack5", 0 },
    { VSND_TYP_ATTACK, "cr_attack6", 0 },
    { VSND_TYP_ATTACK, "cr_attack7", 0 },
    { VSND_TYP_ATTACK, "cr_attack8", 0 },
    { VSND_TYP_SOAPED, "cr_bubble1", 0 },
    { VSND_TYP_SOAPED, "cr_bubble2", 0 },
    { VSND_TYP_SOAPED, "cr_bubble3", 0 },
    { VSND_TYP_SOAPED, "cr_bubble4", 0 },
    { VSND_TYP_DIE, "cr_death1", 0 },
    { VSND_TYP_FALL, "cr_bodyfall1", 0 },
    { VSND_TYP_LAUGH, "cr_gloat1", 0 },
    { VSND_TYP_LAUGH, "cr_gloat2", 0 },
    { VSND_TYP_LAUGH, "cr_gloat3", 0 },
    { VSND_TYP_GUMMED, "cr_gummed1", 0 },
    { VSND_TYP_GUMMED, "cr_gummed2", 0 },
    { VSND_TYP_GUMMED, "cr_gummed3", 0 },
    { VSND_TYP_GUMMED, "cr_gummed4", 0 },
    { VSND_TYP_OUCH, "cr_hit1", 0 },
    { VSND_TYP_OUCH, "cr_hit2", 0 },
    { VSND_TYP_OUCH, "cr_hit3", 0 },
    { VSND_TYP_IDLESLOP, "cr_idle1", 0 },
    { VSND_TYP_IDLESLOP, "cr_idle2", 0 },
    { VSND_TYP_IDLESLOP, "cr_idle3", 0 },
    { VSND_TYP_IDLESLOP, "cr_idle4", 0 },
    { VSND_TYP_CURIOUS, "cr_investigate1", 0 },
    { VSND_TYP_CURIOUS, "cr_investigate2", 0 },
    { VSND_TYP_CURIOUS, "cr_investigate3", 0 },
    { VSND_TYP_CURIOUS, "cr_investigate4", 0 },
    { VSND_TYP_LISTEN, "cr_listen1", 0 },
    { VSND_TYP_LISTEN, "cr_listen2", 0 },
    { VSND_TYP_LISTEN, "cr_listen3", 0 },
    { VSND_TYP_EXCLAIM, "cr_look1", 0 },
    { VSND_TYP_EXCLAIM, "cr_look2", 0 },
    { VSND_TYP_MOVE, "cr_patrol1", 0 },
    { VSND_TYP_MOVE, "cr_patrol2", 0 },
    { VSND_TYP_EXCLAIM, "cr_react1", 0 },
    { VSND_TYP_EXCLAIM, "cr_react2", 0 },
    { VSND_TYP_EXCLAIM, "cr_react3", 0 },
    { VSND_TYP_DIZZY, "cr_stunned1", 0 },
    { VSND_TYP_DIZZY, "cr_stunned2", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_GhostDiver[73] = {
    { VSND_TYP_ATTACK, "gd_attack01", 0 },
    { VSND_TYP_ATTACK, "gd_attack02", 0 },
    { VSND_TYP_ATTACK, "gd_attack03", 0 },
    { VSND_TYP_ATTACK, "gd_attack04", 0 },
    { VSND_TYP_ATTACK, "gd_attack05", 0 },
    { VSND_TYP_ATTACK, "gd_attack06", 0 },
    { VSND_TYP_ATTACK, "gd_attack07", 0 },
    { VSND_TYP_SOAPED, "gd_bubble01", 0 },
    { VSND_TYP_SOAPED, "gd_bubble02", 0 },
    { VSND_TYP_SOAPED, "gd_bubble03", 0 },
    { VSND_TYP_SOAPED, "gd_bubble04", 0 },
    { VSND_TYP_SOAPED, "gd_bubble05", 0 },
    { VSND_TYP_SOAPED, "gd_bubble06", 0 },
    { VSND_TYP_SOAPED, "gd_bubble07", 0 },
    { VSND_TYP_SOAPED, "gd_bubble08", 0 },
    { VSND_TYP_DIE, "gd_death1", 0 },
    { VSND_TYP_DIE, "gd_death2", 0 },
    { VSND_TYP_DIE, "gd_death3", 0 },
    { VSND_TYP_DIE, "gd_death4", 0 },
    { VSND_TYP_DIE, "gd_death5", 0 },
    { VSND_TYP_DIE, "gd_death6", 0 },
    { VSND_TYP_DIE, "gd_death7", 0 },
    { VSND_TYP_LAUGH, "gd_gloat1", 0 },
    { VSND_TYP_LAUGH, "gd_gloat2", 0 },
    { VSND_TYP_LAUGH, "gd_gloat3", 0 },
    { VSND_TYP_LAUGH, "gd_gloat4", 0 },
    { VSND_TYP_LAUGH, "gd_gloat5", 0 },
    { VSND_TYP_GUMBOUNCE, "gd_gumbounce1", 0 },
    { VSND_TYP_GUMBOUNCE, "gd_gumbounce2", 0 },
    { VSND_TYP_GUMBOUNCE, "gd_gumbounce3", 0 },
    { VSND_TYP_OUCH, "gd_hit1", 0 },
    { VSND_TYP_OUCH, "gd_hit2", 0 },
    { VSND_TYP_OUCH, "gd_hit3", 0 },
    { VSND_TYP_OUCH, "gd_hit4", 0 },
    { VSND_TYP_OUCH, "gd_hit5", 0 },
    { VSND_TYP_OUCH, "gd_hit6", 0 },
    { VSND_TYP_IDLESLOP, "gd_idle01", 0 },
    { VSND_TYP_IDLESLOP, "gd_idle02", 0 },
    { VSND_TYP_IDLESLOP, "gd_idle3", 0 },
    { VSND_TYP_IDLESLOP, "gd_idle4", 0 },
    { VSND_TYP_LISTEN, "gd_listen1", 0 },
    { VSND_TYP_LISTEN, "gd_listen10", 0 },
    { VSND_TYP_LISTEN, "gd_listen11", 0 },
    { VSND_TYP_LISTEN, "gd_listen2", 0 },
    { VSND_TYP_LISTEN, "gd_listen3", 0 },
    { VSND_TYP_LISTEN, "gd_listen4", 0 },
    { VSND_TYP_LISTEN, "gd_listen5", 0 },
    { VSND_TYP_LISTEN, "gd_listen6", 0 },
    { VSND_TYP_LISTEN, "gd_listen7", 0 },
    { VSND_TYP_LISTEN, "gd_listen8", 0 },
    { VSND_TYP_LISTEN, "gd_listen9", 0 },
    { VSND_TYP_LOOK, "gd_look1", 0 },
    { VSND_TYP_LOOK, "gd_look2", 0 },
    { VSND_TYP_LOOK, "gd_look3", 0 },
    { VSND_TYP_CURIOUS, "gd_investigate1", 0 },
    { VSND_TYP_CURIOUS, "gd_investigate2", 0 },
    { VSND_TYP_CURIOUS, "gd_investigate3", 0 },
    { VSND_TYP_CURIOUS, "gd_investigate4", 0 },
    { VSND_TYP_CURIOUS, "gd_investigate5", 0 },
    { VSND_TYP_CURIOUS, "gd_investigate6", 0 },
    { VSND_TYP_EXCLAIM, "gd_surprise1", 0 },
    { VSND_TYP_EXCLAIM, "gd_surprise2", 0 },
    { VSND_TYP_EXCLAIM, "gd_surprise3", 0 },
    { VSND_TYP_EXCLAIM, "gd_surprised1", 0 },
    { VSND_TYP_EXCLAIM, "gd_surprised2", 0 },
    { VSND_TYP_EXCLAIM, "gd_surprised3", 0 },
    { VSND_TYP_DIZZY, "gd_stunned1", 0 },
    { VSND_TYP_DIZZY, "gd_stunned2", 0 },
    { VSND_TYP_DIZZY, "gd_stunned3", 0 },
    { VSND_TYP_DIZZY, "gd_stunned4", 0 },
    { VSND_TYP_DIZZY, "gd_stunned5", 0 },
    { VSND_TYP_DIZZY, "gd_stunned6", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_Gargoyle[61] = {
    { VSND_TYP_ATTACK, "gg_attack1", 0 },
    { VSND_TYP_ATTACK, "gg_attack2", 0 },
    { VSND_TYP_SOAPED, "gg_bubble1", 0 },
    { VSND_TYP_SOAPED, "gg_bubble2", 0 },
    { VSND_TYP_SOAPED, "gg_bubble3", 0 },
    { VSND_TYP_SOAPED, "gg_bubble4", 0 },
    { VSND_TYP_SOAPED, "gg_bubble5", 0 },
    { VSND_TYP_IDLESLOP, "ga_wingflap1", 0 },
    { VSND_TYP_CLANKING, "gg_chase1", 0 },
    { VSND_TYP_CLANKING, "gg_chase2", 0 },
    { VSND_TYP_CLANKING, "gg_chase3", 0 },
    { VSND_TYP_DIE, "gg_death1", 0 },
    { VSND_TYP_DIE, "gg_death2", 0 },
    { VSND_TYP_DIE, "gg_death3", 0 },
    { VSND_TYP_DIE, "gg_death4", 0 },
    { VSND_TYP_LAUGH, "gg_gloat1", 0 },
    { VSND_TYP_LAUGH, "gg_gloat2", 0 },
    { VSND_TYP_LAUGH, "gg_gloat3", 0 },
    { VSND_TYP_GUMBOUNCE, "gg_gumbounce1", 0 },
    { VSND_TYP_GUMBOUNCE, "gg_gumbounce2", 0 },
    { VSND_TYP_GUMMED, "gg_gummed1", 0 },
    { VSND_TYP_GUMMED, "gg_gummed2", 0 },
    { VSND_TYP_GUMMED, "gg_gummed3", 0 },
    { VSND_TYP_LISTEN, "gg_hear1", 0 },
    { VSND_TYP_LISTEN, "gg_hear2", 0 },
    { VSND_TYP_OUCH, "gg_hit1", 0 },
    { VSND_TYP_OUCH, "gg_hit2", 0 },
    { VSND_TYP_OUCH, "gg_hit3", 0 },
    { VSND_TYP_OUCH, "gg_hit4", 0 },
    { VSND_TYP_IDLESLOP, "gg_idle1", 0 },
    { VSND_TYP_IDLESLOP, "gg_idle2", 0 },
    { VSND_TYP_IDLESLOP, "gg_idle3", 0 },
    { VSND_TYP_IDLESLOP, "gg_idle4", 0 },
    { VSND_TYP_IDLESLOP, "gg_idle5", 0 },
    { VSND_TYP_IDLESLOP, "gg_idle6", 0 },
    { VSND_TYP_IDLESLOP, "gg_idle7", 0 },
    { VSND_TYP_CURIOUS, "gg_investigate1", 0 },
    { VSND_TYP_CURIOUS, "gg_investigate2", 0 },
    { VSND_TYP_CURIOUS, "gg_investigate3", 0 },
    { VSND_TYP_CLANKING, "gg_listen1", 0 },
    { VSND_TYP_CLANKING, "gg_listen2", 0 },
    { VSND_TYP_CLANKING, "gg_listen3", 0 },
    { VSND_TYP_CLANKING, "gg_listen4", 0 },
    { VSND_TYP_CLANKING, "gg_listen5", 0 },
    { VSND_TYP_CLANKING, "gg_looking1", 0 },
    { VSND_TYP_CLANKING, "gg_looking2", 0 },
    { VSND_TYP_CLANKING, "gg_see1", 0 },
    { VSND_TYP_CLANKING, "gg_see2", 0 },
    { VSND_TYP_EXCLAIM, "gg_see3", 0 },
    { VSND_TYP_DIZZY, "gg_stunned1", 0 },
    { VSND_TYP_EXCLAIM, "gg_surprise1", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball1", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball2", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball3", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball4", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball5", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball6", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball7", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball8", 0 },
    { VSND_TYP_PRJ_FLIGHT, "fireball_loop", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_Ghost[5] = {
    { VSND_TYP_MOVE, "gz_moan1", 0 },
    { VSND_TYP_MOVE, "gz_moan2", 0 },
    { VSND_TYP_MOVE, "gz_moan3", 0 },
    { VSND_TYP_MOVE, "gz_moan4", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_HeadlessSpecter[35] = {
    { VSND_TYP_ATTACK, "hs_attack01", 0 },
    { VSND_TYP_ATTACK, "hs_attack02", 0 },
    { VSND_TYP_ATTACK, "hs_attack03", 0 },
    { VSND_TYP_ATTACK, "hs_attack04", 0 },
    { VSND_TYP_ATTACK, "hs_attack05", 0 },
    { VSND_TYP_ATTACK, "hs_attack06", 0 },
    { VSND_TYP_ATTACK, "hs_attack07", 0 },
    { VSND_TYP_ATTACK, "hs_attack08", 0 },
    { VSND_TYP_ATTACK, "hs_attack09", 0 },
    { VSND_TYP_ATTACK, "hs_attack10", 0 },
    { VSND_TYP_SOAPED, "hs_bubble1", 0 },
    { VSND_TYP_SOAPED, "hs_bubble2", 0 },
    { VSND_TYP_SOAPED, "hs_bubble3", 0 },
    { VSND_TYP_LAUGH, "hs_gloat1", 0 },
    { VSND_TYP_LAUGH, "hs_gloat2", 0 },
    { VSND_TYP_LAUGH, "hs_gloat3", 0 },
    { VSND_TYP_LAUGH, "hs_gloat4", 0 },
    { VSND_TYP_LAUGH, "hs_gloat5", 0 },
    { VSND_TYP_LAUGH, "hs_gloat6", 0 },
    { VSND_TYP_LAUGH, "hs_gloat7", 0 },
    { VSND_TYP_OUCH, "hs_hit1", 0 },
    { VSND_TYP_OUCH, "hs_hit2", 0 },
    { VSND_TYP_OUCH, "hs_hit3", 0 },
    { VSND_TYP_OUCH, "hs_hit4", 0 },
    { VSND_TYP_OUCH, "hs_hit5", 0 },
    { VSND_TYP_OUCH, "hs_hit6", 0 },
    { VSND_TYP_IDLESLOP, "hs_idle1", 0 },
    { VSND_TYP_IDLESLOP, "hs_idle2", 0 },
    { VSND_TYP_IDLESLOP, "hs_idle3", 0 },
    { VSND_TYP_IDLESLOP, "hs_idle4", 0 },
    { VSND_TYP_IDLESLOP, "hs_idle5", 0 },
    { VSND_TYP_IDLESLOP, "hs_idle6", 0 },
    { VSND_TYP_IDLESLOP, "hs_idle7", 0 },
    { VSND_TYP_IDLESLOP, "hs_idle8", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_KillerPlant[7] = {
    { VSND_TYP_IDLESLOP, "flytrap_growl01", 0 },
    { VSND_TYP_IDLESLOP, "flytrap_growl02", 0 },
    { VSND_TYP_IDLESLOP, "flytrap_growl03", 0 },
    { VSND_TYP_IDLESLOP, "flytrap_growl04", 0 },
    { VSND_TYP_ATTACK, "flytrap_growl05", 0 },
    { VSND_TYP_IDLESLOP, "flytrap_growl06", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_SeaCreature[66] = {
    { VSND_TYP_ATTACK, "se_attack1", 0 },
    { VSND_TYP_ATTACK, "se_attack2", 0 },
    { VSND_TYP_ATTACK, "se_attack3", 0 },
    { VSND_TYP_SOAPED, "se_bubble1", 0 },
    { VSND_TYP_SOAPED, "se_bubble2", 0 },
    { VSND_TYP_SOAPED, "se_bubble3", 0 },
    { VSND_TYP_SOAPED, "se_bubble4", 0 },
    { VSND_TYP_SOAPED, "se_bubble5", 0 },
    { VSND_TYP_SOAPED, "se_bubble6", 0 },
    { VSND_TYP_CLANKING, "se_chase2", 0 },
    { VSND_TYP_CLANKING, "se_chase3", 0 },
    { VSND_TYP_CLANKING, "se_chase4", 0 },
    { VSND_TYP_CLANKING, "se_chase5", 0 },
    { VSND_TYP_CLANKING, "se_chase6", 0 },
    { VSND_TYP_CLANKING, "se_chase7", 0 },
    { VSND_TYP_CLANKING, "se_chase8", 0 },
    { VSND_TYP_CLANKING, "se_chase9", 0 },
    { VSND_TYP_DIE, "se_death1", 0 },
    { VSND_TYP_DIE, "se_death2", 0 },
    { VSND_TYP_LAUGH, "se_gloat1", 0 },
    { VSND_TYP_GUMBOUNCE, "se_gumbounce1", 0 },
    { VSND_TYP_GUMBOUNCE, "se_gumbounce2", 0 },
    { VSND_TYP_GUMBOUNCE, "se_gumbounce3", 0 },
    { VSND_TYP_GUMBOUNCE, "se_gumbounce4", 0 },
    { VSND_TYP_GUMBOUNCE, "se_gumbounce5", 0 },
    { VSND_TYP_GUMMED, "se_gummed1", 0 },
    { VSND_TYP_GUMMED, "se_gummed2", 0 },
    { VSND_TYP_GUMMED, "se_gummed3", 0 },
    { VSND_TYP_GUMMED, "se_gummed4", 0 },
    { VSND_TYP_GUMMED, "se_gummed5", 0 },
    { VSND_TYP_LISTEN, "se_hear1", 0 },
    { VSND_TYP_LISTEN, "se_hear2", 0 },
    { VSND_TYP_LISTEN, "se_hear3", 0 },
    { VSND_TYP_LISTEN, "se_hear4", 0 },
    { VSND_TYP_LISTEN, "se_hear5", 0 },
    { VSND_TYP_LISTEN, "se_listen1", 0 },
    { VSND_TYP_LISTEN, "se_listen2", 0 },
    { VSND_TYP_LISTEN, "se_listen3", 0 },
    { VSND_TYP_LISTEN, "se_listen4", 0 },
    { VSND_TYP_LISTEN, "se_listen5", 0 },
    { VSND_TYP_OUCH, "se_hit1", 0 },
    { VSND_TYP_OUCH, "se_hit2", 0 },
    { VSND_TYP_OUCH, "se_hit3", 0 },
    { VSND_TYP_OUCH, "se_hit4", 0 },
    { VSND_TYP_IDLESLOP, "se_idle1", 0 },
    { VSND_TYP_IDLESLOP, "se_idle2", 0 },
    { VSND_TYP_IDLESLOP, "se_idle3", 0 },
    { VSND_TYP_IDLESLOP, "se_idle4", 0 },
    { VSND_TYP_IDLESLOP, "se_idle5", 0 },
    { VSND_TYP_CURIOUS, "se_investigate1", 0 },
    { VSND_TYP_LOOK, "se_look1", 0 },
    { VSND_TYP_LOOK, "se_look2", 0 },
    { VSND_TYP_LOOK, "se_look3", 0 },
    { VSND_TYP_LOOK, "se_look4", 0 },
    { VSND_TYP_LOOK, "se_look5", 0 },
    { VSND_TYP_LOOK, "se_look6", 0 },
    { VSND_TYP_LOOK, "se_look7", 0 },
    { VSND_TYP_LOOK, "se_look8", 0 },
    { VSND_TYP_EXCLAIM, "se_see1", 0 },
    { VSND_TYP_EXCLAIM, "se_see2", 0 },
    { VSND_TYP_EXCLAIM, "se_see3", 0 },
    { VSND_TYP_EXCLAIM, "se_see4", 0 },
    { VSND_TYP_EXCLAIM, "se_surprise1", 0 },
    { VSND_TYP_DIZZY, "se_stunned1", 0 },
    { VSND_TYP_DIZZY, "se_stunned2", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_ScareCrow[78] = {
    { VSND_TYP_ATTACK, "sc_attack1", 0 },
    { VSND_TYP_ATTACK, "sc_attack2", 0 },
    { VSND_TYP_ATTACK, "sc_attack3", 0 },
    { VSND_TYP_ATTACK, "sc_attack4", 0 },
    { VSND_TYP_SOAPED, "sc_bubble1", 0 },
    { VSND_TYP_SOAPED, "sc_bubble2", 0 },
    { VSND_TYP_SOAPED, "sc_bubble3", 0 },
    { VSND_TYP_SOAPED, "sc_bubble4", 0 },
    { VSND_TYP_SOAPED, "sc_bubble5", 0 },
    { VSND_TYP_SOAPED, "sc_bubble6", 0 },
    { VSND_TYP_CLANKING, "sc_chase1", 0 },
    { VSND_TYP_CLANKING, "sc_chase2", 0 },
    { VSND_TYP_CLANKING, "sc_chase3", 0 },
    { VSND_TYP_CLANKING, "sc_chase4", 0 },
    { VSND_TYP_CLANKING, "sc_chase5", 0 },
    { VSND_TYP_DIE, "sc_death1", 0 },
    { VSND_TYP_DIE, "sc_death2", 0 },
    { VSND_TYP_DIE, "sc_death3", 0 },
    { VSND_TYP_DIE, "sc_death4", 0 },
    { VSND_TYP_DIE, "sc_death5", 0 },
    { VSND_TYP_DIE, "sc_death6", 0 },
    { VSND_TYP_LAUGH, "sc_gloat1", 0 },
    { VSND_TYP_LAUGH, "sc_gloat2", 0 },
    { VSND_TYP_LAUGH, "sc_gloat3", 0 },
    { VSND_TYP_LAUGH, "sc_gloat4", 0 },
    { VSND_TYP_LAUGH, "sc_gloat5", 0 },
    { VSND_TYP_LAUGH, "sc_gloat6", 0 },
    { VSND_TYP_LAUGH, "sc_gloat7", 0 },
    { VSND_TYP_GUMBOUNCE, "sc_gumbounce1", 0 },
    { VSND_TYP_GUMBOUNCE, "sc_gumbounce2", 0 },
    { VSND_TYP_GUMBOUNCE, "sc_gumbounce3", 0 },
    { VSND_TYP_GUMBOUNCE, "sc_gumbounce4", 0 },
    { VSND_TYP_GUMBOUNCE, "sc_gumbounce5", 0 },
    { VSND_TYP_GUMMED, "sc_gummed1", 0 },
    { VSND_TYP_GUMMED, "sc_gummed2", 0 },
    { VSND_TYP_GUMMED, "sc_gummed3", 0 },
    { VSND_TYP_GUMMED, "sc_gummed4", 0 },
    { VSND_TYP_GUMMED, "sc_gummed5", 0 },
    { VSND_TYP_GUMMED, "sc_gummed6", 0 },
    { VSND_TYP_GUMMED, "sc_gummed7", 0 },
    { VSND_TYP_LISTEN, "sc_hear1", 0 },
    { VSND_TYP_LISTEN, "sc_hear2", 0 },
    { VSND_TYP_LISTEN, "sc_hear3", 0 },
    { VSND_TYP_LISTEN, "sc_hear4", 0 },
    { VSND_TYP_LISTEN, "sc_hear5", 0 },
    { VSND_TYP_OUCH, "sc_hit1", 0 },
    { VSND_TYP_OUCH, "sc_hit2", 0 },
    { VSND_TYP_OUCH, "sc_hit3", 0 },
    { VSND_TYP_OUCH, "sc_hit4", 0 },
    { VSND_TYP_OUCH, "sc_hit5", 0 },
    { VSND_TYP_IDLESLOP, "sc_idle1", 0 },
    { VSND_TYP_IDLESLOP, "sc_idle2", 0 },
    { VSND_TYP_IDLESLOP, "sc_idle3", 0 },
    { VSND_TYP_IDLESLOP, "sc_idle4", 0 },
    { VSND_TYP_IDLESLOP, "sc_idle5", 0 },
    { VSND_TYP_IDLESLOP, "sc_idle6", 0 },
    { VSND_TYP_CURIOUS, "sc_investigate1", 0 },
    { VSND_TYP_CURIOUS, "sc_investigate2", 0 },
    { VSND_TYP_CURIOUS, "sc_investigate3", 0 },
    { VSND_TYP_LISTEN, "sc_listen1", 0 },
    { VSND_TYP_LISTEN, "sc_listen2", 0 },
    { VSND_TYP_LISTEN, "sc_listen3", 0 },
    { VSND_TYP_LISTEN, "sc_listen4", 0 },
    { VSND_TYP_LISTEN, "sc_listen5", 0 },
    { VSND_TYP_LISTEN, "sc_listen6", 0 },
    { VSND_TYP_LOOK, "sc_look1", 0 },
    { VSND_TYP_LOOK, "sc_look2", 0 },
    { VSND_TYP_LOOK, "sc_look3", 0 },
    { VSND_TYP_LOOK, "sc_look4", 0 },
    { VSND_TYP_LOOK, "sc_see1", 0 },
    { VSND_TYP_LOOK, "sc_see2", 0 },
    { VSND_TYP_LOOK, "sc_see3", 0 },
    { VSND_TYP_LOOK, "sc_see4", 0 },
    { VSND_TYP_LOOK, "sc_see5", 0 },
    { VSND_TYP_DIZZY, "sc_stunned1", 0 },
    { VSND_TYP_DIZZY, "sc_stunned2", 0 },
    { VSND_TYP_EXCLAIM, "sc_surprise1", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_Shark[17] = {
    { VSND_TYP_MOVE, "sa_swim1", 0 },
    { VSND_TYP_MOVE, "sa_swim2", 0 },
    { VSND_TYP_MOVE, "sa_swim3", 0 },
    { VSND_TYP_MOVE, "sa_swim4", 0 },
    { VSND_TYP_MOVE, "sa_swim5", 0 },
    { VSND_TYP_MOVE, "sa_swim6", 0 },
    { VSND_TYP_MOVE, "sa_swim7", 0 },
    { VSND_TYP_MOVE, "sa_swim8", 0 },
    { VSND_TYP_MOVE, "sa_swim8", 0 },
    { VSND_TYP_MOVE_ALT, "sa_bite1", 0 },
    { VSND_TYP_MOVE_ALT, "sa_bite2", 0 },
    { VSND_TYP_SPLISH, "sa_jump1", 0 },
    { VSND_TYP_SPLISH, "sa_jump2", 0 },
    { VSND_TYP_SPLASH, "sa_splash1", 0 },
    { VSND_TYP_SPLASH, "sa_splash2", 0 },
    { VSND_TYP_SPLASH, "sa_splash3", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_Rat[6] = {
    { VSND_TYP_MOVE, "rat_scury1", 0 },
    { VSND_TYP_MOVE, "rat_scury1", 0 },
    { VSND_TYP_DIE, "rat_squeak01", 0 },
    { VSND_TYP_DIE, "rat_smash1", 0 },
    { VSND_TYP_DIE, "rat_smash2", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_Robot[65] = {
    { VSND_TYP_ATTACK, "oldsk_attack1", 0 },
    { VSND_TYP_ATTACK, "oldsk_attack2", 0 },
    { VSND_TYP_ATTACK, "oldsk_attack3", 0 },
    { VSND_TYP_ATTACK, "oldsk_attack4", 0 },
    { VSND_TYP_ATTACK, "oldsk_attack5", 0 },
    { VSND_TYP_ATTACK, "oldsk_attack6", 0 },
    { VSND_TYP_SOAPED, "oldsk_bubble1", 0 },
    { VSND_TYP_SOAPED, "oldsk_bubble2", 0 },
    { VSND_TYP_SOAPED, "oldsk_bubble3", 0 },
    { VSND_TYP_SOAPED, "oldsk_bubble4", 0 },
    { VSND_TYP_DIE, "oldsk_death1", 0 },
    { VSND_TYP_DIE, "oldsk_death2", 0 },
    { VSND_TYP_LAUGH, "oldsk_chase1", 0 },
    { VSND_TYP_LAUGH, "oldsk_chase2", 0 },
    { VSND_TYP_LAUGH, "oldsk_gloat1", 0 },
    { VSND_TYP_LAUGH, "oldsk_gloat2", 0 },
    { VSND_TYP_LAUGH, "oldsk_gloat3", 0 },
    { VSND_TYP_LAUGH, "oldsk_gloat4", 0 },
    { VSND_TYP_LAUGH, "oldsk_gloat5", 0 },
    { VSND_TYP_LAUGH, "oldsk_gloat6", 0 },
    { VSND_TYP_LAUGH, "oldsk_gloat7", 0 },
    { VSND_TYP_LAUGH, "oldsk_gloat8", 0 },
    { VSND_TYP_LAUGH, "oldsk_gloat9", 0 },
    { VSND_TYP_GUMBOUNCE, "oldsk_gumbounce1", 0 },
    { VSND_TYP_GUMBOUNCE, "oldsk_gumbounce2", 0 },
    { VSND_TYP_GUMMED, "oldsk_gummed1", 0 },
    { VSND_TYP_GUMMED, "oldsk_gummed2", 0 },
    { VSND_TYP_GUMMED, "oldsk_gummed3", 0 },
    { VSND_TYP_GUMMED, "oldsk_gummed4", 0 },
    { VSND_TYP_GUMMED, "oldsk_gummed5", 0 },
    { VSND_TYP_LISTEN, "oldsk_hear1", 0 },
    { VSND_TYP_LISTEN, "oldsk_hear2", 0 },
    { VSND_TYP_LISTEN, "oldsk_hear3", 0 },
    { VSND_TYP_LISTEN, "oldsk_hear4", 0 },
    { VSND_TYP_LISTEN, "oldsk_hear5", 0 },
    { VSND_TYP_OUCH, "oldsk_hit1", 0 },
    { VSND_TYP_OUCH, "oldsk_hit2", 0 },
    { VSND_TYP_OUCH, "oldsk_hit3", 0 },
    { VSND_TYP_OUCH, "oldsk_hit4", 0 },
    { VSND_TYP_CURIOUS, "oldsk_investigate1", 0 },
    { VSND_TYP_CURIOUS, "oldsk_investigate2", 0 },
    { VSND_TYP_CURIOUS, "oldsk_investigate3", 0 },
    { VSND_TYP_CURIOUS, "oldsk_investigate4", 0 },
    { VSND_TYP_CURIOUS, "oldsk_investigate5", 0 },
    { VSND_TYP_CURIOUS, "oldsk_investigate6", 0 },
    { VSND_TYP_CURIOUS, "oldsk_investigate7", 0 },
    { VSND_TYP_CURIOUS, "oldsk_investigate8", 0 },
    { VSND_TYP_LISTEN, "oldsk_listen1", 0 },
    { VSND_TYP_LOOK, "oldsk_look1", 0 },
    { VSND_TYP_LOOK, "oldsk_look2", 0 },
    { VSND_TYP_LOOK, "oldsk_look3", 0 },
    { VSND_TYP_EXCLAIM, "oldsk_see1", 0 },
    { VSND_TYP_EXCLAIM, "oldsk_see10", 0 },
    { VSND_TYP_EXCLAIM, "oldsk_see2", 0 },
    { VSND_TYP_EXCLAIM, "oldsk_see3", 0 },
    { VSND_TYP_EXCLAIM, "oldsk_see4", 0 },
    { VSND_TYP_EXCLAIM, "oldsk_see5", 0 },
    { VSND_TYP_EXCLAIM, "oldsk_see6", 0 },
    { VSND_TYP_EXCLAIM, "oldsk_see7", 0 },
    { VSND_TYP_EXCLAIM, "oldsk_see8", 0 },
    { VSND_TYP_EXCLAIM, "oldsk_see9", 0 },
    { VSND_TYP_DIZZY, "oldsk_stunned2", 0 },
    { VSND_TYP_EXCLAIM, "oldsk_surprise1", 0 },
    { VSND_TYP_EXCLAIM, "oldsk_surprise2", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_SpaceKook[25] = {
    { VSND_TYP_DIE, "sk_death1", 0 },
    { VSND_TYP_DIE, "sk_death2", 0 },
    { VSND_TYP_MOVE, "robot_loop", 0 },
    { VSND_TYP_GUMMED, "cm_attack1", 0 },
    { VSND_TYP_GUMMED, "cm_attack2", 0 },
    { VSND_TYP_SOAPED, "cm_attack3", 0 },
    { VSND_TYP_SOAPED, "cm_attack4", 0 },
    { VSND_TYP_EXCLAIM, "sk_gloat1", 0 },
    { VSND_TYP_EXCLAIM, "sk_gloat2", 0 },
    { VSND_TYP_EXCLAIM, "sk_gloat3", 0 },
    { VSND_TYP_EXCLAIM, "sk_gloat4", 0 },
    { VSND_TYP_EXCLAIM, "sk_gloat5", 0 },
    { VSND_TYP_EXCLAIM, "sk_gloat6", 0 },
    { VSND_TYP_EXCLAIM, "sk_gloat7", 0 },
    { VSND_TYP_SPELL_CAST, "laser1", 0 },
    { VSND_TYP_SPELL_CAST, "laser2", 0 },
    { VSND_TYP_SPELL_CAST, "laser3", 0 },
    { VSND_TYP_SPELL_CAST, "laser4", 0 },
    { VSND_TYP_SPELL_CAST, "laser5", 0 },
    { VSND_TYP_SPELL_CAST, "laser6", 0 },
    { VSND_TYP_SPELL_CAST, "laser7", 0 },
    { VSND_TYP_SPELL_LOOP, "laser_loop1", 0 },
    { VSND_TYP_SPELL_LOOP, "laser_loop2", 0 },
    { VSND_TYP_SPELL_LOOP, "laser_loop3", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_TarMonster[88] = {
    { VSND_TYP_ATTACK, "tm_attack1", 0 },
    { VSND_TYP_ATTACK, "tm_attack2", 0 },
    { VSND_TYP_ATTACK, "tm_attack3", 0 },
    { VSND_TYP_ATTACK, "tm_attack4", 0 },
    { VSND_TYP_ATTACK, "tm_attack5", 0 },
    { VSND_TYP_ATTACK, "tm_attack6", 0 },
    { VSND_TYP_ATTACK, "tm_attack7", 0 },
    { VSND_TYP_SOAPED, "tm_bubble1", 0 },
    { VSND_TYP_SOAPED, "tm_bubble10", 0 },
    { VSND_TYP_SOAPED, "tm_bubble2", 0 },
    { VSND_TYP_SOAPED, "tm_bubble3", 0 },
    { VSND_TYP_SOAPED, "tm_bubble4", 0 },
    { VSND_TYP_SOAPED, "tm_bubble5", 0 },
    { VSND_TYP_SOAPED, "tm_bubble6", 0 },
    { VSND_TYP_SOAPED, "tm_bubble7", 0 },
    { VSND_TYP_SOAPED, "tm_bubble8", 0 },
    { VSND_TYP_SOAPED, "tm_bubble9", 0 },
    { VSND_TYP_DIE, "tm_death1", 0 },
    { VSND_TYP_DIE, "tm_death2", 0 },
    { VSND_TYP_DIE, "tm_death3", 0 },
    { VSND_TYP_DIE, "tm_death4", 0 },
    { VSND_TYP_DIE, "tm_death5", 0 },
    { VSND_TYP_DIE, "tm_death6", 0 },
    { VSND_TYP_DIE, "tm_death7", 0 },
    { VSND_TYP_DIE, "tm_death8", 0 },
    { VSND_TYP_LAUGH, "tm_gloat1", 0 },
    { VSND_TYP_LAUGH, "tm_gloat10", 0 },
    { VSND_TYP_LAUGH, "tm_gloat2", 0 },
    { VSND_TYP_LAUGH, "tm_gloat3", 0 },
    { VSND_TYP_LAUGH, "tm_gloat4", 0 },
    { VSND_TYP_LAUGH, "tm_gloat5", 0 },
    { VSND_TYP_LAUGH, "tm_gloat6", 0 },
    { VSND_TYP_LAUGH, "tm_gloat7", 0 },
    { VSND_TYP_LAUGH, "tm_gloat8", 0 },
    { VSND_TYP_LAUGH, "tm_gloat9", 0 },
    { VSND_TYP_GUMBOUNCE, "tm_gumbounce1", 0 },
    { VSND_TYP_GUMBOUNCE, "tm_gumbounce2", 0 },
    { VSND_TYP_GUMBOUNCE, "tm_gumbounce3", 0 },
    { VSND_TYP_GUMBOUNCE, "tm_gumbounce4", 0 },
    { VSND_TYP_GUMBOUNCE, "tm_gumbounce5", 0 },
    { VSND_TYP_GUMBOUNCE, "tm_gumbounce6", 0 },
    { VSND_TYP_GUMMED, "tm_gummed1", 0 },
    { VSND_TYP_GUMMED, "tm_gummed2", 0 },
    { VSND_TYP_GUMMED, "tm_gummed3", 0 },
    { VSND_TYP_IDLESLOP, "tm_idle1", 0 },
    { VSND_TYP_IDLESLOP, "tm_idle2", 0 },
    { VSND_TYP_IDLESLOP, "tm_idle3", 0 },
    { VSND_TYP_IDLESLOP, "tm_idle4", 0 },
    { VSND_TYP_IDLESLOP, "tm_idle5", 0 },
    { VSND_TYP_IDLESLOP, "tm_idle6", 0 },
    { VSND_TYP_IDLESLOP, "tm_idle7", 0 },
    { VSND_TYP_CLANKING, "tm_puddle1", 0 },
    { VSND_TYP_CLANKING, "tm_puddlewalk1", 0 },
    { VSND_TYP_CURIOUS, "tm_investigate1", 0 },
    { VSND_TYP_CURIOUS, "tm_investigate2", 0 },
    { VSND_TYP_CURIOUS, "tm_investigate3", 0 },
    { VSND_TYP_CURIOUS, "tm_investigate4", 0 },
    { VSND_TYP_CURIOUS, "tm_investigate5", 0 },
    { VSND_TYP_CURIOUS, "tm_investigate6", 0 },
    { VSND_TYP_CURIOUS, "tm_investigate7", 0 },
    { VSND_TYP_CURIOUS, "tm_investigate8", 0 },
    { VSND_TYP_CURIOUS, "tm_investigate9", 0 },
    { VSND_TYP_LISTEN, "tm_listen1", 0 },
    { VSND_TYP_LISTEN, "tm_listen2", 0 },
    { VSND_TYP_LISTEN, "tm_listen3", 0 },
    { VSND_TYP_LISTEN, "tm_listen4", 0 },
    { VSND_TYP_LISTEN, "tm_listen5", 0 },
    { VSND_TYP_LISTEN, "tm_listen6", 0 },
    { VSND_TYP_LISTEN, "tm_listen7", 0 },
    { VSND_TYP_LISTEN, "tm_listen8", 0 },
    { VSND_TYP_LISTEN, "tm_listen9", 0 },
    { VSND_TYP_LOOK, "tm_look1", 0 },
    { VSND_TYP_LOOK, "tm_look2", 0 },
    { VSND_TYP_LOOK, "tm_look3", 0 },
    { VSND_TYP_LOOK, "tm_look4", 0 },
    { VSND_TYP_EXCLAIM, "tm_surprised1", 0 },
    { VSND_TYP_EXCLAIM, "tm_surprised2", 0 },
    { VSND_TYP_EXCLAIM, "tm_surprised3", 0 },
    { VSND_TYP_EXCLAIM, "tm_surprised4", 0 },
    { VSND_TYP_EXCLAIM, "tm_surprised5", 0 },
    { VSND_TYP_EXCLAIM, "tm_surprised6", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball2", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball6", 0 },
    { VSND_TYP_PRJ_FLIGHT, "fireball_loop", 0 },
    { VSND_TYP_PRJ_EXPLODE, "tar_ball_hit1", 0 },
    { VSND_TYP_PRJ_SPLAT, "tar_ball_splat1", 0 },
    { VSND_TYP_PRJ_SPLAT, "tar_ball_splat2", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_Witch[74] = {
    { VSND_TYP_ATTACK, "wc_attack1", 0 },
    { VSND_TYP_ATTACK, "wc_attack2", 0 },
    { VSND_TYP_ATTACK, "wc_attack3", 0 },
    { VSND_TYP_ATTACK, "wc_attack4", 0 },
    { VSND_TYP_ATTACK, "wc_attack5", 0 },
    { VSND_TYP_ATTACK, "wc_attack6", 0 },
    { VSND_TYP_SOAPED, "wc_bubble1", 0 },
    { VSND_TYP_SOAPED, "wc_bubble2", 0 },
    { VSND_TYP_SOAPED, "wc_bubble3", 0 },
    { VSND_TYP_SOAPED, "wc_bubble4", 0 },
    { VSND_TYP_SOAPED, "wc_bubble5", 0 },
    { VSND_TYP_DIE, "wc_death1", 0 },
    { VSND_TYP_DIE, "wc_death2", 0 },
    { VSND_TYP_DIE, "wc_death3", 0 },
    { VSND_TYP_DIE, "wc_deathhit1", 0 },
    { VSND_TYP_DIE, "wc_deathhit2", 0 },
    { VSND_TYP_DIE, "wc_deathhit3", 0 },
    { VSND_TYP_DIE, "wc_deathhit4", 0 },
    { VSND_TYP_DIE, "wc_deathspin1", 0 },
    { VSND_TYP_DIE, "wc_deathspin2", 0 },
    { VSND_TYP_DIE, "wc_deathspin3", 0 },
    { VSND_TYP_LAUGH, "wc_gloat1", 0 },
    { VSND_TYP_LAUGH, "wc_gloat2", 0 },
    { VSND_TYP_LAUGH, "wc_gloat3", 0 },
    { VSND_TYP_LAUGH, "wc_gloat4", 0 },
    { VSND_TYP_GUMBOUNCE, "wc_gumbounce1", 0 },
    { VSND_TYP_GUMBOUNCE, "wc_gumbounce2", 0 },
    { VSND_TYP_GUMBOUNCE, "wc_gumbounce3", 0 },
    { VSND_TYP_GUMBOUNCE, "wc_gumbounce4", 0 },
    { VSND_TYP_GUMMED, "wc_gummed1", 0 },
    { VSND_TYP_GUMMED, "wc_gummed2", 0 },
    { VSND_TYP_GUMMED, "wc_gummed3", 0 },
    { VSND_TYP_GUMMED, "wc_gummed4", 0 },
    { VSND_TYP_GUMMED, "wc_gummed5", 0 },
    { VSND_TYP_IDLESLOP, "wc_idle1", 0 },
    { VSND_TYP_IDLESLOP, "wc_idle2", 0 },
    { VSND_TYP_IDLESLOP, "wc_idle3", 0 },
    { VSND_TYP_IDLESLOP, "wc_idle4", 0 },
    { VSND_TYP_IDLESLOP, "wc_idle5", 0 },
    { VSND_TYP_CURIOUS, "wc_investigate1", 0 },
    { VSND_TYP_CURIOUS, "wc_investigate2", 0 },
    { VSND_TYP_CURIOUS, "wc_investigate3", 0 },
    { VSND_TYP_CURIOUS, "wc_investigate4", 0 },
    { VSND_TYP_CURIOUS, "wc_investigate5", 0 },
    { VSND_TYP_LISTEN, "wc_listen1", 0 },
    { VSND_TYP_LISTEN, "wc_listen2", 0 },
    { VSND_TYP_LISTEN, "wc_listen3", 0 },
    { VSND_TYP_LISTEN, "wc_listen4", 0 },
    { VSND_TYP_LISTEN, "wc_listen5", 0 },
    { VSND_TYP_LISTEN, "wc_listen6", 0 },
    { VSND_TYP_LISTEN, "wc_listen7", 0 },
    { VSND_TYP_LISTEN, "wc_listen8", 0 },
    { VSND_TYP_LOOK, "wc_look1", 0 },
    { VSND_TYP_LOOK, "wc_look2", 0 },
    { VSND_TYP_LOOK, "wc_look3", 0 },
    { VSND_TYP_LOOK, "wc_look4", 0 },
    { VSND_TYP_EXCLAIM, "wc_surprised1", 0 },
    { VSND_TYP_EXCLAIM, "wc_surprised2", 0 },
    { VSND_TYP_EXCLAIM, "wc_surprised3", 0 },
    { VSND_TYP_EXCLAIM, "wc_surprised4", 0 },
    { VSND_TYP_MOVE, "wc_move1", 0 },
    { VSND_TYP_MOVE, "wc_move2", 0 },
    { VSND_TYP_MOVE, "wc_move3", 0 },
    { VSND_TYP_MOVE, "wc_move4", 0 },
    { VSND_TYP_MOVE, "wc_move5", 0 },
    { VSND_TYP_MOVE, "wc_move6", 0 },
    { VSND_TYP_SNORE, "wc_sleep1", 0 },
    { VSND_TYP_SNORE, "wc_sleep2", 0 },
    { VSND_TYP_SNORE, "wc_sleep3", 0 },
    { VSND_TYP_SNORE, "wc_sleep4", 0 },
    { VSND_TYP_SNORE, "wc_sleep5", 0 },
    { VSND_TYP_SPELL_CAST, "magic_wand4", 0 },
    { VSND_TYP_SPELL_PLAY, "magic_wand5", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_WitchDoctor[53] = {
    { VSND_TYP_ATTACK, "wd_attack1", 0 },
    { VSND_TYP_ATTACK, "wd_attack2", 0 },
    { VSND_TYP_ATTACK, "wd_attack3", 0 },
    { VSND_TYP_ATTACK, "wd_attack4", 0 },
    { VSND_TYP_ATTACK, "wd_attack5", 0 },
    { VSND_TYP_ATTACK, "wd_attack6", 0 },
    { VSND_TYP_ATTACK, "wd_attack7", 0 },
    { VSND_TYP_ATTACK, "wd_attack8", 0 },
    { VSND_TYP_MOVE_ALT, "wd_chase1", 0 },
    { VSND_TYP_MOVE_ALT, "wd_chase2", 0 },
    { VSND_TYP_DIE, "wd_death1", 0 },
    { VSND_TYP_DIE, "wd_death2", 0 },
    { VSND_TYP_DIE, "wd_death3", 0 },
    { VSND_TYP_LAUGH, "wd_gloat1", 0 },
    { VSND_TYP_LAUGH, "wd_gloat2", 0 },
    { VSND_TYP_LAUGH, "wd_gloat3", 0 },
    { VSND_TYP_LAUGH, "wd_gloat4", 0 },
    { VSND_TYP_LAUGH, "wd_gloat5", 0 },
    { VSND_TYP_GUMMED, "wd_gummed1", 0 },
    { VSND_TYP_GUMMED, "wd_gummed2", 0 },
    { VSND_TYP_GUMMED, "wd_gummed3", 0 },
    { VSND_TYP_LISTEN, "wd_hear1", 0 },
    { VSND_TYP_LISTEN, "wd_hear2", 0 },
    { VSND_TYP_OUCH, "wd_hurt1", 0 },
    { VSND_TYP_OUCH, "wd_hurt2", 0 },
    { VSND_TYP_OUCH, "wd_hurt3", 0 },
    { VSND_TYP_LISTEN, "wd_listen1", 0 },
    { VSND_TYP_LISTEN, "wd_listen2", 0 },
    { VSND_TYP_LISTEN, "wd_listen3", 0 },
    { VSND_TYP_LOOK, "wd_look1", 0 },
    { VSND_TYP_LOOK, "wd_look2", 0 },
    { VSND_TYP_LOOK, "wd_see1", 0 },
    { VSND_TYP_LOOK, "wd_see2", 0 },
    { VSND_TYP_LOOK, "wd_see3", 0 },
    { VSND_TYP_LOOK, "wd_see4", 0 },
    { VSND_TYP_LOOK, "wd_see5", 0 },
    { VSND_TYP_LOOK, "wd_see6", 0 },
    { VSND_TYP_DIZZY, "wd_stunned1", 0 },
    { VSND_TYP_IDLESLOP, "wd_staff_shake", 0 },
    { VSND_TYP_EXCLAIM, "wd_surprise1", 0 },
    { VSND_TYP_EXCLAIM, "wd_surprise2", 0 },
    { VSND_TYP_EXCLAIM, "wd_surprise3", 0 },
    { VSND_TYP_EXCLAIM, "wd_surprise4", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball1", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball2", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball3", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball4", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball5", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball6", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball7", 0 },
    { VSND_TYP_PRJ_LAUNCH, "fireball8", 0 },
    { VSND_TYP_PRJ_FLIGHT, "fireball_loop", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_Wolfman[39] = {
    { VSND_TYP_ATTACK, "wm_attack1", 0 },
    { VSND_TYP_ATTACK, "wm_attack2", 0 },
    { VSND_TYP_ATTACK, "wm_attack3", 0 },
    { VSND_TYP_ATTACK, "wm_attack4", 0 },
    { VSND_TYP_ATTACK, "wm_attack5", 0 },
    { VSND_TYP_ATTACK, "wm_attack6", 0 },
    { VSND_TYP_SOAPED, "wm_bubble1", 0 },
    { VSND_TYP_SOAPED, "wm_bubble2", 0 },
    { VSND_TYP_SOAPED, "wm_bubble3", 0 },
    { VSND_TYP_SOAPED, "wm_bubble4", 0 },
    { VSND_TYP_LAUGH, "wm_chase1", 0 },
    { VSND_TYP_LAUGH, "wm_chase2", 0 },
    { VSND_TYP_LAUGH, "wm_chase3", 0 },
    { VSND_TYP_LAUGH, "wm_chase4", 0 },
    { VSND_TYP_LAUGH, "wm_chase5", 0 },
    { VSND_TYP_LAUGH, "wm_chase6", 0 },
    { VSND_TYP_DIE, "wm_death1", 0 },
    { VSND_TYP_DIE, "wm_death2", 0 },
    { VSND_TYP_DIE, "wm_death3", 0 },
    { VSND_TYP_DIE, "wm_death4", 0 },
    { VSND_TYP_DIE, "wm_death5", 0 },
    { VSND_TYP_DIE, "wm_death6", 0 },
    { VSND_TYP_DIE, "wm_death7", 0 },
    { VSND_TYP_GUMBOUNCE, "wm_gumbounce1", 0 },
    { VSND_TYP_GUMMED, "wm_gummed1", 0 },
    { VSND_TYP_CLANKING, "wm_growl1", 0 },
    { VSND_TYP_IDLESLOP, "wm_idle1", 0 },
    { VSND_TYP_IDLESLOP, "wm_idle2", 0 },
    { VSND_TYP_IDLESLOP, "wm_idle3", 0 },
    { VSND_TYP_IDLESLOP, "wm_idle4", 0 },
    { VSND_TYP_IDLESLOP, "wm_idle5", 0 },
    { VSND_TYP_MOVE, "wm_walk1", 0 },
    { VSND_TYP_MOVE, "wm_walk2", 0 },
    { VSND_TYP_MOVE, "wm_walk3", 0 },
    { VSND_TYP_MOVE, "wm_walk4", 0 },
    { VSND_TYP_MOVE, "wm_walk5", 0 },
    { VSND_TYP_MOVE, "wm_walk6", 0 },
    { VSND_TYP_MOVE, "wm_walk7", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_RECORDS g_sndTrax_Zombie[32] = {
    { VSND_TYP_ATTACK, "zz_attack1", 0 },
    { VSND_TYP_ATTACK, "zz_attack2", 0 },
    { VSND_TYP_ATTACK, "zz_attack3", 0 },
    { VSND_TYP_ATTACK, "zz_attack4", 0 },
    { VSND_TYP_ATTACK, "zz_attack5", 0 },
    { VSND_TYP_DIE, "zz_death1", 0 },
    { VSND_TYP_DIE, "zz_death2", 0 },
    { VSND_TYP_DIE, "zz_death3", 0 },
    { VSND_TYP_DIE, "zz_death4", 0 },
    { VSND_TYP_DIE, "zz_death5", 0 },
    { VSND_TYP_LAUGH, "zz_gloat1", 0 },
    { VSND_TYP_LAUGH, "zz_gloat2", 0 },
    { VSND_TYP_LAUGH, "zz_gloat3", 0 },
    { VSND_TYP_LAUGH, "zz_gloat4", 0 },
    { VSND_TYP_LAUGH, "zz_gloat5", 0 },
    { VSND_TYP_LAUGH, "zz_gloat6", 0 },
    { VSND_TYP_LAUGH, "zz_gloat7", 0 },
    { VSND_TYP_LISTEN, "zz_hear1", 0 },
    { VSND_TYP_OUCH, "zz_hurt1", 0 },
    { VSND_TYP_OUCH, "zz_hurt2", 0 },
    { VSND_TYP_OUCH, "zz_hurt3", 0 },
    { VSND_TYP_IDLESLOP, "zz_idle1", 0 },
    { VSND_TYP_IDLESLOP, "zz_idle2", 0 },
    { VSND_TYP_CURIOUS, "zz_investigate1", 0 },
    { VSND_TYP_CURIOUS, "zz_investigate2", 0 },
    { VSND_TYP_CURIOUS, "zz_investigate3", 0 },
    { VSND_TYP_LISTEN, "zz_listen1", 0 },
    { VSND_TYP_DIZZY, "zz_stunned", 0 },
    { VSND_TYP_DIZZY, "zz_headshake1", 0 },
    { VSND_TYP_DIZZY, "zz_headshake2", 0 },
    { VSND_TYP_DIZZY, "zz_headshake3", 0 },
    { VSND_LISTEND, NULL, 0 },
};

static st_NPC_SOUND_ENTRY g_npcJukeBox[33] = {
    { eEntVillainType_Undefined, g_sndTrax_General },
    { eEntVillainTypeBat, g_sndTrax_Bat },
    { eEntVillainTypeBuckMasters, g_sndTrax_BuckMaster },
    { eEntVillainCaptainMoody, g_sndTrax_CaptainMoody },
    { eEntVillainCaveMan, g_sndTrax_Caveman },
    { eEntVillainTypeCreeper, g_sndTrax_Creeper },
    { eEntVillainTypeGhost, g_sndTrax_Ghost },
    { eEntVillainTypeGhostDiver, g_sndTrax_GhostDiver },
    { eEntVillainTypeGargoyle, g_sndTrax_Gargoyle },
    { eEntVillainTypeHeadlessSpecter, g_sndTrax_HeadlessSpecter },
    { eEntVillainTypeKillerPlant, g_sndTrax_KillerPlant },
    { eEntVillainTypeRat, g_sndTrax_Rat },
    { eEntVillainTypeRobot, g_sndTrax_Robot },
    { eEntVillainTypeSeaCreature, g_sndTrax_SeaCreature },
    { eEntVillainTypeScarecrow, g_sndTrax_ScareCrow },
    { eEntVillainTypeSpaceKook, g_sndTrax_SpaceKook },
    { eEntVillainTypeShark, g_sndTrax_Shark },
    { eEntVillainTypeTarMonster, g_sndTrax_TarMonster },
    { eEntVillainTypeWitch, g_sndTrax_Witch },
    { eEntVillainTypeWitchDoctor, g_sndTrax_WitchDoctor },
    { eEntVillainTypeWolfman, g_sndTrax_Wolfman },
    { eEntVillainTypeZombie, g_sndTrax_Zombie },
    { eEntVillainTypeFred, g_sndTrax_Fred },
    { eEntVillainTypeDaphne, g_sndTrax_Daphne },
    { eEntVillainTypeVelma, g_sndTrax_Velma },
    { eEntVillainHolly, g_sndTrax_Holly },
    { eEntVillainProfessor, g_sndTrax_Professor },
    { eEntVillainGroundsKeeper, g_sndTrax_GroundsKeeper },
    { eEntVillainBossBlackKnight, g_sndTrax_BlackKnight },
    { eEntVillainBossGreenGhost, g_sndTrax_GreenGhost },
    { eEntVillainBossRedbeard, g_sndTrax_RedBeard },
    { eEntVillainBossMastermind, g_sndTrax_MasterMind },
    { eEntVillainType_Undefined, NULL },
};

static st_ZVIL_NPCTYPE_LOOKUP g_npcTable[51] = {
    { 1, 1.0f, 0.3f, 1.0f, 2.0f, 1, 8.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 0.3f, -1.0f, 1.0f, 1, 90.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 2.0f, 1.0f, 2.0f, 0.7f, 1, 15.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 2.0f, 1.0f, 3.0f, 0.4f, 1, 15.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 2.0f, 1.0f, 2.0f, 0.2f, 1, 15.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 2.0f, 1.0f, -1.0f, 0.5f, 1, 15.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 2.0f, 0.5f, 0.5f, 2.0f, 1, 15.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 2.0f, 1.0f, 1.0f, 0.5f, 1, 15.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 2.0f, 0.5f, -1.0f, 1.0f, 1, 90.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 2.0f, 1.0f, 0.3f, 1.0f, 1, 12.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 0.5f, 1.0f, 1.0f, 1, 90.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 0.5f, 1.0f, 1.0f, 1, 90.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 2.0f, 1.0f, 1.0f, 0.3f, 1, 15.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 2.0f, 1.0f, -0.5f, 0.7f, 1, 15.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 1.0f, 5.0f, 1.0f, 1, 20.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 2.0f, 1.0f, -1.0f, 0.6f, 1, 15.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 2.0f, 1.0f, -1.0f, 0.3f, 1, 15.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 0.01f, -1.0f, 0.9f, 1, 15.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 2.0f, 1.0f, -1.0f, 0.7f, 1, 15.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 2.0f, 1.0f, 0.5f, 0.7f, 1, 15.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 2.0f, 1.0f, 1.0f, 0.6f, 1, 15.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 1.0f, 0.5f, 0.2f, 1.0f, 1, 10.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 1.0f, 0.5f, 0.2f, 1.0f, 1, 12.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 0.5f, 0.2f, 1.0f, 1, 12.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 1.0f, 0.5f, 0.2f, 1.0f, 1, 10.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 1.0f, 0.5f, 0.2f, 1.0f, 1, 10.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 0.5f, 0.2f, 1.0f, 1, 15.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 1.0f, 0.5f, 0.2f, 1.0f, 1, 12.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 1.0f, -1.0f, -1.0f, 0, 20.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 1.0f, -1.0f, -1.0f, 0, 20.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 1.0f, -1.0f, -1.0f, 0, 20.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 1.0f, -1.0f, -1.0f, 0, 20.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 1.0f, -1.0f, -1.0f, 0, 20.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 0.4f, -1.0f, -1.0f, 0, 20.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 1.0f, -1.0f, -1.0f, 0, 20.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 1.0f, -1.0f, -1.0f, 0, 20.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 0.3f, -1.0f, -1.0f, 0, 20.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 1.0f, -1.0f, -1.0f, 0, 20.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 0.5f, -1.0f, -1.0f, 0, 90.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 0.5f, -1.0f, -1.0f, 0, 90.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 0.5f, -1.0f, -1.0f, 0, 90.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 3.0f, 0.5f, 1.0f, 1.0f, 1, 100.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 3.0f, 0.3f, 1.0f, 1.0f, 1, 100.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 3.0f, 0.5f, 1.0f, 1.0f, 1, 100.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 3.0f, 0.5f, 1.0f, 1.0f, 1, 100.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 2.0f, 0.5f, 2.0f, 1.0f, 1, 25.0f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, 2.0f, 0.5f, 3.0f, 0.4f, 1, 15.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 0.5f, 0.0f, 0.0f, 0, 90.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 0.5f, 0.0f, 0.0f, 0, 90.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 1, -1.0f, 0.5f, 0.0f, 0.0f, 0, 90.5f, 0.7f, { 0.0f, 0.7f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
    { 0, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0.0f, 0.0f, { 0.0f, 0.0f, 0.0f }, 0.0f, { 0.0f, 0.0f, 0.0f }, 0, 0, 0, 0, 0, 0, ZVIL_PERSON_NORMAL, 0, { 0, 0, 0 }, 0, 0, 0, 0 },
};

static st_ZVIL_NPC_HANDLERFUNCS g_npcFuncs[51] = {
    { 1, VILN_crit_flag, VILN_crit_query, VILN_crit_sysevent, VILN_crit_notice, VILN_crit_aware, VILN_crit_stimulus, VILN_crit_trangoal, VILN_crit_calcmove, VILN_crit_respond, VILN_crit_pickanim, NULL, NULL },
    { 1, VILN_sham_flag, VILN_sham_query, VILN_sham_sysevent, VILN_sham_notice, VILN_sham_aware, VILN_sham_stimulus, VILN_sham_trangoal, VILN_sham_calcmove, VILN_sham_respond, VILN_sham_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_sham_flag, VILN_sham_query, VILN_sham_sysevent, VILN_sham_notice, VILN_sham_aware, VILN_sham_stimulus, VILN_sham_trangoal, VILN_sham_calcmove, VILN_sham_respond, VILN_sham_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_sham_flag, VILN_sham_query, VILN_sham_sysevent, VILN_sham_notice, VILN_sham_aware, VILN_sham_stimulus, VILN_sham_trangoal, VILN_sham_calcmove, VILN_sham_respond, VILN_sham_pickanim, NULL, NULL },
    { 1, VILN_sham_flag, VILN_sham_query, VILN_sham_sysevent, VILN_sham_notice, VILN_sham_aware, VILN_sham_stimulus, VILN_sham_trangoal, VILN_sham_calcmove, VILN_sham_respond, VILN_sham_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_shark_flag, VILN_shark_query, VILN_shark_sysevent, VILN_shark_notice, VILN_shark_aware, VILN_shark_stimulus, VILN_shark_trangoal, VILN_shark_calcmove, VILN_shark_respond, VILN_shark_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_crit_flag, VILN_crit_query, VILN_crit_sysevent, VILN_crit_notice, VILN_crit_aware, VILN_crit_stimulus, VILN_crit_trangoal, VILN_crit_calcmove, VILN_crit_respond, VILN_crit_pickanim, NULL, NULL },
    { 1, VILN_crit_flag, VILN_crit_query, VILN_crit_sysevent, VILN_crit_notice, VILN_crit_aware, VILN_crit_stimulus, VILN_crit_trangoal, VILN_crit_calcmove, VILN_crit_respond, VILN_crit_pickanim, NULL, NULL },
    { 1, VILN_crit_flag, VILN_crit_query, VILN_crit_sysevent, VILN_crit_notice, VILN_crit_aware, VILN_crit_stimulus, VILN_crit_trangoal, VILN_crit_calcmove, VILN_crit_respond, VILN_crit_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_kp_query, VILN_std_sysevent, VILN_std_notice, VILN_kp_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_crit_flag, VILN_crit_query, VILN_crit_sysevent, VILN_crit_notice, VILN_crit_aware, VILN_crit_stimulus, VILN_crit_trangoal, VILN_crit_calcmove, VILN_crit_respond, VILN_crit_pickanim, NULL, NULL },
    { 1, VILN_shag_flag, VILN_shag_query, VILN_shag_sysevent, VILN_shag_notice, VILN_shag_aware, VILN_shag_stimulus, VILN_shag_trangoal, VILN_shag_calcmove, VILN_shag_respond, VILN_shag_pickanim, NULL, NULL },
    { 1, VILN_shag_flag, VILN_shag_query, VILN_shag_sysevent, VILN_shag_notice, VILN_shag_aware, VILN_shag_stimulus, VILN_shag_trangoal, VILN_shag_calcmove, VILN_shag_respond, VILN_shag_pickanim, NULL, NULL },
    { 1, VILN_shag_flag, VILN_shag_query, VILN_shag_sysevent, VILN_shag_notice, VILN_shag_aware, VILN_shag_stimulus, VILN_shag_trangoal, VILN_shag_calcmove, VILN_shag_respond, VILN_shag_pickanim, NULL, NULL },
    { 1, VILN_shag_flag, VILN_shag_query, VILN_shag_sysevent, VILN_shag_notice, VILN_shag_aware, VILN_shag_stimulus, VILN_shag_trangoal, VILN_shag_calcmove, VILN_shag_respond, VILN_shag_pickanim, NULL, NULL },
    { 1, VILN_shag_flag, VILN_shag_query, VILN_shag_sysevent, VILN_shag_notice, VILN_shag_aware, VILN_shag_stimulus, VILN_shag_trangoal, VILN_shag_calcmove, VILN_shag_respond, VILN_shag_pickanim, NULL, NULL },
    { 1, VILN_shagM_flag, VILN_shagM_query, VILN_shagM_sysevent, VILN_shagM_notice, VILN_shagM_aware, VILN_shagM_stimulus, VILN_shagM_trangoal, VILN_shagM_calcmove, VILN_shagM_respond, VILN_shagM_pickanim, NULL, NULL },
    { 1, VILN_shag_flag, VILN_shag_query, VILN_shag_sysevent, VILN_shag_notice, VILN_shag_aware, VILN_shag_stimulus, VILN_shag_trangoal, VILN_shag_calcmove, VILN_shag_respond, VILN_shag_pickanim, NULL, NULL },
    { 1, VILN_shag_flag, VILN_shag_query, VILN_shag_sysevent, VILN_shag_notice, VILN_shag_aware, VILN_shag_stimulus, VILN_shag_trangoal, VILN_shag_calcmove, VILN_shag_respond, VILN_shag_pickanim, NULL, NULL },
    { 1, VILN_shag_flag, VILN_shag_query, VILN_shag_sysevent, VILN_shag_notice, VILN_shag_aware, VILN_shag_stimulus, VILN_shag_trangoal, VILN_shag_calcmove, VILN_shag_respond, VILN_shag_pickanim, NULL, NULL },
    { 1, VILN_shag_flag, VILN_shag_query, VILN_shag_sysevent, VILN_shag_notice, VILN_shag_aware, VILN_shag_stimulus, VILN_shag_trangoal, VILN_shag_calcmove, VILN_shag_respond, VILN_shag_pickanim, NULL, NULL },
    { 1, VILN_gang_flag, VILN_gang_query, VILN_gang_sysevent, VILN_gang_notice, VILN_gang_aware, VILN_gang_stimulus, VILN_gang_trangoal, VILN_gang_calcmove, VILN_gang_respond, VILN_gang_pickanim, VILN_gang_animtran, NULL },
    { 1, VILN_gang_flag, VILN_gang_query, VILN_gang_sysevent, VILN_gang_notice, VILN_gang_aware, VILN_gang_stimulus, VILN_gang_trangoal, VILN_gang_calcmove, VILN_gang_respond, VILN_gang_pickanim, NULL, NULL },
    { 1, VILN_gang_flag, VILN_gang_query, VILN_gang_sysevent, VILN_gang_notice, VILN_gang_aware, VILN_gang_stimulus, VILN_gang_trangoal, VILN_gang_calcmove, VILN_gang_respond, VILN_gang_pickanim, NULL, NULL },
    { 1, VILN_BBK_flag, VILN_BBK_query, VILN_BBK_sysevent, VILN_BBK_notice, VILN_BBK_aware, VILN_BBK_stimulus, VILN_BBK_trangoal, VILN_BBK_calcmove, VILN_BBK_respond, VILN_BBK_pickanim, VILN_BBK_animtran, NULL },
    { 1, VILN_BGG_flag, VILN_BGG_query, VILN_BGG_sysevent, VILN_BGG_notice, VILN_BGG_aware, VILN_BGG_stimulus, VILN_BGG_trangoal, VILN_BGG_calcmove, VILN_BGG_respond, VILN_BGG_pickanim, NULL, NULL },
    { 1, VILN_BRB_flag, VILN_BRB_query, VILN_BRB_sysevent, VILN_BRB_notice, VILN_BRB_aware, VILN_BRB_stimulus, VILN_BRB_trangoal, VILN_BRB_calcmove, VILN_BRB_respond, VILN_BRB_pickanim, NULL, VILN_BRB_xtrarend },
    { 1, VILN_BMM_flag, VILN_BMM_query, VILN_BMM_sysevent, VILN_BMM_notice, VILN_BMM_aware, VILN_BMM_stimulus, VILN_BMM_trangoal, VILN_BMM_calcmove, VILN_BMM_respond, VILN_BMM_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_std_flag, VILN_std_query, VILN_std_sysevent, VILN_std_notice, VILN_std_aware, VILN_std_stimulus, VILN_std_trangoal, VILN_std_calcmove, VILN_std_respond, VILN_std_pickanim, NULL, NULL },
    { 1, VILN_gang_flag, VILN_gang_query, VILN_gang_sysevent, VILN_gang_notice, VILN_gang_aware, VILN_gang_stimulus, VILN_gang_trangoal, VILN_gang_calcmove, VILN_gang_respond, VILN_gang_pickanim, NULL, NULL },
    { 1, VILN_gang_flag, VILN_gang_query, VILN_gang_sysevent, VILN_gang_notice, VILN_gang_aware, VILN_gang_stimulus, VILN_gang_trangoal, VILN_gang_calcmove, VILN_gang_respond, VILN_gang_pickanim, NULL, NULL },
    { 1, VILN_gang_flag, VILN_gang_query, VILN_gang_sysevent, VILN_gang_notice, VILN_gang_aware, VILN_gang_stimulus, VILN_gang_trangoal, VILN_gang_calcmove, VILN_gang_respond, VILN_gang_pickanim, NULL, NULL },
    { 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
};

static st_NPCTYPE_CONFIG g_npcTypeTable[50] = { 0 };

static st_NPC_BONUS g_npcBonus[11] = {
    { 0, NULL },
    { 0, NULL },
    { 0, NULL },
    { 0, NULL },
    { 0, NULL },
    { 0, NULL },
    { 0, NULL },
    { 0, NULL },
    { 0, NULL },
    { 0, NULL },
    { 0, NULL },
};

static char* g_strz_npcBonus[11] = {
    "_PICK_VILN_FOOD_FOOD", "_PICK_VILN_FOOD_BURGER", "_PICK_VILN_FOOD_ICECREAM",
    "_PICK_VILN_FOOD_TURKEY", "_PICK_VILN_FOOD_CAKE", "_PICK_VILN_AMMO_SOAP_A",
    "_PICK_VILN_AMMO_SOAP_B", "_PICK_VILN_AMMO_SOAP_C", "_PICK_VILN_AMMO_GUM_A",
    "_PICK_VILN_AMMO_GUM_B", "_PICK_VILN_AMMO_GUM_C",
};

static st_NPC_NOBONUS g_nobonus[20] = {
    { 'C006', 0x449DB5F4 },
    { 'O001', 0xE8EA9736 },
    { 'O001', 0x120782D1 },
    { 'O001', 0xBD63DC69 },
    { 'O001', 0xD93EE089 },
    { 'W023', 0x7BCB3A79 },
    { 'W023', 0xECF98FF8 },
    { 'W023', 0x7BCB3A7A },
    { 'W023', 0x7BCB3A7B },
    { 'W023', 0x7BCB3A7C },
    { 'W023', 0x7BCB3A7D },
    { 'W023', 0xECF98FF9 },
    { 'W023', 0xECF98FFA },
    { 'W023', 0x3B9E586B },
    { 'W023', 0xECF98FFB },
    { 'W023', 0xECF98FFC },
    { 'W023', 0x7BCB3A7E },
    { 'W023', 0x7BCB3A7F },
    { 'W023', 0x7BCB3A80 },
    { 0, 0 },
};

static st_NPC_TALLY_RECORD g_stattally[51] = { 0 };

static st_XORDEREDARRAY g_emitlist = { 0 };

static st_zEntVillain_ParEmitInfo g_emitinfo[32] = {
    { "VilPar_Kook_Attack", VPAR_EMIT_EXTATTACK, eEntVillainTypeSpaceKook, 0, 0 },
    { "VilPar_Witch_Attack", VPAR_EMIT_EXTATTACK, eEntVillainTypeWitch, 0, 0 },
    { "VilPar_Plant_Attack", VPAR_EMIT_EXTATTACK, eEntVillainTypeKillerPlant, 0, 0 },
    { "VilPar_GreenGhost_Attack", VPAR_EMIT_EXTATTACK, eEntVillainBossGreenGhost, 0, 0 },
    { "ParEmit_Vil_Sleeping", VPAR_EMIT_BORED, eEntVillainType_Undefined, 0x20000, 0 },
    { "ParEmit_Vil_Bored", VPAR_EMIT_BORED, eEntVillainType_Undefined, 0, 0x20000 },
    { "ParEmit_Vil_Bored", VPAR_EMIT_GUARD, eEntVillainType_Undefined, 2, 0 },
    { "ParEmit_Vil_Question", VPAR_EMIT_QUESTMARK, eEntVillainType_Undefined, 0, 0 },
    { "ParEmit_Vil_Surprize", VPAR_EMIT_EXCLAIM, eEntVillainType_Undefined, 0, 0 },
    { "ParEmit_Vil_Soapy", VPAR_EMIT_SOAPY, eEntVillainType_Undefined, 0, 0 },
    { "ParEmit_Vil_PopSoap", VPAR_EMIT_POPSOAP, eEntVillainType_Undefined, 0, 0 },
    { "ParEmit_Vil_Gummy", VPAR_EMIT_GUMMY, eEntVillainType_Undefined, 0, 0 },
    { "ParEmit_Vil_PopGum", VPAR_EMIT_POPGUM, eEntVillainType_Undefined, 0, 0 },
    { "ParEmit_SD_BounceOnVillain", VPAR_EMIT_BUTTBOUNCE, eEntVillainType_Undefined, 2, 0 },
    { "ParEmit_SD_Headbutt", VPAR_EMIT_HEADBUTT, eEntVillainType_Undefined, 2, 0 },
    { "ParEmit_SD_Headbutt2", VPAR_EMIT_HEADBUTT, eEntVillainType_Undefined, 2, 0 },
    { "ParEmit_Vil_Stunned", VPAR_EMIT_STUN, eEntVillainType_Undefined, 2, 0 },
    { "ParEmit_Vil_Teleport", VPAR_EMIT_TELEPORT, eEntVillainBossMastermind, 0, 0 },
    { "ParEmit_Vil_Dying", VPAR_EMIT_DYING, eEntVillainType_Undefined, 0, 0 },
    { "ParEmit_Vil_Decay", VPAR_EMIT_DECAY, eEntVillainType_Undefined, 0, 0 },
    { "ParEmit_Vil_DecayRobot", VPAR_EMIT_DECAY, eEntVillainTypeRobot, 0, 0 },
    { "ParEmit_Vil_KookRespawn", VPAR_EMIT_RESPAWN, eEntVillainTypeSpaceKook, 0, 0 },
    { "ParEmit_Vil_HoloDecay", VPAR_EMIT_DECAY, eEntVillainType_Undefined, 8, 0 },
    { "ParEmit_Vil_HoloPoof", VPAR_EMIT_POOF, eEntVillainType_Undefined, 8, 0 },
    { "ParEmit_Vil_HoloDots", VPAR_EMIT_POOF, eEntVillainType_Undefined, 8, 0 },
    { NULL, VPAR_EMIT_WALLPRINT, eEntVillainTypeBat, 0, 0 },
    { NULL, VPAR_EMIT_WALLPRINT, eEntVillainTypeBat, 0, 0 },
    { NULL, VPAR_EMIT_WALLPRINT, eEntVillainTypeBat, 0, 0 },
    { NULL, VPAR_EMIT_WALLPRINT, eEntVillainTypeBat, 0, 0 },
    { NULL, VPAR_EMIT_WALLPRINT, eEntVillainTypeBat, 0, 0 },
    { NULL, VPAR_EMIT_WALLPRINT, eEntVillainTypeBat, 0, 0 },
    { NULL, VPAR_EMIT_WALLPRINT, eEntVillainTypeBat, 0, 0 },
};

static _zParEmitterCustomSettings g_pecust = { 0 };

static unsigned int g_anamhash[147] = { 0 };

static char* g_animname[148] = {
    "UNKNOWN", "Idle", "Idle_Fidget_1", "Idle_Fidget_2", "Bored", "Bored_Fidget_1",
    "Bored_Fidget_2", "Move", "Fashion_Pose", "Turn", "Chase", "Action_OnceNoMove",
    "Action_LoopAndMove", "Surprize", "Taunt", "Taunt_1", "Taunt_2", "Hear_Start", "Hear_Loop",
    "Look_Start", "Look_Loop", "Investigate_Start", "Investigate", "AttackIdle", "Attack",
    "Attack_1", "Attack_2", "Combo", "Combo_1", "Combo_2", "Lob_Attack", "Lob_Combo",
    "Extend_Attack_Start", "Extend_Attack_Loop", "Extend_Combo", "Hit_By_Gum", "Gummed",
    "Gummed_Resist", "Gummed_BreakFree", "Hit_When_Gummed", "Bounced_When_Gummed", "Hit_By_Soap",
    "Soaped", "Soaped_Resist", "Soaped_BreakFree", "Hit_When_Soaped", "Bounced_When_Soaped",
    "Dazed_Begin", "Dazed", "Dazed_Recovery", "Death", "Death_From_Above", "Death_From_Dazed",
    "Death_Idle", "Jump_Begin", "Jump_Loop", "Jump_Land", "Fall_Loop", "Fall_Splat",
    "BK_ScaryIdle", "BK_Walk", "BK_Turn", "BK_Patrol", "BK_PatrolFlip", "BK_Attack", "BK_Shock",
    "BK_ShockSkel", "BK_Gloat", "BK_Gloat_1", "BK_Gloat_2", "BK_Death", "GG_Chase",
    "GG_ChaseLeft", "GG_ChaseRight", "GG_AttackBuildBegin", "GG_AttackBuildLoop", "GG_Attack",
    "GG_Gloat", "GG_Gloat_1", "GG_Hit", "GG_Hit_1", "GG_Hit_2", "RB_IdleWind", "RB_Walk",
    "RB_Turn", "RB_Chase", "RB_TauntFeel", "RB_TauntThroat", "RB_TauntLaugh", "RB_AttackPunch",
    "RB_AttackThrow", "RB_AttackThrowWait", "RB_AttackCatch", "RB_React", "RB_LookBegin",
    "RB_LookLoop", "RB_LookEnd", "RB_DeathLoop", "MM_Foreplay_Move", "MM_Foreplay_Taunt",
    "MM_Foreplay_Action", "MM_Foreplay_JumpDown", "MM_Battle_Idle", "MM_Battle_Move",
    "MM_Battle_Turn", "MM_Battle_Backstep", "MM_Battle_Taunt", "MM_AngerReact",
    "MM_Battle_Attack_start", "MM_Battle_Attack_ready_loop", "MM_Battle_Attack",
    "MM_Battle_Shield_start", "MM_Battle_Shield_loop", "MM_Battle_Shield_end",
    "MM_Battle_Stunned_start", "MM_Battle_Stunned_loop", "MM_Battle_Stunned_end",
    "MM_Battle_Knocked_begin", "MM_Battle_Knocked_loop", "MM_Battle_Knocked_recover",
    "MM_Death_start", "MM_Death", "GANG_Idle_1", "GANG_Idle_2", "GANG_Idle_3", "GANG_IdleFidget",
    "GANG_IdleFidget_1", "GANG_IdleFidget_2", "GANG_IdleFidget_3", "GANG_Turn", "GANG_Walk",
    "GANG_Run", "GANG_RunChased", "GANG_Action", "GANG_BossAction", "GANG_Talk", "GANG_Talk_1",
    "GANG_Talk_2", "GANG_Talk_3", "GANG_TalkAndTurn", "SHAM_Move", "SHAM_Taunt", "CRIT_Move",
    "CRIT_BatSleep", "CRIT_SpiderDrop", "CRIT_Smashed", "SIMP_Action", NULL,
};

static char* g_parmname[49] = {
    "Parm_Hit_Points", "Parm_Bubbled_Time", "Parm_Gummed_Time", "Parm_Dazed_Time",
    "Parm_Extend_Range", "Parm_Extend_Rate", "Parm_Extend_Width", "Parm_Extend_Time",
    "Parm_FOV_Angle", "Parm_Gloat_Time", "Parm_Lob_Range", "Parm_Lob_Speed", "Parm_Lob_Salvo",
    "Parm_Lob_ProjectileTypeName", "Parm_Lob_Reload_Time", "Parm_Lob_Arcness",
    "Parm_Lob_Heaviness", "Parm_Detect_Radius", "Parm_Detect_Height", "Parm_Detect_HeightOffset",
    "Parm_Pursue_Radius", "Parm_Move_Rate", "Parm_Chase_Rate", "Parm_Turn_Rate",
    "Parm_ModelScale_Base", "Parm_SAnim_Idle", "Parm_SAnim_Move", "Parm_SAnim_Chase",
    "Parm_Wander_Angle", "Parm_GumVert_LeftFoot", "Parm_GumVert_RightFoot", "Parm_GumVert_Elbow",
    "Parm_GumVert_CustomA", "Parm_GumVert_CustomB", "Parm_WepVert_Attach", "Parm_WepVert_Launch",
    "Parm_WepVert_LaunchAlt", "Parm_DmgVert_Buttbounce", "Parm_LOSVert_Eyeball",
    "Parm_WepVert_RBSwordHandle", "Parm_WepVert_RBSwordPivot", "Parm_SBoundOffsetVec",
    "Parm_SBoundRadius", "Parm_SBound2OffsetVec", "Parm_SBound2Radius", "Parm_SpawnMask",
    "Parm_MinDifficulty", "Parm_AttackDmgBegTime", "Parm_AttackDmgEndTime",
};

static st_BKAXE_DATA g_axfxpool[10] = { 0 };

static st_BGG_QUAKEBALL g_quakepool[4] = { 0 };

static char* g_strz_rbtgt[5] = {
    "ScoobyDoo!", "Rope01", "Rope02", "Rope03", "Rope04",
};

static char* g_strz_mmtp[12] = {
    "MM_TP_SAFE_A", "MM_TP_SAFE_B", "MM_TP_SAFE_C", "MM_TP_SAFE_D", "MM_TP_SAFE_E",
    "MM_TP_SAFE_F", "MM_TP_SAFE_G", "MM_TP_SAFE_H", "MM_TP_DNGR_A", "MM_TP_DNGR_B",
    "MM_TP_DNGR_C", "MM_TP_DNGR_D",
};

static _zMovePoint* g_mmtp[12] = { 0 };

static char* g_stz_mmftyp_name[3] = {
    "_PRJ_VWEP_FLASK", "_PRJ_VWEP_FLASK", "_PRJ_VWEP_FLASK",
};

static st_ZVMM_FLASK_DATA g_mmflask[10] = { 0 };

#define VIL_MIN(a, b) ((a) < (b) ? (a) : (b))
#define VIL_MAX(a, b) ((a) > (b) ? (a) : (b))

static inline void VEFX_place_gumBlob(xModelInstance* blob, _xMat4x3* mat);
static inline void VSUP_cheap_reflect(_xVec3* out, _xVec3* alter);
static signed int VSUP_accum_collide(st_zEntVillain* vil, _xVec3* vec_accum);
static inline float VSUP_yaw(_xVec3 dir);
static inline void VSUP_wtadjust(float* wts, signed int cnt, float arbref);
static inline signed int VSUP_choose(signed int* list, float* wts, signed int cnt);
static inline float VSUP_ent_AOI(st_zEntVillain* vil, _zEnt* ent, signed int* qdrant);
static inline float VSUP_ent_facedot(st_zEntVillain* vil, _zEnt* ent);
static inline signed int VSUP_ent_inLOS_timz(st_zEntVillain* vil, _zEnt* ent, signed int prev, float rand);
static inline signed int VSUP_ent_inLOS(st_zEntVillain* vil, _zEnt* ent, signed int prev, float rand);
static inline _xVec3* VUTIL_mdlpos(st_zEntVillain* vil);
static inline float VUTIL_sqrt(float x);
static inline signed int VSUP_ent_inFOV(st_zEntVillain* vil, _zEnt* ent);
static inline void VSUP_ang_toXZDir(float angle, _xVec3* dir);
static inline _xVec3 VUTIL_ctrOf(_xEnt* ent);
static inline _xVec3 VUTIL_posOf(_xEnt* ent);
static inline float VSUP_dir_toPos(_zEnt* ent, _xVec3 pos, _xVec3* dir);
static inline float VSUP_ds2_toPos(_zEnt* ent, _xVec3 pos);
static inline void VOUT_steer(st_zEntVillain* vil);
static st_NPC_BONUS* VBON_choose_bonusItem(en_zEntVillainSubTypeID vtyp);
static void VBON_drop_bonusItem(st_zEntVillain* vil);
static inline void VBON_setup_bonusTable();
static inline st_NPC_SOUND_PROPERTIES* VSND_findProps(en_NPC_SOUND sndEvent, st_NPC_SOUND_PROPERTIES** sprop_caller);
static unsigned int VSND_select_sound(st_zEntVillain* vil, en_NPC_SOUND sndEvent);
static inline void VANM_playRand5(st_zEntVillain* vil, en_NPC_ANIMSTATES a, en_NPC_ANIMSTATES b, en_NPC_ANIMSTATES c, en_NPC_ANIMSTATES d, en_NPC_ANIMSTATES e);
static inline void VANM_playRand4(st_zEntVillain* vil, en_NPC_ANIMSTATES a, en_NPC_ANIMSTATES b, en_NPC_ANIMSTATES c, en_NPC_ANIMSTATES d);
static inline void VANM_playRand3(st_zEntVillain* vil, en_NPC_ANIMSTATES a, en_NPC_ANIMSTATES b, en_NPC_ANIMSTATES c);
static inline void VANM_play5(st_zEntVillain* vil, en_NPC_ANIMSTATES a, en_NPC_ANIMSTATES b, en_NPC_ANIMSTATES c, en_NPC_ANIMSTATES d, en_NPC_ANIMSTATES e);
static inline void VANM_play4(st_zEntVillain* vil, en_NPC_ANIMSTATES a, en_NPC_ANIMSTATES b, en_NPC_ANIMSTATES c, en_NPC_ANIMSTATES d);
static inline void VANM_play3(st_zEntVillain* vil, en_NPC_ANIMSTATES a, en_NPC_ANIMSTATES b, en_NPC_ANIMSTATES c);
static inline void VANM_playRand(st_zEntVillain* vil, en_NPC_ANIMSTATES a, en_NPC_ANIMSTATES b);
static inline void VANM_playLoop(st_zEntVillain* vil, en_NPC_ANIMSTATES a);
static inline void VANM_play(st_zEntVillain* vil, en_NPC_ANIMSTATES a, en_NPC_ANIMSTATES b);
static inline en_NPC_SOUND VSND_typeFromHash(st_zEntVillain* vil, unsigned int aid_snd);
static inline signed int VSND_isPlaying(st_zEntVillain* vil, signed int channel);
static inline float VUTIL_jitter(float tym);
static unsigned int VSND_goPlay(st_zEntVillain* vil, en_NPC_SOUND sndEvent, unsigned int sndExplicit, signed int flg_vsnd, float dist);
static signed int VSND_procQueue(st_zEntVillain* vil, float dt);
static inline signed int VSND_addQueueEntry(st_zEntVillain* vil, en_NPC_SOUND sndEvent, signed int flg_vsnd, float radius, float delay, float repeatDelay, signed int repeatCount, unsigned int useThisSound);
static void VSND_play(st_zEntVillain* vil, en_NPC_SOUND sndEvent);
static inline void VSND_cue(st_zEntVillain* vil, en_NPC_SOUND sndEvent);
static inline void VSND_setup_soundTable();
static inline xAnimSingle* VANM_entSingle(_zEnt* ent);
static inline xAnimState* VANM_entState(_zEnt* ent);
static inline float VANM_duration(xAnimState* astate, st_zEntVillain* ent, float* tym);
static xAnimTransition* VANM_findtran(st_zEntVillain* ent, xAnimTable* table, xAnimState* from, xAnimState* to, signed int autoslamit, xAnimTransition** newtran);
static inline en_NPC_ANIMSTATES VANM_set1(st_zEntVillain* ent, en_NPC_ANIMSTATES anim);
static en_NPC_ANIMSTATES VANM_randset(st_zEntVillain* ent, float* wts, en_NPC_ANIMSTATES* choice_list, signed int cnt);
static inline void VANM_starttran(st_zEntVillain* ent, xAnimState* from, xAnimState* to);
static en_NPC_ANIMSTATES VANM_set(st_zEntVillain* ent, en_NPC_ANIMSTATES* best, signed int cnt, signed int sync);
static inline en_NPC_ANIMSTATES VANM_hashToAnim(unsigned int id);
static unsigned int VANM_diddflt(xAnimTransition* tran, xAnimSingle* single, st_zEntVillain* ent);
static unsigned int VANM_dodflt(xAnimTransition* tran, xAnimSingle* single, st_zEntVillain* ent);
static unsigned int VANM_didtran(xAnimTransition* tran, xAnimSingle* single, st_zEntVillain* ent);
static unsigned int VANM_dotran(xAnimTransition* tran, xAnimSingle* single, st_zEntVillain* ent);
static xAnimTable* VANM_mktable_STD();
static xAnimTable* VANM_mktable_GANG();
static xAnimTable* VANM_mktable_SIMP();
static xAnimTable* VANM_mktable_SHARK();
static xAnimTable* VANM_mktable_WITCH();
static xAnimTable* VANM_mktable_SHAM();
static xAnimTable* VANM_mktable_MM();
static xAnimTable* VANM_mktable_RB();
static xAnimTable* VANM_mktable_GG();
static xAnimTable* VANM_mktable_BK();
static void VPAR_emit_extattack(st_zEntVillain* ent, _xVec3* origin, _xVec3* aimdir, float length, float slope, float dt);
static inline signed int VPAR_emit_headbutt(float dt, st_zEntVillain_ParEmitRecord* vpar, _zParEmitterCustomSettings* pecust);
static inline signed int VUTIL_tagpos(st_zEntVillain* ent, signed int idx, _xVec3* pos);
static void VPAR_emit_partype(st_zEntVillain* ent, en_VPAR_EMITTER_ROLE emittype, float dt);
static void VPAR_readyize_emitters();
static inline _tagLightning* VEFX_zapper_type_A(_xVec3* from, _xVec3* to, float dur);
static inline _tagLightning* VEFX_zapper_type_B(_xVec3* from, _xVec3* to, float dur);
static void VEFX_zapTheKnight(st_zEntVillain* vil, signed int cleanup);
static void VEFX_mm_zapper(st_zEntVillain* vil, signed int cleanup);
static void VEFX_hologram(st_zEntVillain* vil, float dt);
static inline void VEFX_hologram_emit(st_zEntVillain* vil, float dt);
static void VEFX_plop_gum(st_zEntVillain* ent);
static void SuperVillainGumDraw(_xVec3* from, _xVec3* to, float top_radius, float bot_radius);
static void VWEP_rbs_chkCollide(st_REDBEARD_SWORD* rbs, float dt);
static inline void VWEP_rbs_flight(st_REDBEARD_SWORD* rbs, float dt);
static void VWEP_rbs_throw(st_REDBEARD_SWORD* rbs);
static void VWEP_rbs_bupdate(_xEnt* ent_rbs, _xVec3* pos);
static inline xModelInstance* VWEP_rbs_animmodel(st_zEntVillain* vil);
static void VWEP_rbs_setup(st_REDBEARD_SWORD* rbs, st_zEntVillain* rbvil);
static inline void VPRJ_BMM_blackflask(zEntProjectile* prj, float dt);
static inline void VPRJ_BMM_greenflask(zEntProjectile* prj, float dt);
static inline void VPRJ_BMM_redflask(zEntProjectile* prj, float dt);
static inline void VPRJ_prjsound(st_zEntVillain* vil, zEntProjectile* prj, en_NPC_SOUND snd);
static inline en_ZVMM_FLASK_TYPE VPRJ_BMM_select_flasktype(st_zEntVillain* vil);
static signed int VPRJ_BMM_notify(zEntProjectile* prj, en_PROJECTILE_EVENT event);
static signed int VPRJ_BMM_logix(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame);
static void VPRJ_BMM_detach(zEntProjectile* prj);
static signed int VPRJ_BMM_attach(zEntProjectile* prj);
static void VPRJ_BMM_bupdate(_xEnt* ent, _xVec3* pos);
static signed int VPRJ_BRB_notify(zEntProjectile* prj, en_PROJECTILE_EVENT event);
static signed int VPRJ_BRB_logix(zEntProjectile* prj, float dt);
static void VPRJ_BRB_detach(zEntProjectile* prj);
static signed int VPRJ_BRB_attach(zEntProjectile* prj);
static inline void VPRJ_emitat(_zParEmitter* emitter, zEntProjectile* prj, float dt, float yoff);
static inline void VPRJ_BGG_shockpos(zEntProjectile* prj, float dt);
static signed int VPRJ_BGG_shockwave(zEntProjectile* prj, float dt);
static signed int VPRJ_BGG_notify(zEntProjectile* prj, en_PROJECTILE_EVENT event);
static signed int VPRJ_BGG_logix(zEntProjectile* prj, float dt);
static void VPRJ_BGG_detach(zEntProjectile* prj);
static signed int VPRJ_BGG_attach(zEntProjectile* prj);
static void VPRJ_zapTheAxe(zEntProjectile* prj, signed int cleanup);
static signed int VPRJ_BBK_notify(zEntProjectile* prj, en_PROJECTILE_EVENT event);
static signed int VPRJ_BBK_prjmove(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame);
static signed int VPRJ_BBK_logix(zEntProjectile* prj, float dt);
static void VPRJ_BBK_detach(zEntProjectile* prj);
static signed int VPRJ_BBK_attach(zEntProjectile* prj);
static void VPRJ_BBK_bupdate(_xEnt* ent, _xVec3* pos);
static inline void VPRJ_startup();
static signed int VWEP_lobInitMMFlask(st_zEntVillain* vil, _xVec3* pos_tgt, _xVec3* vec_off, _xVec3* vec_spin, float arcness, float heavy);
static signed int VWEP_lobInitRBSword(st_zEntVillain* vil, _xVec3* pos_tgt, _xVec3* vec_off, _xVec3* vec_spin, float arcness, float heavy);
static signed int VWEP_lobInitGGQuaker(st_zEntVillain* vil, _xVec3* pos_tgt, _xVec3* vec_off, _xVec3* vec_spin, float arcness, float heavy);
static signed int VWEP_lobInitBKAxe(st_zEntVillain* vil, _xVec3* pos_tgt, _xVec3* vec_off, _xVec3* vec_spin, float arcness, float heavy);
static signed int VWEP_lobInitArcing(st_zEntVillain* vil, _xVec3* pos_tgt, _xVec3* vec_off, _xVec3* vec_spin, float arcness, float heavy);
static inline signed int VWEP_lobInitArrow(st_zEntVillain* vil, _xVec3* pos_tgt, _xVec3* vec_off);
static inline void VWEP_lobLaunchPos(st_zEntVillain* vil, _xVec3* pos);
static signed int VWEP_prjOwnerNotify(zEntProjectile* prj, en_PROJECTILE_EVENT event, void* ctxt);
static signed int VWEP_lobActivatePRJ(zEntProjectile* prj, st_zEntVillain* vil, _xVec3* pos_tgt, _xVec3* vec_off, _xVec3* vec_spin, float arcness, float heavy);
static zEntProjectile* VWEP_lobReservePRJ(st_zEntVillain* vil);
static signed int VWEP_extend_particles(st_zEntVillain* vil, float dt, _xVec3* pos_tgt);
static signed int VWEP_swipe(st_zEntVillain* vil, float);
static en_BEHAVIOUR_GOAL VGOL_set(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal);
static signed int VCFG_getParm(st_ZVIL_CONFIG_REFERENCE* vilcfg, en_NPC_INI_PARAMETERS pid, void* val);
static void VCFG_readAsset(st_ZVIL_CONFIG_REFERENCE* vilcfg);
static void VCFG_readParms(st_ZVIL_CONFIG_REFERENCE* vilcfg);
static void VIRM_initVilDflt(st_zEntVillain* vil);
static inline signed int VUTIL_rayhole(_xRay3* ray, _xVec3* pos_eye, _xVec3* pos_probe);
static signed int zVIL_chk_pitfall(st_zEntVillain* vil, signed int def, float chance);
static void zVIL_apply_move(st_zEntVillain* vil, _xScene*, float dt, _xEntFrame* frame);
static void zVIL_precalc(st_zEntVillain* vil, signed int flags);
static inline void VTMR_tick(st_zEntVillain* vil, signed int idx, float dt);
static void zVIL_upd_timers(st_zEntVillain* vil, float dt);
static void zVIL_apply_vilcfg(st_zEntVillain* vil, st_ZVIL_CONFIG_REFERENCE* vilcfg);
static _zMovePoint* VNAV_find_telepoint(st_zEntVillain* vil, signed int force);
static inline xAnimSingle* VANM_single(st_zEntVillain* vil);
static inline unsigned int VANM_stateUserFlags(st_zEntVillain* vil, unsigned int mask);
static inline void VNAV_animspeed(st_zEntVillain* vil, float spd, _xVec3* sanim);
static float VNAV_get_movespeed(st_zEntVillain* vil, float rnd);
static signed int VNAV_chklos_timz(void* usrdata, _xVec3* pos, _zMovePoint* mvpt);
static inline float VNAV_dist(_xVec3* d, _xVec3* a, _xVec3* b);
static signed int VNAV_get_nav(st_zEntVillain* vil);
static en_ZVILLAIN_HEADBUTT_RESPONSE VOUT_spikey_header(st_zEntVillain* vil);
static signed int VOUT_apply_anchor(st_zEntVillain* vil);
static inline signed int VOUT_hit_isWall(st_zEntVillain* vil, _xVec3* dir_trav, _xCollis* colrec);
static signed int VOUT_dogoto_widget(st_zEntVillain* vil, xBase* widge, float keepaway, signed int follow);
static inline signed int VOUT_chase_move(st_zEntVillain* vil);
static inline signed int VOUT_chkdo_followArrived(st_zEntVillain* vil);
static inline signed int VOUT_follow_move(st_zEntVillain* vil);
static inline signed int VOUT_chkdo_minipatrolArrived(st_zEntVillain* vil);
static inline signed int VOUT_chkdo_patrolArrived(st_zEntVillain* vil);
static inline signed int VOUT_patrol_move(st_zEntVillain* vil);
static signed int zVIL_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static inline void zVIL_input_stim(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt);
static inline void VUTIL_stimulate(st_zEntVillain* vil, en_NPC_STIMULUS stim);
static void zVIL_elcb_move(_xEnt* xe, _xScene* scn, float dt, _xEntFrame* frame);
static void zVIL_elcb_bupdate(_xEnt* xe, _xVec3* pos_bound);
static inline void VUTIL_resetctx(st_STIMULUS_CONTEXT_DATA* ctx);
static void zVIL_elcb_update(_xEnt* xe, _xScene* scn, float dt);
static signed int zVIL_elcb_event(xBase* from, xBase* to, unsigned int toEvent, float* toParam, xBase* toParamWidget);
static inline signed int VILN_flagsFromTable(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup);
static inline signed int VILN_cmn_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static inline void VILN_tran(st_STIMULUS_CONTEXT_DATA* resp, signed int goal, signed int mode);
static inline void VILN_stimGotoPos(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt);
static inline _zMovePoint* VNAV_mvptFromID(unsigned int aid);
static inline void VILN_restart(st_zEntVillain* vil);
static signed int VILN_BMM_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_BMM_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BMM_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static inline void VILN_BMM_enterGoal(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, signed int spot);
static inline void VILN_BMM_tymrRand(st_zEntVillain* vil, signed int idx);
static signed int VILN_BMM_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_BMM_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static inline signed int VILN_BMM_teleport(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp, signed int hurt);
static signed int VILN_BMM_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BMM_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BMM_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BMM_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_BMM_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_BRB_xtrarend(st_zEntVillain* vil);
static signed int VILN_BRB_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_BRB_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BRB_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_BRB_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_BRB_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BRB_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static inline void VILN_BRB_flamesnd(unsigned int evt, signed int on);
static signed int VILN_BRB_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BRB_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BRB_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_BRB_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_BGG_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_BGG_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static inline void VILN_BGG_face(st_zEntVillain* vil, _xVec3* pos);
static signed int VILN_BGG_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_BGG_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_BGG_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static inline void VILN_BGG_quakeSalvo(st_zEntVillain* vil);
static signed int VILN_BGG_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BGG_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BGG_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BGG_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_BGG_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_BBK_animtran(st_zEntVillain* vil, xAnimTransition* tran, xAnimSingle* single);
static signed int VILN_BBK_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_BBK_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BBK_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_BBK_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_BBK_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static inline void VILN_BBK_patrol(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BBK_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BBK_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BBK_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_BBK_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_BBK_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_sham_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_sham_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_sham_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_sham_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_sham_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static inline void VILN_holoDeath(st_zEntVillain* vil, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_sham_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_sham_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_sham_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_sham_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_sham_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_gang_animtran(st_zEntVillain* vil, xAnimTransition* tran, xAnimSingle* single);
static signed int VILN_gang_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_gang_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_gang_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_gang_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static inline void VSND_startTalk(st_zEntVillain* vil, zSFX* sfx);
static signed int VILN_gang_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static inline void VILN_gangIdle(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_gang_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_gang_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_gang_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_gang_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_gang_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_shagM_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_shagM_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shagM_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static inline en_BEHAVIOUR_GOAL VILN_tranGoalOf(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt);
static inline void VILN_enterGoal(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal);
static signed int VILN_shagM_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_shagM_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shagM_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shagM_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shagM_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shagM_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_shagM_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_shag_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_shag_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shag_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_shag_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_shag_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shag_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shag_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shag_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shag_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_shag_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static inline void VILN_nocollide(st_zEntVillain* vil);
static signed int VILN_shark_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_shark_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shark_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_shark_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static inline void VILN_settran(st_STIMULUS_CONTEXT_DATA* resp, signed int goal, signed int mode);
static signed int VILN_shark_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shark_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shark_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shark_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_shark_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_shark_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static signed int VILN_crit_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static signed int VILN_crit_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_crit_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static signed int VILN_crit_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static signed int VILN_crit_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_crit_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_crit_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_crit_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_crit_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_crit_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static inline signed int VANM_inDmgWindow(st_zEntVillain* vil);
static inline void VILN_lobSalvo(st_zEntVillain* vil);
static signed int VILN_kp_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_kp_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static void VILN_KookRender(_xEnt* ent);
static signed int VILN_std_xtrarend(st_zEntVillain* vil);
static signed int VILN_std_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot);
static inline void VILN_flipAway(float* knock, float push);
static inline void VILN_cmn_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_std_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static inline void VOUT_stop(_xEntFrame* frame);
static inline void VOUT_faceAng(st_zEntVillain* vil, float ang);
static signed int VILN_std_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame);
static inline void VILN_std_enterGoal(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal);
static inline void VILN_std_tymrRand(st_zEntVillain* vil, signed int idx);
static inline void VILN_std_flagsAll(st_zEntVillain* vil);
static inline void VILN_std_stunTimer(st_zEntVillain* vil, signed int idx);
static signed int VILN_std_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt);
static inline signed int VUTIL_hitSide(st_zEntVillain* vil);
static signed int VILN_std_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp);
static inline void VILN_std_awareGuardOrIdle(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp);
static inline void VILN_std_awareTymr(st_zEntVillain* vil, signed int idx);
static inline void VILN_std_awarePick(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, signed int spot);
static signed int VILN_std_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_std_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp);
static inline void VILN_clearDirectives(st_zEntVillain* vil);
static inline void VILN_sceneBegin();
static signed int VILN_std_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp);
static signed int VILN_std_query(st_zEntVillain* vil, en_ZVIL_QUERY query);
static signed int VILN_std_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal);
static void zVIL_setup_npcTable();
signed int zEntVillain_PlayAFXSound(st_zEntVillain* vil, zAnimFxSound* snd, unsigned int* sid_played);
void zEntVillain_Stun(st_zEntVillain* vil, float tym);
void zEntVillain_HeroWasHurt();
signed int zEntVillain_Bumped(st_zEntVillain* vil, _xCollis* colrec);
en_ZVILLAIN_BUBBLE_RESPONSE zEntVillain_Bubble(st_zEntVillain* vil, _zEntBubble* bub);
static inline void VTAL_hitOutcome(st_zEntVillain* vil);
signed int zEntVillain_Headbutt(st_zEntVillain* vil, _xVec3* dir);
unsigned int zEntVillain_Bounce(st_zEntVillain* vil);
void zEntVillain_BonusRender();
void zEntVillain_Load(st_zEntVillain* vil, xSerial* s);
void zEntVillain_Save(st_zEntVillain* vil, xSerial* s);
static inline void VCFG_setup(st_ZVIL_CONFIG_REFERENCE* cfg, p2EntAsset* asset);
void zEntVillain_InitDep(st_zEntVillain* vil, zScene* scn);
void fn_800A8F88(xBase* b, zScene* s);
void zEntVillain_Init(st_zEntVillain* vil, p2EntAsset* asset);
st_zEntVillain* zEntVillain_NPCInst(unsigned int npcidx);
st_zEntVillain* zEntVillain_MemPool(unsigned int npccnt);
xAnimTable* zEntVillain_AnimTable_Standard();
xAnimTable* zEntVillain_AnimTable_Gang();
xAnimTable* zEntVillain_AnimTable_Simple();
xAnimTable* zEntVillain_AnimTable_Witch();
xAnimTable* zEntVillain_AnimTable_Shark();
xAnimTable* zEntVillain_AnimTable_BossSham();
xAnimTable* zEntVillain_AnimTable_MasterMind();
xAnimTable* zEntVillain_AnimTable_RedBeard();
xAnimTable* zEntVillain_AnimTable_GreenGhost();
xAnimTable* zEntVillain_AnimTable_BlackKnight();
signed int zEntVillain_sceneFinish();
signed int zEntVillain_scenePrepare();
signed int zEntVillain_Shutdown();
signed int zEntVillain_Startup();

signed int zEntVillain_Startup()
{
    typedef signed int (*LogixFn)(zEntProjectile*, float, zScene*, _xEntFrame*);
    typedef signed int (*NotifyFn)(zEntProjectile*, en_PROJECTILE_EVENT, void*);
    signed int i;

    if (g_npcinit++ != 0)
    {
        return g_npcinit;
    }

    g_datablock = (char*)xMemAlloc(gActiveHeap, 0x20000);
    memset(g_datablock, 0, 0x20000);

    // Count the tables (the counts are not used).
    i = 0;
    while (g_npcTable[i++].enabled)
    {
    }
    i = 0;
    while (g_npcFuncs[i++].flg_funcs & 1)
    {
    }
    i = 0;
    while (g_animname[i++])
    {
    }

    for (i = 0; i < 147; i++)
    {
        g_anamhash[i] = xStrHash(g_animname[i]);
    }
    zVIL_setup_npcTable();
    for (i = 0; i < 147; i++)
    {
        g_anamhash[i] = xStrHash(g_animname[i]);
    }

    VPRJ_startup();
    return g_npcinit;
}

signed int zEntVillain_Shutdown()
{
    if (--g_npcinit != 0)
    {
        return g_npcinit;
    }
    return g_npcinit;
}

signed int zEntVillain_scenePrepare()
{
    if (g_npcpool && g_npccnt > 0)
    {
        memset(g_npcpool, 0x33, g_npccnt * sizeof(st_zEntVillain));
    }
    g_npccnt = 0;
    g_npcpool = NULL;
    XOrdInit(&g_emitlist, 0x20);
    g_emitpool = (st_zEntVillain_ParEmitRecord*)xMemAlloc(gActiveHeap, 0x400);
    memset(g_emitpool, 0, 0x400);
    g_emitnext = 0;
    g_emitready = 0;
    return 1;
}

signed int zEntVillain_sceneFinish()
{
    if (g_npcpool && g_npccnt > 0)
    {
        memset(g_npcpool, 0x33, g_npccnt * sizeof(st_zEntVillain));
    }
    g_npccnt = 0;
    g_npcpool = NULL;
    return g_npcinit;
}

xAnimTable* zEntVillain_AnimTable_BlackKnight()
{
    return VANM_mktable_BK();
}

xAnimTable* zEntVillain_AnimTable_GreenGhost()
{
    return VANM_mktable_GG();
}

xAnimTable* zEntVillain_AnimTable_RedBeard()
{
    return VANM_mktable_RB();
}

xAnimTable* zEntVillain_AnimTable_MasterMind()
{
    return VANM_mktable_MM();
}

xAnimTable* zEntVillain_AnimTable_BossSham()
{
    return VANM_mktable_SHAM();
}

xAnimTable* zEntVillain_AnimTable_Shark()
{
    return VANM_mktable_SHARK();
}

xAnimTable* zEntVillain_AnimTable_Witch()
{
    return VANM_mktable_WITCH();
}

xAnimTable* zEntVillain_AnimTable_Simple()
{
    return VANM_mktable_SIMP();
}

xAnimTable* zEntVillain_AnimTable_Gang()
{
    return VANM_mktable_GANG();
}

xAnimTable* zEntVillain_AnimTable_Standard()
{
    return VANM_mktable_STD();
}

st_zEntVillain* zEntVillain_MemPool(unsigned int npccnt)
{
    if (npccnt)
    {
        g_npccnt = npccnt;
        g_npcpool = (st_zEntVillain*)xMemAlloc(gActiveHeap, npccnt * sizeof(st_zEntVillain));
        memset(g_npcpool, 0, npccnt * sizeof(st_zEntVillain));
    }
    else
    {
        if (g_npcpool && g_npccnt > 0)
        {
            memset(g_npcpool, 0x33, g_npccnt * sizeof(st_zEntVillain));
        }
        g_npccnt = 0;
        g_npcpool = NULL;
    }
    return g_npcpool;
}

st_zEntVillain* zEntVillain_NPCInst(unsigned int npcidx)
{
    return &g_npcpool[npcidx];
}

void zEntVillain_Init(st_zEntVillain* vil, p2EntAsset* asset)
{
    st_ZVIL_NPCTYPE_LOOKUP* npcinfo;
    _xMat4x3* mat;

    zEntInit(vil, asset, 0x4E504320);
    vil->eventFunc = zVIL_elcb_event;
    vil->update = zVIL_elcb_update;
    vil->bupdate = zVIL_elcb_bupdate;
    vil->move = zVIL_elcb_move;
    vil->bound2.type = 1;
    if (vil->linkCount)
    {
        vil->link = (p2LinkAsset*)((char*)vil->asset + sizeof(p2EntAsset) + sizeof(p2EntVillainAsset));
    }
    else
    {
        vil->link = NULL;
    }
    vil->vhf = &g_npcFuncs[vil->asset->subtype];
    vil->vilcfg.asset = vil->asset;
    vil->vilcfg.vilp2ass = (p2EntVillainAsset*)((char*)vil->asset + sizeof(p2EntAsset));
    VCFG_readParms(&vil->vilcfg);
    VCFG_readAsset(&vil->vilcfg);

    npcinfo = &g_npcTable[vil->asset->subtype];
    vil->ability = npcinfo->ability;
    vil->abl_atak = npcinfo->abl_atak;
    vil->abl_dtct = npcinfo->abl_dtct;
    vil->abl_move = npcinfo->abl_move;
    vil->abl_spyk = npcinfo->abl_spyk;
    vil->abl_vuln = npcinfo->abl_vuln;
    vil->chkby = vil->vhf->flags(vil, ZVIL_FGRP_CHKBY, ZVIL_GOAL_NONE);
    vil->penby = vil->vhf->flags(vil, ZVIL_FGRP_PENBY, ZVIL_GOAL_NONE);
    vil->collis->chk = vil->vhf->flags(vil, ZVIL_FGRP_CHK, ZVIL_GOAL_NONE);
    vil->collis->pen = vil->vhf->flags(vil, ZVIL_FGRP_PEN, ZVIL_GOAL_NONE);
    vil->pflags = vil->vhf->flags(vil, ZVIL_FGRP_PHYS, ZVIL_GOAL_NONE);
    vil->typ_person = g_npcTable[vil->asset->subtype].typ_person;
    if (vil->flg_misc & 8)
    {
        VCFG_getParm(&vil->vilcfg, ZVIL_PARM_MDLSCL_BASE, &vil->asset->scale);
    }
    xEntReset(vil);
    vil->frame->rot.axis = g_Y3;
    vil->frame->rot.angle = vil->asset->ang.x;
    mat = (_xMat4x3*)vil->model->Mat;
    vil->pos_guard_home = mat->pos;
    xSceneID2Name(globals.sceneCur, vil->id);
    if (vil->subType == 0xF)
    {
        vil->render = VILN_KookRender;
    }
}

void fn_800A8F88(xBase* b, zScene* s)
{
    st_zEntVillain* vil = (st_zEntVillain*)b;

    VILN_restart(vil);

    switch (vil->vilcfg.vilp2ass->behaviorState)
    {
    case 0:
        vil->goaldflt = ZVIL_GOAL_IDLE;
        vil->mode = ZVIL_MODE_SHUTDOWN;
        VGOL_set(vil, ZVIL_GOAL_BASEINIT);
        vil->chkby = 0;
        vil->penby = 0;
        vil->flags &= ~1;
        break;
    case 3:
        vil->goaldflt = ZVIL_GOAL_AMBUSH;
        vil->mode = ZVIL_MODE_SHUTDOWN;
        VGOL_set(vil, ZVIL_GOAL_BASEINIT);
        vil->pflags = 0;
        vil->collis->chk = 0;
        vil->collis->pen = 0;
        vil->chkby = 0;
        vil->penby = 0;
        vil->flags &= ~1;
        break;
    case 1:
        vil->goaldflt = ZVIL_GOAL_IDLE;
        vil->mode = ZVIL_MODE_SHUTDOWN;
        VGOL_set(vil, ZVIL_GOAL_STARTUP);
        break;
    case 2:
        vil->goaldflt = ZVIL_GOAL_BORED;
        vil->mode = ZVIL_MODE_SHUTDOWN;
        VGOL_set(vil, ZVIL_GOAL_STARTUP);
        break;
    default:
        vil->goaldflt = ZVIL_GOAL_IDLE;
        vil->mode = ZVIL_MODE_SHUTDOWN;
        VGOL_set(vil, ZVIL_GOAL_STARTUP);
        break;
    }

    {
        signed int spawn;
        signed int mindiff;
        signed int spawnmask;

        spawnmask = 0;
        mindiff = 0;
        spawn = 1;
        VCFG_getParm(&vil->vilcfg, ZVIL_PARM_MINDIFFICULTY, &mindiff);
        VCFG_getParm(&vil->vilcfg, ZVIL_PARM_SPAWNMASK, &spawnmask);
        if (mindiff < 0)
        {
            spawn = 0;
            xSceneID2Name(globals.sceneCur, vil->id);
        }
        if ((unsigned int)spawnmask != (spawnmask & globals.player.Inv_Specials))
        {
            spawn = 0;
        }
        if (!spawn)
        {
            VGOL_set(vil, ZVIL_GOAL_BASEINIT);
        }
    }

    if ((vil->asset->flags & 1) && vil->mode != ZVIL_MODE_SHUTDOWN)
    {
        vil->flags |= 1;
    }
    else
    {
        vil->flags &= ~1;
    }
    vil->nav_prev = NULL;
    vil->nav_next = NULL;
    if (vil->nav_mvptlist)
    {
        vil->need_navpt = 1;
    }
    vil->flg_misc &= 0x3FFFFFFF;
    vil->tgt_stat = 0;

    if (zEntIsShaggy(vil))
    {
        zEntShaggy_Init(vil);
        st_STIMULUS_CONTEXT_DATA stimctxt = {};
        VILN_tran(&stimctxt, vil->goaldflt, ZVIL_MODE_NORMAL);
        zVIL_trangoal(vil, &stimctxt, globals.update_dt);
    }
}

void zEntVillain_InitDep(st_zEntVillain* vil, zScene* scn)
{
    _xMat4x3* mat;

    xEntReset(vil);
    memset(&vil->vhf, 0, sizeof(st_zEntVillain) - 0xEC);
    vil->eventFunc = zVIL_elcb_event;
    vil->update = zVIL_elcb_update;
    vil->bupdate = zVIL_elcb_bupdate;
    vil->move = zVIL_elcb_move;
    vil->bound2.type = 1;
    if (vil->linkCount)
    {
        vil->link = (p2LinkAsset*)((char*)vil->asset + sizeof(p2EntAsset) + sizeof(p2EntVillainAsset));
    }
    else
    {
        vil->link = NULL;
    }
    vil->vhf = &g_npcFuncs[vil->asset->subtype];
    VCFG_setup(&vil->vilcfg, vil->asset);

    vil->ability = g_npcTable[vil->asset->subtype].ability;
    vil->abl_atak = g_npcTable[vil->asset->subtype].abl_atak;
    vil->abl_dtct = g_npcTable[vil->asset->subtype].abl_dtct;
    vil->abl_move = g_npcTable[vil->asset->subtype].abl_move;
    vil->abl_spyk = g_npcTable[vil->asset->subtype].abl_spyk;
    vil->abl_vuln = g_npcTable[vil->asset->subtype].abl_vuln;
    vil->chkby = vil->vhf->flags(vil, ZVIL_FGRP_CHKBY, ZVIL_GOAL_NONE);
    vil->penby = vil->vhf->flags(vil, ZVIL_FGRP_PENBY, ZVIL_GOAL_NONE);
    vil->collis->chk = vil->vhf->flags(vil, ZVIL_FGRP_CHK, ZVIL_GOAL_NONE);
    vil->collis->pen = vil->vhf->flags(vil, ZVIL_FGRP_PEN, ZVIL_GOAL_NONE);
    vil->pflags = vil->vhf->flags(vil, ZVIL_FGRP_PHYS, ZVIL_GOAL_NONE);
    vil->typ_person = g_npcTable[vil->asset->subtype].typ_person;
    if (vil->flg_misc & 8)
    {
        VCFG_getParm(&vil->vilcfg, ZVIL_PARM_MDLSCL_BASE, &vil->asset->scale);
    }
    xEntReset(vil);
    vil->frame->rot.axis = g_Y3;
    vil->frame->rot.angle = vil->asset->ang.x;
    mat = (_xMat4x3*)vil->model->Mat;
    vil->pos_guard_home = mat->pos;
    xSceneID2Name(globals.sceneCur, vil->id);
    fn_800A8F88(vil, scn);
}

static inline void VCFG_setup(st_ZVIL_CONFIG_REFERENCE* cfg, p2EntAsset* asset)
{
    cfg->asset = asset;
    cfg->vilp2ass = (p2EntVillainAsset*)((char*)asset + sizeof(p2EntAsset));
    VCFG_readParms(cfg);
    VCFG_readAsset(cfg);
}

void zEntVillain_Save(st_zEntVillain* vil, xSerial* s)
{
    signed int state = 0;

    if (!(vil->baseFlags & 2))
    {
        return;
    }
    zEntSave(vil, s);
    if (vil->goal == ZVIL_GOAL_AFTERLIFE || vil->goal == ZVIL_GOAL_BASEINIT)
    {
        state = 1;
    }
    else if (vil->mode > ZVIL_MODE_SHUTDOWN && vil->mode < 7 && !vil->vilcfg.vilp2ass->behaviorState)
    {
        state = 2;
    }
    s->Write_b2(state);
}

void zEntVillain_Load(st_zEntVillain* vil, xSerial* s)
{
    signed int state = 0;

    if (!(vil->baseFlags & 2))
    {
        return;
    }
    zEntLoad(vil, s);
    s->Read_b2(&state);
    if (state & 1)
    {
        vil->goaldflt = ZVIL_GOAL_IDLE;
        vil->mode = ZVIL_MODE_SHUTDOWN;
        VGOL_set(vil, ZVIL_GOAL_BASEINIT);
        vil->chkby = 0;
        vil->penby = 0;
        vil->flags &= ~1;
    }
    else if (state & 2)
    {
        vil->goaldflt = ZVIL_GOAL_IDLE;
        vil->mode = ZVIL_MODE_SHUTDOWN;
        VGOL_set(vil, ZVIL_GOAL_STARTUP);
    }
}

void zEntVillain_BonusRender()
{
    signed int cnt = g_npccnt;
    st_zEntVillain* vil;
    signed int i;

    for (i = 0; i < cnt; i++)
    {
        vil = &g_npcpool[i];

        if (vil->goal == ZVIL_GOAL_GUMMED)
        {
            VEFX_plop_gum(vil);
        }
        if (vil->mode >= ZVIL_MODE_NORMAL && vil->vhf->xtrarend)
        {
            xSceneID2Name(globals.sceneCur, vil->id);
            vil->vhf->xtrarend(vil);
        }
    }
}

unsigned int zEntVillain_Bounce(st_zEntVillain* vil)
{
    st_STIMULUS_CONTEXT_DATA stimctxt = { ZVIL_STIM_BOUNCED };

    stimctxt.res_buttbounce = (en_ZVILLAIN_BOUNCE_RESPONSE)0;
    zVIL_input_stim(vil, &stimctxt);
    if (stimctxt.res_buttbounce != 0 && stimctxt.res_buttbounce != 3)
    {
        VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)6, globals.update_dt);
    }
    VTAL_hitOutcome(vil);
    return stimctxt.res_buttbounce;
}

signed int zEntVillain_Headbutt(st_zEntVillain* vil, _xVec3* dir)
{
    st_STIMULUS_CONTEXT_DATA stimctxt = { ZVIL_STIM_HEADBUTT };

    stimctxt.res_headbutt = (en_ZVILLAIN_HEADBUTT_RESPONSE)0;
    if (dir)
    {
        float len = xVec3Normalize(&stimctxt.dir_headbutt, dir);
        _xVec3 dir_back = {};
        _xMat4x3* pmat = (_xMat4x3*)globals.player.ent.model->Mat;
        float dot;

        xVec3SMul(&dir_back, &vil->precalc.dir_VtoP, -1.0f);
        dot = dir_back.x * pmat->at.x + dir_back.y * pmat->at.y + dir_back.z * pmat->at.z;
        dot = VIL_MAX(0.0f, VIL_MIN(dot, 1.0f));
        stimctxt.mag_headbutt = VIL_MAX(0.0f, dot * len);
    }
    zVIL_input_stim(vil, &stimctxt);
    if (stimctxt.res_headbutt != 0 && stimctxt.res_headbutt != 4)
    {
        VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)7, globals.update_dt);
    }
    VTAL_hitOutcome(vil);
    return stimctxt.res_headbutt;
}

static inline void VTAL_hitOutcome(st_zEntVillain* vil)
{
    if (vil->ability & 4)
    {
        zEntPlayer_Notify((signed int)0x1F);
    }
    if (vil->goal == 0x20)
    {
        if (vil->ability & 0x200000)
        {
            if ((g_npcTable[vil->asset->subtype].ability & 0x200000) &&
                (g_npcTable[vil->asset->subtype].abl_vuln & 4))
            {
                g_stattally[vil->asset->subtype].cnt_killed++;
                g_stattally[50].cnt_killed++;
            }
        }
        if (vil->allow & 0x800)
        {
            VBON_drop_bonusItem(vil);
        }
    }
}

en_ZVILLAIN_BUBBLE_RESPONSE zEntVillain_Bubble(st_zEntVillain* vil, _zEntBubble* bub)
{
    st_STIMULUS_CONTEXT_DATA stimctxt = { ZVIL_STIM_GUMMED };
    unsigned char kind = *((unsigned char*)bub + 0x15);

    stimctxt.res_gods = 0;
    switch (kind)
    {
    case 1:
        stimctxt.stim = ZVIL_STIM_SOAPED;
        stimctxt.bubble = bub;
        zVIL_input_stim(vil, &stimctxt);
        if (stimctxt.res_bubble == 2)
        {
            VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)3, globals.update_dt);
            VSND_cue(vil, (en_NPC_SOUND)0x1A);
        }
        else if (stimctxt.res_bubble != 0)
        {
            zLaughNotify(0xB);
            VSND_cue(vil, (en_NPC_SOUND)0x19);
        }
        break;
    case 0:
        stimctxt.stim = ZVIL_STIM_GUMMED;
        stimctxt.bubble = bub;
        zVIL_input_stim(vil, &stimctxt);
        if (stimctxt.res_bubble == 2)
        {
            VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)5, globals.update_dt);
            VSND_cue(vil, (en_NPC_SOUND)0x1D);
        }
        else if (stimctxt.res_bubble != 0)
        {
            zLaughNotify(0xA);
            VSND_cue(vil, (en_NPC_SOUND)0x1C);
        }
        break;
    }
    if (vil->ability & 4)
    {
        zEntPlayer_Notify((signed int)0x1F);
    }
    if (kind == 0)
    {
        return (en_ZVILLAIN_BUBBLE_RESPONSE)2;
    }
    return stimctxt.res_bubble;
}

signed int zEntVillain_Bumped(st_zEntVillain* vil, _xCollis* colrec)
{
    st_STIMULUS_CONTEXT_DATA stimctxt = { ZVIL_STIM_BUMPED };

    stimctxt.stim = ZVIL_STIM_BUMPED;
    stimctxt.colrec = colrec;
    stimctxt.res_bumped = (en_ZVILLAIN_BUMPED_RESPONSE)0;
    zVIL_input_stim(vil, &stimctxt);
    return stimctxt.res_bumped;
}

void zEntVillain_HeroWasHurt()
{
}

void zEntVillain_Stun(st_zEntVillain* vil, float tym)
{
    st_STIMULUS_CONTEXT_DATA stimctxt = { ZVIL_STIM_STUNNING };

    stimctxt.amt_stun = (tym > 0.1f) ? tym : 0.1f;
    zVIL_input_stim(vil, &stimctxt);
    zLaughNotify(0xC);
}

signed int zEntVillain_PlayAFXSound(st_zEntVillain* vil, zAnimFxSound* snd, unsigned int* sid_played)
{
    st_NPC_SOUND_ENTRY* entry;
    st_NPC_SOUND_PROPERTIES* props;
    signed int found = -1;
    unsigned int sndID;
    signed int subtype;
    unsigned int sid;

    if (vil->goal >= 0 && vil->goal < 3 && vil->goaldflt != ZVIL_GOAL_AMBUSH)
    {
        // The original still reports success here.
        return 1;
    }

    sndID = snd->ID;
    subtype = vil->asset->subtype;
    for (entry = g_npcJukeBox; entry->sndList; entry++)
    {
        if (entry->viltyp == subtype || entry->viltyp != -1)
        {
            st_NPC_SOUND_RECORDS* rec;
            for (rec = entry->sndList; rec->typ_sound != 0x8000; rec++)
            {
                if (rec->aid_sound == sndID)
                {
                    found = rec->typ_sound;
                    break;
                }
            }
            if (entry->viltyp == subtype || found != -1)
            {
                break;
            }
        }
    }

    for (props = g_sndProps; props->sndEvent != 0x8000 && props->sndEvent != found; props++)
    {
    }

    if (found != -1)
    {
        sid = VSND_goPlay(vil, (en_NPC_SOUND)found, 0, props->flg_vsnd, g_npcTable[subtype].dst_noisy);
    }
    else
    {
        sid = VSND_goPlay(vil, (en_NPC_SOUND)0x8000, sndID, props->flg_vsnd, g_npcTable[subtype].dst_noisy);
    }
    if (sid_played)
    {
        *sid_played = sid;
    }
    return 1;
}

static void zVIL_setup_npcTable()
{
    signed int i;
    signed int person = 0;

    for (i = 0; i < 50; i++)
    {
        // Behaviour flags per villain type (the original builds them from
        // named flag constants; the values are what it compiles to). The
        // personality is only cleared once, before the loop.
        signed int vuln;
        signed int dtct;
        signed int spyk;
        signed int atak;
        signed int ability;
        unsigned char penby;
        unsigned char chkby;
        unsigned char pen;
        unsigned char chk;
        unsigned char physics;
        signed int move;

        switch (i)
        {
        case 0:
            person = 0x1;
            chkby = 0x10;
            penby = 0x10;
            chk = 0x10;
            pen = 0x10;
            physics = 0x1B;
            ability = 0x22804A;
            atak = 0x10003;
            dtct = 0x80;
            move = 0xC031BF;
            spyk = 0x0;
            vuln = 0x3F9E7;
            break;
        case 5:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1B;
            ability = 0x2A1F4A;
            atak = 0x40001;
            dtct = 0x800000B1;
            move = 0x31BF;
            spyk = 0x0;
            vuln = 0x1F9E7;
            break;
        case 2:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x281F4A;
            atak = 0x20001;
            dtct = 0xB1;
            move = 0x21BF;
            spyk = 0x0;
            vuln = 0x5F9E7;
            break;
        case 3:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x281FCA;
            atak = 0x20001;
            dtct = 0xB1;
            move = 0x21BF;
            spyk = 0x0;
            vuln = 0x5F9E7;
            break;
        case 4:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x281FCA;
            atak = 0x20001;
            dtct = 0xB1;
            move = 0x21BF;
            spyk = 0x0;
            vuln = 0x5F9E7;
            break;
        case 6:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x0;
            chk = 0x2E;
            pen = 0x0;
            physics = 0x1B;
            ability = 0x281F4A;
            atak = 0x10003;
            dtct = 0x80;
            move = 0x4313F;
            spyk = 0x0;
            vuln = 0x61;
            break;
        case 7:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x281F4A;
            atak = 0x20001;
            dtct = 0x80;
            move = 0x21BF;
            move &= ~0x80;
            spyk = 0x0;
            vuln = 0x5F9E7;
            break;
        case 9:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x299F4A;
            atak = 0x20001;
            dtct = 0x80;
            move = 0x213F;
            spyk = 0x0;
            vuln = 0x5F9E7;
            break;
        case 12:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x281FCA;
            atak = 0x20001;
            dtct = 0xB1;
            move = 0x21BF;
            spyk = 0x0;
            vuln = 0x8001F8E7;
            break;
        case 13:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x281F4A;
            atak = 0x20001;
            dtct = 0x80;
            move = 0x213F;
            move &= ~0x80;
            spyk = 0x0;
            vuln = 0x5F9E7;
            break;
        case 15:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x281F4A;
            atak = 0x80001;
            dtct = 0xB1;
            move = 0x21BF;
            spyk = 0x0;
            vuln = 0x5F9E7;
            break;
        case 16:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x281F4A;
            atak = 0x40001;
            dtct = 0xB1;
            move = 0x29BF;
            spyk = 0x0;
            vuln = 0x4005F9E7;
            break;
        case 17:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1B;
            ability = 0x2A1F4A;
            atak = 0x80001;
            dtct = 0x800000B1;
            move = 0x31BF;
            spyk = 0x0;
            vuln = 0x1F9E7;
            break;
        case 18:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x281F4A;
            atak = 0x40001;
            dtct = 0xB1;
            move = 0x21BF;
            spyk = 0x0;
            vuln = 0x5F9E7;
            break;
        case 19:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x20014A;
            atak = 0x20001;
            dtct = 0x80;
            move = 0x213F;
            move &= ~0x80;
            spyk = 0x0;
            vuln = 0x5F9E7;
            break;
        case 20:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x281F4A;
            atak = 0x20001;
            dtct = 0xB1;
            move = 0x21BF;
            spyk = 0x0;
            vuln = 0x5F9E7;
            break;
        case 45:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x8;
            chk = 0x2E;
            pen = 0x8;
            physics = 0x1B;
            ability = 0x281F4A;
            atak = 0x10001;
            dtct = 0xB1;
            move = 0x431BF;
            spyk = 0x0;
            vuln = 0x1F987;
            break;
        case 46:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x281FCA;
            atak = 0x20001;
            dtct = 0xB1;
            move = 0x21BF;
            spyk = 0x0;
            vuln = 0x5F9E7;
            break;
        case 21:
            person = 0x1;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x2080CA;
            atak = 0x0;
            dtct = 0x80;
            move = 0x613F;
            spyk = 0x0;
            vuln = 0x40107;
            break;
        case 14:
            person = 0x1;
            chkby = 0x0;
            penby = 0x0;
            chk = 0x20;
            pen = 0x20;
            physics = 0x0;
            ability = 0x20000A;
            atak = 0x0;
            dtct = 0x0;
            move = 0x0;
            spyk = 0x0;
            vuln = 0x0;
            break;
        case 22:
            person = 0x1;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x20804A;
            atak = 0x0;
            dtct = 0x80;
            move = 0x20213F;
            spyk = 0x0;
            vuln = 0x601E7;
            break;
        case 23:
            person = 0x1;
            chkby = 0x10;
            penby = 0x10;
            chk = 0x10;
            pen = 0x10;
            physics = 0x1B;
            ability = 0x20804A;
            atak = 0x0;
            dtct = 0x80;
            move = 0x10253F;
            spyk = 0x0;
            vuln = 0x201E7;
            break;
        case 24:
            person = 0x1;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x22804A;
            atak = 0x0;
            dtct = 0x80;
            move = 0xA13F;
            spyk = 0x0;
            vuln = 0x601E3;
            break;
        case 25:
            person = 0x1;
            chkby = 0x1C;
            penby = 0x0;
            chk = 0x2E;
            pen = 0x0;
            physics = 0x1B;
            ability = 0x281F4A;
            atak = 0x20001;
            dtct = 0xB1;
            move = 0x31BF;
            spyk = 0x0;
            vuln = 0x0;
            break;
        case 26:
            person = 0x1;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x281F4A;
            atak = 0x80001;
            dtct = 0x81;
            move = 0x2012;
            spyk = 0x0;
            vuln = 0x0;
            break;
        case 27:
            person = 0x1;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1B;
            ability = 0x22804A;
            atak = 0x10003;
            dtct = 0x80;
            move = 0x40313F;
            spyk = 0x0;
            vuln = 0x1F807;
            break;
        case 1:
            person = 0x5;
        case 41:
            if (!person)
            {
                person = 0x6;
            }
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x20014B;
            atak = 0x40001;
            dtct = 0x1;
            move = 0x2137;
            spyk = 0x0;
            vuln = 0x9;
            break;
        case 10:
            person = 0x5;
        case 44:
            if (!person)
            {
                person = 0x6;
            }
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x2001C3;
            atak = 0x40001;
            dtct = 0x1;
            move = 0x137;
            spyk = 0x0;
            vuln = 0x109;
            break;
        case 11:
            person = 0x5;
        case 43:
            if (!person)
            {
                person = 0x6;
            }
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x20014B;
            atak = 0x80001;
            dtct = 0x1;
            move = 0x2137;
            spyk = 0x0;
            vuln = 0x9;
            break;
        case 8:
            person = 0x5;
        case 42:
            if (!person)
            {
                person = 0x6;
            }
            chkby = 0x1C;
            penby = 0x0;
            chk = 0x2E;
            pen = 0x0;
            physics = 0x1B;
            ability = 0x20014B;
            atak = 0x40001;
            dtct = 0x1;
            move = 0x43137;
            spyk = 0x0;
            vuln = 0x9;
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
        case 38:
        case 39:
        case 40:
        case 47:
        case 48:
        case 49:
            person = 0x7;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x48004;
            atak = 0x0;
            dtct = 0x1;
            move = 0x2137;
            spyk = 0x0;
            vuln = 0x0;
            break;
        default:
            person = 0x0;
            chkby = 0x1C;
            penby = 0x1C;
            chk = 0x2E;
            pen = 0x6E;
            physics = 0x1F;
            ability = 0x81F40;
            atak = 0x0;
            dtct = 0x80;
            move = 0x2;
            spyk = 0x40;
            vuln = 0x1F800;
            break;
        }

        if (move & 0x40000)
        {
            pen &= 0xBF;
        }
        if (move & 0x400)
        {
            pen &= 0xDF;
        }
        g_npcTable[i].typ_person = (en_NPC_PERSONALITIES)person;
        g_npcTable[i].flg_physics = physics;
        g_npcTable[i].flg_chk = chk;
        g_npcTable[i].flg_pen = pen;
        g_npcTable[i].flg_chkby = chkby;
        g_npcTable[i].flg_penby = penby;
        g_npcTable[i].ability = ability;
        g_npcTable[i].abl_atak = atak;
        g_npcTable[i].abl_dtct = dtct;
        g_npcTable[i].abl_move = move;
        g_npcTable[i].abl_spyk = spyk;
        g_npcTable[i].abl_vuln = vuln;

        if (!(g_npcFuncs[i].flg_funcs & 1))
        {
            g_npcFuncs[i].flg_funcs |= 2;
            g_npcFuncs[i].flags = VILN_std_flag;
            g_npcFuncs[i].query = VILN_std_query;
            g_npcFuncs[i].sysevent = VILN_std_sysevent;
            g_npcFuncs[i].notice = VILN_std_notice;
            g_npcFuncs[i].aware = VILN_std_aware;
            g_npcFuncs[i].stimulus = VILN_std_stimulus;
            g_npcFuncs[i].trangoal = VILN_std_trangoal;
            g_npcFuncs[i].calcmove = VILN_std_calcmove;
            g_npcFuncs[i].respond = VILN_std_respond;
            g_npcFuncs[i].pickanim = VILN_std_pickanim;
        }
    }
}

static signed int VILN_std_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal)
{
    return VILN_flagsFromTable(vil, flaggroup);
}

static signed int VILN_std_query(st_zEntVillain* vil, en_ZVIL_QUERY query)
{
    st_ZVIL_PRECALC* pc = &vil->precalc;

    switch (query)
    {
    case ZVIL_Q_DOUPDATE:
        return 1;
    case ZVIL_Q_ALIVE:
        return vil->mode >= ZVIL_MODE_NORMAL;
    case ZVIL_Q_ASLEEP:
        return vil->goal == ZVIL_GOAL_BORED && (vil->ability & 0x20000);
    case ZVIL_Q_PITFALL:
        return 0;
    case ZVIL_Q_HAVEANCHOR:
        if (vil->ability & 1)
        {
            return 0;
        }
        if (!(vil->allow & 0x80))
        {
            return 0;
        }
        if (!(vil->allow & 0x40))
        {
            return 0;
        }
        return (vil->flg_misc & 0x80000000) ? 1 : 0;
    case ZVIL_Q_HAVEGUARDPOS:
        if (vil->ability & 1)
        {
            return 0;
        }
        return (vil->flg_misc & 0x40000000) ? 1 : 0;
    case ZVIL_Q_DOFOLLOW:
        if (vil->ability & 1)
        {
            return 0;
        }
        if (!(vil->abl_move & 1))
        {
            return 0;
        }
        if (!(vil->abl_move & 0x100))
        {
            return 0;
        }
        if (vil->flg_misc & 0x10000000)
        {
            return 1;
        }
        if (vil->flg_misc & 0x20000000)
        {
            return 1;
        }
        return 0;
    case ZVIL_Q_NEEDNAV:
        if (!vil->vhf->query(vil, ZVIL_Q_CANPATROL))
        {
            return 0;
        }
        if (!vil->need_navpt)
        {
            return 0;
        }
        if (vil->nav_next && !(vil->tmr_list[3] <= 0.0f))
        {
            return 0;
        }
        return 1;
    case ZVIL_Q_HAVENAV:
        if (!vil->need_navpt && vil->nav_next && zMovePointIsOn(vil->nav_next))
        {
            return 1;
        }
        return 0;
    case ZVIL_Q_CANGUARD:
        return (vil->abl_move & 0x10) && (vil->allow & 2);
    case ZVIL_Q_CANPATROL:
        if (!(vil->allow & 1))
        {
            return 0;
        }
        if (!(vil->abl_move & 0x20))
        {
            return 0;
        }
        if (!(vil->abl_move & 4))
        {
            return 0;
        }
        if (!(vil->tmr_list[ZVIL_TDUR_NAVDELAY] <= 0.0f))
        {
            return 0;
        }
        return vil->nav_mvptlist != NULL;
    case ZVIL_Q_CANCHASE:
        if (!vil->vhf->query(vil, ZVIL_Q_CANDETECT))
        {
            return 0;
        }
        if (!(vil->abl_move & 1))
        {
            return 0;
        }
        if (!(vil->abl_move & 0x80))
        {
            return 0;
        }
        if (!(vil->allow & 0x10))
        {
            return 0;
        }
        return 1;
    case ZVIL_Q_RUNAWAY:
        return 0;
    case ZVIL_Q_CANPURSUE:
        if (vil->ability & 1)
        {
            return 0;
        }
        if (g_scene_is_a_boss_battle > 2)
        {
            return 1;
        }
        if (!vil->vhf->query(vil, ZVIL_Q_CANCHASE))
        {
            return 0;
        }
        if (!vil->vhf->query(vil, ZVIL_Q_PLYRSEEN))
        {
            return 0;
        }
        if ((pc->flg_xtra & 1) && !(vil->abl_move & 0x1000) && !(vil->abl_move & 0x400))
        {
            return 0;
        }
        if (vil->vhf->query(vil, ZVIL_Q_HAVEANCHOR))
        {
            if (pc->ds2_anchor < vil->ds2_pursue)
            {
                return 1;
            }
            if (pc->ds2_plyrToAnchor < vil->ds2_pursue)
            {
                return 1;
            }
            if (pc->flg_plyr & 0x400)
            {
                return 1;
            }
            return 0;
        }
        return 1;
    case ZVIL_Q_CANATTACK:
    {
        float ang_face;
        float ang_plyr;

        if ((vil->ability & 1) && (vil->allow & 8))
        {
            return 1;
        }
        ang_plyr = pc->ang_VtoP;
        ang_face = vil->frame->rot.angle;
        if (!(vil->allow & 8))
        {
            return 0;
        }
        if (!vil->vhf->query(vil, ZVIL_Q_CANSEEPLYR))
        {
            return 0;
        }
        if (!(pc->flg_plyr & 1))
        {
            return 0;
        }
        if ((vil->abl_move & 0x80) && !(pc->flg_plyr & 8))
        {
            return 0;
        }
        if (!(pc->flg_plyr & 4))
        {
            return 0;
        }
        if (!vil->vhf->query(vil, ZVIL_Q_CANDETECT))
        {
            return 0;
        }
        if (vil->asset->subtype == 0xF)
        {
            float da = ang_face - ang_plyr;
            if (da < 0.0f)
            {
                da = -da;
            }
            if (!(da <= 0.34f))
            {
                return 0;
            }
        }
        return 1;
    }
    case ZVIL_Q_CANDETECT:
        if (!(vil->abl_dtct & 1))
        {
            return 0;
        }
        if (!(vil->allow & 4))
        {
            return 0;
        }
        return 1;
    case ZVIL_Q_PLYRTOOCLOSE:
        return 0;
    case ZVIL_Q_PLYRNEAR:
        if (vil->ability & 1)
        {
            return 1;
        }
        if (!(pc->plyr_mvinf & 0x100) && (pc->flg_plyr & 1))
        {
            return 1;
        }
        return (pc->flg_plyr & 1) ? 1 : 0;
    case ZVIL_Q_PLYRINFOV:
        return (pc->flg_plyr & 4) ? 1 : 0;
    case ZVIL_Q_PLYRINLOS:
        return (pc->flg_plyr & 8) ? 1 : 0;
    case ZVIL_Q_CANSEEPLYR:
        if (vil->ability & 1)
        {
            return 1;
        }
        if (!vil->vhf->query(vil, ZVIL_Q_CANDETECT))
        {
            return 0;
        }
        if (g_scene_is_a_boss_battle > 2 && (pc->flg_plyr & 8))
        {
            return 1;
        }
        if (!(vil->abl_dtct & 0x10))
        {
            return 0;
        }
        if (!(pc->flg_plyr & 1))
        {
            return 0;
        }
        if (!(pc->flg_plyr & 4))
        {
            return 0;
        }
        if (!(pc->flg_plyr & 8))
        {
            return 0;
        }
        if (vil->vhf->query(vil, ZVIL_Q_ASLEEP))
        {
            return 0;
        }
        if (pc->plyr_mvinf & 0x800)
        {
            return 0;
        }
        if (pc->plyr_mvinf & 0x1000)
        {
            return 0;
        }
        if ((pc->plyr_mvinf & 0x400) && !(pc->flg_plyr & 1))
        {
            return 0;
        }
        return 1;
    case ZVIL_Q_CANHEARPLYR:
        if (vil->ability & 1)
        {
            return 1;
        }
        if (!vil->vhf->query(vil, ZVIL_Q_CANDETECT))
        {
            return 0;
        }
        if (g_scene_is_a_boss_battle > 2)
        {
            return 0;
        }
        if (!(vil->abl_dtct & 0x20))
        {
            return 0;
        }
        if (!(pc->flg_plyr & 1))
        {
            return 0;
        }
        if (pc->plyr_mvinf & 0x400)
        {
            return 0;
        }
        if (pc->plyr_mvinf & 0x800)
        {
            return 0;
        }
        if (pc->plyr_mvinf & 0x1000)
        {
            return 0;
        }
        return 1;
    case ZVIL_Q_PLYRSEEN:
    {
        signed int near;

        if (vil->ability & 1)
        {
            return 1;
        }
        if (g_scene_is_a_boss_battle > 2)
        {
            return 1;
        }
        near = pc->flg_plyr & 1;
        if (!near)
        {
            return 0;
        }
        if (!(vil->tgt_stat & 0x20))
        {
            return 0;
        }
        if (!(pc->flg_plyr & 8))
        {
            return 0;
        }
        if (!(vil->tgt_stat & 8))
        {
            return 0;
        }
        if (pc->plyr_mvinf & 0x800)
        {
            return 0;
        }
        if (pc->plyr_mvinf & 0x1000)
        {
            return 0;
        }
        if ((pc->plyr_mvinf & 0x400) && !near)
        {
            return 0;
        }
        return 1;
    }
    case ZVIL_Q_PLYRHEARD:
        if (vil->ability & 1)
        {
            return 1;
        }
        if (g_scene_is_a_boss_battle > 2)
        {
            return 1;
        }
        if (vil->tgt_stat & 0x20)
        {
            return 1;
        }
        if (vil->tgt_stat & 4)
        {
            return 1;
        }
        return 0;
    default:
        return 1;
    }
}

static signed int VILN_std_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam,
                                    xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp)
{
    switch (toEvent)
    {
    case 0:
    case 0x59:
    case 0x5A:
    case 0x24:
    case 0xBC:
    case 0xBD:
        break;
    case 0x57:
        VILN_sceneBegin();
        break;
    case 0x58:
        g_scene_is_a_boss_battle = 0;
        g_bonusTableReady = 0;
        g_NPCSoundStreamLock = 0;
        break;
    case 0x3:
        vil->flags |= 1;
        break;
    case 0x4:
        vil->flags &= ~1;
        break;
    case 0x25:
        resp->stim = ZVIL_STIM_KILLSELF;
        break;
    case 0x33:
        resp->stim = ZVIL_STIM_DOTAUNT;
        vil->mode = (vil->mode < ZVIL_MODE_NORMAL) ? vil->mode : ZVIL_MODE_NORMAL;
        break;
    case 0x28:
        vil->allow |= 1;
        VILN_clearDirectives(vil);
        resp->stim = ZVIL_STIM_STARTPATROL;
        break;
    case 0x29:
        vil->allow &= ~1;
        VILN_clearDirectives(vil);
        resp->stim = ZVIL_STIM_STOPPATROL;
        break;
    case 0x2A:
        vil->allow |= 2;
        VILN_clearDirectives(vil);
        resp->stim = ZVIL_STIM_STARTGUARD;
        break;
    case 0x2B:
        vil->allow &= ~2;
        VILN_clearDirectives(vil);
        resp->stim = ZVIL_STIM_STOPGUARD;
        break;
    case 0x2C:
        vil->allow |= 4;
        break;
    case 0x2D:
        vil->allow &= ~4;
        break;
    case 0x2E:
        vil->allow |= 0x10;
        break;
    case 0x2F:
        vil->allow &= ~0x10;
        break;
    case 0xFF:
        vil->allow |= 0x400;
        resp->stim = ZVIL_STIM_STARTTALK;
        if (toParamWidget)
        {
            resp->widge = toParamWidget;
        }
        break;
    case 0x100:
        vil->allow &= ~0x400;
        resp->stim = ZVIL_STIM_STOPTALK;
        break;
    case 0x31:
        resp->stim = ZVIL_STIM_WAKEUP;
        break;
    case 0x30:
        if ((vil->allow & 1) && vil->nav_mvptlist && vil->nav_next)
        {
            xSceneID2Name(globals.sceneCur, vil->id);
            vil->allow &= ~1;
        }
        resp->stim = ZVIL_STIM_GOTOSLEEP;
        break;
    case 0xAE:
        if (vil->ability & 1)
        {
            if (globals.player.Health)
            {
                if (vil->hitpts == 1)
                {
                    if (globals.player.ControlOffTimer > 0.0f)
                    {
                        globals.player.ControlOffTimer += 25.0f;
                    }
                    else
                    {
                        globals.player.ControlOffTimer = 25.0f;
                    }
                }
                resp->stim = ZVIL_STIM_HURTBOSS;
                resp->widge = toParamWidget;
            }
            else
            {
                resp->stim = ZVIL_STIM_DOTAUNT;
            }
        }
        break;
    case 0xAF:
        if ((vil->allow & 8) && (vil->abl_atak & 1))
        {
            resp->stim = ZVIL_STIM_ATTACKHERO;
        }
        break;
    case 0xB0:
        vil->allow |= 8;
        break;
    case 0xB1:
        vil->allow &= ~8;
        break;
    case 0xEC:
        vil->allow |= 0x20;
        break;
    case 0xED:
        vil->allow &= ~0x20;
        break;
    case 0xD9:
        if (toParamWidget && toParamWidget->id != vil->id)
        {
            resp->stim = ZVIL_STIM_FOLLOWWIDGET;
            resp->widge = toParamWidget;
            resp->off_goto = *toParam;
        }
        break;
    case 0xCD:
        if (toParamWidget && toParamWidget->id != vil->id)
        {
            resp->stim = ZVIL_STIM_GOTOWIDGET;
            resp->widge = toParamWidget;
            resp->off_goto = *toParam;
        }
        break;
    case 0x32:
    {
        signed int spawn;
        signed int mindiff;
        signed int spawnmask;

        spawnmask = 0;
        mindiff = 0;
        spawn = 1;

        VCFG_getParm(&vil->vilcfg, ZVIL_PARM_MINDIFFICULTY, &mindiff);
        VCFG_getParm(&vil->vilcfg, ZVIL_PARM_SPAWNMASK, &spawnmask);
        if (mindiff < 0)
        {
            spawn = 0;
            xSceneID2Name(globals.sceneCur, vil->id);
        }
        if ((unsigned int)spawnmask != (spawnmask & globals.player.Inv_Specials))
        {
            spawn = 0;
        }
        if (spawn)
        {
            VUTIL_stimulate(vil, ZVIL_STIM_GOTOAFTERLIFE);
            VUTIL_stimulate(vil, ZVIL_STIM_MAKEREADY);
            VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)0xC, globals.update_dt);
        }
        break;
    }
    case 0xA:
        if (vil->bubble)
        {
            zEntBubble_Pop(vil->bubble);
        }
        vil->bubble = NULL;
        zEntVillain_InitDep(vil, globals.sceneCur);
        break;
    case 0xEA:
        resp->stim = ZVIL_STIM_DOACTION;
        break;
    case 0xEB:
        resp->stim = ZVIL_STIM_GANGBOSSACTION;
        break;
    case 0xF0:
        resp->stim = ZVIL_STIM_GUARDWIDGET;
        resp->widge = toParamWidget;
        resp->off_goto = *toParam;
        break;
    }
    return 1;
}

static inline void VILN_sceneBegin()
{
    signed int i;

    if (!g_bonusTableReady)
    {
        VBON_setup_bonusTable();
        g_gumModelAsset = (RpAtomic*)xSTFindAsset(xStrHash("gum_puddle"), NULL);
        VSND_setup_soundTable();
    }
    if (!g_emitready)
    {
        VPAR_readyize_emitters();
    }
    memset(g_stattally, 0, sizeof(g_stattally));
    g_stattotvil = 0;
    for (i = 0; i < g_npccnt; i++)
    {
        st_zEntVillain* vil = &g_npcpool[i];
        if (vil->abl_vuln & 4)
        {
            g_stattally[vil->subType].cnt_mortals++;
            g_stattally[50].cnt_mortals++;
        }
    }
}

static inline void VILN_clearDirectives(st_zEntVillain* vil)
{
    vil->flg_misc &= ~0x20000000;
    vil->flg_misc &= ~0x10000000;
    vil->flg_misc &= ~0x40000000;
    vil->flg_misc &= ~0x80000000;
    vil->ds2_goto_keepaway = 0.0f;
}

static signed int VILN_std_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp)
{
    return VILN_cmn_notice(vil, note, resp);
}

static signed int VILN_std_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp)
{
    float ang = 0.0f;

    if (g_scene_is_a_boss_battle > 2 && vil->asset->subtype != 7)
    {
        vil->tgt_stat = 0x0400887C;
    }

    switch (vil->goal)
    {
    case 0:
        break;
    case 1:
        vil->collis->chk = 0x26;
        vil->collis->pen = 0x66;
        vil->chkby = 0;
        vil->penby = 0;
        break;
    case 2:
        VILN_tran(resp, 3, 2);
        vil->collis->chk = 0x26;
        vil->collis->pen = 0x66;
        vil->chkby = 0;
        vil->penby = 0;
        break;
    case 3:
        VILN_tran(resp, vil->goaldflt, 3);
        break;
    case 4:
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x12))
        {
            VILN_tran(resp, 0x11, 5);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x16))
        {
            VILN_tran(resp, 0xF, 5);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x17))
        {
            VILN_tran(resp, 0xE, 4);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)6))
        {
            VILN_tran(resp, 8, 3);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)4))
        {
            VILN_tran(resp, 7, 3);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)5))
        {
            VILN_tran(resp, 7, 3);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)7))
        {
            VNAV_get_nav(vil);
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)8))
        {
            VILN_tran(resp, 6, 3);
            break;
        }
        if (vil->tmr_list[1] <= 0.0f && !(vil->ability & 0x20000))
        {
            VILN_tran(resp, 5, 3);
        }
        else if (vil->tmr_list[0] <= 0.0f)
        {
            VILN_std_awarePick(vil, (en_BEHAVIOUR_GOAL)4, 2);
            VILN_std_awareTymr(vil, 0);
        }
        break;
    case 5:
        VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)0xE, dt);
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x12))
        {
            VILN_tran(resp, 0x11, 5);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x16))
        {
            VILN_tran(resp, 0xF, 5);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x17))
        {
            VILN_tran(resp, 0xE, 4);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)6))
        {
            VILN_tran(resp, 8, 3);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)4))
        {
            VILN_tran(resp, 7, 3);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)5))
        {
            VILN_tran(resp, 7, 3);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)7))
        {
            VNAV_get_nav(vil);
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)8))
        {
            VILN_tran(resp, 6, 3);
            break;
        }
        if (vil->tmr_list[0] <= 0.0f && !(vil->ability & 0x20000))
        {
            VILN_std_awarePick(vil, (en_BEHAVIOUR_GOAL)5, 2);
            VILN_std_awareTymr(vil, 0);
        }
        break;
    case 31:
        if (vil->abl_move & 0x8000)
        {
            _xEntCollis* collis = vil->collis;
            if (collis->env_sidx != collis->env_eidx || collis->stat_sidx != collis->stat_eidx ||
                collis->dyn_sidx != collis->dyn_eidx)
            {
                VILN_tran(resp, 4, 3);
            }
        }
        break;
    case 8:
        if (!vil->vhf->query(vil, (en_ZVIL_QUERY)6))
        {
            VILN_tran(resp, 4, 3);
        }
        break;
    case 6:
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x12))
        {
            VILN_tran(resp, 0x11, 5);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x16))
        {
            VILN_tran(resp, 0xF, 5);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x17))
        {
            VILN_tran(resp, 0xE, 4);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)7) && VNAV_get_nav(vil))
        {
            VSND_play(vil, (en_NPC_SOUND)0xA);
        }
        if (!vil->vhf->query(vil, (en_ZVIL_QUERY)8))
        {
            VILN_std_awareGuardOrIdle(vil, resp);
        }
        break;
    case 7:
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)6))
        {
            VILN_tran(resp, 8, 3);
        }
        else if (!vil->vhf->query(vil, (en_ZVIL_QUERY)4))
        {
            vil->mode = (en_NPC_MODE)VIL_MIN(vil->mode, 4);
            if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x12))
            {
                VILN_tran(resp, 0x11, 5);
                break;
            }
            if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x16))
            {
                VILN_tran(resp, 0xF, 5);
                break;
            }
            if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x17))
            {
                VILN_tran(resp, 0xE, 4);
                break;
            }
            if (!vil->vhf->query(vil, (en_ZVIL_QUERY)5))
            {
                VILN_std_awareGuardOrIdle(vil, resp);
            }
        }
        break;
    case 9:
        if (vil->tmr_list[28] <= 0.0f)
        {
            VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)0xF, dt);
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x12))
        {
            VILN_tran(resp, 0x11, 5);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x16))
        {
            VILN_tran(resp, 0xF, 5);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x17))
        {
            VILN_tran(resp, 0xE, 4);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)6))
        {
            VILN_tran(resp, 8, 3);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)4))
        {
            VILN_tran(resp, 7, 3);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)5))
        {
            VILN_tran(resp, 7, 3);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)7))
        {
            VNAV_get_nav(vil);
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)8))
        {
            VILN_tran(resp, 6, 0);
            break;
        }
        if (vil->anim_playnext == 0x14 && vil->anim_playing)
        {
            if (vil->tmr_list[15] <= 0.0f)
            {
                VILN_std_awarePick(vil, (en_BEHAVIOUR_GOAL)9, 0);
                VILN_std_awareTymr(vil, 0);
            }
        }
        if (vil->tmr_list[0] <= 0.0f)
        {
            VILN_std_awarePick(vil, (en_BEHAVIOUR_GOAL)9, 2);
            VILN_std_awareTymr(vil, 0);
            vil->tmr_list[15] = 2.0f;
        }
        break;
    case 10:
        vil->tgt_stat = 0x0400887C;
        if (vil->tmr_list[ZVIL_TYMR_GENERIC] <= 0.0f)
        {
            VILN_tran(resp, 0x11, 5);
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)7))
        {
            VILN_tran(resp, 0x11, 5);
        }
        break;
    case 11:
        if (vil->tmr_list[ZVIL_TYMR_GENERIC] <= 0.0f)
        {
            VILN_tran(resp, 0x11, 5);
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)7))
        {
            VILN_tran(resp, 0x11, 5);
        }
        break;
    case 13:
        if (vil->tmr_list[28] <= 0.0f)
        {
            vil->tgt_stat &= ~2;
            if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x12))
            {
                VILN_tran(resp, 0x11, 5);
            }
            else if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x16))
            {
                VILN_tran(resp, 0xF, 5);
            }
            else if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x17) && !vil->vhf->query(vil, (en_ZVIL_QUERY)0x14))
            {
                VILN_tran(resp, 0xE, 4);
            }
            else if (vil->tmr_list[15] <= 0.0f)
            {
                VILN_tran(resp, 0x10, 4);
            }
        }
        break;
    case 14:
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x13))
        {
            vil->ang_listen = vil->precalc.ang_VtoP;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x12))
        {
            vil->tgt_stat &= ~2;
            VILN_tran(resp, 0x11, 5);
        }
        else if (vil->tmr_list[28] <= 0.0f)
        {
            if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x16))
            {
                VILN_tran(resp, 0xF, 5);
            }
            else if ((float)__fabs(vil->frame->rot.angle - vil->ang_listen) <= 0.2f)
            {
                VILN_tran(resp, 0xD, 0);
            }
            else if (vil->tmr_list[16] <= 0.0f)
            {
                VILN_tran(resp, 0xD, 4);
            }
        }
        break;
    case 15:
        if (vil->tmr_list[28] <= 0.0f)
        {
            vil->tgt_stat &= ~2;
            VILN_tran(resp, 0x11, 4);
        }
        break;
    case 16:
        vil->tgt_stat &= ~2;
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x12))
        {
            VILN_tran(resp, 0x11, 5);
        }
        else if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x16))
        {
            VILN_tran(resp, 0x11, 5);
        }
        else if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x17))
        {
            VILN_tran(resp, 0xE, 4);
        }
        else if (vil->tmr_list[17] <= 0.0f)
        {
            vil->tgt_stat = 0;
            VILN_std_awareGuardOrIdle(vil, resp);
        }
        break;
    case 17:
        if (!(vil->abl_move & 0x4000) && vil->tmr_list[28] < 0.3f)
        {
            ang = vil->frame->rot.angle;
            if ((float)__fabs(vil->frame->rot.angle - vil->precalc.ang_VtoP) <= 0.1f)
            {
                VILN_std_awarePick(vil, (en_BEHAVIOUR_GOAL)0x11, 0);
            }
            else if (!(vil->precalc.plyr_mvinf & 0xFF00))
            {
                VILN_std_awarePick(vil, (en_BEHAVIOUR_GOAL)0x11, 0xC);
            }
        }
        if (vil->anim_playing == 9)
        {
            // Still turning to face the player.
            float diff = __fabs(ang - vil->precalc.ang_VtoP);
            float fov = VIL_MAX(vil->fov_detect, 0.2f);
            if (diff <= 0.5 * fov)
            {
                break;
            }
        }
        if (g_scene_is_a_boss_battle && vil->asset->subtype == 0x11 && !(vil->tmr_list[28] <= 0.0f))
        {
            break;
        }
        if (!vil->vhf->query(vil, (en_ZVIL_QUERY)0x10))
        {
            VILN_tran(resp, 4, 3);
            break;
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x13) && vil->vhf->query(vil, (en_ZVIL_QUERY)0x16))
        {
            VILN_std_awareTymr(vil, 10);
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0xE))
        {
            unsigned int flg = vil->precalc.flg_plyr;
            if ((flg & 0x10) && (vil->abl_atak & 0x20000))
            {
                VILN_tran(resp, 0x14, 5);
                break;
            }
            if ((flg & 0x20) && (vil->abl_atak & 0x40000) && vil->tmr_list[19] <= 0.0f)
            {
                vil->prj_reserved = VWEP_lobReservePRJ(vil);
                if (vil->prj_reserved)
                {
                    VILN_tran(resp, 0x15, 5);
                    break;
                }
            }
            else if ((flg & 0x40) && (vil->abl_atak & 0x80000) && vil->tmr_list[20] <= 0.0f)
            {
                VILN_tran(resp, 0x16, 5);
                break;
            }
        }
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0xB) && vil->tmr_list[5] <= 0.0f &&
            vil->vhf->query(vil, (en_ZVIL_QUERY)0xD) &&
            ((vil->abl_atak & 0x10000) || (vil->abl_atak & 0x20000) || (vil->abl_atak & 0x40000) ||
             (vil->abl_atak & 0x80000)))
        {
            VILN_tran(resp, 0x13, 5);
            VILN_std_awareTymr(vil, 5);
            break;
        }
        if (vil->tmr_list[10] <= 0.0f)
        {
            VILN_tran(resp, 0x10, 4);
        }
        if (vil->tmr_list[2] <= 0.0f)
        {
            vil->tmr_list[2] = 3.2e7f;
        }
        break;
    case 19:
        if (!vil->vhf->query(vil, (en_ZVIL_QUERY)0x18))
        {
            VILN_tran(resp, 0x10, 4);
        }
        else if (!vil->vhf->query(vil, (en_ZVIL_QUERY)0xD))
        {
            VILN_tran(resp, 0x11, 5);
        }
        else if (vil->vhf->query(vil, (en_ZVIL_QUERY)0xE))
        {
            unsigned int flg = vil->precalc.flg_plyr;
            if ((flg & 0x10) && (vil->abl_atak & 0x20000))
            {
                VILN_tran(resp, 0x14, 5);
            }
            else if ((flg & 0x20) && (vil->abl_atak & 0x40000))
            {
                vil->prj_reserved = VWEP_lobReservePRJ(vil);
                if (vil->prj_reserved)
                {
                    VILN_tran(resp, 0x15, 5);
                }
            }
            else if ((flg & 0x40) && (vil->abl_atak & 0x80000))
            {
                VILN_tran(resp, 0x16, 5);
            }
        }
        break;
    case 20:
        if (VANM_inDmgWindow(vil) && !vil->goodattack)
        {
            vil->goodattack = VWEP_swipe(vil, dt);
            if (vil->goodattack)
            {
                if (vil->precalc.plyr_mvinf & 1)
                {
                    VIRM_initVilDflt(vil);
                }
                vil->tmr_list[26] = vil->vilcfg.tymref_list[26];
            }
        }
        if (vil->tmr_list[28] <= 0.0f)
        {
            if (vil->vhf->query(vil, (en_ZVIL_QUERY)0xC))
            {
                if (vil->nav_next)
                {
                    vil->pos_guard_home = *zMovePointGetPos(vil->nav_next);
                    vil->flg_misc |= 0x40000000;
                }
                else if (vil->nav_prev)
                {
                    vil->pos_guard_home = *zMovePointGetPos(vil->nav_prev);
                    vil->flg_misc |= 0x40000000;
                }
                else
                {
                    _xMat4x3* mat;

                    vil->pos_guard_home = vil->precalc.dir_VtoP;
                    vil->pos_guard_home.x *= -3.0f;
                    vil->pos_guard_home.y *= -3.0f;
                    vil->pos_guard_home.z *= -3.0f;
                    mat = (_xMat4x3*)vil->model->Mat;
                    vil->pos_guard_home.x += mat->pos.x;
                    vil->pos_guard_home.y += mat->pos.y;
                    vil->pos_guard_home.z += mat->pos.z;
                    vil->flg_misc |= 0x40000000;
                }
                VILN_tran(resp, 7, 6);
            }
            else if (vil->goodattack && (vil->ability & 0x100))
            {
                if (vil->tmr_list[26] <= 0.0f)
                {
                    VILN_tran(resp, 0x17, 0);
                }
            }
            else
            {
                VILN_tran(resp, 0x11, 5);
            }
        }
        break;
    case 21:
        if (vil->tmr_list[21] <= 0.0f)
        {
            VILN_std_awarePick(vil, (en_BEHAVIOUR_GOAL)0x15, 4);
            if (vil->anim_playnext == 0x1E && vil->anim_playing && VANM_inDmgWindow(vil) && vil->prj_reserved &&
                vil->cnt_lobSalvo > 0)
            {
                _zMovePoint* bull = vil->tgt_bullseye;
                if (bull && (vil->allow & 0x100))
                {
                    VWEP_lobActivatePRJ(vil->prj_reserved, vil, zMovePointGetPos(bull), NULL, &vil->vec_lobSpin,
                                        vil->fac_lobarc, vil->fac_lobheavy);
                }
                else if (vil->precalc.flg_plyr & 0x20)
                {
                    _xVec3 vec_off = {};
                    float spread;

                    vec_off.x = 0.5f - iurand();
                    vec_off.y = 0.5f - iurand();
                    vec_off.z = 0.5f - iurand();
                    xVec3Normalize(&vec_off, &vec_off);
                    spread = vil->cnt_lobSalvo - 1;
                    vec_off.x *= spread;
                    vec_off.y *= spread;
                    vec_off.z *= spread;
                    VWEP_lobActivatePRJ(vil->prj_reserved, vil, xBoundCenter(&globals.player.ent.bound), &vec_off,
                                        &vil->vec_lobSpin, vil->fac_lobarc, vil->fac_lobheavy);
                }
                else if (bull)
                {
                    VWEP_lobActivatePRJ(vil->prj_reserved, vil, zMovePointGetPos(bull), NULL, &vil->vec_lobSpin,
                                        vil->fac_lobarc, vil->fac_lobheavy);
                }
                vil->cnt_lobSalvo--;
                if (vil->cnt_lobSalvo > 0 && !vil->prj_reserved)
                {
                    vil->prj_reserved = VWEP_lobReservePRJ(vil);
                }
            }
            if (vil->tmr_list[28] <= 0.0f)
            {
                VILN_tran(resp, 0x11, 5);
            }
        }
        break;
    case 22:
        if (vil->anim_playnext == 0x21 && vil->anim_playing)
        {
            if (!vil->goodattack)
            {
                if (VANM_inDmgWindow(vil))
                {
                    vil->goodattack |= VWEP_extend_particles(vil, dt, &vil->pos_exttgt);
                }
                if (vil->goodattack)
                {
                    VIRM_initVilDflt(vil);
                }
                vil->tmr_list[26] = vil->vilcfg.tymref_list[26];
            }
            else if (VANM_inDmgWindow(vil))
            {
                VWEP_extend_particles(vil, dt, &vil->pos_exttgt);
            }
        }
        if (vil->tmr_list[18] <= 0.0f)
        {
            VILN_tran(resp, 0x11, 0);
        }
        else if (vil->goodattack && (vil->ability & 0x100) && vil->tmr_list[26] <= 0.0f)
        {
            VILN_tran(resp, 0x17, 0);
        }
        else if (vil->precalc.dst_VtoP > 1.5f * vil->dst_extend)
        {
            VILN_tran(resp, 0x11, 0);
        }
        break;
    case 23:
        if (vil->tmr_list[28] <= 0.0f)
        {
            if (vil->mode < 4)
            {
                if (vil->vhf->query(vil, (en_ZVIL_QUERY)9))
                {
                    VILN_tran(resp, 9, 3);
                }
                else
                {
                    VILN_tran(resp, 4, 3);
                }
            }
            else if (vil->vhf->query(vil, (en_ZVIL_QUERY)0xC))
            {
                if (vil->precalc.ds2_grdpos < 9.0f)
                {
                    vil->pos_guard_home = vil->precalc.dir_VtoP;
                    vil->pos_guard_home.x *= -3.0f;
                    vil->pos_guard_home.y *= -3.0f;
                    vil->pos_guard_home.z *= -3.0f;
                }
                vil->flg_misc |= 0x40000000;
                VILN_tran(resp, 7, 6);
            }
            else
            {
                VILN_tran(resp, 0x11, 0);
            }
        }
        break;
    case 24:
        VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)8, dt);
        if (vil->tmr_list[4] <= 0.0f)
        {
            VILN_std_awarePick(vil, (en_BEHAVIOUR_GOAL)0x18, 9);
            vil->tmr_list[4] = 3.2e7f;
        }
        else if (vil->tmr_list[12] <= 0.0f)
        {
            vil->hitpts = vil->vilcfg.hitpts;
            VILN_tran(resp, 0x11, 4);
        }
        break;
    case 25:
        VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)2, dt);
        if (vil->tmr_list[14] <= 0.0f)
        {
            VILN_tran(resp, 0x11, 0);
        }
        else if (vil->tmr_list[4] <= 0.0f)
        {
            VILN_std_awarePick(vil, (en_BEHAVIOUR_GOAL)0x19, 8);
            VSND_play(vil, (en_NPC_SOUND)0x1A);
            vil->tmr_list[4] = 3.2e7f;
            vil->tmr_list[0] = 3.2e7f;
        }
        else if (vil->tmr_list[0] <= 0.0f)
        {
            VILN_std_awarePick(vil, (en_BEHAVIOUR_GOAL)0x19, 7);
            vil->tmr_list[0] = 2.0f * (0.25f * (iurand() - 0.5f)) + 2.0f;
        }
        break;
    case 26:
        if (vil->mdl_gumBlob)
        {
            _xMat4x3* blob = (_xMat4x3*)vil->mdl_gumBlob->Mat;
            blob->pos = ((_xMat4x3*)vil->model->Mat)->pos;
            if (vil->asset->subtype == 0)
            {
                ((_xMat4x3*)vil->mdl_gumBlob->Mat)->pos.y -= 0.3f;
            }
        }
        VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)4, dt);
        if ((vil->abl_vuln & 0x200) && vil->tmr_list[25] <= 0.0f)
        {
            VILN_tran(resp, 0x20, 0);
        }
        else if (vil->tmr_list[4] <= 0.0f)
        {
            VILN_std_awarePick(vil, (en_BEHAVIOUR_GOAL)0x1A, 8);
            VSND_play(vil, (en_NPC_SOUND)0x1E);
            vil->tmr_list[4] = 3.2e7f;
            vil->tmr_list[0] = 3.2e7f;
        }
        else if (vil->tmr_list[13] <= 0.0f)
        {
            VILN_tran(resp, 0x11, 4);
        }
        else if (vil->tmr_list[0] <= 0.0f)
        {
            VILN_std_awarePick(vil, (en_BEHAVIOUR_GOAL)0x1A, 7);
            vil->tmr_list[0] = 3.0f * (0.25f * (iurand() - 0.5f)) + 3.0f;
        }
        break;
    case 32:
        if (vil->asset->subtype == 0x11 && sEmitWitchDying)
        {
            _zParEmitterCustomSettings pecust;
            _xMat4x3* mat = (_xMat4x3*)vil->model->Mat;

            pecust.custom_flags = 0x20;
            pecust.pos = mat->pos;
            pecust.pos.y += 1.0f;
            zParEmitterEmitCustom(sEmitWitchDying, dt, &pecust);
        }
        VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)0xA, dt);
        if (vil->tmr_list[28] <= 0.0f)
        {
            if ((vil->ability & 8) && vil->tmr_list[ZVIL_TYMR_GENERIC] <= 0.0f)
            {
                zHologramAdd(vil);
                VSND_play(vil, (en_NPC_SOUND)0x25);
                vil->tmr_list[23] = vil->vilcfg.tymref_list[23];
                vil->tmr_list[ZVIL_TYMR_GENERIC] = 3.2e7f;
            }
            VEFX_hologram(vil, dt);
            if (vil->tmr_list[23] <= 0.0f)
            {
                VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)0xB, dt);
                VILN_tran(resp, 0, 1);
            }
        }
        break;
    default:
        VILN_tran(resp, 4, 0);
        break;
    }
    return 1;
}

static inline void VILN_std_awarePick(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, signed int spot)
{
    vil->anim_loopmult = 0.97f;
    vil->vhf->pickanim(vil, goal, (en_ZVIL_GOAL_SPOT)spot);
}

static inline void VILN_std_awareTymr(st_zEntVillain* vil, signed int idx)
{
    float tym = vil->vilcfg.tymref_list[idx];
    vil->tmr_list[idx] = VUTIL_jitter(tym);
}

static inline void VILN_std_awareGuardOrIdle(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp)
{
    if (vil->vhf->query(vil, (en_ZVIL_QUERY)9))
    {
        resp->gotopos = ((_xMat4x3*)vil->model->Mat)->pos;
        VILN_tran(resp, 9, 3);
    }
    else
    {
        VILN_tran(resp, 4, 3);
    }
}

// Follow / mini patrol / patrol checks shared by the idle-type goals.
static signed int VILN_std_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp)
{
    _xVec3 vecC;
    float sp18;
    float sp1C;
    float sp20;
    float sp24;
    float sp28;
    float sp2C;
    float sp30;
    float sp34;
    float sp38;
    float sp3C;
    float sp40;
    float sp44;
    float sp48;
    float sp4C;
    float sp50;
    float sp8;
    signed int temp_cr0_eq;
    float temp_f0;
    float temp_f0_2;
    float temp_f0_3;
    float temp_f0_5;
    float temp_f0_6;
    float temp_f1;
    float temp_f1_2;
    float temp_f1_3;
    float temp_f1_4;
    float temp_f1_5;
    float temp_f1_6;
    float temp_f1_7;
    float temp_f1_8;
    float temp_f2;
    float temp_f2_2;
    float temp_f2_3;
    float temp_f3;
    float temp_f31;
    float temp_f31_2;
    float temp_f31_3;
    float temp_f31_4;
    float temp_f31_5;
    float temp_f31_6;
    float temp_f31_7;
    float temp_f3_2;
    float temp_f4;
    float temp_f5;
    signed int temp_r0;
    signed int temp_r0_2;
    signed int temp_r0_3;
    signed int temp_r0_4;
    signed int temp_r0_5;
    signed int temp_r0_6;
    signed int temp_r3;
    signed int temp_r30;
    signed int temp_r3_2;
    signed int temp_r3_3;
    signed int temp_r3_4;
    signed int temp_r3_5;
    signed int temp_r3_6;
    signed int temp_r3_7;
    _xVec3* temp_r3_8;
    st_NPC_TALLY_RECORD* temp_r5;
    _xMat4x3* temp_r5_6;
    _xMat4x3* temp_r5_7;
    _xMat4x3* temp_r5_8;
    st_NPC_TALLY_RECORD* temp_r5_9;
    float var_f0;
    float var_f0_2;
    float var_f0_3;
    float var_f0_4;
    float var_f1;
    float var_f1_2;
    float var_f1_3;
    float var_f2;
    float var_f2_2;
    float var_f5;
    float var_f6;

    temp_r3 = vil->goal;
    temp_r30 = stimctxt->stim;
    g_pgol = (en_BEHAVIOUR_GOAL)temp_r3;
    switch (temp_r3) {                              /* switch 1 */
    case 0:                                         /* switch 1 */
    case 1:                                         /* switch 1 */
    case 2:                                         /* switch 1 */
    case 3:                                         /* switch 1 */
    case 24:                                        /* switch 1 */
    case 32:                                        /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r30;
        switch (temp_r30) {                         /* switch 2; irregular */
        case 5:                                     /* switch 2 */
        case 0:                                     /* switch 2 */
            break;
        case 36:                                    /* switch 2 */
            temp_f0 = stimctxt->off_goto;
            vil->tgt_protect = (_zEnt*)stimctxt->widge;
            vil->ds2_goto_keepaway = temp_f0 * temp_f0;
            break;
        }
        break;
    case 25:                                        /* switch 1 */
    case 26:                                        /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r30;
        switch (temp_r30) {                         /* switch 3; irregular */
        case 36:                                    /* switch 3 */
            temp_f0_2 = stimctxt->off_goto;
            vil->tgt_protect = (_zEnt*)stimctxt->widge;
            vil->ds2_goto_keepaway = temp_f0_2 * temp_f0_2;
            break;
        case 34:                                    /* switch 3 */
            vil->hitpts = 0;
            VILN_tran(resp, 0x20, 7);
            break;
        case 35:                                    /* switch 3 */
            VILN_tran(resp, 0, 1);
            break;
        case 32:                                    /* switch 3 */
            stimctxt->res_gods = 2;
            if (vil->goal == 0x19) {
                temp_f31 = vil->vilcfg.tymref_list[14];
                vil->tmr_list[14] = VUTIL_jitter(temp_f31);
                vil->tmr_list[0] = (3.0f * (0.25f * (iurand() - 0.5f))) + 3.0f;
            }
            break;
        case 33:                                    /* switch 3 */
            stimctxt->res_gods = 2;
            if (vil->goal == 0x1A) {
                temp_f31_2 = vil->vilcfg.tymref_list[13];
                vil->tmr_list[13] = VUTIL_jitter(temp_f31_2);
                vil->tmr_list[0] = (3.0f * (0.25f * (iurand() - 0.5f))) + 3.0f;
            }
            break;
        }
        break;
    case 4:                                         /* switch 1 */
    case 5:                                         /* switch 1 */
    case 6:                                         /* switch 1 */
    case 7:                                         /* switch 1 */
    case 8:                                         /* switch 1 */
    case 9:                                         /* switch 1 */
    case 10:                                        /* switch 1 */
    case 11:                                        /* switch 1 */
    case 13:                                        /* switch 1 */
    case 14:                                        /* switch 1 */
    case 15:                                        /* switch 1 */
    case 16:                                        /* switch 1 */
    case 17:                                        /* switch 1 */
    case 19:                                        /* switch 1 */
    case 20:                                        /* switch 1 */
    case 21:                                        /* switch 1 */
    case 22:                                        /* switch 1 */
    case 23:                                        /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r30;
        if ((unsigned int)temp_r30 <= 0x25U) {
            switch (temp_r30) {                     /* switch 4 */
            case 34:                                /* switch 4 */
                vil->hitpts = 0;
                VILN_tran(resp, 0x20, 7);
                break;
            case 35:                                /* switch 4 */
                VILN_tran(resp, 0, 1);
                break;
            case 24:                                /* switch 4 */
                VILN_tran(resp, 0x1F, 0);
                break;
            case 37:                                /* switch 4 */
                VILN_tran(resp, 0x1F, 0);
                break;
            case 32:                                /* switch 4 */
                if (vil->abl_vuln & 0x40) {
                    stimctxt->res_gods = 1;
                    VILN_tran(resp, 0x19, 4);
                    temp_r5 = &g_stattally[vil->asset->subtype];
                    temp_r5->cnt_hitsoap += 1;
                    g_stattally[0x32].cnt_hitsoap += 1;
                } else {
                    stimctxt->res_gods = 2;
                }
                break;
            case 33:                                /* switch 4 */
                if (vil->abl_vuln & 0x20) {
                    stimctxt->res_gods = 1;
                    VILN_tran(resp, 0x1A, 4);
                    temp_r0 = vil->asset->subtype;
                    g_stattally[temp_r0].cnt_hitgum += 1;
                    g_stattally[0x32].cnt_hitgum += 1;
                } else {
                    stimctxt->res_gods = 2;
                }
                break;
            case 27:                                /* switch 4 */
                if (vil->ability & 4) {
                    stimctxt->res_gods = 3;
                } else if (VOUT_spikey_header(vil) == 4) {
                    stimctxt->res_gods = 4;
                } else if (vil->abl_vuln & 0x80) {
                    temp_r3_2 = vil->ability;
                    if ((temp_r3_2 & 0x80000) && !(temp_r3_2 & 0x80)) {
                        stimctxt->res_gods = 2;
                        temp_f1 = stimctxt->off_goto;
                        vil->dir_bowling.x = stimctxt->gotopos.x * temp_f1;
                        vil->dir_bowling.y = stimctxt->gotopos.y * temp_f1;
                        vil->dir_bowling.z = stimctxt->gotopos.z * temp_f1;
                    } else {
                        stimctxt->res_gods = 1;
                    }
                    if ((vil->ability & 0x80) || !(globals.player.Inv_Specials & 0x800)) {
                        VILN_tran(resp, 0x18, 5);
                    } else {
                        temp_r0_2 = vil->hitpts - 1;
                        vil->hitpts = temp_r0_2;
                        if (temp_r0_2 < 1) {
                            VILN_tran(resp, 0x20, 7);
                        } else {
                            VILN_tran(resp, 0x11, 5);
                        }
                    }
                } else {
                    stimctxt->res_gods = 0;
                }
                break;
            case 28:                                /* switch 4 */
                temp_r3_3 = vil->abl_spyk;
                if ((temp_r3_3 & 1) && (temp_r3_3 & 0x20)) {
                    stimctxt->res_gods = 3;
                } else if (vil->abl_vuln & 0x100) {
                    stimctxt->res_gods = 1;
                    if (vil->ability & 0x80) {
                        VILN_tran(resp, 0x18, 5);
                    } else {
                        temp_r0_3 = vil->hitpts - 1;
                        vil->hitpts = temp_r0_3;
                        if (temp_r0_3 < 1) {
                            VILN_tran(resp, 0x20, 7);
                        } else {
                            VILN_tran(resp, 0x11, 5);
                        }
                    }
                } else {
                    stimctxt->res_gods = 0;
                }
                break;
            case 29:                                /* switch 4 */
                if (!(vil->ability & 0x40)) {
                    stimctxt->res_gods = 0;
                } else {
                    temp_r3_4 = vil->abl_atak;
                    if (!(temp_r3_4 & 1)) {
                        stimctxt->res_gods = 3;
                    } else if (temp_r3_4 & 0x10000) {
                        stimctxt->res_gods = 3;
                    } else if (temp_r3_4 & 0xFF0000) {
                        stimctxt->res_gods = 3;
                    } else {
                        stimctxt->res_gods = 3;
                    }
                }
                if ((vil->goal == 5) && (vil->abl_move & 0x8000)) {
                    VILN_tran(resp, 0x1F, 3);
                    resp->use_alt_action = 1;
                }
                if ((vil->allow & 8) && (temp_r3_5 = vil->abl_atak, (((temp_r3_5 & 1) == 0) == 0)) && (temp_r3_5 & 0x20000)) {
                    VILN_tran(resp, 0x14, 4);
                    vil->ang_moveto = vil->precalc.ang_VtoP;
                    vil->domove |= 0x80000000;
                    vil->domove = vil->domove | 0x20000000 | 0x40;
                } else if (vil->ability & 0x100) {
                    VILN_tran(resp, 0x17, 4);
                } else {
                    if (vil->vhf->query(vil, (en_ZVIL_QUERY)0xC) != 0) {
                        if (vil->precalc.ds2_grdpos < 9.0f) {
                            vil->pos_guard_home.x = vil->precalc.dir_VtoP.x;
                            vil->pos_guard_home.y = vil->precalc.dir_VtoP.y;
                            vil->pos_guard_home.z = vil->precalc.dir_VtoP.z;
                            vil->pos_guard_home.x *= -3.0f;
                            vil->pos_guard_home.y *= -3.0f;
                            vil->pos_guard_home.z *= -3.0f;
                        }
                        vil->flg_misc |= 0x40000000;
                        VILN_tran(resp, 7, 6);
                    }
                }
                break;
            case 30:                                /* switch 4 */
                if (vil->abl_vuln & 0x40000) {
                    VILN_tran(resp, 0x18, 0);
                }
                break;
            case 19:                                /* switch 4 */
                VILN_tran(resp, 0xD, 4);
                break;
            case 21:                                /* switch 4 */
                VILN_tran(resp, 0xE, 4);
                break;
            case 20:                                /* switch 4 */
                VILN_tran(resp, 0xF, 4);
                break;
            case 22:                                /* switch 4 */
                VILN_tran(resp, 0xF, 4);
                break;
            case 23:                                /* switch 4 */
                if (vil->vhf->query(vil, (en_ZVIL_QUERY)0xE) != 0) {
                    VILN_tran(resp, 0x11, 5);
                } else {
                    xSceneID2Name(globals.sceneCur, vil->id);
                }
                break;
            case 6:                                 /* switch 4 */
                temp_r3_6 = vil->goal;
                if ((temp_r3_6 == 5) && (vil->abl_move & 0x8000)) {
                    VILN_tran(resp, 0x1F, 3);
                    resp->use_alt_action = 1;
                } else if (temp_r3_6 == 5) {
                    VILN_tran(resp, 4, 3);
                }
                break;
            case 7:                                 /* switch 4 */
                if (vil->mode < 4) {
                    VILN_tran(resp, 5, 3);
                }
                break;
            case 36:                                /* switch 4 */
                temp_f0_3 = stimctxt->off_goto;
                vil->tgt_protect = (_zEnt*)stimctxt->widge;
                vil->ds2_goto_keepaway = temp_f0_3 * temp_f0_3;
                break;
            case 8:                                 /* switch 4 */
                VILN_stimGotoPos(vil, stimctxt);
                VILN_tran(resp, 8, 3);
                break;
            case 9:                                 /* switch 4 */
                if ((stimctxt->widge != NULL) && (vil->abl_move & 0x100)) {
                    VOUT_dogoto_widget(vil, stimctxt->widge, stimctxt->off_goto, 0);
                    vil->tmr_list[24] = -1.0f;
                    VILN_tran(resp, 8, 3);
                }
                break;
            case 10:                                /* switch 4 */
                if ((stimctxt->widge != NULL) && (vil->abl_move & 0x100)) {
                    temp_cr0_eq = VOUT_dogoto_widget(vil, stimctxt->widge, stimctxt->off_goto, 1);
                    vil->tmr_list[24] = -1.0f;
                    if (temp_cr0_eq) {
                        VILN_tran(resp, 6, 3);
                    } else {
                        VILN_tran(resp, 8, 3);
                    }
                }
                break;
            case 11:                                /* switch 4 */
                if ((vil->goal == 5) && (vil->abl_move & 0x8000)) {
                    VILN_tran(resp, 0x1F, 3);
                    resp->use_alt_action = 1;
                } else {
                    vil->tmr_list[24] = -1.0f;
                    if (vil->vhf->query(vil, (en_ZVIL_QUERY)7) != 0) {
                        VNAV_get_nav(vil);
                    }
                    if (vil->vhf->query(vil, (en_ZVIL_QUERY)8) != 0) {
                        VILN_tran(resp, 6, 3);
                    } else {
                        VILN_tran(resp, 4, 3);
                    }
                }
                break;
            case 12:                                /* switch 4 */
                if (vil->mode < 4) {
                    if ((vil->allow & 2) && (vil->abl_move & 0x10)) {
                        temp_r5_6 = (_xMat4x3*)vil->model->Mat;
                        resp->gotopos.x = temp_r5_6->pos.x;
                        resp->gotopos.y = temp_r5_6->pos.y;
                        resp->gotopos.z = temp_r5_6->pos.z;
                        VILN_tran(resp, 9, 3);
                    } else {
                        VILN_tran(resp, 4, 3);
                    }
                }
                break;
            case 13:                                /* switch 4 */
                if (vil->mode < 4) {
                    if ((vil->allow & 2) && (vil->abl_move & 0x10)) {
                        temp_r5_7 = (_xMat4x3*)vil->model->Mat;
                        resp->gotopos.x = temp_r5_7->pos.x;
                        resp->gotopos.y = temp_r5_7->pos.y;
                        resp->gotopos.z = temp_r5_7->pos.z;
                        VILN_tran(resp, 9, 3);
                    } else {
                        VILN_tran(resp, 4, 3);
                    }
                }
                break;
            case 14:                                /* switch 4 */
                if (vil->mode < 4) {
                    VILN_tran(resp, 4, 3);
                }
                break;
            }
        }
        break;
    }
    temp_r3_7 = vil->goal;
    g_pgol = (en_BEHAVIOUR_GOAL)temp_r3_7;
    switch (temp_r3_7) {                            /* switch 5 */
    case 0:                                         /* switch 5 */
        g_pstm = (en_NPC_STIMULUS)temp_r30;
        switch (temp_r30) {                         /* switch 6; irregular */
        case 5:                                     /* switch 6 */
        case 0:                                     /* switch 6 */
            break;
        case 2:                                     /* switch 6 */
            VILN_tran(resp, 2, 1);
            break;
        }
        break;
    case 1:                                         /* switch 5 */
        g_pstm = (en_NPC_STIMULUS)temp_r30;
        switch (temp_r30) {                         /* switch 7; irregular */
        case 5:                                     /* switch 7 */
        case 0:                                     /* switch 7 */
            break;
        case 2:                                     /* switch 7 */
            VILN_tran(resp, 2, 1);
            break;
        }
        break;
    case 2:                                         /* switch 5 */
    case 3:                                         /* switch 5 */
    case 4:                                         /* switch 5 */
    case 5:                                         /* switch 5 */
    case 6:                                         /* switch 5 */
    case 8:                                         /* switch 5 */
    case 10:                                        /* switch 5 */
    case 11:                                        /* switch 5 */
        g_pstm = (en_NPC_STIMULUS)temp_r30;
        break;
    case 7:                                         /* switch 5 */
    case 9:                                         /* switch 5 */
    case 13:                                        /* switch 5 */
    case 14:                                        /* switch 5 */
    case 15:                                        /* switch 5 */
    case 16:                                        /* switch 5 */
    case 17:                                        /* switch 5 */
        g_pstm = (en_NPC_STIMULUS)temp_r30;
        switch (temp_r30) {                         /* switch 8; irregular */
        case 5:                                     /* switch 8 */
        case 0:                                     /* switch 8 */
            break;
        case 25:                                    /* switch 8 */
            if (vil->ability & 0x100) {
                VILN_tran(resp, 0x17, 0);
            }
            break;
        }
        break;
    case 19:                                        /* switch 5 */
        g_pstm = (en_NPC_STIMULUS)temp_r30;
        switch (temp_r30) {                         /* switch 9; irregular */
        case 5:                                     /* switch 9 */
        case 0:                                     /* switch 9 */
            break;
        case 29:                                    /* switch 9 */
            if (VUTIL_hitSide(vil) == 1) {
                VILN_tran(resp, 0x14, 5);
            }
            break;
        case 25:                                    /* switch 9 */
            if (vil->ability & 0x100) {
                VILN_tran(resp, 0x17, 0);
            }
            break;
        }
        break;
    case 20:                                        /* switch 5 */
    case 21:                                        /* switch 5 */
    case 22:                                        /* switch 5 */
    case 23:                                        /* switch 5 */
        g_pstm = (en_NPC_STIMULUS)temp_r30;
        switch (temp_r30) {                         /* switch 10; irregular */
        case 5:                                     /* switch 10 */
        case 0:                                     /* switch 10 */
            break;
        case 25:                                    /* switch 10 */
            if (vil->ability & 0x100) {
                VILN_tran(resp, 0x17, 0);
            }
            break;
        }
        break;
    case 24:                                        /* switch 5 */
        g_pstm = (en_NPC_STIMULUS)temp_r30;
        if ((unsigned int)temp_r30 <= 0x23U) {
            switch (temp_r30) {                     /* switch 11 */
            case 27:                                /* switch 11 */
                if (vil->abl_vuln & 0x80) {
                    if (vil->ability & 0x80000) {
                        stimctxt->res_gods = 2;
                    } else {
                        stimctxt->res_gods = 1;
                    }
                    if (!(globals.player.Inv_Specials & 0x800)) {
                        temp_f31_4 = vil->vilcfg.tymref_list[12];
                        vil->tmr_list[12] = VUTIL_jitter(temp_f31_4);
                    } else {
                        temp_r0_4 = vil->hitpts - 1;
                        vil->hitpts = temp_r0_4;
                        if (temp_r0_4 < 1) {
                            VILN_tran(resp, 0x20, 7);
                        } else {
                            temp_f31_5 = vil->vilcfg.tymref_list[12];
                            vil->tmr_list[12] = VUTIL_jitter(temp_f31_5);
                        }
                    }
                }
                break;
            case 28:                                /* switch 11 */
                if (vil->abl_vuln & 0x100) {
                    stimctxt->res_gods = 1;
                    temp_r0_5 = vil->hitpts - 1;
                    vil->hitpts = temp_r0_5;
                    if (temp_r0_5 < 1) {
                        VILN_tran(resp, 0x20, 7);
                    } else {
                        vil->anim_loopmult = 0.97f;
                        vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x18, (en_ZVIL_GOAL_SPOT)1);
                        temp_f31_6 = vil->vilcfg.tymref_list[12];
                        var_f2 = -1.0f;
                        vil->tmr_list[12] = VUTIL_jitter(temp_f31_6);
                        vil->tmr_list[4] = vil->tmr_list[12];
                        temp_f0_5 = -1.0f + vil->tmr_list[4];
                        if (temp_f0_5 < -1.0f) {

                        } else {
                            var_f2 = temp_f0_5;
                        }
                        vil->tmr_list[4] = var_f2;
                    }
                } else {
                    stimctxt->res_gods = 1;
                }
                break;
            case 30:                                /* switch 11 */
                if (vil->abl_vuln & 0x40000) {
                    temp_f31_7 = vil->vilcfg.tymref_list[12];
                    vil->tmr_list[12] = VUTIL_jitter(temp_f31_7);
                    temp_f1_3 = stimctxt->off_goto;
                    if (temp_f1_3 > 0.0f) {
                        var_f0 = 1.5f;
                        temp_f2 = vil->tmr_list[12] * temp_f1_3;
                        if (temp_f2 < 1.5f) {

                        } else {
                            var_f0 = temp_f2;
                        }
                        vil->tmr_list[12] = var_f0;
                    }
                    var_f1 = -1.0f;
                    vil->tmr_list[4] = vil->tmr_list[12];
                    temp_f0_6 = -1.0f + vil->tmr_list[4];
                    if (temp_f0_6 < -1.0f) {

                    } else {
                        var_f1 = temp_f0_6;
                    }
                    vil->tmr_list[4] = var_f1;
                    vil->anim_loopmult = 0.97f;
                    vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x18, (en_ZVIL_GOAL_SPOT)1);
                }
                break;
            case 34:                                /* switch 11 */
                vil->hitpts = 0;
                VILN_tran(resp, 0x20, 7);
                break;
            case 35:                                /* switch 11 */
                VILN_tran(resp, 0, 1);
                break;
            case 32:                                /* switch 11 */
                if (vil->abl_vuln & 0x40) {
                    stimctxt->res_gods = 1;
                    VILN_tran(resp, 0x19, 0);
                    temp_r5_9 = &g_stattally[vil->asset->subtype];
                    temp_r5_9->cnt_hitsoap += 1;
                    g_stattally[0x32].cnt_hitsoap += 1;
                }
                break;
            case 33:                                /* switch 11 */
                if (vil->abl_vuln & 0x20) {
                    stimctxt->res_gods = 1;
                    VILN_tran(resp, 0x1A, 0);
                    temp_r0_6 = vil->asset->subtype;
                    g_stattally[temp_r0_6].cnt_hitgum += 1;
                    g_stattally[0x32].cnt_hitgum += 1;
                }
                break;
            }
        }
        break;
    case 25:                                        /* switch 5 */
        g_pstm = (en_NPC_STIMULUS)temp_r30;
        switch (temp_r30) {                         /* switch 12; irregular */
        case 30:                                    /* switch 12 */
        case 5:                                     /* switch 12 */
        case 0:                                     /* switch 12 */
            break;
        case 27:                                    /* switch 12 */
            if (vil->ability & 0x100000) {
                _xVec3 vec_tmp = {};
                static float fac = 1.0f;
                float s = stimctxt->off_goto * g_npcTable[vil->asset->subtype].inv_mass;

                vec_tmp.x = stimctxt->gotopos.x * s;
                vec_tmp.y = stimctxt->gotopos.y * s;
                vec_tmp.z = stimctxt->gotopos.z * s;
                vec_tmp.y = (float)__fabs(vec_tmp.y);
                vil->vec_knocked.x += vec_tmp.x;
                vil->vec_knocked.y += vec_tmp.y;
                vil->vec_knocked.z += vec_tmp.z;
                vil->vec_knocked.x = VIL_MAX(-5.0f, VIL_MIN(vil->vec_knocked.x, 5.0f));
                vil->vec_knocked.y = VIL_MAX(-5.0f, VIL_MIN(vil->vec_knocked.y, 5.0f));
                vil->vec_knocked.z = VIL_MAX(-5.0f, VIL_MIN(vil->vec_knocked.z, 5.0f));
                vil->flg_misc |= 0x100000;
            }
            stimctxt->res_gods = 0;
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x19, (en_ZVIL_GOAL_SPOT)5);
            break;
        case 28:                                    /* switch 12 */
            if (vil->ability & 0x100000) {
                _xVec3 vec_tmp = {};

                vec_tmp.x = g_NY3.x * 2.5f;
                vec_tmp.y = g_NY3.y * 2.5f;
                vec_tmp.z = g_NY3.z * 2.5f;
                vil->vec_knocked.x += vec_tmp.x;
                vil->vec_knocked.y += vec_tmp.y;
                vil->vec_knocked.z += vec_tmp.z;
                vil->vec_knocked.y = VIL_MAX(-5.0f, VIL_MIN(vil->vec_knocked.y, 5.0f));
                vil->flg_misc |= 0x100000;
            }
            stimctxt->res_gods = 4;
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x19, (en_ZVIL_GOAL_SPOT)6);
            break;
        case 29:                                    /* switch 12 */
            stimctxt->res_gods = 1;
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x19, (en_ZVIL_GOAL_SPOT)5);
            break;
        }
        break;
    case 26:                                        /* switch 5 */
        g_pstm = (en_NPC_STIMULUS)temp_r30;
        switch (temp_r30) {                         /* switch 13; irregular */
        case 30:                                    /* switch 13 */
        case 5:                                     /* switch 13 */
        case 0:                                     /* switch 13 */
            break;
        case 27:                                    /* switch 13 */
            stimctxt->res_gods = 0;
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x1A, (en_ZVIL_GOAL_SPOT)5);
            break;
        case 28:                                    /* switch 13 */
            stimctxt->res_gods = 2;
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x1A, (en_ZVIL_GOAL_SPOT)6);
            break;
        case 29:                                    /* switch 13 */
            stimctxt->res_gods = 2;
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x1A, (en_ZVIL_GOAL_SPOT)5);
            break;
        }
        break;
    case 32:                                        /* switch 5 */
        g_pstm = (en_NPC_STIMULUS)temp_r30;
        break;
    }
    return 1;
}

static inline signed int VUTIL_hitSide(st_zEntVillain* vil)
{
    signed int side = 0;

    VSUP_ent_AOI(vil, &globals.player.ent, &side);
    return side;
}

static signed int VILN_std_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt)
{
    en_NPC_STIMULUS stim = stimctxt->stim;
    en_BEHAVIOUR_GOAL goal;

    if (stim == ZVIL_STIM_ENTERGOAL || stim == ZVIL_STIM_EXITGOAL)
    {
        goal = stimctxt->trangoal;
    }
    else
    {
        goal = vil->goal;
    }
    g_pgol = goal;

    switch (goal)
    {
    case ZVIL_GOAL_NONE:
        break;
    case ZVIL_GOAL_AFTERLIFE:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_AFTERLIFE);
            vil->chkby = 0;
            vil->penby = 0;
            vil->flags &= ~1;
            break;
        case ZVIL_STIM_EXITGOAL:
            vil->flags = vil->asset->flags;
            break;
        }
        break;
    case ZVIL_GOAL_BASEINIT:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_BASEINIT);
            vil->chkby = 0;
            vil->penby = 0;
            break;
        }
        break;
    case ZVIL_GOAL_STARTUP:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            st_ZVIL_NPCTYPE_LOOKUP* npcinfo;
            _xMat4x3* mat;

            VILN_std_enterGoal(vil, ZVIL_GOAL_STARTUP);
            vil->frame->dvel.x = 0.0f;
            vil->frame->dvel.y = 0.0f;
            vil->frame->dvel.z = 0.0f;
            vil->frame->vel.x = 0.0f;
            vil->frame->vel.y = 0.0f;
            vil->frame->vel.z = 0.0f;
            vil->frame->mode |= 0xC;

            // Re-run the dependent init.
            npcinfo = &g_npcTable[vil->asset->subtype];
            vil->ability = npcinfo->ability;
            vil->abl_atak = npcinfo->abl_atak;
            vil->abl_dtct = npcinfo->abl_dtct;
            vil->abl_move = npcinfo->abl_move;
            vil->abl_spyk = npcinfo->abl_spyk;
            vil->abl_vuln = npcinfo->abl_vuln;
            VILN_std_flagsAll(vil);
            vil->typ_person = g_npcTable[vil->asset->subtype].typ_person;
            if (vil->flg_misc & 8)
            {
                VCFG_getParm(&vil->vilcfg, ZVIL_PARM_MDLSCL_BASE, &vil->asset->scale);
            }
            xEntReset(vil);
            vil->frame->rot.axis = g_Y3;
            vil->frame->rot.angle = vil->asset->ang.x;
            mat = (_xMat4x3*)vil->model->Mat;
            vil->pos_guard_home = mat->pos;

            VILN_restart(vil);
            if ((vil->asset->flags & 1) && vil->mode != ZVIL_MODE_SHUTDOWN)
            {
                vil->flags |= 1;
            }
            else
            {
                vil->flags &= ~1;
            }
            vil->nav_prev = NULL;
            vil->nav_next = NULL;
            if (vil->nav_mvptlist)
            {
                vil->need_navpt = 1;
            }
            vil->flg_misc &= 0x3FFFFFFF;
            vil->tgt_stat = 0;
            break;
        }
        break;
    case ZVIL_GOAL_GOONLINE:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_GOONLINE);
            VILN_std_flagsAll(vil);
            vil->flags |= 1;
            break;
        }
        break;
    case ZVIL_GOAL_IDLE:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_IDLE);
            VILN_std_tymrRand(vil, 0);
            VILN_std_tymrRand(vil, 1);
            vil->tgt_stat = 0;
            break;
        }
        break;
    case ZVIL_GOAL_BORED:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_BORED);
            VILN_std_tymrRand(vil, 0);
            switch (vil->asset->subtype)
            {
            case 0:
            case 24:
                vil->pflags &= ~4;
                break;
            case 5:
            case 17:
                vil->pflags |= 4;
                break;
            default:
                if (vil->ability & 0x20000)
                {
                    vil->pflags |= 4;
                }
                break;
            }
            break;
        case ZVIL_STIM_EXITGOAL:
            vil->pflags = vil->vhf->flags(vil, ZVIL_FGRP_PHYS, ZVIL_GOAL_NONE);
            break;
        }
        break;
    case ZVIL_GOAL_ACTION:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            if (stimctxt->use_alt_action)
            {
                vil->anim_loopmult = 0.97f;
                vil->vhf->pickanim(vil, goal, (en_ZVIL_GOAL_SPOT)0xA);
            }
            else
            {
                VILN_std_enterGoal(vil, ZVIL_GOAL_ACTION);
            }
            if (vil->abl_move & 0x8000)
            {
                vil->pflags &= ~4;
            }
            break;
        case ZVIL_STIM_EXITGOAL:
            if (vil->abl_move & 0x8000)
            {
                vil->pflags = vil->vhf->flags(vil, ZVIL_FGRP_PHYS, ZVIL_GOAL_NONE);
            }
            break;
        }
        break;
    case ZVIL_GOAL_AMBUSH:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_AMBUSH);
            vil->pflags &= ~4;
            vil->collis->chk = 0;
            vil->collis->pen = 0;
            vil->chkby = 0;
            vil->penby = 0;
            vil->nav_prev = vil->nav_mvptlist;
            vil->nav_next = vil->nav_mvptlist;
            vil->need_navpt = 0;
            vil->tmr_list[3] = -1.0f;
            _xVec3 d = {};
            if (vil->nav_next)
            {
                _xMat4x3* mat = (_xMat4x3*)vil->model->Mat;
                _xVec3* pos = zMovePointGetPos(vil->nav_next);
                float len;

                d.x = pos->x - mat->pos.x;
                d.y = pos->y - mat->pos.y;
                d.z = pos->z - mat->pos.z;
                len = d.z * d.z + (d.x * d.x + d.y * d.y);
                if (len > 0.0f)
                {
                    len = xsqrt(len);
                }
                vil->dst_navTween = len;
                vil->tmr_list[ZVIL_TYMR_GENERIC] = vil->dst_navTween / 6.0f;
            }
            else
            {
                float d = 0.8f * vil->precalc.dst_VtoP;
                if (!(d < 2.5f))
                {
                    d = 2.5f;
                }
                vil->dst_navTween = d;
                vil->tmr_list[ZVIL_TYMR_GENERIC] = vil->dst_navTween / 6.0f;
            }
            break;
        case ZVIL_STIM_EXITGOAL:
            vil->pflags = vil->vhf->flags(vil, ZVIL_FGRP_PHYS, ZVIL_GOAL_NONE);
            vil->chkby = vil->vhf->flags(vil, ZVIL_FGRP_CHKBY, ZVIL_GOAL_NONE);
            vil->penby = vil->vhf->flags(vil, ZVIL_FGRP_PENBY, ZVIL_GOAL_NONE);
            vil->collis->chk = vil->vhf->flags(vil, ZVIL_FGRP_CHK, ZVIL_GOAL_NONE);
            vil->collis->pen = vil->vhf->flags(vil, ZVIL_FGRP_PEN, ZVIL_GOAL_NONE);
            break;
        }
        break;
    case ZVIL_GOAL_FOLLOW:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_FOLLOW);
            break;
        }
        break;
    case ZVIL_GOAL_PATROL:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_PATROL);
            vil->tgt_stat = 0;
            if (vil->nav_next == NULL)
            {
                vil->need_navpt = 1;
            }
            break;
        }
        break;
    case ZVIL_GOAL_MINIPATROL:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_MINIPATROL);
            break;
        }
        break;
    case ZVIL_GOAL_GUARD:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_GUARD);
            VILN_std_tymrRand(vil, 0);
            vil->tgt_stat = 0;
            if (!(vil->flg_misc & 0x40000000))
            {
                vil->pos_guard_home = stimctxt->pos_guard;
            }
            break;
        }
        break;
    case ZVIL_GOAL_LOOKFORHERO:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_LOOKFORHERO);
            VILN_std_tymrRand(vil, 15);
            vil->tgt_stat |= 2;
            vil->tgt_stat |= 0x04000000 | 0x8000;
            break;
        case ZVIL_STIM_EXITGOAL:
            vil->tgt_stat &= ~2;
            break;
        }
        break;
    case ZVIL_GOAL_LISTEN:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_LISTEN);
            VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)0x10, dt);
            VILN_std_tymrRand(vil, 16);
            vil->tgt_stat |= 6;
            vil->tgt_stat |= 0x04000000 | 0x8000;
            vil->ang_listen = vil->precalc.ang_VtoP;
            break;
        case ZVIL_STIM_EXITGOAL:
            vil->tgt_stat &= ~2;
            break;
        }
        break;
    case ZVIL_GOAL_SURPRIZE:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_SURPRIZE);
            VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)0x11, dt);
            vil->tgt_stat |= 0xC;
            vil->tgt_stat |= 0x12;
            vil->tgt_stat |= 0x20;
            vil->tgt_stat |= 0x820;
            break;
        case ZVIL_STIM_EXITGOAL:
            vil->tgt_stat &= ~2;
            break;
        }
        break;
    case ZVIL_GOAL_INVESTIGATE:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)0x10, dt);
            VILN_std_enterGoal(vil, ZVIL_GOAL_INVESTIGATE);
            VILN_std_tymrRand(vil, 17);
            vil->tgt_stat |= 0x42;
            break;
        case ZVIL_STIM_EXITGOAL:
            vil->tgt_stat &= ~2;
            break;
        }
        break;
    case ZVIL_GOAL_ATTACKREADY:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            signed int found = 0;

            VILN_std_enterGoal(vil, ZVIL_GOAL_ATTACKREADY);
            VILN_std_tymrRand(vil, 10);
            VILN_std_tymrRand(vil, 5);
            vil->goodattack = 0;
            vil->tgt_stat = 0x0400887C;
            if (!(vil->flg_misc & 0x80000000))
            {
                _zEnt* prot = vil->tgt_protect;
                if (prot)
                {
                    switch (prot->baseType)
                    {
                    case 13:
                        vil->pos_pursue_home = *zMovePointGetPos((_zMovePoint*)prot);
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
                        found = 1;
                        vil->pos_pursue_home = ((_xMat4x3*)prot->model->Mat)->pos;
                        break;
                    default:
                        xSceneID2Name(globals.sceneCur, vil->id);
                        xSceneID2Name(globals.sceneCur, vil->tgt_protect->id);
                        found = 0;
                        vil->tgt_protect = NULL;
                        vil->ds2_goto_keepaway = 0.0f;
                        break;
                    }
                }
                if (!found)
                {
                    vil->pos_pursue_home = ((_xMat4x3*)vil->model->Mat)->pos;
                    vil->ds2_goto_keepaway = 0.0f;
                }
                vil->flg_misc |= 0x80000000;
                vil->flg_misc &= ~0x40000000;
            }
            break;
        }
        break;
    case ZVIL_GOAL_CHASE:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_CHASE);
            break;
        case ZVIL_STIM_EXITGOAL:
            VILN_std_tymrRand(vil, 2);
            break;
        }
        break;
    case ZVIL_GOAL_CQCATTACK:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_CQCATTACK);
            vil->goodattack = 0;
            break;
        case ZVIL_STIM_EXITGOAL:
            vil->goodattack = 0;
            break;
        }
        break;
    case ZVIL_GOAL_LOBBING:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_LOBBING);
            vil->goodattack = 0;
            vil->tmr_list[21] = -1.0f;
            VCFG_getParm(&vil->vilcfg, (en_NPC_INI_PARAMETERS)0xC, &vil->cnt_lobSalvo);
            break;
        case ZVIL_STIM_EXITGOAL:
            vil->goodattack = 0;
            VILN_std_tymrRand(vil, 19);
            if (vil->prj_reserved)
            {
                vil->prj_reserved->Abort();
            }
            vil->prj_reserved = NULL;
            break;
        }
        break;
    case ZVIL_GOAL_SPELLCAST:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            _zMovePoint* bull;

            VILN_std_enterGoal(vil, ZVIL_GOAL_SPELLCAST);
            vil->goodattack = 0;
            VILN_std_tymrRand(vil, 18);
            bull = vil->tgt_bullseye;
            if (bull && (vil->allow & 0x100))
            {
                vil->pos_exttgt = *zMovePointGetPos(bull);
            }
            else if (vil->precalc.flg_plyr & 0x40)
            {
                vil->pos_exttgt = ((_xMat4x3*)globals.player.ent.model->Mat)->pos;
            }
            else if (bull)
            {
                vil->pos_exttgt = *zMovePointGetPos(bull);
            }
            break;
        case ZVIL_STIM_EXITGOAL:
            vil->goodattack = 0;
            VILN_std_tymrRand(vil, 20);
            xSndStopChildren((unsigned int)vil + 3);
            break;
        }
        break;
    case ZVIL_GOAL_TAUNT:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_TAUNT);
            vil->tmr_list[11] = vil->vilcfg.tymref_list[11];
            break;
        }
        break;
    case ZVIL_GOAL_DAZED:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            float off;

            VILN_std_enterGoal(vil, ZVIL_GOAL_DAZED);
            VILN_std_tymrRand(vil, 12);
            off = stimctxt->off_goto;
            if (off > 0.0f)
            {
                float t = vil->tmr_list[12] * off;
                vil->tmr_list[12] = (t < 1.5f) ? 1.5f : t;
            }
            VILN_std_stunTimer(vil, 12);
            vil->flinch = 0;
            if (vil->abl_vuln & 0x80000000)
            {
                vil->abl_vuln |= 0x100;
                vil->abl_vuln &= ~0x80;
            }
            break;
        case ZVIL_STIM_EXITGOAL:
            if (vil->abl_vuln & 0x80000000)
            {
                vil->abl_vuln &= ~0x100;
                vil->abl_vuln |= 0x80;
            }
            break;
        }
        break;
    case ZVIL_GOAL_SOAPED:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_SOAPED);
            VILN_std_tymrRand(vil, 14);
            vil->tmr_list[0] = 2.0f * (0.25f * (iurand() - 0.5f)) + 2.0f;
            VILN_std_stunTimer(vil, 14);
            vil->pflags |= 0x20;
            vil->bubble = stimctxt->bubble;
            vil->pflags &= ~4;
            vil->collis->pen |= 0x62;
            vil->pos_moveto = ((_xMat4x3*)vil->model->Mat)->pos;
            break;
        case ZVIL_STIM_EXITGOAL:
            vil->pflags &= ~0x20;
            VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)3, dt);
            if (vil->bubble)
            {
                zEntBubble_Pop(vil->bubble);
            }
            vil->bubble = NULL;
            VILN_std_flagsAll(vil);
            if (vil->flg_misc & 0x100000)
            {
                VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)0x10, dt);
            }
            vil->flg_misc &= ~0x100000;
            vil->vec_knocked.x = 0.0f;
            vil->vec_knocked.y = 0.0f;
            vil->vec_knocked.z = 0.0f;
            vil->frame->vel.x = 0.0f;
            vil->frame->vel.y = 0.0f;
            vil->frame->vel.z = 0.0f;
            vil->frame->dvel.x = 0.0f;
            vil->frame->dvel.y = 0.0f;
            vil->frame->dvel.z = 0.0f;
            break;
        }
        break;
    case ZVIL_GOAL_GUMMED:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_GUMMED);
            VILN_std_tymrRand(vil, 13);
            vil->tmr_list[0] = 3.0f * (0.25f * (iurand() - 0.5f)) + 3.0f;
            vil->tmr_list[25] = vil->vilcfg.tymref_list[25];
            VILN_std_stunTimer(vil, 13);
            vil->bubble = NULL;
            if (g_gumModelAsset && !(vil->abl_move & 0x100000))
            {
                vil->mdl_gumBlob = xModelInstanceAlloc(g_gumModelAsset, vil, 0, 0, NULL);
                if (vil->mdl_gumBlob)
                {
                    VEFX_place_gumBlob(vil->mdl_gumBlob, (_xMat4x3*)vil->model->Mat);
                    if (vil->asset->subtype == 0)
                    {
                        ((_xMat4x3*)vil->mdl_gumBlob->Mat)->pos.y -= 0.3f;
                    }
                    if (g_npcFuncs[vil->asset->subtype].xtrarend == NULL)
                    {
                        g_npcFuncs[vil->asset->subtype].xtrarend = VILN_std_xtrarend;
                    }
                }
            }
            if (!(vil->abl_move & 0x100000))
            {
                vil->pflags |= 4;
                vil->collis->chk |= 0x26;
                vil->collis->pen |= 0x66;
            }
            break;
        case ZVIL_STIM_EXITGOAL:
            VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)5, dt);
            if (vil->bubble)
            {
                zEntBubble_Pop(vil->bubble);
            }
            if (vil->mdl_gumBlob)
            {
                xModelInstanceFree(vil->mdl_gumBlob);
            }
            vil->bubble = NULL;
            vil->mdl_gumBlob = NULL;
            VILN_std_flagsAll(vil);
            break;
        }
        break;
    case ZVIL_GOAL_DIE:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_std_enterGoal(vil, ZVIL_GOAL_DIE);
            vil->pflags |= 4;
            vil->chkby = 0;
            vil->penby = 0;
            vil->flg_misc &= 0x3FFFFFFF;
            vil->tmr_list[23] = vil->vilcfg.tymref_list[23];
            vil->tmr_list[ZVIL_TYMR_GENERIC] = -1.0f;
            vil->tgt_stat = 0;
            zEntEvent(vil, vil, 0x24);
            VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)9, dt);
            break;
        case ZVIL_STIM_EXITGOAL:
            zEntEvent(vil, vil, 0xEF);
            vil->collis->chk = 0;
            vil->collis->pen = 0;
            break;
        }
        break;
    }
    return 1;
}

static inline void VILN_std_stunTimer(st_zEntVillain* vil, signed int idx)
{
    float t;

    vil->tmr_list[4] = vil->tmr_list[idx];
    t = -1.0f + vil->tmr_list[4];
    vil->tmr_list[4] = (t < -1.0f) ? -1.0f : t;
}

static inline void VILN_std_flagsAll(st_zEntVillain* vil)
{
    vil->chkby = vil->vhf->flags(vil, ZVIL_FGRP_CHKBY, ZVIL_GOAL_NONE);
    vil->penby = vil->vhf->flags(vil, ZVIL_FGRP_PENBY, ZVIL_GOAL_NONE);
    vil->collis->chk = vil->vhf->flags(vil, ZVIL_FGRP_CHK, ZVIL_GOAL_NONE);
    vil->collis->pen = vil->vhf->flags(vil, ZVIL_FGRP_PEN, ZVIL_GOAL_NONE);
    vil->pflags = vil->vhf->flags(vil, ZVIL_FGRP_PHYS, ZVIL_GOAL_NONE);
}

static inline void VILN_std_tymrRand(st_zEntVillain* vil, signed int idx)
{
    float tym = vil->vilcfg.tymref_list[idx];
    vil->tmr_list[idx] = VUTIL_jitter(tym);
}

static inline void VILN_std_enterGoal(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal)
{
    vil->anim_loopmult = 0.97f;
    vil->vhf->pickanim(vil, goal, (en_ZVIL_GOAL_SPOT)0);
}

static signed int VILN_std_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame)
{
    _xVec3 entpos = {};

    entpos = ((_xMat4x3*)vil->model->Mat)->pos;
    switch (vil->goal)
    {
    case 0:
    case 1:
    case 2:
    case 3:
        VOUT_stop(frame);
        break;
    case 4:
    case 5:
        if (vil->abl_move & 0x8000)
        {
            VOUT_stop(frame);
        }
        break;
    case 31:
        if (vil->abl_move & 0x8000)
        {
            // Sink into the ground (ambush hide).
            vil->pos_moveto = entpos;
            vil->pos_moveto.y -= 1.0f;
            vil->spd_moveto = 1.0f;
            vil->domove |= 0x8000;
            vil->domove |= 0x800;
            vil->domove |= 4;
            vil->domove |= 0x80000000;
        }
        break;
    case 10:
        // Ambush: rise towards the ambush point (or the player), bobbing.
        if (vil->nav_next)
        {
            VOUT_patrol_move(vil);
        }
        else
        {
            vil->pos_moveto = ((_xMat4x3*)globals.player.ent.model->Mat)->pos;
            vil->domove |= 0x800;
            vil->domove |= 0xA;
            vil->domove |= 0x80000000;
        }
        vil->pos_moveto.y += 0.25f * dt * sin(vil->tmr_list[ZVIL_TYMR_CURGOAL]);
        vil->domove |= 4;
        vil->spd_moveto = 6.0f;
        vil->domove |= 0x8000;
        VOUT_stop(frame);
        if (vil->nav_next)
        {
            VOUT_chkdo_patrolArrived(vil);
        }
        break;
    case 8:
        VOUT_follow_move(vil);
        VOUT_chkdo_followArrived(vil);
        break;
    case 6:
        VOUT_patrol_move(vil);
        VOUT_chkdo_patrolArrived(vil);
        break;
    case 7:
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)4))
        {
            vil->pos_moveto = vil->pos_pursue_home;
        }
        else
        {
            vil->pos_moveto = vil->pos_guard_home;
        }
        VOUT_steer(vil);
        VOUT_chkdo_minipatrolArrived(vil);
        break;
    case 14:
        VOUT_faceAng(vil, vil->ang_listen);
        break;
    case 15:
        VOUT_faceAng(vil, vil->precalc.ang_VtoP);
        break;
    case 17:
        if (!(vil->precalc.plyr_mvinf & 0xFF00))
        {
            VOUT_faceAng(vil, vil->precalc.ang_VtoP);
        }
        break;
    case 19:
        VOUT_chase_move(vil);
        break;
    case 20:
    case 21:
        VOUT_faceAng(vil, vil->precalc.ang_VtoP);
        break;
    case 23:
        VOUT_faceAng(vil, vil->precalc.ang_VtoP);
        break;
    case 25:
        if ((vil->flg_misc & 0x100000) && xVec3Length2(&vil->vec_knocked) > 0.01f)
        {
            // Bubble still drifting from the knock.
            vil->vec_knocked.x *= 0.99f;
            vil->vec_knocked.y *= 0.99f;
            vil->vec_knocked.z *= 0.99f;
            vil->vel_moveto = vil->vec_knocked;
            vil->domove |= 0x1000;
            vil->domove |= 0x80000000;
        }
        else
        {
            vil->vec_knocked.x *= 0.0f;
            vil->vec_knocked.y *= 0.0f;
            vil->vec_knocked.z *= 0.0f;
            vil->pos_moveto.y += 0.5f * dt * sin(5.0f * vil->tmr_list[ZVIL_TYMR_CURGOAL]);
            vil->domove |= 0x800;
            vil->domove |= 4;
            vil->domove |= 0x80000000;
        }
        break;
    case 26:
        // Gummed villains that can still move slide along their path.
        if ((vil->abl_move & 0x100000) && vil->nav_next && vil->nav_prev)
        {
            float dist;

            vil->pos_moveto = *zMovePointGetPos(vil->nav_next);
            dist = VSUP_ds2_toPos(vil, vil->pos_moveto);
            vil->ang_moveto = VSUP_dir_toPos(vil, vil->pos_moveto, &vil->dir_moveto);
            if (vil->abl_move & 0x400)
            {
                float dy;

                vil->domove |= 4;
                dy = vil->pos_moveto.y - entpos.y;
                dy = dy * dy;
                if (!(dist > dy))
                {
                    dist = dy;
                }
            }
            vil->domove |= 0x800;
            vil->domove |= 1;
            vil->domove |= 0xA;
            vil->domove |= 0x80000000;
            if (dist < g_npcTable[vil->asset->subtype].ds2_atpos)
            {
                _zMovePoint* nav_old = vil->nav_next;
                _zMovePoint* nav_tmp = NULL;

                zMovePointGetNext(vil->nav_next, vil->nav_prev, &nav_tmp, NULL);
                if (vil->nav_next)
                {
                    vil->nav_prev = vil->nav_next;
                }
                vil->nav_next = nav_tmp;
                zEntEvent((xBase*)vil, (xBase*)nav_old, 0x1F);
            }
        }
        break;
    case 32:
        if ((vil->ability & 0x80000) && (vil->flg_misc & 0x100000))
        {
            vil->vel_moveto.x = vil->dir_bowling.x * 0.5f;
            vil->vel_moveto.y = vil->dir_bowling.y * 0.5f;
            vil->vel_moveto.z = vil->dir_bowling.z * 0.5f;
            vil->domove |= 0x1000;
            vil->domove |= 0x80000000;
        }
        break;
    }
    return 1;
}

static inline void VOUT_faceAng(st_zEntVillain* vil, float ang)
{
    vil->ang_moveto = ang;
    vil->domove |= 0x40;
    vil->domove |= 1;
    vil->domove |= 0x10;
    vil->domove |= 0x80000000;
}

static inline void VOUT_stop(_xEntFrame* frame)
{
    frame->dvel.x = 0.0f;
    frame->dvel.y = 0.0f;
    frame->dvel.z = 0.0f;
    frame->vel.x = 0.0f;
    frame->vel.y = 0.0f;
    frame->vel.z = 0.0f;
    frame->mode |= 0xC;
}

static signed int VILN_std_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp)
{
    _xVec3 unused = {};

    VILN_cmn_respond(vil, resp);
}

static inline void VILN_cmn_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp)
{
    _xVec3 push = {};

    if (VSUP_accum_collide(vil, &push) && vil->goal == ZVIL_GOAL_SOAPED && (vil->ability & 0x100000))
    {
        VILN_flipAway(&vil->vec_knocked.x, push.x);
        VILN_flipAway(&vil->vec_knocked.y, push.y);
        VILN_flipAway(&vil->vec_knocked.z, push.z);
    }
}

static inline void VILN_flipAway(float* knock, float push)
{
    if (push < 0.0f && *knock > 0.0f)
    {
        *knock *= -1.0f;
    }
    else if (push > 0.0f && *knock < 0.0f)
    {
        *knock *= -1.0f;
    }
}

static signed int VILN_std_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot)
{

    {
        switch ((unsigned int)goal) {                       /* switch 1 */
        case 0:                                     /* switch 1 */
        case 1:                                     /* switch 1 */
        case 2:                                     /* switch 1 */
        case 3:                                     /* switch 1 */
            VANM_play(vil, (en_NPC_ANIMSTATES)0x8, (en_NPC_ANIMSTATES)0x1);
            break;
        case 4:                                     /* switch 1 */
            if (gspot == 2) {
                VANM_playRand(vil, (en_NPC_ANIMSTATES)0x2, (en_NPC_ANIMSTATES)0x3);
                vil->anim_playsync = 1;
            } else {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            }
            break;
        case 5:                                     /* switch 1 */
            if (gspot == 2) {
                VANM_playRand(vil, (en_NPC_ANIMSTATES)0x5, (en_NPC_ANIMSTATES)0x6);
                vil->anim_playsync = 1;
            } else {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x4, (en_NPC_ANIMSTATES)0x1);
            }
            break;
        case 31:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_play3(vil, (en_NPC_ANIMSTATES)0xb, (en_NPC_ANIMSTATES)0xe, (en_NPC_ANIMSTATES)0x18);
            } else if (gspot == 0xA) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0xc, (en_NPC_ANIMSTATES)0x36);
            }
            break;
        case 8:                                     /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x7);
            break;
        case 6:                                     /* switch 1 */
            if (vil->allow & 0x20) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0xa, (en_NPC_ANIMSTATES)0x7);
            } else {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x7);
            }
            break;
        case 7:                                     /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x7);
            break;
        case 9:                                     /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            } else if (gspot == 2) {
                VANM_playRand(vil, (en_NPC_ANIMSTATES)0x2, (en_NPC_ANIMSTATES)0x3);
                vil->anim_playsync = 1;
            } else if (gspot == 11 || gspot == 12) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x9, (en_NPC_ANIMSTATES)0x1);
            }
            break;
        case 10:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_play3(vil, (en_NPC_ANIMSTATES)0x36, (en_NPC_ANIMSTATES)0x37, (en_NPC_ANIMSTATES)0xa);
            } else if (gspot == 0xA) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x8);
            }
            break;
        case 11:                                    /* switch 1 */
            VANM_play3(vil, (en_NPC_ANIMSTATES)0xd, (en_NPC_ANIMSTATES)0x15, (en_NPC_ANIMSTATES)0x16);
            break;
        case 13:                                    /* switch 1 */
            VANM_play(vil, (en_NPC_ANIMSTATES)0x13, (en_NPC_ANIMSTATES)0x14);
            vil->anim_loopmult = 0.6f;
            break;
        case 14:                                    /* switch 1 */
            VANM_play3(vil, (en_NPC_ANIMSTATES)0x11, (en_NPC_ANIMSTATES)0x12, (en_NPC_ANIMSTATES)0x9);
            vil->anim_loopmult = 0.6f;
            break;
        case 15:                                    /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0xd);
            break;
        case 16:                                    /* switch 1 */
            VANM_play(vil, (en_NPC_ANIMSTATES)0x15, (en_NPC_ANIMSTATES)0x16);
            break;
        case 23:                                    /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0xe);
            vil->anim_playsync = 1;
            break;
        case 24:                                    /* switch 1 */
            if (gspot == 9) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x31, (en_NPC_ANIMSTATES)0x30);
            } else if (gspot == 1) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x30);
            } else {
                VANM_play4(vil, (en_NPC_ANIMSTATES)0x2f, (en_NPC_ANIMSTATES)0x30, (en_NPC_ANIMSTATES)0x17, (en_NPC_ANIMSTATES)0x1);
            }
            break;
        case 17:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x17, (en_NPC_ANIMSTATES)0x1);
            } else if (gspot == 11 || gspot == 12) {
                VANM_play3(vil, (en_NPC_ANIMSTATES)0x9, (en_NPC_ANIMSTATES)0x17, (en_NPC_ANIMSTATES)0x1);
            }
            break;
        case 19:                                    /* switch 1 */
            VANM_play(vil, (en_NPC_ANIMSTATES)0xa, (en_NPC_ANIMSTATES)0x7);
            break;
        case 20:                                    /* switch 1 */
            VANM_playRand3(vil, (en_NPC_ANIMSTATES)0x18, (en_NPC_ANIMSTATES)0x19, (en_NPC_ANIMSTATES)0x1a);
            break;
        case 21:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1e);
            }
            break;
        case 22:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_play5(vil, (en_NPC_ANIMSTATES)0x20, (en_NPC_ANIMSTATES)0x21, (en_NPC_ANIMSTATES)0x18, (en_NPC_ANIMSTATES)0x19, (en_NPC_ANIMSTATES)0x1a);
            }
            break;
        case 26:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x24);
            } else if (gspot == 5) {
                VANM_play3(vil, (en_NPC_ANIMSTATES)0x27, (en_NPC_ANIMSTATES)0x28, (en_NPC_ANIMSTATES)0x24);
            } else if (gspot == 6) {
                VANM_play3(vil, (en_NPC_ANIMSTATES)0x28, (en_NPC_ANIMSTATES)0x27, (en_NPC_ANIMSTATES)0x24);
            } else if (gspot == 7) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x25, (en_NPC_ANIMSTATES)0x24);
            } else if (gspot == 8) {
                VANM_play3(vil, (en_NPC_ANIMSTATES)0x26, (en_NPC_ANIMSTATES)0x25, (en_NPC_ANIMSTATES)0x24);
            }
            break;
        case 25:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x2a);
            } else if (gspot == 5) {
                VANM_play3(vil, (en_NPC_ANIMSTATES)0x2d, (en_NPC_ANIMSTATES)0x2e, (en_NPC_ANIMSTATES)0x2a);
            } else if (gspot == 6) {
                VANM_play3(vil, (en_NPC_ANIMSTATES)0x2e, (en_NPC_ANIMSTATES)0x2d, (en_NPC_ANIMSTATES)0x2a);
            } else if (gspot == 7) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x2b, (en_NPC_ANIMSTATES)0x2a);
            } else if (gspot == 8) {
                VANM_play3(vil, (en_NPC_ANIMSTATES)0x2c, (en_NPC_ANIMSTATES)0x2b, (en_NPC_ANIMSTATES)0x2a);
            }
            break;
        case 32:                                    /* switch 1 */
            VANM_play4(vil, (en_NPC_ANIMSTATES)0x34, (en_NPC_ANIMSTATES)0x32, (en_NPC_ANIMSTATES)0x33, (en_NPC_ANIMSTATES)0x35);
            break;
        }
    }
    return 1;
}

static signed int VILN_std_xtrarend(st_zEntVillain* vil)
{
    if (vil->mdl_gumBlob)
    {
        xModelRender(vil->mdl_gumBlob);
    }
    return 1;
}

static void VILN_KookRender(_xEnt* ent)
{
    RwRenderStateSet(0x14, (void*)2);
    xEntRender(ent);
    RwRenderStateSet(0x14, (void*)1);
}

static signed int VILN_kp_query(st_zEntVillain* vil, en_ZVIL_QUERY query)
{
    switch (query)
    {
    case ZVIL_Q_CANATTACK:
        if (!(vil->allow & 8))
        {
            return 0;
        }
        if (!(vil->precalc.flg_plyr & 1))
        {
            return 0;
        }
        return 1;
    default:
        return VILN_std_query(vil, query);
    }
}

static signed int VILN_kp_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp)
{
    switch (vil->goal)
    {
    case 0:
    case 1:
        vil->collis->chk = 0;
        vil->collis->pen = 0;
        vil->chkby = 0;
        vil->penby = 0;
        break;
    case 2:
        VILN_tran(resp, 3, 2);
        vil->collis->chk = 0;
        vil->collis->pen = 0;
        vil->chkby = 0;
        vil->penby = 0;
        break;
    case 3:
        VILN_tran(resp, vil->goaldflt, 3);
        break;
    case 4:
    case 5:
    case 9:
        VILN_tran(resp, 0x11, 5);
        break;
    case 6:
    case 7:
    case 8:
    case 10:
    case 11:
    case 13:
    case 14:
    case 15:
    case 16:
    case 19:
    case 24:
    case 25:
    case 26:
        VILN_tran(resp, 4, 0);
        break;
    case 17:
        if (vil->vhf->query(vil, ZVIL_Q_CANATTACK))
        {
            signed int atak = vil->abl_atak;
            if ((atak & 0x80000) && vil->tmr_list[ZVIL_TDUR_EXTRELOAD] <= 0.0f)
            {
                VILN_tran(resp, 0x16, 5);
            }
            else if ((atak & 0x40000) && vil->tmr_list[ZVIL_TDUR_LOBRELOAD] <= 0.0f)
            {
                vil->prj_reserved = VWEP_lobReservePRJ(vil);
                if (vil->prj_reserved)
                {
                    VILN_tran(resp, 0x15, 5);
                }
            }
            else if (atak & 0x20000)
            {
                VILN_tran(resp, 0x14, 5);
            }
        }
        break;
    case 20:
        if (!vil->goodattack && (vil->precalc.plyr_mvinf & 1))
        {
            vil->goodattack = VWEP_swipe(vil, dt);
            if (vil->goodattack)
            {
                VIRM_initVilDflt(vil);
            }
            vil->tmr_list[26] = vil->vilcfg.tymref_list[26];
        }
        if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f)
        {
            if (vil->goodattack && (vil->ability & 0x100))
            {
                if (vil->tmr_list[26] <= 0.0f)
                {
                    VILN_tran(resp, 0x17, 0);
                }
            }
            else
            {
                VILN_tran(resp, 0x11, 5);
            }
        }
        break;
    case 21:
        if (vil->tmr_list[21] <= 0.0f)
        {
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x15, (en_ZVIL_GOAL_SPOT)4);
            VILN_lobSalvo(vil);
            if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f)
            {
                VILN_tran(resp, 0x11, 5);
            }
        }
        break;
    case 22:
        if (VANM_inDmgWindow(vil))
        {
            vil->goodattack |= VWEP_extend_particles(vil, dt, &vil->pos_exttgt);
        }
        if (vil->goodattack)
        {
            VIRM_initVilDflt(vil);
        }
        else
        {
            vil->tmr_list[26] = vil->vilcfg.tymref_list[26];
        }
        if (vil->tmr_list[ZVIL_TDUR_EXTEND] <= 0.0f)
        {
            VILN_tran(resp, 0x11, 0);
        }
        else if (vil->goodattack && (vil->ability & 0x100))
        {
            if (vil->tmr_list[26] <= 0.0f)
            {
                VILN_tran(resp, 0x17, 0);
            }
        }
        break;
    case 23:
        if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f)
        {
            VILN_tran(resp, 0x11, 0);
        }
        break;
    case 32:
        VILN_holoDeath(vil, dt, resp);
        break;
    default:
        VILN_tran(resp, 4, 0);
        break;
    }
    return 1;
}

static inline void VILN_lobSalvo(st_zEntVillain* vil)
{
    if (VANM_inDmgWindow(vil) && vil->prj_reserved && vil->cnt_lobSalvo > 0)
    {
        _xVec3 vec_off = {};
        float spread;

        vec_off.x = 0.5f - iurand();
        vec_off.y = 0.5f - iurand();
        vec_off.z = 0.5f - iurand();
        xVec3Normalize(&vec_off, &vec_off);
        spread = vil->cnt_lobSalvo - 1;
        vec_off.x *= spread;
        vec_off.y *= spread;
        vec_off.z *= spread;
        VWEP_lobActivatePRJ(vil->prj_reserved, vil, xBoundCenter(&globals.player.ent.bound), &vec_off,
                            &vil->vec_lobSpin, vil->fac_lobarc, vil->fac_lobheavy);
        vil->cnt_lobSalvo--;
        if (vil->cnt_lobSalvo > 0 && !vil->prj_reserved)
        {
            vil->prj_reserved = VWEP_lobReservePRJ(vil);
        }
    }
}

static inline signed int VANM_inDmgWindow(st_zEntVillain* vil)
{
    xAnimSingle* single = VANM_single(vil);
    float tym = single ? single->Time : 0.0f;
    float beg = -1.0f;
    float end = -2.0f;

    VCFG_getParm(&vil->vilcfg, ZVIL_PARM_DMG_BEGTIME, &beg);
    VCFG_getParm(&vil->vilcfg, ZVIL_PARM_DMG_ENDTIME, &end);
    unsigned char ok = tym >= beg;
    if (ok)
    {
        ok = tym <= end;
    }
    return ok;
}

static signed int VILN_crit_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal)
{
    return VILN_flagsFromTable(vil, flaggroup);
}

static signed int VILN_crit_query(st_zEntVillain* vil, en_ZVIL_QUERY query)
{
    return VILN_std_query(vil, query);
}

static signed int VILN_crit_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp)
{
    return VILN_std_sysevent(vil, from, toEvent, toParam, toParamWidget, resp);
}

static signed int VILN_crit_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp)
{
    return VILN_cmn_notice(vil, note, resp);
}

static signed int VILN_crit_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp)
{
    return VILN_std_aware(vil, scn, dt, resp);
}

static signed int VILN_crit_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp)
{
    return VILN_std_stimulus(vil, stimctxt, resp);
}

static signed int VILN_crit_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt)
{
    return VILN_std_trangoal(vil, stimctxt, dt);
}

static signed int VILN_crit_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame)
{
    return VILN_std_calcmove(vil, scn, dt, frame);
}

static signed int VILN_crit_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp)
{
    VILN_cmn_respond(vil, resp);
}

static signed int VILN_crit_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot)
{
    return VILN_std_pickanim(vil, goal, gspot);
}

static signed int VILN_shark_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal)
{
    return VILN_flagsFromTable(vil, flaggroup);
}

static signed int VILN_shark_query(st_zEntVillain* vil, en_ZVIL_QUERY query)
{
    return VILN_std_query(vil, query);
}

static signed int VILN_shark_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp)
{
    return VILN_std_sysevent(vil, from, toEvent, toParam, toParamWidget, resp);
}

static signed int VILN_shark_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp)
{
    // Widen the culling spheres so the shark is never culled.
    *(float*)((char*)vil->model->Data + 0x38) = 10.0f;
    *(float*)((char*)vil->model->Data + 0x28) = 10.0f;
    *(float*)(*(char**)(*(char**)((char*)vil->model->Data + 0x18) + 0x68) + 0x10) = 10.0f;
    return VILN_cmn_notice(vil, note, resp);
}

static signed int VILN_shark_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp)
{
    switch (vil->goal)
    {
    case 0:
    case 1:
        VILN_nocollide(vil);
        break;
    case 2:
        VILN_settran(resp, 3, 2);
        VILN_nocollide(vil);
        break;
    case 3:
        VILN_settran(resp, vil->goaldflt, 3);
        break;
    case 4:
        if (vil->tmr_list[0] <= 0.0f)
        {
            float tym;

            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)4, (en_ZVIL_GOAL_SPOT)2);
            tym = vil->vilcfg.tymref_list[0];
            vil->tmr_list[0] = VUTIL_jitter(tym);
        }
        break;
    case 32:
        VILN_settran(resp, 0, 1);
        break;
    default:
        VILN_settran(resp, 4, 0);
        break;
    }
    return 1;
}

static signed int VILN_shark_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt,
                                      st_STIMULUS_CONTEXT_DATA* resp)
{
    en_NPC_STIMULUS stim = stimctxt->stim;

    g_pgol = vil->goal;
    g_pstm = stim;
    switch (vil->goal)
    {
    case 0:
    case 1:
        if (stim == 2)
        {
            VILN_settran(resp, 2, 1);
        }
        break;
    case 4:
        switch (stim)
        {
        case 34:
            vil->hitpts = 0;
            VILN_settran(resp, 0x20, 7);
            break;
        case 35:
            VILN_settran(resp, 0, 1);
            break;
        case 32:
        case 33:
            stimctxt->res_gods = 2;
            break;
        case 27:
        case 28:
            stimctxt->res_gods = 0;
            break;
        case 29:
            stimctxt->res_gods = 3;
            break;
        }
        break;
    }
    return 1;
}

static inline void VILN_settran(st_STIMULUS_CONTEXT_DATA* resp, signed int goal, signed int mode)
{
    resp->trangoal = (en_BEHAVIOUR_GOAL)goal;
    resp->tranmode = (en_NPC_MODE)mode;
    resp->stim = (en_NPC_STIMULUS)5;
}

static signed int VILN_shark_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt)
{
    return VILN_std_trangoal(vil, stimctxt, dt);
}

static signed int VILN_shark_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame)
{
    return 1;
}

static signed int VILN_shark_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp)
{
    return 1;
}

static signed int VILN_shark_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot)
{
    switch (goal)
    {
    case 0:
        VILN_nocollide(vil);
        break;
    case 1:
        VANM_playLoop(vil, (en_NPC_ANIMSTATES)1);
        VILN_nocollide(vil);
        break;
    case 2:
        VILN_nocollide(vil);
        break;
    case 3:
        VANM_playLoop(vil, (en_NPC_ANIMSTATES)1);
        break;
    case 4:
        if (gspot == 2)
        {
            VANM_playRand4(vil, (en_NPC_ANIMSTATES)2, (en_NPC_ANIMSTATES)3, (en_NPC_ANIMSTATES)5, (en_NPC_ANIMSTATES)6);
            vil->anim_playsync = 1;
        }
        else
        {
            VANM_playRand(vil, (en_NPC_ANIMSTATES)4, (en_NPC_ANIMSTATES)1);
        }
        break;
    case 32:
        VANM_playLoop(vil, (en_NPC_ANIMSTATES)1);
        break;
    }
    return 1;
}

static inline void VILN_nocollide(st_zEntVillain* vil)
{
    vil->collis->chk = 0;
    vil->collis->pen = 0;
    vil->chkby = 0;
    vil->penby = 0;
}

static signed int VILN_shag_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal)
{
    return VILN_flagsFromTable(vil, flaggroup);
}

static signed int VILN_shag_query(st_zEntVillain* vil, en_ZVIL_QUERY query)
{
    return VILN_std_query(vil, query);
}

static signed int VILN_shag_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp)
{
    return VILN_std_sysevent(vil, from, toEvent, toParam, toParamWidget, resp);
}

static signed int VILN_shag_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp)
{
    return VILN_cmn_notice(vil, note, resp);
}

static signed int VILN_shag_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp)
{
    return VILN_std_aware(vil, scn, dt, resp);
}

static signed int VILN_shag_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp)
{
    return VILN_std_stimulus(vil, stimctxt, resp);
}

static signed int VILN_shag_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt)
{
    return VILN_std_trangoal(vil, stimctxt, dt);
}

static signed int VILN_shag_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame)
{
    return VILN_std_calcmove(vil, scn, dt, frame);
}

static signed int VILN_shag_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp)
{
    VILN_cmn_respond(vil, resp);
}

static signed int VILN_shag_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot)
{
    return VILN_std_pickanim(vil, goal, gspot);
}

static signed int VILN_shagM_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal)
{
    return VILN_flagsFromTable(vil, flaggroup);
}

static signed int VILN_shagM_query(st_zEntVillain* vil, en_ZVIL_QUERY query)
{
    return VILN_std_query(vil, query);
}

static signed int VILN_shagM_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp)
{
    return VILN_std_sysevent(vil, from, toEvent, toParam, toParamWidget, resp);
}

static signed int VILN_shagM_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp)
{
    return VILN_cmn_notice(vil, note, resp);
}

static signed int VILN_shagM_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp)
{
    switch (vil->goal)
    {
    case 0:
    case 1:
        vil->collis->chk = 0;
        vil->collis->pen = 0;
        vil->chkby = 0;
        vil->penby = 0;
        break;
    case 2:
        VILN_tran(resp, 3, 2);
        vil->collis->chk = 0;
        vil->collis->pen = 0;
        vil->chkby = 0;
        vil->penby = 0;
        break;
    case 3:
        VILN_tran(resp, vil->goaldflt, 3);
        break;
    case 4:
        if (vil->vhf->query(vil, ZVIL_Q_CANPATROL))
        {
            if (vil->vhf->query(vil, ZVIL_Q_NEEDNAV))
            {
                VNAV_get_nav(vil);
            }
            if (vil->vhf->query(vil, ZVIL_Q_HAVENAV))
            {
                VILN_tran(resp, 6, 3);
            }
        }
        break;
    case 31:
        if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f)
        {
            VILN_tran(resp, 4, 3);
        }
        break;
    case 8:
        if (!vil->vhf->query(vil, ZVIL_Q_DOFOLLOW))
        {
            VILN_tran(resp, 4, 3);
        }
        break;
    case 6:
        if (vil->vhf->query(vil, ZVIL_Q_NEEDNAV))
        {
            VNAV_get_nav(vil);
        }
        if (!vil->vhf->query(vil, ZVIL_Q_HAVENAV))
        {
            VILN_tran(resp, 4, 3);
        }
        break;
    case 32:
        VILN_holoDeath(vil, dt, resp);
        break;
    default:
        VILN_tran(resp, 4, 0);
        break;
    }
    return 1;
}

static signed int VILN_shagM_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt,
                                      st_STIMULUS_CONTEXT_DATA* resp)
{
    signed int goal = vil->goal;
    en_NPC_STIMULUS stim = stimctxt->stim;

    g_pgol = (en_BEHAVIOUR_GOAL)goal;
    switch (goal)
    {
    case -1:
        break;
    case 0:
    case 1:
        g_pstm = stim;
        switch (stim)
        {
        case 5:
        case 0:
            break;
        case 2:
            VILN_tran(resp, 2, 1);
            break;
        }
        break;
    case 2:
    case 3:
    case 32:
        g_pstm = stim;
        break;
    case 4:
    case 6:
    case 8:
    case 31:
        g_pstm = stim;
        switch (stim)
        {
        case 0:
            break;
        case ZVIL_STIM_KILLSELF:
            vil->hitpts = 0;
            VILN_tran(resp, 0x20, 7);
            break;
        case ZVIL_STIM_GOTOAFTERLIFE:
            VILN_tran(resp, 0, 1);
            break;
        case ZVIL_STIM_DOACTION:
            VILN_tran(resp, 0x1F, 3);
            break;
        case ZVIL_STIM_GANGBOSSACTION:
            VILN_tran(resp, 0x1F, 0);
            break;
        case ZVIL_STIM_GOTOPOS:
            VILN_stimGotoPos(vil, stimctxt);
            VILN_tran(resp, 8, 3);
            break;
        case ZVIL_STIM_GOTOWIDGET:
            if (stimctxt->widge && (vil->abl_move & 0x100))
            {
                VOUT_dogoto_widget(vil, stimctxt->widge, stimctxt->off_goto, 0);
                vil->tmr_list[ZVIL_TDUR_NAVDELAY] = -1.0f;
                VILN_tran(resp, 8, 3);
            }
            break;
        case ZVIL_STIM_FOLLOWWIDGET:
            if (stimctxt->widge && (vil->abl_move & 0x100))
            {
                signed int follow = VOUT_dogoto_widget(vil, stimctxt->widge, stimctxt->off_goto, 1);
                vil->tmr_list[ZVIL_TDUR_NAVDELAY] = -1.0f;
                if (follow)
                {
                    VILN_tran(resp, 6, 3);
                }
                else
                {
                    VILN_tran(resp, 8, 3);
                }
            }
            break;
        case ZVIL_STIM_STARTPATROL:
            vil->tmr_list[ZVIL_TDUR_NAVDELAY] = -1.0f;
            if (vil->vhf->query(vil, ZVIL_Q_NEEDNAV))
            {
                VNAV_get_nav(vil);
            }
            if (vil->vhf->query(vil, ZVIL_Q_HAVENAV))
            {
                VILN_tran(resp, 6, 3);
            }
            else
            {
                VILN_tran(resp, 4, 3);
            }
            break;
        case ZVIL_STIM_STOPPATROL:
            VILN_tran(resp, 4, 3);
            break;
        }
        break;
    }
    return 1;
}

static signed int VILN_shagM_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt)
{
    signed int rc = 1;
    en_NPC_STIMULUS stim = stimctxt->stim;
    en_BEHAVIOUR_GOAL goal;

    if (stim == ZVIL_STIM_ENTERGOAL || stim == ZVIL_STIM_EXITGOAL)
    {
        goal = stimctxt->trangoal;
    }
    else
    {
        goal = vil->goal;
    }
    switch (goal)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 32:
        rc = VILN_std_trangoal(vil, stimctxt, dt);
        break;
    }
    g_pgol = goal;
    switch (goal)
    {
    case ZVIL_GOAL_NONE:
        break;
    case 4:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_enterGoal(vil, (en_BEHAVIOUR_GOAL)4);
            break;
        }
        break;
    case 31:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_enterGoal(vil, (en_BEHAVIOUR_GOAL)31);
            break;
        }
        break;
    case 8:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_enterGoal(vil, (en_BEHAVIOUR_GOAL)8);
            break;
        }
        break;
    case 6:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_enterGoal(vil, (en_BEHAVIOUR_GOAL)6);
            if (!vil->nav_next)
            {
                vil->need_navpt = 1;
            }
            break;
        }
        break;
    }
    return rc;
}

static inline void VILN_enterGoal(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal)
{
    vil->anim_loopmult = 0.97f;
    vil->vhf->pickanim(vil, goal, (en_ZVIL_GOAL_SPOT)0);
}

static inline en_BEHAVIOUR_GOAL VILN_tranGoalOf(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt)
{
    if (stimctxt->stim == 4 || stimctxt->stim == 3)
    {
        return stimctxt->trangoal;
    }
    return vil->goal;
}

static signed int VILN_shagM_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame)
{
    switch (vil->goal) {
    case 7:
        break;
    case 4:
        vil->ang_moveto = vil->precalc.ang_VtoP;
        vil->domove |= 0x40;
        vil->domove |= 1;
        vil->domove |= 0x10;
        vil->domove |= 0x80000000;
        break;
    case 8:
        VOUT_follow_move(vil);
        VOUT_chkdo_followArrived(vil);
        break;
    case 6:
        VOUT_patrol_move(vil);
        VOUT_chkdo_patrolArrived(vil);
        break;
    }
    return 1;
}

static signed int VILN_shagM_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp)
{
    return 1;
}

static signed int VILN_shagM_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot)
{
    vil->anim_playnext = (en_NPC_ANIMSTATES)0;
    vil->anim_playing = 0;
    vil->anim_loopmult = 0.97f;
    return 1;
}

static signed int VILN_gang_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal)
{
    return VILN_flagsFromTable(vil, flaggroup);
}

static signed int VILN_gang_query(st_zEntVillain* vil, en_ZVIL_QUERY query)
{
    return VILN_std_query(vil, query);
}

static signed int VILN_gang_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp)
{
    return VILN_std_sysevent(vil, from, toEvent, toParam, toParamWidget, resp);
}

static signed int VILN_gang_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp)
{
    xModelInstance* minst = vil->model;
    signed int idx = 0;

    if (note == 0)
    {
        vil->vilcfg.tymref_list[0] = 2.0f;
    }
    if (note == 0 && vil->asset->subtype == 0x26)
    {
        xAnimState* state = xAnimTableGetStateID(vil->model->Anim->Table, g_anamhash[134]);
        if (state)
        {
            state->Flags &= ~0x30;
            state->Flags |= 0x10;
        }
    }
    if (note == 0 && vil->asset->subtype == 0x26)
    {
        while (minst)
        {
            switch (idx)
            {
            case 0:
                xSceneID2Name(globals.sceneCur, vil->id);
                minst->Flags |= 2;
                minst->Flags |= 1;
                break;
            case 1:
                xSceneID2Name(globals.sceneCur, vil->id);
                minst->Flags &= ~1;
                break;
            default:
                xSceneID2Name(globals.sceneCur, vil->id);
                break;
            }
            minst = minst->Next;
            idx++;
        }
    }
    return VILN_cmn_notice(vil, note, resp);
}

static signed int VILN_gang_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp)
{
    switch (vil->goal)
    {
    case 0:
    case 1:
        vil->collis->chk = 0;
        vil->collis->pen = 0;
        vil->chkby = 0;
        vil->penby = 0;
        break;
    case 2:
        VILN_tran(resp, 3, 2);
        vil->collis->chk = 0;
        vil->collis->pen = 0;
        vil->chkby = 0;
        vil->penby = 0;
        break;
    case 3:
        VILN_tran(resp, vil->goaldflt, 3);
        break;
    case 4:
        if (vil->vhf->query(vil, ZVIL_Q_CANPATROL))
        {
            if (vil->vhf->query(vil, ZVIL_Q_NEEDNAV))
            {
                VNAV_get_nav(vil);
            }
            if (vil->vhf->query(vil, ZVIL_Q_HAVENAV))
            {
                VILN_tran(resp, 6, 3);
                break;
            }
        }
        VILN_gangIdle(vil, resp);
        break;
    case 12:
    {
        float tym = vil->vilcfg.tymref_list[8];
        vil->tmr_list[8] = VUTIL_jitter(tym);
        if (!(vil->allow & 0x400) && !vil->sfx_curTalk)
        {
            xSceneID2Name(globals.sceneCur, vil->id);
        }
        if (vil->tmr_list[ZVIL_TYMR_GENERIC] <= 0.0f && vil->sfx_curTalk &&
            !iSndIsPlaying(0, (unsigned int)vil + 2))
        {
            xSceneID2Name(globals.sceneCur, vil->id);
            vil->sfx_curTalk = NULL;
            VILN_tran(resp, 4, 3);
        }
        if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f && vil->tmr_list[0] <= 0.0f)
        {
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, ZVIL_GOAL_TALK, (en_ZVIL_GOAL_SPOT)2);
            vil->tmr_list[0] = 4.0f * (0.25f * (iurand() - 0.5f)) + 4.0f;
        }
        break;
    }
    case 31:
        if (vil->asset->subtype != 0x26 && vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f)
        {
            VILN_tran(resp, 4, 3);
        }
        break;
    case 8:
        if (!vil->vhf->query(vil, ZVIL_Q_DOFOLLOW))
        {
            VILN_tran(resp, 4, 3);
        }
        break;
    case 6:
        if (vil->vhf->query(vil, ZVIL_Q_NEEDNAV))
        {
            VNAV_get_nav(vil);
        }
        if (!vil->vhf->query(vil, ZVIL_Q_HAVENAV))
        {
            VILN_tran(resp, 4, 3);
        }
        break;
    case 23:
        if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f && vil->tmr_list[ZVIL_TDUR_TAUNT] <= 0.0f)
        {
            VILN_tran(resp, 4, 3);
        }
        break;
    case 32:
        VILN_holoDeath(vil, dt, resp);
        break;
    default:
        VILN_tran(resp, 4, 0);
        break;
    }
    return 1;
}

static inline void VILN_gangIdle(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp)
{
    st_ZVIL_PRECALC* pc = &vil->precalc;

    if (vil->tmr_list[8] <= 0.0f && (pc->flg_plyr & 1) && (pc->flg_plyr & 8))
    {
        float tym = vil->vilcfg.tymref_list[8];
        vil->tmr_list[8] = VUTIL_jitter(tym);
        if (VSND_select_sound(vil, (en_NPC_SOUND)0x30))
        {
            VILN_tran(resp, 0xC, 0);
            resp->talkSFX = (zSFX*)-1;
            return;
        }
    }
    if (vil->tmr_list[0] <= 0.0f && vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f)
    {
        signed int anim = vil->anim_playnext;

        vil->anim_loopmult = 0.97f;
        if (anim == 0x7D || (unsigned int)(anim - 0x7E) <= 1 || anim == 0x80)
        {
            vil->vhf->pickanim(vil, ZVIL_GOAL_IDLE, (en_ZVIL_GOAL_SPOT)1);
            vil->tmr_list[0] = 5.0f * (0.25f * (iurand() - 0.5f)) + 5.0f;
        }
        else if (xUtil_yesno(0.35f))
        {
            vil->vhf->pickanim(vil, ZVIL_GOAL_IDLE, (en_ZVIL_GOAL_SPOT)2);
            vil->tmr_list[0] = 2.0f * (0.25f * (iurand() - 0.5f)) + 2.0f;
        }
        else
        {
            vil->vhf->pickanim(vil, ZVIL_GOAL_IDLE, (en_ZVIL_GOAL_SPOT)1);
            vil->tmr_list[0] = 5.0f * (0.25f * (iurand() - 0.5f)) + 5.0f;
        }
    }
}

static signed int VILN_gang_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt,
                                     st_STIMULUS_CONTEXT_DATA* resp)
{
    signed int goal = vil->goal;
    en_NPC_STIMULUS stim = stimctxt->stim;

    g_pgol = (en_BEHAVIOUR_GOAL)goal;
    switch (goal)
    {
    case -1:
        break;
    case 0:
        g_pstm = stim;
        switch (stim)
        {
        case 5:
        case 0:
            break;
        case 2:
            VILN_tran(resp, 2, 1);
            break;
        }
        break;
    case 1:
        g_pstm = stim;
        switch (stim)
        {
        case 5:
        case 0:
            break;
        case 2:
            VILN_tran(resp, 2, 1);
            break;
        }
        break;
    case 2:
    case 3:
    case 32:
        g_pstm = stim;
        break;
    case 4:
    case 6:
    case 8:
    case 12:
    case 31:
        g_pstm = stim;
        switch (stim)
        {
        case 0:
            break;
        case ZVIL_STIM_KILLSELF:
            vil->hitpts = 0;
            VILN_tran(resp, 0x20, 7);
            break;
        case ZVIL_STIM_GOTOAFTERLIFE:
            VILN_tran(resp, 0, 1);
            break;
        case ZVIL_STIM_HEADBUTT:
            if (vil->ability & 4)
            {
                stimctxt->res_gods = 3;
            }
            break;
        case ZVIL_STIM_STARTTALK:
            if (vil->ability & 0x40000)
            {
                if (vil->goal == ZVIL_GOAL_TALK)
                {
                    VSND_startTalk(vil, stimctxt->talkSFX);
                    vil->tmr_list[ZVIL_TYMR_GENERIC] = 0.5f;
                    if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f)
                    {
                        vil->anim_loopmult = 0.97f;
                        vil->vhf->pickanim(vil, ZVIL_GOAL_TALK, (en_ZVIL_GOAL_SPOT)2);
                        vil->tmr_list[0] = 4.0f * (0.25f * (iurand() - 0.5f)) + 4.0f;
                    }
                }
                else
                {
                    VILN_tran(resp, 0xC, 0);
                    resp->talkSFX = stimctxt->talkSFX;
                }
            }
            break;
        case ZVIL_STIM_STOPTALK:
            VILN_tran(resp, 4, 0);
            break;
        case ZVIL_STIM_DOACTION:
            VILN_tran(resp, 0x1F, 0);
            break;
        case ZVIL_STIM_GANGBOSSACTION:
            VILN_tran(resp, 0x1F, 0);
            resp->use_alt_action = 1;
            break;
        case ZVIL_STIM_GOTOPOS:
            VILN_stimGotoPos(vil, stimctxt);
            VILN_tran(resp, 8, 3);
            break;
        case ZVIL_STIM_GOTOWIDGET:
            if (stimctxt->widge && (vil->abl_move & 0x100))
            {
                VOUT_dogoto_widget(vil, stimctxt->widge, stimctxt->off_goto, 0);
                vil->tmr_list[ZVIL_TDUR_NAVDELAY] = -1.0f;
                VILN_tran(resp, 8, 3);
            }
            break;
        case ZVIL_STIM_FOLLOWWIDGET:
            if (stimctxt->widge && (vil->abl_move & 0x100))
            {
                signed int follow = VOUT_dogoto_widget(vil, stimctxt->widge, stimctxt->off_goto, 1);
                vil->tmr_list[ZVIL_TDUR_NAVDELAY] = -1.0f;
                if (follow)
                {
                    VILN_tran(resp, 6, 3);
                }
                else
                {
                    VILN_tran(resp, 8, 3);
                }
            }
            break;
        case ZVIL_STIM_STARTPATROL:
            vil->tmr_list[ZVIL_TDUR_NAVDELAY] = -1.0f;
            if (vil->vhf->query(vil, ZVIL_Q_NEEDNAV))
            {
                VNAV_get_nav(vil);
            }
            if (vil->vhf->query(vil, ZVIL_Q_HAVENAV))
            {
                VILN_tran(resp, 6, 3);
            }
            else
            {
                VILN_tran(resp, 4, 3);
            }
            break;
        case ZVIL_STIM_STOPPATROL:
            VILN_tran(resp, 4, 3);
            break;
        case ZVIL_STIM_STARTGUARD:
            VILN_tran(resp, 4, 3);
            break;
        case ZVIL_STIM_STOPGUARD:
            VILN_tran(resp, 4, 3);
            break;
        }
        break;
    }
    return 1;
}

static inline void VSND_startTalk(st_zEntVillain* vil, zSFX* sfx)
{
    st_NPC_SOUND_PROPERTIES* sprop = NULL;

    xSndStopChildren((unsigned int)vil + 2);
    vil->sfx_curTalk = sfx;
    if (sfx)
    {
        char* asset = *(char**)((char*)sfx + 0x10);
        if (asset && *(unsigned int*)asset)
        {
            signed int flg;
            signed int i;

            flg = VSND_findProps((en_NPC_SOUND)0x2E, &sprop)->flg_vsnd;
            for (i = 0; i < 4; i++)
            {
                vil->snd_queue[i].sndEvent = (en_NPC_SOUND)-1;
                vil->snd_queue[i].sndDirect = 0;
            }
            VSND_addQueueEntry(vil, (en_NPC_SOUND)0x8000, flg, g_npcTable[vil->asset->subtype].dst_noisy,
                               sprop->tym_delayStart, -1.0f, 0, *(unsigned int*)(*(char**)((char*)sfx + 0x10) + 0x10));
        }
    }
}

static signed int VILN_gang_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt)
{
    signed int rc = 1;
    en_NPC_STIMULUS stim = stimctxt->stim;
    en_BEHAVIOUR_GOAL goal;

    if (stim == ZVIL_STIM_ENTERGOAL || stim == ZVIL_STIM_EXITGOAL)
    {
        goal = stimctxt->trangoal;
    }
    else
    {
        goal = vil->goal;
    }

    switch (goal)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 32:
        rc = VILN_std_trangoal(vil, stimctxt, dt);
        break;
    }
    g_pgol = goal;
    switch (goal)
    {
    case ZVIL_GOAL_NONE:
        break;
    case 4:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            float tym;

            VILN_enterGoal(vil, (en_BEHAVIOUR_GOAL)4);
            tym = vil->vilcfg.tymref_list[0];
            vil->tmr_list[0] = VUTIL_jitter(tym);
            tym = vil->vilcfg.tymref_list[8];
            vil->tmr_list[8] = VUTIL_jitter(tym);
            break;
        }
        break;
    case 12:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            float tym;
            zSFX* sfx;

            VILN_enterGoal(vil, (en_BEHAVIOUR_GOAL)12);
            tym = vil->vilcfg.tymref_list[0];
            vil->tmr_list[0] = VUTIL_jitter(tym);
            vil->tmr_list[ZVIL_TYMR_GENERIC] = 0.5f;
            sfx = stimctxt->talkSFX;
            if (sfx == (zSFX*)-1)
            {
                // No scripted line: play a greeting, or a hint for the hint giver.
                signed int snd = 0x30;
                unsigned int specials = globals.player.Inv_Specials;

                if (vil->asset->subtype == 0x30 && xUtil_yesno(0.1f))
                {
                    if (!(specials & 4) && VSND_select_sound(vil, (en_NPC_SOUND)0x32))
                    {
                        snd = 0x32;
                    }
                    else if ((specials & 0x200) && !(specials & 0x20) && VSND_select_sound(vil, (en_NPC_SOUND)0x33))
                    {
                        snd = 0x33;
                    }
                    else if (!(specials & 0x1000) && VSND_select_sound(vil, (en_NPC_SOUND)0x34))
                    {
                        snd = 0x34;
                    }
                    else if ((specials & 0x1000) && VSND_select_sound(vil, (en_NPC_SOUND)0x35))
                    {
                        snd = 0x35;
                    }
                }
                VSND_cue(vil, (en_NPC_SOUND)snd);
                vil->sfx_curTalk = stimctxt->talkSFX;
            }
            else if (sfx)
            {
                VSND_startTalk(vil, sfx);
            }
            break;
        case ZVIL_STIM_EXITGOAL:
            if (vil->sfx_curTalk)
            {
                xSndStopChildren((unsigned int)vil + 2);
                vil->sfx_curTalk = NULL;
            }
            break;
        }
        break;
    case 31:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, goal, (en_ZVIL_GOAL_SPOT)(stimctxt->use_alt_action ? 0xA : 0));
            break;
        }
        break;
    case 8:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_enterGoal(vil, (en_BEHAVIOUR_GOAL)8);
            break;
        }
        break;
    case 6:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_enterGoal(vil, (en_BEHAVIOUR_GOAL)6);
            vil->tgt_stat = 0;
            if (!vil->nav_next)
            {
                vil->need_navpt = 1;
            }
            break;
        }
        break;
    }
    return rc;
}

static signed int VILN_gang_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame)
{
    switch (vil->goal) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 5:
    case 7:
    case 9:
    case 10:
    case 11:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
        break;
    case 4:
        vil->ang_moveto = vil->precalc.ang_VtoP;
        vil->domove |= 0x40;
        vil->domove |= 1;
        vil->domove |= 0x10;
        vil->domove |= 0x80000000;
        break;
    case 12:
        vil->ang_moveto = vil->precalc.ang_VtoP;
        vil->domove |= 0x40;
        vil->domove |= 1;
        vil->domove |= 0x10;
        vil->domove |= 0x80000000;
        break;
    case 8:
        VOUT_follow_move(vil);
        VOUT_chkdo_followArrived(vil);
        break;
    case 6:
        VOUT_patrol_move(vil);
        VOUT_chkdo_patrolArrived(vil);
        break;
    }
    return 1;
}

static signed int VILN_gang_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp)
{
    return 1;
}

static signed int VILN_gang_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot)
{

    {
        switch (goal) {                       /* switch 1 */
        case 0:
            break;
        case 1:                                     /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            break;
        case 3:                                     /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            break;
        case 4:                                     /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            } else if (gspot == 1) {
                VANM_playRand4(vil, (en_NPC_ANIMSTATES)0x1, (en_NPC_ANIMSTATES)0x7a, (en_NPC_ANIMSTATES)0x7b, (en_NPC_ANIMSTATES)0x7c);
                vil->anim_loopmult = 5.9700003f;
                if (xUtil_yesno(0.5f) != 0) {
                    vil->anim_loopmult += 2.0f;
                } else if (xUtil_yesno(0.5f) != 0) {
                    vil->anim_loopmult += 1.0f;
                }
            } else if (gspot == 12) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x81);
            } else if (gspot == 2) {
                VANM_playRand4(vil, (en_NPC_ANIMSTATES)0x7d, (en_NPC_ANIMSTATES)0x7e, (en_NPC_ANIMSTATES)0x7f, (en_NPC_ANIMSTATES)0x80);
                vil->anim_loopmult = 1.97f;
                if (xUtil_yesno(0.5f) != 0) {
                    vil->anim_loopmult += 1.0f;
                }
            }
            break;
        case 12:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playRand4(vil, (en_NPC_ANIMSTATES)0x87, (en_NPC_ANIMSTATES)0x88, (en_NPC_ANIMSTATES)0x89, (en_NPC_ANIMSTATES)0x8a);
            } else if (gspot == 1) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x87);
            } else if (gspot == 12) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x8b, (en_NPC_ANIMSTATES)0x87);
            } else if (gspot == 2) {
                VANM_playRand4(vil, (en_NPC_ANIMSTATES)0x87, (en_NPC_ANIMSTATES)0x88, (en_NPC_ANIMSTATES)0x89, (en_NPC_ANIMSTATES)0x8a);
                vil->anim_loopmult = 4.9700003f;
                if (xUtil_yesno(0.5f) != 0) {
                    vil->anim_loopmult += 2.0f;
                } else if (xUtil_yesno(0.5f) != 0) {
                    vil->anim_loopmult += 1.0f;
                }
            }
            break;
        case 31:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x85);
            } else if (gspot == 0xA) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x86, (en_NPC_ANIMSTATES)0x85);
            }
            break;
        case 8:                                     /* switch 1 */
            if (vil->allow & 0x20) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x83, (en_NPC_ANIMSTATES)0x82);
            } else {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x82);
            }
            break;
        case 6:                                     /* switch 1 */
            if (vil->allow & 0x20) {
                VANM_play3(vil, (en_NPC_ANIMSTATES)0x84, (en_NPC_ANIMSTATES)0x83, (en_NPC_ANIMSTATES)0x82);
            } else {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x83, (en_NPC_ANIMSTATES)0x82);
            }
            break;
        case 32:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            }
            break;
        }
    }
    return 1;
}

static signed int VILN_gang_animtran(st_zEntVillain* vil, xAnimTransition* tran, xAnimSingle* single)
{
    xModelInstance* minst;
    signed int idx = 0;
    minst = vil->model;

    if (vil->asset->subtype != 0x26)
    {
        return 1;
    }
    while (minst)
    {
        switch (idx)
        {
        case 0:
            minst->Flags |= 2;
            minst->Flags |= 1;
            break;
        case 1:
            if (tran->Dest->ID == g_anamhash[134])
            {
                minst->Flags |= 1;
            }
            else
            {
                minst->Flags &= ~1;
            }
            break;
        }
        minst = minst->Next;
        idx++;
    }
    return 1;
}

static signed int VILN_sham_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal)
{
    return VILN_flagsFromTable(vil, flaggroup);
}

static signed int VILN_sham_query(st_zEntVillain* vil, en_ZVIL_QUERY query)
{
    return VILN_std_query(vil, query);
}

static signed int VILN_sham_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp)
{
    return VILN_std_sysevent(vil, from, toEvent, toParam, toParamWidget, resp);
}

static signed int VILN_sham_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp)
{
    return VILN_cmn_notice(vil, note, resp);
}

static signed int VILN_sham_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp)
{
    switch (vil->goal)
    {
    case 0:
    case 1:
        vil->collis->chk = 0;
        vil->collis->pen = 0;
        vil->chkby = 0;
        vil->penby = 0;
        break;
    case 2:
        VILN_tran(resp, 3, 2);
        vil->collis->chk = 0;
        vil->collis->pen = 0;
        vil->chkby = 0;
        vil->penby = 0;
        break;
    case 3:
        VILN_tran(resp, vil->goaldflt, 3);
        break;
    case 4:
        if (vil->vhf->query(vil, ZVIL_Q_CANPATROL))
        {
            if (vil->vhf->query(vil, ZVIL_Q_NEEDNAV))
            {
                VNAV_get_nav(vil);
            }
            if (vil->vhf->query(vil, ZVIL_Q_HAVENAV))
            {
                VILN_tran(resp, 6, 3);
            }
        }
        break;
    case 8:
        if (!vil->vhf->query(vil, ZVIL_Q_DOFOLLOW))
        {
            VILN_tran(resp, 4, 3);
        }
        break;
    case 6:
        if (vil->vhf->query(vil, ZVIL_Q_NEEDNAV))
        {
            VNAV_get_nav(vil);
        }
        if (!vil->vhf->query(vil, ZVIL_Q_HAVENAV))
        {
            VILN_tran(resp, 4, 3);
        }
        break;
    case 23:
        if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f && vil->tmr_list[ZVIL_TDUR_TAUNT] <= 0.0f)
        {
            VILN_tran(resp, 4, 4);
        }
        break;
    case 32:
        VILN_holoDeath(vil, dt, resp);
        break;
    default:
        VILN_tran(resp, 4, 0);
        break;
    }
    return 1;
}

static inline void VILN_holoDeath(st_zEntVillain* vil, float dt, st_STIMULUS_CONTEXT_DATA* resp)
{
    if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f)
    {
        if ((vil->ability & 8) && vil->tmr_list[ZVIL_TYMR_GENERIC] <= 0.0f)
        {
            zHologramAdd(vil);
            VSND_cue(vil, VSND_TYP_HOLOFADE);
            vil->tmr_list[ZVIL_TDUR_DECAY] = vil->vilcfg.tymref_list[ZVIL_TDUR_DECAY];
            vil->tmr_list[ZVIL_TYMR_GENERIC] = 3.2e7f;
        }
        VEFX_hologram_emit(vil, dt);
        if (vil->tmr_list[ZVIL_TDUR_DECAY] <= 0.0f)
        {
            VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)0xB, dt);
            VILN_tran(resp, 0, 1);
        }
    }
}

static signed int VILN_sham_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp)
{
    signed int temp_cr0_eq;
    signed int temp_r3;
    signed int temp_r5;

    temp_r3 = vil->goal;
    temp_r5 = stimctxt->stim;
    g_pgol = (en_BEHAVIOUR_GOAL)temp_r3;
    switch (temp_r3) {                              /* switch 1 */
    case -1:
        break;
    case 0:                                         /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r5;
        switch (temp_r5) {                          /* switch 2; irregular */
        case 5:                                     /* switch 2 */
        case 0:                                     /* switch 2 */
            break;
        case 2:                                     /* switch 2 */
            VILN_tran(resp, 2, 1);
            break;
        }
        break;
    case 1:                                         /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r5;
        switch (temp_r5) {                          /* switch 3; irregular */
        case 5:                                     /* switch 3 */
        case 0:                                     /* switch 3 */
            break;
        case 2:                                     /* switch 3 */
            VILN_tran(resp, 2, 1);
            break;
        }
        break;
    case 2:                                         /* switch 1 */
    case 3:                                         /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r5;
        break;
    case 4:                                         /* switch 1 */
    case 6:                                         /* switch 1 */
    case 8:                                         /* switch 1 */
    case 23:                                        /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r5;
        {
            switch (temp_r5) {                      /* switch 4 */
            case 38:
                break;
            case 0:
                break;
            case 34:                                /* switch 4 */
                vil->hitpts = 0;
                VILN_tran(resp, 0x20, 7);
                break;
            case 35:                                /* switch 4 */
                VILN_tran(resp, 0, 1);
                break;
            case 32:                                /* switch 4 */
                stimctxt->res_gods = 2;
                break;
            case 33:                                /* switch 4 */
                stimctxt->res_gods = 2;
                break;
            case 27:                                /* switch 4 */
                stimctxt->res_gods = 4;
                break;
            case 28:                                /* switch 4 */
                stimctxt->res_gods = 3;
                break;
            case 29:                                /* switch 4 */
                stimctxt->res_gods = 3;
                break;
            case 25:                                /* switch 4 */
                if (vil->ability & 0x100) {
                    VILN_tran(resp, 0x17, 0);
                }
                break;
            case 8:                                 /* switch 4 */
                VILN_stimGotoPos(vil, stimctxt);
                VILN_tran(resp, 8, 3);
                break;
            case 9:                                 /* switch 4 */
                if ((stimctxt->widge != NULL) && (vil->abl_move & 0x100)) {
                    VOUT_dogoto_widget(vil, stimctxt->widge, stimctxt->off_goto, 0);
                    vil->tmr_list[24] = -1.0f;
                    VILN_tran(resp, 8, 3);
                }
                break;
            case 10:                                /* switch 4 */
                if ((stimctxt->widge != NULL) && (vil->abl_move & 0x100)) {
                    temp_cr0_eq = VOUT_dogoto_widget(vil, stimctxt->widge, stimctxt->off_goto, 1);
                    vil->tmr_list[24] = -1.0f;
                    if (temp_cr0_eq) {
                        VILN_tran(resp, 6, 3);
                    } else {
                        VILN_tran(resp, 8, 3);
                    }
                }
                break;
            case 11:                                /* switch 4 */
                vil->tmr_list[24] = -1.0f;
                if (vil->vhf->query(vil, (en_ZVIL_QUERY)7) != 0) {
                    VNAV_get_nav(vil);
                }
                if (vil->vhf->query(vil, (en_ZVIL_QUERY)8) != 0) {
                    VILN_tran(resp, 6, 3);
                } else {
                    VILN_tran(resp, 4, 3);
                }
                break;
            case 12:                                /* switch 4 */
                VILN_tran(resp, 4, 3);
                break;
            case 13:                                /* switch 4 */
                VILN_tran(resp, 4, 3);
                break;
            case 14:                                /* switch 4 */
                VILN_tran(resp, 4, 3);
                break;
            }
        }
        break;
    case 32:                                        /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r5;
        switch (temp_r5) {                          /* switch 5; irregular */
        case 5:                                     /* switch 5 */
        case 0:                                     /* switch 5 */
            break;
        case 35:                                    /* switch 5 */
            VILN_tran(resp, 0, 1);
            break;
        }
        break;
    }
    return 1;
}

static signed int VILN_sham_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt)
{
    signed int rc = 1;
    en_NPC_STIMULUS stim = stimctxt->stim;
    en_BEHAVIOUR_GOAL goal;

    if (stim == ZVIL_STIM_ENTERGOAL || stim == ZVIL_STIM_EXITGOAL)
    {
        goal = stimctxt->trangoal;
    }
    else
    {
        goal = vil->goal;
    }

    switch (goal)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 32:
        rc = VILN_std_trangoal(vil, stimctxt, dt);
        break;
    }
    g_pgol = goal;
    switch (goal)
    {
    case ZVIL_GOAL_NONE:
        break;
    case 4:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_enterGoal(vil, (en_BEHAVIOUR_GOAL)4);
            vil->tgt_stat = 0;
            break;
        }
        break;
    case 8:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_enterGoal(vil, (en_BEHAVIOUR_GOAL)8);
            break;
        }
        break;
    case 6:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_enterGoal(vil, (en_BEHAVIOUR_GOAL)6);
            vil->tgt_stat = 0;
            if (!vil->nav_next)
            {
                vil->need_navpt = 1;
            }
            break;
        }
        break;
    case 23:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_enterGoal(vil, (en_BEHAVIOUR_GOAL)23);
            vil->tmr_list[11] = vil->vilcfg.tymref_list[11];
            break;
        }
        break;
    }
    return rc;
}

static signed int VILN_sham_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame)
{
    switch (vil->goal) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 5:
    case 7:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
        break;
    case 4:
    case 23:
        vil->ang_moveto = vil->precalc.ang_VtoP;
        vil->domove |= 0x40;
        vil->domove |= 1;
        vil->domove |= 0x10;
        vil->domove |= 0x80000000;
        break;
    case 8:
        VOUT_follow_move(vil);
        VOUT_chkdo_followArrived(vil);
        break;
    case 6:
        VOUT_patrol_move(vil);
        VOUT_chkdo_patrolArrived(vil);
        break;
    }
    return 1;
}

static signed int VILN_sham_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp)
{
    return 1;
}

static signed int VILN_sham_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot)
{

    {
        switch (goal) {
        case 0:
            break;
        case 1:
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            break;
        case 3:
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            break;
        case 4:
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            break;
        case 8:
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x8c);
            break;
        case 6:
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x8c);
            break;
        case 23:
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x8d);
            break;
        case 32:
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            }
            break;
        }
    }
    return 1;
}

static signed int VILN_BBK_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal)
{
    return VILN_flagsFromTable(vil, flaggroup);
}

static signed int VILN_BBK_query(st_zEntVillain* vil, en_ZVIL_QUERY query)
{
    return VILN_std_query(vil, query);
}

static signed int VILN_BBK_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam,
                                    xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp)
{
    if (toEvent == 0xB0)
    {
        if (vil->hitpts > 3)
        {
            return 1;
        }
        VCFG_getParm(&vil->vilcfg, ZVIL_PARM_LOB_SALVO, &vil->cnt_lobSalvo);
        if (vil->cnt_lobSalvo < 4)
        {
            vil->cnt_lobSalvo = 4 - vil->hitpts;
        }
        else
        {
            vil->cnt_lobSalvo = 0;
            switch (vil->hitpts)
            {
            case 1:
                vil->cnt_lobSalvo += 2;
            case 2:
                vil->cnt_lobSalvo += 2;
            case 3:
                vil->cnt_lobSalvo += 1;
            case 4:
                vil->cnt_lobSalvo += 1;
                break;
            case 0:
            default:
                vil->cnt_lobSalvo = 4;
                break;
            }
        }
    }
    return VILN_std_sysevent(vil, from, toEvent, toParam, toParamWidget, resp);
}

static signed int VILN_BBK_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp)
{
    xModelInstance* minst = vil->model;
    signed int idx = 0;

    if (note == 0)
    {
        g_scene_is_a_boss_battle = 1;
        vil->allow &= ~8;
        vil->hitpts = (vil->vilcfg.hitpts >= 4) ? vil->vilcfg.hitpts : 4;
        vil->vilcfg.tymref_list[22] = 2.2f;
        vil->vec_lobSpin.x = 12.566371f;
        vil->vec_lobSpin.y = 0.0f;
        vil->vec_lobSpin.z = 0.0f;
    }
    if (note == 0)
    {
        while (minst)
        {
            switch (idx)
            {
            case 0:
                minst->Flags |= 1;
                minst->Flags |= 2;
                break;
            case 1:
                break;
            }
            minst = minst->Next;
            idx++;
        }
    }
    return VILN_cmn_notice(vil, note, resp);
}

static signed int VILN_BBK_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp)
{
    switch (vil->goal)
    {
    case 2:
        VILN_tran(resp, 3, 2);
        break;
    case 3:
        VILN_tran(resp, vil->goaldflt, 3);
        break;
    case 4:
    {
        float yaw = vil->frame->rot.angle;

        if ((float)__fabs(yaw - vil->precalc.ang_VtoP) <= 0.2f)
        {
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, ZVIL_GOAL_IDLE, (en_ZVIL_GOAL_SPOT)0);
        }
        else
        {
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, ZVIL_GOAL_IDLE, (en_ZVIL_GOAL_SPOT)0xC);
        }
        if (vil->tmr_list[ZVIL_TDUR_LOBRELOAD] <= 0.0f)
        {
            if (vil->vhf->query(vil, ZVIL_Q_CANATTACK))
            {
                if (!((float)__fabs(yaw - vil->precalc.ang_VtoP) <= 0.4f))
                {
                    break;
                }
                vil->prj_reserved = VWEP_lobReservePRJ(vil);
                if (vil->prj_reserved)
                {
                    VILN_tran(resp, 0x15, 5);
                    break;
                }
            }
            VILN_BBK_patrol(vil, resp);
        }
        break;
    }
    case 8:
        if (!vil->vhf->query(vil, ZVIL_Q_DOFOLLOW))
        {
            VILN_tran(resp, 4, 3);
        }
        break;
    case 6:
        if (vil->vhf->query(vil, ZVIL_Q_NEEDNAV))
        {
            VNAV_get_nav(vil);
        }
        if (vil->vhf->query(vil, ZVIL_Q_HAVENAV))
        {
            _xVec3 delta = {};

            delta = *zMovePointGetPos(vil->nav_next);
            delta.x -= VUTIL_mdlpos(vil)->x;
            delta.y -= VUTIL_mdlpos(vil)->y;
            delta.z -= VUTIL_mdlpos(vil)->z;
            if (delta.x > 0.0f)
            {
                vil->anim_loopmult = 0.97f;
                vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)6, (en_ZVIL_GOAL_SPOT)0xB);
            }
            else
            {
                vil->anim_loopmult = 0.97f;
                vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)6, (en_ZVIL_GOAL_SPOT)0xC);
            }
        }
        else
        {
            VILN_tran(resp, 4, 3);
        }
        break;
    case 21:
        if (vil->anim_playing && vil->anim_playnext == 0x40 && VANM_inDmgWindow(vil) && vil->cnt_lobSalvo > 0)
        {
            _xVec3 off = {};
            float spread;

            off.x = 2.0f * (iurand() - 0.5f);
            off.y = -iurand();
            off.z = -0.1f;
            xVec3Normalize(&off, &off);
            spread = 5.0f * (vil->cnt_lobSalvo - 1);
            off.x *= spread;
            off.y *= spread;
            off.z *= spread;
            VWEP_lobActivatePRJ(vil->prj_reserved, vil, xBoundCenter(&globals.player.ent.bound), &off,
                                &vil->vec_lobSpin, vil->fac_lobarc, vil->fac_lobheavy);
            vil->cnt_lobSalvo--;
            if (vil->cnt_lobSalvo > 0 && !vil->prj_reserved)
            {
                vil->prj_reserved = VWEP_lobReservePRJ(vil);
            }
        }
        if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f)
        {
            if (vil->cnt_lobSalvo < 1)
            {
                zEntEvent(vil, vil, 0xB1);
            }
            VILN_tran(resp, 4, 5);
        }
        break;
    case 23:
        if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f && vil->tmr_list[ZVIL_TDUR_TAUNT] <= 0.0f)
        {
            VILN_tran(resp, 4, 4);
        }
        break;
    case 27:
        if (vil->tmr_list[22] <= 0.0f && vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f)
        {
            if (vil->hitpts < 1)
            {
                VILN_tran(resp, 0x20, 7);
            }
            else
            {
                VILN_tran(resp, 4, 3);
            }
        }
        break;
    case 32:
        VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)0xA, dt);
        VILN_holoDeath(vil, dt, resp);
        break;
    default:
        VILN_tran(resp, 4, 0);
        break;
    }
    return 1;
}

static inline void VILN_BBK_patrol(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp)
{
    if (vil->tmr_list[ZVIL_TYMR_CURGOAL] > 120.0f)
    {
        vil->cnt_lobSalvo = 0;
        vil->allow &= ~8;
        vil->need_navpt = 1;
        vil->allow |= 1;
    }
    if (vil->cnt_lobSalvo <= 0 || !vil->vhf->query(vil, ZVIL_Q_CANATTACK))
    {
        if (vil->vhf->query(vil, ZVIL_Q_CANPATROL))
        {
            if (vil->vhf->query(vil, ZVIL_Q_NEEDNAV))
            {
                VNAV_get_nav(vil);
                if (!vil->nav_next)
                {
                    vil->nav_prev = vil->nav_mvptlist;
                    vil->nav_next = vil->nav_mvptlist;
                }
            }
            if (vil->vhf->query(vil, ZVIL_Q_HAVENAV))
            {
                VILN_tran(resp, 6, 3);
            }
        }
    }
}

static signed int VILN_BBK_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp)
{
    signed int temp_cr0_eq;
    signed int temp_r3;
    signed int temp_r5;

    temp_r3 = vil->goal;
    temp_r5 = stimctxt->stim;
    g_pgol = (en_BEHAVIOUR_GOAL)temp_r3;
    switch (temp_r3) {                              /* switch 1 */
    case -1:
        break;
    case 0:                                         /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r5;
        switch (temp_r5) {                          /* switch 2; irregular */
        case 5:                                     /* switch 2 */
        case 0:                                     /* switch 2 */
            break;
        case 2:                                     /* switch 2 */
            VILN_tran(resp, 2, 1);
            break;
        }
        break;
    case 1:                                         /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r5;
        switch (temp_r5) {                          /* switch 3; irregular */
        case 5:                                     /* switch 3 */
        case 0:                                     /* switch 3 */
            break;
        case 2:                                     /* switch 3 */
            VILN_tran(resp, 2, 1);
            break;
        }
        break;
    case 2:                                         /* switch 1 */
    case 3:                                         /* switch 1 */
    case 27:                                        /* switch 1 */
    case 32:                                        /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r5;
        break;
    case 4:                                         /* switch 1 */
    case 6:                                         /* switch 1 */
    case 8:                                         /* switch 1 */
    case 21:                                        /* switch 1 */
    case 23:                                        /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r5;
        {
            switch (temp_r5) {                      /* switch 4 */
            case 0:
                break;
            case 34:                                /* switch 4 */
                vil->hitpts = 0;
                VILN_tran(resp, 0x20, 7);
                break;
            case 35:                                /* switch 4 */
                VILN_tran(resp, 0, 1);
                break;
            case 38:                                /* switch 4 */
                VILN_tran(resp, 0x1B, 5);
                break;
            case 32:                                /* switch 4 */
                stimctxt->res_gods = 2;
                break;
            case 33:                                /* switch 4 */
                stimctxt->res_gods = 2;
                break;
            case 27:                                /* switch 4 */
                stimctxt->res_gods = 4;
                break;
            case 28:                                /* switch 4 */
                stimctxt->res_gods = 3;
                break;
            case 29:                                /* switch 4 */
                stimctxt->res_gods = 3;
                break;
            case 23:                                /* switch 4 */
                if ((vil->allow & 8) && (vil->abl_atak & 1)) {
                    VILN_tran(resp, 4, 5);
                }
                break;
            case 25:                                /* switch 4 */
                if (vil->ability & 0x100) {
                    VILN_tran(resp, 0x17, 0);
                }
                break;
            case 8:                                 /* switch 4 */
                VILN_stimGotoPos(vil, stimctxt);
                VILN_tran(resp, 8, 3);
                break;
            case 9:                                 /* switch 4 */
                if ((stimctxt->widge != NULL) && (vil->abl_move & 0x100)) {
                    VOUT_dogoto_widget(vil, stimctxt->widge, stimctxt->off_goto, 0);
                    vil->tmr_list[24] = -1.0f;
                    VILN_tran(resp, 8, 0);
                }
                break;
            case 10:                                /* switch 4 */
                if ((stimctxt->widge != NULL) && (vil->abl_move & 0x100)) {
                    temp_cr0_eq = VOUT_dogoto_widget(vil, stimctxt->widge, stimctxt->off_goto, 1);
                    vil->tmr_list[24] = -1.0f;
                    if (temp_cr0_eq) {
                        VILN_tran(resp, 6, 0);
                    } else {
                        VILN_tran(resp, 8, 0);
                    }
                }
                break;
            case 11:                                /* switch 4 */
                vil->tmr_list[24] = -1.0f;
                if (vil->vhf->query(vil, (en_ZVIL_QUERY)7) != 0) {
                    VNAV_get_nav(vil);
                }
                if (vil->vhf->query(vil, (en_ZVIL_QUERY)8) != 0) {
                    VILN_tran(resp, 6, 0);
                } else {
                    VILN_tran(resp, 4, 0);
                }
                break;
            case 12:                                /* switch 4 */
                VILN_tran(resp, 4, 0);
                break;
            case 13:                                /* switch 4 */
                VILN_tran(resp, 4, 0);
                break;
            case 14:                                /* switch 4 */
                VILN_tran(resp, 4, 0);
                break;
            }
        }
        break;
    }
    return 1;
}

static signed int VILN_BBK_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt)
{
    signed int rc = 1;
    en_NPC_STIMULUS stim = stimctxt->stim;
    en_BEHAVIOUR_GOAL goal;

    if (stim == ZVIL_STIM_ENTERGOAL || stim == ZVIL_STIM_EXITGOAL)
    {
        goal = stimctxt->trangoal;
    }
    else
    {
        goal = vil->goal;
    }

    switch (goal)
    {
    case 0:
    case 1:
    case 2:
    case 3:
        rc = VILN_std_trangoal(vil, stimctxt, dt);
        break;
    case 0x20:
        rc = VILN_std_trangoal(vil, stimctxt, dt);
        if (stim == ZVIL_STIM_ENTERGOAL)
        {
            VEFX_zapTheKnight(vil, 0);
        }
        else if (stim == ZVIL_STIM_EXITGOAL)
        {
            VEFX_zapTheKnight(vil, 1);
        }
        break;
    }

    g_pgol = goal;
    switch (goal)
    {
    case ZVIL_GOAL_NONE:
        break;
    case 8:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_enterGoal(vil, (en_BEHAVIOUR_GOAL)8);
            break;
        }
        break;
    case 6:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_enterGoal(vil, (en_BEHAVIOUR_GOAL)6);
            vil->tgt_stat = 0;
            if (!vil->nav_next)
            {
                vil->need_navpt = 1;
            }
            break;
        }
        break;
    case 21:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_enterGoal(vil, (en_BEHAVIOUR_GOAL)21);
            vil->tmr_list[21] = -1.0f;
            break;
        case ZVIL_STIM_EXITGOAL:
            if (vil->prj_reserved)
            {
                vil->prj_reserved->Abort();
            }
            vil->prj_reserved = NULL;
            break;
        }
        break;
    case 23:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_enterGoal(vil, (en_BEHAVIOUR_GOAL)23);
            vil->tmr_list[11] = vil->vilcfg.tymref_list[11];
            vil->allow &= ~8;
            break;
        }
        break;
    case 27:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            float tym;

            VILN_enterGoal(vil, (en_BEHAVIOUR_GOAL)27);
            tym = vil->vilcfg.tymref_list[22];
            vil->tmr_list[22] = VUTIL_jitter(tym);
            vil->hitpts--;
            VEFX_zapTheKnight(vil, 0);
            vil->allow &= ~8;
            break;
        case ZVIL_STIM_EXITGOAL:
            VEFX_zapTheKnight(vil, 1);
            xSndStopChildren((unsigned int)vil + 1);
            break;
        }
        break;
    }
    return rc;
}

static signed int VILN_BBK_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame)
{
    switch (vil->goal)
    {
    case 0:
    case 1:
    case 2:
    case 3:
        break;
    case 4:
    case 17:
    case 21:
    case 23:
        vil->ang_moveto = vil->precalc.ang_VtoP;
        vil->domove |= 0x40;
        vil->domove |= 1;
        vil->domove |= 0x10;
        vil->domove |= 0x80000000;
        break;
    case 8:
        VOUT_follow_move(vil);
        VOUT_chkdo_followArrived(vil);
        break;
    case 6:
        VOUT_patrol_move(vil);
        VOUT_chkdo_patrolArrived(vil);
        break;
    case 27:
    case 32:
        if (vil->hitpts <= 0)
        {
            vil->ang_moveto = 4.912389f;
        }
        vil->domove |= 0x40;
        vil->domove |= 1;
        vil->domove |= 0x10;
        vil->domove |= 0x80000000;
        break;
    }
    return 1;
}

static signed int VILN_BBK_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp)
{
    return 1;
}

static signed int VILN_BBK_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot)
{

    {
        switch (goal) {                       /* switch 1 */
        case 0:
            break;
        case 1:                                     /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            break;
        case 3:                                     /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            break;
        case 4:                                     /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            } else if (gspot == 2) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x3b);
            } else if (gspot == 12) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x3d);
            }
            break;
        case 8:                                     /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x3c);
            break;
        case 6:                                     /* switch 1 */
            if (gspot == 0) {
                VANM_play3(vil, (en_NPC_ANIMSTATES)0x3c, (en_NPC_ANIMSTATES)0x3e, (en_NPC_ANIMSTATES)0x3f);
            } else if (gspot == 11) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x3f, (en_NPC_ANIMSTATES)0x3c);
            } else if (gspot == 12) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x3e, (en_NPC_ANIMSTATES)0x3c);
            }
            break;
        case 23:                                    /* switch 1 */
            if ((gspot == 0) || (gspot == 0xD) || (gspot == 0xE)) {
                VANM_playRand3(vil, (en_NPC_ANIMSTATES)0x43, (en_NPC_ANIMSTATES)0x44, (en_NPC_ANIMSTATES)0x45);
            }
            break;
        case 21:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x40);
            }
            break;
        case 27:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x41);
            } else if (gspot == 15) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x42);
            } else if (gspot == 16) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x41);
            }
            break;
        case 32:                                    /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x46);
            break;
        }
    }
    return 1;
}

static signed int VILN_BBK_animtran(st_zEntVillain* vil, xAnimTransition* tran, xAnimSingle* single)
{
    xModelInstance* minst = vil->model;
    signed int idx = 0;

    while (minst)
    {
        switch (idx)
        {
        case 0:
            if (tran->Dest->ID == g_anamhash[66])
            {
                minst->Flags &= ~1;
            }
            else
            {
                minst->Flags |= 2;
                minst->Flags |= 1;
            }
            break;
        case 1:
            if (tran->Dest->ID == g_anamhash[66])
            {
                minst->Flags |= 2;
                minst->Flags |= 1;
            }
            else
            {
                minst->Flags &= ~1;
            }
            break;
        }
        minst = minst->Next;
        idx++;
    }
    return 1;
}

static signed int VILN_BGG_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal)
{
    return VILN_flagsFromTable(vil, flaggroup);
}

static signed int VILN_BGG_query(st_zEntVillain* vil, en_ZVIL_QUERY query)
{
    return VILN_std_query(vil, query);
}

static signed int VILN_BGG_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp)
{
    return VILN_std_sysevent(vil, from, toEvent, toParam, toParamWidget, resp);
}

static signed int VILN_BGG_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp)
{
    if (note == 0)
    {
        g_scene_is_a_boss_battle = 2;
        vil->allow &= ~8;
        vil->hitpts = (vil->vilcfg.hitpts >= 3) ? vil->vilcfg.hitpts : 3;
        vil->vilcfg.tymref_list[22] = 3.0f;
    }
    return VILN_cmn_notice(vil, note, resp);
}

static signed int VILN_BGG_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp)
{
    switch (vil->goal)
    {
    case 2:
        VILN_tran(resp, 3, 2);
        break;
    case 3:
        VILN_tran(resp, vil->goaldflt, 3);
        break;
    case 4:
        if (vil->tmr_list[ZVIL_TDUR_LOBRELOAD] <= 0.0f)
        {
            if (vil->vhf->query(vil, ZVIL_Q_CANATTACK) && (vil->prj_reserved = VWEP_lobReservePRJ(vil)) != NULL)
            {
                VILN_tran(resp, 0x15, 5);
            }
            else
            {
                if (vil->allow & 8)
                {
                    zEntEvent(vil, vil, 0xB1);
                }
                if (vil->vhf->query(vil, ZVIL_Q_CANDETECT))
                {
                    vil->vhf->query(vil, ZVIL_Q_PLYRTOOCLOSE);
                    vil->vhf->query(vil, ZVIL_Q_PLYRNEAR);
                }
                if (vil->vhf->query(vil, ZVIL_Q_CANPATROL))
                {
                    if (vil->vhf->query(vil, ZVIL_Q_NEEDNAV))
                    {
                        VNAV_get_nav(vil);
                    }
                    if (vil->vhf->query(vil, ZVIL_Q_HAVENAV))
                    {
                        VILN_tran(resp, 6, 3);
                    }
                }
            }
        }
        break;
    case 8:
        if (!vil->vhf->query(vil, ZVIL_Q_DOFOLLOW))
        {
            VILN_tran(resp, 4, 3);
        }
        break;
    case 6:
        if (vil->vhf->query(vil, ZVIL_Q_PLYRTOOCLOSE))
        {
            VILN_tran(resp, 4, 5);
        }
        else
        {
            if (vil->vhf->query(vil, ZVIL_Q_NEEDNAV))
            {
                VNAV_get_nav(vil);
            }
            if (vil->vhf->query(vil, ZVIL_Q_HAVENAV))
            {
                _xVec3 delta = {};

                delta = *zMovePointGetPos(vil->nav_next);
                delta.x -= VUTIL_mdlpos(vil)->x;
                delta.y -= VUTIL_mdlpos(vil)->y;
                delta.z -= VUTIL_mdlpos(vil)->z;
                if (delta.x > 0.0f)
                {
                    vil->anim_loopmult = 0.97f;
                    vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)6, (en_ZVIL_GOAL_SPOT)0xB);
                }
                else
                {
                    vil->anim_loopmult = 0.97f;
                    vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)6, (en_ZVIL_GOAL_SPOT)0xC);
                }
            }
            else
            {
                VILN_tran(resp, 4, 3);
            }
        }
        break;
    case 21:
    {
        signed int anim = vil->anim_playnext;
        if (anim != 0x4A)
        {
            if (vil->anim_playing && anim == 0x4B)
            {
                if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f)
                {
                    vil->anim_loopmult = 0.97f;
                    vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x15, (en_ZVIL_GOAL_SPOT)4);
                }
            }
            else if (vil->anim_playing && anim == 0x4C)
            {
                VILN_BGG_quakeSalvo(vil);
            }
        }
        if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f)
        {
            VILN_tran(resp, 4, 5);
        }
        break;
    }
    case 23:
        if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f && vil->tmr_list[ZVIL_TDUR_TAUNT] <= 0.0f)
        {
            VILN_tran(resp, 4, 4);
        }
        break;
    case 27:
        if (vil->hitpts > 0 && (vil->flg_misc & 4))
        {
            float spd = vil->sanim_chase.y;
            if (vil->anim_playing && !(spd < 0.001f))
            {
                xAnimSingle* single = VANM_single(vil);
                if (single)
                {
                    single->CurrentSpeed = spd;
                }
            }
        }
        *((unsigned char*)sEmitGGVortexHurt + 0x18) |= 1;
        if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f && vil->tmr_list[22] <= 0.0f)
        {
            if (vil->hitpts < 1)
            {
                VILN_tran(resp, 0x20, 7);
            }
            else
            {
                VILN_tran(resp, 4, 3);
            }
            *((unsigned char*)sEmitGGVortexHurt + 0x18) &= ~1;
        }
        break;
    case 32:
        VILN_holoDeath(vil, dt, resp);
        break;
    default:
        VILN_tran(resp, 4, 0);
        break;
    }
    return 1;
}

static inline void VILN_BGG_quakeSalvo(st_zEntVillain* vil)
{
    if (VANM_inDmgWindow(vil) && vil->prj_reserved && vil->cnt_lobSalvo > 0)
    {
        _xVec3 off = {};
        _xVec3* ctr;
        st_BGG_QUAKEBALL* quake;

        if (!(vil->cnt_lobSalvo & 1))
        {
            off.x = vil->precalc.vec_VtoP.x * 0.6f;
            off.y = 0.0f;
            off.z = vil->precalc.vec_VtoP.z * 0.6f;
        }
        ctr = xBoundCenter(&globals.player.ent.bound);
        quake = (st_BGG_QUAKEBALL*)vil->prj_reserved->xtradata;
        quake->pos_tgt.x = off.x + ctr->x;
        quake->pos_tgt.y = off.y + ctr->y;
        quake->pos_tgt.z = off.z + ctr->z;
        quake->phase = BGG_PHAZ_FLY;
        quake->flg_stat |= 2;
        vil->prj_reserved = NULL;
        vil->cnt_lobSalvo--;
        if (vil->cnt_lobSalvo > 0 && !vil->prj_reserved)
        {
            zEntProjectile* prj;
            vil->prj_reserved = VWEP_lobReservePRJ(vil);
            prj = vil->prj_reserved;
            if (prj)
            {
                VWEP_lobActivatePRJ(prj, vil, xBoundCenter(&globals.player.ent.bound), NULL, &vil->vec_lobSpin,
                                    vil->fac_lobarc, vil->fac_lobheavy);
            }
        }
    }
}

static signed int VILN_BGG_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp)
{
    signed int temp_cr0_eq;
    signed int temp_r3;
    signed int temp_r5;

    temp_r3 = vil->goal;
    temp_r5 = stimctxt->stim;
    g_pgol = (en_BEHAVIOUR_GOAL)temp_r3;
    switch (temp_r3) {                              /* switch 1 */
    case -1:
        break;
    case 0:                                         /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r5;
        switch (temp_r5) {                          /* switch 2; irregular */
        case 5:                                     /* switch 2 */
        case 0:                                     /* switch 2 */
            break;
        case 2:                                     /* switch 2 */
            VILN_tran(resp, 2, 1);
            break;
        }
        break;
    case 1:                                         /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r5;
        switch (temp_r5) {                          /* switch 3; irregular */
        case 5:                                     /* switch 3 */
        case 0:                                     /* switch 3 */
            break;
        case 2:                                     /* switch 3 */
            VILN_tran(resp, 2, 1);
            break;
        }
        break;
    case 2:                                         /* switch 1 */
    case 3:                                         /* switch 1 */
    case 27:                                        /* switch 1 */
    case 32:                                        /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r5;
        break;
    case 4:                                         /* switch 1 */
    case 6:                                         /* switch 1 */
    case 8:                                         /* switch 1 */
    case 22:                                        /* switch 1 */
    case 23:                                        /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r5;
        {
            switch (temp_r5) {                      /* switch 4 */
            case 0:
                break;
            case 34:                                /* switch 4 */
                vil->hitpts = 0;
                VILN_tran(resp, 0x20, 7);
                break;
            case 35:                                /* switch 4 */
                VILN_tran(resp, 0, 1);
                break;
            case 38:                                /* switch 4 */
                VILN_tran(resp, 0x1B, 5);
                break;
            case 32:                                /* switch 4 */
                stimctxt->res_gods = 2;
                break;
            case 33:                                /* switch 4 */
                stimctxt->res_gods = 2;
                break;
            case 27:                                /* switch 4 */
                stimctxt->res_gods = 4;
                break;
            case 28:                                /* switch 4 */
                stimctxt->res_gods = 3;
                break;
            case 29:                                /* switch 4 */
                stimctxt->res_gods = 3;
                break;
            case 23:                                /* switch 4 */
                if ((vil->allow & 8) && (vil->abl_atak & 1)) {
                    VILN_tran(resp, 4, 5);
                }
                break;
            case 25:                                /* switch 4 */
                if (vil->ability & 0x100) {
                    VILN_tran(resp, 0x17, 0);
                }
                break;
            case 8:                                 /* switch 4 */
                VILN_stimGotoPos(vil, stimctxt);
                VILN_tran(resp, 8, 3);
                break;
            case 9:                                 /* switch 4 */
                if ((stimctxt->widge != NULL) && (vil->abl_move & 0x100)) {
                    VOUT_dogoto_widget(vil, stimctxt->widge, stimctxt->off_goto, 0);
                    vil->tmr_list[24] = -1.0f;
                    VILN_tran(resp, 8, 3);
                }
                break;
            case 10:                                /* switch 4 */
                if ((stimctxt->widge != NULL) && (vil->abl_move & 0x100)) {
                    temp_cr0_eq = VOUT_dogoto_widget(vil, stimctxt->widge, stimctxt->off_goto, 1);
                    vil->tmr_list[24] = -1.0f;
                    if (temp_cr0_eq) {
                        VILN_tran(resp, 6, 3);
                    } else {
                        VILN_tran(resp, 8, 3);
                    }
                }
                break;
            case 11:                                /* switch 4 */
                vil->tmr_list[24] = -1.0f;
                if (vil->vhf->query(vil, (en_ZVIL_QUERY)7) != 0) {
                    VNAV_get_nav(vil);
                }
                if (vil->vhf->query(vil, (en_ZVIL_QUERY)8) != 0) {
                    VILN_tran(resp, 6, 3);
                } else {
                    VILN_tran(resp, 4, 3);
                }
                break;
            case 12:                                /* switch 4 */
                VILN_tran(resp, 4, 3);
                break;
            case 13:                                /* switch 4 */
                VILN_tran(resp, 4, 3);
                break;
            case 14:                                /* switch 4 */
                VILN_tran(resp, 4, 3);
                break;
            }
        }
        break;
    }
    return 1;
}

static signed int VILN_BGG_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt)
{
    signed int temp_r0;
    signed int temp_r29;
    zEntProjectile* temp_r29_2;
    signed int temp_r3;
    zEntProjectile* temp_r3_2;
    xBase* temp_r3_3;
    _xVec3* temp_r3_4;
    _xMat4x3* temp_r5;
    signed int var_r0;
    signed int var_r28;
    signed int var_r31;

    var_r31 = 1;
    temp_r29 = stimctxt->stim;
    if ((temp_r29 == 4) || (temp_r29 == 3)) {
        var_r28 = stimctxt->trangoal;
    } else {
        var_r28 = vil->goal;
    }
    switch (var_r28) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 32:
        var_r31 = VILN_std_trangoal(vil, stimctxt, dt);
        break;
    }
    g_pgol = (en_BEHAVIOUR_GOAL)var_r28;
    switch (var_r28) {                              /* switch 1 */
    case -1:
        break;
    case 4:                                         /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r29;
        switch (temp_r29) {                         /* switch 2; irregular */
        case 3:                                     /* switch 2 */
        case 5:                                     /* switch 2 */
        case 0:                                     /* switch 2 */
            break;
        case 4:                                     /* switch 2 */
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)4, (en_ZVIL_GOAL_SPOT)0);
            vil->tgt_stat = 0;
            break;
        }
        break;
    case 8:                                         /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r29;
        switch (temp_r29) {                         /* switch 3; irregular */
        case 3:                                     /* switch 3 */
        case 5:                                     /* switch 3 */
        case 0:                                     /* switch 3 */
            break;
        case 4:                                     /* switch 3 */
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)8, (en_ZVIL_GOAL_SPOT)0);
            break;
        }
        break;
    case 6:                                         /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r29;
        switch (temp_r29) {                         /* switch 4; irregular */
        case 3:                                     /* switch 4 */
        case 5:                                     /* switch 4 */
        case 0:                                     /* switch 4 */
            break;
        case 4:                                     /* switch 4 */
            if (vil->allow & 8) {
                zEntEvent((xBase *) vil, (xBase *) vil, 0xB1U);
            }
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)6, (en_ZVIL_GOAL_SPOT)0);
            vil->tgt_stat = 0;
            if ((unsigned int)vil->nav_next == 0U) {
                vil->need_navpt = 1;
            }
            break;
        }
        break;
    case 21:                                        /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r29;
        switch (temp_r29) {                         /* switch 5; irregular */
        case 5:                                     /* switch 5 */
        case 0:                                     /* switch 5 */
            break;
        case 4:                                     /* switch 5 */
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x15, (en_ZVIL_GOAL_SPOT)0);
            temp_r29_2 = vil->prj_reserved;
            VWEP_lobActivatePRJ(temp_r29_2, vil, xBoundCenter(&globals.player.ent.bound), NULL, &vil->vec_lobSpin, vil->fac_lobarc, vil->fac_lobheavy);
            vil->tmr_list[21] = -1.0f;
            VCFG_getParm(&vil->vilcfg, (en_NPC_INI_PARAMETERS) 0xC, &vil->cnt_lobSalvo);
            temp_r3 = (vil->cnt_lobSalvo >= 1) ? vil->cnt_lobSalvo : 1;
            vil->cnt_lobSalvo = temp_r3;
            vil->cnt_lobSalvo = temp_r3;
            break;
        case 3:                                     /* switch 5 */
            if (vil->allow & 8) {
                zEntEvent((xBase *) vil, (xBase *) vil, 0xB1U);
            }
            temp_r3_2 = vil->prj_reserved;
            if (temp_r3_2 != NULL) {
                temp_r3_2->Abort();
            }
            vil->prj_reserved = NULL;
            break;
        }
        break;
    case 23:                                        /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r29;
        switch (temp_r29) {                         /* switch 6; irregular */
        case 3:                                     /* switch 6 */
        case 5:                                     /* switch 6 */
        case 0:                                     /* switch 6 */
            break;
        case 4:                                     /* switch 6 */
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x17, (en_ZVIL_GOAL_SPOT)0);
            vil->tmr_list[11] = vil->vilcfg.tymref_list[11];
            break;
        }
        break;
    case 27:                                        /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r29;
        switch (temp_r29) {                         /* switch 7; irregular */
        case 3:                                     /* switch 7 */
        case 5:                                     /* switch 7 */
        case 0:                                     /* switch 7 */
            break;
        case 4:                                     /* switch 7 */
            vil->tmr_list[22] = 0.5f;
            temp_r0 = vil->hitpts;
            if (temp_r0 > 2) {
                vil->anim_loopmult = 0.97f;
                vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x1B, (en_ZVIL_GOAL_SPOT)0);
            } else if (temp_r0 == 2) {
                vil->anim_loopmult = 0.97f;
                vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x1B, (en_ZVIL_GOAL_SPOT)0xF);
            } else if (temp_r0 <= 1) {
                vil->anim_loopmult = 0.97f;
                vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x1B, (en_ZVIL_GOAL_SPOT)0x10);
            }
            vil->hitpts -= 1;
            temp_r3_3 = stimctxt->widge;
            if ((temp_r3_3 != NULL) && ((unsigned char)temp_r3_3->baseType == 0xD)) {
                temp_r3_4 = zMovePointGetPos((_zMovePoint*)temp_r3_3);
                vil->pos_vortex.x = temp_r3_4->x;
                vil->pos_vortex.y = temp_r3_4->y;
                vil->pos_vortex.z = temp_r3_4->z;
                vil->pos_vortex.y = vil->frame->mat.pos.y;
                _xVec3 vec14 = {};
                temp_r5 = (_xMat4x3*)vil->model->Mat;
                vec14.x = temp_r5->pos.x - vil->pos_vortex.x;
                vec14.y = temp_r5->pos.y - vil->pos_vortex.y;
                vec14.z = temp_r5->pos.z - vil->pos_vortex.z;
                vil->rad_spiral = xVec3Normalize(&vec14, &vec14);
                vil->ang_spiral = VSUP_yaw(vec14);
            }
            break;
        }
        break;
    }
    return var_r31;
}

static signed int VILN_BGG_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame)
{
    _xVec3 pos = {};

    pos = ((_xMat4x3*)vil->model->Mat)->pos;

    switch (vil->goal)
    {
    case 0:
    case 1:
    case 2:
    case 3:
        break;
    case 4:
    case 21:
    case 23:
        vil->ang_moveto = vil->precalc.ang_VtoP;
        vil->domove |= 0x40;
        vil->domove |= 1;
        vil->domove |= 0x10;
        vil->domove |= 0x80000000;
        break;
    case 8:
        VOUT_follow_move(vil);
        VILN_BGG_face(vil, &pos);
        VOUT_chkdo_followArrived(vil);
        break;
    case 6:
        VOUT_patrol_move(vil);
        VILN_BGG_face(vil, &pos);
        VOUT_chkdo_patrolArrived(vil);
        break;
    case 27:
    case 32:
    {
        // Spiral in towards the vortex.
        _xVec3 off = {};

        VSUP_ang_toXZDir(vil->ang_spiral, &off);
        xVec3SMulBy(&off, vil->rad_spiral);
        xVec3Add(&vil->pos_moveto, &vil->pos_vortex, &off);
        vil->domove |= 0x800;
        vil->domove |= 0x40000000;
        vil->domove |= 0xA;
        vil->domove |= 0x80000000;
        vil->ang_spiral = xAngleClamp(3.1415927f * dt + vil->ang_spiral);
        vil->rad_spiral *= 0.95f;
        if (vil->hitpts == 0)
        {
            vil->pos_vortex.y = -(0.6f * dt - vil->pos_vortex.y);
        }
        break;
    }
    }
    return 1;
}

// The Green Ghost faces left or right of where it was standing.
static inline void VILN_BGG_face(st_zEntVillain* vil, _xVec3* pos)
{
    vil->domove &= ~0xC0000;
    if (vil->pos_moveto.x - pos->x > 0.0f)
    {
        vil->domove |= 0x80000;
    }
    else
    {
        vil->domove |= 0x40000;
    }
}

static signed int VILN_BGG_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp)
{
    return 1;
}

static signed int VILN_BGG_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot)
{

    {
        switch (goal) {                       /* switch 1 */
        case 0:
            break;
        case 1:                                     /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            break;
        case 3:                                     /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            break;
        case 4:                                     /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            break;
        case 8:                                     /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x47);
            break;
        case 6:                                     /* switch 1 */
            if (gspot == 0) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x49, (en_NPC_ANIMSTATES)0x47);
            } else if (gspot == 11) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x48, (en_NPC_ANIMSTATES)0x47);
            } else if (gspot == 12) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x49, (en_NPC_ANIMSTATES)0x47);
            }
            break;
        case 23:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x4d, (en_NPC_ANIMSTATES)0x4e);
            } else if (gspot == 0xD) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x4e, (en_NPC_ANIMSTATES)0x4d);
            }
            break;
        case 21:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_play3(vil, (en_NPC_ANIMSTATES)0x4a, (en_NPC_ANIMSTATES)0x4b, (en_NPC_ANIMSTATES)0x4c);
                vil->anim_loopmult = 2.0f + vil->hitpts;
            } else if (gspot == 4) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x4c);
            }
            break;
        case 27:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x4f);
                vil->anim_loopmult = 3.17f;
            } else if (gspot == 15) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x50, (en_NPC_ANIMSTATES)0x4f);
                vil->anim_loopmult = 2.47f;
            } else if (gspot == 16) {
                VANM_play(vil, (en_NPC_ANIMSTATES)0x51, (en_NPC_ANIMSTATES)0x4f);
                vil->anim_loopmult = 1.0f;
            }
            break;
        case 32:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_play3(vil, (en_NPC_ANIMSTATES)0x51, (en_NPC_ANIMSTATES)0x50, (en_NPC_ANIMSTATES)0x4f);
            }
            break;
        }
    }
    return 1;
}

static signed int VILN_BRB_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal)
{
    return VILN_flagsFromTable(vil, flaggroup);
}

static signed int VILN_BRB_query(st_zEntVillain* vil, en_ZVIL_QUERY query)
{
    return VILN_std_query(vil, query);
}

static signed int VILN_BRB_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam, xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp)
{
    return VILN_std_sysevent(vil, from, toEvent, toParam, toParamWidget, resp);
}

static signed int VILN_BRB_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp)
{
    static float tmr_flames = 0.0f;
    static _xVec3 pos_start = { -0.5f, 0.0f, 0.0f };
    xModelInstance* minst = vil->model;
    signed int idx = 0;

    if (sEmitRBFlames)
    {
        if ((unsigned int)(vil->goal + 1) <= 5 || vil->goal == 0x20)
        {
            VILN_BRB_flamesnd(0x13, 0);
        }
        else
        {
            _xVec3* pos = &((_xMat4x3*)globals.player.ent.model->Mat)->pos;
            float dx = -0.5f - pos->x;
            float dz = pos->z;

            if (dx * dx + dz * dz < 30.0f)
            {
                if (!(*((unsigned char*)sEmitRBFlames + 0x18) & 1))
                {
                    VILN_BRB_flamesnd(0x18, 1);
                    tmr_flames = 3.0f;
                }
            }
            else
            {
                tmr_flames -= 0.033333335f;
                if (tmr_flames <= 0.0f && (*((unsigned char*)sEmitRBFlames + 0x18) & 1))
                {
                    VILN_BRB_flamesnd(0x13, 0);
                }
            }
        }
    }

    if (note == 0)
    {
        g_scene_is_a_boss_battle = 3;
        vil->allow &= ~8;
        vil->hitpts = (vil->vilcfg.hitpts >= 4) ? vil->vilcfg.hitpts : 4;
        vil->mode = (en_NPC_MODE)5;
        vil->vec_lobSpin.y = 6.2831855f;
        vil->vilcfg.tymref_list[22] = -1.0f;
        *VUTIL_mdlpos(vil) = pos_start;
    }
    if (note == 0)
    {
        VWEP_rbs_setup(&g_rbsword[0], vil);
    }
    if (note == 0)
    {
        while (minst)
        {
            switch (idx)
            {
            case 0:
                minst->Flags |= 1;
                minst->Flags |= 2;
                minst->Flags |= 4;
                break;
            case 1:
                minst->Flags &= 0xFFDF;
                minst->Flags |= 8;
                minst->Flags |= 1;
                minst->Flags |= 2;
                minst->Flags |= 4;
                break;
            }
            minst = minst->Next;
            idx++;
        }
    }
    return VILN_cmn_notice(vil, note, resp);
}

static inline void VILN_BRB_flamesnd(unsigned int evt, signed int on)
{
    zEntEvent((xBase*)zSceneFindObject(xStrHash("FIREBALL_LOOP_SFX")), evt);
    if (on)
    {
        *((unsigned char*)sEmitRBFlames + 0x18) |= 1;
    }
    else
    {
        *((unsigned char*)sEmitRBFlames + 0x18) &= ~1;
    }
}

static signed int VILN_BRB_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp)
{
    switch (vil->goal)
    {
    case 2:
        VILN_tran(resp, 3, 2);
        break;
    case 3:
        VILN_tran(resp, vil->goaldflt, 5);
        break;
    case 4:
    {
        if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] < 0.3f)
        {
            float da = vil->frame->rot.angle - vil->precalc.ang_VtoP;
            vil->anim_loopmult = 0.97f;
            if ((float)__fabs(da) <= 0.2f)
            {
                vil->vhf->pickanim(vil, ZVIL_GOAL_IDLE, (en_ZVIL_GOAL_SPOT)0);
            }
            else
            {
                vil->vhf->pickanim(vil, ZVIL_GOAL_IDLE, (en_ZVIL_GOAL_SPOT)0xC);
            }
        }
        if (vil->vhf->query(vil, ZVIL_Q_CANATTACK) && vil->cnt_lobSalvo > 0)
        {
            VILN_tran(resp, 0x16, 0);
            break;
        }
        if (vil->vhf->query(vil, ZVIL_Q_CANDETECT))
        {
            vil->vhf->query(vil, ZVIL_Q_PLYRTOOCLOSE);
            vil->vhf->query(vil, ZVIL_Q_PLYRNEAR);
        }
        if (vil->vhf->query(vil, ZVIL_Q_CANPATROL))
        {
            if (vil->vhf->query(vil, ZVIL_Q_NEEDNAV))
            {
                VNAV_get_nav(vil);
            }
            if (vil->vhf->query(vil, ZVIL_Q_HAVENAV))
            {
                VILN_tran(resp, 6, 0);
                break;
            }
        }
        if (vil->tmr_list[0] <= 0.0f)
        {
            float tym;
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, ZVIL_GOAL_IDLE, (en_ZVIL_GOAL_SPOT)2);
            tym = vil->vilcfg.tymref_list[0];
            vil->tmr_list[0] = VUTIL_jitter(tym);
        }
        break;
    }
    case 8:
        if (!vil->vhf->query(vil, ZVIL_Q_DOFOLLOW))
        {
            VILN_tran(resp, 4, 0);
        }
        break;
    case 6:
        if (vil->vhf->query(vil, ZVIL_Q_PLYRTOOCLOSE))
        {
            VILN_tran(resp, 0x16, 0);
        }
        else
        {
            if (vil->vhf->query(vil, ZVIL_Q_NEEDNAV))
            {
                VNAV_get_nav(vil);
            }
            if (!vil->vhf->query(vil, ZVIL_Q_HAVENAV))
            {
                VILN_tran(resp, 4, 0);
            }
        }
        break;
    case 22:
        VWEP_rbs_flight(&g_rbsword[0], dt);
        if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f)
        {
            if (vil->tmr_list[4] <= 0.0f)
            {
                vil->anim_loopmult = 0.97f;
                vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x16, (en_ZVIL_GOAL_SPOT)9);
                vil->tmr_list[4] = 3.2e7f;
            }
            if (vil->goodattack)
            {
                VILN_tran(resp, 0x17, 0);
            }
            if (vil->tmr_list[ZVIL_TDUR_EXTEND] <= 0.0f)
            {
                VILN_tran(resp, 4, 0);
            }
        }
        break;
    case 23:
        if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f && vil->tmr_list[ZVIL_TDUR_TAUNT] <= 0.0f)
        {
            VILN_tran(resp, 4, 0);
        }
        break;
    case 27:
        if (vil->tmr_list[ZVIL_TYMR_MINANIMPLAY] <= 0.0f && vil->tmr_list[22] <= 0.0f)
        {
            VILN_tran(resp, 4, 0);
        }
        break;
    case 32:
        vil->collis->chk = 0;
        vil->collis->pen = 0;
        vil->chkby = 0;
        vil->penby = 0;
        VILN_holoDeath(vil, dt, resp);
        break;
    default:
        VILN_tran(resp, 4, 0);
        break;
    }
    return 1;
}

static signed int VILN_BRB_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, st_STIMULUS_CONTEXT_DATA* resp)
{
    signed int temp_cr0_eq;
    signed int temp_r3;
    signed int temp_r5;

    temp_r3 = vil->goal;
    temp_r5 = stimctxt->stim;
    g_pgol = (en_BEHAVIOUR_GOAL)temp_r3;
    switch (temp_r3) {                              /* switch 1 */
    case -1:
        break;
    case 0:                                         /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r5;
        switch (temp_r5) {                          /* switch 2; irregular */
        case 5:                                     /* switch 2 */
        case 0:                                     /* switch 2 */
            break;
        case 2:                                     /* switch 2 */
            VILN_tran(resp, 2, 1);
            break;
        }
        break;
    case 1:                                         /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r5;
        switch (temp_r5) {                          /* switch 3; irregular */
        case 5:                                     /* switch 3 */
        case 0:                                     /* switch 3 */
            break;
        case 2:                                     /* switch 3 */
            VILN_tran(resp, 2, 1);
            break;
        }
        break;
    case 2:                                         /* switch 1 */
    case 3:                                         /* switch 1 */
    case 27:                                        /* switch 1 */
    case 32:                                        /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r5;
        break;
    case 4:                                         /* switch 1 */
    case 6:                                         /* switch 1 */
    case 8:                                         /* switch 1 */
    case 22:                                        /* switch 1 */
    case 23:                                        /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r5;
        {
            switch (temp_r5) {                      /* switch 4 */
            case 0:
                break;
            case 34:                                /* switch 4 */
                vil->hitpts = 0;
                VILN_tran(resp, 0x20, 7);
                break;
            case 35:                                /* switch 4 */
                VILN_tran(resp, 0, 1);
                break;
            case 38:                                /* switch 4 */
                vil->hitpts -= 1;
                if (vil->hitpts < 1) {
                    VILN_tran(resp, 0x20, 7);
                } else {
                    VILN_tran(resp, 0x1B, 5);
                }
                break;
            case 32:                                /* switch 4 */
                stimctxt->res_gods = 2;
                break;
            case 33:                                /* switch 4 */
                stimctxt->res_gods = 2;
                break;
            case 27:                                /* switch 4 */
                stimctxt->res_gods = 4;
                break;
            case 28:                                /* switch 4 */
                stimctxt->res_gods = 3;
                break;
            case 29:                                /* switch 4 */
                stimctxt->res_gods = 3;
                break;
            case 23:                                /* switch 4 */
                if ((vil->allow & 8) && (vil->abl_atak & 1)) {
                    VILN_tran(resp, 4, 0);
                }
                break;
            case 25:                                /* switch 4 */
                if (vil->ability & 0x100) {
                    VILN_tran(resp, 0x17, 0);
                }
                break;
            case 8:                                 /* switch 4 */
                VILN_stimGotoPos(vil, stimctxt);
                VILN_tran(resp, 8, 0);
                break;
            case 9:                                 /* switch 4 */
                if ((stimctxt->widge != NULL) && (vil->abl_move & 0x100)) {
                    VOUT_dogoto_widget(vil, stimctxt->widge, stimctxt->off_goto, 0);
                    vil->tmr_list[24] = -1.0f;
                    VILN_tran(resp, 8, 0);
                }
                break;
            case 10:                                /* switch 4 */
                if ((stimctxt->widge != NULL) && (vil->abl_move & 0x100)) {
                    temp_cr0_eq = VOUT_dogoto_widget(vil, stimctxt->widge, stimctxt->off_goto, 1);
                    vil->tmr_list[24] = -1.0f;
                    if (temp_cr0_eq) {
                        VILN_tran(resp, 6, 0);
                    } else {
                        VILN_tran(resp, 8, 0);
                    }
                }
                break;
            case 11:                                /* switch 4 */
                vil->tmr_list[24] = -1.0f;
                if (vil->vhf->query(vil, (en_ZVIL_QUERY)7) != 0) {
                    VNAV_get_nav(vil);
                }
                if (vil->vhf->query(vil, (en_ZVIL_QUERY)8) != 0) {
                    VILN_tran(resp, 6, 0);
                } else {
                    VILN_tran(resp, 4, 0);
                }
                break;
            case 12:                                /* switch 4 */
                VILN_tran(resp, 4, 0);
                break;
            case 13:                                /* switch 4 */
                VILN_tran(resp, 4, 0);
                break;
            case 14:                                /* switch 4 */
                VILN_tran(resp, 4, 0);
                break;
            }
        }
        break;
    }
    return 1;
}

static signed int VILN_BRB_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt)
{
    float temp_f0;
    float temp_f31;
    float temp_f31_2;
    signed int temp_r0;
    signed int temp_r29;
    float var_f1;
    signed int var_r28;
    signed int var_r31;

    temp_r29 = stimctxt->stim;
    var_r31 = 1;
    if ((temp_r29 == 4) || (temp_r29 == 3)) {
        var_r28 = stimctxt->trangoal;
    } else {
        var_r28 = vil->goal;
    }
    switch (var_r28) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 32:
        var_r31 = VILN_std_trangoal(vil, stimctxt, dt);
        break;
    }
    g_pgol = (en_BEHAVIOUR_GOAL)var_r28;
    switch (var_r28) {                              /* switch 1 */
    case -1:
        break;
    case 4:                                         /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r29;
        switch (temp_r29) {                         /* switch 2; irregular */
        case 3:                                     /* switch 2 */
        case 5:                                     /* switch 2 */
        case 0:                                     /* switch 2 */
            break;
        case 4:                                     /* switch 2 */
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)4, (en_ZVIL_GOAL_SPOT)0);
            vil->tgt_stat = 0;
            break;
        }
        break;
    case 8:                                         /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r29;
        switch (temp_r29) {                         /* switch 3; irregular */
        case 3:                                     /* switch 3 */
        case 5:                                     /* switch 3 */
        case 0:                                     /* switch 3 */
            break;
        case 4:                                     /* switch 3 */
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)8, (en_ZVIL_GOAL_SPOT)0);
            break;
        }
        break;
    case 6:                                         /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r29;
        switch (temp_r29) {                         /* switch 4; irregular */
        case 3:                                     /* switch 4 */
        case 5:                                     /* switch 4 */
        case 0:                                     /* switch 4 */
            break;
        case 4:                                     /* switch 4 */
            if (vil->allow & 8) {
                zEntEvent((xBase *) vil, (xBase *) vil, 0xB1U);
            }
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)6, (en_ZVIL_GOAL_SPOT)0);
            vil->tgt_stat = 0;
            if ((unsigned int)vil->nav_next == 0U) {
                vil->need_navpt = 1;
            }
            break;
        }
        break;
    case 22:                                        /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r29;
        switch (temp_r29) {                         /* switch 5; irregular */
        case 5:                                     /* switch 5 */
        case 0:                                     /* switch 5 */
            break;
        case 4:                                     /* switch 5 */
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x16, (en_ZVIL_GOAL_SPOT)0);
            vil->goodattack = 0;
                        vil->tmr_list[18] = 5.0f;
            vil->tmr_list[4] = vil->tmr_list[18];
            temp_f0 = -1.0f + vil->tmr_list[4];
            vil->tmr_list[4] = (temp_f0 < -1.0f) ? -1.0f : temp_f0;
            VWEP_rbs_throw(&g_rbsword[0]);
            break;
        case 3:                                     /* switch 5 */
            vil->goodattack = 0;
            temp_f31 = vil->vilcfg.tymref_list[20];
            vil->tmr_list[20] = VUTIL_jitter(temp_f31);
            if (vil->allow & 8) {
                zEntEvent((xBase *) vil, (xBase *) vil, 0xB1U);
            }
            g_rbsword[0].mdl_animsword->Flags = (unsigned short)(g_rbsword[0].mdl_animsword->Flags | 1);
            g_rbsword[0].model->Flags = (unsigned short)(g_rbsword[0].model->Flags & 0xFFFE);
            g_rbsword[0].flg_stat &= 0xFFFFFFFB;
            xSndStopChildren((unsigned int)&g_rbsword[0]);
            break;
        }
        break;
    case 23:                                        /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r29;
        switch (temp_r29) {                         /* switch 6; irregular */
        case 3:                                     /* switch 6 */
        case 5:                                     /* switch 6 */
        case 0:                                     /* switch 6 */
            break;
        case 4:                                     /* switch 6 */
            vil->anim_loopmult = 0.97f;
            vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x17, (en_ZVIL_GOAL_SPOT)0);
            vil->tmr_list[11] = vil->vilcfg.tymref_list[11];
            break;
        }
        break;
    case 27:                                        /* switch 1 */
        g_pstm = (en_NPC_STIMULUS)temp_r29;
        switch (temp_r29) {                         /* switch 7; irregular */
        case 3:                                     /* switch 7 */
        case 5:                                     /* switch 7 */
        case 0:                                     /* switch 7 */
            break;
        case 4:                                     /* switch 7 */
            temp_f31_2 = vil->vilcfg.tymref_list[22];
            vil->tmr_list[22] = VUTIL_jitter(temp_f31_2);
            temp_r0 = vil->hitpts;
            switch (temp_r0) {                      /* switch 8; irregular */
            case 4:                                 /* switch 8 */
                vil->anim_loopmult = 0.97f;
                vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x1B, (en_ZVIL_GOAL_SPOT)0);
                break;
            case 3:                                 /* switch 8 */
                vil->anim_loopmult = 0.97f;
                vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x1B, (en_ZVIL_GOAL_SPOT)0xF);
                break;
            case 2:                                 /* switch 8 */
                vil->anim_loopmult = 0.97f;
                vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x1B, (en_ZVIL_GOAL_SPOT)0x10);
                break;
            case 1:                                 /* switch 8 */
                vil->anim_loopmult = 0.97f;
                vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x1B, (en_ZVIL_GOAL_SPOT)0x10);
                break;
            default:                                /* switch 8 */
                vil->anim_loopmult = 0.97f;
                vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x1B, (en_ZVIL_GOAL_SPOT)0);
                break;
            }
            break;
        }
        break;
    }
    return var_r31;
}

static signed int VILN_BRB_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame)
{
    // Redbeard fights from a fixed spot in his arena.
    _xVec3 pos_fight = { -0.5f, 0.0f, 0.0f };

    switch (vil->goal)
    {
    case 0:
    case 1:
    case 2:
    case 3:
        break;
    case 4:
        vil->pos_moveto = pos_fight;
        vil->domove |= 0x800;
        vil->domove |= 0x40000000;
        vil->domove |= 0xA;
        vil->ang_moveto = vil->precalc.ang_VtoP;
        vil->domove |= 0x40;
        vil->domove |= 1;
        vil->domove |= 0x80000000;
        break;
    case 22:
    case 23:
        vil->pos_moveto = pos_fight;
        vil->domove |= 0x800;
        vil->domove |= 0x40000000;
        vil->domove |= 0xA;
        vil->ang_moveto = vil->precalc.ang_VtoP;
        vil->domove |= 0x40;
        vil->domove |= 1;
        vil->domove |= 0x80000000;
        break;
    case 8:
        VOUT_follow_move(vil);
        VOUT_chkdo_followArrived(vil);
        break;
    case 6:
        VOUT_patrol_move(vil);
        VOUT_chkdo_patrolArrived(vil);
        break;
    }
    return 1;
}

static signed int VILN_BRB_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp)
{
    return 1;
}

static signed int VILN_BRB_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot)
{

    {
        switch (goal) {                       /* switch 1 */
        case 0:
            break;
        case 1:                                     /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            break;
        case 3:                                     /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            break;
        case 4:                                     /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            } else if (gspot == 2) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x52);
            } else if (gspot == 12) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x54);
            } else if (gspot == 11) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x54);
            }
            break;
        case 8:                                     /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x53);
            break;
        case 6:                                     /* switch 1 */
            if (gspot == 0) {
                if (vil->allow & 0x20) {
                    VANM_play(vil, (en_NPC_ANIMSTATES)0x55, (en_NPC_ANIMSTATES)0x53);
                } else {
                    VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x53);
                }
            }
            break;
        case 23:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playRand3(vil, (en_NPC_ANIMSTATES)0x56, (en_NPC_ANIMSTATES)0x57, (en_NPC_ANIMSTATES)0x58);
            } else if (gspot == 13) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x57);
            } else if (gspot == 14) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x58);
            }
            break;
        case 22:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x5a);
            } else if (gspot == 9) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x5c);
            }
            break;
        case 27:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x5d);
            } else if (gspot == 15) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x5d);
            } else if (gspot == 16) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x5d);
            }
            break;
        case 32:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x61);
            }
            break;
        }
    }
    return 1;
}

static signed int VILN_BRB_xtrarend(st_zEntVillain* vil)
{
    if (g_rbsword[0].flg_stat & 4)
    {
        xModelRender(g_rbsword[0].model);
    }
    return 1;
}

static signed int VILN_BMM_flag(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup, en_BEHAVIOUR_GOAL goal)
{
    return VILN_flagsFromTable(vil, flaggroup);
}

static signed int VILN_BMM_query(st_zEntVillain* vil, en_ZVIL_QUERY query)
{
    switch (query)
    {
    case ZVIL_Q_PLYRTOOCLOSE:
        return vil->precalc.ds2_VtoP < 9.0f;
    case ZVIL_Q_CANATTACK:
        return (vil->allow & 8) ? 1 : 0;
    default:
        return VILN_std_query(vil, query);
    }
}

static signed int VILN_BMM_sysevent(st_zEntVillain* vil, xBase* from, unsigned int toEvent, float* toParam,
                                    xBase* toParamWidget, st_STIMULUS_CONTEXT_DATA* resp)
{
    switch (toEvent)
    {
    case 0xEE:
        if (vil->vilcfg.hitpts > 1)
        {
            if (vil->hitpts > 1)
            {
                resp->stim = (en_NPC_STIMULUS)0x27;
            }
        }
        else
        {
            resp->stim = (en_NPC_STIMULUS)0x27;
        }
        return 1;
    case 0x2E:
    case 0x2F:
    default:
        return VILN_std_sysevent(vil, from, toEvent, toParam, toParamWidget, resp);
    }
}

static signed int VILN_BMM_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp)
{
    xModelInstance* minst = vil->model;
    signed int idx = 0;
    signed int i;

    if (note == 0)
    {
        g_scene_is_a_boss_battle = 4;
        vil->allow |= 8;
        vil->hitpts = (vil->vilcfg.hitpts >= 1) ? vil->vilcfg.hitpts : 1;
        vil->vec_lobSpin.x = 6.2831855f;
        vil->vilcfg.tymref_list[22] = -1.0f;
    }
    if (note == 0)
    {
        for (i = 0; i < 12; i++)
        {
            g_mmtp[i] = zMovePoint_From_xAssetID(xStrHash(g_strz_mmtp[i]));
        }
    }
    if (note == 0)
    {
        while (minst)
        {
            switch (idx)
            {
            case 0:
            case 1:
                minst->Flags |= 1;
                minst->Flags |= 2;
                minst->Flags |= 4;
                break;
            case 2:
            case 3:
                minst->Flags &= 0xFFFE;
                minst->Flags &= 0xFFFD;
                minst->Flags &= 0xFFFB;
                break;
            case 4:
            case 5:
                minst->Flags &= 0xFFFE;
                minst->Flags &= 0xFFFD;
                minst->Flags &= 0xFFFB;
                break;
            case 6:
            case 7:
                minst->Flags &= 0xFFFE;
                minst->Flags &= 0xFFFD;
                minst->Flags &= 0xFFFB;
                break;
            }
            minst = minst->Next;
            idx++;
        }
    }
    return VILN_cmn_notice(vil, note, resp);
}

static signed int VILN_BMM_aware(st_zEntVillain* vil, _xScene* scn, float dt, st_STIMULUS_CONTEXT_DATA* resp)
{
    switch (vil->goal)
    {
    case 0:
    case 1:
        break;
    case 2:
        VILN_tran(resp, 3, 2);
        break;
    case 3:
        VILN_tran(resp, vil->goaldflt, 3);
        break;
    case 4:
        if (vil->allow & 0x10)
        {
            VILN_tran(resp, 0x11, 0);
        }
        else if (!((vil->allow & 4) && VILN_BMM_teleport(vil, resp, 0)))
        {
            if (vil->vhf->query(vil, (en_ZVIL_QUERY)0xA))
            {
                if (vil->vhf->query(vil, (en_ZVIL_QUERY)7))
                {
                    VNAV_get_nav(vil);
                }
                if (vil->vhf->query(vil, (en_ZVIL_QUERY)8))
                {
                    VILN_tran(resp, 6, 0);
                }
            }
        }
        break;
    case 31:
        if (vil->tmr_list[28] <= 0.0f)
        {
            if (vil->allow & 0x10)
            {
                VILN_tran(resp, 0x11, 0);
            }
            else if (!((vil->allow & 4) && VILN_BMM_teleport(vil, resp, 0)))
            {
                VILN_tran(resp, 4, 0);
            }
        }
        break;
    case 8:
        if (vil->allow & 0x10)
        {
            VILN_tran(resp, 0x11, 0);
        }
        else if (!((vil->allow & 4) && VILN_BMM_teleport(vil, resp, 0)))
        {
            if (!vil->vhf->query(vil, (en_ZVIL_QUERY)6))
            {
                VILN_tran(resp, 4, 0);
            }
        }
        break;
    case 6:
        if (vil->allow & 0x10)
        {
            VILN_tran(resp, 0x11, 0);
        }
        else if (!((vil->allow & 4) && VILN_BMM_teleport(vil, resp, 0)))
        {
            if (vil->vhf->query(vil, (en_ZVIL_QUERY)7))
            {
                VNAV_get_nav(vil);
            }
            if (!vil->vhf->query(vil, (en_ZVIL_QUERY)8))
            {
                VILN_tran(resp, 4, 0);
            }
        }
        break;
    case 29:
        if (vil->tmr_list[ZVIL_TYMR_CURGOAL] > 0.2f)
        {
            VEFX_mm_zapper(vil, 1);
        }
        if (vil->allow & 0x10)
        {
            if (vil->tmr_list[ZVIL_TYMR_GENERIC] <= 0.0f && !iSndIsPlaying(0, (unsigned int)vil + 1))
            {
                VILN_tran(resp, 0x11, 0);
            }
        }
        break;
    case 11:
        if (vil->tmr_list[28] <= 0.0f && (vil->allow & 0x10))
        {
            VILN_tran(resp, 0x11, 0);
        }
        break;
    case 23:
        if (vil->tmr_list[28] <= 0.0f)
        {
            if (vil->allow & 0x10)
            {
                VILN_tran(resp, 0x11, 0);
            }
            else
            {
                VILN_tran(resp, 4, 0);
            }
        }
        break;
    case 17:
        if (!(vil->allow & 0x14))
        {
            VILN_tran(resp, 4, 0);
        }
        else if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x12))
        {
            VILN_tran(resp, 0x1C, 0);
        }
        else
        {
            if (vil->tmr_list[ZVIL_TYMR_GENERIC] <= 0.0f)
            {
                vil->anim_loopmult = 0.97f;
                if ((float)__fabs(vil->frame->rot.angle - vil->precalc.ang_VtoP) <= 0.2f)
                {
                    vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x11, (en_ZVIL_GOAL_SPOT)0);
                }
                else
                {
                    vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x11, (en_ZVIL_GOAL_SPOT)0xC);
                }
                vil->tmr_list[ZVIL_TYMR_GENERIC] = 0.2f;
            }
            if (vil->vhf->query(vil, (en_ZVIL_QUERY)0xE) && vil->tmr_list[19] <= 0.0f)
            {
                vil->prj_reserved = VWEP_lobReservePRJ(vil);
                if (vil->prj_reserved)
                {
                    VILN_tran(resp, 0x15, 0);
                }
            }
        }
        break;
    case 21:
        if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x12))
        {
            VILN_tran(resp, 0x1C, 0);
        }
        else
        {
            signed int an = vil->anim_playnext;
            if (an == 0x6C)
            {
            }
            else if (an == 0x6D)
            {
                if (vil->tmr_list[21] <= 0.0f && !iSndIsPlaying(0, (unsigned int)vil + 1))
                {
                    vil->anim_loopmult = 0.97f;
                    vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x15, (en_ZVIL_GOAL_SPOT)4);
                    vil->tmr_list[28] = 3.2e7f;
                }
            }
            else if (an == 0x6E && vil->anim_playing)
            {
                // Throw the flask that is in hand; ready the next one of the salvo.
                if (VANM_inDmgWindow(vil) && vil->prj_reserved && vil->cnt_lobSalvo > 0)
                {
                    _xVec3 off = {};
                    _xVec3* ctr;
                    st_ZVMM_FLASK_DATA* flask;

                    if (!(vil->cnt_lobSalvo & 1))
                    {
                        off.x = vil->precalc.vec_VtoP.x * 0.6f;
                        off.y = 0.0f;
                        off.z = vil->precalc.vec_VtoP.z * 0.6f;
                    }
                    ctr = xBoundCenter(&globals.player.ent.bound);
                    flask = (st_ZVMM_FLASK_DATA*)vil->prj_reserved->xtradata;
                    flask->pos_tgt.x = off.x + ctr->x;
                    flask->pos_tgt.y = off.y + ctr->y;
                    flask->pos_tgt.z = off.z + ctr->z;
                    flask->stage = MAST_STAGE_TOSS;
                    flask->flg_stat |= 2;
                    vil->prj_reserved = NULL;
                    vil->cnt_lobSalvo--;
                    if (vil->cnt_lobSalvo > 0 && !vil->prj_reserved)
                    {
                        vil->prj_reserved = VWEP_lobReservePRJ(vil);
                        if (vil->prj_reserved)
                        {
                            VWEP_lobActivatePRJ(vil->prj_reserved, vil, xBoundCenter(&globals.player.ent.bound), NULL,
                                                &vil->vec_lobSpin, vil->fac_lobarc, vil->fac_lobheavy);
                        }
                    }
                }
            }
            else if (vil->tmr_list[28] <= 0.0f)
            {
                VILN_BMM_teleport(vil, resp, 0);
            }
        }
        break;
    case 24:
        VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)8, dt);
        if (vil->tmr_list[28] <= 0.0f && !iSndIsPlaying(0, (unsigned int)vil + 1))
        {
            if (vil->tmr_list[4] <= 0.0f)
            {
                vil->anim_loopmult = 0.97f;
                vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x18, (en_ZVIL_GOAL_SPOT)9);
                vil->tmr_list[4] = 3.2e7f;
            }
            else if (vil->tmr_list[12] <= 0.0f)
            {
                VILN_tran(resp, 0x11, 0);
            }
        }
        break;
    case 28:
        if (iSndIsPlaying(0, (unsigned int)vil + 1))
        {
            vil->tmr_list[ZVIL_TYMR_GENERIC] = 3.0f * (0.25f * (iurand() - 0.5f)) + 3.0f;
        }
        else if (vil->tmr_list[28] <= 0.0f)
        {
            if (vil->tmr_list[ZVIL_TYMR_GENERIC] <= 0.0f && VILN_BMM_teleport(vil, resp, 1))
            {
                break;
            }
            if (vil->vhf->query(vil, (en_ZVIL_QUERY)0x12))
            {
                vil->anim_loopmult = 0.97f;
                vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x1C, (en_ZVIL_GOAL_SPOT)1);
            }
            else
            {
                signed int an = vil->anim_playnext;
                if (an == 0x70)
                {
                    vil->anim_loopmult = 0.97f;
                    vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x1C, (en_ZVIL_GOAL_SPOT)9);
                    vil->tmr_list[28] = 3.2e7f;
                }
                else if (an == 0x71 || an == 0x66)
                {
                    VILN_tran(resp, 0x11, 0);
                }
            }
        }
        break;
    case 30:
        if (vil->tmr_list[28] <= 0.0f)
        {
            signed int an = vil->anim_playnext;
            if (an == 0x75)
            {
            }
            else if (an == 0x76 && vil->anim_playing)
            {
                if (vil->tmr_list[ZVIL_TYMR_GENERIC] <= 0.0f)
                {
                    vil->anim_loopmult = 0.97f;
                    vil->vhf->pickanim(vil, (en_BEHAVIOUR_GOAL)0x1E, (en_ZVIL_GOAL_SPOT)9);
                    VSND_cue(vil, (en_NPC_SOUND)0x3E);
                    vil->tmr_list[28] = 3.2e7f;
                    vil->pflags |= 4;
                }
            }
            else if (an == 0x77 && vil->anim_playing)
            {
                if (!iSndIsPlaying(0, (unsigned int)vil + 1) && !VILN_BMM_teleport(vil, resp, 0))
                {
                    VILN_tran(resp, 0x11, 0);
                }
            }
            else
            {
                VILN_tran(resp, 0x11, 0);
            }
        }
        break;
    case 27:
        if (vil->tmr_list[28] <= 0.0f && vil->tmr_list[22] <= 0.0f)
        {
            if (vil->hitpts < 1)
            {
                VILN_tran(resp, 0x20, 7);
            }
            else
            {
                VILN_tran(resp, 0x11, 0);
            }
        }
        break;
    case 32:
        if (vil->tmr_list[28] <= 0.0f)
        {
            zEntEvent(vil, vil, 0xEF);
            vil->tmr_list[28] = 3.2e7f;
        }
        break;
    default:
        VILN_tran(resp, 4, 0);
        break;
    }
    return 1;
}

// Pick another teleport point and request the teleport goal. Returns 1 if one was found.
static inline signed int VILN_BMM_teleport(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp, signed int hurt)
{
    signed int found = 0;
    _zMovePoint* mvpt;
    float tym;

    g_mm_telepos = ((_xMat4x3*)vil->model->Mat)->pos;
    if (vil->nav_next && vil->nav_next != vil->nav_prev)
    {
        vil->nav_prev = vil->nav_next;
    }
    mvpt = VNAV_find_telepoint(vil, 1);
    if (mvpt && mvpt != vil->nav_next)
    {
        vil->nav_next = mvpt;
        found = 1;
        vil->need_navpt = 0;
        tym = vil->vilcfg.tymref_list[3];
        vil->tmr_list[3] = VUTIL_jitter(tym);
    }
    else
    {
        vil->need_navpt = 1;
        tym = vil->vilcfg.tymref_list[3];
        vil->tmr_list[3] = VUTIL_jitter(tym);
    }
    if (found)
    {
        VILN_tran(resp, 0x1D, 0);
        resp->mm_teleport_hurtz_doggie = hurt;
    }
    return found;
}

static signed int VILN_BMM_stimulus(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt,
                                    st_STIMULUS_CONTEXT_DATA* resp)
{
    en_NPC_STIMULUS stim = stimctxt->stim;

    g_pgol = vil->goal;
    switch (vil->goal)
    {
    case 0:
    case 1:
        g_pstm = stim;
        if (stim == ZVIL_STIM_MAKEREADY)
        {
            VILN_tran(resp, 2, 1);
        }
        break;
    case 2:
    case 3:
    case 11:
    case 24:
    case 27:
    case 29:
    case 32:
        g_pstm = stim;
        break;
    case 4:
    case 6:
    case 8:
    case 21:
    case 23:
    case 28:
    case 30:
    case 31:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_KILLSELF:
            vil->hitpts = 0;
            VILN_tran(resp, 0x20, 7);
            break;
        case ZVIL_STIM_GOTOAFTERLIFE:
            VILN_tran(resp, 0, 1);
            break;
        case ZVIL_STIM_MMPUSHBUTTON:
            VILN_tran(resp, 0x1F, 0);
            break;
        case ZVIL_STIM_HURTBOSS:
            VILN_tran(resp, 0x1B, 0);
            break;
        case ZVIL_STIM_SOAPED:
        case ZVIL_STIM_GUMMED:
            stimctxt->res_gods = 2;
            VSND_cue(vil, (en_NPC_SOUND)0x17);
            break;
        case ZVIL_STIM_HEADBUTT:
            stimctxt->res_gods = 4;
            break;
        case ZVIL_STIM_BOUNCED:
            stimctxt->res_gods = 1;
            VILN_tran(resp, 0x18, 5);
            break;
        case ZVIL_STIM_BUMPED:
            stimctxt->res_gods = 3;
            break;
        case ZVIL_STIM_ATTACKHERO:
            if ((vil->allow & 8) && (vil->abl_atak & 1))
            {
                vil->prj_reserved = VWEP_lobReservePRJ(vil);
                if (vil->prj_reserved)
                {
                    VILN_tran(resp, 0x15, 5);
                }
            }
            break;
        case ZVIL_STIM_DOTAUNT:
            if (vil->ability & 0x100)
            {
                VILN_tran(resp, 0x17, 0);
            }
            break;
        case ZVIL_STIM_GOTOPOS:
            VILN_stimGotoPos(vil, stimctxt);
            VILN_tran(resp, 8, 0);
            break;
        case ZVIL_STIM_GOTOWIDGET:
            if (stimctxt->widge && (vil->abl_move & 0x100))
            {
                VOUT_dogoto_widget(vil, stimctxt->widge, stimctxt->off_goto, 0);
                vil->tmr_list[ZVIL_TDUR_NAVDELAY] = -1.0f;
                VILN_tran(resp, 8, 0);
            }
            break;
        case ZVIL_STIM_FOLLOWWIDGET:
            if (stimctxt->widge && (vil->abl_move & 0x100))
            {
                signed int follow = VOUT_dogoto_widget(vil, stimctxt->widge, stimctxt->off_goto, 1);
                vil->tmr_list[ZVIL_TDUR_NAVDELAY] = -1.0f;
                VILN_tran(resp, follow ? 6 : 8, 0);
            }
            break;
        case ZVIL_STIM_STARTPATROL:
            vil->tmr_list[ZVIL_TDUR_NAVDELAY] = -1.0f;
            if (vil->vhf->query(vil, ZVIL_Q_NEEDNAV))
            {
                VNAV_get_nav(vil);
            }
            VILN_tran(resp, vil->vhf->query(vil, ZVIL_Q_HAVENAV) ? 6 : 4, 0);
            break;
        case ZVIL_STIM_STOPPATROL:
        case ZVIL_STIM_STARTGUARD:
        case ZVIL_STIM_STOPGUARD:
            VILN_tran(resp, 4, 0);
            break;
        }
        break;
    }

    g_pgol = vil->goal;
    switch (vil->goal)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 6:
    case 8:
    case 21:
    case 23:
    case 28:
    case 29:
    case 30:
    case 31:
        g_pstm = stim;
        break;
    case 24:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_KILLSELF:
            vil->hitpts = 0;
            VILN_tran(resp, 0x20, 7);
            break;
        case ZVIL_STIM_GOTOAFTERLIFE:
            VILN_tran(resp, 0, 1);
            break;
        case ZVIL_STIM_HURTBOSS:
            VILN_tran(resp, 0x1B, 0);
            break;
        case ZVIL_STIM_SOAPED:
        case ZVIL_STIM_GUMMED:
            stimctxt->res_gods = 2;
            break;
        case ZVIL_STIM_HEADBUTT:
            stimctxt->res_gods = 1;
            VILN_tran(resp, 0x1E, 0);
            break;
        case ZVIL_STIM_BOUNCED:
            stimctxt->res_gods = 0;
            break;
        case ZVIL_STIM_BUMPED:
            stimctxt->res_gods = 3;
            break;
        case ZVIL_STIM_DOTAUNT:
            if (vil->ability & 0x100)
            {
                VILN_tran(resp, 0x17, 0);
            }
            break;
        }
        break;
    }
    return 1;
}

static signed int VILN_BMM_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt)
{
    en_BEHAVIOUR_GOAL goal;
    signed int rc = 1;
    en_NPC_STIMULUS stim;
    stim = stimctxt->stim;

    if (stim == ZVIL_STIM_ENTERGOAL || stim == ZVIL_STIM_EXITGOAL)
    {
        goal = stimctxt->trangoal;
    }
    else
    {
        goal = vil->goal;
    }
    switch (goal)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 0x20:
        rc = VILN_std_trangoal(vil, stimctxt, dt);
        if (goal == 0x20 && stim == ZVIL_STIM_ENTERGOAL)
        {
            VSND_cue(vil, (en_NPC_SOUND)0x41);
        }
        break;
    }

    g_pgol = goal;
    switch (goal)
    {
    case ZVIL_GOAL_NONE:
        break;
    case ZVIL_GOAL_IDLE:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_BMM_enterGoal(vil, (en_BEHAVIOUR_GOAL)ZVIL_GOAL_IDLE, 0);
            vil->tgt_stat = 0;
            break;
        }
        break;
    case ZVIL_GOAL_ACTION:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_BMM_enterGoal(vil, (en_BEHAVIOUR_GOAL)ZVIL_GOAL_ACTION, 0);
            break;
        }
        break;
    case ZVIL_GOAL_FOLLOW:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_BMM_enterGoal(vil, (en_BEHAVIOUR_GOAL)ZVIL_GOAL_FOLLOW, 0);
            break;
        }
        break;
    case ZVIL_GOAL_PATROL:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_BMM_enterGoal(vil, (en_BEHAVIOUR_GOAL)ZVIL_GOAL_PATROL, 0);
            vil->tgt_stat = 0;
            if (!vil->nav_next)
            {
                vil->need_navpt = 1;
            }
            break;
        }
        break;
    case ZVIL_GOAL_LEAP:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_BMM_enterGoal(vil, (en_BEHAVIOUR_GOAL)ZVIL_GOAL_LEAP, 0);
            break;
        }
        break;
    case ZVIL_GOAL_TELEPORT:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_BMM_enterGoal(vil, (en_BEHAVIOUR_GOAL)ZVIL_GOAL_TELEPORT, 0);
            vil->flags &= ~1;
            vil->chkby &= 0xEF;
            vil->penby &= 0xEF;
            vil->collis->chk &= 0xEF;
            vil->collis->pen &= 0xEF;
            if (stimctxt->mm_teleport_hurtz_doggie > 0)
            {
                VEFX_mm_zapper(vil, 0);
                VIRM_initVilDflt(vil);
                VSND_cue(vil, (en_NPC_SOUND)0x39);
            }
            else if (vil->allow & 0x10)
            {
                VSND_cue(vil, (en_NPC_SOUND)0x3A);
            }
            if (vil->allow & 0x10)
            {
                _zParEmitterCustomSettings pecust;

                vil->tmr_list[ZVIL_TYMR_GENERIC] = 0.75f * (0.25f * (iurand() - 0.5f)) + 0.75f;
                VSND_cue(vil, (en_NPC_SOUND)0x3F);
                VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)0xD, dt);
                pecust.custom_flags = 0x20;
                pecust.pos = g_mm_telepos;
                zParEmitterEmitCustom(sEmitMMTeleportHide, dt, &pecust);
            }
            else
            {
                vil->tmr_list[ZVIL_TYMR_GENERIC] = 0.5f;
            }
            VILN_BMM_tymrRand(vil, 19);
            break;
        case ZVIL_STIM_EXITGOAL:
            _zParEmitterCustomSettings pecust;
            _xMat4x3* mat = (_xMat4x3*)vil->model->Mat;

            pecust.custom_flags = 0x20;
            pecust.pos = mat->pos;
            pecust.pos.y += 1.0f;
            zParEmitterEmitCustom(sEmitMMTeleportShow, dt, &pecust);
            VEFX_mm_zapper(vil, 1);
            VPAR_emit_partype(vil, (en_VPAR_EMITTER_ROLE)0xD, dt);
            VSND_cue(vil, (en_NPC_SOUND)0x40);
            vil->flags |= 1;
            vil->chkby |= 0x10;
            vil->penby |= 0x10;
            vil->collis->chk |= 0x10;
            vil->collis->pen |= 0x10;
            break;
        }
        break;
    case ZVIL_GOAL_ATTACKREADY:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_BMM_enterGoal(vil, (en_BEHAVIOUR_GOAL)ZVIL_GOAL_ATTACKREADY, 0);
            vil->tmr_list[ZVIL_TYMR_GENERIC] = 0.2f;
            break;
        }
        break;
    case ZVIL_GOAL_BLOCKHEAD:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_BMM_enterGoal(vil, (en_BEHAVIOUR_GOAL)ZVIL_GOAL_BLOCKHEAD, 0);
            VSND_cue(vil, (en_NPC_SOUND)0x38);
            vil->tmr_list[ZVIL_TYMR_GENERIC] = 3.0f * (0.25f * (iurand() - 0.5f)) + 3.0f;
            break;
        }
        break;
    case ZVIL_GOAL_DAZED:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            float t;

            VILN_BMM_enterGoal(vil, (en_BEHAVIOUR_GOAL)ZVIL_GOAL_DAZED, 0);
            VILN_BMM_tymrRand(vil, 12);
            vil->tmr_list[4] = vil->tmr_list[12];
            t = -1.0f + vil->tmr_list[4];
            vil->tmr_list[4] = (t < -1.0f) ? -1.0f : t;
            vil->abl_vuln |= 0x80;
            break;
        case ZVIL_STIM_EXITGOAL:
            vil->abl_vuln &= ~0x80;
            break;
        }
        break;
    case ZVIL_GOAL_KNOCKBACK:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_BMM_enterGoal(vil, (en_BEHAVIOUR_GOAL)ZVIL_GOAL_KNOCKBACK, 0);
            vil->dst_navTween = 7.5f;
            vil->tmr_list[ZVIL_TYMR_GENERIC] = 1.5f;
            vil->pos_bowling = ((_xMat4x3*)globals.player.ent.model->Mat)->pos;
            vil->pflags &= 0xFB;
            break;
        case ZVIL_STIM_EXITGOAL:
            vil->frame->dvel.x = 0.0f;
            vil->frame->dvel.y = 0.0f;
            vil->frame->dvel.z = 0.0f;
            vil->frame->vel.x = 0.0f;
            vil->frame->vel.y = 0.0f;
            vil->frame->vel.z = 0.0f;
            vil->frame->mode |= 0xC;
            vil->pflags |= 4;
            break;
        }
        break;
    case ZVIL_GOAL_LOBBING:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_BMM_enterGoal(vil, (en_BEHAVIOUR_GOAL)ZVIL_GOAL_LOBBING, 0);
            VILN_BMM_tymrRand(vil, 21);
            VWEP_lobActivatePRJ(vil->prj_reserved, vil, xBoundCenter(&globals.player.ent.bound), NULL,
                                &vil->vec_lobSpin, vil->fac_lobarc, vil->fac_lobheavy);
            VSND_cue(vil, (en_NPC_SOUND)0x3D);
            vil->cnt_lobSalvo = 1;
            vil->goodattack = 0;
            break;
        case ZVIL_STIM_EXITGOAL:
            vil->goodattack = 0;
            VILN_BMM_tymrRand(vil, 19);
            if (vil->prj_reserved)
            {
                vil->prj_reserved->Abort();
            }
            vil->prj_reserved = NULL;
            break;
        }
        break;
    case ZVIL_GOAL_TAUNT:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_BMM_enterGoal(vil, (en_BEHAVIOUR_GOAL)ZVIL_GOAL_TAUNT, 0);
            vil->tmr_list[11] = vil->vilcfg.tymref_list[11];
            VSND_cue(vil, (en_NPC_SOUND)0x42);
            break;
        }
        break;
    case ZVIL_GOAL_HURT:
        g_pstm = stim;
        switch (stim)
        {
        case ZVIL_STIM_EXITGOAL:
        case ZVIL_STIM_NONE:
        case ZVIL_STIM_TRANGOAL:
            break;
        case ZVIL_STIM_ENTERGOAL:
            VILN_BMM_tymrRand(vil, 22);
            if (vil->hitpts > 1)
            {
                VILN_BMM_enterGoal(vil, (en_BEHAVIOUR_GOAL)ZVIL_GOAL_HURT, 0xF);
                VSND_cue(vil, (en_NPC_SOUND)0x3B);
            }
            else
            {
                VILN_BMM_enterGoal(vil, (en_BEHAVIOUR_GOAL)ZVIL_GOAL_HURT, 0);
            }
            vil->hitpts--;
            break;
        }
        break;
    }
    return rc;
}

static inline void VILN_BMM_tymrRand(st_zEntVillain* vil, signed int idx)
{
    float tym = vil->vilcfg.tymref_list[idx];
    vil->tmr_list[idx] = VUTIL_jitter(tym);
}

static inline void VILN_BMM_enterGoal(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, signed int spot)
{
    vil->anim_loopmult = 0.97f;
    vil->vhf->pickanim(vil, goal, (en_ZVIL_GOAL_SPOT)spot);
}

static signed int VILN_BMM_calcmove(st_zEntVillain* vil, _xScene* scn, float dt, _xEntFrame* frame)
{
    switch (vil->goal)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 31:
    case 32:
        break;
    case 4:
    case 23:
        if (!(vil->allow & 0x10))
        {
            break;
        }
        // fall through
    case 17:
    case 21:
    case 24:
        vil->ang_moveto = vil->precalc.ang_VtoP;
        vil->domove |= 0x40;
        vil->domove |= 1;
        vil->domove |= 0x10;
        vil->domove |= 0x80000000;
        break;
    case 8:
        VOUT_follow_move(vil);
        VOUT_chkdo_followArrived(vil);
        break;
    case 6:
        VOUT_patrol_move(vil);
        VOUT_chkdo_patrolArrived(vil);
        break;
    case 11:
        vil->pos_moveto = *zMovePointGetPos(vil->nav_next);
        vil->domove |= 0x800;
        vil->domove |= 0xA;
        vil->domove |= 4;
        vil->domove |= 0x80000000;
        break;
    case 29:
        // Teleporting: fly straight to the chosen point.
        vil->pos_moveto = *zMovePointGetPos(vil->nav_next);
        vil->ang_moveto = vil->precalc.ang_VtoP;
        vil->domove |= 0x800;
        vil->domove |= 0xA;
        vil->domove |= 4;
        vil->domove |= 0x40;
        vil->domove |= 0x40000000;
        vil->domove |= 0x20000000;
        vil->domove |= 0x80000000;
        break;
    case 28:
        vil->ang_moveto = vil->precalc.ang_VtoP;
        vil->domove |= 0x40;
        vil->domove |= 1;
        vil->domove |= 0x10;
        vil->domove |= 0x20000000;
        vil->domove |= 0x80000000;
        break;
    case 30:
        if (vil->anim_playnext == 0x75 || vil->anim_playnext == 0x76)
        {
            // Knocked back: bowl along towards the player's position with a hop.
            float t;

            vil->pos_moveto = vil->pos_bowling;
            vil->spd_moveto = 5.0f;
            t = vil->tmr_list[ZVIL_TYMR_CURGOAL] / 1.5f;
            if (!(t < 1.0f))
            {
                t = 1.0f;
            }
            vil->pos_moveto.y += 0.35f * dt * sin((1.0f - t) * 6.2831855f);
            vil->domove |= 0x8000;
            vil->domove |= 4;
            vil->domove |= 0x800;
            vil->domove |= 0xA;
            vil->domove |= 0x20;
            vil->domove |= 0x80000000;
        }
        else
        {
            vil->frame->dvel.x = 0.0f;
            vil->frame->dvel.y = 0.0f;
            vil->frame->dvel.z = 0.0f;
            vil->frame->vel.x = 0.0f;
            vil->frame->vel.y = 0.0f;
            vil->frame->vel.z = 0.0f;
            vil->frame->mode |= 0xC;
        }
        break;
    }
    return 1;
}

static signed int VILN_BMM_respond(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* resp)
{
    // Sums the depenetration of everything the Mastermind touched; the total
    // is never used.
    _xVec3 sum = { 0.0f, 0.0f, 0.0f };
    signed int i;

    for (i = vil->collis->npc_sidx; i < vil->collis->npc_eidx; i++)
    {
        _xCollis* colrec = &vil->collis->colls[i];
        if (colrec->optr)
        {
            sum.x += colrec->depen.x;
            sum.y += colrec->depen.y;
            sum.z += colrec->depen.z;
        }
    }
    for (i = vil->collis->env_sidx; i < vil->collis->env_eidx; i++)
    {
        _xCollis* colrec = &vil->collis->colls[i];
        if (colrec->optr)
        {
            sum.x += colrec->depen.x;
            sum.y += colrec->depen.y;
            sum.z += colrec->depen.z;
        }
    }
    for (i = vil->collis->stat_sidx; i < vil->collis->stat_eidx; i++)
    {
        _xCollis* colrec = &vil->collis->colls[i];
        if (colrec->optr)
        {
            sum.x += colrec->depen.x;
            sum.y += colrec->depen.y;
            sum.z += colrec->depen.z;
        }
    }
    return 1;
}

static signed int VILN_BMM_pickanim(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal, en_ZVIL_GOAL_SPOT gspot)
{

    signed int alt;
    alt = (vil->allow & 4) ? 1 : 0;
    alt |= (vil->allow & 0x10) ? 1 : 0;
    {
        switch (goal) {                       /* switch 1 */
        case 0:                                     /* switch 1 */
        case 1:                                     /* switch 1 */
        case 2:                                     /* switch 1 */
        case 3:                                     /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            break;
        case 4:                                     /* switch 1 */
            if (alt != 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x66);
            } else {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            }
            break;
        case 31:                                    /* switch 1 */
            if (alt != 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x6a);
            } else {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x64);
            }
            break;
        case 8:                                     /* switch 1 */
            if (alt != 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x67);
            } else {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x62);
            }
            break;
        case 6:                                     /* switch 1 */
            if (alt != 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x67);
            } else {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x62);
            }
            break;
        case 11:                                    /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x65);
            break;
        case 29:                                    /* switch 1 */
            if (alt != 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x66);
            } else {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x66);
            }
            break;
        case 23:                                    /* switch 1 */
            if (alt != 0) {
                VANM_playRand3(vil, (en_NPC_ANIMSTATES)0x6a, (en_NPC_ANIMSTATES)0x63, (en_NPC_ANIMSTATES)0x6b);
            } else {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x63);
            }
            break;
        case 17:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x66);
            } else if (gspot == 0xC) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x68);
            }
            break;
        case 28:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x6f);
            } else if (gspot == 1) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x70);
            } else if (gspot == 9) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x71);
            }
            break;
        case 24:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x72);
            } else if (gspot == 1) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x73);
            } else if (gspot == 9) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x74);
            }
            break;
        case 21:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x6c);
            } else if (gspot == 3) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x6d);
            } else if (gspot == 4) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x6e);
            }
            break;
        case 30:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x75);
            } else if (gspot == 1) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x76);
            } else if (gspot == 9) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x77);
            }
            break;
        case 27:                                    /* switch 1 */
            if (gspot == 0) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x78);
            } else if (gspot == 0xF) {
                VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x1);
            }
            break;
        case 32:                                    /* switch 1 */
            VANM_playLoop(vil, (en_NPC_ANIMSTATES)0x79);
            break;
        }
    }
    return 1;
}

static inline void VILN_restart(st_zEntVillain* vil)
{
    unsigned int mvpt;

    vil->nav_mvptlist = NULL;
    vil->nav_prev = NULL;
    vil->nav_next = NULL;
    mvpt = vil->vilcfg.vilp2ass->movePointAssetID;
    vil->nav_mvptlist = VNAV_mvptFromID(mvpt);
    zVIL_apply_vilcfg(vil, &vil->vilcfg);

    if (vil->ability & 2)
    {
        vil->allow |= 0x800;
    }
    if (vil->ability & 2)
    {
        if (vil->asset->subtype == 0x17)
        {
            vil->allow &= ~0x800;
        }
        else
        {
            st_NPC_NOBONUS* nb;
            for (nb = g_nobonus; nb->sceneID; nb++)
            {
                if (nb->sceneID == globals.sceneCur->sceneID && (nb->aid_vil == 0 || nb->aid_vil == vil->id))
                {
                    vil->allow &= ~0x800;
                    break;
                }
            }
        }
    }

    xAnimPlaySetState(vil->model->Anim->Single, vil->model->Anim->Table->StateList, 0.0f);
    vil->anim_playing = 1;
    vil->anim_playsync = 0;
    VANM_playLoop(vil, (en_NPC_ANIMSTATES)1);
    vil->vhf->notice(vil, (en_ZVIL_NOTICE)0, NULL);
}

// Animation list helpers. Each takes the states as parameters, so the list is
// built from a template and then overwritten, as in the original.

static inline _zMovePoint* VNAV_mvptFromID(unsigned int aid)
{
    _zMovePoint* mvpt = NULL;
    if (aid)
    {
        mvpt = zMovePoint_From_xAssetID(aid);
    }
    return mvpt;
}

static inline void VILN_stimGotoPos(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt)
{
    float off = stimctxt->off_goto;

    vil->allow |= 1;
    vil->need_navpt = 0;
    vil->flg_misc &= ~0x20000000;
    vil->flg_misc &= ~0x40000000;
    vil->flg_misc &= ~0x80000000;
    vil->flg_misc |= 0x10000000;
    vil->ds2_goto_keepaway = off * off;
    vil->pos_goto = stimctxt->gotopos;
}

static inline void VILN_tran(st_STIMULUS_CONTEXT_DATA* resp, signed int goal, signed int mode)
{
    resp->trangoal = (en_BEHAVIOUR_GOAL)goal;
    resp->tranmode = (en_NPC_MODE)mode;
    resp->stim = ZVIL_STIM_TRANGOAL;
}

static inline signed int VILN_cmn_notice(st_zEntVillain* vil, en_ZVIL_NOTICE note, st_STIMULUS_CONTEXT_DATA* resp)
{
    signed int i;

    switch (note)
    {
    case 0:
        for (i = 0; i < 12; i++)
        {
            _xVec3* v = &vil->vilcfg.pos_verts[i];
            if (v->x * v->x + v->y * v->y + v->z * v->z)
            {
                iModelTagSetup(&vil->tag_vert[i], vil->model->Data, v->x, v->y, v->z);
                vil->flg_vert |= 1 << i;
                xSceneID2Name(globals.sceneCur, vil->id);
            }
            else
            {
                xSceneID2Name(globals.sceneCur, vil->id);
            }
        }
        break;
    case 1:
        if (vil->mode >= ZVIL_MODE_NORMAL)
        {
            zVIL_precalc(vil, 0xF);
        }
        break;
    case 2:
    case 3:
        break;
    }
    return 1;
}

static inline signed int VILN_flagsFromTable(st_zEntVillain* vil, en_ZVIL_FLAG_GROUPS flaggroup)
{
    signed int flg = 0;

    switch (flaggroup)
    {
    case ZVIL_FGRP_PHYS:
        flg = g_npcTable[vil->asset->subtype].flg_physics;
        break;
    case ZVIL_FGRP_CHK:
        flg = g_npcTable[vil->asset->subtype].flg_chk;
        break;
    case ZVIL_FGRP_PEN:
        flg = g_npcTable[vil->asset->subtype].flg_pen;
        break;
    case ZVIL_FGRP_CHKBY:
        flg = g_npcTable[vil->asset->subtype].flg_chkby;
        break;
    case ZVIL_FGRP_PENBY:
        flg = g_npcTable[vil->asset->subtype].flg_penby;
        break;
    case ZVIL_FGRP_ABLE:
        flg = g_npcTable[vil->asset->subtype].ability;
        break;
    case ZVIL_FGRP_ATAK:
        flg = g_npcTable[vil->asset->subtype].abl_atak;
        break;
    case ZVIL_FGRP_DTCT:
        flg = g_npcTable[vil->asset->subtype].abl_dtct;
        break;
    case ZVIL_FGRP_MOVE:
        flg = g_npcTable[vil->asset->subtype].abl_move;
        break;
    case ZVIL_FGRP_SPYK:
        flg = g_npcTable[vil->asset->subtype].abl_spyk;
        break;
    case ZVIL_FGRP_VULN:
        flg = g_npcTable[vil->asset->subtype].abl_vuln;
        break;
    case (en_ZVIL_FLAG_GROUPS)11:
        break;
    }
    return flg;
}

static signed int zVIL_elcb_event(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                                  xBase* toParamWidget)
{
    st_zEntVillain* vil = (st_zEntVillain*)to;
    st_STIMULUS_CONTEXT_DATA stimctxt = {};

    vil->vhf->sysevent(vil, from, toEvent, toParam, toParamWidget, &stimctxt);
    if (stimctxt.stim)
    {
        zVIL_input_stim(vil, &stimctxt);
    }
    return 1;
}

static void zVIL_elcb_update(_xEnt* xe, _xScene* scn, float dt)
{
    st_zEntVillain* vil = (st_zEntVillain*)xe;
    st_STIMULUS_CONTEXT_DATA ctx = {};

    zVIL_upd_timers(vil, dt);
    if (vil->anim_playing && !zEntIsShaggy(vil) && vil->subType != 0x11)
    {
        xAnimSingle* single = VANM_single(vil);
        xAnimState* state = single ? single->State : NULL;

        if (state->ID != g_anamhash[vil->anim_playnext])
        {
            vil->anim_playing = 0;
            vil->model->Flags |= 0x100;
        }
        else
        {
            vil->model->Flags &= ~0x100;
        }
    }
    else
    {
        vil->model->Flags |= 0x100;
    }

    vil->vhf->query(vil, ZVIL_Q_DOUPDATE);

    VUTIL_resetctx(&ctx);
    vil->vhf->notice(vil, (en_ZVIL_NOTICE)1, &ctx);
    if (ctx.stim)
    {
        zVIL_trangoal(vil, &ctx, dt);
    }

    VUTIL_resetctx(&ctx);
    vil->vhf->aware(vil, scn, dt, &ctx);
    if (ctx.stim)
    {
        zVIL_trangoal(vil, &ctx, dt);
    }

    xEntBeginUpdate(vil, scn, dt);
    if (vil->pflags & 2)
    {
        xEntApplyPhysics(vil, scn, dt);
    }
    if (vil->pflags & 1)
    {
        xEntMove(vil, scn, dt);
    }
    xFFXApply(vil, scn, dt);
    if (vil->collis)
    {
        xEntCollide(vil, scn, dt);
    }
    xEntEndUpdate(vil, scn, dt);

    VUTIL_resetctx(&ctx);
    vil->vhf->respond(vil, &ctx);
    if (ctx.stim)
    {
        zVIL_trangoal(vil, &ctx, dt);
    }

    vil->precalc.flg_status = 0x40000000;
    VCFG_getParm(&vil->vilcfg, ZVIL_PARM_MDLSCL_BASE, &vil->model->Scale);

    VUTIL_resetctx(&ctx);
    vil->vhf->notice(vil, (en_ZVIL_NOTICE)2, &ctx);
    vil->precalc.flg_status = 0x20000000;
    VSND_procQueue(vil, dt);
}

static inline void VUTIL_resetctx(st_STIMULUS_CONTEXT_DATA* ctx)
{
    memset(ctx, 0, sizeof(st_STIMULUS_CONTEXT_DATA));
    ctx->stim = (en_NPC_STIMULUS)0;
}

static void zVIL_elcb_bupdate(_xEnt* xe, _xVec3* pos_bound)
{
    st_zEntVillain* vil = (st_zEntVillain*)xe;
    _xVec3 vec_off = {};

    vec_off = *(&g_npcTable[vil->asset->subtype].vec_bndoff + 0);
    xMat3x3RMulVec(&vec_off, (_xMat3x3*)vil->model->Mat, &vec_off);
    xVec3Add(&vil->bound.sph.center, &vec_off, pos_bound);
    vil->bound.sph.r = g_npcTable[vil->asset->subtype].rad_bound;
    xBoundUpdate(&vil->bound);

    vil->bound2.sph.r = g_npcTable[vil->asset->subtype].rad_bound2;
    if (vil->bound2.sph.r > 0.0f)
    {
        vec_off = *(&g_npcTable[vil->asset->subtype].vec_bnd2off + 0);
        xMat3x3RMulVec(&vec_off, (_xMat3x3*)vil->model->Mat, &vec_off);
        xVec3Add(&vil->bound2.sph.center, &vec_off, pos_bound);
    }
}

static void zVIL_elcb_move(_xEnt* xe, _xScene* scn, float dt, _xEntFrame* frame)
{
    st_zEntVillain* vil = (st_zEntVillain*)xe;

    vil->vhf->calcmove(vil, scn, dt, frame);
    if (vil->domove)
    {
        zVIL_apply_move(vil, scn, dt, frame);
    }
}

static inline void VUTIL_stimulate(st_zEntVillain* vil, en_NPC_STIMULUS stim)
{
    st_STIMULUS_CONTEXT_DATA stimctxt = {};

    stimctxt.stim = stim;
    zVIL_input_stim(vil, &stimctxt);
}

static inline void zVIL_input_stim(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt)
{
    st_STIMULUS_CONTEXT_DATA resp = {};

    if (!vil->standby || (vil->standby && vil->tmr_list[ZVIL_TDUR_STDBY] <= 0.0f))
    {
        resp = *stimctxt;
        resp.stim = (en_NPC_STIMULUS)0;
        vil->vhf->stimulus(vil, stimctxt, &resp);
        if (resp.stim)
        {
            zVIL_trangoal(vil, &resp, globals.update_dt);
        }
    }
}

static signed int zVIL_trangoal(st_zEntVillain* vil, st_STIMULUS_CONTEXT_DATA* stimctxt, float dt)
{
    signed int rc = 1;
    st_STIMULUS_CONTEXT_DATA exitctxt = {};
    signed int i;

    if (stimctxt->stim == 5 && stimctxt->trangoal != vil->goal)
    {
        if (stimctxt->tranmode)
        {
            vil->mode = stimctxt->tranmode;
        }
        for (i = 0; i < 4; i++)
        {
            vil->snd_queue[i].sndEvent = (en_NPC_SOUND)-1;
            vil->snd_queue[i].sndDirect = 0;
        }

        exitctxt.stim = (en_NPC_STIMULUS)3;
        exitctxt.trangoal = vil->goal;
        if (!vil->vhf->trangoal(vil, &exitctxt, dt))
        {
            rc = 0;
        }
        stimctxt->stim = (en_NPC_STIMULUS)4;
        if (!vil->vhf->trangoal(vil, stimctxt, dt))
        {
            rc = 0;
        }
        VGOL_set(vil, stimctxt->trangoal);
        vil->tmr_list[ZVIL_TYMR_CURGOAL] = 0.0f;
    }
    return rc;
}

static inline signed int VOUT_patrol_move(st_zEntVillain* vil)
{
    if (!vil->nav_next)
    {
        xSceneID2Name(globals.sceneCur, vil->id);
        return 0;
    }
    vil->pos_moveto = *zMovePointGetPos(vil->nav_next);
    VOUT_steer(vil);
    return 1;
}

static inline signed int VOUT_chkdo_patrolArrived(st_zEntVillain* vil)
{
    _xVec3 entpos = {};
    float dist = VSUP_ds2_toPos(vil, vil->pos_moveto);
    _xMat4x3* mat = (_xMat4x3*)vil->model->Mat;

    entpos = mat->pos;
    if ((vil->abl_move & 0x1000) || (vil->abl_move & 0x400))
    {
        float dy = vil->pos_moveto.y - entpos.y;
        dy = dy * dy;
        dist = (dist > dy) ? dist : dy;
    }

    if (dist > g_npcTable[vil->asset->subtype].ds2_atpos)
    {
        return 0;
    }
    if (!(vil->flg_misc & 0x40000000))
    {
        vil->pos_guard_home = mat->pos;
    }
    vil->tmr_list[24] = zMovePointGetDelay(vil->nav_next);
    vil->need_navpt = 1;
    vil->tmr_list[3] = -1.0f;
    zEntEvent((xBase*)vil, (xBase*)vil->nav_next, 0x1F);
    return 1;
}

static inline signed int VOUT_chkdo_minipatrolArrived(st_zEntVillain* vil)
{
    _xMat4x3* mat = (_xMat4x3*)vil->model->Mat;
    _xVec3 entpos = {};
    float dy = 0.0f;
    float dist;

    entpos = mat->pos;
    if ((vil->abl_move & 0x1000) || (vil->abl_move & 0x400))
    {
        dy = vil->pos_moveto.y - entpos.y;
        dy = dy * dy;
    }
    dist = VSUP_ds2_toPos(vil, vil->pos_moveto);
    if (!(dist > dy))
    {
        dist = dy;
    }
    if (dist > vil->ds2_goto_keepaway + g_npcTable[vil->asset->subtype].ds2_atpos)
    {
        return 0;
    }
    if (vil->vhf->query(vil, (en_ZVIL_QUERY)4))
    {
        vil->flg_misc &= ~0x80000000;
    }
    else
    {
        vil->flg_misc &= ~0x40000000;
    }
    return 1;
}

// Distance (squared) to the move target; flyers and swimmers also count height.
static inline signed int VOUT_follow_move(st_zEntVillain* vil)
{
    if (!vil->nav_next && !(vil->flg_misc & 0x10000000) && !(vil->flg_misc & 0x20000000))
    {
        return 0;
    }
    if (vil->flg_misc & 0x10000000)
    {
        vil->pos_moveto = vil->pos_goto;
    }
    else if (vil->flg_misc & 0x20000000)
    {
        vil->pos_moveto = ((_xMat4x3*)vil->tgt_follow->model->Mat)->pos;
    }
    else
    {
        vil->pos_moveto = *zMovePointGetPos(vil->nav_next);
    }
    VOUT_steer(vil);
    return 1;
}

static inline signed int VOUT_chkdo_followArrived(st_zEntVillain* vil)
{
    _xVec3 entpos = {};
    float dist = VSUP_ds2_toPos(vil, vil->pos_moveto);
    _xMat4x3* mat = (_xMat4x3*)vil->model->Mat;

    entpos = mat->pos;
    if ((vil->abl_move & 0x1000) || (vil->abl_move & 0x400))
    {
        float dy = vil->pos_moveto.y - entpos.y;
        dy = dy * dy;
        dist = (dist > dy) ? dist : dy;
    }

    if (dist > vil->ds2_goto_keepaway + g_npcTable[vil->asset->subtype].ds2_atpos)
    {
        return 0;
    }
    if (!(vil->flg_misc & 0x40000000))
    {
        vil->pos_guard_home = mat->pos;
    }
    if ((vil->flg_misc & 0x10000000) || (vil->flg_misc & 0x20000000))
    {
        vil->ds2_goto_keepaway = 0.0f;
        vil->flg_misc &= ~0x10000000;
        vil->flg_misc &= ~0x20000000;
        vil->allow &= ~1;
    }
    else
    {
        vil->tmr_list[24] = zMovePointGetDelay(vil->nav_next);
        vil->need_navpt = 1;
        vil->tmr_list[3] = -1.0f;
        zEntEvent((xBase*)vil, (xBase*)vil->nav_next, 0x1F);
    }
    return 1;
}

static inline signed int VOUT_chase_move(st_zEntVillain* vil)
{
    if (vil->precalc.flg_plyr & 0x400)
    {
        vil->pos_moveto = vil->precalc.pos_intercept;
    }
    else
    {
        vil->pos_moveto = *xBoundCenter(&globals.player.ent.bound);
    }
    vil->ang_moveto = vil->precalc.ang_VtoP;
    vil->domove |= 0x800;
    vil->domove |= 1;
    vil->domove |= 0xA;
    if ((vil->abl_move & 0x1000) || (vil->abl_move & 0x400))
    {
        vil->domove |= 4;
    }
    vil->domove |= 0x80000000;
    return 1;
}

static signed int VOUT_dogoto_widget(st_zEntVillain* vil, xBase* widge, float keepaway, signed int follow)
{
    signed int rc = 0;

    vil->allow |= 1;
    if (vil->nav_next)
    {
        vil->nav_prev = vil->nav_next;
    }
    vil->nav_next = NULL;
    vil->need_navpt = 0;
    if (follow)
    {
        vil->flg_misc |= 0x20000000;
        vil->flg_misc &= ~0x10000000;
    }
    else
    {
        vil->flg_misc &= ~0x20000000;
        vil->flg_misc |= 0x10000000;
    }
    vil->flg_misc &= ~0x40000000;
    vil->flg_misc &= ~0x80000000;
    vil->ds2_goto_keepaway = keepaway * keepaway;

    switch (widge->baseType)
    {
    case 13:
        if (follow)
        {
            vil->nav_next = (_zMovePoint*)widge;
            rc = 1;
            vil->flg_misc &= ~0x10000000;
            vil->flg_misc &= ~0x20000000;
        }
        else
        {
            vil->pos_goto = *zMovePointGetPos((_zMovePoint*)widge);
        }
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
        if (follow)
        {
            vil->tgt_follow = (_xEnt*)widge;
        }
        else
        {
            vil->pos_goto = ((_xMat4x3*)((_xEnt*)widge)->model->Mat)->pos;
        }
        break;
    }
    return rc;
}

static inline signed int VOUT_hit_isWall(st_zEntVillain* vil, _xVec3* dir_trav, _xCollis* colrec)
{
    _xVec3 vec_tmp = {};
    signed int flg_look;
    signed int result = 0;

    flg_look = 0xC;
    if (vil->abl_move & 0x1000)
    {
        flg_look = 8;
    }

    if ((float)__fabs(colrec->norm.y) > 0.01f)
    {
        result = 0;
    }
    else if (vil->mode > ZVIL_MODE_NORMAL)
    {
        result = 0;
    }
    else if (!(colrec->norm.x * colrec->norm.x + colrec->norm.z * colrec->norm.z > 0.0f))
    {
        result = 0;
    }
    else
    {
        vec_tmp.x = colrec->norm.x;
        vec_tmp.z = colrec->norm.z;
        xVec3Normalize(&vec_tmp, &vec_tmp);
        if ((float)__fabs(xVec3Dot(dir_trav, &vec_tmp)) > 0.8f)
        {
            _zMovePoint* mvpt;

            result = 1;
            mvpt = zMovePointFindNearest(flg_look | 0x11, xBoundCenter(&vil->bound), vil,
                                         VNAV_chklos_timz);
            vil->need_navpt = 0;
            if (mvpt && mvpt != vil->nav_next)
            {
                vil->nav_next = mvpt;
            }
        }
    }
    return result;
}

static signed int VOUT_apply_anchor(st_zEntVillain* vil)
{
    signed int found = 0;

    if (vil->tgt_protect)
    {
        switch (vil->tgt_protect->baseType)
        {
        case 13:
            vil->pos_pursue_home = *zMovePointGetPos((_zMovePoint*)vil->tgt_protect);
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
            found = 1;
            vil->pos_pursue_home = ((_xMat4x3*)vil->tgt_protect->model->Mat)->pos;
            break;
        default:
            xSceneID2Name(globals.sceneCur, vil->id);
            xSceneID2Name(globals.sceneCur, vil->tgt_protect->id);
            vil->tgt_protect = NULL;
            found = 0;
            vil->ds2_goto_keepaway = 0.0f;
            break;
        }
    }
    if (!found)
    {
        found = 1;
        vil->pos_pursue_home = *VUTIL_mdlpos(vil);
        vil->ds2_goto_keepaway = 0.0f;
    }
    return found;
}

static en_ZVILLAIN_HEADBUTT_RESPONSE VOUT_spikey_header(st_zEntVillain* vil)
{
    signed int resp = 0;
    signed int side = 0;

    if (!(vil->abl_spyk & 1))
    {
        return (en_ZVILLAIN_HEADBUTT_RESPONSE)0;
    }

    VSUP_ent_AOI(vil, &globals.player.ent, &side);

    // The original has no breaks here, so each side also checks the later ones.
    switch (side)
    {
    case 1:
        if (vil->abl_spyk & 2)
        {
            resp = 4;
        }
    case 2:
        if (vil->abl_spyk & 4)
        {
            resp = 4;
        }
    case 3:
        if (vil->abl_spyk & 8)
        {
            resp = 4;
        }
    case 4:
        if (vil->abl_spyk & 0x10)
        {
            resp = 4;
        }
        break;
    }
    return (en_ZVILLAIN_HEADBUTT_RESPONSE)resp;
}

static signed int VNAV_get_nav(st_zEntVillain* vil)
{
    _zMovePoint* nxt = NULL;

    vil->tmr_list[3] = vil->vilcfg.tymref_list[3];
    if (!(vil->tmr_list[ZVIL_TDUR_NAVDELAY] <= 0.0f))
    {
        nxt = NULL;
    }
    else if (!vil->nav_prev)
    {
        if (zMovePointIsOn(vil->nav_mvptlist))
        {
            vil->nav_prev = vil->nav_mvptlist;
            nxt = vil->nav_mvptlist;
        }
        else
        {
            nxt = NULL;
        }
    }
    else if (vil->nav_next)
    {
        zMovePointGetNext(vil->nav_next, vil->nav_prev, &nxt, NULL);
    }
    else
    {
        zMovePointGetNext(vil->nav_prev, NULL, &nxt, NULL);
    }

    if (nxt)
    {
        vil->need_navpt = 0;
        _xVec3 d = {};
        if (vil->nav_prev)
        {
            vil->dst_navTween = VNAV_dist(&d, zMovePointGetPos(nxt), zMovePointGetPos(vil->nav_prev));
        }
        else if (((_xMovePoint*)nxt)->spl)
        {
            vil->dst_navTween = VNAV_dist(&d, zMovePointGetPos(nxt), VUTIL_mdlpos(vil));
        }
    }

    if (vil->nav_next)
    {
        vil->nav_prev = vil->nav_next;
    }
    vil->nav_next = nxt;
    return vil->nav_next != NULL;
}

static inline float VNAV_dist(_xVec3* d, _xVec3* a, _xVec3* b)
{
    float d2;

    xVec3Sub(d, a, b);
    d2 = xVec3Dot(d, d);
    if (d2 > 0.0f)
    {
        d2 = xsqrt(d2);
    }
    return d2;
}

static signed int VNAV_chklos_timz(void* usrdata, _xVec3* pos, _zMovePoint* mvpt)
{
    st_zEntVillain* vil = (st_zEntVillain*)usrdata;
    _xRay3 ray;
    _xVec3* pos_mp;
    float len;

    ray.min_t = 0.001f + vil->bound.sph.r;
    pos_mp = zMovePointGetPos(mvpt);
    xVec3Sub(&ray.dir, pos_mp, pos);
    len = xVec3Normalize(&ray.dir, &ray.dir);
    ray.origin.x = ray.dir.x * ray.min_t + pos->x;
    ray.origin.y = ray.dir.y * ray.min_t + pos->y;
    ray.origin.z = ray.dir.z * ray.min_t + pos->z;
    ray.max_t = len - ray.min_t;
    ray.flags = 0x800;
    return (unsigned char)(xRayHitsSceneFast(&ray, globals.sceneCur) == 0);
}

static float VNAV_get_movespeed(st_zEntVillain* vil, float rnd)
{
    float spd = rnd;

    if (VANM_stateUserFlags(vil, 0x1E000000) & 0x04000000)
    {
        return 5.0f;
    }

    if (vil->goal == ZVIL_GOAL_GUMMED && (vil->abl_move & 0x100000))
    {
        spd *= vil->spd_move;
        if (vil->flg_misc & 2)
        {
            VNAV_animspeed(vil, spd, &vil->sanim_move);
        }
        return spd;
    }

    if (VANM_stateUserFlags(vil, 0x1E) == 0xA)
    {
        return spd * 5.0f;
    }

    if (VANM_stateUserFlags(vil, 0x1E) == 6)
    {
        spd *= vil->spd_chase;
        if (vil->flg_misc & 4)
        {
            VNAV_animspeed(vil, spd, &vil->sanim_chase);
        }
        return spd;
    }

    if (VANM_stateUserFlags(vil, 0x1E) == 4)
    {
        if (vil->asset->subtype == eEntVillainBossBlackKnight)
        {
            float ext = vil->spd_chase - vil->spd_move;
            if (ext < 0.0f)
            {
                ext = -ext;
            }
            switch (vil->hitpts)
            {
            case 4:
                ext *= 0.0f;
                break;
            case 3:
                ext *= 0.33f;
                break;
            case 2:
                ext *= 0.66f;
                break;
            }
            spd *= vil->spd_move + ext;
        }
        else
        {
            spd *= vil->spd_move;
        }
        if (vil->flg_misc & 2)
        {
            VNAV_animspeed(vil, spd, &vil->sanim_move);
        }
        return spd;
    }

    if (VANM_stateUserFlags(vil, 0x1E) == 1)
    {
        if (vil->flg_misc & 1)
        {
            float base = vil->sanim_idle.x;
            if (vil->anim_playing && !(base < 0.001f))
            {
                xAnimSingle* single = VANM_single(vil);
                if (single)
                {
                    single->CurrentSpeed = base;
                }
            }
        }
        return spd;
    }

    return 0.0f;
}

static inline void VNAV_animspeed(st_zEntVillain* vil, float spd, _xVec3* sanim)
{
    _xVec3 sa = *sanim;
    float fac;
    float lim;
    xAnimSingle* single;

    if (!vil->anim_playing || sa.x < 0.001f)
    {
        return;
    }
    fac = spd / sa.x;
    lim = (fac < sa.z) ? fac : sa.z;
    if (!(sa.y > lim))
    {
        sa.y = (fac < sa.z) ? fac : sa.z;
    }
    single = VANM_single(vil);
    if (single)
    {
        single->CurrentSpeed = sa.y;
    }
}

static inline unsigned int VANM_stateUserFlags(st_zEntVillain* vil, unsigned int mask)
{
    xAnimSingle* single = VANM_single(vil);
    xAnimState* state = single ? single->State : NULL;
    return state ? (state->UserFlags & mask) : 0;
}

static inline xAnimSingle* VANM_single(st_zEntVillain* vil)
{
    if (!vil->model)
    {
        return NULL;
    }
    if (!vil->model->Anim)
    {
        return NULL;
    }
    return vil->model->Anim->Single;
}

static _zMovePoint* VNAV_find_telepoint(st_zEntVillain* vil, signed int force)
{
    _zMovePoint* mvpt = NULL;
    signed int idx_far = -1;
    float unused_a[12] = {};
    float unused_b[20] = {};
    float ds2_plyr[12] = {};
    float ds2_vil[12] = {};
    _xVec3 pos_ref = {};
    _xVec3 pos_plyr = {};
    signed int i;

    if (!force && vil->precalc.ds2_VtoP > 16.0f)
    {
        return NULL;
    }

    for (i = 0; i < 12; i++)
    {
        if (!g_mmtp[i])
        {
            ds2_plyr[i] = -1.0f;
            ds2_vil[i] = -1.0f;
        }
        else
        {
            // The original measures the villain's distance from pos_ref (a
            // zero vector) rather than from the move point.
            ds2_vil[i] = VSUP_ds2_toPos(vil, pos_ref);
            pos_plyr = ((_xMat4x3*)globals.player.ent.model->Mat)->pos;
            xVec3Sub(&pos_plyr, &pos_plyr, zMovePointGetPos(g_mmtp[i]));
            ds2_plyr[i] = pos_plyr.x * pos_plyr.x + pos_plyr.y * pos_plyr.y + pos_plyr.z * pos_plyr.z;
        }
    }

    for (i = 0; i < 12; i++)
    {
        if (!(ds2_plyr[i] < 0.0f) && !(ds2_vil[i] < 0.0f))
        {
            if (idx_far < 0)
            {
                idx_far = i;
            }
            if (ds2_plyr[i] > ds2_plyr[idx_far])
            {
                idx_far = i;
            }
        }
    }
    if (idx_far >= 0 && ds2_vil[idx_far] > 1.0f)
    {
        mvpt = g_mmtp[idx_far];
    }

    {
        static signed int use_random = 1;

        if (use_random)
        {
            signed int list_near[4] = { 8, 9, 10, 11 };
            signed int list_far[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
            float wts_near[4] = { 0.2f, 0.3f, 0.3f, 0.2f };
            float wts_far[12] = { 1.0f, 2.0f, 2.0f, 1.0f, 2.0f, 1.0f, 1.0f, 2.0f };

            VSUP_wtadjust(wts_far, 8, 1.0f);
            if (xUtil_yesno(0.2f))
            {
                mvpt = g_mmtp[VSUP_choose(list_near, wts_near, 4)];
            }
            else
            {
                mvpt = g_mmtp[VSUP_choose(list_far, wts_far, 8)];
            }
        }
    }

    if (!mvpt && force)
    {
        // Looks for any teleport point but then falls back to the
        // villain's own path regardless.
        for (i = 0; i < 12; i++)
        {
            if (g_mmtp[i])
            {
                break;
            }
        }
        mvpt = vil->nav_mvptlist;
        vil->nav_next = NULL;
    }
    else if (force && mvpt && mvpt == vil->nav_next)
    {
        vil->nav_next = NULL;
    }
    return mvpt;
}

static void zVIL_apply_vilcfg(st_zEntVillain* vil, st_ZVIL_CONFIG_REFERENCE* vilcfg)
{
    signed int allow = 0;
    unsigned int vflags = vilcfg->vilp2ass->villFlags;
    st_ZVIL_NPCTYPE_LOOKUP* npcinfo;

    if (vflags & 1)
    {
        allow |= 1;
    }
    if (vflags & 2)
    {
        allow |= 2;
    }
    if (vflags & 4)
    {
        allow |= 4;
    }
    if (vflags & 8)
    {
        allow |= 0x10;
    }
    if (vflags & 0x10)
    {
        allow |= 8;
    }
    if (vflags & 0x80)
    {
        allow |= 0x100;
    }
    if (vflags & 0x100)
    {
        allow |= 0x200;
    }
    if (vflags & 0x20)
    {
        allow |= 0x40;
    }
    if (vflags & 0x40)
    {
        allow |= 0x80;
    }
    vil->allow = allow;
    vil->flg_misc = vilcfg->misc_flags;

    VCFG_getParm(vilcfg, ZVIL_PARM_SANIM_IDLE, &vil->sanim_idle);
    VCFG_getParm(vilcfg, ZVIL_PARM_SANIM_MOVE, &vil->sanim_move);
    VCFG_getParm(vilcfg, ZVIL_PARM_SANIM_CHASE, &vil->sanim_chase);

    npcinfo = &g_npcTable[vil->asset->subtype];
    if (npcinfo->vec_bndoff.x != vilcfg->vec_bndoffset.x || npcinfo->vec_bndoff.y != vilcfg->vec_bndoffset.y ||
        npcinfo->vec_bndoff.z != vilcfg->vec_bndoffset.z)
    {
        npcinfo->vec_bndoff = vilcfg->vec_bndoffset;
    }
    if (npcinfo->rad_bound != vilcfg->rad_bound)
    {
        npcinfo->rad_bound = vilcfg->rad_bound;
    }
    npcinfo->rad_bound2 = vilcfg->rad_bound2;
    npcinfo->vec_bnd2off = vilcfg->vec_bnd2offset;

    VCFG_getParm(vilcfg, ZVIL_PARM_WANDER_ARC, &vil->ang_wander);
    VCFG_getParm(vilcfg, ZVIL_PARM_HIT_POINTS, &vil->hitpts);
    VCFG_getParm(vilcfg, ZVIL_PARM_MOVE_RATE, &vil->spd_move);
    VCFG_getParm(vilcfg, ZVIL_PARM_CHASE_RATE, &vil->spd_chase);
    VCFG_getParm(vilcfg, ZVIL_PARM_TURN_RATE, &vil->spd_turn);
    VCFG_getParm(vilcfg, ZVIL_PARM_DETECT_RADIUS, &vil->ds2_detect);
    VCFG_getParm(vilcfg, ZVIL_PARM_DETECT_HEIGHT, &vil->hyt_detect);
    VCFG_getParm(vilcfg, ZVIL_PARM_DETECT_HYTOFF, &vil->hyt_dtctoff);
    VCFG_getParm(vilcfg, ZVIL_PARM_FOV_ANGLE, &vil->fov_detect);
    VCFG_getParm(vilcfg, ZVIL_PARM_PURSUE_RADIUS, &vil->ds2_pursue);
    VCFG_getParm(vilcfg, ZVIL_PARM_LOB_RANGE, &vil->dst_lob);
    VCFG_getParm(vilcfg, ZVIL_PARM_LOB_SPEED, &vil->spd_lob);
    VCFG_getParm(vilcfg, ZVIL_PARM_LOB_SALVO, &vil->cnt_lobSalvo);
    VCFG_getParm(vilcfg, ZVIL_PARM_LOB_ARC, &vil->fac_lobarc);
    VCFG_getParm(vilcfg, ZVIL_PARM_LOB_HEAVY, &vil->fac_lobheavy);
    VCFG_getParm(vilcfg, ZVIL_PARM_LOB_PRJTYPE, &vil->prjTypeID);
    VCFG_getParm(vilcfg, ZVIL_PARM_EXTEND_RANGE, &vil->dst_extend);
    VCFG_getParm(vilcfg, ZVIL_PARM_EXTEND_WIDTH, &vil->wid_extend);
    VCFG_getParm(vilcfg, ZVIL_PARM_EXTEND_RATE, &vil->spd_extend);

    vilcfg->tymref_list[23] =
        (g_npcTable[vil->asset->subtype].decay > vilcfg->tymref_list[23]) ? g_npcTable[vil->asset->subtype].decay
                                                                            : vilcfg->tymref_list[23];
}

static void zVIL_upd_timers(st_zEntVillain* vil, float dt)
{
    VTMR_tick(vil, 9, dt);
    VTMR_tick(vil, 28, dt);
    VTMR_tick(vil, 29, -dt);
    VTMR_tick(vil, 30, dt);
    VTMR_tick(vil, 19, dt);
    VTMR_tick(vil, 20, dt);
    VTMR_tick(vil, 24, dt);
    VTMR_tick(vil, 6, dt);
    VTMR_tick(vil, 7, dt);
    VTMR_tick(vil, 27, dt);

    switch (vil->goal)
    {
    case 9:
        VTMR_tick(vil, 15, dt);
    case 4:
        VTMR_tick(vil, 1, dt);
    case 5:
        VTMR_tick(vil, 0, dt);
        VTMR_tick(vil, 3, dt);
        VTMR_tick(vil, 24, dt);
        VTMR_tick(vil, 8, dt);
        break;
    case 6:
    case 7:
    case 8:
        VTMR_tick(vil, 3, dt);
        VTMR_tick(vil, 24, dt);
        break;
    case 12:
        VTMR_tick(vil, 0, dt);
        break;
    case 13:
        VTMR_tick(vil, 15, dt);
        break;
    case 14:
        VTMR_tick(vil, 16, dt);
        break;
    case 16:
        VTMR_tick(vil, 17, dt);
        break;
    case 23:
        VTMR_tick(vil, 11, dt);
        break;
    case 17:
    case 18:
        VTMR_tick(vil, 10, dt);
        VTMR_tick(vil, 2, dt);
        VTMR_tick(vil, 5, dt);
        break;
    case 19:
        VTMR_tick(vil, 3, dt);
        break;
    case 21:
    case 22:
        VTMR_tick(vil, 18, dt);
        VTMR_tick(vil, 21, dt);
    case 20:
        VTMR_tick(vil, 26, dt);
        break;
    case 24:
        VTMR_tick(vil, 12, dt);
        VTMR_tick(vil, 0, dt);
        VTMR_tick(vil, 4, dt);
        break;
    case 26:
        VTMR_tick(vil, 13, dt);
        VTMR_tick(vil, 0, dt);
        VTMR_tick(vil, 25, dt);
        VTMR_tick(vil, 4, dt);
        break;
    case 25:
        VTMR_tick(vil, 14, dt);
        VTMR_tick(vil, 0, dt);
        VTMR_tick(vil, 4, dt);
        break;
    case 27:
        VTMR_tick(vil, 22, dt);
        break;
    case 32:
        VTMR_tick(vil, 23, dt);
        break;
    }
}

static inline void VTMR_tick(st_zEntVillain* vil, signed int idx, float dt)
{
    float t = vil->tmr_list[idx] - dt;
    vil->tmr_list[idx] = (t < -1.0f) ? -1.0f : t;
}

static void zVIL_precalc(st_zEntVillain* vil, signed int flags)
{
    _xVec3 pos_vil = {};
    _xVec3 ctr_vil = {};
    _xVec3 pos_plyr = {};
    _xVec3 ctr_plyr = {};
    _xVec3 vec = {};
    st_ZVIL_PRECALC* pc;
    signed int doflags = flags;
    signed int old_pit;
    signed int old_los;

    if (flags & 2)
    {
        doflags |= 1;
    }

    pc = &vil->precalc;
    old_pit = pc->flg_xtra & 1;
    old_los = (pc->flg_plyr >> 3) & 1;
    memset(pc, 0, sizeof(st_ZVIL_PRECALC));

    pos_vil = ((_xMat4x3*)vil->model->Mat)->pos;
    ctr_vil = *xBoundCenter(&vil->bound);
    pos_plyr = ((_xMat4x3*)globals.player.ent.model->Mat)->pos;
    ctr_plyr = *xBoundCenter(&globals.player.ent.bound);

    if (doflags & 1)
    {
        float ds2;

        pc->ds2_VtoP = VSUP_ds2_toPos(vil, VUTIL_posOf(&globals.player.ent));
        ds2 = pc->ds2_VtoP;
        if (ds2 > 0.0f)
        {
            ds2 = VUTIL_sqrt(ds2);
        }
        pc->dst_VtoP = ds2;
        pc->ang_VtoP = VSUP_dir_toPos(vil, VUTIL_ctrOf(&globals.player.ent), &pc->dir_VtoP);
        pc->vec_VtoP.x = pos_plyr.x - pos_vil.x;
        pc->vec_VtoP.y = pos_plyr.y - pos_vil.y;
        pc->vec_VtoP.z = pos_plyr.z - pos_vil.z;

        pc->plyr_mvinf = zEntPlayer_MoveInfo();
        if (g_scene_is_a_boss_battle)
        {
            pc->plyr_mvinf &= ~0xFF00;
        }
        else if (!(pc->plyr_mvinf & 0xFF00))
        {
            float tym = vil->vilcfg.tymref_list[ZVIL_TWIN_IGNORESNEAK];
            vil->tmr_list[ZVIL_TWIN_IGNORESNEAK] = VUTIL_jitter(tym);
        }
        else if (!(vil->tmr_list[ZVIL_TWIN_IGNORESNEAK] <= 0.0f))
        {
            pc->plyr_mvinf &= ~0xFF00;
        }
        pc->flg_status |= 1;
    }

    if (doflags & 4)
    {
        pc->dif_plyrCtr = ctr_plyr.y - pos_plyr.y;
        pc->hyt_plyr = 2.0f * pc->dif_plyrCtr;
        pc->dif_vilCtr = ctr_vil.y - pos_vil.y;
        pc->hyt_vil = 2.0f * pc->dif_vilCtr;
        pc->flg_status |= 4;
    }

    if (vil->mode > ZVIL_MODE_NORMAL && zVIL_chk_pitfall(vil, old_pit, 0.25f))
    {
        pc->flg_xtra |= 1;
    }

    if (doflags & 8)
    {
        float yaw;

        pc->ds2_anchor = VSUP_ds2_toPos(vil, vil->pos_pursue_home);
        pc->ds2_grdpos = VSUP_ds2_toPos(vil, vil->pos_guard_home);
        yaw = vil->frame->rot.angle;
        pc->cos_faceang = cos(yaw);
        pc->sin_faceang = sin(yaw);
        pc->flg_status |= 8;
    }

    if (doflags & 2)
    {
        float hd = vil->hyt_detect;
        float top = 0.5f * hd + vil->hyt_dtctoff;
        float bot = top - hd;

        if (vil->vhf->query(vil, ZVIL_Q_CANDETECT))
        {
            float rad2 = vil->ds2_detect;

            if (pc->plyr_mvinf & 0xFF00)
            {
                float r = 0.0625f * rad2;
                rad2 = r * r;
            }
            if (pc->ds2_VtoP < rad2)
            {
                float dy = pc->vec_VtoP.y;

                if (dy < top && dy > bot)
                {
                    float rclose;

                    pc->flg_plyr |= 1;
                    rclose = 0.0625f * vil->ds2_detect;
                    if (pc->ds2_VtoP < rclose * rclose)
                    {
                        pc->flg_plyr |= 1;
                    }
                    {
                        if (VSUP_ent_inFOV(vil, &globals.player.ent))
                        {
                            pc->flg_plyr |= 4;
                        }
                        if (vil->allow & 0x200)
                        {
                            pc->flg_plyr |= 8;
                        }
                        else if (VSUP_ent_inLOS(vil, &globals.player.ent, old_los, 0.1f))
                        {
                            pc->flg_plyr |= 8;
                        }
                    }
                }
            }
            if (pc->dst_VtoP < 1.1f)
            {
                pc->flg_plyr |= 0x10;
            }
            if (pc->dst_VtoP < vil->dst_lob)
            {
                pc->flg_plyr |= 0x20;
            }
            if (pc->dst_VtoP < vil->dst_extend)
            {
                pc->flg_plyr |= 0x40;
            }
        }

        if (vil->flg_misc & 0x80000000)
        {
            if (!(pc->flg_plyr & 1))
            {
                {
                        if (VSUP_ent_inFOV(vil, &globals.player.ent))
                        {
                            pc->flg_plyr |= 4;
                        }
                        if (vil->allow & 0x200)
                        {
                            pc->flg_plyr |= 8;
                        }
                        else if (VSUP_ent_inLOS(vil, &globals.player.ent, old_los, 0.1f))
                        {
                            pc->flg_plyr |= 8;
                        }
                    }
            }

            vec.x = ctr_plyr.x - vil->pos_pursue_home.x;
            vec.y = ctr_plyr.y - vil->pos_pursue_home.y;
            vec.z = ctr_plyr.z - vil->pos_pursue_home.z;
            pc->ds2_plyrToAnchor = vec.x * vec.x + vec.z * vec.z;
            if (pc->ds2_plyrToAnchor > vil->ds2_pursue)
            {
                float atpos = g_npcTable[vil->asset->subtype].ds2_atpos;

                if (vil->ds2_pursue > atpos)
                {
                    float rad;
                    float dx;
                    float dz;

                    xVec3Normalize(&pc->pos_intercept, &vec);
                    rad = vil->ds2_pursue;
                    if (rad > 0.0f)
                    {
                        rad = VUTIL_sqrt(rad);
                    }
                    xVec3SMulBy(&pc->pos_intercept, rad);
                    xVec3AddTo(&pc->pos_intercept, &vil->pos_pursue_home);
                    dx = pos_vil.x - pc->pos_intercept.x;
                    dz = pos_vil.z - pc->pos_intercept.z;
                    if (g_npcTable[vil->asset->subtype].ds2_atpos < dx * dx + dz * dz)
                    {
                        pc->flg_plyr |= 0x400;
                    }
                }
            }
        }
        pc->flg_status |= 2;
    }

    if (pc->dst_VtoP < 5.0f)
    {
        float dy = pc->vec_VtoP.y;
        if (dy < 0.0f)
        {
            dy = -dy;
        }
        if (dy < 3.0f)
        {
            zEntPlayer_Notify((unsigned char)vil->asset->subtype);
        }
    }
    pc->flg_status |= 0x80000000;
}

static void zVIL_apply_move(st_zEntVillain* vil, _xScene*, float dt, _xEntFrame* frame)
{
    signed int dotran = 0;
    float yaw;
    float dang = 0.0f;
    signed int dovel = 0;
    signed int bigturn = 0;
    signed int doturn = 0;
    _xVec3 pos_cur = {};
    _xMat4x3* mdlmat = (_xMat4x3*)vil->model->Mat;

    pos_cur = mdlmat->pos;
    if (!vil->domove)
    {
        return;
    }

    if (vil->abl_move & 0x2000)
    {
        vil->ang_mdlface = frame->rot.angle;
    }
    if (vil->abl_move & 0x4000)
    {
        frame->rot.angle -= 1.5707964f;
    }
    if (vil->domove & 0x80000)
    {
        frame->rot.angle += 1.5707964f;
    }
    else if (vil->domove & 0x40000)
    {
        frame->rot.angle -= 1.5707964f;
    }

    if (vil->domove & 0x40)
    {
        dang = vil->ang_moveto - frame->rot.angle;
        doturn = 1;
    }
    else if (vil->domove & 0x80)
    {
        dang = xAngleClamp(atan2(vil->dir_moveto.x, vil->dir_moveto.z)) - frame->rot.angle;
        doturn = 1;
    }
    else if (vil->domove & 0x100)
    {
        dang = vil->dlt_faceang;
        doturn = 1;
    }
    else if (vil->domove & 0x800)
    {
        dang = xAngleClamp(atan2(vil->pos_moveto.x - pos_cur.x, vil->pos_moveto.z - pos_cur.z)) -
               frame->rot.angle;
        doturn = 1;
    }
    else if (vil->domove & 0x400)
    {
        dang = xAngleClamp(atan2(vil->dlt_moveto.x, vil->dlt_moveto.z)) - frame->rot.angle;
        doturn = 1;
    }

    if (!(vil->domove & 1))
    {
        doturn = 0;
    }
    if (vil->domove & 2)
    {
        dotran = 1;
    }
    if (vil->domove & 4)
    {
        dotran = 1;
    }
    if (vil->domove & 8)
    {
        dotran = 1;
    }
    if (vil->domove & 0x5000)
    {
        dovel = 1;
    }
    if (vil->domove & 0x20000000)
    {
        doturn = 0;
        frame->drot.angle = vil->ang_moveto - frame->rot.angle;
        frame->mode |= 0x20;
    }
    if (vil->domove & 0x40000000)
    {
        dotran = 0;
        ((_xMat4x3*)frame)->pos = vil->pos_moveto;
        frame->mode |= 1;
    }

    yaw = frame->rot.angle;
    if (doturn &&
        ((vil->abl_move & 0x1000) || (vil->abl_move & 0x400) || (VANM_stateUserFlags(vil, 0x1E000000) & 0x02000000)))
    {
        float rnd = 0.15f * iurand() + 0.85f;
        float rate;

        if (vil->domove & 0x10000)
        {
            rate = dt * vil->wiz_moveto;
        }
        else
        {
            rate = rnd * (dt * vil->spd_turn);
        }
        if (dang > 3.1415927f)
        {
            dang -= 6.2831855f;
        }
        if (dang < -3.1415927f)
        {
            dang += 6.2831855f;
        }
        if (vil->abl_move & 0x4000)
        {
            frame->rot.angle += 1.5707964f;
        }
        if (vil->domove & 0x80000)
        {
            frame->rot.angle -= 1.5707964f;
        }
        else if (vil->domove & 0x40000)
        {
            frame->rot.angle += 1.5707964f;
        }
        frame->drot.angle = rate * dang;
        frame->mode |= 0x20;
        yaw += frame->drot.angle;
        if ((float)__fabs(dang) > 0.5f * vil->fov_detect)
        {
            bigturn = 1;
        }
        dotran = 1;
    }

    if (vil->domove & 0x10)
    {
        dotran = 0;
    }
    if (vil->domove & 0x40000000)
    {
        dotran = 0;
    }

    if (dotran)
    {
        float rnd;
        float spd;
        float dst;
        float ds2;
        float ds2_step;
        float lim_rand;
        float lim_snap;
        float dx;
        float dz;

        if (bigturn)
        {
            rnd = 0.4f * iurand() + 0.6f;
        }
        else
        {
            rnd = 0.1f * iurand() + 0.9f;
        }
        if (vil->domove & 0x8000)
        {
            spd = vil->spd_moveto;
        }
        else
        {
            spd = VNAV_get_movespeed(vil, rnd);
        }
        spd = spd * dt;

        dx = vil->pos_moveto.x - pos_cur.x;
        dz = vil->pos_moveto.z - pos_cur.z;
        ds2_step = spd * spd;
        {
            float x2 = dx * dx;
            float z2 = dz * dz;
            ds2 = x2 + z2;
        }
        if (ds2 < ds2_step)
        {
            dst = xsqrt(ds2);
        }
        else
        {
            dst = spd;
        }

        lim_rand = 0.25f;
        lim_snap = 0.1f;
        frame->dpos.x = 0.0f;
        frame->dpos.y = 0.0f;
        frame->dpos.z = 0.0f;
        if (vil->asset->subtype == 0x18)
        {
            lim_rand = 0.1f;
            lim_snap = 0.01f;
        }

        if (ds2 < lim_rand)
        {
            frame->dpos.x = (vil->pos_moveto.x - pos_cur.x) * (0.5f * iurand());
            frame->dpos.z = (vil->pos_moveto.z - pos_cur.z) * (0.5f * iurand());
        }
        else if (ds2 < lim_snap)
        {
            frame->dpos.x = vil->pos_moveto.x - pos_cur.x;
            frame->dpos.z = vil->pos_moveto.z - pos_cur.z;
        }
        else
        {
            float cs = cos(yaw);
            float sn = sin(yaw);

            if (vil->domove & 2)
            {
                if (vil->domove & 0x20)
                {
                    frame->dpos.x = 0.5f * -dst * sn;
                }
                else
                {
                    frame->dpos.x = dst * sn;
                }
            }
            else
            {
                frame->dpos.x = 0.0f;
            }
            if (vil->domove & 8)
            {
                if (vil->domove & 0x20)
                {
                    frame->dpos.z = 0.5f * -dst * cs;
                }
                else
                {
                    frame->dpos.z = dst * cs;
                }
            }
            else
            {
                frame->dpos.z = 0.0f;
            }
        }

        if (vil->domove & 4)
        {
            float dy = vil->pos_moveto.y - pos_cur.y;

            if (dy * dy < ds2_step)
            {
                frame->dpos.y = dy;
            }
            else if (ds2 < 0.0625f)
            {
                if (dy < 0.0f)
                {
                    frame->dpos.y = -1.0f * spd;
                }
                else
                {
                    frame->dpos.y = spd;
                }
            }
            else
            {
                float frac = dy / xsqrt(ds2);
                frame->dpos.y = spd * VIL_MAX(-1.0f, VIL_MIN(frac, 1.0f));
            }
        }
        frame->mode |= 2;
    }

    if (dovel)
    {
        if (vil->domove & 0x1000)
        {
            frame->vel = vil->vel_moveto;
            frame->mode |= 4;
        }
        else if (vil->domove & 0x4000)
        {
            xVec3AddTo(&frame->dvel, &vil->vel_moveto);
            frame->mode |= 8;
        }
    }

    if (!(vil->abl_move & 0x1000) && !(vil->abl_move & 0x400))
    {
        if (vil->goal != ZVIL_GOAL_GUMMED && vil->goal != ZVIL_GOAL_SOAPED && !(vil->pflags & 4))
        {
            if (frame->dpos.y < 0.01f)
            {
                frame->dpos.y -= 0.01f;
                frame->mode |= 2;
            }
        }
    }
    vil->domove = 0;
}

static signed int zVIL_chk_pitfall(st_zEntVillain* vil, signed int def, float chance)
{
    _xRay3 ray = {};
    _xVec3 pos_eye = {};
    _xVec3 pos_probe = {};
    _xVec3 vec_side = {};
    signed int pitfall;
    float rad;
    _xMat4x3* mat;

    if (!xUtil_yesno(chance))
    {
        return def;
    }

    rad = vil->bound.sph.r;
    vil->bound.sph.r = 0.0f;
    if (!VUTIL_tagpos(vil, 9, &pos_eye))
    {
        pos_eye = *xBoundCenter(&vil->bound);
    }

    mat = (_xMat4x3*)vil->model->Mat;
    xVec3SMul(&pos_probe, &mat->at, 1.5f * rad);
    pos_probe.y -= 3.5f * rad;
    xVec3AddTo(&pos_probe, &mat->pos);
    xVec3SMul(&vec_side, &mat->right, 0.3f);
    xVec3AddTo(&pos_probe, &vec_side);

    ray.min_t = 0.001f;
    ray.flags = 0x800;
    pitfall = VUTIL_rayhole(&ray, &pos_eye, &pos_probe);
    if (!pitfall)
    {
        xVec3Sub(&pos_probe, &pos_probe, &vec_side);
        xVec3Sub(&pos_probe, &pos_probe, &vec_side);
        pitfall = VUTIL_rayhole(&ray, &pos_eye, &pos_probe);
    }

    vil->bound.sph.r = rad;
    return pitfall;
}

static inline signed int VUTIL_rayhole(_xRay3* ray, _xVec3* pos_eye, _xVec3* pos_probe)
{
    xVec3Sub(&ray->dir, pos_probe, pos_eye);
    ray->max_t = xVec3Normalize(&ray->dir, &ray->dir);
    ray->origin.x = ray->dir.x * ray->min_t + pos_eye->x;
    ray->origin.y = ray->dir.y * ray->min_t + pos_eye->y;
    ray->origin.z = ray->dir.z * ray->min_t + pos_eye->z;
    return xRayHitsSceneFast(ray, globals.sceneCur) == 0;
}

static void VIRM_initVilDflt(st_zEntVillain* vil)
{
    _xVec3 vec_push = {};
    float dot;
    signed int dmg;
    st_ZVIL_NPCTYPE_LOOKUP* npcinfo;

    dot = VSUP_ent_facedot(vil, &globals.player.ent);
    if (0.2f > dot)
    {
        dot = 0.2f;
    }

    npcinfo = &g_npcTable[vil->asset->subtype];
    vec_push = vil->precalc.dir_VtoP;
    vec_push.x *= dot * npcinfo->strength;
    vec_push.y *= dot * npcinfo->strength;
    dmg = (npcinfo->dmg_give > 0) ? npcinfo->dmg_give : 0;
    zEntPlayer_Damage(vil, dmg, &vec_push);
}

static void VCFG_readParms(st_ZVIL_CONFIG_REFERENCE* vilcfg)
{
    unsigned int pmsize = 0;
    _xVec3 def_vert = { 0.0f, 0.0f, 0.0f };
    _xVec3 def_bnd = { 0.0f, 0.0f, 0.0f };
    _xVec3 def_scl = { 1.0f, 1.0f, 1.0f };
    _xVec3 vec = { 0.0f, 0.0f, 0.0f };
    signed int i;
    zAssetModelParam* parms;

    if (!vilcfg->asset->modelInfoID)
    {
        return;
    }
    parms = zEntGetModelParams(vilcfg->asset->modelInfoID, &pmsize);
    if (!parms || !pmsize)
    {
        return;
    }

    for (i = 0; i < 49; i++)
    {
        switch (i)
        {
        case ZVIL_PARM_MDLSCL_BASE:
            if (zParamGetVector(parms, pmsize, g_parmname[i], def_scl, &vec) > 2)
            {
                vilcfg->misc_flags |= 0x8;
            }
            else
            {
                vilcfg->misc_flags &= ~0x8;
            }
            vilcfg->scl_modelBase = vec;
            break;
        case ZVIL_PARM_SANIM_IDLE:
            if (zParamGetVector(parms, pmsize, g_parmname[i], def_scl, &vec) > 2)
            {
                vilcfg->misc_flags |= 0x1;
            }
            else
            {
                vilcfg->misc_flags &= ~0x1;
            }
            vilcfg->sanim_idle = vec;
            break;
        case ZVIL_PARM_SANIM_MOVE:
            if (zParamGetVector(parms, pmsize, g_parmname[i], def_scl, &vec) > 2)
            {
                vilcfg->misc_flags |= 0x2;
            }
            else
            {
                vilcfg->misc_flags &= ~0x2;
            }
            vilcfg->sanim_move = vec;
            break;
        case ZVIL_PARM_SANIM_CHASE:
            // Unlike the other scales, any value count turns the chase scale on.
            if (zParamGetVector(parms, pmsize, g_parmname[i], def_scl, &vec) != 0)
            {
                vilcfg->misc_flags |= 0x4;
            }
            else
            {
                vilcfg->misc_flags &= ~0x4;
            }
            vilcfg->sanim_chase = vec;
            break;
        case ZVIL_PARM_WANDER_ARC:
            vilcfg->ang_wander = zParamGetFloat(parms, pmsize, g_parmname[i], 5.0f);
            break;
        case ZVIL_PARM_HIT_POINTS:
            vilcfg->hitpts = zParamGetInt(parms, pmsize, g_parmname[i], 1);
            break;
        case ZVIL_PARM_BUBBLED_TIME:
            vilcfg->tymref_list[ZVIL_TDUR_GUM] = zParamGetFloat(parms, pmsize, g_parmname[i], 10.0f);
            break;
        case ZVIL_PARM_GUMMED_TIME:
            vilcfg->tymref_list[ZVIL_TDUR_SOAP] = zParamGetFloat(parms, pmsize, g_parmname[i], 10.0f);
            break;
        case ZVIL_PARM_DAZED_TIME:
            vilcfg->tymref_list[ZVIL_TDUR_DAZED] = zParamGetFloat(parms, pmsize, g_parmname[i], 5.0f);
            break;
        case ZVIL_PARM_EXTEND_RANGE:
            vilcfg->dst_extend = zParamGetFloat(parms, pmsize, g_parmname[i], 1.0f);
            break;
        case ZVIL_PARM_EXTEND_RATE:
            vilcfg->spd_extend = zParamGetFloat(parms, pmsize, g_parmname[i], 1.0f);
            break;
        case ZVIL_PARM_EXTEND_WIDTH:
            vilcfg->wid_extend = zParamGetFloat(parms, pmsize, g_parmname[i], 1.0f);
            break;
        case ZVIL_PARM_EXTEND_TIME:
            vilcfg->tymref_list[ZVIL_TDUR_EXTEND] = zParamGetFloat(parms, pmsize, g_parmname[i], 2.0f);
            break;
        case ZVIL_PARM_FOV_ANGLE:
            vilcfg->fov_detect = (3.1415927f * zParamGetFloat(parms, pmsize, g_parmname[i], 50.0f)) / 180.0f;
            break;
        case ZVIL_PARM_GLOAT_TIME:
            vilcfg->tymref_list[ZVIL_TDUR_TAUNT] = zParamGetFloat(parms, pmsize, g_parmname[i], 3.0f);
            break;
        case ZVIL_PARM_LOB_RANGE:
            vilcfg->dst_lob = zParamGetFloat(parms, pmsize, g_parmname[i], 5.0f);
            break;
        case ZVIL_PARM_LOB_SPEED:
            vilcfg->spd_lob = zParamGetFloat(parms, pmsize, g_parmname[i], 2.0f);
            break;
        case ZVIL_PARM_LOB_SALVO:
            vilcfg->cnt_lobSalvo = zParamGetInt(parms, pmsize, g_parmname[i], 1);
            break;
        case ZVIL_PARM_LOB_RELOAD_TIME:
            vilcfg->tymref_list[ZVIL_TDUR_LOBRELOAD] = zParamGetFloat(parms, pmsize, g_parmname[i], 1.0f);
            break;
        case ZVIL_PARM_LOB_ARC:
            // The original stores the arc and heaviness parameters into dst_lob.
            vilcfg->dst_lob = zParamGetFloat(parms, pmsize, g_parmname[i], -1.0f);
            break;
        case ZVIL_PARM_LOB_HEAVY:
            vilcfg->dst_lob = zParamGetFloat(parms, pmsize, g_parmname[i], -1.0f);
            break;
        case ZVIL_PARM_DETECT_RADIUS:
        {
            float rad = zParamGetFloat(parms, pmsize, g_parmname[i], 5.0f);
            vilcfg->ds2_detect = rad * rad;
            break;
        }
        case ZVIL_PARM_DETECT_HEIGHT:
            vilcfg->hyt_detect = zParamGetFloat(parms, pmsize, g_parmname[i], 6.0f);
            break;
        case ZVIL_PARM_DETECT_HYTOFF:
            vilcfg->hyt_dtctoff = zParamGetFloat(parms, pmsize, g_parmname[i], 0.0f);
            break;
        case ZVIL_PARM_PURSUE_RADIUS:
        {
            float rad = zParamGetFloat(parms, pmsize, g_parmname[i], 5.0f);
            vilcfg->ds2_pursue = rad * rad;
            break;
        }
        case ZVIL_PARM_MOVE_RATE:
            vilcfg->spd_move = zParamGetFloat(parms, pmsize, g_parmname[i], 1.0f);
            break;
        case ZVIL_PARM_CHASE_RATE:
            vilcfg->spd_chase = zParamGetFloat(parms, pmsize, g_parmname[i], 1.0f);
            break;
        case ZVIL_PARM_TURN_RATE:
            vilcfg->spd_turn = zParamGetFloat(parms, pmsize, g_parmname[i], 5.235988f);
            break;
        case ZVIL_PARM_VERT_GUM_LFOOT:
            zParamGetVector(parms, pmsize, g_parmname[i], def_vert, &vec);
            vilcfg->pos_verts[0] = vec;
            break;
        case ZVIL_PARM_VERT_GUM_RFOOT:
            zParamGetVector(parms, pmsize, g_parmname[i], def_vert, &vec);
            vilcfg->pos_verts[1] = vec;
            break;
        case ZVIL_PARM_VERT_GUM_ELBOW:
            zParamGetVector(parms, pmsize, g_parmname[i], def_vert, &vec);
            vilcfg->pos_verts[2] = vec;
            break;
        case ZVIL_PARM_VERT_GUM_CUSTA:
            zParamGetVector(parms, pmsize, g_parmname[i], def_vert, &vec);
            vilcfg->pos_verts[3] = vec;
            break;
        case ZVIL_PARM_VERT_GUM_CUSTB:
            zParamGetVector(parms, pmsize, g_parmname[i], def_vert, &vec);
            vilcfg->pos_verts[4] = vec;
            break;
        case ZVIL_PARM_VERT_WEAPON:
            zParamGetVector(parms, pmsize, g_parmname[i], def_vert, &vec);
            vilcfg->pos_verts[5] = vec;
            break;
        case ZVIL_PARM_VERT_ATTACK:
            zParamGetVector(parms, pmsize, g_parmname[i], def_vert, &vec);
            vilcfg->pos_verts[6] = vec;
            break;
        case ZVIL_PARM_VERT_ATTACKALT:
            zParamGetVector(parms, pmsize, g_parmname[i], def_vert, &vec);
            vilcfg->pos_verts[7] = vec;
            break;
        case ZVIL_PARM_VERT_DMG_BOUNCE:
            zParamGetVector(parms, pmsize, g_parmname[i], def_vert, &vec);
            vilcfg->pos_verts[8] = vec;
            break;
        case ZVIL_PARM_VERT_LOSEYEBALL:
            zParamGetVector(parms, pmsize, g_parmname[i], def_vert, &vec);
            vilcfg->pos_verts[9] = vec;
            break;
        case ZVIL_PARM_VERT_RBSWORDHANDLE:
            zParamGetVector(parms, pmsize, g_parmname[i], def_vert, &vec);
            vilcfg->pos_verts[10] = vec;
            break;
        case ZVIL_PARM_VERT_RBSWORDPIVOT:
            zParamGetVector(parms, pmsize, g_parmname[i], def_vert, &vec);
            vilcfg->pos_verts[11] = vec;
            break;
        case ZVIL_PARM_BND_SPH_CENTER:
            def_bnd.x = 0.0f;
            def_bnd.y = 0.7f;
            def_bnd.z = 0.0f;
            zParamGetVector(parms, pmsize, g_parmname[i], def_bnd, &vec);
            vilcfg->vec_bndoffset = vec;
            break;
        case ZVIL_PARM_BND_SPH_RADIUS:
            vilcfg->rad_bound = zParamGetFloat(parms, pmsize, g_parmname[i], 0.7f);
            break;
        case ZVIL_PARM_BND2_SPH_CENTER:
            def_bnd.x = 0.0f;
            def_bnd.y = 2.1f;
            def_bnd.z = 0.0f;
            zParamGetVector(parms, pmsize, g_parmname[i], def_bnd, &vec);
            vilcfg->vec_bnd2offset = vec;
            break;
        case ZVIL_PARM_BND2_SPH_RADIUS:
            vilcfg->rad_bound2 = zParamGetFloat(parms, pmsize, g_parmname[i], 0.0f);
            break;
        case ZVIL_PARM_SPAWNMASK:
            vilcfg->msk_condspawn = zParamGetInt(parms, pmsize, g_parmname[i], 0);
            break;
        case ZVIL_PARM_MINDIFFICULTY:
            vilcfg->minDifficulty = zParamGetInt(parms, pmsize, g_parmname[i], 0);
            break;
        case ZVIL_PARM_DMG_BEGTIME:
            vilcfg->tym_attack_begin = zParamGetInt(parms, pmsize, g_parmname[i], -1) / 30.0f;
            break;
        case ZVIL_PARM_DMG_ENDTIME:
            vilcfg->tym_attack_end = zParamGetInt(parms, pmsize, g_parmname[i], 1000) / 30.0f;
            break;
        }
    }
}

static void VCFG_readAsset(st_ZVIL_CONFIG_REFERENCE* vilcfg)
{
    p2EntVillainAsset* ass = vilcfg->vilp2ass;

    vilcfg->hitpts = ass->hitpoints;
    vilcfg->fov_detect = (3.1415927f * ass->activateFOV) / 180.0f;
    vilcfg->ds2_detect = ass->activateRadius * ass->activateRadius;
    vilcfg->hyt_detect = ass->detectHeight;
    vilcfg->hyt_dtctoff = ass->detectHeightOffset;
    vilcfg->ds2_pursue = ass->pursuitRange * ass->pursuitRange;
    vilcfg->spd_move = ass->speedMovement;
    vilcfg->spd_chase = ass->speedPursue;
    vilcfg->spd_turn = (3.1415927f * ass->speedTurn) / 180.0f;
    vilcfg->spd_lob = ass->lobSpeed;
    vilcfg->dst_lob = ass->lobRange;
    vilcfg->cnt_lobSalvo = ass->lobSalvo;
    vilcfg->fac_lobarc = ass->lobArcness;
    vilcfg->fac_lobheavy = ass->lobHeavy;
    vilcfg->dst_extend = ass->extenderRange;
    vilcfg->wid_extend = ass->extenderWidth;
    vilcfg->spd_extend = ass->extenderRate;

    vilcfg->tymref_list[0] = 8.0f;
    vilcfg->tymref_list[1] = 20.0f;
    vilcfg->tymref_list[2] = 5.0f;
    vilcfg->tymref_list[3] = 7.0f;
    vilcfg->tymref_list[4] = 100.0f;
    vilcfg->tymref_list[5] = 0.3f;
    vilcfg->tymref_list[6] = -1.0f;
    vilcfg->tymref_list[7] = 5.0f;
    vilcfg->tymref_list[8] = 10.0f;
    vilcfg->tymref_list[9] = 10.0f;
    vilcfg->tymref_list[10] = 2.0f;
    vilcfg->tymref_list[ZVIL_TDUR_TAUNT] = ass->durGloatState;
    vilcfg->tymref_list[ZVIL_TDUR_DAZED] = ass->durDazedState;
    vilcfg->tymref_list[ZVIL_TDUR_GUM] = ass->durGummedState;
    vilcfg->tymref_list[ZVIL_TDUR_SOAP] = ass->durBubbleState;
    vilcfg->tymref_list[15] = 3.0f;
    vilcfg->tymref_list[16] = 3.0f;
    vilcfg->tymref_list[17] = 4.0f;
    vilcfg->tymref_list[ZVIL_TDUR_EXTEND] = ass->extenderDuration;
    vilcfg->tymref_list[ZVIL_TDUR_LOBRELOAD] = ass->lobDurReload;
    vilcfg->tymref_list[ZVIL_TDUR_EXTRELOAD] = ass->extenderReloadTime;
    vilcfg->tymref_list[21] = 3.0f;
    vilcfg->tymref_list[22] = -1.0f;
    vilcfg->tymref_list[23] = -1.0f;
    vilcfg->tymref_list[24] = -1.0f;
    vilcfg->tymref_list[25] = 2.0f;
    vilcfg->tymref_list[26] = 0.2f;
    vilcfg->tymref_list[27] = 0.25f;
    vilcfg->tymref_list[28] = -1.0f;
    vilcfg->tymref_list[29] = -1.0f;
    vilcfg->tymref_list[30] = -1.0f;
}

static signed int VCFG_getParm(st_ZVIL_CONFIG_REFERENCE* vilcfg, en_NPC_INI_PARAMETERS pid, void* val)
{
    signed int rc = 1;
    float* fval = (float*)val;
    signed int* ival = (signed int*)val;

    switch (pid)
    {
    case ZVIL_PARM_MDLSCL_BASE:
        *(_xVec3*)val = vilcfg->scl_modelBase;
        break;
    case ZVIL_PARM_SANIM_IDLE:
        *(_xVec3*)val = vilcfg->sanim_idle;
        break;
    case ZVIL_PARM_SANIM_MOVE:
        *(_xVec3*)val = vilcfg->sanim_move;
        break;
    case ZVIL_PARM_SANIM_CHASE:
        *(_xVec3*)val = vilcfg->sanim_chase;
        break;
    case ZVIL_PARM_WANDER_ARC:
        *fval = vilcfg->ang_wander;
        break;
    case ZVIL_PARM_HIT_POINTS:
        *ival = vilcfg->hitpts;
        break;
    case ZVIL_PARM_BUBBLED_TIME:
        *fval = vilcfg->tymref_list[ZVIL_TDUR_SOAP];
        break;
    case ZVIL_PARM_GUMMED_TIME:
        *fval = vilcfg->tymref_list[ZVIL_TDUR_GUM];
        break;
    case ZVIL_PARM_DAZED_TIME:
        *fval = vilcfg->tymref_list[ZVIL_TDUR_DAZED];
        break;
    case ZVIL_PARM_EXTEND_RANGE:
        *fval = vilcfg->dst_extend;
        break;
    case ZVIL_PARM_EXTEND_RATE:
        *fval = vilcfg->spd_extend;
        break;
    case ZVIL_PARM_EXTEND_WIDTH:
        *fval = vilcfg->wid_extend;
        break;
    case ZVIL_PARM_EXTEND_TIME:
        *fval = vilcfg->tymref_list[ZVIL_TDUR_EXTEND];
        break;
    case ZVIL_PARM_FOV_ANGLE:
        *fval = vilcfg->fov_detect;
        break;
    case ZVIL_PARM_GLOAT_TIME:
        *fval = vilcfg->tymref_list[ZVIL_TDUR_TAUNT];
        break;
    case ZVIL_PARM_LOB_RANGE:
        *fval = vilcfg->dst_lob;
        break;
    case ZVIL_PARM_LOB_SPEED:
        *fval = vilcfg->spd_lob;
        break;
    case ZVIL_PARM_LOB_SALVO:
        *ival = vilcfg->cnt_lobSalvo;
        break;
    case ZVIL_PARM_LOB_PRJTYPE:
    {
        unsigned int prjtype;
        switch (vilcfg->asset->subtype)
        {
        case 5:
            prjtype = xStrHash("_PRJ_VWEP_GARGOYLE");
            break;
        case 0x10:
            prjtype = xStrHash("_PRJ_VWEP_TARMONSTER");
            break;
        case 0x12:
            prjtype = xStrHash("_PRJ_VWEP_WITCHDOCTOR");
            break;
        case eEntVillainBossBlackKnight:
            prjtype = xStrHash("_PRJ_VWEP_BLACKKNIGHT");
            break;
        case eEntVillainBossGreenGhost:
            prjtype = xStrHash("_PRJ_VWEP_GREENGHOST");
            break;
        case eEntVillainBossRedbeard:
            prjtype = xStrHash("_PRJ_VWEP_REDBEARD");
            break;
        case eEntVillainBossMastermind:
            prjtype = xStrHash("_PRJ_VWEP_MASTERMIND");
            break;
        default:
            prjtype = 0;
            break;
        }
        *(unsigned int*)val = prjtype;
        break;
    }
    case ZVIL_PARM_LOB_ARC:
        *fval = vilcfg->fac_lobarc;
        break;
    case ZVIL_PARM_LOB_HEAVY:
        *fval = vilcfg->fac_lobheavy;
        break;
    case ZVIL_PARM_DETECT_RADIUS:
        *fval = vilcfg->ds2_detect;
        break;
    case ZVIL_PARM_DETECT_HEIGHT:
        *fval = vilcfg->hyt_detect;
        break;
    case ZVIL_PARM_DETECT_HYTOFF:
        *fval = vilcfg->hyt_dtctoff;
        break;
    case ZVIL_PARM_PURSUE_RADIUS:
        *fval = vilcfg->ds2_pursue;
        break;
    case ZVIL_PARM_MOVE_RATE:
        *fval = vilcfg->spd_move;
        break;
    case ZVIL_PARM_CHASE_RATE:
        *fval = vilcfg->spd_chase;
        break;
    case ZVIL_PARM_TURN_RATE:
        *fval = vilcfg->spd_turn;
        break;
    case ZVIL_PARM_SPAWNMASK:
        *ival = vilcfg->msk_condspawn;
        break;
    case ZVIL_PARM_MINDIFFICULTY:
        *ival = vilcfg->minDifficulty;
        break;
    case ZVIL_PARM_DMG_BEGTIME:
        *fval = vilcfg->tym_attack_begin;
        break;
    case ZVIL_PARM_DMG_ENDTIME:
        *fval = vilcfg->tym_attack_end;
        break;
    default:
        rc = 0;
        break;
    }
    return rc;
}

static en_BEHAVIOUR_GOAL VGOL_set(st_zEntVillain* vil, en_BEHAVIOUR_GOAL goal)
{
    vil->goal = goal;
    if (vil->linkCount)
    {
        switch (vil->goal)
        {
        case ZVIL_GOAL_IDLE:
            zEntEvent(vil, 0xB3);
            break;
        case ZVIL_GOAL_BORED:
            if (vil->vhf->query(vil, ZVIL_Q_ASLEEP))
            {
                zEntEvent(vil, 0xB4);
            }
            break;
        case ZVIL_GOAL_GUARD:
            zEntEvent(vil, 0xB5);
            break;
        case ZVIL_GOAL_PATROL:
            zEntEvent(vil, 0xB6);
            break;
        case ZVIL_GOAL_DAZED:
            zEntEvent(vil, 0xB7);
            break;
        case ZVIL_GOAL_LOOKFORHERO:
            zEntEvent(vil, 0xB8);
            break;
        case ZVIL_GOAL_LISTEN:
            zEntEvent(vil, 0xB9);
            break;
        case ZVIL_GOAL_INVESTIGATE:
            zEntEvent(vil, 0xBA);
            break;
        case ZVIL_GOAL_CHASE:
            zEntEvent(vil, 0xBB);
            break;
        case ZVIL_GOAL_CQCATTACK:
        case ZVIL_GOAL_LOBBING:
        case ZVIL_GOAL_SPELLCAST:
            zEntEvent(vil, 0xBC);
            break;
        case ZVIL_GOAL_MINIPATROL:
            zEntEvent(vil, 0xBD);
            break;
        }
    }
    return vil->goal;
}

static signed int VWEP_swipe(st_zEntVillain* vil, float)
{
    static const iColor_tag clr_swipe = { 0x80, 0x00, 0x00, 0xFF };
    signed int hit = 0;
    _xCollis colrec = {};
    iColor_tag clr = {};
    _xBound bound = {};

    if (vil->precalc.ds2_VtoP > 2.2f)
    {
        return 0;
    }

    colrec.flags = 0;
    colrec.oid = globals.player.ent.id;
    colrec.optr = &globals.player.ent;
    bound.type = 1;
    bound.qcd = globals.player.ent.bound.qcd;
    clr = clr_swipe;
    xDrawSetColor(clr);

    if (VUTIL_tagpos(vil, 6, &bound.sph.center))
    {
        bound.sph.r = 0.2f;
        xBoundHitsBound(&bound, &globals.player.ent.bound, &colrec);
        if (colrec.flags & 1)
        {
            hit = 1;
        }
    }
    if (!hit && VUTIL_tagpos(vil, 7, &bound.sph.center))
    {
        bound.sph.r = 0.2f;
        xBoundHitsBound(&bound, &globals.player.ent.bound, &colrec);
        if (colrec.flags & 1)
        {
            hit = 1;
        }
    }
    if (!hit && VUTIL_tagpos(vil, 5, &bound.sph.center))
    {
        bound.sph.r = 0.3f;
        xBoundHitsBound(&bound, &globals.player.ent.bound, &colrec);
        if (colrec.flags & 1)
        {
            hit = 1;
        }
    }
    if (hit)
    {
        xSceneID2Name(globals.sceneCur, vil->id);
    }
    return hit;
}

static signed int VWEP_extend_particles(st_zEntVillain* vil, float dt, _xVec3* pos_tgt)
{
    // The original carries a second, unused set of the same locals.
    _xVec3 unused_ext = {};
    _xVec3 unused_emit = {};
    _xVec3 unused_alt = {};
    _xVec3 unused_tgt = {};
    _xMat3x3 unused_rot = {};
    _xVec3 unused_vec = {};
    _xVec3 dir_ext = {};
    _xVec3 pos_emit = {};
    _xVec3 pos_alt = {};
    _xVec3 dir_tgt = {};
    _xMat3x3 mat_rot = {};
    _xVec3 vec_tgt = {};
    signed int got_main;
    signed int got_alt;
    float rad;
    float fac_wid;
    float spd;
    float yaw;
    float pitch;

    got_main = VUTIL_tagpos(vil, 6, &pos_emit);
    got_alt = VUTIL_tagpos(vil, 5, &pos_alt);
    if (!got_main && !got_alt)
    {
        pos_emit = pos_alt;
    }
    else if (got_main && got_alt && vil->asset->subtype == eEntVillainTypeKillerPlant)
    {
        xVec3AddTo(&pos_emit, &pos_alt);
        xVec3SMulBy(&pos_emit, 0.5f);
    }

    if (!got_main)
    {
        xSceneID2Name(globals.sceneCur, vil->id);
        pos_emit = *xBoundCenter(&vil->bound);
        pos_emit.y += 0.4f;
        pos_emit.x += 1.5f * vil->precalc.sin_faceang;
        pos_emit.z += 1.5f * vil->precalc.cos_faceang;
    }

    spd = vil->tmr_list[ZVIL_TDUR_EXTEND];
    if (!(spd < vil->tmr_list[ZVIL_TYMR_CURGOAL]))
    {
        spd = vil->tmr_list[ZVIL_TYMR_CURGOAL];
    }
    spd = vil->spd_extend * spd;
    if (!(spd < vil->dst_extend))
    {
        spd = vil->dst_extend;
    }
    rad = (0.01f > spd) ? 0.01f : spd;
    fac_wid = vil->wid_extend / vil->dst_extend;

    yaw = vil->frame->rot.angle;
    dir_ext.x = isin(yaw);
    dir_ext.y = 0.0f;
    dir_ext.z = icos(yaw);
    yaw = vil->frame->rot.angle;

    if (pos_tgt)
    {
        xVec3Sub(&vec_tgt, pos_tgt, &pos_emit);
        xVec3Normalize(&dir_tgt, &vec_tgt);
        pitch = asin(dir_tgt.y);
        if (vil->asset->subtype == eEntVillainTypeKillerPlant)
        {
            pitch = 0.0f;
        }
        else
        {
            float hi = (pitch < 0.25f) ? pitch : 0.25f;
            if (-0.25f > hi)
            {
                pitch = -0.25f;
            }
            else if (!(pitch < 0.25f))
            {
                pitch = 0.25f;
            }
        }
        xMat3x3Euler(&mat_rot, 0.0f, pitch, 0.0f);
        xMat3x3LMulVec(&dir_ext, &mat_rot, (_xVec3*)&g_Z3);
        xMat3x3Euler(&mat_rot, -yaw, 0.0f, 0.0f);
        xMat3x3LMulVec(&dir_ext, &mat_rot, &dir_ext);
    }

    if (vil->asset->subtype == eEntVillainTypeKillerPlant)
    {
        if (sEmitKillerPlant)
        {
            _zParEmitterCustomSettings pecust;
            _xVec3 dir_vel;
            _xMat4x3* mat = (_xMat4x3*)vil->model->Mat;
            float spd_vel;

            pecust.pos = mat->pos;
            pecust.pos.y += 1.6f;
            pecust.pos.x += 0.25f * mat->at.x;
            pecust.pos.z += 0.25f * mat->at.z;
            xVec3Normalize(&dir_vel, &dir_ext);
            pecust.custom_flags = 0x60;
            spd_vel = 0.25f * iurand() + 1.0f;
            pecust.vel.x = dir_vel.x * spd_vel;
            pecust.vel.y = dir_vel.y * spd_vel;
            pecust.vel.z = dir_vel.z * spd_vel;
            zParEmitterEmitCustom(sEmitKillerPlant, dt, &pecust);
        }
    }
    else
    {
        VPAR_emit_extattack(vil, &pos_emit, &dir_ext, rad, fac_wid, dt);
    }
    return 0;
}

static zEntProjectile* VWEP_lobReservePRJ(st_zEntVillain* vil)
{
    en_ZPRJ_CATEGORY cat;
    en_ZPRJ_CATEGORY prefer;
    en_ZVMM_FLASK_TYPE typ_flask = MAST_FLSK_SMOG;
    zEntProjectile* prj;

    switch (vil->asset->subtype)
    {
    case 5:
    case 0x10:
        prefer = ZPRJ_CAT_GRAVBALL;
        break;
    case 0x12:
        prefer = ZPRJ_CAT_ARROW;
        break;
    case eEntVillainBossBlackKnight:
        prefer = ZPRJ_CAT_BKAXE;
        break;
    case eEntVillainBossGreenGhost:
        prefer = ZPRJ_CAT_GGSHOCK;
        break;
    case eEntVillainBossRedbeard:
        prefer = ZPRJ_CAT_RBSWORD;
        break;
    case eEntVillainBossMastermind:
        prefer = ZPRJ_CAT_MMFLASK;
        typ_flask = VPRJ_BMM_select_flasktype(vil);
        break;
    default:
        xSceneID2Name(globals.sceneCur, vil->id);
        prefer = ZPRJ_CAT_ARROW;
        break;
    }

    cat = zEntProjectile_catFromPrjType(vil->prjTypeID, prefer);
    prj = zEntProjectile_acquire(cat, vil->prjTypeID);
    if (!prj)
    {
        return NULL;
    }
    if (cat == ZPRJ_CAT_MMFLASK)
    {
        ((st_ZVMM_FLASK_DATA*)prj->xtradata)->typ_flask = typ_flask;
    }
    prj->owner = vil;
    prj->ownerContext = NULL;
    prj->ownerNotify = VWEP_prjOwnerNotify;
    return prj;
}

static signed int VWEP_lobActivatePRJ(zEntProjectile* prj, st_zEntVillain* vil, _xVec3* pos_tgt, _xVec3* vec_off,
                                      _xVec3* vec_spin, float arcness, float heavy)
{
    // Unidentified zeroed locals of the original (only their templates remain).
    float unused_pad[11] = {};
    signed int rc = 0;

    if (!prj)
    {
        return 0;
    }

    switch (vil->prj_reserved->cat_proj)
    {
    case ZPRJ_CAT_ARROW:
        rc = VWEP_lobInitArrow(vil, pos_tgt, vec_off);
        prj->flg_prop |= 0x30000014;
        break;
    case ZPRJ_CAT_GRAVBALL:
    case ZPRJ_CAT_BOUNCEBALL:
        rc = VWEP_lobInitArcing(vil, pos_tgt, vec_off, vec_spin, arcness, heavy);
        prj->flg_prop |= 0x30000014;
        if (vil->asset->subtype == 5 || vil->asset->subtype == 0x10)
        {
            prj->flags &= ~1;
        }
        break;
    case ZPRJ_CAT_BKAXE:
        rc = VWEP_lobInitBKAxe(vil, pos_tgt, vec_off, vec_spin, arcness, heavy);
        break;
    case ZPRJ_CAT_GGSHOCK:
        rc = VWEP_lobInitGGQuaker(vil, pos_tgt, vec_off, vec_spin, arcness, heavy);
        break;
    case ZPRJ_CAT_RBSWORD:
        rc = VWEP_lobInitRBSword(vil, pos_tgt, vec_off, vec_spin, arcness, heavy);
        break;
    case ZPRJ_CAT_MMFLASK:
        rc = VWEP_lobInitMMFlask(vil, pos_tgt, vec_off, vec_spin, arcness, heavy);
        break;
    }

    if (rc)
    {
        prj->StateSet((en_ZPRJ_STATES)2);
        if (prj->cat_proj != ZPRJ_CAT_GGSHOCK && prj->cat_proj != ZPRJ_CAT_MMFLASK)
        {
            prj->SendEvent(ZPRJ_EVNT_LAUNCH);
        }
    }
    else
    {
        prj->Abort();
    }

    if (prj->cat_proj != ZPRJ_CAT_GGSHOCK && prj->cat_proj != ZPRJ_CAT_MMFLASK)
    {
        vil->prj_reserved = NULL;
    }
    return 1;
}

static signed int VWEP_prjOwnerNotify(zEntProjectile* prj, en_PROJECTILE_EVENT event, void* ctxt)
{
    signed int rc = 0;
    st_zEntVillain* vil = (st_zEntVillain*)prj->owner;
    signed int dosnd = (vil->ability & 1) ^ 1;

    switch (event)
    {
    case ZPRJ_EVNT_LAUNCH:
        if (dosnd)
        {
            VPRJ_prjsound(vil, prj, VSND_TYP_PRJ_LAUNCH);
        }
        rc = 1;
        break;
    case ZPRJ_EVNT_HITPLYR:
        VUTIL_stimulate(vil, (en_NPC_STIMULUS)0x19);
        if (dosnd)
        {
            VPRJ_prjsound(vil, prj, VSND_TYP_PRJ_EXPLODE);
        }
        rc = 1;
        break;
    case ZPRJ_EVNT_INFLIGHT:
        if (dosnd && !iSndIsPlaying(0, (unsigned int)prj))
        {
            VPRJ_prjsound(vil, prj, VSND_TYP_PRJ_FLIGHT);
        }
        rc = 1;
        break;
    case ZPRJ_EVNT_DETONATE:
        if (dosnd)
        {
            xSndStopChildren((unsigned int)prj);
            VPRJ_prjsound(vil, prj, VSND_TYP_PRJ_SPLAT);
        }
        rc = 1;
        break;
    case ZPRJ_EVNT_TRANSTATE:
        if (prj->state != 2)
        {
            xSndStopChildren((unsigned int)prj);
        }
        rc = 1;
        break;
    case ZPRJ_EVNT_ABORT:
    case ZPRJ_EVNT_SELFDESTRUCT:
    case ZPRJ_EVNT_DEATH:
        xSndStopChildren((unsigned int)prj);
        if (vil->prj_reserved == prj)
        {
            vil->prj_reserved = NULL;
        }
        rc = 1;
        break;
    }
    return rc;
}

static inline void VWEP_lobLaunchPos(st_zEntVillain* vil, _xVec3* pos)
{
    _xVec3 vec_tmp = {};

    if (!VUTIL_tagpos(vil, 6, pos))
    {
        xSceneID2Name(globals.sceneCur, vil->id);
        *pos = *xBoundCenter(&vil->bound);
        xVec3SMul(&vec_tmp, &((_xMat4x3*)vil->model->Mat)->at, 0.5f);
        vec_tmp.y += 0.5f;
        xVec3AddTo(pos, &vec_tmp);
    }
}

static inline signed int VWEP_lobInitArrow(st_zEntVillain* vil, _xVec3* pos_tgt, _xVec3* vec_off)
{
    _xVec3 pos = {};
    ST_CAT_AIMDATA aiminf = {};
    _xVec3 tmp_vec = {};
    zEntProjectile* prj = vil->prj_reserved;

    prj->ColDelaySet(0.4f, 1);
    VWEP_lobLaunchPos(vil, &pos);

    aiminf.spd_base = vil->spd_lob;
    aiminf.dir_base = ((_xMat4x3*)vil->model->Mat)->at;
    aiminf.arrow.pos_src = pos;
    if (vec_off)
    {
        xVec3Add(&tmp_vec, pos_tgt, vec_off);
        aiminf.arrow.pos_tgt = tmp_vec;
    }
    else
    {
        aiminf.arrow.pos_tgt = *pos_tgt;
    }
    return prj->AimAtTarget(&aiminf, 1);
}

static signed int VWEP_lobInitArcing(st_zEntVillain* vil, _xVec3* pos_tgt, _xVec3* vec_off, _xVec3* vec_spin,
                                     float arcness, float heavy)
{
    _xMat4x3* mat;
    zEntProjectile* prj;
    _xVec3 vec_tmp = {};
    ST_CAT_AIMDATA aiminf = { ZPRJ_CAIM_GRAVBALL };
    _xVec3 pos = {};
    signed int retval = 1;
    prj = vil->prj_reserved;

    aiminf.idx_caim = ZPRJ_CAIM_GRAVBALL;
    prj->ColDelaySet(0.4f, 1);
    VWEP_lobLaunchPos(vil, &pos);

    if (vil->asset->subtype == eEntVillainBossBlackKnight)
    {
        mat = (_xMat4x3*)vil->model->Mat;
        xVec3SMul(&vec_tmp, &mat->up, 2.0f);
        xVec3AddTo(&pos, &vec_tmp);
        xVec3SMul(&vec_tmp, &mat->right, -1.5f);
        xVec3AddTo(&pos, &vec_tmp);
    }

    aiminf.spd_base = vil->spd_lob;
    aiminf.dir_base = ((_xMat4x3*)vil->model->Mat)->at;
    aiminf.grav.pos_src = pos;
    if (vec_off)
    {
        xVec3Add(&aiminf.grav.pos_tgt, pos_tgt, vec_off);
    }
    else
    {
        aiminf.grav.pos_tgt = *pos_tgt;
    }
    aiminf.grav.fac_arcness = (arcness <= 0.0f) ? 1.0f : arcness;
    aiminf.grav.acc_gravity = (heavy <= 0.0f) ? 1.5f : heavy;

    if (vec_spin)
    {
        prj->FlexMatSet(vec_spin, globals.update_dt);
        prj->flg_categ |= 0x4000000;
    }
    else
    {
        prj->flg_categ &= ~0x4000000;
    }

    if (!prj->AimAtTarget(&aiminf, 1))
    {
        retval = 0;
    }
    return retval;
}

static signed int VWEP_lobInitBKAxe(st_zEntVillain* vil, _xVec3* pos_tgt, _xVec3* vec_off, _xVec3* vec_spin,
                                    float arcness, float heavy)
{
    zEntProjectile* prj = vil->prj_reserved;
    _xVec3 ang_flip = { 0.0f, 0.0f, 1.5707964f };
    _xMat3x3 mat_rot = {};
    _xVec3 delta = {};
    _xVec3 ang_yaw = {};
    st_BKAXE_DATA* axfx = (st_BKAXE_DATA*)prj->xtradata;
    _xMat4x3* frame;

    VWEP_lobInitArcing(vil, pos_tgt, vec_off, vec_spin, arcness, heavy);

    xMat3x3Euler(&mat_rot, &ang_flip);
    xMat4x3RMul3x3((_xMat4x3*)prj->frame, &mat_rot, (_xMat4x3*)prj->frame);

    frame = (_xMat4x3*)prj->frame;
    delta.x = frame->pos.x - pos_tgt->x;
    delta.y = frame->pos.y - pos_tgt->y;
    delta.z = frame->pos.z - pos_tgt->z;
    ang_yaw.y = xAngleClamp(atan2(delta.x, delta.z));
    xMat3x3Euler(&mat_rot, &ang_yaw);
    xMat4x3RMul3x3((_xMat4x3*)prj->frame, &mat_rot, (_xMat4x3*)prj->frame);
    prj->frame->mode |= 0x101;
    prj->SyncFrame(1);

    axfx->rad_zap = vil->dst_extend;
    axfx->dst_zapdrop = vil->wid_extend;
    axfx->tym_zapdur = vil->spd_extend;
    axfx->planes.y = vil->dst_lob;
    axfx->planes.z = -268.5f;
    axfx->planes.x = 46.0f;

    if (vec_spin)
    {
        prj->FlexMatSet(vec_spin, globals.update_dt);
        prj->flg_categ |= 0x4000000;
    }
    else
    {
        prj->flg_categ &= ~0x4000000;
    }

    prj->ds2_maxlife = 10000.0f;
    return 1;
}

static signed int VWEP_lobInitGGQuaker(st_zEntVillain* vil, _xVec3* pos_tgt, _xVec3* vec_off, _xVec3* vec_spin,
                                       float arcness, float heavy)
{
    zEntProjectile* prj = vil->prj_reserved;
    st_BGG_QUAKEBALL* quake = (st_BGG_QUAKEBALL*)prj->xtradata;

    VWEP_lobInitArrow(vil, pos_tgt, vec_off);
    prj->cat_data.gravball.acc_gravity = (heavy <= 0.0f) ? 1.5f : heavy;
    quake->rad_build = vil->dst_lob;
    quake->spd_flight = (vil->spd_lob > 0.2f) ? vil->spd_lob : 0.2f;
    quake->hyt_shock = vil->wid_extend;
    quake->rad_shock = vil->dst_extend;
    quake->tym_shock = vil->spd_extend;
    return 1;
}

static signed int VWEP_lobInitRBSword(st_zEntVillain* vil, _xVec3* pos_tgt, _xVec3* vec_off, _xVec3* vec_spin,
                                      float arcness, float heavy)
{
    zEntProjectile* prj = vil->prj_reserved;
    _xVec3 vec_pos = {};
    st_REDBEARD_SWORD* rbs = (st_REDBEARD_SWORD*)prj->xtradata;
    signed int rc;

    rc = VUTIL_tagpos(vil, 10, &vec_pos);
    if (!rc)
    {
        rc = VUTIL_tagpos(vil, 6, &vec_pos);
    }
    if (!rc)
    {
        rc = VUTIL_tagpos(vil, 5, &vec_pos);
    }
    if (!rc)
    {
        rc = VUTIL_tagpos(vil, 9, &vec_pos);
    }
    if (!rc)
    {
        vec_pos = *xBoundCenter(&vil->bound);
    }

    ((_xMat4x3*)prj->frame)->pos = vec_pos;
    xMat3x3Euler((_xMat3x3*)prj->frame, 0.0f, 0.0f, 3.1415927f);
    prj->frame->mode |= 0x101;
    prj->SyncFrame(1);

    rbs->dst_far = vil->dst_extend;
    rbs->spd_fly = (1.999999f * vil->dst_extend) / 5.0f;
    rbs->dir_end = vil->precalc.dir_VtoP;
    xVec3SMul(&rbs->pos_end, &rbs->dir_end, rbs->dst_far);
    return 1;
}

static signed int VWEP_lobInitMMFlask(st_zEntVillain* vil, _xVec3* pos_tgt, _xVec3* vec_off, _xVec3* vec_spin,
                                      float arcness, float heavy)
{
    static _xVec3 ang_tilt = { 0.0f, 0.0f, 0.7853982f };
    zEntProjectile* prj = vil->prj_reserved;
    _xMat3x3 mat_rot = {};
    st_ZVMM_FLASK_DATA* flask = (st_ZVMM_FLASK_DATA*)prj->xtradata;

    VUTIL_tagpos(vil, 6, &((_xMat4x3*)prj->frame)->pos);
    xMat3x3Euler(&mat_rot, &ang_tilt);
    xMat4x3RMul3x3((_xMat4x3*)prj->frame, &mat_rot, (_xMat4x3*)prj->frame);
    prj->frame->mode |= 0x101;
    prj->SyncFrame(1);

    if (vec_spin)
    {
        prj->FlexMatSet(vec_spin, globals.update_dt);
        prj->flg_categ |= 0x4000000;
    }
    else
    {
        prj->flg_categ &= ~0x4000000;
    }

    flask->stage = MAST_STAGE_TEASE;
    flask->spd_flight = (vil->spd_lob > 0.2f) ? vil->spd_lob : 0.2f;
    flask->hyt_aftermath = vil->wid_extend;
    flask->tym_aftermath = vil->wid_extend;
    flask->rad_aftermath = vil->wid_extend;
    if (arcness <= 0.0f)
    {
        flask->arcness = 1.0f;
    }
    else
    {
        flask->arcness = arcness;
    }
    if (heavy <= 0.0f)
    {
        flask->heavy = 1.5f;
    }
    else
    {
        flask->heavy = heavy;
    }
    return 1;
}

static inline void VPRJ_startup()
{
    typedef signed int (*LogixFn)(zEntProjectile*, float, zScene*, _xEntFrame*);
    typedef signed int (*NotifyFn)(zEntProjectile*, en_PROJECTILE_EVENT, void*);
    st_ZPRJ_CATEGORY_FUNCS fn_bbk = { 1, VPRJ_BBK_attach, VPRJ_BBK_detach, (LogixFn)VPRJ_BBK_logix,
                                      VPRJ_BBK_prjmove, (NotifyFn)VPRJ_BBK_notify, NULL,
                                      zEntProjectile_gen_aimer, NULL, NULL };
    st_ZPRJ_CATEGORY_FUNCS fn_bgg = { 1, VPRJ_BGG_attach, VPRJ_BGG_detach, (LogixFn)VPRJ_BGG_logix,
                                      NULL, (NotifyFn)VPRJ_BGG_notify, NULL,
                                      zEntProjectile_gen_aimer, NULL, NULL };
    st_ZPRJ_CATEGORY_FUNCS fn_brb = { 1, VPRJ_BRB_attach, VPRJ_BRB_detach, (LogixFn)VPRJ_BRB_logix,
                                      NULL, (NotifyFn)VPRJ_BRB_notify, NULL,
                                      zEntProjectile_gen_aimer, NULL, NULL };
    st_ZPRJ_CATEGORY_FUNCS fn_bmm = { 1, VPRJ_BMM_attach, VPRJ_BMM_detach, VPRJ_BMM_logix,
                                      NULL, (NotifyFn)VPRJ_BMM_notify, NULL,
                                      zEntProjectile_gen_aimer, NULL, NULL };
    st_ZPRJ_CATEGORY_PROPS pr_bbk = { 0x3D, 0x03, 0x3F, 0x00, 0, 0 };
    st_ZPRJ_CATEGORY_PROPS pr_bgg = { 0x30000031, 0x03, 0x3F, 0x3F, 0, 0 };
    st_ZPRJ_CATEGORY_PROPS pr_brb = { 0x3D, 0x03, 0x14, 0x3F, 0, 0 };
    st_ZPRJ_CATEGORY_PROPS pr_bmm = { 0x3000003D, 0x03, 0x3F, 0x3F, 0, 0 };
    // Unreferenced in the original as well (only the templates remain).
    _xVec3 unused_spin_a = { 0.0f, 0.0f, 1.5707964f };
    _xVec3 unused_spin_b = { 0.0f, 0.0f, 1.5707964f };
    _xVec3 unused_spin_c = {};

    zEntProjectile_addCat(ZPRJ_CAT_BKAXE, &fn_bbk);
    zEntProjectile_addProps(ZPRJ_CAT_BKAXE, &pr_bbk);
    zEntProjectile_addCat(ZPRJ_CAT_GGSHOCK, &fn_bgg);
    zEntProjectile_addProps(ZPRJ_CAT_GGSHOCK, &pr_bgg);
    zEntProjectile_addCat(ZPRJ_CAT_RBSWORD, &fn_brb);
    zEntProjectile_addProps(ZPRJ_CAT_RBSWORD, &pr_brb);
    zEntProjectile_addCat(ZPRJ_CAT_MMFLASK, &fn_bmm);
    zEntProjectile_addProps(ZPRJ_CAT_MMFLASK, &pr_bmm);
}

static void VPRJ_BBK_bupdate(_xEnt* ent, _xVec3* pos)
{
    ent->bound.sph.center = *pos;
    ent->bound.sph.r = 0.75f;
    xBoundUpdate(&ent->bound);
}

static signed int VPRJ_BBK_attach(zEntProjectile* prj)
{
    signed int i;

    prj->bupdate = VPRJ_BBK_bupdate;
    prj->cat_data.idx_cdat = 2;
    prj->OrientSet(0);
    prj->ColDelaySet(50.0f, 1);
    prj->tym_maxlife = VIL_MAX(30.0f, prj->tym_maxlife);
    for (i = 0; i < 10; i++)
    {
        st_BKAXE_DATA* axfx = &g_axfxpool[i];
        if (!(axfx->flg_stat & 1))
        {
            memset(axfx, 0, sizeof(st_BKAXE_DATA));
            axfx->flg_stat |= 1;
            prj->xtradata = axfx;
            break;
        }
    }
    if (prj->xtradata)
    {
        return 1;
    }
    return 0;
}

static void VPRJ_BBK_detach(zEntProjectile* prj)
{
    st_BKAXE_DATA* axfx = (st_BKAXE_DATA*)prj->xtradata;

    if (axfx)
    {
        VPRJ_zapTheAxe(prj, 1);
        memset(axfx, 0, sizeof(st_BKAXE_DATA));
        prj->xtradata = NULL;
    }
}

static signed int VPRJ_BBK_logix(zEntProjectile* prj, float dt)
{
    _xVec3 pos_prj = {};
    st_BKAXE_DATA* axfx = (st_BKAXE_DATA*)prj->xtradata;
    st_ZPRJ_CAT_BOUNCEBALL* bounce = &prj->cat_data.bounce;

    if (bounce->atRest)
    {
        prj->SendEvent(ZPRJ_EVNT_ATREST, dt);
    }
    else
    {
        prj->SendEvent(ZPRJ_EVNT_INFLIGHT, dt);
    }

    pos_prj = ((_xMat4x3*)prj->model->Mat)->pos;
    if (prj->tmr_alive >= prj->tym_maxlife)
    {
        prj->SendEvent(ZPRJ_EVNT_SELFDESTRUCT);
        prj->StateSet((en_ZPRJ_STATES)3);
        return 1;
    }

    if (pos_prj.z > axfx->planes.z || pos_prj.y < axfx->planes.x)
    {
        prj->ColDelaySet(-1.0f, 0);
        prj->flg_pen = 0x7E;
    }
    else
    {
        prj->ColDelaySet(5.0f, 0);
    }

    if (pos_prj.y < axfx->planes.y)
    {
        prj->StateSet((en_ZPRJ_STATES)3);
    }

    if (prj->plyr_colrec->flags & 1)
    {
        if (prj->flg_prop & 0x10)
        {
            zEntPlayer_Damage(prj, 1);
        }
        prj->SendEvent(ZPRJ_EVNT_DETONATE, dt);
        prj->StateSet((en_ZPRJ_STATES)3);
    }
    else if ((bounce->atRest == 1 || (prj->collis->colls[0].flags & 1)) && bounce->atRest == 0)
    {
        prj->SendEvent(ZPRJ_EVNT_STARTRESTING);
        prj->tmr_curstate = 0.0f;
        bounce->atRest = 1;
        bounce->acc_gravity = 0.0f;
        VPRJ_zapTheAxe(prj, 0);
        prj->frame->dvel.x = 0.0f;
        prj->frame->dvel.y = 0.0f;
        prj->frame->dvel.z = 0.0f;
        prj->frame->vel.x = 0.0f;
        prj->frame->vel.y = 0.0f;
        prj->frame->vel.z = 0.0f;
        prj->frame->mode |= 0xC;
        axfx->flg_stat |= 2;
    }

    if (bounce->atRest == 1 && prj->tmr_curstate > axfx->tym_zapdur)
    {
        prj->SendEvent(ZPRJ_EVNT_DETONATE);
        prj->StateSet((en_ZPRJ_STATES)3);
    }
    return 1;
}

static signed int VPRJ_BBK_prjmove(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame)
{
    st_BKAXE_DATA* axfx = (st_BKAXE_DATA*)prj->xtradata;
    st_ZPRJ_CAT_BOUNCEBALL* bounce = &prj->cat_data.bounce;

    if (bounce->atRest != 1)
    {
        frame->dvel.x = 0.0f;
        frame->dvel.y = -(dt * bounce->acc_gravity);
        frame->dvel.z = 0.0f;
        frame->mode |= 8;
        if (prj->flg_categ & 0x4000000)
        {
            xMat4x3RMul3x3((_xMat4x3*)frame, &prj->common.mat_flex, (_xMat4x3*)frame);
            prj->frame->mode |= 0x100;
        }
    }
    else
    {
        if (((_xMat4x3*)frame)->at.y > -0.25f)
        {
            xMat4x3RMul3x3((_xMat4x3*)frame, &prj->common.mat_flex, (_xMat4x3*)frame);
            prj->frame->mode |= 0x100;
        }
        if ((axfx->flg_stat & 2) && !(prj->frame->mode & 0x100))
        {
            axfx->flg_stat &= ~2;
        }
    }
    return 1;
}

static signed int VPRJ_BBK_notify(zEntProjectile* prj, en_PROJECTILE_EVENT event)
{
    st_zEntVillain* vil = (st_zEntVillain*)prj->owner;

    switch (event)
    {
    case ZPRJ_EVNT_LAUNCH:
        VPRJ_prjsound(vil, prj, VSND_TYP_PRJ_LAUNCH);
        break;
    case ZPRJ_EVNT_INFLIGHT:
        if (!iSndIsPlaying(0, (unsigned int)prj))
        {
            VPRJ_prjsound(vil, prj, VSND_TYP_PRJ_FLIGHT);
        }
        break;
    case ZPRJ_EVNT_STARTRESTING:
        xSndStopChildren((unsigned int)prj);
        VPRJ_prjsound(vil, prj, VSND_TYP_PRJ_SPLAT);
        xSndPlay3D(xStrHash("bk_shock"), 1.0f, 0.0f, 0x80, 0x202, prj, 25.0f);
    case ZPRJ_EVNT_ATREST:
        if (!iSndIsPlaying(0, (unsigned int)prj))
        {
            xSndPlay3D(xStrHash("bk_shock"), 1.0f, 0.0f, 0x80, 0x202, prj, 25.0f);
        }
        break;
    case ZPRJ_EVNT_HITPLYR:
        VUTIL_stimulate(vil, (en_NPC_STIMULUS)0x19);
        VPRJ_prjsound(vil, prj, VSND_TYP_PRJ_EXPLODE);
        break;
    case ZPRJ_EVNT_TRANSTATE:
    case ZPRJ_EVNT_DEATH:
        if (prj->state != 2)
        {
            xSndStopChildren((unsigned int)prj);
        }
        break;
    }
    return 1;
}

static void VPRJ_zapTheAxe(zEntProjectile* prj, signed int cleanup)
{
    st_BKAXE_DATA* axfx = (st_BKAXE_DATA*)prj->xtradata;
    _xVec3 loca = {};
    _xVec3 from = {};
    signed int i;
    _xVec3* pos;

    for (i = 0; i < 8; i++)
    {
        if (axfx->strk[i])
        {
            zLightningKill(axfx->strk[i]);
            axfx->strk[i] = NULL;
        }
    }

    if (cleanup)
    {
        return;
    }

    pos = &((_xMat4x3*)prj->model->Mat)->pos;
    loca = *pos;
    loca.y += 50.0f;
    axfx->strk[0] = VEFX_zapper_type_B(pos, &loca, axfx->tym_zapdur);
    axfx->strk[1] = VEFX_zapper_type_A(&((_xMat4x3*)prj->model->Mat)->pos, &loca, axfx->tym_zapdur);

    from = ((_xMat4x3*)prj->model->Mat)->pos;
    from.y += 1.0f;
    for (i = 2; i <= 4; i++)
    {
        loca = ((_xMat4x3*)prj->model->Mat)->pos;
        loca.x += axfx->rad_zap * (2.0f * (iurand() - 0.5f));
        loca.z += axfx->rad_zap * (2.0f * (iurand() - 0.5f));
        loca.y -= axfx->dst_zapdrop;
        axfx->strk[i] = VEFX_zapper_type_B(&from, &loca, axfx->tym_zapdur);
    }
}

static signed int VPRJ_BGG_attach(zEntProjectile* prj)
{
    signed int i;

    prj->cat_data.idx_cdat = 0;
    prj->flags &= ~1;
    prj->flg_prop &= ~8;
    for (i = 0; i < 4; i++)
    {
        st_BGG_QUAKEBALL* quake = &g_quakepool[i];
        if (!(quake->flg_stat & 1))
        {
            memset(quake, 0, sizeof(st_BGG_QUAKEBALL));
            quake->flg_stat |= 1;
            quake->phase = BGG_PHAZ_BUILDUP;
            prj->xtradata = quake;
            break;
        }
    }
    return prj->xtradata != NULL;
}

static void VPRJ_BGG_detach(zEntProjectile* prj)
{
    if (prj->xtradata)
    {
        memset(prj->xtradata, 0, sizeof(st_BGG_QUAKEBALL));
        prj->xtradata = NULL;
    }
}

static signed int VPRJ_BGG_logix(zEntProjectile* prj, float dt)
{
    signed int start_fall;
    float dist;
    _xVec3 dir_delt = {};
    _xVec3 vec_delt = {};
    _xVec3 pos_ryht = {};
    _xVec3 pos_left = {};
    _xVec3 vec_tmp = {};
    st_zEntVillain* vil;
    st_BGG_QUAKEBALL* quake;
    _xEntCollis* collis;

    quake = (st_BGG_QUAKEBALL*)prj->xtradata;
    vil = (st_zEntVillain*)prj->owner;

    if (prj->tmr_alive > prj->tym_maxlife)
    {
        prj->StateSet((en_ZPRJ_STATES)3);
        return 1;
    }
    if ((prj->plyr_colrec->flags & 1) && (prj->flg_prop & 0x10))
    {
        zEntPlayer_Damage(prj, 1);
        prj->StateSet((en_ZPRJ_STATES)3);
        return 1;
    }

    switch (quake->phase)
    {
    case BGG_PHAZ_BUILDUP:
        VPRJ_emitat(sEmitGreenBall, prj, dt, 3.0f);
        prj->SendEvent((en_PROJECTILE_EVENT)0xF, dt);
        prj->ColDelaySet(0.4f, 0);
        VUTIL_tagpos(vil, 6, &pos_left);
        VUTIL_tagpos(vil, 7, &pos_ryht);
        xVec3Add(&vec_tmp, &pos_left, &pos_ryht);
        xVec3SMul(&((_xMat4x3*)prj->frame)->pos, &vec_tmp, 0.5f);
        prj->frame->mode |= 1;
        break;
    case BGG_PHAZ_FLY:
        prj->SendEvent(ZPRJ_EVNT_INFLIGHT, dt);
        VPRJ_emitat(sEmitGreenBall, prj, dt, 1.5f);
        start_fall = 0;
        xVec3Sub(&vec_delt, &quake->pos_tgt, &((_xMat4x3*)prj->model->Mat)->pos);
        if (quake->flg_stat & 2)
        {
            quake->flg_stat &= ~2;
            prj->tmr_curstate = 0.0f;
            dist = xVec3Normalize(&dir_delt, &vec_delt);
            quake->tym_flightMax = (0.2f + dist) / quake->spd_flight;
            xVec3SMul(&prj->frame->vel, &dir_delt, quake->spd_flight);
            prj->frame->mode |= 4;
        }
        collis = prj->collis;
        if (prj->tmr_curstate > quake->tym_flightMax)
        {
            start_fall = 1;
        }
        else if (vec_delt.x * vec_delt.x + vec_delt.y * vec_delt.y + vec_delt.z * vec_delt.z < 0.0625f)
        {
            start_fall = 1;
        }
        else if (collis->env_sidx < collis->env_eidx || collis->npc_sidx < collis->npc_eidx ||
                 collis->stat_sidx < collis->stat_eidx || collis->dyn_sidx < collis->dyn_eidx)
        {
            start_fall = 1;
        }
        if (start_fall)
        {
            quake->flg_stat |= 2;
            quake->phase = BGG_PHAZ_FALL;
        }
        break;
    case BGG_PHAZ_FALL:
        prj->SendEvent(ZPRJ_EVNT_INFLIGHT, dt);
        VPRJ_emitat(sEmitGreenBall, prj, dt, 0.0f);
        if (quake->flg_stat & 2)
        {
            quake->flg_stat &= ~2;
            prj->tmr_curstate = 0.0f;
            prj->frame->vel.x = 0.0f;
            prj->frame->vel.z = 0.0f;
            prj->frame->mode |= 4;
        }
        if (prj->collis->colls[0].flags & 1)
        {
            quake->flg_stat |= 2;
            quake->phase = BGG_PHAZ_SHOCKWAVE;
        }
        else
        {
            prj->frame->dvel.x = 0.0f;
            prj->frame->dvel.y = -(dt * prj->cat_data.gravball.acc_gravity);
            prj->frame->dvel.z = 0.0f;
            prj->frame->mode |= 8;
        }
        break;
    case BGG_PHAZ_SHOCKWAVE:
        if (quake->flg_stat & 2)
        {
            quake->flg_stat &= ~2;
            prj->tmr_curstate = 0.0f;
            prj->frame->dvel.x = 0.0f;
            prj->frame->dvel.y = 0.0f;
            prj->frame->dvel.z = 0.0f;
            prj->frame->vel.x = 0.0f;
            prj->frame->vel.y = 0.0f;
            prj->frame->vel.z = 0.0f;
            prj->frame->mode |= 0xC;
            prj->flg_chk &= ~0x10;
            prj->flg_pen &= ~0x10;
            VPRJ_emitat(sEmitGreenBallSmack, prj, dt, 0.0f);
            VPRJ_BGG_shockpos(prj, dt);
            prj->SendEvent(ZPRJ_EVNT_DETONATE, dt);
        }
        else
        {
            prj->SendEvent((en_PROJECTILE_EVENT)0x10, dt);
        }
        VPRJ_BGG_shockwave(prj, dt);
        if (prj->tmr_curstate > quake->tym_shock)
        {
            prj->StateSet((en_ZPRJ_STATES)3);
        }
        break;
    }
    return 1;
}

static signed int VPRJ_BGG_notify(zEntProjectile* prj, en_PROJECTILE_EVENT event)
{
    st_zEntVillain* vil = (st_zEntVillain*)prj->owner;

    switch (event)
    {
    case ZPRJ_EVNT_HITPLYR:
        VUTIL_stimulate(vil, (en_NPC_STIMULUS)0x19);
        break;
    case ZPRJ_EVNT_LAUNCH:
        VPRJ_prjsound(vil, prj, VSND_TYP_PRJ_LAUNCH);
        break;
    case ZPRJ_EVNT_INFLIGHT:
        if (!iSndIsPlaying(0, (unsigned int)prj))
        {
            VPRJ_prjsound(vil, prj, VSND_TYP_PRJ_FLIGHT);
        }
        break;
    case ZPRJ_EVNT_DETONATE:
        if (!iSndIsPlaying(0, (unsigned int)prj))
        {
            VPRJ_prjsound(vil, prj, VSND_TYP_PRJ_EXPLODE);
        }
        break;
    case (en_PROJECTILE_EVENT)0x10:
        if (!iSndIsPlaying(0, (unsigned int)prj))
        {
            VPRJ_prjsound(vil, prj, VSND_TYP_PRJ_SPLAT);
        }
        break;
    case ZPRJ_EVNT_TRANSTATE:
    case ZPRJ_EVNT_ABORT:
    case ZPRJ_EVNT_SELFDESTRUCT:
    case ZPRJ_EVNT_DEATH:
        if (prj->state != 2)
        {
            xSndStopChildren((unsigned int)prj);
        }
        break;
    }
    return 1;
}

static signed int VPRJ_BGG_shockwave(zEntProjectile* prj, float dt)
{
    float inner;
    float outer;
    _xVec3 pos_prj = {};
    _xVec3 pos_plyr = {};
    _xVec3 delta = {};
    float fb;
    float br;
    st_BGG_QUAKEBALL* quake = (st_BGG_QUAKEBALL*)prj->xtradata;
    _zParEmitterCustomSettings pecust;
    float d2;
    float hh;

    if (quake->tym_shock < 0.1f)
    {
        return 0;
    }

    outer = quake->rad_shock * (prj->tmr_curstate / quake->tym_shock);
    pos_prj = ((_xMat4x3*)prj->model->Mat)->pos;
    pos_plyr = ((_xMat4x3*)globals.player.ent.model->Mat)->pos;
    xVec3Sub(&delta, &pos_plyr, &pos_prj);
    inner = 0.85f * outer;
    br = globals.player.ent.bound.sph.r;

    pecust.custom_flags = 0x20020;
    pecust.xzcircle_radius = outer;
    pecust.pos = g_bgg_prj_pos;
    zParEmitterEmitCustom(sEmitGreenBallShockwave, dt, &pecust);

    fb = outer + br;
    d2 = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
    if (d2 > fb * fb)
    {
        return 1;
    }
    hh = 0.5f * quake->hyt_shock;
    if (pos_plyr.y - br > pos_prj.y + hh)
    {
        return 1;
    }
    if (pos_plyr.y + br < pos_prj.y - hh)
    {
        return 1;
    }
    if (inner > br && d2 < (inner - br) * (inner - br))
    {
        return 1;
    }
    return 0;
}

static inline void VPRJ_BGG_shockpos(zEntProjectile* prj, float dt)
{
    if (prj)
    {
        g_bgg_prj_pos = ((_xMat4x3*)prj->model->Mat)->pos;
    }
    else
    {
        _zParEmitterCustomSettings pecust;
        pecust.custom_flags = 0x20020;
        pecust.xzcircle_radius = 0.0f;
        pecust.pos = g_bgg_prj_pos;
        zParEmitterEmitCustom(sEmitGreenBallShockwave, dt, &pecust);
    }
}

static inline void VPRJ_emitat(_zParEmitter* emitter, zEntProjectile* prj, float dt, float yoff)
{
    _zParEmitterCustomSettings pecust;

    pecust.custom_flags = 0x20;
    pecust.pos = ((_xMat4x3*)prj->model->Mat)->pos;
    pecust.pos.y += yoff;
    zParEmitterEmitCustom(emitter, dt, &pecust);
}

static signed int VPRJ_BRB_attach(zEntProjectile* prj)
{
    signed int i;

    prj->cat_data.idx_cdat = 0;
    for (i = 0; i < 4; i++)
    {
        st_REDBEARD_SWORD* rbs = &g_rbsword[i];
        if (!(rbs->flg_stat & 1))
        {
            memset(rbs, 0, sizeof(st_REDBEARD_SWORD));
            rbs->flg_stat |= 1;
            prj->xtradata = rbs;
            break;
        }
    }
    return prj->xtradata != NULL;
}

static void VPRJ_BRB_detach(zEntProjectile* prj)
{
    if (prj->xtradata)
    {
        memset(prj->xtradata, 0, sizeof(st_REDBEARD_SWORD));
        prj->xtradata = NULL;
    }
}

static signed int VPRJ_BRB_logix(zEntProjectile* prj, float dt)
{
    // Unreferenced in the original as well (only the templates remain).
    _xVec3 unused_a = {};
    _xVec3 unused_b = {};
    st_ZPRJ_CAT_ARROW* arrow = &prj->cat_data.arrow;
    _xEntCollis* collis;

    if (arrow->amStuck)
    {
        prj->SendEvent(ZPRJ_EVNT_ATREST, dt);
    }
    else
    {
        prj->SendEvent(ZPRJ_EVNT_INFLIGHT, dt);
    }

    if (prj->tmr_alive > prj->tym_maxlife)
    {
        prj->StateSet((en_ZPRJ_STATES)3);
        return 1;
    }

    if (prj->plyr_colrec->flags & 1)
    {
        if (prj->flg_prop & 0x10)
        {
            zEntPlayer_Damage(prj, 1);
        }
        if (prj->flg_categ & 0x1000000)
        {
            if (!arrow->amStuck)
            {
                arrow->amStuck = 1;
                arrow->ent_stuck = &globals.player.ent;
                xVec3Sub(&arrow->pos_stuck, &((_xMat4x3*)arrow->ent_stuck->model->Mat)->pos,
                         &((_xMat4x3*)prj->model->Mat)->pos);
                prj->frame->vel.x = 0.0f;
                prj->frame->vel.y = 0.0f;
                prj->frame->vel.z = 0.0f;
                prj->OrientSet(0x10000000);
            }
        }
        else
        {
            prj->SendEvent(ZPRJ_EVNT_DETONATE, dt);
            prj->StateSet((en_ZPRJ_STATES)3);
        }
        return 1;
    }

    collis = prj->collis;
    if (collis->colls[0].flags & 1)
    {
        prj->SendEvent(ZPRJ_EVNT_DETONATE, dt);
        prj->StateSet((en_ZPRJ_STATES)3);
    }
    else if (collis->colls[1].flags & 1)
    {
        if (prj->flg_categ & 0x2000000)
        {
            arrow->amStuck = 2;
            prj->pflags |= 4;
            prj->frame->vel.x = 0.0f;
            prj->frame->vel.z = 0.0f;
            prj->frame->mode |= 4;
        }
        else
        {
            prj->SendEvent(ZPRJ_EVNT_DETONATE, dt);
            prj->StateSet((en_ZPRJ_STATES)3);
        }
    }
    else if (collis->env_sidx < collis->env_eidx || collis->stat_sidx < collis->stat_eidx ||
             collis->dyn_sidx < collis->dyn_eidx)
    {
        if (prj->flg_categ & 0x2000000)
        {
            arrow->amStuck = 2;
            prj->pflags |= 4;
            prj->frame->vel.x = 0.0f;
            prj->frame->vel.z = 0.0f;
            prj->frame->mode |= 4;
        }
        else if (prj->flg_categ & 0x1000000)
        {
            arrow->amStuck = 1;
            prj->pflags |= 4;
            prj->frame->vel.x = 0.0f;
            prj->frame->vel.y = 0.0f;
            prj->frame->vel.z = 0.0f;
            prj->frame->mode |= 4;
            prj->OrientSet(0);
        }
        else
        {
            prj->SendEvent(ZPRJ_EVNT_DETONATE, dt);
            prj->StateSet((en_ZPRJ_STATES)3);
        }
    }
    return 1;
}

static signed int VPRJ_BRB_notify(zEntProjectile* prj, en_PROJECTILE_EVENT event)
{
    return 1;
}

static void VPRJ_BMM_bupdate(_xEnt* ent, _xVec3* pos)
{
    ent->bound.sph.center = *pos;
    ent->bound.sph.r = 0.5f;
    xBoundUpdate(&ent->bound);
}

static signed int VPRJ_BMM_attach(zEntProjectile* prj)
{
    signed int i;

    prj->cat_data.idx_cdat = 2;
    prj->bupdate = VPRJ_BMM_bupdate;
    prj->OrientSet(0);
    for (i = 0; i < 10; i++)
    {
        st_ZVMM_FLASK_DATA* flask = &g_mmflask[i];
        if (!(flask->flg_stat & 1))
        {
            memset(flask, 0, sizeof(st_ZVMM_FLASK_DATA));
            flask->flg_stat |= 1;
            prj->xtradata = flask;
            break;
        }
    }
    return prj->xtradata != NULL;
}

static void VPRJ_BMM_detach(zEntProjectile* prj)
{
    if (prj->xtradata)
    {
        memset(prj->xtradata, 0, sizeof(st_ZVMM_FLASK_DATA));
        prj->xtradata = NULL;
    }
}

static signed int VPRJ_BMM_logix(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame)
{
    st_zEntVillain* vil;
    st_ZVMM_FLASK_DATA* flask;
    _xEntFrame* f;

    flask = (st_ZVMM_FLASK_DATA*)prj->xtradata;
    vil = (st_zEntVillain*)prj->owner;

    if (prj->tmr_alive > prj->tym_maxlife)
    {
        prj->StateSet((en_ZPRJ_STATES)3);
        return 1;
    }

    if ((prj->plyr_colrec->flags & 1) && (prj->flg_prop & 0x10))
    {
        zEntPlayer_Damage(prj, 1);
        prj->SendEvent(ZPRJ_EVNT_DETONATE, dt);
        prj->StateSet((en_ZPRJ_STATES)3);
        return 1;
    }

    switch (flask->stage)
    {
    case MAST_STAGE_TEASE:
        prj->SendEvent((en_PROJECTILE_EVENT)0xF, dt);
        prj->ColDelaySet(0.4f, 0);
        VUTIL_tagpos(vil, 6, &((_xMat4x3*)prj->frame)->pos);
        f = prj->frame;
        f->mode |= 1;
        prj->frame->dvel.x = 0.0f;
        prj->frame->dvel.y = 0.0f;
        prj->frame->dvel.z = 0.0f;
        prj->frame->vel.x = 0.0f;
        prj->frame->vel.y = 0.0f;
        prj->frame->vel.z = 0.0f;
        prj->frame->mode |= 0xC;
        break;
    case MAST_STAGE_TOSS:
        if (flask->flg_stat & 2)
        {
            ST_CAT_AIMDATA aiminf = { ZPRJ_CAIM_GRAVBALL };
            _xVec3* tgt;
            _xVec3* src;

            flask->flg_stat &= ~2;
            prj->tmr_curstate = 0.0f;
            aiminf.spd_base = flask->spd_flight;
            tgt = &((_xMat4x3*)globals.player.ent.model->Mat)->pos;
            src = &((_xMat4x3*)prj->model->Mat)->pos;
            xVec3Sub(&aiminf.dir_base, src, tgt);
            aiminf.grav.pos_src = *src;
            aiminf.grav.pos_tgt = *tgt;
            aiminf.grav.acc_gravity = flask->heavy;
            aiminf.grav.fac_arcness = 1.0f;
            prj->AimAtTarget(&aiminf, 1);
            prj->cat_data.gravball.acc_gravity = flask->heavy;
            prj->SendEvent(ZPRJ_EVNT_LAUNCH, dt);
        }
        prj->SendEvent(ZPRJ_EVNT_INFLIGHT, dt);
        if (prj->collis->colls[0].flags & 1)
        {
            flask->flg_stat |= 2;
            flask->stage = MAST_STAGE_AFTERMATH;
        }
        prj->frame->dvel.x = 0.0f;
        prj->frame->dvel.y = -(dt * prj->cat_data.gravball.acc_gravity);
        prj->frame->dvel.z = 0.0f;
        prj->frame->mode |= 8;
        if (prj->flg_categ & 0x4000000)
        {
            xMat4x3RMul3x3((_xMat4x3*)frame, &prj->common.mat_flex, (_xMat4x3*)frame);
            prj->frame->mode |= 0x100;
        }
        break;
    case MAST_STAGE_AFTERMATH:
        if (flask->flg_stat & 2)
        {
            flask->flg_stat &= ~2;
            prj->tmr_curstate = 0.0f;
            prj->flags &= ~1;
            prj->flg_prop &= ~8;
            prj->frame->dvel.x = 0.0f;
            prj->frame->dvel.y = 0.0f;
            prj->frame->dvel.z = 0.0f;
            prj->frame->vel.x = 0.0f;
            prj->frame->vel.y = 0.0f;
            prj->frame->vel.z = 0.0f;
            prj->frame->mode |= 0xC;
            prj->flg_chk &= 0xEF;
            prj->flg_pen &= 0xEF;
            prj->SendEvent(ZPRJ_EVNT_DETONATE, dt);
        }
        prj->SendEvent((en_PROJECTILE_EVENT)0x10, dt);
        switch (flask->typ_flask)
        {
        case MAST_FLSK_FIRE:
            VPRJ_BMM_redflask(prj, dt);
            break;
        case MAST_FLSK_ACID:
            VPRJ_BMM_greenflask(prj, dt);
            break;
        case MAST_FLSK_SMOG:
            VPRJ_BMM_blackflask(prj, dt);
            break;
        }
        if (prj->tmr_curstate > flask->tym_aftermath)
        {
            prj->StateSet((en_ZPRJ_STATES)3);
        }
        break;
    }
    return 1;
}

static signed int VPRJ_BMM_notify(zEntProjectile* prj, en_PROJECTILE_EVENT event)
{
    // Unreferenced in the original as well (only the template remains).
    _xVec3 unused = {};
    st_zEntVillain* vil = (st_zEntVillain*)prj->owner;

    switch (event)
    {
    case ZPRJ_EVNT_LAUNCH:
        VPRJ_prjsound(vil, prj, VSND_TYP_PRJ_LAUNCH);
        break;
    case ZPRJ_EVNT_INFLIGHT:
        if (!iSndIsPlaying(0, (unsigned int)prj))
        {
            VPRJ_prjsound(vil, prj, VSND_TYP_PRJ_FLIGHT);
        }
        break;
    case ZPRJ_EVNT_DETONATE:
    {
        unsigned int aid;

        xSndStopChildren((unsigned int)prj);
        aid = VSND_select_sound(vil, VSND_TYP_PRJ_SPLAT);
        if (aid)
        {
            xSndPlay3D(aid, 0.5f, 0.0f, 0x80, 0x202, prj, 25.0f);
            xSndPlay3D(xStrHash("bk_shock"), 1.0f, 0.0f, 0x80, 0x202, prj, 40.0f);
        }
        break;
    }
    case ZPRJ_EVNT_HITPLYR:
        VUTIL_stimulate(vil, (en_NPC_STIMULUS)0x19);
        VPRJ_prjsound(vil, prj, VSND_TYP_PRJ_EXPLODE);
        break;
    case ZPRJ_EVNT_TRANSTATE:
    case ZPRJ_EVNT_DEATH:
        if (prj->state != 2)
        {
            xSndStopChildren((unsigned int)prj);
        }
        break;
    }
    return 1;
}

static inline en_ZVMM_FLASK_TYPE VPRJ_BMM_select_flasktype(st_zEntVillain* vil)
{
    en_ZVMM_FLASK_TYPE flasks[3] = { MAST_FLSK_SMOG, MAST_FLSK_ACID, MAST_FLSK_FIRE };
    en_ZVMM_FLASK_TYPE typ = MAST_FLSK_SMOG;

    switch (vil->hitpts)
    {
    case 4:
        typ = MAST_FLSK_SMOG;
        break;
    case 3:
        typ = MAST_FLSK_ACID;
        break;
    case 2:
        typ = MAST_FLSK_FIRE;
        break;
    case 1:
    {
        signed int idx = 0;
        signed int i;
        float r = iurand();
        float lo;
        float hi = 0.0f;
        for (i = 0; i < 3; i++)
        {
            lo = hi;
            hi += 0.33333334f;
            if (r >= lo && r <= hi)
            {
                idx = i;
                break;
            }
        }
        typ = flasks[idx];
        break;
    }
    default:
        typ = MAST_FLSK_SMOG;
        break;
    }
    return typ;
}

static inline void VPRJ_prjsound(st_zEntVillain* vil, zEntProjectile* prj, en_NPC_SOUND snd)
{
    unsigned int aid = VSND_select_sound(vil, snd);
    if (aid)
    {
        xSndPlay3D(aid, 0.5f, 0.0f, 0x80, 0x202, prj, 25.0f);
    }
}

static inline void VPRJ_BMM_redflask(zEntProjectile* prj, float dt)
{
    _xCollis colrec = {};
    float rad_oldbnd;
    st_ZVMM_FLASK_DATA* flask = (st_ZVMM_FLASK_DATA*)prj->xtradata;
    _zParEmitterCustomSettings pecust;

    if (flask->tym_aftermath < 0.1f)
    {
        return;
    }
    rad_oldbnd = prj->bound.sph.r;
    prj->bound.sph.r = (prj->tmr_curstate / flask->tym_aftermath) * flask->rad_aftermath;
    xBoundHitsBound(&prj->bound, &globals.player.ent.bound, &colrec);
    if (colrec.flags & 1)
    {
        zEntPlayer_Damage(prj, 1);
        prj->SendEvent(ZPRJ_EVNT_HITPLYR);
    }
    prj->bound.sph.r = rad_oldbnd;
    pecust.custom_flags = 0x20;
    pecust.pos = ((_xMat4x3*)prj->model->Mat)->pos;
    zParEmitterEmitCustom(sEmitMMRedFlask, dt, &pecust);
}

static inline void VPRJ_BMM_greenflask(zEntProjectile* prj, float dt)
{
    _xCollis colrec = {};
    float rad_oldbnd;
    st_ZVMM_FLASK_DATA* flask = (st_ZVMM_FLASK_DATA*)prj->xtradata;
    _zParEmitterCustomSettings pecust;

    if (flask->tym_aftermath < 0.1f)
    {
        return;
    }
    rad_oldbnd = prj->bound.sph.r;
    prj->bound.sph.r = (prj->tmr_curstate / flask->tym_aftermath) * flask->rad_aftermath;
    xBoundHitsBound(&prj->bound, &globals.player.ent.bound, &colrec);
    if (colrec.flags & 1)
    {
        zEntPlayer_Damage(prj, 1);
        prj->SendEvent(ZPRJ_EVNT_HITPLYR);
    }
    prj->bound.sph.r = rad_oldbnd;
    pecust.custom_flags = 0x20;
    pecust.pos = ((_xMat4x3*)prj->model->Mat)->pos;
    zParEmitterEmitCustom(sEmitMMGreenFlask, dt, &pecust);
}

static inline void VPRJ_BMM_blackflask(zEntProjectile* prj, float dt)
{
    _xCollis colrec = {};
    float rad_oldbnd;
    st_ZVMM_FLASK_DATA* flask = (st_ZVMM_FLASK_DATA*)prj->xtradata;
    _zParEmitterCustomSettings pecust;

    if (flask->tym_aftermath < 0.1f)
    {
        return;
    }
    rad_oldbnd = prj->bound.sph.r;
    prj->bound.sph.r = (prj->tmr_curstate / flask->tym_aftermath) * flask->rad_aftermath;
    xBoundHitsBound(&prj->bound, &globals.player.ent.bound, &colrec);
    if (colrec.flags & 1)
    {
        zEntPlayer_Damage(prj, 1);
        prj->SendEvent(ZPRJ_EVNT_HITPLYR);
    }
    prj->bound.sph.r = rad_oldbnd;
    pecust.custom_flags = 0x20;
    pecust.pos = ((_xMat4x3*)prj->model->Mat)->pos;
    zParEmitterEmitCustom(sEmitMMBlackFlask, dt, &pecust);
}

static void VWEP_rbs_setup(st_REDBEARD_SWORD* rbs, st_zEntVillain* rbvil)
{
    unsigned int aid;
    signed int i;
    _xVec3 vec_pos = {};
    _xMat4x3 mat = {};

    if (rbs->flg_stat & 1)
    {
        xModelInstanceFree(rbs->model);
        rbs->model = NULL;
        rbs->flg_stat &= ~1;
    }

    memset(rbs, 0, sizeof(st_REDBEARD_SWORD));
    rbs->flg_stat |= 1;
    rbs->rbvil = rbvil;
    rbs->mdl_animsword = VWEP_rbs_animmodel(rbs->rbvil);
    rbs->aid_mdlFile = xStrHash("weprb_sword");
    rbs->mdlFile = (RpAtomic*)xSTFindAsset(rbs->aid_mdlFile, NULL);
    rbs->model = xModelInstanceAlloc(rbs->mdlFile, rbs, 0, 0, NULL);
    rbs->frame = &rbs->raw_frame;
    rbs->collis = (_xEntCollis*)&rbs->raw_collis;

    vec_pos = ((_xMat4x3*)rbs->rbvil->model->Mat)->pos;
    xMat3x3Euler((_xMat3x3*)&mat, 0.0f, 0.0f, 0.0f);
    mat.flags = 0;
    mat.pos = vec_pos;

    rbs->bound.type = 1;
    rbs->bound.sph.r = 0.75f;
    rbs->bound.sph.center = vec_pos;
    xBoundUpdate(&rbs->bound);
    memcpy(rbs->model->Mat, &mat, 0x40);

    rbs->frame->rot.axis = g_Y3;
    rbs->frame->rot.angle = 0.0f;
    rbs->frame->vel = g_O3;
    rbs->bupdate = VWEP_rbs_bupdate;

    rbs->ent_colchk[0] = &globals.player.ent;
    for (i = 1; i < 5; i++)
    {
        rbs->ent_colchk[i] = (_xEnt*)zSceneFindObject(xStrHash(g_strz_rbtgt[i]));
        if (rbs->ent_colchk[i] && rbs->ent_colchk[i]->baseType != 0x1B)
        {
            rbs->ent_colchk[i] = NULL;
        }
    }
    rbs->flg_stat |= 2;
}

static inline xModelInstance* VWEP_rbs_animmodel(st_zEntVillain* vil)
{
    signed int idx = 0;
    xModelInstance* minst = vil->model;
    xModelInstance* da_minst = NULL;

    while (minst && !da_minst)
    {
        if (idx == 1)
        {
            da_minst = minst;
        }
        minst = minst->Next;
        idx++;
    }
    return da_minst;
}

static void VWEP_rbs_bupdate(_xEnt* ent_rbs, _xVec3* pos)
{
    ent_rbs->bound.type = 1;
    ent_rbs->bound.sph.r = 0.75f;
    ent_rbs->bound.sph.center = *pos;
    xBoundUpdate(&ent_rbs->bound);
}

static void VWEP_rbs_throw(st_REDBEARD_SWORD* rbs)
{
    signed int rc;
    _xMat4x3* mat;

    rbs->tym_age = 0.0f;
    rbs->dst_far = rbs->rbvil->dst_extend;
    rbs->spd_fly = (1.99999f * rbs->rbvil->dst_extend) / 5.0f;
    rbs->dir_end = rbs->rbvil->precalc.dir_VtoP;
    xVec3SMul(&rbs->pos_end, &rbs->dir_end, rbs->dst_far);

    mat = (_xMat4x3*)rbs->frame;
    xMat3x3Euler((_xMat3x3*)mat, 0.0f, 0.0f, 0.0f);
    rc = VUTIL_tagpos(rbs->rbvil, 10, &mat->pos);
    if (!rc)
    {
        rc = VUTIL_tagpos(rbs->rbvil, 6, &mat->pos);
    }
    if (!rc)
    {
        rc = VUTIL_tagpos(rbs->rbvil, 5, &mat->pos);
    }
    if (!rc)
    {
        rc = VUTIL_tagpos(rbs->rbvil, 9, &mat->pos);
    }
    if (!rc)
    {
        mat->pos = *xBoundCenter(&rbs->rbvil->bound);
    }

    memcpy(rbs->model->Mat, rbs->frame, 0x40);
    rbs->bound.sph.center = ((_xMat4x3*)rbs->frame)->pos;
    rbs->mdl_animsword->Flags &= ~1;
    rbs->model->Flags |= 1;
    rbs->flg_stat |= 4;
}

static inline void VWEP_rbs_flight(st_REDBEARD_SWORD* rbs, float dt)
{
    _xMat3x3 mat_rot = {};
    _xVec3 ang = { 25.132741f, 0.0f, 0.0f };
    _xVec3 dpos = {};
    _xMat4x3* mat;
    unsigned int snd;

    rbs->tym_age += dt;
    if (rbs->tym_age > 2.5f)
    {
        rbs->flg_stat |= 8;
    }
    else
    {
        rbs->flg_stat &= ~8;
    }

    ang.x *= dt;
    ang.y *= dt;
    ang.z *= dt;
    xMat3x3Euler(&mat_rot, &ang);
    if (rbs->tym_age < 2.5f)
    {
        xVec3SMul(&dpos, &rbs->dir_end, dt * rbs->spd_fly);
    }
    else
    {
        xVec3SMul(&dpos, &rbs->dir_end, -dt * rbs->spd_fly);
    }

    xEntBeginUpdate(rbs, globals.sceneCur, dt);
    mat = (_xMat4x3*)rbs->frame;
    xMat4x3RMul3x3(mat, &mat_rot, mat);
    xVec3AddTo(&mat->pos, &dpos);
    xEntMotionToMatrix(rbs, rbs->frame);
    xEntEndUpdate(rbs, globals.sceneCur, dt);

    if (!iSndIsPlaying(0, (unsigned int)rbs))
    {
        snd = VSND_select_sound(rbs->rbvil, VSND_TYP_PRJ_FLIGHT);
        if (snd)
        {
            xSndPlay3D(snd, 0.5f, 0.0f, 0x80, 0x202, rbs, 25.0f);
        }
    }
    VWEP_rbs_chkCollide(rbs, dt);
}

static void VWEP_rbs_chkCollide(st_REDBEARD_SWORD* rbs, float dt)
{
    _xEnt* tgt;
    signed int i;
    _xCollis colrec = {};

    xEntBeginCollide(rbs, globals.sceneCur, dt);
    if (rbs->ent_colchk[0])
    {
        xBoundHitsBound(&rbs->bound, &rbs->ent_colchk[0]->bound, &colrec);
        if (colrec.flags & 1)
        {
            VIRM_initVilDflt(rbs->rbvil);
        }
    }

    rbs->bound.sph.r = 1.5f;
    for (i = 1; i < 5; i++)
    {
        tgt = rbs->ent_colchk[i];
        colrec.flags = 0;
        if (!tgt)
        {
            continue;
        }
        if (!xEntIsVisible(tgt))
        {
            rbs->ent_colchk[i] = NULL;
            continue;
        }
        colrec.optr = tgt;
        colrec.oid = tgt->id;
        xBoundHitsBound(&rbs->bound, &tgt->bound, &colrec);
        if (colrec.flags & 1)
        {
            colrec.flags = 0;
            xBoundHitsModel(&rbs->bound, tgt->model, &colrec, 1, 0.7853982f);
            if (colrec.flags & 1)
            {
                zEntEvent(rbs->rbvil, 0, tgt, 0x16, NULL, NULL, 0);
                rbs->ent_colchk[i] = NULL;
            }
        }
    }
    rbs->bound.sph.r = 0.75f;
    xEntEndCollide(rbs, globals.sceneCur, dt);
}

static void SuperVillainGumDraw(_xVec3* from, _xVec3* to, float top_radius, float bot_radius)
{
    _xVec3 offset = { 0.0f, 0.0f, 1.0f };
    _xMat3x3 mat;
    signed int i;
    RxObjSpace3DVertex* vert = sStripVert;
    _xVec3 vec_in;

    for (i = 0; i < 16; i++)
    {
        float u;

        xMat3x3Euler(&mat, (3.1415927f * (22.5f * i)) / 180.0f, 0.0f, 0.0f);
        offset.z = top_radius;
        xMat3x3LMulVec(&vec_in, &mat, &offset);
        vert[0].objVertex.x = from->x + vec_in.x;
        vert[0].objVertex.y = from->y + vec_in.y;
        u = 0.0625f * i;
        vert[0].objVertex.z = from->z + vec_in.z;
        vert[0].u = u;
        vert[0].v = 0.0f;
        vert[0].color.red = 0x80;
        vert[0].color.green = 0x80;
        vert[0].color.blue = 0x80;
        vert[0].color.alpha = 0xFF;

        offset.z = bot_radius;
        xMat3x3LMulVec(&vec_in, &mat, &offset);
        vert[1].objVertex.x = to->x + vec_in.x;
        vert[1].objVertex.y = to->y + vec_in.y;
        vert[1].objVertex.z = to->z + vec_in.z;
        vert[1].u = u;
        vert[1].v = 1.0f;
        vert[1].color.red = 0x80;
        vert[1].color.green = 0x80;
        vert[1].color.blue = 0x80;
        vert[1].color.alpha = 0xFF;
        vert += 2;
    }

    // Close the strip: the original writes these two past the end of the
    // 32-vertex array (kept as-is).
    vert[0] = sStripVert[0];
    vert[0].u = 1.0f;
    vert[1] = sStripVert[1];
    vert[1].u = 1.0f;

    if (RwIm3DTransform(sStripVert, 34, NULL, 0x19))
    {
        RwIm3DRenderPrimitive(4);
        RwIm3DEnd();
    }
}

static void VEFX_plop_gum(st_zEntVillain* ent)
{
    RwTexture* tex;
    _xMat3x3 dir_shoot = {};
    _xVec3 pos_vert = {};
    _xVec3 npcpos = {};
    signed int i;
    _xVec3 to = {};
    _xVec3 from = {};

    tex = (RwTexture*)xSTFindAsset(xStrHash("GUM-FLOOR"), NULL);
    if (tex)
    {
        RwRenderStateSet(1, *(void**)tex);
    }

    npcpos = ((_xMat4x3*)ent->model->Mat)->pos;
    for (i = 0; i <= 4; i++)
    {
        if (!VUTIL_tagpos(ent, i, &pos_vert))
        {
            continue;
        }
        from = pos_vert;
        to.x = pos_vert.x;
        to.z = pos_vert.z;
        if (pos_vert.y > npcpos.y)
        {
            to.y = npcpos.y;
        }
        else
        {
            to.y = pos_vert.y;
            from.y = npcpos.y;
        }
        if (!(from.y - to.y < 0.002f))
        {
            _xVec3 dir;
            xVec3Sub(&dir, &to, &from);
            xMat3x3LookVec(&dir_shoot, &dir);
            SuperVillainGumDraw(&from, &to, 0.1f, 0.35f);
        }
    }
}

static inline void VEFX_hologram_emit(st_zEntVillain* vil, float dt)
{
    _zParEmitterCustomSettings pecust;
    float tym;
    ScoobRGBA* dot_birth;
    ScoobRGBA* dot_death;
    ScoobRGBA* clr_birth;
    ScoobRGBA* clr_death;

    if (vil->ability & 8)
    {
        tym = (vil->tmr_list[ZVIL_TDUR_DECAY] > vil->tmr_list[ZVIL_TYMR_MINANIMPLAY]) ?
                  vil->tmr_list[ZVIL_TDUR_DECAY] :
                  vil->tmr_list[ZVIL_TYMR_MINANIMPLAY];
        if (tym)
        {
            clr_birth = &g_holoPoofBirth[vil->asset->subtype];
            clr_death = &g_holoPoofDeath[vil->asset->subtype];
            pecust.custom_flags = 0x5A0;
            pecust.color_birth[0] = clr_birth->r;
            pecust.color_birth[1] = clr_birth->g;
            pecust.color_birth[2] = clr_birth->b;
            pecust.color_death[0] = clr_death->r;
            pecust.color_death[1] = clr_death->g;
            pecust.color_death[2] = clr_death->b;
            pecust.pos = *xBoundCenter(&vil->bound);
            pecust.size_death = 5.0f * vil->bound.sph.r;
            pecust.color_birth[3] = 0;
            pecust.color_death[3] = 0;
            zParEmitterEmitCustom(sEmitHoloPoof, dt, &pecust);

            dot_birth = &g_holoDotsBirth[vil->asset->subtype];
            dot_death = &g_holoDotsDeath[vil->asset->subtype];
            pecust.color_birth[0] = dot_birth->r;
            pecust.color_birth[1] = dot_birth->g;
            pecust.color_birth[2] = dot_birth->b;
            pecust.color_birth[3] = 0xFF;
            pecust.color_death[0] = dot_death->r;
            pecust.color_death[1] = dot_death->g;
            pecust.color_death[2] = dot_death->b;
            pecust.color_death[3] = 0;
            pecust.custom_flags = 0x201A0;
            pecust.xzcircle_radius = vil->bound.sph.r;
            zParEmitterEmitCustom(sEmitHoloDots, dt, &pecust);
        }

        if (xUtil_yesno(tym))
        {
            vil->flags |= 1;
        }
        else
        {
            vil->flags &= ~1;
        }
    }
}

// GameCube addition: the hologram poof/dots effect while a villain fades.
static void VEFX_hologram(st_zEntVillain* vil, float dt)
{
    VEFX_hologram_emit(vil, dt);
}

static void VEFX_mm_zapper(st_zEntVillain* vil, signed int cleanup)
{
    _tagLightningAdd add = {};
    signed int i;

    for (i = 0; i < 5; i++)
    {
        if (g_mm_lytstrk[i])
        {
            zLightningKill(g_mm_lytstrk[i]);
            g_mm_lytstrk[i] = NULL;
        }
    }

    if (cleanup)
    {
        return;
    }

    add.type = eLT_Line;
    add.start = xBoundCenter(&vil->bound);
    add.end = xBoundCenter(&globals.player.ent.bound);
    add.time = 0.2f;
    add.total_points = 0x10;
    add.thickness = 0.15f;
    add.r = 0xFF;
    add.g = 0xFF;
    add.b = 0xFF;
    add.rand_radius = 5.0f;
    add.flags = 0x34;
    g_mm_lytstrk[0] = zLightningAdd(&add);

    add.type = eLT_Rotating;
    add.total_points = 0x10;
    add.thickness = 0.1f;
    add.r = 0x50;
    add.g = 0x50;
    add.b = 0xFF;
    add.rand_radius = 3.0f;
    add.setup_degrees = 90.0f * iurand() + 20.0f;
    add.move_degrees = 360.0f * iurand() + 180.0f;
    if (irand() & 1)
    {
        add.move_degrees = -add.move_degrees;
    }
    add.flags = 0x3C;
    add.rot_radius = 0.5f;
    g_mm_lytstrk[1] = zLightningAdd(&add);
}

static void VEFX_zapTheKnight(st_zEntVillain* vil, signed int cleanup)
{
    _xVec3 loca = {};
    signed int i;
    _tagLightningAdd add = {};

    for (i = 0; i < 5; i++)
    {
        if (g_bk_lytstrk[i])
        {
            zLightningKill(g_bk_lytstrk[i]);
            g_bk_lytstrk[i] = NULL;
        }
    }

    if (cleanup)
    {
        return;
    }

    loca = *xBoundCenter(&vil->bound);
    loca.y += 20.0f;
    add.start = xBoundCenter(&vil->bound);
    add.end = &loca;
    add.time = (vil->tmr_list[ZVIL_TDUR_HURT] > vil->tmr_list[ZVIL_TYMR_MINANIMPLAY]) ?
                   vil->tmr_list[ZVIL_TDUR_HURT] :
                   vil->tmr_list[ZVIL_TYMR_MINANIMPLAY];
    add.type = eLT_Line;
    add.total_points = 0x10;
    add.thickness = 1.25f;
    add.r = 0xFF;
    add.g = 0xFF;
    add.b = 0xFF;
    add.rand_radius = 5.0f;
    add.flags = 0x34;
    g_bk_lytstrk[0] = zLightningAdd(&add);

    add.type = eLT_Rotating;
    add.total_points = 0x10;
    add.thickness = 0.6f;
    add.r = 0x50;
    add.g = 0x50;
    add.b = 0xFF;
    add.rand_radius = 4.0f;
    add.setup_degrees = 90.0f * iurand() + 20.0f;
    add.move_degrees = 12360.0f * iurand() + 1280.0f;
    if (irand() & 1)
    {
        add.move_degrees = -add.move_degrees;
    }
    add.flags = 0x3C;
    add.rot_radius = 0.5f;
    g_bk_lytstrk[1] = zLightningAdd(&add);
}

static inline _tagLightning* VEFX_zapper_type_B(_xVec3* from, _xVec3* to, float dur)
{
    _tagLightningAdd add = {};

    add.start = from;
    add.end = to;
    add.time = dur;
    add.total_points = 0x10;
    add.thickness = 0.25f;
    add.r = 0xFF;
    add.g = 0xFF;
    add.b = 0xFF;
    add.rand_radius = 5.0f;
    add.flags = 0x34;
    return zLightningAdd(&add);
}

static inline _tagLightning* VEFX_zapper_type_A(_xVec3* from, _xVec3* to, float dur)
{
    _tagLightningAdd add = {};

    add.start = from;
    add.end = to;
    add.time = dur;
    add.total_points = 0x10;
    add.thickness = 0.16f;
    add.r = 0x50;
    add.g = 0x50;
    add.b = 0xFF;
    add.rand_radius = 3.0f;
    add.setup_degrees = 90.0f * iurand() + 20.0f;
    add.move_degrees = 360.0f * iurand() + 180.0f;
    if (irand() & 1)
    {
        add.move_degrees = -add.move_degrees;
    }
    add.flags = 0x3C;
    add.rot_radius = 0.5f;
    return zLightningAdd(&add);
}

static void VPAR_readyize_emitters()
{
    en_VPAR_EMITTER_ROLE emitrole;
    signed int flg_need;
    signed int flg_omit;
    st_zEntVillain_ParEmitInfo* vinf;
    en_zEntVillainSubTypeID viltype;
    xBase* pebase;
    unsigned int emitid;

    vinf = g_emitinfo;
    while (vinf->emitname)
    {
        emitid = xStrHash(vinf->emitname);
        emitrole = vinf->emitrole;
        viltype = vinf->viltype;
        flg_need = vinf->flg_need;
        flg_omit = vinf->flg_omit;
        vinf++;
        pebase = zSceneFindObject(emitid);
        if (pebase && pebase->baseType == 0x26)
        {
            st_zEntVillain_ParEmitRecord* rec;
            if (g_emitnext >= 32)
            {
                rec = NULL;
            }
            else
            {
                rec = &g_emitpool[g_emitnext++];
            }
            if (rec)
            {
                rec->emitrole = emitrole;
                rec->viltype = viltype;
                rec->vilowner = 0;
                rec->emitterID = emitid;
                rec->emitctxt = (_zParEmitter*)pebase;
                rec->flg_need = flg_need;
                rec->flg_omit = flg_omit;
                XOrdAppend(&g_emitlist, rec);
            }
        }
    }

    g_emitready = 1;
    sEmitHoloPoof = zParEmitterFind("ParEmit_Vil_HoloPoof");
    sEmitHoloDots = zParEmitterFind("ParEmit_Vil_HoloDots");
    sEmitGreenBall = zParEmitterFind("GREENBALL EMITTER");
    sEmitGreenBallSmack = zParEmitterFind("GREENBALL SMACK EMITTER");
    sEmitGreenBallShockwave = zParEmitterFind("GREENBALL SHOCKWAVE EMITTER");
    sEmitGGVortexHurt = zParEmitterFind("VORTEX EMITTER GG HURT");
    sEmitGGVortex1 = zParEmitterFind("VORTEX EMITTER 1");
    sEmitGGVortex2 = zParEmitterFind("VORTEX EMITTER 2");
    sEmitGGVortex3 = zParEmitterFind("VORTEX EMITTER 3");
    sEmitWitchDying = zParEmitterFind("PAREMIT_WITCH_DYING");
    sEmitKillerPlant = zParEmitterFind("PAREMIT_KILLERPLANT_CLOUD");
    sEmitMMRedFlask = zParEmitterFind("PAREMIT_MM_REDFLASK");
    sEmitMMGreenFlask = zParEmitterFind("PAREMIT_MM_GREENFLASK");
    sEmitMMBlackFlask = zParEmitterFind("PAREMIT_MM_BLACKFLASK");
    sEmitMMTeleportShow = zParEmitterFind("PAREMIT_MM_TELEPORT_SHOW");
    sEmitMMTeleportHide = zParEmitterFind("PAREMIT_MM_TELEPORT_HIDE");
    sEmitRBFlames = zParEmitterFind("RB STOPPING WALL EMITTER");
}

static void VPAR_emit_partype(st_zEntVillain* ent, en_VPAR_EMITTER_ROLE emittype, float dt)
{
    _zParEmitterCustomSettings pecust = { 0 };
    st_zEntVillain_ParEmitRecord* vpar;
    signed int i;
    signed int do_emit = 0;
    signed int idx;
    _xVec3 vilctr = {};
    _xVec3 vilhed = {};

    vilctr = *xBoundCenter(&ent->bound);
    if (!VUTIL_tagpos(ent, 8, &vilhed))
    {
        vilhed = *(_xVec3*)&((_xMat4x3*)ent->model->Mat)->pos;
        vilhed.y += ent->precalc.hyt_vil;
    }
    vilhed.y += 0.1f;

    for (i = 0; i < g_emitlist.cnt; i++)
    {
        vpar = (st_zEntVillain_ParEmitRecord*)g_emitlist.list[i];
        if (!vpar->emitctxt)
        {
            continue;
        }
        if (vpar->emitrole != emittype)
        {
            continue;
        }
        if (vpar->viltype != -1 && vpar->viltype != ent->asset->subtype)
        {
            continue;
        }
        if (vpar->flg_need != (vpar->flg_need & ent->ability))
        {
            continue;
        }
        if (ent->ability & vpar->flg_omit)
        {
            continue;
        }

        idx = 0;
        pecust.custom_flags = 0;
        switch (emittype)
        {
        case VPAR_EMIT_EXTATTACK:
            do_emit = 0;
            break;
        case VPAR_EMIT_GUMMY:
            pecust.custom_flags = 0x20;
            for (idx = 0; idx <= 4; idx++)
            {
                if (VUTIL_tagpos(ent, idx, &pecust.pos))
                {
                    zParEmitterEmitCustom(vpar->emitctxt, dt, &pecust);
                }
            }
            do_emit = 0;
            break;
        case VPAR_EMIT_HEADBUTT:
            do_emit = VPAR_emit_headbutt(dt, vpar, &pecust);
            break;
        case VPAR_EMIT_BORED:
        case VPAR_EMIT_GUARD:
            pecust.pos = vilhed;
            pecust.custom_flags = 0x20;
            if (vpar->emitterID == xStrHash("ParEmit_Vil_Bored"))
            {
                pecust.color_birth[3] = 0;
                pecust.custom_flags |= 0x180;
                pecust.color_death[3] = 0;
            }
            do_emit = 1;
            break;
        case VPAR_EMIT_QUESTMARK:
        case VPAR_EMIT_EXCLAIM:
            if (g_scene_is_a_boss_battle)
            {
                break;
            }
        case VPAR_EMIT_BUTTBOUNCE:
        case VPAR_EMIT_STUN:
            do_emit = 1;
            pecust.custom_flags = 0x20;
            pecust.pos = vilhed;
            break;
        case VPAR_EMIT_WALLPRINT:
            pecust.custom_flags = 0x20;
            do_emit = 1;
            pecust.pos = vilctr;
            break;
        case VPAR_EMIT_POOF:
        case VPAR_EMIT_TELEPORT:
            do_emit = 0;
            break;
        default:
            pecust.custom_flags = 0x20;
            do_emit = 1;
            pecust.pos = vilctr;
            break;
        }

        if (do_emit)
        {
            zParEmitterEmitCustom(vpar->emitctxt, dt, &pecust);
        }
    }
}

static inline signed int VUTIL_tagpos(st_zEntVillain* ent, signed int idx, _xVec3* pos)
{
    if (!(ent->flg_vert & (1 << idx)))
    {
        return 0;
    }
    iModelTagEval(ent->model->Data, &ent->tag_vert[idx], ent->model->Mat, pos);
    return 1;
}

static inline signed int VPAR_emit_headbutt(float dt, st_zEntVillain_ParEmitRecord* vpar,
                                            _zParEmitterCustomSettings* pecust)
{
    _xVec3 angles = {};
    _xMat4x3* mat;

    pecust->custom_flags = 0x20;
    mat = (_xMat4x3*)globals.player.ent.model->Mat;
    pecust->pos.x = mat->pos.x + mat->at.x;
    pecust->pos.y = 1.1f + mat->pos.y;
    pecust->pos.z = mat->pos.z + mat->at.z;
    pecust->custom_flags |= 0x10000;
    xMat3x3GetEuler((_xMat3x3*)mat, &angles);
    pecust->rot[1] = 0x40;
    pecust->rot[0] = 255.0f * (angles.x / 6.2831855f);
    pecust->rot[2] = 255.0f * (angles.z / 6.2831855f);
    if (vpar->emitterID == xStrHash("ParEmit_SD_Headbutt2"))
    {
        pecust->rot[0] += 0x40;
    }
    else
    {
        dt = 0.0f;
    }
    zParEmitterEmitCustom(vpar->emitctxt, dt, pecust);
    return 0;
}

static void VPAR_emit_extattack(st_zEntVillain* ent, _xVec3* origin, _xVec3* aimdir, float length, float slope,
                                float dt)
{
    st_zEntVillain_ParEmitRecord* vpar;
    _zParEmitterCustomSettings* pecust;
    float delta;
    float fv = 0.0f;
    signed int j;
    signed int i;

    for (i = 0; i < g_emitlist.cnt; i++)
    {
        vpar = (st_zEntVillain_ParEmitRecord*)g_emitlist.list[i];
        if (!vpar->emitctxt)
        {
            continue;
        }
        if (vpar->emitrole != VPAR_EMIT_EXTATTACK)
        {
            continue;
        }
        if (vpar->viltype != -1 && vpar->viltype != ent->asset->subtype)
        {
            continue;
        }
        if (vpar->flg_need != (vpar->flg_need & ent->ability))
        {
            continue;
        }
        if (ent->ability & vpar->flg_omit)
        {
            continue;
        }

        delta = length / 25.0f;
        pecust = &g_pecust;
        pecust->custom_flags = 0x2FE1;
        pecust->life = 0.75f;
        pecust->count = 1;
        pecust->color_birth[0] = 0xAA;
        pecust->color_birth[1] = 0xAA;
        pecust->color_birth[2] = 0xAA;
        pecust->color_birth[3] = 0xFF;
        pecust->color_death[0] = 0x11;
        pecust->color_death[1] = 0x11;
        pecust->color_death[2] = 0x11;
        pecust->color_death[3] = 0x11;
        for (j = 0; j < 25; j++)
        {
            fv += delta;
            pecust->size_birth = slope * fv * (1.0f + (1.0f - iurand()));
            pecust->size_death = 0.25f * pecust->size_birth;
            pecust->size_birth_variation = pecust->size_death;
            xVec3SMul(&pecust->pos, aimdir, fv);
            xVec3AddTo(&pecust->pos, origin);
            xVec3SMul(&pecust->vel, aimdir, -ent->spd_extend);
            zParEmitterEmitCustom(vpar->emitctxt, dt, pecust);
        }
    }
}

static xAnimTable* VANM_mktable_BK()
{
    xAnimTable* table;
    xAnimTransition* tran;
    signed int i;
    char* names[32] = {};
    xAnimTransition* deftran[147] = {}; // unreferenced in the original as well

    table = xAnimTableNew("Villain_BlackKnight", NULL);
    xAnimTableNewState(table, "Idle", 0x110, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "BK_ScaryIdle", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "BK_Walk", 0x10, 0x02000004, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "BK_Turn", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "BK_Patrol", 0x10, 0x02000004, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "BK_PatrolFlip", 0x10, 0x02000004, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "BK_Attack", 0x20, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "BK_Shock", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "BK_ShockSkel", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "BK_Gloat", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "BK_Gloat_1", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "BK_Gloat_2", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "BK_Death", 0, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    for (i = 59; i < 71; i++)
    {
        xAnimTableNewTransition(table, g_animname[i], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
        xAnimTableNewTransition(table, g_animname[i], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 5, 0, 0.1f, NULL);
        xAnimTableNewTransition(table, "Idle", g_animname[i], (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    names[0] = "BK_ScaryIdle";
    names[1] = "BK_Patrol";
    names[2] = "BK_PatrolFlip";
    names[3] = "BK_Shock";
    names[4] = "BK_ShockSkel";
    names[5] = "BK_Gloat";
    names[6] = "BK_Gloat_1";
    names[7] = "BK_Gloat_2";
    for (i = 0; i < 8; i++)
    {
        xAnimTableNewTransition(table, names[i], "BK_Turn", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
        xAnimTableNewTransition(table, names[i], "BK_Walk", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    xAnimTableNewTransition(table, "BK_Patrol", "BK_PatrolFlip", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "BK_PatrolFlip", "BK_Patrol", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    names[0] = "BK_ScaryIdle";
    names[1] = "BK_Walk";
    names[2] = "BK_Turn";
    names[3] = "BK_Shock";
    names[4] = "BK_ShockSkel";
    names[5] = "BK_Gloat";
    names[6] = "BK_Gloat_1";
    names[7] = "BK_Gloat_2";
    for (i = 0; i < 8; i++)
    {
        xAnimTableNewTransition(table, names[i], "BK_Patrol", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
        xAnimTableNewTransition(table, names[i], "BK_PatrolFlip", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    names[0] = "BK_ScaryIdle";
    names[1] = "BK_Walk";
    names[2] = "BK_Turn";
    names[3] = "BK_Patrol";
    names[4] = "BK_PatrolFlip";
    names[5] = "BK_Shock";
    names[6] = "BK_ShockSkel";
    names[7] = "BK_Gloat";
    names[8] = "BK_Gloat_1";
    names[9] = "BK_Gloat_2";
    for (i = 0; i < 10; i++)
    {
        xAnimTableNewTransition(table, names[i], "BK_Attack", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    tran = xAnimTableNewTransition(table, "BK_Shock", "BK_ShockSkel", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    tran->Flags |= 0x20;
    tran = xAnimTableNewTransition(table, "BK_ShockSkel", "BK_Shock", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    tran->Flags |= 0x20;
    names[0] = "BK_ScaryIdle";
    names[1] = "BK_Walk";
    names[2] = "BK_Turn";
    names[3] = "BK_Patrol";
    names[4] = "BK_PatrolFlip";
    names[5] = "BK_Gloat";
    names[6] = "BK_Gloat_1";
    names[7] = "BK_Gloat_2";
    for (i = 0; i < 8; i++)
    {
        xAnimTableNewTransition(table, names[i], "BK_ShockSkel", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
        xAnimTableNewTransition(table, names[i], "BK_Shock", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    names[0] = "BK_ScaryIdle";
    names[1] = "BK_Walk";
    names[2] = "BK_Turn";
    names[3] = "BK_Patrol";
    names[4] = "BK_PatrolFlip";
    names[5] = "BK_Shock";
    names[6] = "BK_ShockSkel";
    names[7] = "BK_Attack";
    for (i = 0; i < 8; i++)
    {
        xAnimTableNewTransition(table, names[i], "BK_Gloat", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
        xAnimTableNewTransition(table, names[i], "BK_Gloat_1", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
        xAnimTableNewTransition(table, names[i], "BK_Gloat_2", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    names[0] = "BK_ScaryIdle";
    names[1] = "BK_Walk";
    names[2] = "BK_Turn";
    names[3] = "BK_Patrol";
    names[4] = "BK_PatrolFlip";
    names[5] = "BK_Shock";
    names[6] = "BK_ShockSkel";
    names[7] = "BK_Gloat";
    names[8] = "BK_Gloat_1";
    names[9] = "BK_Gloat_2";
    for (i = 0; i < 10; i++)
    {
        xAnimTableNewTransition(table, names[i], "BK_Death", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    return table;
}

static xAnimTable* VANM_mktable_GG()
{
    xAnimTable* table;
    xAnimTransition* tran;
    signed int i;
    char* names[32] = {};
    xAnimTransition* deftran[147] = {}; // unreferenced in the original as well

    table = xAnimTableNew("Villain_GreenGhost", NULL);
    xAnimTableNewState(table, "Idle", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GG_Chase", 0x10, 0x02000004, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GG_ChaseLeft", 0x10, 0x02000004, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GG_ChaseRight", 0x10, 0x02000004, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GG_AttackBuildBegin", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GG_AttackBuildLoop", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GG_Attack", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GG_Gloat", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GG_Gloat_1", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GG_Hit", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GG_Hit_1", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GG_Hit_2", 0, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    for (i = 71; i < 82; i++)
    {
        xAnimTableNewTransition(table, g_animname[i], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);
        xAnimTableNewTransition(table, g_animname[i], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 5, 0, 0.2f, NULL);
        xAnimTableNewTransition(table, "Idle", g_animname[i], (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    }
    xAnimTableNewTransition(table, "GG_Chase", "GG_ChaseLeft", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "GG_Chase", "GG_ChaseRight", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "GG_ChaseLeft", "GG_Chase", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "GG_ChaseLeft", "GG_ChaseRight", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "GG_ChaseRight", "GG_Chase", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "GG_ChaseRight", "GG_ChaseLeft", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    names[0] = "GG_Chase";
    names[1] = "GG_ChaseLeft";
    names[2] = "GG_ChaseRight";
    names[3] = "GG_Gloat";
    names[4] = "GG_Gloat_1";
    names[5] = "GG_Hit";
    names[6] = "GG_Hit_1";
    names[7] = "GG_Hit_2";
    for (i = 0; i < 8; i++)
    {
        xAnimTableNewTransition(table, names[i], "GG_AttackBuildBegin", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    }
    names[0] = "GG_Chase";
    names[1] = "GG_ChaseLeft";
    names[2] = "GG_ChaseRight";
    names[3] = "GG_AttackBuildBegin";
    names[4] = "GG_AttackBuildLoop";
    names[5] = "GG_Attack";
    names[6] = "GG_Hit";
    names[7] = "GG_Hit_1";
    names[8] = "GG_Hit_2";
    for (i = 0; i < 9; i++)
    {
        xAnimTableNewTransition(table, names[i], "GG_Gloat", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
        xAnimTableNewTransition(table, names[i], "GG_Gloat_1", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    }
    xAnimTableNewTransition(table, "GG_AttackBuildBegin", "GG_AttackBuildLoop", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 5, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "GG_AttackBuildLoop", "GG_Attack", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    names[0] = "GG_Chase";
    names[1] = "GG_ChaseLeft";
    names[2] = "GG_ChaseRight";
    names[3] = "GG_AttackBuildBegin";
    names[4] = "GG_AttackBuildLoop";
    names[5] = "GG_Attack";
    names[6] = "GG_Gloat";
    names[7] = "GG_Gloat_1";
    for (i = 0; i < 8; i++)
    {
        xAnimTableNewTransition(table, names[i], "GG_Hit", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
        xAnimTableNewTransition(table, names[i], "GG_Hit_1", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
        xAnimTableNewTransition(table, names[i], "GG_Hit_2", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    }
    return table;
}

static xAnimTable* VANM_mktable_RB()
{
    xAnimTable* table;
    xAnimTransition* tran;
    signed int i;
    char* names[32] = {};
    xAnimTransition* deftran[147] = {}; // unreferenced in the original as well

    table = xAnimTableNew("Villain_RedBeard", NULL);
    xAnimTableNewState(table, "Idle", 0x110, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RB_IdleWind", 0x110, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RB_Walk", 0x10, 0x02000004, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RB_Turn", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RB_Chase", 0x10, 0x02000006, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RB_TauntFeel", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RB_TauntThroat", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RB_TauntLaugh", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RB_AttackPunch", 0x20, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RB_AttackThrow", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RB_AttackThrowWait", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RB_AttackCatch", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RB_React", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RB_LookBegin", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RB_LookLoop", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RB_LookEnd", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "RB_DeathLoop", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    for (i = 82; i < 98; i++)
    {
        xAnimTableNewTransition(table, g_animname[i], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);
        xAnimTableNewTransition(table, g_animname[i], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 5, 0, 0.2f, NULL);
        xAnimTableNewTransition(table, "Idle", g_animname[i], (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
        if (i != 0x61)
        {
            xAnimTableNewTransition(table, g_animname[i], "RB_DeathLoop", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
        }
    }
    xAnimTableNewTransition(table, "RB_IdleWind", "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "RB_IdleWind", "RB_Turn", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "RB_Turn", "RB_IdleWind", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "RB_Chase", "RB_Walk", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "RB_Walk", "RB_Chase", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    names[0] = "RB_IdleWind";
    names[1] = "RB_Turn";
    names[2] = "RB_React";
    names[3] = "RB_TauntFeel";
    names[4] = "RB_TauntThroat";
    names[5] = "RB_TauntLaugh";
    names[6] = "RB_AttackPunch";
    names[7] = "RB_AttackThrowWait";
    names[8] = "RB_AttackThrow";
    names[9] = "RB_AttackPunch";
    names[10] = "RB_LookBegin";
    names[11] = "RB_LookLoop";
    names[12] = "RB_LookEnd";
    for (i = 0; i < 13; i++)
    {
        xAnimTableNewTransition(table, names[i], "RB_Chase", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
        xAnimTableNewTransition(table, names[i], "RB_Walk", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    }
    names[0] = "RB_IdleWind";
    names[1] = "RB_React";
    names[2] = "RB_AttackThrow";
    names[3] = "RB_AttackCatch";
    names[4] = "RB_AttackCatch";
    names[5] = "RB_AttackPunch";
    names[6] = "RB_LookBegin";
    names[7] = "RB_LookLoop";
    names[8] = "RB_LookEnd";
    names[9] = "RB_Chase";
    names[10] = "RB_Walk";
    names[11] = "RB_Turn";
    for (i = 0; i < 12; i++)
    {
        xAnimTableNewTransition(table, names[i], "RB_TauntFeel", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
        xAnimTableNewTransition(table, names[i], "RB_TauntThroat", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
        xAnimTableNewTransition(table, names[i], "RB_TauntLaugh", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    }
    xAnimTableNewTransition(table, "RB_AttackThrow", "RB_AttackThrowWait", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 5, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "RB_AttackThrowWait", "RB_AttackCatch", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    names[0] = "RB_IdleWind";
    names[1] = "RB_React";
    names[2] = "RB_TauntFeel";
    names[3] = "RB_TauntThroat";
    names[4] = "RB_TauntLaugh";
    names[5] = "RB_AttackPunch";
    names[6] = "RB_LookBegin";
    names[7] = "RB_LookLoop";
    names[8] = "RB_LookEnd";
    names[9] = "RB_Chase";
    names[10] = "RB_Walk";
    names[11] = "RB_Turn";
    for (i = 0; i < 12; i++)
    {
        xAnimTableNewTransition(table, names[i], "RB_AttackThrow", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
        xAnimTableNewTransition(table, names[i], "RB_AttackCatch", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
        xAnimTableNewTransition(table, names[i], "RB_AttackThrowWait", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    }
    xAnimTableNewTransition(table, "RB_LookBegin", "RB_LookLoop", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, "RB_LookLoop", "RB_LookEnd", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.2f, NULL);
    names[0] = "RB_IdleWind";
    names[1] = "RB_React";
    names[2] = "RB_TauntFeel";
    names[3] = "RB_TauntThroat";
    names[4] = "RB_TauntLaugh";
    names[5] = "RB_AttackPunch";
    names[6] = "RB_AttackThrowWait";
    names[7] = "RB_AttackThrow";
    names[8] = "RB_AttackCatch";
    names[9] = "RB_Chase";
    names[10] = "RB_Turn";
    names[11] = "RB_Walk";
    for (i = 0; i < 12; i++)
    {
        xAnimTableNewTransition(table, names[i], "RB_LookBegin", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
        xAnimTableNewTransition(table, names[i], "RB_LookLoop", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
        xAnimTableNewTransition(table, names[i], "RB_LookEnd", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.2f, NULL);
    }
    return table;
}

static xAnimTable* VANM_mktable_MM()
{
    xAnimTable* table;
    xAnimTransition* tran;
    signed int i;
    char* names[32] = {};
    xAnimTransition* deftran[147] = {}; // unreferenced in the original as well

    table = xAnimTableNew("Villain_MasterMind", NULL);
    xAnimTableNewState(table, "Idle", 0x110, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Foreplay_Move", 0x10, 0x02000004, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Foreplay_Action", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Foreplay_Taunt", 0x20, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Foreplay_JumpDown", 0x20, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Battle_Idle", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Battle_Move", 0x10, 0x02000006, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Battle_Backstep", 0x10, 0x02000006, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Battle_Turn", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Battle_Attack_start", 0x20, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Battle_Attack_ready_loop", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Battle_Attack", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Battle_Shield_start", 0x20, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Battle_Shield_loop", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Battle_Shield_end", 0x20, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Battle_Stunned_start", 0x20, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Battle_Stunned_loop", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Battle_Stunned_end", 0x20, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Battle_Knocked_begin", 0x20, 6, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Battle_Knocked_loop", 0x10, 6, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Battle_Knocked_recover", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Battle_Taunt", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_AngerReact", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Death_start", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "MM_Death", 0, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    for (i = 98; i < 122; i++)
    {
        if (i < 0x66)
        {
            xAnimTableNewTransition(table, g_animname[i], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
            xAnimTableNewTransition(table, g_animname[i], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 5, 0, 0.1f, NULL);
            xAnimTableNewTransition(table, "Idle", g_animname[i], (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 5, 0, 0.1f, NULL);
        }
        else if (i != 0x66)
        {
            xAnimTableNewTransition(table, g_animname[i], "MM_Battle_Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
            xAnimTableNewTransition(table, g_animname[i], "MM_Battle_Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 5, 0, 0.1f, NULL);
            xAnimTableNewTransition(table, "MM_Battle_Idle", g_animname[i], (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 5, 0, 0.1f, NULL);
        }
    }
    names[0] = "MM_Foreplay_Move";
    names[1] = "MM_Foreplay_Action";
    for (i = 0; i < 2; i++)
    {
        xAnimTableNewTransition(table, names[i], "MM_Foreplay_Taunt", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    names[0] = "MM_Foreplay_Move";
    names[1] = "MM_Foreplay_Taunt";
    for (i = 0; i < 2; i++)
    {
        xAnimTableNewTransition(table, names[i], "MM_Foreplay_Action", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    names[0] = "MM_Foreplay_Action";
    names[1] = "MM_Foreplay_Taunt";
    for (i = 0; i < 2; i++)
    {
        xAnimTableNewTransition(table, names[i], "MM_Foreplay_Move", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    xAnimTableNewTransition(table, "MM_Foreplay_JumpDown", "MM_Battle_Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    names[0] = "MM_Foreplay_Action";
    names[1] = "MM_Foreplay_Taunt";
    names[2] = "MM_Foreplay_Move";
    for (i = 0; i < 3; i++)
    {
        xAnimTableNewTransition(table, names[i], "MM_Foreplay_JumpDown", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    names[0] = "Idle";
    names[1] = "MM_Foreplay_Move";
    names[2] = "MM_Foreplay_Taunt";
    names[3] = "MM_Foreplay_JumpDown";
    for (i = 0; i < 4; i++)
    {
        xAnimTableNewTransition(table, names[i], "MM_Battle_Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
        xAnimTableNewTransition(table, names[i], "MM_Battle_Move", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    xAnimTableNewTransition(table, "MM_Battle_Idle", "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    names[0] = "MM_Battle_Turn";
    names[1] = "MM_Battle_Backstep";
    names[2] = "MM_Battle_Knocked_begin";
    names[3] = "MM_Battle_Knocked_loop";
    names[4] = "MM_Battle_Knocked_recover";
    names[5] = "MM_Battle_Shield_start";
    names[6] = "MM_Battle_Shield_loop";
    names[7] = "MM_Battle_Shield_end";
    names[8] = "MM_Battle_Stunned_start";
    names[9] = "MM_Battle_Stunned_loop";
    names[10] = "MM_Battle_Stunned_end";
    names[11] = "MM_Battle_Attack_start";
    names[12] = "MM_Battle_Attack_ready_loop";
    names[13] = "MM_Battle_Attack";
    for (i = 0; i < 14; i++)
    {
        xAnimTableNewTransition(table, names[i], "MM_Battle_Move", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    xAnimTableNewTransition(table, "MM_Battle_Move", "MM_Battle_Backstep", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "MM_Battle_Shield_start", "MM_Battle_Shield_loop", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    names[0] = "MM_Battle_Move";
    names[1] = "MM_Battle_Turn";
    names[2] = "MM_Battle_Backstep";
    names[3] = "MM_Foreplay_Taunt";
    names[4] = "MM_AngerReact";
    names[5] = "MM_Battle_Taunt";
    names[6] = "MM_Battle_Stunned_start";
    names[7] = "MM_Battle_Stunned_loop";
    names[8] = "MM_Battle_Stunned_end";
    names[9] = "MM_Battle_Attack_start";
    names[0] = "MM_Battle_Attack_ready_loop";
    names[11] = "MM_Battle_Attack";
    for (i = 0; i < 12; i++)
    {
        xAnimTableNewTransition(table, names[i], "MM_Battle_Shield_start", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    names[0] = "MM_Battle_Move";
    names[1] = "MM_Battle_Turn";
    names[2] = "MM_Battle_Backstep";
    names[3] = "MM_Battle_Knocked_begin";
    names[4] = "MM_Battle_Knocked_loop";
    names[5] = "MM_Battle_Knocked_recover";
    names[6] = "MM_Battle_Shield_start";
    names[7] = "MM_Battle_Shield_loop";
    names[8] = "MM_Battle_Shield_end";
    names[9] = "MM_Battle_Stunned_start";
    names[10] = "MM_Battle_Stunned_loop";
    names[11] = "MM_Battle_Stunned_end";
    names[12] = "MM_Battle_Attack_start";
    names[13] = "MM_Battle_Attack_ready_loop";
    names[14] = "MM_Battle_Attack";
    for (i = 0; i < 15; i++)
    {
        xAnimTableNewTransition(table, names[i], "MM_Battle_Taunt", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
        xAnimTableNewTransition(table, names[i], "MM_Foreplay_Taunt", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 9, 0, 0.1f, NULL);
        xAnimTableNewTransition(table, names[i], "MM_AngerReact", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 9, 0, 0.1f, NULL);
    }
    xAnimTableNewTransition(table, "MM_Battle_Attack_start", "MM_Battle_Attack_ready_loop", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 5, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "MM_Battle_Attack", "MM_Battle_Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 5, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "MM_Battle_Attack_ready_loop", "MM_Battle_Attack", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 9, 0, 0.1f, NULL);
    names[0] = "MM_Battle_Move";
    names[1] = "MM_Battle_Turn";
    names[2] = "MM_Battle_Backstep";
    names[3] = "MM_Foreplay_Taunt";
    names[4] = "MM_AngerReact";
    names[5] = "MM_Battle_Taunt";
    names[6] = "MM_Battle_Shield_start";
    names[7] = "MM_Battle_Shield_loop";
    names[8] = "MM_Battle_Shield_end";
    names[9] = "MM_Battle_Stunned_start";
    names[10] = "MM_Battle_Stunned_loop";
    names[11] = "MM_Battle_Stunned_end";
    for (i = 0; i < 12; i++)
    {
        xAnimTableNewTransition(table, names[i], "MM_Battle_Attack_start", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    xAnimTableNewTransition(table, "MM_Battle_Stunned_start", "MM_Battle_Stunned_loop", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    names[0] = "MM_Battle_Shield_start";
    names[1] = "MM_Battle_Shield_loop";
    names[2] = "MM_Battle_Shield_end";
    names[3] = "MM_Battle_Attack_start";
    names[4] = "MM_Battle_Attack_ready_loop";
    names[5] = "MM_Battle_Attack";
    for (i = 0; i < 6; i++)
    {
        xAnimTableNewTransition(table, names[i], "MM_Battle_Stunned_start", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    xAnimTableNewTransition(table, "MM_Battle_Knocked_begin", "MM_Battle_Knocked_loop", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    names[0] = "MM_Battle_Stunned_start";
    names[1] = "MM_Battle_Stunned_loop";
    names[2] = "MM_Battle_Stunned_end";
    for (i = 0; i < 3; i++)
    {
        xAnimTableNewTransition(table, names[i], "MM_Battle_Knocked_begin", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
        xAnimTableNewTransition(table, names[i], "MM_Battle_Knocked_loop", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    xAnimTableNewTransition(table, "MM_Battle_Knocked_loop", "MM_Battle_Knocked_recover", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    names[0] = "MM_Battle_Knocked_begin";
    names[1] = "MM_Battle_Knocked_loop";
    names[2] = "MM_Battle_Knocked_recover";
    for (i = 0; i < 3; i++)
    {
        xAnimTableNewTransition(table, names[i], "MM_Death_start", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    xAnimTableNewTransition(table, "MM_Death_start", "MM_Death", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    return table;
}

// Small enough to be auto-inlined into its export; the out-of-line copy is
// dead-stripped by the linker, but its string literals keep their place.
#pragma auto_inline on
static xAnimTable* VANM_mktable_SHAM()
{
    // Unreferenced in the original as well (only the templates remain).
    char* unused_names[32] = {};
    xAnimTransition* unused_tran[147] = {};
    xAnimTable* table;
    xAnimTransition* tran;
    signed int i;

    table = xAnimTableNew("Villain_BossSham", NULL);
    xAnimTableNewState(table, "Idle", 0x110, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "SHAM_Move", 0x10, 0x02000004, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "SHAM_Taunt", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    for (i = 140; i < 142; i++)
    {
        xAnimTableNewTransition(table, g_animname[i], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
        xAnimTableNewTransition(table, g_animname[i], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 5, 0, 0.1f, NULL);
        xAnimTableNewTransition(table, "Idle", g_animname[i], (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    xAnimTableNewTransition(table, "SHAM_Taunt", "SHAM_Move", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 5, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "SHAM_Move", "SHAM_Taunt", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "SHAM_Taunt", "SHAM_Move", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    return table;
}

// Small enough to be auto-inlined into its export; the out-of-line copy is
// dead-stripped by the linker, but its string literals keep their place.
#pragma auto_inline on
static xAnimTable* VANM_mktable_WITCH()
{
    // Unreferenced in the original as well (only the templates remain).
    char* unused_names[32] = {};
    xAnimTransition* unused_tran[147] = {};
    xAnimTable* table;
    xAnimTransition* tran;
    signed int i;

    table = VANM_mktable_STD();
    xAnimTableNewState(table, "Shaggy_hold", 0x10, 0x02000004, 1.0f, NULL, NULL, 0.0f, NULL);
    Shaggy5_WitchTransitions(table);
    return table;
}

#pragma auto_inline off
static xAnimTable* VANM_mktable_SHARK()
{
    // Unreferenced in the original as well (only the templates remain).
    char* unused_names[32] = {};
    xAnimTransition* unused_tran[147] = {};
    xAnimTable* table;
    xAnimTransition* tran;
    signed int i;

    table = xAnimTableNew("Villain_Shark", NULL);
    xAnimTableNewState(table, "Idle", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Bored", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_Fidget_1", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_Fidget_2", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Bored_Fidget_1", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Bored_Fidget_2", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    for (i = 1; i < 7; i++)
    {
        xAnimTableNewTransition(table, g_animname[i], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.0f, NULL);
        xAnimTableNewTransition(table, g_animname[i], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 5, 0, 0.0f, NULL);
        xAnimTableNewTransition(table, "Idle", g_animname[i], (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.0f, NULL);
    }
    tran = xAnimTableNewTransition(table, "Idle_Fidget_1 Idle_Fidget_2", "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 2, 0, 0.0f, NULL);
    tran->Flags |= 0x20;
    tran = xAnimTableNewTransition(table, "Idle_Fidget_1 Idle_Fidget_2", "Bored", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.0f, NULL);
    tran->Flags |= 0x20;
    tran = xAnimTableNewTransition(table, "Bored_Fidget_1 Bored_Fidget_2", "Bored", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 2, 0, 0.0f, NULL);
    tran->Flags |= 0x20;
    tran = xAnimTableNewTransition(table, "Bored_Fidget_1 Bored_Fidget_2", "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.0f, NULL);
    tran->Flags |= 0x20;
    tran = xAnimTableNewTransition(table, "Idle", "Idle_Fidget_1", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 8, 0, 0.0f, NULL);
    tran->Flags |= 0x20;
    tran = xAnimTableNewTransition(table, "Idle", "Idle_Fidget_2", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 8, 0, 0.0f, NULL);
    tran->Flags |= 0x20;
    tran = xAnimTableNewTransition(table, "Idle", "Bored_Fidget_1", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 7, 0, 0.0f, NULL);
    tran->Flags |= 0x20;
    tran = xAnimTableNewTransition(table, "Idle", "Bored_Fidget_2", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 7, 0, 0.0f, NULL);
    tran->Flags |= 0x20;
    tran = xAnimTableNewTransition(table, "Bored", "Bored_Fidget_1", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 8, 0, 0.0f, NULL);
    tran->Flags |= 0x20;
    tran = xAnimTableNewTransition(table, "Bored", "Bored_Fidget_2", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 8, 0, 0.0f, NULL);
    tran->Flags |= 0x20;
    tran = xAnimTableNewTransition(table, "Bored", "Idle_Fidget_1", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 7, 0, 0.0f, NULL);
    tran->Flags |= 0x20;
    tran = xAnimTableNewTransition(table, "Bored", "Idle_Fidget_2", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 7, 0, 0.0f, NULL);
    tran->Flags |= 0x20;
    return table;
}

// Small enough to be auto-inlined into its export; the out-of-line copy is
// dead-stripped by the linker, but its string literals keep their place.
#pragma auto_inline on
static xAnimTable* VANM_mktable_SIMP()
{
    // Unreferenced in the original as well (only the templates remain).
    char* unused_names[32] = {};
    xAnimTransition* unused_tran[147] = {};
    xAnimTable* table;
    xAnimTransition* tran;
    signed int i;

    table = xAnimTableNew("Villain_Simple", NULL);
    xAnimTableNewState(table, "Idle", 0x110, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "SIMP_Action", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewTransition(table, g_animname[146], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, g_animname[146], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 5, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Idle", g_animname[146], (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    return table;
}

#pragma auto_inline off
static xAnimTable* VANM_mktable_GANG()
{
    xAnimTable* table;
    xAnimTransition* tran;
    signed int i;
    char* names[32] = {};
    xAnimTransition* tr2DC[147] = {};
    xAnimTransition* tr90[147] = {};

    table = xAnimTableNew("Villain_Gang", NULL);
    xAnimTableNewState(table, "Idle", 0x110, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_Idle_1", 0x110, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_Idle_2", 0x110, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_Idle_3", 0x110, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_IdleFidget", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_IdleFidget_1", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_IdleFidget_2", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_IdleFidget_3", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_Turn", 0x10, 0x02000000, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_Walk", 0x10, 0x02000004, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_Run", 0x10, 0x02000006, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_RunChased", 0x10, 0x02000006, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_Action", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_BossAction", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_Talk", 0x10, 0x02000000, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_Talk_1", 0x10, 0x02000000, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_Talk_2", 0x10, 0x02000000, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_Talk_3", 0x10, 0x02000000, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "GANG_TalkAndTurn", 0x10, 0x02000000, 1.0f, NULL, NULL, 0.0f, NULL);
    for (i = 122; i < 140; i++)
    {
        if (tr90[1] == NULL)
        {
            tr90[1] = xAnimTableNewTransition(table, g_animname[i], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.5f, NULL);
        }
        else
        {
            xAnimTableAddTransition(table, tr90[1], g_animname[i]);
        }
        if (tr2DC[1] == NULL)
        {
            tr2DC[1] = xAnimTableNewTransition(table, g_animname[i], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 5, 0, 0.5f, NULL);
        }
        else
        {
            xAnimTableAddTransition(table, tr2DC[1], g_animname[i]);
        }
        tr2DC[i] = xAnimTableNewTransition(table, "Idle", g_animname[i], (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.5f, NULL);
    }
    xAnimTableNewTransition(table, "GANG_IdleFidget_1", "GANG_Idle_1", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 5, 0, 0.5f, NULL);
    xAnimTableNewTransition(table, "GANG_IdleFidget_2", "GANG_Idle_2", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 5, 0, 0.5f, NULL);
    xAnimTableNewTransition(table, "GANG_IdleFidget_3", "GANG_Idle_3", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 5, 0, 0.5f, NULL);
    xAnimTableAddTransition(table, tr2DC[122], "GANG_Idle_2 GANG_Idle_3");
    xAnimTableAddTransition(table, tr2DC[123], "GANG_Idle_1 GANG_Idle_3");
    xAnimTableAddTransition(table, tr2DC[124], "GANG_Idle_1 GANG_Idle_2");
    names[0] = "GANG_Idle_1";
    names[1] = "GANG_Idle_2";
    names[2] = "GANG_Idle_3";
    for (i = 0; i < 3; i++)
    {
        xAnimTableAddTransition(table, tr2DC[125], names[i]);
        xAnimTableAddTransition(table, tr2DC[126], names[i]);
        xAnimTableAddTransition(table, tr2DC[127], names[i]);
        xAnimTableAddTransition(table, tr2DC[128], names[i]);
    }
    names[0] = "GANG_IdleFidget";
    names[1] = "GANG_IdleFidget_1";
    names[2] = "GANG_IdleFidget_2";
    names[3] = "GANG_IdleFidget_3";
    names[4] = "GANG_Turn";
    names[5] = "GANG_Walk";
    names[6] = "GANG_Run";
    names[7] = "GANG_RunChased";
    names[8] = "GANG_Talk";
    names[9] = "GANG_Talk_1";
    names[10] = "GANG_Talk_2";
    names[11] = "GANG_Talk_3";
    names[12] = "GANG_TalkAndTurn";
    for (i = 0; i < 13; i++)
    {
        xAnimTableAddTransition(table, tr2DC[122], names[i]);
        xAnimTableAddTransition(table, tr2DC[123], names[i]);
        xAnimTableAddTransition(table, tr2DC[124], names[i]);
    }
    names[0] = "GANG_Idle_1";
    names[1] = "GANG_Idle_2";
    names[2] = "GANG_Idle_3";
    names[3] = "GANG_IdleFidget";
    names[4] = "GANG_IdleFidget_1";
    names[5] = "GANG_IdleFidget_2";
    names[6] = "GANG_IdleFidget_3";
    names[7] = "GANG_Turn";
    names[8] = "GANG_Walk";
    names[9] = "GANG_Run";
    names[10] = "GANG_RunChased";
    names[11] = "GANG_Talk";
    names[12] = "GANG_Talk_1";
    names[13] = "GANG_Talk_2";
    names[14] = "GANG_Talk_3";
    names[15] = "GANG_TalkAndTurn";
    for (i = 0; i < 16; i++)
    {
        xAnimTableAddTransition(table, tr2DC[133], names[i]);
        xAnimTableAddTransition(table, tr2DC[134], names[i]);
    }
    xAnimTableAddTransition(table, tr2DC[130], "GANG_Run GANG_RunChased");
    xAnimTableAddTransition(table, tr2DC[131], "GANG_Walk GANG_RunChased");
    xAnimTableAddTransition(table, tr2DC[132], "GANG_Walk GANG_Run");
    names[0] = "GANG_Idle_1";
    names[1] = "GANG_Idle_2";
    names[2] = "GANG_Idle_3";
    names[3] = "GANG_IdleFidget";
    names[4] = "GANG_IdleFidget_1";
    names[5] = "GANG_IdleFidget_2";
    names[6] = "GANG_IdleFidget_3";
    names[7] = "GANG_Action";
    names[8] = "GANG_BossAction";
    names[9] = "GANG_Talk";
    names[10] = "GANG_Talk_1";
    names[11] = "GANG_Talk_2";
    names[12] = "GANG_Talk_3";
    names[13] = "GANG_TalkAndTurn";
    for (i = 0; i < 14; i++)
    {
        xAnimTableAddTransition(table, tr2DC[129], names[i]);
        xAnimTableAddTransition(table, tr2DC[130], names[i]);
        xAnimTableAddTransition(table, tr2DC[131], names[i]);
        xAnimTableAddTransition(table, tr2DC[132], names[i]);
    }
    names[0] = "GANG_Talk";
    names[1] = "GANG_Talk_1";
    names[2] = "GANG_Talk_2";
    names[3] = "GANG_Talk_3";
    names[4] = "GANG_TalkAndTurn";
    for (i = 0; i < 5; i++)
    {
        for (signed int j = 0; j < 5; j++)
        {
            if (i != j)
            {
                xAnimTableNewTransition(table, names[i], names[j], (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.5f, NULL);
            }
        }
    }
    names[2] = "GANG_IdleFidget_1";
    names[1] = "GANG_IdleFidget";
    names[2] = "GANG_IdleFidget_2";
    names[3] = "GANG_IdleFidget_3";
    names[4] = "GANG_Turn";
    names[5] = "GANG_Walk";
    names[6] = "GANG_Run";
    names[7] = "GANG_RunChased";
    names[8] = "GANG_Idle_1";
    names[9] = "GANG_Idle_2";
    names[10] = "GANG_Idle_3";
    for (i = 0; i < 11; i++)
    {
        xAnimTableAddTransition(table, tr2DC[135], names[i]);
        xAnimTableAddTransition(table, tr2DC[136], names[i]);
        xAnimTableAddTransition(table, tr2DC[137], names[i]);
        xAnimTableAddTransition(table, tr2DC[138], names[i]);
        xAnimTableAddTransition(table, tr2DC[139], names[i]);
    }
    return table;
}

static xAnimTable* VANM_mktable_STD()
{
    xAnimTable* table;
    xAnimTransition* tran;
    signed int i;
    char* names[32] = {};
    xAnimTransition* tr2DC[147] = {};
    xAnimTransition* tr90[147] = {};

    table = xAnimTableNew("Villain_Standard", NULL);
    xAnimTableNewState(table, "Idle", 0x110, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Bored", 0x110, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_Fidget_1", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Idle_Fidget_2", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Bored_Fidget_1", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Bored_Fidget_2", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Fashion_Pose", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Move", 0x110, 0x02000004, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Turn", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Chase", 0x110, 0x02000006, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Action_OnceNoMove", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Action_LoopAndMove", 0x110, 0x02000004, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Surprize", 0x20, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Taunt", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Taunt_1", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Taunt_2", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Hear_Start", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Hear_Loop", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Look_Start", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Look_Loop", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Investigate_Start", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Investigate", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "AttackIdle", 0x10, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Attack", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Attack_1", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Attack_2", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Combo", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Combo_1", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Combo_2", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Lob_Attack", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Lob_Combo", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Extend_Attack_Start", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Extend_Attack_Loop", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Extend_Combo", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Hit_By_Gum", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Gummed", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Gummed_Resist", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Gummed_BreakFree", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Hit_When_Gummed", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Bounced_When_Gummed", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Hit_By_Soap", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Soaped", 0x10, 0x04000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Soaped_Resist", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Soaped_BreakFree", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Hit_When_Soaped", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Bounced_When_Soaped", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Dazed_Begin", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Dazed", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Dazed_Recovery", 0x20, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Death", 0, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Death_From_Above", 0, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Death_From_Dazed", 0, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Death_Idle", 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Jump_Begin", 0x20, 0x0200000A, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Jump_Loop", 0x10, 0x0200000A, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Jump_Land", 0x20, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Fall_Loop", 0x20, 0x0200000A, 1.0f, NULL, NULL, 0.0f, NULL);
    xAnimTableNewState(table, "Fall_Splat", 0x20, 0x02000001, 1.0f, NULL, NULL, 0.0f, NULL);
    for (i = 2; i < 59; i++)
    {
        if (tr90[1] == NULL)
        {
            tr90[1] = xAnimTableNewTransition(table, g_animname[i], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
        }
        else
        {
            xAnimTableAddTransition(table, tr90[1], g_animname[i]);
        }
        if (tr2DC[1] == NULL)
        {
            tr2DC[1] = xAnimTableNewTransition(table, g_animname[i], "Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 5, 0, 0.1f, NULL);
        }
        else
        {
            xAnimTableAddTransition(table, tr2DC[1], g_animname[i]);
        }
        tr2DC[i] = xAnimTableNewTransition(table, "Idle", g_animname[i], (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    xAnimTableAddTransition(table, tr90[1], "Idle_Fidget_1 Idle_Fidget_2");
    tr90[4] = xAnimTableNewTransition(table, "Bored_Fidget_1 Bored_Fidget_2", "Bored", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 0xF, 0, 0.1f, NULL);
    names[0] = "Idle";
    names[1] = "Bored";
    for (i = 0; i < 2; i++)
    {
        xAnimTableAddTransition(table, tr2DC[2], names[i]);
        xAnimTableAddTransition(table, tr2DC[3], names[i]);
        xAnimTableAddTransition(table, tr2DC[5], names[i]);
        xAnimTableAddTransition(table, tr2DC[6], names[i]);
    }
    names[0] = "Turn";
    names[1] = "Surprize";
    names[2] = "Investigate_Start";
    names[3] = "Investigate";
    names[4] = "Hear_Start";
    names[5] = "Hear_Loop";
    names[6] = "Look_Start";
    names[7] = "Look_Loop";
    names[8] = "Gummed";
    names[9] = "Soaped";
    names[10] = "Dazed";
    names[11] = "Dazed_Recovery";
    names[12] = "Attack";
    names[13] = "Attack_1";
    names[14] = "Attack_2";
    names[15] = "Lob_Attack";
    names[16] = "Extend_Attack_Loop";
    names[17] = "Combo";
    names[18] = "Combo_1";
    names[19] = "Combo_2";
    names[20] = "Taunt";
    names[21] = "Taunt_1";
    names[22] = "Taunt_2";
    names[23] = "Bored";
    for (i = 0; i < 24; i++)
    {
        xAnimTableAddTransition(table, tr2DC[7], names[i]);
        xAnimTableAddTransition(table, tr2DC[10], names[i]);
    }
    names[24] = "Move";
    names[25] = "Chase";
    for (i = 0; i < 26; i++)
    {
        xAnimTableAddTransition(table, tr2DC[11], names[i]);
        xAnimTableNewTransition(table, "Action_OnceNoMove", names[i], (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
        xAnimTableAddTransition(table, tr2DC[12], names[i]);
        xAnimTableNewTransition(table, "Action_LoopAndMove", names[i], (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 0xA, 0, 0.1f, NULL);
    }
    xAnimTableAddTransition(table, tr2DC[10], "Move");
    xAnimTableAddTransition(table, tr2DC[7], "Chase");
    names[0] = "Move";
    names[1] = "Surprize";
    names[2] = "Investigate_Start";
    names[3] = "Investigate";
    names[4] = "Hear_Start";
    names[5] = "Hear_Loop";
    names[6] = "Look_Start";
    names[7] = "Look_Loop";
    names[8] = "Gummed";
    names[9] = "Soaped";
    names[10] = "Dazed";
    names[11] = "Dazed_Recovery";
    names[12] = "Attack";
    names[13] = "Attack_1";
    names[14] = "Attack_2";
    names[15] = "Lob_Attack";
    names[16] = "Extend_Attack_Loop";
    names[17] = "Combo";
    names[18] = "Combo_1";
    names[19] = "Combo_2";
    names[20] = "Taunt";
    names[21] = "Taunt_1";
    names[22] = "Taunt_2";
    names[23] = "Bored";
    names[24] = "Move";
    names[25] = "Chase";
    for (i = 0; i < 26; i++)
    {
        if (tr90[9] == NULL)
        {
            tr90[9] = xAnimTableNewTransition(table, names[i], "Turn", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
        }
        else
        {
            xAnimTableAddTransition(table, tr90[9], names[i]);
        }
        xAnimTableAddTransition(table, tr2DC[9], names[i]);
    }
    names[0] = "Attack";
    names[1] = "Attack_1";
    names[2] = "Attack_2";
    names[3] = "Combo";
    names[4] = "Combo_1";
    names[5] = "Combo_2";
    names[6] = "Lob_Attack";
    names[7] = "Extend_Attack_Loop";
    for (i = 0; i < 8; i++)
    {
        if (tr90[23] == NULL)
        {
            tr90[23] = xAnimTableNewTransition(table, names[i], "AttackIdle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.1f, NULL);
        }
        else
        {
            xAnimTableAddTransition(table, tr90[23], names[i]);
        }
    }
    tr90[33] = xAnimTableNewTransition(table, "Extend_Attack_Start", "Extend_Attack_Loop", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    names[0] = "Move";
    names[1] = "Turn";
    names[2] = "Chase";
    names[3] = "Surprize";
    names[4] = "AttackIdle";
    names[5] = "Taunt";
    names[6] = "Taunt_1";
    names[7] = "Taunt_2";
    for (i = 0; i < 8; i++)
    {
        xAnimTableAddTransition(table, tr2DC[24], names[i]);
        xAnimTableAddTransition(table, tr2DC[25], names[i]);
        xAnimTableAddTransition(table, tr2DC[26], names[i]);
        xAnimTableAddTransition(table, tr2DC[30], names[i]);
        xAnimTableAddTransition(table, tr2DC[32], names[i]);
        xAnimTableAddTransition(table, tr2DC[33], names[i]);
    }
    names[0] = "Attack";
    names[1] = "Attack_1";
    names[2] = "Attack_2";
    for (i = 0; i < 3; i++)
    {
        xAnimTableAddTransition(table, tr2DC[27], names[i]);
        xAnimTableAddTransition(table, tr2DC[28], names[i]);
        xAnimTableAddTransition(table, tr2DC[29], names[i]);
    }
    xAnimTableAddTransition(table, tr2DC[31], "Lob_Attack");
    xAnimTableAddTransition(table, tr2DC[34], "Extend_Attack_Loop");
    names[0] = "Move";
    names[1] = "Chase";
    names[2] = "Surprize";
    names[3] = "Soaped";
    names[4] = "Gummed";
    names[5] = "Hear_Start";
    names[6] = "Hear_Loop";
    names[7] = "Look_Start";
    names[8] = "Look_Loop";
    names[9] = "Investigate_Start";
    names[10] = "Investigate";
    names[11] = "Attack";
    names[12] = "Attack_1";
    names[13] = "Attack_2";
    names[14] = "Lob_Attack";
    names[15] = "Extend_Attack_Start";
    names[16] = "Extend_Attack_Loop";
    names[17] = "Combo";
    names[18] = "Combo_1";
    names[19] = "Combo_2";
    names[20] = "Lob_Combo";
    names[21] = "Extend_Combo";
    names[22] = "AttackIdle";
    names[23] = "Turn";
    names[24] = "Bored";
    for (i = 0; i < 25; i++)
    {
        xAnimTableAddTransition(table, tr2DC[14], names[i]);
        xAnimTableAddTransition(table, tr2DC[15], names[i]);
        xAnimTableAddTransition(table, tr2DC[16], names[i]);
        if (i != 0x16)
        {
            xAnimTableAddTransition(table, tr2DC[23], names[i]);
        }
    }
    tr90[18] = xAnimTableNewTransition(table, "Hear_Start", "Hear_Loop", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    names[0] = "Move";
    names[1] = "Bored";
    names[2] = "Turn";
    names[3] = "Chase";
    names[4] = "Gummed";
    names[5] = "Soaped";
    names[6] = "Dazed";
    names[7] = "Dazed_Recovery";
    names[8] = "Investigate_Start";
    names[9] = "Investigate";
    names[10] = "Look_Start";
    names[11] = "Look_Loop";
    names[12] = "Surprize";
    for (i = 0; i < 13; i++)
    {
        xAnimTableAddTransition(table, tr2DC[17], names[i]);
        xAnimTableAddTransition(table, tr2DC[18], names[i]);
    }
    tr90[20] = xAnimTableNewTransition(table, "Look_Start", "Look_Loop", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    names[0] = "Move";
    names[1] = "Bored";
    names[2] = "Turn";
    names[3] = "Chase";
    names[4] = "Gummed";
    names[5] = "Soaped";
    names[6] = "Dazed";
    names[7] = "Dazed_Recovery";
    names[8] = "Investigate_Start";
    names[9] = "Investigate";
    names[10] = "Hear_Start";
    names[11] = "Hear_Loop";
    names[12] = "Surprize";
    for (i = 0; i < 13; i++)
    {
        xAnimTableAddTransition(table, tr2DC[19], names[i]);
        xAnimTableAddTransition(table, tr2DC[20], names[i]);
    }
    names[0] = "Move";
    names[1] = "Bored";
    names[2] = "Turn";
    names[3] = "Chase";
    names[4] = "Gummed";
    names[5] = "Soaped";
    names[6] = "Dazed";
    names[7] = "Dazed_Recovery";
    names[8] = "Investigate_Start";
    names[9] = "Investigate";
    names[10] = "Hear_Start";
    names[11] = "Hear_Loop";
    names[12] = "Look_Start";
    names[13] = "Look_Loop";
    for (i = 0; i < 14; i++)
    {
        xAnimTableAddTransition(table, tr2DC[13], names[i]);
    }
    tr90[22] = xAnimTableNewTransition(table, "Investigate_Start", "Investigate", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    names[0] = "Move";
    names[1] = "Bored";
    names[2] = "Turn";
    names[3] = "Chase";
    names[4] = "Gummed";
    names[5] = "Soaped";
    names[6] = "Dazed";
    names[8] = "Dazed_Recovery";
    names[9] = "Hear_Start";
    names[10] = "Hear_Loop";
    names[11] = "Look_Start";
    names[12] = "Look_Loop";
    names[13] = "Surprize";
    for (i = 0; i < 14; i++)
    {
        xAnimTableAddTransition(table, tr2DC[22], names[i]);
        xAnimTableAddTransition(table, tr2DC[21], names[i]);
    }
    names[0] = "Hit_By_Soap";
    names[1] = "Soaped_Resist";
    names[2] = "Hit_When_Soaped";
    names[3] = "Bounced_When_Soaped";
    for (i = 0; i < 4; i++)
    {
        if (tr90[42] == NULL)
        {
            tr90[42] = xAnimTableNewTransition(table, names[i], "Soaped", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
        }
        else
        {
            xAnimTableAddTransition(table, tr90[42], names[i]);
        }
    }
    xAnimTableAddTransition(table, tr2DC[43], "Soaped");
    xAnimTableAddTransition(table, tr2DC[44], "Soaped");
    xAnimTableAddTransition(table, tr2DC[45], "Soaped");
    xAnimTableAddTransition(table, tr2DC[46], "Soaped");
    names[0] = "Hit_By_Gum";
    names[1] = "Gummed_Resist";
    names[2] = "Hit_When_Gummed";
    names[3] = "Bounced_When_Gummed";
    for (i = 0; i < 4; i++)
    {
        if (tr90[36] == NULL)
        {
            tr90[36] = xAnimTableNewTransition(table, names[i], "Gummed", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
        }
        else
        {
            xAnimTableAddTransition(table, tr90[36], names[i]);
        }
    }
    xAnimTableAddTransition(table, tr2DC[37], "Gummed");
    xAnimTableAddTransition(table, tr2DC[38], "Gummed");
    xAnimTableAddTransition(table, tr2DC[39], "Gummed");
    xAnimTableAddTransition(table, tr2DC[40], "Gummed");
    names[0] = "Move";
    names[1] = "Chase";
    names[2] = "Hear_Start";
    names[3] = "Hear_Loop";
    names[4] = "Look_Start";
    names[5] = "Look_Loop";
    names[6] = "Investigate_Start";
    names[7] = "Investigate";
    names[8] = "Surprize";
    names[9] = "Taunt";
    names[10] = "Taunt_1";
    names[11] = "Taunt_2";
    names[12] = "AttackIdle";
    names[13] = "Attack";
    names[14] = "Attack_1";
    names[15] = "Attack_2";
    names[16] = "Lob_Attack";
    names[17] = "Lob_Combo";
    names[18] = "Extend_Attack_Start";
    names[19] = "Extend_Attack_Loop";
    names[20] = "Extend_Combo";
    names[21] = "Combo";
    names[22] = "Combo_1";
    names[23] = "Combo_2";
    names[24] = "Dazed";
    names[25] = "Dazed_Begin";
    names[26] = "Dazed_Recovery";
    names[27] = "Bored";
    for (i = 0; i < 28; i++)
    {
        xAnimTableAddTransition(table, tr2DC[41], names[i]);
        xAnimTableAddTransition(table, tr2DC[46], names[i]);
        xAnimTableAddTransition(table, tr2DC[42], names[i]);
        xAnimTableAddTransition(table, tr2DC[35], names[i]);
        xAnimTableAddTransition(table, tr2DC[40], names[i]);
        xAnimTableAddTransition(table, tr2DC[36], names[i]);
    }
    tr90[48] = xAnimTableNewTransition(table, "Dazed_Begin", "Dazed", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    names[0] = "Move";
    names[1] = "Turn";
    names[2] = "Chase";
    names[3] = "Hear_Start";
    names[4] = "Hear_Loop";
    names[5] = "Look_Start";
    names[6] = "Look_Loop";
    names[7] = "Investigate_Start";
    names[8] = "Investigate";
    names[9] = "Taunt";
    names[10] = "Taunt_1";
    names[11] = "Taunt_2";
    names[12] = "Surprize";
    names[13] = "Bored";
    names[14] = "Lob_Attack";
    names[15] = "Lob_Combo";
    names[16] = "Extend_Attack_Start";
    names[17] = "Extend_Attack_Loop";
    names[18] = "AttackIdle";
    names[19] = "Attack";
    names[20] = "Attack_1";
    names[21] = "Attack_2";
    for (i = 0; i < 22; i++)
    {
        xAnimTableAddTransition(table, tr2DC[48], names[i]);
        xAnimTableAddTransition(table, tr2DC[47], names[i]);
    }
    xAnimTableAddTransition(table, tr2DC[49], "Dazed");
    xAnimTableAddTransition(table, tr2DC[52], "Dazed");
    xAnimTableAddTransition(table, tr2DC[50], "Dazed");
    xAnimTableAddTransition(table, tr2DC[51], "Dazed");
    for (i = 2; i < 59; i++)
    {
        if (i != 0x32)
        {
            xAnimTableNewTransition(table, g_animname[i], "Death", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 2, 0, 0.1f, NULL);
        }
        if (i != 0x34)
        {
            xAnimTableNewTransition(table, g_animname[i], "Death_From_Dazed", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 2, 0, 0.1f, NULL);
        }
        if (i != 0x33)
        {
            xAnimTableNewTransition(table, g_animname[i], "Death_From_Above", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dotran, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_didtran, 0, 0, 0.0f, 0.0f, 2, 0, 0.1f, NULL);
        }
    }
    names[0] = "Death";
    names[1] = "Death_From_Dazed";
    names[2] = "Death_From_Above";
    for (i = 0; i < 3; i++)
    {
        if (tr90[53] == NULL)
        {
            xAnimTableNewTransition(table, names[i], "Death_Idle", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
        }
        else
        {
            xAnimTableAddTransition(table, tr90[53], names[i]);
        }
        xAnimTableAddTransition(table, tr2DC[53], names[i]);
    }
    xAnimTableAddTransition(table, tr2DC[54], "Fashion_Pose");
    xAnimTableAddTransition(table, tr2DC[55], "Fashion_Pose");
    xAnimTableNewTransition(table, "Jump_Begin", "Jump_Loop", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableNewTransition(table, "Jump_Loop", "Jump_Land", (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_dodflt, (unsigned int (*)(xAnimTransition*, xAnimSingle*, void*))VANM_diddflt, 0x10, 0, 0.0f, 0.0f, 1, 0, 0.1f, NULL);
    xAnimTableAddTransition(table, tr2DC[56], "Jump_Loop");
    xAnimTableAddTransition(table, tr2DC[58], "Fall_Loop");
    return table;
}

static unsigned int VANM_dotran(xAnimTransition* tran, xAnimSingle* single, st_zEntVillain* ent)
{
    unsigned int dotran = 0;

    if (ent->anim_playnext == ZVIL_ANIM_UNKNOWN)
    {
        dotran = 0;
    }
    else if (ent->anim_playing)
    {
        dotran = 0;
    }
    else if (!tran->Dest)
    {
        dotran = 0;
    }
    else if (!tran->Dest->Name || !tran->Dest->ID)
    {
        dotran = 0;
    }
    else if (tran->Dest->ID == g_anamhash[ent->anim_playnext])
    {
        dotran = 1;
        if (ent->anim_playsync)
        {
            float tmr;

            tran->SrcTime = single->State->Data->Duration;
            ent->tmr_list[ZVIL_TYMR_MINANIMPLAY] = single->State->Data->Duration;
            tmr = ent->tmr_list[ZVIL_TYMR_MINANIMPLAY] - (single->Time - 0.05f);
            ent->tmr_list[ZVIL_TYMR_MINANIMPLAY] = (tmr < -1.0f) ? -1.0f : tmr;
        }
        if (ent->vhf->animtran)
        {
            ent->vhf->animtran(ent, tran, single);
        }
    }

    return dotran;
}

static unsigned int VANM_didtran(xAnimTransition* tran, xAnimSingle* single, st_zEntVillain* ent)
{
    float tym = 0.0f;

    ent->anim_playing = 1;
    ent->anim_playnext = VANM_hashToAnim(single->State->ID);
    float mult = ent->anim_loopmult;
    float dur = VANM_duration(single->State, ent, &tym);
    ent->tmr_list[ZVIL_TYMR_MINANIMPLAY] = dur * mult;
    if (single->State->Flags & 0x10)
    {
        ent->anim_loopmult = 0.97f;
    }
    ent->anim_playsync = 0;
    return 1;
}

static unsigned int VANM_dodflt(xAnimTransition* tran, xAnimSingle* single, st_zEntVillain* ent)
{
    return 1;
}

static unsigned int VANM_diddflt(xAnimTransition* tran, xAnimSingle* single, st_zEntVillain* ent)
{
    float tym = 0.0f;

    ent->anim_playing = 1;
    ent->anim_playnext = VANM_hashToAnim(single->State->ID);
    float mult = ent->anim_loopmult;
    float dur = VANM_duration(single->State, ent, &tym);
    ent->tmr_list[ZVIL_TYMR_MINANIMPLAY] = dur * mult;
    if (single->State->Flags & 0x10)
    {
        ent->anim_loopmult = 0.97f;
    }
    ent->anim_playsync = 0;
    return 1;
}

static inline en_NPC_ANIMSTATES VANM_hashToAnim(unsigned int id)
{
    en_NPC_ANIMSTATES anim = ZVIL_ANIM_UNKNOWN;
    signed int i;

    for (i = 0; i < 147; i++)
    {
        if (id == g_anamhash[i])
        {
            anim = (en_NPC_ANIMSTATES)i;
            break;
        }
    }
    return anim;
}

// TODO: 99.9%. The duration multiply has its operands swapped.
static en_NPC_ANIMSTATES VANM_set(st_zEntVillain* ent, en_NPC_ANIMSTATES* best, signed int cnt, signed int sync)
{
    xAnimState* astate = NULL;
    xAnimState* cur;
    signed int i;

    if (!best)
    {
        return ent->anim_playnext;
    }
    if (cnt < 1)
    {
        return ent->anim_playnext;
    }

    cur = VANM_entState(ent);
    for (i = 0; i < cnt; i++)
    {
        astate = xAnimTableGetStateID(ent->model->Anim->Table, g_anamhash[best[i]]);
        if (!astate)
        {
            continue;
        }
        // States flagged 0x40000000 are skipped: try the next candidate.
        if (astate->UserFlags & 0x40000000)
        {
            astate = NULL;
            continue;
        }
        if (!astate)
        {
            continue;
        }
        if (ent->anim_playnext != best[i])
        {
            ent->anim_playnext = best[i];
            ent->anim_playing = 0;
            ent->anim_playsync = sync;
            ent->anim_loopmult = 0.97f;
            ent->tmr_list[ZVIL_TYMR_MINANIMPLAY] = VANM_duration(astate, ent, NULL) * 0.97f;
        }
        else
        {
            astate = NULL;
        }
        break;
    }

    if (cur && astate && astate->ID != cur->ID)
    {
        VANM_starttran(ent, cur, astate);
    }

    return ent->anim_playnext;
}

static inline void VANM_starttran(st_zEntVillain* ent, xAnimState* from, xAnimState* to)
{
    xAnimTable* table = ent->model->Anim->Table;
    xAnimTransition* tran = NULL;

    if (from && to && from->ID != to->ID)
    {
        if (!VANM_findtran(ent, table, from, to, 1, &tran) && tran)
        {
            xAnimPlayStartTransition(ent->model->Anim, tran);
        }
    }
}

static en_NPC_ANIMSTATES VANM_randset(st_zEntVillain* ent, float* wts, en_NPC_ANIMSTATES* choice_list,
                                      signed int cnt)
{
    en_NPC_ANIMSTATES fnl_anim;
    signed int i;
    xAnimState* chk_ast;
    signed int* act_list;
    signed int use_anim;
    float* act_wts = NULL;
    signed int act_cnt = 0;

    if (!choice_list)
    {
        return ent->anim_playnext;
    }
    if (cnt < 1)
    {
        return ent->anim_playnext;
    }

    act_list = (signed int*)xMemPushTemp(cnt * sizeof(signed int));
    memset(act_list, 0, cnt * sizeof(signed int));
    if (wts)
    {
        act_wts = (float*)xMemPushTemp(cnt * sizeof(float));
        memset(act_wts, 0, cnt * sizeof(float));
    }

    for (i = 0; i < cnt; i++)
    {
        chk_ast = xAnimTableGetStateID(ent->model->Anim->Table, g_anamhash[choice_list[i]]);
        if (chk_ast && !(chk_ast->UserFlags & 0x40000000))
        {
            act_list[act_cnt] = choice_list[i];
            if (act_wts)
            {
                act_wts[act_cnt] = wts[i];
            }
            act_cnt++;
        }
    }

    if (act_cnt == 0)
    {
        fnl_anim = ent->anim_playnext;
    }
    else if (act_cnt == 1)
    {
        fnl_anim = VANM_set1(ent, (en_NPC_ANIMSTATES)act_list[0]);
    }
    else
    {
        if (act_wts)
        {
            VSUP_wtadjust(act_wts, act_cnt, 1.0f);
        }
        use_anim = VSUP_choose(act_list, act_wts, act_cnt);
        fnl_anim = VANM_set1(ent, (en_NPC_ANIMSTATES)use_anim);
    }

    xMemPopTemp(act_list);
    if (act_wts)
    {
        xMemPopTemp(act_wts);
    }
    return fnl_anim;
}

static inline en_NPC_ANIMSTATES VANM_set1(st_zEntVillain* ent, en_NPC_ANIMSTATES anim)
{
    en_NPC_ANIMSTATES list[2] = { ZVIL_ANIM_UNKNOWN, ZVIL_ANIM_IDLE };

    list[0] = anim;
    return VANM_set(ent, list, 2, 0);
}

static xAnimTransition* VANM_findtran(st_zEntVillain* ent, xAnimTable* table, xAnimState* from, xAnimState* to,
                                      signed int autoslamit, xAnimTransition** newtran)
{
    xAnimTransition* tmp_tran;
    xAnimTransitionList* tmp_tranlist;
    xAnimTransition* smack;
    xAnimTransition* da_tran = NULL;

    if (!from || !to)
    {
        return NULL;
    }
    if (from->ID == to->ID)
    {
        return NULL;
    }

    if (!from->Default)
    {
        if (from->ID != g_anamhash[ZVIL_ANIM_IDLE])
        {
            xSceneID2Name(globals.sceneCur, ent->id);
        }
    }
    else if (!(tmp_tran = from->Default->T))
    {
        if (from->ID != g_anamhash[ZVIL_ANIM_IDLE])
        {
            xSceneID2Name(globals.sceneCur, ent->id);
        }
    }
    else if (to->ID == tmp_tran->Dest->ID)
    {
        da_tran = tmp_tran;
    }

    if (!da_tran)
    {
        tmp_tranlist = from->List;
        while (tmp_tranlist)
        {
            tmp_tran = tmp_tranlist->T;
            if (!tmp_tran)
            {
                tmp_tranlist = tmp_tranlist->Next;
                continue;
            }
            if (to->ID == tmp_tran->Dest->ID)
            {
                da_tran = tmp_tran;
                break;
            }
            tmp_tranlist = tmp_tranlist->Next;
        }
    }

    if (!da_tran)
    {
        xSceneID2Name(globals.sceneCur, ent->id);
        if (autoslamit && newtran)
        {
            smack = xAnimTempTransitionAlloc();
            smack->Dest = to;
            smack->Callback = NULL;
            smack->SrcTime = 0.0f;
            smack->DestTime = 0.0f;
            smack->BlendRecip = 10.0f;
            smack->BlendOffset = NULL;
            *newtran = smack;
        }
    }

    return da_tran;
}

static inline float VANM_duration(xAnimState* astate, st_zEntVillain* ent, float* tym)
{
    float dur = 0.0f;

    if (astate)
    {
        dur = astate->Data->Duration;
    }
    else if (ent)
    {
        xAnimState* cur = VANM_entState(ent);
        if (cur)
        {
            dur = cur->Data->Duration;
        }
    }
    if (tym)
    {
        *tym = dur;
    }
    return dur;
}

static inline xAnimState* VANM_entState(_zEnt* ent)
{
    xAnimSingle* single = VANM_entSingle(ent);

    if (single)
    {
        return single->State;
    }
    return NULL;
}

static inline xAnimSingle* VANM_entSingle(_zEnt* ent)
{
    xAnimSingle* single;

    if (!ent->model)
    {
        single = NULL;
    }
    else if (!ent->model->Anim)
    {
        single = NULL;
    }
    else
    {
        single = ent->model->Anim->Single;
    }
    return single;
}

static inline void VSND_setup_soundTable()
{
    st_NPC_SOUND_ENTRY* entry;

    for (entry = g_npcJukeBox; entry->sndList; entry++)
    {
        st_NPC_SOUND_RECORDS* rec;
        for (rec = entry->sndList; rec->typ_sound != 0x8000; rec++)
        {
            unsigned int aid = xStrHash(rec->nam_sound);
            if (iSndLookup(aid))
            {
                rec->aid_sound = aid;
            }
            else
            {
                rec->aid_sound = 0;
            }
        }
    }
}

// Inline twin of VSND_play: every caller except VILN_std_aware gets the sound
// queued inline in the original, std_aware calls the out-of-line VSND_play.
static inline void VSND_cue(st_zEntVillain* vil, en_NPC_SOUND sndEvent)
{
    st_NPC_SOUND_PROPERTIES* sprop = NULL;
    signed int flg_vsnd;
    float delay;
    float radius;

    flg_vsnd = VSND_findProps(sndEvent, &sprop)->flg_vsnd;
    if (sndEvent != VSND_TYP_HOLOFADE)
    {
        flg_vsnd &= 0x0FFFFFFF;
    }
    delay = sprop->tym_delayStart;
    radius = g_npcTable[vil->asset->subtype].dst_noisy;
    if (sndEvent != VSND_TYP_BOGUS)
    {
        VSND_addQueueEntry(vil, sndEvent, flg_vsnd, radius, delay, -1.0f, 0, 0);
    }
}

static void VSND_play(st_zEntVillain* vil, en_NPC_SOUND sndEvent)
{
    st_NPC_SOUND_PROPERTIES* sprop = NULL;
    signed int flg_vsnd;
    float radius;
    float delay;

    flg_vsnd = VSND_findProps(sndEvent, &sprop)->flg_vsnd;
    if (sndEvent != VSND_TYP_HOLOFADE)
    {
        flg_vsnd &= 0x0FFFFFFF;
    }
    delay = sprop->tym_delayStart;
    radius = g_npcTable[vil->asset->subtype].dst_noisy;
    if (sndEvent != VSND_TYP_BOGUS)
    {
        VSND_addQueueEntry(vil, sndEvent, flg_vsnd, radius, delay, -1.0f, 0, 0);
    }
}

static inline signed int VSND_addQueueEntry(st_zEntVillain* vil, en_NPC_SOUND sndEvent, signed int flg_vsnd,
                                            float radius, float delay, float repeatDelay,
                                            signed int repeatCount, unsigned int useThisSound)
{
    signed int added = 0;
    signed int i;
    st_NPC_SOUND_QUEUE* que;

    for (i = 0; i < 4; i++)
    {
        que = &vil->snd_queue[i];
        if (que->sndEvent == VSND_TYP_BOGUS)
        {
            memset(que, 0, sizeof(st_NPC_SOUND_QUEUE));
            if (useThisSound)
            {
                que->sndEvent = (en_NPC_SOUND)0x8000;
            }
            else
            {
                que->sndEvent = sndEvent;
            }
            que->sndDirect = useThisSound;
            que->flg_frolic = flg_vsnd;
            que->tmr_delay = delay;
            que->cnt_repeat = repeatCount;
            que->radius = radius;
            que->tym_repeatDelay = repeatDelay;
            added = 1;
            break;
        }
    }
    return added;
}

static signed int VSND_procQueue(st_zEntVillain* vil, float dt)
{
    st_NPC_SOUND_QUEUE* que;
    signed int i;
    signed int cnt = 0;

    for (i = 0; i < 4; i++)
    {
        que = &vil->snd_queue[i];
        if (que->sndEvent == VSND_TYP_BOGUS)
        {
            continue;
        }

        if (que->sndEvent == VSND_LISTEND)
        {
            if (!que->sndDirect)
            {
                que->sndEvent = VSND_TYP_BOGUS;
                continue;
            }
        }
        else
        {
            que->sndDirect = 0;
        }

        que->tmr_delay -= dt;
        if (que->tmr_delay > 0.0f)
        {
            continue;
        }

        VSND_goPlay(vil, que->sndEvent, que->sndDirect, que->flg_frolic & 0xFFFFFF, que->radius);

        switch (que->flg_frolic & 0xF0000000)
        {
        case 0x10000000:
            que->cnt_repeat = 1;
            break;
        case 0x20000000:
            que->cnt_repeat--;
            break;
        default:
            que->cnt_repeat = 0;
            break;
        }

        if (que->cnt_repeat > 0)
        {
            que->tmr_delay = que->tym_repeatDelay;
        }
        else
        {
            que->sndEvent = VSND_TYP_BOGUS;
        }

        if (que->sndEvent != VSND_TYP_BOGUS)
        {
            cnt++;
        }
    }

    return cnt;
}

static unsigned int VSND_goPlay(st_zEntVillain* vil, en_NPC_SOUND sndEvent, unsigned int sndExplicit,
                                signed int flg_vsnd, float dist)
{
    float vol;
    unsigned int xsid = 0;
    unsigned int owner;
    unsigned int priority;
    unsigned int aid_toplay;

    aid_toplay = sndExplicit ? sndExplicit : VSND_select_sound(vil, sndEvent);

    if (flg_vsnd & 0x8000)
    {
        if (!(vil->tmr_list[ZVIL_TTIL_NEXTVOICE] <= 0.0f))
        {
            return 0;
        }
    }

    if (flg_vsnd & 0x8000)
    {
        if (VSND_isPlaying(vil, 1))
        {
            vil->tmr_list[ZVIL_TTIL_NEXTVOICE] = VUTIL_jitter(vil->vilcfg.tymref_list[ZVIL_TTIL_NEXTVOICE]);
            return 0;
        }
    }

    if (flg_vsnd & 0x800)
    {
        if (VSND_isPlaying(vil, 3))
        {
            return 0;
        }
    }

    if (flg_vsnd & 0x1000)
    {
        vol = 1.0f;
        priority = 150;
    }
    else if (flg_vsnd & 0x2000)
    {
        vol = 0.9f;
        priority = 100;
    }
    else if (flg_vsnd & 0x4000)
    {
        vol = 0.8f;
        priority = 50;
    }
    else
    {
        vol = 0.75f;
        priority = 100;
    }

    if (g_scene_is_a_boss_battle && !(vil->ability & 1))
    {
        vol *= 0.5f;
    }

    owner = (unsigned int)vil + (flg_vsnd & 3);
    if (aid_toplay && (flg_vsnd & 0x200))
    {
        xsid = xSndPlay(aid_toplay, vol, vol, 0.0f, priority, 0x202, owner);
    }
    else if (aid_toplay)
    {
        xsid = xSndPlay3D(aid_toplay, 0.5f * vol, 0.0f, priority, 0x202, (_xEnt*)owner, dist);
    }

    return xsid;
}

static inline float VUTIL_jitter(float tym)
{
    float rnd = 0.25f * (iurand() - 0.5f);
    return rnd * tym + tym;
}

static inline signed int VSND_isPlaying(st_zEntVillain* vil, signed int channel)
{
    return iSndIsPlaying(0, (unsigned int)vil + channel);
}

// Movement helpers shared by the calcmove handlers (out-of-line on PS2,
// inlined in the GameCube build).

static inline en_NPC_SOUND VSND_typeFromHash(st_zEntVillain* vil, unsigned int aid_snd)
{
    st_NPC_SOUND_RECORDS* trax;
    st_NPC_SOUND_ENTRY* entry;
    en_zEntVillainSubTypeID viltyp;
    en_NPC_SOUND typ_sndEvent = VSND_TYP_BOGUS;

    viltyp = (en_zEntVillainSubTypeID)vil->asset->subtype;
    for (entry = g_npcJukeBox; entry->sndList; entry++)
    {
        if (entry->viltyp != viltyp && entry->viltyp != -1)
        {
            continue;
        }
        for (trax = entry->sndList; trax->typ_sound != VSND_LISTEND; trax++)
        {
            if (trax->aid_sound == aid_snd)
            {
                typ_sndEvent = trax->typ_sound;
                break;
            }
        }
        if (entry->viltyp == viltyp)
        {
            break;
        }
        if (typ_sndEvent != VSND_TYP_BOGUS)
        {
            break;
        }
    }
    return typ_sndEvent;
}

static inline void VANM_play(st_zEntVillain* vil, en_NPC_ANIMSTATES a, en_NPC_ANIMSTATES b)
{
    en_NPC_ANIMSTATES list[2] = { a, b };
    VANM_set(vil, list, 2, 0);
}

static inline void VANM_playLoop(st_zEntVillain* vil, en_NPC_ANIMSTATES a)
{
    en_NPC_ANIMSTATES list[2] = { a, (en_NPC_ANIMSTATES)1 };
    VANM_set(vil, list, 2, 0);
}

static inline void VANM_playRand(st_zEntVillain* vil, en_NPC_ANIMSTATES a, en_NPC_ANIMSTATES b)
{
    en_NPC_ANIMSTATES list[2] = { a, b };
    VANM_randset(vil, NULL, list, 2);
}

static inline void VANM_play3(st_zEntVillain* vil, en_NPC_ANIMSTATES a, en_NPC_ANIMSTATES b, en_NPC_ANIMSTATES c)
{
    en_NPC_ANIMSTATES list[3] = { a, b, c };
    VANM_set(vil, list, 3, 0);
}

static inline void VANM_play4(st_zEntVillain* vil, en_NPC_ANIMSTATES a, en_NPC_ANIMSTATES b, en_NPC_ANIMSTATES c,
                              en_NPC_ANIMSTATES d)
{
    en_NPC_ANIMSTATES list[4] = { a, b, c, d };
    VANM_set(vil, list, 4, 0);
}

static inline void VANM_play5(st_zEntVillain* vil, en_NPC_ANIMSTATES a, en_NPC_ANIMSTATES b, en_NPC_ANIMSTATES c,
                              en_NPC_ANIMSTATES d, en_NPC_ANIMSTATES e)
{
    en_NPC_ANIMSTATES list[5] = { a, b, c, d, e };
    VANM_set(vil, list, 5, 0);
}

static inline void VANM_playRand3(st_zEntVillain* vil, en_NPC_ANIMSTATES a, en_NPC_ANIMSTATES b, en_NPC_ANIMSTATES c)
{
    en_NPC_ANIMSTATES list[3] = { a, b, c };
    VANM_randset(vil, NULL, list, 3);
}

static inline void VANM_playRand4(st_zEntVillain* vil, en_NPC_ANIMSTATES a, en_NPC_ANIMSTATES b, en_NPC_ANIMSTATES c,
                                  en_NPC_ANIMSTATES d)
{
    en_NPC_ANIMSTATES list[4] = { a, b, c, d };
    VANM_randset(vil, NULL, list, 4);
}

static inline void VANM_playRand5(st_zEntVillain* vil, en_NPC_ANIMSTATES a, en_NPC_ANIMSTATES b, en_NPC_ANIMSTATES c,
                                  en_NPC_ANIMSTATES d, en_NPC_ANIMSTATES e)
{
    en_NPC_ANIMSTATES list[5] = { a, b, c, d, e };
    VANM_randset(vil, NULL, list, 5);
}

static unsigned int VSND_select_sound(st_zEntVillain* vil, en_NPC_SOUND sndEvent)
{
    float use_wt;
    signed int cnt;
    float wts[32] = {};
    signed int list[32] = { 0x3F800000 };
    st_NPC_SOUND_RECORDS* trax;
    st_NPC_SOUND_ENTRY* entry;
    en_zEntVillainSubTypeID viltyp;
    unsigned int da_aid_of_sound;

    viltyp = (en_zEntVillainSubTypeID)vil->asset->subtype;
    da_aid_of_sound = 0;
    cnt = 0;

    for (entry = g_npcJukeBox; entry->sndList; entry++)
    {
        if (entry->viltyp == viltyp)
        {
            use_wt = 2.0f;
        }
        else if (entry->viltyp == -1)
        {
            use_wt = 1.0f;
        }
        else
        {
            continue;
        }

        for (trax = entry->sndList; trax->typ_sound != VSND_LISTEND; trax++)
        {
            if (trax->typ_sound == sndEvent && trax->aid_sound)
            {
                wts[cnt] = use_wt;
                list[cnt++] = trax->aid_sound;
            }
            if (cnt >= 32)
            {
                break;
            }
        }

        if (entry->viltyp == viltyp)
        {
            break;
        }
        if (cnt >= 32)
        {
            break;
        }
    }

    if (cnt > 0)
    {
        VSUP_wtadjust(wts, cnt, 1.0f);
        da_aid_of_sound = VSUP_choose((signed int*)list, wts, cnt);
        xSTAssetName(da_aid_of_sound);
    }

    return da_aid_of_sound;
}

static inline st_NPC_SOUND_PROPERTIES* VSND_findProps(en_NPC_SOUND sndEvent, st_NPC_SOUND_PROPERTIES** sprop_caller)
{
    st_NPC_SOUND_PROPERTIES* sprop = g_sndProps;

    while (sprop->sndEvent != VSND_LISTEND)
    {
        if (sprop->sndEvent == sndEvent)
        {
            break;
        }
        sprop++;
    }
    if (sprop_caller)
    {
        *sprop_caller = sprop;
    }
    return sprop;
}

static inline void VBON_setup_bonusTable()
{
    _xVec3 pos_hide = { 867.0f, -5309.0f, 2469.0f };
    st_NPC_BONUS* bonus;
    signed int i;

    memset(g_npcBonus, 0, sizeof(g_npcBonus));
    for (i = 0; i < 11; i++)
    {
        bonus = &g_npcBonus[i];
        bonus->pickup = (_zEntPickup*)zSceneFindObject(xStrHash(g_strz_npcBonus[i]));
        if (bonus->pickup)
        {
            zEntPickup_MakeDroppable(bonus->pickup, &pos_hide, NULL);
            switch (i)
            {
            case 8:
            case 9:
            case 10:
                bonus->flg_info |= 0x10000;
                break;
            case 5:
            case 6:
            case 7:
                bonus->flg_info |= 0x20000;
                break;
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
                bonus->flg_info |= 0x40000;
                break;
            }
        }
    }
    g_bonusTableReady = 1;
}

static void VBON_drop_bonusItem(st_zEntVillain* vil)
{
    signed int overlap = 0;
    signed int skip_distchk = 0;
    _xVec3 vec_tmp = {};
    signed int i;
    st_NPC_BONUS* bon;

    if (!(vil->allow & 0x800))
    {
        return;
    }

    if (g_scene_is_a_boss_battle)
    {
        skip_distchk = 1;
    }

    for (i = 0; i < 11; i++)
    {
        if (skip_distchk)
        {
            break;
        }
        bon = &g_npcBonus[i];
        if (!bon->pickup)
        {
            continue;
        }
        if (!(bon->flg_info & 1))
        {
            continue;
        }
        if (bon->pickup->state & 8)
        {
            continue;
        }
        xVec3Sub(&vec_tmp, (_xVec3*)&bon->pickup->model->Mat->pos, (_xVec3*)&vil->model->Mat->pos);
        if (xVec3Length2(&vec_tmp) < 4.0f)
        {
            overlap = 1;
            break;
        }
    }

    if (overlap)
    {
        return;
    }

    bon = VBON_choose_bonusItem((en_zEntVillainSubTypeID)vil->asset->subtype);
    if (bon)
    {
        bon->flg_info |= 1;
        bon->pickup->flg_opts |= 3;
        bon->pickup->useThisEntPos = vil;
        zEntPickup_MakeDroppable(bon->pickup, xBoundCenter(&vil->bound), NULL);
        zEntPickup_Drop(bon->pickup);
    }
}

static st_NPC_BONUS* VBON_choose_bonusItem(en_zEntVillainSubTypeID vtyp)
{
    signed int cnt;
    signed int choice;
    float rat;
    float soap_weight;
    float gum_weight;
    float food_weight;
    signed int soap_always;
    signed int wt_ramp;
    float wts[11] = {};
    signed int list[11] = {};
    _xVec3 farfar = { -9999.0f, -8888.0f, -7777.0f };
    signed int i;
    unsigned int pfx;

    cnt = 0;
    soap_always = 0;
    wt_ramp = 0;
    pfx = globals.sceneCur->sceneID & 0xFF000000;
    if (pfx == 'P\0\0\0' || pfx == 'B\0\0\0' || pfx == 'S\0\0\0')
    {
        wt_ramp = 1;
    }

    if (g_scene_is_a_boss_battle)
    {
        if (g_scene_is_a_boss_battle != 4 && !xUtil_yesno(0.8f))
        {
            return NULL;
        }
    }
    else if (vtyp == eEntVillainTypeBat && pfx == 'P\0\0\0')
    {
        soap_always = 1;
    }
    else if (wt_ramp)
    {
        if (!xUtil_yesno(0.8f))
        {
            return NULL;
        }
    }
    else if (!xUtil_yesno(0.4f))
    {
        return NULL;
    }

    rat = (float)globals.player.Inv_Ammo[0] / (float)globals.player.Inv_AmmoMax[0];
    if (rat < 0.2f)
    {
        gum_weight = 1.5f;
    }
    else if (rat > 0.8f)
    {
        gum_weight = 0.5f;
    }
    else
    {
        gum_weight = 1.0f;
    }

    rat = (float)globals.player.Inv_Ammo[1] / (float)globals.player.Inv_AmmoMax[1];
    if (rat < 0.2f)
    {
        soap_weight = 1.5f;
    }
    else if (rat > 0.8f)
    {
        soap_weight = 0.5f;
    }
    else
    {
        soap_weight = 1.0f;
    }

    if (globals.player.Health == 1)
    {
        food_weight = 2.0f;
    }
    else if (globals.player.Health == 4)
    {
        food_weight = 0.5f;
    }
    else
    {
        food_weight = 1.0f;
    }

    if (soap_always)
    {
        gum_weight = 0.0f;
        soap_weight = 100.0f;
        food_weight = gum_weight;
    }
    else if (g_scene_is_a_boss_battle)
    {
        gum_weight = 1.0f;
        soap_weight = gum_weight;
        food_weight = gum_weight;
    }
    else if (wt_ramp)
    {
        gum_weight *= 4.0f;
        soap_weight *= 4.0f;
        food_weight *= 0.25f;
    }

    for (i = 0; i < 11; i++)
    {
        st_NPC_BONUS* bon = &g_npcBonus[i];

        if (!bon->pickup)
        {
            continue;
        }
        if (soap_always && !(bon->flg_info & 0x20000))
        {
            continue;
        }
        if (bon->flg_info & 0x10000)
        {
            if (globals.player.Inv_AmmoMax[0] == 0 ||
                globals.player.Inv_Ammo[0] == globals.player.Inv_AmmoMax[0])
            {
                continue;
            }
        }
        if (bon->flg_info & 0x20000)
        {
            if (globals.player.Inv_AmmoMax[1] == 0 ||
                globals.player.Inv_Ammo[1] == globals.player.Inv_AmmoMax[1])
            {
                continue;
            }
        }
        if (bon->flg_info & 0x40000)
        {
            if (globals.player.Health == 5)
            {
                continue;
            }
        }

        if ((bon->flg_info & 1) && (bon->pickup->state & 8))
        {
            bon->flg_info &= ~1;
            zEntPickup_Reset(bon->pickup);
            zEntPickup_MakeDroppable(bon->pickup, &farfar, NULL);
        }

        if (bon->flg_info & 1)
        {
            continue;
        }

        gum_weight *= 4.0f;
        soap_weight *= 4.0f;
        food_weight *= 0.25f;

        switch (i)
        {
        case 8:
        case 9:
        case 10:
            if (globals.player.Inv_Specials & 1)
            {
                wts[cnt] = gum_weight;
                list[cnt++] = i;
            }
            break;
        case 5:
        case 6:
        case 7:
            if (globals.player.Inv_Specials & 2)
            {
                wts[cnt] = soap_weight;
                list[cnt++] = i;
            }
            break;
        case 0:
            wts[cnt] = food_weight;
            list[cnt++] = 0;
            break;
        case 1:
            if (globals.player.Inv_Specials & 4)
            {
                list[cnt] = 1;
                wts[cnt++] = 0.25f + food_weight;
            }
            break;
        case 3:
            if (globals.player.Inv_Specials & 0x200)
            {
                list[cnt] = 3;
                wts[cnt++] = 0.5f + food_weight;
            }
            break;
        case 4:
            if (globals.player.Inv_Specials & 0x20)
            {
                list[cnt] = 4;
                wts[cnt++] = 1.0f + food_weight;
            }
            break;
        case 2:
            if (globals.player.Inv_Specials & 0x1000)
            {
                list[cnt] = 2;
                wts[cnt++] = 0.75f + food_weight;
            }
            break;
        }
    }

    if (cnt < 1)
    {
        return NULL;
    }

    VSUP_wtadjust(wts, cnt, 1.0f);
    choice = VSUP_choose(list, wts, cnt);
    return &g_npcBonus[choice];
}

// Entity position / bound centre by value.


static inline void VOUT_steer(st_zEntVillain* vil)
{
    vil->ang_moveto = VSUP_dir_toPos(vil, vil->pos_moveto, &vil->dir_moveto);
    if ((vil->abl_move & 0x1000) || (vil->abl_move & 0x400))
    {
        vil->domove |= 4;
    }
    vil->domove |= 0x800;
    vil->domove |= 1;
    vil->domove |= 0xA;
    vil->domove |= 0x80000000;
}

static inline float VSUP_ds2_toPos(_zEnt* ent, _xVec3 pos)
{
    _xVec3 delta = {};

    xVec3Sub(&delta, &pos, &((_xMat4x3*)ent->model->Mat)->pos);
    return delta.x * delta.x + delta.z * delta.z;
}

static inline float VSUP_dir_toPos(_zEnt* ent, _xVec3 pos, _xVec3* dir)
{
    _xVec3 delta = {};
    float ang;

    xVec3Sub(&delta, &pos, &((_xMat4x3*)ent->model->Mat)->pos);
    xVec3Normalize(&delta, &delta);
    ang = VSUP_yaw(delta);
    if (dir)
    {
        xVec3Normalize(dir, &delta);
    }
    return ang;
}

static inline _xVec3 VUTIL_posOf(_xEnt* ent)
{
    _xVec3 pos = {};

    pos.x = ((_xMat4x3*)ent->model->Mat)->pos.x;
    pos.z = ((_xMat4x3*)ent->model->Mat)->pos.z;
    return pos;
}

static inline _xVec3 VUTIL_ctrOf(_xEnt* ent)
{
    _xVec3 ctr = {};

    ctr = *xBoundCenter(&ent->bound);
    return ctr;
}

static inline void VSUP_ang_toXZDir(float angle, _xVec3* dir)
{
    dir->x = sin(angle);
    dir->z = cos(angle);
}

static inline signed int VSUP_ent_inFOV(st_zEntVillain* vil, _zEnt* ent)
{
    signed int result = 0;
    // Unreferenced in the original as well (only the template remains).
    float unused_pad[29] = {};
    float ang = VSUP_dir_toPos(vil, VUTIL_ctrOf(ent), NULL);

    if (vil->fov_detect <= 0.0f)
    {
        result = 0;
    }
    else if ((float)__fabs(vil->frame->rot.angle - ang) < 0.5f * vil->fov_detect)
    {
        result = 1;
    }
    return result;
}

static inline float VUTIL_sqrt(float x)
{
    return xsqrt(x);
}

static inline _xVec3* VUTIL_mdlpos(st_zEntVillain* vil)
{
    return &((_xMat4x3*)vil->model->Mat)->pos;
}

static inline signed int VSUP_ent_inLOS(st_zEntVillain* vil, _zEnt* ent, signed int prev, float rand)
{
    return VSUP_ent_inLOS_timz(vil, ent, prev, rand);
}

static inline signed int VSUP_ent_inLOS_timz(st_zEntVillain* vil, _zEnt* ent, signed int prev, float rand)
{
    _xRay3 r = {};
    _xVec3 mypos = {};
    float dst_max;

    if (!xUtil_yesno(rand))
    {
        return prev;
    }

    mypos = *xBoundCenter(&vil->bound);
    r.min_t = 0.001f + vil->bound.sph.r;
    xVec3Sub(&r.dir, xBoundCenter(&ent->bound), &mypos);
    r.max_t = xVec3Normalize(&r.dir, &r.dir);
    dst_max = VUTIL_sqrt(vil->ds2_detect);
    if (dst_max < r.max_t)
    {
        r.max_t = VUTIL_sqrt(vil->ds2_detect);
    }
    r.origin.x = r.dir.x * r.min_t + mypos.x;
    r.origin.y = r.dir.y * r.min_t + mypos.y;
    r.origin.z = r.dir.z * r.min_t + mypos.z;
    r.flags = 0x800;
    return xRayHitsSceneFast(&r, globals.sceneCur) == 0;
}

static inline float VSUP_ent_facedot(st_zEntVillain* vil, _zEnt* ent)
{
    _xVec3 dir_VtoE = {};
    _xVec3 dir_face = {};
    float yaw;

    VSUP_dir_toPos(vil, VUTIL_ctrOf(ent), &dir_VtoE);
    yaw = vil->frame->rot.angle;
    dir_face.x = sin(yaw);
    dir_face.y = 0.0f;
    dir_face.z = cos(yaw);
    return xVec3Dot(&dir_VtoE, &dir_face);
}

// Angle of interest: where ent is relative to the villain's facing, and in
// which quadrant (1 front, 4 right, 2 back, 3 left).
static inline float VSUP_ent_AOI(st_zEntVillain* vil, _zEnt* ent, signed int* qdrant)
{
    float dang = VSUP_dir_toPos(vil, VUTIL_ctrOf(ent), NULL) - vil->frame->rot.angle;
    float ang = xAngleClamp(dang + 0.7853982f);

    *qdrant = 0;
    if (ang > 0.0f && ang <= 1.5707964f)
    {
        *qdrant = 1;
    }
    else if (ang > 1.5707964f && ang <= 3.1415927f)
    {
        *qdrant = 4;
    }
    else if (ang > 3.1415927f && ang < 4.712389f)
    {
        *qdrant = 2;
    }
    else if (ang > 0.0f && ang < 1.5707964f)
    {
        *qdrant = 3;
    }
    return xAngleClamp(dang);
}

static inline signed int VSUP_choose(signed int* list, float* wts, signed int cnt)
{
    float dlta;
    float pt;
    float lo;
    float hi;
    signed int i;
    signed int idx = 0;

    dlta = 1.0f / cnt;
    pt = iurand();
    hi = 0.0f;
    for (i = 0; i < cnt; i++)
    {
        lo = hi;
        if (wts)
        {
            hi += wts[i];
        }
        else
        {
            hi += dlta;
        }
        if (pt >= lo && pt <= hi)
        {
            idx = i;
            break;
        }
    }
    return list[idx];
}

static inline void VSUP_wtadjust(float* wts, signed int cnt, float arbref)
{
    float fac;
    float sum = 0.0f;
    signed int i;

    for (i = 0; i < cnt; i++)
    {
        if (wts[i] < 0.0f)
        {
            wts[i] = -wts[i];
        }
        sum += wts[i];
    }

    fac = arbref / sum;
    for (i = 0; i < cnt; i++)
    {
        wts[i] *= fac;
    }
}

static inline float VSUP_yaw(_xVec3 dir)
{
    return xAngleClamp(atan2(dir.x, dir.z));
}

static signed int VSUP_accum_collide(st_zEntVillain* vil, _xVec3* vec_accum)
{
    signed int looked;
    _xVec3 dir_trav = {};
    signed int hit_something = 0;
    _xCollis* colrec;
    signed int i;

    looked = 1;
    if (vil->abl_move & 0x10000)
    {
        float ang;

        looked = 0;
        ang = vil->frame->rot.angle;
        dir_trav.x = isin(ang);
        dir_trav.y = 0.0f;
        dir_trav.z = icos(ang);
    }

    colrec = &vil->collis->colls[0];
    if (colrec->flags & 1)
    {
        hit_something = 1;
        xVec3AddTo(vec_accum, &colrec->depen);
    }
    else if (vil->collis->colls[1].flags & 1)
    {
        hit_something = 1;
        xVec3AddTo(vec_accum, &vil->collis->colls[1].depen);
    }

    for (i = vil->collis->npc_sidx; i < vil->collis->npc_eidx; i++)
    {
        colrec = &vil->collis->colls[i];
        if (colrec->optr && (colrec->flags & 1) && !(colrec->flags & 2))
        {
            hit_something++;
            xVec3AddTo(vec_accum, &colrec->depen);
        }
    }

    for (i = vil->collis->env_sidx; i < vil->collis->env_eidx; i++)
    {
        colrec = &vil->collis->colls[i];
        hit_something++;
        if (colrec->optr && (colrec->flags & 1) && !(colrec->flags & 2))
        {
            xVec3AddTo(vec_accum, &colrec->depen);
            if (!looked)
            {
                looked = VOUT_hit_isWall(vil, &dir_trav, colrec);
            }
        }
    }

    for (i = vil->collis->stat_sidx; i < vil->collis->stat_eidx; i++)
    {
        colrec = &vil->collis->colls[i];
        if (colrec->optr && (colrec->flags & 1) && !(colrec->flags & 2))
        {
            hit_something++;
            xVec3AddTo(vec_accum, &colrec->depen);
            if (!looked)
            {
                looked = VOUT_hit_isWall(vil, &dir_trav, colrec);
            }
        }
    }

    return hit_something;
}

// Mirror out about the (unit) normal alter. Not used by the GameCube build;
// only its template remains.
static inline void VSUP_cheap_reflect(_xVec3* out, _xVec3* alter)
{
    _xVec3 vec = {};

    xVec3SMul(&vec, alter, 2.0f * xVec3Dot(out, alter));
    xVec3Sub(out, out, &vec);
}

// Upright gum blob at the villain's position.
static inline void VEFX_place_gumBlob(xModelInstance* blob, _xMat4x3* mat)
{
    _xMat4x3 m = {};

    xMat3x3Euler((_xMat3x3*)&m, (_xVec3*)&g_O3);
    m.right.x *= 1.0f;
    m.right.y *= 1.0f;
    m.right.z *= 1.0f;
    m.up.x *= 1.0f;
    m.up.y *= 1.0f;
    m.up.z *= 1.0f;
    m.at.x *= 1.0f;
    m.at.y *= 1.0f;
    m.at.z *= 1.0f;
    m.pos = mat->pos;
    memcpy(blob->Mat, &m, sizeof(_xMat4x3));
}

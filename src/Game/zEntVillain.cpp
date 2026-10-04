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
#include "iMath.h"
#include "iSnd.h"
#include "xMemMgr.h"

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

static en_ZVILLAIN_HEADBUTT_RESPONSE VOUT_spikey_header(st_zEntVillain* vil);
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

static inline void VSUP_cheap_reflect(_xVec3* out, _xVec3* alter);
static inline signed int VOUT_hit_isWall(st_zEntVillain* vil, _xVec3* dir_trav, _xCollis* colrec);
static signed int VSUP_accum_collide(st_zEntVillain* vil, _xVec3* vec_accum);
static inline void VSUP_wtadjust(float* wts, signed int cnt, float arbref);
static inline signed int VSUP_choose(signed int* list, float* wts, signed int cnt);
static st_NPC_BONUS* VBON_choose_bonusItem(en_zEntVillainSubTypeID vtyp);
static void VBON_drop_bonusItem(st_zEntVillain* vil);
static inline void VBON_setup_bonusTable();
static inline signed int VSND_findProps(en_NPC_SOUND sndEvent, st_NPC_SOUND_PROPERTIES** sprop_caller);
static inline signed int VSND_addQueueEntry(st_zEntVillain* vil, en_NPC_SOUND sndEvent, signed int flg_vsnd, float radius, float delay, float repeatDelay, signed int repeatCount, unsigned int useThisSound);
static void VSND_play(st_zEntVillain* vil, en_NPC_SOUND sndEvent);
static unsigned int VSND_select_sound(st_zEntVillain* vil, en_NPC_SOUND sndEvent);
static inline en_NPC_SOUND VSND_typeFromHash(st_zEntVillain* vil, unsigned int aid_snd);
static unsigned int VSND_goPlay(st_zEntVillain* vil, en_NPC_SOUND sndEvent, unsigned int sndExplicit, signed int flg_vsnd, float dist);
static signed int VSND_procQueue(st_zEntVillain* vil, float dt);
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
static xAnimTable* VANM_mktable_SHARK();
static xAnimTable* VANM_mktable_MM();
static xAnimTable* VANM_mktable_RB();
static xAnimTable* VANM_mktable_GG();
static xAnimTable* VANM_mktable_BK();

static xAnimTable* VANM_mktable_BK()
{
    xAnimTable* table;
    xAnimTransition* tran;
    signed int i;
    char* names[32] = {};

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
    names[1] = "RB_Turn";
    names[0] = "RB_IdleWind";
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

static xAnimTable* VANM_mktable_SHARK()
{
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

static xAnimTable* VANM_mktable_GANG()
{
    xAnimTable* table;
    xAnimTransition* tran;
    signed int i;
    char* names[32] = {};
    xAnimTransition* tr90[147] = {};
    xAnimTransition* tr2DC[147] = {};

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
    xAnimTransition* tr90[147] = {};
    xAnimTransition* tr2DC[147] = {};

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
    names[1] = "Attack_1";
    names[0] = "Attack";
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
    names[22] = "Combo_1";
    names[26] = "Dazed_Recovery";
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
    names[23] = "Combo_2";
    names[24] = "Dazed";
    names[25] = "Dazed_Begin";
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
    names[1] = "Death_From_Dazed";
    names[0] = "Death";
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
    ent->tmr_list[ZVIL_TYMR_MINANIMPLAY] = VANM_duration(single->State, ent, &tym) * ent->anim_loopmult;
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
    ent->tmr_list[ZVIL_TYMR_MINANIMPLAY] = VANM_duration(single->State, ent, &tym) * ent->anim_loopmult;
    if (single->State->Flags & 0x10)
    {
        ent->anim_loopmult = 0.97f;
    }
    ent->anim_playsync = 0;
    return 1;
}

static inline en_NPC_ANIMSTATES VANM_hashToAnim(unsigned int id)
{
    signed int i;
    en_NPC_ANIMSTATES anim = ZVIL_ANIM_UNKNOWN;

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
        if (astate->UserFlags & 0x40000000)
        {
            astate = NULL;
        }
        if (astate)
        {
            if (ent->anim_playnext != best[i])
            {
                ent->anim_playnext = best[i];
                ent->anim_playing = 0;
                ent->anim_playsync = sync;
                ent->anim_loopmult = 0.97f;
                ent->tmr_list[ZVIL_TYMR_MINANIMPLAY] = VANM_duration(astate, ent, NULL) * ent->anim_loopmult;
            }
            else
            {
                astate = NULL;
            }
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
    xAnimTransition* tran = NULL;

    if (from && to && from->ID != to->ID)
    {
        if (!VANM_findtran(ent, ent->model->Anim->Table, from, to, 1, &tran) && tran)
        {
            xAnimPlayStartTransition(ent->model->Anim, tran);
        }
    }
}

static en_NPC_ANIMSTATES VANM_randset(st_zEntVillain* ent, float* wts, en_NPC_ANIMSTATES* choice_list,
                                      signed int cnt)
{
    signed int act_cnt = 0;
    float* act_wts = NULL;
    signed int use_anim;
    signed int* act_list;
    xAnimState* chk_ast;
    signed int i;
    en_NPC_ANIMSTATES fnl_anim;

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
        for (tmp_tranlist = from->List; tmp_tranlist; tmp_tranlist = tmp_tranlist->Next)
        {
            tmp_tran = tmp_tranlist->T;
            if (!tmp_tran)
            {
                continue;
            }
            if (to->ID == tmp_tran->Dest->ID)
            {
                da_tran = tmp_tran;
                break;
            }
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
    xAnimPlay* play;

    if (!ent->model)
    {
        play = NULL;
    }
    else if (!ent->model->Anim)
    {
        play = NULL;
    }
    else
    {
        play = ent->model->Anim;
    }
    if (play)
    {
        return play->Single;
    }
    return NULL;
}

static inline void VSND_setup_soundTable()
{
}

static signed int VSND_procQueue(st_zEntVillain* vil, float dt)
{
    signed int cnt = 0;
    signed int i;
    st_NPC_SOUND_QUEUE* que;

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
        if (iSndIsPlaying(0, (unsigned int)vil + 1))
        {
            float tymref = vil->vilcfg.tymref_list[ZVIL_TTIL_NEXTVOICE];
            vil->tmr_list[ZVIL_TTIL_NEXTVOICE] = 0.25f * (iurand() - 0.5f) * tymref + tymref;
            return 0;
        }
    }

    if (flg_vsnd & 0x800)
    {
        if (iSndIsPlaying(0, (unsigned int)vil + 3))
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

static unsigned int VSND_select_sound(st_zEntVillain* vil, en_NPC_SOUND sndEvent)
{
    float use_wt;
    signed int cnt;
    float wts[32] = {};
    signed int list[32] = {};
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

static void VSND_play(st_zEntVillain* vil, en_NPC_SOUND sndEvent)
{
    st_NPC_SOUND_PROPERTIES* sprop = NULL;
    signed int flg_vsnd;
    float delay;
    float radius;

    VSND_findProps(sndEvent, &sprop);
    flg_vsnd = sprop->flg_vsnd;
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
            que->sndEvent = sndEvent;
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

static inline signed int VSND_findProps(en_NPC_SOUND sndEvent, st_NPC_SOUND_PROPERTIES** sprop_caller)
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
    return sprop->sndEvent != VSND_LISTEND;
}

static inline void VBON_setup_bonusTable()
{
}

static void VBON_drop_bonusItem(st_zEntVillain* vil)
{
    signed int skip_distchk = 0;
    signed int overlap = 0;
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
    signed int wt_ramp;
    _xVec3 farfar = { -9999.0f, -8888.0f, -7777.0f };
    signed int soap_always;
    float food_weight;
    float gum_weight;
    float soap_weight;
    float rat;
    signed int choice;
    signed int cnt;
    float wts[11] = {};
    signed int list[11] = {};
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
    else if (vtyp == eEntVillainType_Undefined && pfx == 'P\0\0\0')
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

static inline signed int VSUP_choose(signed int* list, float* wts, signed int cnt)
{
    float dlta;
    float pt;
    float hi;
    float lo;
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
                                         (signed int (*)(void*, _xVec3*, _zMovePoint*))VOUT_spikey_header);
            vil->need_navpt = 0;
            if (mvpt && mvpt != vil->nav_next)
            {
                vil->nav_next = mvpt;
            }
        }
    }
    return result;
}

static inline void VSUP_cheap_reflect(_xVec3* out, _xVec3* alter)
{
}

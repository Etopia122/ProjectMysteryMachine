#ifndef ZGLOBALS_H
#define ZGLOBALS_H

#include "xCamera.h"
#include "zEnt.h"
#include "iCamera.h"
#include "iModel.h"
#include "xEntDrive.h"

class zScene;

// The GameCube layout is the PS2 one with a 0x30-byte smaller camera frame
// (checked against player.ent.model at 0x224 and sceneCur at 0xB64).

class _tagPadAnalog
{
public:
    signed char x;
    signed char y;
};

class _tagxRumble
{
public:
    signed int type;        // 0x0
    float seconds;          // 0x4
    _tagxRumble* next;      // 0x8
    signed short active;    // 0xC
    unsigned short fxflags; // 0xE
};

class _tagiPad
{
public:
    signed int port;
};

class _tagxPad
{
public:
    unsigned char value[22];
    unsigned char last_value[22];
    unsigned int on;          // 0x2C
    unsigned int pressed;     // 0x30
    unsigned int released;    // 0x34
    _tagPadAnalog analog1;    // 0x38
    _tagPadAnalog analog2;    // 0x3A
    signed int state;         // 0x3C
    unsigned int flags;       // 0x40
    _tagxRumble rumble_head;  // 0x44
    signed short port;        // 0x54
    signed short slot;        // 0x56
    _tagiPad context;         // 0x58
    float al2d_timer;         // 0x5C
    float ar2d_timer;         // 0x60
    float d_timer;            // 0x64
};

class _zEntHangable;
class _zPlatform;

class xBase;

class zShaggy1Globals
{
public:
    _zEnt* GrabTry;              // 0x0
    _zEnt* Grab;                 // 0x4
    float GrabRot;               // 0x8
    float GrabOffset;            // 0xC
    float GrabTimer;             // 0x10
    _xVec3 ReleaseTrans;         // 0x14
    _xVec3 HangTrans;            // 0x20
    _xVec3 LastDelta;            // 0x2C
    unsigned int ThrowEnded;     // 0x38
    float ExtraTransY;           // 0x3C
    _xVec3 ThrowPos;             // 0x40
    _xVec3 ThrowVel;             // 0x4C
    unsigned int CatchMe;        // 0x58
    unsigned int CatchYou;       // 0x5C
    _zEnt* HangTarget;           // 0x60
    float ThrowTimer;            // 0x64
    _xVec3 HangStart;            // 0x68
    _xVec3 HangRelative;         // 0x74
    unsigned int SetDown;        // 0x80
    unsigned int GrabOn;         // 0x84
    unsigned int ThrowState;     // 0x88
    float ThrowRot;              // 0x8C
    float CatchHeight;           // 0x90
    float CatchVel;              // 0x94
    xBase* ThrowTgt;             // 0x98
    float ThrowTgtAngle;         // 0x9C
    float ThrowTgtPeak;          // 0xA0
    float ThrowTgtOffset;        // 0xA4
    unsigned int ThrowTgtAdjust; // 0xA8
    _xVec3 ThrowTgtVec;          // 0xAC
    float ThrowTgtSpeed;         // 0xB8
    float ThrowAccel;            // 0xBC
    _zEnt* Hangable[10];         // 0xC0
};

// Only the fields used so far; offsets from the PS2 DWARF (same on GameCube).
class zJumpParam
{
public:
    float PeakHeight;     // 0x0
    float TimeGravChange; // 0x4
    float TimeHold;       // 0x8
    float ImpulseVel;     // 0xC
};

class zPlayerStatic
{
public:
    unsigned short AnalogMin;     // 0x0
    unsigned short AnalogMax;     // 0x2
    float SpeedSneak[4];          // 0x4
    float SpeedWalk[4];           // 0x14
    float SpeedRun[4];            // 0x24
    float SpeedAir[4];            // 0x34
    float SpeedHotsauce[4];       // 0x44
    float AnimSneak[3];           // 0x54
    float AnimWalk[3];            // 0x60
    float Gravity;                // 0x6C
    float JumpGravity;            // 0x70
    float GravSmooth;             // 0x74
    float FloatSpeed;             // 0x78
    float ButtsmashSpeed;         // 0x7C
    zJumpParam Jump;              // 0x80
    zJumpParam Double;            // 0x90
    zJumpParam Bounce;            // 0xA0
    zJumpParam Spring;            // 0xB0
    zJumpParam Toss;              // 0xC0
    float HeadbuttSpeedMin;       // 0xD0
    float HeadbuttSpeedMax;       // 0xD4
    float HeadbuttAccel;          // 0xD8
    float HeadbuttTimeMin;        // 0xDC
    float HeadbuttTimeMax;        // 0xE0
    float HeadbuttTurn;           // 0xE4
    float FireDelay;              // 0xE8
    float FireRepeat;             // 0xEC
    float FirePress;              // 0xF0
    float FireTargetYaw;          // 0xF4
    float FireTargetPitch;        // 0xF8
    float FireTargetDist;         // 0xFC
    float FireTargetRadius;       // 0x100
    float FireTrack;              // 0x104
    float FireGumSpeed;           // 0x108
    float FireSoapSpeed;          // 0x10C
    float FireGumTimer;           // 0x110
    float FireSoapTimer;          // 0x114
    unsigned int Initial_Specials; // 0x118
    unsigned int TakeDamage;      // 0x11C
    float StartSlideAngle;        // 0x120
    float StopSlideAngle;         // 0x124
    float RotMatchMaxAngle;       // 0x128
    float RotMatchMatchTime;      // 0x12C
    float RotMatchRelaxTime;      // 0x130
    unsigned short AmmoCapacityGum;  // 0x134
    unsigned short AmmoCapacitySoap; // 0x136
    unsigned short AmmoSceneGum;  // 0x138
    unsigned short AmmoSceneSoap; // 0x13A
};

class zCheckPoint
{
public:
    _xVec3 pos;             // 0x0
    float rot;              // 0xC
    unsigned int initCamID; // 0x10
};

class zPlayerGlobals
{
public:
    _zEnt ent;                      // 0x0
    unsigned int Visible;           // 0xEC
    unsigned int Health;            // 0xF0
    signed int Speed;               // 0xF4
    signed int Sneak;               // 0xF8
    signed int Teeter;              // 0xFC
    signed int CanSlip;             // 0x100
    float SlipFadeTimer;            // 0x104
    signed int Slide;               // 0x108
    float SlideTimer;               // 0x10C
    signed int Stepping;            // 0x110
    signed int JumpState;           // 0x114
    signed int LastJumpState;       // 0x118
    float JumpTimer;                // 0x11C
    float LookAroundTimer;          // 0x120
    unsigned int LookAroundRand;    // 0x124
    unsigned int LastProjectile;    // 0x128
    float DecelRun;                 // 0x12C
    float DecelRunSpeed;            // 0x130
    float HotsauceTimer;            // 0x134
    float LeanLerp;                 // 0x138
    unsigned int HeadbuttHit;       // 0x13C
    float ScareTimer;               // 0x140
    xBase* ScareSource;             // 0x144
    unsigned int ScareShaggy;       // 0x148
    float CowerTimer;               // 0x14C
    float DamageTimer;              // 0x150
    float ControlOffTimer;          // 0x154
    float HelmetTimer;              // 0x158
    unsigned int WorldDisguise;     // 0x15C
    unsigned int Bounced;           // 0x160
    float FallDeathTimer;           // 0x164
    float HeadbuttVel;              // 0x168
    float HeadbuttTimer;            // 0x16C
    unsigned int SpecialReceived;   // 0x170
    _xEnt* MountChimney;            // 0x174
    float MountChimOldY;            // 0x178
    float ShockRadius;              // 0x17C
    float ShockRadiusOld;           // 0x180
    float Face_ScareTimer;          // 0x184
    unsigned int Face_ScareRandom;  // 0x188
    unsigned int Face_Event;        // 0x18C
    float Face_EventTimer;          // 0x190
    float Face_PantTimer;           // 0x194
    unsigned int Face_AnimSpecific; // 0x198
    unsigned int IdleRand;          // 0x19C
    float IdleMinorTimer;           // 0x1A0
    float IdleMajorTimer;           // 0x1A4
    float IdleSitTimer;             // 0x1A8
    signed int Transparent;         // 0x1AC
    _zEnt* FireTarget;              // 0x1B0
    _zEnt* DamageSource;            // 0x1B4
    unsigned int ControlOn;         // 0x1B8
    unsigned int ControlOnEvent;    // 0x1BC
    unsigned int AutoMoveSpeed;     // 0x1C0
    float AutoMoveDist;             // 0x1C4
    _xVec3 AutoMoveTarget;          // 0x1C8
    xBase* AutoMoveObject;          // 0x1D4
    _zEnt* Diggable;                // 0x1D8
    float DigTimer;                 // 0x1DC
    zShaggy1Globals Shaggy1;        // 0x1E0
    _zEnt* Shaggy8Call;             // 0x2C8
    float Shaggy8CallTimer;         // 0x2CC
    _zEntHangable* HangFound;       // 0x2D0
    _zEntHangable* HangEnt;         // 0x2D4
    _zEntHangable* HangEntLast;     // 0x2D8
    _xVec3 HangPivot;               // 0x2DC
    _xVec3 HangVel;                 // 0x2E8
    float HangLength;               // 0x2F4
    _xVec3 HangStartPos;            // 0x2F8
    float HangStartLerp;            // 0x304
    iModelTag HangPawTag[4];        // 0x308
    float HangPawOffset;            // 0x388
    float HangElapsed;              // 0x38C
    float Jump_CurrGravity;         // 0x390
    float Jump_HoldTimer;           // 0x394
    float Jump_ChangeTimer;         // 0x398
    signed int Jump_CanDouble;      // 0x39C
    signed int Jump_CanFloat;       // 0x3A0
    signed int Jump_SpringboardStart; // 0x3A4
    _zPlatform* Jump_Springboard;   // 0x3A8
    signed int cheat_mode;          // 0x3AC
    unsigned int Inv_Clue;          // 0x3B0
    unsigned int Inv_Key;           // 0x3B4
    unsigned int Inv_Chances;       // 0x3B8
    unsigned int Inv_ScoobySnacks;  // 0x3BC
    unsigned int Inv_Specials;      // 0x3C0
    unsigned int Inv_Mtokens[1];    // 0x3C4
    signed int Inv_Ammo[2];         // 0x3C8
    signed int Inv_AmmoMax[2];      // 0x3D0
    iModelTag BubbleTag;            // 0x3D8
    _xEntDrive drv;                 // 0x3F8
    unsigned char unk448[0x4A0 - 0x448];
    _xSphere head_sph;              // 0x4A0
    unsigned char unk4B0[0x510 - 0x4B0];
    zPlayerStatic s;                // 0x510
    zCheckPoint cp;                 // 0x64C
};

class zGlobalGameStats
{
public:
    float gameTime;               // 0x0
    unsigned int visited;         // 0x4
    float levelTime;              // 0x8
    float levelTime_sum;          // 0xC
    float areaTime;               // 0x10
    float areaTime_sum;           // 0x14
    float roomTime;               // 0x18
    float roomTime_sum;           // 0x1C
    float timeRunning;            // 0x20
    float timeRunning_sum;        // 0x24
    float timeWalking;            // 0x28
    float timeWalking_sum;        // 0x2C
    unsigned int scares;          // 0x30
    unsigned int scares_sum;      // 0x34
    unsigned int deaths;          // 0x38
    unsigned int deaths_sum;      // 0x3C
    unsigned int singleJumps;     // 0x40
    unsigned int singleJumps_sum; // 0x44
    unsigned int doubleJumps;     // 0x48
    unsigned int doubleJumps_sum; // 0x4C
    unsigned int gumShot;         // 0x50
    unsigned int gumShot_sum;     // 0x54
    unsigned int gumHit;          // 0x58
    unsigned int gumHit_sum;      // 0x5C
    unsigned int gumEmpty;        // 0x60
    unsigned int gumEmpty_sum;    // 0x64
    unsigned int soapShot;        // 0x68
    unsigned int soapShot_sum;    // 0x6C
    unsigned int soapHit;         // 0x70
    unsigned int soapHit_sum;     // 0x74
    unsigned int soapEmpty;       // 0x78
    unsigned int soapEmpty_sum;   // 0x7C
    unsigned int food;            // 0x80
    unsigned int food_sum;        // 0x84
    unsigned int snackBoxes;      // 0x88
    unsigned int snackBoxes_sum;  // 0x8C
    unsigned int enemyKills;      // 0x90
    unsigned int enemyKills_sum;  // 0x94
    unsigned int enemyCount;      // 0x98
    unsigned int enemyCount_sum;  // 0x9C
    unsigned int specials;        // 0xA0
    unsigned int specials_tot;    // 0xA4
    unsigned int tokens;     // 0xA8
    unsigned int tokens_tot; // 0xAC
    unsigned int snacks;     // 0xB0
    unsigned int snacks_tot; // 0xB4
    unsigned int chances;    // 0xB8
    unsigned int chances_tot; // 0xBC
    unsigned int keys;       // 0xC0
    unsigned int keys_tot;   // 0xC4
    unsigned int clues;      // 0xC8
    unsigned int clues_tot;  // 0xCC
};

class zGlobals
{
public:
    union
    {
        _tagxCamFrame camera; // 0x0
        unsigned char pad[0x1F0];
    };
    _tagxPad* pad0;          // 0x1F0
    _tagxPad* pad1;          // 0x1F4
    zPlayerGlobals player;   // 0x1F8
    void* pickupTable;       // 0x858
    signed int profile;      // 0x85C
    void* cmgr;              // 0x860
    char profFunc[6][128];   // 0x864
    zScene* sceneCur;        // 0xB64
    zScene* scenePreload;    // 0xB68
    signed int sceneFirst;   // 0xB6C
    char sceneStart[32];     // 0xB70
    iFogParams fog;          // 0xB90
    iFogParams fogA;         // 0xBAC
    iFogParams fogB;         // 0xBC8
    long long fog_t0;        // 0xBE8
    long long fog_t1;        // 0xBF0
    zGlobalGameStats stats;  // 0xBF8
    zGlobalGameStats areaStats[30];  // 0xCC8
    zGlobalGameStats levelStats[13]; // 0x2528
    zGlobalGameStats worldStats[4];  // 0x2FB8
    signed int option_vibration; // 0x32F8
    float update_dt;         // 0x32FC
};

extern zGlobals globals;

#endif

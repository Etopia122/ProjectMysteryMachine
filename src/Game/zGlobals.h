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

class zShaggy1Globals
{
public:
    unsigned char unk0[0x14];
    _xVec3 ReleaseTrans; // 0x14
    _xVec3 HangTrans;    // 0x20
    unsigned char unk2C[0xE8 - 0x2C];
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

class zPlayerGlobals
{
public:
    _zEnt ent;                      // 0x0
    unsigned int Visible;           // 0xEC
    unsigned int Health;            // 0xF0
    unsigned char unkF4[0x114 - 0xF4];
    signed int JumpState;           // 0x114
    unsigned char unk118[0x128 - 0x118];
    unsigned int LastProjectile;    // 0x128
    unsigned char unk12C[0x1B0 - 0x12C];
    _zEnt* FireTarget;              // 0x1B0
    unsigned char unk1B4[0x1E0 - 0x1B4];
    zShaggy1Globals Shaggy1;        // 0x1E0
    unsigned char unk2C8[0x2D4 - 0x2C8];
    _zEntHangable* HangEnt;         // 0x2D4
    _zEntHangable* HangEntLast;     // 0x2D8
    _xVec3 HangPivot;               // 0x2DC
    unsigned char unk2E8[0x308 - 0x2E8];
    iModelTag HangPawTag[4];        // 0x308
    unsigned char unk388[0x3C8 - 0x388];
    signed int Inv_Ammo[2];         // 0x3C8
    signed int Inv_AmmoMax[2];      // 0x3D0
    iModelTag BubbleTag;            // 0x3D8
    _xEntDrive drv;                 // 0x3F8
    unsigned char unk448[0x510 - 0x448];
    zPlayerStatic s;                // 0x510
    unsigned char unk64C[0x660 - 0x64C];
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
    unsigned char unkBF8[0x32F8 - 0xBF8]; // stats, areaStats, levelStats, worldStats
    signed int option_vibration; // 0x32F8
    float update_dt;         // 0x32FC
};

extern zGlobals globals;

#endif

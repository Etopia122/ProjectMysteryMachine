#ifndef ZGLOBALS_H
#define ZGLOBALS_H

#include "xCamera.h"
#include "zEnt.h"

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

class zPlayerGlobals
{
public:
    _zEnt ent;            // 0x0
    unsigned int Visible; // 0xEC
    unsigned int Health;  // 0xF0
    unsigned char unkF4[0x660 - 0xF4];
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
};

extern zGlobals globals;

#endif

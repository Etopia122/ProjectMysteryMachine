#ifndef ZGLOBALS_H
#define ZGLOBALS_H

#include "xCamera.h"
#include "zEnt.h"

class zScene;

// The GameCube layout is the PS2 one with a 0x30-byte smaller camera frame
// (checked against player.ent.model at 0x224 and sceneCur at 0xB64).

class _tagxPad
{
public:
    unsigned char value[22];
    unsigned char last_value[22];
    unsigned int on;       // 0x2C
    unsigned int pressed;  // 0x30
    unsigned int released; // 0x34
    // rest unknown
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

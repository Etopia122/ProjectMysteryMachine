#ifndef XENTMOTION_H
#define XENTMOTION_H

#include "xEnt.h"

class p2EntMotionPenData
{
public:
    unsigned char flags;
    unsigned char plane;
    unsigned char pad[2];
    float len;
    float range;
    float period;
    float phase;
};

class p2EntMotionAsset
{
public:
    unsigned char type;
    unsigned char pad;
    unsigned short flags;
    union
    {
        p2EntMotionPenData pen;
        unsigned char data[0x2C]; // er/orb/spl/mp/mech/pen
    };
};

// GameCube layout (the PS2 build aligns the union to 16 bytes)
class _xEntMotion
{
public:
    p2EntMotionAsset* asset; // 0x0
    unsigned char type;      // 0x4
    unsigned char pad;       // 0x5
    unsigned short flags;    // 0x6
    float t;                 // 0x8
    float tmr;               // 0xC
    float d;                 // 0x10
    unsigned char data[0x50]; // 0x14: er/orb/spl/mp/mech/pen
    _xEnt* owner;            // 0x64
    _xEnt* target;           // 0x68
};

void xEntMotionInit(_xEntMotion* motion, _xEnt* owner, p2EntMotionAsset* asset);
void xEntMotionSetup(_xEntMotion* motion, _xScene* sc);
void xEntMotionReset(_xEntMotion* motion, _xScene* sc);
void xEntMotionMove(_xEntMotion* motion, _xScene* sc, float dt, _xEntFrame* frame);
void xEntMotionTranslate(_xEntMotion* motion, _xVec3* dpos);

inline void xEntMotionRun(_xEntMotion* motion)
{
    motion->flags &= 0xFFFB;
}

inline void xEntMotionStop(_xEntMotion* motion)
{
    motion->flags |= 4;
}

#endif

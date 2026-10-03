#ifndef XENTMOTION_H
#define XENTMOTION_H

#include "xEnt.h"

class _xMovePoint;
class xSpline3;

class p2EntMotionERData
{
public:
    _xVec3 ret_pos;    // 0x0
    _xVec3 ext_dpos;   // 0xC
    float ext_tm;      // 0x18
    float ext_wait_tm; // 0x1C
    float ret_tm;      // 0x20
    float ret_wait_tm; // 0x24
};

class p2EntMotionOrbitData
{
public:
    _xVec3 center; // 0x0
    float w;       // 0xC
    float h;       // 0x10
    float period;  // 0x14
};

class p2EntMotionMPData
{
public:
    unsigned int flags; // 0x0
    unsigned int mp_id; // 0x4
    float speed;        // 0x8
};

class p2EntMotionMechData
{
public:
    unsigned char type;     // 0x0
    unsigned char flags;    // 0x1
    unsigned char sld_axis; // 0x2
    unsigned char rot_axis; // 0x3
    float sld_dist;         // 0x4
    float sld_tm;           // 0x8
    float sld_acc_tm;       // 0xC
    float sld_dec_tm;       // 0x10
    float rot_dist;         // 0x14
    float rot_tm;           // 0x18
    float rot_acc_tm;       // 0x1C
    float rot_dec_tm;       // 0x20
    float ret_delay;        // 0x24
    float post_ret_delay;   // 0x28
};

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
    unsigned char type;   // 0x0
    unsigned char pad;    // 0x1
    unsigned short flags; // 0x2
    union
    {
        p2EntMotionERData er;
        p2EntMotionOrbitData orb;
        p2EntMotionMPData mp;
        p2EntMotionMechData mech;
        p2EntMotionPenData pen;
    }; // 0x4
};

class _xEntERData
{
public:
    _xVec3 a;    // 0x0
    _xVec3 b;    // 0xC
    _xVec3 dir;  // 0x18
    float et;    // 0x24
    float wet;   // 0x28
    float rt;    // 0x2C
    float wrt;   // 0x30
    float p;     // 0x34
    float brt;   // 0x38
    float ert;   // 0x3C
    signed int state; // 0x40
};

class _xEntOrbitData
{
public:
    _xVec3 orig; // 0x0
    _xVec3 c;    // 0xC
    float a;     // 0x18
    float b;     // 0x1C
    float p;     // 0x20
    float w;     // 0x24
};

class _xEntMPData
{
public:
    float curdist;       // 0x0
    float speed;         // 0x4
    _xMovePoint* dest;   // 0x8
    _xMovePoint* src;    // 0xC
    xSpline3* spl;       // 0x10
    float dist;          // 0x14
    unsigned int padalign; // 0x18
    _xQuat aquat;        // 0x1C
    _xQuat bquat;        // 0x2C
};

class _xEntMechData
{
public:
    _xVec3 apos;  // 0x0
    _xVec3 bpos;  // 0xC
    _xVec3 dir;   // 0x18
    float arot;   // 0x24
    float brot;   // 0x28
    float ss;     // 0x2C
    float sr;     // 0x30
    signed int state; // 0x34
    float tsfd;   // 0x38
    float trfd;   // 0x3C
    float tsbd;   // 0x40
    float trbd;   // 0x44
    float* rotptr; // 0x48
};

class _xEntPenData
{
public:
    _xVec3 top;    // 0x0
    float w;       // 0xC
    _xMat4x3 omat; // 0x10
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
    union
    {
        _xEntERData er;
        _xEntOrbitData orb;
        _xEntMPData mp;
        _xEntMechData mech;
        _xEntPenData pen;
    }; // 0x14
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

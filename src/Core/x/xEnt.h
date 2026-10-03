#ifndef XENT_H
#define XENT_H

#include "xBase.h"
#include "xMath3.h"
#include "xBound.h"

class RwMatrixTag;

// Placeholder: only Mat's offset is known on GameCube.
class xModelInstance
{
public:
    unsigned char pad0[0x20];
    RwMatrixTag* Mat; // 0x20
};
class _xScene;
class _xEntCollis;
class xSurface;
class xFFX;
class _xEntShadow;

class p2EntAsset : public p2BaseAsset
{
public:
    unsigned char flags;
    unsigned char subtype;
    unsigned char pflags;
    unsigned char moreFlags;
    unsigned int surfaceID;
    _xVec3 ang;
    _xVec3 pos;
    _xVec3 scale;
    float redMult;
    float greenMult;
    float blueMult;
    float seeThru;
    float seeThruSpeed;
    unsigned int modelInfoID;
    unsigned int animListID;
};

class _xEntFrame
{
public:
    _xMat4x3 mat;   // 0x0
    _xMat4x3 dmat;  // 0x40
    _xVec3 oldpos;  // 0x80
    _xVec3 oldvel;  // 0x8C
    _xRot oldrot;   // 0x98
    _xRot drot;     // 0xA8
    _xRot rot;      // 0xB8
    _xVec3 dpos;    // 0xC8
    _xVec3 dvel;    // 0xD4
    _xVec3 vel;     // 0xE0
    unsigned int mode; // 0xEC
};

class iShadowCache
{
public:
    _xVec3 pos;
    _xVec3 right;
    _xVec3 corner[4];
};

// Layout from the PS2 DWARF; matches GameCube offsets checked so far.
class _xEnt : public xBase
{
public:
    p2EntAsset* asset;        // 0x10
    unsigned char flags;      // 0x14
    unsigned char subType;    // 0x15
    unsigned char pflags;     // 0x16
    unsigned char moreFlags;  // 0x17
    unsigned char pad0[3];    // 0x18
    unsigned char num_ffx;    // 0x1B
    unsigned short idx;       // 0x1C
    unsigned short num_updates; // 0x1E
    float material_alpha;     // 0x20
    float fade_tmr;           // 0x24
    unsigned char collType;   // 0x28
    unsigned char collLev;    // 0x29
    unsigned char chkby;      // 0x2A
    unsigned char penby;      // 0x2B
    xModelInstance* model;    // 0x2C
    void (*update)(_xEnt*, _xScene*, float);              // 0x30
    void (*bupdate)(_xEnt*, _xVec3*);                     // 0x34
    void (*move)(_xEnt*, _xScene*, float, _xEntFrame*);   // 0x38
    void (*render)(_xEnt*);                               // 0x3C
    _xEntFrame* frame;        // 0x40
    _xEntCollis* collis;      // 0x44
    _xBound bound;            // 0x48
    float updateTimer;        // 0x80
    float updateAcc;          // 0x84
    void (*updateTimerFunc)(_xEnt*); // 0x88
    void (*transl)(_xEnt*, _xVec3*); // 0x8C
    iShadowCache ishadow;     // 0x90
    xSurface* surf;           // 0xD8
    xFFX* ffx;                // 0xDC
    _xEnt* driver;            // 0xE0
    _xEntShadow* ent_shadow;  // 0xE4
};

void xEntUpdate(_xEnt* ent, _xScene* sc, float dt);
unsigned int xEntIsVisible(_xEnt* ent);
void xEntReset(_xEnt* ent);
void xEntSetMaterialMul(_xEnt* ent, float r, float g, float b);


#endif

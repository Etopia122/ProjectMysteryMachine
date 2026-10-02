#ifndef ZSURFACE_H
#define ZSURFACE_H

#include "xBase.h"
#include "xMath3.h"
#include "xSurface.h"

class zSurfMatFX
{
public:
    unsigned int flags;
    unsigned int bumpmapID;
    unsigned int envmapID;
    float shininess;
    float bumpiness;
    unsigned int dualmapID;
};

class zSurfColorFX
{
public:
    unsigned short flags;
    unsigned short mode;
    float speed;
};

class zSurfTextureAnim
{
public:
    unsigned short pad;
    unsigned short mode;
    unsigned int group;
    float speed;
};

class zSurfUVFX
{
public:
    signed int mode;
    float rot;
    float rot_spd;
    _xVec3 trans;
    _xVec3 trans_spd;
    _xVec3 scale;
    _xVec3 scale_spd;
};

class zSurfAsset : public p2BaseAsset
{
public:
    unsigned char game_damage_type;  // 0x8
    unsigned char game_sticky;       // 0x9
    unsigned char game_damage_flags; // 0xA
    unsigned char surf_type;         // 0xB
    unsigned char phys_pad;          // 0xC
    unsigned char sld_start;         // 0xD
    unsigned char sld_stop;          // 0xE
    unsigned char phys_flags;        // 0xF
    float friction;                  // 0x10
    zSurfMatFX matfx;                // 0x14
    zSurfColorFX colorfx;            // 0x2C
    unsigned int texture_anim_flags; // 0x34
    zSurfTextureAnim texture_anim[2]; // 0x38
    unsigned int uvfx_flags;         // 0x50
    zSurfUVFX uvfx[2];               // 0x54
    unsigned char on;                // 0xCC
    unsigned char surf_pad[3];
};

class zSurfacePropTexAnim
{
public:
    unsigned short mode;
    float speed;
    float frame;
    unsigned int group;
    signed int group_idx;
    xBase* group_ptr;
};

class zSurfacePropUVFX
{
public:
    signed int mode;
    float rot;
    float rot_spd;
    _xVec3 trans;
    _xVec3 trans_spd;
    _xVec3 scale;
    _xVec3 scale_spd;
};

class zSurfaceProps
{
public:
    zSurfAsset* asset;                 // 0x0
    unsigned int texanim_flags;        // 0x4
    zSurfacePropTexAnim texanim[2];    // 0x8
    unsigned int uvfx_flags;           // 0x38
    zSurfacePropUVFX uvfx[2];          // 0x3C
};

void zSurfaceRegisterMapper(unsigned int assetId);
xSurface* zSurfaceGetSurface(unsigned int mat_id);

#endif

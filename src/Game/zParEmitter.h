#ifndef ZPAREMITTER_H
#define ZPAREMITTER_H

#include "xBase.h"
#include "xBound.h"
#include "xMath3.h"

class xPar;
class xParGroup;

class _tagEmitCircle
{
public:
    float radius;     // 0x0
    float deflection; // 0x4
};

class _tagEmitSphere
{
public:
    float radius; // 0x0
};

class _tagEmitRect
{
public:
    float x_len; // 0x0
    float z_len; // 0x4
};

class _tagEmitLine
{
public:
    _xVec3 pos1;  // 0x0
    _xVec3 pos2;  // 0xC
    float radius; // 0x18
};

class _tagEmitVolume
{
public:
    unsigned int emit_volumeID; // 0x0
};

class _tagEmitOffsetPoint
{
public:
    _xVec3 offset; // 0x0
};

class zParEmitterAsset : public p2BaseAsset
{
public:
    unsigned char emit_flags;      // 0x8
    unsigned char emit_type;       // 0x9
    unsigned char count;           // 0xA
    unsigned char count_variation; // 0xB
    float interval;                // 0xC
    union
    {
        _tagEmitCircle e_circle;
        _tagEmitSphere e_sphere;
        _tagEmitRect e_rect;
        _tagEmitLine e_line;
        _tagEmitVolume e_volume;
        _tagEmitOffsetPoint e_offsetp;
    };                             // 0x10
    unsigned int attachToID;       // 0x2C
    unsigned int parSysID;         // 0x30
    _xVec3 pos;                    // 0x34
    _xVec3 vel;                    // 0x40
    float vel_angle_variation;     // 0x4C
    unsigned char color_birth[4];  // 0x50
    unsigned char color_death[4];  // 0x54
    float size_birth;              // 0x58
    float size_birth_variation;    // 0x5C
    float size_death;              // 0x60
    float life;                    // 0x64
    float life_variation;          // 0x68
    unsigned char pad_emit[2];     // 0x6C
    unsigned char cull_mode;       // 0x6E
    float cull_dist_sqr;           // 0x70
    unsigned char max_emit;        // 0x74
};

class iModelTag
{
public:
    _xVec3 v;            // 0x0
    unsigned int matidx; // 0xC
    float wt[4];         // 0x10
};

class _zParEmitter : public xBase
{
public:
    zParEmitterAsset* tasset;    // 0x10
    xParGroup* group;            // 0x14
    unsigned char emit_flags;    // 0x18
    unsigned char emit_pad[3];   // 0x19
    float emit_interval_current; // 0x1C
    unsigned char rot[3];        // 0x20
    iModelTag tag;               // 0x24
    void* attachTo;              // 0x44
    void* parSys;                // 0x48
    void* emit_volume;           // 0x4C
};

class _zParEmitterCustomSettings
{
public:
    unsigned int custom_flags;      // 0x0
    unsigned char count;            // 0x4
    unsigned char count_variation;  // 0x5
    float interval;                 // 0x8
    unsigned int attachToID;        // 0xC
    _xVec3 pos;                     // 0x10
    _xVec3 vel;                     // 0x1C
    float vel_angle_variation;      // 0x28
    unsigned char color_birth[4];   // 0x2C
    unsigned char color_death[4];   // 0x30
    float size_birth;               // 0x34
    float size_birth_variation;     // 0x38
    float size_death;               // 0x3C
    float life;                     // 0x40
    float life_variation;           // 0x44
    unsigned char rot[3];           // 0x48
    float xzcircle_radius;          // 0x4C
    float emit_interval_current;    // 0x50
};

_zParEmitter* zParEmitterFind(char* asset_name);
_zParEmitter* zParEmitterFind(unsigned int asset_id);
xPar* zParEmitterEmitCustom(_zParEmitter* p, float dt, _zParEmitterCustomSettings* info);

#endif

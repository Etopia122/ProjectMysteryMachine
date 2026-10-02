#ifndef ZPAREMITTER_H
#define ZPAREMITTER_H

#include "xBase.h"
#include "xMath3.h"

class _zParEmitter : public xBase
{
public:
    void* tasset;             // 0x10
    void* group;              // 0x14
    unsigned char emit_flags; // 0x18
};

class _zParEmitterCustomSettings
{
public:
    unsigned int custom_flags;
    unsigned char count;
    unsigned char count_variation;
    float interval;
    unsigned int attachToID;
    _xVec3 pos;               // 0x10
    _xVec3 vel;               // 0x1C
    float vel_angle_variation;
    unsigned char color_birth[4];
    unsigned char color_death[4];
    float size_birth;
    float size_birth_variation;
    float size_death;
    float life;               // 0x40
    float life_variation;
    unsigned char rot[3];
    float xzcircle_radius;
    float emit_interval_current;
};

_zParEmitter* zParEmitterFind(char* asset_name);
_zParEmitter* zParEmitterFind(unsigned int asset_id);
void zParEmitterEmitCustom(_zParEmitter* p, float dt, _zParEmitterCustomSettings* info);

#endif

#ifndef XFFX_H
#define XFFX_H

#include "xEnt.h"

class xFFX
{
public:
    unsigned int flags;
    void (*doEffect)(_xEnt*, _xScene*, float, void*);
    void* fdata;
    xFFX* next;
};

class xFFXShakeState
{
public:
    _xVec3 disp;
    float dur;
    float freq;
    float tmr;
    float alpha;
    float lval;
    xFFXShakeState* next;
};

class xFFXRotMatchState
{
public:
    unsigned char data[0x40];
    xFFXRotMatchState* next;
};

void xFFXPoolInit(unsigned int num_ffx);
xFFX* xFFXAlloc();
void xFFXTurnOn(xFFX* f);
void xFFXTurnOff(xFFX* f);
signed short xFFXAddEffect(_xEnt* ent, xFFX* f);
signed short xFFXAddEffect(_xEnt* ent, void (*dof)(_xEnt*, _xScene*, float, void*), void* fd);
void xFFXApply(_xEnt* ent, _xScene* sc, float dt);
void xFFXShakeUpdateEnt(_xEnt* ent, _xScene* sc, float dt, void* fdata);
void xFFXShakePoolInit(unsigned int num);
xFFXShakeState* xFFXShakeAlloc();
void xFFXShakeFree(xFFXShakeState* s);
void xFFXRotMatchPoolInit(unsigned int num);
xFFXRotMatchState* xFFXRotMatchAlloc();

#endif

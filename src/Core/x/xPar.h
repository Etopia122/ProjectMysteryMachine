#ifndef XPAR_H
#define XPAR_H

#include "xMath3.h"

class zParEmitterAsset;

class xPar
{
public:
    xPar* m_next;
    xPar* m_prev;
    float m_lifetime;
    unsigned char m_c[4];
    _xVec3 m_pos;
    float m_size;
    _xVec3 m_vel;
    float m_sizeVel;
    unsigned char m_flag;
    unsigned char m_mode;
    unsigned char m_texIdx[2];
    unsigned char m_rotdeg[3];
    unsigned char pad8;
    unsigned int pad32;
    zParEmitterAsset* m_asset;
    float m_cvel[4];
};

extern xPar gParPool[2000];
extern xPar* gParDead;

void xParMemInit();
xPar* xParAlloc();
void xParFree(xPar* par);
void xParInit(xPar* p);

#endif

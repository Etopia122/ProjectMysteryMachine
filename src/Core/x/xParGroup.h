#ifndef XPARGROUP_H
#define XPARGROUP_H

#include "xPar.h"

class xParCmdTex;

class xParGroup
{
public:
    xPar* m_root;
    xPar* m_dead;
    signed int m_num_of_particles;
    unsigned char m_alive;
    unsigned char m_killWhenDead;
    unsigned char m_active;
    unsigned char m_visible;
    unsigned char m_culled;
    unsigned char m_priority;
    unsigned char m_flags;
    unsigned char m_regidx;
    xParGroup* m_next;
    xParGroup* m_prev;
    void (*draw)(void*, xParGroup*);
    xParCmdTex* m_cmdTex;
};

void xParGroupInit(xParGroup* ps);
void xParGroupSetAging(xParGroup* ps, signed int age);
void xParGroupSetBack2Life(xParGroup* ps, signed int b2l);
void xParGroupSetStealing(xParGroup* ps, signed int stealing);
void xParGroupSetVisibility(xParGroup* ps, signed int vis);
void xParGroupSetPriority(xParGroup* ps, unsigned char val);
void xParGroupRegister(xParGroup* ps);
void xParGroupUnregister(xParGroup* ps);
void xParGroupSetActive(xParGroup* ps, unsigned int isActive);
void xParGroupKillAllParticles(xParGroup* ps);
void xParGroupAnimate(xParGroup* ps, float dt);
xPar* xParGroupAddPar(xParGroup* ps);
void xParGroupKillPar(xParGroup* ps, xPar* p);

#endif

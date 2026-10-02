#include "xPar.h"

xPar gParPool[2000];
xPar* gParDead;

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xParMemInit()
{
    signed int i;
    xPar* p;

    for (i = 0; i < 2000; i++)
    {
        p = &gParPool[i];
        p->m_next = 0;
        p->m_prev = 0;
        if (gParDead)
        {
            gParDead->m_prev = p;
            p->m_next = gParDead;
        }
        gParDead = p;
    }
}

xPar* xParAlloc()
{
    xPar* p = gParDead;
    if (p == 0)
    {
        return 0;
    }
    if (p->m_next)
    {
        p->m_next->m_prev = 0;
    }
    gParDead = p->m_next;
    p->m_next = 0;
    p->m_prev = 0;
    return p;
}

void xParFree(xPar* par)
{
    if (par->m_next)
    {
        par->m_next->m_prev = par->m_prev;
    }
    if (par->m_prev)
    {
        par->m_prev->m_next = par->m_next;
    }
    if (gParDead)
    {
        gParDead->m_prev = par;
    }
    par->m_next = gParDead;
    par->m_prev = 0;
    gParDead = par;
}

void xParInit(xPar* p)
{
    signed int i;

    p->m_pos.x = 0.0f;
    p->m_pos.y = 0.0f;
    p->m_pos.z = 0.0f;
    p->m_vel.x = 0.0f;
    p->m_vel.y = 0.0f;
    p->m_vel.z = 0.0f;
    p->m_size = 0.0f;
    p->m_sizeVel = 0.0f;
    p->m_lifetime = 0.0f;
    p->m_asset = 0;
    p->m_cvel[0] = 0.0f;
    p->m_cvel[1] = 0.0f;
    p->m_cvel[2] = 0.0f;
    p->m_cvel[3] = 0.0f;
    p->m_c[0] = 255;
    p->m_c[1] = 255;
    p->m_c[2] = 255;
    p->m_c[3] = 255;
    p->m_flag = 0;
    for (i = 0; i < 3; i++)
    {
        p->m_rotdeg[i] = 0;
    }
    p->m_next = 0;
    p->m_prev = 0;
    p->m_texIdx[0] = 0;
    p->m_texIdx[1] = 0;
}

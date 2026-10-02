#include "xParGroup.h"

class zParEmitterAsset
{
public:
    unsigned char pad0[0x64];
    float life; // 0x64
};

static signed int sParGroupRegTableInit;
static signed int sParGroupRegTableCount;
xParGroup* sParGroupRegTable[255];

static inline void xParGroupRegisterInit()
{
    for (signed int i = 0; i < 255; i++)
    {
        sParGroupRegTable[i] = 0;
    }
    sParGroupRegTableInit = 1;
    sParGroupRegTableCount = 0;
}

static inline void xParGroupAddParP(xParGroup* ps, xPar* p)
{
    ps->m_alive = 1;
    ps->m_num_of_particles++;
    p->m_prev = 0;
    p->m_next = 0;
    if (ps->m_root)
    {
        p->m_next = ps->m_root;
        ps->m_root->m_prev = p;
        ps->m_root = p;
    }
    else
    {
        ps->m_root = p;
    }
}

static inline void xParGroupAddParToDeadList(xParGroup* ps, xPar* p)
{
    if (ps->m_dead)
    {
        ps->m_dead->m_prev = p;
    }
    p->m_next = ps->m_dead;
    p->m_prev = 0;
    ps->m_dead = p;
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xParGroupInit(xParGroup* ps)
{
    ps->m_root = 0;
    ps->m_dead = 0;
    ps->m_alive = 0;
    ps->m_num_of_particles = 0;
    ps->m_killWhenDead = 0;
    ps->m_flags = 0;
    ps->m_active = 1;
    ps->m_culled = 0;
    ps->m_visible = 1;
    ps->m_cmdTex = 0;
    ps->m_priority = 0;
    ps->m_flags |= 2;
    ps->m_flags |= 1;
}

void xParGroupSetAging(xParGroup* ps, signed int age)
{
    if (age)
    {
        if (ps->m_flags & 4)
        {
            ps->m_flags ^= 4;
        }
    }
    else
    {
        ps->m_flags |= 4;
    }
}

void xParGroupSetBack2Life(xParGroup* ps, signed int b2l)
{
    if (b2l)
    {
        if (ps->m_flags & 8)
        {
            ps->m_flags ^= 8;
        }
    }
    else
    {
        ps->m_flags |= 8;
    }
}

void xParGroupSetStealing(xParGroup* ps, signed int stealing)
{
    if (!stealing)
    {
        ps->m_flags |= 0x10;
    }
    else
    {
        if (ps->m_flags & 0x10)
        {
            ps->m_flags ^= 0x10;
        }
    }
}

void xParGroupSetVisibility(xParGroup* ps, signed int vis)
{
    ps->m_visible = vis;
}

void xParGroupSetPriority(xParGroup* ps, unsigned char val)
{
    ps->m_priority = val;
}

void xParGroupRegister(xParGroup* ps)
{
    if (!sParGroupRegTableInit)
    {
        xParGroupRegisterInit();
    }
    for (unsigned char i = 0; i < 255; i++)
    {
        if (!sParGroupRegTable[i])
        {
            sParGroupRegTable[i] = ps;
            ps->m_regidx = i;
            return;
        }
    }
}

void xParGroupUnregister(xParGroup* ps)
{
    for (unsigned char i = 0; i < 255; i++)
    {
        if (sParGroupRegTable[i] == ps)
        {
            sParGroupRegTable[i] = 0;
            ps->m_regidx = 0;
            return;
        }
    }
}

void xParGroupSetActive(xParGroup* ps, unsigned int isActive)
{
    ps->m_active = isActive;
}

void xParGroupKillAllParticles(xParGroup* ps)
{
    xPar* i = ps->m_root;

    while (i)
    {
        xPar* tmp = i;
        i = i->m_next;
        xParGroupKillPar(ps, tmp);
    }
}

void xParGroupAnimate(xParGroup* ps, float dt)
{
    xPar* i = ps->m_root;
    xPar* tmp;
    float age = dt;

    if (ps->m_flags & 4)
    {
        age = 0.0f;
    }
    if (!ps->m_root)
    {
        ps->m_alive = 0;
    }

    while (i)
    {
        if (i->m_lifetime < 0.0f)
        {
            tmp = i;
            i = i->m_next;
            if (ps->m_flags & 8)
            {
                tmp->m_lifetime = tmp->m_asset->life;
            }
            else
            {
                xParGroupKillPar(ps, tmp);
            }
        }
        else
        {
            signed int c;

            c = i->m_c[0] + (signed int)(i->m_cvel[0] * dt);
            i->m_c[0] = (c < 0) ? 0 : ((c > 255) ? 255 : c);
            c = i->m_c[1] + (signed int)(i->m_cvel[1] * dt);
            i->m_c[1] = (c < 0) ? 0 : ((c > 255) ? 255 : c);
            c = i->m_c[2] + (signed int)(i->m_cvel[2] * dt);
            i->m_c[2] = (c < 0) ? 0 : ((c > 255) ? 255 : c);
            c = i->m_c[3] + (signed int)(i->m_cvel[3] * dt);
            i->m_c[3] = (c < 0) ? 0 : ((c > 255) ? 255 : c);
            i->m_size += i->m_sizeVel * dt;
            i->m_lifetime -= age;
            i = i->m_next;
        }
    }
}

xPar* xParGroupAddPar(xParGroup* ps)
{
    xPar* p;

    if (ps->m_flags & 2)
    {
        p = xParAlloc();
        if (!p)
        {
            signed int myPriority = ps->m_priority;

            for (unsigned char i = 0; i < 255; i++)
            {
                if (sParGroupRegTable[i] && sParGroupRegTable[i] != ps &&
                    (sParGroupRegTable[i]->m_flags & 2) && !(sParGroupRegTable[i]->m_flags & 0x10) &&
                    sParGroupRegTable[i]->m_priority < myPriority && sParGroupRegTable[i]->m_root)
                {
                    xParGroupKillPar(sParGroupRegTable[i], sParGroupRegTable[i]->m_root);
                    p = xParAlloc();
                    break;
                }
            }
        }
    }
    else if (ps->m_dead)
    {
        p = ps->m_dead;
        ps->m_dead = p->m_next;
        if (ps->m_dead)
        {
            ps->m_dead->m_prev = 0;
        }
    }
    else
    {
        p = 0;
    }

    if (p)
    {
        xParInit(p);
        xParGroupAddParP(ps, p);
    }
    return p;
}

#pragma auto_inline on
void xParGroupKillPar(xParGroup* ps, xPar* p)
{
    if (p == ps->m_root)
    {
        ps->m_root = p->m_next;
        if (ps->m_root)
        {
            ps->m_root->m_prev = 0;
        }
        p->m_next = 0;
        p->m_prev = 0;
    }
    else
    {
        if (p->m_next)
        {
            p->m_next->m_prev = p->m_prev;
        }
        if (p->m_prev)
        {
            p->m_prev->m_next = p->m_next;
        }
        p->m_prev = 0;
        p->m_next = 0;
    }

    ps->m_num_of_particles--;

    if (ps->m_flags & 2)
    {
        xParFree(p);
    }
    else
    {
        xParGroupAddParToDeadList(ps, p);
    }
}
#pragma auto_inline off

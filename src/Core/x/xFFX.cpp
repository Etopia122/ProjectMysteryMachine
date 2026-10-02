#include "xFFX.h"
#include "xMemMgr.h"
#include "rwtypes.h"

extern "C" double exp(double x);
extern "C" double sin(double x);

static inline float xexp(float x)
{
    return exp(x);
}

static inline float isin(float x)
{
    return sin(x);
}

static unsigned int psize;
static xFFX* pool;
static xFFX* alist;
static unsigned int shake_psize;
static xFFXShakeState* shake_pool;
static xFFXShakeState* shake_alist;
static unsigned int rot_match_psize;
static xFFXRotMatchState* rot_match_pool;
static xFFXRotMatchState* rot_match_alist;

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xFFXPoolInit(unsigned int num_ffx)
{
    psize = num_ffx;
    pool = (xFFX*)xMemAlloc(gActiveHeap, num_ffx * sizeof(xFFX));
    pool[0].next = 0;
    for (unsigned int i = 1; i < psize; i++)
    {
        pool[i].next = &pool[i - 1];
    }
    alist = &pool[psize - 1];
}

#pragma auto_inline on
xFFX* xFFXAlloc()
{
    xFFX* alist2 = alist;
    if (alist == 0)
    {
        return 0;
    }
    alist = alist->next;
    return alist2;
}

void xFFXFree(xFFX* ffx)
{
    ffx->next = alist;
    alist = ffx;
}
#pragma auto_inline off

void xFFXTurnOn(xFFX* f)
{
    f->flags |= 1;
}

void xFFXTurnOff(xFFX* f)
{
    f->flags &= 0xFFFFFFFE;
}

#pragma auto_inline on
signed short xFFXAddEffect(_xEnt* ent, xFFX* f)
{
    unsigned char numFFX;
    f->next = ent->ffx;
    f->flags |= 1;
    ent->ffx = f;
    numFFX = ent->num_ffx;
    ent->num_ffx = numFFX + 1;
    return numFFX;
}
#pragma auto_inline off

signed short xFFXAddEffect(_xEnt* ent, void (*dof)(_xEnt*, _xScene*, float, void*), void* fd)
{
    xFFX* f = xFFXAlloc();
    if (f == 0)
    {
        return -1;
    }
    f->doEffect = dof;
    f->fdata = fd;
    return xFFXAddEffect(ent, f);
}

#pragma auto_inline on
unsigned int xFFXRemoveEffectByFData(_xEnt* ent, void* fdata)
{
    xFFX** ffxh = &ent->ffx;

    while (*ffxh != 0)
    {
        xFFX* next = *ffxh;
        if (fdata == next->fdata)
        {
            next = next->next;
            ent->num_ffx--;
            xFFXFree(*ffxh);
            *ffxh = next;
            return 1;
        }
        ffxh = &(*ffxh)->next;
    }

    return 0;
}

static void xFFXApplyOne(xFFX* ffx, _xEnt* ent, _xScene* sc, float dt)
{
    if (ffx->next != 0)
    {
        xFFXApplyOne(ffx->next, ent, sc, dt);
    }
    if (ffx->flags & 1)
    {
        ffx->doEffect(ent, sc, dt, ffx->fdata);
    }
}
#pragma auto_inline off

void xFFXApply(_xEnt* ent, _xScene* sc, float dt)
{
    if (ent->ffx != 0)
    {
        xFFXApplyOne(ent->ffx, ent, sc, dt);
    }
}

void xFFXShakeUpdateEnt(_xEnt* ent, _xScene* sc, float dt, void* fdata)
{
    xFFXShakeState* ss = (xFFXShakeState*)fdata;
    float tnext = ss->tmr + dt;
    float mag = isin(ss->freq * tnext) * xexp(ss->alpha * tnext);

    if (ss->tmr == 0.0f)
    {
        ss->lval = 0.0f;
    }
    else if (ss->tmr >= ss->dur)
    {
        if (ss->lval * mag < 0.0f)
        {
            xFFXRemoveEffectByFData(ent, fdata);
            xFFXShakeFree(ss);
            return;
        }
    }

    _xVec3 dv;
    xVec3SMul(&dv, (_xVec3*)fdata, mag - ss->lval);
    xVec3AddTo((_xVec3*)&ent->frame->mat.pos, &dv);
    ss->lval = mag;
    ss->tmr = tnext;
}

void xFFXShakePoolInit(unsigned int num)
{
    shake_psize = num;
    shake_pool = (xFFXShakeState*)xMemAlloc(gActiveHeap, num * sizeof(xFFXShakeState));
    shake_pool->next = 0;
    for (unsigned int i = 1; i < shake_psize; i++)
    {
        shake_pool[i].next = &shake_pool[i - 1];
    }
    shake_alist = &shake_pool[shake_psize - 1];
}

xFFXShakeState* xFFXShakeAlloc()
{
    xFFXShakeState* alist2 = shake_alist;
    if (shake_alist == 0)
    {
        return 0;
    }
    shake_alist = shake_alist->next;
    return alist2;
}

#pragma auto_inline on
void xFFXShakeFree(xFFXShakeState* s)
{
    s->next = shake_alist;
    shake_alist = s;
}
#pragma auto_inline off

void xFFXRotMatchPoolInit(unsigned int num)
{
    rot_match_psize = num;
    rot_match_pool = (xFFXRotMatchState*)xMemAlloc(gActiveHeap, num * sizeof(xFFXRotMatchState));
    rot_match_pool->next = 0;
    for (unsigned int i = 1; i < rot_match_psize; ++i)
    {
        rot_match_pool[i].next = &rot_match_pool[i - 1];
    }
    rot_match_alist = &rot_match_pool[rot_match_psize - 1];
}

xFFXRotMatchState* xFFXRotMatchAlloc()
{
    xFFXRotMatchState* alist2 = rot_match_alist;
    if (rot_match_alist == 0)
    {
        return 0;
    }
    rot_match_alist = rot_match_alist->next;
    return alist2;
}

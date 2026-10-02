#include "xMovePoint.h"
#include "xMemMgr.h"
#include "xMath.h"
#include "zGlobals.h"
#include "zScene.h"


class _xRay3
{
public:
    _xVec3 origin;
    _xVec3 dir;
    float min_t;
    float max_t;
    signed int flags;
};

class _xCollis
{
public:
    unsigned int flags;
    unsigned int oid;
    void* optr;
    float dist;
    _xVec3 norm;
    _xVec3 tohit;
    _xVec3 depen;
    _xVec3 hdng;
    unsigned int ixtra;
};

class _zMovePoint : public _xMovePoint
{
public:
    unsigned int pad30;
};

unsigned int iRayHitsEnv(_xRay3* r, _xEnv* env, _xCollis* coll);

float xVec3Normalize(_xVec3* o, _xVec3* v);

static _zMovePoint* g_mvpt_list;
static signed int g_mvpt_cnt;

signed int zMovePointEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                             xBase* toParamWidget);
signed int MVPT_dflt_chklos(void* context, _xVec3* pos, _zMovePoint* mvpt);
_zMovePoint* zMovePointFindNearest(signed int flags, _xVec3* pos, float rad_close, float rad_max,
                                   void* context,
                                   signed int (*chklos)(void*, _xVec3*, _zMovePoint*),
                                   signed int* idx_resume, _zMovePoint** mvpt_resume);

static inline void zMovePointReset(_zMovePoint* m)
{
    xMovePointReset(m);
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

_zMovePoint* zMovePoint_GetMemPool(signed int cnt)
{
    g_mvpt_list = cnt ? (_zMovePoint*)xMemAlloc(gActiveHeap, cnt * sizeof(_zMovePoint)) : 0;
    g_mvpt_cnt = cnt;
    return g_mvpt_list;
}

void zMovePointInit(_zMovePoint* m, p2MovePointAsset* asset)
{
    xMovePointInit(m, asset);
    m->eventFunc = zMovePointEventCB;
    if (m->linkCount)
    {
        m->link = (p2LinkAsset*)(((unsigned int*)asset + sizeof(p2MovePointAsset) / 4) +
                                (unsigned int)asset->numPoints);
    }
    else
    {
        m->link = 0;
    }
}

_zMovePoint* zMovePoint_GetInst(signed int n)
{
    return &g_mvpt_list[n];
}

void zMovePointSetup(_zMovePoint* mvpt, zScene* scn)
{
    xMovePointSetup(mvpt, (_xScene*)scn);
}

_zMovePoint* zMovePoint_From_xAssetID(unsigned int aid)
{
    _zMovePoint* ret = 0;
    for (signed int i = 0; i < g_mvpt_cnt; i++)
    {
        _zMovePoint* pnt = &g_mvpt_list[i];
        if (pnt->asset->id == aid)
        {
            ret = pnt;
            break;
        }
    }
    return ret;
}

void zMovePointSave(_zMovePoint* ent, xSerial* s)
{
    xMovePointSave(ent, s);
}

void zMovePointLoad(_zMovePoint* ent, xSerial* s)
{
    xMovePointLoad(ent, s);
}

signed int zMovePointEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                             xBase* toParamWidget)
{
    switch (toEvent)
    {
    case 0x26:
        ((_zMovePoint*)to)->on = 1;
        break;
    case 0x27:
        ((_zMovePoint*)to)->on = 0;
        break;
    case 0x1F:
        break;
    case 0xA:
        zMovePointReset((_zMovePoint*)to);
        break;
    }
    return 1;
}

float zMovePointGetNext(_zMovePoint* current, _zMovePoint* prev, _zMovePoint** next,
                        _xVec3* hdng)
{
    return xMovePointGetNext(current, prev, (_xMovePoint**)next, hdng);
}

_xVec3* zMovePointGetPos(_zMovePoint* m)
{
    return xMovePointGetPos(m);
}

float zMovePointGetDelay(_zMovePoint* m)
{
    return m->delay;
}

unsigned char zMovePointIsOn(_zMovePoint* m)
{
    if (m)
    {
        return m->on;
    }
    return 0;
}

_zMovePoint* zMovePointFindNearest(signed int flags, _xVec3* pos, void* context,
                                   signed int (*chklos)(void*, _xVec3*, _zMovePoint*))
{
    return zMovePointFindNearest(flags, pos, -1.0f, 999.0f, context, chklos, 0, 0);
}

// TODO (99.5%): the original gives cnt/mvpt r30/r29 and the hoisted flag
// tests r28-r25; here the flag tests rank first.
_zMovePoint* zMovePointFindNearest(signed int flags, _xVec3* pos, float rad_close, float rad_max,
                                   void* context,
                                   signed int (*chklos)(void*, _xVec3*, _zMovePoint*),
                                   signed int* idx_resume, _zMovePoint** mvpt_resume)
{
    signed int cnt;
    _zMovePoint* mvpt;
    float rcsq = rad_close * rad_close;
    float rmsq = rad_max * rad_max;
    _xVec3 delta = { 0 };
    float dsq_best = 999999.0f;
    signed int resume;
    _zMovePoint* mvpt_close = 0;
    _zMovePoint* mvpt_best = 0;
    signed int i;
    signed int idx_end;
    signed int didlos = 0;
    signed int idx_start;

    cnt = g_mvpt_cnt;
    if (!idx_resume || !mvpt_resume)
    {
        flags &= ~0x20;
    }

    if (!chklos && (flags & 0x10))
    {
        chklos = MVPT_dflt_chklos;
    }
    else if (!(flags & 0x10))
    {
        chklos = 0;
    }

    resume = flags & 0x20;
    if (resume)
    {
        idx_start = *idx_resume < 0 ? 0 : (*idx_resume < cnt ? *idx_resume : cnt);
        idx_end = idx_start + 50 < cnt ? idx_start + 50 : cnt;
        if (mvpt_resume)
        {
            mvpt_best = *mvpt_resume;
            if (mvpt_best)
            {
                xVec3Sub(&delta, mvpt_best->pos, pos);
                if (flags & 4)
                {
                    dsq_best = delta.x * delta.x + delta.z * delta.z;
                }
                else
                {
                    dsq_best = xVec3Dot(&delta, &delta);
                }
            }
        }
    }
    else
    {
        idx_start = 0;
        idx_end = cnt;
    }

    for (i = idx_start; i < idx_end; i++)
    {
        mvpt = &g_mvpt_list[i];
        float dsq;

        if (!mvpt->pos)
            continue;
        if (!(flags & 1) && mvpt->on)
            continue;
        if (!(flags & 2) && !mvpt->on)
            continue;
        if ((flags & 8) && !(mvpt->flg_props & 1))
            continue;

        xVec3Sub(&delta, mvpt->pos, pos);
        if (flags & 4)
        {
            if (delta.y > 1.0f || delta.y < -1.0f)
                continue;
            dsq = delta.x * delta.x + delta.z * delta.z;
        }
        else
        {
            dsq = xVec3Dot(&delta, &delta);
        }

        if (rad_max > 0.0f && dsq > rmsq)
            continue;
        if (mvpt_best && dsq > dsq_best)
            continue;
        if (flags & 0x10)
        {
            didlos = 1;
            if (!chklos(context, pos, mvpt))
                continue;
        }

        dsq_best = dsq;
        mvpt_best = mvpt;
        if (rad_close > 0.0f && dsq < rcsq)
        {
            mvpt_close = mvpt;
            break;
        }
        if (resume && didlos)
            break;
    }

    if (mvpt_close)
    {
        if (idx_resume)
            *idx_resume = -2;
        if (mvpt_resume)
            *mvpt_resume = mvpt_close;
    }
    else if (i == cnt)
    {
        mvpt_close = mvpt_best;
        if (idx_resume)
            *idx_resume = -2;
        if (mvpt_resume)
            *mvpt_resume = mvpt_best;
    }
    else if (resume && i == idx_end)
    {
        *mvpt_resume = mvpt_best;
        *idx_resume = i;
    }
    else if (resume && didlos)
    {
        *mvpt_resume = mvpt_best;
        *idx_resume = i + 1;
    }

    return mvpt_close;
}

// TODO (96%): the original loads the 0.0f constant after mvpt->pos.
signed int MVPT_dflt_chklos(void* context, _xVec3* pos, _zMovePoint* mvpt)
{
    _xRay3 ray = { 0 };
    _xVec3 delta = { 0 };
    _xCollis colrec = { 0 };
    float dist;

    ray.origin = *mvpt->pos;
    xVec3Sub(&delta, pos, &ray.origin);
    dist = xVec3Dot(&delta, &delta);
    dist = xsqrt(dist);
    xVec3Normalize(&ray.dir, &delta);
    ray.flags = 0xC00;
    ray.max_t = dist;
    ray.min_t = 0.0f;
    iRayHitsEnv(&ray, globals.sceneCur->env, &colrec);
    if (colrec.optr)
    {
        return 0;
    }
    return 1;
}

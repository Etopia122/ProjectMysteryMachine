#include "xMovePoint.h"
#include "xMemMgr.h"
#include "iMath.h"

xBase* xSceneResolvID(_xScene* sc, unsigned int id);
float xVec3Normalize(_xVec3* o, _xVec3* v);
xSpline3* xSpline3_Bezier(_xVec3* points, float* time, unsigned int numpoints,
                          unsigned int numalloc, _xVec3* p1, _xVec3* p2);
void xSpline3_ArcInit(xSpline3* spl, unsigned int sample);

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xMovePointInit(_xMovePoint* m, p2MovePointAsset* asset)
{
    xBaseInit(m, asset);
    m->asset = asset;
    m->pos = &m->asset->pos;
    m->on = asset->on;
    m->delay = asset->delay;
    m->flg_props = asset->flg_props;
    m->spl = 0;

    if (asset->numPoints)
    {
        m->nodes = (_xMovePoint**)xMemAlloc(gActiveHeap, asset->numPoints * sizeof(_xMovePoint*));
    }
    else
    {
        m->nodes = 0;
    }
}

void xMovePointSave(_xMovePoint* m, xSerial* s)
{
    xBaseSave(m, s);
}

void xMovePointLoad(_xMovePoint* m, xSerial* s)
{
    xBaseLoad(m, s);
}

void xMovePointReset(_xMovePoint* m)
{
    xBaseReset(m, m->asset);
    m->on = m->asset->on;
    m->delay = m->asset->delay;
}

void xMovePointSetup(_xMovePoint* m, _xScene* sc)
{
    m->node_wt_sum = 0;
    unsigned int* pointIds = (unsigned int*)(m->asset + 1);
    for (unsigned short idx = 0; idx < m->asset->numPoints; ++idx)
    {
        unsigned int id = pointIds[idx];
        m->nodes[idx] = (_xMovePoint*)xSceneResolvID(sc, id);
        m->node_wt_sum += m->nodes[idx]->asset->wt;
        m->nodes[idx]->prev = m;
    }
}

void xMovePointSplineSetup(_xMovePoint* m)
{
    _xMovePoint *w0, *w1, *w2, *w3;
    _xVec3 points[2];
    _xVec3 p1, p2;

    if (m->asset->bezIndex != 1)
        return;
    if (m->spl)
        return;

    w0 = m->prev;
    w1 = m;
    w2 = m->nodes[0];

    points[0] = *w0->pos;
    if (w2->asset->bezIndex)
    {
        w3 = w2->nodes[0];
        p1 = *w1->pos;
        p2 = *w2->pos;
        points[1] = *w3->pos;
    }
    else
    {
        p1.x = (1 / 3.f) * w0->pos->x + (2 / 3.f) * w1->pos->x;
        p1.y = (1 / 3.f) * w0->pos->y + (2 / 3.f) * w1->pos->y;
        p1.z = (1 / 3.f) * w0->pos->z + (2 / 3.f) * w1->pos->z;
        p2.x = (2 / 3.f) * w1->pos->x + (1 / 3.f) * w2->pos->x;
        p2.y = (2 / 3.f) * w1->pos->y + (1 / 3.f) * w2->pos->y;
        p2.z = (2 / 3.f) * w1->pos->z + (1 / 3.f) * w2->pos->z;
        points[1] = *w2->pos;
    }

    m->spl = xSpline3_Bezier(points, 0, 2, 0, &p1, &p2);
    xSpline3_ArcInit(m->spl, 20);
}

void xMovePointSplineDestroy(_xMovePoint* m)
{
    if (m->spl)
    {
        m->spl = 0;
    }
}

float xMovePointGetNext(_xMovePoint* m, _xMovePoint* prev, _xMovePoint** next, _xVec3* hdng)
{
    if (m->asset->numPoints < 1)
    {
        *next = 0;
        return 0.0f;
    }

    _xMovePoint* previousOption = 0;
    signed int rnd = irand() % m->node_wt_sum;

    for (unsigned short idx = 0; idx < m->asset->numPoints; idx++)
    {
        *next = m->nodes[idx];
        rnd -= (*next)->asset->wt;
        if ((*next)->on == 0)
        {
            *next = 0;
        }
        else
        {
            previousOption = *next;
            if (*next == prev)
            {
                *next = 0;
            }
            else if (rnd < 0)
            {
                break;
            }
        }
    }

    if (!*next)
    {
        if (previousOption)
        {
            *next = previousOption;
        }
        else
        {
            return 0.0f;
        }
    }

    if (hdng)
    {
        xVec3Sub(hdng, (*next)->pos, m->pos);
        return xVec3Normalize(hdng, hdng);
    }
    return 0.0f;
}

_xVec3* xMovePointGetPos(_xMovePoint* m)
{
    return m->pos;
}

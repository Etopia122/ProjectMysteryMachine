#include "xCollide.h"
#include "xMath.h"

class _xScene;

enum _xCollsIdx
{
    k_XCOLLS_IDX_FLOOR,
    k_XCOLLS_IDX_CEIL,
    k_XCOLLS_IDX_FRONT,
    k_XCOLLS_IDX_LEFT,
    k_XCOLLS_IDX_REAR,
    k_XCOLLS_IDX_RIGHT,
    k_XCOLLS_IDX_COUNT
};

extern const _xVec3 g_O3;

float xVec3Normalize(_xVec3* o, _xVec3* v);
void xMat3x3Normalize(_xMat3x3* o, _xMat3x3* m);
void iBoxIsectSphere(_xBox* box, _xSphere* p, _xIsect* isx);
void iSphereIsectSphere(_xSphere* s, _xSphere* p, _xIsect* isx);
void iCollideInit(_xScene* sc);

static inline void xVec3Inv(_xVec3* o, _xVec3* v)
{
    xVec3SMul(o, v, -1.0f);
}

static inline void xVec3SubL(_xVec3* o, const _xVec3* a, const _xVec3* b)
{
    float x = a->x - b->x;
    float y = a->y - b->y;
    float z = a->z - b->z;
    o->x = x;
    o->y = y;
    o->z = z;
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

_xCollsIdx xCollideGetCollsIdx(_xVec3* tohit, _xMat3x3* mat)
{
    if (tohit->y * tohit->y > tohit->x * tohit->x + tohit->z * tohit->z)
    {
        return (_xCollsIdx)!(tohit->y < 0.0f);
    }

    float local_x = mat->right.x * tohit->x + mat->right.z * tohit->z;
    float local_z = mat->at.x * tohit->x + mat->at.z * tohit->z;
    if (local_x > 0.0f)
    {
        if (local_z > 0.0f)
        {
            if (local_x > local_z)
            {
                return k_XCOLLS_IDX_LEFT;
            }
            return k_XCOLLS_IDX_FRONT;
        }
        if (local_x > -local_z)
        {
            return k_XCOLLS_IDX_LEFT;
        }
        return k_XCOLLS_IDX_REAR;
    }
    if (local_z > 0.0f)
    {
        if (local_x < -local_z)
        {
            return k_XCOLLS_IDX_RIGHT;
        }
        return k_XCOLLS_IDX_FRONT;
    }
    if (local_x < local_z)
    {
        return k_XCOLLS_IDX_RIGHT;
    }
    return k_XCOLLS_IDX_REAR;
}

void xCollideInit(_xScene* sc)
{
    iCollideInit(sc);
}

unsigned int xSphereHitsSphere(_xSphere* a, _xSphere* b, _xCollis* coll)
{
    _xIsect isx;

    iSphereIsectSphere(b, a, &isx);
    if (!(isx.penned <= 0.0f))
    {
        coll->flags &= ~1;
        return 0;
    }

    if (isx.contained <= 0.0f)
    {
        coll->flags |= 0x10;
    }
    coll->dist = a->r + isx.penned;
    if (coll->flags & 0x1600)
    {
        if (isx.dist == 0.0f)
        {
            xVec3Copy(&coll->tohit, (_xVec3*)&g_O3);
        }
        else
        {
            xVec3SMul(&coll->tohit, &isx.norm, -coll->dist / isx.dist);
        }
    }
    if (coll->flags & 0x800)
    {
        if (isx.dist == 0.0f)
        {
            xVec3Copy(&coll->depen, (_xVec3*)&g_O3);
        }
        else
        {
            xVec3SMul(&coll->depen, &isx.norm, -isx.penned / isx.dist);
        }
    }
    if (coll->flags & 0x1200)
    {
        xVec3Normalize(&coll->hdng, &coll->tohit);
    }
    if (coll->flags & 0x200)
    {
        xVec3Inv(&coll->norm, &coll->hdng);
    }
    coll->flags |= 1;
    return 1;
}

#pragma auto_inline on
unsigned int xSphereHitsBox(_xSphere* a, _xBox* b, _xCollis* coll)
{
    _xIsect isx;

    iBoxIsectSphere(b, a, &isx);
    if (!(isx.penned <= 0.0f))
    {
        coll->flags &= ~1;
        return 0;
    }

    if (isx.contained <= 0.0f)
    {
        coll->flags |= 0x10;
    }
    coll->dist = isx.dist;
    if (coll->flags & 0x400)
    {
        xVec3Copy(&coll->tohit, &isx.norm);
    }
    if (coll->flags & 0x800)
    {
        if (isx.dist == 0.0f)
        {
            xVec3Copy(&coll->depen, (_xVec3*)&g_O3);
        }
        else
        {
            xVec3SMul(&coll->depen, &isx.norm, isx.penned / isx.dist);
        }
    }
    if (coll->flags & 0x1200)
    {
        xVec3Normalize(&coll->hdng, &isx.norm);
    }
    if (coll->flags & 0x200)
    {
        xVec3Inv(&coll->norm, &coll->hdng);
    }
    coll->flags |= 1;
    return 1;
}
#pragma auto_inline off

// TODO (98.9%): the original computes the three differences in x, y, z order
// and stores x first; here the scheduler starts with y.
unsigned int xSphereHitsOBB(_xSphere* a, _xBox* b, _xMat4x3* m, _xCollis* coll)
{
    _xSphere xs;
    _xMat3x3 mn;
    float scale;

    scale = xsqrt(m->right.x * m->right.x + m->right.y * m->right.y + m->right.z * m->right.z);
    xMat3x3Normalize(&mn, (_xMat3x3*)m);
    xVec3SubL(&xs.center, &a->center, &m->pos);
    xMat3x3Tolocal(&xs.center, (_xMat3x3*)m, &xs.center);
    xs.r = a->r / scale;

    xSphereHitsBox(&xs, b, coll);
    if (!(coll->flags & 1))
    {
        return 0;
    }

    if (coll->flags & 0x200)
    {
        xMat3x3RMulVec(&coll->norm, &mn, &coll->norm);
    }
    if (coll->flags & 0x800)
    {
        xMat3x3RMulVec(&coll->depen, (_xMat3x3*)m, &coll->depen);
    }
    if (coll->flags & 0x400)
    {
        xMat3x3RMulVec(&coll->tohit, (_xMat3x3*)m, &coll->tohit);
    }
    if (coll->flags & 0x1000)
    {
        xMat3x3RMulVec(&coll->hdng, &mn, &coll->hdng);
    }
    coll->dist *= scale;
    return 1;
}

#include "iMath3.h"
#include "xBound.h"
#include "xMath.h"

void xMat3x3Normalize(_xMat3x3* o, _xMat3x3* m);
signed int iSphereHitsModel3(_xSphere* b, xModelInstance* m, _xCollis* colls, unsigned char ncolls,
                             float sth);

// xsqrt with the result written back to x: closer here than xMath.h's form
// (which xMath.cpp needs).
static inline float xsqrt2(float x)
{
    if (x > 0.0f)
    {
        volatile float y;
        double guess = __frsqrte((double)x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        y = (float)(x * guess);
        x = y;
    }
    return x;
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

_xVec3* xBoundCenter(_xBound* b)
{
    if (b->type == 1)
    {
        return &b->sph.center;
    }
    else if (b->type == 4)
    {
        return &b->box.center;
    }
    else if (b->type == 2)
    {
        return &b->box.center;
    }
    else if (b->type == 3)
    {
        return &b->cyl.center;
    }
    return 0;
}

void xBoundUpdate(_xBound* b)
{
    if (b->type == 2)
    {
        xVec3Add(&b->box.center, &b->box.box.lower, &b->box.box.upper);
        xVec3SMulBy(&b->box.center, 0.5f);
    }
    else if (b->type == 4)
    {
        xVec3Add(&b->box.center, &b->box.box.lower, &b->box.box.upper);
        xVec3SMulBy(&b->box.center, 0.5f);
        xMat4x3Toworld(&b->box.center, b->mat, &b->box.center);
    }

    if (b->type != 0)
    {
        xQuickCullForBound(&b->qcd, b);
    }
}

void xBoundHitsBound(_xBound* a, _xBound* b, _xCollis* c)
{
    if (!xQuickCullIsects(&a->qcd, &b->qcd))
    {
        c->flags &= ~1;
        return;
    }

    switch (a->type)
    {
    case 1:
        switch (b->type)
        {
        case 1:
            xSphereHitsSphere(&a->sph, &b->sph, c);
            break;
        case 4:
            xSphereHitsOBB(&a->sph, &b->box.box, b->mat, c);
            break;
        case 2:
            xSphereHitsBox(&a->sph, &b->box.box, c);
            break;
        case 3:
            break;
        }
        break;
    case 2:
    case 3:
    case 4:
        break;
    }
}

void xBoundHitsModel(_xBound* b, xModelInstance* m, _xCollis* colls, unsigned char ncolls,
                     float sth)
{
    switch (b->type)
    {
    case 1:
        iSphereHitsModel3(&b->sph, m, colls, ncolls, sth);
        break;
    case 2:
    case 3:
    case 4:
        break;
    }
}

// TODO (87%) and the two callers below (90%, 93%): the original loads all of
// r->origin / v before subtracting mat->pos; xMat4x3Tolocal's xVec3Sub here
// loads and stores one component at a time.
#pragma auto_inline on
void xRayHitsBound(const _xRay3* r, const _xBound* b, _xCollis* c)
{
    _xIsect isect;
    _xRay3 lr;
    _xMat3x3 mn;
    _xMat3x3* dmp;

    if (b->type == 1)
    {
        iSphereIsectRay(&b->sph, r, &isect);
    }
    else if (b->type == 4)
    {
        float len2 = b->mat->right.x * b->mat->right.x + b->mat->right.y * b->mat->right.y +
                     b->mat->right.z * b->mat->right.z;
        if ((float)__fabs(len2 - 1.0f) >= 0.001f)
        {
            len2 = xsqrt2(len2);
            float one_ms = 1.0f / len2;
            lr.max_t = r->max_t * one_ms;
            lr.min_t = r->min_t * one_ms;
            xMat3x3Normalize(&mn, (_xMat3x3*)b->mat);
            dmp = &mn;
        }
        else
        {
            lr.max_t = r->max_t;
            dmp = (_xMat3x3*)b->mat;
            lr.min_t = r->min_t;
        }
        xMat4x3Tolocal(&lr.origin, b->mat, &r->origin);
        xMat3x3Tolocal(&lr.dir, dmp, (_xVec3*)&r->dir);
        lr.flags = r->flags;
        iBoxIsectRay(&b->box.box, &lr, &isect);
    }
    else if (b->type == 2)
    {
        iBoxIsectRay(&b->box.box, r, &isect);
    }

    if (isect.penned <= 0.0f)
    {
        c->flags |= 1;
        c->dist = isect.dist;
    }
    else
    {
        c->flags &= ~1;
    }
}
#pragma auto_inline off

void xRayHitsBound(const _xRay3* r, const xQCData* q, const _xBound* b, _xCollis* c)
{
    if (!xQuickCullIsects(q, &b->qcd))
    {
        c->flags &= ~1;
    }
    else
    {
        xRayHitsBound(r, b, c);
    }
}

void xVecHitsBound(const _xVec3* v, const _xBound* b, _xCollis* c)
{
    _xIsect isect;

    if (b->type == 1)
    {
        iSphereIsectVec(&b->sph, v, &isect);
    }
    else if (b->type == 4)
    {
        _xVec3 lv;
        xMat4x3Tolocal(&lv, b->mat, v);
        iBoxIsectVec(&b->box.box, &lv, &isect);
    }
    else if (b->type == 2)
    {
        iBoxIsectVec(&b->box.box, v, &isect);
    }

    if (isect.penned <= 0.0f)
    {
        c->flags |= 1;
    }
    else
    {
        c->flags &= ~1;
    }
}

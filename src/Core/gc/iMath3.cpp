#include "iMath3.h"

#include "xMath.h"
#include "xMath3.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static inline void xVec3Init(_xVec3* v, float x, float y, float z)
{
    v->x = x;
    v->y = y;
    v->z = z;
}

#pragma fp_contract off
static inline float xVec3Length(const _xVec3* v)
{
    return xsqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

static inline float xVec2Dist(float x1, float y1, float x2, float y2)
{
    float dx = x1 - x2;
    float dy = y1 - y2;
    return xsqrt(dx * dx + dy * dy);
}
#pragma fp_contract on

static inline unsigned int iBoxRangeCode(float lo, float hi, float lower, float upper)
{
    if (lo < lower)
    {
        if (hi < lower)
        {
            return 2;
        }
        else if (hi > upper)
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        if (lo > upper)
        {
            return 5;
        }
        else if (hi > upper)
        {
            return 4;
        }
        else
        {
            return 3;
        }
    }
}

static inline void xRay3Point(_xVec3* o, const _xRay3* r, float t)
{
    xVec3Copy(o, &r->dir);
    xVec3SMulBy(o, t);
    xVec3AddTo(o, &r->origin);
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void iMath3Init()
{
}

void iMath3Exit()
{
}

void iSphereIsectVec(const _xSphere* s, const _xVec3* v, _xIsect* isx)
{
    xVec3Sub(&isx->norm, v, &s->center);

    isx->dist = xVec3Length(&isx->norm);
    isx->penned = isx->dist - s->r;
}

void iSphereIsectRay(const _xSphere* s, const _xRay3* r, _xIsect* isx)
{
    float t_in, t_out;
    unsigned int num;

    if (!(r->flags & 0x400))
    {
        ((_xRay3*)r)->min_t = 0.0f;
    }

    if (!(r->flags & 0x800))
    {
        ((_xRay3*)r)->max_t = 1.0f;
    }

    xVec3Sub(&isx->norm, &r->origin, &s->center);

    num = xMathSolveQuadratic(xVec3Dot(&r->dir, &r->dir), 2.0f * xVec3Dot(&isx->norm, &r->dir),
                              xVec3Dot(&isx->norm, &isx->norm) - s->r * s->r, &t_in, &t_out);

    if (num == 0)
    {
        isx->penned = 1.0f;
        isx->contained = 1.0f;
    }
    else if (num == 1)
    {
        if (t_in < r->min_t || t_in > r->max_t)
        {
            isx->dist = t_in;
            isx->penned = 1.0f;
            isx->contained = 1.0f;
        }
        else
        {
            isx->dist = t_in;
            isx->penned = -1.0f;
            isx->contained = 1.0f;
        }
    }
    else if (t_in < r->min_t)
    {
        if (t_out < r->min_t)
        {
            isx->dist = t_out;
            isx->penned = 1.0f;
            isx->contained = 1.0f;
        }
        else
        {
            isx->dist = t_out;
            isx->penned = -1.0f;
            isx->contained = -1.0f;
        }
    }
    else
    {
        if (t_in <= r->max_t)
        {
            isx->dist = t_in;
            isx->penned = -1.0f;
            isx->contained = 1.0f;
        }
        else
        {
            isx->dist = t_in;
            isx->penned = 1.0f;
            isx->contained = 1.0f;
        }
    }
}

void iSphereIsectSphere(const _xSphere* s, const _xSphere* p, _xIsect* isx)
{
    xVec3Sub(&isx->norm, &p->center, &s->center);

    isx->dist = xVec3Length(&isx->norm);
    isx->penned = isx->dist - p->r - s->r;
    isx->contained = isx->dist - s->r;
}

void iSphereInitBoundOBB(_xSphere* s, const _xBox* b, const _xMat4x3* m)
{
    _xVec3 ext;

    xVec3Add(&s->center, &b->lower, &b->upper);
    xVec3SMulBy(&s->center, 0.5f);
    ext.x = b->upper.x - s->center.x;
    ext.y = b->upper.y - s->center.y;
    ext.z = b->upper.z - s->center.z;
    xMat4x3Toworld(&s->center, m, &s->center);
    s->r = xVec3Length(&m->right) * xVec3Length(&ext);
}

void iCylinderIsectVec(const _xCylinder* c, const _xVec3* v, _xIsect* isx)
{
    float b = c->center.y + c->h;

    if (v->y >= c->center.y - c->h && v->y <= b &&
        xVec2Dist(c->center.x, c->center.z, v->x, v->z) <= c->r)
    {
        isx->penned = -1.0f;
    }
    else
    {
        isx->penned = 1.0f;
    }
}

// TODO: 98.5%. Register allocation of dx/dz.
void iCylinderIsectRay(const _xCylinder* c, const _xRay3* r, _xIsect* isx)
{
    float a;
    float b;
    float t_in;
    float t_out;
    unsigned int num;

    float dz = r->origin.z - c->center.z;
    float dx = r->origin.x - c->center.x;
    num = xMathSolveQuadratic(r->dir.x * r->dir.x + r->dir.z * r->dir.z,
                              2.0f * (dx * r->dir.x + dz * r->dir.z),
                              dx * dx + dz * dz - c->r * c->r, &t_in, &t_out);
    if (num == 0)
    {
        isx->penned = 1.0f;
        return;
    }

    a = c->center.y - c->h;
    b = c->center.y + c->h;
    if (t_in < 0.0f)
    {
        if (num == 1 || t_out < 0.0f)
        {
            isx->penned = 1.0f;
            return;
        }
        xRay3Point(&isx->norm, r, t_in);
        if (isx->norm.y >= a && isx->norm.y <= b)
        {
            isx->dist = t_out;
            isx->contained = -1.0f;
            goto hit;
        }
        isx->penned = 1.0f;
        return;
    }

    xRay3Point(&isx->norm, r, t_in);
    if (isx->norm.y >= a && isx->norm.y <= b)
    {
        isx->dist = t_in;
        isx->contained = 1.0f;
        goto hit;
    }
    xRay3Point(&isx->norm, r, t_in);
    if (isx->norm.y >= a && isx->norm.y <= b)
    {
        isx->dist = t_out;
        isx->contained = 1.0f;
        goto hit;
    }
    isx->penned = 1.0f;
    return;

hit:
    isx->penned = -1.0f;
}

void iBoxVecDist(const _xBox* box, const _xVec3* v, _xIsect* isx)
{
    if (v->x < box->lower.x)
    {
        if (v->y < box->lower.y)
        {
            if (v->z < box->lower.z)
            {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x00000000;
                isx->flags |= 0x80000000;
            }
            else if (v->z <= box->upper.z)
            {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = 0.0f;
                isx->flags = isx->flags & ~0x1F000000 | 0x01000000;
                isx->flags |= 0x40000000;
            }
            else
            {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x02000000;
                isx->flags |= 0x80000000;
            }
        }
        else if (v->y <= box->upper.y)
        {
            if (v->z < box->lower.z)
            {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = 0.0f;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x03000000;
                isx->flags |= 0x40000000;
            }
            else if (v->z <= box->upper.z)
            {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = 0.0f;
                isx->norm.z = 0.0f;
                isx->flags = isx->flags & ~0x1F000000 | 0x04000000;
                isx->flags |= 0x20000000;
            }
            else
            {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = 0.0f;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x05000000;
                isx->flags |= 0x40000000;
            }
        }
        else
        {
            if (v->z < box->lower.z)
            {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x06000000;
                isx->flags |= 0x80000000;
            }
            else if (v->z <= box->upper.z)
            {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = 0.0f;
                isx->flags = isx->flags & ~0x1F000000 | 0x07000000;
                isx->flags |= 0x40000000;
            }
            else
            {
                isx->norm.x = box->lower.x - v->x;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x08000000;
                isx->flags |= 0x80000000;
            }
        }
    }
    else if (v->x <= box->upper.x)
    {
        if (v->y < box->lower.y)
        {
            if (v->z < box->lower.z)
            {
                isx->norm.x = 0.0f;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x09000000;
                isx->flags |= 0x40000000;
            }
            else if (v->z <= box->upper.z)
            {
                isx->norm.x = 0.0f;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = 0.0f;
                isx->flags = isx->flags & ~0x1F000000 | 0x0A000000;
                isx->flags |= 0x20000000;
            }
            else
            {
                isx->norm.x = 0.0f;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x0B000000;
                isx->flags |= 0x40000000;
            }
        }
        else if (v->y <= box->upper.y)
        {
            if (v->z < box->lower.z)
            {
                isx->norm.x = 0.0f;
                isx->norm.y = 0.0f;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x0C000000;
                isx->flags |= 0x20000000;
            }
            else if (v->z <= box->upper.z)
            {
            }
            else
            {
                isx->norm.x = 0.0f;
                isx->norm.y = 0.0f;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x0E000000;
                isx->flags |= 0x20000000;
            }
        }
        else
        {
            if (v->z < box->lower.z)
            {
                isx->norm.x = 0.0f;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x0F000000;
                isx->flags |= 0x40000000;
            }
            else if (v->z <= box->upper.z)
            {
                isx->norm.x = 0.0f;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = 0.0f;
                isx->flags = isx->flags & ~0x1F000000 | 0x10000000;
                isx->flags |= 0x20000000;
            }
            else
            {
                isx->norm.x = 0.0f;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x11000000;
                isx->flags |= 0x40000000;
            }
        }
    }
    else
    {
        if (v->y < box->lower.y)
        {
            if (v->z < box->lower.z)
            {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x12000000;
                isx->flags |= 0x80000000;
            }
            else if (v->z <= box->upper.z)
            {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = 0.0f;
                isx->flags = isx->flags & ~0x1F000000 | 0x13000000;
                isx->flags |= 0x40000000;
            }
            else
            {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = box->lower.y - v->y;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x14000000;
                isx->flags |= 0x80000000;
            }
        }
        else if (v->y <= box->upper.y)
        {
            if (v->z < box->lower.z)
            {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = 0.0f;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x15000000;
                isx->flags |= 0x40000000;
            }
            else if (v->z <= box->upper.z)
            {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = 0.0f;
                isx->norm.z = 0.0f;
                isx->flags = isx->flags & ~0x1F000000 | 0x16000000;
                isx->flags |= 0x20000000;
            }
            else
            {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = 0.0f;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x17000000;
                isx->flags |= 0x40000000;
            }
        }
        else
        {
            if (v->z < box->lower.z)
            {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = box->lower.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x18000000;
                isx->flags |= 0x80000000;
            }
            else if (v->z <= box->upper.z)
            {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = 0.0f;
                isx->flags = isx->flags & ~0x1F000000 | 0x19000000;
                isx->flags |= 0x40000000;
            }
            else
            {
                isx->norm.x = box->upper.x - v->x;
                isx->norm.y = box->upper.y - v->y;
                isx->norm.z = box->upper.z - v->z;
                isx->flags = isx->flags & ~0x1F000000 | 0x1A000000;
                isx->flags |= 0x80000000;
            }
        }
    }

    isx->dist = xVec3Length(&isx->norm);
}

#pragma auto_inline on
void iBoxIsectVec(const _xBox* b, const _xVec3* v, _xIsect* isx)
{
    if (v->x >= b->lower.x && v->x <= b->upper.x && v->y >= b->lower.y && v->y <= b->upper.y &&
        v->z >= b->lower.z && v->z <= b->upper.z)
    {
        isx->penned = -1.0f;
    }
    else
    {
        isx->penned = 1.0f;
    }
}
#pragma auto_inline reset


static inline unsigned int ClipPlane(float denom, float numer, float* t_in, float* t_out)
{
    if (denom > 0.0f)
    {
        if (numer > denom * (*t_out))
        {
            return 0;
        }

        if (numer > denom * (*t_in))
        {
            *t_in = numer / denom;
        }

        return 1;
    }
    else if (denom < 0.0f)
    {
        if (numer > denom * (*t_in))
        {
            return 0;
        }

        if (numer > denom * (*t_out))
        {
            *t_out = numer / denom;
        }

        return 1;
    }

    return (numer <= 0.0f);
}


static unsigned int ClipBox(const _xVec3* r3, const _xVec3* r4, const _xVec3* r5, float* t_in, float* t_out)
{
    return (ClipPlane(r5->x, -r4->x - r3->x, t_in, t_out) &&
            ClipPlane(-r5->x, r4->x - r3->x, t_in, t_out) &&
            ClipPlane(r5->y, -r4->y - r3->y, t_in, t_out) &&
            ClipPlane(-r5->y, r4->y - r3->y, t_in, t_out) &&
            ClipPlane(r5->z, -r4->z - r3->z, t_in, t_out) &&
            ClipPlane(-r5->z, r4->z - r3->z, t_in, t_out));
}

void iBoxIsectRay(const _xBox* b, const _xRay3* r, _xIsect* isx)
{
    _xVec3 var_14, var_20;
    float t_in, t_out;

    if (!(r->flags & 0x400))
    {
        ((_xRay3*)r)->min_t = 0.0f;
    }

    if (!(r->flags & 0x800))
    {
        ((_xRay3*)r)->max_t = 1.0f;
    }

    t_in = -1e19f;
    t_out = 1e19f;

    var_14.x = b->upper.x - b->lower.x;
    var_14.y = b->upper.y - b->lower.y;
    var_14.z = b->upper.z - b->lower.z;

    var_20.x = b->lower.x + b->upper.x;
    var_20.y = b->lower.y + b->upper.y;

    var_14.x *= 0.5f;
    var_14.y *= 0.5f;
    var_14.z *= 0.5f;

    var_20.z = b->lower.z + b->upper.z;

    var_20.x *= -0.5f;
    var_20.y *= -0.5f;
    var_20.z *= -0.5f;

    var_20.x += r->origin.x;
    var_20.y += r->origin.y;
    var_20.z += r->origin.z;

    if (ClipBox(&var_14, &var_20, &r->dir, &t_in, &t_out))
    {
        if (t_in < r->min_t)
        {
            if (t_out < r->min_t)
            {
                isx->dist = t_out;
                isx->penned = 1.0f;
                isx->contained = 1.0f;
            }
            else
            {
                isx->dist = t_out;
                isx->penned = -1.0f;
                isx->contained = -1.0f;
            }
        }
        else
        {
            if (t_in <= r->max_t)
            {
                isx->dist = t_in;
                isx->penned = -1.0f;
                isx->contained = 1.0f;
            }
            else
            {
                isx->dist = t_in;
                isx->penned = 1.0f;
                isx->contained = 1.0f;
            }
        }
    }
    else
    {
        isx->penned = 1.0f;
        isx->contained = 1.0f;
    }
}

// TODO: 96.6%. The original keeps ycode/zcode in the register that already holds
// the divisor 3 (no separate li for the "3" case). The .sdata2 pool also starts
// with -1.0f before 1.0f in the original.
void iBoxIsectSphere(const _xBox* box, const _xSphere* p, _xIsect* isx)
{
    unsigned int xcode, ycode, zcode;

    {
        float lo = p->center.x - p->r;
        float hi = p->center.x + p->r;
        if (lo < box->lower.x)
        {
            if (hi < box->lower.x)
            {
                xcode = 2;
            }
            else if (hi > box->upper.x)
            {
                xcode = 0;
            }
            else
            {
                xcode = 1;
            }
        }
        else
        {
            if (lo > box->upper.x)
            {
                xcode = 5;
            }
            else if (hi > box->upper.x)
            {
                xcode = 4;
            }
            else
            {
                xcode = 3;
            }
        }
    }
    if (xcode % 3 == 2)
    {
        isx->penned = 1.0f;
        return;
    }

    {
        float lo = p->center.y - p->r;
        float hi = p->center.y + p->r;
        if (lo < box->lower.y)
        {
            if (hi < box->lower.y)
            {
                ycode = 2;
            }
            else if (hi > box->upper.y)
            {
                ycode = 0;
            }
            else
            {
                ycode = 1;
            }
        }
        else
        {
            if (lo > box->upper.y)
            {
                ycode = 5;
            }
            else if (hi > box->upper.y)
            {
                ycode = 4;
            }
            else
            {
                ycode = 3;
            }
        }
    }
    if (ycode % 3 == 2)
    {
        isx->penned = 1.0f;
        return;
    }

    {
        float lo = p->center.z - p->r;
        float hi = p->center.z + p->r;
        if (lo < box->lower.z)
        {
            if (hi < box->lower.z)
            {
                zcode = 2;
            }
            else if (hi > box->upper.z)
            {
                zcode = 0;
            }
            else
            {
                zcode = 1;
            }
        }
        else
        {
            if (lo > box->upper.z)
            {
                zcode = 5;
            }
            else if (hi > box->upper.z)
            {
                zcode = 4;
            }
            else
            {
                zcode = 3;
            }
        }
    }
    if (zcode % 3 == 2)
    {
        isx->penned = 1.0f;
        return;
    }

    iBoxIsectVec(box, &p->center, isx);

    if (isx->penned < 0.0f)
    {
        _xVec3 var_28;

        xVec3Add(&var_28, &box->lower, &box->upper);
        xVec3SMulBy(&var_28, 0.5f);
        xVec3Sub(&isx->norm, &p->center, &var_28);

        isx->dist = xVec3Length(&isx->norm);
        isx->contained = -1.0f;
    }
    else
    {
        isx->flags = xcode | (ycode << 4) | (zcode << 8);

        iBoxVecDist(box, &p->center, isx);

        isx->penned = isx->dist - p->r;
        isx->contained = 1.0f;
    }
}

void iBoxInitBoundVec(_xBox* b, const _xVec3* v)
{
    xVec3Copy(&b->lower, v);
    xVec3Copy(&b->upper, v);
}

void iBoxBoundVec(_xBox* o, const _xBox* b, const _xVec3* v)
{
    xVec3Init(&o->lower, MIN(v->x, b->lower.x), MIN(v->y, b->lower.y), MIN(v->z, b->lower.z));
    xVec3Init(&o->upper, MAX(v->x, b->upper.x), MAX(v->y, b->upper.y), MAX(v->z, b->upper.z));
}

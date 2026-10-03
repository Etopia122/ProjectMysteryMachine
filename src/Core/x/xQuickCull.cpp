#include "xQuickCull.h"
#include "xBound.h"
#include "xMath3.h"

extern "C" double floor(double);

void iSphereInitBoundOBB(_xSphere* o, const _xBox* b, const _xMat4x3* m);

static float world_xmin;
static float world_ymin;
static float world_zmin;
static float world_xmax;
static float world_ymax;
static float world_zmax;
static float world_xsz;
static float world_ysz;
static float world_zsz;
static float block_xsz;
static float block_ysz;
static float block_zsz;
static float one_block_xsz;
static float one_block_ysz;
static float one_block_zsz;
static float lcl_world_xsz;
static float lcl_world_ysz;
static float lcl_world_zsz;
static float ucl_world_xsz;
static float ucl_world_ysz;
static float ucl_world_zsz;

inline float xfloor(float x)
{
    return floor(x);
}

#define QC_MIN(a, b) ((a) < (b) ? (a) : (b))
#define QC_MAX(a, b) ((a) > (b) ? (a) : (b))

// Functions are in reverse DOL order (-inline deferred emits last to first).

inline void xQuickCullInit(float xmin, float ymin, float zmin, float xmax, float ymax, float zmax)
{
    world_xmin = xmin;
    world_ymin = ymin;
    world_zmin = zmin;
    world_xmax = xmax;
    world_ymax = ymax;
    world_zmax = zmax;
    world_xsz = xmax - xmin;
    world_ysz = ymax - ymin;
    world_zsz = zmax - zmin;

    if (world_xsz <= 0.0f || world_ysz <= 0.0f || world_zsz <= 0.0f)
    {
        world_zsz = 1.0f;
        world_ysz = 1.0f;
        world_xsz = 1.0f;
    }

    lcl_world_xsz = xmin - world_xsz;
    lcl_world_ysz = ymin - world_ysz;
    lcl_world_zsz = zmin - world_zsz;
    block_xsz = world_xsz / 30.0f;
    block_ysz = world_ysz / 30.0f;
    block_zsz = world_zsz / 30.0f;
    one_block_xsz = 1.0f / block_xsz;
    one_block_ysz = 1.0f / block_ysz;
    one_block_zsz = 1.0f / block_zsz;
    ucl_world_xsz = xmax + world_xsz;
    ucl_world_ysz = ymax + world_ysz;
    ucl_world_zsz = zmax + world_zsz;
}

void xQuickCullInit(const _xBox* box)
{
    xQuickCullInit(box->lower.x, box->lower.y, box->lower.z, box->upper.x, box->upper.y,
                   box->upper.z);
}

signed int xQuickCullIsects(const xQCData* a, const xQCData* b)
{
    return (a->xmask & b->xmask) | (a->ymask & b->ymask) | (a->zmask & b->zmask);
}

static inline unsigned int xQuickCullCell(signed short i)
{
    if (i < 0)
    {
        return 0;
    }
    if (i > 29)
    {
        return 31;
    }
    return i + 1;
}

static inline void xQuickCullCellForVec(xQCData* c, const _xVec3* v)
{
    float x = QC_MAX(lcl_world_xsz, QC_MIN(v->x, ucl_world_xsz));
    float y = QC_MAX(lcl_world_ysz, QC_MIN(v->y, ucl_world_ysz));
    float z = QC_MAX(lcl_world_zsz, QC_MIN(v->z, ucl_world_zsz));
    signed short ix = xfloor((x - world_xmin) / block_xsz);
    signed short iy = xfloor((y - world_ymin) / block_ysz);
    signed short iz = xfloor((z - world_zmin) / block_zsz);
    c->xmask = xQuickCullCell(ix);
    c->ymask = xQuickCullCell(iy);
    c->zmask = xQuickCullCell(iz);
}

static inline void xQuickCullCellMerge(xQCData* o, const xQCData* a, const xQCData* b)
{
    signed short xlo = QC_MIN(a->xmask, b->xmask);
    signed short ylo = QC_MIN(a->ymask, b->ymask);
    signed short zlo = QC_MIN(a->zmask, b->zmask);
    signed short xhi = QC_MAX(a->xmask, b->xmask);
    signed short yhi = QC_MAX(a->ymask, b->ymask);
    signed short zhi = QC_MAX(a->zmask, b->zmask);

    if (xhi == 31 && xlo == 0)
    {
        o->xmask = 0xFFFFFFFF;
    }
    else
    {
        o->xmask = ((1 << (xhi - xlo + 1)) - 1) << xlo;
    }
    if (yhi == 31 && ylo == 0)
    {
        o->ymask = 0xFFFFFFFF;
    }
    else
    {
        o->ymask = ((1 << (yhi - ylo + 1)) - 1) << ylo;
    }
    if (zhi == 31 && zlo == 0)
    {
        o->zmask = 0xFFFFFFFF;
    }
    else
    {
        o->zmask = ((1 << (zhi - zlo + 1)) - 1) << zlo;
    }
}

static void xQuickCullForSphere(xQCData* q, const _xSphere* s);
static void xQuickCullForBox(xQCData* q, const _xBox* box);
static void xQuickCullForOBB(xQCData* q, const _xBox* b, const _xMat4x3* m);

void xQuickCullForBound(xQCData* q, _xBound* b)
{
    if (b->type == 1)
    {
        xQuickCullForSphere(q, &b->sph);
    }
    else if (b->type == 2)
    {
        xQuickCullForBox(q, &b->box.box);
    }
    else if (b->type == 4)
    {
        xQuickCullForOBB(q, &b->box.box, b->mat);
    }
}

void xQuickCullForRay(xQCData* q, _xRay3* r)
{
    _xVec3 p1;
    _xVec3 p2;
    xQCData a;
    xQCData b;

    if (r->flags & 0x400)
    {
        _xVec3 delta;
        xVec3SMul(&delta, &r->dir, r->min_t);
        p1.x = r->origin.x + delta.x;
        p1.y = r->origin.y + delta.y;
        p1.z = r->origin.z + delta.z;
    }
    else
    {
        p1.x = r->origin.x;
        p1.y = r->origin.y;
        p1.z = r->origin.z;
    }

    if (r->flags & 0x800)
    {
        float len;
        if (r->flags & 0x400)
        {
            len = r->max_t - r->min_t;
        }
        else
        {
            len = r->max_t;
        }
        p2.x = r->dir.x * len;
        p2.y = r->dir.y * len;
        p2.z = r->dir.z * len;
    }
    else
    {
        p2.x = r->dir.x;
        p2.y = r->dir.y;
        p2.z = r->dir.z;
    }

    p2.x = p1.x + p2.x;
    p2.y = p1.y + p2.y;
    p2.z = p1.z + p2.z;

    xQuickCullCellForVec(&a, &p1);
    xQuickCullCellForVec(&b, &p2);
    xQuickCullCellMerge(q, &a, &b);
}

// TODO: 99.8% - the center adds have their operands swapped (target: center + r);
// writing them center-first changes the register numbering instead.
static void xQuickCullForSphere(xQCData* q, const _xSphere* s)
{
    _xVec3 v1;
    _xVec3 v2;
    xQCData a;
    xQCData b;
    v1.x = -s->r;
    v1.y = -s->r;
    v1.z = -s->r;
    v2.x = s->r;
    v2.y = s->r;
    v2.z = s->r;
    v1.x += s->center.x;
    v1.y += s->center.y;
    v1.z += s->center.z;
    v2.x += s->center.x;
    v2.y += s->center.y;
    v2.z += s->center.z;

    xQuickCullCellForVec(&a, &v1);
    xQuickCullCellForVec(&b, &v2);
    xQuickCullCellMerge(q, &a, &b);
}

static void xQuickCullForBox(xQCData* q, const _xBox* box)
{
    xQCData a;
    xQCData b;
    xQuickCullCellForVec(&a, &box->lower);
    xQuickCullCellForVec(&b, &box->upper);
    xQuickCullCellMerge(q, &a, &b);
}

static void xQuickCullForOBB(xQCData* q, const _xBox* b, const _xMat4x3* m)
{
    _xSphere s;
    iSphereInitBoundOBB(&s, b, m);
    xQuickCullForSphere(q, &s);
}

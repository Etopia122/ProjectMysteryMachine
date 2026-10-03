#include "zGlobals.h"
#include "zScene.h"
#include "xMemMgr.h"
#include "xString.h"
#include "xstransvc.h"
#include "zParEmitter.h"

extern "C" {
char* strcpy(char* dst, const char* src);
char* strcat(char* dst, const char* src);
}


enum _tagLightningType
{
    eLightningLine,
    eLightningRotating,
    eLightningForce32 = 0x7FFFFFFF
};

class _tagLightningAdd
{
public:
    _tagLightningType type;  // 0x0
    float setup_degrees;     // 0x4
    float move_degrees;      // 0x8
    float rot_radius;        // 0xC
    _xVec3* start;           // 0x10
    _xVec3* end;             // 0x14
    signed int total_points; // 0x18
    float time;              // 0x1C
    float arc_height;        // 0x20
    float thickness;         // 0x24
    unsigned char r;         // 0x28
    unsigned char g;         // 0x29
    unsigned char b;         // 0x2A
    unsigned char alpha_pad; // 0x2B
    float rand_radius;       // 0x2C
    unsigned int flags;      // 0x30
};

class _tagLightningRot
{
public:
    float deg[16]; // 0x0
    float degrees; // 0x40
    float height;  // 0x44
};

class _tagLightning
{
public:
    _tagLightningType type;  // 0x0
    unsigned int flags;      // 0x4
    _xVec3 base_point[16];   // 0x8
    _xVec3 point[16];        // 0xC8
    signed int total_points; // 0x188
    float arc_height;        // 0x18C
    float thickness;         // 0x190
    _tagLightningRot rot;    // 0x194
    unsigned char r;         // 0x1DC
    unsigned char g;         // 0x1DD
    unsigned char b;         // 0x1DE
    unsigned char alpha_pad; // 0x1DF
    float rand_radius;       // 0x1E0
    float time_left;         // 0x1E4
    float time_total;        // 0x1E8
};

class RwIm3DVertex
{
public:
    float x;                // 0x0
    float y;                // 0x4
    float z;                // 0x8
    float nx;               // 0xC
    float ny;               // 0x10
    float nz;               // 0x14
    unsigned char color[4]; // 0x18
    float u;                // 0x1C
    float v;                // 0x20
};

extern "C" {
signed int RwRenderStateSet(signed int state, void* value);
void* RwIm3DTransform(RwIm3DVertex* pVerts, unsigned int numVerts, void* ltm, unsigned int flags);
signed int RwIm3DRenderPrimitive(signed int primType);
signed int RwIm3DEnd();
}

unsigned int irand();
float iurand();
void xVec3Normalize(_xVec3* o, _xVec3* v);
void xMat3x3LookVec(_xMat3x3* m, _xVec3* at);
void xMat3x3GetEuler(_xMat3x3* m, _xVec3* a);
void xMat3x3Euler(_xMat3x3* m, float yaw, float pitch, float roll);
void xMat3x3LMulVec(_xVec3* o, _xMat3x3* m, _xVec3* v);

static _tagLightning* sLightning[48];
static RwIm3DVertex sLightningVert[128];

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zLightningInit()
{
    for (signed int i = 0; i < 48; i++)
    {
        sLightning[i] = 0;
    }
}

static inline _tagLightning* FindFreeLightning()
{
    for (signed int i = 0; i < 48; i++)
    {
        if (sLightning[i])
        {
            if (!(sLightning[i]->flags & 1))
            {
                return sLightning[i];
            }
        }
        else
        {
            sLightning[i] = (_tagLightning*)xMemAlloc(gActiveHeap, sizeof(_tagLightning));
            return sLightning[i];
        }
    }
    return 0;
}

// TODO (99.9%): add->end is loaded before add->start in the original.
_tagLightning* zLightningAdd(_tagLightningAdd* add)
{
    _tagLightning* l = FindFreeLightning();
    float currot = 0.0f;

    l->type = add->type;
    l->flags = add->flags | 0x41;
    l->total_points = add->total_points;
    l->arc_height = add->arc_height;
    l->thickness = 0.5f * add->thickness;
    l->rand_radius = add->rand_radius;
    l->r = add->r;
    l->g = add->g;
    l->b = add->b;

    switch (l->type)
    {
    case eLightningLine:
        break;
    case eLightningRotating:
        l->rot.degrees = add->move_degrees;
        l->rot.height = add->rot_radius;
        break;
    }

    for (signed int i = 0; i < l->total_points; i++)
    {
        float t = (float)i / ((float)l->total_points - 1.0f);
        float x = t * (add->end->x - add->start->x) + add->start->x;
        float y = t * (add->end->y - add->start->y) + add->start->y;
        float z = t * (add->end->z - add->start->z) + add->start->z;
        l->point[i].x = x;
        l->point[i].y = y;
        l->point[i].z = z;
        float scalar = 4.0f * t + -4.0f * (t * t);
        if (scalar < 0.0f)
        {
            scalar = 0.0f;
        }

        switch (l->type)
        {
        case eLightningLine:
            break;
        case eLightningRotating:
            l->rot.deg[i] = currot;
            currot += add->setup_degrees;
            break;
        }

        if (l->flags & 0x20)
        {
            l->point[i].y = scalar * l->arc_height + l->point[i].y;
        }
        l->base_point[i] = l->point[i];
    }

    if (l->flags & 0x10)
    {
        l->time_left = l->time_total = 0.1f;
    }
    return l;
}

// TODO (95.6%): the original loads the spark colours one at a time (index
// c * 4 + k off the rodata base) instead of hoisting them.
static void UpdateLightning(_tagLightning* l, float dt)
{
    static const unsigned char cSparkColors[6][4] = {
        { 0xFF, 0x00, 0x00, 0xFF }, { 0x80, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0xFF, 0xFF },
        { 0x00, 0x00, 0x80, 0xFF }, { 0xFF, 0xFF, 0x00, 0xFF }, { 0x80, 0x80, 0x00, 0xFF },
    };
    if (!(l->flags & 0x10))
    {
        l->time_left -= dt;
    }
    if (l->time_left <= 0.0f)
    {
        l->flags &= ~1;
    }

    if (l->type == eLightningLine)
    {
        float full = l->rand_radius * dt;
        float half = -(0.5f * full);
        for (signed int i = 1; i < l->total_points - 1; i++)
        {
            l->point[i].x = l->base_point[i].x + (full * iurand() + half);
            l->point[i].y = l->base_point[i].y + (full * iurand() + half);
            l->point[i].z = l->base_point[i].z + (full * iurand() + half);
            if (l->flags & 0x20)
            {
                float t = (float)i / (float)l->total_points;
                l->point[i].y = (4.0f * t + -4.0f * (t * t)) * l->arc_height + l->point[i].y;
            }
        }
    }
    else if (l->type == eLightningRotating)
    {
        _xVec3 dir;
        _xMat3x3 lookmat;
        _xVec3 euler;

        xVec3Sub(&dir, &l->base_point[l->total_points - 1], &l->base_point[0]);
        xVec3Normalize(&dir, &dir);
        xMat3x3LookVec(&lookmat, &dir);
        xMat3x3GetEuler(&lookmat, &euler);

        float full = l->rand_radius * dt;
        float half = -(0.5f * full);
        for (signed int i = 1; i < l->total_points - 1; i++)
        {
            _xMat3x3 rotmat;
            xMat3x3Euler(&rotmat, euler.x, 0.0f, 3.1415927f * l->rot.deg[i] / 180.0f);

            _xVec3 offset = { 0.0f, 1.0f, 0.0f };
            float scalar = 1.0f;
            // Original bug: always true (& binds tighter than |).
            if (l->flags & 8 | 0x20)
            {
                float t = (float)i / (float)l->total_points;
                scalar = 4.0f * t + -4.0f * (t * t);
                if (l->flags & 8)
                {
                    offset.y = l->rot.height * scalar;
                }
            }
            xMat3x3LMulVec(&offset, &rotmat, &offset);

            l->rot.deg[i] = l->rot.degrees * dt + l->rot.deg[i];
            l->point[i].x = l->base_point[i].x + (full * iurand() + half);
            l->point[i].y = l->base_point[i].y + (full * iurand() + half);
            l->point[i].z = l->base_point[i].z + (full * iurand() + half);
            l->point[i].x += offset.x;
            l->point[i].y += offset.y;
            l->point[i].z += offset.z;
            if (l->flags & 0x20)
            {
                l->point[i].y = scalar * l->arc_height + l->point[i].y;
            }
        }
    }

    if ((l->flags & 2) && (irand() & 3))
    {
        _zParEmitter* emitter = (_zParEmitter*)zSceneFindObject(xStrHash("PAREMIT_EG_SPARK"));
        if (emitter)
        {
            _zParEmitterCustomSettings info;
            info.custom_flags = 0x1A0;
            signed int p = irand() % l->total_points;
            info.pos.x = l->point[p].x;
            info.pos.y = l->point[p].y;
            info.pos.z = l->point[p].z;
            signed int c = (signed int)irand() % 6;
            info.color_birth[0] = cSparkColors[c][0];
            info.color_death[0] = cSparkColors[c][0];
            info.color_birth[1] = cSparkColors[c][1];
            info.color_death[1] = cSparkColors[c][1];
            info.color_birth[2] = cSparkColors[c][2];
            info.color_death[2] = cSparkColors[c][2];
            info.color_birth[3] = cSparkColors[c][3];
            info.color_death[3] = 0x80;
            zParEmitterEmitCustom(emitter, dt, &info);
        }
    }
}

void zLightningUpdate(float dt)
{
    for (signed int i = 0; i < 48; i++)
    {
        if (sLightning[i] && (sLightning[i]->flags & 1))
        {
            UpdateLightning(sLightning[i], dt);
        }
    }
}

static inline void LightningVertPos(RwIm3DVertex* v, float x, float y, float z)
{
    v->x = x;
    v->y = y;
    v->z = z;
}

static inline void LightningVertColor(RwIm3DVertex* v, unsigned char r, unsigned char g,
                                      unsigned char b, unsigned char a)
{
    v->color[0] = r;
    v->color[1] = g;
    v->color[2] = b;
    v->color[3] = a;
}

#define RENDER_LIGHTNING_STRIP(l, alpha) \
    { \
        unsigned int n = 2; \
        LightningVertPos(&sLightningVert[0], l->point[0].x, l->point[0].y + l->thickness, l->point[0].z); \
        sLightningVert[0].u = 0.0f; \
        sLightningVert[0].v = 0.0f; \
        LightningVertColor(&sLightningVert[0], l->r, l->g, l->b, alpha); \
        LightningVertPos(&sLightningVert[1], l->point[0].x, l->point[0].y - l->thickness, l->point[0].z); \
        sLightningVert[1].u = 0.0f; \
        sLightningVert[1].v = 1.0f; \
        LightningVertColor(&sLightningVert[1], l->r, l->g, l->b, alpha); \
        for (signed int i = 1; i < l->total_points; i++) \
        { \
            RwIm3DVertex* v = &sLightningVert[n]; \
            LightningVertPos(v, l->point[i].x, l->point[i].y + l->thickness, l->point[i].z); \
            if (i & 1) \
            { \
                v->u = 1.0f; \
            } \
            else \
            { \
                v->u = 0.0f; \
            } \
            v->v = 0.0f; \
            LightningVertColor(v, l->r, l->g, l->b, alpha); \
            v = &sLightningVert[n + 1]; \
            LightningVertPos(v, l->point[i].x, l->point[i].y - l->thickness, l->point[i].z); \
            if (i & 1) \
            { \
                v->u = 1.0f; \
            } \
            else \
            { \
                v->u = 0.0f; \
            } \
            v->v = 1.0f; \
            LightningVertColor(v, l->r, l->g, l->b, alpha); \
            n += 2; \
            if (n >= 128) \
            { \
                n = 128; \
                break; \
            } \
        } \
        if (RwIm3DTransform(sLightningVert, n, 0, 0x19)) \
        { \
            RwIm3DRenderPrimitive(4); \
            RwIm3DEnd(); \
        } \
    }

// TODO (98.0%): the colour bytes load g, b, r in the original, and it keeps two
// more saved registers.
static void RenderLightning(_tagLightning* l)
{
    unsigned char alpha = l->time_left / l->time_total * 255.0f;
    RENDER_LIGHTNING_STRIP(l, alpha);

    float thickness = l->thickness;
    alpha = l->time_left / l->time_total * 128.0f;
    l->thickness *= 3.0f + iurand();
    RENDER_LIGHTNING_STRIP(l, alpha);
    l->thickness = thickness;
}

// Dead-stripped by the linker: only its strings survive in .rodata. The
// original function is unknown.
void zLightningDebugText(char* buf)
{
    strcpy(buf, "X to test lightning\n");
    strcat(buf, "               ");
}

void zLightningRender()
{
    unsigned int* tex = (unsigned int*)xSTFindAsset(xStrHash("LIGHTNING"), 0);
    if (tex)
    {
        RwRenderStateSet(1, (void*)*tex);
    }

    for (signed int i = 0; i < 48; i++)
    {
        if (sLightning[i] && (sLightning[i]->flags & 0x41) == 0x41)
        {
            RenderLightning(sLightning[i]);
        }
    }
}

void zLightningKill(_tagLightning* l)
{
    l->flags &= ~1;
}

void zLightningModifyEndpoints(_tagLightning* l, _xVec3* start, _xVec3* end)
{
    signed int i;
    for (i = 0; i < l->total_points; i++)
    {
        float t = (float)i / ((float)l->total_points - 1.0f);
        float x = t * (end->x - start->x) + start->x;
        float y = t * (end->y - start->y) + start->y;
        float z = t * (end->z - start->z) + start->z;
        l->base_point[i].x = x;
        l->base_point[i].y = y;
        l->base_point[i].z = z;
        float scalar = 4.0f * t + -4.0f * (t * t);
        if (scalar < 0.0f)
        {
            scalar = 0.0f;
        }
        if (l->flags & 0x20)
        {
            l->base_point[i].y = scalar * l->arc_height + l->base_point[i].y;
        }
    }
    l->point[l->total_points - 1] = l->base_point[l->total_points - 1];
}

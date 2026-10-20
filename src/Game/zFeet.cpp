#include "xMath.h"
#include "xMath3.h"
#include "xEnt.h"
#include "xAnim.h"
#include "xSnd.h"
#include "xMemMgr.h"
#include "xString.h"
#include "xstransvc.h"
#include "iModel.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zSurface.h"
#include "zParEmitter.h"
#include "rwtypes.h"

extern "C" {
char* strcpy(char* dst, const char* src);
char* strcat(char* dst, const char* src);
void* memcpy(void* dst, const void* src, unsigned int n);
}

class xAnimActiveEffect;
class _xEnv;
class RwRaster;

typedef unsigned int (*xAnimEffectCallback)(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*);

class xAnimEffect
{
public:
    xAnimEffect* Next;           // 0x0
    unsigned int Flags;          // 0x4
    float StartTime;             // 0x8
    float EndTime;               // 0xC
    xAnimEffectCallback Callback; // 0x10
    unsigned char data[8];       // 0x14
};

class xAnimActiveEffect
{
public:
    xAnimEffect* Effect;
    unsigned int Handle;
};

class tagzFeet
{
public:
    signed int total;         // 0x0
    _xEnt* owner;             // 0x4
    signed short entity_type; // 0x8
    signed short pitch;       // 0xA
};

class _tagFoot
{
public:
    tagzFeet* feet;
    signed int idx;
};

class _tagSkidEdge
{
public:
    _xVec3 centre;  // 0x0
    _xVec3 local_x; // 0xC
    float width;    // 0x18
};

class _tagFeetSkidMark
{
public:
    signed int total_feet;   // 0x0
    _tagSkidEdge* edge[4];   // 0x4
    signed int first_idx;    // 0x14
    signed int total_edges;  // 0x18
    float sample_timer;      // 0x1C
    float remove_timer;      // 0x20
};

class _xEntCollis
{
public:
    unsigned char chk;
    unsigned char pen;
    unsigned char env_sidx;
    unsigned char env_eidx;
    unsigned char npc_sidx;
    unsigned char npc_eidx;
    unsigned char dyn_sidx;
    unsigned char dyn_eidx;
    unsigned char stat_sidx;
    unsigned char stat_eidx;
    unsigned char trig_idx;
    unsigned char idx;
    _xCollis colls[18]; // 0xC
};

class RxObjSpace3DVertex
{
public:
    RwV3d objVertex;          // 0x0
    RwV3d objNormal;          // 0xC
    unsigned char color[4];   // 0x18
    float u;                  // 0x1C
    float v;                  // 0x20
};

class zFeetNPC
{
public:
    unsigned char pad[0x1B4];
    iModelTag tag[2];
};

class _tagPlayerFX
{
public:
    _zParEmitter* emit_land[23];
};

extern "C" {
signed int RwRenderStateSet(signed int state, void* value);
void* RwIm3DTransform(RxObjSpace3DVertex* pVerts, unsigned int numVerts, RwMatrixTag* ltm,
                      unsigned int flags);
signed int RwIm3DRenderPrimitive(signed int primType);
signed int RwIm3DEnd();
signed int zPitchChange(signed int angle, float* offset, signed int range, float max);
}

xAnimState* xAnimTableGetState(xAnimTable* table, char* name);
void xAnimTableRemoveEffectByCB(xAnimTable* table, xAnimEffectCallback callback);
xAnimEffect* xAnimStateNewEffect(xAnimState* state, unsigned int flags, float startTime,
                                 float endTime, xAnimEffectCallback callback,
                                 unsigned int userDataSize);
float xVec3Normalize(_xVec3* o, _xVec3* v);
unsigned int xRayHitsEnvFast(_xRay3* r, _xEnv* env);
signed int zMenuRunning();
xSurface* zSurfaceGetSurface(_xCollis* coll);
void zSurfaceGetName(signed int type, char* buffer);
void xDebugModeAdd(char* mode, void (*func)());

extern unsigned int gActiveHeap;
extern _xEnv* gCurXEnv;
extern _tagPlayerFX gPlayerfx;

static unsigned int sSurfaceSoundIDStep[23];
static xBase* paremit_sd_pawprint;
static xBase* paremit_vil_footprint;
static RwRaster* sSkidMarkRaster;
static const float cPlayerSkidWidth[4] = { 0.075f, 0.075f, 0.1f, 0.1f };

static unsigned int zFeetStepVillainCB(unsigned int, xAnimActiveEffect* acteffect, xAnimSingle*,
                                       void*);
static unsigned int zFeetStepCB(unsigned int, xAnimActiveEffect* acteffect, xAnimSingle*, void*);
static void zFeetDebugCB();

static inline void zFeetAddEffects(xAnimTable* table, char* name, tagzFeet* feet,
                                   signed int* tag_idx, float* step_time,
                                   xAnimEffectCallback callback)
{
    xAnimState* state = xAnimTableGetState(table, name);
    for (signed int i = 0; i < feet->total; i++)
    {
        _tagFoot foot;
        foot.feet = feet;
        foot.idx = tag_idx[i];
        memcpy(xAnimStateNewEffect(state, 1, step_time[i], 0.0f, callback, sizeof(_tagFoot))->data,
               &foot, sizeof(_tagFoot));
    }
}

static inline signed int zFeetCanLeaveMark(_xEnt* ent)
{
    if (ent->collis->colls != 0 && ent->collis->colls[0].optr != 0)
    {
        return 0;
    }
    if (globals.player.JumpState != 0)
    {
        return 0;
    }
    return 1;
}

static inline signed int zFeetSurfaceType(_xEnt* ent)
{
    signed int surface_type = 0;
    xSurface* xsurf = zSurfaceGetSurface(&ent->collis->colls[0]);
    if (xsurf != 0 && xsurf->state == 0)
    {
        zSurfaceProps* surface = (zSurfaceProps*)xsurf->moprops;
        if (surface != 0 && surface->asset != 0)
        {
            surface_type = surface->asset->surf_type;
        }
    }
    return surface_type;
}

void zFeetGetIDs()
{
    char type_name[128];
    char name[128];
    signed int i;

    for (i = 0; i < 23; i++)
    {
        zSurfaceGetName(i, type_name);
        strcpy(name, "SNDFX_STEP_");
        strcat(name, type_name);
        sSurfaceSoundIDStep[i] = xStrHash(name);
    }

    paremit_sd_pawprint = zSceneFindObject(xStrHash("PAREMIT_SD_PAWPRINT"));
    paremit_vil_footprint = zSceneFindObject(xStrHash("PAREMIT_VIL_FOOTPRINT"));

    RwRaster** tex = (RwRaster**)xSTFindAsset(xStrHash("SKIDMARKTEX"), 0);
    if (tex != 0)
    {
        sSkidMarkRaster = *tex;
    }

    xDebugModeAdd("Feet", zFeetDebugCB);
}

void zFeetExit(_xEnt*, xAnimTable* table)
{
    xAnimTableRemoveEffectByCB(table, zFeetStepCB);
}

void zFeetInit(signed int ent_type, _xEnt* owner, tagzFeet* feet, signed int feet_subtype,
               xAnimTable* table)
{
    float step_time[6];
    signed int tag_idx[6];
    signed int i;

    feet->owner = owner;
    feet->entity_type = ent_type;

    if (feet->entity_type == -1)
    {
        for (i = 0; i < 6; i++)
        {
            tag_idx[i] = i;
        }
        feet->total = 4;

        switch (feet_subtype)
        {
        case 0:
            step_time[0] = 0.69825f;
            step_time[1] = 0.03325f;
            step_time[2] = 1.33f;
            step_time[3] = 0.56525f;
            zFeetAddEffects(table, "Walk_brave", feet, tag_idx, step_time, zFeetStepCB);
            zFeetAddEffects(table, "Walk_normal", feet, tag_idx, step_time, zFeetStepCB);
            zFeetAddEffects(table, "Walk_scared", feet, tag_idx, step_time, zFeetStepCB);
            zFeetAddEffects(table, "Walk_boots", feet, tag_idx, step_time, zFeetStepCB);
            break;
        case 1:
            step_time[0] = 0.17434211f;
            step_time[1] = 0.12453008f;
            step_time[2] = 0.32377818f;
            step_time[3] = 0.32377818f;
            zFeetAddEffects(table, "Run_brave", feet, tag_idx, step_time, zFeetStepCB);
            zFeetAddEffects(table, "Run_normal", feet, tag_idx, step_time, zFeetStepCB);
            zFeetAddEffects(table, "Run_scared", feet, tag_idx, step_time, zFeetStepCB);
            zFeetAddEffects(table, "Run_boots", feet, tag_idx, step_time, zFeetStepCB);
            break;
        case 2:
            feet->total = 2;
            step_time[0] = 0.5657377f;
            step_time[1] = 1.5973771f;
            tag_idx[0] = 2;
            tag_idx[1] = 3;
            zFeetAddEffects(table, "Walk_sneak", feet, tag_idx, step_time, zFeetStepCB);
            break;
        case 3:
            step_time[0] = 0.349125f;
            step_time[1] = 0.016625f;
            step_time[2] = 0.665f;
            step_time[3] = 0.282625f;
            zFeetAddEffects(table, "Walk_sticky", feet, tag_idx, step_time, zFeetStepCB);
            break;
        case 4:
            step_time[0] = 0.12531328f;
            step_time[1] = 0.12531328f;
            step_time[2] = 0.25062656f;
            step_time[3] = 0.25062656f;
            zFeetAddEffects(table, "Action_attackrun", feet, tag_idx, step_time, zFeetStepCB);
            break;
        case 5:
            step_time[0] = 0.29333332f;
            step_time[1] = 0.26666668f;
            step_time[2] = 0.026666667f;
            step_time[3] = 0.08f;
            zFeetAddEffects(table, "Land_normal", feet, tag_idx, step_time, zFeetStepCB);
            break;
        case 6:
            step_time[0] = 0.02505f;
            step_time[1] = 0.00835f;
            step_time[2] = 0.32565f;
            step_time[3] = 0.2505f;
            zFeetAddEffects(table, "Land_running", feet, tag_idx, step_time, zFeetStepCB);
            break;
        case 7:
            step_time[0] = 0.067058824f;
            step_time[1] = 0.13411765f;
            step_time[2] = 0.38f;
            step_time[3] = 0.38f;
            zFeetAddEffects(table, "Land_walk", feet, tag_idx, step_time, zFeetStepCB);
            break;
        case 8:
            step_time[0] = 0.100250624f;
            step_time[1] = 0.3508772f;
            step_time[2] = 0.5012531f;
            step_time[3] = 0.7017544f;
            zFeetAddEffects(table, "Walk_slippery", feet, tag_idx, step_time, zFeetStepCB);
            break;
        case 9:
            step_time[0] = 0.06f;
            step_time[1] = 0.12f;
            step_time[2] = 0.18f;
            step_time[3] = 0.24f;
            zFeetAddEffects(table, "Run_slippery", feet, tag_idx, step_time, zFeetStepCB);
            break;
        }
    }
    else
    {
        feet->total = 2;
        for (i = 0; i < 6; i++)
        {
            tag_idx[i] = i;
        }

        switch (ent_type)
        {
        case 0: // the original switch covers types 0-48
            break;
        case 3:
            step_time[0] = 0.31935483f;
            step_time[1] = 0.07096774f;
            zFeetAddEffects(table, "Move", feet, tag_idx, step_time, zFeetStepVillainCB);
            step_time[0] = 0.17741935f;
            step_time[1] = 0.46129033f;
            zFeetAddEffects(table, "Chase", feet, tag_idx, step_time, zFeetStepVillainCB);
            break;
        case 7:
            step_time[0] = 1.0f;
            step_time[1] = 0.516129f;
            zFeetAddEffects(table, "Move", feet, tag_idx, step_time, zFeetStepVillainCB);
            step_time[0] = 0.06923077f;
            step_time[1] = 0.5192308f;
            zFeetAddEffects(table, "Chase", feet, tag_idx, step_time, zFeetStepVillainCB);
            break;
        case 12:
            step_time[0] = 1.625f;
            step_time[1] = 0.75f;
            zFeetAddEffects(table, "Move", feet, tag_idx, step_time, zFeetStepVillainCB);
            step_time[0] = 0.46875f;
            step_time[1] = 0.25f;
            zFeetAddEffects(table, "Chase", feet, tag_idx, step_time, zFeetStepVillainCB);
            break;
        case 15:
            step_time[0] = 0.46666667f;
            step_time[1] = 1.1f;
            zFeetAddEffects(table, "Move", feet, tag_idx, step_time, zFeetStepVillainCB);
            step_time[0] = 0.46666667f;
            step_time[1] = 1.1f;
            zFeetAddEffects(table, "Chase", feet, tag_idx, step_time, zFeetStepVillainCB);
            break;
        case 19:
            step_time[0] = 0.945f;
            step_time[1] = 0.21f;
            zFeetAddEffects(table, "Move", feet, tag_idx, step_time, zFeetStepVillainCB);
            step_time[0] = 0.5785714f;
            step_time[1] = 0.9f;
            zFeetAddEffects(table, "Chase", feet, tag_idx, step_time, zFeetStepVillainCB);
            break;
        case 48:
            break;
        }
    }
}

// TODO: 99.8%. feet and collided_with_ground swap r27/r29.
static unsigned int zFeetStepCB(unsigned int, xAnimActiveEffect* acteffect, xAnimSingle*, void*)
{
    _tagFoot* foot;
    tagzFeet* feet;
    _xEnt* ent;
    _zParEmitterCustomSettings info;
    _xRay3 ray;
    signed int collided_with_ground;
    signed int surface_type;
    float pitch;
    xBase* b;
    _xVec3 angles;

    if (zMenuRunning())
    {
        return 0;
    }

    foot = (_tagFoot*)acteffect->Effect->data;
    feet = foot->feet;
    ent = feet->owner;
    if (!xEntIsVisible(ent))
    {
        return 0;
    }

    info.custom_flags = 0x10020;
    iModelTagEval(ent->model->Data, &globals.player.HangPawTag[foot->idx], ent->model->Mat,
                  &info.pos);

    collided_with_ground = 0;
    xVec3Copy(&ray.origin, &info.pos);
    ray.origin.y = 0.05f + ((_xMat4x3*)ent->model->Mat)->pos.y;
    ray.dir.x = 0.0f;
    ray.dir.y = -0.1f;
    ray.dir.z = 0.0f;
    ray.flags = 0;
    if (xRayHitsEnvFast(&ray, gCurXEnv))
    {
        collided_with_ground = 1;
    }

    info.pos.y = 0.05f + ((_xMat4x3*)ent->model->Mat)->pos.y;
    surface_type = zFeetSurfaceType(ent);
    if (surface_type == 22)
    {
        return 0;
    }

    if (sSurfaceSoundIDStep[surface_type] != 0)
    {
        feet->pitch = zPitchChange(feet->pitch, &pitch, 45, 3.0f);
        if (xSndPlay3D(sSurfaceSoundIDStep[surface_type], 0.25f, pitch, 0, 2, &info.pos, 25.0f) ==
            0xFFFFFFFF)
        {
            sSurfaceSoundIDStep[surface_type] = 0;
        }
    }

    if (collided_with_ground && zFeetCanLeaveMark(ent))
    {
        b = paremit_sd_pawprint;
        xMat3x3GetEuler((_xMat3x3*)ent->model->Mat, &angles);
        info.rot[0] = 255.0f * (angles.x / 6.2831855f);
        info.rot[1] = 255.0f * (angles.y / 6.2831855f);
        info.rot[2] = 255.0f * (angles.z / 6.2831855f);
        zParEmitterEmitCustom((_zParEmitter*)b, 0.0f, &info);
        info.custom_flags = 0x20;
        zParEmitterEmitCustom(gPlayerfx.emit_land[surface_type], 1.0f / 30.0f, &info);
    }

    return 0;
}

// TODO: 98.9%. Saved-register allocation differs (effect, feet, ent).
static unsigned int zFeetStepVillainCB(unsigned int, xAnimActiveEffect* acteffect, xAnimSingle*,
                                       void*)
{
    _xVec3 angles;
    xBase* b;
    float pitch;
    signed int surface_type;
    _zParEmitterCustomSettings info;
    _xEnt* ent;
    tagzFeet* feet;
    _tagFoot* foot;

    foot = (_tagFoot*)acteffect->Effect->data;
    feet = foot->feet;
    if (feet->total <= 0)
    {
        return 0;
    }

    ent = feet->owner;
    if (!xBaseIsEnabled(ent) || !xEntIsVisible(ent))
    {
        return 0;
    }

    iModelTagEval(ent->model->Data, &((zFeetNPC*)ent)->tag[foot->idx], ent->model->Mat,
                  &info.pos);

    surface_type = zFeetSurfaceType(ent);
    if (surface_type == 22)
    {
        return 0;
    }

    if (sSurfaceSoundIDStep[surface_type] != 0)
    {
        feet->pitch = zPitchChange(feet->pitch, &pitch, 45, 3.0f);
        if (xSndPlay3D(sSurfaceSoundIDStep[surface_type], 0.25f, pitch, 0, 2, &info.pos, 25.0f) ==
            0xFFFFFFFF)
        {
            sSurfaceSoundIDStep[surface_type] = 0;
        }
    }

    b = paremit_vil_footprint;
    info.custom_flags = 0x10020;
    xMat3x3GetEuler((_xMat3x3*)ent->model->Mat, &angles);
    info.rot[0] = 255.0f * (angles.x / 6.2831855f);
    info.rot[1] = 255.0f * (angles.y / 6.2831855f);
    info.rot[2] = 255.0f * (angles.z / 6.2831855f);
    zParEmitterEmitCustom((_zParEmitter*)b, 0.0f, &info);
    info.custom_flags = 0x20;
    zParEmitterEmitCustom(gPlayerfx.emit_land[surface_type], 1.0f / 30.0f, &info);

    return 0;
}

static void zFeetDebugCB()
{
}

void zFeetInitSkidMark(_tagFeetSkidMark* m, signed int total_feet)
{
    signed int i;

    m->total_feet = total_feet;
    for (i = 0; i < m->total_feet; i++)
    {
        m->edge[i] = (_tagSkidEdge*)xMemAlloc(gActiveHeap, 16 * sizeof(_tagSkidEdge));
    }
    m->first_idx = 0;
    m->total_edges = 0;
}

void zFeetAddSkidMark(_xEnt* ent, _tagFeetSkidMark* m, float seconds)
{
    _xVec3 feet_dir;
    signed int i;
    _tagSkidEdge* e;

    m->sample_timer -= seconds;
    if (xEntIsVisible(ent) && zFeetCanLeaveMark(ent) && m->sample_timer <= 0.0f)
    {
        m->sample_timer = 0.0f;
        if (m->total_edges == 0)
        {
            m->remove_timer = 1.0f;
        }
        else if (m->total_edges >= 16)
        {
            m->total_edges = 15;
        }

        _xMat4x3* mat = (_xMat4x3*)ent->model->Mat;
        feet_dir.x = mat->right.x;
        feet_dir.y = 0.0f;
        feet_dir.z = mat->right.z;
        xVec3Normalize(&feet_dir, &feet_dir);

        for (i = 0; i < m->total_feet; i++)
        {
            e = &m->edge[i][m->total_edges];
            iModelTagEval(ent->model->Data, &globals.player.HangPawTag[i], ent->model->Mat,
                          &e->centre);
            e->centre.y = ((_xMat4x3*)globals.player.ent.model->Mat)->pos.y - 0.15f;
            e->local_x = feet_dir;
            e->width = cPlayerSkidWidth[i];
        }
        m->total_edges++;
    }
}

void zFeetUpdateSkidMark(_tagFeetSkidMark* m, float seconds)
{
    if (m->total_edges > 0)
    {
        m->remove_timer -= seconds;
        if (m->remove_timer <= 0.0f)
        {
            m->remove_timer = 1.0f;
            m->first_idx++;
            if (m->first_idx >= m->total_edges - 1)
            {
                m->first_idx = 0;
                m->total_edges = 0;
            }
        }
    }
}

// TODO: 86.3%. The original multiplies vert_idx (2) by the vertex size at run
// time and keeps vert_idx in r11; ours folds it. Store scheduling of the first
// two vertices differs too.
static void DrawSkidMarkStrip(_tagFeetSkidMark* m, const _tagSkidEdge* edge_array,
                              unsigned char red, unsigned char green, unsigned char blue)
{
    static RxObjSpace3DVertex strip_vert[256];
    unsigned int vert_idx;
    const _tagSkidEdge* e;
    float alphaf;
    unsigned char alphau8;
    signed int count;

    vert_idx = 2;
    e = &edge_array[m->first_idx];
    alphaf = m->remove_timer * 128.0f;
    alphau8 = alphaf;

    strip_vert[0].objVertex.x = e->centre.x + e->local_x.x * -e->width;
    strip_vert[0].objVertex.y = 0.2f + (e->centre.y + e->local_x.y * -e->width);
    strip_vert[0].objVertex.z = e->centre.z + e->local_x.z * -e->width;
    strip_vert[0].u = 0.0f;
    strip_vert[0].v = 0.0f;
    strip_vert[0].color[0] = red;
    strip_vert[0].color[1] = green;
    strip_vert[0].color[2] = blue;
    strip_vert[0].color[3] = alphau8;

    strip_vert[1].objVertex.x = e->centre.x + e->local_x.x * e->width;
    strip_vert[1].objVertex.y = 0.2f + (e->centre.y + e->local_x.y * e->width);
    strip_vert[1].objVertex.z = e->centre.z + e->local_x.z * e->width;
    strip_vert[1].u = 0.0f;
    strip_vert[1].v = 1.0f;
    strip_vert[1].color[0] = red;
    strip_vert[1].color[1] = green;
    strip_vert[1].color[2] = blue;
    strip_vert[1].color[3] = alphau8;

    for (count = m->first_idx + 1; count < m->total_edges; count++)
    {
        e++;
        strip_vert[vert_idx].objVertex.x = e->centre.x + e->local_x.x * -e->width;
        strip_vert[vert_idx].objVertex.y = 0.2f + (e->centre.y + e->local_x.y * -e->width);
        strip_vert[vert_idx].objVertex.z = e->centre.z + e->local_x.z * -e->width;
        if (count == m->total_edges - 1)
        {
            strip_vert[vert_idx].u = 1.0f;
        }
        else if (count & 1)
        {
            strip_vert[vert_idx].u = 0.2f;
        }
        else
        {
            strip_vert[vert_idx].u = 0.4f;
        }
        strip_vert[vert_idx].v = 0.0f;
        strip_vert[vert_idx].color[0] = red;
        strip_vert[vert_idx].color[1] = green;
        strip_vert[vert_idx].color[2] = blue;
        strip_vert[vert_idx].color[3] = 0x80;
        vert_idx++;
        if (vert_idx >= 256)
        {
            vert_idx = 256;
            break;
        }

        strip_vert[vert_idx].objVertex.x = e->centre.x + e->local_x.x * e->width;
        strip_vert[vert_idx].objVertex.y = 0.2f + (e->centre.y + e->local_x.y * e->width);
        strip_vert[vert_idx].objVertex.z = e->centre.z + e->local_x.z * e->width;
        if (count == m->total_edges - 1)
        {
            strip_vert[vert_idx].u = 1.0f;
        }
        else if (count & 1)
        {
            strip_vert[vert_idx].u = 0.2f;
        }
        else
        {
            strip_vert[vert_idx].u = 0.4f;
        }
        strip_vert[vert_idx].v = 1.0f;
        strip_vert[vert_idx].color[0] = red;
        strip_vert[vert_idx].color[1] = green;
        strip_vert[vert_idx].color[2] = blue;
        strip_vert[vert_idx].color[3] = 0x80;
        vert_idx++;
        if (vert_idx >= 256)
        {
            vert_idx = 256;
            break;
        }
    }

    if (RwIm3DTransform(strip_vert, vert_idx, 0, 0x19))
    {
        RwIm3DRenderPrimitive(4);
        RwIm3DEnd();
    }
}

void zFeetRenderSkidMark(_tagFeetSkidMark* m)
{
    signed int i;

    if (m->total_edges < 2)
    {
        return;
    }

    RwRenderStateSet(1, sSkidMarkRaster);
    for (i = 0; i < m->total_feet; i++)
    {
        DrawSkidMarkStrip(m, m->edge[i], 20, 18, 3);
    }
}

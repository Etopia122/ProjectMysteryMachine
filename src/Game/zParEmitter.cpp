#include "zParEmitter.h"
#include "zGlobals.h"
#include "zScene.h"
#include "xParGroup.h"
#include "xstransvc.h"
#include "xString.h"
#include "xMath.h"
#include "iMath.h"
#include "rwtypes.h"

extern "C" void* memset(void* dst, int c, unsigned int n);
extern "C" double cos(double x);
extern "C" double sin(double x);

class RpAtomic;
class _zGroup;

class zGroupAsset : public p2BaseAsset
{
public:
    unsigned short itemCount;  // 0x8
    unsigned short groupFlags; // 0xA
};

class _zGroup : public xBase
{
public:
    zGroupAsset* asset; // 0x10
};

class zParSysAsset : public p2BaseAsset
{
public:
    unsigned int type;                // 0x8
    unsigned int parentParSysID;      // 0xC
    unsigned int textureID;           // 0x10
    unsigned char parFlags;           // 0x14
    unsigned char priority;           // 0x15
    unsigned short maxPar;            // 0x16
    unsigned char renderFunc;         // 0x18
    unsigned char renderSrcBlendMode; // 0x19
    unsigned char renderDstBlendMode; // 0x1A
    unsigned char cmdCount;           // 0x1B
    unsigned int cmdSize;             // 0x1C
};

class zParSys : public xBase
{
public:
    zParSysAsset* tasset; // 0x10
    unsigned int cmdCount; // 0x14
    void* cmd;            // 0x18
    zParSys* parent;      // 0x1C
    xParGroup* group;     // 0x20
    unsigned char visible; // 0x24
};

class zVolumeAsset : public p2BaseAsset
{
public:
    unsigned int flags; // 0x8
    _xBound bound;      // 0xC
};

class _zVolume : public xBase
{
public:
    zVolumeAsset* asset; // 0x10
};

class _xEntModel
{
public:
    unsigned char pad0[0x10];
    RpAtomic* Data;   // 0x10
    unsigned char pad14[0x20 - 0x14];
    RwMatrixTag* Mat; // 0x20
};

class _xEntAttach : public xBase
{
public:
    unsigned char pad10[0x2C - 0x10];
    _xEntModel* model; // 0x2C
};

class _xMovePointAttach : public xBase
{
public:
    unsigned char pad10[0x14 - 0x10];
    _xVec3* pos; // 0x14
};

class _xVec2
{
public:
    float x;
    float y;
};

class _xMat2x2
{
public:
    float a[4];
};

static inline void xMat2x2MulVec(_xVec2* o, const _xMat2x2* m, const _xVec2* v)
{
    float x = m->a[0] * v->x + m->a[1] * v->y;
    float y = m->a[2] * v->x + m->a[3] * v->y;
    o->x = x;
    o->y = y;
}

static inline float xVec3Length(const _xVec3* v)
{
    return xsqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

unsigned int zGroupGetCount(_zGroup* g);
xBase* zGroupGetItemPtr(_zGroup* g, unsigned int index);
unsigned int zGroupGetItem(_zGroup* g, unsigned int index);
signed int zGameIsPaused();
void xBaseReset(xBase* xb, p2BaseAsset* asset);
unsigned int iModelTagSetup(iModelTag* tag, RpAtomic* model, float x, float y, float z);
void iModelTagEval(RpAtomic* model, iModelTag* tag, RwMatrixTag* mat, _xVec3* dest);
float xVec3Normalize(_xVec3* o, _xVec3* v);

static _zParEmitterCustomSettings sSaveEmmiterSettings;

xPar* zParEmitterEmit(_zParEmitter* pe, float dt);
signed int zParEmitterEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                              xBase* toParamWidget);

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zParEmitterInit(xBase* b, zParEmitterAsset* tasset)
{
    _zParEmitter* t = (_zParEmitter*)b;

    xBaseInit(b, tasset);
    b->eventFunc = zParEmitterEventCB;
    t->tasset = tasset;
    if (b->linkCount)
    {
        b->link = (p2LinkAsset*)(t->tasset + 1);
    }
    else
    {
        b->link = 0;
    }
    t->emit_flags = tasset->emit_flags;
    t->emit_interval_current = tasset->interval;
    if (tasset->attachToID && tasset->emit_type == 9)
    {
        _xEntAttach* ent = (_xEntAttach*)zSceneFindObject(tasset->attachToID);
        iModelTagSetup(&t->tag, ent->model->Data, tasset->e_offsetp.offset.x,
                       tasset->e_offsetp.offset.y, tasset->e_offsetp.offset.z);
    }
    t->attachTo = tasset->attachToID ? zSceneFindObject(tasset->attachToID) : 0;
    t->parSys = tasset->parSysID ? zSceneFindObject(tasset->parSysID) : 0;
    if (tasset->emit_type == 6)
    {
        t->emit_volume = zSceneFindObject(tasset->e_volume.emit_volumeID);
    }
    else
    {
        t->emit_volume = 0;
    }
}

void zParEmitterSetup(_zParEmitter* t)
{
    if (t->parSys)
    {
        t->group = ((zParSys*)t->parSys)->group;
    }
}

signed int zParEmitterEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                              xBase* toParamWidget)
{
    _zParEmitter* t = (_zParEmitter*)to;

    switch (toEvent)
    {
    case 10:
        xBaseReset(t, t->tasset);
        t->emit_flags = t->tasset->emit_flags;
        t->emit_interval_current = t->tasset->interval;
        break;
    case 0x26:
        t->emit_flags |= 1;
        break;
    case 0x27:
        if (t->emit_flags & 1)
        {
            t->emit_flags ^= 1;
        }
        break;
    case 0xFA:
    {
        _zParEmitterCustomSettings fake_settings;
        memset(&fake_settings, 0, sizeof(_zParEmitterCustomSettings));
        zParEmitterEmitCustom(t, 1.0f / 30.0f, &fake_settings);
        break;
    }
    }
    return 1;
}

static inline void zParEmitterAngleVariation(xPar* p, zParEmitterAsset* a)
{
    if (a->vel_angle_variation != 0.0f)
    {
        _xMat3x3 mat_rot = {};
        float ang[3] = {};
        float ang_debrisCone = a->vel_angle_variation;

        ang[0] = ang_debrisCone * (iurand() - 0.5f);
        ang[1] = ang_debrisCone * (iurand() - 0.5f);
        ang[2] = ang_debrisCone * (iurand() - 0.5f);
        xMat3x3Euler(&mat_rot, ang[0], ang[1], ang[2]);
        xMat3x3LMulVec(&p->m_vel, &mat_rot, &p->m_vel);
    }
}

static inline void zParEmitterEmitPoint(xPar* p, zParEmitterAsset* a, float dt)
{
    xVec3Copy(&p->m_vel, &a->vel);
    xVec3SMulBy(&p->m_vel, dt);
    zParEmitterAngleVariation(p, a);
}

static inline void zParEmitterEmitRect(xPar* p, zParEmitterAsset* a, float dt)
{
    xVec3Copy(&p->m_vel, &a->vel);
    xVec3SMulBy(&p->m_vel, dt);
    p->m_pos.x += 2.0f * (a->e_rect.x_len * iurand()) - a->e_rect.x_len;
    p->m_pos.z += 2.0f * (a->e_rect.z_len * iurand()) - a->e_rect.z_len;
    zParEmitterAngleVariation(p, a);
}

static inline void zParEmitterEmitVolume(_zParEmitter* pe, xPar* p, zParEmitterAsset* a, float dt)
{
    _zVolume* vol = (_zVolume*)pe->emit_volume;
    xVec3Copy(&p->m_vel, &a->vel);
    xVec3SMulBy(&p->m_vel, dt);
    if (vol)
    {
        zVolumeAsset* va = vol->asset;
        if (va->bound.type == 2)
        {
            float dx = va->bound.box.box.upper.x - va->bound.box.box.lower.x;
            float dy = va->bound.box.box.upper.y - va->bound.box.box.lower.y;
            float dz = va->bound.box.box.upper.z - va->bound.box.box.lower.z;
            dx *= iurand();
            dy *= iurand();
            dz *= iurand();
            p->m_pos.x = dx + va->bound.box.box.lower.x;
            p->m_pos.y = dy + va->bound.box.box.lower.y;
            p->m_pos.z = dz + va->bound.box.box.lower.z;
        }
    }
    zParEmitterAngleVariation(p, a);
}

static inline void zParEmitterEmitOffsetPoint(_zParEmitter* pe, xPar* p, zParEmitterAsset* a)
{
    _xEntModel* model = ((_xEntAttach*)pe->attachTo)->model;
    iModelTagEval(model->Data, &pe->tag, model->Mat, &p->m_pos);
    zParEmitterAngleVariation(p, a);
}

static inline void zParEmitterEmitCircleEdge(xPar* p, zParEmitterAsset* a, float dt)
{
    xVec3Copy(&p->m_vel, &a->vel);
    xVec3SMulBy(&p->m_vel, dt);

    float rot_amount = 360.0f * iurand();
    float ct = cos(rot_amount);
    float st = sin(rot_amount);
    _xMat2x2 rot_mat = { 1.0f, 0.0f, 0.0f, 1.0f };
    _xVec2 vec2 = { 0.0f, 1.0f };
    float dt_radius;
    rot_mat.a[0] = ct;
    rot_mat.a[1] = st;
    rot_mat.a[2] = -st;
    rot_mat.a[3] = ct;
    _xVec2 res;
    res.x = (rot_mat.a[0] * vec2.x + rot_mat.a[1] * vec2.y) * a->e_circle.radius;
    res.y = (rot_mat.a[2] * vec2.x + rot_mat.a[3] * vec2.y) * a->e_circle.radius;
    p->m_pos.x += res.x;
    p->m_pos.z += res.y;
    zParEmitterAngleVariation(p, a);
    if (a->e_circle.deflection != 0.0f)
    {
        dt_radius = a->e_circle.deflection * (a->e_circle.radius * dt);
        p->m_vel.x += res.x * dt_radius;
        p->m_vel.z += res.y * dt_radius;
    }
}

static inline void zParEmitterEmitCircle(xPar* p, zParEmitterAsset* a, float dt)
{
    xVec3Copy(&p->m_vel, &a->vel);
    xVec3SMulBy(&p->m_vel, dt);

    float rot_amount = 360.0f * iurand();
    float ct = cos(rot_amount);
    float st = sin(rot_amount);
    _xMat2x2 rot_mat = { 1.0f, 0.0f, 0.0f, 1.0f };
    float dt_radius;
    rot_mat.a[0] = ct;
    rot_mat.a[1] = st;
    rot_mat.a[2] = -st;
    rot_mat.a[3] = ct;
    _xVec2 vec2 = { 0.0f, 1.0f };
    vec2.y = a->e_circle.radius * iurand();
    _xVec2 res;
    res.x = rot_mat.a[0] * vec2.x + rot_mat.a[1] * vec2.y;
    res.y = rot_mat.a[2] * vec2.x + rot_mat.a[3] * vec2.y;
    p->m_pos.x += res.x;
    p->m_pos.z += res.y;
    zParEmitterAngleVariation(p, a);
    if (a->e_circle.deflection != 0.0f)
    {
        dt_radius = a->e_circle.deflection * (a->e_circle.radius * dt);
        p->m_vel.x += res.x * dt_radius;
        p->m_vel.z += res.y * dt_radius;
    }
}

static inline void zParEmitterEmitLine(xPar* p, zParEmitterAsset* a, float dt)
{
    _xVec3 dir;
    float len;

    xVec3Copy(&p->m_vel, &a->vel);
    xVec3SMulBy(&p->m_vel, dt);
    xVec3Sub(&dir, &a->e_line.pos2, &a->e_line.pos1);
    len = xVec3Normalize(&dir, &dir);
    len *= iurand();
    p->m_pos.x = dir.x * len + a->e_line.pos1.x;
    p->m_pos.y = dir.y * len + a->e_line.pos1.y;
    p->m_pos.z = dir.z * len + a->e_line.pos1.z;
    zParEmitterAngleVariation(p, a);
}

static inline void zParEmitterEmitSphere(xPar* p, zParEmitterAsset* a, float dt)
{
    _xVec3 dirvec = { 0.0f, 1.0f, 0.0f };
    _xVec3 offset = {};
    _xMat3x3 mat_rot = {};

    xVec3Copy(&p->m_vel, &a->vel);
    xVec3SMulBy(&p->m_vel, dt);
    xMat3x3Euler(&mat_rot, 2.0f * PI * iurand(), 2.0f * PI * iurand(), 2.0f * PI * iurand());
    xMat3x3LMulVec(&dirvec, &mat_rot, &dirvec);
    float len = xVec3Length(&p->m_vel);
    xVec3SMul(&offset, &dirvec, a->e_sphere.radius * iurand());
    xVec3AddTo(&p->m_pos, &offset);
    zParEmitterAngleVariation(p, a);
}

static void zParEmitterEmitSphereEdge(xPar* p, zParEmitterAsset* a, float dt, signed int subtype)
{
    _xVec3 dirvec = { 0.0f, 1.0f, 0.0f };
    _xVec3 offset = {};
    _xMat3x3 mat_rot = {};

    xVec3Copy(&p->m_vel, &a->vel);
    xVec3SMulBy(&p->m_vel, dt);
    switch (subtype)
    {
    case 7:
        xMat3x3Euler(&mat_rot, 2.0f * PI * iurand(), 2.0f * PI * iurand(), 2.0f * PI * iurand());
        break;
    case 10:
        xMat3x3Euler(&mat_rot, 2.0f * PI * iurand(), PI * iurand(), 0.0f);
        break;
    case 11:
        xMat3x3Euler(&mat_rot, 2.0f * PI * iurand(), PI * iurand() + PI, 2.0f * PI * iurand());
        break;
    }
    xMat3x3LMulVec(&dirvec, &mat_rot, &dirvec);
    xVec3SMul(&p->m_vel, &dirvec, xVec3Length(&p->m_vel));
    xVec3SMul(&offset, &dirvec, a->e_sphere.radius);
    xVec3AddTo(&p->m_pos, &offset);
    zParEmitterAngleVariation(p, a);
}

xPar* zParEmitterEmitCustom(_zParEmitter* p, float dt, _zParEmitterCustomSettings* info)
{
    unsigned int flags = info->custom_flags;
    zParEmitterAsset* pe = p->tasset;
    _zParEmitterCustomSettings* s = &sSaveEmmiterSettings;
    xPar* emitted_par;

    if (flags & 0x4000)
    {
        s->count = pe->count;
        s->count_variation = pe->count_variation;
        s->interval = pe->interval;
        s->attachToID = pe->attachToID;
        s->pos = pe->pos;
        s->vel = pe->vel;
        s->vel_angle_variation = pe->vel_angle_variation;
        for (signed int i = 0; i < 4; i++)
        {
            s->color_birth[i] = pe->color_birth[i];
            s->color_death[i] = pe->color_death[i];
        }
        s->size_birth = pe->size_birth;
        s->size_birth_variation = pe->size_birth_variation;
        s->size_death = pe->size_death;
        s->life = pe->life;
        s->life_variation = pe->life_variation;
        s->emit_interval_current = p->emit_interval_current;
        s->rot[0] = p->rot[0];
        s->rot[1] = p->rot[1];
        s->rot[2] = p->rot[2];
        s->xzcircle_radius = pe->e_circle.radius;
    }
    if (flags & 4)
    {
        pe->count = info->count;
    }
    if (flags & 8)
    {
        pe->count_variation = info->count_variation;
    }
    if (flags & 0x10)
    {
        pe->interval = info->interval;
    }
    if (flags & 0x800)
    {
        p->emit_interval_current = 0.0f;
    }
    if (flags & 0x8000)
    {
        pe->attachToID = info->attachToID;
        p->attachTo = pe->attachToID ? zSceneFindObject(pe->attachToID) : 0;
    }
    if (flags & 0x20)
    {
        pe->pos = info->pos;
    }
    if (flags & 0x10000)
    {
        p->rot[0] = info->rot[0];
        p->rot[1] = info->rot[1];
        p->rot[2] = info->rot[2];
    }
    if (flags & 0x20000)
    {
        pe->e_circle.radius = info->xzcircle_radius;
    }
    if (flags & 0x40)
    {
        pe->vel = info->vel;
    }
    if (flags & 2)
    {
        pe->vel_angle_variation = info->vel_angle_variation;
    }
    if (flags & 0x80)
    {
        for (signed int i = 0; i < 4; i++)
        {
            pe->color_birth[i] = info->color_birth[i];
        }
    }
    if (flags & 0x100)
    {
        for (signed int i = 0; i < 4; i++)
        {
            pe->color_death[i] = info->color_death[i];
        }
    }
    if (flags & 0x200)
    {
        pe->size_birth = info->size_birth;
    }
    if (flags & 0x2000)
    {
        pe->size_birth_variation = info->size_birth_variation;
    }
    if (flags & 0x400)
    {
        pe->size_death = info->size_death;
    }
    if (flags & 1)
    {
        pe->life = info->life;
    }
    if (flags & 0x1000)
    {
        pe->life_variation = info->life_variation;
    }

    emitted_par = zParEmitterEmit(p, dt);

    if (flags & 0x4000)
    {
        pe->count = s->count;
        pe->count_variation = s->count_variation;
        pe->interval = s->interval;
        pe->attachToID = s->attachToID;
        p->attachTo = pe->attachToID ? zSceneFindObject(pe->attachToID) : 0;
        pe->pos = s->pos;
        pe->vel = s->vel;
        pe->vel_angle_variation = s->vel_angle_variation;
        for (signed int i = 0; i < 4; i++)
        {
            pe->color_birth[i] = s->color_birth[i];
            pe->color_death[i] = s->color_death[i];
        }
        pe->size_birth = s->size_birth;
        pe->size_birth_variation = s->size_birth_variation;
        pe->size_death = s->size_death;
        pe->life = s->life;
        pe->life_variation = s->life_variation;
        p->emit_interval_current = s->emit_interval_current;
        p->rot[0] = s->rot[0];
        p->rot[1] = s->rot[1];
        p->rot[2] = s->rot[2];
        pe->e_circle.radius = s->xzcircle_radius;
    }
    return emitted_par;
}

static inline signed int zParEmitterCull(zParEmitterAsset* a, xPar* p)
{
    if (a->emit_flags & 2)
    {
        _xVec3* cam = (_xVec3*)&globals.camera.hi_cam.mat.pos;
        float dx = cam->x - p->m_pos.x;
        float dy = cam->y - p->m_pos.y;
        float dz = cam->z - p->m_pos.z;
        float d2 = dx * dx + dy * dy + dz * dz;
        if (a->cull_mode == 3)
        {
            return d2 > a->cull_dist_sqr;
        }
        return d2 < a->cull_dist_sqr;
    }
    return 0;
}

// TODO (99.3%): same instructions and stack layout; some saved registers are
// assigned differently (pe, the .rodata base and several float temporaries).
xPar* zParEmitterEmit(_zParEmitter* pe, float dt)
{
    signed int attachGroupTotal;
    zParSys* ps;
    xPar* last_p;
    zParEmitterAsset* a;
    unsigned int get_rnd_group_idx;
    signed int attachGroupIndex;
    _xVec3 pos;
    unsigned int count;
    signed int emitAgain;
    xBase* emitObj;
    signed int marker;
    float life;
    float size_birth;
    xPar* p;
    signed int i;
    _zGroup* g;

    a = pe->tasset;
    last_p = 0;
    if (pe->emit_interval_current > 0.0f)
    {
        pe->emit_interval_current -= dt;
        if (pe->emit_interval_current > 0.0f)
        {
            return 0;
        }
        pe->emit_interval_current = pe->tasset->interval;
    }

    ps = (zParSys*)pe->parSys;
    if (ps && ps->group)
    {
        if (ps->tasset->maxPar && ps->group->m_num_of_particles >= ps->tasset->maxPar)
        {
            return 0;
        }

        life = a->life;
        size_birth = a->size_birth;
        count = a->count;
        if (pe->tasset->count_variation)
        {
            signed int var = pe->tasset->count_variation;
            signed int c = irand() % (var * 2) - var;
            count = a->count + (0 > c ? 0 : c);
        }
        if (ps->tasset->maxPar && ps->group->m_num_of_particles + count >= ps->tasset->maxPar)
        {
            count = ps->tasset->maxPar - ps->group->m_num_of_particles;
        }

        g = (_zGroup*)pe->attachTo;
        attachGroupIndex = -1;
        attachGroupTotal = -1;
        emitAgain = 0;
        do
        {
            emitObj = 0;
            marker = 0;
            if (g)
            {
                if (g->baseType == 0x11)
                {
                    if (attachGroupIndex == -1)
                    {
                        attachGroupTotal = zGroupGetCount(g);
                        attachGroupIndex = 0;
                    }
                    if (g->asset->groupFlags & 1)
                    {
                        get_rnd_group_idx = irand() % attachGroupTotal;
                        emitObj = zGroupGetItemPtr(g, get_rnd_group_idx);
                        emitAgain = 0;
                        if (!emitObj)
                        {
                            emitObj = (xBase*)xSTFindAsset(zGroupGetItem(g, get_rnd_group_idx), 0);
                            if (emitObj)
                            {
                                marker = 1;
                            }
                        }
                    }
                    else
                    {
                        emitObj = zGroupGetItemPtr(g, attachGroupIndex);
                        if (!emitObj)
                        {
                            emitObj = (xBase*)xSTFindAsset(zGroupGetItem(g, attachGroupIndex), 0);
                            if (emitObj)
                            {
                                marker = 1;
                            }
                        }
                        attachGroupIndex++;
                        emitAgain = attachGroupIndex < attachGroupTotal;
                    }
                }
                else
                {
                    emitObj = g;
                }
            }

            if (emitObj)
            {
                if (marker)
                {
                    pos = *(_xVec3*)emitObj;
                }
                else
                {
                    switch (emitObj->baseType)
                    {
                    case 0xD:
                        pos = *((_xMovePointAttach*)emitObj)->pos;
                        break;
                    default:
                        pos = *(_xVec3*)&((_xEntAttach*)emitObj)->model->Mat->pos;
                        break;
                    }
                }
            }
            else
            {
                pos = a->pos;
            }

            for (i = 0; i < (signed int)count; i++)
            {
                p = xParGroupAddPar(ps->group);
                if (!p)
                {
                    continue;
                }
                last_p = p;

                if (a->life_variation != 0.0f)
                {
                    life = 2.0f * (a->life_variation * iurand()) + a->life - a->life_variation;
                    if (life < 0.0f)
                    {
                        life = a->life;
                    }
                }
                if (a->size_birth_variation != 0.0f)
                {
                    size_birth = 2.0f * (a->size_birth_variation * iurand()) + a->size_birth -
                                 a->size_birth_variation;
                }

                p->m_asset = pe->tasset;
                p->m_lifetime = life;
                p->m_size = size_birth;
                if (a->size_death == size_birth)
                {
                    p->m_sizeVel = 0.0f;
                }
                else
                {
                    p->m_sizeVel = (a->size_death - size_birth) / life;
                }
                p->m_flag = 0;
                p->m_rotdeg[0] = pe->rot[0];
                p->m_rotdeg[1] = pe->rot[1];
                p->m_rotdeg[2] = pe->rot[2];
                for (signed int c = 0; c < 4; c++)
                {
                    p->m_c[c] = a->color_birth[c];
                    p->m_cvel[c] = a->color_death[c] - a->color_birth[c];
                    p->m_cvel[c] /= life;
                }
                p->m_pos = pos;

                switch (a->emit_type)
                {
                case 0:
                    zParEmitterEmitPoint(p, a, dt);
                    break;
                case 1:
                    zParEmitterEmitCircleEdge(p, a, dt);
                    break;
                case 2:
                    zParEmitterEmitCircle(p, a, dt);
                    break;
                case 4:
                    zParEmitterEmitRect(p, a, dt);
                    break;
                case 5:
                    zParEmitterEmitLine(p, a, dt);
                    break;
                case 7:
                    zParEmitterEmitSphereEdge(p, a, dt, 7);
                    break;
                case 10:
                    zParEmitterEmitSphereEdge(p, a, dt, 10);
                    break;
                case 11:
                    zParEmitterEmitSphereEdge(p, a, dt, 11);
                    break;
                case 8:
                    zParEmitterEmitSphere(p, a, dt);
                    break;
                case 6:
                    zParEmitterEmitVolume(pe, p, a, dt);
                    break;
                case 9:
                    zParEmitterEmitOffsetPoint(pe, p, a);
                    break;
                }

                if (zParEmitterCull(pe->tasset, p))
                {
                    xParGroupKillPar(ps->group, p);
                }
            }
        } while (emitAgain);
    }

    return last_p;
}

void zParEmitterUpdate(xBase* to, float dt)
{
    _zParEmitter* pe = (_zParEmitter*)to;

    if (!zGameIsPaused() && (pe->emit_flags & 1))
    {
        zParEmitterEmit(pe, dt);
    }
}

#pragma auto_inline on
_zParEmitter* zParEmitterFind(unsigned int asset_id)
{
    zScene* s = globals.sceneCur;
    if (!s)
    {
        return 0;
    }

    _zParEmitter* pe = (_zParEmitter*)s->baseList[38];
    for (signed int i = 0; i < (signed int)s->baseCount[38]; i++)
    {
        if (pe->tasset->id == asset_id)
        {
            return pe;
        }
        pe++;
    }
    return 0;
}
#pragma auto_inline off

_zParEmitter* zParEmitterFind(char* asset_name)
{
    unsigned int asset_id = xStrHash(asset_name);
    return zParEmitterFind(asset_id);
}

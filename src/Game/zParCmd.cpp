#include "xParGroup.h"
#include "xMath.h"
#include "xMath3.h"
#include "xBound.h"
#include "xString.h"
#include "iMath.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zParEmitter.h"

extern "C" void* memset(void* dst, int c, unsigned int n);
extern "C" int sprintf(char* s, const char* format, ...);

class xParCmd
{
public:
    unsigned int flag;
};

class xParCmdAsset
{
public:
    unsigned int type;
    unsigned char enabled;
    unsigned char mode;
    unsigned char pad[2];
};

class zParCmd : public xParCmd
{
public:
    xParCmdAsset* tasset;
};

class xParCmdMove : public xParCmdAsset
{
public:
    _xVec3 dir;
};

class xParCmdAccelerate : public xParCmdAsset
{
public:
    _xVec3 acc;
};

class xParCmdJet : public xParCmdAsset
{
public:
    _xVec3 center;
    _xVec3 acc;
    float gravity;
    float epsilon;
    float radiusSqr;
};

class xParCmdKillSlow : public xParCmdAsset
{
public:
    float speedLimitSqr;
    unsigned int kill_less_than;
};

class xParCmdFollow : public xParCmdAsset
{
public:
    float gravity;
    float epsilon;
};

class xParCmdOrbitPoint : public xParCmdAsset
{
public:
    _xVec3 center;
    float gravity;
    float epsilon;
    float maxRadiusSqr;
};

class xParCmdOrbitLine : public xParCmdAsset
{
public:
    _xVec3 p;
    _xVec3 axis;
    float gravity;
    float epsilon;
    float maxRadiusSqr;
};

class xParCmdMoveRandomPar : public xParCmdAsset
{
public:
    _xVec3 dim;
};

class xParCmdScale3rdPolyReg : public xParCmdAsset
{
public:
    float constant;
    float a;
    float a2;
    float a3;
};

class xParCmdTex : public xParCmdAsset
{
public:
    float x1;
    float y1;
    float x2;
    float y2;
    unsigned char birthMode;
    unsigned char rows;
    unsigned char cols;
    unsigned char unit_count;
    float unit_width;
    float unit_height;
};

class xParCmdTexAnim : public xParCmdAsset
{
public:
    unsigned char anim_mode;
    unsigned char anim_wrap_mode;
    unsigned char pad_anim;
    unsigned char throttle_spd_less_than;
    float throttle_spd_sqr;
    float throttle_time;
    float throttle_time_elapsed;
};

class xParCmdPlayerCollision : public xParCmdAsset
{
public:
    float min;
    float max;
};

class xParCmdRandomVelocityPar : public xParCmdAsset
{
public:
    float x;
    float y;
    float z;
};

class xParCmdCustom : public xParCmdAsset
{
public:
    unsigned int kind;
};

class xParCmdKillDistance : public xParCmdAsset
{
public:
    float dSqr;
    unsigned int kill_greater_than;
};

class xParCmdAge : public xParCmdAsset
{
public:
    float age_rate;
};

class xParCmdAlpha3rdPolyReg : public xParCmdAsset
{
public:
    float constant;
    float a;
    float a2;
    float a3;
};

class xParCmdApplyWind : public xParCmdAsset
{
public:
    float strength;
};

class xParCmdRotPar : public xParCmdAsset
{
public:
    _xVec3 min;
    _xVec3 max;
};

class xParCmdApplyCamMat : public xParCmdAsset
{
public:
    _xVec3 apply;
};

class xParCmdRotateAround : public xParCmdAsset
{
public:
    _xVec3 pos;
    float unused1;
    float radius_growth;
    float yaw;
};

class xParCmdCurve : public xParCmdAsset
{
public:
    signed int curve;
};

class zVolumeAsset
{
public:
    unsigned char unk0[0xC];
    _xBound bound; // 0xC
};

class _zVolume : public xBase
{
public:
    zVolumeAsset* asset;
};

class _tagClimate;

typedef void (*zParCmdUpdateFunc)(zParCmd*, xParGroup*, float);

class zCmdInfo
{
public:
    unsigned int type;
    unsigned int size;
    zParCmdUpdateFunc func;
};

extern _tagClimate gClimate;

float xVec3Normalize(_xVec3* o, _xVec3* v);
float xVec3LengthFast(float x, float y, float z);
float xMat3x3LookVec(_xMat3x3* m, _xVec3* at);
void xClimateGetRealWindDir(_tagClimate* climate, _xVec3* dir);
void zEntPlayer_Damage(xBase* src, unsigned int damage, _xVec3* knock);
signed int zRayHitsPlayer(_xRay3* ray);

void xParCmdCustom_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdTexAnim_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdTex_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdJet_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdRotateAround_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdDamagePlayer_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdAnimalMagentism_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdApplyCamMat_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdVelocityApply_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdRotPar_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdApplyWind_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdRandomVelocityPar_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdPlayerCollision_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdAlpha3rdPolyReg_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdClipVolumes_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdScale_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdSmokeAlpha_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdScale3rdPolyReg_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdMoveRandomPar_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdMoveRandom_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdMove_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdAccelerate_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdOrbitLine_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdOrbitPoint_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdFollow_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdAge_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdKillSlow_Update(zParCmd* c, xParGroup* ps, float dt);
void xParCmdKillDistance_Update(zParCmd* c, xParGroup* ps, float dt);

#define UPDATE(f) (f)

static inline void xVec3Cross(_xVec3* o, const _xVec3* a, const _xVec3* b)
{
    o->x = a->y * b->z - b->y * a->z;
    o->y = a->z * b->x - b->z * a->x;
    o->z = a->x * b->y - b->x * a->y;
}

static zCmdInfo sCmdInfo[29] = {
    { 0, sizeof(xParCmdMove), UPDATE(xParCmdMove_Update) },
    { 1, sizeof(xParCmdMove), UPDATE(xParCmdMoveRandom_Update) },
    { 2, sizeof(xParCmdAccelerate), UPDATE(xParCmdAccelerate_Update) },
    { 3, sizeof(xParCmdAsset), UPDATE(xParCmdVelocityApply_Update) },
    { 4, sizeof(xParCmdJet), UPDATE(xParCmdJet_Update) },
    { 5, 0xC, 0 },
    { 6, sizeof(xParCmdKillSlow), UPDATE(xParCmdKillSlow_Update) },
    { 7, sizeof(xParCmdFollow), UPDATE(xParCmdFollow_Update) },
    { 8, sizeof(xParCmdOrbitPoint), UPDATE(xParCmdOrbitPoint_Update) },
    { 9, sizeof(xParCmdOrbitLine), UPDATE(xParCmdOrbitLine_Update) },
    { 10, sizeof(xParCmdMoveRandomPar), UPDATE(xParCmdMoveRandomPar_Update) },
    { 11, sizeof(xParCmdScale3rdPolyReg), UPDATE(xParCmdScale3rdPolyReg_Update) },
    { 12, sizeof(xParCmdTex), UPDATE(xParCmdTex_Update) },
    { 13, sizeof(xParCmdTexAnim), UPDATE(xParCmdTexAnim_Update) },
    { 14, sizeof(xParCmdPlayerCollision), UPDATE(xParCmdPlayerCollision_Update) },
    { 15, sizeof(xParCmdRandomVelocityPar), UPDATE(xParCmdRandomVelocityPar_Update) },
    { 16, 0x1C, UPDATE(xParCmdCustom_Update) },
    { 17, sizeof(xParCmdKillDistance), UPDATE(xParCmdKillDistance_Update) },
    { 18, sizeof(xParCmdAge), UPDATE(xParCmdAge_Update) },
    { 19, 0x18, UPDATE(xParCmdAlpha3rdPolyReg_Update) },
    { 20, sizeof(xParCmdApplyWind), UPDATE(xParCmdApplyWind_Update) },
    { 21, sizeof(xParCmdRotPar), UPDATE(xParCmdRotPar_Update) },
    { 22, sizeof(xParCmdApplyCamMat), UPDATE(xParCmdApplyCamMat_Update) },
    { 23, sizeof(xParCmdRotateAround), UPDATE(xParCmdRotateAround_Update) },
    { 24, sizeof(xParCmdCurve), UPDATE(xParCmdSmokeAlpha_Update) },
    { 25, sizeof(xParCmdCurve), UPDATE(xParCmdScale_Update) },
    { 26, sizeof(xParCmdAsset) + 4, UPDATE(xParCmdClipVolumes_Update) },
    { 27, sizeof(xParCmdAsset) + 4, UPDATE(xParCmdAnimalMagentism_Update) },
    { 28, sizeof(xParCmdAsset) + 4, UPDATE(xParCmdDamagePlayer_Update) },
};

static _zVolume* sClipVolume[32];
static signed int sClipVolumeTotal;

void zParCmdFindClipVolumes()
{
    char findname[64];

    memset(sClipVolume, 0, sizeof(sClipVolume));
    for (sClipVolumeTotal = 0; sClipVolumeTotal < 32; sClipVolumeTotal++)
    {
        sprintf(findname, "PARTICLE_CLIP_%d", sClipVolumeTotal + 1);
        _zVolume* vol = (_zVolume*)zSceneFindObject(xStrHash(findname));
        if (vol == 0)
        {
            break;
        }
        sClipVolume[sClipVolumeTotal] = vol;
    }
}

unsigned int zParCmdGetSize(unsigned int parType)
{
    for (signed int i = 0; i < 29; i++)
    {
        if (sCmdInfo[i].type == parType)
        {
            return sCmdInfo[i].size;
        }
    }
    return 0;
}

zParCmdUpdateFunc zParCmdGetUpdateFunc(unsigned int parType)
{
    for (signed int i = 0; i < 29; i++)
    {
        if (sCmdInfo[i].type == parType)
        {
            return sCmdInfo[i].func;
        }
    }
    return 0;
}

extern "C" void fn_800C4AB8()
{
}

// TODO: 91.0%. The original computes &camera_pos->y/z in a block placed after
// the function for the second loop; ours hoists them inline.
void xParCmdKillDistance_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xPar* p = ps->m_root;
    xParCmdKillDistance* cmd = (xParCmdKillDistance*)c->tasset;
    _xVec3* camera_pos = &globals.camera.mat.pos;

    if (cmd->kill_greater_than)
    {
        while (p)
        {
            _xVec3 d;
            xVec3Sub(&d, &p->m_pos, camera_pos);
            if (xVec3Dot(&d, &d) > cmd->dSqr)
            {
                p->m_lifetime = -1.0f;
            }
            p = p->m_next;
        }
    }
    else
    {
        while (p)
        {
            _xVec3 d;
            xVec3Sub(&d, &p->m_pos, camera_pos);
            if (xVec3Dot(&d, &d) < cmd->dSqr)
            {
                p->m_lifetime = -1.0f;
            }
            p = p->m_next;
        }
    }
}

void xParCmdKillSlow_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xPar* p = ps->m_root;
    xParCmdKillSlow* cmd = (xParCmdKillSlow*)c->tasset;
    float speedLimit = cmd->speedLimitSqr * dt;

    if (cmd->kill_less_than)
    {
        while (p)
        {
            if (p->m_vel.x * p->m_vel.x + p->m_vel.y * p->m_vel.y + p->m_vel.z * p->m_vel.z <
                speedLimit)
            {
                p->m_lifetime = -1.0f;
            }
            p = p->m_next;
        }
    }
    else
    {
        while (p)
        {
            if (p->m_vel.x * p->m_vel.x + p->m_vel.y * p->m_vel.y + p->m_vel.z * p->m_vel.z >
                speedLimit)
            {
                p->m_lifetime = -1.0f;
            }
            p = p->m_next;
        }
    }
}

void xParCmdAge_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xPar* p = ps->m_root;
    float age_rate = ((xParCmdAge*)c->tasset)->age_rate * dt;

    while (p)
    {
        p->m_lifetime -= age_rate;
        p = p->m_next;
    }
}

void xParCmdFollow_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xPar* p = ps->m_root;
    xParCmdFollow* cmd = (xParCmdFollow*)c->tasset;
    float mdt = cmd->gravity * dt;

    while (p && p->m_next)
    {
        float dx = p->m_next->m_pos.x - p->m_pos.x;
        float dy = p->m_next->m_pos.y - p->m_pos.y;
        float dz = p->m_next->m_pos.z - p->m_pos.z;
        float lenSqr = dx * dx + dy * dy + dz * dz;
        float len = xVec3LengthFast(dx, dy, dz);
        float force = mdt / (len * (lenSqr + cmd->epsilon));

        p->m_vel.x += dx * force;
        p->m_vel.y += dy * force;
        p->m_vel.z += dz * force;
        p = p->m_next;
    }
}

void xParCmdOrbitPoint_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xPar* p = ps->m_root;
    xParCmdOrbitPoint* cmd = (xParCmdOrbitPoint*)c->tasset;
    float mdt = cmd->gravity * dt;

    while (p)
    {
        _xVec3 d;
        xVec3Sub(&d, &cmd->center, &p->m_pos);
        float rSqr = xVec3Dot(&d, &d);
        if (rSqr < cmd->maxRadiusSqr)
        {
            float len = xVec3LengthFast(d.x, d.y, d.z);
            float force = mdt / (len + (rSqr + cmd->epsilon));
            p->m_vel.x += d.x * force;
            p->m_vel.y += d.y * force;
            p->m_vel.z += d.z * force;
        }
        p = p->m_next;
    }
}

// TODO: 98.8%. d and c1 swap scratch registers (PS2 lists no vector locals here,
// so the vectors probably live in an inline).
void xParCmdOrbitLine_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xPar* p = ps->m_root;
    xParCmdOrbitLine* cmd = (xParCmdOrbitLine*)c->tasset;
    float mdt = cmd->gravity * dt;

    while (p)
    {
        _xVec3 d;
        _xVec3 c1;
        _xVec3 c2;
        _xVec3 v;
        float rSqr;
        xVec3Sub(&d, &p->m_pos, &cmd->p);
        xVec3Cross(&c1, &d, &cmd->axis);
        xVec3Cross(&c2, &cmd->axis, &c1);
        xVec3Sub(&v, &c2, &d);
        rSqr = xVec3Dot(&v, &v);
        if (rSqr < cmd->maxRadiusSqr)
        {
            float len = xVec3LengthFast(v.x, v.y, v.z);
            float force = mdt / (len + (rSqr + cmd->epsilon));
            p->m_vel.x += v.x * force;
            p->m_vel.y += v.y * force;
            p->m_vel.z += v.z * force;
        }
        p = p->m_next;
    }
}

void xParCmdAccelerate_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xPar* p;
    xParCmdAccelerate* cmd = (xParCmdAccelerate*)c->tasset;
    _xVec3 acc;
    acc = cmd->acc;
    xVec3SMulBy(&acc, dt);
    p = ps->m_root;
    while (p)
    {
        p->m_vel.x += acc.x;
        p->m_vel.y += acc.y;
        p->m_vel.z += acc.z;
        p = p->m_next;
    }
}

void xParCmdMove_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xPar* p;
    xParCmdMove* cmd = (xParCmdMove*)c->tasset;
    _xVec3 dir;
    dir = cmd->dir;
    xVec3SMulBy(&dir, dt);
    p = ps->m_root;
    while (p)
    {
        xVec3AddTo(&p->m_pos, &dir);
        p = p->m_next;
    }
}

void xParCmdMoveRandom_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xPar* p;
    xParCmdMove* cmd = (xParCmdMove*)c->tasset;
    _xVec3 dir;
    dir = cmd->dir;
    dir.x *= 2.0f * iurand();
    dir.y *= 2.0f * iurand();
    dir.z *= 2.0f * iurand();
    xVec3Sub(&dir, &dir, &cmd->dir);
    xVec3SMulBy(&dir, dt);
    p = ps->m_root;
    while (p)
    {
        xVec3AddTo(&p->m_pos, &dir);
        p = p->m_next;
    }
}

void xParCmdMoveRandomPar_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xParCmdMoveRandomPar* cmd = (xParCmdMoveRandomPar*)c->tasset;
    xPar* p = ps->m_root;
    float half = 0.5f;
    float dx = cmd->dim.x * (dt * half);
    float dz = cmd->dim.z * (dt * half);

    while (p)
    {
        p->m_pos.x += dx * (iurand() - half);
        p->m_pos.z += dz * (iurand() - half);
        p = p->m_next;
    }
}

// TODO: 97.3%. Register allocation of the polynomial (t, size_death).
void xParCmdScale3rdPolyReg_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xParCmdScale3rdPolyReg* cmd = (xParCmdScale3rdPolyReg*)c->tasset;
    xPar* p = ps->m_root;

    while (p)
    {
        float t = p->m_lifetime / p->m_asset->life;
        if (t < 0.0f)
        {
            t = 0.0f;
        }
        else if (t > 1.0f)
        {
            t = 1.0f;
        }
        t = 1.0f - t;
        float t2 = t * t;
        p->m_size = p->m_asset->size_death *
                    (cmd->constant + cmd->a * t + cmd->a2 * t2 + cmd->a3 * (t2 * t));
        p->m_sizeVel = 0.0f;
        p = p->m_next;
    }
}

void xParCmdSmokeAlpha_Update(zParCmd* c, xParGroup* ps, float dt)
{
    float constant, a, a2, a3;

    switch (((xParCmdCurve*)c->tasset)->curve)
    {
    case 0:
        constant = 0.0f;
        a = 8.067227f;
        a2 = -14.789916f;
        a3 = 6.722689f;
        break;
    case 1:
        constant = 0.0f;
        a = 1.2666667f;
        a2 = 3.4f;
        a3 = -4.6666665f;
        break;
    }

    xPar* p = ps->m_root;
    while (p)
    {
        float t = p->m_lifetime / p->m_asset->life;
        if (t < 0.0f)
        {
            t = 0.0f;
        }
        else if (t > 1.0f)
        {
            t = 1.0f;
        }
        t = 1.0f - t;
        float t2 = t * t;
        float alpha = 255.0f * (constant + a * t + a2 * t2 + a3 * (t2 * t));
        if (alpha > 255.0f)
        {
            alpha = 255.0f;
        }
        p->m_c[3] = alpha;
        p->m_cvel[3] = 0.0f;
        p = p->m_next;
    }
}

// TODO: 98.5%. Same register allocation difference as Scale3rdPolyReg.
void xParCmdScale_Update(zParCmd* c, xParGroup* ps, float dt)
{
    float constant;
    float a;
    float a2;
    float a3;
    float t;
    float t2;
    xPar* p;

    switch (((xParCmdCurve*)c->tasset)->curve)
    {
    case 0:
        constant = 0.0f;
        a = 1.266667f;
        a2 = 3.4f;
        a3 = -4.666667f;
        break;
    case 1:
        constant = 0.2f;
        a = 5.615805f;
        a2 = -11.646663f;
        a3 = 5.830858f;
        break;
    }

    p = ps->m_root;
    while (p)
    {
        t = p->m_lifetime / p->m_asset->life;
        if (t < 0.0f)
        {
            t = 0.0f;
        }
        else if (t > 1.0f)
        {
            t = 1.0f;
        }
        t = 1.0f - t;
        t2 = t * t;
        p->m_size = (constant + a * t + a2 * t2 + a3 * (t2 * t)) * p->m_asset->size_death;
        p->m_sizeVel = 0.0f;
        p = p->m_next;
    }
}

void xParCmdClipVolumes_Update(zParCmd* c, xParGroup* ps, float dt)
{
    for (xPar* p = ps->m_root; p != 0; p = p->m_next)
    {
        for (signed int i = 0; i < sClipVolumeTotal; i++)
        {
            _xCollis collis;
            xVecHitsBound(&p->m_pos, &sClipVolume[i]->asset->bound, &collis);
            if (collis.flags & 1)
            {
                p->m_lifetime = -1.0f;
            }
        }
    }
}

void xParCmdAlpha3rdPolyReg_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xParCmdAlpha3rdPolyReg* cmd = (xParCmdAlpha3rdPolyReg*)c->tasset;
    xPar* p = ps->m_root;

    while (p)
    {
        float t = p->m_lifetime / p->m_asset->life;
        if (t < 0.0f)
        {
            t = 0.0f;
        }
        else if (t > 1.0f)
        {
            t = 1.0f;
        }
        t = 1.0f - t;
        float t2 = t * t;
        float alpha = 255.0f * (cmd->constant + cmd->a * t + cmd->a2 * t2 + cmd->a3 * (t2 * t));
        if (alpha > 255.0f)
        {
            alpha = 255.0f;
        }
        p->m_c[3] = alpha;
        p->m_cvel[3] = 0.0f;
        p = p->m_next;
    }
}

void xParCmdPlayerCollision_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xPar* p;
    xParCmdPlayerCollision* cmd = (xParCmdPlayerCollision*)c->tasset;
    p = ps->m_root;

    while (p)
    {
        float t = p->m_lifetime / p->m_asset->life;
        if (t < 0.0f)
        {
            t = 0.0f;
        }
        else if (t > 1.0f)
        {
            t = 1.0f;
        }
        t = 1.0f - t;
        if (t >= cmd->min && t <= cmd->max)
        {
            _xRay3 ray;
            xVec3Copy(&ray.origin, &p->m_pos);
            xVec3Copy(&ray.dir, &p->m_vel);
            ray.max_t = xVec3Normalize(&ray.dir, &ray.dir);
            ray.min_t = ray.max_t * cmd->min;
            if (cmd->max < ray.max_t)
            {
                ray.max_t = cmd->max;
            }
            if (zRayHitsPlayer(&ray))
            {
                p->m_lifetime = -1.0f;
            }
        }
        p = p->m_next;
    }
}

void xParCmdRandomVelocityPar_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xParCmdRandomVelocityPar* cmd = (xParCmdRandomVelocityPar*)c->tasset;
    xPar* p = ps->m_root;
    float x = cmd->x * dt;
    float y = cmd->y * dt;
    float z = cmd->z * dt;

    while (p)
    {
        _xMat3x3 rot;
        float ry = 2.0f * (x * iurand()) - x;
        float rx = 2.0f * (y * iurand()) - y;
        float rz = 2.0f * (z * iurand()) - z;
        xMat3x3Euler(&rot, rx, ry, rz);
        xMat3x3LMulVec(&p->m_vel, &rot, &p->m_vel);
        p = p->m_next;
    }
}

void xParCmdApplyWind_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xParCmdApplyWind* cmd = (xParCmdApplyWind*)c->tasset;
    _xVec3 wind_vec;

    xClimateGetRealWindDir(&gClimate, &wind_vec);
    float s = cmd->strength * dt;
    wind_vec.x *= s;
    wind_vec.z *= s;

    for (xPar* p = ps->m_root; p != 0; p = p->m_next)
    {
        p->m_vel.x += wind_vec.x;
        p->m_vel.z += wind_vec.z;
    }
}

void xParCmdRotPar_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xParCmdRotPar* cmd = (xParCmdRotPar*)c->tasset;
    xPar* p = ps->m_root;
    float rx = 255.0f * ((cmd->max.x - cmd->min.x) / 360.0f);
    float ry = 255.0f * ((cmd->max.y - cmd->min.y) / 360.0f);
    float rz = 255.0f * ((cmd->max.z - cmd->min.z) / 360.0f);
    float bx = 255.0f * (cmd->min.x / 360.0f);
    float by = 255.0f * (cmd->min.y / 360.0f);
    float bz = 255.0f * (cmd->min.z / 360.0f);

    while (p)
    {
        p->m_rotdeg[0] += dt * (rx * iurand() + bx);
        p->m_rotdeg[1] += dt * (ry * iurand() + by);
        p->m_rotdeg[2] += dt * (rz * iurand() + bz);
        p = p->m_next;
    }
}

void xParCmdVelocityApply_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xPar* p = ps->m_root;

    while (p)
    {
        p->m_pos.x += p->m_vel.x;
        p->m_pos.y += p->m_vel.y;
        p->m_pos.z += p->m_vel.z;
        p = p->m_next;
    }
}

void xParCmdApplyCamMat_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xParCmdApplyCamMat* cmd = (xParCmdApplyCamMat*)c->tasset;
    _xMat4x3* mat = &globals.camera.mat;
    xPar* p;

    if (cmd->apply.x != 0.0f)
    {
        float mul = dt * cmd->apply.x;
        for (p = ps->m_root; p != 0; p = p->m_next)
        {
            p->m_vel.x += mat->right.x * mul;
            p->m_vel.y += mat->right.y * mul;
            p->m_vel.z += mat->right.z * mul;
        }
    }
    if (cmd->apply.y != 0.0f)
    {
        float mul = dt * cmd->apply.y;
        for (p = ps->m_root; p != 0; p = p->m_next)
        {
            p->m_vel.x += mat->up.x * mul;
            p->m_vel.y += mat->up.y * mul;
            p->m_vel.z += mat->up.z * mul;
        }
    }
    if (cmd->apply.z != 0.0f)
    {
        float mul = dt * cmd->apply.z;
        for (p = ps->m_root; p != 0; p = p->m_next)
        {
            p->m_vel.x += mat->at.x * mul;
            p->m_vel.y += mat->at.y * mul;
            p->m_vel.z += mat->at.z * mul;
        }
    }
}

void xParCmdAnimalMagentism_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xPar* p = ps->m_root;
    _xVec3 pos = *(_xVec3*)&globals.player.ent.model->Mat->pos;
    pos.y += 1.0f;
    float mul = dt * -((xParCmdAge*)c->tasset)->age_rate;

    while (p)
    {
        _xVec3 vec;
        vec.x = p->m_pos.x - pos.x;
        vec.y = p->m_pos.y - pos.y;
        vec.z = p->m_pos.z - pos.z;
        float dist = xVec3Normalize(&vec, &vec);

        p->m_vel.x += vec.x * mul;
        p->m_vel.y += vec.y * mul;
        p->m_vel.z += vec.z * mul;
        if (dist < 0.25f)
        {
            p->m_lifetime = -1.0f;
        }
        else if (dist < 2.0f)
        {
            dist *= 0.49f;
            p->m_vel.x *= dist;
            p->m_vel.y *= dist;
            p->m_vel.z *= dist;
        }
        p = p->m_next;
    }
}

void xParCmdDamagePlayer_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xPar* p;
    signed int i;
    static signed int last_idx = 0.0f;
    _xBound* pbound = &globals.player.ent.bound;

    last_idx++;
    if (last_idx >= 10)
    {
        last_idx = 0;
    }

    p = ps->m_root;
    for (i = 0; i < last_idx; i++)
    {
        if (p)
        {
            p = p->m_next;
        }
    }

    while (p)
    {
        if (p->m_lifetime > 0.1f)
        {
            _xCollis collis = { 0 };
            xVecHitsBound(&p->m_pos, pbound, &collis);
            if (collis.flags & 1)
            {
                zEntPlayer_Damage(0, 1, 0);
            }
        }
        for (i = 0; i < 10; i++)
        {
            if (p)
            {
                p = p->m_next;
            }
        }
    }
}

void xParCmdRotateAround_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xPar* p;
    xParCmdRotateAround* cmd = (xParCmdRotateAround*)c->tasset;
    float yaw = PI * (dt * cmd->yaw) / 180.0f;
    float radius_growth = dt * cmd->radius_growth;
    p = ps->m_root;

    while (p)
    {
        _xMat3x3 lookmat;
        _xMat3x3 rotmat;
        _xVec3 offset;
        _xVec3 angles;
        _xVec3 at;
        _xVec3 out_offset;

        xVec3Sub(&offset, &cmd->pos, &p->m_pos);
        offset.y = 0.0f;
        float radius = xMat3x3LookVec(&lookmat, &offset);
        xMat3x3GetEuler(&lookmat, &angles);
        angles.x += yaw;
        xMat3x3Euler(&rotmat, angles.x, angles.y, angles.z);
        at.x = 0.0f;
        at.y = 0.0f;
        at.z = radius + radius_growth;
        xMat3x3RMulVec(&out_offset, &rotmat, &at);
        p->m_pos.x = out_offset.x + cmd->pos.x;
        p->m_pos.z = out_offset.z + cmd->pos.z;
        p = p->m_next;
    }
}

// TODO: 95.5%. dx/dz and the sqrt temporaries get different scratch registers.
void xParCmdJet_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xParCmdJet* cmd = (xParCmdJet*)c->tasset;
    float cx;
    float cy;
    float cz;
    float mdt;
    float oorSqr;
    float rSqr;
    float dx;
    float dy;
    float dz;

    mdt = cmd->gravity * dt;
    if (cmd->mode == 0)
    {
        _xVec3* pos = (_xVec3*)&globals.player.ent.model->Mat->pos;
        cx = pos->x;
        cy = pos->y;
        cz = pos->z;
    }
    else
    {
        cx = cmd->center.x;
        cy = cmd->center.y;
        cz = cmd->center.z;
    }

    for (xPar* p = ps->m_root; p != 0; p = p->m_next)
    {
        dy = cy - p->m_pos.y;
        dx = cx - p->m_pos.x;
        dz = cz - p->m_pos.z;
        rSqr = dx * dx + dy * dy + dz * dz;
        if (rSqr < cmd->radiusSqr)
        {
            oorSqr = mdt / xsqrt(rSqr + cmd->epsilon);
            float sx = (dx < 0.0f) ? 1.0f : -1.0f;
            float sz = (dz < 0.0f) ? 1.0f : -1.0f;
            p->m_vel.x += sx * (cmd->acc.x * oorSqr);
            p->m_vel.y += cmd->acc.y * oorSqr;
            p->m_vel.z += sz * (cmd->acc.z * oorSqr);
        }
    }
}

void xParCmdTex_Update(zParCmd* c, xParGroup* ps, float dt)
{
}

void xParCmdTexAnim_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xPar* p;
    xParCmdTex* tex = ps->m_cmdTex;
    xParCmdTexAnim* cmd = (xParCmdTexAnim*)c->tasset;

    if (tex == 0)
    {
        return;
    }

    if (cmd->throttle_time > 0.0f)
    {
        cmd->throttle_time_elapsed -= dt;
        if (cmd->throttle_time_elapsed > 0.0f)
        {
            return;
        }
        cmd->throttle_time_elapsed = cmd->throttle_time;
    }

    p = ps->m_root;
    if (cmd->anim_mode == 0)
    {
        while (p)
        {
            p->m_texIdx[0] = irand() % tex->cols;
            p->m_texIdx[1] = irand() % tex->rows;
            p = p->m_next;
        }
    }
    else if (cmd->anim_mode == 1)
    {
        if (tex->cols > 1)
        {
            while (p)
            {
                p->m_texIdx[0]++;
                if (p->m_texIdx[0] >= tex->cols)
                {
                    p->m_texIdx[0] = 0;
                    if (cmd->anim_wrap_mode == 1)
                    {
                        p->m_texIdx[1]++;
                        if (p->m_texIdx[1] >= tex->rows)
                        {
                            p->m_texIdx[1] = 0;
                        }
                    }
                    else if (cmd->anim_wrap_mode == 2)
                    {
                        if (p->m_texIdx[1] == 0)
                        {
                            p->m_texIdx[1] = tex->rows - 1;
                        }
                        else
                        {
                            p->m_texIdx[1]--;
                        }
                    }
                    else if (cmd->anim_wrap_mode == 3)
                    {
                        p->m_texIdx[1]++;
                        if (p->m_texIdx[1] >= tex->rows)
                        {
                            p->m_texIdx[1] = tex->rows - 1;
                            p->m_texIdx[0] = tex->cols - 1;
                        }
                    }
                }
                p = p->m_next;
            }
        }
    }
    else if (cmd->anim_mode == 2)
    {
        if (tex->cols > 1)
        {
            while (p)
            {
                if (p->m_texIdx[0] == 0)
                {
                    p->m_texIdx[0] = tex->cols - 1;
                    if (cmd->anim_wrap_mode == 1)
                    {
                        p->m_texIdx[1]++;
                        if (p->m_texIdx[1] >= tex->rows)
                        {
                            p->m_texIdx[1] = 0;
                        }
                    }
                    else if (cmd->anim_wrap_mode == 2)
                    {
                        if (p->m_texIdx[1] == 0)
                        {
                            p->m_texIdx[1] = tex->rows - 1;
                        }
                        else
                        {
                            p->m_texIdx[1]--;
                        }
                    }
                }
                else
                {
                    p->m_texIdx[0]--;
                }
                p = p->m_next;
            }
        }
    }
    else if (cmd->anim_mode == 3)
    {
        if (tex->rows > 1)
        {
            while (p)
            {
                if (p->m_texIdx[1] == 0)
                {
                    p->m_texIdx[1] = tex->rows - 1;
                    if (cmd->anim_wrap_mode == 1)
                    {
                        p->m_texIdx[0]++;
                        if (p->m_texIdx[0] >= tex->cols)
                        {
                            p->m_texIdx[0] = 0;
                        }
                    }
                    else if (cmd->anim_wrap_mode == 2)
                    {
                        if (p->m_texIdx[0] == 0)
                        {
                            p->m_texIdx[0] = tex->cols - 1;
                        }
                        else
                        {
                            p->m_texIdx[0]--;
                        }
                    }
                }
                else
                {
                    p->m_texIdx[1]--;
                }
                p = p->m_next;
            }
        }
    }
    else if (cmd->anim_mode == 4)
    {
        if (tex->rows > 1)
        {
            while (p)
            {
                p->m_texIdx[1]++;
                if (p->m_texIdx[1] >= tex->rows)
                {
                    p->m_texIdx[1] = 0;
                    if (cmd->anim_wrap_mode == 1)
                    {
                        p->m_texIdx[0]++;
                        if (p->m_texIdx[0] >= tex->cols)
                        {
                            p->m_texIdx[0] = 0;
                        }
                    }
                    else if (cmd->anim_wrap_mode == 2)
                    {
                        if (p->m_texIdx[0] == 0)
                        {
                            p->m_texIdx[0] = tex->cols - 1;
                        }
                        else
                        {
                            p->m_texIdx[0]--;
                        }
                    }
                }
                p = p->m_next;
            }
        }
    }
}

// TODO: 99.4%. Register allocation in the case 10 loop.
void xParCmdCustom_Update(zParCmd* c, xParGroup* ps, float dt)
{
    xPar* p = ps->m_root;
    float cx;
    float dx;
    float cy;
    float grow;
    float cz;
    float rSqr;
    float oorSqr;
    float dy;
    float dz;
    float mdt;
    float sx;
    float sz;
    _xVec3* pos;

    switch (((xParCmdCustom*)c->tasset)->kind)
    {
    case 0:
        mdt = 4.0f * dt;
        pos = (_xVec3*)&globals.player.ent.model->Mat->pos;
        cx = pos->x;
        cy = pos->y;
        cz = pos->z;
        while (p)
        {
            dy = cy - p->m_pos.y;
            dx = cx - p->m_pos.x;
            dz = cz - p->m_pos.z;
            rSqr = dx * dx + dy * dy + dz * dz;
            if (rSqr < 16.0f)
            {
                oorSqr = mdt / xsqrt(rSqr + 0.001f);
                sx = (dx < 0.0f) ? 1.0f : -1.0f;
                sz = (dz < 0.0f) ? 1.0f : -1.0f;
                p->m_size = 1.0f / (10.0f * oorSqr * (sx + sz));
            }
            p = p->m_next;
        }
        break;
    case 2:
        while (p)
        {
            p->m_vel.x = 0.0f;
            p->m_vel.y = 0.0f;
            p->m_vel.z = 0.0f;
            p = p->m_next;
        }
        break;
    case 10:
        mdt = 4.0f * dt;
        grow = 0.01f * dt;
        pos = (_xVec3*)&globals.player.ent.model->Mat->pos;
        cx = pos->x;
        cy = pos->y;
        cz = pos->z;
        while (p)
        {
            dy = cy - p->m_pos.y;
            dx = cx - p->m_pos.x;
            dz = cz - p->m_pos.z;
            if (dx * dx + dy * dy + dz * dz < 2.0f)
            {
                p->m_size -= mdt;
                if (p->m_size < 0.1f)
                {
                    p->m_size = 0.1f;
                }
            }
            else
            {
                p->m_size += grow;
                if (p->m_size > 1.0f)
                {
                    p->m_size = 1.0f;
                }
            }
            p = p->m_next;
        }
        break;
    }
}

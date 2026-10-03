#include "zGlobals.h"
#include "zScene.h"
#include "zParEmitter.h"
#include "xCollide.h"
#include "xMath.h"
#include "xMemMgr.h"
#include "xString.h"
#include "xstransvc.h"
#include "xParGroup.h"
#include "xPar.h"

extern "C" void* memcpy(void* dst, const void* src, unsigned int n);
extern "C" void* memset(void* dst, int c, unsigned int n);

class RpMaterial
{
public:
    void* texture; // 0x0
    RwRGBA color;  // 0x4
};

class RpGeometry
{
public:
    unsigned char pad0[0x20];
    RpMaterial** materials; // 0x20
};

class RpAtomic
{
public:
    unsigned char pad0[0x18];
    RpGeometry* geometry;   // 0x18
    _xVec3 boundingCenter;  // 0x1C
    float boundingRadius;   // 0x28
};

class _xEntCollis
{
public:
    unsigned char chk;       // 0x0
    unsigned char pen;       // 0x1
    unsigned char env_sidx;  // 0x2
    unsigned char env_eidx;  // 0x3
    unsigned char npc_sidx;  // 0x4
    unsigned char npc_eidx;  // 0x5
    unsigned char dyn_sidx;  // 0x6
    unsigned char dyn_eidx;  // 0x7
    unsigned char stat_sidx; // 0x8
    unsigned char stat_eidx; // 0x9
    unsigned char trig_idx;  // 0xA
    unsigned char idx;       // 0xB
    _xCollis colls[18];      // 0xC
    void (*post)(_xEnt*, _xScene*, float, _xEntCollis*);                    // 0x4D4
    unsigned int (*depenq)(_xEnt*, _xEnt*, _xScene*, float, _xCollis*);     // 0x4D8
    unsigned char trigs_cur[8];     // 0x4DC
    unsigned char trigs_prev[8];    // 0x4E4
    unsigned char trigs_entered[8]; // 0x4EC
    unsigned char trigs_exited[8];  // 0x4F4
};

class _zEntBubble : public _zEnt
{
public:
    unsigned int state;   // 0xEC
    _zEnt* villain;       // 0xF0
    float bubbleScale;    // 0xF4
    float timer;          // 0xF8
    float stuckTimer;     // 0xFC
    _xVec3 envelopStart;  // 0x100
    float fx_timer;       // 0x10C
    float fx_scale;       // 0x110
    xPar* particle;       // 0x114
};

class st_zEntVillain;

enum en_ZVILLAIN_BUBBLE_RESPONSE
{
    ZVILLAIN_BUBBLE_IGNORE = 0,
    ZVILLAIN_BUBBLE_ENVELOP = 1,
    ZVILLAIN_BUBBLE_POP = 2
};

en_ZVILLAIN_BUBBLE_RESPONSE zEntVillain_Bubble(st_zEntVillain* ent, _zEntBubble* bub);
void xEntBeginUpdate(_xEnt* ent, _xScene* sc, float dt);
void xEntEndUpdate(_xEnt* ent, _xScene* sc, float dt);
void xEntApplyPhysics(_xEnt* ent, _xScene* sc, float dt);
void xEntMove(_xEnt* ent, _xScene* sc, float dt);
void xEntCollide(_xEnt* ent, _xScene* sc, float dt);
void xEntDefaultBoundUpdate(_xEnt* ent, _xVec3* pos);
xModelInstance* xModelInstanceAlloc(RpAtomic* data, void* object, unsigned short flags,
                                    unsigned char boneIndex, unsigned char* boneRemap);
void xModelRender(xModelInstance* modelInst);


static inline float xlerp(float a, float b, float t)
{
    return t * (b - a) + a;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(x, lo, hi) (MIN(x, hi) < (lo) ? (lo) : MIN(x, hi))

#define BUBBLE_MAT(ent) ((_xMat4x3*)(ent)->model->Mat)

static RpAtomic* gBubbleModel;
static _zParEmitter* sEmitGumWad;
static _zParEmitter* sEmitSoapFlight;

void zEntBubble_Update(_zEntBubble* ent, _xScene* sc, float dt);
static void zEntBubble_FX(_zEntBubble* ent, float dt);
void zEntBubble_Move(_zEntBubble* ent, _xScene* sc, float dt, _xEntFrame* frame);

signed int zEntBubbleEventCB(xBase*, xBase*, unsigned int, float*, xBase*);

void zEntBubble_SceneInit(zScene* scene)
{
    unsigned int bufsize;
    _zEntBubble* bubbleBlock = (_zEntBubble*)xMemAlloc(gActiveHeap, 20 * sizeof(_zEntBubble));
    _xEntFrame* frameBlock = (_xEntFrame*)xMemAlloc(gActiveHeap, 20 * sizeof(_xEntFrame));

    scene->bubbles = (_zEnt**)xMemAlloc(gActiveHeap, 20 * sizeof(_zEnt*));
    scene->num_bubbles = 0;

    gBubbleModel = (RpAtomic*)xSTFindAsset(xStrHash("sphere.dff"), &bufsize);

    _zEntBubble* b;
    for (unsigned int idx = 0; idx < 20; idx++)
    {
        b = &bubbleBlock[idx];
        scene->bubbles[idx] = b;

        b->id = 0;
        b->baseType = 0xF;
        b->baseFlags = 5;
        b->linkCount = 0;
        b->link = 0;
        b->eventFunc = zEntBubbleEventCB;
        b->flags = 1;
        b->collType = 0xFF;
        b->collLev = 4;
        b->pflags = 3;
        b->chkby = 0x10;
        b->penby = 0x10;
        b->idx = idx;
        b->model = xModelInstanceAlloc(gBubbleModel, b, 0, 0, 0);
        b->update = (void (*)(_xEnt*, _xScene*, float))zEntBubble_Update;
        b->bupdate = xEntDefaultBoundUpdate;
        b->move = (void (*)(_xEnt*, _xScene*, float, _xEntFrame*))zEntBubble_Move;
        b->frame = &frameBlock[idx];
        b->collis = 0;
        b->bound.type = 1;
        b->collLev = 4;
    }

    sEmitGumWad = zParEmitterFind("PAREMIT_GUMWAD");
    if (sEmitGumWad && sEmitGumWad->group)
    {
        xParGroupSetStealing(sEmitGumWad->group, 0);
    }

    sEmitSoapFlight = zParEmitterFind("PAREMIT_SOAP_FLIGHT");
    if (sEmitSoapFlight && sEmitSoapFlight->group)
    {
        xParGroupSetStealing(sEmitSoapFlight->group, 0);
    }
}

// TODO: 99.89%, the player frame and scene pointers load into swapped temporaries.
static _zEnt* AutoTarget()
{
    unsigned int i;
    float dirAngle;
    float minusAngle;
    float plusAngle;
    float pitchSin;
    float dx;
    _zEnt* near;
    _zEnt** vill;
    float px;
    float plusSin;
    float py;
    float pz;
    float dirSin;
    float dirCos;
    float minusSin;
    float minusCos;
    float plusCos;
    float nearDist;
    float dy;
    float dz;
    float dist;
    float xzdist;
    float spitch;

    near = 0;
    dirAngle = globals.player.ent.frame->rot.angle;
    minusAngle = dirAngle - 0.017453292f * globals.player.s.FireTargetYaw - 1.5707964f;
    plusAngle = 1.5707964f + (dirAngle + 0.017453292f * globals.player.s.FireTargetYaw);
    nearDist = 0.0f;
    vill = (_zEnt**)globals.sceneCur->npcs;
    px = globals.player.ent.bound.sph.center.x;
    py = globals.player.ent.bound.sph.center.y;
    pz = globals.player.ent.bound.sph.center.z;
    dirSin = isin(dirAngle);
    dirCos = icos(dirAngle);
    minusSin = isin(minusAngle);
    minusCos = icos(minusAngle);
    plusSin = isin(plusAngle);
    plusCos = icos(plusAngle);
    pitchSin = isin(0.017453292f * globals.player.s.FireTargetPitch);

    for (i = 0; i < globals.sceneCur->num_npcs; i++)
    {
        if (!xEntIsVisible(vill[i]))
        {
            continue;
        }
        if (!(vill[i]->baseFlags & 1))
        {
            continue;
        }

        dz = vill[i]->bound.sph.center.z - pz;
        dx = vill[i]->bound.sph.center.x - px;
        dy = vill[i]->bound.sph.center.y - py;

        dist = dx * dirSin + dz * dirCos;
        if (dist > globals.player.s.FireTargetDist &&
            dx * minusSin + dz * minusCos < globals.player.s.FireTargetRadius &&
            dx * plusSin + dz * plusCos < globals.player.s.FireTargetRadius)
        {
            xzdist = xsqrt(dx * dx + dy * dy + dz * dz);
            spitch = __fabs(dy / xzdist);
            if (spitch <= pitchSin)
            {
                if (!near || dist < nearDist)
                {
                    near = vill[i];
                    nearDist = dist;
                }
            }
        }
    }

    globals.player.FireTarget = near;
    return near;
}

void zEntBubble_EnableCollision()
{
    signed int i;
    zScene* sc = globals.sceneCur;

    _zEnt** bubbles = sc->bubbles;
    for (i = 0; i < (signed int)sc->num_bubbles; i++)
    {
        if (((_zEntBubble*)bubbles[i])->state == 0x10 && ((_zEntBubble*)bubbles[i])->timer != 0.0f && sc->num_dyns < sc->num_dyns_allocd)
        {
            sc->dyns[sc->num_dyns++] = bubbles[i];
        }
    }
}

void zEntBubble_DisableCollision()
{
    signed int i;
    zScene* sc = globals.sceneCur;

    for (i = 0; i < sc->num_dyns; i++)
    {
        if (sc->dyns[i]->baseType == 0xF)
        {
            sc->num_dyns = i;
            return;
        }
    }
}

void zEntBubble_Launch(float wait)
{
    zScene* sc = globals.sceneCur;

    if (sc->num_bubbles < 20)
    {
        _zEntBubble* bub = (_zEntBubble*)sc->bubbles[sc->num_bubbles++];

        bub->subType = globals.player.LastProjectile;
        bub->fx_timer = 0.0f;
        memcpy(BUBBLE_MAT(bub), &g_I3, sizeof(_xMat4x3));
        BUBBLE_MAT(bub)->pos.x = bub->bound.sph.center.x = globals.player.ent.bound.sph.center.x;
        BUBBLE_MAT(bub)->pos.y = bub->bound.sph.center.y = globals.player.ent.bound.sph.center.y;
        BUBBLE_MAT(bub)->pos.z = bub->bound.sph.center.z = globals.player.ent.bound.sph.center.z;
        bub->bound.sph.r = 0.5f;
        memcpy(&bub->frame->mat, BUBBLE_MAT(bub), sizeof(_xMat4x3));
        bub->frame->rot.axis.x = 0.0f;
        bub->frame->rot.axis.y = 1.0f;
        bub->frame->rot.axis.z = 0.0f;
        bub->frame->rot.angle = 0.0f;
        bub->frame->vel.x = 0.0f;
        bub->frame->vel.y = 0.0f;
        bub->frame->vel.z = 0.0f;
        bub->state = 1;
        bub->villain = AutoTarget();
        bub->bubbleScale = 1.0f;
        bub->timer = wait;
        bub->stuckTimer = 0.0f;
        bub->fx_scale = 0.25f;
        bub->model->Scale.x = bub->fx_scale;
        bub->model->Scale.y = bub->fx_scale;
        bub->model->Scale.z = bub->fx_scale;
        bub->model->Flags &= ~1;

        globals.player.Inv_Ammo[globals.player.LastProjectile] =
            CLAMP(globals.player.Inv_Ammo[globals.player.LastProjectile] - 1, 0,
                  globals.player.Inv_AmmoMax[globals.player.LastProjectile]);

        if (globals.player.LastProjectile == 1)
        {
            bub->pflags |= 0x20;
            bub->particle = 0;
        }
        else
        {
            bub->pflags &= 0xDF;
        }
    }
}

// TODO: 99.88%, case 0x10 colours envelopStart and the villain position into swapped
// float registers.
void zEntBubble_Update(_zEntBubble* ent, _xScene* sc, float dt)
{
    _xEntCollis coll;
    _zParEmitterCustomSettings info;

    memset(&coll, 0, sizeof(coll));
    coll.chk = 0x2E;
    ent->collis = &coll;

    switch (ent->state)
    {
    case 1:
        ent->timer -= dt;
        if (ent->timer < 0.0f)
        {
            float flyspeed;

            ent->state = 4;
            flyspeed = 10.0f;

            iModelTagEval((RpAtomic*)globals.player.ent.model->Data, &globals.player.BubbleTag,
                          globals.player.ent.model->Mat, &BUBBLE_MAT(ent)->pos);
            memcpy(&ent->frame->mat.pos, &BUBBLE_MAT(ent)->pos, sizeof(_xVec3));
            globals.player.FireTarget = 0;

            ent->frame->vel.x = isin(globals.player.ent.frame->rot.angle);
            ent->frame->vel.y = 0.0f;
            ent->frame->vel.z = icos(globals.player.ent.frame->rot.angle);

            if (ent->villain)
            {
                float dx = ent->villain->bound.sph.center.x - BUBBLE_MAT(ent)->pos.x;
                float dy = ent->villain->bound.sph.center.y - BUBBLE_MAT(ent)->pos.y;
                float dz = ent->villain->bound.sph.center.z - BUBBLE_MAT(ent)->pos.z;
                float mag = xsqrt(dx * dx + dy * dy + dz * dz);

                if (mag > 1e-6f)
                {
                    float spitch = dy / mag;
                    float cpitch = 1.0f - spitch * spitch;
                    cpitch = (cpitch < 0.0f) ? 0.0f : xsqrt(cpitch);

                    ent->frame->vel.x *= cpitch;
                    ent->frame->vel.y = spitch;
                    ent->frame->vel.z *= cpitch;
                }
            }

            switch (ent->subType)
            {
            case 0:
                flyspeed = globals.player.s.FireGumSpeed;
                ent->timer = globals.player.s.FireGumTimer;
                if (sEmitGumWad)
                {
                    info.custom_flags = 0x20;
                    info.pos = BUBBLE_MAT(ent)->pos;
                    ent->particle = zParEmitterEmitCustom(sEmitGumWad, 1.0f / 30.0f, &info);
                }
                break;
            case 1:
                ent->model->Flags |= 1;
                flyspeed = globals.player.s.FireSoapSpeed;
                ent->timer = globals.player.s.FireSoapTimer;
                break;
            }

            ent->frame->vel.x *= flyspeed;
            ent->frame->vel.y *= flyspeed;
            ent->frame->vel.z *= flyspeed;
        }
        break;

    case 2:
        break;

    case 0x10:
    {
        float dy;
        float dz;
        float scale;
        float dx;
        float lerp;

        ent->timer += dt;
        ent->model->Scale.x = 0.0f;
        ent->model->Scale.y = 0.0f;
        ent->model->Scale.z = 0.0f;

        lerp = 5.0f * ent->timer;
        if (lerp > 1.0f)
        {
            lerp = 1.0f;
        }

        scale =
            xlerp(1.0f, 2.2f * ((RpAtomic*)ent->villain->model->Data)->boundingRadius, lerp);
        ent->model->Mat->at.z = scale;
        ent->model->Mat->up.y = scale;
        ent->model->Mat->right.x = scale;

        dx = ent->villain->bound.sph.center.x;
        dy = ent->villain->bound.sph.center.y;
        dz = ent->villain->bound.sph.center.z;
        ent->model->Mat->pos.x = lerp * (dx - ent->envelopStart.x) + ent->envelopStart.x;
        ent->model->Mat->pos.y = lerp * (dy - ent->envelopStart.y) + ent->envelopStart.y;
        ent->model->Mat->pos.z = lerp * (dz - ent->envelopStart.z) + ent->envelopStart.z;

        ent->bound.sph.r = 0.5f * ent->model->Mat->right.x;
        ent->bound.sph.center.x = dx;
        ent->bound.sph.center.y = dy;
        ent->bound.sph.center.z = dz;
        xBoundUpdate(&ent->bound);
        break;
    }

    case 4:
        xEntBeginUpdate(ent, sc, dt);

        zEntBubble_FX(ent, dt);

        ent->timer -= dt;
        if (ent->timer < 0.0f)
        {
            ent->state = 0x80;
            if (ent->particle)
            {
                ent->particle->m_lifetime = 0.0f;
                ent->particle = 0;
            }
            break;
        }

        if (ent->pflags & 0x2)
        {
            xEntApplyPhysics(ent, sc, dt);
        }
        if (ent->pflags & 0x1)
        {
            xEntMove(ent, sc, dt);
        }
        xEntCollide(ent, sc, dt);

        if (ent->collis->npc_eidx > ent->collis->npc_sidx)
        {
            _xCollis* coll = &ent->collis->colls[ent->collis->npc_sidx];
            _xCollis* endcoll = &ent->collis->colls[ent->collis->npc_eidx];

            for (; coll < endcoll; coll++)
            {
                if (ent->particle)
                {
                    ent->particle->m_lifetime = 0.0f;
                    ent->particle = 0;
                }

                _zEnt* vill = (_zEnt*)coll->optr;
                switch (zEntVillain_Bubble((st_zEntVillain*)vill, ent))
                {
                case ZVILLAIN_BUBBLE_IGNORE:
                    break;
                case ZVILLAIN_BUBBLE_ENVELOP:
                    ent->state = 0x10;
                    coll = endcoll;
                    ent->villain = vill;
                    ent->timer = 0.0f;
                    ent->envelopStart = ent->frame->mat.pos;
                    break;
                case ZVILLAIN_BUBBLE_POP:
                    ent->state = 8;
                    coll = endcoll;
                    ent->timer = 0.0f;
                    break;
                case 3:
                    break;
                }
            }
        }

        xEntEndUpdate(ent, sc, dt);

        {
            float dy;
            float dx;
            float dz;
            dx = ent->frame->oldpos.x - ent->frame->mat.pos.x;
            dy = ent->frame->oldpos.y - ent->frame->mat.pos.y;
            dz = ent->frame->oldpos.z - ent->frame->mat.pos.z;
            float dx2 = dx * dx;
            float dy2 = dy * dy;
            float dz2 = dz * dz;

            if (dz2 + (dx2 + dy2) < (0.5f * dt) * (0.5f * dt))
            {
                ent->stuckTimer += dt;
                if (ent->stuckTimer > 0.2f)
                {
                    ent->state = 8;
                }
            }
            else
            {
                ent->stuckTimer = 0.0f;
            }
        }
        break;

    case 3:
    case 8:
    case 0x40:
    default:
        ent->state = 0x80;
        break;
    }
}

#pragma auto_inline on
static void zEntBubble_FX(_zEntBubble* ent, float dt)
{
    if (ent->subType == 1)
    {
        ent->fx_timer -= dt;
        if (ent->fx_timer <= 0.0f && sEmitSoapFlight)
        {
            _zParEmitterCustomSettings info;
            info.custom_flags = 0x20;
            info.pos = BUBBLE_MAT(ent)->pos;
            ent->fx_timer = 0.0f;
            zParEmitterEmitCustom(sEmitSoapFlight, 1.0f / 30.0f, &info);
        }

        ent->fx_scale += dt;
        if (ent->fx_scale > 1.0f)
        {
            ent->fx_scale = 1.0f;
        }
        ent->model->Scale.x = ent->fx_scale;
        ent->model->Scale.y = ent->fx_scale;
        ent->model->Scale.z = ent->fx_scale;
    }
    else
    {
        ent->fx_timer -= dt;
        if (ent->fx_timer <= 0.0f)
        {
            ent->fx_timer = 0.1f;
        }

        if (ent->particle)
        {
            ent->particle->m_pos.x = BUBBLE_MAT(ent)->pos.x;
            ent->particle->m_pos.y = BUBBLE_MAT(ent)->pos.y;
            ent->particle->m_pos.z = BUBBLE_MAT(ent)->pos.z;
        }
    }
}
#pragma auto_inline off

void zEntBubble_Move(_zEntBubble* ent, _xScene* sc, float dt, _xEntFrame* frame)
{
    frame->mode = 0;
}

void zEntBubble_Pop(_zEntBubble* ent)
{
    ent->state = 0x40;
    ent->timer = 0.0f;
}

void zEntBubble_Cleanup()
{
    unsigned int i = 0;
    zScene* sc = globals.sceneCur;

    while (i < sc->num_bubbles)
    {
        if (((_zEntBubble*)sc->bubbles[i])->state == 0x80)
        {
            sc->num_bubbles--;
            _zEnt* tmp = sc->bubbles[i];
            sc->bubbles[i] = sc->bubbles[sc->num_bubbles];
            sc->bubbles[sc->num_bubbles] = tmp;
        }
        else
        {
            i++;
        }
    }
}

void zEntBubble_Render()
{
    unsigned int i;
    zScene* sc = globals.sceneCur;

    for (i = 0; i < sc->num_bubbles; i++)
    {
        if (sc->bubbles[i]->subType == 1)
        {
            RwRGBA colsoap = { 0x64, 0x64, 0x80, 0x80 };
            ((RpAtomic*)sc->bubbles[i]->model->Data)->geometry->materials[0]->color = colsoap;
            xModelRender(sc->bubbles[i]->model);
        }
    }
}

signed int zEntBubbleEventCB(xBase*, xBase*, unsigned int, float*, xBase*)
{
    return 0;
}

#include "zGlobals.h"
#include "zScene.h"
#include "zParEmitter.h"
#include "xEntMotion.h"
#include "xMovePoint.h"
#include "xCollide.h"
#include "xFFX.h"
#include "xAnim.h"
#include "xMemMgr.h"
#include "xstransvc.h"

extern "C" void* memcpy(void* dst, const void* src, unsigned int n);

class xSerial;

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
    void (*post)(_xEnt*, _xScene*, float, _xEntCollis*);                // 0x4D4
    unsigned int (*depenq)(_xEnt*, _xEnt*, _xScene*, float, _xCollis*); // 0x4D8
    unsigned char trigs_cur[8];     // 0x4DC
    unsigned char trigs_prev[8];    // 0x4E4
    unsigned char trigs_entered[8]; // 0x4EC
    unsigned char trigs_exited[8];  // 0x4F4
};

class p2PlatformAsset
{
public:
    unsigned char type;   // 0x0
    unsigned char pad;    // 0x1
    unsigned short flags; // 0x2
    union
    {
        struct
        {
            float speed; // 0x4
        } cb;
        struct
        {
            float ba_delay;           // 0x4
            unsigned int bustModelID; // 0x8
            float reset_delay;        // 0xC
        } ba;
        struct
        {
            float jmph[3];            // 0x4
            unsigned int animID[3];   // 0x10
            _xVec3 jmpdir;            // 0x1C
        } sb;
        struct
        {
            float itilt;   // 0x4
            float maxtilt; // 0x8
            float invmass; // 0xC
        } teet;
    };
};

class _zPlatform : public _zEnt
{
public:
    p2PlatformAsset* passet; // 0xEC
    _xEntMotion motion;      // 0xF0
    unsigned short state;    // 0x15C
    unsigned short plat_flags; // 0x15E
    float tmr;               // 0x160
    signed int ctr;          // 0x164
    _xMovePoint* src;        // 0x168
    xModelInstance* am;      // 0x16C
    xModelInstance* bm;      // 0x170
    signed int moving;       // 0x174
};

#define MOTION_DATA(plat, type, off) (*(type*)((unsigned char*)&(plat)->motion + (off)))

void xEntDefaultTranslate(_xEnt* ent, _xVec3* dpos);
void xEntLoadModel(_xEnt* ent, RpAtomic* imodel);
void xEntMechForward(_xEntMotion* motion);
void xEntMechReverse(_xEntMotion* motion);
void xMat3x3Tolocal(_xVec3* o, _xMat3x3* m, _xVec3* v);
xAnimPlay* xAnimPoolAlloc(xMemPool* pool, void* object, xAnimTable* table,
                          xModelInstance* modelInst);

extern const _xVec3 g_O3;

static _zParEmitter* sEmitTremble;
static _zParEmitter* sEmitBreakaway;

void zPlatform_Update(_xEnt* entplat, _xScene* sc, float dt);
void zPlatform_Move(_xEnt* entplat, _xScene* s, float dt, _xEntFrame* frame);
void zPlatformTranslate(_xEnt* xent, _xVec3* dpos);
signed int zPlatformEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                            xBase* toParamWidget);

void zPlatform_Init(_zPlatform* plat, p2EntAsset* asset);

void zPlatform_Init(void* plat, void* asset)
{
    zPlatform_Init((_zPlatform*)plat, (p2EntAsset*)asset);
}

void zPlatform_Init(_zPlatform* plat, p2EntAsset* asset)
{
    p2PlatformAsset* passet = (p2PlatformAsset*)(asset + 1);
    p2EntMotionAsset* masset = (p2EntMotionAsset*)((unsigned char*)passet + 0x28);

    zEntInit(plat, asset, 'PLAT');
    plat->passet = passet;
    plat->subType = passet->type;

    if (plat->linkCount)
    {
        plat->link = (p2LinkAsset*)((unsigned char*)masset + 0x30);
    }
    else
    {
        plat->link = 0;
    }

    plat->update = zPlatform_Update;
    plat->move = zPlatform_Move;
    plat->eventFunc = zPlatformEventCB;
    plat->transl = zPlatformTranslate;
    plat->am = 0;
    plat->bm = 0;
    plat->state = 0;
    plat->plat_flags = 0;

    if (plat->subType == 9)
    {
        unsigned int size;

        plat->collis = (_xEntCollis*)xMemAlloc(gActiveHeap, sizeof(_xEntCollis));
        plat->collis->chk = 0;
        plat->collis->pen = 0;
        plat->collis->post = 0;
        plat->am = plat->model;

        RpAtomic* bust = (RpAtomic*)xSTFindAsset(passet->ba.bustModelID, &size);
        if (bust)
        {
            xEntLoadModel(plat, bust);
            plat->bm = plat->model;
        }
        else
        {
            plat->bm = 0;
        }
        plat->model = plat->am;
    }
    else if (plat->subType == 10)
    {
        if (passet->sb.animID[0])
        {
            unsigned int size;
            void* data = xSTFindAsset(passet->sb.animID[0], &size);
            if (data)
            {
                xAnimFile* afile = xAnimFileNew(data, "", 0, 0);
                plat->atbl = xAnimTableNew("", 0);
                xAnimTableNewState(plat->atbl, "Idle", 0, 0, 1.0f, 0, 0, 0.0f, 0);
                xAnimTableAddFile(plat->atbl, afile, "Idle");
                xAnimPoolAlloc(&globals.sceneCur->mempool, plat, plat->atbl, plat->model);
            }
        }
    }

    xEntMotionInit(&plat->motion, plat, masset);
}

void zPlatform_Setup(_zPlatform* plat, _xScene* sc)
{
    zEntSetup(plat);
    xEntMotionSetup(&plat->motion, sc);
    sEmitTremble = zParEmitterFind("PAREMIT_PLAT_TREMBLE");
    sEmitBreakaway = zParEmitterFind("PAREMIT_PLAT_BREAKAWAY");
}

void zPlatform_Save(_zPlatform* ent, xSerial* s)
{
    zEntSave(ent, s);
}

void zPlatform_Load(_zPlatform* ent, xSerial* s)
{
    zEntLoad(ent, s);
}

static inline void zPlatform_BreakawayStop(_zPlatform* plat)
{
    plat->pflags &= 0xF9;
    plat->collis->chk = 0;
    plat->frame->vel = g_O3;
}

#pragma auto_inline on
void zPlatform_Reset(_zPlatform* plat, _xScene* sc)
{
    zEntReset(plat);
    xEntMotionReset(&plat->motion, sc);
    plat->plat_flags = 1;

    if (plat->subType == 0)
    {
        plat->state = 3;
    }
    else if (plat->subType == 9)
    {
        plat->tmr = plat->passet->ba.ba_delay;
        plat->state = 0;
        zPlatform_BreakawayStop(plat);
        plat->model = plat->am;
        plat->bound.mat = (_xMat4x3*)plat->model->Mat;
    }
    else if (plat->subType == 4)
    {
        plat->state = MOTION_DATA(plat, signed int, 0x48);
    }
    else if (plat->subType == 10)
    {
        plat->tmr = -1.0f;
        plat->ctr = 0;
    }

    if (plat->motion.type == 3)
    {
        plat->src = MOTION_DATA(plat, _xMovePoint*, 0x20);
    }

    if (!(plat->passet->flags & 0x4))
    {
        plat->chkby = 0;
    }
    else
    {
        plat->chkby = 0x18;
    }

    plat->moving = 0;
}
#pragma auto_inline off

static inline void zPlatform_TeeterRotate(_zPlatform* plat, float drot)
{
    plat->frame->rot.axis.z += drot;
    xMat3x3Euler((_xMat3x3*)plat->model->Mat, plat->frame->rot.axis.x, plat->frame->rot.axis.y,
                 plat->frame->rot.axis.z);

    p2EntAsset* asset = plat->asset;
    xVec3SMulBy(&((_xMat3x3*)plat->model->Mat)->right, asset->scale.x);
    xVec3SMulBy(&((_xMat3x3*)plat->model->Mat)->up, asset->scale.y);
    xVec3SMulBy(&((_xMat3x3*)plat->model->Mat)->at, asset->scale.z);
}

// TODO: 99.1%, the teeter's return-to-rest clamp keeps plat->frame in a register where
// the original reloads it.
void zPlatform_Update(_xEnt* entplat, _xScene* sc, float dt)
{
    _zPlatform* plat = (_zPlatform*)entplat;

    xEntUpdate(plat, sc, dt);

    if (plat->subType == 0)
    {
        plat->state = MOTION_DATA(plat, signed int, 0x54);
    }
    else if (plat->subType == 9)
    {
        if (plat->state == 1)
        {
            plat->tmr -= dt;
            if (plat->tmr <= 0.0f)
            {
                _zParEmitterCustomSettings info;
                info.custom_flags = 0x20;
                info.pos = *xBoundCenter(&plat->bound);
                info.pos.y += 0.5f;

                for (signed int i = 0; i < 25; i++)
                {
                    zParEmitterEmitCustom(sEmitBreakaway, 1.0f / 30.0f, &info);
                }

                plat->state = 2;
                plat->pflags |= 0x6;
                plat->tmr = plat->passet->ba.reset_delay;

                if (plat->bm)
                {
                    plat->model = plat->bm;
                    memcpy(plat->bm->Mat, plat->am->Mat, sizeof(_xMat4x3));
                    plat->bound.mat = (_xMat4x3*)plat->model->Mat;
                }

                zEntEvent(plat, 0x22);
            }
        }
        else if (plat->state == 2)
        {
            plat->tmr -= dt;
            if (plat->collis->colls[0].flags & 1)
            {
                plat->state = 3;
                zPlatform_BreakawayStop(plat);
            }
            else if (plat->tmr <= 0.0f)
            {
                zPlatform_Reset(plat, sc);
            }
            else if (plat->passet->ba.reset_delay - plat->tmr >= 25.0f)
            {
                plat->state = 4;
                zPlatform_BreakawayStop(plat);
            }
        }
        else if (plat->state == 3)
        {
            plat->tmr -= dt;
            if (plat->tmr <= 0.0f)
            {
                zPlatform_Reset(plat, sc);
            }
        }
    }
    else if (plat->subType == 4)
    {
        unsigned short state = MOTION_DATA(plat, signed int, 0x48);
        if (state != plat->state)
        {
            p2EntMotionAsset* masset = plat->motion.asset;
            if (plat->state == 2)
            {
                zEntEvent(plat, 0x39);
            }
            else if ((unsigned short)(state - 6) <= 1 ||
                     (!(masset->data[1] & 0x2) && (state == 0 || state == 1)))
            {
                zEntEvent(plat, 0x1F);
            }
            plat->state = state;
        }
    }
    else if (plat->subType == 10)
    {
        plat->tmr -= dt;
    }
    else if (plat->subType == 6)
    {
        _xEntDrive* drv = &globals.player.drv;
        if (plat == drv->odriver || plat == drv->driver)
        {
            float s = (plat == drv->driver) ? drv->s : drv->os;
            _xEnt* driven = drv->driven;
            xVec3SMul(&driven->frame->dpos, (_xVec3*)&plat->model->Mat->right,
                      dt * (s * plat->passet->cb.speed));
            xVec3AddTo((_xVec3*)&driven->model->Mat->pos, &driven->frame->dpos);
        }
    }
    else if (plat->subType == 11)
    {
        _xEntDrive* drv = &globals.player.drv;
        if (plat != drv->odriver && plat != drv->driver)
        {
            float step = (float)__fabs(((3.1415927f * plat->passet->teet.invmass) / 180.0f) * dt);
            float drot = step;
            float cur = plat->frame->rot.axis.z;
            float rest = plat->asset->ang.z;

            if (cur != rest)
            {
                if (cur < rest)
                {
                    float d = rest - cur;
                    step = (step < d) ? step : d;
                    drot = step;
                }
                else if (cur > rest)
                {
                    step = -step;
                    float d = rest - cur;
                    step = (step > d) ? step : d;
                    drot = step;
                }
                zPlatform_TeeterRotate(plat, drot);
            }
        }
        else
        {
            float s = (plat == drv->driver) ? drv->s : drv->os;
            _xVec3 local;
            local.x = drv->driven->frame->mat.pos.x - plat->model->Mat->pos.x;
            local.y = drv->driven->frame->mat.pos.y - plat->model->Mat->pos.y;
            local.z = drv->driven->frame->mat.pos.z - plat->model->Mat->pos.z;
            xMat3x3Tolocal(&local, (_xMat3x3*)plat->model->Mat, &local);

            float drot = dt * (local.x * (-s * ((3.1415927f * plat->passet->teet.invmass) / 180.0f)));
            float over = (float)__fabs(drot + plat->frame->rot.axis.z) -
                         (float)__fabs(plat->passet->teet.maxtilt);
            if (over > 0.0f)
            {
                if (drot < 0.0f)
                {
                    drot += over;
                }
                else if (drot > 0.0f)
                {
                    drot -= over;
                }
            }
            zPlatform_TeeterRotate(plat, drot);
        }
    }

    if (plat->motion.type == 3)
    {
        _xMovePoint* src = MOTION_DATA(plat, _xMovePoint*, 0x20);
        if (src != plat->src)
        {
            zEntEvent(plat, src, 0x1F);
            plat->src = src;
        }
    }

    signed int moving;
    _xEntFrame* frame = plat->frame;
    if (plat->subType == 11)
    {
        if (frame->oldrot.axis.x != frame->rot.axis.x || frame->oldrot.axis.y != frame->rot.axis.y ||
            frame->oldrot.axis.z != frame->rot.axis.z)
        {
            moving = 1;
        }
        else
        {
            moving = 0;
        }
    }
    else
    {
        if (frame->oldpos.x != frame->mat.pos.x || frame->oldpos.y != frame->mat.pos.y ||
            frame->oldpos.z != frame->mat.pos.z || frame->oldrot.axis.x != frame->rot.axis.x ||
            frame->oldrot.axis.y != frame->rot.axis.y || frame->oldrot.axis.z != frame->rot.axis.z)
        {
            moving = 1;
        }
        else
        {
            moving = 0;
        }
    }

    if (moving != plat->moving)
    {
        if (moving)
        {
            zEntEvent(plat, 0xE2);
        }
        else
        {
            zEntEvent(plat, 0xE3);
        }
    }
    plat->moving = moving;
}

void zPlatform_Move(_xEnt* entplat, _xScene* s, float dt, _xEntFrame* frame)
{
    xEntMotionMove(&((_zPlatform*)entplat)->motion, s, dt, frame);
}

void zPlatform_Shake(_zPlatform* plat, float, float ampl, float freq)
{
    xFFXShakeState* shake = xFFXShakeAlloc();
    if (shake)
    {
        xFFX* ffx = xFFXAlloc();
        if (!ffx)
        {
            xFFXShakeFree(shake);
            return;
        }

        shake->disp.x = 0.0f;
        shake->disp.y = -ampl;
        shake->disp.z = 0.0f;
        shake->dur = 1.0f;
        shake->alpha = -7.0f / shake->dur;
        shake->freq = freq;
        shake->tmr = 0.0f;
        ffx->doEffect = xFFXShakeUpdateEnt;
        ffx->fdata = shake;
        xFFXAddEffect(plat, ffx);
    }
}

// Dead-stripped on GameCube; only its 0.5f in the .sdata2 pool survives. The body is a
// stand-in.
void zPlatform_Mount(_zPlatform* plat)
{
    plat->tmr = 0.5f;
}

void zPlatform_Tremble(_zPlatform* plat, float ampl, float freq, float dur)
{
    if (plat->subType == 9)
    {
        if (plat->state == 0)
        {
            plat->state = 1;
        }

        float d = 1.0f + plat->passet->ba.ba_delay;
        xFFXShakeState* shake = xFFXShakeAlloc();
        if (shake)
        {
            xFFX* ffx = xFFXAlloc();
            if (!ffx)
            {
                xFFXShakeFree(shake);
                return;
            }

            xVec3SMul(&shake->disp, &globals.camera.mat.right, 0.06f);
            shake->dur = d;
            shake->alpha = 1.0f / d;
            shake->freq = 25.132742f;
            shake->tmr = 0.0f;
            ffx->doEffect = xFFXShakeUpdateEnt;
            ffx->fdata = shake;
            xFFXAddEffect(plat, ffx);
        }

        _zParEmitterCustomSettings info;
        info.custom_flags = 0x20;
        info.pos = *xBoundCenter(&plat->bound);
        for (signed int i = 0; i < 25; i++)
        {
            zParEmitterEmitCustom(sEmitTremble, 1.0f / 30.0f, &info);
        }
    }
}

void zPlatform_Dismount()
{
}

void zPlatform_MakeDiggable(_zPlatform* plat)
{
    plat->flags &= ~1;
    plat->chkby = 0;
    plat->passet->flags |= 0x2;
}

void zPlatformTranslate(_xEnt* xent, _xVec3* dpos)
{
    xEntDefaultTranslate(xent, dpos);
    xEntMotionTranslate(&((_zPlatform*)xent)->motion, dpos);
}

signed int zPlatformEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                            xBase* toParamWidget)
{
    _zPlatform* plat = (_zPlatform*)to;

    switch (toEvent)
    {
    case 0x26:
        if (!(plat->plat_flags & 1))
        {
            plat->plat_flags |= 1;
        }
        break;
    case 0x27:
        if (plat->plat_flags & 1)
        {
            plat->plat_flags &= ~1;
        }
        break;
    case 0xF:
        if (plat->plat_flags & 1)
        {
            plat->plat_flags &= ~1;
        }
        else
        {
            plat->plat_flags |= 1;
        }
        break;
    case 0x12:
        if (plat->subType == 4 && (MOTION_DATA(plat, signed int, 0x48) == 6 ||
                                   MOTION_DATA(plat, signed int, 0x48) == 7))
        {
            xEntMotionReset(&plat->motion, globals.sceneCur);
        }
        plat->motion.flags &= 0xFFFB;
        break;
    case 0x13:
        plat->motion.flags |= 0x4;
        break;
    case 0x12A:
        if (plat->subType == 4)
        {
            xEntMechForward(&plat->motion);
        }
        break;
    case 0x12B:
        if (plat->subType == 4)
        {
            xEntMechReverse(&plat->motion);
        }
        break;
    case 0xA:
        zPlatform_Reset(plat, globals.sceneCur);
        break;
    case 0x3:
        plat->flags |= 1;
        break;
    case 0x4:
        plat->flags &= ~1;
        break;
    case 0x55:
        plat->flags |= 1;
    case 0x53:
        plat->chkby = 0x18;
        break;
    case 0x56:
        plat->flags &= ~1;
    case 0x54:
        plat->chkby = 0;
        break;
    case 0xC3:
    case 0xC4:
    case 0xC5:
    case 0xC6:
    case 0xC7:
    case 0xC8:
    case 0xC9:
    case 0xCA:
        zEntAnimEvent(plat, toEvent, toParam);
        break;
    case 0xFC:
        plat->moreFlags |= 1;
        break;
    case 0xFD:
        plat->moreFlags &= ~1;
        plat->flags &= 0xFB;
        plat->fade_tmr = 0.0f;
        break;
    case 0xFE:
        plat->moreFlags ^= 1;
        if (!(plat->moreFlags & 1))
        {
            plat->flags &= 0xFB;
            plat->fade_tmr = 0.0f;
        }
        break;
    case 0xCB:
        if (plat->subType == 3)
        {
            float v = *toParam;
            MOTION_DATA(plat, float, 0x18) = (0.0f > v) ? 0.0f : v;
        }
        break;
    case 0xCC:
        if (plat->subType == 3)
        {
            MOTION_DATA(plat, float, 0x18) =
                (0.0f > MOTION_DATA(plat, float, 0x18) + *toParam)
                    ? 0.0f
                    : MOTION_DATA(plat, float, 0x18) + *toParam;
        }
        break;
    case 0x22:
        break;
    }

    return 1;
}

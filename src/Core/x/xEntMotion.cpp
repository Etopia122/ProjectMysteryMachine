#include "xEntMotion.h"
#include "xMath.h"
#include "xMath3.h"
#include "xMovePoint.h"
#include "xMemMgr.h"
#include "iDraw.h"
#include "zGlobals.h"

extern "C" {
void* memcpy(void* dst, const void* src, unsigned int n);
double acos(double x);
}

class xSpline3
{
public:
    unsigned short type;    // 0x0
    unsigned short flags;   // 0x2
    unsigned int N;         // 0x4
    unsigned int allocN;    // 0x8
    _xVec3* points;         // 0xC
    float* time;            // 0x10
    _xVec3* p12;            // 0x14
    _xVec3* bctrl;          // 0x18
    float* knot;            // 0x1C
    void* coef;             // 0x20
    unsigned int arcSample; // 0x24
    float* arcLength;       // 0x28
};

extern const iColor_tag g_NEON_RED;
extern const iColor_tag g_NEON_GREEN;
extern const iColor_tag g_PIMP_GOLD;
extern _xScene* g_xSceneCur;
extern _tagxPad* gDebugPad;

void xprintf(char* fmt, ...);
char* xSceneID2Name(_xScene* sc, unsigned int id);
void* xSceneResolvID(_xScene* sc, unsigned int id);
void xDebugModeAdd(char* name, void (*func)());
void xDrawSetColor(iColor_tag color);
void xDrawLine(_xVec3* a, _xVec3* b);
void xEntReset(_xEnt* ent);
_xVec3* xMovePointGetPos(_xMovePoint* m);
float xMovePointGetNext(_xMovePoint* current, _xMovePoint* prev, _xMovePoint** next,
                        _xVec3* hdng);
float xSpline3_EvalArcApprox(xSpline3* spl, float s, unsigned int deriv, _xVec3* o);
void xSpline3_EvalSeg(xSpline3* spl, float u, unsigned int deriv, _xVec3* o);
void xMat3x3Euler(_xMat3x3* m, float yaw, float pitch, float roll);
void xMat3x3RotC(_xMat3x3* m, float _x, float _y, float _z, float t);
void xMat3x3LookVec(_xMat3x3* m, _xVec3* at);
void xMat4x3Rot(_xMat4x3* m, _xVec3* a, float t, _xVec3* p);
void xQuatFromMat(_xQuat* q, _xMat3x3* m);
void xQuatToMat(_xQuat* q, _xMat3x3* m);
void xQuatSlerp(_xQuat* q, _xQuat* a, _xQuat* b, float t);
float xQuatNormalize(_xQuat* o, _xQuat* q);

static _xEntMotion** dbg_xems;
static unsigned short dbg_num;
static unsigned short dbg_num_allocd;
static signed short dbg_idx = -1;

static void xEntMotionDebugWrite(_xEntMotion* xem);
static void xEntMotionDebugDraw(_xEntMotion* xem);
static void xEntERMove(_xEntMotion* motion, _xScene* sc, float dt, _xEntFrame* frame);
static void xEntMPMove(_xEntMotion* motion, _xScene* sc, float dt, _xEntFrame* frame);
static void xEntMechMove(_xEntMotion* motion, _xScene* sc, float dt, _xEntFrame* frame);
void xEntMotionDebugCB();

#pragma auto_inline on

static float xacos(float x)
{
    return acos(x);
}

static void xVec3Inv(_xVec3* o, const _xVec3* v)
{
    xVec3SMul(o, v, -1.0f);
}

static unsigned int xVec3Equals(const _xVec3* a, const _xVec3* b)
{
    return a->x == b->x && a->y == b->y && a->z == b->z;
}

static void xQuatCopy(_xQuat* o, const _xQuat* q)
{
    o->s = q->s;
    o->v.x = q->v.x;
    o->v.y = q->v.y;
    o->v.z = q->v.z;
}

static unsigned int xQuatEquals(const _xQuat* a, const _xQuat* b)
{
    return a->s == b->s && xVec3Equals(&a->v, &b->v);
}

static float xQuatDot(const _xQuat* a, const _xQuat* b)
{
    return a->v.x * b->v.x + a->v.y * b->v.y + a->v.z * b->v.z + a->s * b->s;
}

static void xQuatFlip(_xQuat* o, const _xQuat* q)
{
    o->s = -q->s;
    xVec3Inv(&o->v, &q->v);
}

static float xSpline3_ArcTotal(xSpline3* spl)
{
    return spl->arcLength[spl->N * spl->arcSample - 1];
}

#pragma fp_contract off
static float xVec3Length2(const _xVec3* v)
{
    return v->x * v->x + v->y * v->y + v->z * v->z;
}
#pragma fp_contract on

static void xEntMotionDebugAdd(_xEntMotion* motion)
{
    if (dbg_num < dbg_num_allocd)
    {
        dbg_xems[dbg_num++] = motion;
    }
}

static void xEntMPSetSpeed(_xEntMotion* motion, float speed)
{
    motion->mp.speed = (0.0f > speed) ? 0.0f : speed;
}

static void xEntMPGetNext(_xEntMotion* motion, _xMovePoint* prev, _xScene* sc)
{
    _xEntMPData* mp = &motion->mp;
    _xVec3 tempdir;
    _xMat3x3 mat;

    mp->spl = 0;
    mp->curdist = 0.0f;
    mp->dist = xMovePointGetNext(mp->src, prev, &mp->dest, &tempdir);

    if (mp->dest == mp->src)
    {
        xEntMotionStop(motion);
    }
    else if (mp->dest == 0)
    {
        xEntMotionStop(motion);
    }
    else
    {
        if (motion->asset->flags & 1)
        {
            xQuatFromMat(&mp->aquat, (_xMat3x3*)motion->owner->model->Mat);
            xVec3Inv(&tempdir, &tempdir);
            xMat3x3LookVec(&mat, &tempdir);
            xQuatFromMat(&mp->bquat, &mat);
        }
        if (mp->dest->spl)
        {
            mp->spl = mp->dest->spl;
            while (mp->dest->asset->bezIndex)
            {
                mp->dest = mp->dest->nodes[0];
            }
            mp->dist = xSpline3_ArcTotal(mp->spl);
        }
    }
}

static void xEntOrbitMove(_xEntMotion* motion, _xScene* sc, float dt, _xEntFrame* frame)
{
    float rem = 0.5f * dt;
    float u = motion->orb.w * (motion->t + rem);
    float th = isin(motion->orb.w * rem);
    float newt = motion->t - motion->orb.p;

    if (newt > 0.0f)
    {
        motion->t = newt;
    }

    if (-newt < dt && -newt > 0.0f)
    {
        xVec3Sub(&frame->dpos, &motion->orb.orig, &frame->mat.pos);
    }
    else
    {
        frame->dpos.x = 2.0f * th * motion->orb.a * isin(u);
        frame->dpos.y = 0.0f;
        frame->dpos.z = -2.0f * th * motion->orb.b * icos(u);
    }

    frame->mode = 2;
}

static void xEntPenMove(_xEntMotion* motion, _xScene* sc, float dt, _xEntFrame* frame)
{
    _xEntPenData* pen = &motion->pen;
    float w;
    float th = 0.5f * dt;
    float rem = motion->t + th;
    p2EntMotionPenData* aspen = &motion->asset->pen;
    w = pen->w;
    float dangle = 2.0f * aspen->range * isin(th * w) * icos(rem * w + aspen->phase);
    float newt = motion->t - aspen->period;

    if (newt > 0.0f)
    {
        motion->t = newt;
    }

    if (-newt < dt && -newt > 0.0f)
    {
        memcpy(&frame->mat, &motion->pen.omat, sizeof(_xMat4x3));
        frame->mode = 0x11;
    }
    else
    {
        xMat4x3Rot(&frame->dmat, &frame->mat.at, dangle, &pen->top);
        frame->mode = 0x40;
    }
}

static unsigned char xEntERIsExtending(_xEntMotion* motion)
{
    return motion->t < motion->er.et;
}

static unsigned char xEntERIsExtended(_xEntMotion* motion)
{
    return motion->t >= motion->er.et && motion->t < motion->er.brt;
}

static unsigned char xEntERIsRetracting(_xEntMotion* motion)
{
    return motion->t >= motion->er.brt && motion->t < motion->er.ert;
}

static unsigned char xEntERIsRetracted(_xEntMotion* motion)
{
    return motion->t >= motion->er.ert;
}

static inline unsigned int xEntSldMove(_xEntMotion* motion, _xScene* sc, float dt, _xEntFrame* frame)
{
    _xEntMechData* mech = &motion->mech;
    p2EntMotionMechData* mkasst = &motion->asset->mech;
    unsigned int last;
    float tmr = motion->tmr;
    float rem = mkasst->sld_tm - tmr;
    float speed;

    if (dt - rem > 0.0f)
    {
        if (mech->state == 0)
        {
            xVec3Sub(&frame->dpos, &mech->bpos, &frame->mat.pos);
        }
        else
        {
            xVec3Sub(&frame->dpos, &mech->apos, &frame->mat.pos);
        }
        last = 1;
    }
    else
    {
        if (mech->state == 0)
        {
            if (tmr < mkasst->sld_acc_tm)
            {
                speed = (mech->ss * tmr) / mkasst->sld_acc_tm;
            }
            else if (tmr > mech->tsfd)
            {
                speed = (mech->ss * rem) / mkasst->sld_dec_tm;
            }
            else
            {
                speed = mech->ss;
            }
        }
        else if (tmr < mkasst->sld_dec_tm)
        {
            speed = (mech->ss * tmr) / mkasst->sld_dec_tm;
        }
        else if (tmr > mech->tsbd)
        {
            speed = (mech->ss * rem) / mkasst->sld_acc_tm;
        }
        else
        {
            speed = mech->ss;
        }

        last = 0;
        xVec3SMul(&frame->dpos, &mech->dir, speed * dt);
    }

    frame->mode |= 2;
    return last;
}

static inline unsigned int xEntRotMove(_xEntMotion* motion, _xScene* sc, float dt, _xEntFrame* frame)
{
    _xEntMechData* mech = &motion->mech;
    p2EntMotionMechData* mkasst = &motion->asset->mech;
    unsigned int last;
    float tmr = motion->tmr;
    float rem = mkasst->rot_tm - tmr;
    float speed;

    if (dt - rem > 0.0f)
    {
        if (mech->state == 1 || (mkasst->type == 2 && mech->state == 0))
        {
            *mech->rotptr = mech->brot;
        }
        else
        {
            *mech->rotptr = mech->arot;
        }
        last = 1;
    }
    else
    {
        if (mech->state == 1 || (mkasst->type == 2 && mech->state == 0))
        {
            if (tmr < mkasst->rot_acc_tm)
            {
                speed = (mech->sr * tmr) / mkasst->rot_acc_tm;
            }
            else if (tmr > mech->trfd)
            {
                speed = (mech->sr * rem) / mkasst->rot_dec_tm;
            }
            else
            {
                speed = mech->sr;
            }
        }
        else if (tmr < mkasst->rot_dec_tm)
        {
            speed = (mech->sr * tmr) / mkasst->rot_dec_tm;
        }
        else if (tmr > mech->trbd)
        {
            speed = (mech->sr * rem) / mkasst->rot_acc_tm;
        }
        else
        {
            speed = mech->sr;
        }

        *mech->rotptr = xAngleClamp(speed * dt + *mech->rotptr);
        last = 0;
    }

    xMat3x3Euler((_xMat3x3*)&frame->mat, frame->rot.axis.x, frame->rot.axis.y,
                 frame->rot.axis.z);

    if (motion->owner)
    {
        p2EntAsset* easst = motion->owner->asset;
        xVec3SMulBy(&frame->mat.right, easst->scale.x);
        xVec3SMulBy(&frame->mat.up, easst->scale.y);
        xVec3SMulBy(&frame->mat.at, easst->scale.z);
    }

    frame->mode |= 0x10;
    return last;
}

#pragma auto_inline off

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xEntMotionInit(_xEntMotion* motion, _xEnt* owner, p2EntMotionAsset* asset)
{
    motion->asset = asset;
    motion->type = asset->type;
    motion->flags = asset->flags;

    if (motion->type == 0)
    {
        xVec3Copy(&motion->er.a, &asset->er.ret_pos);
        xVec3Add(&motion->er.b, &asset->er.ret_pos, &asset->er.ext_dpos);
        motion->er.et = asset->er.ext_tm;
        motion->er.wet = asset->er.ext_wait_tm;
        motion->er.rt = asset->er.ret_tm;
        motion->er.wrt = asset->er.ret_wait_tm;
        if (motion->er.p <= 0.0f)
        {
            motion->er.p = 1.0f;
        }
    }
    else if (motion->type == 1)
    {
        xVec3Copy(&motion->orb.c, &asset->orb.center);
        motion->orb.a = asset->orb.w;
        motion->orb.b = asset->orb.h;
        motion->orb.p = asset->orb.period;
        motion->orb.w = 6.2831855f / asset->orb.period;
    }
    else if (motion->type == 3)
    {
    }
    else if (motion->type == 5)
    {
        motion->pen.w = 6.2831855f / asset->pen.period;
    }
    else if (motion->type == 4)
    {
        if (asset->mech.sld_tm < 0.0f)
        {
            asset->mech.sld_tm = 0.0f;
        }
        if (asset->mech.sld_acc_tm < 0.0f)
        {
            asset->mech.sld_acc_tm = 0.0f;
        }
        if (asset->mech.sld_dec_tm < 0.0f)
        {
            asset->mech.sld_dec_tm = 0.0f;
        }
        if (asset->mech.sld_acc_tm + asset->mech.sld_dec_tm > asset->mech.sld_tm)
        {
            asset->mech.sld_dec_tm = asset->mech.sld_acc_tm = 0.5f * asset->mech.sld_tm;
        }
        if (asset->mech.rot_tm < 0.0f)
        {
            asset->mech.rot_tm = 0.0f;
        }
        if (asset->mech.rot_acc_tm < 0.0f)
        {
            asset->mech.rot_acc_tm = 0.0f;
        }
        if (asset->mech.rot_dec_tm < 0.0f)
        {
            asset->mech.rot_dec_tm = 0.0f;
        }
        if (asset->mech.type == 2)
        {
            if (asset->mech.rot_tm != asset->mech.sld_tm)
            {
                asset->mech.rot_tm = asset->mech.sld_tm;
            }
        }
        if (asset->mech.rot_acc_tm + asset->mech.rot_dec_tm > asset->mech.rot_tm)
        {
            asset->mech.rot_dec_tm = asset->mech.rot_acc_tm = 0.5f * asset->mech.rot_tm;
        }
    }

    motion->owner = owner;
    motion->target = 0;
    xEntMotionDebugAdd(motion);
}

void xEntMotionSetup(_xEntMotion* motion, _xScene* sc)
{
    if (motion->type == 0)
    {
        motion->er.brt = motion->er.et + motion->er.wet;
        motion->er.ert = motion->er.brt + motion->er.rt;
        motion->er.p = motion->er.ert + motion->er.wrt;
    }
}

// TODO: 93.2% - register allocation / scheduling
void xEntMotionReset(_xEntMotion* motion, _xScene* sc)
{
    motion->flags = motion->asset->flags;
    motion->t = 0.0f;

    if (motion->type == 0)
    {
        motion->er.state = 0;
    }
    else if (motion->type == 3)
    {
        motion->mp.src = (_xMovePoint*)xSceneResolvID(sc, motion->asset->mp.mp_id);
        xEntMPSetSpeed(motion, motion->asset->mp.speed);
        xEntMPGetNext(motion, 0, sc);
        xVec3Copy((_xVec3*)&motion->owner->model->Mat->pos, xMovePointGetPos(motion->mp.src));
    }
    else if (motion->type == 1)
    {
        xVec3Copy(&motion->orb.orig, (_xVec3*)&motion->owner->model->Mat->pos);
    }
    else if (motion->type == 5)
    {
        p2EntMotionPenData* aspen = &motion->asset->pen;
        _xMat4x3* modlrot = (_xMat4x3*)motion->owner->model->Mat;
        _xMat3x3 pshrot;
        _xVec3 a;
        _xVec3 b;

        xMat3x3RotC(&pshrot, modlrot->at.x, modlrot->at.y, modlrot->at.z,
                    aspen->range * isin(aspen->phase));
        a.x = 0.0f;
        a.y = aspen->len;
        a.z = 0.0f;
        xMat3x3RMulVec(&b, &pshrot, &a);
        xMat3x3Mul((_xMat3x3*)modlrot, (_xMat3x3*)modlrot, &pshrot);
        modlrot->pos.x += a.x - b.x;
        modlrot->pos.y += a.y - b.y;
        modlrot->pos.z += a.z - b.z;
        xVec3Add(&motion->pen.top, &modlrot->pos, &b);
        memcpy(&motion->pen.omat, modlrot, sizeof(_xMat4x3));
    }
    else if (motion->type == 3)
    {
        if (motion->asset->flags & 1)
        {
            xQuatFromMat(&motion->mp.aquat, (_xMat3x3*)motion->owner->model->Mat);
            xQuatCopy(&motion->mp.bquat, &motion->mp.aquat);
        }
        motion->tmr = 0.0f;
    }
    else if (motion->type == 4)
    {
        _xEnt* ownr = motion->owner;
        p2EntMotionMechData* mkasst = &motion->asset->mech;
        _xEntMechData* mech = &motion->mech;

        if (ownr && ownr->frame && ownr->model)
        {
            xVec3Copy(&mech->apos, (_xVec3*)&ownr->model->Mat->pos);
            if (mkasst->sld_axis == 0)
            {
                xVec3Copy(&mech->dir, (_xVec3*)&ownr->model->Mat->right);
            }
            else if (mkasst->sld_axis == 1)
            {
                xVec3Copy(&mech->dir, (_xVec3*)&ownr->model->Mat->up);
            }
            else
            {
                xVec3Copy(&mech->dir, (_xVec3*)&ownr->model->Mat->at);
            }

            xVec3SMul(&mech->bpos, &mech->dir, mkasst->sld_dist);
            xVec3AddTo(&mech->bpos, &mech->apos);
            mech->ss = mkasst->sld_dist /
                       -(0.5f * (mkasst->sld_acc_tm + mkasst->sld_dec_tm) - mkasst->sld_tm);
            mech->tsfd = mkasst->sld_tm - mkasst->sld_dec_tm;
            mech->tsbd = mkasst->sld_tm - mkasst->sld_acc_tm;

            if (mkasst->rot_axis == 0)
            {
                mech->rotptr = &ownr->frame->rot.axis.y;
            }
            else if (mkasst->rot_axis == 1)
            {
                mech->rotptr = &ownr->frame->rot.axis.x;
            }
            else
            {
                mech->rotptr = &ownr->frame->rot.axis.z;
            }

            mech->arot = *mech->rotptr;
            float drot = (3.1415927f * mkasst->rot_dist) / 180.0f;
            mech->brot = xAngleClamp(mech->arot + drot);
            mech->sr = drot / -(0.5f * (mkasst->rot_acc_tm + mkasst->rot_dec_tm) - mkasst->rot_tm);
            mech->trfd = mkasst->rot_tm - mkasst->rot_dec_tm;
            mech->trbd = mkasst->rot_tm - mkasst->rot_acc_tm;

            if (mkasst->type == 0 || mkasst->type == 2 || mkasst->type == 3)
            {
                mech->state = 0;
            }
            else
            {
                mech->state = 1;
            }
            motion->tmr = 0.0f;
        }
    }
}

// TODO: 97.3% - pendulum block float register allocation (f28-f30 permuted)
void xEntMotionMove(_xEntMotion* motion, _xScene* sc, float dt, _xEntFrame* frame)
{
    if (motion->flags & 4)
    {
        return;
    }

    if (motion->type == 0)
    {
        xEntERMove(motion, sc, dt, frame);
    }
    else if (motion->type == 1)
    {
        xEntOrbitMove(motion, sc, dt, frame);
    }
    else if (motion->type == 3)
    {
        xEntMPMove(motion, sc, dt, frame);
    }
    else if (motion->type == 5)
    {
        xEntPenMove(motion, sc, dt, frame);
    }
    else if (motion->type == 4)
    {
        xEntMechMove(motion, sc, dt, frame);
    }
    else
    {
        frame->mode = 0;
    }

    motion->t += dt;
}

// TODO: 96.5% - register allocation
static void xEntERMove(_xEntMotion* motion, _xScene* sc, float dt, _xEntFrame* frame)
{
    float newt = motion->t - motion->er.p;

    if (newt > 0.0f)
    {
        motion->t = newt;
    }

    if (xEntERIsExtending(motion))
    {
        float rem = motion->er.et - motion->t;
        float scale = ((rem < dt) ? rem : dt) / motion->er.et;

        if (rem < dt)
        {
            xVec3Sub(&frame->dpos, &motion->er.b, &frame->mat.pos);
        }
        else
        {
            xVec3Sub(&frame->dpos, &motion->er.b, &motion->er.a);
            xVec3SMulBy(&frame->dpos, scale);
        }

        frame->mode = 2;
        motion->er.state = 0;
    }
    else if (xEntERIsExtended(motion))
    {
        motion->er.state = 1;
    }
    else if (xEntERIsRetracting(motion))
    {
        float rem = motion->er.ert - motion->t;
        float scale = ((rem < dt) ? rem : dt) / motion->er.rt;

        if (rem < dt)
        {
            xVec3Sub(&frame->dpos, &motion->er.a, &frame->mat.pos);
        }
        else
        {
            xVec3Sub(&frame->dpos, &motion->er.a, &motion->er.b);
            xVec3SMulBy(&frame->dpos, scale);
        }

        frame->mode = 2;
        motion->er.state = 2;
    }
    else if (xEntERIsRetracted(motion))
    {
        motion->er.state = 3;
    }
}

// TODO: 98.0% - register allocation / instruction scheduling
static void xEntMPMove(_xEntMotion* motion, _xScene* sc, float dt, _xEntFrame* frame)
{
    _xEntMPData* mp = &motion->mp;

    if (mp->dest == mp->src || !mp->dest)
    {
        return;
    }

    if (motion->tmr > 0.0f)
    {
        motion->tmr -= dt;
        return;
    }

    frame->mode = 2;

    float newdist = mp->speed * dt + mp->curdist;
    if (newdist >= mp->dist)
    {
        frame->dpos.x = mp->dest->pos->x - frame->mat.pos.x;
        frame->dpos.y = mp->dest->pos->y - frame->mat.pos.y;
        frame->dpos.z = mp->dest->pos->z - frame->mat.pos.z;

        if (motion->asset->mp.flags & 1)
        {
            xEntMotionStop(motion);
        }

        _xMovePoint* prev = mp->src;
        mp->src = mp->dest;
        xEntMPGetNext(motion, prev, sc);
        motion->tmr = mp->src->asset->delay;
        return;
    }

    mp->curdist = newdist;

    float qdot;
    _xVec3 tgt;
    _xVec3 dir;
    _xQuat quat;
    _xQuat qold;
    _xMat3x3 tmpmat;

    if (mp->spl)
    {
        float u = xSpline3_EvalArcApprox(mp->spl, newdist, 0, &tgt);

        if (motion->asset->flags & 1)
        {
            xSpline3_EvalSeg(mp->spl, u, 1, &dir);
            if (xVec3Length2(&dir) < 1e-6f)
            {
                if (u < 0.1f)
                {
                    u += 0.01f;
                }
                else
                {
                    u -= 0.01f;
                }
                xSpline3_EvalSeg(mp->spl, u, 1, &dir);
            }

            xVec3Inv(&dir, &dir);
            xMat3x3LookVec(&tmpmat, &dir);
            xQuatFromMat(&quat, &tmpmat);
            xQuatFromMat(&qold, (_xMat3x3*)&frame->mat);

            qdot = xQuatDot(&quat, &qold);
            if (qdot < 0.0f)
            {
                qdot = -qdot;
                xQuatFlip(&quat, &quat);
            }
            if (qdot > 1.0f)
            {
                qdot = 1.0f;
            }
            qdot = 2.0f * xacos(qdot);

            if (qdot <= 3.1415927f * dt)
            {
                frame->mode |= 0x10;
                *(_xMat3x3*)&frame->mat = tmpmat;
            }
            else
            {
                xQuatSlerp(&quat, &qold, &quat, 3.1415927f * dt / qdot);
                xQuatNormalize(&quat, &quat);
                xQuatToMat(&quat, (_xMat3x3*)&frame->mat);
            }
        }
    }
    else
    {
        xVec3Lerp(&tgt, mp->src->pos, mp->dest->pos, mp->curdist / mp->dist);

        if ((motion->asset->flags & 1) && !xQuatEquals(&mp->aquat, &mp->bquat))
        {
            float t = 2.0f * (mp->curdist / mp->dist);
            xQuatSlerp(&quat, &mp->aquat, &mp->bquat, (1.0f < t) ? 1.0f : t);
            xQuatToMat(&quat, (_xMat3x3*)&frame->mat);
            frame->mode |= 0x10;
        }
    }

    frame->dpos.x = tgt.x - frame->mat.pos.x;
    frame->dpos.y = tgt.y - frame->mat.pos.y;
    frame->dpos.z = tgt.z - frame->mat.pos.z;
}

// TODO: 99.5% - register allocation
static void xEntMechMove(_xEntMotion* motion, _xScene* sc, float dt, _xEntFrame* frame)
{
    p2EntMotionMechData* mkasst = &motion->asset->mech;
    _xEntMechData* mech = &motion->mech;
    unsigned int last = 0;

    frame->mode = 0;

    if (mech->state == 0)
    {
        last = xEntSldMove(motion, sc, dt, frame);
        if (mkasst->type == 2)
        {
            xEntRotMove(motion, sc, dt, frame);
        }
        if (last)
        {
            if (mkasst->type == 3)
            {
                mech->state = 1;
            }
            else if (mkasst->flags & 1)
            {
                mech->state = 2;
                mech->ss = -mech->ss;
                mech->sr = -mech->sr;
            }
            else if (mkasst->flags & 2)
            {
                mech->state = 6;
            }
            else
            {
                mech->state = 5;
            }
        }
    }
    else if (mech->state == 1)
    {
        last = xEntRotMove(motion, sc, dt, frame);
        if (last)
        {
            if (mkasst->type == 4)
            {
                mech->state = 0;
            }
            else if (mkasst->flags & 1)
            {
                mech->state = 2;
                mech->ss = -mech->ss;
                mech->sr = -mech->sr;
            }
            else if (mkasst->flags & 2)
            {
                mech->state = 6;
            }
            else
            {
                mech->state = 5;
            }
        }
    }
    else if (mech->state == 2)
    {
        if (motion->tmr >= mkasst->ret_delay)
        {
            if (mkasst->type == 0 || mkasst->type == 2 || mkasst->type == 4)
            {
                mech->state = 3;
            }
            else
            {
                mech->state = 4;
            }
            last = 1;
        }
    }
    else if (mech->state == 3)
    {
        last = xEntSldMove(motion, sc, dt, frame);
        if (mkasst->type == 2)
        {
            xEntRotMove(motion, sc, dt, frame);
        }
        if (last)
        {
            if (mkasst->type == 4)
            {
                mech->state = 4;
            }
            else if (mkasst->flags & 2)
            {
                mech->state = 7;
            }
            else
            {
                mech->state = 5;
            }
        }
    }
    else if (mech->state == 4)
    {
        last = xEntRotMove(motion, sc, dt, frame);
        if (last)
        {
            if (mkasst->type == 3)
            {
                mech->state = 3;
            }
            else if (mkasst->flags & 2)
            {
                mech->state = 7;
            }
            else
            {
                mech->state = 5;
            }
        }
    }
    else if (mech->state == 5 && motion->tmr >= mkasst->post_ret_delay)
    {
        xEntMotionReset(motion, sc);
        xEntMotionRun(motion);
        last = 1;
    }

    if (last)
    {
        motion->tmr = 0.0f;
    }
    else
    {
        motion->tmr += dt;
    }
}

void xEntMechForward(_xEntMotion* motion)
{
    p2EntMotionMechData* mech = &motion->asset->mech;
    p2EntMotionAsset* mkasst = motion->asset;

    xEntMotionRun(motion);

    if (motion->mech.state != 0 && motion->mech.state != 1 && motion->mech.state != 2)
    {
        if (motion->mech.state == 3)
        {
            motion->mech.ss = -motion->mech.ss;
            motion->mech.sr = -motion->mech.sr;
            motion->tmr = mkasst->mech.sld_tm - motion->tmr;
            motion->mech.state = 0;
        }
        else if (motion->mech.state == 4)
        {
            motion->mech.ss = -motion->mech.ss;
            motion->mech.sr = -motion->mech.sr;
            motion->tmr = mkasst->mech.rot_tm - motion->tmr;
            motion->mech.state = 1;
        }
        else if (motion->mech.state != 5 && motion->mech.state != 6 && motion->mech.state == 7)
        {
            motion->mech.ss = -motion->mech.ss;
            motion->mech.sr = -motion->mech.sr;
            motion->tmr = 0.0f;
            if (mech->type == 0 || mech->type == 2 || mech->type == 4)
            {
                motion->mech.state = 0;
            }
            else
            {
                motion->mech.state = 1;
            }
        }
    }
}

void xEntMechReverse(_xEntMotion* motion)
{
    p2EntMotionMechData* mech = &motion->asset->mech;
    p2EntMotionAsset* mkasst = motion->asset;

    xEntMotionRun(motion);

    if (motion->mech.state == 0)
    {
        motion->mech.ss = -motion->mech.ss;
        motion->mech.sr = -motion->mech.sr;
        motion->tmr = mkasst->mech.sld_tm - motion->tmr;
        motion->mech.state = 3;
    }
    else if (motion->mech.state == 1)
    {
        motion->mech.ss = -motion->mech.ss;
        motion->mech.sr = -motion->mech.sr;
        motion->tmr = mkasst->mech.rot_tm - motion->tmr;
        motion->mech.state = 4;
    }
    else if (motion->mech.state != 2 && motion->mech.state != 3 && motion->mech.state != 4 &&
             motion->mech.state != 5 && motion->mech.state == 6)
    {
        motion->mech.ss = -motion->mech.ss;
        motion->mech.sr = -motion->mech.sr;
        motion->tmr = 0.0f;
        if (mech->type == 0 || mech->type == 2 || mech->type == 4)
        {
            motion->mech.state = 3;
        }
        else
        {
            motion->mech.state = 4;
        }
    }
}

void xEntMotionTranslate(_xEntMotion* motion, _xVec3* dpos)
{
    if (motion->type == 0)
    {
        xVec3AddTo(&motion->er.a, dpos);
        xVec3AddTo(&motion->er.b, dpos);
    }
    else if (motion->type == 1)
    {
        xVec3AddTo(&motion->orb.orig, dpos);
        xVec3AddTo(&motion->orb.c, dpos);
    }
    else if (motion->type == 3)
    {
        return;
    }
    else if (motion->type == 5)
    {
        xVec3AddTo(&motion->pen.top, dpos);
        xVec3AddTo(&motion->pen.omat.pos, dpos);
    }
    else if (motion->type == 4)
    {
        xVec3AddTo(&motion->mech.apos, dpos);
        xVec3AddTo(&motion->mech.bpos, dpos);
    }
}

void xEntMotionDebugInit(unsigned short num_xems)
{
    if (num_xems)
    {
        xDebugModeAdd("DBG_XENTMOTION", xEntMotionDebugCB);
        dbg_num = 0;
        dbg_xems = (_xEntMotion**)xMemAlloc(gActiveHeap, num_xems * sizeof(_xEntMotion*));
        dbg_num_allocd = num_xems;
        dbg_idx = 0;
    }
}

void xEntMotionDebugExit()
{
    dbg_num = 0;
    dbg_xems = 0;
    dbg_num_allocd = 0;
    dbg_idx = -1;
}

#pragma auto_inline on

static void xEntMotionDebugIPad(_xEntMotion* xem)
{
    if (gDebugPad->pressed & 0x20)
    {
        if (++dbg_idx >= dbg_num)
        {
            dbg_idx = 0;
        }
    }

    if (gDebugPad->pressed & 0x80)
    {
        if (--dbg_idx < 0)
        {
            dbg_idx = dbg_num - 1;
        }
    }

    if (gDebugPad->pressed & 0x10000)
    {
        if (xem->owner)
        {
            xEntReset(xem->owner);
        }
        xEntMotionReset(xem, g_xSceneCur);
    }

    if (gDebugPad->pressed & 0x20000)
    {
        if (xem->flags & 4)
        {
            xEntMotionRun(xem);
        }
        else
        {
            xEntMotionStop(xem);
        }
    }
}

#pragma auto_inline off

// TODO: 96.8% - register allocation
void xEntMotionDebugCB()
{
    if (dbg_idx != -1 && dbg_num)
    {
        _xEntMotion* xem = dbg_xems[dbg_idx];
        xEntMotionDebugWrite(xem);
        xEntMotionDebugDraw(xem);
        xEntMotionDebugIPad(xem);
    }
}

// TODO: 98.5% - register allocation in the print block
static void xEntMotionDebugWrite(_xEntMotion* xem)
{
    const char* name;

    switch (xem->type)
    {
    case 0:
        name = "extend/retract";
        break;
    case 1:
        name = "orbital";
        break;
    case 2:
        name = "spline";
        break;
    case 3:
        name = "movepoint";
        break;
    case 4:
        name = "mechanism";
        break;
    case 5:
        name = "pendulum";
        break;
    default:
        name = "????";
        break;
    }

    xprintf("type:             %s\n", name);
    xprintf("stopped:          %s\n", (xem->flags & 4) ? "true " : "false");
    xprintf("owner:            %s\n",
            xem->owner ? xSceneID2Name(g_xSceneCur, xem->owner->id) : "null");
    xprintf("t:  %.3f   d: %.3f   tmr: %.3f\n", xem->t, xem->d, xem->tmr);

    switch (xem->type)
    {
    case 0:
        xprintf("a:       <%.3f %.3f %.3f>\n", xem->er.a.x, xem->er.a.y, xem->er.a.z);
        xprintf("b:       <%.3f %.3f %.3f>\n", xem->er.b.x, xem->er.b.y, xem->er.b.z);
        xprintf("b-a:     <%.3f %.3f %.3f>\n", xem->asset->er.ext_dpos.x,
                xem->asset->er.ext_dpos.y, xem->asset->er.ext_dpos.z);
        xprintf("ext_tm:  %.3f\n", xem->er.et);
        xprintf("ret_tm:  %.3f\n", xem->er.rt);
        xprintf("wait_et: %.3f\n", xem->er.wet);
        xprintf("wait_rt: %.3f\n", xem->er.wrt);
        xprintf("period:  %.3f\n", xem->er.p);
        break;
    case 1:
        xprintf("c:   <%.3f %.3f %.3f>\n", xem->orb.c.x, xem->orb.c.y, xem->orb.c.z);
        xprintf("a:   %.3f\n", xem->orb.a);
        xprintf("b:   %.3f\n", xem->orb.b);
        xprintf("p:   %.3f\n", xem->orb.p);
        xprintf("w:   %.3f\n", xem->orb.w);
        break;
    case 3:
        xprintf("src-mp:           %s\n",
                xem->mp.src ? xSceneID2Name(g_xSceneCur, xem->mp.src->id) : "");
        xprintf("dest-mp:          %s\n",
                xem->mp.dest ? xSceneID2Name(g_xSceneCur, xem->mp.dest->id) : "");
        xprintf("dist:  %.3f\n", xem->mp.dist);
        xprintf("speed: %.3f\n", xem->mp.speed);
        break;
    case 4:
        switch (xem->asset->mech.type)
        {
        case 0:
            name = "slide";
            break;
        case 1:
            name = "rot";
            break;
        case 2:
            name = "slide_rot";
            break;
        case 3:
            name = "slide_then_rot";
            break;
        case 4:
            name = "rot_then_slide";
            break;
        }
        xprintf("type:             %s\n", name);
        xprintf("returns:          %s", (xem->asset->mech.flags & 1) ? "true " : "false");
        if (xem->asset->mech.flags & 1)
        {
            xprintf("   ret_delay: %.3f", xem->asset->mech.ret_delay);
        }
        xprintf("\n");
        xprintf("continuous:       %s", !(xem->asset->mech.flags & 2) ? "true " : "false");
        if (!(xem->asset->mech.flags & 2))
        {
            xprintf("   end_delay: %.3f", xem->asset->mech.post_ret_delay);
        }
        xprintf("\n");

        switch (xem->mech.state)
        {
        case 0:
            name = (xem->asset->mech.type == 2) ? "sliding + rotating forth" : "sliding forth";
            break;
        case 1:
            name = "rotating forth";
            break;
        case 2:
            name = "waiting to return";
            break;
        case 3:
            name = (xem->asset->mech.type == 2) ? "sliding + rotating back" : "sliding back";
            break;
        case 4:
            name = "rotating back";
            break;
        case 5:
            name = "waiting to begin again";
            break;
        case 6:
            name = "done";
            break;
        case 7:
            name = "undone";
            break;
        }
        xprintf("state:            %s\n", name);

        if (xem->asset->mech.type != 1)
        {
            xprintf("slide_axis:   %s", (xem->asset->mech.sld_axis == 0) ? "X" :
                                        (xem->asset->mech.sld_axis == 1) ? "Y" :
                                                                           "Z");
            xprintf("       slide_dist:   %.3f\n", xem->asset->mech.sld_dist);
            xprintf("slide_tm:     %.3f", xem->asset->mech.sld_tm);
            xprintf("   slide_speed:  %.3f\n", xem->mech.ss);
            xprintf("slide_acc_tm: %.3f", xem->asset->mech.sld_acc_tm);
            xprintf("   slide_dec_tm: %.3f\n", xem->asset->mech.sld_dec_tm);
            xprintf("slide_bd_tm:  %.3f", xem->mech.tsbd);
            xprintf("   slide_fd_tm:  %.3f\n", xem->mech.tsfd);
            xprintf("start_pos:  <%.3f %.3f %.3f>\n", xem->mech.apos.x, xem->mech.apos.y,
                    xem->mech.apos.z);
            xprintf("end_pos:    <%.3f %.3f %.3f>\n", xem->mech.bpos.x, xem->mech.bpos.y,
                    xem->mech.bpos.z);
            xprintf("dir:        <%.3f %.3f %.3f>\n", xem->mech.dir.x, xem->mech.dir.y,
                    xem->mech.dir.z);
        }

        if (xem->asset->mech.type != 0)
        {
            xprintf("rot_axis:   %s", (xem->asset->mech.rot_axis == 0) ? "X" :
                                      (xem->asset->mech.rot_axis == 1) ? "Y" :
                                                                         "Z");
            xprintf("       rot_dist:   %.3f\n", xem->asset->mech.rot_dist);
            xprintf("rot_tm:     %.3f", xem->asset->mech.rot_tm);
            xprintf("   rot_speed:  %.3f\n", (180.0f * xem->mech.sr) / 3.1415927f);
            xprintf("rot_acc_tm: %.3f", xem->asset->mech.rot_acc_tm);
            xprintf("   rot_dec_tm: %.3f\n", xem->asset->mech.rot_dec_tm);
            xprintf("rot_bd_tm:  %.3f", xem->mech.trbd);
            xprintf("   rot_fd_tm:  %.3f\n", xem->mech.trfd);
            xprintf("arot:       %.3f\n", (180.0f * xem->mech.arot) / 3.1415927f);
            xprintf("brot:       %.3f\n", (180.0f * xem->mech.brot) / 3.1415927f);
        }
        break;
    case 5:
        xprintf("top:    <%.3f %.3f %.3f>\n", xem->pen.top.x, xem->pen.top.y, xem->pen.top.z);
        xprintf("length: %.3f\n", xem->asset->pen.len);
        xprintf("period: %.3f\n", xem->asset->pen.period);
        xprintf("phase:  %.3f\n", (180.0f * xem->asset->pen.phase) / 3.1415927f);
        xprintf("range:  %.3f\n", (180.0f * xem->asset->pen.range) / 3.1415927f);
        xprintf("w:      %.3f\n", xem->pen.w);
        break;
    }
}

// TODO: 99.5% - register allocation
static void xEntMotionDebugDraw(_xEntMotion* xem)
{
    if (xem->owner && xem->target)
    {
        xDrawSetColor(g_NEON_GREEN);
        xDrawLine((_xVec3*)&xem->owner->model->Mat->pos, (_xVec3*)&xem->target->model->Mat->pos);
    }

    switch (xem->type)
    {
    case 0:
        xDrawSetColor(g_NEON_RED);
        xDrawLine(&xem->er.a, &xem->er.b);
        break;
    case 1:
        if (xem->owner)
        {
            xDrawSetColor(g_NEON_RED);
            xDrawLine(&xem->orb.c, (_xVec3*)&xem->owner->model->Mat->pos);
        }
        break;
    case 3:
    {
        xDrawSetColor(g_PIMP_GOLD);
        _xMovePoint* mp = xem->mp.dest;
        if (mp)
        {
            for (unsigned short i = 0; i < mp->asset->numPoints; i++)
            {
                _xMovePoint* mp2 = mp->nodes[i];
                if (mp2 != xem->mp.src)
                {
                    xDrawLine(xMovePointGetPos(mp), xMovePointGetPos(mp2));
                }
                for (unsigned short j = 0; j < mp2->asset->numPoints; j++)
                {
                    xDrawLine(xMovePointGetPos(mp2), xMovePointGetPos(mp2->nodes[j]));
                }
            }
        }
        if (xem->mp.src && xem->mp.dest)
        {
            xDrawSetColor(g_NEON_RED);
            xDrawLine(xMovePointGetPos(xem->mp.src), xMovePointGetPos(xem->mp.dest));
        }
        break;
    }
    case 4:
        xDrawSetColor(g_NEON_RED);
        xDrawLine(&xem->mech.apos, &xem->mech.bpos);
        break;
    case 5:
        if (xem->owner)
        {
            xDrawSetColor(g_NEON_RED);
            xDrawLine(&xem->pen.top, (_xVec3*)&xem->owner->model->Mat->pos);
        }
        break;
    }
}

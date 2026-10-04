#include "zEnt.h"
#include "zGlobals.h"
#include "zParEmitter.h"
#include "xAnim.h"
#include "xString.h"

class xSerial;

class p2EntHangableAsset
{
public:
    unsigned int flags; // 0x0
    float pivotOffset;  // 0x4
    float leverArm;     // 0x8
    float gravity;      // 0xC
    float accel;        // 0x10
    float decay;        // 0x14
    float grabDelay;    // 0x18
    float stopDecel;    // 0x1C
};

class _zEntHangable : public _zEnt
{
public:
    p2EntHangableAsset* hangInfo; // 0xEC
    _xVec3 pivot;                 // 0xF0
    _xVec3 endpos;                // 0xFC
    _xVec3 vel;                   // 0x108
    _xVec3 swingplane;            // 0x114
    float grabTimer;              // 0x120
    float spin;                   // 0x124
    unsigned int state;           // 0x128
    _zEnt* shaggy;                // 0x12C
    signed int enabled;           // 0x130
    _zEnt* follow;                // 0x134
    signed int moving;            // 0x138
    float candle_timer;           // 0x13C
    signed int candle_state;      // 0x140
};

class xAnimState;
class xAnimSingle;

class xAnimPlay
{
public:
    xAnimPlay* Next;          // 0x0
    unsigned short NumSingle; // 0x4
    unsigned short BoneCount; // 0x6
    xAnimSingle* Single;      // 0x8
    void* Object;             // 0xC
    xAnimTable* Table;        // 0x10
};

void xVec3Normalize(_xVec3* o, _xVec3* v);
void xMat3x3RotC(_xMat3x3* m, float _x, float _y, float _z, float t);
void xMat3x3RMulVec(_xVec3* o, _xMat3x3* m, _xVec3* v);
void xMat3x3GetEuler(_xMat3x3* m, _xVec3* a);
void xEntReset(_xEnt* ent);
xAnimState* xAnimTableGetState(xAnimTable* table, char* name);
void xAnimPlaySetState(xAnimSingle* single, xAnimState* state, float startTime);
float zEntShaggy_GetSpin(_zEnt* ent);
void Shaggy1_SetHangable(_zEnt* shaggy, _zEntHangable* ent);
float iurand();

static _zParEmitter* sCandleEmitter;
static _zParEmitter* sCandleSmokeEmitter;
static unsigned int sChandelierHash;
static _zParEmitter* sMountEmitter;

void zEntHangable_Update(_zEntHangable* ent, _xScene* sc, float dt);
signed int zEntHangableEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                               xBase* toParamWidget);
void zEntHangable_FollowUpdate(_zEntHangable* ent);
void zEntHangable_SetShaggy(_zEntHangable* ent, _zEnt* shaggy);
void zEntHangable_SetMatrix(_zEntHangable* ent, float dt);
void zEntHangable_Reset(_zEntHangable* ent);

static inline void xVec3Init(_xVec3* v, float _x, float _y, float _z)
{
    v->x = _x;
    v->y = _y;
    v->z = _z;
}

static inline void xVec3Cross(_xVec3* o, const _xVec3* a, const _xVec3* b)
{
    _xVec3 v;
    v.x = a->y * b->z - a->z * b->y;
    v.y = a->z * b->x - a->x * b->z;
    v.z = a->x * b->y - a->y * b->x;
    *o = v;
}

void zEntHangable_SetupFX()
{
    sCandleEmitter = zParEmitterFind("PAREMIT_CHAND_CANDLE");
    sCandleSmokeEmitter = zParEmitterFind("PAREMIT_CHAND_CANDLE_SMOKE");
    sChandelierHash = xStrHash("rlii0006");
    sMountEmitter = zParEmitterFind("PAREMIT_HANGABLE_MOUNT");
}

static inline void HangableSetup(_zEntHangable* ent, p2EntAsset* asset)
{
    p2EntHangableAsset* hangAsset = (p2EntHangableAsset*)(asset + 1);
    _xVec3* center;
    _xMat3x3 hackMat;

    ent->pflags = 0;
    ent->chkby = 0;
    ent->penby = 0;
    ent->update = (void (*)(_xEnt*, _xScene*, float))zEntHangable_Update;
    ent->move = 0;
    ent->eventFunc = zEntHangableEventCB;
    ent->hangInfo = hangAsset;
    center = &ent->asset->pos;
    ent->pivot.x = center->x;
    ent->pivot.y = center->y + hangAsset->pivotOffset;
    ent->pivot.z = center->z;
    ent->endpos.x = center->x;
    ent->endpos.y = ent->pivot.y - hangAsset->leverArm;
    ent->endpos.z = center->z;
    ent->vel.x = 0.0f;
    ent->vel.y = 0.0f;
    ent->vel.z = 0.0f;
    ent->grabTimer = 0.0f;
    ent->spin = 0.0f;
    ent->state = 0;
    ent->shaggy = 0;
    ent->follow = 0;
    ent->enabled = 1;
    ent->moving = 0;
    ent->candle_timer = 3.0f;
    ent->candle_state = 0;
    center = &ent->asset->ang;
    xMat3x3Euler(&hackMat, center->x, center->y, center->z);
    ent->swingplane.x = hackMat.right.x;
    ent->swingplane.y = hackMat.right.y;
    ent->swingplane.z = hackMat.right.z;
}

void zEntHangable_Init(void* a, void* b)
{
    _zEntHangable* ent = (_zEntHangable*)a;
    p2EntAsset* asset = (p2EntAsset*)b;

    zEntInit(ent, asset, 'HANG');

    if (ent->linkCount)
    {
        ent->link = (p2LinkAsset*)((unsigned char*)ent->asset + 0x70);
    }
    else
    {
        ent->link = 0;
    }

    HangableSetup(ent, asset);
}

static inline void zEntHangable_UpdateFX(_zEntHangable* ent)
{
    _xVec3 offset_rlii0006[8] = {
        { 0.0f, 0.561f, 0.613f },          { 0.43345639f, 0.432f, 0.43345639f },
        { 0.613f, 0.543f, 0.0f },          { 0.43345639f, 0.526f, -0.43345639f },
        { 0.0f, 0.526f, -0.613f },         { -0.43345639f, 0.505f, -0.43345639f },
        { -0.613f, 0.543f, 0.0f },         { -0.43345639f, 0.48f, 0.43345639f },
    };

    if ((ent->flags & 1) && ent->asset->modelInfoID == sChandelierHash)
    {
        _xVec3* local_offset = offset_rlii0006;
        if (ent->candle_state != 2)
        {
            _zParEmitterCustomSettings info;
            _zParEmitter* emitter;

            info.custom_flags = 0x20;
            if (ent->candle_state == 0)
            {
                emitter = sCandleEmitter;
            }
            else
            {
                emitter = sCandleSmokeEmitter;
            }

            for (signed int i = 0; i < 8; i++)
            {
                _xVec3 mul;
                xMat3x3RMulVec(&mul, (_xMat3x3*)ent->model->Mat, &local_offset[i]);
                info.pos.x = mul.x + ent->model->Mat->pos.x;
                info.pos.y = mul.y + ent->model->Mat->pos.y;
                info.pos.z = mul.z + ent->model->Mat->pos.z;
                zParEmitterEmitCustom(emitter, 1.0f / 30.0f, &info);
            }
        }
    }
}

// TODO: 95.9%. The original keeps unitHang.x/z (0.0f) in f31/f29 with a
// 0x130 frame; we constant-fold them.
void zEntHangable_Update(_zEntHangable* ent, _xScene*, float dt)
{
    _xVec3 sub;

    ent->frame->oldpos = *(_xVec3*)&ent->model->Mat->pos;
    xMat3x3GetEuler((_xMat3x3*)ent->model->Mat, &ent->frame->oldrot.axis);

    if (ent->enabled < 0)
    {
        ent->enabled++;
        if (ent->enabled == 0)
        {
            ent->enabled = 1;
        }
    }

    zEntHangable_UpdateFX(ent);

    if (!(ent->hangInfo->flags & 0x80000000))
    {
        ent->hangInfo->leverArm -= globals.player.ent.bound.sph.r;
        ent->hangInfo->flags |= 0x80000000;
    }

    ent->grabTimer = (0.0f > ent->grabTimer - dt) ? 0.0f : ent->grabTimer - dt;

    if (globals.player.HangEnt == ent)
    {
        return;
    }

    zEntHangable_FollowUpdate(ent);

    _xVec3 unitHang;
    unitHang.x = 0.0f;
    unitHang.y = -ent->hangInfo->gravity;
    unitHang.z = 0.0f;

    sub.x = ent->endpos.x - ent->pivot.x;
    sub.y = ent->endpos.y - ent->pivot.y;
    sub.z = ent->endpos.z - ent->pivot.z;
    xVec3Normalize(&sub, &sub);

    float dot = xVec3Dot(&sub, &unitHang);
    unitHang.x = -(dot * sub.x - unitHang.x);
    unitHang.y = -(dot * sub.y - unitHang.y);
    unitHang.z = -(dot * sub.z - unitHang.z);

    ent->vel.x = unitHang.x * dt + ent->vel.x;
    ent->vel.y = unitHang.y * dt + ent->vel.y;
    ent->vel.z = unitHang.z * dt + ent->vel.z;

    dot = xVec3Dot(&sub, &ent->vel);
    ent->vel.x = -(dot * sub.x - ent->vel.x);
    ent->vel.y = -(dot * sub.y - ent->vel.y);
    ent->vel.z = -(dot * sub.z - ent->vel.z);

    xVec3SMulBy(&ent->vel, 0.97f);

#pragma fp_contract off
    if (ent->vel.x * ent->vel.x + ent->vel.y * ent->vel.y + ent->vel.z * ent->vel.z < 0.001f)
#pragma fp_contract on
    {
        ent->vel.x = 0.0f;
        ent->vel.y = 0.0f;
        ent->vel.z = 0.0f;
    }

    ent->endpos.x = ent->vel.x * dt + ent->endpos.x;
    ent->endpos.y = ent->vel.y * dt + ent->endpos.y;
    ent->endpos.z = ent->vel.z * dt + ent->endpos.z;

    sub.x = ent->endpos.x - ent->pivot.x;
    sub.y = ent->endpos.y - ent->pivot.y;
    sub.z = ent->endpos.z - ent->pivot.z;
    xVec3Normalize(&sub, &sub);
    xVec3SMul(&sub, &sub, ent->hangInfo->leverArm);
    xVec3Add(&ent->endpos, &ent->pivot, &sub);

    zEntHangable_SetMatrix(ent, dt);
}

#pragma auto_inline on
static void zEntHangableMountFX(_zEntHangable* ent)
{
    if (sMountEmitter)
    {
        _zParEmitterCustomSettings info;
        info.custom_flags = 0x60;
        for (signed int i = 0; i < 10; i++)
        {
            info.pos = ent->pivot;
            info.vel.x = 0.0f;
            info.vel.y = -2.0f - 2.0f * iurand();
            info.vel.z = 0.0f;
            zParEmitterEmitCustom(sMountEmitter, 1.0f / 30.0f, &info);
        }
    }
}

static void zEntHangable_SetFollow(_zEntHangable* ent, _zEnt* follow)
{
    if (follow)
    {
        ent->follow = follow;
        zEntHangable_FollowUpdate(ent);
    }
    else if (ent->follow)
    {
        ent->follow = 0;
    }
}
#pragma auto_inline off

signed int zEntHangableEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                               xBase* toParamWidget)
{
    _zEntHangable* ent = (_zEntHangable*)to;
    unsigned int wasVisible = xEntIsVisible(ent);

    switch (toEvent)
    {
    case 0x3:
        ent->flags |= 1;
        break;
    case 0x4:
        ent->flags &= ~1;
        break;
    case 0x53:
        ent->enabled = 1;
        break;
    case 0x20:
        zEntHangableMountFX(ent);
        if (ent->asset->modelInfoID == sChandelierHash && ent->candle_state == 0)
        {
            ent->candle_timer = 3.0f;
        }
        break;
    case 0x21:
        ent->enabled = -2;
        break;
    case 0xCF:
    {
        _zEnt* shaggy = (_zEnt*)toParamWidget;
        if (shaggy && shaggy->subType == 0x1D)
        {
            xAnimPlay* play = (xAnimPlay*)shaggy->model->Anim;
            xAnimPlaySetState(((xAnimPlay*)shaggy->model->Anim)->Single,
                              xAnimTableGetState(play->Table, "Hang"), 0.0f);
            zEntHangable_SetShaggy(ent, shaggy);
            Shaggy1_SetHangable(shaggy, ent);
            zEntHangableMountFX(ent);
        }
        break;
    }
    case 0x54:
        ent->enabled = 0;
        break;
    case 0x55:
        ent->flags |= 1;
        ent->enabled = 1;
        break;
    case 0x56:
        ent->flags &= ~1;
        ent->enabled = 0;
        break;
    case 0xA:
        zEntHangable_Reset(ent);
        break;
    case 0xCE:
        zEntHangable_SetFollow(ent, (_zEnt*)from);
        break;
    }

    if (!wasVisible && xEntIsVisible(ent))
    {
        ent->vel.x = 0.0f;
        ent->vel.y = 0.0f;
        ent->vel.z = 0.0f;
        ent->endpos.x = ent->pivot.x;
        ent->endpos.y = ent->pivot.y - ent->hangInfo->leverArm;
        ent->endpos.z = ent->pivot.z;
    }

    return 1;
}

#pragma auto_inline on
static signed int HangableIsMovingTooMuch(_xVec3* opos, _xVec3* pos)
{
#pragma fp_contract off
    float dist = (pos->x * pos->x + pos->y * pos->y + pos->z * pos->z) -
                 (opos->x * opos->x + opos->y * opos->y + opos->z * opos->z);
#pragma fp_contract on
    if (dist > 20.0f || dist < -20.0f)
    {
        return 1;
    }
    return 0;
}
#pragma auto_inline off

// TODO: 99.2%. Register allocation of the swing plane cross product and
// the order of the tmpMat stack loads differ.
void zEntHangable_SetMatrix(_zEntHangable* ent, float dt)
{
    _xMat4x3 spinMat;
    _xMat3x3 tmpMat;
    _xVec3 rot;
    _xVec3* opos;
    _xVec3* orot;
    _xVec3* pos;
    signed int moving;

    spinMat.up.x = ent->pivot.x - ent->endpos.x;
    spinMat.up.y = ent->pivot.y - ent->endpos.y;
    spinMat.up.z = ent->pivot.z - ent->endpos.z;
    xVec3Normalize(&spinMat.up, &spinMat.up);

    spinMat.at.x = 0.0f;
    spinMat.at.y = 0.0f;
    spinMat.at.z = 1.0f;
    xVec3Cross(&spinMat.right, &spinMat.up, &spinMat.at);
    xVec3Normalize(&spinMat.right, &spinMat.right);
    xMat3x3RotC(&tmpMat, spinMat.up.x, spinMat.up.y, spinMat.up.z, ent->spin);
    xMat3x3RMulVec(&spinMat.right, &tmpMat, &spinMat.right);
    xVec3Cross(&spinMat.at, &spinMat.right, &spinMat.up);

    spinMat.pos.x = 0.0f;
    spinMat.pos.y = -ent->hangInfo->pivotOffset;
    spinMat.pos.z = 0.0f;
    xMat3x3RMulVec(&spinMat.pos, (_xMat3x3*)&spinMat, &spinMat.pos);
    spinMat.pos.x += ent->pivot.x;
    spinMat.pos.y += ent->pivot.y;
    spinMat.pos.z += ent->pivot.z;
    spinMat.flags = 0;

    *(_xMat4x3*)ent->model->Mat = spinMat;

    _zEnt* shaggy = ent->shaggy;
    if (shaggy)
    {
        float d = (globals.player.Shaggy1.HangTrans.y - globals.player.Shaggy1.ReleaseTrans.y) -
                  globals.player.ent.bound.sph.r;
        shaggy->model->Mat->pos.x = d * spinMat.up.x + ent->endpos.x;
        shaggy->model->Mat->pos.y = d * spinMat.up.y + ent->endpos.y;
        shaggy->model->Mat->pos.z = d * spinMat.up.z + ent->endpos.z;

        xMat3x3RotC(&tmpMat, spinMat.up.x, spinMat.up.y, spinMat.up.z, zEntShaggy_GetSpin(shaggy));
        xMat3x3RMulVec(&spinMat.right, &tmpMat, &spinMat.right);
        xVec3Cross(&spinMat.at, &spinMat.right, &spinMat.up);
        *(_xMat3x3*)shaggy->model->Mat = *(_xMat3x3*)&spinMat;
    }

    _xEntFrame* frame = ent->frame;
    RwMatrixTag* mat = ent->model->Mat;
    xMat3x3GetEuler((_xMat3x3*)mat, &rot);
    opos = &frame->oldpos;
    orot = &frame->oldrot.axis;
    pos = (_xVec3*)&mat->pos;

    if (opos->x != pos->x || opos->y != pos->y || opos->z != pos->z || orot->x != rot.x ||
        orot->y != rot.y || orot->z != rot.z)
    {
        moving = 1;
        if (ent->moving != 1)
        {
            zEntEvent(ent, 0xE2);
        }
    }
    else
    {
        moving = 0;
        if (ent->moving != 0)
        {
            zEntEvent(ent, 0xE3);
        }
    }
    ent->moving = moving;

    if (ent->asset->modelInfoID != sChandelierHash)
    {
        return;
    }

    if (ent->candle_state == 0)
    {
        ent->candle_timer -= dt;
        if (ent->candle_timer <= 0.0f)
        {
            ent->candle_timer = 0.0f;
            if (HangableIsMovingTooMuch(opos, pos))
            {
                ent->candle_state = 1;
                ent->candle_timer = iurand() * 10.0f + 10.0f;
            }
        }
    }
    else if (ent->candle_state == 1)
    {
        ent->candle_timer -= dt;
        if (ent->candle_timer <= 0.0f)
        {
            ent->candle_timer = iurand() * 5.0f + 10.0f;
            ent->candle_state = 2;
        }
    }
    else
    {
        ent->candle_timer -= dt;
        if (ent->candle_timer <= 0.0f)
        {
            ent->candle_timer = 0.0f;
            if (!HangableIsMovingTooMuch(opos, pos))
            {
                ent->candle_timer = 3.0f;
                ent->candle_state = 0;
            }
        }
    }
}

void zEntHangable_Save(_zEntHangable* ent, xSerial* s)
{
    zEntSave(ent, s);
}

void zEntHangable_Load(_zEntHangable* ent, xSerial* s)
{
    zEntLoad(ent, s);
}

void zEntHangable_Reset(_zEntHangable* ent)
{
    zEntHangable_SetShaggy(ent, 0);
    zEntHangable_SetFollow(ent, 0);
    xEntReset(ent);
    HangableSetup(ent, ent->asset);
}

#pragma auto_inline on
void zEntHangable_SetShaggy(_zEntHangable* ent, _zEnt* shaggy)
{
    if (shaggy)
    {
        if (!ent->shaggy)
        {
            ent->shaggy = shaggy;
            ent->hangInfo->leverArm += 2.2f;
            ent->endpos.y -= 2.2f;
            shaggy->frame->mode = 1;
            shaggy->frame->vel.x = 0.0f;
            shaggy->frame->vel.y = 0.0f;
            shaggy->frame->vel.z = 0.0f;
            shaggy->frame->mat.pos.x = ent->model->Mat->pos.x;
            shaggy->frame->mat.pos.z = ent->model->Mat->pos.z;
            shaggy->frame->mat.pos.y = ((ent->endpos.y + globals.player.Shaggy1.HangTrans.y) -
                                        globals.player.Shaggy1.ReleaseTrans.y) -
                                       globals.player.ent.bound.sph.r;
            *(_xMat4x3*)shaggy->model->Mat = shaggy->frame->mat;
        }
    }
    else if (ent->shaggy)
    {
        ent->shaggy = 0;
        ent->hangInfo->leverArm -= 2.2f;
        ent->endpos.y += 2.2f;
    }
}

void zEntHangable_FollowUpdate(_zEntHangable* ent)
{
    if (ent->follow)
    {
        float x = ent->follow->model->Mat->pos.x;
        float y = ent->follow->model->Mat->pos.y +
                  ((p2EntHangableAsset*)(ent->asset + 1))->pivotOffset;
        float z = ent->follow->model->Mat->pos.z;
        float dx = x - ent->pivot.x;
        float dy = y - ent->pivot.y;
        float dz = z - ent->pivot.z;

        ent->pivot.x = x;
        ent->pivot.y = y;
        ent->pivot.z = z;

        if (globals.player.HangEnt == ent)
        {
            globals.player.HangPivot.x = x;
            globals.player.HangPivot.y = y;
            globals.player.HangPivot.z = z;
            globals.player.ent.model->Mat->pos.x += dx;
            globals.player.ent.model->Mat->pos.y += dy;
            globals.player.ent.model->Mat->pos.z += dz;
        }

        ent->endpos.x += dx;
        ent->endpos.y += dy;
        ent->endpos.z += dz;
        ent->model->Mat->pos.x += dx;
        ent->model->Mat->pos.y += dy;
        ent->model->Mat->pos.z += dz;

        if (ent->shaggy)
        {
            ent->shaggy->model->Mat->pos.x += dx;
            ent->shaggy->model->Mat->pos.y += dy;
            ent->shaggy->model->Mat->pos.z += dz;
        }
    }
}
#pragma auto_inline off

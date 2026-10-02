#include "zEnt.h"
#include "zGlobals.h"
#include "xEntMotion.h"

void xEntDefaultTranslate(_xEnt* ent, _xVec3* dpos);

class _zPendulum : public _zEnt
{
public:
    _xEntMotion motion; // 0xEC
    float lt;           // 0x158
    float q1t;          // 0x15C
    float q3t;          // 0x160
};

void zPendulum_Update(_zPendulum* pend, _xScene* sc, float dt);
void zPendulum_Move(_zPendulum* pend, _xScene* s, float dt, _xEntFrame* frame);
signed int zPendulumEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                            xBase* toParamWidget);
static void zPendulumTranslate(_xEnt* xent, _xVec3* dpos);

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zPendulum_Init(void* pend, void* asset)
{
    p2EntMotionAsset* motionAsset;

    zEntInit((_zEnt*)pend, (p2EntAsset*)asset, 'PEND');
    motionAsset = (p2EntMotionAsset*)((p2EntAsset*)asset + 1);
    if (((_zPendulum*)pend)->linkCount)
    {
        ((_zPendulum*)pend)->link = (p2LinkAsset*)(motionAsset + 1);
    }
    else
    {
        ((_zPendulum*)pend)->link = 0;
    }
    ((_zPendulum*)pend)->update = (void (*)(_xEnt*, _xScene*, float))zPendulum_Update;
    ((_zPendulum*)pend)->move = (void (*)(_xEnt*, _xScene*, float, _xEntFrame*))zPendulum_Move;
    ((_zPendulum*)pend)->eventFunc = zPendulumEventCB;
    ((_zPendulum*)pend)->transl = zPendulumTranslate;
    zEntReset((_zPendulum*)pend);
    xEntMotionInit(&((_zPendulum*)pend)->motion, (_zPendulum*)pend, motionAsset);
}

void zPendulum_Save(_zPendulum* pend, xSerial* s)
{
    zEntSave(pend, s);
}

void zPendulum_Load(_zPendulum* pend, xSerial* s)
{
    zEntLoad(pend, s);
}

void zPendulum_Setup(_zPendulum* pend, _xScene* sc)
{
    zEntSetup(pend);
    xEntMotionSetup(&pend->motion, sc);
}

#pragma auto_inline on
void zPendulum_Reset(_zPendulum* pend, _xScene* sc)
{
    p2EntMotionAsset* asset;

    zEntReset(pend);
    xEntMotionReset(&pend->motion, sc);

    asset = pend->motion.asset;
    pend->lt = 0.0f;
    pend->q1t = 1.0f - asset->pen.phase / 6.2831855f;
    pend->q3t = 0.5f - asset->pen.phase / 6.2831855f;
    if (pend->q1t <= 0.0f)
    {
        pend->q1t += 1.0f;
    }
    if (pend->q1t > 1.0f)
    {
        pend->q1t -= 1.0f;
    }
    if (pend->q3t <= 0.0f)
    {
        pend->q3t += 1.0f;
    }
    if (pend->q3t > 1.0f)
    {
        pend->q3t -= 1.0f;
    }
    pend->q1t *= asset->pen.period;
    pend->q3t *= asset->pen.period;
}
#pragma auto_inline off

void zPendulum_Update(_zPendulum* pend, _xScene* sc, float dt)
{
    float lt;
    float t;

    xEntUpdate(pend, sc, dt);

    t = pend->motion.t;
    lt = pend->lt;
    p2EntMotionPenData* pen = &pend->motion.asset->pen;
    if (t < lt)
    {
        t += pen->period;
    }

    if (lt <= pend->q1t && t > pend->q1t)
    {
        zEntEvent(pend, 0xE4);
    }
    else if (lt <= pend->q3t && t > pend->q3t)
    {
        zEntEvent(pend, 0xE4);
    }

    pend->lt = pend->motion.t;
}

void zPendulum_Move(_zPendulum* pend, _xScene* s, float dt, _xEntFrame* frame)
{
    xEntMotionMove(&pend->motion, s, dt, frame);
}

static void zPendulumTranslate(_xEnt* xent, _xVec3* dpos)
{
    xEntDefaultTranslate(xent, dpos);
    xEntMotionTranslate(&((_zPendulum*)xent)->motion, dpos);
}

signed int zPendulumEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                            xBase* toParamWidget)
{
    _zPendulum* pend = (_zPendulum*)to;

    switch (toEvent)
    {
    case 0x12:
        xEntMotionRun(&pend->motion);
        break;
    case 0x13:
        xEntMotionStop(&pend->motion);
        break;
    case 0xA:
        zPendulum_Reset(pend, (_xScene*)globals.sceneCur);
        break;
    case 0x3:
        pend->flags |= 1;
        break;
    case 0x4:
        pend->flags &= ~1;
        break;
    case 0x55:
        pend->flags |= 1;
    case 0x53:
        pend->chkby = 0x18;
        break;
    case 0x56:
        pend->flags &= ~1;
    case 0x54:
        pend->chkby = 0;
        break;
    case 0xC3:
    case 0xC4:
    case 0xC5:
    case 0xC6:
    case 0xC7:
    case 0xC8:
    case 0xC9:
    case 0xCA:
        zEntAnimEvent(pend, toEvent, toParam);
        break;
    }

    return 1;
}

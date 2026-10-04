#include "zTimer.h"
#include "xserializer.h"
#include "zEnt.h"

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zTimerInit(void* b, void* tasset)
{
    _zTimer* t = (_zTimer*)b;
    zTimerAsset* asset = (zTimerAsset*)tasset;

    xBaseInit(t, asset);
    t->eventFunc = zTimerEventCB;
    t->tasset = asset;
    if (t->linkCount)
    {
        t->link = (p2LinkAsset*)(t->tasset + 1);
    }
    else
    {
        t->link = 0;
    }
    t->state = 0;
    t->secondsLeft = asset->seconds;
}

void zTimerSave(_zTimer* ent, xSerial* s)
{
    xBaseSave(ent, s);
    s->Write(ent->state);
    s->Write(ent->secondsLeft);
}

void zTimerLoad(_zTimer* ent, xSerial* s)
{
    xBaseLoad(ent, s);
    s->Read(&ent->state);
    s->Read(&ent->secondsLeft);
}

signed int zTimerEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                         xBase* toParamWidget)
{
    _zTimer* t = (_zTimer*)to;

    switch (toEvent)
    {
    case 0x12:
        t->state = 1;
        break;
    case 0x13:
        if (t->state != 1)
        {
            t->state = 0;
        }
        break;
    case 0xA:
        xBaseReset(t, t->tasset);
        t->state = 0;
        t->secondsLeft = t->tasset->seconds;
        break;
    case 0x14:
        t->state = 0;
        break;
    }
    return 1;
}

void zTimerUpdate(xBase* to, _xScene*, float dt)
{
    _zTimer* t = (_zTimer*)to;

    if (t->state == 1)
    {
        t->secondsLeft -= dt;
        if (t->secondsLeft <= 0.0f)
        {
            zEntEvent(t, t, 0x14);
        }
    }
}

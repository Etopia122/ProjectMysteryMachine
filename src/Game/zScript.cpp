#include "xBase.h"
#include "zEnt.h"
#include "zGlobals.h"
#include "zScene.h"

class xSerial;

class zScriptAsset : public p2BaseAsset
{
public:
    float scriptStartTime;
    unsigned int eventCount;
};

class zScriptEventAsset
{
public:
    float time;
    unsigned int widget;
    unsigned int paramEvent;
    float param[4];
    unsigned int paramWidget;
};

class _zScript : public xBase
{
public:
    zScriptAsset* tasset;
    unsigned char state;
    unsigned char more;
    unsigned char pad[2];
    float time;
};

signed int zScriptEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                          xBase* toParamWidget);

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zScriptInit(void* b, void* tasset)
{
    _zScript* t = (_zScript*)b;
    zScriptAsset* asset = (zScriptAsset*)tasset;

    xBaseInit(t, asset);
    t->eventFunc = zScriptEventCB;
    t->tasset = asset;
    if (t->linkCount)
    {
        t->link = (p2LinkAsset*)(t->tasset + 1);
        t->link = (p2LinkAsset*)((zScriptEventAsset*)t->link + asset->eventCount);
    }
    else
    {
        t->link = 0;
    }
    t->time = t->tasset->scriptStartTime;
    t->more = 1;
}

void zScriptSave(_zScript* ent, xSerial* s)
{
    xBaseSave(ent, s);
}

void zScriptLoad(_zScript* ent, xSerial* s)
{
    xBaseLoad(ent, s);
}

signed int zScriptEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                          xBase* toParamWidget)
{
    _zScript* t = (_zScript*)to;

    switch (toEvent)
    {
    case 0x12:
        t->state = 1;
        break;
    case 0x13:
        if (t->state == 1 || t->state == 3)
        {
            t->state = 0;
        }
        break;
    case 0x14:
        t->state = 0;
        break;
    case 0x124:
        t->state = 0;
        t->time = t->tasset->scriptStartTime;
        break;
    case 0x125:
        if (t->state == 1)
        {
            t->state = 3;
        }
        break;
    case 0xA:
        xBaseReset(t, t->tasset);
        t->time = t->tasset->scriptStartTime;
        t->more = 1;
        break;
    }
    return 1;
}

static inline void zScriptExecuteEvents(_zScript* t, float start, float end)
{
    zScriptEventAsset* a;
    signed int i;
    xBase* b;

    t->more = 0;
    a = (zScriptEventAsset*)(t->tasset + 1);
    for (i = 0; i < (signed int)t->tasset->eventCount; a++, i++)
    {
        if (a->widget && a->time >= start)
        {
            if (a->time < end)
            {
                b = zSceneFindObject(a->widget);
                if (b)
                {
                    zEntEvent(t, 0, b, a->paramEvent, a->param,
                              !a->paramWidget ? 0 : zSceneFindObject(a->paramWidget), 0);
                }
            }
            else
            {
                t->more = 1;
            }
        }
    }
}

void zScriptUpdate(xBase* to, float dt)
{
    _zScript* t = (_zScript*)to;

    if (t->state == 1)
    {
        zScriptExecuteEvents(t, t->time, t->time + dt);
        t->time += dt;
        if (!t->more && t->state == 3)
        {
            zEntEvent(t, 0x14);
        }
    }
    else if (t->state == 3)
    {
        if (globals.pad0 && (globals.pad0->on & 0x40000))
        {
            t->state = 1;
        }
    }
}

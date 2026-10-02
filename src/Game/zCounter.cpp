#include "zCounter.h"
#include "xserializer.h"
#include "zEnt.h"

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zCounterInit(void* b, void* asset)
{
    _zCounter* c = (_zCounter*)b;
    zCounterAsset* a = (zCounterAsset*)asset;

    xBaseInit(c, a);
    c->eventFunc = zCounterEventCB;
    c->asset = a;
    if (c->linkCount)
    {
        c->link = (p2LinkAsset*)(c->asset + 1);
    }
    else
    {
        c->link = 0;
    }
    c->state = 0;
    c->count = a->count;
}

void zCounterReset(xBase* b)
{
    _zCounter* c = (_zCounter*)b;

    xBaseInit(c, c->asset);
    if (c->linkCount)
    {
        c->link = (p2LinkAsset*)(c->asset + 1);
    }
    else
    {
        c->link = 0;
    }
    c->count = c->asset->count;
    c->state = 0;
}

void zCounterSave(_zCounter* ent, xSerial* s)
{
    xBaseSave(ent, s);
    s->Write(ent->state);
    s->Write(ent->count);
}

void zCounterLoad(_zCounter* ent, xSerial* s)
{
    xBaseLoad(ent, s);
    s->Read(&ent->state);
    s->Read(&ent->count);
}

signed int zCounterEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                           xBase* toParamWidget)
{
    _zCounter* c = (_zCounter*)to;

    switch (toEvent)
    {
    case 0xC:
        if (c->state != 1)
        {
            c->count--;
            if (c->count == 0)
            {
                zEntEvent(c, c, 0x14);
            }
        }
        break;
    case 0xB:
        if (c->state != 1)
        {
            c->count++;
            if (c->count == 0)
            {
                zEntEvent(c, c, 0x14);
            }
        }
        break;
    case 0xA:
        c->state = 0;
        c->count = c->asset->count;
        break;
    case 0x14:
        c->count = 0;
        break;
    }
    return 1;
}

#include "zConditional.h"
#include "zEnt.h"
#include "zVar.h"

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zConditionalInit(void* b, void* asset)
{
    _zConditional* c = (_zConditional*)b;
    zCondAsset* a = (zCondAsset*)asset;

    xBaseInit(c, a);
    c->eventFunc = zConditionalEventCB;
    c->asset = a;
    if (c->linkCount)
    {
        c->link = (p2LinkAsset*)(c->asset + 1);
    }
    else
    {
        c->link = 0;
    }
    zVarInit(zVarEntryTable);
}

void zConditionalSave(_zConditional* ent, xSerial* s)
{
    xBaseSave(ent, s);
}

void zConditionalLoad(_zConditional* ent, xSerial* s)
{
    xBaseLoad(ent, s);
}

unsigned int zConditional_Evaluate(_zConditional* c)
{
    zVarEntry* v = 0;
    signed int i;
    unsigned int val;

    for (i = 0; i < 107; i++)
    {
        if (c->asset->expr1 == zVarEntryTable[i].varNameID)
        {
            v = &zVarEntryTable[i];
        }
    }

    if (v == 0)
    {
        return 0;
    }

    val = v->varCB();

    switch (c->asset->op)
    {
    case 0:
        return val == c->asset->constNum;
    case 1:
        return val > c->asset->constNum;
    case 2:
        return val < c->asset->constNum;
    case 3:
        return val >= c->asset->constNum;
    case 4:
        return val <= c->asset->constNum;
    case 5:
        return val != c->asset->constNum;
    }
    return 0;
}

// TODO: 97% - the original has a redundant extra branch after the switch
// dispatch (a jump to the default/end block) that this version doesn't produce.
signed int zConditionalEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                               xBase* toParamWidget)
{
    _zConditional* c = (_zConditional*)to;

    switch (toEvent)
    {
    case 0x3C:
        if (zConditional_Evaluate(c))
        {
            zEntEvent(c, c, 0x3D);
        }
        else
        {
            zEntEvent(c, c, 0x3E);
        }
        break;
    case 0xA:
        xBaseReset(c, c->asset);
        break;
    case 0x3D:
        break;
    }
    return 1;
}

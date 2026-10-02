#include "xBase.h"
#include "iMath.h"
#include "zEnt.h"
#include "zScene.h"

class xSerial;

class zGroupAsset : public p2BaseAsset
{
public:
    unsigned short itemCount;
    unsigned short groupFlags;
    // followed by itemCount item IDs (unsigned int)
};

class _zGroup : public xBase
{
public:
    zGroupAsset* asset;
};

char* zSceneGetName(xBase* b);
signed int zGroupEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                         xBase* toParamWidget);

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zGroupInit(void* b, void* asset)
{
    _zGroup* g = (_zGroup*)b;
    zGroupAsset* a = (zGroupAsset*)asset;

    xBaseInit(g, a);
    g->eventFunc = zGroupEventCB;
    g->asset = a;
    if (g->linkCount)
    {
        g->link = (p2LinkAsset*)((char*)g->asset + sizeof(zGroupAsset) + a->itemCount * 4);
    }
    else
    {
        g->link = 0;
    }
}

void zGroupSave(_zGroup* ent, xSerial* s)
{
    xBaseSave(ent, s);
}

void zGroupLoad(_zGroup* ent, xSerial* s)
{
    xBaseLoad(ent, s);
}

signed int zGroupEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                         xBase* toParamWidget)
{
    _zGroup* g = (_zGroup*)to;
    signed int rand;
    unsigned int* items;
    signed int i;
    xBase* b;

    if (toEvent == 0xA)
    {
        xBaseReset(g, g->asset);
    }
    else if (toEvent == 0x106)
    {
        toEvent = 2;
    }

    rand = -1;

    if (g->asset->groupFlags & 1)
    {
        rand = irand() % g->asset->itemCount;
    }

    items = (unsigned int*)(g->asset + 1);
    for (i = 0; i < g->asset->itemCount; i++)
    {
        if (rand == -1 || rand == i)
        {
            b = zSceneFindObject(items[i]);
            if (b)
            {
                zEntEvent(b, toEvent, toParam, toParamWidget);
            }
            else
            {
                zSceneGetName(g);
            }
        }
    }
    return 1;
}

unsigned int zGroupGetCount(_zGroup* g)
{
    return g->asset->itemCount;
}

xBase* zGroupGetItemPtr(_zGroup* g, unsigned int index)
{
    return zSceneFindObject(((unsigned int*)(g->asset + 1))[index]);
}

unsigned int zGroupGetItem(_zGroup* g, unsigned int index)
{
    return ((unsigned int*)(g->asset + 1))[index];
}

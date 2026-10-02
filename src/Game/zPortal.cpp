#include "xBase.h"
#include "zGlobals.h"

class xSerial;

class zPortalAsset : public p2BaseAsset
{
public:
    unsigned int assetCameraID;
    unsigned int assetMarkerID;
    float ang;
    unsigned int sceneID;
};

class _zPortal : public xBase
{
public:
    zPortalAsset* passet;
};

void zSceneSwitch(_zPortal* p);
signed int zPortalEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                          xBase* toParamWidget);

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zPortalInit(void* portal, void* passet)
{
    _zPortal* p = (_zPortal*)portal;

    xBaseInit(p, (zPortalAsset*)passet);
    p->passet = (zPortalAsset*)passet;
    p->eventFunc = zPortalEventCB;
    if (p->linkCount)
    {
        p->link = (p2LinkAsset*)(p->passet + 1);
    }
}

void zPortalSave(_zPortal* ent, xSerial* s)
{
    xBaseSave(ent, s);
}

void zPortalLoad(_zPortal* ent, xSerial* s)
{
    xBaseLoad(ent, s);
}

signed int zPortalEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                          xBase* toParamWidget)
{
    _zPortal* p = (_zPortal*)to;

    switch (toEvent)
    {
    case 0xA:
        xBaseReset(p, p->passet);
        break;
    case 0x10:
        if (globals.unk2E8)
        {
            zSceneSwitch(p);
        }
        break;
    }
    return 1;
}

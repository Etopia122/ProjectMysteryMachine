#include "zEnt.h"
#include "zScene.h"

class _zEntPickup;
class _zPlatform;

void zEntPickup_MakeDroppable(_zEntPickup* ent, _xVec3* pos, xBase* dropper);
void zPlatform_MakeDiggable(_zPlatform* plat);

class p2SimpleObjAsset
{
public:
    float animSpeed;
    unsigned int initAnimState;
    unsigned char collType;
    unsigned char flags;
};

class _zEntSimpleObj : public _zEnt
{
public:
    p2SimpleObjAsset* sasset; // 0xEC
    unsigned int sflags;      // 0xF0
};

signed int zEntSimpleObjEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                                xBase* toParamWidget);

static inline void DiggableSetup(_zEntSimpleObj* ent)
{
    xBase* tgt;
    unsigned int i;
    p2LinkAsset* curlink = ent->link;
    unsigned int dug = 0;

    for (i = 0; i < ent->linkCount; i++, curlink++)
    {
        if (curlink->srcEvent == 0xA4)
        {
            tgt = zSceneFindObject(curlink->dstAssetID);
            switch (tgt->baseType)
            {
            case 4:
                zEntPickup_MakeDroppable((_zEntPickup*)tgt, &ent->asset->pos, ent);
                break;
            case 6:
                zPlatform_MakeDiggable((_zPlatform*)tgt);
                break;
            }
            dug = 1;
        }
    }
    if (dug)
    {
        ent->sflags |= 1;
    }
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zEntSimpleObj_Init(_zEntSimpleObj* ent, p2EntAsset* asset)
{
    zEntInit(ent, asset, 'SIMP');
    ent->sasset = (p2SimpleObjAsset*)(asset + 1);
    ent->sflags = 0;
    ent->pflags = 0;
    ent->penby |= 0x10;
    if (ent->sasset->collType & 2)
    {
        ent->chkby = 0x18;
    }
    else
    {
        ent->chkby = 0;
    }
    ent->move = 0;
    ent->eventFunc = zEntSimpleObjEventCB;
    if (ent->linkCount)
    {
        ent->link = (p2LinkAsset*)((p2SimpleObjAsset*)(ent->asset + 1) + 1);
    }
    else
    {
        ent->link = 0;
    }
    ent->eventFunc = zEntSimpleObjEventCB;
    zEntReset(ent);
}

void zEntSimpleObj_Setup(_zEntSimpleObj* ent)
{
    zEntSetup(ent);
    DiggableSetup(ent);
}

void zEntSimpleObj_Save(_zEntSimpleObj* ent, xSerial* s)
{
    zEntSave(ent, s);
}

void zEntSimpleObj_Load(_zEntSimpleObj* ent, xSerial* s)
{
    zEntLoad(ent, s);
}

signed int zEntSimpleObjEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                                xBase* toParamWidget)
{
    _zEntSimpleObj* s = (_zEntSimpleObj*)to;

    switch (toEvent)
    {
    case 0x3:
        s->flags |= 1;
        break;
    case 0x4:
        s->flags &= ~1;
        break;
    case 0x55:
        s->flags |= 1;
    case 0x53:
        s->chkby = 0x18;
        break;
    case 0x56:
        s->flags &= ~1;
    case 0x54:
        s->chkby = 0;
        break;
    case 0xA:
        zEntReset(s);
        if (s->sasset->collType & 2)
        {
            s->chkby = 0x18;
        }
        else
        {
            s->chkby = 0;
        }
        DiggableSetup(s);
        break;
    case 0xC3:
    case 0xC4:
    case 0xC5:
    case 0xC6:
    case 0xC7:
    case 0xC8:
    case 0xC9:
    case 0xCA:
        zEntAnimEvent(s, toEvent, toParam);
        break;
    case 0xFC:
        s->moreFlags |= 1;
        break;
    case 0xFD:
        s->moreFlags &= ~1;
        s->flags &= 0xFB;
        s->fade_tmr = 0.0f;
        break;
    case 0xFE:
        s->moreFlags ^= 1;
        if (!(s->moreFlags & 1))
        {
            s->flags &= 0xFB;
            s->fade_tmr = 0.0f;
        }
        break;
    }
    return 1;
}

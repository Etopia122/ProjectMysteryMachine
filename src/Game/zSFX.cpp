#include "xBase.h"
#include "xSnd.h"
#include "iMath.h"
#include "zScene.h"

class zSFXAsset : public p2BaseAsset
{
public:
    unsigned short flagsSFX;
    unsigned short freq;
    float freqm;
    unsigned int soundAssetID;
    unsigned int attachID;
    unsigned char loopCount;
    unsigned char priority;
    unsigned char volume;
    unsigned char pad;
    _xVec3 pos;
    float range;
    float range_falloff;
};

class zSFX : public xBase
{
public:
    zSFXAsset* asset;
    unsigned int sndID;
};

// Functions are in reverse DOL order (-inline deferred emits last to first).

signed int zSFXEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                       xBase* toParamWidget);

void zSFXInit(void* t, void* asset)
{
    xBaseInit((xBase*)t, (p2BaseAsset*)asset);
    ((zSFX*)t)->eventFunc = zSFXEventCB;
    ((zSFX*)t)->asset = (zSFXAsset*)asset;
    if (((zSFX*)t)->linkCount)
    {
        ((zSFX*)t)->link = (p2LinkAsset*)(((zSFX*)t)->asset + 1);
    }
    else
    {
        ((zSFX*)t)->link = 0;
    }
    ((zSFX*)t)->sndID = 0;
}

void zSFXSave(zSFX* ent, xSerial* s)
{
    xBaseSave(ent, s);
}

void zSFXLoad(zSFX* ent, xSerial* s)
{
    xBaseLoad(ent, s);
}

signed int zSFXEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                       xBase* toParamWidget)
{
    zSFX* t = (zSFX*)to;
    unsigned int sndFlags;

    switch (toEvent)
    {
    case 0xA:
        xBaseReset(t, t->asset);
        break;
    case 0x18:
        sndFlags = 0;
        if (t->asset->flagsSFX & 4)
        {
            sndFlags |= 0x100;
        }
        if (t->asset->flagsSFX & 2)
        {
            if (t->asset->flagsSFX & 8)
            {
                _xEnt* ent = (_xEnt*)zSceneFindObject(t->asset->attachID);
                t->sndID = xSndPlay3D(t->asset->soundAssetID, 0.5f * (t->asset->volume / 100.0f),
                                      t->asset->freq * t->asset->freqm, t->asset->priority,
                                      sndFlags, ent, t->asset->range);
            }
            else
            {
                t->sndID = xSndPlay3D(t->asset->soundAssetID, 0.5f * (t->asset->volume / 100.0f),
                                      t->asset->freq * t->asset->freqm, t->asset->priority,
                                      sndFlags, &t->asset->pos, t->asset->range);
            }
        }
        else
        {
            t->sndID = xSndPlay(t->asset->soundAssetID, 0.5f * (t->asset->volume / 100.0f),
                                0.5f * (t->asset->volume / 100.0f),
                                t->asset->freq * t->asset->freqm, t->asset->priority, sndFlags, 0);
        }
        break;
    case 0x1A:
        sndFlags = 0;
        if (t->asset->flagsSFX & 4)
        {
            sndFlags |= 0x100;
        }
        if ((float)(irand() % 100) >= toParam[0])
        {
            if (t->asset->flagsSFX & 2)
            {
                if (t->asset->flagsSFX & 8)
                {
                    _xEnt* ent = (_xEnt*)zSceneFindObject(t->asset->attachID);
                    t->sndID =
                        xSndPlay3D(t->asset->soundAssetID, 0.5f * (t->asset->volume / 100.0f),
                                   t->asset->freq * t->asset->freqm, t->asset->priority, sndFlags,
                                   ent, t->asset->range);
                }
                else
                {
                    t->sndID =
                        xSndPlay3D(t->asset->soundAssetID, 0.5f * (t->asset->volume / 100.0f),
                                   t->asset->freq * t->asset->freqm, t->asset->priority, sndFlags,
                                   &t->asset->pos, t->asset->range);
                }
            }
            else
            {
                t->sndID = xSndPlay(t->asset->soundAssetID, 0.5f * (t->asset->volume / 100.0f),
                                    0.5f * (t->asset->volume / 100.0f),
                                    t->asset->freq * t->asset->freqm, t->asset->priority, sndFlags,
                                    0);
            }
        }
        break;
    case 0x13:
        if (t->sndID)
        {
            xSndStop(t->sndID);
            t->sndID = 0;
        }
        break;
    case 0x17:
        if (t->sndID)
        {
            xSndPause(t->sndID, 1);
        }
        break;
    }
    return 1;
}

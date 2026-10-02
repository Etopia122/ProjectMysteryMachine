#include "xCollide.h"
#include "xBase.h"
#include "xMath3.h"
#include "xMemMgr.h"
#include "xString.h"
#include "xstransvc.h"
#include "xEnt.h"
#include "iMath.h"
#include "zScene.h"
#include "zParEmitter.h"





class zVolumeAsset : public p2BaseAsset
{
public:
    unsigned int flags; // 0x8
    unsigned char boundType;
    unsigned char boundPad[3];
    _xBBox box;         // 0x10 (inside the bound's union)
    unsigned char boundRest[0x38 - 0x4 - 0x24];
    float rot;
    float xpivot;
    float zpivot;
};

class _zVolume : public xBase
{
public:
    zVolumeAsset* asset; // 0x10
};

class zGustAsset : public p2BaseAsset
{
public:
    unsigned int flags;    // 0x8
    unsigned int volumeID; // 0xC
    unsigned int effectID; // 0x10
    _xVec3 vel;            // 0x14
    float fade;            // 0x20
    float partMod;         // 0x24
};

class _zGust : public xBase
{
public:
    unsigned int flags;   // 0x10
    zGustAsset* asset;    // 0x14
    _zVolume* volume;     // 0x18
    _zVolume* fx_volume;  // 0x1C
    float debris_timer;   // 0x20
};

class zGustData
{
public:
    unsigned int gust_on;
    _zGust* g[4];
    float lerp[4];
};

static _zGust* gusts;
static unsigned short ngusts;
static _zParEmitter* sGustDustEmitter;
static _zParEmitter* sGustDebrisEmitter;

signed int zGustEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                        xBase* toParamWidget);

static inline void zGustInit(_zGust* g, zGustAsset* a)
{
    xBaseInit(g, a);
    g->eventFunc = zGustEventCB;
    g->asset = a;
    g->flags = a->flags;
    if (g->linkCount)
    {
        g->link = (p2LinkAsset*)&g->asset[1];
    }
    else
    {
        g->link = 0;
    }
    g->debris_timer = 0.15f;
}

static inline void zGustSetup(_zGust* g)
{
    g->volume = (_zVolume*)zSceneFindObject(g->asset->volumeID);
    g->fx_volume = (_zVolume*)zSceneFindObject(g->asset->effectID);
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zGustInit()
{
    ngusts = xSTAssetCountByType('GUST');

    if (ngusts)
    {
        gusts = (_zGust*)xMemAlloc(gActiveHeap, sizeof(_zGust) * ngusts);
        for (unsigned short i = 0; i < ngusts; i++)
        {
            unsigned int size;
            zGustAsset* asset = (zGustAsset*)xSTFindAssetByType('GUST', i, &size);
            zGustInit(&gusts[i], asset);
        }
    }
    else
    {
        gusts = 0;
    }
}

void zGustSetup()
{
    if (gusts)
    {
        for (unsigned short i = 0; i < ngusts; i++)
        {
            zGustSetup(&gusts[i]);
        }
        sGustDustEmitter = zParEmitterFind(xStrHash("PAREMIT_GUST_DUST"));
        sGustDebrisEmitter = zParEmitterFind(xStrHash("PAREMIT_GUST_DEBRIS"));
    }
}

_zGust* zGustGetGust(unsigned short n)
{
    if (gusts)
    {
        return &gusts[n];
    }
    return 0;
}

void zGustUpdateEnt(_xEnt* ent, _xScene* sc, float dt, void* gdata)
{
    zGustData* data = (zGustData*)gdata;

    if (!gusts)
    {
        return;
    }

    _xCollis coll;
    coll.flags = 0;
    for (unsigned int i = 0; i < ngusts; i++)
    {
        if (gusts[i].flags & 1)
        {
            xBoundHitsBound(&ent->bound, (_xBound*)&gusts[i].volume->asset->boundType, &coll);
            if (coll.flags & 1)
            {
                unsigned int j;
                unsigned int minidx;
                for (j = 0; j < 4; j++)
                {
                    if (data->lerp[j] < 2.0f)
                    {
                        minidx = j;
                    }
                    if (data->g[j] == &gusts[i])
                    {
                        break;
                    }
                }

                if (j == 4)
                {
                    data->g[minidx] = &gusts[i];
                    data->lerp[minidx] = 1.0E-7f;
                }
            }
            else
            {
                for (unsigned int j = 0; j < 4; j++)
                {
                    if (data->g[j] == &gusts[i] && 1.0f == data->lerp[j])
                    {
                        data->lerp[j] = -1.0f;
                        break;
                    }
                }
            }
        }
    }

    for (unsigned int i = 0; i < 4; i++)
    {
        _zGust* g = data->g[i];
        if (g)
        {
            if (g->flags & 1)
            {
                float lerpinc = g->asset->fade;
                if (dt >= lerpinc)
                {
                    if (!(data->lerp[i] < 0.0f))
                    {
                        data->lerp[i] = 1.0f;
                        continue;
                    }
                }
                else
                {
                    lerpinc = dt / lerpinc;
                    if (data->lerp[i] >= 0.0f)
                    {
                        data->lerp[i] += lerpinc;
                        if (data->lerp[i] >= 1.0f)
                        {
                            data->lerp[i] = 1.0f;
                        }
                        continue;
                    }
                    else
                    {
                        data->lerp[i] += lerpinc;
                        if (!(data->lerp[i] >= 0.0f))
                        {
                            continue;
                        }
                    }
                }
            }

            data->g[i] = 0;
            data->lerp[i] = 0.0f;
        }
    }

    data->gust_on = 0;

    for (unsigned int i = 0; i < 4; i++)
    {
        if (data->g[i])
        {
            data->gust_on = 1;

            _xVec3 dpos;
            xVec3SMul(&dpos, &data->g[i]->asset->vel, dt * (float)__fabs(data->lerp[i]));
            xVec3AddTo(&ent->frame->mat.pos, &dpos);
        }
    }
}

void zGustSave(_zGust* g, xSerial* s)
{
    xBaseSave(g, s);
}

void zGustLoad(_zGust* g, xSerial* s)
{
    xBaseLoad(g, s);
}

signed int zGustEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                        xBase* toParamWidget)
{
    switch (toEvent)
    {
    case 0x26:
        ((_zGust*)to)->flags |= 1;
        ((_zGust*)to)->debris_timer = 0.15f;
        break;
    case 0x27:
        ((_zGust*)to)->flags &= ~1;
        break;
    case 0xF:
        ((_zGust*)to)->flags ^= 1;
        ((_zGust*)to)->debris_timer = 0.15f;
        break;
    case 0xA:
        xBaseReset(to, ((_zGust*)to)->asset);
        ((_zGust*)to)->flags = ((_zGust*)to)->asset->flags;
        break;
    }

    return 1;
}

static void UpdateGustFX(_zGust* g, float seconds)
{
    signed int total_debris;
    signed int i;

    if (g->asset->partMod <= 0.0f)
    {
        return;
    }

    g->debris_timer -= seconds;
    if (!(g->debris_timer <= 0.0f))
    {
        return;
    }

    _zVolume* volume = !g->asset->effectID ? g->volume : g->fx_volume;
    zVolumeAsset* asset = volume->asset;

    signed int vol_area = (signed int)((asset->box.box.upper.x - asset->box.box.lower.x) *
                                       (asset->box.box.upper.z - asset->box.box.lower.z));

    if (vol_area > 1000)
    {
        g->debris_timer = 1000000.0f;
        return;
    }

    total_debris = vol_area >> 5;
    if (total_debris > 5)
    {
        total_debris = 5;
    }
    else if (total_debris < 1)
    {
        total_debris = 1;
    }

    g->debris_timer = iurand() * 0.15f + 0.15f;

    _zParEmitter* e = g->asset->flags & 2 ? sGustDustEmitter : sGustDebrisEmitter;
    if (!e)
    {
        return;
    }

    _zParEmitterCustomSettings info;
    info.custom_flags = 0x61;

    for (i = 0; i < total_debris; i++)
    {
        if (g->asset->effectID == 0)
        {
            asset = g->volume->asset;
            _xBBox* box = &asset->box;

            info.pos = box->box.lower;
            info.pos.x = (box->box.upper.x - box->box.lower.x) * iurand() + info.pos.x;
            info.pos.z = (box->box.upper.z - box->box.lower.z) * iurand() + info.pos.z;
            info.vel.x = 0.0f;
            info.vel.y = 5.0f;
            info.vel.z = 0.0f;
            info.life = (box->box.upper.y - box->box.lower.y) / 5.0f;
            info.life *= 2.0f;
        }
        else
        {
            asset = g->fx_volume->asset;
            _xBBox* box = &asset->box;

            info.pos.x = (box->box.upper.x - box->box.lower.x) * iurand() + box->box.lower.x;
            info.pos.y = (box->box.upper.y - box->box.lower.y) * iurand() + box->box.lower.y;
            info.pos.z = (box->box.upper.z - box->box.lower.z) * iurand() + box->box.lower.z;
            info.vel.x = 1.5f * g->asset->vel.x;
            info.vel.y = 1.5f * g->asset->vel.y;
            info.vel.z = 1.5f * g->asset->vel.z;
            info.life = 7.5f * g->asset->fade;
        }

        info.life *= g->asset->partMod;
        zParEmitterEmitCustom(e, 1.0f / 30.0f, &info);
    }
}

void zGustUpdateFX(float seconds)
{
    for (signed int i = 0; i < ngusts; i++)
    {
        _zGust* curr = &gusts[i];
        if (curr->flags & 1)
        {
            UpdateGustFX(curr, seconds);
        }
    }
}

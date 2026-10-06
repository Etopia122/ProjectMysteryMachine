#include "zGlobals.h"
#include "zScene.h"
#include "xMemMgr.h"
#include "xString.h"
#include "rwtypes.h"

extern "C" void* memset(void* dst, int c, unsigned int n);

class _xEnv;
class _zVolume;

class _xFColor
{
public:
    float r; // 0x0
    float g; // 0x4
    float b; // 0x8
    float a; // 0xC
};

class RwLinkList
{
public:
    RwLLLink link; // 0x0
};

class RpLightHW
{
public:
    RwObject object;     // 0x0
    unsigned char pad8[0x34 - 0x8];
    RwLLLink inWorld;    // 0x34
};

class RpWorldView
{
public:
    unsigned char pad0[0x3C];
    RwLinkList directionalLightList; // 0x3C
};

class iLight
{
public:
    unsigned int type;  // 0x0
    RpLightHW* hw;      // 0x4
    _xSphere sph;       // 0x8
    float radius_sq;    // 0x18
    _xFColor color;     // 0x1C
    _xVec3 dir;         // 0x2C
    float coneangle;    // 0x38
};

class zLightAsset : public p2BaseAsset
{
public:
    unsigned char lightType;   // 0x8
    unsigned char lightEffect; // 0x9
    unsigned char lightPad[2]; // 0xA
    unsigned int lightFlags;   // 0xC
    float lightColor[4];       // 0x10
    _xVec3 lightDir;           // 0x20
    float lightConeAngle;      // 0x2C
    _xSphere lightSphere;      // 0x30
    unsigned int attachID;     // 0x40
};

class _zLight : public xBase
{
public:
    unsigned int flags;   // 0x10
    zLightAsset* tasset;  // 0x14
    iLight light;         // 0x18
    xBase* attached_to;   // 0x54
    signed int true_idx;  // 0x58
    float* reg;           // 0x5C
    signed int effect_idx; // 0x60
};

class _tagPartLink
{
public:
    void* data;          // 0x0
    _tagPartLink* next;  // 0x4
};

class _tagPartSpace
{
public:
    signed int total;    // 0x0
    _tagPartLink head;   // 0x4
};

class _tagPartition
{
public:
    _xVec3 min;           // 0x0
    _xVec3 max;           // 0xC
    _xVec3 space_dim;     // 0x18
    signed int total_x;   // 0x24
    signed int total_y;   // 0x28
    signed int total_z;   // 0x2C
    _tagPartSpace* space; // 0x30
    _tagPartSpace global; // 0x34
};

class _xEntShadow
{
public:
    _xVec3 pos; // 0x0
    _xVec3 vec; // 0xC
};

extern RpWorldView* gLightWorld;

iLight* iLightCreate(iLight* light, unsigned int type);
void iLightModify(iLight* light, unsigned int flags);
void iLightSetPos(iLight* light, _xVec3* pos);
void iLightDestroy(iLight* light);
void iLightEnv(iLight* light, signed int env);
void iLightAdd(iLight* light);
void iLightRemove(iLight* light);
signed int xPartitionInsert(_tagPartition* part, void* insert_data, _xVec3* insert_pos);
signed int xPartitionUpdate(_tagPartition* part, void* data, signed int old_idx, _xVec3* current_pos);
void xPartitionVolume(_tagPartition* part, _zVolume* volume, signed int partition_x,
                      signed int partition_y, signed int partition_z);
void xPartitionWorld(_tagPartition* part, _xEnv* env, signed int partition_x,
                     signed int partition_y, signed int partition_z);
void xPartitionDump(_tagPartition* part, char* name);
void xPartitionGetVirtualIdx(_tagPartition* part, _xVec3* pos, signed int* x, signed int* y,
                             signed int* z);
void xPartitionGetLoop(_tagPartition* part, signed int range, signed int* sx, signed int* sy,
                       signed int* sz, signed int x, signed int y, signed int z, signed int* ex,
                       signed int* ey, signed int* ez);
signed int xPartitionGetTrueIdx(_tagPartition* part, signed int x, signed int y, signed int z);
_tagPartSpace* xPartitionGetSpace(_tagPartition* part, signed int idx);
float xVec3Normalize(_xVec3* o, _xVec3* v);
void xShadowSetLight(_xVec3* pos, _xVec3* vec, float shadow_depth);
signed int zGameIsPaused();

void zLightEffectCauldron(_zLight* zlight, float seconds);
void zLightEffectInitCauldron(_zLight* zlight);
void zLightEffectRandomColFast(_zLight* zlight, float seconds);
void zLightEffectRandomCol(_zLight* zlight, float seconds);
void zLightEffectRandomColSlow(_zLight* zlight, float seconds);
void zLightEffectHalfDimFast(_zLight* zlight, float seconds);
void zLightEffectHalfDim(_zLight* zlight, float seconds);
void zLightEffectHalfDimSlow(_zLight* zlight, float seconds);
void zLightEffectDimFast(_zLight* zlight, float seconds);
void zLightEffectDim(_zLight* zlight, float seconds);
void zLightEffectDimSlow(_zLight* zlight, float seconds);
void zLightEffectStrobeFast(_zLight* zlight, float seconds);
void zLightEffectStrobe(_zLight* zlight, float seconds);
void zLightEffectStrobeSlow(_zLight* zlight, float seconds);
void zLightEffectFlickerErratic(_zLight* zlight, float seconds);
void zLightEffectFlickerSlow(_zLight* zlight, float seconds);
void zLightEffectFlicker(_zLight* zlight, float seconds);
void zLightEffectInitFlicker(_zLight* zlight);
void zLightEffectInitRandomCol(_zLight* zlight);
void zLightEffectInitHalfDim(_zLight* zlight);
void zLightEffectInitDim(_zLight* zlight);
void zLightEffectInitStrobe(_zLight* zlight);

signed int zLightEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                         xBase* toParamWidget);

static _zLight* sLight[32];
static _tagPartition sLightPart;
static _zLight* gTemporaryLights[32];
static signed int sLightTotal;
static _zVolume* sPartitionVolume;
static signed int gNumTemporaryLights;

static void (*sEffectFuncs[18])(_zLight*, float) = {
    0,
    0,
    zLightEffectFlickerSlow,
    zLightEffectFlicker,
    zLightEffectFlickerErratic,
    zLightEffectStrobeSlow,
    zLightEffectStrobe,
    zLightEffectStrobeFast,
    zLightEffectDimSlow,
    zLightEffectDim,
    zLightEffectDimFast,
    zLightEffectHalfDimSlow,
    zLightEffectHalfDim,
    zLightEffectHalfDimFast,
    zLightEffectRandomColSlow,
    zLightEffectRandomCol,
    zLightEffectRandomColFast,
    zLightEffectCauldron,
};

static void (*sEffectInitFuncs[18])(_zLight*) = {
    0,
    0,
    zLightEffectInitFlicker,
    zLightEffectInitFlicker,
    zLightEffectInitFlicker,
    zLightEffectInitStrobe,
    zLightEffectInitStrobe,
    zLightEffectInitStrobe,
    zLightEffectInitDim,
    zLightEffectInitDim,
    zLightEffectInitDim,
    zLightEffectInitHalfDim,
    zLightEffectInitHalfDim,
    zLightEffectInitHalfDim,
    zLightEffectInitRandomCol,
    zLightEffectInitRandomCol,
    zLightEffectInitRandomCol,
    zLightEffectInitCauldron,
};

static _xVec3 sDefaultShadowVec = { 0.0f, 1.0f, 0.0f };

static inline void zLightEffectSet(_zLight* zlight, signed int idx)
{
    if (zlight->reg)
    {
        zlight->effect_idx = idx;
        if (sEffectInitFuncs[zlight->effect_idx])
        {
            sEffectInitFuncs[zlight->effect_idx](zlight);
        }
    }
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zLightResetAll(_xEnv* env)
{
    memset(sLight, 0, sizeof(sLight));
    sLightTotal = 0;
    if (sPartitionVolume)
    {
        xPartitionVolume(&sLightPart, sPartitionVolume, 10, 1, 10);
    }
    else
    {
        xPartitionWorld(&sLightPart, env, 10, 1, 10);
    }
    xPartitionDump(&sLightPart, "Lighting");
}

void zLightInit(xBase* b, zLightAsset* tasset);

void zLightInit(void* b, void* tasset)
{
    zLightInit((xBase*)b, (zLightAsset*)tasset);
}

void zLightInit(xBase* b, zLightAsset* tasset)
{
    _zLight* t = (_zLight*)b;

    xBaseInit(b, tasset);
    t->eventFunc = zLightEventCB;
    t->tasset = tasset;
    if (b->linkCount)
    {
        b->link = (p2LinkAsset*)(t->tasset + 1);
    }
    else
    {
        b->link = 0;
    }

    unsigned int itype = 1;
    switch (tasset->lightType)
    {
    case 0:
        itype = 1;
        break;
    case 1:
        itype = 2;
        break;
    case 2:
        itype = 1;
        break;
    case 3:
        itype = 1;
        break;
    }

    if (iLightCreate(&t->light, itype))
    {
        sLight[sLightTotal++] = t;
        t->light.sph.center.x = tasset->lightSphere.center.x;
        t->light.sph.center.y = tasset->lightSphere.center.y;
        t->light.sph.center.z = tasset->lightSphere.center.z;
        t->light.sph.r = tasset->lightSphere.r;
        t->light.color.r = tasset->lightColor[0];
        t->light.color.g = tasset->lightColor[1];
        t->light.color.b = tasset->lightColor[2];
        t->light.color.a = tasset->lightColor[3];
        t->light.coneangle = tasset->lightConeAngle;
        t->light.radius_sq = t->light.sph.r * t->light.sph.r;
        iLightModify(&t->light, 31);
        t->flags = 0;
        if (t->tasset->lightFlags & 0x20)
        {
            t->flags |= 1;
        }
        if (t->tasset->lightEffect)
        {
            t->reg = (float*)xMemAlloc(gActiveHeap, 32);
            zLightEffectSet(t, t->tasset->lightEffect);
        }
        else
        {
            t->reg = 0;
        }
        t->true_idx = xPartitionInsert(&sLightPart, t, &t->light.sph.center);
    }
}

void zLightResolveLinks()
{
    for (signed int i = 0; i < sLightTotal; i++)
    {
        _zLight* zl = sLight[i];
        if (zl->tasset->attachID)
        {
            zl->attached_to = zSceneFindObject(zl->tasset->attachID);
        }
        else
        {
            zl->attached_to = 0;
        }
    }
}

void zLightDestroyAll()
{
    signed int total = sLightTotal;
    for (signed int i = 0; i < total; i++, sLightTotal--)
    {
        iLightDestroy(&sLight[i]->light);
    }
    sLightTotal = 0;
}

void zLightSave(_zLight* ent, xSerial* s)
{
    xBaseSave(ent, s);
}

void zLightLoad(_zLight* ent, xSerial* s)
{
    xBaseLoad(ent, s);
}

signed int zLightEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                         xBase* toParamWidget)
{
    _zLight* t = (_zLight*)to;

    switch (toEvent)
    {
    case 0x26:
        t->flags |= 1;
        break;
    case 0x27:
        t->flags &= ~1;
        break;
    case 10:
        xBaseReset(t, t->tasset);
        t->flags = 0;
        if (t->tasset->lightFlags & 0x20)
        {
            t->flags |= 1;
        }
        zLightEffectSet(t, t->tasset->lightEffect);
        break;
    }
    return 1;
}

void zLightUpdate(xBase* to, _xScene*, float dt)
{
    _zLight* t = (_zLight*)to;

    if ((t->flags & 1) && t->attached_to)
    {
        _xVec3 pos = ((_xMat4x3*)((_xEnt*)t->attached_to)->model->Mat)->pos;
        pos.y += 1.0f;
        iLightSetPos(&t->light, &pos);
        t->light.sph.center = pos;
        t->true_idx = xPartitionUpdate(&sLightPart, t, t->true_idx, &pos);
    }
    if (sEffectFuncs[t->effect_idx])
    {
        sEffectFuncs[t->effect_idx](t, dt);
    }
}

void zLightAddLocalEnv()
{
    for (signed int i = 0; i < sLightTotal; i++)
    {
        _zLight* zlight = sLight[i];
        if ((zlight->flags & 1) && (zlight->tasset->lightFlags & 8))
        {
            iLight* light = &zlight->light;
            iLightEnv(light, 1);
            light->hw->inWorld.prev = gLightWorld->directionalLightList.link.prev;
            light->hw->inWorld.next = &gLightWorld->directionalLightList.link;
            gLightWorld->directionalLightList.link.prev->next = &light->hw->inWorld;
            gLightWorld->directionalLightList.link.prev = &light->hw->inWorld;
            gTemporaryLights[gNumTemporaryLights] = zlight;
            gNumTemporaryLights++;
        }
    }
}

void zLightAddLocal(_xEnt* ent)
{
    _zLight* lights[32];
    float dists[32];
    _xVec3 dirs[32];
    _xVec3* pos = &((_xMat4x3*)ent->model->Mat)->pos;
    signed int count = 0;
    _xVec3 default_light_pos = *pos;
    default_light_pos.y += 1.0f;

    if (!ent->ent_shadow)
    {
        ent->ent_shadow = (_xEntShadow*)xMemAlloc(gActiveHeap, sizeof(_xEntShadow));
        if (ent->ent_shadow)
        {
            ent->ent_shadow->pos = default_light_pos;
            ent->ent_shadow->vec = sDefaultShadowVec;
        }
    }

    signed int vx, vz;
    signed int sx, sz, ex, ez;
    xPartitionGetVirtualIdx(&sLightPart, pos, &vx, 0, &vz);
    xPartitionGetLoop(&sLightPart, 2, &sx, 0, &sz, vx, 0, vz, &ex, 0, &ez);

    for (signed int z = sz; z <= ez; z++)
    {
        for (signed int x = sx; x <= ex; x++)
        {
            signed int idx = xPartitionGetTrueIdx(&sLightPart, x, 0, z);
            _tagPartSpace* space = xPartitionGetSpace(&sLightPart, idx);
            if (space->total > 0)
            {
                _tagPartLink* link = &space->head;
                while (link->next)
                {
                    link = link->next;
                    _zLight* zl = (_zLight*)link->data;
                    if (zl->flags & 1)
                    {
                        unsigned int mask = 0;
                        switch (ent->collType)
                        {
                        case 2:
                        case 4:
                            mask |= 4;
                            break;
                        case 8:
                            mask |= 1;
                            break;
                        case 16:
                            mask |= 2;
                            break;
                        default:
                            return;
                        }

                        if (zl->tasset->lightFlags & mask)
                        {
                            iLight* light = &zl->light;
                            float dx = light->sph.center.x - pos->x;
                            float dy = light->sph.center.y - pos->y;
                            float dz = light->sph.center.z - pos->z;
                            if (1.0f + (dx * dx + dy * dy + dz * dz) <= light->radius_sq)
                            {
                                iLightEnv(light, 2);
                                iLightAdd(light);
                                gTemporaryLights[gNumTemporaryLights] = zl;
                                gNumTemporaryLights++;
                            }
                        }

                        if (zl->tasset->lightFlags & 0x10)
                        {
                            float dx = zl->light.sph.center.x - pos->x;
                            float dy = zl->light.sph.center.y - pos->y;
                            float dz = zl->light.sph.center.z - pos->z;
                            float d2 = 1.0f + (dx * dx + dy * dy + dz * dz);
                            if (d2 < zl->light.radius_sq)
                            {
                                dirs[count].x = dx;
                                dirs[count].y = dy;
                                dirs[count].z = dz;
                                lights[count] = zl;
                                dists[count] = d2;
                                count++;
                            }
                        }
                    }
                }
            }
        }
    }

    if (count)
    {
        _xVec3 dir = { 0.0f, 2.0f, 0.0f };
        _xVec3 lpos = { 0.0f, 0.0f, 0.0f };
        for (signed int i = 0; i < count; i++)
        {
            xVec3Normalize(&dirs[i], &dirs[i]);
            float w = 1.0f - dists[i] / lights[i]->light.radius_sq;
            dir.x = w * dirs[i].x + dir.x;
            dir.y = w * dirs[i].y + dir.y;
            dir.z = w * dirs[i].z + dir.z;
            lpos.x += lights[i]->light.sph.center.x;
            lpos.y += lights[i]->light.sph.center.y;
            lpos.z += lights[i]->light.sph.center.z;
        }
        lpos.x /= count;
        lpos.y /= count;
        lpos.z /= count;

        if (!zGameIsPaused() && ent->ent_shadow)
        {
            _xVec3 p;
            _xVec3 v;
            xVec3Lerp(&v, &ent->ent_shadow->vec, &dir, 0.1f);
            xVec3Lerp(&p, &ent->ent_shadow->pos, &lpos, 0.1f);
            ent->ent_shadow->pos = p;
            ent->ent_shadow->vec = v;
        }
    }
    else if (!zGameIsPaused() && ent->ent_shadow)
    {
        _xVec3 p;
        _xVec3 v;
        xVec3Lerp(&v, &ent->ent_shadow->vec, &sDefaultShadowVec, 0.01f);
        xVec3Lerp(&p, &ent->ent_shadow->pos, &default_light_pos, 0.01f);
        ent->ent_shadow->pos = p;
        ent->ent_shadow->vec = v;
    }

    if (ent->ent_shadow)
    {
        xShadowSetLight(&ent->ent_shadow->pos, &ent->ent_shadow->vec, 1.0f);
    }
}

void zLightRemoveLocalEnv()
{
    for (signed int i = 0; i < gNumTemporaryLights; i++)
    {
        RwLLLink* link = gLightWorld->directionalLightList.link.prev;
        link->prev->next = link->next;
        link->next->prev = link->prev;
    }
    gNumTemporaryLights = 0;
}

void zLightRemoveLocal()
{
    for (signed int i = 0; i < gNumTemporaryLights; i++)
    {
        iLightRemove(&gTemporaryLights[i]->light);
    }
    gNumTemporaryLights = 0;
}

void zLightSetVolume(_zVolume* vol)
{
    if (!vol)
    {
        sPartitionVolume = 0;
    }
    else if (((xBase*)vol)->id == xStrHash("LIGHT_PARTITION"))
    {
        sPartitionVolume = vol;
    }
}

void zLightOn(_zLight* zl, signed int on)
{
    if (on)
    {
        zl->flags |= 1;
    }
    else
    {
        zl->flags &= ~1;
    }
}

#include "xCollide.h"
#include "xSurface.h"
#include "xMath.h"
#include "xMath3.h"
#include "xMemMgr.h"
#include "xstransvc.h"
#include "iMath.h"
#include "zEnt.h"
#include "zScene.h"
#include "zSurface.h"

extern "C" char* strcpy(char* dst, const char* src);

class _zGroup;
unsigned int zGroupGetCount(_zGroup* g);

extern unsigned int gFrameCount;


class zMaterialMapEntry
{
public:
    unsigned int surfaceAssetID;
    unsigned int materialIndex;
};

class zMaterialMapAsset
{
public:
    unsigned int id;
    unsigned int count;
};

static zSurfaceProps* zsps;
static signed int sMapperCount;
static zMaterialMapAsset* sMapper[1] = { 0 };
static xSurface sDef_surf;
static zSurfaceProps sDef_surf_props;
static zSurfAsset sDef_surf_asset;

static signed int zSurfaceEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                                  xBase* toParamWidget);

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zSurfaceInit()
{
    unsigned int size;
    unsigned short nsurfs = xSTAssetCountByType('SURF');

    xSurfaceInit(nsurfs);

    if (nsurfs)
    {
        zsps = (zSurfaceProps*)xMemAlloc(gActiveHeap, nsurfs * sizeof(zSurfaceProps));

        for (unsigned short i = 0; i < nsurfs; i++)
        {
            zSurfAsset* asset;
            xSurface* surf;
            zSurfaceProps* moprops;

            surf = xSurfaceGetByIdx(i);
            moprops = &zsps[i];
            surf->moprops = moprops;

            asset = (zSurfAsset*)xSTFindAssetByType('SURF', i, &size);
            moprops->asset = asset;

            xBaseInit(surf, asset);

            if (surf->linkCount)
            {
                surf->link = (p2LinkAsset*)(asset + 1);
            }
            else
            {
                surf->link = 0;
            }

            surf->eventFunc = zSurfaceEventCB;
            surf->friction = asset->friction;
            surf->state = asset->on ? 0 : 1;
            moprops->uvfx_flags = asset->uvfx_flags;
            moprops->texanim_flags = asset->texture_anim_flags;

            for (signed int j = 0; j < 2; j++)
            {
                moprops->texanim[j].mode = asset->texture_anim[j].mode;
                moprops->texanim[j].speed = asset->texture_anim[j].speed;
                moprops->texanim[j].group = asset->texture_anim[j].group;
                moprops->texanim[j].group_idx = 0;
                moprops->uvfx[j].mode = asset->uvfx[j].mode;
                if (moprops->uvfx[j].rot <= 360.0f)
                {
                    moprops->uvfx[j].rot = 360.0f;
                }
                if (moprops->uvfx[j].rot >= 0.0f)
                {
                    moprops->uvfx[j].rot = 0.0f;
                }
                moprops->uvfx[j].rot = asset->uvfx[j].rot;
                moprops->uvfx[j].rot_spd = asset->uvfx[j].rot_spd;
                moprops->uvfx[j].trans = asset->uvfx[j].trans;
                moprops->uvfx[j].trans_spd = asset->uvfx[j].trans_spd;
                moprops->uvfx[j].scale = asset->uvfx[j].scale;
                moprops->uvfx[j].scale_spd = asset->uvfx[j].scale_spd;
            }
        }
    }
    else
    {
        zsps = 0;
    }

    sDef_surf.friction = 1.0f;
    sDef_surf.state = 0;
    sDef_surf.moprops = &sDef_surf_props;
    sDef_surf_props.asset = &sDef_surf_asset;
    sDef_surf_asset.game_damage_type = 0;
    sDef_surf_asset.game_sticky = 0;
    sDef_surf_asset.game_damage_flags = 0;
    sDef_surf_asset.sld_start = 20;
    sDef_surf_asset.sld_stop = 10;
    sDef_surf_asset.phys_flags = 0;
    sDef_surf_asset.friction = 1.0f;
}

void zSurfaceRegisterMapper(unsigned int assetId)
{
    if (sMapperCount >= 1)
    {
        return;
    }
    if (!assetId)
    {
        return;
    }

    sMapper[sMapperCount] = (zMaterialMapAsset*)xSTFindAsset(assetId, 0);
    if (sMapper[sMapperCount])
    {
        sMapperCount++;
    }
}

void zSurfaceExit()
{
    xSurfaceExit();
    sMapperCount = 0;
}

#pragma auto_inline on
xSurface* zSurfaceGetSurface(unsigned int mat_id)
{
    for (signed int map = 0; map < sMapperCount; map++)
    {
        zMaterialMapAsset* mapper = sMapper[map];
        if (mapper)
        {
            for (unsigned short i = 0; i < mapper->count; i++)
            {
                zMaterialMapEntry* entry = (zMaterialMapEntry*)(mapper + 1) + i;
                if (entry->materialIndex == mat_id)
                {
                    unsigned short nsurfs = xSurfaceGetNumSurfaces();
                    for (unsigned short j = 0; j < nsurfs; j++)
                    {
                        xSurface* surf = xSurfaceGetByIdx(j);
                        if (surf->id == entry->surfaceAssetID)
                        {
                            return surf;
                        }
                    }
                }
            }
        }
    }
    return &sDef_surf;
}
#pragma auto_inline off

xSurface* zSurfaceGetSurface(_xCollis* coll)
{
    xSurface* surf = 0;

    if (coll->flags & 1)
    {
        if (coll->optr)
        {
            surf = ((_xEnt*)coll->optr)->surf;
        }
        else
        {
            surf = zSurfaceGetSurface(coll->oid);
        }
    }
    if (surf)
    {
        return surf;
    }
    return &sDef_surf;
}

unsigned int zSurfaceGetSlide(xSurface* surf)
{
    if (surf->moprops)
    {
        return ((zSurfaceProps*)surf->moprops)->asset->phys_flags & 1;
    }
    return 0;
}

unsigned int zSurfaceGetStep(xSurface* surf)
{
    if (surf->moprops)
    {
        return ((zSurfaceProps*)surf->moprops)->asset->phys_flags & 4;
    }
    return 0;
}

float zSurfaceGetSlideStartAngle(xSurface* surf)
{
    if (surf->moprops)
    {
        return (3.1415927f * ((zSurfaceProps*)surf->moprops)->asset->sld_start) / 180.0f;
    }
    return 0.34906587f;
}

float zSurfaceGetSlideStopAngle(xSurface* surf)
{
    if (surf->moprops)
    {
        return (3.1415927f * ((zSurfaceProps*)surf->moprops)->asset->sld_stop) / 180.0f;
    }
    return 0.17453294f;
}

unsigned int zSurfaceGetMatchOrient(xSurface* surf)
{
    if (surf->moprops)
    {
        return ((zSurfaceProps*)surf->moprops)->asset->phys_flags & 2;
    }
    return 0;
}

signed int zSurfaceGetDamageType(xSurface* surf)
{
    if (surf->moprops)
    {
        return ((zSurfaceProps*)surf->moprops)->asset->game_damage_type;
    }
    return 0;
}

unsigned int zSurfaceGetDamagePassthrough(xSurface* surf)
{
    if (surf->moprops)
    {
        return ((zSurfaceProps*)surf->moprops)->asset->game_damage_flags & 1;
    }
    return 0;
}

unsigned int zSurfaceGetSticky(xSurface* surf)
{
    if (surf->moprops)
    {
        return ((zSurfaceProps*)surf->moprops)->asset->game_sticky;
    }
    return 0;
}

unsigned int zSurfaceGetStandOn(xSurface* surf)
{
    if (surf->moprops)
    {
        return (unsigned char)!(((zSurfaceProps*)surf->moprops)->asset->phys_flags & 8);
    }
    return 1;
}

float zSurfaceGetFriction(xSurface* surf)
{
    return surf->friction;
}

signed int zSurfaceGetSlickness(xSurface* surf)
{
    return (signed int)(1.0f / surf->friction);
}

float zSurfaceGetDamping(xSurface* surf, float min_vel)
{
    return xpow(min_vel, surf->friction);
}

void zSurfaceSave(xSurface* ent, xSerial* s)
{
    xSurfaceSave(ent, s);
}

void zSurfaceLoad(xSurface* ent, xSerial* s)
{
    xSurfaceLoad(ent, s);
}

void zSurfaceSetup(xSurface* s)
{
    zSurfaceProps* pp = (zSurfaceProps*)s->moprops;

    if (pp)
    {
        for (signed int i = 0; i < 2; i++)
        {
            pp->texanim[i].group_ptr = 0;
            if (pp->texanim[i].group)
            {
                pp->texanim[i].group_ptr = zSceneFindObject(pp->texanim[i].group);
            }
        }
    }
}

void zSurfaceUpdate(xBase* to, float dt)
{
    signed int j;
    xSurface* t = (xSurface*)to;
    zSurfaceProps* moprops = (zSurfaceProps*)t->moprops;

    for (j = 0; j < 2; j++)
    {
        if ((j == 0 && (moprops->uvfx_flags & 1)) || (j == 1 && (moprops->uvfx_flags & 2)))
        {
            switch (moprops->uvfx[j].mode)
            {
            case 1:
                    moprops->uvfx[j].trans.x = isin(1.0f / 60 * (2.0f * gFrameCount));
                    moprops->uvfx[j].trans.y = isin(1.0f / 60 * (2.0f * gFrameCount));
                    moprops->uvfx[j].scale.x = isin(1.0f / 60 * (2.0f * gFrameCount));
                    moprops->uvfx[j].scale.y = isin(1.0f / 60 * (2.0f * gFrameCount));
            
                break;
            default:
            {
                _xVec3 d;

                    moprops->uvfx[j].rot += moprops->uvfx[j].rot_spd * dt;
                    while (moprops->uvfx[j].rot >= 360.0f)
                    {
                        moprops->uvfx[j].rot -= 360.0f;
                    }
                    while (moprops->uvfx[j].rot < 0.0f)
                    {
                        moprops->uvfx[j].rot += 360.0f;
                    }

                    xVec3SMul(&d, &moprops->uvfx[j].trans_spd, dt);
                    xVec3Add(&moprops->uvfx[j].trans, &d, &moprops->uvfx[j].trans);
                    while (moprops->uvfx[j].trans.x >= 1.0f)
                    {
                        moprops->uvfx[j].trans.x -= 1.0f;
                    }
                    while (moprops->uvfx[j].trans.x < 0.0f)
                    {
                        moprops->uvfx[j].trans.x += 1.0f;
                    }
                    while (moprops->uvfx[j].trans.y >= 1.0f)
                    {
                        moprops->uvfx[j].trans.y -= 1.0f;
                    }
                    while (moprops->uvfx[j].trans.y < 0.0f)
                    {
                        moprops->uvfx[j].trans.y += 1.0f;
                    }

                    xVec3SMul(&d, &moprops->uvfx[j].scale_spd, dt);
                    xVec3Add(&moprops->uvfx[j].scale, &d, &moprops->uvfx[j].scale);
                    while (moprops->uvfx[j].scale.x >= 8.0f)
                    {
                        moprops->uvfx[j].scale.x -= 8.0f;
                    }
                    while (moprops->uvfx[j].scale.x < 0.0f)
                    {
                        moprops->uvfx[j].scale.x += 8.0f;
                    }
                    while (moprops->uvfx[j].scale.y >= 8.0f)
                    {
                        moprops->uvfx[j].scale.y -= 8.0f;
                    }
                    while (moprops->uvfx[j].scale.y < 0.0f)
                    {
                        moprops->uvfx[j].scale.y += 8.0f;
                    }
            
            }
                break;
            }
        }
    }

    for (j = 0; j < 2; j++)
    {
        if ((j == 0 && (moprops->texanim_flags & 1)) || (j == 1 && (moprops->texanim_flags & 2)))
        {
            _zGroup* g = (_zGroup*)moprops->texanim[j].group_ptr;
            if (g)
            {
                signed int max = zGroupGetCount(g);
                if (max > 0)
                {
                    if (moprops->texanim[j].mode == 0)
                    {
                        moprops->texanim[j].frame += moprops->texanim[j].speed * dt;
                        if (moprops->texanim[j].frame >= 1.0f)
                        {
                            moprops->texanim[j].group_idx += (signed int)moprops->texanim[j].frame;
                            moprops->texanim[j].frame -= (signed int)moprops->texanim[j].frame;
                        }
                    }
                    else if (moprops->texanim[j].mode == 1)
                    {
                        moprops->texanim[j].frame += moprops->texanim[j].speed * dt;
                        if (moprops->texanim[j].frame >= 1.0f)
                        {
                            moprops->texanim[j].group_idx -= (signed int)moprops->texanim[j].frame;
                            moprops->texanim[j].frame -= (signed int)moprops->texanim[j].frame;
                        }
                    }
                    else if (moprops->texanim[j].mode == 2)
                    {
                        moprops->texanim[j].frame += moprops->texanim[j].speed * dt;
                        if (moprops->texanim[j].frame >= 1.0f)
                        {
                            moprops->texanim[j].group_idx = irand() % max;
                            moprops->texanim[j].frame -= (signed int)moprops->texanim[j].frame;
                        }
                    }
                    while (moprops->texanim[j].group_idx >= max)
                    {
                        moprops->texanim[j].group_idx -= max;
                    }
                    while (moprops->texanim[j].group_idx < 0)
                    {
                        moprops->texanim[j].group_idx += max;
                    }
                }
            }
        }
    }
}

static signed int zSurfaceEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                                  xBase* toParamWidget)
{
    xSurface* t = (xSurface*)to;

    switch (toEvent)
    {
    case 0x26:
        t->state = 0;
        break;
    case 0x27:
        t->state = 1;
        break;
    case 0xF:
        if (t->state == 0)
        {
            t->state = 1;
        }
        else
        {
            t->state = 0;
        }
        break;
    case 0xA:
        xSurfaceReset(t);
        t->friction = ((zSurfaceProps*)t->moprops)->asset->friction;
        t->state = 0;
        break;
    case 0xF1:
    {
        zSurfaceProps* p = (zSurfaceProps*)t->moprops;
        if (p)
        {
            if (0.0f == toParam[3])
            {
                p->texanim_flags |= 1;
            }
            else
            {
                p->texanim_flags |= 2;
            }
        }
        break;
    }
    case 0xF9:
    {
        zSurfaceProps* p = (zSurfaceProps*)t->moprops;
        if (p && (p->texanim_flags & 1))
        {
            if (0.0f == toParam[3])
            {
                p->texanim[0].frame += toParam[0];
            }
            else
            {
                p->texanim[1].frame += toParam[0];
            }
        }
        break;
    }
    case 0xF2:
    {
        zSurfaceProps* p = (zSurfaceProps*)t->moprops;
        if (p)
        {
            if (0.0f == toParam[3])
            {
                if (p->texanim_flags & 1)
                {
                    p->texanim_flags ^= 1;
                }
            }
            else
            {
                if (p->texanim_flags & 2)
                {
                    p->texanim_flags ^= 2;
                }
            }
        }
        break;
    }
    case 0xF3:
    {
        if (t->moprops)
        {
            if (0.0f == toParam[3])
            {
                if (((zSurfaceProps*)t->moprops)->texanim_flags & 1)
                {
                    if (((zSurfaceProps*)t->moprops)->texanim_flags & 1)
                    {
                        ((zSurfaceProps*)t->moprops)->texanim_flags ^= 1;
                    }
                }
                else
                {
                    ((zSurfaceProps*)t->moprops)->texanim_flags |= 1;
                }
            }
            else
            {
                if (((zSurfaceProps*)t->moprops)->texanim_flags & 2)
                {
                    if (((zSurfaceProps*)t->moprops)->texanim_flags & 2)
                    {
                        ((zSurfaceProps*)t->moprops)->texanim_flags ^= 2;
                    }
                }
                else
                {
                    ((zSurfaceProps*)t->moprops)->texanim_flags |= 2;
                }
            }
        }
        break;
    }
    case 0xF7:
    {
        zSurfaceProps* p = (zSurfaceProps*)t->moprops;
        if (p)
        {
            if (toParamWidget)
            {
                signed int idx = (0.0f == toParam[3]) ? 0 : 1;
                p->texanim[idx].group = toParamWidget->id;
                p->texanim[idx].group_ptr = toParamWidget;
            }
            else
            {
                if (0.0f == toParam[3])
                {
                    p->texanim[0].group = 0;
                    p->texanim[0].group_ptr = 0;
                }
                else
                {
                    p->texanim[1].group = 0;
                    p->texanim[1].group_ptr = 0;
                }
            }
        }
        break;
    }
    case 0xF8:
    {
        zSurfaceProps* p = (zSurfaceProps*)t->moprops;
        if (p)
        {
            if (0.0f == toParam[3])
            {
                p->texanim[0].speed = toParam[0];
            }
            else
            {
                p->texanim[1].speed = toParam[0];
            }
        }
        break;
    }
    }

    return 1;
}

void zSurfaceGetName(signed int type, char* buffer)
{
    *buffer = 0;
    switch (type)
    {
    case 0:
        strcpy(buffer, "DEFAULT");
        break;
    case 1:
        strcpy(buffer, "TILE");
        break;
    case 2:
        strcpy(buffer, "CARPET");
        break;
    case 3:
        strcpy(buffer, "SHORTGRASS");
        break;
    case 4:
        strcpy(buffer, "LONGGRASS");
        break;
    case 5:
        strcpy(buffer, "GRAVEL");
        break;
    case 6:
        strcpy(buffer, "DIRT");
        break;
    case 7:
        strcpy(buffer, "MUD");
        break;
    case 8:
        strcpy(buffer, "THORNS");
        break;
    case 9:
        strcpy(buffer, "METAL");
        break;
    case 10:
        strcpy(buffer, "SHEETMETAL");
        break;
    case 11:
        strcpy(buffer, "CONCRETE");
        break;
    case 12:
        strcpy(buffer, "MARBLE");
        break;
    case 13:
        strcpy(buffer, "STONE");
        break;
    case 14:
        strcpy(buffer, "WOOD");
        break;
    case 15:
        strcpy(buffer, "SNOW");
        break;
    case 16:
        strcpy(buffer, "ICE");
        break;
    case 17:
        strcpy(buffer, "SHALLOWWATER");
        break;
    case 18:
        strcpy(buffer, "DEEPWATER");
        break;
    case 19:
        strcpy(buffer, "TAR");
        break;
    case 20:
        strcpy(buffer, "SPECIAL1");
        break;
    case 21:
        strcpy(buffer, "DeepAcid");
        break;
    case 22:
        strcpy(buffer, "NONE");
    }
}

#include "xEnt.h"
#include "xMath.h"
#include "xMemMgr.h"
#include "xstransvc.h"
#include "iFX.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zSurface.h"
#include "rwtypes.h"

#define NULL 0

extern "C" void* memcpy(void* dst, const void* src, unsigned int n);

#define SCENE_ID(s) ((((s)[0] & ~0x20) << 24) | ((s)[1] << 16) | ((s)[2] << 8) | (s)[3])

class RwTexture
{
public:
    unsigned char pad0[0x10];
    char name[32]; // 0x10
};

class RwTexCoords
{
public:
    float u;
    float v;
};

class RpMaterial
{
public:
    RwTexture* texture; // 0x0
    RwRGBA color;       // 0x4
};

class RpGeometry
{
public:
    RwObject object;           // 0x0
    unsigned int flags;        // 0x8
    unsigned char pad0C[0x14 - 0xC];
    signed int numVertices;    // 0x14
    unsigned char pad18[0x34 - 0x18];
    RwTexCoords* texCoords[8]; // 0x34
};

class RpAtomic
{
public:
    unsigned char pad0[0x18];
    RpGeometry* geometry; // 0x18
};

class RpWorld
{
public:
    unsigned char pad0[0x10];
    RpMaterial** materials;  // 0x10
    signed int numMaterials; // 0x14
};

class RpWorldSector;
class RpSkin;
class RxPipeline;

class RwCamera
{
public:
    RwObject object;
};

class RwGlobals
{
public:
    unsigned char pad0[0x114];
    signed int (*vecStrcmp)(const char*, const char*); // 0x114
};

typedef RpMaterial* (*RpMaterialCallBack)(RpMaterial*, void*);
typedef RpWorldSector* (*RpWorldSectorCallBack)(RpWorldSector*, void*);

extern RwGlobals* RwEngineInstance;

extern "C" {
RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry, RpMaterialCallBack fpCallBack,
                                      void* pData);
RpGeometry* RpGeometryLock(RpGeometry* geometry, signed int lockMode);
RpGeometry* RpGeometryUnlock(RpGeometry* geometry);
RpMaterial* RpMaterialSetTexture(RpMaterial* material, RwTexture* texture);
RpWorld* RpWorldForAllWorldSectors(RpWorld* world, RpWorldSectorCallBack fpCallBack, void* pData);
RpMaterial* RpMatFXMaterialSetEffects(RpMaterial* material, signed int flags);
signed int RpMatFXMaterialGetEffects(const RpMaterial* material);
RpMaterial* RpMatFXMaterialSetupEnvMap(RpMaterial* material, RwTexture* texture, RwFrame* frame,
                                       signed int useFrameBufferAlpha, float coef);
RpMaterial* RpMatFXMaterialSetupBumpMap(RpMaterial* material, RwTexture* texture, RwFrame* frame,
                                        float coef);
RpMaterial* RpMatFXMaterialSetEnvMapCoefficient(RpMaterial* material, float coef);
RpMaterial* RpMatFXMaterialSetBumpMapCoefficient(RpMaterial* material, float coef);
RpWorldSector* RpMatFXWorldSectorEnableEffects(RpWorldSector* worldSector);
RpAtomic* RpMatFXAtomicEnableEffects(RpAtomic* atomic);
RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry);
RpAtomic* RpSkinAtomicSetType(RpAtomic* atomic, signed int type);
RpLight* RpLightCreate(signed int type);
signed int RpLightDestroy(RpLight* light);
RwFrame* RwFrameCreate();
RwFrame* RwFrameRotate(RwFrame* frame, const RwV3d* axis, float angle, signed int combine);
void* _rwObjectHasFrameSetFrame(void* object, RwFrame* frame);
}

#define LO_CAM (*(RwCamera**)&globals.camera)

extern unsigned int gFXSurfaceFlags;

float EnvMapShininess = 1.0f;
RpLight* MainLight;
static unsigned int sAnimUVCount;
unsigned int xfx_initted;

float xFXanimUVRotMat0[2] = { 1.0f, 0.0f };
float xFXanimUVRotMat1[2] = { 0.0f, 1.0f };
float xFXanimUVTrans[2] = { 0.0f, 0.0f };
float xFXanimUVScale[2] = { 1.0f, 1.0f };
float xFXanimUV2PRotMat0[2] = { 1.0f, 0.0f };
float xFXanimUV2PRotMat1[2] = { 0.0f, 1.0f };
float xFXanimUV2PTrans[2] = { 0.0f, 0.0f };
float xFXanimUV2PScale[2] = { 1.0f, 1.0f };

static signed int sSetMaterialTextureBufferCount;
static unsigned int sMaterialAlpha;
static signed int sMaterialTotal;
static RxPipeline* xFXanimUVPipeline;
RwTexture* xFXanimUV2PTexture;

class xFXAnimUVCache
{
public:
    RpAtomic* atomic;
    RwTexCoords* uv;
};

static xFXAnimUVCache sAnimUVCache[32];
static RwTexture* sSetMaterialTextureBuffer[32];
static RwRGBA sMaterialColor[16];

static void LightResetFrame(RpLight* light);

void xFXInit()
{
    if (!xfx_initted)
    {
        xfx_initted = 1;

        RpLight* light = RpLightCreate(1);
        if (light)
        {
            RwFrame* frame = RwFrameCreate();
            if (frame)
            {
                _rwObjectHasFrameSetFrame(light, frame);
                LightResetFrame(light);
                MainLight = light;
            }
            else
            {
                RpLightDestroy(light);
            }
        }

        xFXanimUVPipeline = iFXanimUVCreatePipe();
    }
}

static RpMaterial* MaterialSetShininess(RpMaterial* material, void*);
RpMaterial* MaterialSetEnvMap2(RpMaterial* material, void* data);
static RpWorldSector* SectorMatFXEnableCB(RpWorldSector* worldSector, void*);
static RpMaterial* MaterialSetEnvMap(RpMaterial* material, void* data);

static inline RpMaterial* MaterialSetBumpMap(RpMaterial* material, void* data)
{
    if (data == NULL)
    {
        return NULL;
    }
    if (material->texture)
    {
        if (data)
        {
            RwFrame* frame = (RwFrame*)MainLight->object.parent;
            RpMatFXMaterialSetEffects(material, 1);
            RpMatFXMaterialSetupBumpMap(material, (RwTexture*)data, frame, 1.0f);
        }
        else
        {
            RpMatFXMaterialSetEffects(material, 0);
        }
    }
    return material;
}

static inline RpMaterial* MaterialSetBumpEnvMap(RpMaterial* material, RwTexture* envMap,
                                                float envCoef, RwTexture* bumpMap, float bumpCoef)
{
    if (envMap == NULL || bumpMap == NULL)
    {
        return NULL;
    }

    RwFrame* frame;
    RpMatFXMaterialSetEffects(material, 3);
    if (gFXSurfaceFlags & 0x10)
    {
        frame = (RwFrame*)LO_CAM->object.parent;
    }
    else
    {
        frame = (RwFrame*)MainLight->object.parent;
    }
    RpMatFXMaterialSetupEnvMap(material, envMap, frame, 1, envCoef);
    RpMatFXMaterialSetupBumpMap(material, bumpMap, (RwFrame*)MainLight->object.parent, bumpCoef);
    return material;
}

static inline signed int xFXBumpDisabled()
{
    return 0;
}

static inline RpAtomic* AtomicSetShininess(RpAtomic* atomic, void* data)
{
    RpGeometry* geometry = atomic->geometry;
    if (geometry)
    {
        RpGeometryForAllMaterials(geometry, MaterialSetShininess, data);
    }
    return atomic;
}

static inline RpAtomic* AtomicSetEnvMap(RpAtomic* atomic, void* data)
{
    RpMatFXAtomicEnableEffects(atomic);
    if (atomic->geometry)
    {
        RpGeometryForAllMaterials(atomic->geometry, MaterialSetEnvMap2, data);
    }
    return atomic;
}

static inline RpAtomic* xFXAtomicEnvMapSetup(RpAtomic* atomic, unsigned int envmapID,
                                             float shininess)
{
    void* asset = xSTFindAsset(envmapID, NULL);
    if (asset)
    {
        AtomicSetEnvMap(atomic, asset);
        float tmp = EnvMapShininess;
        EnvMapShininess = shininess;
        AtomicSetShininess(atomic, NULL);
        EnvMapShininess = tmp;
        if (RpSkinGeometryGetSkin(atomic->geometry))
        {
            RpSkinAtomicSetType(atomic, 2);
        }
        return atomic;
    }
    return NULL;
}

// TODO: 96.7%. The original tests each xSTFindAsset result in r3 and copies it
// into a saved register only where the inlined helper needs it; here the
// variable and the helper's parameter are coalesced into one saved register.
void xFX_SceneEnter(RpWorld* world)
{
    RpWorldForAllWorldSectors(world, SectorMatFXEnableCB, NULL);

    signed int i;
    signed int num;
    num = world->numMaterials;
    for (i = 0; i < num; i++)
    {
        xSurface* sp = zSurfaceGetSurface(i);
        zSurfaceProps* pp = (zSurfaceProps*)sp->moprops;

        if (pp && pp->asset)
        {
            zSurfMatFX* fxp = &pp->asset->matfx;

            if (fxp->flags)
            {
                if (fxp->flags == 0x10)
                {
                    fxp->flags |= 0x1;
                }

                RpMaterial* mp = world->materials[i];

                if (mp->texture)
                {
                    RwTexture* bump;
                    RwTexture* env;

                    gFXSurfaceFlags = fxp->flags;

                    if (fxp->flags & 0x1)
                    {
                        env = (RwTexture*)xSTFindAsset(fxp->envmapID, NULL);
                        if (!env)
                        {
                            continue;
                        }

                        MaterialSetEnvMap(mp, env);
                        RpMatFXMaterialSetEnvMapCoefficient(mp, 0.5f * fxp->shininess);
                    }

                    if (fxp->flags & 0x2)
                    {
                        if (globals.sceneCur->sceneID == 'H002')
                        {
                            continue;
                        }

                        bump = (RwTexture*)xSTFindAsset(fxp->bumpmapID, NULL);
                        static signed int sNoBump = xFXBumpDisabled();
                        if (!bump || sNoBump)
                        {
                            continue;
                        }

                        MaterialSetBumpMap(mp, bump);
                        RpMatFXMaterialSetBumpMapCoefficient(mp, fxp->bumpiness);
                    }

                    if (fxp->flags & 0x4)
                    {
                        static signed int sNoBumpEnv = xFXBumpDisabled();
                        env = (RwTexture*)xSTFindAsset(fxp->envmapID, NULL);
                        bump = (RwTexture*)xSTFindAsset(fxp->bumpmapID, NULL);
                        if (!env || !bump || sNoBumpEnv)
                        {
                            continue;
                        }

                        MaterialSetBumpEnvMap(mp, env, fxp->shininess, bump, fxp->bumpiness);
                    }
                }
            }
        }
    }

    zScene* sc = globals.sceneCur;
    for (i = 0; i < sc->num_act_ents; i++)
    {
        _xEnt* ent = sc->act_ents[i];

        if (ent->id == 0xEBB8C925 || ent->id == 0xE8E6A355)
        {
            xFXAtomicEnvMapSetup((RpAtomic*)ent->model->Data, 0xE891750E, 0.3f);
        }

        if (ent->baseType == 2)
        {
            switch (ent->subType)
            {
            case 1:
                xFXAtomicEnvMapSetup((RpAtomic*)ent->model->Data, 0xE891750E, 0.3f);
                break;
            case 4:
                break;
            case 7:
                break;
            }
        }

        if (ent->baseType == 4)
        {
            unsigned char* npc = *(unsigned char**)((unsigned char*)ent + 0xF0);
            if (npc[4] == 1)
            {
                switch (npc[5])
                {
                case 5:
                case 6:
                case 8:
                case 9:
                    xFXAtomicEnvMapSetup((RpAtomic*)ent->model->Data, 0xE891750E, 0.3f);
                    break;
                case 7:
                    break;
                }
            }
            else if (npc[4] == 2)
            {
                xFXAtomicEnvMapSetup((RpAtomic*)ent->model->Data, 0xBBBC909C, 0.5f);
            }
            else if (npc[4] == 4)
            {
                xFXAtomicEnvMapSetup((RpAtomic*)ent->model->Data, 0xE891750E, 2.0f);
            }
        }

        if (sc->sceneID == SCENE_ID("h003") && ent->baseType == 6)
        {
            xFXAtomicEnvMapSetup((RpAtomic*)ent->model->Data, 0xE891750E, 2.0f);
        }
    }

    sAnimUVCount = 0;
}

#pragma auto_inline on
static void LightResetFrame(RpLight* light)
{
    RwV3d v1 = { 1, 0, 0 };
    RwV3d v2 = { 0, 1, 0 };

    RwFrame* frame = (RwFrame*)light->object.parent;

    RwFrameRotate(frame, &v1, 45.0f, 0);
    RwFrameRotate(frame, &v2, 45.0f, 2);
}

#pragma auto_inline off

void xFX_SceneExit(RpWorld*)
{
}

void xFXUpdate(float dt)
{
    iFXWarbUpdate(dt);
}

static RpWorldSector* SectorMatFXEnableCB(RpWorldSector* worldSector, void*)
{
    RpMatFXWorldSectorEnableEffects(worldSector);
    return worldSector;
}

static RpMaterial* MaterialSetShininess(RpMaterial* material, void*)
{
    signed int flags = RpMatFXMaterialGetEffects(material);

    if (flags == 2 || flags == 3)
    {
        RpMatFXMaterialSetEnvMapCoefficient(material, EnvMapShininess);
    }

    return material;
}

RpMaterial* MaterialSetEnvMap2(RpMaterial* material, void* data)
{
    if (material->texture)
    {
        RwTexture* texture = (RwTexture*)data;
        char* textureName = texture->name;
        RwFrame* frame;
        if (RwEngineInstance->vecStrcmp(textureName, "spec3"))
        {
            frame = (RwFrame*)LO_CAM->object.parent;
        }
        else
        {
            frame = (RwFrame*)MainLight->object.parent;
        }
        RpMatFXMaterialSetEffects(material, 2);
        RpMatFXMaterialSetupEnvMap(material, texture, frame, 0, EnvMapShininess);
    }
    return material;
}

#pragma auto_inline on
static RpMaterial* MaterialSetEnvMap(RpMaterial* material, void* data)
{
    if (data == NULL)
    {
        return NULL;
    }
    if (material->texture)
    {
        if (data)
        {
            RwFrame* frame;
            if (gFXSurfaceFlags & 0x10)
            {
                if (LO_CAM)
                {
                    frame = (RwFrame*)LO_CAM->object.parent;
                }
                else
                {
                    frame = (RwFrame*)MainLight->object.parent;
                }
            }
            else
            {
                frame = (RwFrame*)MainLight->object.parent;
            }
            RpMatFXMaterialSetEffects(material, 2);
            RpMatFXMaterialSetupEnvMap(material, (RwTexture*)data, frame, 0, 1.0f);
        }
        else
        {
            RpMatFXMaterialSetEffects(material, 0);
        }
    }
    return material;
}

#pragma auto_inline off

static RpMaterial* iModelSetMaterialTextureCallBack(RpMaterial* material, void* data)
{
    sSetMaterialTextureBuffer[sSetMaterialTextureBufferCount++] = material->texture;
    RpMaterialSetTexture(material, (RwTexture*)data);
    return material;
}

static RpMaterial* iModelSetMaterialTextureRestoreCallBack(RpMaterial* material, void*)
{
    RpMaterialSetTexture(material, sSetMaterialTextureBuffer[--sSetMaterialTextureBufferCount]);
    return material;
}

void iModelSetMaterialTexture(RpAtomic* model, void* texture)
{
    RpGeometryForAllMaterials(model->geometry, iModelSetMaterialTextureCallBack, texture);
}

void iModelSetMaterialTextureRestore(RpAtomic* model)
{
    RpGeometryForAllMaterials(model->geometry, iModelSetMaterialTextureRestoreCallBack, NULL);
}

static RpMaterial* iModelMaterialCallBack(RpMaterial* material, void* data)
{
    RwRGBA new_rw_col = material->color;
    new_rw_col.alpha = sMaterialAlpha;
    material->color = new_rw_col;
    return material;
}

void iModelSetMaterialAlpha(RpAtomic* model, signed int alpha)
{
    RpGeometry* geom = model->geometry;
    geom->flags |= 0x40;
    sMaterialAlpha = alpha;
    RpGeometryForAllMaterials(geom, iModelMaterialCallBack, &alpha);
}

static RpMaterial* iModelMatMultiplyCallBack(RpMaterial* material, void* data)
{
    float* mods = (float*)data;
    RwRGBA* rw_col = &sMaterialColor[sMaterialTotal++];
    *rw_col = material->color;
    RwRGBA col = *rw_col;
    float tmp;

    tmp = col.red * mods[0];
    if (tmp < 0.0f)
    {
        tmp = 0.0f;
    }
    else if (tmp > 255.0f)
    {
        tmp = 255.0f;
    }
    col.red = tmp;

    tmp = col.green * mods[1];
    if (tmp < 0.0f)
    {
        tmp = 0.0f;
    }
    else if (tmp > 255.0f)
    {
        tmp = 255.0f;
    }
    col.green = tmp;

    tmp = col.blue * mods[2];
    if (tmp < 0.0f)
    {
        tmp = 0.0f;
    }
    else if (tmp > 255.0f)
    {
        tmp = 255.0f;
    }
    col.blue = tmp;

    material->color = col;
    return material;
}

static RpMaterial* iModelMatMultiplyResetCallBack(RpMaterial* material, void*)
{
    material->color = sMaterialColor[sMaterialTotal++];
    return material;
}

void iModelMaterialMul(RpAtomic* model, float rm, float gm, float bm)
{
    float cols[3];
    RpGeometry* geom;

    sMaterialTotal = 0;
    geom = model->geometry;
    geom->flags |= 0x40;
    cols[0] = rm;
    cols[1] = gm;
    cols[2] = bm;
    RpGeometryForAllMaterials(geom, iModelMatMultiplyCallBack, cols);
}

void iModelResetMaterial(RpAtomic* model)
{
    sMaterialTotal = 0;
    RpGeometryForAllMaterials(model->geometry, iModelMatMultiplyResetCallBack, NULL);
}

void xFXanimUVSetTranslation(_xVec3* trans)
{
    xFXanimUVTrans[0] = trans->x;
    xFXanimUVTrans[1] = trans->y;
}

void xFXanimUVSetScale(_xVec3* scale)
{
    xFXanimUVScale[0] = scale->x;
    xFXanimUVScale[1] = scale->y;
}

void xFXanimUVSetAngle(float angle)
{
    float s = isin(angle);
    float c = icos(angle);
    xFXanimUVRotMat1[0] = s;
    xFXanimUVRotMat0[0] = c;
    xFXanimUVRotMat0[1] = -s;
    xFXanimUVRotMat1[1] = c;
}

void xFXanimUV2PSetTranslation(_xVec3* trans)
{
    xFXanimUV2PTrans[0] = trans->x;
    xFXanimUV2PTrans[1] = trans->y;
}

void xFXanimUV2PSetScale(_xVec3* scale)
{
    xFXanimUV2PScale[0] = scale->x;
    xFXanimUV2PScale[1] = scale->y;
}

void xFXanimUV2PSetAngle(float angle)
{
    float s = isin(angle);
    float c = icos(angle);
    xFXanimUV2PRotMat1[0] = s;
    xFXanimUV2PRotMat0[0] = c;
    xFXanimUV2PRotMat0[1] = -s;
    xFXanimUV2PRotMat1[1] = c;
}

void xFXanimUV2PSetTexture(RwTexture* texture)
{
    xFXanimUV2PTexture = texture;
}

static inline void xFXanimUVApply(RpAtomic* atomic, RwTexCoords* orig)
{
    if (atomic && orig)
    {
        RpGeometry* geom = atomic->geometry;
        if (geom)
        {
            RwTexCoords* tc = geom->texCoords[0];
            if (tc)
            {
                RpGeometryLock(geom, 0x10);
                signed int n = geom->numVertices;
                for (signed int j = 0; j < n; j++)
                {
                    tc->u = xFXanimUVScale[0] *
                                (xFXanimUVRotMat0[0] * orig->u + xFXanimUVRotMat0[1] * orig->v) +
                            xFXanimUVTrans[0];
                    tc->v = xFXanimUVScale[1] *
                                (xFXanimUVRotMat1[0] * orig->u + xFXanimUVRotMat1[1] * orig->v) +
                            xFXanimUVTrans[1];
                    orig++;
                    tc++;
                }
                RpGeometryUnlock(geom);
            }
        }
    }

}

RpAtomic* xFXanimUVAtomicSetup(RpAtomic* atomic)
{
    if (!atomic)
    {
        return NULL;
    }

    unsigned int i;
    for (i = 0; i < sAnimUVCount; i++)
    {
        if (sAnimUVCache[i].atomic == atomic)
        {
            break;
        }
    }

    if (i == sAnimUVCount)
    {
        signed int numVerts = atomic->geometry->numVertices;
        RwTexCoords* src = atomic->geometry->texCoords[0];
        RwTexCoords* copy = (RwTexCoords*)xMemAlloc(gActiveHeap, numVerts * sizeof(RwTexCoords));
        if (copy)
        {
            memcpy(copy, src, numVerts * 8);
            sAnimUVCache[sAnimUVCount].atomic = atomic;
            sAnimUVCache[sAnimUVCount].uv = copy;
            sAnimUVCount++;
        }
    }

    xFXanimUVApply(atomic, sAnimUVCache[i].uv);

    return atomic;
}

// TODO: 99.3% (NonMatching). Left: register allocation in xEntCollide (the
// inlined ceiling/collision helpers), xEntCollideFloor and xEntDebugWrite;
// xEntRender's visibility test branches as `bne; b` in the target;
// xEntDefaultTranslate reloads ent->model. xEntDeadStrippedNames must be
// stripped by the linker once this unit is Matching.
#include "xEnt.h"
#include "xMath.h"
#include "xMath3.h"
#include "xMemMgr.h"
#include "xScene.h"
#include "xShadow.h"
#include "xFFX.h"
#include "iTime.h"
#include "zGlobals.h"

#ifndef NULL
#define NULL 0
#endif

extern "C" {
void* memcpy(void* dst, const void* src, unsigned int n);
void* memset(void* dst, int c, unsigned int n);
char* strcat(char* dst, const char* src);
int sprintf(char* s, const char* format, ...);
double cos(double x);
double fabs(double x);
signed int RwRenderStateSet(signed int state, void* value);
// TODO: unidentified iBound helpers (set an OBB/box bound from a model)
void fn_80007738(void* bound, xModelInstance* model);
void fn_800077C0(void* bound, xModelInstance* model);
}

class _xEnv;
class _xRay3;
class _zGroup;
class RwTexture;
class xSerial
{
public:
    signed int Write_b1(signed int bit);
    signed int Read_b1(signed int* bit);
};

class _xEntCollis
{
public:
    unsigned char chk;       // 0x0
    unsigned char pen;       // 0x1
    unsigned char env_sidx;  // 0x2
    unsigned char env_eidx;  // 0x3
    unsigned char npc_sidx;  // 0x4
    unsigned char npc_eidx;  // 0x5
    unsigned char dyn_sidx;  // 0x6
    unsigned char dyn_eidx;  // 0x7
    unsigned char stat_sidx; // 0x8
    unsigned char stat_eidx; // 0x9
    unsigned char trig_idx;  // 0xA
    unsigned char idx;       // 0xB
    _xCollis colls[18];      // 0xC
    void (*post)(_xEnt*, _xScene*, float, _xEntCollis*);                     // 0x4D4
    unsigned int (*depenq)(_xEnt*, _xEnt*, _xScene*, float, _xCollis*);      // 0x4D8
    unsigned char trigs_cur[8];     // 0x4DC
    unsigned char trigs_prev[8];    // 0x4E4
    unsigned char trigs_entered[8]; // 0x4EC
    unsigned char trigs_exited[8];  // 0x4F4
};

class xModCollRec
{
public:
    _xEnt* owner;         // 0x0
    unsigned int nusing;  // 0x4
    long long ctime;      // 0x8
    long long ltime;      // 0x10
    float frac;           // 0x18
};

// Surface data used by xEntRender (xSurface is not decompiled yet).
class xSurfaceFX
{
public:
    p2BaseAsset* asset;          // 0x0
    unsigned int flags;          // 0x4
    unsigned char pad8[0xC];     // 0x8
    unsigned int texGroupID;     // 0x14
    unsigned int texItem;        // 0x18
    unsigned char pad1C[0x10];   // 0x1C
    unsigned int uv2GroupID;     // 0x2C
    unsigned int uv2Item;        // 0x30
    unsigned char pad34[4];      // 0x34
    unsigned int uvFlags;        // 0x38
    unsigned char pad3C[4];      // 0x3C
    float uvAngle;               // 0x40
    unsigned char pad44[4];      // 0x44
    _xVec3 uvTrans;              // 0x48
    unsigned char pad54[0xC];    // 0x54
    _xVec3 uvScale;              // 0x60
    unsigned char pad6C[0x10];   // 0x6C
    float uv2Angle;              // 0x7C
    unsigned char pad80[4];      // 0x80
    _xVec3 uv2Trans;             // 0x84
    unsigned char pad90[0xC];    // 0x90
    _xVec3 uv2Scale;             // 0x9C
};

class xSurfaceLike
{
public:
    unsigned char pad0[0x14];
    unsigned int state;   // 0x14
    _xEnt* ent;           // 0x18
    unsigned char pad1C[8];
    xSurfaceFX* fx;       // 0x24
};

class p2SurfaceAssetLike
{
public:
    unsigned char pad0[0x28];
    unsigned int texture; // 0x28
};

class p2EntAssetLike : public p2EntAsset
{
};

class p2LinkAssetLike
{
public:
    unsigned short srcEvent;   // 0x0
    unsigned short dstEvent;   // 0x2
    unsigned int dstAssetID;   // 0x4
    unsigned char pad8[0x18];
};

class RpAtomicLike
{
public:
    unsigned char pad0[0x6C];
    void* pipeline; // 0x6C
};

extern _xScene* g_xSceneCur;
extern _tagxPad* gDebugPad;
extern unsigned int gActiveHeap;
extern float ShadowThreshold;
extern const _xVec3 g_O3;
extern const _xVec3 g_NY3;

void xprintf(char* fmt, ...);
char* xSceneID2Name(_xScene* sc, unsigned int id);
void* xSceneResolvID(_xScene* sc, unsigned int id);
void xSceneForAllTriggers(_xScene* sc, _xEnt* (*func)(_xEnt*, _xScene*, void*), void* data);
void xSceneForAllStatics(_xScene* sc, _xEnt* (*func)(_xEnt*, _xScene*, void*), void* data);
void xSceneForAllDynamics(_xScene* sc, _xEnt* (*func)(_xEnt*, _xScene*, void*), void* data);
void xSceneForAllNPCs(_xScene* sc, _xEnt* (*func)(_xEnt*, _xScene*, void*), void* data);
void xRayHitsScene(_xScene* sc, _xRay3* r, _xCollis* coll);
void xDebugModeRegisterCallback(signed int mode, void (*func)());
float iTimeDiffSec(iTime t);
unsigned int iSphereHitsEnv3(const _xSphere* b, const _xEnv* env, _xCollis* colls,
                             unsigned char ncolls, float sth);
unsigned int iSphereHitsModel3(const _xSphere* b, const xModelInstance* m, _xCollis* colls,
                               unsigned char ncolls, float sth);
void iBoxInitBoundVec(_xBox* b, const _xVec3* v);
void iBoxBoundVec(_xBox* o, const _xBox* b, const _xVec3* v);
signed int iModelCull(RpAtomic* model, RwMatrixTag* mat);
RpAtomic* iModelFile_RWMultiAtomic(RpAtomic* model);
xModelInstance* xModelInstanceAlloc(RpAtomic* data, void* object, unsigned short flags,
                                    unsigned char boneIndex, unsigned char* boneRemap);
void xModelInstanceAttach(xModelInstance* inst, xModelInstance* parent);
void xModelUpdate(xModelInstance* model, float dt);
void xModelEval(xModelInstance* model);
void xModelRender(xModelInstance* model);
void xModelSetMaterialAlpha(xModelInstance* model, signed int alpha);
void xModelSetMaterialTexture(xModelInstance* model, void* texture);
void xModelSetMaterialTextureRestore(xModelInstance* model);
void xModelMaterialMul(xModelInstance* model, float r, float g, float b);
void xModelResetMaterial(xModelInstance* model);
void* xSTFindAsset(unsigned int aid, unsigned int* size);
_zGroup* zSceneFindObject(unsigned int id);
unsigned int zGroupGetItem(_zGroup* grp, unsigned int index);
void zLightAddLocal(_xEnt* ent);
void zLightRemoveLocal();
void xFXanimUVSetTranslation(_xVec3* trans);
void xFXanimUV2PSetTranslation(_xVec3* trans);
void xFXanimUVSetScale(_xVec3* scale);
void xFXanimUV2PSetScale(_xVec3* scale);
void xFXanimUVSetAngle(float angle);
void xFXanimUV2PSetAngle(float angle);
void xFXanimUV2PSetTexture(RwTexture* texture);
void xFXanimUVAtomicSetup(RpAtomic* model);
void iShadowCollide(iShadowCache* cache, _xEnt* ent, float radius, float scale);
void iShadowAdd(iShadowCache* cache, unsigned char alpha);
void xMat3x3GetScale(_xMat3x3* m, _xVec3* s);
void xMat3x3Normalize(_xMat3x3* o, _xMat3x3* m);
void xMat3x3LMulVec(_xVec3* o, _xMat3x3* m, _xVec3* v);
void xMat3x3RotC(_xMat3x3* m, float _x, float _y, float _z, float t);
void xMat4x3RMul3x3(_xMat4x3* o, _xMat3x3* a, _xMat4x3* b);
void xMat4x3LMul3x3(_xMat4x3* o, _xMat4x3* a, _xMat3x3* b);
void xMat4x3Mul(_xMat4x3* o, _xMat4x3* a, _xMat4x3* b);
signed int irand();
void xBaseSave(xBase* ent, xSerial* s);
void xBaseLoad(xBase* ent, xSerial* s);

static float nsn_angle = 0.5235988f;
static float sEntityTimePassed;
static _xBox all_ents_box;
static signed int all_ents_box_init;
static _xEnt** dbg_xents;
static unsigned short dbg_num;
static signed short dbg_idx = -1;
static signed int dbg_mode;
static signed int dbg_holding_inc;
static signed int dbg_holding_dec;
static float dbg_tmr;
static float dbg_ltm;
static xModCollRec* mcrecs;
static signed int num_mcrecs;
static long long mctime;
static xModCollRec smcrecs[101];
static signed int mcmode = 2;
static char scoll_name[6][6] = { "floor", "ceil", "front", "left", "rear", "right" };

static void xEntUpdateTimer(_xEnt* ent);
static void xEntRenderNoCull(_xEnt* ent);
static void xEntDebugCB();
static void xModCollCB();
void xEntRender(_xEnt* ent);
_xEnt* xEntCollCheckOneEntNoDepen(_xEnt* ent, _xScene* sc, void* data);
_xEnt* xEntCollCheckOneTrigNoDepen(_xEnt* ent, _xScene* sc, void* data);
void xEntCollideFloor(_xEnt* p, _xScene* sc, float dt);
static void xEntCollideCeiling(_xEnt* p, _xScene* sc, float dt);
void xEntCollideWalls(_xEnt* p, _xScene* sc, float dt);
void xEntBeginCollide(_xEnt* ent, _xScene* sc, float dt);
void xEntBeginCollCheckTrigs(_xEnt* ent);
void xEntEndCollCheckTrigs(_xEnt* ent);
void xEntEndCollide(_xEnt* ent, _xScene* sc, float dt);
void xEntCollCheckNPCs(_xEnt* p, _xScene* sc, _xEnt* (*hitIt)(_xEnt*, _xScene*, void*));
void xEntCollCheckDyns(_xEnt* p, _xScene* sc, _xEnt* (*hitIt)(_xEnt*, _xScene*, void*));
void xEntCollCheckStats(_xEnt* p, _xScene* sc, _xEnt* (*hitIt)(_xEnt*, _xScene*, void*));
void xEntApplyPhysics(_xEnt* ent, _xScene* sc, float dt);
void xEntMove(_xEnt* ent, _xScene* sc, float dt);
void xEntCollide(_xEnt* ent, _xScene* sc, float dt);
void xEntBeginUpdate(_xEnt* ent, _xScene* sc, float dt);
void xEntEndUpdate(_xEnt* ent, _xScene* sc, float dt);
void xEntDefaultBoundUpdate(_xEnt* ent, _xVec3* pos);
void xEntDefaultTranslate(_xEnt* ent, _xVec3* dpos);
void xEntMotionToMatrix(_xEnt* ent, _xEntFrame* frame);

static inline float xVec3Length(const _xVec3* v)
{
    return xsqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

// Stand-in for a function the linker dead-stripped; only its strings remain at
// the end of xEnt's .rodata.
void xEntDeadStrippedNames()
{
    xprintf("anim-file");
    xprintf("anim-table");
    xprintf("ent-table");
}

static void xEntUpdateTimer(_xEnt* ent)
{
    ent->updateTimer = 0.0f;
    ent->updateAcc = 0.0f;
}

void xEntSetTimePassed(float sec)
{
    sEntityTimePassed = sec;
}

static inline void xModCollInit()
{
    mcrecs = smcrecs;
    num_mcrecs = 0;
    mctime = 0;
    mcmode = 2;
    for (signed int i = 0; i < 101; i++)
    {
        smcrecs[i].owner = NULL;
    }
}

static inline void xEntSceneInitCommon()
{
    xDebugModeRegisterCallback(3, xEntDebugCB);
    dbg_num = 0;
    dbg_xents = (_xEnt**)xMemAlloc(gActiveHeap, 0x4FC);
    dbg_idx = 0;
    dbg_mode = 1;
    dbg_holding_inc = 0;
    dbg_holding_dec = 0;
    xDebugModeRegisterCallback(1, xModCollCB);
    mcrecs = (xModCollRec*)xMemAlloc(gActiveHeap, sizeof(smcrecs));
    num_mcrecs = 0;
    mctime = 0;
    mcmode = 2;
    for (signed int i = 0; i < 101; i++)
    {
        smcrecs[i].owner = NULL;
    }
}

void xEntSceneInit()
{
    xEntSceneInitCommon();
    all_ents_box_init = 1;
}

void xEntSceneExit()
{
    xEntSceneInitCommon();
}

static void xEntRenderNoCull(_xEnt* ent)
{
    RwRenderStateSet(0x14, (void*)2);
    xEntRender(ent);
    RwRenderStateSet(0x14, (void*)1);
}

void xEntInit(_xEnt* ent, p2EntAsset* asset)
{
    xBaseInit(ent, asset);
    ent->asset = asset;
    ent->update = xEntUpdate;
    ent->bupdate = xEntDefaultBoundUpdate;
    ent->render = xEntRender;
    ent->updateTimerFunc = xEntUpdateTimer;
    ent->updateAcc = 0.0f;
    ent->move = NULL;
    ent->transl = xEntDefaultTranslate;
    ent->flags = asset->flags;
    ent->moreFlags = asset->moreFlags;
    ent->subType = asset->subtype;
    ent->pflags = asset->pflags;
    ent->ffx = NULL;
    ent->num_ffx = 0;
    ent->driver = NULL;
    ent->model = NULL;
    ent->frame = NULL;
    ent->collis = NULL;
    ent->surf = NULL;
    ent->material_alpha = 255.0f;
    ent->fade_tmr = 0.0f;
    ent->ent_shadow = NULL;
}

static inline void xModCollAdd(_xEnt* xent)
{
    if (xent != NULL && xent->model != NULL && xent->model->Data != NULL)
    {
        xModCollRec* mc = mcrecs;
        xModCollRec* end = &mcrecs[num_mcrecs];
        for (; mc < end; mc++)
        {
            if (mc->owner->model->Data == xent->model->Data)
            {
                mc->nusing++;
                return;
            }
        }
        num_mcrecs++;
        mc->owner = xent;
        mc->ctime = 0;
        mc->ltime = 0;
        mc->frac = 0.0f;
        mc->nusing = 1;
    }
}

void xEntInitForType(_xEnt* ent)
{
    ent->update = xEntUpdate;
    ent->render = xEntRender;
    ent->updateTimerFunc = xEntUpdateTimer;
    ent->updateAcc = 0.0f;
    if (ent->id == 0x67751F69)
    {
        ent->render = xEntRenderNoCull;
    }

    if (ent->collType == 1)
    {
        ent->pflags &= 0xFC;
        ent->chkby = 0x18;
        ent->penby = 0;
    }
    else if (ent->collType == 2)
    {
        ent->pflags &= 0xFC;
        ent->chkby = 0x18;
        ent->penby = 0x18;
    }
    else if (ent->collType == 4)
    {
        ent->pflags |= 1;
        ent->move = NULL;
        ent->frame = (_xEntFrame*)xMemAlloc(gActiveHeap, sizeof(_xEntFrame));
        memset(ent->frame, 0, sizeof(_xEntFrame));
        ent->pflags &= 0xFD;
        ent->chkby = 0x18;
        ent->penby = 0x18;
    }
    else if (ent->collType == 8)
    {
        ent->pflags |= 1;
        ent->move = NULL;
        ent->frame = (_xEntFrame*)xMemAlloc(gActiveHeap, sizeof(_xEntFrame));
        memset(ent->frame, 0, sizeof(_xEntFrame));
        ent->pflags |= 2;
        ent->chkby = 0x10;
        ent->penby = 0x10;
        ent->collis = (_xEntCollis*)xMemAlloc(gActiveHeap, sizeof(_xEntCollis));
        ent->collis->chk = 0x26;
        ent->collis->pen = 0x26;
        ent->collis->post = NULL;
        ent->collis->depenq = NULL;
    }
    else if (ent->collType == 16)
    {
        ent->pflags |= 1;
        ent->move = NULL;
        ent->frame = (_xEntFrame*)xMemAlloc(gActiveHeap, sizeof(_xEntFrame));
        memset(ent->frame, 0, sizeof(_xEntFrame));
        ent->pflags |= 2;
        ent->chkby = 0;
        ent->penby = 0;
        ent->collis = (_xEntCollis*)xMemAlloc(gActiveHeap, sizeof(_xEntCollis));
        ent->collis->chk = 0x2F;
        ent->collis->pen = 0x2E;
        ent->collis->post = NULL;
        ent->collis->depenq = NULL;
    }

    if (ent->bound.type != 0 && dbg_num < 319)
    {
        dbg_xents[dbg_num++] = ent;
        if (num_mcrecs < 101)
        {
            xModCollAdd(ent);
        }
    }
}

void xEntSetup(_xEnt* ent)
{
    xBaseSetup();

    if (ent->asset->surfaceID != 0)
    {
        ent->surf = (xSurface*)xSceneResolvID(g_xSceneCur, ent->asset->surfaceID);
        if (ent->surf != NULL)
        {
            ((xSurfaceLike*)ent->surf)->state = 1;
            ((xSurfaceLike*)ent->surf)->ent = ent;
        }
    }
    else
    {
        ent->surf = NULL;
    }

    for (signed int i = 0; i < ent->linkCount; i++)
    {
        p2LinkAssetLike* link = &((p2LinkAssetLike*)ent->link)[i];
        if (link->dstEvent == 0xD8)
        {
            xBase* b = (xBase*)xSceneResolvID(g_xSceneCur, link->dstAssetID);
            if (b != NULL)
            {
                ent->driver = (_xEnt*)b;
            }
        }
    }
}

void xEntSave(_xEnt* ent, xSerial* s)
{
    xBaseSave(ent, s);
    if (xEntIsVisible(ent))
    {
        s->Write_b1(1);
    }
    else
    {
        s->Write_b1(0);
    }
}

void xEntLoad(_xEnt* ent, xSerial* s)
{
    xBaseLoad(ent, s);
    signed int b = 0;
    s->Read_b1(&b);
    if (b)
    {
        ent->flags |= 1;
    }
    ent->flags &= 0xFE;
}

void xEntReset(_xEnt* ent)
{
    _xMat4x3 frame;

    xBaseReset(ent, ent->asset);
    ent->flags = ent->asset->flags;
    ent->moreFlags = ent->asset->moreFlags;

    xMat3x3Euler((_xMat3x3*)&frame, ent->asset->ang.x, ent->asset->ang.y, ent->asset->ang.z);
    xVec3SMulBy(&frame.right, ent->asset->scale.x);
    xVec3SMulBy(&frame.up, ent->asset->scale.y);
    xVec3SMulBy(&frame.at, ent->asset->scale.z);
    xVec3Copy(&frame.pos, &ent->asset->pos);
    frame.flags = 0;

    if (ent->model != NULL)
    {
        memcpy(ent->model->Mat, &frame, sizeof(_xMat4x3));
        if (ent->bound.type == 2)
        {
            fn_800077C0(&ent->bound.box.box, ent->model);
        }
        else if (ent->bound.type == 4)
        {
            fn_80007738(&ent->bound.box.box, ent->model);
        }
    }

    if (ent->frame != NULL)
    {
        memcpy(&ent->frame->mat, &frame, sizeof(_xMat4x3));
        xVec3Copy(&ent->frame->oldpos, &ent->frame->mat.pos);
        xVec3Copy(&ent->frame->vel, &g_O3);
        xVec3Copy(&ent->frame->oldvel, &g_O3);
        xVec3Copy(&ent->frame->rot.axis, &ent->asset->ang);
        ent->frame->rot.angle = 0.0f;
        xVec3Copy(&ent->frame->oldrot.axis, &ent->frame->rot.axis);
        ent->frame->oldrot.angle = ent->frame->rot.angle;
    }

    switch (ent->bound.type)
    {
    case 0:
        break;
    case 1:
        xVec3Copy(&ent->bound.sph.center, &ent->asset->pos);
        break;
    }

    ent->num_updates = irand() & 0x7F;

    if (ent->bound.type != 0)
    {
        if (all_ents_box_init)
        {
            iBoxInitBoundVec(&all_ents_box, &frame.pos);
            all_ents_box_init = 0;
        }
        else
        {
            iBoxBoundVec(&all_ents_box, &all_ents_box, &frame.pos);
        }
    }

    if (ent->collType == 16 || ent->collType == 8)
    {
        for (signed int i = 0; i < 8; i++)
        {
            ent->collis->trigs_cur[i] = 0;
            ent->collis->trigs_prev[i] = 0;
            ent->collis->trigs_entered[i] = 0;
            ent->collis->trigs_exited[i] = 0;
        }
    }
}

xModelInstance* xEntLoadModel(_xEnt* ent, RpAtomic* imodel)
{
    xModelInstance* model = xModelInstanceAlloc(imodel, ent, 0, 0, NULL);
    while ((imodel = iModelFile_RWMultiAtomic(imodel)) != NULL)
    {
        xModelInstanceAttach(xModelInstanceAlloc(imodel, ent, 8, 0, NULL), model);
    }
    if (ent != NULL)
    {
        ent->model = model;
    }
    return model;
}

void xEntUpdate(_xEnt* ent, _xScene* sc, float dt)
{
    xEntBeginUpdate(ent, sc, dt);

    if (ent->pflags & 2)
    {
        xEntApplyPhysics(ent, sc, dt);
    }
    if (ent->pflags & 1)
    {
        xEntMove(ent, sc, dt);
    }
    xFFXApply(ent, sc, dt);
    if (ent->collis != NULL)
    {
        xEntCollide(ent, sc, dt);
    }

    xEntEndUpdate(ent, sc, dt);
}

#pragma auto_inline on
void xEntBeginUpdate(_xEnt* ent, _xScene* sc, float dt)
{
    if (ent->model != NULL)
    {
        xModelUpdate(ent->model, dt);
        if (ent->frame != NULL)
        {
            xVec3Copy(&ent->frame->oldvel, &ent->frame->vel);
            xVec3Copy(&ent->frame->oldpos, &ent->frame->mat.pos);
            xVec3Copy(&ent->frame->oldrot.axis, &ent->frame->rot.axis);
            ent->frame->oldrot.angle = ent->frame->rot.angle;
            memcpy(&ent->frame->mat, ent->model->Mat, sizeof(_xMat4x3));
            ent->frame->mode = 0;
        }
    }
    if (ent->fade_tmr > 0.0f)
    {
        ent->fade_tmr -= dt;
    }
}

void xEntEndUpdate(_xEnt* ent, _xScene* sc, float dt)
{
    ent->num_updates++;
    if (ent->model != NULL)
    {
        if (ent->frame != NULL)
        {
            if (!(ent->frame->mode & 0x20000))
            {
                memcpy(ent->model->Mat, &ent->frame->mat, sizeof(_xMat3x3));
            }
            if (!(ent->frame->mode & 0x10000))
            {
                xVec3Copy((_xVec3*)&((_xMat4x3*)ent->model->Mat)->pos, &ent->frame->mat.pos);
            }
        }
        if (ent->bupdate != NULL)
        {
            ent->bupdate(ent, (_xVec3*)&((_xMat4x3*)ent->model->Mat)->pos);
        }
        xModelEval(ent->model);
    }
}
#pragma auto_inline reset

void xEntDefaultBoundUpdate(_xEnt* ent, _xVec3* pos)
{
    if (ent->bound.type == 1)
    {
        xVec3Copy(&ent->bound.sph.center, pos);
        ent->bound.sph.center.y += 0.7f;
        ent->bound.sph.r = 0.7f;
    }
    xBoundUpdate(&ent->bound);
}

void xEntDefaultTranslate(_xEnt* ent, _xVec3* dpos)
{
    if (ent->model != NULL)
    {
        xVec3AddTo((_xVec3*)&((_xMat4x3*)ent->model->Mat)->pos, dpos);
    }
    if (ent->frame != NULL)
    {
        xVec3AddTo(&ent->frame->mat.pos, dpos);
    }
    xVec3AddTo(xBoundCenter(&ent->bound), dpos);
}

static inline void xEntRotationToMatrix(_xEntFrame* frame)
{
    if (frame->mode & 0x20)
    {
        if (frame->mode & 0x400)
        {
            xVec3AddTo(&frame->rot.axis, &frame->drot.axis);
            xMat3x3Euler((_xMat3x3*)&frame->mat, frame->rot.axis.x, frame->rot.axis.y,
                         frame->rot.axis.z);
        }
        else
        {
            frame->rot.angle = xAngleClamp(frame->rot.angle + frame->drot.angle);
            xMat3x3RotC((_xMat3x3*)&frame->mat, frame->rot.axis.x, frame->rot.axis.y,
                        frame->rot.axis.z, frame->rot.angle);
        }
    }
}

void xEntMotionToMatrix(_xEnt* ent, _xEntFrame* frame)
{
    if (frame->mode & 0x80)
    {
        if (frame->mode & 0x800)
        {
            xMat4x3RMul3x3(&frame->mat, (_xMat3x3*)&frame->dmat, &frame->mat);
        }
        else
        {
            xMat4x3LMul3x3(&frame->mat, &frame->mat, (_xMat3x3*)&frame->dmat);
        }
    }
    else if (frame->mode & 0x40)
    {
        if (frame->mode & 0x800)
        {
            xMat4x3Mul(&frame->mat, &frame->dmat, &frame->mat);
        }
        else
        {
            xMat4x3Mul(&frame->mat, &frame->mat, &frame->dmat);
        }
    }

    if (frame->mode & 0x1000)
    {
        xEntRotationToMatrix(frame);
    }

    if (frame->mode & 0x2)
    {
        if (frame->mode & 0x800)
        {
            xMat3x3RMulVec(&frame->dpos, (_xMat3x3*)&frame->mat, &frame->dpos);
        }
        xVec3AddTo(&ent->frame->mat.pos, &frame->dpos);
    }

    if (frame->mode & 0x8)
    {
        if (frame->mode & 0x800)
        {
            xMat3x3RMulVec(&frame->dvel, (_xMat3x3*)&frame->mat, &frame->dvel);
        }
        xVec3AddTo(&frame->vel, &frame->dvel);
    }

    if (!(frame->mode & 0x1000))
    {
        xEntRotationToMatrix(frame);
    }
}

void xEntMove(_xEnt* ent, _xScene* sc, float dt)
{
    ent->move(ent, sc, dt, ent->frame);
    xEntMotionToMatrix(ent, ent->frame);

    if (ent->driver != NULL)
    {
        _xEntFrame* dframe = ent->driver->frame;
        _xVec3 dpos;
        xVec3Sub(&dpos, &dframe->mat.pos, &dframe->oldpos);
        ent->transl(ent, &dpos);
    }
}

void xEntApplyPhysics(_xEnt* ent, _xScene* sc, float dt)
{
    if ((ent->pflags & 0x4) && (sc->flags & 0x1))
    {
        ent->frame->vel.y += sc->gravity * dt;
    }
    if ((ent->pflags & 0x10) && (sc->flags & 0x2))
    {
        float tfric = -(sc->friction * dt - 1.0f);
        xVec3SMulBy(&ent->frame->vel, tfric);
    }
    if ((ent->pflags & 0x8) && (sc->flags & 0x4))
    {
        float tdrag = -(sc->drag * dt - 1.0f);
        xVec3SMulBy(&ent->frame->vel, tdrag);
    }

    _xVec3 dposvel;
    float half = 0.5f * dt;
    dposvel.y = (ent->frame->vel.y + ent->frame->oldvel.y) * half;
    dposvel.x = (ent->frame->vel.x + ent->frame->oldvel.x) * half;
    dposvel.z = (ent->frame->vel.z + ent->frame->oldvel.z) * half;

    if (dposvel.y < 0.0f)
    {
        float len = xVec3Length(&dposvel);
        float scale = (0.63f * (30.0f * dt)) / len;
        if (scale < 1.0f)
        {
            dposvel.x *= scale;
            dposvel.y *= scale;
            dposvel.z *= scale;
        }
    }

    xVec3AddTo(&ent->frame->mat.pos, &dposvel);
}

static inline unsigned int xEntCollDepenOk(_xEnt* ent, _xScene* sc, _xCollis* coll, float dt)
{
    _xEnt* fent = (_xEnt*)coll->optr;
    return ent->collis->depenq != NULL
               ? ent->collis->depenq(ent, fent, sc, dt, coll)
               : (unsigned char)((fent->collType & ent->collis->pen) &&
                                 (fent->penby & ent->collType));
}


void xEntCollide(_xEnt* ent, _xScene* sc, float dt)
{
    if (ent->model == NULL)
    {
        return;
    }

    if (ent->collis->chk & 0x2E)
    {
        xEntBeginCollide(ent, sc, dt);
    }
    if (ent->collis->chk & 0x8)
    {
        xEntCollCheckNPCs(ent, sc, xEntCollCheckOneEntNoDepen);
    }
    if (ent->collis->chk & 0x4)
    {
        xEntCollCheckDyns(ent, sc, xEntCollCheckOneEntNoDepen);
    }
    if (ent->collis->chk & 0x2)
    {
        xEntCollCheckStats(ent, sc, xEntCollCheckOneEntNoDepen);
    }
    if (ent->collis->chk & 0x20)
    {
        ent->collis->env_sidx = ent->collis->idx;
        _xCollis* coll = &ent->collis->colls[ent->collis->idx];
        coll->flags = 0x1F00;
        ent->collis->idx += (unsigned char)iSphereHitsEnv3(&ent->bound.sph, sc->env, coll,
                                                           18 - ent->collis->idx, 0.78539819f);
        ent->collis->env_eidx = ent->collis->idx;
    }

    if (ent->collis->chk & 0x2E)
    {
        float sph_r = (ent->bound.type == 1) ? ent->bound.sph.r : 0.7f;
        _xCollis* coll = &ent->collis->colls[0];

        if ((ent->pflags & 0x80) && (coll->flags & 1))
        {
            float depen_len = xVec3Dot(&coll->hdng, &coll->norm);
            if (depen_len > 0.0f)
            {
                depen_len = -depen_len;
                xVec3SMulBy(&coll->norm, -1.0f);
            }
            depen_len = depen_len * coll->dist + sph_r;
            if (depen_len < 0.0f || depen_len > sph_r)
            {
                depen_len = (0.0f > (depen_len < sph_r ? depen_len : sph_r))
                                ? 0.0f
                                : (depen_len < sph_r ? depen_len : sph_r);
            }
            xVec3SMul(&coll->depen, &coll->norm, depen_len);
        }

        if (ent->frame->vel.y <= 0.0f)
        {
            xEntCollideFloor(ent, sc, dt);
        }
        else
        {
            xEntCollideCeiling(ent, sc, dt);
        }

        if (!(ent->collis->colls[0].flags & 1) && ent->collType == 16)
        {
            struct
            {
                _xVec3 origin;
                _xVec3 dir;
                float min_t;
                float max_t;
                signed int flags;
            } ray;
            xVec3Copy(&ray.origin, xBoundCenter(&ent->bound));
            xVec3Copy(&ray.dir, &g_NY3);
            ray.max_t = 10.0f;
            ray.flags = 0x800;
            xRayHitsScene(sc, (_xRay3*)&ray, &ent->collis->colls[0]);
            ent->collis->colls[0].flags &= ~1;
        }

        xEntCollideWalls(ent, sc, dt);
        xEntEndCollide(ent, sc, dt);
    }

    if (ent->collis->chk & 1)
    {
        xEntBeginCollCheckTrigs(ent);
        xSceneForAllTriggers(sc, xEntCollCheckOneTrigNoDepen, ent);
        xEntEndCollCheckTrigs(ent);
    }
}

#pragma auto_inline on
void xEntBeginCollide(_xEnt* ent, _xScene* sc, float dt)
{
    if (ent->bupdate != NULL)
    {
        ent->bupdate(ent, &ent->frame->mat.pos);
    }

    for (unsigned char idx = 0; idx < 18; idx++)
    {
        _xCollis* coll = &ent->collis->colls[idx];
        coll->flags = 0x1F00;
        coll->optr = NULL;
        coll->dist = 1e19f;
    }

    ent->collis->idx = 6;
    ent->collis->stat_sidx = 6;
    ent->collis->stat_eidx = 6;
    ent->collis->dyn_sidx = 6;
    ent->collis->dyn_eidx = 6;
    ent->collis->npc_sidx = 6;
    ent->collis->npc_eidx = 6;
    ent->collis->env_sidx = 6;
    ent->collis->env_eidx = 6;
}

void xEntEndCollide(_xEnt* ent, _xScene* sc, float dt)
{
    if (ent->collis->post != NULL)
    {
        ent->collis->post(ent, sc, dt, ent->collis);
    }
}

void xEntCollCheckStats(_xEnt* p, _xScene* sc, _xEnt* (*hitIt)(_xEnt*, _xScene*, void*))
{
    p->collis->stat_sidx = p->collis->idx;
    xSceneForAllStatics(sc, hitIt, p);
    p->collis->stat_eidx = p->collis->idx;
}

void xEntCollCheckDyns(_xEnt* p, _xScene* sc, _xEnt* (*hitIt)(_xEnt*, _xScene*, void*))
{
    p->collis->dyn_sidx = p->collis->idx;
    xSceneForAllDynamics(sc, hitIt, p);
    p->collis->dyn_eidx = p->collis->idx;
}

void xEntCollCheckNPCs(_xEnt* p, _xScene* sc, _xEnt* (*hitIt)(_xEnt*, _xScene*, void*))
{
    p->collis->npc_sidx = p->collis->idx;
    xSceneForAllNPCs(sc, hitIt, p);
    p->collis->npc_eidx = p->collis->idx;
}

void xEntBeginCollCheckTrigs(_xEnt* ent)
{
    for (unsigned short i = 0; i < 8; i++)
    {
        ent->collis->trigs_prev[i] = ent->collis->trigs_cur[i];
    }
    ent->collis->trig_idx = 0;
}

void xEntEndCollCheckTrigs(_xEnt* ent)
{
    for (unsigned short i = 0; i < 8; i++)
    {
        ent->collis->trigs_entered[i] = ent->collis->trigs_cur[i] & ~ent->collis->trigs_prev[i];
        ent->collis->trigs_exited[i] = ent->collis->trigs_prev[i] & ~ent->collis->trigs_cur[i];
    }
}
#pragma auto_inline reset

static inline void xModCollAddSample(_xEnt* xent, long long elapsed)
{
    if (xent != NULL && xent->model != NULL && xent->model->Data != NULL)
    {
        xModCollRec* mc = mcrecs;
        xModCollRec* end = &mcrecs[num_mcrecs];
        for (; mc < end; mc++)
        {
            if (mc->owner->model->Data == xent->model->Data)
            {
                mc->ctime += elapsed;
                mc->ltime += elapsed;
            }
        }
        mctime += elapsed;
    }
}

_xEnt* xEntCollCheckOneEntNoDepen(_xEnt* ent, _xScene* sc, void* data)
{
    _xEnt* p = (_xEnt*)data;
    _xCollis* coll;
    unsigned int modl_coll = 0;

    if (p->collis->idx >= 15)
    {
        return NULL;
    }
    if (!(ent->chkby & p->collType))
    {
        return ent;
    }
    if (ent->id == p->id)
    {
        return ent;
    }

    coll = &p->collis->colls[p->collis->idx];
    if (ent->collLev == 5 && (p->collType & 0x18))
    {
        modl_coll = 1;
    }
    if (modl_coll)
    {
        coll->flags = 0;
    }
    else
    {
        coll->flags = 0x1F00;
    }

    xBoundHitsBound(&p->bound, &ent->bound, coll);

    if (coll->flags & 1)
    {
        if (modl_coll)
        {
            coll->flags = 0x1F00;
            iTime t0 = iTimeGet();
            unsigned char ncolls =
                iSphereHitsModel3(&p->bound.sph, ent->model, coll, 15 - p->collis->idx, 0.78539819f);
            iTime t1 = iTimeGet();
            xModCollAddSample(ent, t1 - t0);
            for (unsigned char i = 0; i < ncolls; i++)
            {
                coll[i].optr = ent;
                p->collis->idx++;
            }
            return ent;
        }

        coll->oid = ent->id;
        coll->optr = ent;
        p->collis->idx++;

        if ((ent->pflags & 0x20) && ent->bound.type == 1 && p->bound.type == 1 &&
            coll->hdng.y < -0.866025f)
        {
            float dz = p->bound.sph.center.z - ent->bound.sph.center.z;
            float dx = p->bound.sph.center.x - ent->bound.sph.center.x;
            float rsum = p->bound.sph.r + ent->bound.sph.r;
            float dy = p->bound.sph.center.y - ent->bound.sph.center.y;
            float hsqr = rsum * rsum - (dz * dz + dx * dx);
            if (hsqr >= 0.0f)
            {
                coll->depen.x = 0.0f;
                coll->depen.y = xsqrt(hsqr) - dy;
                coll->depen.z = 0.0f;
                coll->dist = 0.7f - coll->depen.y;
                coll->hdng.x = 0.0f;
                coll->hdng.y = -1.0f;
                coll->hdng.z = 0.0f;
            }
        }
    }

    return ent;
}

_xEnt* xEntCollCheckOneTrigNoDepen(_xEnt* ent, _xScene* sc, void* data)
{
    _xEnt* p = (_xEnt*)data;
    unsigned char idx = p->collis->trig_idx++;
    p->collis->trigs_cur[idx >> 3] &= (unsigned char)~(1 << (idx % 8));
    return ent;
}

void xEntCollideFloor(_xEnt* p, _xScene* sc, float dt)
{
    _xCollis* coll = &p->collis->colls[0];
    unsigned char idx;
    _xCollis* ml = coll;
    _xVec3 motion;
    float mlen;
    signed int stepping = 0;
    float sbr = (p->bound.type == 1) ? p->bound.sph.r : 0.7f;

    xVec3Sub(&motion, &p->frame->mat.pos, &p->frame->oldpos);
    motion.y = 0.0f;
    mlen = xVec3Length(&motion);

    for (idx = 6; idx < p->collis->idx; idx++)
    {
        _xCollis* mf = &p->collis->colls[idx];
        if (!(mf->flags & 1))
        {
            continue;
        }

        _xEnt* fent = (_xEnt*)mf->optr;
        if (fent != NULL)
        {
            if (fent->collType == 4 || fent->collType == 2 ||
                ((fent->pflags & 0x20) && 0.0f == mf->hdng.x && 0.0f == mf->hdng.z))
            {
                if (!xEntCollDepenOk(p, sc, mf, dt))
                {
                    continue;
                }
            }
            else
            {
                continue;
            }
        }
        else if (!(p->collis->pen & 0x20))
        {
            continue;
        }

        if (mf->dist < ml->dist)
        {
            if (mf->hdng.y < -(float)cos(1.0471976f) &&
                (mf->norm.y > (float)cos(nsn_angle) ||
                 p->frame->oldpos.y > dt * (sc->gravity * dt) + p->frame->mat.pos.y))
            {
                ml = mf;
                stepping = 0;
            }
            else if (mlen > 0.001f && mf->hdng.y < 0.65f / sbr - 1.0f &&
                     mf->norm.y > (float)cos(nsn_angle))
            {
                stepping = 1;
                ml = mf;
            }
        }
    }

    if (ml != coll)
    {
        float flr_dist = ml->dist * (float)__fabs(ml->hdng.y);
        *coll = *ml;
        if (flr_dist < sbr)
        {
            ml->flags |= 6;
            if (stepping)
            {
                p->frame->mat.pos.y += 1.5f * dt;
                p->frame->mat.pos.x += ml->depen.x;
                p->frame->mat.pos.z += ml->depen.z;
            }
            else
            {
                p->frame->mat.pos.y += ml->depen.y;
            }
            p->frame->vel.y = 0.0f;
        }
    }
}

#pragma auto_inline on
static void xEntCollideCeiling(_xEnt* p, _xScene* sc, float dt)
{
    _xEntCollis* collis = p->collis;
    _xCollis* cl = &collis->colls[1];
    _xCollis* hit = cl;
    float sph_r = (p->bound.type == 1) ? p->bound.sph.r : 0.7f;

    for (unsigned char idx = 6; idx < p->collis->idx; idx++)
    {
        _xCollis* coll = &p->collis->colls[idx];
        unsigned int ok;
        if (coll->optr != NULL)
        {
            ok = xEntCollDepenOk(p, sc, coll, dt);
        }
        else
        {
            ok = p->collis->pen & 0x20;
        }
        if (ok && coll->hdng.y > (float)cos(0.78539819f) && coll->dist < hit->dist)
        {
            hit = coll;
        }
    }

    if (hit != cl)
    {
        float depen = hit->dist * (float)__fabs(hit->hdng.y);
        *cl = *hit;
        hit->flags |= 0xA;
        if (depen < sph_r)
        {
            p->frame->mat.pos.y -= sph_r - depen;
            p->frame->vel.y = 0.0f;
        }
    }
}
#pragma auto_inline reset

void xEntCollideWalls(_xEnt* p, _xScene* sc, float dt)
{
    float sbr = (p->bound.type == 1) ? p->bound.sph.r : 0.7f;
    unsigned char idx;
    unsigned char sidx;

    if (p->collis->pen & 0x8)
    {
        idx = p->collis->npc_eidx;
        for (sidx = p->collis->npc_sidx; sidx < idx; sidx++)
        {
            _xCollis* coll = &p->collis->colls[sidx];
            _xEnt* cent = (_xEnt*)coll->optr;
            if (!(coll->flags & 2) && coll->dist < sbr)
            {
                unsigned int ok;
                if (p->collis->depenq != NULL)
                {
                    ok = p->collis->depenq(p, cent, sc, dt, coll);
                }
                else
                {
                    ok = cent->penby & p->collType;
                }
                if (ok)
                {
                    if (0.0f != coll->depen.x || 0.0f != coll->depen.z)
                    {
                        coll->depen.y = 0.0f;
                    }
                    xVec3AddTo(&p->frame->mat.pos, &coll->depen);
                }
            }
        }
    }

    if (p->collis->pen & 0x4)
    {
        idx = p->collis->dyn_eidx;
        for (sidx = p->collis->dyn_sidx; sidx < idx; sidx++)
        {
            _xCollis* coll = &p->collis->colls[sidx];
            _xEnt* cent = (_xEnt*)coll->optr;
            if (!(coll->flags & 2) && coll->dist < sbr)
            {
                unsigned int ok;
                if (p->collis->depenq != NULL)
                {
                    ok = p->collis->depenq(p, cent, sc, dt, coll);
                }
                else
                {
                    ok = cent->penby & p->collType;
                }
                if (ok)
                {
                    coll->depen.y = 0.0f;
                    xVec3AddTo(&p->frame->mat.pos, &coll->depen);
                }
            }
        }
    }

    if (p->collis->pen & 0x2)
    {
        idx = p->collis->stat_eidx;
        for (sidx = p->collis->stat_sidx; sidx < idx; sidx++)
        {
            _xCollis* coll = &p->collis->colls[sidx];
            _xEnt* cent = (_xEnt*)coll->optr;
            if (!(coll->flags & 2) && coll->dist < sbr)
            {
                unsigned int ok;
                if (p->collis->depenq != NULL)
                {
                    ok = p->collis->depenq(p, cent, sc, dt, coll);
                }
                else
                {
                    ok = cent->penby & p->collType;
                }
                if (ok)
                {
                    coll->depen.y = 0.0f;
                    xVec3AddTo(&p->frame->mat.pos, &coll->depen);
                }
            }
        }
    }

    if (p->collis->pen & 0x20)
    {
        idx = p->collis->env_eidx;
        for (sidx = p->collis->env_sidx; sidx < idx; sidx++)
        {
            _xCollis* coll = &p->collis->colls[sidx];
            if (!(coll->flags & 2) && coll->dist < sbr)
            {
                coll->depen.y = 0.0f;
                xVec3AddTo(&p->frame->mat.pos, &coll->depen);
            }
        }
    }
}

static inline signed int xEntFadeMode()
{
    return -1;
}

void xEntRender(_xEnt* ent)
{
    if (ent->model == NULL)
    {
        return;
    }
    if (!xEntIsVisible(ent))
    {
        return;
    }
    if (iModelCull(ent->model->Data, ent->model->Mat))
    {
        return;
    }

    signed int alphaSet = 0;
    if (ent->flags & 4)
    {
        if (ent->fade_tmr <= 0.0f)
        {
            ent->flags ^= 4;
            ent->fade_tmr = 0.0f;
        }

        static signed int sFadeMode = xEntFadeMode();
        if (sFadeMode == 0)
        {
            ent->material_alpha -= ent->asset->seeThruSpeed * sEntityTimePassed;
            if (ent->asset->seeThru < 0.3f)
            {
                if (ent->material_alpha <= 0.0f)
                {
                    ent->material_alpha = 0.0f;
                }
            }
            else
            {
                float min = 255.0f * ent->asset->seeThru;
                if (ent->material_alpha <= min)
                {
                    ent->material_alpha = min;
                }
            }
        }
        else if (sFadeMode == 1)
        {
            ent->material_alpha -= 2000.0f * sEntityTimePassed;
            if (ent->material_alpha <= 0.0f)
            {
                ent->material_alpha = 0.0f;
            }
        }
        else if (sFadeMode == 2)
        {
            ent->material_alpha -= 150.0f * sEntityTimePassed;
            if (ent->material_alpha <= 0.0f)
            {
                ent->material_alpha = 0.0f;
            }
        }
        else
        {
            ent->material_alpha -= ent->asset->seeThruSpeed * sEntityTimePassed;
            float min = 255.0f * ent->asset->seeThru;
            if (ent->material_alpha <= min)
            {
                ent->material_alpha = min;
            }
        }

        if (globals.cmgr == NULL)
        {
            xModelSetMaterialAlpha(ent->model, (unsigned char)ent->material_alpha);
            alphaSet = 1;
        }
    }
    else if (ent->material_alpha < 255.0f)
    {
        ent->material_alpha += ent->asset->seeThruSpeed * sEntityTimePassed;
        if (ent->material_alpha > 255.0f)
        {
            ent->material_alpha = 255.0f;
        }
        if (globals.cmgr == NULL)
        {
            xModelSetMaterialAlpha(ent->model, (unsigned char)ent->material_alpha);
            alphaSet = 1;
        }
    }

    signed int texSet = 0;
    signed int uvSet = 0;
    void* pipeline;
    if (ent->surf != NULL)
    {
        xSurfaceFX* fx = ((xSurfaceLike*)ent->surf)->fx;
        if (fx != NULL)
        {
            if (fx->flags & 1)
            {
                _zGroup* grp = zSceneFindObject(fx->texGroupID);
                if (grp != NULL)
                {
                    void* tex = xSTFindAsset(zGroupGetItem(grp, fx->texItem), NULL);
                    if (tex != NULL)
                    {
                        xModelSetMaterialTexture(ent->model, tex);
                        texSet = 1;
                    }
                }
            }

            if (fx->flags & 2)
            {
                _zGroup* grp = zSceneFindObject(fx->uv2GroupID);
                if (grp != NULL)
                {
                    RwTexture* tex = (RwTexture*)xSTFindAsset(zGroupGetItem(grp, fx->uv2Item), NULL);
                    if (tex != NULL)
                    {
                        xFXanimUV2PSetTexture(tex);
                        uvSet = 1;
                    }
                }
            }
            else if (fx->uvFlags & 2)
            {
                RwTexture* tex = (RwTexture*)xSTFindAsset(
                    ((p2SurfaceAssetLike*)fx->asset)->texture, NULL);
                if (tex != NULL)
                {
                    xFXanimUV2PSetTexture(tex);
                    uvSet = 1;
                }
            }
            else
            {
                xFXanimUV2PSetTexture(NULL);
            }

            if (fx->uvFlags & 3)
            {
                uvSet = 1;
                xFXanimUVSetTranslation(&fx->uvTrans);
                xFXanimUV2PSetTranslation(&fx->uv2Trans);
                xFXanimUVSetScale(&fx->uvScale);
                xFXanimUV2PSetScale(&fx->uv2Scale);
                xFXanimUVSetAngle(0.0017683882f * fx->uvAngle);
                xFXanimUV2PSetAngle(0.0017683882f * fx->uv2Angle);
            }

            if (uvSet)
            {
                RpAtomic* atomic = ent->model->Data;
                pipeline = ((RpAtomicLike*)atomic)->pipeline;
                xFXanimUVAtomicSetup(atomic);
            }
        }
    }

    signed int matMul = 0;
    p2EntAsset* asset = ent->asset;
    if (1.0f != asset->redMult || 1.0f != asset->greenMult || 1.0f != asset->blueMult)
    {
        matMul = 1;
        xModelMaterialMul(ent->model, asset->redMult, asset->greenMult, asset->blueMult);
    }

    zLightAddLocal(ent);
    xModelRender(ent->model);
    zLightRemoveLocal();

    if (matMul)
    {
        xModelResetMaterial(ent->model);
    }
    if (alphaSet)
    {
        xModelSetMaterialAlpha(ent->model, 0xFF);
    }
    if (texSet)
    {
        xModelSetMaterialTextureRestore(ent->model);
    }
    if (uvSet)
    {
        ((RpAtomicLike*)ent->model->Data)->pipeline = pipeline;
    }

    unsigned char shadowMode = ent->baseType;
    if (shadowMode == 2 || shadowMode == 3)
    {
        _xVec3* pos = (_xVec3*)&((_xMat4x3*)ent->model->Mat)->pos;
        float dy = pos->y - globals.camera.hi_cam.mat.pos.y;
        float dx = pos->x - globals.camera.hi_cam.mat.pos.x;
        float dz = pos->z - globals.camera.hi_cam.mat.pos.z;
        if (dx * dx + dy * dy + dz * dz < ShadowThreshold || shadowMode == 3)
        {
            xShadowRender(ent);
        }

        float radius = ent->bound.sph.r;
        if (shadowMode == 2)
        {
            switch (ent->subType)
            {
            case 6:
            case 0x2D:
                radius *= 1.75f;
                break;
            }
        }
        iShadowCollide(&ent->ishadow, ent, radius, 1.0f);
        iShadowAdd(&ent->ishadow, 0x50);
    }
}

void xEntSetMaterialMul(_xEnt* ent, float r, float g, float b)
{
    ent->asset->redMult = r;
    ent->asset->greenMult = g;
    ent->asset->blueMult = b;
}

void xEntSetNostepNormAngle(float angle)
{
    nsn_angle = angle;
}

_xBox* xEntGetAllEntsBox()
{
    return &all_ents_box;
}

static void xEntDebugIPad(_xEnt* xent);
static void xEntDebugWrite(_xEnt* xent);

static void xEntDebugCB()
{
    if (dbg_idx != -1 && dbg_num != 0)
    {
        _xEnt* xent = dbg_xents[dbg_idx];
        xEntDebugWrite(xent);
        xEntDebugIPad(xent);
    }
}

static inline char* xEntCollTypeStrAll(char* str, unsigned char type)
{
    str[0] = '\0';
    if (type & 0x1)
    {
        strcat(str, "trig ");
    }
    if (type & 0x2)
    {
        strcat(str, "stat ");
    }
    if (type & 0x4)
    {
        strcat(str, "dyn ");
    }
    if (type & 0x8)
    {
        strcat(str, "npc ");
    }
    if (type & 0x10)
    {
        strcat(str, "pc ");
    }
    if (type & 0x20)
    {
        strcat(str, "env ");
    }
    if (type & 0x40)
    {
        strcat(str, "walls ");
    }
    return str;
}

static void xEntDebugWrite(_xEnt* xent)
{
    char str[128];

    xprintf("name:         %s", xSceneID2Name(g_xSceneCur, xent->id));
    xprintf("   id: 0x%08x\n", xent->id);

    if (dbg_mode == 1)
    {
        xprintf("is enabled:   %s\n", (xent->baseFlags & 1) ? "true " : "false");
        xprintf("is visible:   %s\n", (xent->flags & 1) ? "true " : "false");
        xprintf("is moving:    %s\n", (xent->pflags & 1) ? "true " : "false");
        xprintf("has velocity: %s\n", (xent->pflags & 2) ? "true " : "false");
        xprintf("has gravity:  %s\n", (xent->pflags & 4) ? "true " : "false");
        xprintf("has drag:     %s\n", (xent->pflags & 8) ? "true " : "false");

        _xMat3x3* mat;
        if (xent->model != NULL)
        {
            mat = (_xMat3x3*)xent->model->Mat;
        }
        else if (xent->frame != NULL)
        {
            mat = (_xMat3x3*)&xent->frame->mat;
        }
        else
        {
            mat = NULL;
        }

        _xVec3* pos;
        if (xent->model != NULL)
        {
            pos = (_xVec3*)&((_xMat4x3*)xent->model->Mat)->pos;
        }
        else if (xent->frame != NULL)
        {
            pos = &xent->frame->mat.pos;
        }
        else
        {
            pos = NULL;
        }

        if (pos != NULL)
        {
            xprintf("pos:   <%8.3f %8.3f %8.3f>\n", pos->x, pos->y, pos->z);
            if (xent->pflags & 1)
            {
                _xEntFrame* frame = xent->frame;
                xprintf("dpos:  <%8.3f %8.3f %8.3f>\n", pos->x - frame->oldpos.x,
                        pos->y - frame->oldpos.y, pos->z - frame->oldpos.z);
            }
        }

        if (xent->pflags & 2)
        {
            _xEntFrame* frame = xent->frame;
            xprintf("vel:   <%8.3f %8.3f %8.3f>\n", frame->vel.x, frame->vel.y, frame->vel.z);
            xprintf("dvel:  <%8.3f %8.3f %8.3f>\n", frame->vel.x - frame->oldvel.x,
                    frame->vel.y - frame->oldvel.y, frame->vel.z - frame->oldvel.z);
        }

        if (mat != NULL)
        {
            _xVec3 scale;
            _xMat3x3 nmat;
            _xVec3 ang;
            xMat3x3GetScale(mat, &scale);
            xprintf("scale: <%8.3f %8.3f %8.3f>\n", scale.x, scale.y, scale.z);
            xMat3x3Normalize(&nmat, mat);
            xMat3x3GetEuler(&nmat, &ang);
            xprintf("ang:   <%6.1f   %6.1f   %6.1f  >   Y P R\n",
                    180.0f * xDangleClamp(ang.x) / 3.1415927f,
                    180.0f * xDangleClamp(ang.y) / 3.1415927f,
                    180.0f * xDangleClamp(ang.z) / 3.1415927f);
        }
    }

    if (dbg_mode == 2 || dbg_mode == 3)
    {
        char* name;
        switch (xent->collType)
        {
        case 0x1:
            name = "trig";
            break;
        case 0x2:
            name = "stat";
            break;
        case 0x4:
            name = "dyn";
            break;
        case 0x8:
            name = "npc";
            break;
        case 0x10:
            name = "pc";
            break;
        case 0x20:
            name = "env";
            break;
        case 0x40:
            name = "walls";
            break;
        default:
            name = "???";
            break;
        }
        xprintf("colltype:     %s\n", name);

        switch (xent->collLev)
        {
        case 0:
            name = "fastest";
            break;
        case 1:
            name = "aabb (box)";
            break;
        case 2:
            name = "sph";
            break;
        case 3:
            name = "obb";
            break;
        case 4:
            name = "bound";
            break;
        case 5:
            name = "model";
            break;
        default:
            name = "???";
            break;
        }
        xprintf("coll-lev:     %s\n", name);

        xprintf("chkby:        %s\n", xEntCollTypeStrAll(str, xent->chkby));
        xprintf("penby:        %s\n", xEntCollTypeStrAll(str, xent->penby));

        switch (xent->bound.type)
        {
        case 0:
            name = "none";
            break;
        case 1:
            name = "sph";
            break;
        case 2:
            name = "box";
            break;
        case 3:
            name = "cyl";
            break;
        case 4:
            name = "obb";
            break;
        default:
            name = "???";
            break;
        }
        xprintf("bound: %s", name);

        str[0] = '\0';
        switch (xent->bound.type)
        {
        case 0:
            break;
        case 1:
            sprintf(str, "<%.3f %.3f %.3f> %.3f", xent->bound.sph.center.x,
                    xent->bound.sph.center.y, xent->bound.sph.center.z, xent->bound.sph.r);
            break;
        case 2:
            sprintf(str, "<%.3f %.3f %.3f> <%.3f %.3f %.3f>", xent->bound.box.box.lower.x,
                    xent->bound.box.box.lower.y, xent->bound.box.box.lower.z,
                    xent->bound.box.box.upper.x, xent->bound.box.box.upper.y,
                    xent->bound.box.box.upper.z);
            break;
        case 4:
            sprintf(str, "<%.3f %.3f %.3f> <%.3f %.3f %.3f>", xent->bound.box.box.lower.x,
                    xent->bound.box.box.lower.y, xent->bound.box.box.lower.z,
                    xent->bound.box.box.upper.x, xent->bound.box.box.upper.y,
                    xent->bound.box.box.upper.z);
            break;
        }
        xprintf("   %s\n", str);

        _xEntCollis* collis = xent->collis;
        if (collis != NULL)
        {
            if (dbg_mode == 2)
            {
                xprintf("chk:        %s\n", xEntCollTypeStrAll(str, collis->chk));
                xprintf("pen:        %s\n", xEntCollTypeStrAll(str, xent->collis->pen));
                xprintf("hits:       stat:%d", xent->collis->stat_eidx - xent->collis->stat_sidx);
                xprintf("   dyn:%d", xent->collis->dyn_eidx - xent->collis->dyn_sidx);
                xprintf("   npc:%d", xent->collis->npc_eidx - xent->collis->npc_sidx);
                xprintf("   env:%d\n", xent->collis->env_eidx - xent->collis->env_sidx);
                return;
            }

            _xCollis* coll = &collis->colls[0];
            for (signed int i = 0; i < 6; i++, coll++)
            {
                if (coll->flags & 1)
                {
                    xprintf("%s-hit", scoll_name[i]);
                }
                else
                {
                    xprintf("no %s", scoll_name[i]);
                }

                if (coll->dist < 1e19f)
                {
                    xprintf("   %.3f", coll->dist);
                    xprintf(" <%.3f %.3f %.3f>\n", coll->hdng.x, coll->hdng.y, coll->hdng.z);
                    xprintf("%s-oid:  %u", scoll_name[i], coll->oid);
                    _xVec3 lhdng;
                    xMat3x3LMulVec(&lhdng, (_xMat3x3*)xent->frame, &coll->hdng);
                    xprintf("   lhdng: <%.3f %.3f %.3f>\n", lhdng.x, lhdng.y, lhdng.z);
                    if (coll->optr != NULL)
                    {
                        xprintf("%s-optr: %d", scoll_name[i], coll->optr);
                        xprintf("   (%s)\n",
                                xSceneID2Name(g_xSceneCur, ((xBase*)coll->optr)->id));
                    }
                    else
                    {
                        xprintf("\n");
                    }
                }
                else
                {
                    xprintf("\n\n\n");
                }
            }
            return;
        }
        xprintf("no checking\n");
    }
}

static void xEntDebugIPad(_xEnt* xent)
{
    float now = iTimeDiffSec(iTimeGet());
    float dt = now - dbg_ltm;
    dbg_ltm = now;
    dbg_tmr += dt;

    if (gDebugPad->on & 0x20)
    {
        if (gDebugPad->pressed & 0x20)
        {
            dbg_holding_inc = 1;
            dbg_tmr = 0.0f;
        }
        else if (dbg_holding_inc)
        {
            if (dbg_tmr >= 0.25f)
            {
                dbg_holding_inc = 0;
                goto inc;
            }
        }
        else if (dbg_tmr >= 0.065f)
        {
            goto inc;
        }
    }
    else if (dbg_holding_inc)
    {
        dbg_holding_inc = 0;
    inc:
        dbg_tmr = 0.0f;
        if (++dbg_idx >= dbg_num)
        {
            dbg_idx = 0;
        }
    }
    else if (gDebugPad->on & 0x80)
    {
        if (gDebugPad->pressed & 0x80)
        {
            dbg_holding_dec = 1;
            dbg_tmr = 0.0f;
        }
        else if (dbg_holding_dec)
        {
            if (dbg_tmr >= 0.25f)
            {
                dbg_holding_dec = 0;
                goto dec;
            }
        }
        else if (dbg_tmr >= 0.065f)
        {
            goto dec;
        }
    }
    else if (dbg_holding_dec)
    {
        dbg_holding_dec = 0;
    dec:
        dbg_tmr = 0.0f;
        if (--dbg_idx < 0)
        {
            dbg_idx = dbg_num - 1;
        }
    }

    if (gDebugPad->pressed & 0x40000)
    {
        if (dbg_mode == 1)
        {
            dbg_mode = 2;
            dbg_xents[dbg_idx]->flags |= 0x80;
        }
        else if (dbg_mode == 2)
        {
            dbg_mode = 3;
        }
        else
        {
            dbg_mode = 1;
            dbg_xents[dbg_idx]->flags &= 0x7F;
        }
    }
}

static void xModCollCB()
{
}

#pragma auto_inline on
unsigned int xEntIsVisible(_xEnt* ent)
{
    if (!(ent->flags & 1))
    {
        return 0;
    }
    if (globals.cmgr != NULL && !(ent->baseFlags & 8))
    {
        return 0;
    }
    return 1;
}
#pragma auto_inline reset

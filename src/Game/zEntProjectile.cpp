// TODO: 99.3% (NonMatching). Startup inlines addCat with the category still in
// a register (li r0, N; mulli) while addProps is folded; prepSystem,
// catFromPrjType, Init, ZPRJ_render and gen_aimer differ only in register
// allocation or one empty switch case. zEntProjectileDeadStrippedConst must be
// stripped by the linker once this unit is Matching.
#include "zEntProjectile.h"
#include "xMath.h"
#include "xMath3.h"
#include "xBound.h"
#include "xCollide.h"
#include "xMemMgr.h"
#include "zGlobals.h"
#include "zScene.h"
#include "zParEmitter.h"

#ifndef NULL
#define NULL 0
#endif

extern "C" {
void* memset(void* dst, int c, unsigned int n);
void* memcpy(void* dst, const void* src, unsigned int n);
}

class _zEnt;
class zSFX;
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
    void (*post)(_xEnt*, _xScene*, float, _xEntCollis*);                // 0x4D4
    unsigned int (*depenq)(_xEnt*, _xEnt*, _xScene*, float, _xCollis*); // 0x4D8
    unsigned char trigs_cur[8];     // 0x4DC
    unsigned char trigs_prev[8];    // 0x4E4
    unsigned char trigs_entered[8]; // 0x4EC
    unsigned char trigs_exited[8];  // 0x4F4
};

extern unsigned int gActiveHeap;
extern const _xVec3 g_Y3;
extern const _xVec3 g_NY3;

float xVec3Normalize(_xVec3* o, _xVec3* v);
void xMat3x3Euler(_xMat3x3* m, _xVec3* ypr);
void xMat3x3LookVec(_xMat3x3* m, _xVec3* at);
void xMat4x3RMul3x3(_xMat4x3* o, _xMat3x3* a, _xMat4x3* b);
void xEntMotionToMatrix(_xEnt* ent, _xEntFrame* frame);
void xModelEval(xModelInstance* model);
void xModelInstanceFree(xModelInstance* model);
xModelInstance* xModelInstanceAlloc(RpAtomic* data, void* object, unsigned short flags,
                                    unsigned char boneIndex, unsigned char* boneRemap);
void xModelPoolInit(unsigned int count, unsigned int numMatrices);
void xEntRender(_xEnt* ent);
void xEntBeginUpdate(_xEnt* ent, _xScene* sc, float dt);
void xEntEndUpdate(_xEnt* ent, _xScene* sc, float dt);
void xEntBeginCollide(_xEnt* ent, _xScene* sc, float dt);
void xEntEndCollide(_xEnt* ent, _xScene* sc, float dt);
void xEntCollide(_xEnt* ent, _xScene* sc, float dt);
void xEntApplyPhysics(_xEnt* ent, _xScene* sc, float dt);
void xFFXApply(_xEnt* ent, _xScene* sc, float dt);
signed int xFFXAddEffect(_xEnt* ent, void (*dof)(_xEnt*, _xScene*, float, void*), void* fdata);
void zGustUpdateEnt(_xEnt* ent, _xScene* sc, float dt, void* gdata);
void xShadowRender(_xEnt* ent);
void iShadowCollide(iShadowCache* cache, _xEnt* ent, float a, float b);
void iShadowAdd(iShadowCache* cache, unsigned char alpha);
void zEntPlayer_Damage(xBase* src, unsigned int damage);
signed int xUtil_yesno(float wt_yes);
void* xSTFindAsset(unsigned int aid, unsigned int* size);
unsigned int xStrHash(const char* str);
void NewList(st_ELIST* list);
st_ENODE* RemHead(st_ELIST* list);
void AddHead(st_ELIST* list, st_ENODE* node);
void AddTail(st_ELIST* list, st_ENODE* node);
void MoveList(st_ELIST* dst, st_ELIST* src);

void ZPRJ_wrap_bupdate(_xEnt* prjent, _xVec3* pos);

static signed int ZPRJ_bounce_calcmove(zEntProjectile* prj, float dt, zScene* scn,
                                       _xEntFrame* frame);
static signed int ZPRJ_bounce_logix(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame);
static signed int ZPRJ_bounce_attach(zEntProjectile* prj);
static signed int ZPRJ_grav_calcmove(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame);
static signed int ZPRJ_grav_logix(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame);
static signed int ZPRJ_grav_attach(zEntProjectile* prj);
static signed int ZPRJ_arrow_logix(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame);
static signed int ZPRJ_arrow_attach(zEntProjectile* prj);
static signed int ZPRJ_solv_velvec(float grav, float arc_factor, float spd_limit,
                                   float dst_range, _xVec3 vec_tgt, _xVec3* vec_vel);

static inline st_ZPRJ_TYPERECORD* ZPRJ_findType(unsigned int aid);

static inline signed int ZPRJ_SetupPrelim(zEntProjectile* prj, en_ZPRJ_CATEGORY cat,
                                          unsigned int ptaid);

static inline void ZPRJ_poolAdd(signed int idx);
static signed int g_prj_init;
static st_PROJECTILE_DEPOT g_prjdepot = {};
static st_ZPRJ_CATEGORY_FUNCS g_catFuncs[13] = {};
static st_ZPRJ_CATEGORY_PROPS g_iniProps[13] = {};
static _zParEmitter* g_paremit[32] = {};
static char* g_parname[32] = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "PAREMIT_PROJFLIGHT_SMOKE",
    "PAREMIT_PROJFLIGHT_WOOD",
    "PAREMIT_PROJFLIGHT_TAR",
    NULL,
    "PAREMIT_PROJFLIGHT_FIREBALL",
    "PAREMIT_PROJFLIGHT_NO_DAMAGE_FIREBALL",
    "PAREMIT_PROJFLIGHT_WITCHDOCTOR",
    "PAREMIT_PROJFLIGHT_MASTERMIND",
};
static zSFX* g_sounds[1] = {};
static char* g_sfxnames[1] = {};

class st_ZPRJ_AUTOCALC_INFO
{
public:
    signed int method;     // 0x0
    _xVec3 vel_launch;     // 0x4
    _xVec3 dir_launch;     // 0x10
    float spd_muzzle;      // 0x1C
    _xVec3 pos_launch;     // 0x20
    _xVec3 pos_target;     // 0x2C
    float acc_gravity;     // 0x38
    float fac_arcness;     // 0x3C
    float dst_range;       // 0x40
    unsigned char pad[0x60];
};

#define ZPRJ_NOTIFY_DEFAULT 0x45

// Event dispatch: category handler first, then the owner. Every caller inlines it.
inline void zEntProjectile_DoEffects(zEntProjectile* prj, en_PROJECTILE_EVENT event, float dt)
{
    en_ZPRJ_FX_TYPE fxtyp = (en_ZPRJ_FX_TYPE)prj->ptype->src_ptype->efxType;
    switch (event)
    {
    case ZPRJ_EVNT_LAUNCH:
        break;
    case ZPRJ_EVNT_INFLIGHT:
        prj->PEFX_flight(fxtyp, dt);
        break;
    case ZPRJ_EVNT_ATREST:
        break;
    }
}

inline signed int zEntProjectile_SendEvent(zEntProjectile* prj, en_PROJECTILE_EVENT event,
                                           float dt, void* edata)
{
    signed int rc = ZPRJ_NOTIFY_DEFAULT;
    st_ZPRJ_CATEGORY_FUNCS* catfuncs = &g_catFuncs[prj->cat_proj];
    if (catfuncs->notify != NULL)
    {
        rc = catfuncs->notify(prj, event, edata);
    }
    if (prj->ownerNotify != NULL)
    {
        prj->ownerNotify(prj, event, edata);
    }
    if (prj->flg_prop & 0x20)
    {
        zEntProjectile_DoEffects(prj, event, dt);
    }
    return rc;
}

// Did the last collision pass touch anything?
#define ZPRJ_HIT_SOMETHING(prj)                                                                \
    (((prj)->plyr_colrec->flags & 1) || ((prj)->collis->colls[0].flags & 1) ||             \
     ((prj)->collis->colls[1].flags & 1) ||                                                 \
     (prj)->collis->env_sidx < (prj)->collis->env_eidx ||                                   \
     (prj)->collis->npc_sidx < (prj)->collis->npc_eidx ||                                   \
     (prj)->collis->stat_sidx < (prj)->collis->stat_eidx ||                                 \
     (prj)->collis->dyn_sidx < (prj)->collis->dyn_eidx)


inline void ZPRJ_recycle(zEntProjectile* prj)
{
    if (!(prj->flg_stat & 0x10000000) && !(prj->flg_stat & 0x40000))
    {
        zEntProjectile_SendEvent(prj, ZPRJ_EVNT_DEATH, 0.0f, NULL);
    }
    if (!(prj->flg_stat & 0x08000000))
    {
        if (g_catFuncs[prj->cat_proj].detach != NULL)
        {
            g_catFuncs[prj->cat_proj].detach(prj);
        }
    }
    prj->flg_stat = 0;
    if (prj->model != NULL)
    {
        xModelInstanceFree(prj->model);
    }
    prj->model = NULL;
}

void zEntProjectile_Startup()
{
    st_ZPRJ_CATEGORY_FUNCS fmap_gen = {
        1, zEntProjectile_gen_attach, NULL, zEntProjectile_gen_logix, NULL, NULL,
        zEntProjectile_gen_render, zEntProjectile_gen_aimer, NULL, NULL
    };
    st_ZPRJ_CATEGORY_FUNCS fmap_arrow = {
        1, ZPRJ_arrow_attach, NULL, ZPRJ_arrow_logix, NULL, NULL,
        zEntProjectile_gen_render, zEntProjectile_gen_aimer, NULL, NULL
    };
    st_ZPRJ_CATEGORY_FUNCS fmap_grav = {
        1, ZPRJ_grav_attach, NULL, ZPRJ_grav_logix, ZPRJ_grav_calcmove, NULL,
        zEntProjectile_gen_render, zEntProjectile_gen_aimer, NULL, NULL
    };
    st_ZPRJ_CATEGORY_FUNCS fmap_bounce = {
        1, ZPRJ_bounce_attach, NULL, ZPRJ_bounce_logix, ZPRJ_bounce_calcmove, NULL,
        zEntProjectile_gen_render, zEntProjectile_gen_aimer, NULL, NULL
    };
    st_ZPRJ_CATEGORY_PROPS prop_gen = { 0x80000001, 3, 0, 0, 0, 0 };
    st_ZPRJ_CATEGORY_PROPS prop_arrow = { 0xB0000021, 3, 0x3F, 0, 0, 0 };
    st_ZPRJ_CATEGORY_PROPS prop_grav = { 0xB0000021, 3, 0x3F, 0x7E, 0, 0 };
    st_ZPRJ_CATEGORY_PROPS prop_bounce = { 0xB0000021, 3, 0x3F, 0x7E, 0, 0x02000000 };

    zEntProjectile_addCat(ZPRJ_CAT_UNKNOWN, &fmap_gen);
    zEntProjectile_addProps(ZPRJ_CAT_UNKNOWN, &prop_gen);
    zEntProjectile_addCat(ZPRJ_CAT_ARROW, &fmap_arrow);
    zEntProjectile_addProps(ZPRJ_CAT_ARROW, &prop_arrow);
    zEntProjectile_addCat(ZPRJ_CAT_GRAVBALL, &fmap_grav);
    zEntProjectile_addProps(ZPRJ_CAT_GRAVBALL, &prop_grav);
    zEntProjectile_addCat(ZPRJ_CAT_BOUNCEBALL, &fmap_bounce);
    zEntProjectile_addProps(ZPRJ_CAT_BOUNCEBALL, &prop_bounce);
}

void zEntProjectile_Shutdown()
{
}

#pragma auto_inline on
signed int zEntProjectile_addCat(en_ZPRJ_CATEGORY cat, st_ZPRJ_CATEGORY_FUNCS* funcs)
{
    signed int isok = 1;
    st_ZPRJ_CATEGORY_FUNCS* catmap = &g_catFuncs[cat];
    if (cat >= ZPRJ_CAT_NOMORE)
    {
        isok = 0;
    }
    else
    {
        if (catmap->flg_cback & 1)
        {
            isok = -1;
        }
        memcpy(catmap, funcs, sizeof(st_ZPRJ_CATEGORY_FUNCS));
    }
    if (catmap->render == NULL)
    {
        catmap->render = zEntProjectile_gen_render;
    }
    return isok;
}

#pragma auto_inline reset

#pragma auto_inline on
signed int zEntProjectile_addProps(en_ZPRJ_CATEGORY cat, st_ZPRJ_CATEGORY_PROPS* props)
{
    signed int isok = 1;
    if (cat >= ZPRJ_CAT_NOMORE)
    {
        isok = 0;
    }
    else
    {
        memcpy(&g_iniProps[cat], props, sizeof(st_ZPRJ_CATEGORY_PROPS));
    }
    return isok;
}

#pragma auto_inline reset

void zEntProjectile_registerType(zProjectileTypeAsset* ptype)
{
    st_PROJECTILE_DEPOT* depot = &g_prjdepot;
    if (depot->typeidx < depot->typecnt)
    {
        signed int idx = depot->typeidx++;
        st_ZPRJ_TYPERECORD* rec = &depot->typetable[idx];
        rec->src_ptype = ptype;
        rec->aid_ptype = ptype->id;
        rec->mdlFile[ZPRJ_MDLIDX_STD] = (RpAtomic*)xSTFindAsset(ptype->modelID, NULL);
        if (ptype->atRestModelID != 0)
        {
            rec->mdlFile[ZPRJ_MDLIDX_POSTHIT] = (RpAtomic*)xSTFindAsset(ptype->atRestModelID, NULL);
        }
        else
        {
            rec->mdlFile[ZPRJ_MDLIDX_POSTHIT] = NULL;
        }
    }
}

void zEntProjectile_scenePrepare()
{
    st_PROJECTILE_DEPOT* depot = &g_prjdepot;
    if (g_prj_init++ == 0)
    {
        memset(depot, 0, sizeof(st_PROJECTILE_DEPOT));
        depot->rawcnt = 0;
        depot->rawpool = NULL;
        depot->typetable = NULL;
        NewList(&depot->prjpool);
        NewList(&depot->prjinuse);
    }
}

void zEntProjectile_sceneReset()
{
    st_PROJECTILE_DEPOT* depot = &g_prjdepot;
    while (depot->prjinuse.head->next != NULL)
    {
        zEntProjectile* prj = ((st_ZPRJ_LINKNODE*)RemHead(&depot->prjinuse))->backref;
        ZPRJ_recycle(prj);
        AddTail(&g_prjdepot.prjpool, &prj->linknode.node);
    }
}

void zEntProjectile_sceneFinish()
{
    st_PROJECTILE_DEPOT* depot = &g_prjdepot;
    if (--g_prj_init == 0)
    {
        depot->rawcnt = 0;
        depot->rawpool = NULL;
        NewList(&depot->prjpool);
        NewList(&depot->prjinuse);
    }
}

static inline st_ZPRJ_TYPERECORD* ZPRJ_allocTypes(signed int cnt)
{
    st_ZPRJ_TYPERECORD* table;
    if (cnt < 1)
    {
        table = NULL;
    }
    else
    {
        table = (st_ZPRJ_TYPERECORD*)xMemAlloc(gActiveHeap, cnt * sizeof(st_ZPRJ_TYPERECORD));
        memset(table, 0, cnt * (signed int)sizeof(st_ZPRJ_TYPERECORD));
    }
    return table;
}

void zEntProjectile_prepSystem(signed int num_ptype, zScene*)
{
    st_PROJECTILE_DEPOT* depot = &g_prjdepot;
    signed int i;

    if (depot->typetable != NULL || depot->rawpool != NULL)
    {
        return;
    }
    {
        if (num_ptype != 0)
        {
            st_ZPRJ_TYPERECORD* table = ZPRJ_allocTypes(num_ptype);
            depot->typetable = table;
            if (table != NULL)
            {
                depot->typecnt = num_ptype;
            }
        }

        if (depot->typetable != NULL)
        {
            zEntProjectile* pool =
                (zEntProjectile*)xMemAlloc(gActiveHeap, 100 * sizeof(zEntProjectile));
            memset(pool, 0x33, 100 * sizeof(zEntProjectile));
            depot->rawpool = pool;
            if (pool != NULL)
            {
                depot->rawcnt = 100;
                xModelPoolInit(100, 2);
            }
        }

        for (i = 0; i < depot->rawcnt; i++)
        {
            ZPRJ_poolAdd(i);
        }

        for (i = 0; i < 1; i++)
        {
            if (g_sfxnames[i] == NULL)
            {
                g_sounds[i] = NULL;
            }
            else
            {
                g_sounds[i] = (zSFX*)zSceneFindObject(xStrHash(g_sfxnames[i]));
            }
        }

        for (i = 0; i < 32; i++)
        {
            if (g_parname[i] == NULL)
            {
                g_paremit[i] = NULL;
            }
            else
            {
                g_paremit[i] = (_zParEmitter*)zSceneFindObject(xStrHash(g_parname[i]));
            }
        }
    }
}

static inline void ZPRJ_poolAdd(signed int idx)
{
    st_PROJECTILE_DEPOT* depot = &g_prjdepot;
    zEntProjectile* prj = &depot->rawpool[idx];
    memset(prj, 0, sizeof(zEntProjectile));
    prj->linknode.backref = prj;
    AddTail(&depot->prjpool, &prj->linknode.node);
}

signed int zEntProjectile_poolUpdate(en_ZPRJ_CATEGORY cat, zScene* scn, float dt)
{
    st_ELIST holdlist;
    st_ELIST* inuse;
    zEntProjectile* tmpprj;
    st_PROJECTILE_DEPOT* depot;
    signed int cnt;
    st_ZPRJ_LINKNODE* tmplink;

    cnt = 0;
    depot = &g_prjdepot;
    inuse = &depot->prjinuse;
    tmplink = (st_ZPRJ_LINKNODE*)inuse->head;

    while (tmplink->node.next != NULL)
    {
        tmplink->backref->ZPRJ_update(scn, dt);
        tmplink = (st_ZPRJ_LINKNODE*)tmplink->node.next;
        cnt++;
    }

    NewList(&holdlist);
    MoveList(&holdlist, &depot->prjinuse);
    while (holdlist.head->next != NULL)
    {
        tmpprj = ((st_ZPRJ_LINKNODE*)RemHead(&holdlist))->backref;
        if (tmpprj->flg_stat & 0x80000000)
        {
            ZPRJ_recycle(tmpprj);
            AddTail(&g_prjdepot.prjpool, &tmpprj->linknode.node);
        }
        else
        {
            AddTail(inuse, &tmpprj->linknode.node);
        }
    }
    return cnt;
}

void zEntProjectile_poolRender(en_ZPRJ_CATEGORY cat)
{
    st_ZPRJ_LINKNODE* tmplink = (st_ZPRJ_LINKNODE*)g_prjdepot.prjinuse.head;
    zEntProjectile* tmpprj;

    while (tmplink->node.next != NULL)
    {
        tmpprj = tmplink->backref;
        if ((cat == ZPRJ_CAT_UNKNOWN || cat == tmpprj->cat_proj) &&
            !(tmpprj->flg_stat & 0x40000000))
        {
            tmpprj->ZPRJ_render();
            tmpprj->flg_stat |= 0x40000000;
        }
        tmplink = (st_ZPRJ_LINKNODE*)tmplink->node.next;
    }
}


en_ZPRJ_CATEGORY zEntProjectile_catFromPrjType(unsigned int ptypeID, en_ZPRJ_CATEGORY prefer)
{
    st_ZPRJ_TYPERECORD* ptype = ZPRJ_findType(ptypeID);
    if (ptype == NULL)
    {
        return prefer;
    }
    if (ptype->categ == ZPRJ_CAT_UNKNOWN)
    {
        return prefer;
    }
    return ptype->categ;
}

static inline zEntProjectile* ZPRJ_getNextAvail()
{
    st_PROJECTILE_DEPOT* depot = &g_prjdepot;
    st_ZPRJ_LINKNODE* link;
    zEntProjectile* prj;
    static signed int warned = 0.0f;

    if (depot->prjpool.head->next == NULL)
    {
        if (!warned)
        {
            warned = 1;
        }
        return NULL;
    }

    warned = 0;
    link = (st_ZPRJ_LINKNODE*)RemHead(&depot->prjpool);
    prj = link->backref;
    memset(prj, 0, sizeof(zEntProjectile));
    prj->linknode.backref = prj;
    return prj;
}

zEntProjectile* zEntProjectile_acquire(en_ZPRJ_CATEGORY cat, unsigned int ptypeID)
{
    zEntProjectile* rc;
    zEntProjectile* prj;
    signed int status;

    status = 1;
    prj = ZPRJ_getNextAvail();
    rc = prj;

    if (prj == NULL)
    {
        return prj;
    }

    AddHead(&g_prjdepot.prjinuse, &prj->linknode.node);
    if (!prj->Init(cat, ptypeID))
    {
        status = 0;
    }
    if (!status)
    {
        rc = NULL;
        prj->flg_stat |= 0x10000000;
        prj->flg_stat |= 0x80000000;
        prj->state = ZPRJ_STATE_DEAD;
    }
    return rc;
}

en_ZPRJ_STATES zEntProjectile::StateSet(en_ZPRJ_STATES state)
{
    en_ZPRJ_STATES oldstate;

    if (this->state == state)
    {
        return this->state;
    }

    switch (this->state)
    {
    case ZPRJ_STATE_UNUSED:
        break;
    case ZPRJ_STATE_STANDBY:
    case ZPRJ_STATE_ACTIVE:
        flg_stat &= 0xFFFF;
        flg_stat |= 0x10000;
        break;
    case ZPRJ_STATE_DEAD:
        break;
    }

    oldstate = this->state;
    this->state = state;
    tmr_curstate = 0.0f;

    switch (this->state)
    {
    case ZPRJ_STATE_UNUSED:
    case ZPRJ_STATE_STANDBY:
        break;
    case ZPRJ_STATE_ACTIVE:
        tmr_alive = 0.0f;
        tmr_emit = 0.0f;
        xVec3Copy(&pos_home, &((_xMat4x3*)model->Mat)->pos);
        flg_stat |= 0x20000;
        zEntProjectile_SendEvent(this, ZPRJ_EVNT_ACTIVE, 0.0f, NULL);
        break;
    case ZPRJ_STATE_DEAD:
        flg_stat |= 0x80000000;
        this->state = ZPRJ_STATE_DEAD;
        break;
    }

    if (oldstate != state)
    {
        zEntProjectile_SendEvent(this, ZPRJ_EVNT_TRANSTATE, 0.0f, (void*)oldstate);
    }
    return this->state;
}

void zEntProjectile::ColDelaySet(float tym, signed int addto)
{
    if (tmr_coldelay < 0.0f)
    {
        tmr_coldelay = tym;
    }
    else if (addto)
    {
        tmr_coldelay += tym;
    }
    else
    {
        tmr_coldelay = tym;
    }
}

void zEntProjectile::Abort()
{
    flg_stat |= 0x40000;
    zEntProjectile_SendEvent(this, ZPRJ_EVNT_ABORT, 0.0f, NULL);
    StateSet(ZPRJ_STATE_DEAD);
}

#pragma auto_inline on
void zEntProjectile::SyncFrame(signed int applyToModel)
{
    _xEnt* ent = this;
    if (ent->frame->mode != 0)
    {
        xEntMotionToMatrix(ent, ent->frame);
        if (applyToModel)
        {
            if (!(ent->frame->mode & 0x20000))
            {
                memcpy(ent->model->Mat, ent->frame, sizeof(_xMat3x3));
            }
            if (!(ent->frame->mode & 0x10000))
            {
                _xVec3* upos = &ent->frame->mat.pos;
                _xVec3* mpos = &((_xMat4x3*)ent->model->Mat)->pos;
                xVec3Copy(mpos, upos);
            }
            xModelEval(ent->model);
            if (ent->bupdate != NULL)
            {
                ent->bupdate(ent, &((_xMat4x3*)ent->model->Mat)->pos);
            }
        }
        frame->mode = 0;
    }
}

#pragma auto_inline reset

signed int zEntProjectile::SendEvent(en_PROJECTILE_EVENT event)
{
    return zEntProjectile_SendEvent(this, event, globals.update_dt, NULL);
}

signed int zEntProjectile::SendEvent(en_PROJECTILE_EVENT event, float dt)
{
    return zEntProjectile_SendEvent(this, event, dt, NULL);
}

void zEntProjectile::FlexVecSet(_xVec3* vec_flex)
{
    xVec3Copy(&common.vec_flex, vec_flex);
}

void zEntProjectile::FlexMatSet(_xVec3* rot_euler, float dt)
{
    _xVec3 ang = {};
    if (dt > 0.0f)
    {
        xVec3SMul(&ang, rot_euler, dt);
    }
    else
    {
        xVec3Copy(&ang, rot_euler);
    }
    xMat3x3Euler(&common.mat_flex, &ang);
}

inline void zEntProjectile_AssignModel(zEntProjectile* prj, en_ZPRJ_MODEL_INDEX midx)
{
    st_ZPRJ_TYPERECORD* ptype = prj->ptype;
    _xMat4x3 mat_copy = {};
    signed int have_copy = 0;

    if (ptype->mdlFile[midx] != NULL)
    {
        if (prj->model != NULL)
        {
            memcpy(&mat_copy, prj->model->Mat, sizeof(_xMat4x3));
            have_copy = 1;
            xModelInstanceFree(prj->model);
        }
        prj->model = NULL;
        prj->model = xModelInstanceAlloc(ptype->mdlFile[midx], prj, 0, 0, NULL);
        if (have_copy)
        {
            memcpy(prj->model->Mat, &mat_copy, sizeof(_xMat4x3));
        }
    }
}

signed int zEntProjectile::AimAtTarget(ST_CAT_AIMDATA* info, signed int apply)
{
    st_ZPRJ_CATEGORY_FUNCS* catf = &g_catFuncs[cat_proj];
    if (catf->aimer != NULL)
    {
        return catf->aimer(this, info, apply);
    }
    return 0;
}

static inline st_ZPRJ_TYPERECORD* ZPRJ_findType(unsigned int aid)
{
    st_PROJECTILE_DEPOT* depot = &g_prjdepot;
    st_ZPRJ_TYPERECORD* rec;
    signed int i;
    st_ZPRJ_TYPERECORD* da_type = NULL;
    for (i = 0; i < depot->typeidx; i++)
    {
        rec = &depot->typetable[i];
        if (rec->aid_ptype == aid)
        {
            da_type = rec;
            break;
        }
    }
    return da_type;
}

signed int zEntProjectile::Init(en_ZPRJ_CATEGORY cat, unsigned int ptypeID)
{
    st_ZPRJ_CATEGORY_FUNCS* catfuncs = NULL;
    signed int result = 1;

    if (!ZPRJ_SetupPrelim(this, cat, ptypeID))
    {
        result = 0;
    }

    if (result)
    {
        catfuncs = &g_catFuncs[cat_proj];
        if (!(catfuncs->flg_cback & 1))
        {
            result = 0;
        }
    }

    if (result)
    {
        asset = &raw_asset;
        asset->flags = 1;
        asset->redMult = 1.0f;
        asset->greenMult = 1.0f;
        asset->blueMult = 1.0f;
        asset->seeThru = 1.0f;
        asset->seeThruSpeed = 0.5f;
        asset->modelInfoID = 0;
        asset->animListID = 0;
    }

    if (result && !Setup_xEnt())
    {
        result = 0;
    }

    if (result)
    {
        SetupFrame(NULL, NULL);
    }

    if (result)
    {
        xEntBeginUpdate(this, globals.sceneCur, globals.update_dt);
    }

    if (result)
    {
        static float pct_docol_default = 1.0f;
        pct_docol = 0.75f;
        pct_docol = pct_docol_default;
        tmr_coldelay = 0.1f;
    }

    if (result && !catfuncs->attach(this))
    {
        result = 0;
        flg_stat |= 0x08000000;
    }
    return result;
}

static inline signed int ZPRJ_SetupPrelim(zEntProjectile* prj, en_ZPRJ_CATEGORY cat,
                                          unsigned int ptaid)
{
    signed int isok = 1;

    prj->flg_stat |= 0x10000;
    prj->state = ZPRJ_STATE_STANDBY;
    prj->cat_proj = cat;
    st_ZPRJ_CATEGORY_PROPS* props = &g_iniProps[prj->cat_proj];
    prj->flg_prop = props->flg_props;
    prj->flg_chk = g_iniProps[prj->cat_proj].flg_chk;
    prj->flg_pen = g_iniProps[prj->cat_proj].flg_pen;
    prj->flg_categ = props->flg_categ;

    if (ptaid == 0)
    {
        isok = 0;
    }
    else
    {
        st_ZPRJ_TYPERECORD* ptype = ZPRJ_findType(ptaid);
        if (ptype == NULL)
        {
            isok = 0;
        }
        else
        {
            prj->ptype = ptype;
            prj->tym_maxlife = prj->ptype->src_ptype->destructTime;
            prj->ds2_maxlife =
                prj->ptype->src_ptype->destructDist * prj->ptype->src_ptype->destructDist;
            if (prj->ptype->src_ptype->oriented)
            {
                prj->OrientSet(0x30000000);
            }
        }
    }
    return isok;
}

static inline void ZPRJ_rotCopy(_xRot* o, _xRot* r)
{
    o->axis.x = r->axis.x;
    o->axis.y = r->axis.y;
    o->axis.z = r->axis.z;
    o->angle = r->angle;
}

static inline void ZPRJ_matScale(_xMat3x3* m, float s)
{
    xVec3SMulBy(&m->right, s);
    xVec3SMulBy(&m->up, s);
    xVec3SMulBy(&m->at, s);
}

inline void zEntProjectile::SetupFrame(_xVec3* pos, _xVec3* veloc)
{
    _xVec3 pos_dflt = {};
    _xVec3 vel_dflt = {};
    _xMat4x3 mat = {};

    if (pos == NULL)
    {
        pos = &pos_dflt;
    }
    if (veloc == NULL)
    {
        veloc = &vel_dflt;
    }

    xMat3x3Euler((_xMat3x3*)&mat, 0.0f, 0.0f, 0.0f);
    ZPRJ_matScale((_xMat3x3*)&mat, 1.0f);
    mat.flags = 0;
    mat.pos = *pos;
    frame->oldpos = *pos;
    bupdate(this, pos);
    memcpy(model->Mat, &mat, sizeof(_xMat4x3));
    memcpy(frame, &mat, sizeof(_xMat4x3));
    xVec3Copy(&frame->rot.axis, &g_Y3);
    frame->rot.angle = 0.0f;
    ZPRJ_rotCopy(&frame->oldrot, &frame->rot);
    frame->vel = *veloc;
    frame->oldvel = *veloc;
}

signed int zEntProjectile::Setup_xEnt()
{
    id = 0;
    baseType = 0x19;
    baseFlags = 5;
    linkCount = 0;
    link = NULL;
    flags = 1;
    moreFlags = 0;
    collType = 4;
    collLev = 4;
    pflags = g_iniProps[cat_proj].flg_physics;
    chkby = 0;
    penby = 0;
    idx = 0;
    material_alpha = 255.0f;
    updateAcc = 0.0f;
    ent_shadow = &raw_shadow;
    xVec3Copy(&ent_shadow->vec, &g_NY3);
    collis = NULL;
    frame = &raw_frame;
    eventFunc = NULL;
    update = NULL;
    bupdate = ZPRJ_wrap_bupdate;
    move = NULL;
    transl = NULL;
    bound.type = 1;
    zEntProjectile_AssignModel(this, ZPRJ_MDLIDX_STD);
    if (globals.sceneCur->baseCount[28] && (g_iniProps[cat_proj].flg_props & 2))
    {
        xFFXAddEffect(this, zGustUpdateEnt, &raw_gust);
    }
    return 1;
}

#pragma auto_inline on
void zEntProjectile::OrientSet(signed int orient)
{
    flg_prop &= 0x8FFFFFFF;
    flg_prop |= orient;
}
#pragma auto_inline reset

static inline void ZPRJ_hitPlayer(zEntProjectile* prj, _xCollis* colrec)
{
    memset(colrec, 0, sizeof(_xCollis));
    colrec->optr = &globals.player.ent;
    colrec->oid = globals.player.ent.id;
    colrec->flags = 0x1F00;
    if (prj->collis->chk & 0x10)
    {
        xBoundHitsBound(&prj->bound, &globals.player.ent.bound, colrec);
    }
}

inline void zEntProjectile::OrientToPath(_xEntFrame* frame, signed int flg_orient)
{
    _xVec3 unused = {};
    _xVec3 vec_tmp = {};
    signed int opt_orient = flg_prop & 0x70000000;

    switch (opt_orient)
    {
    case 0:
        break;
    case 0x30000000:
        xVec3SMul(&vec_tmp, &frame->vel, -1.0f);
        xMat3x3LookVec((_xMat3x3*)&frame->mat, &vec_tmp);
        break;
    case 0x10000000:
    {
        _xVec3* center = xBoundCenter(&((_xEnt*)&globals.player.ent)->bound);
        xVec3Sub(&vec_tmp, &frame->mat.pos, center);
        xMat3x3LookVec((_xMat3x3*)&frame->mat, &vec_tmp);
        break;
    }
    case 0x20000000:
        xVec3Sub(&vec_tmp, &frame->mat.pos, &globals.camera.hi_cam.mat.pos);
        xMat3x3LookVec((_xMat3x3*)&frame->mat, &vec_tmp);
        break;
    }
}

signed int zEntProjectile::ZPRJ_update(zScene* scn, float dt)
{
    _xEntCollis colls = {};
    _xCollis colrec = {};
    st_ZPRJ_CATEGORY_FUNCS* catfuncs;

    if (state == ZPRJ_STATE_STANDBY)
    {
        return 1;
    }
    if (state == ZPRJ_STATE_DEAD)
    {
        return 1;
    }
    if (!(flg_stat & 0x20000))
    {
        return 0;
    }

    catfuncs = &g_catFuncs[cat_proj];
    plyr_colrec = &colrec;
    collis = &colls;
    collis->chk = flg_chk;
    collis->pen = flg_pen;

    if (frame != NULL)
    {
        if (frame->mode != 0)
        {
            SyncFrame(1);
        }
        frame = NULL;
    }
    frame = &raw_frame;

    xEntEndUpdate(this, scn, dt);
    xEntBeginUpdate(this, scn, dt);
    xEntBeginCollide(this, scn, dt);

    ZPRJ_hitPlayer(this, plyr_colrec);

    if (DoColCheck(dt))
    {
        xEntCollide(this, scn, dt);
        if (collis->idx > 6 && !(collis->colls[0].flags & 1) && !(collis->colls[1].flags & 1))
        {
            LookForFloorRoof(scn, dt);
        }
    }
    xEntEndCollide(this, scn, dt);

    tmr_alive += dt;
    tmr_curstate += dt;
    tmr_coldelay = -1.0f > tmr_coldelay - dt ? -1.0f : tmr_coldelay - dt;
    flg_stat &= 0x9FFFFFFF;

    zEntProjectile_SendEvent(this, ZPRJ_EVNT_NEWTIME, dt, NULL);
    if (plyr_colrec->flags & 1)
    {
        zEntProjectile_SendEvent(this, ZPRJ_EVNT_HITPLYR, dt, plyr_colrec);
    }

    catfuncs->logix(this, dt, scn, frame);
    if (frame->mode != 0)
    {
        SyncFrame(0);
    }

    OrientToPath(frame, 1);

    if (pflags & 2)
    {
        xEntApplyPhysics(this, scn, dt);
    }
    if (pflags & 1)
    {
        if (catfuncs->calcmove != NULL)
        {
            catfuncs->calcmove(this, dt, scn, frame);
            if (frame->mode != 0)
            {
                SyncFrame(0);
            }
        }
    }
    if (flg_prop & 2)
    {
        xFFXApply(this, scn, dt);
    }

    zEntProjectile_SendEvent(this, ZPRJ_EVNT_ENDTIME, dt, NULL);

    frame = NULL;
    collis = NULL;
    plyr_colrec = NULL;

    if (tmr_alive > tym_maxlife && state != ZPRJ_STATE_DEAD)
    {
        zEntProjectile_SendEvent(this, ZPRJ_EVNT_SELFDESTRUCT, dt, NULL);
        StateSet(ZPRJ_STATE_DEAD);
    }
    return 1;
}

signed int zEntProjectile::DoColCheck(float dt)
{
    st_ZPRJ_CATEGORY_FUNCS* catfuncs;
    signed int result = 0;
    static signed int heuristic = 2.0f;
    // Unused, but their initializers fill 0xFC bytes of .rodata here.
    _xEntFrame unused_frame = {};
    _xVec3 unused_vec = {};

    if (tmr_coldelay > 0.0f)
    {
        return 0;
    }

    catfuncs = &g_catFuncs[cat_proj];
    if (ownerDoCollide != NULL)
    {
        heuristic = 0;
    }
    else if (catfuncs->collide != NULL)
    {
        heuristic = 1;
    }

    switch (heuristic)
    {
    case 0:
        return ownerDoCollide(this, globals.sceneCur, dt);
    case 1:
        return catfuncs->collide(this, globals.sceneCur, dt);
    case 2:
        return 1;
    case 3:
        return xUtil_yesno(pct_docol);
    case 4:
        return (float)__fabs((float)cos(tmr_alive)) < pct_docol;
    case 6:
        return dt * xsqrt(frame->vel.x * frame->vel.x + frame->vel.y * frame->vel.y +
                          frame->vel.z * frame->vel.z) > 0.49f * bound.sph.r;
    case 5:
        if (pct_docol < 0.0f)
        {
            pct_docol = 1.0f;
        }
        else
        {
            result = 1;
            pct_docol = -1.0f;
        }
        break;
    }
    return result;
}

void ZPRJ_wrap_bupdate(_xEnt* prjent, _xVec3* pos)
{
    xVec3Copy(&prjent->bound.sph.center, pos);
    prjent->bound.sph.r = 0.5f;
    xBoundUpdate(&prjent->bound);
}

signed int zEntProjectile::ZPRJ_render()
{
    _xVec3 vec = {};

    if (!(flg_stat & 0x20000))
    {
        return 1;
    }
    if (state <= ZPRJ_STATE_STANDBY)
    {
        return 1;
    }

    st_ZPRJ_CATEGORY_FUNCS* catf = &g_catFuncs[cat_proj];
    catf->render(this);

    if (flg_prop & 8)
    {
        xShadowRender(this);
    }
    else if (xEntIsVisible(this) && (flg_prop & 4))
    {
        _xMat4x3* mat = (_xMat4x3*)model->Mat;
        _xMat4x3* pmat = (_xMat4x3*)globals.player.ent.model->Mat;
        float dst2;
        float dst2_plyr;

        xVec3Sub(&vec, &globals.camera.hi_cam.mat.pos, &mat->pos);
        dst2 = vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
        xVec3Sub(&vec, &pmat->pos, &mat->pos);
        dst2_plyr = vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
        if (dst2 < dst2_plyr)
        {
        }
        else
        {
            dst2 = dst2_plyr;
        }

        xVec3Copy(&ent_shadow->pos, &bound.sph.center);
        ent_shadow->pos.y = 2.0f * bound.sph.r + ent_shadow->pos.y;
        if (dst2 < 100.0f)
        {
            xShadowRender(this);
        }
        else
        {
            iShadowCollide(&ishadow, this, 0.75f, 1.0f);
            iShadowAdd(&ishadow, 0x50);
        }
    }
    return 1;
}

unsigned char zEntProjectile::LookForFloorRoof(zScene* scn, float dt)
{
    signed int i;
    _xCollis* best;
    _xCollis* flor;
    _xCollis* ceil;
    _xCollis* cur_rec;

    best = NULL;
    cur_rec = NULL;
    flor = &collis->colls[0];
    ceil = &collis->colls[1];

    if (!(flor->flags & 1))
    {
        for (i = collis->stat_sidx; i < collis->stat_eidx; i++)
        {
            cur_rec = &collis->colls[i];
            if (!(cur_rec->hdng.y > -0.9f))
            {
                if (best == NULL)
                {
                    best = cur_rec;
                }
                else if (!(cur_rec->hdng.y > best->hdng.y))
                {
                    best = cur_rec;
                }
            }
        }
        for (i = collis->env_sidx; i < collis->env_eidx; i++)
        {
            cur_rec = &collis->colls[i];
            if (!(cur_rec->hdng.y > -0.9f))
            {
                if (best == NULL)
                {
                    best = cur_rec;
                }
                else if (!(cur_rec->hdng.y > best->hdng.y))
                {
                    best = cur_rec;
                }
            }
        }
        if (best != NULL)
        {
            memcpy(flor, best, sizeof(_xCollis));
            cur_rec->flags |= 6;
        }
    }

    if (!(ceil->flags & 1))
    {
        best = NULL;
        for (i = collis->stat_sidx; i < collis->stat_eidx; i++)
        {
            cur_rec = &collis->colls[i];
            if (!(cur_rec->hdng.y < 0.9f))
            {
                if (best == NULL)
                {
                    best = cur_rec;
                }
                else if (!(cur_rec->hdng.y > best->hdng.y))
                {
                    best = cur_rec;
                }
            }
        }
        for (i = collis->env_sidx; i < collis->env_eidx; i++)
        {
            cur_rec = &collis->colls[i];
            if (!(cur_rec->hdng.y < 0.9f))
            {
                if (best == NULL)
                {
                    best = cur_rec;
                }
                else if (!(cur_rec->hdng.y > best->hdng.y))
                {
                    best = cur_rec;
                }
            }
        }
        if (best != NULL)
        {
            memcpy(ceil, best, sizeof(_xCollis));
            cur_rec->flags |= 0xA;
        }
    }

    return (flor->flags & 1) || (ceil->flags & 1);
}

signed int zEntProjectile_gen_attach(zEntProjectile* prj)
{
    switch (prj->cat_proj)
    {
    case ZPRJ_CAT_UNKNOWN:
        break;
    case ZPRJ_CAT_ARROW:
        prj->cat_data.arrow.amStuck = 0;
        break;
    case ZPRJ_CAT_GRAVBALL:
    case ZPRJ_CAT_BOUNCEBALL:
        prj->cat_data.gravball.acc_gravity = 0.0f;
        break;
    }
    return 1;
}

signed int zEntProjectile_gen_logix(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame)
{
    _xVec3 vec = {};

    if (prj->tmr_alive > prj->tym_maxlife)
    {
        zEntProjectile_SendEvent(prj, ZPRJ_EVNT_SELFDESTRUCT, dt, NULL);
        prj->StateSet(ZPRJ_STATE_DEAD);
    }
    else
    {
        xVec3Sub(&vec, &frame->mat.pos, &prj->pos_home);
        if (xVec3Dot(&vec, &vec) > prj->ds2_maxlife)
        {
            prj->StateSet(ZPRJ_STATE_DEAD);
        }
    }
    return 1;
}

signed int zEntProjectile_gen_render(zEntProjectile* prj)
{
    if (xEntIsVisible(prj))
    {
        xEntRender(prj);
    }
    return 1;
}

static signed int ZPRJ_arrow_attach(zEntProjectile* prj)
{
    prj->cat_data.idx_cdat = ZPRJ_CDAT_ARROW;
    return 1;
}

static signed int ZPRJ_arrow_logix(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame)
{
    st_ZPRJ_CAT_ARROW* arrow = &prj->cat_data.arrow;

    if (arrow->amStuck)
    {
        zEntProjectile_SendEvent(prj, ZPRJ_EVNT_ATREST, dt, NULL);
    }
    else
    {
        zEntProjectile_SendEvent(prj, ZPRJ_EVNT_INFLIGHT, dt, NULL);
    }

    if (prj->tmr_alive > prj->tym_maxlife)
    {
        zEntProjectile_SendEvent(prj, ZPRJ_EVNT_SELFDESTRUCT, dt, NULL);
        prj->StateSet(ZPRJ_STATE_DEAD);
        return 1;
    }

    if (prj->plyr_colrec->flags & 1)
    {
        if (prj->flg_prop & 0x10)
        {
            zEntPlayer_Damage(prj, 1);
        }
        if (prj->flg_categ & 0x01000000)
        {
            if (arrow->amStuck == 0)
            {
                arrow->amStuck = 1;
                _xEnt* plyr = &globals.player.ent;
                arrow->ent_stuck = plyr;
                xVec3Sub(&arrow->pos_stuck, &((_xMat4x3*)plyr->model->Mat)->pos,
                         &((_xMat4x3*)prj->model->Mat)->pos);
                prj->frame->dvel.x = 0.0f;
                prj->frame->dvel.y = 0.0f;
                prj->frame->dvel.z = 0.0f;
                prj->frame->vel.x = 0.0f;
                prj->frame->vel.y = 0.0f;
                prj->frame->vel.z = 0.0f;
                prj->frame->mode |= 0xC;
                prj->OrientSet(0x10000000);
            }
        }
        else
        {
            zEntProjectile_SendEvent(prj, ZPRJ_EVNT_DETONATE, dt, NULL);
            prj->StateSet(ZPRJ_STATE_DEAD);
        }
    }
    else if (prj->collis->colls[0].flags & 1)
    {
        zEntProjectile_SendEvent(prj, ZPRJ_EVNT_DETONATE, dt, NULL);
        prj->StateSet(ZPRJ_STATE_DEAD);
    }
    else if (prj->collis->colls[1].flags & 1)
    {
        if (prj->flg_categ & 0x02000000)
        {
            arrow->amStuck = 2;
            prj->pflags |= 4;
            prj->frame->vel.x = 0.0f;
            prj->frame->vel.z = 0.0f;
            prj->frame->mode |= 4;
        }
        else
        {
            zEntProjectile_SendEvent(prj, ZPRJ_EVNT_DETONATE, dt, NULL);
            prj->StateSet(ZPRJ_STATE_DEAD);
        }
    }
    else if (prj->collis->env_sidx < prj->collis->env_eidx ||
             prj->collis->npc_sidx < prj->collis->npc_eidx ||
             prj->collis->stat_sidx < prj->collis->stat_eidx ||
             prj->collis->dyn_sidx < prj->collis->dyn_eidx)
    {
        if (prj->flg_categ & 0x02000000)
        {
            arrow->amStuck = 2;
            prj->pflags |= 4;
            prj->frame->vel.x = 0.0f;
            prj->frame->vel.z = 0.0f;
            prj->frame->mode |= 4;
        }
        else if (prj->flg_categ & 0x01000000)
        {
            arrow->amStuck = 1;
            prj->pflags |= 4;
            prj->frame->dvel.x = 0.0f;
            prj->frame->dvel.y = 0.0f;
            prj->frame->dvel.z = 0.0f;
            prj->frame->vel.x = 0.0f;
            prj->frame->vel.y = 0.0f;
            prj->frame->vel.z = 0.0f;
            prj->frame->mode |= 0xC;
            prj->OrientSet(0);
        }
        else
        {
            zEntProjectile_SendEvent(prj, ZPRJ_EVNT_DETONATE, dt, NULL);
            prj->StateSet(ZPRJ_STATE_DEAD);
        }
    }
    return 1;
}

static signed int ZPRJ_grav_attach(zEntProjectile* prj)
{
    prj->cat_data.idx_cdat = ZPRJ_CDAT_GRAVBALL;
    prj->cat_data.gravball.acc_gravity = 1.5f;
    return 1;
}

static signed int ZPRJ_grav_logix(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame)
{
    zEntProjectile_SendEvent(prj, ZPRJ_EVNT_INFLIGHT, dt, NULL);

    if (prj->tmr_alive > prj->tym_maxlife)
    {
        zEntProjectile_SendEvent(prj, ZPRJ_EVNT_SELFDESTRUCT, dt, NULL);
        prj->StateSet(ZPRJ_STATE_DEAD);
        return 1;
    }

    if ((prj->flg_prop & 0x10) && (prj->plyr_colrec->flags & 1))
    {
        zEntPlayer_Damage(prj, 1);
    }

    if (ZPRJ_HIT_SOMETHING(prj))
    {
        zEntProjectile_SendEvent(prj, ZPRJ_EVNT_DETONATE, dt, NULL);
        prj->StateSet(ZPRJ_STATE_DEAD);
    }
    return 1;
}

static signed int ZPRJ_grav_calcmove(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame)
{
    frame->dvel.x = 0.0f;
    if (prj->flg_categ & 2)
    {
        frame->dvel.y = -(dt * prj->cat_data.gravball.acc_gravity - frame->dvel.y);
    }
    else
    {
        frame->dvel.y = -(dt * prj->cat_data.gravball.acc_gravity);
        frame->dvel.y = -(dt * prj->cat_data.gravball.acc_gravity);
    }
    frame->dvel.z = 0.0f;
    frame->mode |= 8;
    return 1;
}

static signed int ZPRJ_bounce_attach(zEntProjectile* prj)
{
    prj->cat_data.idx_cdat = ZPRJ_CDAT_BOUNCE;
    prj->cat_data.bounce.tym_atrest = 3.0f;
    prj->cat_data.bounce.acc_gravity = 1.5f;
    prj->cat_data.bounce.cnt_maxBounce = 5;
    prj->cat_data.bounce.fac_elastic = 0.6f;
    prj->cat_data.bounce.pct_randKick = 0.0f;
    return 1;
}

static signed int ZPRJ_bounce_logix(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame)
{
    st_ZPRJ_CAT_BOUNCEBALL* bounce = &prj->cat_data.bounce;
    _xVec3 dir_away = {};

    if (bounce->atRest == 1)
    {
        zEntProjectile_SendEvent(prj, ZPRJ_EVNT_ATREST, dt, NULL);
    }
    else if (bounce->atRest == 2)
    {
        zEntProjectile_SendEvent(prj, ZPRJ_EVNT_INFLIGHT, dt, NULL);
    }
    else
    {
        zEntProjectile_SendEvent(prj, ZPRJ_EVNT_INFLIGHT, dt, NULL);
    }

    if (prj->tmr_alive > prj->tym_maxlife)
    {
        zEntProjectile_SendEvent(prj, ZPRJ_EVNT_SELFDESTRUCT, dt, NULL);
        prj->StateSet(ZPRJ_STATE_DEAD);
        return 1;
    }

    if (bounce->atRest == 1)
    {
        bounce->tym_atrest -= dt;
    }

    if ((prj->plyr_colrec->flags & 1) && (prj->flg_prop & 0x10))
    {
        zEntPlayer_Damage(prj, 1);
    }

    if (bounce->atRest == 1 && bounce->tym_atrest < 0.0f)
    {
        zEntProjectile_SendEvent(prj, ZPRJ_EVNT_DETONATE, dt, NULL);
        prj->StateSet(ZPRJ_STATE_DEAD);
    }
    else if (ZPRJ_HIT_SOMETHING(prj))
    {
        if ((prj->flg_categ & 0x02000000) && bounce->skip_next_bounce &&
            bounce->cnt_curBounce < bounce->cnt_maxBounce)
        {
            bounce->skip_next_bounce = 0;
            xVec3Copy(&prj->frame->vel, &prj->frame->oldvel);
            prj->frame->mode |= 4;
        }
        else if ((prj->flg_categ & 0x02000000) && bounce->cnt_curBounce < bounce->cnt_maxBounce)
        {
            bounce->cnt_curBounce++;
            bounce->skip_next_bounce = 1;
            bounce->atRest = 2;
            float scale = bounce->fac_elastic;
            if (prj->flg_categ & 2)
            {
                scale *= 0.5f;
            }
            zEntProjectile_colAccum(prj, &dir_away);
            zEntProjectile_softReflect(&prj->frame->vel, &prj->frame->oldvel, &dir_away, scale);
            prj->frame->mode |= 4;
            zEntProjectile_SendEvent(prj, ZPRJ_EVNT_BOUNCE, dt, NULL);
            if (xVec3Dot(&frame->vel, &frame->vel) < 0.0625f)
            {
                bounce->cnt_curBounce = bounce->cnt_maxBounce;
            }
        }
        else if (prj->flg_categ & 0x01000000)
        {
            if (bounce->atRest != 1)
            {
                bounce->atRest = 1;
                zEntProjectile_SendEvent(prj, ZPRJ_EVNT_STARTRESTING, dt, NULL);
                zEntProjectile_AssignModel(prj, ZPRJ_MDLIDX_POSTHIT);
            }
        }
        else if (bounce->atRest != 2)
        {
            zEntProjectile_SendEvent(prj, ZPRJ_EVNT_DETONATE, dt, NULL);
            prj->StateSet(ZPRJ_STATE_DEAD);
        }
    }
    return 1;
}

static signed int ZPRJ_bounce_calcmove(zEntProjectile* prj, float dt, zScene* scn,
                                       _xEntFrame* frame)
{
    st_ZPRJ_CAT_BOUNCEBALL* bounce = &prj->cat_data.bounce;

    if (bounce->atRest == 1)
    {
        prj->frame->dvel.x = 0.0f;
        prj->frame->dvel.y = 0.0f;
        prj->frame->dvel.z = 0.0f;
        prj->frame->vel.x = 0.0f;
        prj->frame->vel.y = 0.0f;
        prj->frame->vel.z = 0.0f;
        prj->frame->mode |= 0xC;
    }
    else
    {
        frame->dvel.x = 0.0f;
        if (prj->flg_categ & 2)
        {
            frame->dvel.y = -(dt * bounce->acc_gravity - frame->dvel.y);
        }
        else
        {
            frame->dvel.y = -(dt * bounce->acc_gravity);
        }
        frame->dvel.z = 0.0f;
        frame->mode |= 8;
    }

    if ((prj->flg_categ & 0x04000000) && bounce->atRest != 1)
    {
        xMat4x3RMul3x3(&frame->mat, &prj->common.mat_flex, &frame->mat);
        frame->mode |= 0x100;
    }
    return 1;
}

float zEntProjectile_colAccum(zEntProjectile* prj, _xVec3* dir_away)
{
    float mag_hit = 0.0f;
    _xVec3 vec_accum = {};
    signed int hit_something = 0;
    _xCollis* colrec;
    signed int i;

    if (prj->collis == NULL)
    {
        dir_away->x = dir_away->y = dir_away->z = mag_hit;
        return mag_hit;
    }

    if (prj->collis->colls[0].flags & 1)
    {
        hit_something = 1;
        xVec3AddTo(&vec_accum, &prj->collis->colls[0].depen);
    }
    else if (prj->collis->colls[1].flags & 1)
    {
        hit_something = 1;
        xVec3AddTo(&vec_accum, &prj->collis->colls[1].depen);
    }

    for (i = prj->collis->npc_sidx; i < prj->collis->npc_eidx; i++)
    {
        colrec = &prj->collis->colls[i];
        if ((colrec->flags & 1) && !(colrec->flags & 2))
        {
            hit_something++;
            xVec3AddTo(&vec_accum, &colrec->depen);
        }
    }

    for (i = prj->collis->env_sidx; i < prj->collis->env_eidx; i++)
    {
        hit_something++;
        colrec = &prj->collis->colls[i];
        if ((colrec->flags & 1) && !(colrec->flags & 2))
        {
            xVec3AddTo(&vec_accum, &colrec->depen);
        }
    }

    for (i = prj->collis->stat_sidx; i < prj->collis->stat_eidx; i++)
    {
        colrec = &prj->collis->colls[i];
        if ((colrec->flags & 1) && !(colrec->flags & 2))
        {
            hit_something++;
            xVec3AddTo(&vec_accum, &colrec->depen);
        }
    }

    for (i = prj->collis->dyn_sidx; i <= prj->collis->dyn_eidx; i++)
    {
        colrec = &prj->collis->colls[i];
        if ((colrec->flags & 1) && !(colrec->flags & 2))
        {
            hit_something++;
            xVec3AddTo(&vec_accum, &colrec->depen);
        }
    }

    if ((float)__fabs(vec_accum.x) < 0.00001f)
    {
        vec_accum.x = 0.0f;
    }
    if ((float)__fabs(vec_accum.y) < 0.00001f)
    {
        vec_accum.y = 0.0f;
    }
    if ((float)__fabs(vec_accum.z) < 0.00001f)
    {
        vec_accum.z = 0.0f;
    }

    if (hit_something)
    {
        mag_hit = xVec3Normalize(dir_away, &vec_accum);
    }
    else
    {
        xVec3SMulBy(dir_away, 0.0f);
    }
    return mag_hit;
}

#pragma auto_inline on
void zEntProjectile_softReflect(_xVec3* dir_new, _xVec3* dir_cur, _xVec3* oppose, float scaleMag)
{
    if (scaleMag != 0.0f && scaleMag != 1.0f)
    {
        xVec3SMul(dir_new, dir_cur, scaleMag);
    }
    else
    {
        xVec3Copy(dir_new, dir_cur);
    }

    if (oppose->x < 0.0f && dir_cur->x > 0.0f)
    {
        dir_new->x *= -1.0f;
    }
    else if (oppose->x > 0.0f && dir_cur->x < 0.0f)
    {
        dir_new->x *= -1.0f;
    }

    if (oppose->y < 0.0f && dir_cur->y > 0.0f)
    {
        dir_new->y *= -1.0f;
    }
    else if (oppose->y > 0.0f && dir_cur->y < 0.0f)
    {
        dir_new->y *= -1.0f;
    }

    if (oppose->z < 0.0f && dir_cur->z > 0.0f)
    {
        dir_new->z *= -1.0f;
    }
    else if (oppose->z > 0.0f && dir_cur->z < 0.0f)
    {
        dir_new->z *= -1.0f;
    }
}
#pragma auto_inline reset

signed int zEntProjectile_gen_aimer(zEntProjectile* prj, ST_CAT_AIMDATA* info, signed int apply)
{
    signed int didit;

    switch (info->idx_caim)
    {
    case ZPRJ_CAIM_ARROW:
    {
        st_CAT_AIMARROW* arrow = &info->arrow;
        xVec3Sub(&info->dir_base, &arrow->pos_tgt, &arrow->pos_src);
        xVec3Normalize(&info->dir_base, &info->dir_base);
        if (apply)
        {
            xVec3Copy(&prj->frame->mat.pos, &arrow->pos_src);
            xVec3SMul(&prj->frame->vel, &info->dir_base, info->spd_base);
            prj->frame->mode |= 5;
            prj->SyncFrame(1);
        }
        didit = 1;
        break;
    }
    case ZPRJ_CAIM_GRAVBALL:
    {
        st_CAT_AIMGRAV* grav = &info->grav;
        st_ZPRJ_AUTOCALC_INFO autinf = {};
        didit = 0;
        xVec3Sub(&info->dir_base, &grav->pos_tgt, &grav->pos_src);
        autinf.spd_muzzle = info->spd_base;
        autinf.dir_launch = info->dir_base;
        autinf.pos_launch = grav->pos_src;
        autinf.pos_target = grav->pos_tgt;
        autinf.fac_arcness = grav->fac_arcness;
        autinf.acc_gravity = grav->acc_gravity;
        xVec3SMul(&autinf.vel_launch, &autinf.dir_launch, autinf.spd_muzzle);

        switch (autinf.method)
        {
        case 0:
        {
            _xVec3 vec_tgt = {};
            xVec3Sub(&vec_tgt, &autinf.pos_target, &autinf.pos_launch);
            didit = ZPRJ_solv_velvec(autinf.acc_gravity, autinf.fac_arcness, autinf.spd_muzzle,
                                     autinf.dst_range, vec_tgt, &autinf.vel_launch) +
                    1;
            break;
        }
        case 1:
            didit = 0;
            break;
        case 2:
            didit = 0;
            break;
        }

        if (didit)
        {
            info->dir_base = autinf.vel_launch;
            info->spd_base = xVec3Normalize(&info->dir_base, &info->dir_base);
        }

        if (didit && apply)
        {
            prj->cat_data.gravball.acc_gravity = grav->acc_gravity;
            xVec3Copy(&prj->frame->mat.pos, &grav->pos_src);
            xVec3SMul(&prj->frame->vel, &info->dir_base, info->spd_base);
            prj->frame->mode |= 5;
            prj->SyncFrame(1);
        }
        break;
    }
    default:
        didit = 0;
        break;
    }
    return didit;
}

static signed int ZPRJ_solv_velvec(float grav, float arc_factor, float spd_limit,
                                   float dst_range, _xVec3 vec_tgt, _xVec3* vec_vel)
{
    float dst_tgt;
    _xVec3 vec_tgtOrig = {};
    float sol[2] = {};
    _xVec3 vec_tmp = {};
    _xVec3 unused0 = {};
    _xVec3 unused1 = {};
    signed int limit_tally = 0;
    float tym_limit = -1.0f;
    float tym_ariv;
    float b;
    unsigned int solcnt;

    vec_tgtOrig = vec_tgt;
    dst_tgt = xsqrt(vec_tgt.x * vec_tgt.x + vec_tgt.z * vec_tgt.z);

    if (dst_range > 0.0f && dst_tgt > dst_range)
    {
        limit_tally = 1;
        vec_tgtOrig = vec_tgt;
        vec_tgtOrig.y = 0.0f;
        xVec3Normalize(&vec_tgtOrig, &vec_tgtOrig);
        xVec3SMulBy(&vec_tgtOrig, dst_range);
        vec_tgtOrig.y = vec_tgt.y;
    }

    if (dst_range > 0.0f)
    {
        tym_limit = dst_range / spd_limit;
    }

    float a = -0.5f * grav;
    b = xsqrt(2.0f * grav * (1.5f * arc_factor));
    float c = -vec_tgtOrig.y;

    vec_vel->x = 0.0f;
    vec_vel->y = 0.0f;
    vec_vel->z = 0.0f;

    solcnt = xMathSolveQuadratic(a, b, c, &sol[0], &sol[1]);
    if (solcnt == 2)
    {
        tym_ariv = sol[0] > sol[1] ? sol[0] : sol[1];
    }
    else if (solcnt == 1)
    {
        tym_ariv = sol[0];
    }
    else
    {
        tym_ariv = 1.0f;
        limit_tally++;
    }

    if (tym_ariv <= 0.0f)
    {
        limit_tally++;
        tym_ariv = tym_limit <= 0.0f ? 1.0f : tym_limit;
    }

    vec_tmp.x = vec_tgtOrig.x / tym_ariv;
    vec_tmp.y = 0.0f;
    vec_tmp.z = vec_tgtOrig.z / tym_ariv;
    if (vec_tmp.x * vec_tmp.x + vec_tmp.z * vec_tmp.z > spd_limit * spd_limit)
    {
        limit_tally++;
        xVec3Normalize(&vec_tmp, &vec_tmp);
        xVec3SMulBy(&vec_tmp, spd_limit);
    }

    vec_tmp.y = b;
    *vec_vel = vec_tmp;
    return limit_tally;
}
// Stand-in for a function the linker dead-stripped (DoEffects sits here in the
// PS2 build); only its 0.5f constant remains in .sdata2.
float zEntProjectileDeadStrippedConst()
{
    return 0.5f;
}

void zEntProjectile::PEFX_flight(en_ZPRJ_FX_TYPE fxtyp, float dt)
{
    _zParEmitterCustomSettings info;
    _zParEmitter* emit = NULL;

    tmr_emit -= dt;
    if (tmr_emit > 0.0f)
    {
        return;
    }

    tmr_emit = 0.1f;
    info.custom_flags = 0x20;
    info.pos = ((_xMat4x3*)model->Mat)->pos;

    switch (fxtyp)
    {
    case ZPRJ_FXTYPE_SMOKE:
        emit = g_paremit[6];
        break;
    case ZPRJ_FXTYPE_WOOD:
        emit = g_paremit[7];
        break;
    case ZPRJ_FXTYPE_TAR:
        emit = g_paremit[8];
        tmr_emit = 0.0f;
        break;
    case ZPRJ_FXTYPE_ICE:
        emit = g_paremit[9];
        break;
    case ZPRJ_FXTYPE_FIREBALL:
        emit = g_paremit[10];
        tmr_emit = 0.0f;
        break;
    case ZPRJ_FXTYPE_NODAMAGE_FIREBALL:
        emit = g_paremit[11];
        tmr_emit = 0.0f;
        break;
    case ZPRJ_FXTYPE_WITCHDOCTOR:
        emit = g_paremit[12];
        tmr_emit = 0.0f;
        break;
    case ZPRJ_FXTYPE_MASTERMIND_RED:
    case ZPRJ_FXTYPE_MASTERMIND_GREEN:
    case ZPRJ_FXTYPE_MASTERMIND_BLACK:
        emit = g_paremit[13];
        tmr_emit = 0.0f;
        info.pos.y += 0.35f;
        break;
    case ZPRJ_FXTYPE_BLACKKNIGHT:
    case ZPRJ_FXTYPE_REDBEARD:
        emit = NULL;
        break;
    }

    if (emit != NULL)
    {
        zParEmitterEmitCustom(emit, dt, &info);
    }
}
#include "xBase.h"
#include "xMath3.h"
#include "xBound.h"
#include "xEnt.h"
#include "xMemMgr.h"
#include "iMath.h"
#include "zGlobals.h"
#include "zScene.h"

extern "C" void* memset(void* dst, int c, unsigned int n);
extern "C" void* memcpy(void* dst, const void* src, unsigned int n);

#define SCENE_ID(s) ((((s)[0] & ~0x20) << 24) | ((s)[1] << 16) | ((s)[2] << 8) | (s)[3])

class _zMovePoint;
class _xEntCollis;

_zMovePoint* zMovePoint_From_xAssetID(unsigned int aid);
float zMovePointGetNext(_zMovePoint* current, _zMovePoint* prev, _zMovePoint** next,
                        _xVec3* hdng);
_xVec3* zMovePointGetPos(_zMovePoint* m);
signed int xUtil_yesno();
signed int xUtil_yesno(float wt_yes);
signed int zEntPlayer_MoveInfo();
void zEntPlayer_Damage(xBase* src, unsigned int damage);
void xMat3x3Euler(_xMat3x3* m, _xVec3* ypr);
float xVec3Normalize(_xVec3* o, _xVec3* v);
void xMat4x3RMul3x3(_xMat4x3* o, _xMat3x3* a, _xMat4x3* b);
char* xSceneID2Name(_xScene* sc, unsigned int id);

extern const _xVec3 g_Y3;
extern unsigned int gTransitionSceneID;

enum en_PROJECTILE_EVENT
{
    ZPRJ_EVNT_ACTIVE,
    ZPRJ_EVNT_TRANSTATE,
    ZPRJ_EVNT_NEWTIME,
    ZPRJ_EVNT_ENDTIME,
    ZPRJ_EVNT_ABORT,
    ZPRJ_EVNT_HITPLYR,
    ZPRJ_EVNT_LAUNCH,
    ZPRJ_EVNT_INFLIGHT,
    ZPRJ_EVNT_ATREST,
    ZPRJ_EVNT_HITSTUFF,
    ZPRJ_EVNT_DETONATE,
    ZPRJ_EVNT_SELFDESTRUCT,
    ZPRJ_EVNT_DEATH,
    ZPRJ_EVNT_STARTRESTING,
    ZPRJ_EVNT_BOUNCE,
    ZPRJ_EVNT_VIL_BUILDUP,
    ZPRJ_EVNT_VIL_AFTERMATH,
    ZPRJ_EVNT_NOMORE
};

enum en_ZPRJ_STATES
{
    ZPRJ_STATE_UNUSED,
    ZPRJ_STATE_STANDBY,
    ZPRJ_STATE_ACTIVE,
    ZPRJ_STATE_DEAD
};

enum en_ZPRJ_CATEGORY
{
    ZPRJ_CAT_UNKNOWN,
    ZPRJ_CAT_ARROW,
    ZPRJ_CAT_GRAVBALL,
    ZPRJ_CAT_BOUNCEBALL,
    ZPRJ_CAT_BUBSOAP,
    ZPRJ_CAT_BUBGUM,
    ZPRJ_CAT_BKAXE,
    ZPRJ_CAT_GGSHOCK,
    ZPRJ_CAT_RBSWORD,
    ZPRJ_CAT_MMFLASK,
    ZPRJ_CAT_PICKUP,
    ZPRJ_CAT_STDDEBRIS,
    ZPRJ_CAT_BANGDEBRIS,
    ZPRJ_CAT_NOMORE
};

enum en_ZPRJ_CATAIM_FORM
{
    ZPRJ_CAIM_ARROW,
    ZPRJ_CAIM_GRAVBALL
};

class st_CAT_AIMARROW
{
public:
    _xVec3 pos_src; // 0x0
    _xVec3 pos_tgt; // 0xC
};

class st_CAT_AIMGRAV
{
public:
    _xVec3 pos_src;    // 0x0
    _xVec3 pos_tgt;    // 0xC
    float fac_arcness; // 0x18
    float acc_gravity; // 0x1C
};

class ST_CAT_AIMDATA
{
public:
    en_ZPRJ_CATAIM_FORM idx_caim; // 0x0
    float spd_base;               // 0x4
    _xVec3 dir_base;              // 0x8
    union
    {
        st_CAT_AIMARROW arrow; // 0x14
        st_CAT_AIMGRAV grav;   // 0x14
    };
};

class st_ZPRJ_CAT_GRAVBALL
{
public:
    float acc_gravity; // 0x0
};

class st_ZPRJ_CAT_BOUNCEBALL : public st_ZPRJ_CAT_GRAVBALL
{
public:
    signed int atRest;           // 0x4
    float tym_atrest;            // 0x8
    signed int cnt_curBounce;    // 0xC
    signed int cnt_maxBounce;    // 0x10
    float fac_elastic;           // 0x14
    float pct_randKick;          // 0x18
    signed int skip_next_bounce; // 0x1C
};

class st_ZPRJ_CATDATA
{
public:
    signed int idx_cdat; // 0x0
    union
    {
        st_ZPRJ_CAT_GRAVBALL gravball;
        st_ZPRJ_CAT_BOUNCEBALL bounce; // 0x4
    };
};

class st_ZPRJ_COMMON
{
public:
    _xVec3 vec_flex;   // 0x0
    _xMat3x3 mat_flex; // 0xC
};

class zEntProjectile;

class st_ZPRJ_CATEGORY_FUNCS
{
public:
    signed int flg_cback;                                           // 0x0
    signed int (*attach)(zEntProjectile*);                          // 0x4
    void (*detach)(zEntProjectile*);                                // 0x8
    signed int (*logix)(zEntProjectile*, float, zScene*, _xEntFrame*);    // 0xC
    signed int (*calcmove)(zEntProjectile*, float, zScene*, _xEntFrame*); // 0x10
    signed int (*notify)(zEntProjectile*, en_PROJECTILE_EVENT, void*); // 0x14
    signed int (*render)(zEntProjectile*);                          // 0x18
    signed int (*aimer)(zEntProjectile*, ST_CAT_AIMDATA*, signed int); // 0x1C
    signed int (*collide)(zEntProjectile*, zScene*, float);         // 0x20
    void (*dbgUpdate)(zEntProjectile*, signed int);                 // 0x24
};

class st_ZPRJ_CATEGORY_PROPS
{
public:
    signed int flg_props;      // 0x0
    unsigned char flg_physics; // 0x4
    unsigned char flg_chk;     // 0x5
    unsigned char flg_pen;     // 0x6
    unsigned char pad;         // 0x7
    signed int flg_categ;      // 0x8
};

class zEntProjectile : public _xEnt
{
public:
    signed int flg_prop;     // 0xE8
    signed int flg_categ;    // 0xEC
    unsigned char flg_chk;   // 0xF0
    unsigned char flg_pen;   // 0xF1
    unsigned char pad[2];    // 0xF2
    void* owner;             // 0xF4
    void* ownerContext;      // 0xF8
    signed int (*ownerNotify)(zEntProjectile*, en_PROJECTILE_EVENT, void*); // 0xFC
    signed int (*ownerDoCollide)(zEntProjectile*, zScene*, float);          // 0x100
    float tmr_alive;         // 0x104
    float tmr_curstate;      // 0x108
    _xVec3 pos_home;         // 0x10C
    float tym_maxlife;       // 0x118
    float ds2_maxlife;       // 0x11C
    st_ZPRJ_COMMON common;   // 0x120
    st_ZPRJ_CATDATA cat_data; // 0x15C
    void* xtradata;          // 0x180
    _xCollis* plyr_colrec;   // 0x184

    signed int AimAtTarget(ST_CAT_AIMDATA* info, signed int apply);
    void FlexMatSet(_xVec3* rot_euler, float dt);
    void FlexVecSet(_xVec3* vec_flex);
    signed int SendEvent(en_PROJECTILE_EVENT event, float dt);
    void SyncFrame(signed int applyToModel);
    void ColDelaySet(float tym, signed int addto);
    en_ZPRJ_STATES StateSet(en_ZPRJ_STATES state);
    void OrientSet(signed int orient);
};

zEntProjectile* zEntProjectile_acquire(en_ZPRJ_CATEGORY cat, unsigned int ptypeID);
void zEntProjectile_softReflect(_xVec3* dir_new, _xVec3* dir_cur, _xVec3* oppose, float scaleMag);
float zEntProjectile_colAccum(zEntProjectile* prj, _xVec3* dir_away);
signed int zEntProjectile_gen_render(zEntProjectile* prj);
signed int zEntProjectile_addProps(en_ZPRJ_CATEGORY cat, st_ZPRJ_CATEGORY_PROPS* props);
signed int zEntProjectile_addCat(en_ZPRJ_CATEGORY cat, st_ZPRJ_CATEGORY_FUNCS* funcs);

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
};

class zLobMasterAsset : public p2BaseAsset
{
public:
    signed int lobMasterType;      // 0x8
    unsigned int projectileTypeID; // 0xC
    _xVec3 pos_launch;             // 0x10
    _xVec3 rot_launch;             // 0x1C
    float spd_launch;              // 0x28
    float spd_randPct;             // 0x2C
    _xVec3 scl_model;              // 0x30
    signed int flg_enablers;       // 0x3C
    float tym_maxlife;             // 0x40
    float dst_maxdist;             // 0x44
    unsigned int aid_mvptID;       // 0x48
    signed int cnt_salvo;          // 0x4C
    signed int cnt_ammo;           // 0x50
    float fac_arccoeff;            // 0x54
    signed int ang_debrisCone;     // 0x58
    signed int numBounce;          // 0x5C
    signed int typ_powerup;        // 0x60
    float heavyFactor;             // 0x64
    _xVec3 tumbleRotation;         // 0x68
    float collideDelay;            // 0x74
    float atRestPeriod;            // 0x78
    unsigned int mode;             // 0x7C
};

enum en_LOBMASTER_PARAMID
{
    ZLOB_PARM_RAWASS,
    ZLOB_PARM_LOBFLAGS,
    ZLOB_PARM_LOBMASTERTYPE,
    ZLOB_PARM_PRJTYPEID,
    ZLOB_PARM_POS_LAUNCH,
    ZLOB_PARM_DIR_LAUNCH,
    ZLOB_PARM_SPD_LAUNCH,
    ZLOB_PARM_SPD_RANDPCT,
    ZLOB_PARM_SCL_MODEL,
    ZLOB_PARM_TYM_MAXLIFE,
    ZLOB_PARM_DST_MAXDIST,
    ZLOB_PARM_DEBRISANGLE,
    ZLOB_PARM_SALVO,
    ZLOB_PARM_AMMO,
    ZLOB_PARM_ARCFACTOR,
    ZLOB_PARM_HEAVYFACTOR,
    ZLOB_PARM_PAYLOAD,
    ZLOB_PARM_MVPTBULL,
    ZLOB_PARM_NUMBOUNCE,
    ZLOB_PARM_TUMBLE,
    ZLOB_PARM_COLLIDEDELAY,
    ZLOB_PARM_RESTPERIOD,
    ZLOB_PARM_NOMORE
};

enum en_LOB_MASTER_TYPE
{
    LOBMASTER_CUSTOM,
    LOBMASTER_ARROW,
    LOBMASTER_DEBRIS,
    LOBMASTER_PICKUP,
    LOBMASTER_NOMORE
};

class st_LOBMASTER_CONFIG
{
public:
    zLobMasterAsset* rawass; // 0x0
    _xVec3 dir_launch;       // 0x4
};

class st_LOBMASTER_DATA : public xBase
{
public:
    signed int flg_lmopts;     // 0x10
    float ang_debrisCone;      // 0x14
    signed int cnt_ammo;       // 0x18
    signed int cnt_salvo;      // 0x1C
    _zMovePoint* nav_bullbase; // 0x20
    _zMovePoint* nav_nextbull; // 0x24
    _zMovePoint* nav_prevbull; // 0x28
    signed int flg_suitcase;   // 0x2C
    _xVec3 pos_altLaunch;      // 0x30
    _xVec3 dir_altLaunch;      // 0x3C
    _xBound bnd_altLaunch;     // 0x48
    st_LOBMASTER_CONFIG cfg;   // 0x80
};

class st_LOBMASTER_DEPOT
{
public:
    st_LOBMASTER_DATA* raw_pool; // 0x0
    signed int raw_cnt;          // 0x4
};

class st_LOBMASTER_QUEUE
{
public:
    signed int flg_stat;     // 0x0
    float tmr_delay;         // 0x4
    unsigned int aid_lobber; // 0x8
    _xVec3 pos_launch;       // 0xC
    _xVec3 dir_launch;       // 0x18
    signed int cnt_salvo;    // 0x24
    signed int idx_current;  // 0x28
};

static signed int g_zlob_init;
static st_LOBMASTER_DEPOT g_zlob_depot = { 0, 0 };
st_LOBMASTER_QUEUE g_zlob_queue[5] = {};

static signed int ZLOB_elcb_event(xBase* from, xBase* xb, unsigned int toEvent, float* toParam,
                                  xBase* toParamWidget);
static signed int LCFG_getParam(st_LOBMASTER_CONFIG* cfg, en_LOBMASTER_PARAMID pid, void* vv);
static signed int ZLOB_legacy_notify(zEntProjectile* prj, en_PROJECTILE_EVENT event, void* data);
void zLobMaster_Init(st_LOBMASTER_DATA* lob, p2BaseAsset* basset);
void zLobMaster_InitDep(st_LOBMASTER_DATA* lob, zScene* scene);
static signed int LPRJ_debris_prjmove(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame);
static signed int LPRJ_debris_logix(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame);
static signed int LPRJ_debris_attach(zEntProjectile* prj);
static signed int ZLOB_randpos_fromBound(_xBound* bnd, signed int nth, signed int total,
                                         signed int flg_ropts, _xVec3* pos_launch);
static st_LOBMASTER_DATA* ZLOB_find_lobber(unsigned int aid_lobm);
static void ZLOB_grab_nextBull(st_LOBMASTER_DATA* lob);
static signed int ZLOB_shoot_debris(st_LOBMASTER_DATA* lob, signed int nth, signed int total);
static signed int ZLOB_shoot_arrow(st_LOBMASTER_DATA* lob, signed int nth, signed int total);
static signed int ZLOB_shoot_legacy(st_LOBMASTER_DATA* lob, signed int nth, signed int total);
static signed int ZLOB_shoot_one(st_LOBMASTER_DATA* lob, signed int nth, signed int total);
static signed int ZLOB_doLaunch(st_LOBMASTER_DATA* lob);
static void ZLOB_HEB_to_DIR(_xVec3* dir_result, _xVec3* rot_HPB);

#pragma fp_contract off
static inline float xVec3Length2(const _xVec3* v)
{
    return v->x * v->x + v->y * v->y + v->z * v->z;
}
#pragma fp_contract on

#pragma auto_inline on

// Functions are in reverse DOL order (-inline deferred emits last to first).

static inline void ZLOB_setPayload(st_LOBMASTER_DATA* lob, zEntProjectile* prj)
{
    signed int payload = 0;
    LCFG_getParam(&lob->cfg, ZLOB_PARM_PAYLOAD, &payload);
    if (payload == 1)
    {
        prj->flg_prop |= 0x10;
    }
    else
    {
        prj->flg_prop &= ~0x10;
    }
}

static inline void ZLOB_fadeSetup(_xEnt* ent)
{
    ent->flags |= 4;
    ent->fade_tmr = 1.25f;
    if ((gTransitionSceneID ^ 0x20000000) == SCENE_ID("o004") &&
        (ent->id == 0xF933F89E || ent->id == 0xFFF2B7AA))
    {
        ent->fade_tmr = 0.25f;
    }
}

void zLobMaster_Startup()
{
    st_ZPRJ_CATEGORY_FUNCS fmap_debris = {
        1, LPRJ_debris_attach, 0, LPRJ_debris_logix, LPRJ_debris_prjmove, 0,
        zEntProjectile_gen_render, 0, 0, 0
    };
    st_ZPRJ_CATEGORY_PROPS prop_debris = { 1, 3, 0x20, 0x20, 0, 0 };
    st_ZPRJ_CATEGORY_PROPS prop_bang = { 1, 3, 0x3E, 0x7E, 0, 0 };

    zEntProjectile_addCat(ZPRJ_CAT_STDDEBRIS, &fmap_debris);
    zEntProjectile_addProps(ZPRJ_CAT_STDDEBRIS, &prop_debris);
    zEntProjectile_addCat(ZPRJ_CAT_BANGDEBRIS, &fmap_debris);
    zEntProjectile_addProps(ZPRJ_CAT_BANGDEBRIS, &prop_bang);
}

void zLobMaster_Shutdown()
{
}

void zLobMaster_scenePrepare()
{
    st_LOBMASTER_DEPOT* depot = &g_zlob_depot;

    memset(g_zlob_queue, 0, sizeof(g_zlob_queue));
    if (g_zlob_init++ == 0)
    {
        memset(depot, 0, sizeof(st_LOBMASTER_DEPOT));
    }
}

void zLobMaster_sceneFinish()
{
    st_LOBMASTER_DEPOT* depot = &g_zlob_depot;

    if (--g_zlob_init == 0)
    {
        memset(depot, 0, sizeof(st_LOBMASTER_DEPOT));
    }
}

st_LOBMASTER_DATA* zLobMaster_memPool(signed int cnt)
{
    st_LOBMASTER_DATA* pool;
    st_LOBMASTER_DEPOT* depot = &g_zlob_depot;

    if (cnt < 1)
    {
        return 0;
    }

    pool = (st_LOBMASTER_DATA*)xMemAlloc(gActiveHeap, cnt * sizeof(st_LOBMASTER_DATA));
    memset(pool, 0, cnt * 0x90);
    depot->raw_pool = pool;
    depot->raw_cnt = cnt;
    return pool;
}

st_LOBMASTER_DATA* zLobMaster_getInst(st_LOBMASTER_DATA* pool, signed int idx)
{
    return &pool[idx];
}

void zLobMaster_Init(st_LOBMASTER_DATA* lob, p2BaseAsset* basset)
{
    zLobMasterAsset* tmp_rawass = (zLobMasterAsset*)basset;

    xBaseInit(lob, basset);
    lob->eventFunc = ZLOB_elcb_event;
    if (lob->linkCount)
    {
        lob->link = (p2LinkAsset*)(tmp_rawass + 1);
    }
    else
    {
        lob->link = 0;
    }

    _xVec3 rot = { 0.0f, 0.0f, 0.0f };
    lob->cfg.rawass = tmp_rawass;
    xVec3SMul(&rot, &tmp_rawass->rot_launch, 0.017453292f);
    ZLOB_HEB_to_DIR(&lob->cfg.dir_launch, &rot);
    xSceneID2Name(globals.sceneCur, lob->id);
}

void zLobMaster_InitDep(st_LOBMASTER_DATA* lob, zScene*)
{
    LCFG_getParam(&lob->cfg, ZLOB_PARM_LOBFLAGS, &lob->flg_lmopts);
    LCFG_getParam(&lob->cfg, ZLOB_PARM_DEBRISANGLE, &lob->ang_debrisCone);
    LCFG_getParam(&lob->cfg, ZLOB_PARM_MVPTBULL, &lob->nav_bullbase);
    LCFG_getParam(&lob->cfg, ZLOB_PARM_AMMO, &lob->cnt_ammo);
    if (lob->nav_bullbase)
    {
        ZLOB_grab_nextBull(lob);
    }
    xSceneID2Name(globals.sceneCur, lob->id);
}

void zLobMaster_timestep(zScene*, float dt)
{
    st_LOBMASTER_DATA* lobber;

    for (signed int i = 0; i < 5; i++)
    {
        st_LOBMASTER_QUEUE* q = &g_zlob_queue[i];
        if (!(q->flg_stat & 1))
        {
            continue;
        }

        q->tmr_delay = (-1.0f > q->tmr_delay - dt) ? -1.0f : q->tmr_delay - dt;
        if (q->tmr_delay > 0.0f)
        {
            continue;
        }

        lobber = ZLOB_find_lobber(q->aid_lobber);
        if (lobber == 0)
        {
            memset(lobber, 0, sizeof(st_LOBMASTER_QUEUE));
            continue;
        }

        lobber->pos_altLaunch = q->pos_launch;
        lobber->flg_suitcase |= 1;
        lobber->dir_altLaunch = q->dir_launch;
        lobber->flg_suitcase |= 2;
        q->idx_current = q->cnt_salvo;
        if (q->idx_current >= q->cnt_salvo)
        {
            memset(q, 0, sizeof(st_LOBMASTER_QUEUE));
        }
    }
}

static void ZLOB_HEB_to_DIR(_xVec3* dir_result, _xVec3* rot_HPB)
{
    _xVec3 vec_aim = { 0.0f, 0.0f, 1.0f };
    _xMat3x3 mat_rot = {};

    if (rot_HPB->z)
    {
        xMat3x3Euler(&mat_rot, 0.0f, 0.0f, rot_HPB->z);
        xMat3x3LMulVec(&vec_aim, &mat_rot, &vec_aim);
    }
    xMat3x3Euler(&mat_rot, 0.0f, rot_HPB->y, 0.0f);
    xMat3x3LMulVec(&vec_aim, &mat_rot, &vec_aim);
    xMat3x3Euler(&mat_rot, rot_HPB->x, 0.0f, 0.0f);
    xMat3x3LMulVec(&vec_aim, &mat_rot, &vec_aim);
    xVec3Normalize(dir_result, &vec_aim);
}

static signed int LCFG_getParam(st_LOBMASTER_CONFIG* cfg, en_LOBMASTER_PARAMID pid, void* vv)
{
    signed int found = 1;
    zLobMasterAsset* lmass = cfg->rawass;
    signed int iv;

    switch (pid)
    {
    case ZLOB_PARM_RAWASS:
        *(zLobMasterAsset**)vv = lmass;
        break;
    case ZLOB_PARM_LOBFLAGS:
        iv = 0;
        if (lmass->flg_enablers & 1)
        {
            iv |= 1;
        }
        if (lmass->flg_enablers & 2)
        {
            iv |= 4;
        }
        if (lmass->flg_enablers & 4)
        {
            iv |= 8;
        }
        if (lmass->flg_enablers & 8)
        {
            iv |= 0x10;
        }
        if (lmass->flg_enablers & 0x10)
        {
            iv |= 0x20;
        }
        if (lmass->flg_enablers & 0x20)
        {
            iv |= 0x40;
        }
        if (lmass->flg_enablers & 0x40)
        {
            iv |= 0x80;
        }
        if (lmass->cnt_ammo > 0)
        {
            iv |= 2;
        }
        *(signed int*)vv = iv;
        break;
    case ZLOB_PARM_LOBMASTERTYPE:
        switch (lmass->typ_powerup)
        {
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            *(signed int*)vv = LOBMASTER_PICKUP;
            break;
        default:
            *(signed int*)vv = lmass->mode;
            break;
        }
        break;
    case ZLOB_PARM_PRJTYPEID:
        *(unsigned int*)vv = lmass->projectileTypeID;
        break;
    case ZLOB_PARM_POS_LAUNCH:
        xVec3Copy((_xVec3*)vv, &lmass->pos_launch);
        break;
    case ZLOB_PARM_DIR_LAUNCH:
        xVec3Copy((_xVec3*)vv, &cfg->dir_launch);
        break;
    case ZLOB_PARM_SPD_LAUNCH:
        *(float*)vv = lmass->spd_launch;
        break;
    case ZLOB_PARM_SPD_RANDPCT:
        *(float*)vv = 0.01f * lmass->spd_randPct;
        break;
    case ZLOB_PARM_SCL_MODEL:
        xVec3Copy((_xVec3*)vv, &lmass->scl_model);
        break;
    case ZLOB_PARM_TYM_MAXLIFE:
        if (lmass->flg_enablers & 1)
        {
            *(float*)vv = lmass->tym_maxlife;
        }
        else
        {
            *(float*)vv = 10.0f;
        }
        break;
    case ZLOB_PARM_DST_MAXDIST:
        if (lmass->flg_enablers & 1)
        {
            *(float*)vv = lmass->dst_maxdist;
        }
        else
        {
            *(float*)vv = 15.0f;
        }
        break;
    case ZLOB_PARM_DEBRISANGLE:
        *(float*)vv = (3.1415927f * lmass->ang_debrisCone) / 180.0f;
        break;
    case ZLOB_PARM_NUMBOUNCE:
        *(signed int*)vv = lmass->numBounce;
        break;
    case ZLOB_PARM_SALVO:
        *(signed int*)vv = lmass->cnt_salvo;
        break;
    case ZLOB_PARM_AMMO:
        *(signed int*)vv = lmass->cnt_ammo;
        break;
    case ZLOB_PARM_ARCFACTOR:
        *(float*)vv = lmass->fac_arccoeff;
        break;
    case ZLOB_PARM_HEAVYFACTOR:
        *(float*)vv = lmass->heavyFactor;
        break;
    case ZLOB_PARM_PAYLOAD:
        *(signed int*)vv = lmass->typ_powerup;
        break;
    case ZLOB_PARM_MVPTBULL:
        *(_zMovePoint**)vv = zMovePoint_From_xAssetID(lmass->aid_mvptID);
        break;
    case ZLOB_PARM_TUMBLE:
        ((_xVec3*)vv)->x = (3.1415927f * lmass->tumbleRotation.x) / 180.0f;
        ((_xVec3*)vv)->y = (3.1415927f * lmass->tumbleRotation.y) / 180.0f;
        ((_xVec3*)vv)->z = (3.1415927f * lmass->tumbleRotation.z) / 180.0f;
        break;
    case ZLOB_PARM_COLLIDEDELAY:
        *(float*)vv = lmass->collideDelay;
        break;
    case ZLOB_PARM_RESTPERIOD:
        *(float*)vv = lmass->atRestPeriod;
        break;
    default:
        found = 0;
        break;
    }

    return found;
}

// TODO: 99.8%: stack slots of the inlined helpers' locals (nav_tmp/hdng/vec_aim/rot) are
// assigned in a different order, and basset lands in r28 instead of r29.
static signed int ZLOB_elcb_event(xBase* from, xBase* xb, unsigned int toEvent, float* toParam,
                                  xBase* toParamWidget)
{
    st_LOBMASTER_DATA* lob = (st_LOBMASTER_DATA*)xb;
    signed int do_fromWidge = 0;

    switch (toEvent)
    {
    case 10:
    {
        p2BaseAsset* basset = 0;
        LCFG_getParam(&lob->cfg, ZLOB_PARM_RAWASS, &basset);
        xBaseReset(lob, basset);
        memset(&lob->flg_lmopts, 0, sizeof(st_LOBMASTER_DATA) - sizeof(xBase));
        zLobMaster_Init(lob, basset);
        zLobMaster_InitDep(lob, globals.sceneCur);
        break;
    }
    case 0x5C:
        LCFG_getParam(&lob->cfg, ZLOB_PARM_AMMO, &lob->cnt_ammo);
        break;
    case 0x5B:
        ZLOB_doLaunch(lob);
        break;
    case 0xA7:
        switch (from->baseType)
        {
        case 2:
        case 3:
        case 4:
        case 6:
        case 8:
        case 11:
        case 12:
        case 18:
        case 23:
        case 24:
        case 27:
            lob->pos_altLaunch = *(_xVec3*)&((_xEnt*)from)->model->Mat->pos;
            lob->flg_suitcase = 0;
            lob->flg_suitcase |= 1;
            memcpy(&lob->bnd_altLaunch, &((_xEnt*)from)->bound, sizeof(_xBound));
            do_fromWidge = 1;
            lob->flg_suitcase |= 4;
            break;
        case 13:
            lob->pos_altLaunch = *zMovePointGetPos((_zMovePoint*)from);
            do_fromWidge = 1;
            lob->flg_suitcase = 0;
            lob->flg_suitcase |= 1;
            break;
        case 35:
            LCFG_getParam(&((st_LOBMASTER_DATA*)from)->cfg, ZLOB_PARM_POS_LAUNCH,
                          &lob->pos_altLaunch);
            do_fromWidge = 1;
            lob->flg_suitcase = 0;
            lob->flg_suitcase |= 1;
            break;
        case 1:
        case 5:
        case 7:
        case 9:
        case 10:
        case 14:
        case 15:
        case 16:
        case 17:
        case 19:
        case 20:
        case 21:
        case 22:
        case 25:
        case 26:
        case 28:
        case 29:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 36:
        case 37:
            xSceneID2Name(globals.sceneCur, from->id);
            break;
        default:
            xSceneID2Name(globals.sceneCur, from->id);
            break;
        }
        if (do_fromWidge)
        {
            ZLOB_doLaunch(lob);
        }
        lob->flg_suitcase = 0;
        break;
    case 0x57:
    case 0x58:
    case 0x59:
    case 0x5A:
        break;
    default:
        xSceneID2Name(globals.sceneCur, xb->id);
        break;
    }

    return 1;
}

static signed int ZLOB_doLaunch(st_LOBMASTER_DATA* lob)
{
    signed int salvo = 0;
    en_LOB_MASTER_TYPE type = LOBMASTER_CUSTOM;
    signed int i;

    LCFG_getParam(&lob->cfg, ZLOB_PARM_LOBMASTERTYPE, &type);
    LCFG_getParam(&lob->cfg, ZLOB_PARM_SALVO, &salvo);
    if (type == LOBMASTER_DEBRIS)
    {
        salvo = (salvo < 10) ? salvo : 10;
    }

    for (i = 0; i < salvo; i++)
    {
        if ((lob->flg_lmopts & 2) && lob->cnt_ammo-- < 0)
        {
            break;
        }
        ZLOB_shoot_one(lob, i, salvo);
    }

    ZLOB_grab_nextBull(lob);
    return 1;
}

static signed int ZLOB_shoot_one(st_LOBMASTER_DATA* lob, signed int nth, signed int total)
{
    en_LOB_MASTER_TYPE type = LOBMASTER_CUSTOM;

    LCFG_getParam(&lob->cfg, ZLOB_PARM_LOBMASTERTYPE, &type);
    switch (type)
    {
    case LOBMASTER_CUSTOM:
        ZLOB_shoot_legacy(lob, nth, total);
        break;
    case LOBMASTER_ARROW:
        ZLOB_shoot_arrow(lob, nth, total);
        break;
    case LOBMASTER_DEBRIS:
        ZLOB_shoot_debris(lob, nth, total);
        break;
    case LOBMASTER_PICKUP:
        break;
    default:
        ZLOB_shoot_legacy(lob, nth, total);
        break;
    }

    return 1;
}

static signed int ZLOB_legacy_notify(zEntProjectile* prj, en_PROJECTILE_EVENT event, void* data)
{
    st_LOBMASTER_DATA* lob = (st_LOBMASTER_DATA*)prj->owner;
    float fval = 0.0f;

    switch (event)
    {
    case ZPRJ_EVNT_NEWTIME:
        if (lob->flg_lmopts & 0x20)
        {
            LCFG_getParam(&lob->cfg, ZLOB_PARM_COLLIDEDELAY, &fval);
            if (prj->tmr_alive > fval)
            {
                prj->flg_chk |= 0x10;
                prj->flg_pen |= 0x10;
            }
        }
        break;
    }

    return 1;
}

static signed int ZLOB_shoot_legacy(st_LOBMASTER_DATA* lob, signed int nth, signed int total)
{
    zEntProjectile* prj;
    unsigned int prjTypeID = 0;
    float fval = 0.0f;
    st_ZPRJ_CAT_BOUNCEBALL* bounce;

    LCFG_getParam(&lob->cfg, ZLOB_PARM_PRJTYPEID, &prjTypeID);
    prj = zEntProjectile_acquire(ZPRJ_CAT_BOUNCEBALL, prjTypeID);
    if (prj == 0)
    {
        return 0;
    }

    prj->owner = lob;
    LCFG_getParam(&lob->cfg, ZLOB_PARM_TYM_MAXLIFE, &prj->tym_maxlife);
    LCFG_getParam(&lob->cfg, ZLOB_PARM_DST_MAXDIST, &prj->ds2_maxlife);
    prj->ds2_maxlife = prj->ds2_maxlife * prj->ds2_maxlife;
    if (lob->flg_lmopts & 0x10)
    {
        prj->flg_prop |= 2;
    }

    ZLOB_setPayload(lob, prj);

    bounce = &prj->cat_data.bounce;
    prj->ownerNotify = ZLOB_legacy_notify;
    LCFG_getParam(&lob->cfg, ZLOB_PARM_HEAVYFACTOR, &fval);
    if (fval > 0.0f)
    {
        bounce->acc_gravity = fval;
    }
    else
    {
        bounce->acc_gravity = 1.5f;
    }

    if (lob->flg_lmopts & 8)
    {
        prj->flg_categ |= 0x2000000;
        LCFG_getParam(&lob->cfg, ZLOB_PARM_NUMBOUNCE, &bounce->cnt_maxBounce);
        bounce->cnt_curBounce = 0;
        bounce->fac_elastic = 0.8f;
        bounce->pct_randKick = 0.0f;
    }
    else
    {
        prj->flg_categ &= ~0x2000000;
    }

    if (lob->flg_lmopts & 0x40)
    {
        prj->flg_categ |= 0x1000000;
        LCFG_getParam(&lob->cfg, ZLOB_PARM_RESTPERIOD, &bounce->tym_atrest);
    }
    else
    {
        prj->flg_categ &= ~0x1000000;
    }

    _xVec3 rot_tumble = { 0.0f, 0.0f, 0.0f };
    LCFG_getParam(&lob->cfg, ZLOB_PARM_TUMBLE, &rot_tumble);
    if (xVec3Length2(&rot_tumble))
    {
        prj->flg_categ |= 0x4000000;
        prj->OrientSet(0);
        prj->FlexVecSet(&rot_tumble);
        prj->FlexMatSet(&rot_tumble, globals.update_dt);
    }
    else
    {
        prj->OrientSet(0x30000000);
        prj->flg_categ &= ~0x4000000;
    }

    if (lob->flg_lmopts & 0x20)
    {
        LCFG_getParam(&lob->cfg, ZLOB_PARM_COLLIDEDELAY, &fval);
        if (fval > 0.0f)
        {
            prj->ColDelaySet(fval, 0);
        }
        else
        {
            prj->ColDelaySet(-1.0f, 0);
        }
        prj->flg_chk = 0x2E;
        prj->flg_pen = 0x6E;
    }
    else
    {
        prj->ColDelaySet(-1.0f, 0);
        prj->flg_chk = 0x20;
        prj->flg_pen = 0x60;
    }

    float spd_launch = 0.0f;
    float spd_randPct = 0.0f;
    float tmp_rand;
    _xVec3 pos_launch = { 0.0f, 0.0f, 0.0f };
    _xVec3 dir_launch = { 0.0f, 0.0f, 0.0f };
    _xVec3 pos_tgt = { 0.0f, 0.0f, 0.0f };

    if (lob->flg_suitcase & 1)
    {
        pos_launch = lob->pos_altLaunch;
    }
    else
    {
        LCFG_getParam(&lob->cfg, ZLOB_PARM_POS_LAUNCH, &pos_launch);
    }

    LCFG_getParam(&lob->cfg, ZLOB_PARM_SPD_LAUNCH, &spd_launch);
    if (lob->nav_nextbull)
    {
        float fac_arc = 0.0f;
        pos_tgt = *zMovePointGetPos(lob->nav_nextbull);
        xVec3Sub(&dir_launch, &pos_tgt, &pos_launch);
        xVec3Normalize(&dir_launch, &dir_launch);
        LCFG_getParam(&lob->cfg, ZLOB_PARM_ARCFACTOR, &fac_arc);
        if (0.0f != fac_arc || 0.0f != bounce->acc_gravity)
        {
            ST_CAT_AIMDATA aim = { ZPRJ_CAIM_GRAVBALL };
            st_CAT_AIMGRAV* grav = &aim.grav;
            aim.spd_base = spd_launch;
            aim.dir_base = dir_launch;
            grav->pos_src = pos_launch;
            grav->pos_tgt = *zMovePointGetPos(lob->nav_nextbull);
            grav->acc_gravity = bounce->acc_gravity;
            grav->fac_arcness = (fac_arc < 0.0f) ? 1.0f : fac_arc;
            prj->AimAtTarget(&aim, 0);
            dir_launch = aim.dir_base;
        }
    }
    else if (lob->flg_suitcase & 2)
    {
        dir_launch = lob->dir_altLaunch;
    }
    else
    {
        LCFG_getParam(&lob->cfg, ZLOB_PARM_DIR_LAUNCH, &dir_launch);
    }

    LCFG_getParam(&lob->cfg, ZLOB_PARM_SPD_RANDPCT, &spd_randPct);
    if (spd_randPct > 0.0f)
    {
        float spd_base = spd_launch;
        tmp_rand = 2.0f * (iurand() - 0.5f);
        spd_launch += tmp_rand * spd_randPct;
        if (spd_randPct > 1.0f)
        {
            spd_launch = (0.5f * spd_base > spd_launch) ? 0.5f * spd_base : spd_launch;
        }
        else
        {
            spd_launch = (1.0f > spd_launch) ? 1.0f : spd_launch;
        }
    }

    if (lob->flg_lmopts & 4)
    {
        _xVec3 ang = { 0.0f, 0.0f, 0.0f };
        _xMat3x3 mat_rot = {};
        ang.x = lob->ang_debrisCone * (iurand() - 0.5f);
        ang.y = lob->ang_debrisCone * (iurand() - 0.5f);
        ang.z = lob->ang_debrisCone * (iurand() - 0.5f);
        xMat3x3Euler(&mat_rot, &ang);
        xMat3x3LMulVec(&dir_launch, &mat_rot, &dir_launch);
        xVec3Normalize(&dir_launch, &dir_launch);
    }

    prj->frame->mat.pos = pos_launch;
    xVec3SMul(&prj->frame->vel, &dir_launch, spd_launch);
    prj->frame->mode |= 4;
    prj->SyncFrame(1);
    return prj->StateSet(ZPRJ_STATE_ACTIVE);
}

static signed int ZLOB_shoot_arrow(st_LOBMASTER_DATA* lob, signed int nth, signed int total)
{
    zEntProjectile* prj;
    unsigned int prjTypeID = 0;

    LCFG_getParam(&lob->cfg, ZLOB_PARM_PRJTYPEID, &prjTypeID);
    prj = zEntProjectile_acquire(ZPRJ_CAT_ARROW, prjTypeID);
    if (prj == 0)
    {
        return 0;
    }

    prj->owner = lob;
    LCFG_getParam(&lob->cfg, ZLOB_PARM_TYM_MAXLIFE, &prj->tym_maxlife);
    LCFG_getParam(&lob->cfg, ZLOB_PARM_DST_MAXDIST, &prj->ds2_maxlife);
    prj->ds2_maxlife = prj->ds2_maxlife * prj->ds2_maxlife;
    if (lob->flg_lmopts & 0x10)
    {
        prj->flg_prop |= 2;
    }

    ZLOB_setPayload(lob, prj);

    float spd_base = 0.0f;
    float spd_launch;
    float spd_randPct = 0.0f;
    float tmp_rand;
    _xVec3 pos_launch = { 0.0f, 0.0f, 0.0f };
    _xVec3 dir_launch = { 0.0f, 0.0f, 0.0f };
    _xVec3 pos_tgt = { 0.0f, 0.0f, 0.0f };

    LCFG_getParam(&lob->cfg, ZLOB_PARM_SPD_LAUNCH, &spd_base);
    LCFG_getParam(&lob->cfg, ZLOB_PARM_SPD_RANDPCT, &spd_randPct);
    spd_launch = spd_base;
    if (spd_randPct > 0.0f)
    {
        tmp_rand = 2.0f * (iurand() - 0.5f);
        spd_launch += tmp_rand * spd_randPct;
        if (spd_randPct > 1.0f)
        {
            spd_launch = (0.5f * spd_base > spd_launch) ? 0.5f * spd_base : spd_launch;
        }
        else if (1.0f > spd_launch)
        {
            spd_launch = 1.0f;
        }
    }

    if (lob->flg_suitcase & 1)
    {
        pos_launch = lob->pos_altLaunch;
    }
    else
    {
        LCFG_getParam(&lob->cfg, ZLOB_PARM_POS_LAUNCH, &pos_launch);
    }

    if (lob->nav_nextbull)
    {
        pos_tgt = *zMovePointGetPos(lob->nav_nextbull);
        xVec3Sub(&dir_launch, &pos_tgt, &pos_launch);
        xVec3Normalize(&dir_launch, &dir_launch);
    }

    if (lob->flg_suitcase & 2)
    {
        dir_launch = lob->dir_altLaunch;
    }
    else
    {
        LCFG_getParam(&lob->cfg, ZLOB_PARM_DIR_LAUNCH, &dir_launch);
    }

    prj->frame->mat.pos = pos_launch;
    xVec3SMul(&prj->frame->vel, &dir_launch, spd_launch);
    prj->frame->mode |= 4;
    prj->SyncFrame(1);
    return prj->StateSet(ZPRJ_STATE_ACTIVE);
}

// TODO: 98.5%: the jittered speed should be summed into spd_base's register (f30), and the
// scene-ID load in ZLOB_fadeSetup is scheduled later than in the target.
static signed int ZLOB_shoot_debris(st_LOBMASTER_DATA* lob, signed int nth, signed int total)
{
    zEntProjectile* prj;
    unsigned int prjTypeID = 0;
    st_ZPRJ_CAT_BOUNCEBALL* bounce;
    float ang_cone;
    signed int flg_player;
    float spd_base;
    float spd_launch;
    _xVec3 pos_launch = { 0.0f, 0.0f, 0.0f };
    _xVec3 dir_launch = { 0.0f, 0.0f, 0.0f };
    _xVec3 vec_unused = { 0.0f, 0.0f, 0.0f }; // only its rodata initializer survives
    _xVec3 rot_tumble = { 12.566371f, 12.566371f, 12.566371f };
    _xVec3 ang = { 0.0f, 0.0f, 0.0f };
    _xMat3x3 mat_rot = {};

    LCFG_getParam(&lob->cfg, ZLOB_PARM_PRJTYPEID, &prjTypeID);
    prj = zEntProjectile_acquire(ZPRJ_CAT_STDDEBRIS, prjTypeID);
    if (prj == 0)
    {
        return 0;
    }

    prj->owner = lob;
    LCFG_getParam(&lob->cfg, ZLOB_PARM_TYM_MAXLIFE, &prj->tym_maxlife);
    LCFG_getParam(&lob->cfg, ZLOB_PARM_DST_MAXDIST, &prj->ds2_maxlife);
    prj->ds2_maxlife = prj->ds2_maxlife * prj->ds2_maxlife;
    if (lob->flg_lmopts & 0x10)
    {
        prj->flg_prop |= 2;
    }

    ZLOB_setPayload(lob, prj);

    prj->flg_prop &= ~0x10;
    prj->tym_maxlife = (prj->tym_maxlife < 5.0f) ? prj->tym_maxlife : 5.0f;
    prj->ds2_maxlife = (prj->ds2_maxlife < 25.0f) ? prj->ds2_maxlife : 25.0f;
    prj->OrientSet(0);

    flg_player = zEntPlayer_MoveInfo();
    ZLOB_fadeSetup(prj);
    prj->fade_tmr = 0.2f + prj->tym_maxlife;

    bounce = &prj->cat_data.bounce;
    bounce->acc_gravity = 1.5f;
    prj->flg_categ |= 2;
    prj->flg_categ |= 0x2000000;
    bounce->cnt_maxBounce = 4;
    bounce->cnt_curBounce = 0;
    bounce->fac_elastic = 0.8f;
    bounce->pct_randKick = 0.0f;
    prj->flg_categ |= 0x4000000;

    xVec3SMulBy(&rot_tumble, 2.0f * (iurand() - 0.5f));
    prj->FlexMatSet(&rot_tumble, globals.update_dt);
    prj->FlexVecSet(&rot_tumble);
    prj->ColDelaySet(0.2f, 0);
    prj->flg_chk = 0x3E;
    prj->flg_pen = 0x7E;

    if (flg_player & 8)
    {
        spd_base = 5.0f;
    }
    else if (flg_player & 0x10)
    {
        spd_base = 8.0f;
    }
    else
    {
        spd_base = 6.0f;
    }

    spd_launch = 1.8f * (2.0f * (iurand() - 0.5f)) + spd_base;
    spd_launch = (0.5f * spd_base > spd_launch) ? 0.5f * spd_base : spd_launch;

    if (lob->flg_suitcase & 4)
    {
        ZLOB_randpos_fromBound(&lob->bnd_altLaunch, nth, total, 0xA4A, &pos_launch);
    }
    else if (lob->flg_suitcase & 1)
    {
        pos_launch = lob->pos_altLaunch;
    }
    else
    {
        LCFG_getParam(&lob->cfg, ZLOB_PARM_POS_LAUNCH, &pos_launch);
    }

    ang_cone = lob->ang_debrisCone;
    if (lob->flg_suitcase & 4)
    {
        dir_launch = g_Y3;
        if (flg_player & 8)
        {
            ang_cone = 90.0f * 3.1415927f / 180.0f;
            dir_launch.x = 2.0f * (iurand() - 0.5f);
            dir_launch.y = 0.0f;
            dir_launch.z = 2.0f * (iurand() - 0.5f);
        }
        else if (flg_player & 0x10)
        {
            ang_cone = 75.0f * 3.1415927f / 180.0f;
            xVec3Sub(&dir_launch, xBoundCenter(&lob->bnd_altLaunch),
                     (_xVec3*)&globals.player.ent.model->Mat->pos);
            if (xVec3Normalize(&dir_launch, &dir_launch) > 5.0f)
            {
                dir_launch = g_Y3;
            }
        }
        else
        {
            ang_cone = 170.0f * 3.1415927f / 180.0f;
        }
    }
    else if (lob->flg_suitcase & 2)
    {
        dir_launch = lob->dir_altLaunch;
    }
    else
    {
        LCFG_getParam(&lob->cfg, ZLOB_PARM_DIR_LAUNCH, &dir_launch);
    }

    ang.x = ang.y = ang.z = 0.5f * ang_cone;
    ang.x *= 2.0f * (iurand() - 0.5f);
    ang.y *= 2.0f * (iurand() - 0.5f);
    ang.z *= 2.0f * (iurand() - 0.5f);
    xMat3x3Euler(&mat_rot, &ang);
    xMat3x3LMulVec(&dir_launch, &mat_rot, &dir_launch);
    xVec3Normalize(&dir_launch, &dir_launch);

    prj->frame->mat.pos = pos_launch;
    xVec3SMul(&prj->frame->vel, &dir_launch, spd_launch);
    prj->frame->mode |= 5;
    prj->SyncFrame(1);
    return prj->StateSet(ZPRJ_STATE_ACTIVE);
}

static void ZLOB_grab_nextBull(st_LOBMASTER_DATA* lob)
{
    _zMovePoint* nav_tmp = 0;
    _xVec3 hdng = { 0.0f, 0.0f, 0.0f };

    if (lob->nav_prevbull == 0)
    {
        lob->nav_prevbull = lob->nav_bullbase;
        lob->nav_nextbull = lob->nav_bullbase;
    }
    else
    {
        zMovePointGetNext(lob->nav_nextbull, lob->nav_prevbull, &nav_tmp, &hdng);
        lob->nav_prevbull = lob->nav_nextbull;
        lob->nav_nextbull = nav_tmp;
    }

    if (lob->nav_nextbull == 0)
    {
        lob->nav_nextbull = lob->nav_bullbase;
    }
}

static st_LOBMASTER_DATA* ZLOB_find_lobber(unsigned int aid_lobm)
{
    st_LOBMASTER_DATA* tmp_lob;
    st_LOBMASTER_DATA* lob = 0;
    st_LOBMASTER_DEPOT* depot = &g_zlob_depot;

    for (signed int i = 0; i < depot->raw_cnt; i++)
    {
        tmp_lob = &depot->raw_pool[i];
        if (tmp_lob->id != 0 && tmp_lob->id == aid_lobm)
        {
            lob = tmp_lob;
            break;
        }
    }

    return lob;
}

static signed int ZLOB_randpos_fromBound(_xBound* bnd, signed int nth, signed int total,
                                         signed int flg_ropts, _xVec3* pos_launch)
{
    float rad;
    float hyt;
    _xVec3 delta = { 0.0f, 0.0f, 0.0f };
    _xVec3 vec_tmp = { 0.0f, 0.0f, 0.0f };
    _xVec3 dirvec = { 0.0f, 0.0f, 1.0f };
    _xMat3x3 mat_rot = {};
    _xVec3 rot_range = { 6.2831855f, 6.2831855f, 6.2831855f };
    signed int result = 1;

    if (bnd->type == 2 || bnd->type == 4)
    {
        xVec3Sub(&vec_tmp, &bnd->box.box.upper, &bnd->box.box.lower);
        if (flg_ropts & 0x20)
        {
            xVec3SMul(&delta, &vec_tmp, 0.5f);
            delta.x *= 2.0f * (iurand() - 0.5f);
            delta.y *= 2.0f * (iurand() - 0.5f);
            delta.z *= 2.0f * (iurand() - 0.5f);
        }
        else
        {
            xVec3SMulBy(&vec_tmp, 0.5f);
            delta = vec_tmp;
            delta.x *= 2.0f * (iurand() - 0.5f);
            delta.y *= 2.0f * (iurand() - 0.5f);
            delta.z *= 2.0f * (iurand() - 0.5f);
            if (xUtil_yesno(0.333f))
            {
                if (xUtil_yesno())
                {
                    delta.x = vec_tmp.x;
                }
                else
                {
                    delta.x = -vec_tmp.x;
                }
            }
            else if (xUtil_yesno(0.333f))
            {
                if (xUtil_yesno(0.75f))
                {
                    delta.y = vec_tmp.y;
                }
                else
                {
                    delta.y = -vec_tmp.y;
                }
            }
            else if (xUtil_yesno())
            {
                delta.z = vec_tmp.z;
            }
            else
            {
                delta.z = -vec_tmp.z;
            }
        }
        if (bnd->type == 4)
        {
            xMat3x3LMulVec(&delta, (_xMat3x3*)bnd->mat, &delta);
        }
        xVec3Add(pos_launch, &delta, &bnd->box.center);
    }
    else if (bnd->type == 1)
    {
        if (flg_ropts & 1)
        {
            rot_range.x *= iurand();
            rot_range.y *= iurand();
            rot_range.z *= iurand();
        }
        else if (flg_ropts & 2)
        {
            rot_range.y = 3.1415927f;
            rot_range.z = 0.0f;
            rot_range.x *= iurand();
            rot_range.y *= iurand();
            rot_range.z *= iurand();
        }
        else if (flg_ropts & 4)
        {
            rot_range.y = 3.1415927f;
            rot_range.z = 0.0f;
            rot_range.x *= iurand();
            rot_range.y *= iurand();
            rot_range.z *= iurand();
            rot_range.y += 3.1415927f;
        }
        xMat3x3Euler(&mat_rot, &rot_range);
        xMat3x3LMulVec(&dirvec, &mat_rot, &dirvec);
        if (flg_ropts & 8)
        {
            xVec3SMul(&delta, &dirvec, bnd->sph.r);
        }
        else
        {
            xVec3SMul(&delta, &dirvec, bnd->sph.r * iurand());
        }
        xVec3Add(pos_launch, &delta, &bnd->sph.center);
    }
    else if (bnd->type == 3)
    {
        rad = bnd->cyl.r;
        hyt = 0.5f * bnd->cyl.h;
        if (flg_ropts & 0x800)
        {
            if (xUtil_yesno(0.25f))
            {
                rad = bnd->cyl.r * iurand();
            }
            else
            {
                hyt = 0.5f * bnd->cyl.h * (2.0f * (iurand() - 0.5f));
            }
        }
        else
        {
            hyt = 0.5f * bnd->cyl.h * (2.0f * (iurand() - 0.5f));
            rad = bnd->cyl.r * iurand();
        }
        if (!(flg_ropts & 0x100))
        {
            if ((flg_ropts & 0x200) && hyt < 0.0f)
            {
                hyt = -hyt;
            }
            else if ((flg_ropts & 0x400) && hyt > 0.0f)
            {
                hyt = -hyt;
            }
        }
        delta.x = rad * (2.0f * (iurand() - 0.5f));
        delta.y = hyt;
        delta.z = rad * (2.0f * (iurand() - 0.5f));
        xVec3Add(pos_launch, &delta, &bnd->cyl.center);
    }
    else
    {
        result = 0;
    }

    return result;
}

static signed int LPRJ_debris_attach(zEntProjectile* prj)
{
    st_ZPRJ_CAT_BOUNCEBALL* bounce = &prj->cat_data.bounce;

    prj->cat_data.idx_cdat = 2;
    bounce->tym_atrest = 3.0f;
    bounce->acc_gravity = 15.0f;
    bounce->cnt_maxBounce = 5;
    bounce->fac_elastic = 0.3f;
    bounce->pct_randKick = 0.0f;
    return 1;
}

static signed int LPRJ_debris_logix(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame)
{
    st_ZPRJ_CAT_BOUNCEBALL* bounce = &prj->cat_data.bounce;
    _xVec3 vec_accum = { 0.0f, 0.0f, 0.0f };
    float elastic;

    if (bounce->atRest)
    {
        prj->SendEvent(ZPRJ_EVNT_ATREST, dt);
    }
    else
    {
        prj->SendEvent(ZPRJ_EVNT_INFLIGHT, dt);
    }

    if (prj->tmr_alive > prj->tym_maxlife)
    {
        prj->SendEvent(ZPRJ_EVNT_SELFDESTRUCT, dt);
        prj->StateSet(ZPRJ_STATE_DEAD);
        return 1;
    }

    _xVec3 tmp_vec = { 0.0f, 0.0f, 0.0f };
    xVec3Sub(&tmp_vec, (_xVec3*)&prj->model->Mat->pos, &prj->pos_home);
    if (xVec3Length2(&tmp_vec) > prj->ds2_maxlife)
    {
        prj->SendEvent(ZPRJ_EVNT_SELFDESTRUCT, dt);
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
        prj->SendEvent(ZPRJ_EVNT_DETONATE, dt);
        prj->StateSet(ZPRJ_STATE_DEAD);
    }
    else if ((prj->plyr_colrec->flags & 1) || (prj->collis->colls[0].flags & 1) ||
             (prj->collis->colls[1].flags & 1) ||
             prj->collis->env_sidx < prj->collis->env_eidx ||
             prj->collis->npc_sidx < prj->collis->npc_eidx ||
             prj->collis->stat_sidx < prj->collis->stat_eidx ||
             prj->collis->dyn_sidx < prj->collis->dyn_eidx)
    {
        if ((prj->flg_categ & 0x2000000) && bounce->skip_next_bounce &&
            bounce->cnt_curBounce < bounce->cnt_maxBounce)
        {
            bounce->skip_next_bounce = 0;
            prj->frame->vel = prj->frame->oldvel;
            prj->frame->mode |= 4;
        }
        else if ((prj->flg_categ & 0x2000000) && bounce->cnt_curBounce < bounce->cnt_maxBounce)
        {
            bounce->cnt_curBounce++;
            bounce->skip_next_bounce = 1;
            bounce->atRest = 2;
            elastic = bounce->fac_elastic;
            if (prj->flg_categ & 2)
            {
                elastic *= 0.5f;
            }
            zEntProjectile_colAccum(prj, &vec_accum);
            zEntProjectile_softReflect(&prj->frame->vel, &prj->frame->oldvel, &vec_accum,
                                       elastic);
            prj->frame->mode |= 4;
            prj->SendEvent(ZPRJ_EVNT_BOUNCE, dt);
            if (xVec3Length2(&frame->vel) < 0.2f * 0.2f)
            {
                bounce->cnt_curBounce = bounce->cnt_maxBounce;
            }
        }
        else
        {
            prj->SendEvent(ZPRJ_EVNT_DETONATE, dt);
            prj->StateSet(ZPRJ_STATE_DEAD);
        }
    }

    return 1;
}

static signed int LPRJ_debris_prjmove(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame)
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
            frame->dvel.y -= dt * bounce->acc_gravity;
        }
        else
        {
            frame->dvel.y = -(dt * bounce->acc_gravity);
        }
        frame->dvel.z = 0.0f;
        frame->mode |= 8;
    }

    if ((prj->flg_categ & 0x4000000) && bounce->atRest != 1)
    {
        xMat4x3RMul3x3(&frame->mat, &prj->common.mat_flex, &frame->mat);
        frame->mode |= 0x100;
    }

    return 1;
}

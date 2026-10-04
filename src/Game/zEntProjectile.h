#ifndef ZENTPROJECTILE_H
#define ZENTPROJECTILE_H

#include "xEnt.h"

class zScene;
class _xCollis;
class _zGust;

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

enum en_ZPRJ_FX_TYPE
{
    ZPRJ_FXTYPE_NONE,
    ZPRJ_FXTYPE_SMOKE,
    ZPRJ_FXTYPE_WOOD,
    ZPRJ_FXTYPE_TAR,
    ZPRJ_FXTYPE_FIREBALL,
    ZPRJ_FXTYPE_NODAMAGE_FIREBALL,
    ZPRJ_FXTYPE_ICE,
    ZPRJ_FXTYPE_BLACKKNIGHT,
    ZPRJ_FXTYPE_WITCHDOCTOR,
    ZPRJ_FXTYPE_REDBEARD,
    ZPRJ_FXTYPE_MASTERMIND_RED,
    ZPRJ_FXTYPE_MASTERMIND_GREEN,
    ZPRJ_FXTYPE_MASTERMIND_BLACK,
    ZPRJ_FXTYPE_UNUSED3,
    ZPRJ_FXTYPE_UNUSED4,
    ZPRJ_FXTYPE_UNUSED5,
    ZPRJ_FXTYPE_TOTAL
};

enum en_ZPRJ_MODEL_INDEX
{
    ZPRJ_MDLIDX_STD,
    ZPRJ_MDLIDX_POSTHIT,
    ZPRJ_MDLIDX_NOMORE
};

enum en_ZPRJ_CATDATA_FORM
{
    ZPRJ_CDAT_ARROW,
    ZPRJ_CDAT_GRAVBALL,
    ZPRJ_CDAT_BOUNCE
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

class st_ZPRJ_CAT_ARROW
{
public:
    float acc_gravity; // 0x0
    signed int amStuck; // 0x4
    _xEnt* ent_stuck;   // 0x8
    _xVec3 pos_stuck;   // 0xC
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

class st_ZPRJ_CAT_PICKUP
{
public:
    signed int nada; // 0x0
};

class st_ZPRJ_CATDATA
{
public:
    signed int idx_cdat; // 0x0
    union
    {
        st_ZPRJ_CAT_ARROW arrow;       // 0x4
        st_ZPRJ_CAT_GRAVBALL gravball; // 0x4
        st_ZPRJ_CAT_BOUNCEBALL bounce; // 0x4
        st_ZPRJ_CAT_PICKUP pickup;     // 0x4
    };
};

class st_ZPRJ_COMMON
{
public:
    _xVec3 vec_flex;   // 0x0
    _xMat3x3 mat_flex; // 0xC
};

class st_ENODE
{
public:
    st_ENODE* next; // 0x0
    st_ENODE* prev; // 0x4
};

class st_ELIST
{
public:
    st_ENODE* head;     // 0x0
    st_ENODE* tail;     // 0x4
    st_ENODE* tailprev; // 0x8
};

class zEntProjectile;

class st_ZPRJ_LINKNODE
{
public:
    st_ENODE node;           // 0x0
    zEntProjectile* backref; // 0x8
};

class zProjectileTypeAsset : public p2BaseAsset
{
public:
    signed int efxType;           // 0x8
    unsigned int modelID;         // 0xC
    unsigned int animID;          // 0x10
    unsigned int atRestModelID;   // 0x14
    unsigned int atRestAnimID;    // 0x18
    signed int destructEnabled;   // 0x1C
    float destructTime;           // 0x20
    float destructDist;           // 0x24
    signed int oriented;          // 0x28
    signed int extra_space[6];    // 0x2C
};

class st_ZPRJ_TYPERECORD
{
public:
    unsigned int aid_ptype;          // 0x0
    zProjectileTypeAsset* src_ptype; // 0x4
    en_ZPRJ_CATEGORY categ;          // 0x8
    RpAtomic* mdlFile[2];            // 0xC
};

class st_PROJECTILE_DEPOT
{
public:
    signed int typecnt;              // 0x0
    st_ZPRJ_TYPERECORD* typetable;   // 0x4
    signed int typeidx;              // 0x8
    signed int rawcnt;               // 0xC
    zEntProjectile* rawpool;         // 0x10
    st_ELIST prjpool;                // 0x14
    st_ELIST prjinuse;               // 0x20
};

class st_ZPRJ_CATEGORY_FUNCS
{
public:
    signed int flg_cback;                                                 // 0x0
    signed int (*attach)(zEntProjectile*);                                // 0x4
    void (*detach)(zEntProjectile*);                                      // 0x8
    signed int (*logix)(zEntProjectile*, float, zScene*, _xEntFrame*);    // 0xC
    signed int (*calcmove)(zEntProjectile*, float, zScene*, _xEntFrame*); // 0x10
    signed int (*notify)(zEntProjectile*, en_PROJECTILE_EVENT, void*);    // 0x14
    signed int (*render)(zEntProjectile*);                                // 0x18
    signed int (*aimer)(zEntProjectile*, ST_CAT_AIMDATA*, signed int);    // 0x1C
    signed int (*collide)(zEntProjectile*, zScene*, float);               // 0x20
    void (*dbgUpdate)(zEntProjectile*, signed int);                       // 0x24
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

class _xEntShadow
{
public:
    _xVec3 pos; // 0x0
    _xVec3 vec; // 0xC
};

class zGustData
{
public:
    unsigned int gust_on; // 0x0
    _zGust* g[4];         // 0x4
    float lerp[4];        // 0x14
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
    float tmr_alive;          // 0x104
    float tmr_curstate;       // 0x108
    _xVec3 pos_home;          // 0x10C
    float tym_maxlife;        // 0x118
    float ds2_maxlife;        // 0x11C
    st_ZPRJ_COMMON common;    // 0x120
    st_ZPRJ_CATDATA cat_data; // 0x15C
    void* xtradata;           // 0x180
    _xCollis* plyr_colrec;    // 0x184
    st_ZPRJ_LINKNODE linknode; // 0x188
    en_ZPRJ_STATES state;     // 0x194
    signed int flg_stat;      // 0x198
    float pct_docol;          // 0x19C
    float tmr_coldelay;       // 0x1A0
    en_ZPRJ_CATEGORY cat_proj; // 0x1A4
    st_ZPRJ_TYPERECORD* ptype; // 0x1A8
    float tmr_emit;           // 0x1AC
    _xEntFrame raw_frame;     // 0x1B0
    p2EntAsset raw_asset;     // 0x2A0
    _xEntShadow raw_shadow;   // 0x2F0
    zGustData raw_gust;       // 0x308

    void PEFX_flight(en_ZPRJ_FX_TYPE fxtyp, float dt);
    unsigned char LookForFloorRoof(zScene* scn, float dt);
    signed int ZPRJ_render();
    signed int DoColCheck(float dt);
    signed int ZPRJ_update(zScene* scn, float dt);
    void OrientToPath(_xEntFrame* frame, signed int flg_orient);
    void OrientSet(signed int orient);
    signed int Setup_xEnt();
    void SetupFrame(_xVec3* pos, _xVec3* veloc);
    signed int Init(en_ZPRJ_CATEGORY cat, unsigned int ptypeID);
    signed int AimAtTarget(ST_CAT_AIMDATA* info, signed int apply);
    void FlexMatSet(_xVec3* rot_euler, float dt);
    void FlexVecSet(_xVec3* vec_flex);
    signed int SendEvent(en_PROJECTILE_EVENT event, float dt);
    signed int SendEvent(en_PROJECTILE_EVENT event);
    void SyncFrame(signed int applyToModel);
    void Abort();
    void ColDelaySet(float tym, signed int addto);
    en_ZPRJ_STATES StateSet(en_ZPRJ_STATES state);
};

zEntProjectile* zEntProjectile_acquire(en_ZPRJ_CATEGORY cat, unsigned int ptypeID);
en_ZPRJ_CATEGORY zEntProjectile_catFromPrjType(unsigned int ptypeID, en_ZPRJ_CATEGORY prefer);
signed int zEntProjectile_gen_aimer(zEntProjectile* prj, ST_CAT_AIMDATA* info, signed int apply);
void zEntProjectile_softReflect(_xVec3* dir_new, _xVec3* dir_cur, _xVec3* oppose, float scaleMag);
float zEntProjectile_colAccum(zEntProjectile* prj, _xVec3* dir_away);
signed int zEntProjectile_gen_render(zEntProjectile* prj);
signed int zEntProjectile_gen_logix(zEntProjectile* prj, float dt, zScene* scn, _xEntFrame* frame);
signed int zEntProjectile_gen_attach(zEntProjectile* prj);
void zEntProjectile_poolRender(en_ZPRJ_CATEGORY cat);
signed int zEntProjectile_poolUpdate(en_ZPRJ_CATEGORY cat, zScene* scn, float dt);
void zEntProjectile_prepSystem(signed int num_ptype, zScene* scn);
void zEntProjectile_sceneFinish();
void zEntProjectile_sceneReset();
void zEntProjectile_scenePrepare();
void zEntProjectile_registerType(zProjectileTypeAsset* ptype);
signed int zEntProjectile_addProps(en_ZPRJ_CATEGORY cat, st_ZPRJ_CATEGORY_PROPS* props);
signed int zEntProjectile_addCat(en_ZPRJ_CATEGORY cat, st_ZPRJ_CATEGORY_FUNCS* funcs);
void zEntProjectile_Shutdown();
void zEntProjectile_Startup();

#endif

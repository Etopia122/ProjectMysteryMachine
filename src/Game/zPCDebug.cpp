#ifdef PC_PORT
// PC debug menu actions (pc/src/debug_menu.cpp): level warps and cheats.
// Not part of the GameCube build.
#include <stdio.h>
#include <string.h>
#include "xBase.h"
#include "zGlobals.h"
#include "zScene.h"
#include "xEnt.h"
#include "xString.h"

xBase* zSceneFindObject(unsigned int gameID);
void* xSTFindAsset(unsigned int aid, unsigned int* size);
extern "C" void pc_log(const char* fmt, ...);
extern "C" void pc_rw_atomic_spheres(void* atomic, float* out8);
extern "C" const char* pc_dbg_ent_anim(_xEnt* ent);

class zPortalAsset : public p2BaseAsset
{
public:
    unsigned int assetCameraID;
    unsigned int assetMarkerID;
    float ang;
    unsigned int sceneID;
};

class _zPortal : public xBase
{
public:
    zPortalAsset* passet;
};

void zEntPickupCheat_allWarps();
void zEntEvent(char* to, unsigned int toEvent);
signed int zEntPlayer_Damage(xBase* src, unsigned int damage);
void zVarUnlockEndGame();
void zVarUnlockAllMovies();
signed int zGameStateGet();

static zPortalAsset sWarpAsset;
static _zPortal sWarpPortal;
static int sInfiniteHealth;

extern "C" {

// Leaves through a portal to the given scene (e.g. 'F010'), landing at the
// scene's normal start. The scene ID's bytes are stored big-endian, as the
// game reads them (see zSceneSwitch).
int pc_dbg_warp(unsigned int sceneID)
{
    if (!globals.sceneCur || globals.sceneCur->pendingPortal || zGameStateGet() != 1)
    {
        return 0;
    }
    memset(&sWarpAsset, 0, sizeof(sWarpAsset));
    unsigned char* id = (unsigned char*)&sWarpAsset.sceneID;
    id[0] = (unsigned char)(sceneID >> 24);
    id[1] = (unsigned char)(sceneID >> 16);
    id[2] = (unsigned char)(sceneID >> 8);
    id[3] = (unsigned char)sceneID;
    memset(&sWarpPortal, 0, sizeof(sWarpPortal));
    sWarpPortal.passet = &sWarpAsset;
    globals.sceneCur->pendingPortal = &sWarpPortal;
    return 1;
}

// The game's own "all powerups" cheat.
void pc_dbg_all_upgrades()
{
    globals.player.Inv_Specials |= 0x1 | 0x2 | 0x4 | 0x8 | 0x10 | 0x20 | 0x40 | 0x80 | 0x100 | 0x200 | 0x400 | 0x800 |
                                   0x1000 | 0x2000 | 0x4000;
}

void pc_dbg_all_warps() { zEntPickupCheat_allWarps(); }

void pc_dbg_unlock_extras()
{
    zVarUnlockEndGame();
    zVarUnlockAllMovies();
}

void pc_dbg_refill()
{
    globals.player.Health = 5;
    if (globals.player.Inv_Chances < 9)
    {
        globals.player.Inv_Chances = 9;
    }
    for (int i = 0; i < 2; i++)
    {
        if (globals.player.Inv_AmmoMax[i] > 0)
        {
            globals.player.Inv_Ammo[i] = globals.player.Inv_AmmoMax[i];
        }
    }
}

void pc_dbg_add_snacks(unsigned int n) { globals.player.Inv_ScoobySnacks += n; }

int pc_dbg_toggle_infinite_health() { return sInfiniteHealth = !sInfiniteHealth; }

// Sends an event to the named object (debugging scripted sequences).
void pc_dbg_event(const char* name, unsigned int event) { zEntEvent((char*)name, event); }

// Hurts Scooby by one, as an enemy hit does (testing damage effects).
void pc_dbg_hurt() { zEntPlayer_Damage(&globals.player.ent, 1); }

// Moves an entity (e.g. a boss) to a position, keeping its bound in step.
static void move_ent(_xEnt* e, const _xVec3* pos)
{
    if (!e || !e->frame)
    {
        return;
    }
    e->frame->mat.pos = *pos;
    e->frame->oldpos = *pos;
    if (e->model && e->model->Mat)
    {
        *(_xVec3*)&e->model->Mat->pos = *pos;
    }
    if (e->bupdate)
    {
        e->bupdate(e, (_xVec3*)pos);
    }
}

// Damages the current boss the way the level does it. Returns a status message.
// A Dark and Stormy Knight (O008): when the Black Knight walks into the
// detection trigger at lightning rod n it enables BUTTONn_CNTR; pressing
// button n raises the rod (LR_MECHn) and decrements the counter, whose
// expiry strikes LRn_STRIKE with lightning, hits the knight (event 174) and
// lowers his health. Do the same, with the knight placed at the strike point.
const char* pc_dbg_force_boss_damage()
{
    static int sNextButton = 1;
    static unsigned int sScene;
    static char msg[64];
    unsigned int scene = globals.sceneCur ? globals.sceneCur->sceneID : 0;
    if (scene != sScene)
    {
        sScene = scene;
        sNextButton = 1;
    }
    if (scene == 'O008')
    {
        if (sNextButton > 4)
        {
            return "BLACK KNIGHT ALREADY HIT 4 TIMES";
        }
        char name[32];
        unsigned int size;
        sprintf(name, "LR%d_STRIKE", sNextButton);
        unsigned char* marker = (unsigned char*)xSTFindAsset(xStrHash(name), &size);
        _xEnt* knight = (_xEnt*)zSceneFindObject(xStrHash("BLACK_KNIGHT"));
        if (marker && knight)
        {
            move_ent(knight, (_xVec3*)(marker + 8));
        }
        sprintf(name, "BUTTON%02d_CNTR", sNextButton);
        zEntEvent(name, 1); // enable, as the detection trigger does
        sprintf(name, "LR_MECH%02d", sNextButton);
        zEntEvent(name, 18); // the rod rises, as the button does
        sprintf(name, "BUTTON%02d_CNTR", sNextButton);
        zEntEvent(name, 12); // decrement: strike, hit, health
        sprintf(msg, "BLACK KNIGHT HIT %d OF 4", sNextButton);
        sNextButton++;
        return msg;
    }
    return "NO FORCED DAMAGE FOR THIS BOSS YET";
}

// Logs an entity's visibility and model state (debugging invisible objects).
void pc_dbg_ent_info(const char* name)
{
    _xEnt* e = (_xEnt*)zSceneFindObject(xStrHash(name));
    if (!e)
    {
        pc_log("[ent] %s not found", name);
        return;
    }
    xModelInstance* m = e->model;
    pc_log("[ent] %s anim %s pos %.2f %.2f %.2f", name, pc_dbg_ent_anim(e), e->frame ? e->frame->mat.pos.x : 0,
           e->frame ? e->frame->mat.pos.y : 0, e->frame ? e->frame->mat.pos.z : 0);
    pc_log("[ent] %s type %d baseFlags %x flags %x pflags %x moreFlags %x collType %x alpha %.2f model %p render %p", name,
           e->baseType, e->baseFlags, e->flags, e->pflags, e->moreFlags, e->collType, e->material_alpha, m, e->render);
    for (int i = 0; m && i < 4; i++, m = m->Next)
    {
        float* mat = (float*)m->Mat;
        pc_log("[ent]   model %d atomic %p flags %x anim %p pos %.2f %.2f %.2f scale %.2f %.2f %.2f", i, m->Data, m->Flags,
               m->Anim, mat ? mat[12] : 0, mat ? mat[13] : 0, mat ? mat[14] : 0, m->Scale.x, m->Scale.y, m->Scale.z);
        float sp[8];
        pc_rw_atomic_spheres(m->Data, sp);
        pc_log("[ent]     atomic sphere (%.2f %.2f %.2f) r %.2f, geometry sphere (%.2f %.2f %.2f) r %.2f", sp[0], sp[1], sp[2],
               sp[3], sp[4], sp[5], sp[6], sp[7]);
    }
}

// Called once per frame.
void pc_dbg_frame()
{
    if (sInfiniteHealth && globals.sceneCur && globals.player.Health > 0 && globals.player.Health < 5)
    {
        globals.player.Health = 5;
    }
}

}
#endif

#ifdef PC_PORT
// PC debug menu actions (pc/src/debug_menu.cpp): level warps and cheats.
// Not part of the GameCube build.
#include <string.h>
#include "xBase.h"
#include "zGlobals.h"
#include "zScene.h"

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

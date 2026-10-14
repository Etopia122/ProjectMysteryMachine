// Test hooks driven by environment variables (PC_PORTAL, PC_PROBE, PC_SPHERE,
// PC_POSLOG, PC_TELEPORT, PC_SURFDUMP, PC_FORCESAVE; see pc/PORTING.md), run
// once per presented frame on every platform.
#include "pc_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" void pc_dbg_all_warps();
extern "C" void pc_dbg_all_upgrades();
extern "C" void pc_dbg_refill();
void zEntPickup_PCWarpSetSeen(signed int seen);
void zGameStateSwitchEvent(signed int event);
extern "C" void pc_debug_portal(int index);
extern "C" void pc_debug_surfaces();
extern "C" void pc_debug_teleport(float x, float y, float z);
extern "C" void pc_debug_player_pos(float* out);
extern "C" void pc_dbg_player_colls();
extern "C" void pc_dbg_player_anim(const char** name, unsigned int* userFlags, unsigned int* sticky, int* speed);
extern "C" void pc_debug_probe(float x, float z);
extern "C" void pc_debug_sectors();
extern "C" void pc_debug_sphere(float x, float y, float z, float r);

// Read on the first frame: the 3DS sets its environment in main.
static bool s_teleportDone;
static bool teleportPending() { return !s_teleportDone; }

extern "C" void pc_debug_hooks_frame()
{
    static bool started;
    if (!started)
    {
        started = true;
        s_teleportDone = !getenv("PC_TELEPORT");
    }
    // Debug: PC_PORTAL=<seconds>:<index> takes a portal of the current scene.
    static const char* portal = getenv("PC_PORTAL");
    if (portal && pc_ticks_ms() >= (unsigned int)(atof(portal) * 1000))
    {
        const char* c = strchr(portal, ':');
        portal = NULL;
        pc_debug_portal(c ? atoi(c + 1) : 0);
    }
    // Debug: PC_PROBE=<sec>:x0,z0,x1,z1,n probes n vertical rays from (x0,z0) to (x1,z1).
    static const char* probe = getenv("PC_PROBE");
    if (probe && pc_ticks_ms() >= (unsigned int)(atof(probe) * 1000))
    {
        float x0, z0, x1, z1;
        int n = 1;
        const char* c = strchr(probe, ':');
        probe = NULL;
        if (c && sscanf(c + 1, "%f,%f,%f,%f,%d", &x0, &z0, &x1, &z1, &n) >= 4)
        {
            pc_debug_sectors();
            for (int i = 0; i < n; i++)
            {
                float t = n > 1 ? (float)i / (n - 1) : 0;
                pc_debug_probe(x0 + (x1 - x0) * t, z0 + (z1 - z0) * t);
            }
        }
    }
    // Debug: PC_SPHERE=<sec>:x,y,z,r lists world triangles near a point.
    static const char* sphere = getenv("PC_SPHERE");
    if (sphere && pc_ticks_ms() >= (unsigned int)(atof(sphere) * 1000))
    {
        float x, y, z, r;
        const char* c = strchr(sphere, ':');
        sphere = NULL;
        if (c && sscanf(c + 1, "%f,%f,%f,%f", &x, &y, &z, &r) == 4)
        {
            pc_debug_sphere(x, y, z, r);
        }
    }
    // Debug: PC_POSLOG=1 logs the player position every second (after a teleport).
    static int posLog = getenv("PC_POSLOG") ? 1 : 0;
    static unsigned int nextPos;
    if (posLog && !teleportPending() && pc_ticks_ms() >= nextPos)
    {
        nextPos = pc_ticks_ms() + (getenv("PC_POSLOG")[0] >= '2' ? 250 : 1000);
        float p[3];
        pc_debug_player_pos(p);
        const char* anim;
        unsigned int uf, sticky;
        int speed;
        pc_dbg_player_anim(&anim, &uf, &sticky, &speed);
        pc_log("[pos] %.1fs %.2f %.2f %.2f anim %s flags %x sticky %u speed %d", pc_ticks_ms() / 1000.0, p[0], p[1],
               p[2], anim, uf, sticky, speed);
        if (getenv("PC_POSLOG")[0] == '3')
        {
            pc_dbg_player_colls();
        }
    }
    // Debug: PC_TELEPORT=<seconds>:x,y,z moves the player there.
    static const char* teleport = getenv("PC_TELEPORT");
    if (teleport && pc_ticks_ms() >= (unsigned int)(atof(teleport) * 1000))
    {
        float x = 0, y = 0, z = 0;
        const char* c = strchr(teleport, ':');
        teleport = NULL;
        if (c && sscanf(c + 1, "%f,%f,%f", &x, &y, &z) == 3)
        {
            pc_debug_teleport(x, y, z);
        }
        s_teleportDone = true;
    }
    // Debug: PC_SURFDUMP=<seconds> lists the special surfaces of the world.
    static const char* surfDump = getenv("PC_SURFDUMP");
    if (surfDump && pc_ticks_ms() >= (unsigned int)(atof(surfDump) * 1000))
    {
        surfDump = NULL;
        pc_debug_surfaces();
    }
    // Debug: PC_FORCESAVE=<seconds> fires the event that opens the save screen.
    static const char* forceSave = getenv("PC_FORCESAVE");
    if (forceSave && pc_ticks_ms() >= (unsigned int)(atof(forceSave) * 1000))
    {
        forceSave = NULL;
        zGameStateSwitchEvent(0x97);
    }
    // Debug: PC_ALLWARPS=<seconds> opens every warp gate (the debug menu's item).
    static const char* allWarps = getenv("PC_ALLWARPS");
    if (allWarps && pc_ticks_ms() >= (unsigned int)(atof(allWarps) * 1000))
    {
        allWarps = NULL;
        pc_dbg_all_warps();
        pc_log("[debug] all warp gates open");
    }
    // Debug: PC_UPGRADES=<seconds> gives every upgrade and refills (the debug menu's items).
    static const char* upgrades = getenv("PC_UPGRADES");
    if (upgrades && pc_ticks_ms() >= (unsigned int)(atof(upgrades) * 1000))
    {
        upgrades = NULL;
        pc_dbg_all_upgrades();
        pc_dbg_refill();
        pc_log("[debug] all upgrades, refilled");
    }
    // Debug: PC_WARPSEEN=<seconds> counts as walking by a warp gate (no gate opens).
    static const char* warpSeen = getenv("PC_WARPSEEN");
    if (warpSeen && pc_ticks_ms() >= (unsigned int)(atof(warpSeen) * 1000))
    {
        warpSeen = NULL;
        zEntPickup_PCWarpSetSeen(1);
        pc_log("[debug] walked by a warp gate");
    }
}

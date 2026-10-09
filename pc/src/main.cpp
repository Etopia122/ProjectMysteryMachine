// PC entry point: sets up the platform, then runs the game's main loop.
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <windows.h>
#include <thread>
#include "pc_platform.h"

void scooby_main();
void pc_map_gc_memory();

// Called by generated stubs (pc/src/stubs_gen.S) for library functions that
// have no PC implementation yet. Reports each one the first time it runs.
extern "C" void pc_stub_hit(const char* name)
{
    static const char* seen[1024];
    static int nseen;
    for (int i = 0; i < nseen; i++)
    {
        if (seen[i] == name)
        {
            return;
        }
    }
    if (nseen < 1024)
    {
        seen[nseen++] = name;
    }
    fprintf(stderr, "[stub] %s\n", name);
}

extern "C" void pc_fptrap();
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
extern "C" unsigned int pc_debug_scene_id();
extern "C" void pc_render_set_overlay(const char* text);
static bool s_teleportDone = !getenv("PC_TELEPORT");
static bool teleportPending() { return !s_teleportDone; }

// Writes crash.txt next to the executable: the exception and a frame-pointer
// stack walk, with addresses relative to the default image base so
// `addr2line -f -C -e scooby.exe <addr>` names them.
static LONG WINAPI crash_handler(EXCEPTION_POINTERS* ep)
{
    DWORD base = (DWORD)GetModuleHandleA(NULL);
    char path[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, path, MAX_PATH);
    while (n > 0 && path[n - 1] != '\\' && path[n - 1] != '/')
    {
        n--;
    }
    strcpy(path + n, "crash.txt");
    FILE* f = fopen(path, "w");
    FILE* outs[2] = { stderr, f };
    CONTEXT* c = ep->ContextRecord;
    for (FILE* o : outs)
    {
        if (!o)
        {
            continue;
        }
        fprintf(o, "Scooby PC crashed: exception %08lx at %08lx (image base 0x400000)\n",
                ep->ExceptionRecord->ExceptionCode, c->Eip - base + 0x400000);
        if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
        {
            fprintf(o, "  %s address %08lx\n", ep->ExceptionRecord->ExceptionInformation[0] ? "writing" : "reading",
                    (DWORD)ep->ExceptionRecord->ExceptionInformation[1]);
        }
        fprintf(o, "stack:\n  %08lx\n", c->Eip - base + 0x400000);
        DWORD* fp = (DWORD*)c->Ebp;
        for (int i = 0; i < 48 && fp && !IsBadReadPtr(fp, 8); i++)
        {
            DWORD ret = fp[1];
            fprintf(o, "  %08lx\n", ret >= base ? ret - base + 0x400000 : ret);
            DWORD* next = (DWORD*)fp[0];
            if (next <= fp)
            {
                break;
            }
            fp = next;
        }
    }
    if (f)
    {
        fclose(f);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

extern "C" const char* pc_base_path()
{
    static char* base;
    if (!base)
    {
        base = SDL_GetBasePath();
        if (!base)
        {
            base = SDL_strdup("");
        }
    }
    return base;
}

extern "C" void pc_mkdir(const char* path)
{
    CreateDirectoryA(path, NULL);
}

extern "C" void pc_thread_start(void (*fn)(void), const char* name)
{
    std::thread(fn).detach();
}

extern "C" unsigned int pc_ticks_ms()
{
    return SDL_GetTicks();
}

extern "C" void pc_exit(int code)
{
    SDL_Quit();
    exit(code);
}

bool pc_debug_menu_event(const SDL_Event& e);
void pc_debug_menu_frame();

// Window/system events; called once per presented frame.
void pc_events_pump()
{
    pc_debug_menu_frame();
    // Debug: PC_PORTAL=<seconds>:<index> takes a portal of the current scene.
    static const char* portal = getenv("PC_PORTAL");
    if (portal && SDL_GetTicks() >= (Uint32)(atof(portal) * 1000))
    {
        const char* c = strchr(portal, ':');
        portal = NULL;
        pc_debug_portal(c ? atoi(c + 1) : 0);
    }
    // Debug: PC_PROBE=<sec>:x0,z0,x1,z1,n probes n vertical rays from (x0,z0) to (x1,z1).
    static const char* probe = getenv("PC_PROBE");
    if (probe && SDL_GetTicks() >= (Uint32)(atof(probe) * 1000))
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
    if (sphere && SDL_GetTicks() >= (Uint32)(atof(sphere) * 1000))
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
    static Uint32 nextPos;
    if (posLog && !teleportPending() && SDL_GetTicks() >= nextPos)
    {
        nextPos = SDL_GetTicks() + (getenv("PC_POSLOG")[0] >= '2' ? 250 : 1000);
        float p[3];
        pc_debug_player_pos(p);
        const char* anim;
        unsigned int uf, sticky;
        int speed;
        pc_dbg_player_anim(&anim, &uf, &sticky, &speed);
        pc_log("[pos] %.1fs %.2f %.2f %.2f anim %s flags %x sticky %u speed %d", SDL_GetTicks() / 1000.0, p[0], p[1],
               p[2], anim, uf, sticky, speed);
        if (getenv("PC_POSLOG")[0] == '3')
        {
            pc_dbg_player_colls();
        }
    }
    // Debug: PC_TELEPORT=<seconds>:x,y,z moves the player there.
    static const char* teleport = getenv("PC_TELEPORT");
    if (teleport && SDL_GetTicks() >= (Uint32)(atof(teleport) * 1000))
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
    if (surfDump && SDL_GetTicks() >= (Uint32)(atof(surfDump) * 1000))
    {
        surfDump = NULL;
        pc_debug_surfaces();
    }
    // Debug: PC_FORCESAVE=<seconds> fires the event that opens the save screen.
    static const char* forceSave = getenv("PC_FORCESAVE");
    if (forceSave && SDL_GetTicks() >= (Uint32)(atof(forceSave) * 1000))
    {
        forceSave = NULL;
        zGameStateSwitchEvent(0x97);
    }
    // PC_FPTRAP=N: trap invalid FP operations from the Nth presented frame on.
    static int frame, trapAt = getenv("PC_FPTRAP") ? atoi(getenv("PC_FPTRAP")) : -1;
    if (++frame == trapAt)
    {
        pc_fptrap();
    }
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
        {
            SDL_Quit();
            exit(0);
        }
        if (pc_debug_menu_event(e))
        {
            continue;
        }
        // Alt+Enter or F11 toggles borderless fullscreen.
        if (e.type == SDL_KEYDOWN && !e.key.repeat &&
            (e.key.keysym.sym == SDLK_F11 || (e.key.keysym.sym == SDLK_RETURN && (e.key.keysym.mod & KMOD_ALT))))
        {
            SDL_Window* w = SDL_GetWindowFromID(e.key.windowID);
            if (w)
            {
                bool full = (SDL_GetWindowFlags(w) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
                SDL_SetWindowFullscreen(w, full ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
            }
        }
    }
}

// Logs each code address that performs an invalid x87 operation (NaN creation
// or a stack overflow), once, then lets execution continue.
static LONG CALLBACK fp_trap_handler(EXCEPTION_POINTERS* ep)
{
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_FLT_INVALID_OPERATION && code != EXCEPTION_FLT_STACK_CHECK)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    CONTEXT* c = ep->ContextRecord;
    static DWORD seen[256];
    static int nseen;
    // Report addresses relative to the default image base so addr2line works.
    DWORD ip = c->FloatSave.ErrorOffset - (DWORD)GetModuleHandleA(NULL) + 0x400000;
    int known = 0;
    for (int i = 0; i < nseen; i++)
    {
        known |= seen[i] == ip;
    }
    if (!known && nseen < 256)
    {
        seen[nseen++] = ip;
        fprintf(stderr, "[fptrap] %s at 0x%08lx (status 0x%04lx top %lu)\n",
                code == EXCEPTION_FLT_STACK_CHECK ? "stack fault" : "invalid op", ip,
                c->FloatSave.StatusWord & 0xffff, (c->FloatSave.StatusWord >> 11) & 7);
    }
    c->FloatSave.StatusWord &= ~0x80ffu; // clear exception flags, ES and B
    return EXCEPTION_CONTINUE_EXECUTION;
}

// Debug: trap invalid floating-point operations on this thread.
extern "C" void pc_fptrap()
{
    static bool installed;
    if (!installed)
    {
        installed = true;
        AddVectoredExceptionHandler(1, fp_trap_handler);
    }
    _clearfp();
    _controlfp(_controlfp(0, 0) & ~_EM_INVALID, _MCW_EM);
}

int main(int argc, char** argv)
{
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
    {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    setvbuf(stderr, NULL, _IONBF, 0);
    SetUnhandledExceptionFilter(crash_handler);
    pc_map_gc_memory();
    scooby_main();
    SDL_Quit();
    return 0;
}

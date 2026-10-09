// New 3DS entry point and platform hooks (the PC equivalents are in pc/src/main.cpp).
#include <3ds.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "pc_platform.h"

void scooby_main();

// The game's folder on the SD card: game files in files/, saves in save/,
// the log in log.txt.
#define N3DS_BASE "sdmc:/scooby/"

// The main thread runs the whole game; the default 32 KB stack is too small.
extern "C" {
u32 __stacksize__ = 1024 * 1024;
}

static u64 s_startMs;

extern "C" const char* pc_base_path() { return N3DS_BASE; }

extern "C" void pc_mkdir(const char* path) { mkdir(path, 0777); }

static void thread_entry(void* fn)
{
    ((void (*)(void))fn)();
}

extern "C" void pc_thread_start(void (*fn)(void), const char* name)
{
    s32 prio = 0x30;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    Thread t = threadCreate(thread_entry, (void*)fn, 64 * 1024, prio - 1, 0, true);
    pc_log("[n3ds] thread %s: %s (priority 0x%lx)", name, t ? "started" : "FAILED", (long)(prio - 1));
}

extern "C" unsigned int pc_ticks_ms() { return (unsigned int)(osGetTime() - s_startMs); }

extern "C" void pc_exit(int code)
{
    fflush(stderr);
    gfxExit();
    exit(code);
}

// Called by generated stubs for library functions with no 3DS implementation yet.
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
    pc_log("[stub] %s", name);
}

// Called once per presented frame: system events, quit on Start+Select.
void pc_events_pump()
{
    if (!aptMainLoop())
    {
        pc_exit(0);
    }
    hidScanInput();
    u32 held = hidKeysHeld();
    if ((held & (KEY_START | KEY_SELECT)) == (KEY_START | KEY_SELECT))
    {
        pc_log("[n3ds] Start+Select: exiting");
        pc_exit(0);
    }
}

// No movie player on 3DS yet: movies are skipped.
signed int iFMVPlay(char* filename, unsigned int buttons, float time) { return 0; }

// The debug menu (pc/src/debug_menu.cpp) is PC-only for now.
extern "C" int pc_debug_menu_active() { return 0; }

int main(int argc, char** argv)
{
    s_startMs = osGetTime();
    osSetSpeedupEnable(true);
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);
    mkdir(N3DS_BASE, 0777);
    // pc_log writes to stderr: keep it in a file (the console shows stdout).
    freopen(N3DS_BASE "log.txt", "w", stderr);
    setvbuf(stderr, NULL, _IOLBF, 1024);
    setenv("SCOOBY_DATA", N3DS_BASE "files", 1);
    // Debug switches (the PC port's PC_* environment variables): KEY=VALUE
    // lines in env.txt.
    if (FILE* env = fopen(N3DS_BASE "env.txt", "r"))
    {
        char line[512];
        while (fgets(line, sizeof(line), env))
        {
            line[strcspn(line, "\r\n")] = 0;
            char* eq = strchr(line, '=');
            if (eq && line[0] != '#')
            {
                *eq = 0;
                setenv(line, eq + 1, 1);
                pc_log("[n3ds] env %s=%s", line, eq + 1);
            }
        }
        fclose(env);
    }
    bool n3ds = false;
    APT_CheckNew3DS(&n3ds);
    printf("Scooby-Doo! Night of 100 Frights\nNew 3DS port (experimental)\n\n");
    pc_log("[n3ds] start: new3ds %d, app memory %lu KB, linear free %lu KB", n3ds,
           (unsigned long)(osGetMemRegionSize(MEMREGION_APPLICATION) / 1024), (unsigned long)(linearSpaceFree() / 1024));
    if (!n3ds)
    {
        printf("This port needs a New 3DS or New 2DS XL.\nPress Start to exit.\n");
        while (aptMainLoop())
        {
            hidScanInput();
            if (hidKeysDown() & KEY_START)
            {
                break;
            }
            gspWaitForVBlank();
        }
        gfxExit();
        return 0;
    }
    scooby_main();
    pc_exit(0);
    return 0;
}

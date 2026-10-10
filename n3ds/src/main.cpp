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

// Where the converted movies are: the CIA's RomFS when it carries them, else
// the SD card.
static const char* s_fmvDir = N3DS_BASE "fmv/";
extern "C" const char* n3ds_fmv_dir() { return s_fmvDir; }

static bool is_dir(const char* path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

extern "C" void pc_mkdir(const char* path) { mkdir(path, 0777); }

static void thread_entry(void* fn)
{
    ((void (*)(void))fn)();
}

extern "C" void pc_thread_start(void (*fn)(void), const char* name)
{
    s32 prio = 0x30;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    // Core 2 is the New 3DS's extra application core: helpers there don't take
    // time from the game on core 0. Fall back to core 0 above the game's priority.
    int core = 2;
    Thread t = threadCreate(thread_entry, (void*)fn, 64 * 1024, prio - 1, core, true);
    if (!t)
    {
        core = 0;
        t = threadCreate(thread_entry, (void*)fn, 64 * 1024, prio - 1, core, true);
    }
    pc_log("[n3ds] thread %s: %s on core %d (priority 0x%lx)", name, t ? "started" : "FAILED", core, (long)(prio - 1));
}

extern "C" unsigned long long pc_perf_counter() { return svcGetSystemTick(); }
extern "C" double pc_perf_seconds(unsigned long long t) { return (double)t / SYSCLOCK_ARM11; }

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
    // Debug menu: Select opens and closes it; D-pad, A and B drive it.
    // ZL+X saves the position, ZL+Y returns to it (pad.cpp hides those from the game).
    u32 down = hidKeysDown();
    if (down & KEY_SELECT)
    {
        pc_debug_menu_toggle();
    }
    else if ((held & KEY_ZL) && (down & KEY_X))
    {
        pc_debug_coords_save();
    }
    else if ((held & KEY_ZL) && (down & KEY_Y))
    {
        pc_debug_coords_return();
    }
    else if (pc_debug_menu_active())
    {
        if (down & KEY_DUP) pc_debug_menu_key(PC_MENU_UP);
        if (down & KEY_DDOWN) pc_debug_menu_key(PC_MENU_DOWN);
        if (down & KEY_DLEFT) pc_debug_menu_key(PC_MENU_LEFT);
        if (down & KEY_DRIGHT) pc_debug_menu_key(PC_MENU_RIGHT);
        if (down & KEY_A) pc_debug_menu_key(PC_MENU_OK);
        if (down & KEY_B) pc_debug_menu_key(PC_MENU_BACK);
    }
    pc_debug_menu_frame();
    pc_debug_hooks_frame();
}

extern "C" void pc_set_clipboard(const char* text) {}

// Sampling profiler support (n3ds/tools/sample.sh): N3DS_SAMPLE=<seconds> starts
// a thread that calls n3ds_sample_point four times a second after that warm-up;
// the debugger breaks there and records the main thread's stack.
extern "C" __attribute__((noinline)) void n3ds_sample_point()
{
    __asm__ volatile("" ::: "memory");
}

static float s_sampleWarmup;

static void sample_thread()
{
    svcSleepThread((s64)(s_sampleWarmup * 1e9f));
    for (;;)
    {
        n3ds_sample_point();
        svcSleepThread(250000000LL);
    }
}

// The renderer takes the bottom screen for the HUD: the text console stops
// there (anything printed goes to the log) and the screen gets the renderer's
// format back.
extern "C" void n3ds_console_off()
{
    fflush(stdout);
    if (freopen(N3DS_BASE "log.txt", "a", stdout))
    {
        setvbuf(stdout, NULL, _IOLBF, 1024);
    }
    gfxSetScreenFormat(GFX_BOTTOM, GSP_BGR8_OES);
    gfxSetDoubleBuffering(GFX_BOTTOM, true);
}

extern "C" const char* pc_debug_key_hint(int which)
{
    return which == PC_HINT_MENU ? "SELECT" : which == PC_HINT_SAVE ? "ZL+X" : "ZL+Y";
}



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
    // Game files and movies: inside the CIA (RomFS) when it was built with them
    // (n3ds/tools/make_cia.sh), else on the SD card.
    bool romfs = R_SUCCEEDED(romfsInit());
    if (romfs && is_dir("romfs:/files"))
    {
        setenv("SCOOBY_DATA", "romfs:/files", 1);
    }
    else
    {
        setenv("SCOOBY_DATA", N3DS_BASE "files", 1);
    }
    if (romfs && is_dir("romfs:/fmv"))
    {
        s_fmvDir = "romfs:/fmv/";
    }
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
    pc_log("[n3ds] data in %s, movies in %s", getenv("SCOOBY_DATA"), s_fmvDir);
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
    if (const char* sample = getenv("N3DS_SAMPLE"))
    {
        s_sampleWarmup = (float)atof(sample);
        pc_thread_start(sample_thread, "sample");
    }
    scooby_main();
    pc_exit(0);
    return 0;
}

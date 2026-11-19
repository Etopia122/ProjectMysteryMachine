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

// Logs and crash reports live in %APPDATA%\ProjectMysteryMachine\logs, not
// beside the executable: the release build is a GUI program, so it has no
// console to read, and it may sit in a folder the player cannot write to.
// The launcher's "Open log folder" button opens the same directory.
static const char* log_directory()
{
    static char dir[MAX_PATH];
    if (!dir[0])
    {
        char appdata[MAX_PATH];
        if (GetEnvironmentVariableA("APPDATA", appdata, MAX_PATH) && appdata[0])
        {
            _snprintf(dir, sizeof(dir), "%s\\ProjectMysteryMachine\\logs", appdata);
        }
        else
        {
            char exe[MAX_PATH];
            GetModuleFileNameA(NULL, exe, MAX_PATH);
            char* slash = strrchr(exe, '\\');
            if (slash)
            {
                *slash = 0;
            }
            _snprintf(dir, sizeof(dir), "%s\\logs", exe);
        }
        dir[sizeof(dir) - 1] = 0;
    }
    return dir;
}

static void ensure_directory(const char* path)
{
    char tmp[MAX_PATH];
    lstrcpynA(tmp, path, MAX_PATH);
    for (char* p = tmp + 1; *p; p++)
    {
        if (*p == '\\')
        {
            *p = 0;
            CreateDirectoryA(tmp, NULL);
            *p = '\\';
        }
    }
    CreateDirectoryA(tmp, NULL);
}

// Opens this session's log. If the previous one grew past 2 MB it is kept as
// scooby.1.log, so a crash still has the run that led up to it.
static void pc_log_init()
{
    const char* dir = log_directory();
    ensure_directory(dir);
    char path[MAX_PATH], old[MAX_PATH];
    _snprintf(path, sizeof(path), "%s\\scooby.log", dir);
    _snprintf(old, sizeof(old), "%s\\scooby.1.log", dir);
    path[sizeof(path) - 1] = old[sizeof(old) - 1] = 0;
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (GetFileAttributesExA(path, GetFileExInfoStandard, &info) &&
        (((unsigned long long)info.nFileSizeHigh << 32) | info.nFileSizeLow) > 2ull * 1024 * 1024)
    {
        DeleteFileA(old);
        MoveFileA(path, old);
    }
    pc_log_stream = fopen(path, "a");
    if (!pc_log_stream)
    {
        return;
    }
    SYSTEMTIME t;
    GetLocalTime(&t);
    char exe[MAX_PATH] = "?";
    GetModuleFileNameA(NULL, exe, MAX_PATH);
    fprintf(pc_log_stream, "--- Project: Mystery Machine %04u-%02u-%02u %02u:%02u:%02u ---\n",
            t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    fprintf(pc_log_stream, "exe: %s\n", exe);
    fflush(pc_log_stream);
}

static const char* exception_name(DWORD code)
{
    switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION: return "access violation";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "array bounds exceeded";
    case EXCEPTION_DATATYPE_MISALIGNMENT: return "datatype misalignment";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "float divide by zero";
    case EXCEPTION_FLT_INVALID_OPERATION: return "invalid float operation";
    case EXCEPTION_ILLEGAL_INSTRUCTION: return "illegal instruction";
    case EXCEPTION_INT_DIVIDE_BY_ZERO: return "integer divide by zero";
    case EXCEPTION_PRIV_INSTRUCTION: return "privileged instruction";
    case EXCEPTION_STACK_OVERFLOW: return "stack overflow";
    case 0xE06D7363: return "C++ exception";
    default: return "unknown";
    }
}

// One frame: the module and offset, then the address relative to the default
// image base so `addr2line -f -C -e scooby.exe <addr>` names it.
static void log_frame(FILE* o, DWORD addr)
{
    HMODULE base = GetModuleHandleA(NULL), m = NULL;
    if (!addr || !GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                         GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                     (LPCSTR)addr, &m))
    {
        m = NULL;
    }
    if (m)
    {
        char name[MAX_PATH] = "?";
        GetModuleFileNameA(m, name, MAX_PATH);
        const char* file = strrchr(name, '\\');
        fprintf(o, "  %08lx  %s+0x%lx", addr, file ? file + 1 : name, addr - (DWORD)m);
    }
    else
    {
        fprintf(o, "  %08lx", addr);
    }
    // Only our own code has the image-relative form addr2line understands.
    if (m == base)
    {
        fprintf(o, "  (image+%08lx)\n", addr - (DWORD)base + 0x400000);
    }
    else
    {
        fputc('\n', o);
    }
}

// Writes a crash report to the log folder: the exception, the faulting
// address, the registers and a frame-pointer stack walk.
static LONG WINAPI crash_handler(EXCEPTION_POINTERS* ep)
{
    SYSTEMTIME t;
    GetLocalTime(&t);
    char path[MAX_PATH];
    _snprintf(path, sizeof(path), "%s\\crash-%04u%02u%02u-%02u%02u%02u.log", log_directory(), t.wYear,
              t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    path[sizeof(path) - 1] = 0;
    // The session log holds what the game was doing; get it on disk first.
    if (pc_log_stream)
    {
        fflush(pc_log_stream);
    }
    FILE* f = fopen(path, "w");
    FILE* outs[2] = { stderr, f };
    CONTEXT* c = ep->ContextRecord;
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    for (FILE* o : outs)
    {
        if (!o)
        {
            continue;
        }
        fprintf(o, "Project: Mystery Machine crashed %04u-%02u-%02u %02u:%02u:%02u\n", t.wYear, t.wMonth,
                t.wDay, t.wHour, t.wMinute, t.wSecond);
        fprintf(o, "exception %08lx (%s) at %08lx\n", code, exception_name(code), c->Eip);
        log_frame(o, c->Eip);
        if (code == EXCEPTION_ACCESS_VIOLATION)
        {
            fprintf(o, "  %s address %08lx\n", ep->ExceptionRecord->ExceptionInformation[0] ? "writing" : "reading",
                    (DWORD)ep->ExceptionRecord->ExceptionInformation[1]);
        }
        fprintf(o, "registers: eax %08lx ebx %08lx ecx %08lx edx %08lx\n", (DWORD)c->Eax, (DWORD)c->Ebx,
                (DWORD)c->Ecx, (DWORD)c->Edx);
        fprintf(o, "           esi %08lx edi %08lx esp %08lx ebp %08lx\n", (DWORD)c->Esi, (DWORD)c->Edi,
                (DWORD)c->Esp, (DWORD)c->Ebp);
        fprintf(o, "session log: %s\\scooby.log\n", log_directory());
        fprintf(o, "stack:\n");
        DWORD* fp = (DWORD*)c->Ebp;
        for (int i = 0; i < 48 && fp && !IsBadReadPtr(fp, 8); i++)
        {
            DWORD ret = fp[1];
            log_frame(o, ret);
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

extern "C" unsigned long long pc_perf_counter() { return SDL_GetPerformanceCounter(); }
extern "C" double pc_perf_seconds(unsigned long long t) { return (double)t / SDL_GetPerformanceFrequency(); }

extern "C" unsigned int pc_ticks_ms()
{
    return SDL_GetTicks();
}

extern "C" void pc_exit(int code)
{
    SDL_Quit();
    if (pc_log_stream)
    {
        fclose(pc_log_stream);
        pc_log_stream = NULL;
    }
    exit(code);
}

bool pc_debug_menu_event(const SDL_Event& e);


// Window/system events; called once per presented frame.
void pc_events_pump()
{
    pc_debug_menu_frame();
    pc_debug_hooks_frame();
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
    pc_log_init();
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
    if (pc_log_stream)
    {
        fclose(pc_log_stream);
        pc_log_stream = NULL;
    }
    return 0;
}

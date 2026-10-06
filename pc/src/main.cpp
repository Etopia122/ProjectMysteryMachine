// PC entry point: sets up the platform, then runs the game's main loop.
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <windows.h>

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

// Window/system events; called once per presented frame.
void pc_events_pump()
{
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
    pc_map_gc_memory();
    scooby_main();
    SDL_Quit();
    return 0;
}

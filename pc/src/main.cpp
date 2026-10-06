// PC entry point: sets up the platform, then runs the game's main loop.
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// Window/system events; called once per presented frame.
void pc_events_pump()
{
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

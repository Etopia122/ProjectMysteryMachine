extern "C" int strcmp(const char* a, const char* b);

static char dm_names[14][16] = {
    "",       "dm_collision", "dm_anim", "dm_ent",    "dm_shadow", "dm_snd", "dm_graph",
    "dm_gui", "dm_par",       "dm_fade", "dm_render", "dm_mem",    "dm_vram", "dm_pad",
};
static void (*sDebugModeFunc[46])();
char* sDebugModeName[46];

inline signed int xDebugModeAdd(char* debugModeName)
{
    signed int i;

    for (i = 14; i < 46; i++)
    {
        if (sDebugModeName[i] && !strcmp(sDebugModeName[i], debugModeName))
        {
            return -1;
        }
    }
    for (i = 14; i < 46; i++)
    {
        if (!sDebugModeName[i])
        {
            sDebugModeName[i] = debugModeName;
            return i;
        }
    }
    return -1;
}

void xDebugModeRegisterCallback(signed int mode, void (*func)());

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xprintf(char* fmt, ...)
{
}

signed int xDebugModeAdd(char* debugModeName, void (*func)())
{
    signed int mode = xDebugModeAdd(debugModeName);

    if (mode >= 0)
    {
        xDebugModeRegisterCallback(mode, func);
    }
    return mode;
}

#pragma auto_inline on
void xDebugModeRegisterCallback(signed int mode, void (*func)())
{
    sDebugModeFunc[mode] = func;
}
#pragma auto_inline off

void xDebugInit()
{
    for (signed int i = 0; i < 46; i++)
    {
        sDebugModeFunc[i] = 0;
        if (i < 14)
        {
            sDebugModeName[i] = dm_names[i];
        }
        else
        {
            sDebugModeName[i] = 0;
        }
    }
}

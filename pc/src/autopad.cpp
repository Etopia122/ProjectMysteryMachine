// Test automation: PC_AUTOPAD="sec:hexmask[:sx:sy],..." holds GameCube buttons
// (and optionally the main stick) from each time on, counted from the first read.
#include "pc_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" void pc_autopad(unsigned short* buttons, int* sx, int* sy)
{
    static const char* script = getenv("PC_AUTOPAD");
    if (!script)
    {
        return;
    }
    static unsigned int t0 = pc_ticks_ms();
    float now = (pc_ticks_ms() - t0) / 1000.0f;
    unsigned int mask = 0;
    int ax = 0, ay = 0;
    for (const char* q = script; q && *q;)
    {
        float t;
        unsigned int m;
        int x = 0, y = 0, n = 0;
        if (sscanf(q, "%f:%x%n", &t, &m, &n) < 2)
        {
            break;
        }
        const char* r = q + n;
        int n2 = 0;
        if (sscanf(r, ":%d:%d%n", &x, &y, &n2) == 2)
        {
            r += n2;
        }
        if (t <= now)
        {
            mask = m;
            ax = x;
            ay = y;
        }
        q = strchr(r, ',');
        q = q ? q + 1 : NULL;
    }
    *buttons |= (unsigned short)mask;
    if (ax)
    {
        *sx = ax;
    }
    if (ay)
    {
        *sy = ay;
    }
}

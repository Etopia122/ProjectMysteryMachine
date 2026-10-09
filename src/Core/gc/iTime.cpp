#include "iTime.h"

extern "C" iTime OSGetTime();

#ifdef PC_PORT
// The GameCube OS keeps the bus clock in its globals page; ports can't map it.
#define GET_BUS_FREQUENCY() (162000000u)
#else
#define GET_BUS_FREQUENCY() (*(unsigned int*)0x800000F8)
#endif

static iTime sStartupTime;
static float sGameTime;

// Functions are in reverse DOL order (-inline deferred emits last to first).

void iTimeInit()
{
    sStartupTime = OSGetTime();
}

void iTimeExit()
{
}

iTime iTimeGet()
{
    return OSGetTime() - sStartupTime;
}

#pragma auto_inline on
float iTimeDiffSec(iTime time)
{
    return (float)time / (GET_BUS_FREQUENCY() / 4);
}
#pragma auto_inline off

float iTimeDiffSec(iTime t0, iTime t1)
{
    return iTimeDiffSec(t1 - t0);
}

void iTimeGameAdvance(float elapsed)
{
    sGameTime += elapsed;
}

void iTimeSetGame(float time)
{
    sGameTime = time;
}

void iProfileClear(unsigned int sceneID)
{
}

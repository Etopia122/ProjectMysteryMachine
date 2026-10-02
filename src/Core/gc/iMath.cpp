#include "iMath.h"
#include "iTime.h"

// Like xMath.cpp, this TU inlines its own functions (irand into iurand and
// iMathInit) although the game builds with -inline noauto.
#pragma auto_inline on

static unsigned int rndseed;

// Functions are in reverse DOL order (-inline deferred emits last to first).

void iMathInit()
{
    rndseed = iTimeGet();
    iTimeGet();
    irand();
}

void iMathExit()
{
}

void isrand(unsigned int seed)
{
    rndseed = seed;
}

unsigned int irand()
{
    rndseed = rndseed * 1103515245 + 12345;
    return rndseed;
}

float iurand()
{
    return irand() * 2.3283064e-10f;
}

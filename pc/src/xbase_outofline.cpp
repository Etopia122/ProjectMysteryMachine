// Out-of-line xBaseEnable/xBaseDisable/xBaseIsEnabled for the ports.
// Some game sources call them as ordinary functions (XBASE_ENABLE_OUTOFLINE);
// on GameCube zEnt.cpp emits them, but the ports use xBase.h's inline versions
// there. Unoptimised builds happened to emit copies of those; optimised ones
// (the New 3DS port) don't, so define them here, weak so emitted copies win.
#define XBASE_ENABLE_OUTOFLINE
#include "xBase.h"

__attribute__((weak)) unsigned int xBaseIsEnabled(xBase* xb)
{
    return xb->baseFlags & 1;
}

__attribute__((weak)) void xBaseEnable(xBase* xb)
{
    xb->baseFlags |= 1;
}

__attribute__((weak)) void xBaseDisable(xBase* xb)
{
    xb->baseFlags &= ~1;
}

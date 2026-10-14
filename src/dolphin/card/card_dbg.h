#ifndef CARD_DBG_H
#define CARD_DBG_H

// Scooby links the debug build of the CARD library: every ASSERTLINE is live and
// panics with the bare source file name. Line numbers are the ones in the
// original strings/code, not TWW's. The library was built with -O0 plus
// debug info and the peephole pass (configure.py: -O0,s -sym on -opt peephole);
// debug info keeps peephole within statements, which gives the original's
// `addi rX, rY, 0` copies without merging copies into later compares.
// In debug builds the OS address-translation macros are real functions.
#if defined(VERSION_GIHE78)
#include "dolphin/os/OS.h"
#undef ASSERTLINE
#define ASSERTLINE(line, cond) \
    if (!(cond))               \
    OSPanic(CARD_FILE, line, "Failed assertion " #cond)
#undef OSPhysicalToCached
#undef OSCachedToPhysical
void* OSPhysicalToCached(u32 paddr);
u32 OSCachedToPhysical(void* caddr);
#endif

#endif

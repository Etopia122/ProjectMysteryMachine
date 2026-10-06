#ifndef CARD_DBG_H
#define CARD_DBG_H

// Scooby links the debug build of the CARD library (built without
// optimization): every ASSERTLINE is live and panics with the bare source file
// name. Line numbers are the ones in the original strings/code, not TWW's.
#if defined(VERSION_GIHE78)
#include "dolphin/os/OS.h"
#undef ASSERTLINE
#define ASSERTLINE(line, cond) \
    if (!(cond))               \
    OSPanic(CARD_FILE, line, "Failed assertion " #cond)
#endif

#endif

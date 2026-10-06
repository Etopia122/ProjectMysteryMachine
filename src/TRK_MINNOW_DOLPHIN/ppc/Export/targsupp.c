#include "TRK_MINNOW_DOLPHIN/ppc/Export/targsupp.h"

// The file requests trap into the debugger, which services them.

asm u32 TRKAccessFile(u32, u32, u32*, u8*) {
    // clang-format off
    nofralloc
    twi 31, r0, 0x0
    blr
    // clang-format on
}

asm u32 TRKOpenFile(u32 command, const char* name, u8 mode, u32* handle) {
    // clang-format off
    nofralloc
    twi 31, r0, 0x0
    blr
    // clang-format on
}

asm u32 TRKCloseFile(u32, u32) {
    // clang-format off
    nofralloc
    twi 31, r0, 0x0
    blr
    // clang-format on
}

asm u32 TRKPositionFile(u32 command, u32 handle, u32* position, u8 mode) {
    // clang-format off
    nofralloc
    twi 31, r0, 0x0
    blr
    // clang-format on
}

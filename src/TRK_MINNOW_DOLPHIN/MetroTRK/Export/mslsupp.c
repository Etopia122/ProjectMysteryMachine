#include "TRK_MINNOW_DOLPHIN/MetroTRK/Portable/msghndlr.h"
#include "TRK_MINNOW_DOLPHIN/Os/dolphin/target_options.h"
#include "TRK_MINNOW_DOLPHIN/ppc/Export/targsupp.h"

// Older MetroTRK MSL support: besides the console hooks it implements the MSL
// disk-file procs (open/close/position/read/write) over the TRK file requests.

typedef struct TRKFileModes {
    unsigned int open_mode : 2;
    unsigned int io_mode : 3;
    unsigned int buffer_mode : 2;
    unsigned int file_kind : 3;
    unsigned int file_orientation : 2;
    unsigned int binary_io : 1;
} TRKFileModes;

DSIOResult __read_file(u32 handle, u8* buffer, size_t* count, void* ref_con);
DSIOResult __write_file(u32 handle, u8* buffer, size_t* count, void* ref_con);
DSIOResult __access_file(u32 handle, u8* buffer, size_t* count, void* ref_con,
                         MessageCommandID cmd);

DSIOResult __read_console(u32 handle, u8* buffer, size_t* count, void* ref_con) {
    if (GetUseSerialIO() == 0) {
        return DS_IOError;
    }
    return __read_file(DS_Stdin, buffer, count, ref_con);
}

DSIOResult __TRK_write_console(u32 handle, u8* buffer, size_t* count, void* ref_con) {
    if (GetUseSerialIO() == 0) {
        return DS_IOError;
    }
    return __write_file(DS_Stdout, buffer, count, ref_con);
}

DSIOResult __close_console(u32 handle) {
    u32 r;

    if (GetTRKConnected() == DS_NoError) {
        return DS_IOError;
    }

    r = TRKCloseFile(DSMSG_CloseFile, handle);

    switch ((u8)r) {
    case DS_IONoError:
        return DS_IONoError;
    case DS_IOEOF:
        return DS_IOEOF;
    }

    return DS_IOError;
}

DSIOResult __read_file(u32 handle, u8* buffer, size_t* count, void* ref_con) {
    return __access_file(handle, buffer, count, ref_con, DSMSG_ReadFile);
}

DSIOResult __write_file(u32 handle, u8* buffer, size_t* count, void* ref_con) {
    return __access_file(handle, buffer, count, ref_con, DSMSG_WriteFile);
}

static int __trk_open_mode(TRKFileModes* mode);

DSIOResult __open_file(const char* name, TRKFileModes mode, u32* handle) {
    u32 r;
    int trkMode;

    if (GetTRKConnected() == DS_NoError) {
        return DS_IOError;
    }

    trkMode = __trk_open_mode(&mode);
    r = TRKOpenFile(DSMSG_OpenFile, name, trkMode, handle);

    switch ((u8)r) {
    case DS_IONoError:
        return DS_IONoError;
    case DS_IOEOF:
        return DS_IOEOF;
    }

    return DS_IOError;
}

DSIOResult __close_file(u32 handle) {
    u32 r;

    if (GetTRKConnected() == DS_NoError) {
        return DS_IOError;
    }

    r = TRKCloseFile(DSMSG_CloseFile, handle);

    switch ((u8)r) {
    case DS_IONoError:
        return DS_IONoError;
    case DS_IOEOF:
        return DS_IOEOF;
    }

    return DS_IOError;
}

DSIOResult __position_file(u32 handle, u32* position, int mode, void* idle_proc) {
    int trkMode = 0;
    u32 r;

    if (GetTRKConnected() == DS_NoError) {
        return DS_IOError;
    }

    if (mode == 0) {
        trkMode = 0;
    } else if (mode == 1) {
        trkMode = 1;
    } else if (mode == 2) {
        trkMode = 2;
    }

    r = TRKPositionFile(DSMSG_PositionFile, handle, position, trkMode);

    switch ((u8)r) {
    case DS_IONoError:
        return DS_IONoError;
    case DS_IOEOF:
        return DS_IOEOF;
    }

    return DS_IOError;
}

#pragma dont_inline on
static int __trk_open_mode(TRKFileModes* mode) {
    int trkMode = 0;
    int openMode = mode->open_mode;
    int ioMode = mode->io_mode;
    u32 binaryIO = mode->binary_io;

    switch (openMode) {
    case 0:
        trkMode |= 1;
        break;
    case 2:
        trkMode |= 2;
        break;
    case 1:
        trkMode |= 4;
        break;
    }

    switch (ioMode) {
    case 1:
        trkMode |= 1;
        break;
    case 2:
        trkMode |= 2;
        break;
    case 6:
        trkMode |= 4;
        break;
    case 3:
        trkMode |= 0x12;
        break;
    case 7:
        trkMode |= 7;
        break;
    }

    if (binaryIO == 1) {
        trkMode = (trkMode | 8) & 0xFF;
    }

    return trkMode;
}
#pragma dont_inline reset

DSIOResult __access_file(u32 handle, u8* buffer, size_t* count, void* ref_con,
                         MessageCommandID cmd) {
    size_t countTemp;
    u32 r0;

    if (GetTRKConnected() == DS_NoError) {
        return DS_IOError;
    }

    countTemp = *count;
    r0 = TRKAccessFile(cmd, handle, &countTemp, buffer);
    *count = countTemp;

    switch ((u8)r0) {
    case DS_IONoError:
        return DS_IONoError;
    case DS_IOEOF:
        return DS_IOEOF;
    }

    return DS_IOError;
}

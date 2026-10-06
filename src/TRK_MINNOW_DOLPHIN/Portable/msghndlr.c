#include "TRK_MINNOW_DOLPHIN/MetroTRK/Portable/msghndlr.h"
#include "TRK_MINNOW_DOLPHIN/MetroTRK/Portable/nubevent.h"
#include "TRK_MINNOW_DOLPHIN/utils/common/MWTrace.h"
#include "trk.h"
#include "string.h"

// Older MetroTRK message handlers: replies are built by appending fields to the
// request buffer and sent with up to three retries.

typedef struct TRKVersionsOld {
    u8 kernelMajor;
    u8 kernelMinor;
    u8 protocolMajor;
    u8 protocolMinor;
} TRKVersionsOld;

typedef struct TRKCPUTypeOld {
    u8 cpuMajor;
    u8 cpuMinor;
    u8 bigEndian;
    u8 defaultTypeSize;
    u8 fpTypeSize;
    u8 extended1TypeSize;
    u8 extended2TypeSize;
} TRKCPUTypeOld;

extern int TRKMessageSend(TRKBuffer* buffer);
extern void __TRK_copy_vectors(void);
extern int TRKTargetVersions(TRKVersionsOld* versions);
extern int TRKTargetCPUType(TRKCPUTypeOld* cpuType);
extern int TRKTargetSupportMask(u8 mask[32]);

static BOOL IsTRKConnected;

// TRKAppendBuffer1_ui8 is inlined at every call site in this TRK version.
static inline int msghndlr_Append1_ui8(TRKBuffer* buffer, const u8 data) {
    if (buffer->position >= 0x880) {
        return DS_MessageBufferOverflow;
    }

    buffer->data[buffer->position++] = data;
    buffer->length++;
    return DS_NoError;
}

static inline DSError TRKSendACK(TRKBuffer* buffer) {
    DSError err;
    int retries = 3;

    do {
        err = TRKMessageSend(buffer);
        --retries;
    } while (err != DS_NoError && retries > 0);

    return err;
}

static inline void TRKMessageReplyHeader(TRKBuffer* buffer, u8 command, u8 error) {
    TRKResetBuffer(buffer, 1);
    msghndlr_Append1_ui8(buffer, command);
    msghndlr_Append1_ui8(buffer, error);
}

BOOL GetTRKConnected(void) {
    return IsTRKConnected;
}

void SetTRKConnected(BOOL isTRKConnected) {
    IsTRKConnected = isTRKConnected;
}

DSError TRKStandardACK(TRKBuffer* buffer, MessageCommandID commandID, DSReplyError replyError) {
    TRKMessageReplyHeader(buffer, commandID, replyError);
    return TRKSendACK(buffer);
}

DSError TRKDoUnsupported(TRKBuffer* buffer) {
    return TRKStandardACK(buffer, 0x80, 0x10);
}

DSError TRKDoConnect(TRKBuffer* buffer) {
    IsTRKConnected = TRUE;
    return TRKStandardACK(buffer, 0x80, 0);
}

DSError TRKDoDisconnect(TRKBuffer* buffer) {
    DSError error;
    TRKEvent event;

    IsTRKConnected = FALSE;
    error = TRKStandardACK(buffer, 0x80, 0);
    if (error == DS_NoError) {
        TRKConstructEvent(&event, 1);
        TRKPostEvent(&event);
    }
    return error;
}

DSError TRKDoReset(TRKBuffer* buffer) {
    TRKStandardACK(buffer, 0x80, 0);
    __TRK_copy_vectors();
    return DS_NoError;
}

DSError TRKDoVersions(TRKBuffer* buffer) {
    int error;
    TRKVersionsOld versions;

    if (buffer->length != 1) {
        return TRKStandardACK(buffer, 0x80, 2);
    }

    TRKMessageReplyHeader(buffer, 0x80, 0);

    error = TRKTargetVersions(&versions);
    if (error == DS_NoError) {
        error = msghndlr_Append1_ui8(buffer, versions.kernelMajor);
    }
    if (error == DS_NoError) {
        error = msghndlr_Append1_ui8(buffer, versions.kernelMinor);
    }
    if (error == DS_NoError) {
        error = msghndlr_Append1_ui8(buffer, versions.protocolMajor);
    }
    if (error == DS_NoError) {
        error = msghndlr_Append1_ui8(buffer, versions.protocolMinor);
    }

    if (error != DS_NoError) {
        return TRKStandardACK(buffer, 0x80, 3);
    }

    return TRKSendACK(buffer);
}

DSError TRKDoSupportMask(TRKBuffer* buffer) {
    int error;
    u8 mask[32];

    if (buffer->length != 1) {
        return TRKStandardACK(buffer, 0x80, 2);
    }

    TRKMessageReplyHeader(buffer, 0x80, 0);

    error = TRKTargetSupportMask(mask);
    if (error == DS_NoError) {
        error = TRKAppendBuffer(buffer, mask, 32);
    }
    if (error == DS_NoError) {
        error = msghndlr_Append1_ui8(buffer, 2);
    }

    if (error != DS_NoError) {
        return TRKStandardACK(buffer, 0x80, 3);
    }

    return TRKSendACK(buffer);
}

DSError TRKDoCPUType(TRKBuffer* buffer) {
    int error;
    TRKCPUTypeOld cpuType;

    if (buffer->length != 1) {
        return TRKStandardACK(buffer, 0x80, 2);
    }

    TRKMessageReplyHeader(buffer, 0x80, 0);

    error = TRKTargetCPUType(&cpuType);
    if (error == DS_NoError) {
        error = msghndlr_Append1_ui8(buffer, cpuType.cpuMajor);
    }
    if (error == DS_NoError) {
        error = msghndlr_Append1_ui8(buffer, cpuType.cpuMinor);
    }
    if (error == DS_NoError) {
        error = msghndlr_Append1_ui8(buffer, cpuType.bigEndian);
    }
    if (error == DS_NoError) {
        error = msghndlr_Append1_ui8(buffer, cpuType.defaultTypeSize);
    }
    if (error == DS_NoError) {
        error = msghndlr_Append1_ui8(buffer, cpuType.fpTypeSize);
    }
    if (error == DS_NoError) {
        error = msghndlr_Append1_ui8(buffer, cpuType.extended1TypeSize);
    }
    if (error == DS_NoError) {
        error = msghndlr_Append1_ui8(buffer, cpuType.extended2TypeSize);
    }

    if (error != DS_NoError) {
        return TRKStandardACK(buffer, 0x80, 3);
    }

    return TRKSendACK(buffer);
}

extern int TRKTargetAccessMemory(void* data, u32 start, u32* length, int memorySpace, BOOL read);
extern int TRKTargetAccessDefault(u32 firstRegister, u32 lastRegister, TRKBuffer* buffer,
                                  u32* registersLength, BOOL read);
extern int TRKTargetAccessFP(u32 firstRegister, u32 lastRegister, TRKBuffer* buffer,
                             u32* registersLength, BOOL read);
extern int TRKTargetAccessExtended1(u32 firstRegister, u32 lastRegister, TRKBuffer* buffer,
                                    u32* registersLength, BOOL read);
extern int TRKTargetAccessExtended2(u32 firstRegister, u32 lastRegister, TRKBuffer* buffer,
                                    u32* registersLength, BOOL read);
extern int TRKTargetFlushCache(u8 options, u32 start, u32 end);
extern int TRKTargetStop(void);
extern BOOL TRKTargetStopped(void);
extern u32 TRKTargetGetPC(void);
extern int TRKTargetSingleStep(u8 count, BOOL stepOver);
extern int TRKTargetStepOutOfRange(u32 rangeStart, u32 rangeEnd, BOOL stepOver);
extern void SetUseSerialIO(u8 useSerialIO);

DSError TRKDoReadMemory(TRKBuffer* buffer) {
    int error;
    u8 command;
    u8 options;
    u16 length;
    u32 start;
    u32 tempLength;
    u8 buf[0x800];

    if (buffer->length != 8) {
        return TRKStandardACK(buffer, 0x80, 2);
    }

    TRKSetBufferPosition(buffer, 0);
    error = TRKReadBuffer1_ui8(buffer, &command);
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui8(buffer, &options);
    }
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui16(buffer, &length);
    }
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui32(buffer, &start);
    }

    if (options & 2) {
        return TRKStandardACK(buffer, 0x80, 0x12);
    }

    if (length > 0x800) {
        return TRKStandardACK(buffer, 0x80, 0x11);
    }

    TRKMessageReplyHeader(buffer, 0x80, 0);

    if (error == DS_NoError) {
        tempLength = length;
        error = TRKTargetAccessMemory(buf, start, &tempLength, (options & 8) ? 0 : 1, 1);
        length = tempLength;
        if (error == DS_NoError) {
            error = TRKAppendBuffer1_ui16(buffer, length);
        }
        if (error == DS_NoError) {
            error = TRKAppendBuffer(buffer, buf, tempLength);
        }
    }

    if (error != DS_NoError) {
        int replyErr;

        switch (error) {
        case DS_CWDSException:
            replyErr = 0x15;
            break;
        case DS_InvalidMemory:
            replyErr = 0x13;
            break;
        case DS_InvalidProcessID:
            replyErr = 0x21;
            break;
        case DS_InvalidThreadID:
            replyErr = 0x22;
            break;
        case DS_OSError:
            replyErr = 0x20;
            break;
        default:
            replyErr = 3;
            break;
        }

        return TRKStandardACK(buffer, 0x80, replyErr);
    }

    return TRKSendACK(buffer);
}

DSError TRKDoWriteMemory(TRKBuffer* buffer) {
    int error;
    u8 command;
    u8 options;
    u16 length;
    u32 start;
    u32 tempLength;
    u8 buf[0x800];

    if (buffer->length <= 8) {
        return TRKStandardACK(buffer, 0x80, 2);
    }

    TRKSetBufferPosition(buffer, 0);
    error = TRKReadBuffer1_ui8(buffer, &command);
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui8(buffer, &options);
    }
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui16(buffer, &length);
    }
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui32(buffer, &start);
    }

    if (options & 2) {
        return TRKStandardACK(buffer, 0x80, 0x12);
    }

    if (buffer->length != length + 8 || length > 0x800) {
        return TRKStandardACK(buffer, 0x80, 0x11);
    }

    if (error == DS_NoError) {
        tempLength = length;
        error = TRKReadBuffer(buffer, buf, tempLength);
        if (error == DS_NoError) {
            error = TRKTargetAccessMemory(buf, start, &tempLength, (options & 8) ? 0 : 1, 0);
        }
        length = tempLength;
    }

    if (error == DS_NoError) {
        TRKMessageReplyHeader(buffer, 0x80, 0);
    }

    if (error == DS_NoError) {
        error = TRKAppendBuffer1_ui16(buffer, length);
    }

    if (error != DS_NoError) {
        int replyErr;

        switch (error) {
        case DS_CWDSException:
            replyErr = 0x15;
            break;
        case DS_InvalidMemory:
            replyErr = 0x13;
            break;
        case DS_InvalidProcessID:
            replyErr = 0x21;
            break;
        case DS_InvalidThreadID:
            replyErr = 0x22;
            break;
        case DS_OSError:
            replyErr = 0x20;
            break;
        default:
            replyErr = 3;
            break;
        }

        return TRKStandardACK(buffer, 0x80, replyErr);
    }

    return TRKSendACK(buffer);
}

DSError TRKDoReadRegisters(TRKBuffer* buffer) {
    int error;
    u8 command;
    u8 options;
    u16 firstRegister;
    u16 lastRegister;
    u32 registersLength;

    if (buffer->length != 6) {
        return TRKStandardACK(buffer, 0x80, 2);
    }

    TRKSetBufferPosition(buffer, 0);
    error = TRKReadBuffer1_ui8(buffer, &command);
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui8(buffer, &options);
    }
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui16(buffer, &firstRegister);
    }
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui16(buffer, &lastRegister);
    }

    if (firstRegister > lastRegister) {
        return TRKStandardACK(buffer, 0x80, 0x14);
    }

    if (error == DS_NoError) {
        TRKMessageReplyHeader(buffer, 0x80, 0);
    }

    switch (options & 7) {
    case 0:
        error = TRKTargetAccessDefault(firstRegister, lastRegister, buffer, &registersLength, 1);
        break;
    case 1:
        error = TRKTargetAccessFP(firstRegister, lastRegister, buffer, &registersLength, 1);
        break;
    case 2:
        error = TRKTargetAccessExtended1(firstRegister, lastRegister, buffer, &registersLength, 1);
        break;
    case 3:
        error = TRKTargetAccessExtended2(firstRegister, lastRegister, buffer, &registersLength, 1);
        break;
    default:
        error = DS_UnsupportedError;
        break;
    }

    if (error != DS_NoError) {
        int replyErr;

        switch (error) {
        case DS_UnsupportedError:
            replyErr = 0x12;
            break;
        case DS_InvalidRegister:
            replyErr = 0x14;
            break;
        case DS_CWDSException:
            replyErr = 0x15;
            break;
        case DS_InvalidProcessID:
            replyErr = 0x21;
            break;
        case DS_InvalidThreadID:
            replyErr = 0x22;
            break;
        case DS_OSError:
            replyErr = 0x20;
            break;
        default:
            replyErr = 3;
            break;
        }

        return TRKStandardACK(buffer, 0x80, replyErr);
    }

    return TRKSendACK(buffer);
}

DSError TRKDoWriteRegisters(TRKBuffer* buffer) {
    int error;
    u8 command;
    u8 options;
    u16 firstRegister;
    u16 lastRegister;
    u32 registersLength;

    if (buffer->length <= 6) {
        return TRKStandardACK(buffer, 0x80, 2);
    }

    TRKSetBufferPosition(buffer, 0);
    error = TRKReadBuffer1_ui8(buffer, &command);
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui8(buffer, &options);
    }
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui16(buffer, &firstRegister);
    }
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui16(buffer, &lastRegister);
    }

    if (firstRegister > lastRegister) {
        return TRKStandardACK(buffer, 0x80, 0x14);
    }

    switch (options) {
    case 0:
        error = TRKTargetAccessDefault(firstRegister, lastRegister, buffer, &registersLength, 0);
        break;
    case 1:
        error = TRKTargetAccessFP(firstRegister, lastRegister, buffer, &registersLength, 0);
        break;
    case 2:
        error = TRKTargetAccessExtended1(firstRegister, lastRegister, buffer, &registersLength, 0);
        break;
    case 3:
        error = TRKTargetAccessExtended2(firstRegister, lastRegister, buffer, &registersLength, 0);
        break;
    default:
        error = DS_UnsupportedError;
        break;
    }

    if (error == DS_NoError) {
        TRKMessageReplyHeader(buffer, 0x80, 0);
    }

    if (error != DS_NoError) {
        int replyErr;

        switch (error) {
        case DS_UnsupportedError:
            replyErr = 0x12;
            break;
        case DS_InvalidRegister:
            replyErr = 0x14;
            break;
        case DS_MessageBufferReadError:
            replyErr = 2;
            break;
        case DS_CWDSException:
            replyErr = 0x15;
            break;
        case DS_InvalidProcessID:
            replyErr = 0x21;
            break;
        case DS_InvalidThreadID:
            replyErr = 0x22;
            break;
        case DS_OSError:
            replyErr = 0x20;
            break;
        default:
            replyErr = 3;
            break;
        }

        return TRKStandardACK(buffer, 0x80, replyErr);
    }

    return TRKSendACK(buffer);
}

DSError TRKDoFlushCache(TRKBuffer* buffer) {
    int error;
    u8 command;
    u8 options;
    u32 start;
    u32 end;

    if (buffer->length != 10) {
        return TRKStandardACK(buffer, 0x80, 2);
    }

    TRKSetBufferPosition(buffer, 0);
    error = TRKReadBuffer1_ui8(buffer, &command);
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui8(buffer, &options);
    }
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui32(buffer, &start);
    }
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui32(buffer, &end);
    }

    if (start > end) {
        return TRKStandardACK(buffer, 0x80, 0x13);
    }

    if (error == DS_NoError) {
        error = TRKTargetFlushCache(options, start, end);
    }

    if (error == DS_NoError) {
        TRKMessageReplyHeader(buffer, 0x80, 0);
    }

    if (error != DS_NoError) {
        int replyErr;

        switch (error) {
        case DS_UnsupportedError:
            replyErr = 0x12;
            break;
        default:
            replyErr = 3;
            break;
        }

        return TRKStandardACK(buffer, 0x80, replyErr);
    }

    return TRKSendACK(buffer);
}

DSError TRKDoContinue(TRKBuffer* buffer) {
    DSError error;

    if (!TRKTargetStopped()) {
        return TRKStandardACK(buffer, 0x80, 0x16);
    }

    error = TRKStandardACK(buffer, 0x80, 0);
    if (error == DS_NoError) {
        error = TRKTargetContinue();
    }

    return error;
}

DSError TRKDoStep(TRKBuffer* buffer) {
    DSError error;
    u8 command;
    u8 options;
    u8 count;
    u32 rangeStart;
    u32 rangeEnd;
    u32 pc;

    if (buffer->length < 3) {
        return TRKStandardACK(buffer, 0x80, 2);
    }

    TRKSetBufferPosition(buffer, 0);
    error = TRKReadBuffer1_ui8(buffer, &command);
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui8(buffer, &options);
    }

    switch (options) {
    case 0:
    case 0x10:
        if (error == DS_NoError) {
            error = TRKReadBuffer1_ui8(buffer, &count);
        }
        if (count < 1) {
            return TRKStandardACK(buffer, 0x80, 0x11);
        }
        break;
    case 1:
    case 0x11:
        if (buffer->length != 10) {
            return TRKStandardACK(buffer, 0x80, 2);
        }
        if (error == DS_NoError) {
            error = TRKReadBuffer1_ui32(buffer, &rangeStart);
        }
        if (error == DS_NoError) {
            error = TRKReadBuffer1_ui32(buffer, &rangeEnd);
        }
        pc = TRKTargetGetPC();
        if (pc < rangeStart || pc > rangeEnd) {
            return TRKStandardACK(buffer, 0x80, 0x11);
        }
        break;
    default:
        return TRKStandardACK(buffer, 0x80, 0x12);
    }

    if (!TRKTargetStopped()) {
        return TRKStandardACK(buffer, 0x80, 0x16);
    }

    error = TRKStandardACK(buffer, 0x80, 0);
    if (error == DS_NoError) {
        switch (options) {
        case 0:
        case 0x10:
            error = TRKTargetSingleStep(count, options == 0x10);
            break;
        case 1:
        case 0x11:
            error = TRKTargetStepOutOfRange(rangeStart, rangeEnd, options == 0x11);
            break;
        }
    }

    return error;
}

DSError TRKDoStop(TRKBuffer* buffer) {
    u8 replyError;

    switch (TRKTargetStop()) {
    case DS_NoError:
        replyError = 0;
        break;
    case DS_InvalidProcessID:
        replyError = 0x21;
        break;
    case DS_InvalidThreadID:
        replyError = 0x22;
        break;
    case DS_OSError:
        replyError = 0x20;
        break;
    default:
        replyError = 1;
        break;
    }

    return TRKStandardACK(buffer, 0x80, replyError);
}

DSError TRKDoSetOption(TRKBuffer* buffer) {
    int error;
    u8 command = 0;
    u8 optionID = 0;
    u8 optionValue = 0;

    TRKSetBufferPosition(buffer, 0);
    error = TRKReadBuffer1_ui8(buffer, &command);
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui8(buffer, &optionID);
    }
    if (error == DS_NoError) {
        error = TRKReadBuffer1_ui8(buffer, &optionValue);
    }

    if (error != DS_NoError) {
        TRKStandardACK(buffer, 0x80, 1);
    } else if (optionID == 1) {
        SetUseSerialIO(optionValue);
    }

    return TRKStandardACK(buffer, 0x80, 0);
}

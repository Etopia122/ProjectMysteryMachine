#include "TRK_MINNOW_DOLPHIN/MetroTRK/Portable/serpoll.h"
#include "TRK_MINNOW_DOLPHIN/MetroTRK/Portable/nubevent.h"
#include "trk.h"

static TRKFramingState gTRKFramingState;

void* gTRKInputPendingPtr;

extern int TRKReadUARTPoll(char* c);
extern DSError TRKStandardACK(TRKBuffer* buffer, u8 command, int reply);
extern int TRKGetFreeBuffer(int* id, TRKBuffer** buffer);

// TRKAppendBuffer1_ui8 is inlined at every call site in this TRK version.
static inline int serpoll_AppendBuffer1_ui8(TRKBuffer* buffer, const u8 data) {
    if (buffer->position >= 0x880) {
        return DS_MessageBufferOverflow;
    }

    buffer->data[buffer->position++] = data;
    buffer->length++;
    return DS_NoError;
}

static inline BOOL TRKCheckFrame(TRKBuffer* buffer) {
    if (buffer->length < 2) {
        TRKStandardACK(buffer, 0xFF, 2);
        if (gTRKFramingState.msgBufID != -1) {
            TRKReleaseBuffer(gTRKFramingState.msgBufID);
            gTRKFramingState.msgBufID = -1;
        }
        gTRKFramingState.buffer = NULL;
        gTRKFramingState.receiveState = DSRECV_Wait;
        return FALSE;
    }

    buffer->position = 0;
    buffer->length--;
    return TRUE;
}

// Older MetroTRK serial framing: 0x7E-delimited frames, 0x7D escapes.
MessageBufferID TRKTestForPacket(void) {
    int err = DS_NoError;
    char c;
    int readErr;

    readErr = TRKReadUARTPoll(&c);

    while (readErr == 0 && err == DS_NoError) {
        if (gTRKFramingState.receiveState != DSRECV_InFrame) {
            gTRKFramingState.isEscape = FALSE;
        }

        switch (gTRKFramingState.receiveState) {
        case DSRECV_Wait:
            if (c == 0x7E) {
                err = TRKGetFreeBuffer(&gTRKFramingState.msgBufID, &gTRKFramingState.buffer);
                gTRKFramingState.fcsType = 0;
                gTRKFramingState.receiveState = DSRECV_Found;
            }
            break;

        case DSRECV_Found:
            if (c == 0x7E) {
                break;
            }
            gTRKFramingState.receiveState = DSRECV_InFrame;
            // fallthrough

        case DSRECV_InFrame:
            if (c == 0x7E) {
                if (gTRKFramingState.isEscape) {
                    TRKStandardACK(gTRKFramingState.buffer, 0xFF, 4);
                    if (gTRKFramingState.msgBufID != -1) {
                        TRKReleaseBuffer(gTRKFramingState.msgBufID);
                        gTRKFramingState.msgBufID = -1;
                    }
                    gTRKFramingState.buffer = NULL;
                    gTRKFramingState.receiveState = DSRECV_Wait;
                    break;
                } else {
                    if (TRKCheckFrame(gTRKFramingState.buffer)) {
                        MessageBufferID id = gTRKFramingState.msgBufID;
                        gTRKFramingState.msgBufID = -1;
                        gTRKFramingState.buffer = NULL;
                        gTRKFramingState.receiveState = DSRECV_Wait;
                        return id;
                    }

                    gTRKFramingState.receiveState = DSRECV_Wait;
                }
                break;
            }

            if (gTRKFramingState.isEscape) {
                c ^= 0x20;
                gTRKFramingState.isEscape = FALSE;
            } else if (c == 0x7D) {
                gTRKFramingState.isEscape = TRUE;
                break;
            }

            err = serpoll_AppendBuffer1_ui8(gTRKFramingState.buffer, c);
            gTRKFramingState.fcsType += c;
            break;

        case DSRECV_FrameOverflow:
            if (c == 0x7E) {
                if (gTRKFramingState.msgBufID != -1) {
                    TRKReleaseBuffer(gTRKFramingState.msgBufID);
                    gTRKFramingState.msgBufID = -1;
                }
                gTRKFramingState.buffer = NULL;
                gTRKFramingState.receiveState = DSRECV_Wait;
            }
            break;
        }

        readErr = TRKReadUARTPoll(&c);
    }

    return -1;
}

void TRKGetInput(void) {
    TRKBuffer* buffer;
    MessageBufferID id;
    u8 command;
    TRKEvent event;

    id = TRKTestForPacket();
    if (id != -1) {
        buffer = TRKGetBuffer(id);
        TRKSetBufferPosition(buffer, 0);
        TRKReadBuffer1_ui8(buffer, &command);
        if (command < 0x80) {
            TRKConstructEvent(&event, NUBEVENT_Request);
            event.msgBufID = id;
            gTRKFramingState.msgBufID = -1;
            TRKPostEvent(&event);
        } else {
            TRKReleaseBuffer(id);
        }
    }
}

void TRKProcessInput(int bufferIdx) {
    TRKEvent event;

    TRKConstructEvent(&event, NUBEVENT_Request);
    event.msgBufID = bufferIdx;
    gTRKFramingState.msgBufID = -1;
    TRKPostEvent(&event);
}

DSError TRKInitializeSerialHandler() {
    gTRKFramingState.msgBufID = -1;
    gTRKFramingState.receiveState = DSRECV_Wait;
    gTRKFramingState.isEscape = FALSE;


    return DS_NoError;
}

DSError TRKTerminateSerialHandler(void) {
    return DS_NoError;
}

#include "TRK_MINNOW_DOLPHIN/MetroTRK/Portable/support.h"
#include "TRK_MINNOW_DOLPHIN/utils/common/MWTrace.h"
#include "TRK_MINNOW_DOLPHIN/MetroTRK/Portable/msgbuf.h"
#include "string.h"

// Older MetroTRK file-support requests: the request fields are appended one
// by one (no fixed 0x40-byte command header), replies are read back the same way.

extern int TRKGetFreeBuffer(int* id, TRKBuffer** buffer);
extern int TRKMessageSend(TRKBuffer* buffer);
extern MessageBufferID TRKTestForPacket(void);
extern void TRKProcessInput(int bufferIdx);



// TRKAppendBuffer1_ui8 is inlined at every call site in this TRK version.
static inline int support_AppendBuffer1_ui8(TRKBuffer* buffer, const u8 data) {
    if (buffer->position >= 0x880) {
        return DS_MessageBufferOverflow;
    }

    buffer->data[buffer->position++] = data;
    buffer->length++;
    return DS_NoError;
}

int TRKSuppAccessFile(u32 file_handle, u8* data, size_t* count, u8* io_result, BOOL need_reply,
                      BOOL read) {
    int error;
    int replyBufferId;
    TRKBuffer* replyBuffer;
    u32 length;
    int bufferId;
    TRKBuffer* buffer;
    u32 i;
    u8 replyIOResult;
    u16 replyLength;
    BOOL exit;

    if (data == NULL || *count == 0) {
        return DS_ParameterError;
    }

    exit = FALSE;
    *io_result = 0;
    i = 0;
    error = DS_NoError;
    while (!exit && i < *count && error == DS_NoError && *io_result == 0) {
        if (*count - i <= 0x800) {
            length = *count - i;
        } else {
            length = 0x800;
        }

        error = TRKGetFreeBuffer(&bufferId, &buffer);

        if (error == DS_NoError) {
            u8 cmd = 0xD0;
            if (read) {
                cmd = 0xD1;
            }
            error = support_AppendBuffer1_ui8(buffer, cmd);
        }

        if (error == DS_NoError) {
            error = TRKAppendBuffer1_ui32(buffer, file_handle);
        }

        if (error == DS_NoError) {
            error = TRKAppendBuffer1_ui16(buffer, length);
        }

        if (!read && error == DS_NoError) {
            error = TRKAppendBuffer_ui8(buffer, data + i, length);
        }

        if (error == DS_NoError) {
            if (need_reply) {
                BOOL b;

                replyLength = 0;
                replyIOResult = 0;

                b = read && file_handle == 0;
                error = TRKRequestSend(buffer, &replyBufferId, read ? 5 : 5, 3, b == FALSE);

                if (error == DS_NoError) {
                    replyBuffer = (TRKBuffer*)TRKGetBuffer(replyBufferId);
                    TRKSetBufferPosition(replyBuffer, 2);
                }

                if (error == DS_NoError) {
                    error = TRKReadBuffer1_ui8(replyBuffer, &replyIOResult);
                }

                if (error == DS_NoError) {
                    error = TRKReadBuffer1_ui16(replyBuffer, &replyLength);
                }

                if (read && error == DS_NoError) {
                    if (replyBuffer->length != replyLength + 5) {
                        replyLength = replyBuffer->length - 5;
                        if (replyIOResult == 0) {
                            replyIOResult = 1;
                        }
                    }

                    if (replyLength <= length) {
                        error = TRKReadBuffer_ui8(replyBuffer, data + i, replyLength);
                    }
                }

                if (replyLength != length) {
                    if ((!read || replyLength >= length) && replyIOResult == 0) {
                        replyIOResult = 1;
                    }
                    length = replyLength;
                    exit = TRUE;
                }

                *io_result = replyIOResult;
                TRKReleaseBuffer(replyBufferId);
            } else {
                error = TRKMessageSend(buffer);
            }
        }

        TRKReleaseBuffer(bufferId);
        i += length;
    }

    *count = i;
    return error;
}

DSError TRKRequestSend(TRKBuffer* msgBuf, int* bufferId, u32 p1, u32 p2, int p3) {
    int error = DS_NoError;
    TRKBuffer* buffer;
    u32 counter;
    int count;
    u8 msgCmd;
    u8 msgReplyError;
    BOOL badReply = TRUE;

    *bufferId = -1;

    for (count = p2 + 1; count != 0 && *bufferId == -1 && error == DS_NoError; count--) {
        error = TRKMessageSend(msgBuf);
        if (error == DS_NoError) {
            if (p3) {
                counter = 0;
            }

            while (TRUE) {
                do {
                    *bufferId = TRKTestForPacket();
                    if (*bufferId != -1)
                        break;
                } while (!p3 || ++counter < 79999980);

                if (*bufferId == -1)
                    break;

                badReply = FALSE;

                buffer = TRKGetBuffer(*bufferId);
                TRKSetBufferPosition(buffer, 0);

                if ((error = TRKReadBuffer1_ui8(buffer, &msgCmd)) != DS_NoError)
                    break;

                if (msgCmd >= 0x80)
                    break;

                TRKProcessInput(*bufferId);
                *bufferId = -1;
            }

            if (*bufferId != -1) {
                if (buffer->length < p1) {
                    badReply = TRUE;
                }
                if (error == DS_NoError && !badReply) {
                    error = TRKReadBuffer1_ui8(buffer, &msgReplyError);
                }
                if (error == DS_NoError && !badReply) {
                    if (msgCmd != 0x80 || msgReplyError != 0) {
                        badReply = TRUE;
                    }
                }
                if (error != DS_NoError || badReply) {
                    TRKReleaseBuffer(*bufferId);
                    *bufferId = -1;
                }
            }
        }
    }

    if (*bufferId == -1) {
        error = DS_Error800;
    }

    return error;
}

int HandleOpenFileSupportRequest(const char* path, u8 replyError, u32* handle, u8* ioResult) {
    int error;
    int replyBufferId;
    int bufferId;
    TRKBuffer* replyBuffer;
    TRKBuffer* buffer;

    *handle = 0;
    error = TRKGetFreeBuffer(&bufferId, &buffer);

    if (error == DS_NoError) {
        error = support_AppendBuffer1_ui8(buffer, 0xD2);
    }

    if (error == DS_NoError) {
        error = support_AppendBuffer1_ui8(buffer, replyError);
    }

    if (error == DS_NoError) {
        error = TRKAppendBuffer1_ui16(buffer, strlen(path) + 1);
    }

    if (error == DS_NoError) {
        error = TRKAppendBuffer_ui8(buffer, (u8*)path, strlen(path) + 1);
    }

    if (error == DS_NoError) {
        *ioResult = 0;
        error = TRKRequestSend(buffer, &replyBufferId, 7, 3, 0);

        if (error == DS_NoError) {
            replyBuffer = TRKGetBuffer(replyBufferId);
            TRKSetBufferPosition(replyBuffer, 2);
        }

        if (error == DS_NoError) {
            error = TRKReadBuffer1_ui8(replyBuffer, ioResult);
        }

        if (error == DS_NoError) {
            error = TRKReadBuffer1_ui32(replyBuffer, handle);
        }

        TRKReleaseBuffer(replyBufferId);
    }

    TRKReleaseBuffer(bufferId);
    return error;
}

int HandleCloseFileSupportRequest(u32 handle, u8* ioResult) {
    int error;
    int replyBufferId;
    int bufferId;
    TRKBuffer* buffer;
    TRKBuffer* replyBuffer;

    error = TRKGetFreeBuffer(&bufferId, &buffer);

    if (error == DS_NoError) {
        error = support_AppendBuffer1_ui8(buffer, 0xD3);
    }

    if (error == DS_NoError) {
        error = TRKAppendBuffer1_ui32(buffer, handle);
    }

    if (error == DS_NoError) {
        *ioResult = 0;
        error = TRKRequestSend(buffer, &replyBufferId, 3, 3, 0);

        if (error == DS_NoError) {
            replyBuffer = TRKGetBuffer(replyBufferId);
            TRKSetBufferPosition(replyBuffer, 2);
        }

        if (error == DS_NoError) {
            error = TRKReadBuffer1_ui8(replyBuffer, ioResult);
        }

        TRKReleaseBuffer(replyBufferId);
    }

    TRKReleaseBuffer(bufferId);
    return error;
}

int HandlePositionFileSupportRequest(u32 handle, u32* position, u8 mode, u8* ioResult) {
    int error;
    int replyBufferId;
    int bufferId;
    TRKBuffer* buffer;
    TRKBuffer* replyBuffer;

    error = TRKGetFreeBuffer(&bufferId, &buffer);

    if (error == DS_NoError) {
        error = support_AppendBuffer1_ui8(buffer, 0xD4);
    }

    if (error == DS_NoError) {
        error = TRKAppendBuffer1_ui32(buffer, handle);
    }

    if (error == DS_NoError) {
        error = TRKAppendBuffer1_ui32(buffer, *position);
    }

    if (error == DS_NoError) {
        error = support_AppendBuffer1_ui8(buffer, mode);
    }

    if (error == DS_NoError) {
        *ioResult = 0;
        error = TRKRequestSend(buffer, &replyBufferId, 3, 3, 0);

        if (error == DS_NoError) {
            replyBuffer = TRKGetBuffer(replyBufferId);
            TRKSetBufferPosition(replyBuffer, 2);
        }

        if (error == DS_NoError) {
            error = TRKReadBuffer1_ui8(replyBuffer, ioResult);
        }

        if (error == DS_NoError) {
            error = TRKReadBuffer1_ui32(replyBuffer, position);
        } else {
            *position = -1;
        }

        TRKReleaseBuffer(replyBufferId);
    }

    TRKReleaseBuffer(bufferId);
    return error;
}

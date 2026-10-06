/**
 * msg.c
 * Description: older MetroTRK serial framing (checksummed, 0x7E-delimited,
 * 0x7D-escaped packets written a byte at a time).
 */

#include "TRK_MINNOW_DOLPHIN/MetroTRK/Portable/msg.h"
#include "TRK_MINNOW_DOLPHIN/Os/dolphin/dolphin_trk_glue.h"
#include "trk.h"

#define TRK_FRAME_FLAG 0x7E
#define TRK_FRAME_ESCAPE 0x7D

DSError WriteUART1(s8 c);
DSError WriteUARTFlush(void);

DSError TRKMessageSend(TRKBuffer* b) {
    u8 csum;
    u32 i;
    DSError err;
    u8 c;

    csum = 0;
    for (i = 0; i < b->length; i++) {
        csum = csum + b->data[i];
    }
    csum = csum ^ 0xFF;

    err = WriteUART1(TRK_FRAME_FLAG);
    if (err == DS_NoError) {
        for (i = 0; i < b->length; i++) {
            c = b->data[i];
            if (c == TRK_FRAME_FLAG || c == TRK_FRAME_ESCAPE) {
                err = WriteUART1(TRK_FRAME_ESCAPE);
                c ^= 0x20;
                if (err != DS_NoError) {
                    break;
                }
            }
            err = WriteUART1(c);
            if (err != DS_NoError) {
                break;
            }
        }
    }

    if (err == DS_NoError) {
        c = csum;
        if (c == TRK_FRAME_FLAG || c == TRK_FRAME_ESCAPE) {
            err = WriteUART1(TRK_FRAME_ESCAPE);
            c ^= 0x20;
            if (err != DS_NoError) {
                goto done;
            }
        }
        err = WriteUART1(c);
    }

done:
    if (err == DS_NoError) {
        err = WriteUART1(TRK_FRAME_FLAG);
    }
    if (err == DS_NoError) {
        err = WriteUARTFlush();
    }
    return err;
}

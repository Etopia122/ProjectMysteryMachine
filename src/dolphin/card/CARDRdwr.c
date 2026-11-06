#include "dolphin/card.h"
#define CARD_FILE "CARDRdwr.c"
#include "dolphin/card/card_dbg.h"

// prototypes
static void BlockReadCallback(s32 chan, s32 result);
static void BlockWriteCallback(s32 chan, s32 result);

static void BlockReadCallback(s32 chan, s32 result) {
    CARDControl* card;
    CARDCallback callback;

    card = &__CARDBlock[chan];

    if ((result >= 0)) {
        card->xferred += 0x200;
        card->addr += 0x200;
        ((u8*)card->buffer) += 0x200;

        if (--card->repeat > 0) {
            result = __CARDReadSegment(chan, BlockReadCallback);
            if (result >= 0) {
                return;
            }
        }
    }

    if (!card->apiCallback) {
        __CARDPutControlBlock(card, result);
    }

    callback = card->xferCallback;
    if (callback) {
        card->xferCallback = NULL;
        callback(chan, result);
    }
}

s32 __CARDRead(s32 chan, u32 addr, s32 length, void* dst, CARDCallback callback) {
    CARDControl* card;

    ASSERTLINE(88, 0 < length && length % CARD_SEG_SIZE == 0);
    ASSERTLINE(89, 0 <= chan && chan < 2);

    card = &__CARDBlock[chan];
    if (card->attached == 0) {
        return CARD_RESULT_NOCARD;
    }
    card->xferCallback = callback;
    card->repeat = (length / 512u);
    card->addr = addr;
    card->buffer = dst;
    return __CARDReadSegment(chan, BlockReadCallback);
}

static void BlockWriteCallback(s32 chan, s32 result) {
    CARDControl* card;
    CARDCallback callback;

    card = &__CARDBlock[chan];
    if (result >= 0) {
        card->xferred += 0x80;
        card->addr += 0x80;
        ((u8*)card->buffer) += 0x80;

        if (--card->repeat > 0) {
            result = __CARDWritePage(chan, BlockWriteCallback);
            if (result >= 0) {
                return;
            }
        }
    }

    if (!card->apiCallback) {
        __CARDPutControlBlock(card, result);
    }

    callback = card->xferCallback;
    if (callback) {
        card->xferCallback = NULL;
        callback(chan, result);
    }
}

s32 __CARDWrite(s32 chan, u32 addr, s32 length, void* dst, CARDCallback callback) {
    CARDControl* card;

#if defined(VERSION_GIHE78)
    ASSERTLINE(149, 0 < length && length % CARD_PAGE_SIZE == 0);
    ASSERTLINE(150, 0 <= chan && chan < 2);
    card = &__CARDBlock[chan];
#else
    card = &__CARDBlock[chan];

    ASSERTLINE(149, 0 < length && length % card->pageSize == 0);
    ASSERTLINE(150, 0 <= chan && chan < 2);
#endif

    if (card->attached == 0) {
        return CARD_RESULT_NOCARD;
    }
    card->xferCallback = callback;
    card->repeat = (int)(length / 128u);
    card->addr = addr;
    card->buffer = dst;
    return __CARDWritePage(chan, BlockWriteCallback);
}

#if defined(VERSION_GIHE78)
s32 CARDGetXferredBytes(s32 chan) {
    ASSERTLINE(180, 0 <= chan && chan < 2);
    return __CARDBlock[chan].xferred;
}
#endif

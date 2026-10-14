#include "dolphin/card.h"
#define CARD_FILE "CARDDelete.c"
#include "dolphin/card/card_dbg.h"
#include "string.h"

// The older SDK's file deletion (not in TWW's card sources).

extern CARDDir* __CARDGetDirBlock(CARDControl* card);
extern s32 __CARDGetFileNo(CARDControl* card, const char* fileName, s32* pfileNo);
extern BOOL __CARDIsOpened(CARDControl* card, s32 fileNo);
extern s32 __CARDFreeBlock(s32 chan, u16 nBlock, CARDCallback callback);
extern void __CARDDefaultApiCallback(s32 chan, s32 result);
extern void __CARDSyncCallback(s32 chan, s32 result);

static void DeleteCallback(s32 chan, s32 result) {
    CARDControl* card;
    CARDCallback callback;

    card = &__CARDBlock[chan];
    callback = card->apiCallback;
    card->apiCallback = 0;

    if (result < 0)
        goto error;

    result = __CARDFreeBlock(chan, card->startBlock, callback);
    if (result < 0)
        goto error;
    return;

error:
    __CARDPutControlBlock(card, result);
    if (callback)
        callback(chan, result);
}

s32 CARDDeleteAsync(s32 chan, const char* fileName, CARDCallback callback) {
    CARDControl* card;
    s32 fileNo;
    CARDDir* dir;
    CARDDir* ent;
    s32 result;

    result = __CARDGetControlBlock(chan, &card);
    if (result < 0)
        return result;

    result = __CARDGetFileNo(card, fileName, &fileNo);
    if (result < 0)
        return __CARDPutControlBlock(card, result);

    if (__CARDIsOpened(card, fileNo))
        return __CARDPutControlBlock(card, CARD_RESULT_BUSY);

    dir = __CARDGetDirBlock(card);
    ent = &dir[fileNo];
    card->startBlock = ent->startBlock;
    memset(ent, 0xFF, sizeof(CARDDir));

    card->apiCallback = callback ? callback : __CARDDefaultApiCallback;
    result = __CARDUpdateDir(chan, DeleteCallback);
    if (result < 0)
        __CARDPutControlBlock(card, result);
    return result;
}

s32 CARDDelete(s32 chan, const char* fileName) {
    s32 result = CARDDeleteAsync(chan, fileName, __CARDSyncCallback);

    if (result < 0)
        return result;

    return __CARDSync(chan);
}

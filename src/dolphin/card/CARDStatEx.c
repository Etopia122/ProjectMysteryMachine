#include "dolphin/card.h"
#define CARD_FILE "CARDStatEx.c"
#include "dolphin/card/card_dbg.h"
#include "string.h"

// The older SDK's raw directory-entry access and file attributes (not in TWW's
// card sources).

extern CARDDir* __CARDGetDirBlock(CARDControl* card);
extern s32 __CARDAccess(CARDControl* card, CARDDir* ent);
extern s32 __CARDIsPublic(CARDDir* ent);
extern void __CARDSyncCallback(s32 chan, s32 result);

s32 __CARDGetStatusEx(s32 chan, s32 fileNo, CARDDir* dirent) {
    CARDControl* card;
    CARDDir* dir;
    CARDDir* ent;
    s32 result;

    ASSERTLINE(76, 0 <= chan && chan < 2);
    ASSERTLINE(77, 0 <= fileNo && fileNo < CARD_MAX_FILE);

    if (fileNo < 0 || CARD_MAX_FILE <= fileNo)
        return CARD_RESULT_FATAL_ERROR;

    result = __CARDGetControlBlock(chan, &card);
    if (result < 0)
        return result;

    dir = __CARDGetDirBlock(card);
    ent = &dir[fileNo];
    result = __CARDAccess(card, ent);
    if (result == CARD_RESULT_NOPERM)
        result = __CARDIsPublic(ent);

    if (result >= 0)
        memcpy(dirent, ent, sizeof(CARDDir));

    return __CARDPutControlBlock(card, result);
}

s32 __CARDSetStatusExAsync(s32 chan, s32 fileNo, CARDDir* dirent, CARDCallback callback) {
    CARDControl* card;
    CARDDir* dir;
    CARDDir* ent;
    s32 result;
    u8* p;
    s32 i;

    ASSERTLINE(137, 0 <= fileNo && fileNo < CARD_MAX_FILE);
    ASSERTLINE(138, 0 <= chan && chan < 2);
    ASSERTLINE(139, *dirent->fileName != 0xff && *dirent->fileName != 0x00);
    ASSERTMSGLINE(147, dirent->iconAddr == 0xffffffff || dirent->iconAddr < CARD_READ_SIZE,
                  "CARDSetStatus[Async](): stat->iconAddr must be 0xffffffff or less than CARD_READ_SIZE.");
    ASSERTMSGLINE(150, dirent->commentAddr == 0xffffffff || (dirent->commentAddr & 0x1FFF) <= 8128,
                  "CARDSetStatus[Async](): comment strings (set by stat->commentAddr) must not cross 8KB byte boundary.");

    if (fileNo < 0 || CARD_MAX_FILE <= fileNo || dirent->fileName[0] == 0xFF || dirent->fileName[0] == 0x00)
        return CARD_RESULT_FATAL_ERROR;

    result = __CARDGetControlBlock(chan, &card);
    if (result < 0)
        return result;

    dir = __CARDGetDirBlock(card);
    ent = &dir[fileNo];
    result = __CARDAccess(card, ent);
    if (result < 0)
        return __CARDPutControlBlock(card, result);

    for (p = dirent->fileName; p < dirent->fileName + CARD_FILENAME_MAX; p++) {
        if (*p == '\0') {
            while (++p < dirent->fileName + CARD_FILENAME_MAX)
                *p = '\0';
            break;
        }
    }

    if (memcmp(ent->fileName, dirent->fileName, CARD_FILENAME_MAX) != 0 ||
        memcmp(ent->gameName, dirent->gameName, sizeof(ent->gameName)) != 0 ||
        memcmp(ent->company, dirent->company, sizeof(ent->company)) != 0) {
        for (i = 0; i < CARD_MAX_FILE; i++) {
            if (i != fileNo) {
                CARDDir* other = &dir[i];

                if (other->gameName[0] != 0xFF &&
                    memcmp(other->gameName, dirent->gameName, sizeof(other->gameName)) == 0 &&
                    memcmp(other->company, dirent->company, sizeof(other->company)) == 0 &&
                    memcmp(other->fileName, dirent->fileName, CARD_FILENAME_MAX) == 0)
                    return __CARDPutControlBlock(card, CARD_RESULT_EXIST);
            }
        }
        memcpy(ent->fileName, dirent->fileName, CARD_FILENAME_MAX);
        memcpy(ent->gameName, dirent->gameName, sizeof(ent->gameName));
        memcpy(ent->company, dirent->company, sizeof(ent->company));
    }

    ent->time = dirent->time;
    ent->bannerFormat = dirent->bannerFormat;
    ent->iconAddr = dirent->iconAddr;
    ent->iconFormat = dirent->iconFormat;
    ent->iconSpeed = dirent->iconSpeed;
    ent->commentAddr = dirent->commentAddr;
    ent->permission = dirent->permission;
    ent->copyTimes = dirent->copyTimes;

    result = __CARDUpdateDir(chan, callback);
    if (result < 0)
        __CARDPutControlBlock(card, result);
    return result;
}

s32 CARDSetAttributesAsync(s32 chan, s32 fileNo, u8 attr, CARDCallback callback) {
    CARDDir dirent;
    s32 result;

    result = __CARDGetStatusEx(chan, fileNo, &dirent);
    if (result < 0)
        return result;

    dirent.permission = attr;
    return __CARDSetStatusExAsync(chan, fileNo, &dirent, callback);
}

s32 CARDSetAttributes(s32 chan, s32 fileNo, u8 attr) {
    s32 result = CARDSetAttributesAsync(chan, fileNo, attr, __CARDSyncCallback);

    if (result < 0)
        return result;

    return __CARDSync(chan);
}

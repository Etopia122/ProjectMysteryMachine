// xTRC: TRC (Nintendo's technical requirements) messages for the pad and the
// disc, plus the GameCube-only memory card checks and dialogs used by the boot
// sequence and zSaveLoad. Only xTRCPad and xTRCInit exist on PS2; the other
// names are BFBB's or placeholders.
//
// TODO: 27 of 30 functions match. fn_80043050 and fn_80042A48: the target
// folds xTRCShowCardMessage's text == NULL test for a local array argument but
// keeps its buffer initializer; ours keeps the test. fn_80041548: the info,
// pool base and buf registers are permuted.

#include "xTRC.h"
#include "xFont.h"
#include "xsavegame.h"
#include "isavegame.h"
#include "zGlobals.h"
#include "rwtypes.h"

#ifndef NULL
#define NULL 0
#endif

class RwCamera;

extern "C" {
void* memset(void* dst, int c, unsigned long n);
int sprintf(char* s, const char* format, ...);
}

RwCamera* RwCameraClear(RwCamera* camera, RwRGBA* colour, signed int clearMode);
RwCamera* RwCameraBeginUpdate(RwCamera* camera);
RwCamera* RwCameraEndUpdate(RwCamera* camera);
RwCamera* RwCameraShowRaster(RwCamera* camera, void* dev, unsigned int flags);
RwCamera* iCameraCreate(signed int width, signed int height);
void iCameraDestroy(RwCamera* camera);
void iVSync();
signed int xPadUpdate(signed int idx, float time_passed);

class iTRCDisk
{
public:
    static signed int CheckDVDAndResetState();
};

enum eGameMode
{
    eGameMode_Boot,
    eGameMode_Intro,
    eGameMode_Title,
    eGameMode_Start,
    eGameMode_Load,
    eGameMode_Options,
    eGameMode_Save,
    eGameMode_Pause,
    eGameMode_Stall,
    eGameMode_WorldMap,
    eGameMode_MonsterGallery,
    eGameMode_Game,
    eGameMode_Count
};

enum _GameOstrich
{
    eGameOstrich_Loading,
    eGameOstrich_PlayingMovie,
    eGameOstrich_InScene,
    eGameOstrich_Total
};

extern eGameMode gGameMode;
eGameMode zGameModeGet();
_GameOstrich zGameGetOstrich();
void zGamePauseIfPossible();
unsigned int zGameIsNickCam2();
signed int zMenuRunning();

extern "C" {
// TODO: unidentified iTRC function (asks for a reset to the IPL menu)
void fn_80016698(signed int);
}

// The cutscene manager's skip flag (xCutsceneMgr is not decompiled yet).
class xTRCCutsceneMgrLike
{
public:
    unsigned char pad0[0x18];
    signed int stop; // 0x18
};

// Memory card status for the save/load screens. zSaveLoad fills one of these
// in and asks for the message text that goes with it.
class xTRCCardInfo
{
public:
    signed int flags;     // 0x0: 1 slot set, 2 message set, 4 space set, 8 file set
    signed int msg;       // 0x4
    signed int tgt;       // 0x8
    signed int slot;      // 0xC
    signed int avail;     // 0x10
    signed int needed;    // 0x14
    signed int blockSize; // 0x18
    signed int files;     // 0x1C
    signed int needFile;  // 0x20
    signed int fileIdx;   // 0x24
};

// Button labels for xTRC dialogs (A, B, X, Y).
class xTRCButtons
{
public:
    char* a;
    char* b;
    char* x;
    char* y;
};

// Context for the "card changed" callback while a dialog is up.
class xTRCCardCheck
{
public:
    signed int slot;        // 0x0
    signed int tgt;         // 0x4
    st_ISGSESSION* session; // 0x8
};

_tagTRCPadInfo gTrcPad[4];
_tagTRCState gTrcDisk[2];
static signed int sCardCheckActive;
static xFontBox* sTrcFontBox;
static unsigned int sOwnCamera;

xTRCCardInfo sCardInfo[2];

extern "C" {
signed int fn_80044478(_tagTRCState state);
signed int fn_80041D90(char* text, xTRCButtons* buttons, signed int (*cb)(void*), void* data);
void fn_80041548(signed int idx, char* buf);
signed int fn_80043E18(signed int slot, st_ISGSESSION* session);
signed int fn_80043E20(void* data);
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xTRCInit()
{
    memset(gTrcPad, 0, sizeof(gTrcPad));
    for (signed int i = 0; i < 4; i++)
    {
        gTrcPad[i].id = i;
    }
    memset(gTrcDisk, 0, sizeof(gTrcDisk));
}

static inline iColor_tag xTRCColor(unsigned char r, unsigned char g, unsigned char b,
                                   unsigned char a)
{
    iColor_tag c;
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    return c;
}

static inline unsigned int xTRCPadMessageAllowed()
{
    if (gGameMode == eGameMode_Boot || gGameMode == eGameMode_Intro)
    {
        return 0;
    }
    if (zGameGetOstrich() != eGameOstrich_InScene)
    {
        return 0;
    }
    if (zMenuRunning())
    {
        return 0;
    }
    if (zGameModeGet() == eGameMode_Save || zGameModeGet() == eGameMode_Load)
    {
        return 0;
    }
    if (zGameIsNickCam2())
    {
        return 0;
    }
    return 1;
}

extern "C" signed int fn_80044478(_tagTRCState state)
{
    char* text;
    RwCamera* cam;

    switch (state)
    {
    case TRC_Unknown:
        text = NULL;
        break;
    case TRC_PadMissing:
        if (!xTRCPadMessageAllowed())
        {
            return 0;
        }
        text = "The Controller in Controller Socket 1 has been removed. Reconnect the "
               "Controller to continue with the game.";
        break;
    case TRC_PadInvalidType:
        text = "The Controller in Controller Socket 1 is an unknown type. Connect a standard "
               "Controller to continue with the game.";
        break;
    case TRC_DiskNotIdentified:
        text = "Please wait. Checking Disc.";
        break;
    case TRC_DiskTrayOpen:
        text = "The Disc Cover is open. If you want to continue the game, please close the "
               "Disc Cover.";
        break;
    case TRC_DiskNoDisk:
        text = "Please insert the Scooby-Doo Game Disc.";
        break;
    case TRC_DiskInvalid:
        text = "This is not the Scooby-Doo Game Disc. Please insert the Scooby-Doo Game Disc.";
        break;
    case TRC_DiskRetry:
        text = "The Game Disc could not be read. Please read the Nintendo GameCube\x5E "
               "Instruction Booklet for more information.";
        break;
    case TRC_DiskFatal:
        text = "An error has occurred. Turn the power off and refer to the Nintendo "
               "GameCube\x5E Instruction Booklet for further instructions.";
        break;
    default:
        return 0;
    }

    if (globals.cmgr != NULL)
    {
        ((xTRCCutsceneMgrLike*)globals.cmgr)->stop = 1;
    }
    zGamePauseIfPossible();

    if (sTrcFontBox == NULL)
    {
        if (sOwnCamera)
        {
            cam = iCameraCreate(640, 480);
            RwRGBA clear = { 0, 0, 0, 0 };
            RwCameraClear(cam, &clear, 3);
            RwCameraBeginUpdate(cam);
        }
        sTrcFontBox = xFontBoxCreate(0.0f, 0.0f, 640.0f, 480.0f, 0,
                                     xTRCColor(0xFF, 0xE6, 0, 0xFF));
        xFontBoxSetText(sTrcFontBox, text);
        xFontBoxSetFont(sTrcFontBox, 1);
        xFontBoxSetDrawSize(sTrcFontBox, 15.0f, 20.0f);
        xFontBoxSetDrawSpacing(sTrcFontBox, 15.0f, 20.0f);
        xFontBoxSetBackdrop(sTrcFontBox, xTRCColor(0, 0, 0, 0xC8));
        xFontBoxSetInset(sTrcFontBox, 50.0f, 50.0f, 230.0f, 0.0f);
        xFontBoxSetJustification(sTrcFontBox, 0x40);
        xFontBoxRender(sTrcFontBox);
        xFontBoxDestroy(sTrcFontBox);
        sTrcFontBox = NULL;
        if (sOwnCamera)
        {
            RwCameraEndUpdate(cam);
            RwCameraShowRaster(cam, NULL, 1);
            iCameraDestroy(cam);
        }
    }
    return 1;
}

void xTRCPad(signed int pad_id, _tagTRCState state)
{
    if (pad_id == 0)
    {
        gTrcPad[pad_id].state = state;
    }
}

void xTRCDisk(_tagTRCState state)
{
    if (state != TRC_DiskNotIdentified)
    {
        gTrcDisk[0] = state;
        gTrcDisk[1] = TRC_DiskIdentified;
    }
    else
    {
        gTrcDisk[1] = TRC_DiskNotIdentified;
    }
}

extern "C" unsigned int fn_800443F8()
{
    switch (gTrcDisk[0])
    {
    case TRC_DiskTrayOpen:
    case TRC_DiskNoDisk:
    case TRC_DiskInvalid:
    case TRC_DiskRetry:
    case TRC_DiskFatal:
        return 1;
    case TRC_DiskTrayClosed:
    default:
        return 0;
    }
}

extern "C" void fn_800443AC()
{
    sOwnCamera = 1;
    if (!fn_80044478(gTrcPad[0].state))
    {
        fn_80044478(gTrcDisk[0]);
    }
    sOwnCamera = 0;
}

void xTRCRender()
{
    if (fn_80044478(gTrcPad[0].state))
    {
        return;
    }
    if (fn_80044478(gTrcDisk[0]))
    {
        return;
    }
}

static void xTRCClearScreen();
static void xTRCShowCardMessage(char* text);
static signed int xTRCAsk(char* text, char* a, char* b, signed int (*cb)(void*), void* data);

static inline signed int xTRCShowAccessMessage()
{
    return 0;
}

extern "C" {
signed int fn_80043940();
signed int fn_80043BA4(signed int slot);
signed int fn_80043730(signed int slot);
signed int fn_80043050(signed int slot, signed int* formatted);
signed int fn_80042A48(signed int slot);
signed int fn_80042774(signed int slot);
signed int fn_800423EC(signed int slot);
}

// Checks both memory card slots at boot and walks the player through any
// problems. Each check returns 1 (ok), 2 (skip this slot), 3 (stop checking)
// or 4 (start over).
extern "C" void fn_80043E78()
{
    signed int formatted;
    signed int pass;
    signed int retry;
    signed int result;

    sCardCheckActive = 1;
    formatted = 0;
    static signed int sShowAccess = xTRCShowAccessMessage();
    pass = 0;

    do
    {
        if (pass++ != 0)
        {
            if (sShowAccess)
            {
                xTRCShowCardMessage(NULL);
            }
            else
            {
                xTRCClearScreen();
            }
            signed int i = 14;
            do
            {
                iTRCDisk::CheckDVDAndResetState();
                iVSync();
            } while (i-- > 0);
        }

        retry = 0;
        formatted = 0;

        result = fn_80043940();
        if (result == 3)
        {
            goto done;
        }
        if (result == 4)
        {
            retry = 1;
            continue;
        }

        result = fn_80043BA4(0);
        if (result == 2)
        {
            goto slot1;
        }
        if (result == 3)
        {
            goto done;
        }
        if (result == 4)
        {
            retry = 1;
        }
        if (retry)
        {
            continue;
        }

        result = fn_80043730(0);
        if (result == 2)
        {
            goto slot1;
        }
        if (result == 3)
        {
            goto done;
        }
        if (result == 4)
        {
            retry = 1;
        }
        if (retry)
        {
            continue;
        }

        result = fn_80043050(0, &formatted);
        if (result == 2)
        {
            goto slot1;
        }
        if (result == 3)
        {
            goto done;
        }
        if (result == 4)
        {
            retry = 1;
        }
        if (result == 1 && !formatted)
        {
            goto slot1;
        }
        if (retry)
        {
            continue;
        }

        result = fn_80042A48(0);
        if (result == 2)
        {
            goto slot1;
        }
        if (result == 3)
        {
            goto done;
        }
        if (result == 4)
        {
            retry = 1;
        }
        if (retry)
        {
            continue;
        }

        result = fn_80042774(0);
        if (result == 2)
        {
            goto slot1;
        }
        if (result == 3)
        {
            goto done;
        }
        if (result == 4)
        {
            retry = 1;
        }
        if (retry)
        {
            continue;
        }

        result = fn_800423EC(0);
        if (result == 2)
        {
            goto slot1;
        }
        if (result == 3)
        {
            goto done;
        }
        if (result == 4)
        {
            retry = 1;
        }
        if (retry)
        {
            continue;
        }
        if (result == 1)
        {
            goto done;
        }

    slot1:
        do
        {
            result = fn_80043BA4(1);
        } while (result == 2);
        if (result == 3)
        {
            goto done;
        }
        if (result == 4)
        {
            retry = 1;
        }
        if (retry)
        {
            continue;
        }

        result = fn_80043730(1);
        if (result == 2)
        {
            goto done;
        }
        if (result == 3)
        {
            goto done;
        }
        if (result == 4)
        {
            retry = 1;
        }
        if (retry)
        {
            continue;
        }

        result = fn_80043050(1, &formatted);
        if (result == 2)
        {
            goto done;
        }
        if (result == 3)
        {
            goto done;
        }
        if (result == 4)
        {
            retry = 1;
        }
        if (result == 1 && !formatted)
        {
            goto done;
        }
        if (retry)
        {
            continue;
        }

        result = fn_80042A48(1);
        if (result == 2)
        {
            goto done;
        }
        if (result == 3)
        {
            goto done;
        }
        if (result == 4)
        {
            retry = 1;
        }
        if (retry)
        {
            continue;
        }

        result = fn_80042774(1);
        if (result == 2)
        {
            goto done;
        }
        if (result == 3)
        {
            goto done;
        }
        if (result == 4)
        {
            retry = 1;
        }
        if (retry)
        {
            continue;
        }

        result = fn_800423EC(1);
        if (result == 2)
        {
            goto done;
        }
        if (result == 3)
        {
            goto done;
        }
        if (result == 4)
        {
            retry = 1;
        }
    } while (retry);

done:
    xTRCClearScreen();
    sCardCheckActive = 0;
}

// Dialog callback: true once the card in the slot has changed.
extern "C" signed int fn_80043E20(void* data)
{
    xTRCCardCheck* check = (xTRCCardCheck*)data;
    signed int changed = 0;

    if (check == NULL)
    {
        return 0;
    }
    signed int phys = iSGTgtPhysSlotIdx(check->session, check->tgt);
    if (phys != check->slot)
    {
        changed = 1;
    }
    return changed;
}

// Always lets the caller show its own dialog. The unused buffer accounts for
// the first 0x200 bytes of the .rodata pool, so the body was probably compiled
// out.
extern "C" signed int fn_80043E18(signed int slot, st_ISGSESSION* session)
{
    char text[512] = "";
    return 2;
}

static inline void xTRCCardBegin(signed int idx, signed int tgt, signed int slot)
{
    xTRCCardInfo* info = &sCardInfo[idx];
    memset(info, 0, sizeof(xTRCCardInfo));
    info->tgt = tgt;
    info->slot = slot;
    if (tgt == 0 || tgt == 1)
    {
        info->flags |= 1;
    }
}

static inline void xTRCCardMessage(signed int idx, signed int msg)
{
    xTRCCardInfo* info = &sCardInfo[idx];
    info->flags |= 2;
    info->msg = msg;
}

extern "C" signed int fn_80043BA4(signed int slot)
{
    signed int result = 1;
    char text[512] = "";
    st_ISGSESSION* session = iSGSessionBegin(NULL, NULL);
    signed int state = iSGCheckMemoryCard(session, slot);

    if (slot != 0 && !(state & 1))
    {
        xTRCCardBegin(0, slot, slot);
        xTRCCardMessage(0, 0x18);
        fn_80041548(0, text);
        signed int answer = xTRCAsk(text, "Continue Without Saving", "Retry", NULL, NULL);
        if (answer == -1)
        {
        result = 4;
        }
        else if (answer == 0)
        {
        result = 3;
        }
        else if (answer == 1)
        {
        result = 4;
        }
        else
        {
        result = 3;
        }
    }
    else if ((state & 1) && !(state & 2))
    {
        signed int r = fn_80043E18(slot, session);
        if (r != 2)
        {
            result = r;
        }
        else
        {
            xTRCCardBegin(0, slot, slot);
            xTRCCardMessage(0, 0x1B);
            fn_80041548(0, text);
            signed int answer = xTRCAsk(text, "Continue Without Saving", "Retry", NULL, NULL);
            if (answer == -1)
            {
            result = 4;
            }
            else if (answer == 0)
            {
            result = 3;
            }
            else if (answer == 1)
            {
            result = 4;
            }
            else
            {
            result = 3;
            }
        }
    }

    iSGSessionEnd(session);
    return result;
}

extern "C" signed int fn_80043940()
{
    char text[512] = "";
    signed int result;
    st_ISGSESSION* session = iSGSessionBegin(NULL, NULL);
    signed int state0 = iSGCheckMemoryCard(session, 0);
    signed int state1 = iSGCheckMemoryCard(session, 1);

    if ((state0 & 1) || (state1 & 1))
    {
        result = 1;
    }
    else if (!(state0 & 1) && !(state1 & 1))
    {
        xTRCCardBegin(0, 0, 0);
        xTRCCardMessage(0, 0x19);
        fn_80041548(0, text);
        signed int answer = xTRCAsk(text, "Continue Without Saving", "Retry", NULL, NULL);
        if (answer == -1)
        {
        result = 4;
        }
        else if (answer == 0)
        {
        result = 3;
        }
        else if (answer == 1)
        {
        result = 4;
        }
        else
        {
        result = 3;
        }
    }
    else if (iSGTgtCount(session, NULL) > 0)
    {
        result = 1;
    }
    else
    {
        xTRCCardBegin(0, 0, 0);
        xTRCCardMessage(0, 0x19);
        fn_80041548(0, text);
        signed int answer = xTRCAsk(text, "Continue Without Saving", "Retry", NULL, NULL);
        if (answer == -1)
        {
        result = 4;
        }
        else if (answer == 0)
        {
        result = 3;
        }
        else if (answer == 1)
        {
        result = 4;
        }
        else
        {
        result = 3;
        }
    }

    iSGSessionEnd(session);
    return result;
}

extern "C" signed int fn_80043730(signed int slot)
{
    signed int result = 1;
    char text[512] = "";
    signed int tgt = -1;
    st_ISGSESSION* session = iSGSessionBegin(NULL, NULL);
    signed int count = iSGTgtCount(session, NULL);

    if (count < 1)
    {
        result = 4;
    }
    else if (count == 2)
    {
        tgt = slot;
    }
    else
    {
        signed int phys = iSGTgtPhysSlotIdx(session, 0);
        if (phys == slot)
        {
            tgt = 0;
        }
    }

    if (tgt < 0)
    {
        result = 2;
    }
    else
    {
        signed int msg = 0;
        iSGTgtSetActive(session, tgt);
        unsigned int state = iSGTgtState(session, tgt, NULL);
        if (state & 0x1000000)
        {
            msg = 0x1A;
        }
        else if (state & 0x8000000)
        {
            msg = 0x1C;
        }
        else if (state & 0x2000000)
        {
            msg = 0x1E;
        }
        else if (!(state & 4))
        {
            msg = 0x1F;
        }

        if (msg != 0)
        {
            signed int r = fn_80043E18(slot, session);
            if (r != 2)
            {
                result = r;
            }
            else
            {
                xTRCCardBegin(0, tgt, slot);
                xTRCCardMessage(0, msg);
                fn_80041548(0, text);
                signed int answer =
                    xTRCAsk(text, "Continue Without Saving", "Retry", NULL, NULL);
                if (answer == 1)
                {
                    result = 4;
                }
                else if (answer == 0)
                {
                    result = 3;
                }
                else
                {
                    result = 3;
                }
            }
        }
    }

    iSGSessionEnd(session);
    return result;
}

static inline void xTRCCardSpace(signed int idx, signed int avail, signed int needed,
                                 signed int blockSize, signed int files, signed int needFile)
{
    xTRCCardInfo* info = &sCardInfo[idx];
    if (info->flags != 0)
    {
        info->flags |= 4;
        info->avail = avail;
        info->needed = needed;
        info->blockSize = blockSize >= 1 ? blockSize : 1;
        info->files = files;
        info->needFile = needFile;
    }
}

static inline void xTRCCardFile(signed int idx, signed int fileIdx)
{
    xTRCCardInfo* info = &sCardInfo[idx];
    if (info->flags != 0)
    {
        info->flags |= 8;
        info->fileIdx = fileIdx;
    }
}

extern "C" signed int fn_80043050(signed int slot, signed int* formatted)
{
    signed int result = 1;
    st_ISGSESSION* session;
    xTRCCardCheck check = { 0, 0, NULL };
    check.slot = slot;
    char text[512] = "";
    signed int tgt = -1;

    if (formatted != NULL)
    {
        *formatted = 0;
    }

    session = iSGSessionBegin(NULL, NULL);
    signed int count = iSGTgtCount(session, NULL);
    if (count < 1)
    {
        result = 4;
    }
    else if (count == 2)
    {
        tgt = slot;
    }
    else
    {
        signed int phys = iSGTgtPhysSlotIdx(session, 0);
        if (phys == slot)
        {
            tgt = 0;
        }
    }

    if (tgt < 0)
    {
        result = 2;
    }
    else
    {
        iSGTgtSetActive(session, tgt);
        unsigned int state = iSGTgtState(session, tgt, NULL);
        if (!(state & 4) || (state & 0x8000000))
        {
            tgt = -1;
        }
    }

    if (tgt < 0)
    {
        result = 2;
    }
    else
    {
        iSGTgtSetActive(session, tgt);
        unsigned int state = iSGTgtState(session, tgt, NULL);
        unsigned int otherMarket = state & 0x4000000;
        check.session = session;
        check.slot = slot;
        check.tgt = tgt;
        signed int r = 2;
        if (otherMarket || !(state & 2))
        {
            r = fn_80043E18(slot, session);
            if (r != 2)
            {
                result = r;
            }
        }

        if (r == 2 && (otherMarket || !(state & 2)))
        {
            xTRCCardBegin(0, tgt, slot);
            if (otherMarket)
            {
                xTRCCardMessage(0, 0x11);
            }
            else
            {
                xTRCCardMessage(0, 0x10);
            }
            fn_80041548(0, text);

            xTRCButtons buttons = { "Continue Without Saving", "Retry", "Format Memory Card", NULL };
            signed int answer = fn_80041D90(text, &buttons, fn_80043E20, &check);
            if (answer == -1)
            {
            result = 4;
            }
            else if (answer == 1)
            {
            result = 4;
            }
            else if (answer == 0)
            {
            result = 1;
            }
            else if (answer == 2)
            {
            xTRCCardBegin(0, tgt, slot);
            xTRCCardMessage(0, 0x12);
            fn_80041548(0, text);
            signed int answer = xTRCAsk(text, "No", "Yes", fn_80043E20, &check);
            if (answer == -1)
            {
                result = 4;
            }
            else if (answer == 0)
            {
                result = 4;
            }
            else
            {
                signed int canRecover = 0;
                xTRCCardBegin(0, tgt, slot);
                xTRCCardMessage(0, 0x13);
                fn_80041548(0, text);
                xTRCShowCardMessage(text);
                signed int ok = iSGTgtFormat(session, tgt, 0, &canRecover);
                signed int i = 0x3B;
                do
                {
                    iTRCDisk::CheckDVDAndResetState();
                    iVSync();
                } while (i-- > 0);
                xTRCCardBegin(0, tgt, slot);
                if (ok)
                {
                    result = 1;
                    xTRCCardMessage(0, 0x14);
                    if (formatted != NULL)
                    {
                        *formatted = 1;
                    }
                }
                else if (canRecover)
                {
                    result = 4;
                    xTRCCardMessage(0, 0x15);
                }
                else
                {
                    result = 4;
                    xTRCCardMessage(0, 0x16);
                }
                fn_80041548(0, text);
                if (ok)
                {
                    answer = xTRCAsk(text, NULL, "Continue", fn_80043E20, &check);
                }
                else
                {
                    answer = xTRCAsk(text, NULL, "Continue", NULL, NULL);
                }
                if (answer == -1)
                {
                    result = 4;
                }
            }
            }
        }
        else if ((state & 2) && formatted != NULL)
        {
            *formatted = 1;
        }
    }

    iSGSessionEnd(session);
    return result;
}

extern "C" signed int fn_80042A48(signed int slot)
{
    signed int result = 4;
    st_ISGSESSION* session;
    signed int tgt = -1;
    xTRCCardCheck check = { 0, 0, NULL };
    check.slot = slot;
    char text[512] = "";

    session = iSGSessionBegin(NULL, NULL);
    signed int count = iSGTgtCount(session, NULL);
    if (count < 1)
    {
        result = 4;
    }
    else if (count == 2)
    {
        tgt = slot;
    }
    else
    {
        signed int phys = iSGTgtPhysSlotIdx(session, 0);
        if (phys == slot)
        {
            tgt = 0;
        }
    }

    if (tgt < 0)
    {
        result = 2;
    }
    else
    {
        iSGTgtSetActive(session, tgt);
        check.session = session;
        check.slot = slot;
        check.tgt = tgt;
        signed int r = 2;
        signed int firstBad = -1;
        signed int firstFixed = -1;
        signed int ok = iSGCheckForCorruptFiles(session, 0, &firstBad, NULL);
        if (!ok)
        {
            r = fn_80043E18(slot, session);
            if (r != 2)
            {
                result = r;
            }
        }

        if (r == 2 && ok)
        {
            result = 1;
        }
        else if (r == 2)
        {
            xTRCButtons buttons = { "Continue Without Saving", "Retry", "Delete Corrupt File", NULL };
            xTRCCardBegin(0, tgt, slot);
            xTRCCardMessage(0, 9);
            xTRCCardFile(0, firstBad);
            fn_80041548(0, text);
            signed int answer = fn_80041D90(text, &buttons, fn_80043E20, &check);
            if (answer == -1)
            {
            result = 4;
            }
            else if (answer == 1)
            {
            result = 4;
            }
            else if (answer == 0)
            {
            result = 3;
            }
            else if (answer == 2)
            {
            xTRCCardBegin(0, tgt, slot);
            xTRCCardMessage(0, 0xA);
            fn_80041548(0, text);
            signed int answer = xTRCAsk(text, "No", "Yes", fn_80043E20, &check);
            if (answer == -1)
            {
            result = 4;
            }
            else if (answer == 0)
            {
            result = 4;
            }
            else if (answer == 1)
            {
            xTRCCardBegin(0, tgt, slot);
            xTRCCardMessage(0, 0xD);
            fn_80041548(0, text);
            xTRCShowCardMessage(text);
            xTRCCardBegin(0, tgt, slot);
            if (iSGCheckForCorruptFiles(session, 1, NULL, &firstFixed))
            {
                result = 1;
                xTRCCardMessage(0, 0xB);
            }
            else if (firstFixed >= 0)
            {
                result = 4;
                xTRCCardMessage(0, 0xB);
            }
            else
            {
                result = 4;
                xTRCCardMessage(0, 0xC);
            }
            fn_80041548(0, text);
            if (xTRCAsk(text, NULL, "Continue", fn_80043E20, &check) == -1)
            {
                result = 4;
            }
            }
            else
            {
            result = 3;
            }
            }
            else
            {
            result = 3;
            }
        }
    }

    iSGSessionEnd(session);
    return result;
}

extern "C" signed int fn_80042774(signed int slot)
{
    st_ISGSESSION* session;
    signed int result = 1;
    signed int tgt = -1;
    signed int manage = 0;
    xTRCCardCheck check = { 0, 0, NULL };
    check.slot = slot;
    signed int needed = 0;
    signed int avail = 0;
    signed int bytes = 0;
    signed int needFile = 0;
    signed int files = 0;
    char text[512] = "";

    session = iSGSessionBegin(NULL, NULL);
    signed int count = iSGTgtCount(session, NULL);
    if (count < 1)
    {
        result = 4;
    }
    else if (count == 2)
    {
        tgt = slot;
    }
    else
    {
        signed int phys = iSGTgtPhysSlotIdx(session, 0);
        if (phys == slot)
        {
            tgt = 0;
        }
    }

    if (tgt < 0)
    {
        result = 2;
    }
    else
    {
        iSGTgtSetActive(session, tgt);
        check.session = session;
        check.slot = slot;
        check.tgt = tgt;
        iSGTgtFreeSpace(session, tgt, &bytes, &files);
        signed int r = 2;
        signed int room =
            iSGTgtHaveRoom(session, tgt, 0x6000, NULL, NULL, &needed, &avail, &needFile);
        if (!room)
        {
            r = fn_80043E18(slot, session);
            if (r != 2)
            {
                result = r;
            }
        }

        if (r == 2 && room)
        {
            result = 1;
        }
        else if (r == 2)
        {
            xTRCButtons buttons = { "Continue Without Saving", "Retry", "Manage Memory Card", NULL };
            xTRCCardBegin(0, tgt, slot);
            xTRCCardMessage(0, 0x20);
            xTRCCardSpace(0, avail, needed, bytes, files, needFile);
            fn_80041548(0, text);
            signed int answer = fn_80041D90(text, &buttons, fn_80043E20, &check);
            if (answer == -1)
            {
            result = 4;
            }
            else if (answer == 1)
            {
            result = 4;
            }
            else if (answer == 0)
            {
            result = 3;
            }
            else if (answer == 2)
            {
            manage = 1;
            result = 4;
            }
            else
            {
            result = 3;
            }
        }
    }

    iSGSessionEnd(session);
    if (manage)
    {
        fn_80016698(1);
    }
    return result;
}

extern "C" signed int fn_800423EC(signed int slot)
{
    signed int result = 1;
    st_ISGSESSION* session;
    signed int tgt = -1;
    signed int manage = 0;
    xTRCCardCheck check = { 0, 0, NULL };
    check.slot = slot;
    signed int needed = 0;
    signed int avail = 0;
    signed int bytes = 0;
    signed int needFile = 0;
    signed int files = 0;
    char text[512] = "";

    session = iSGSessionBegin(NULL, NULL);
    signed int count = iSGTgtCount(session, NULL);
    if (count < 1)
    {
        result = 4;
    }
    else if (count == 2)
    {
        tgt = slot;
    }
    else
    {
        signed int phys = iSGTgtPhysSlotIdx(session, 0);
        if (phys == slot)
        {
            tgt = 0;
        }
    }

    if (tgt < 0)
    {
        result = 2;
    }
    else
    {
        iSGTgtSetActive(session, tgt);
        check.session = session;
        check.slot = slot;
        check.tgt = tgt;
        iSGTgtFreeSpace(session, tgt, &bytes, &files);

        signed int n;
        signed int i;
        signed int missing;
        signed int first;
        signed int r;
        missing = 0;
        first = -1;
        signed int sizes[3] = { 0, 0, 0 };
        r = 2;
        n = iSGGameFileCount(session);
        if (n >= 1 && n != 3)
        {
            for (i = 0; i < 3; i++)
            {
                char* name = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, i);
                sizes[i] = iSGFileSize(session, name);
                if (sizes[i] < 1)
                {
                    sizes[i] = -1;
                    missing++;
                    if (first < 0)
                    {
                        first = i;
                    }
                }
            }

            if (missing != 0)
            {
                signed int room =
                    iSGTgtHaveRoom(session, tgt, 0x6000, NULL,
                                   iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, i), &needed, &avail,
                                   &needFile);
                if (!room)
                {
                    r = fn_80043E18(slot, session);
                    if (r != 2)
                    {
                        result = r;
                    }
                }

                if (r == 2 && room)
                {
                    result = 1;
                }
                else if (r == 2)
                {
                    xTRCButtons buttons = { "Continue", "Retry", "Manage Memory Card", NULL };
                    xTRCCardBegin(0, tgt, slot);
                    xTRCCardMessage(0, 0x21);
                    xTRCCardSpace(0, avail, needed, bytes, files, needFile);
                    fn_80041548(0, text);
                    signed int answer = fn_80041D90(text, &buttons, fn_80043E20, &check);
                    if (answer == -1)
                    {
                    result = 4;
                    }
                    else if (answer == 1)
                    {
                    result = 4;
                    }
                    else if (answer == 0)
                    {
                    result = 3;
                    }
                    else if (answer == 2)
                    {
                    manage = 1;
                    result = 4;
                    }
                    else
                    {
                    result = 3;
                    }
                }
            }
        }
    }

    iSGSessionEnd(session);
    if (manage)
    {
        fn_80016698(1);
    }
    return result;
}

#pragma auto_inline on
static void xTRCClearScreen()
{
    RwRGBA clear = { 0, 0, 0, 0 };
    RwCamera* cam = iCameraCreate(640, 480);
    RwCameraClear(cam, &clear, 3);
    RwCameraBeginUpdate(cam);
    RwCameraEndUpdate(cam);
    RwCameraShowRaster(cam, NULL, 1);
    iCameraDestroy(cam);
}

// Shows one message on screen right away (used while a card operation runs).
static void xTRCShowCardMessage(char* text)
{
    RwRGBA clear = { 0, 0, 0, 0 };
    char buf[256] = "";
    RwCamera* cam;
    xFontBox* box;

    if (text == NULL)
    {
        sprintf(buf,
                "$A%sWARNING!$C Accessing the Nintendo GameCube%s Memory Card.\nPlease do not "
                "remove the Memory Card or turn off the power to the Nintendo GameCube%s.\n",
                "200040046", "^", "^");
        text = buf;
    }

    box = xFontBoxCreate(0.0f, 0.0f, 640.0f, 480.0f, 1, xTRCColor(0xFF, 0xE6, 0, 0xFF));
    xFontBoxSetText(box, text);
    xFontBoxSetDrawSize(box, 18.0f, 24.0f);
    xFontBoxSetDrawSpacing(box, 17.0f, 24.0f);
    xFontBoxSetBackdrop(box, xTRCColor(0, 0, 0, 0xFF));
    xFontBoxSetInset(box, 100.0f, 100.0f, 100.0f, 0.0f);
    xFontBoxSetJustification(box, 0x40);

    cam = iCameraCreate(640, 480);
    RwCameraClear(cam, &clear, 3);
    RwCameraBeginUpdate(cam);
    xFontBoxRender(box);
    RwCameraEndUpdate(cam);
    RwCameraShowRaster(cam, NULL, 1);
    iCameraDestroy(cam);
    xFontBoxDestroy(box);
}

static signed int xTRCAsk(char* text, char* a, char* b, signed int (*cb)(void*), void* data)
{
    xTRCButtons buttons = { NULL, NULL, NULL, NULL };
    buttons.a = a;
    buttons.b = b;
    return fn_80041D90(text, &buttons, cb, data);
}
#pragma auto_inline reset

// Shows a message with up to four button choices and waits for one of them.
// Returns the index of the chosen button, or -1 if the callback asks to stop.
extern "C" signed int fn_80041D90(char* text, xTRCButtons* buttons, signed int (*cb)(void*),
                                  void* data)
{
    RwRGBA clear = { 0, 0, 0, 0 };
    signed int result;
    RwCamera* cam;
    xFontBox* box1;
    xFontBox* box2;
    unsigned int pressed;
    xFontBox* box3;
    signed int count;
    unsigned int mask;
    signed int done;
    result = 0;
    box3 = NULL;
    char line1[256] = "";
    char line2[256] = "";
    done = 0;

    if (text == NULL)
    {
        xTRCClearScreen();
        return 0;
    }

    if (buttons->b != NULL && buttons->a != NULL && buttons->x != NULL && buttons->y != NULL)
    {
        count = 4;
        mask = 0xF0000;
        sprintf(line1, "%s %s\t%s %s", "#", buttons->b, "}", buttons->a);
        sprintf(line2, "%s %s\t%s %s", "~", buttons->x, "{", buttons->y);
    }
    else if (buttons->b != NULL && buttons->a != NULL && buttons->x != NULL)
    {
        count = 3;
        mask = 0x70000;
        sprintf(line1, "%s %s\t%s %s", "#", buttons->b, "}", buttons->a);
        sprintf(line2, "%s %s", "~", buttons->x);
    }
    else if (buttons->b != NULL && buttons->a != NULL)
    {
        count = 2;
        mask = 0x50000;
        sprintf(line1, "%s %s\t%s %s", "#", buttons->b, "}", buttons->a);
    }
    else
    {
        count = 1;
        mask = 0x10000;
        sprintf(line1, "%s %s", "#", buttons->b);
    }

    box1 = xFontBoxCreate(0.0f, 0.0f, 640.0f, 360.0f, 1, xTRCColor(0xFF, 0xE6, 0, 0xFF));
    box2 = xFontBoxCreate(0.0f, 360.0f, 640.0f, 100.0f, 1, xTRCColor(0xFF, 0xE6, 0, 0xFF));
    if (count > 2)
    {
        box3 = xFontBoxCreate(0.0f, 400.0f, 640.0f, 100.0f, 1, xTRCColor(0xFF, 0xE6, 0, 0xFF));
    }

    xFontBoxSetText(box1, text);
    xFontBoxSetDrawSize(box1, 18.0f, 24.0f);
    xFontBoxSetDrawSpacing(box1, 17.0f, 24.0f);
    xFontBoxSetBackdrop(box1, xTRCColor(0, 0, 0, 0xFF));
    xFontBoxSetInset(box1, 100.0f, 100.0f, 100.0f, 0.0f);
    xFontBoxSetJustification(box1, 0x40);

    xFontBoxSetText(box2, line1);
    xFontBoxSetDrawSize(box2, 20.0f, 26.0f);
    xFontBoxSetDrawSpacing(box2, 19.0f, 26.0f);
    xFontBoxSetBackdrop(box2, xTRCColor(0, 0, 0, 0xFF));
    xFontBoxSetInset(box2, 40.0f, 10.0f, 0.0f, 0.0f);
    xFontBoxSetJustification(box2, 0x40);

    if (count > 2)
    {
        xFontBoxSetText(box3, line2);
        xFontBoxSetDrawSize(box3, 20.0f, 26.0f);
        xFontBoxSetDrawSpacing(box3, 19.0f, 26.0f);
        xFontBoxSetBackdrop(box3, xTRCColor(0, 0, 0, 0xFF));
        xFontBoxSetInset(box3, 40.0f, 10.0f, 0.0f, 0.0f);
        xFontBoxSetJustification(box3, 0x40);
    }

    cam = iCameraCreate(640, 480);
    while (!done)
    {
        gFontWiggleTime += 1.0f / 60.0f;
        RwCameraClear(cam, &clear, 3);
        RwCameraBeginUpdate(cam);
        xFontBoxRender(box1);
        xFontBoxRender(box2);
        if (count > 2)
        {
            xFontBoxRender(box3);
        }
        RwCameraEndUpdate(cam);
        RwCameraShowRaster(cam, NULL, 1);

        if (cb != NULL && cb(data))
        {
            result = -1;
            break;
        }

        iTRCDisk::CheckDVDAndResetState();
        xPadUpdate(0, 1.0f / 60.0f);
        pressed = mask & globals.pad0->pressed;
        switch (count)
        {
        case 4:
            done = 1;
            if (pressed & 0x40000)
            {
                result = 0;
            }
            else if (pressed & 0x10000)
            {
                result = 1;
            }
            else if (pressed & 0x20000)
            {
                result = 2;
            }
            else if (pressed & 0x80000)
            {
                result = 3;
            }
            else
            {
                done = 0;
            }
            break;
        case 3:
            done = 1;
            if (pressed & 0x40000)
            {
                result = 0;
            }
            else if (pressed & 0x10000)
            {
                result = 1;
            }
            else if (pressed & 0x20000)
            {
                result = 2;
            }
            else
            {
                done = 0;
            }
            break;
        case 2:
            done = 1;
            if (pressed & 0x40000)
            {
                result = 0;
            }
            else if (pressed & 0x10000)
            {
                result = 1;
            }
            else
            {
                done = 0;
            }
            break;
        case 1:
            done = 1;
            if (pressed & 0x10000)
            {
                result = 1;
            }
            else
            {
                done = 0;
            }
            break;
        }
    }

    if (count > 2)
    {
        xFontBoxDestroy(box3);
    }
    xFontBoxDestroy(box2);
    xFontBoxDestroy(box1);
    iCameraDestroy(cam);
    return result;
}

extern "C" void fn_80041D58(signed int idx)
{
    memset(&sCardInfo[idx], 0, sizeof(xTRCCardInfo));
}

extern "C" void fn_80041CE8(signed int idx, signed int tgt, signed int slot)
{
    xTRCCardBegin(idx, tgt, slot);
}

extern "C" void fn_80041CC4(signed int idx, signed int msg)
{
    xTRCCardMessage(idx, msg);
}

extern "C" void fn_80041C78(signed int idx, signed int avail, signed int needed,
                            signed int blockSize, signed int files, signed int needFile)
{
    xTRCCardSpace(idx, avail, needed, blockSize, files, needFile);
}

extern "C" void fn_80041C4C(signed int idx, signed int fileIdx)
{
    xTRCCardFile(idx, fileIdx);
}

// Text for the first button of the current message.
extern "C" void fn_80041BC8(signed int idx, char* buf)
{
    xTRCCardInfo* info = &sCardInfo[idx];
    switch (info->msg)
    {
    case 0x21:
        sprintf(buf, "%s", "Continue");
        break;
    case 4:
    case 0x20:
    default:
        sprintf(buf, "%s", "Continue Without Saving");
        break;
    }
}

// Text for the third button of the current message.
extern "C" void fn_80041B10(signed int idx, char* buf)
{
    xTRCCardInfo* info = &sCardInfo[idx];
    switch (info->msg)
    {
    case 4:
    case 0x20:
    case 0x21:
        sprintf(buf, "%s", "Manage Memory Card");
        break;
    case 9:
        sprintf(buf, "%s", "Delete Corrupt File");
        break;
    case 0xE:
    case 0x10:
        sprintf(buf, "%s", "Format Memory Card");
        break;
    default:
        sprintf(buf, "%s", " ");
        break;
    }
}

extern "C" void fn_80041ADC(signed int idx, char* buf)
{
    sprintf(buf, "%s", "--error--");
}

// Builds the message text for the current card status.
extern "C" void fn_80041548(signed int idx, char* buf)
{
    xTRCCardInfo* info = &sCardInfo[idx];
    char device[64] = "";
    char card[64] = "";
    char slotName[32] = "";
    signed int blocks;
    char* name;

    name = xSGTgtName(NULL, info->slot, 2);
    sprintf(device, name);
    sprintf(slotName, xSGTgtName(NULL, info->slot, 3));
    sprintf(card, xSGTgtName(NULL, info->slot, 4));

    switch (info->msg)
    {
    case 1:
        sprintf(buf, "$A%sWARNING!$C\nSaving. Do not touch the %s or the %s.\n", "200040046",
                card, "POWER Button");
        break;
    case 2:
        sprintf(buf, "$A%sSave Successful.$C\n", "000210068");
        break;
    case 3:
        sprintf(buf, "$A%sSave Failed!$C\nCheck the %s and try again.", "200040046", card);
        break;
    case 4:
        blocks = (info->needed + info->blockSize - 1) / info->blockSize;
        sprintf(buf,
                "$A%sSave Failed!$C\nNot enough space on the %s to create a new %s file.\nAt "
                "least $A%s%d file%s and %d block%s required.\n",
                "200040046", card, "$A170066176Scooby-Doo: Night of 100 Frights$C", "000210068",
                info->needFile, info->needFile == 1 ? "" : "s", blocks,
                blocks == 1 ? "$C is" : "s$C are");
        break;
    case 5:
        sprintf(buf, "$A%sWARNING!$C\nLoading. Do not touch the %s or the %s.\n", "200040046",
                card, "POWER Button");
        break;
    case 6:
        sprintf(buf, "$A%sLoad Successful.$C\n", "000210068");
        break;
    case 7:
        sprintf(buf, "$A%sLoad Failed!$C\nCheck the %s and try again.\n", "200040046", card);
        break;
    case 8:
        sprintf(buf, "$A%sLoad Failed!$C\nCheck the %s and try again.\n", "200040046", card);
        break;
    case 9:
        if (info->fileIdx >= 0)
        {
            sprintf(buf,
                    "The %s file (%d) on the %s is corrupt and needs to be deleted.\n\nDo you "
                    "wish to delete this file?",
                    "$A170066176Scooby-Doo: Night of 100 Frights$C", info->fileIdx + 1, card);
        }
        else
        {
            sprintf(buf,
                    "The %s file on the %s is corrupt and needs to be deleted.\n\nDo you wish "
                    "to delete this file?",
                    "$A170066176Scooby-Doo: Night of 100 Frights$C", card);
        }
        break;
    case 10:
        sprintf(buf,
                "$A%sCAUTION$C\nDeleting this corrupt %s file will erase it permanently.\n\nAre "
                "you sure you wish to delete this file?\n",
                "200040046", "$A170066176Scooby-Doo: Night of 100 Frights$C");
        break;
    case 11:
        sprintf(buf, "Corrupted %s file deleted.",
                "$A170066176Scooby-Doo: Night of 100 Frights$C");
        break;
    case 12:
        sprintf(buf, "$A%sDelete Failed!$C\nUnabled to delete corrupt %s save file.\n",
                "200040046", "$A170066176Scooby-Doo: Night of 100 Frights$C");
        break;
    case 13:
        sprintf(buf, "$A%sWARNING!$C\nDeleting. Do not touch the %s or the %s.\n", "200040046",
                card, "POWER Button");
        break;
    case 14:
    case 16:
        sprintf(buf,
                "The %s contains corrupted data and needs to be formatted. Do you wish to "
                "format this Memory Card?",
                card);
        break;
    case 15:
    case 17:
        sprintf(buf,
                "The %s is formatted for another market.\n\nTo save a %s file, this Memory "
                "Card needs to be formatted. Do you wish to format this Memory Card?",
                card, "$A170066176Scooby-Doo: Night of 100 Frights$C");
        break;
    case 18:
        sprintf(buf,
                "$A%sCAUTION$C\nFormatting the %s will erase all previously saved "
                "data.\n\nAre you sure you wish to format?\n",
                "200040046", card);
        break;
    case 19:
        sprintf(buf, "$A%sWARNING!$C\nFormatting. Do not touch the %s or the %s.\n",
                "200040046", card, "POWER Button");
        break;
    case 20:
        sprintf(buf, "Format Successful.\n");
        break;
    case 21:
        sprintf(buf, "$A%sFormat Failed!$C\nCheck the %s and try again.\n", "200040046", card);
        break;
    case 22:
        sprintf(buf, "$A%sFormat Failed!$C\nThe %s is damaged and cannot be used.\n",
                "200040046", card, "^");
        break;
    case 23:
        sprintf(buf,
                "$A%sCAUTION$C\nThis will restart the Nintendo GameCube%s.\n\nYour current "
                "game data will be lost.\nAre you sure you wish to restart?\n",
                "200040046", "^");
        break;
    case 24:
        sprintf(buf, "No %s\nfound in %s.\n\nPlease insert a\n%s.\n", device, slotName, device);
        break;
    case 25:
        sprintf(buf, "No %s found in Slot A or B.\n\nPlease insert a\n%s.\n", device, device);
        break;
    case 26:
        sprintf(buf, "The %s is damaged and cannot be used.\n", card);
        break;
    case 27:
        sprintf(buf, "Wrong device in %s.\n\nPlease insert a\n%s.\n", slotName, device);
        break;
    case 28:
        sprintf(buf, "The %s is not compatible with the %s save file.", card,
                "$A170066176Scooby-Doo: Night of 100 Frights$C");
        break;
    case 29:
        sprintf(buf,
                "The %s is formatted for a market not compatible with the save file format of "
                "%s.\nTo avoid accidental data loss in this situation, reformatting is not "
                "supported.\nPlease review the %s using the Nintendo GameCube%s IPL Memory "
                "Card Browser.\n",
                card, "$A170066176Scooby-Doo: Night of 100 Frights$C", card, "^");
        break;
    case 30:
        sprintf(buf, "The %s appears to be damaged and cannot be used.\n", card);
        break;
    case 31:
        sprintf(buf, "The %s is not compatible with the save file format of\n%s.\n", card,
                "$A170066176Scooby-Doo: Night of 100 Frights$C");
        break;
    case 32:
        blocks = (info->needed + info->blockSize - 1) / info->blockSize;
        sprintf(buf,
                "Not enough space on the %s.\nAt least $A%s%d file%s and %d block%s "
                "required.\n",
                card, "000210068", info->needFile, info->needFile == 1 ? "" : "s", blocks,
                blocks == 1 ? "$C is" : "s$C are");
        break;
    case 33:
        blocks = (info->needed + info->blockSize - 1) / info->blockSize;
        sprintf(buf,
                "Not enough space on the %s to create a new %s file.\nAt least $A%s%d file%s "
                "and %d block%s required.\nExisting files may be loaded and overwritten.\n",
                card, "$A170066176Scooby-Doo: Night of 100 Frights$C", "000210068",
                info->needFile, info->needFile == 1 ? "" : "s", blocks,
                blocks == 1 ? "$C is" : "s$C are");
        break;
    default:
        sprintf(buf, "$A255050080<<internal error>>$C");
        break;
    }
}

// Checks the card in a slot before a save: returns 0 and sets a message if
// it can't be used.
extern "C" signed int fn_80041310(signed int slot, signed int skipTarget)
{
    signed int result = 1;
    signed int tgt = -1;
    st_ISGSESSION* session;
    signed int msg = 0;
    session = iSGSessionBegin(NULL, NULL);
    signed int state = iSGCheckMemoryCard(session, slot);

    if ((state & 1) && !(state & 2))
    {
        xTRCCardBegin(1, slot, slot);
        xTRCCardMessage(1, 0x1B);
        return 0;
    }
    if (skipTarget)
    {
        return 1;
    }

    signed int count = iSGTgtCount(session, NULL);
    if (count < 1)
    {
        result = 0;
        tgt = -1;
    }
    else if (count == 2)
    {
        tgt = slot;
    }
    else
    {
        signed int phys = iSGTgtPhysSlotIdx(session, 0);
        if (phys == slot)
        {
            tgt = 0;
        }
    }

    if (tgt < 0)
    {
        xTRCCardBegin(1, slot, slot);
        xTRCCardMessage(1, 0x18);
        return 0;
    }

    iSGTgtSetActive(session, tgt);
    unsigned int tstate = iSGTgtState(session, tgt, NULL);
    if (tstate & 0x1000000)
    {
        msg = 0x1A;
    }
    else if (tstate & 0x8000000)
    {
        msg = 0x1C;
    }
    else if (tstate & 0x2000000)
    {
        msg = 0x1E;
    }
    else if (!(tstate & 4))
    {
        msg = 0x1F;
    }

    if (msg != 0)
    {
        result = 0;
        xTRCCardBegin(1, slot, slot);
        xTRCCardMessage(1, msg);
    }

    iSGSessionEnd(session);
    return result;
}

// Looks for corrupt game files on a card (and deletes them if fix is set).
extern "C" signed int fn_800411A8(signed int slot, signed int fix, signed int* firstBad,
                                  signed int* fixed)
{
    signed int result = 1;
    st_ISGSESSION* session;
    signed int tgt = -1;
    session = iSGSessionBegin(NULL, NULL);

    if (firstBad != NULL)
    {
        *firstBad = 0;
    }
    if (fixed != NULL)
    {
        *fixed = 0;
    }

    signed int count = iSGTgtCount(session, NULL);
    if (count < 1)
    {
        tgt = -1;
    }
    else if (count == 2)
    {
        tgt = slot;
    }
    else
    {
        signed int phys = iSGTgtPhysSlotIdx(session, 0);
        if (phys == slot)
        {
            tgt = 0;
        }
    }

    if (tgt < 0)
    {
        result = -1;
    }
    else if (!iSGTgtSetActive(session, tgt))
    {
        result = -1;
    }
    else if (fix)
    {
        signed int firstFixed = 0;
        signed int ok = iSGCheckForCorruptFiles(session, 1, NULL, &firstFixed);
        if (ok == 0 && firstFixed >= 0)
        {
            if (fixed != NULL)
            {
                *fixed = 1;
            }
            result = 0;
        }
        else if (ok == 0)
        {
            result = 0;
        }
    }
    else if (iSGCheckForCorruptFiles(session, 0, firstBad, NULL) == 0)
    {
        result = 0;
    }

    iSGSessionEnd(session);
    return result;
}

// Checks that a game file fits on the card in a slot: 1 yes, 0 no (message
// set), -1 the card can't be used.
extern "C" signed int fn_80040FCC(signed int slot, signed int fileIdx)
{
    signed int result;
    signed int tgt = -1;
    signed int needed = 0;
    signed int avail = 0;
    signed int bytes = 0;
    signed int needFile = 0;
    signed int files = 0;
    st_ISGSESSION* session = iSGSessionBegin(NULL, NULL);
    signed int count = iSGTgtCount(session, NULL);

    if (count >= 1)
    {
        if (count == 2)
        {
            tgt = slot;
        }
        else
        {
            signed int phys = iSGTgtPhysSlotIdx(session, 0);
            if (phys == slot)
            {
                tgt = 0;
            }
        }
    }

    if (tgt < 0)
    {
        result = -1;
    }
    else
    {
        iSGTgtSetActive(session, tgt);
        unsigned int state = iSGTgtState(session, tgt, NULL);
        if (state & 0x1000000)
        {
            result = -1;
        }
        else if (state & 0x8000000)
        {
            result = -1;
        }
        else if (state & 0x2000000)
        {
            result = -1;
        }
        else if (!(state & 4))
        {
            result = -1;
        }
        else if (!(state & 2))
        {
            result = -1;
        }
        else
        {
            iSGTgtFreeSpace(session, tgt, &bytes, &files);
            char* name = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, fileIdx);
            if (iSGTgtHaveRoom(session, tgt, 0x6000, NULL, name, &needed, &avail, &needFile))
            {
                result = 1;
            }
            else
            {
                result = 0;
                xTRCCardMessage(1, 4);
                xTRCCardSpace(1, avail, needed, bytes, files, needFile);
            }
        }
    }

    iSGSessionEnd(session);
    return result;
}

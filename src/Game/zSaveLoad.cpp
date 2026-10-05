// TODO: 99.8% (NonMatching). Left: register allocation in SaveGame, LoadGame,
// CardCheckSlotOverwrite and CardCheckFormattedSingle (the `*badEncode = 0`
// store should reuse rc's zero register); .rodata is missing an unreferenced
// 32-byte zero buffer after xSGT_LoadLoadCB's bigbuf.
// zSaveLoad sees xSGGameIsEmpty as returning int (no clrlwi at the call sites),
// so hide the header's unsigned char prototype and declare it here.
#define xSGGameIsEmpty xSGGameIsEmpty_header
#include "xsavegame.h"
#undef xSGGameIsEmpty
#include "zGlobals.h"
#include "iTime.h"

#ifndef NULL
#define NULL 0
#endif

class _xEnt;
void zEntPlayer_SNDVan(_xEnt* ent);

extern "C" {
int sprintf(char* s, const char* format, ...);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, unsigned int n);
int strcmp(const char* a, const char* b);
unsigned int strlen(const char* s);
unsigned int fn_800D8C14();
// TODO: unidentified (xDebug: per-frame debug update)
void fn_80021B20();
// iSystem: ask for a reset to the IPL menu
void fn_80016698(signed int toMenu);
// xTRC: memory card message state for the save/load screens
void fn_80041D58(signed int idx);
void fn_80041CE8(signed int idx, signed int tgt, signed int slot);
void fn_80041CC4(signed int idx, signed int msg);
void fn_80041C78(signed int idx, signed int avail, signed int needed, signed int blockSize,
                 signed int files, signed int needFile);
void fn_80041C4C(signed int idx, signed int fileIdx);
signed int fn_80041BC8(signed int idx, char* buf);
signed int fn_80041B10(signed int idx, char* buf);
signed int fn_80041ADC(signed int idx, char* buf);
signed int fn_80041548(signed int idx, char* buf);
signed int fn_80040FCC(signed int slot, signed int fileIdx);
signed int fn_800411A8(signed int slot, signed int fix, signed int* firstBad, signed int* fixed);
signed int fn_80041310(signed int slot, signed int skipTarget);
}

class xBase;
class xSerial;
#include "zScene.h"
class _tagxCamFrame;
class iEnv;
class xSndGlobalsLike
{
public:
    unsigned int stereo;  // 0x0
    unsigned char pad4[0xC];
    float categoryVolFader[4]; // 0x10
};

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

extern xSndGlobalsLike gSnd;
extern unsigned int gFrameCount;
extern unsigned int gScreenDump;

xBase* zSceneFindObject(unsigned int id);
void zEntEvent(xBase* to, unsigned int toEvent);
unsigned int xStrHash(char* str);
void zGameModeSwitch(eGameMode mode);
void zGameStateSwitch(signed int state);
void zEntPlayer_SNDSavePickup(_xEnt* ent);
signed int xPadUpdate(signed int idx, float time_passed);
void xPadRumbleEnable(signed int idx, signed int enable);
void xDrawBegin();
void xDrawEnd();
void xParMgrUpdate(float dt);
void zSceneUpdate(float dt);
void zSceneRender();
void zSceneSave(zScene* s, xSerial* ser);
signed int zSceneCalcProgress();
char* zSceneGetAreaname(unsigned int id);
void xSndSetFrame(_xVec3* right, _xVec3* up, _xVec3* at, _xVec3* pos, float scale);
void xSndSetFrameFar(_xVec3* pos);
void xSndUpdate(float dt);
void xCameraUpdate(_tagxCamFrame* cam, float dt);
void xCameraBegin(_tagxCamFrame* cam, signed int clear);
void xCameraEnd(_tagxCamFrame* cam, float seconds, signed int update_scrn_fx);
void xCameraShowRaster(_tagxCamFrame* cam);
void iEnvEndRenderFX(iEnv* env);
void iDebugScreenDump();
void xTRCRender();
char* xUtil_idtag2string(unsigned int id, signed int r);
void xSerial_svgame_register(st_XSAVEGAME_DATA* xsg, en_SAVEGAME_MODE mode);
signed int xSGWriteStrLen(char* str);
signed int xSGReadData(st_XSAVEGAME_DATA* xsg, st_XSAVEGAME_READCONTEXT* rctxt, char* buf,
                       signed int elesize, signed int n);
signed int xSGReadData(st_XSAVEGAME_DATA* xsg, st_XSAVEGAME_READCONTEXT* rctxt,
                       unsigned int* buf, signed int n);
signed int xSGReadData(st_XSAVEGAME_DATA* xsg, st_XSAVEGAME_READCONTEXT* rctxt, float* buf,
                       signed int n);
signed int xSGReadData(st_XSAVEGAME_DATA* xsg, st_XSAVEGAME_READCONTEXT* rctxt, signed int* buf,
                       signed int n);
signed int xSGWriteData(st_XSAVEGAME_DATA* xsg, st_XSAVEGAME_WRITECONTEXT* wctxt,
                        unsigned int* data, signed int n);
signed int xSGWriteData(st_XSAVEGAME_DATA* xsg, st_XSAVEGAME_WRITECONTEXT* wctxt, float* data,
                        signed int n);
signed int xSGWriteData(st_XSAVEGAME_DATA* xsg, st_XSAVEGAME_WRITECONTEXT* wctxt,
                        signed int* data, signed int n);

extern "C" signed int fn_800C9368(signed int mode, signed int type, unsigned char (*cb)(signed int),
                                  signed int arg);
unsigned char zSaveLoad_poll(signed int sel);
signed int zSaveLoad_CardCheckGames(signed int cardNumber, signed int mode);
signed int zSaveLoad_CardCheckSpace(signed int cardNumber, signed int mode, signed int checkGames);
signed int zSaveLoad_CardCheckSlotEmpty(signed int cardNumber, signed int gameNumber);
signed int zSaveLoad_CardCheckSlotOverwrite(signed int cardNumber, signed int gameNumber);
signed int xSGT_SaveInfoCB(void* vp, st_XSAVEGAME_DATA* xsgdata, signed int* need, signed int* most);
signed int xSGT_SaveProcCB(void* vp, st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt);
signed int xSGT_SaveInfoPrefsCB(void* vp, st_XSAVEGAME_DATA* xsgdata, signed int* need,
                                signed int* most);
signed int xSGT_SaveProcPrefsCB(void* vp, st_XSAVEGAME_DATA* xsgdata,
                                st_XSAVEGAME_WRITECONTEXT* wctxt);
signed int xSGT_LoadLoadCB(void* vp, st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt,
                           unsigned int ui, signed int i);
signed int xSGT_LoadPrefsCB(void* vp, st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt,
                            unsigned int ui, signed int i);

signed int xSGGameIsEmpty(st_XSAVEGAME_DATA* xsgdata, signed int gidx);

class zSaveLoadUI
{
public:
    unsigned int entry;  // 0x0
    unsigned int nameID; // 0x4
    char* name;          // 0x8
};

class zSaveLoadGame
{
public:
    char label[64];      // 0x0
    char date[32];       // 0x40
    signed int progress; // 0x60
};

static unsigned int saveSuccess;
static float time_elapsed = 0.01f;
static float time_last;
static float time_current;
static iTime t0;
static iTime t1;
static signed int currentCard = -1;
static signed int currentGame = -1;
static signed int promptSel;
static signed int badCard;
// Lets the B button back out of the current prompt (GameCube only).
static signed int sAllowCancel;
static float dontPoll = 1.0f;
static char currSceneStr[32] = "TEMP";
static char sceneRead[32] = "0000";

zSaveLoadUI zSaveLoadUITable[61] = {
    { 0, 0, "ld gameslot group" },
    { 1, 0, "ld memcards group" },
    { 2, 0, "ld format prompt group" },
    { 3, 0, "ld mc missing group" },
    { 4, 0, "mnu3 ld mc1" },
    { 5, 0, "mnu3 ld mc2" },
    { 6, 0, "ld gameslot 0" },
    { 7, 0, "ld gameslot 1" },
    { 8, 0, "ld gameslot 2" },
    { 9, 0, "ld gameslot 3" },
    { 10, 0, "ld gameslot 4" },
    { 11, 0, "ld gameslot 5" },
    { 12, 0, "ld gameslot 6" },
    { 13, 0, "ld gameslot 7" },
    { 14, 0, "ld format prompt" },
    { 15, 0, "ld format yes" },
    { 16, 0, "ld format no" },
    { 17, 0, "ld mc missing" },
    { 18, 0, "ld nogames" },
    { 19, 0, "ld nogame" },
    { 20, 0, "mnu3 start group" },
    { 21, 0, "sv gameslot group" },
    { 22, 0, "sv memcards group" },
    { 23, 0, "sv mc1" },
    { 24, 0, "sv mc2" },
    { 25, 0, "sv gameslot 0" },
    { 26, 0, "sv gameslot 1" },
    { 27, 0, "sv gameslot 2" },
    { 28, 0, "sv gameslot 3" },
    { 29, 0, "sv gameslot 4" },
    { 30, 0, "sv gameslot 5" },
    { 31, 0, "sv gameslot 6" },
    { 32, 0, "sv gameslot 7" },
    { 33, 0, "sv format prompt" },
    { 34, 0, "sv overwrite" },
    { 35, 0, "sv mc missing" },
    { 36, 0, "sv nospace" },
    { 37, 0, "sv nospacegame" },
    { 38, 0, "ld mc dontremove" },
    { 39, 0, "sv mc dontremove" },
    { 40, 0, "mnu4 mc dontremove" },
    { 41, 0, "ld badload" },
    { 42, 0, "sv badsave" },
    { 43, 0, "mnu3 badformat" },
    { 44, 0, "mnu4 badformat" },
    { 45, 0, "mnu4 badformat cantrecover" },
    { 46, 0, "mnu3 format confirm" },
    { 47, 0, "mnu4 format confirm" },
    { 48, 0, "sv format group" },
    { 49, 0, "ld format prompt group" },
    { 50, 0, "ld badload" },
    { 51, 0, "mnu3 badformat" },
    { 52, 0, "ld format prompt group" },
    { 53, 0, "ld format prompt group" },
    { 54, 0, "ld_nospace" },
    { 55, 0, "sv badsave" },
    { 56, 0, "mnu4 badformat" },
    { 57, 0, "sv mc missing" },
    { 58, 0, "sv format group" },
    { 59, 0, "sv nospace" },
    { 0, 0, NULL },
};

zSaveLoadGame zSaveLoadGameTable[3];

static inline void zSaveLoad_UIEvent(signed int i, unsigned int toEvent)
{
    zEntEvent(zSceneFindObject(zSaveLoadUITable[i].nameID), toEvent);
}

// Is the card for this slot still in? 1 yes, 0 no, -1 can't tell.
static inline signed int zSaveLoad_CardCheckSingle(signed int cardNumber)
{
    st_XSAVEGAME_DATA* ldinst = xSGInit(XSG_MODE_LOAD);
    signed int tgtmax;

    switch (xSGTgtCount(ldinst, &tgtmax))
    {
    case 2:
        xSGDone(ldinst);
        return 1;
    case 1:
    {
        signed int idx = xSGTgtPhysSlotIdx(ldinst, 0);
        xSGDone(ldinst);
        if (idx == cardNumber)
        {
            return 1;
        }
        return 0;
    }
    case 0:
        xSGDone(ldinst);
        return 0;
    }
    return -1;
}

static inline void zSaveLoad_pollCheck(signed int sel)
{
    if (dontPoll < 0.0f)
    {
        dontPoll = 1.0f;
    }
    else if (zSaveLoad_CardCheckSingle(currentCard) == 0)
    {
        promptSel = sel;
        currentGame = 0;
    }
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zSaveLoad_Tick()
{
    time_current = (1.0f / (*(unsigned int*)0x800000F8 >> 2)) * iTimeGet();
    time_elapsed = time_current - time_last;
    if (time_elapsed < 0.0f)
    {
        time_elapsed = 1.0f / 60.0f;
    }
    time_last = time_current;
    dontPoll -= time_elapsed;
    t0 = t1;
    t1 = iTimeGet();

    xPadUpdate(0, time_elapsed);
    iTRCDisk::CheckDVDAndResetState();
    if (globals.pad1 != NULL)
    {
        xPadUpdate(1, time_elapsed);
    }
    xDrawBegin();
    xParMgrUpdate(time_elapsed);
    zSceneUpdate(time_elapsed);
    xSndSetFrame(&globals.camera.hi_cam.mat.right, &globals.camera.hi_cam.mat.up,
                 &globals.camera.hi_cam.mat.at, &globals.camera.hi_cam.mat.pos, 25.0f);
    xSndSetFrameFar((_xVec3*)&((_xMat4x3*)globals.player.ent.model->Mat)->pos);
    xSndUpdate(time_elapsed);
    xCameraUpdate(&globals.camera, time_elapsed);
    xCameraBegin(&globals.camera, 1);
    zSceneRender();
    fn_80021B20();
    xTRCRender();
    xDrawEnd();
    if (gScreenDump)
    {
        iDebugScreenDump();
    }
    xCameraEnd(&globals.camera, time_elapsed, 1);
    iEnvEndRenderFX(NULL);
    xCameraShowRaster(&globals.camera);
    gFrameCount++;
}

signed int zSaveLoad_getgame()
{
    return currentGame;
}

signed int zSaveLoad_getcard()
{
    return currentCard;
}

static inline void zSaveLoad_ErrorFormatPrompt(signed int mode, signed int canRecover)
{
    if (canRecover)
    {
        fn_80041CC4(1, 0x15);
    }
    else
    {
        fn_80041CC4(1, 0x16);
    }
    signed int i = mode == 1 ? 0x2B : 0x2C;
    zSaveLoad_UIEvent(i, 0x5E);
    promptSel = -1;
    while (promptSel == -1)
    {
        zSaveLoad_Tick();
    }
    zSaveLoad_UIEvent(i, 0x5F);
}

signed int format(signed int num, signed int mode)
{
    zSaveLoad_UIEvent(0x28, 0x51);
    zSaveLoad_Tick();

    st_XSAVEGAME_DATA* data = xSGInit(XSG_MODE_LOAD);
    signed int tgtmax;
    signed int canRecover = 0;
    signed int rc = 0;

    switch (xSGTgtCount(data, &tgtmax))
    {
    case 2:
        xSGTgtSelect(data, num);
        rc = xSGTgtFormatTgt(data, num, &canRecover);
        zSaveLoad_UIEvent(0x28, 0x52);
        xSGDone(data);
        if (rc == 0)
        {
            zSaveLoad_ErrorFormatPrompt(mode, canRecover);
        }
        if (rc != 0)
        {
            rc = 1;
        }
        break;
    case 1:
    {
        signed int idx = xSGTgtPhysSlotIdx(data, 0);
        if (idx != num)
        {
            zSaveLoad_UIEvent(0x28, 0x52);
            xSGDone(data);
            rc = 5;
        }
        else if (idx ^ num)
        {
            zSaveLoad_UIEvent(0x28, 0x52);
            xSGDone(data);
        }
        else
        {
            xSGTgtSelect(data, 0);
            rc = xSGTgtFormatTgt(data, 0, &canRecover);
            zSaveLoad_UIEvent(0x28, 0x52);
            xSGDone(data);
            if (rc == 0)
            {
                zSaveLoad_ErrorFormatPrompt(mode, canRecover);
            }
        }
        break;
    }
    case 0:
        rc = 5;
        zSaveLoad_UIEvent(0x28, 0x52);
        xSGDone(data);
        break;
    }
    return rc;
}

signed int zSaveLoad_CardCheckFormattedSingle(signed int cardNumber, signed int* badEncode)
{
    signed int rc;
    st_XSAVEGAME_DATA* ldinst = xSGInit(XSG_MODE_LOAD);
    signed int tgtmax;

    rc = 0;
    if (badEncode != NULL)
    {
        *badEncode = rc;
    }

    switch (xSGTgtCount(ldinst, &tgtmax))
    {
    case 2:
        rc = xSGTgtIsFormat(ldinst, cardNumber, badEncode);
        if (rc != 0)
        {
            rc = 1;
        }
        if (rc == 0)
        {
            rc = 0;
        }
        break;
    case 1:
    {
        signed int idx = xSGTgtPhysSlotIdx(ldinst, 0);
        if (idx != cardNumber)
        {
            rc = -1;
        }
        else if (!(idx ^ cardNumber))
        {
            if (xSGTgtIsFormat(ldinst, 0, badEncode) == 0)
            {
                rc = 0;
            }
            else
            {
                rc = 1;
            }
        }
        break;
    }
    case 0:
        rc = -1;
        break;
    }

    xSGDone(ldinst);
    return rc;
}

signed int zSaveLoad_CardCheckSpaceSingle_doCheck(st_XSAVEGAME_DATA* xsgdata, signed int cardNumber,
                                                  signed int checkGames, signed int* gameFull)
{
    signed int badEncode = 0;
    signed int avail = 0;
    signed int files = 0;
    signed int needed = 0;
    signed int needFile = 0;
    signed int bytes = 0;
    signed int rc;

    if (gameFull != NULL)
    {
        *gameFull = 0;
    }

    xSGTgtSelect(xsgdata, cardNumber);
    if (xSGTgtIsFormat(xsgdata, cardNumber, &badEncode) == 0)
    {
        return 6;
    }

    signed int hasDir = xSGTgtHasGameDir(xsgdata, cardNumber);
    xSGTgtFreeSpace(xsgdata, cardNumber, &bytes, &files);
    if (hasDir == 1)
    {
        rc = xSGTgtHaveRoom(xsgdata, cardNumber, 0x6000, -1, &needed, &avail, &needFile);
    }
    else
    {
        rc = xSGTgtHaveRoom(xsgdata, cardNumber, 0x6000, -1, &needed, &avail, &needFile);
    }

    if (rc == 0)
    {
        fn_80041CE8(1, cardNumber, xSGTgtPhysSlotIdx(xsgdata, cardNumber));
        fn_80041CC4(1, 0x20);
        fn_80041C78(1, avail, needed, bytes, files, needFile);
    }

    if (rc == 0)
    {
        rc = 0;
    }
    else if (checkGames)
    {
        rc = 1;
        for (signed int i = 0; i < 3; i++)
        {
            if (xSGGameIsEmpty(xsgdata, i))
            {
                if (xSGTgtHaveRoom(xsgdata, cardNumber, 0x6000, i, &needed, &avail, &needFile))
                {
                    rc = 1;
                }
                else
                {
                    fn_80041CE8(1, cardNumber, xSGTgtPhysSlotIdx(xsgdata, cardNumber));
                    fn_80041CC4(1, 0x21);
                    fn_80041C78(1, avail, needed, bytes, files, needFile);
                    rc = 0;
                    if (gameFull != NULL)
                    {
                        *gameFull = 1;
                    }
                }
                break;
            }
        }
    }
    return rc;
}

static inline void zSaveLoad_CardPrompt(signed int mode)
{
    signed int i = 0x15;
    if (mode == 1)
    {
        i = 0;
    }
    zSaveLoad_UIEvent(i, 1);
    i = 0x15;
    if (mode == 1)
    {
        i = 0;
    }
    zSaveLoad_UIEvent(i, 0x5F);
    i = mode == 1 ? 1 : 0x16;
    zSaveLoad_UIEvent(i, 0x5F);
    i = mode == 1 ? 0x11 : 0x23;
    zSaveLoad_UIEvent(i, 0x5E);

    promptSel = -1;
    while (promptSel == -1)
    {
        zSaveLoad_Tick();
    }

    i = mode == 1 ? 0x11 : 0x23;
    zSaveLoad_UIEvent(i, 0x5F);
    if (mode == 1)
    {
        zSaveLoad_UIEvent(0, 2);
    }
}

signed int zSaveLoad_CardCheck(signed int cardNumber, signed int mode)
{
    signed int retry;

    do
    {
        if (fn_80041310(cardNumber, 1) != 0)
        {
            retry = 0;
        }
        else
        {
            fn_80041CE8(1, cardNumber, cardNumber);
            fn_80041CC4(1, 0x1B);
            zSaveLoad_CardPrompt(mode);
            if (promptSel == 2 || promptSel == 4)
            {
                return 2;
            }
            retry = 1;
        }

        if (retry == 0)
        {
            while (zSaveLoad_CardCheckSingle(cardNumber) == 0)
            {
                badCard = 1;
                fn_80041CE8(1, cardNumber, cardNumber);
                fn_80041CC4(1, 0x18);
                zSaveLoad_CardPrompt(mode);
                if (promptSel == 2 || promptSel == 4)
                {
                    return 2;
                }
                retry = 1;
                break;
            }

            if (retry == 0)
            {
                if (fn_80041310(cardNumber, 0) != 0)
                {
                    retry = 0;
                }
                else
                {
                    zSaveLoad_CardPrompt(mode);
                    if (promptSel == 2 || promptSel == 4)
                    {
                        return 2;
                    }
                    retry = 1;
                }
            }
        }
    } while (retry != 0);
    return 1;
}

static inline signed int zSaveLoad_CardPromptFormat(signed int mode)
{
    signed int i = 0x15;
    if (mode == 1)
    {
        i = 0;
    }
    zSaveLoad_UIEvent(i, 0x5F);
    i = mode == 1 ? 1 : 0x16;
    zSaveLoad_UIEvent(i, 0x5F);
    i = 0x30;
    if (mode == 1)
    {
        i = 0x31;
    }
    zSaveLoad_UIEvent(i, 0x51);

    promptSel = -1;
    while (promptSel == -1)
    {
        zSaveLoad_Tick();
        zSaveLoad_pollCheck(5);
    }

    i = 0x30;
    if (mode == 1)
    {
        i = 0x31;
    }
    zSaveLoad_UIEvent(i, 0x5F);
    return promptSel;
}

signed int zSaveLoad_CardCheckFormatted(signed int cardNumber, signed int mode)
{
    signed int badEncode = 0;
    signed int rc;

    while (zSaveLoad_CardCheckFormattedSingle(cardNumber, &badEncode) != 1)
    {
        badCard = 1;
        if (zSaveLoad_CardCheckFormattedSingle(cardNumber, &badEncode) == -1)
        {
            return -1;
        }

        signed int msg;
        if (badEncode && mode == 0)
        {
            msg = 0x11;
        }
        else if (mode == 0)
        {
            msg = 0x10;
        }
        else if (badEncode)
        {
            msg = 0xF;
        }
        else
        {
            msg = 0xE;
        }
        fn_80041CE8(1, cardNumber, cardNumber);
        fn_80041CC4(1, msg);

        zSaveLoad_CardPromptFormat(mode);
        if (promptSel == 5 || promptSel == 2 || promptSel == 4)
        {
            return 2;
        }

        fn_80041CE8(1, cardNumber, cardNumber);
        fn_80041CC4(1, 0x12);
        signed int sel = fn_800C9368(mode, 3, zSaveLoad_poll, 0);
        if (sel == 5 || sel == 2 || sel == 4)
        {
            return 2;
        }

        fn_80041CE8(1, cardNumber, cardNumber);
        fn_80041CC4(1, 0x13);
        zSaveLoad_UIEvent(0x28, 0x51);
        zSaveLoad_Tick();
        rc = format(cardNumber, mode);
        zSaveLoad_UIEvent(0x28, 0x52);
        if (rc != 1)
        {
            return rc;
        }
        fn_80041CE8(1, cardNumber, cardNumber);
        fn_80041CC4(1, 0x14);
        fn_800C9368(mode, 0, NULL, 0);
    }

    fn_80041D58(1);
    iTRCDisk::CheckDVDAndResetState();
    signed int firstBad = -1;
    signed int fixed = 0;
    rc = fn_800411A8(cardNumber, 0, &firstBad, NULL);
    if (rc <= 0)
    {
        if (rc < 0)
        {
            return 2;
        }

        fn_80041CE8(1, cardNumber, cardNumber);
        fn_80041CC4(1, 9);
        fn_80041C4C(1, firstBad);
        signed int sel = fn_800C9368(mode, 3, zSaveLoad_poll, 0);
        if (sel == 5 || sel == 2 || sel == 4)
        {
            return 2;
        }

        fn_80041CE8(1, cardNumber, cardNumber);
        fn_80041CC4(1, 0xA);
        sel = fn_800C9368(mode, 3, zSaveLoad_poll, 0);
        if (sel == 5 || sel == 2 || sel == 4)
        {
            return 2;
        }

        fn_80041CE8(1, cardNumber, cardNumber);
        fn_80041CC4(1, 0xD);
        zSaveLoad_UIEvent(0x28, 0x51);
        zSaveLoad_Tick();
        rc = fn_800411A8(cardNumber, 1, NULL, &fixed);
        zSaveLoad_UIEvent(0x28, 0x52);
        if (rc > 0)
        {
            fn_80041CE8(1, cardNumber, cardNumber);
            fn_80041CC4(1, 0xB);
            fn_800C9368(mode, 0, NULL, 0);
        }
        else if (rc == 0 && fixed)
        {
            fn_80041CE8(1, cardNumber, cardNumber);
            fn_80041CC4(1, 0xB);
            fn_800C9368(mode, 0, NULL, 0);
            return 2;
        }
        else
        {
            fn_80041CE8(1, cardNumber, cardNumber);
            fn_80041CC4(1, 0xC);
            fn_800C9368(mode, 0, NULL, 0);
            return 2;
        }
    }

    fn_80041D58(1);
    iTRCDisk::CheckDVDAndResetState();
    return 1;
}

signed int zSaveLoad_CardCheckValid(signed int cardNumber, signed int mode, signed int checkGames)
{
    if (mode == 1)
    {
        return zSaveLoad_CardCheckGames(cardNumber, mode);
    }
    return zSaveLoad_CardCheckSpace(cardNumber, mode, checkGames);
}

static inline signed int zSaveLoad_CardCheckSpaceSingle(signed int cardNumber, signed int checkGames,
                                                        signed int* gameFull)
{
    st_XSAVEGAME_DATA* ldinst = xSGInit(XSG_MODE_SAVE);
    signed int tgtmax;
    signed int rc = 0;

    switch (xSGTgtCount(ldinst, &tgtmax))
    {
    case 2:
        rc = zSaveLoad_CardCheckSpaceSingle_doCheck(ldinst, cardNumber, checkGames, gameFull);
        if (rc != 0)
        {
            rc = 1;
        }
        break;
    case 1:
    {
        signed int idx = xSGTgtPhysSlotIdx(ldinst, 0);
        if (idx != cardNumber)
        {
            rc = 5;
        }
        if (!(idx ^ cardNumber))
        {
            rc = zSaveLoad_CardCheckSpaceSingle_doCheck(ldinst, 0, checkGames, gameFull);
        }
        break;
    }
    case 0:
        rc = 5;
        break;
    }

    xSGDone(ldinst);
    return rc;
}

signed int zSaveLoad_CardCheckSpace(signed int cardNumber, signed int mode, signed int checkGames)
{
    signed int gameFull = 0;
    signed int result = zSaveLoad_CardCheckSpaceSingle(cardNumber, checkGames, &gameFull);

    while (result != 1)
    {
        badCard = 1;
        if (result == -1)
        {
            return result;
        }
        if (result == 6)
        {
            return result;
        }
        if (result == 5)
        {
            return result;
        }

        sAllowCancel = 1;
        signed int i = 0x15;
        if (mode == 1)
        {
            i = 0;
        }
        zSaveLoad_UIEvent(i, 0x5F);
        zSaveLoad_UIEvent(0x16, 0x5F);
        zSaveLoad_UIEvent(0x24, 0x5E);
        promptSel = -1;
        while (promptSel == -1)
        {
            zSaveLoad_Tick();
            zSaveLoad_pollCheck(3);
        }
        zSaveLoad_UIEvent(0x24, 0x5F);
        signed int sel = promptSel;
        if (gameFull && sel == 4)
        {
            return 1;
        }
        sAllowCancel = 0;
        if (sel == 2)
        {
            fn_80041CE8(1, cardNumber, cardNumber);
            fn_80041CC4(1, 0x17);
            if (fn_800C9368(mode, 3, zSaveLoad_poll, 0) == 3)
            {
                fn_80016698(1);
                return 2;
            }
            return 2;
        }
        if (sel == 2 || sel == 4)
        {
            return 2;
        }
        result = zSaveLoad_CardCheckSpaceSingle(cardNumber, checkGames, &gameFull);
    }
    return 1;
}

static inline signed int zSaveLoad_CardCheckGamesSingle_doCheck(st_XSAVEGAME_DATA* xsgdata,
                                                               signed int cardNumber)
{
    signed int rc = 0;
    signed int badEncode = 0;

    xSGTgtSelect(xsgdata, cardNumber);
    if (xSGTgtIsFormat(xsgdata, cardNumber, &badEncode) == 0)
    {
        rc = 6;
    }
    else if (xSGTgtHasGameDir(xsgdata, cardNumber) == 0)
    {
        rc = 0;
    }
    else
    {
        for (signed int i = 0; i < 3; i++)
        {
            if (xSGGameIsEmpty(xsgdata, i) == 0)
            {
                rc = 1;
                break;
            }
        }
    }
    return rc;
}

static inline signed int zSaveLoad_CardCheckGamesSingle(signed int cardNumber)
{
    st_XSAVEGAME_DATA* ldinst = xSGInit(XSG_MODE_LOAD);
    signed int tgtmax;
    signed int rc = 0;

    switch (xSGTgtCount(ldinst, &tgtmax))
    {
    case 2:
        rc = zSaveLoad_CardCheckGamesSingle_doCheck(ldinst, cardNumber);
        if (rc != 0)
        {
            rc = 1;
        }
        break;
    case 1:
    {
        signed int idx = xSGTgtPhysSlotIdx(ldinst, 0);
        if (idx != cardNumber)
        {
            rc = 5;
        }
        if (!(idx ^ cardNumber))
        {
            rc = zSaveLoad_CardCheckGamesSingle_doCheck(ldinst, 0);
        }
        break;
    }
    case 0:
        rc = 5;
        break;
    }

    xSGDone(ldinst);
    return rc;
}

static inline signed int zSaveLoad_CardPromptGames(signed int mode)
{
    signed int i = 0x15;
    if (mode == 1)
    {
        i = 0;
    }
    zSaveLoad_UIEvent(i, 0x5F);
    zSaveLoad_UIEvent(1, 0x5F);
    zSaveLoad_UIEvent(0x12, 0x5E);
    promptSel = -1;
    while (promptSel == -1)
    {
        zSaveLoad_Tick();
        zSaveLoad_pollCheck(3);
    }
    zSaveLoad_UIEvent(0x12, 0x5F);
    return promptSel;
}

signed int zSaveLoad_CardCheckGames(signed int cardNumber, signed int mode)
{
    while (zSaveLoad_CardCheckGamesSingle(cardNumber) != 1)
    {
        badCard = 1;
        signed int result = zSaveLoad_CardCheckGamesSingle(cardNumber);
        if (result == -1 || result == 6 || result == 5)
        {
            return result;
        }

        zSaveLoad_CardPromptGames(mode);
        if (promptSel == 2 || promptSel == 4)
        {
            return 2;
        }
    }
    return 1;
}

signed int zSaveLoad_CardCheckGameSlot(signed int cardNumber, signed int gameNumber,
                                       signed int mode)
{
    if (mode == 1)
    {
        return zSaveLoad_CardCheckSlotEmpty(cardNumber, gameNumber);
    }
    return zSaveLoad_CardCheckSlotOverwrite(cardNumber, gameNumber);
}

static inline signed int zSaveLoad_CardCheckSlotEmpty_hasGame_doCheck(st_XSAVEGAME_DATA* xsgdata,
                                                                     signed int cardNumber,
                                                                     signed int gameNumber)
{
    signed int badEncode = 0;
    signed int rc;

    xSGTgtSelect(xsgdata, cardNumber);
    if (xSGTgtIsFormat(xsgdata, cardNumber, &badEncode) == 0)
    {
        rc = -1;
    }
    else
    {
        rc = xSGGameIsEmpty(xsgdata, gameNumber);
        if (rc != 0)
        {
            rc = 0;
        }
        else if (rc == 0)
        {
            rc = 1;
        }
    }
    return rc;
}

static inline signed int zSaveLoad_CardCheckSlotEmpty_hasGame(signed int cardNumber,
                                                             signed int gameNumber)
{
    st_XSAVEGAME_DATA* ldinst = xSGInit(XSG_MODE_LOAD);
    signed int tgtmax;
    signed int rc = 0;

    switch (xSGTgtCount(ldinst, &tgtmax))
    {
    case 2:
        rc = zSaveLoad_CardCheckSlotEmpty_hasGame_doCheck(ldinst, cardNumber, gameNumber);
        if (rc != 0)
        {
            rc = 1;
        }
        break;
    case 1:
    {
        signed int idx = xSGTgtPhysSlotIdx(ldinst, 0);
        if (idx != cardNumber)
        {
            rc = -1;
        }
        if (!(idx ^ cardNumber))
        {
            rc = zSaveLoad_CardCheckSlotEmpty_hasGame_doCheck(ldinst, 0, gameNumber);
        }
        break;
    }
    case 0:
        rc = -1;
        break;
    }
    xSGDone(ldinst);
    return rc;
}

signed int zSaveLoad_CardCheckSlotEmpty(signed int cardNumber, signed int gameNumber)
{
    while (zSaveLoad_CardCheckSlotEmpty_hasGame(cardNumber, gameNumber) != 1)
    {
        if (zSaveLoad_CardCheckSlotEmpty_hasGame(cardNumber, gameNumber) == -1)
        {
            return -1;
        }

        zSaveLoad_UIEvent(0, 0x5F);
        zSaveLoad_UIEvent(0x13, 0x5E);
        promptSel = -1;
        while (promptSel == -1)
        {
            zSaveLoad_Tick();
        }
        zSaveLoad_UIEvent(0x13, 0x5F);
        if (promptSel == 2 || promptSel == 4)
        {
            return 2;
        }
    }
    return 1;
}

static inline signed int zSaveLoad_CardCheckSlotOverwrite_Free(signed int cardNumber,
                                                              signed int gameNumber)
{
    signed int i = zSaveLoad_CardCheckSlotEmpty_hasGame(cardNumber, gameNumber);
    switch (i)
    {
    case -1:
        return i;
    case 0:
        return 1;
    case 1:
        return 0;
    default:
        return 0;
    }
}

signed int zSaveLoad_CardCheckSlotOverwrite(signed int cardNumber, signed int gameNumber)
{
    while (zSaveLoad_CardCheckSlotOverwrite_Free(cardNumber, gameNumber) != 1)
    {
        if (zSaveLoad_CardCheckSlotOverwrite_Free(cardNumber, gameNumber) == -1)
        {
            return -1;
        }

        zSaveLoad_UIEvent(0x15, 0x5F);
        zSaveLoad_UIEvent(0x22, 0x5E);
        promptSel = -1;
        while (promptSel == -1)
        {
            zSaveLoad_Tick();
            zSaveLoad_pollCheck(5);
        }
        zSaveLoad_UIEvent(0x22, 0x5F);
        if (promptSel == 5)
        {
            return promptSel;
        }
        if (promptSel == 2 || promptSel == 4)
        {
            return 2;
        }
        break;
    }
    return 1;
}

signed int zSaveLoad_CardPick(signed int mode)
{
    signed int done = 0;

    currentCard = -1;
    promptSel = -1;
    zSaveLoad_UIEvent(mode == 1 ? 1 : 0x16, 0x51);
    zSaveLoad_UIEvent(mode == 1 ? 4 : 0x17, 0x4F);

    while (done == 0 && promptSel == -1)
    {
        if (currentCard != -1)
        {
            done = zSaveLoad_CardCheck(currentCard, mode);
            switch (done)
            {
            case 2:
                currentCard = -1;
                promptSel = -1;
                continue;
            case -1:
                done = 0;
                promptSel = -1;
                continue;
            }

            done = zSaveLoad_CardCheckFormatted(currentCard, mode);
            switch (done)
            {
            case 5:
            case 6:
                done = 0;
                promptSel = -1;
                continue;
            case 2:
            case 4:
                currentCard = -1;
                promptSel = -1;
                continue;
            case -1:
                done = 0;
                promptSel = -1;
                continue;
            case 3:
                break;
            }

            done = zSaveLoad_CardCheckValid(currentCard, mode, 1);
            switch (done)
            {
            case 5:
            case 6:
                done = 0;
                promptSel = -1;
                continue;
            case 2:
            case 4:
                promptSel = -1;
                if (done != 1)
                {
                    currentGame = -1;
                }
                break;
            case -1:
                done = 0;
                promptSel = -1;
                continue;
            case 3:
                break;
            }
        }
        zSaveLoad_Tick();
    }

    zSaveLoad_UIEvent(mode == 1 ? 1 : 0x16, 0x5F);
    if (done == 1)
    {
        return done;
    }
    return promptSel != -1 ? promptSel : done;
}

static inline void zSaveLoad_BuildIt(char* build_txt, signed int i)
{
    char date1[32] = "";
    char date2[32];
    char biggerbuf[256] = "";

    strncpy(date1, zSaveLoadGameTable[i].date, 5);
    date1[2] = '/';
    sprintf(biggerbuf, "%s/%c%c", date1, zSaveLoadGameTable[i].date[8],
            zSaveLoadGameTable[i].date[9]);
    strncpy(date2, biggerbuf, sizeof(date2));
    date2[31] = '\0';
    sprintf(biggerbuf, "%s   %d%%\n%s", date2, zSaveLoadGameTable[i].progress,
            zSaveLoadGameTable[i].label);
    strncpy(build_txt, biggerbuf, 0x40);
    build_txt[0x3F] = '\0';
}

void zSaveLoad_BuildName(char* name_txt, signed int idx)
{
    char desired[64];
    char current_name[64];

    zSaveLoad_BuildIt(desired, idx);

    signed int counter = 0;
    for (signed int i = 0; i < idx; i++)
    {
        zSaveLoad_BuildIt(current_name, i);
        if (strcmp(current_name, desired) == 0)
        {
            counter++;
        }
    }

    if (counter > 0)
    {
        sprintf(name_txt, "%s (%d)", desired, counter);
        name_txt[0x3F] = '\0';
    }
    else
    {
        strcpy(name_txt, desired);
    }
}

// Maps a memory card slot to a save target index.
static inline signed int zSaveLoad_CardtoTgt(signed int card)
{
    st_XSAVEGAME_DATA* data = xSGInit(XSG_MODE_LOAD);
    signed int tgtmax;

    switch (xSGTgtCount(data, &tgtmax))
    {
    case 2:
        xSGDone(data);
        break;
    case 1:
        xSGDone(data);
        card = 0;
        break;
    case 0:
        xSGDone(data);
        card = -1;
        break;
    default:
        card = -1;
        break;
    }
    return card;
}

signed int zSaveLoad_GameSelect(signed int mode)
{
    signed int done = 0;
    signed int i;
    st_XSAVEGAME_DATA* svinst;
    signed int use_tgt;

    badCard = 1;
    while (badCard != 0)
    {
        currentGame = -1;
        promptSel = -1;
        use_tgt = zSaveLoad_CardtoTgt(currentCard);

        if (mode == 1)
        {
            zSaveLoad_UIEvent(0, 1);
            zSaveLoad_UIEvent(0, 0x51);
        }

        for (i = 0; i < 3; i++)
        {
            svinst = xSGInit(XSG_MODE_LOAD);
            xSGTgtSelect(svinst, use_tgt);
            if (xSGGameIsEmpty(svinst, i))
            {
                if (mode == 1)
                {
                    zSaveLoad_UIEvent(i + 6, 2);
                }
                strcpy(zSaveLoadGameTable[i].label, "Empty");
                strcpy(zSaveLoadGameTable[i].date, "Empty");
                zSaveLoadGameTable[i].progress = 0;
            }
            else
            {
                strcpy(zSaveLoadGameTable[i].label, xSGGameLabel(svinst, i));
                strcpy(zSaveLoadGameTable[i].date, xSGGameModDate(svinst, i));
                zSaveLoadGameTable[i].progress = xSGGameThumIndex(svinst, i);
            }
            xSGDone(svinst);
        }

        if (mode == 1)
        {
            zSaveLoad_UIEvent(0, 0x4F);
        }
        else
        {
            zSaveLoad_UIEvent(0x15, 0x51);
            zSaveLoad_UIEvent(0x19, 0x4F);
        }
        badCard = 0;

        while (done == 0 && promptSel == -1)
        {
            zSaveLoad_Tick();
            zSaveLoad_pollCheck(3);
            if (promptSel == 4)
            {
                done = promptSel;
            }

            if (currentGame == -1)
            {
                continue;
            }

            done = zSaveLoad_CardCheck(currentCard, mode);
            if (done != 1)
            {
                currentGame = -1;
                continue;
            }

            done = zSaveLoad_CardCheckFormatted(currentCard, mode);
            if (done != 1)
            {
                switch (done)
                {
                case 5:
                    done = 0;
                    promptSel = -1;
                    break;
                case 2:
                    currentGame = -1;
                    break;
                }
                continue;
            }

            done = zSaveLoad_CardCheckValid(currentCard, mode, 0);
            if (done != 1)
            {
                switch (done)
                {
                case 5:
                case 6:
                    done = 0;
                    promptSel = -1;
                    break;
                case 2:
                    currentGame = -1;
                    break;
                }
                continue;
            }

            if (badCard == 0)
            {
                done = zSaveLoad_CardCheckGameSlot(currentCard, currentGame, mode);
                if (done != 1)
                {
                    switch (done)
                    {
                    case -1:
                    case 5:
                        done = 0;
                        promptSel = -1;
                        break;
                    case 2:
                        currentGame = -1;
                        break;
                    }
                }
            }
            else
            {
                done = 0;
                currentGame = -1;
            }
        }
    }

    if (mode == 1)
    {
        zSaveLoad_UIEvent(0, 1);
    }

    signed int index = 0x15;
    if (mode == 1)
    {
        index = 0;
    }
    zSaveLoad_UIEvent(index, 0x5F);
    return done;
}

signed int zSaveLoad_SaveGame()
{
    en_XSGASYNC_STATUS asstat;
    signed int use_tgt;
    st_XSAVEGAME_DATA* xsgdata;
    signed int success;
    en_XSG_WHYFAIL whyFail;
    signed int teststat;
    signed int tgtmax;
    st_XSAVEGAME_DATA* ldinst;

    success = 0;
    teststat = 1;
    asstat = XSG_ASTAT_NOOP;
    use_tgt = currentCard;
    whyFail = XSG_WHYERR_NONE;
    ldinst = xSGInit(XSG_MODE_LOAD);

    switch (xSGTgtCount(ldinst, &tgtmax))
    {
    case 2:
        xSGDone(ldinst);
        break;
    case 1:
        xSGDone(ldinst);
        use_tgt = 0;
        break;
    case 0:
        xSGDone(ldinst);
        use_tgt = -1;
        break;
    default:
        use_tgt = -1;
        break;
    }

    zSaveLoad_UIEvent(0x28, 0x51);
    zSaveLoad_Tick();

    xsgdata = xSGInit(XSG_MODE_SAVE);
    xSGTgtSelect(xsgdata, use_tgt);
    xSGGameSet(xsgdata, currentGame);
    if (xsgdata == NULL)
    {
        teststat = 0;
    }

    zSceneSave(globals.sceneCur, NULL);
    xSGAddSaveClient(xsgdata, 'ROOM', NULL, xSGT_SaveInfoCB, xSGT_SaveProcCB);
    xSGAddSaveClient(xsgdata, 'PREF', NULL, xSGT_SaveInfoPrefsCB, xSGT_SaveProcPrefsCB);
    xSerial_svgame_register(xsgdata, XSG_MODE_SAVE);

    signed int progress = zSceneCalcProgress();
    char label[64];
    char* area = zSceneGetAreaname(globals.sceneCur->sceneID);
    strncpy(label, area, sizeof(label));
    label[63] = '\0';

    if (!xSGSetup(xsgdata, currentGame, label, progress, 0))
    {
        teststat = 0;
    }

    if (teststat)
    {
        signed int rc = xSGProcess(xsgdata);
        if (rc)
        {
            asstat = xSGAsyncStatus(xsgdata, 1, &whyFail, NULL);
        }

        xSGGameIsEmpty(xsgdata, currentGame);
        xSGTgtHasGameDir(xsgdata, use_tgt);
        if (!rc)
        {
            teststat = 0;
        }
        else
        {
            switch (asstat)
            {
            case XSG_ASTAT_SUCCESS:
                success = 1;
                break;
            case XSG_ASTAT_FAILED:
                success = 0;
                break;
            case XSG_ASTAT_NOOP:
            case XSG_ASTAT_INPROG:
                break;
            }
        }
    }

    if (teststat && !xSGWrapup(xsgdata))
    {
        teststat = 0;
    }

    zSaveLoad_UIEvent(0x28, 0x52);
    if (!xSGDone(xsgdata))
    {
        teststat = 0;
    }

    if (success && teststat)
    {
        return 1;
    }

    fn_80041CE8(1, currentCard, currentCard);
    signed int rc = whyFail == XSG_WHYERR_NOROOM ? fn_80040FCC(currentCard, currentGame) : -1;
    if (rc > 0)
    {
        fn_80041CC4(1, 3);
        fn_800C9368(0, 1, NULL, 0);
    }
    else if (rc < 0)
    {
        fn_80041CC4(1, 3);
        fn_800C9368(0, 1, NULL, 0);
    }
    else
    {
        fn_80041CC4(1, 4);
        if (fn_800C9368(0, 4, zSaveLoad_poll, 0) == 2)
        {
            fn_80041CC4(1, 0x17);
            if (fn_800C9368(0, 3, zSaveLoad_poll, 0) == 3)
            {
                fn_80016698(1);
            }
        }
    }
    return -1;
}

signed int zSaveLoad_LoadGame()
{
    signed int success = 0;
    signed int teststat = 1;
    en_XSGASYNC_STATUS asstat = XSG_ASTAT_NOOP;
    signed int use_tgt = currentCard;
    en_XSG_WHYFAIL whyFail = XSG_WHYERR_NONE;
    st_XSAVEGAME_DATA* ldinst = xSGInit(XSG_MODE_LOAD);
    signed int tgtmax;

    switch (xSGTgtCount(ldinst, &tgtmax))
    {
    case 2:
        xSGDone(ldinst);
        break;
    case 1:
        xSGDone(ldinst);
        use_tgt = 0;
        break;
    case 0:
        xSGDone(ldinst);
        use_tgt = -1;
        break;
    default:
        use_tgt = -1;
        break;
    }

    zSaveLoad_UIEvent(0x28, 0x51);
    zSaveLoad_Tick();

    st_XSAVEGAME_DATA* xsgdata = xSGInit(XSG_MODE_LOAD);
    xSGTgtSelect(xsgdata, use_tgt);
    xSGGameSet(xsgdata, currentGame);
    if (xsgdata == NULL)
    {
        teststat = 0;
    }

    xSGAddLoadClient(xsgdata, 'ROOM', NULL, xSGT_LoadLoadCB);
    xSGAddLoadClient(xsgdata, 'PREF', NULL, xSGT_LoadPrefsCB);
    xSerial_svgame_register(xsgdata, XSG_MODE_LOAD);

    if (!xSGSetup(xsgdata))
    {
        teststat = 0;
    }

    if (teststat)
    {
        signed int rc = xSGProcess(xsgdata);
        if (rc)
        {
            asstat = xSGAsyncStatus(xsgdata, 1, &whyFail, NULL);
        }
        if (!rc)
        {
            teststat = 0;
        }
        else
        {
            switch (asstat)
            {
            case XSG_ASTAT_SUCCESS:
                success = 1;
                break;
            case XSG_ASTAT_FAILED:
                success = 0;
                break;
            case XSG_ASTAT_NOOP:
            case XSG_ASTAT_INPROG:
                break;
            }
        }
    }

    if (teststat && !xSGWrapup(xsgdata))
    {
        teststat = 0;
    }

    zSaveLoad_UIEvent(0x28, 0x52);
    if (!xSGDone(xsgdata))
    {
        teststat = 0;
    }

    if (success && teststat)
    {
        return 1;
    }
    return -1;
}

static inline void zSaveLoadUITableInit(zSaveLoadUI* saveTable)
{
    for (signed int i = 0; i < 60; i++)
    {
        saveTable[i].nameID = xStrHash(saveTable[i].name);
    }
}

static inline void zSaveLoadGameTableInit(zSaveLoadGame* saveTable)
{
    for (signed int i = 0; i < 3; i++)
    {
        saveTable[i].label[0] = '\0';
        saveTable[i].date[0] = '\0';
        saveTable[i].progress = 0;
    }
}

static inline void zSaveLoadInit()
{
    zSaveLoadUITableInit(zSaveLoadUITable);
    zSaveLoadGameTableInit(zSaveLoadGameTable);
}

unsigned int zSaveLoad_LoadLoop()
{
    zSaveLoadInit();

    signed int state = 0;
    while (state != 6)
    {
        switch (state)
        {
        case 0:
            zSaveLoad_UIEvent(0x14, 0x5F);
            state = 1;
            break;
        case 1:
            switch (zSaveLoad_CardPick(1))
            {
            case 2:
                break;
            case 1:
                state = 2;
                break;
            case 4:
                state = 8;
                break;
            case -1:
                fn_80041CE8(1, currentCard, currentCard);
                fn_80041CC4(1, 7);
                state = 0;
                break;
            case 0:
            default:
                state = 0;
                break;
            }
            break;
        case 2:
            switch (zSaveLoad_GameSelect(1))
            {
            case 0:
                break;
            case 2:
            case 4:
            case 5:
            case 6:
                state = 1;
                break;
            case 1:
                state = 9;
                break;
            case -1:
                fn_80041CE8(1, currentCard, currentCard);
                fn_80041CC4(1, 7);
                fn_800C9368(1, 1, NULL, 0);
                state = 1;
                break;
            }
            break;
        case 3:
        case 4:
        case 5:
        case 7:
            break;
        case 8:
            zGameModeSwitch(eGameMode_Title);
            zGameStateSwitch(0);
            state = 6;
            break;
        case 9:
            fn_80041CE8(1, currentCard, currentCard);
            fn_80041CC4(1, 5);
            if (zSaveLoad_LoadGame() == 1)
            {
                zGameModeSwitch(eGameMode_Game);
                zGameStateSwitch(0);
                state = 6;
            }
            else
            {
                fn_80041CE8(1, currentCard, currentCard);
                fn_80041CC4(1, 7);
                fn_800C9368(1, 1, NULL, 0);
                state = 1;
            }
            break;
        }
    }

    return (sceneRead[0] << 24) | (sceneRead[1] << 16) | (sceneRead[2] << 8) | sceneRead[3];
}

unsigned int zSaveLoad_SaveLoop()
{
    zSaveLoadInit();

    signed int state = 0;
    saveSuccess = 0;
    zEntPlayer_SNDSavePickup(NULL);
    while (state != 6)
    {
        switch (state)
        {
        case 0:
            state = 1;
            break;
        case 1:
        {
            signed int sel = zSaveLoad_CardPick(0);
            if (sel == 1)
            {
                state = 2;
            }
            else if (sel == 4)
            {
                state = 11;
            }
            else
            {
                state = 0;
            }
            break;
        }
        case 2:
            switch (zSaveLoad_GameSelect(0))
            {
            case 0:
                break;
            case 2:
            case 4:
            case 5:
            case 6:
                state = 1;
                break;
            case 1:
                state = 10;
                break;
            case -1:
                fn_80041CE8(1, currentCard, currentCard);
                fn_80041CC4(1, 3);
                fn_800C9368(0, 1, NULL, 0);
                state = 1;
                break;
            }
            break;
        case 3:
        case 4:
        case 5:
        case 7:
        case 8:
        case 9:
            break;
        case 11:
            zGameModeSwitch(eGameMode_Game);
            zGameStateSwitch(1);
            zEntPlayer_SNDVan(NULL);
            state = 6;
            break;
        case 10:
            fn_80041CE8(1, currentCard, currentCard);
            fn_80041CC4(1, 1);
            if (zSaveLoad_SaveGame() == 1)
            {
                zGameModeSwitch(eGameMode_Game);
                zGameStateSwitch(1);
                state = 6;
                saveSuccess = 1;
                zEntPlayer_SNDVan(NULL);
            }
            else
            {
                state = 1;
            }
            break;
        }
    }
    return saveSuccess;
}

void zSaveLoad_DispatchCB(unsigned int dispatchEvent, float* toParam)
{
    switch (dispatchEvent)
    {
    case 0xA8:
        promptSel = 4;
        break;
    case 0xA9:
        if (sAllowCancel)
        {
            promptSel = 2;
        }
        break;
    case 0xAA:
        promptSel = 3;
        break;
    case 0xAB:
        currentCard = (signed int)*toParam;
        break;
    case 0xAC:
        currentGame = (signed int)*toParam;
        break;
    case 0xAD:
        promptSel = 3;
        break;
    }
}

signed int xSGT_SaveInfoCB(void* vp, st_XSAVEGAME_DATA* xsgdata, signed int* need, signed int* most)
{
    *need = xSGWriteStrLen(currSceneStr);
    *most = *need * 2;
    return 1;
}

signed int xSGT_SaveProcCB(void* vp, st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt)
{
    strcpy(currSceneStr, xUtil_idtag2string(globals.sceneCur->sceneID, 0));
    return xSGWriteData(xsgdata, wctxt, currSceneStr, 1, strlen(currSceneStr)) + 1;
}

signed int xSGT_SaveInfoPrefsCB(void* vp, st_XSAVEGAME_DATA* xsgdata, signed int* need,
                                signed int* most)
{
    *need = 0x18;
    *most = *need * 2;
    return 1;
}

signed int xSGT_SaveProcPrefsCB(void* vp, st_XSAVEGAME_DATA* xsgdata,
                                st_XSAVEGAME_WRITECONTEXT* wctxt)
{
    signed int sum = 0;
    gSnd.stereo = fn_800D8C14();
    sum += xSGWriteData(xsgdata, wctxt, &gSnd.stereo, 1);
    sum += xSGWriteData(xsgdata, wctxt, &gSnd.categoryVolFader[0], 1);
    sum += xSGWriteData(xsgdata, wctxt, &gSnd.categoryVolFader[1], 1);
    sum += xSGWriteData(xsgdata, wctxt, &gSnd.categoryVolFader[2], 1);
    sum += xSGWriteData(xsgdata, wctxt, &gSnd.categoryVolFader[3], 1);
    sum += xSGWriteData(xsgdata, wctxt, &globals.option_vibration, 1);
    return sum + 1;
}

// Stand-in for a function the linker strips; it keeps a second 32-byte zeroed
// buffer template after xSGT_LoadLoadCB's in .rodata.
signed int zSaveLoadBufDeadStripped(char* name)
{
    char buf[32] = "";
    strcpy(buf, name);
    return strlen(buf);
}

signed int xSGT_LoadLoadCB(void* vp, st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt,
                           unsigned int ui, signed int i)
{
    char bigbuf[32] = "";
    signed int compdiff = 0;

    xSGReadData(xsgdata, rctxt, bigbuf, 1, strlen(currSceneStr));
    if (strlen(currSceneStr) != strlen(bigbuf))
    {
        compdiff = 1;
    }
    if (compdiff == 0)
    {
        strcpy(sceneRead, bigbuf);
    }
    return (unsigned char)(compdiff == 0);
}

signed int xSGT_LoadPrefsCB(void* vp, st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt,
                            unsigned int ui, signed int i)
{
    xSGReadData(xsgdata, rctxt, &gSnd.stereo, 1);
    gSnd.stereo = fn_800D8C14();
    xSGReadData(xsgdata, rctxt, &gSnd.categoryVolFader[0], 1);
    xSGReadData(xsgdata, rctxt, &gSnd.categoryVolFader[1], 1);
    xSGReadData(xsgdata, rctxt, &gSnd.categoryVolFader[2], 1);
    xSGReadData(xsgdata, rctxt, &gSnd.categoryVolFader[3], 1);
    xSGReadData(xsgdata, rctxt, &globals.option_vibration, 1);
    if (globals.option_vibration != 0)
    {
        xPadRumbleEnable(0, 1);
    }
    else
    {
        xPadRumbleEnable(0, 0);
    }
    return 1;
}

unsigned int zSaveLoad_slotIsEmpty(unsigned int slot)
{
    return strcmp(zSaveLoadGameTable[slot].label, "Empty") == 0;
}

// Text for the current memory card message (GameCube only).
extern "C" signed int fn_800C9710(char* buf)
{
    return fn_80041548(1, buf);
}

extern "C" signed int fn_800C96E8(char* buf)
{
    return fn_80041BC8(1, buf);
}

extern "C" signed int fn_800C96C0(char* buf)
{
    return fn_80041B10(1, buf);
}

extern "C" signed int fn_800C9698(char* buf)
{
    return fn_80041ADC(1, buf);
}

unsigned char zSaveLoad_poll(signed int)
{
    signed int prev = promptSel;
    zSaveLoad_pollCheck(5);
    return promptSel != prev;
}

// Shows a yes/no style prompt and waits for an answer (GameCube only).
extern "C" signed int fn_800C9368(signed int mode, signed int type, unsigned char (*cb)(signed int),
                                  signed int arg)
{
    signed int i = 0;
    switch (type)
    {
    case 0:
        if (mode == 1)
        {
            i = 0x32;
        }
        else
        {
            i = 0x37;
        }
        break;
    case 1:
        if (mode == 1)
        {
            i = 0x33;
        }
        else
        {
            i = 0x38;
        }
        break;
    case 2:
        if (mode == 1)
        {
            i = 0x34;
        }
        else
        {
            i = 0x39;
        }
        break;
    case 3:
        if (mode == 1)
        {
            i = 0x35;
        }
        else
        {
            i = 0x3A;
        }
        break;
    case 4:
        if (mode == 1)
        {
            i = 0x36;
        }
        else
        {
            i = 0x3B;
        }
        break;
    }

    if (i == 0)
    {
        return 0;
    }

    if (mode == 1)
    {
        zSaveLoad_UIEvent(0, 1);
        zSaveLoad_UIEvent(0, 0x5F);
        zSaveLoad_UIEvent(1, 0x5F);
    }
    else
    {
        zSaveLoad_UIEvent(0x15, 0x5F);
        zSaveLoad_UIEvent(0x16, 0x5F);
    }

    if (type == 4)
    {
        sAllowCancel = 1;
    }
    if (type == 3)
    {
        zSaveLoad_UIEvent(i, 0x51);
    }
    else
    {
        zSaveLoad_UIEvent(i, 0x5E);
    }

    unsigned char done = 0;
    promptSel = -1;
    while (promptSel == -1)
    {
        zSaveLoad_Tick();
        if (cb != NULL)
        {
            done = cb(arg);
        }
        if (done)
        {
            break;
        }
    }

    sAllowCancel = 0;
    zSaveLoad_UIEvent(i, 0x5F);
    signed int sel = promptSel;
    promptSel = -1;
    return sel;
}

// This TU was built without deferred codegen: functions and literal pools are in
// source order.
#pragma defer_codegen off
#include "zGlobals.h"
#include "zScene.h"
#include "xIni.h"
#include "xFont.h"
#include "xMemMgr.h"
#include "xString.h"
#include "xserializer.h"

class RwCamera;

extern "C" {
void* memset(void* dst, int c, unsigned int n);
void* memcpy(void* dst, const void* src, unsigned int n);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, unsigned int n);
unsigned int strlen(const char* s);

RwCamera* RwCameraClear(RwCamera* camera, RwRGBA* colour, signed int clearMode);
RwCamera* RwCameraBeginUpdate(RwCamera* camera);
RwCamera* RwCameraEndUpdate(RwCamera* camera);
RwCamera* RwCameraShowRaster(RwCamera* camera, void* dev, unsigned int flags);

void fn_80015EAC(const char* fmt, ...);
void fn_80016218();
void fn_800162D0(char* elf, signed int profile);
void fn_800162D4();
void fn_80043E78();
void fn_800443AC();
void fn_80044714();
void fn_8004C84C();
void fn_8006D398();
void fn_8006D39C();
void fn_800A9CE4();
void fn_800A9CF4();
void fn_800CD270();
}

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

class iTRCDisk
{
public:
    static signed int CheckDVDAndResetState();
};

RwCamera* iCameraCreate(signed int width, signed int height);
void iCameraDestroy(RwCamera* camera);
void* iFileLoad(char* name, unsigned int* buffer, unsigned int* size);
void iVSync();
void iSystemExit();
void iShadowInit();
void iTimeSetGame(float time);
void iProfileClear(unsigned int sceneID);
void xUtilStartup();
void xUtilShutdown();
char* xUtil_idtag2string(unsigned int srctag, signed int bufidx);
void xScrFxInit();
void xParMgrInit();
_tagxPad* xPadEnable(signed int idx);
void xPadRumbleEnable(signed int idx, signed int enable);
void xSGStartup();
void xSGShutdown();
signed int xSTPreLoadScene(unsigned int sid, void* userdata);
signed int xSTQueueSceneAssets(unsigned int sid);
float xSTLoadStep(unsigned int sid);
void zDispatcher_Startup();
void zDispatcher_Shutdown();
void zAssetStartup();
void zAssetShutdown();
void zLobMaster_Startup();
void zLobMaster_Shutdown();
void zEntPlayer_InitCutsceneModelHack();
void zCutSceneNamesTable_clearAll();
void zMenuInit(unsigned int theSceneID);
void zMenuSetup();
unsigned int zMenuLoop();
void zMenuExit();
void zGameModeSwitch(eGameMode modeNew);
void zGameStateSwitch(signed int theNewState);
void zGameInit(unsigned int theSceneID);
void zGameSetup();
void zGameLoop();
void zGameExit();
void zVarNewGame();
void zSceneMemLvlChkCB();
void CalcJumpImpulse(zJumpParam* param);

extern _tagxPad* gDebugPad;
static signed int sShowMenuOnBoot = 1;
static st_SERIAL_PERCID_SIZE g_xser_sizeinfo[2] = { { 'PLYR', 0x148 }, { 0, 0 } };

static void zMainLoop();
static void zMainMemLvlChkCB();
static void zMainParseINIGlobals(xIniFile* ini);

#pragma auto_inline on

// TODO: once inlined into main, box should get r30 (it gets r28).
void zMainFirstScreen(signed int mode)
{
    iColor_tag color = { 255, 230, 0, 200 };
    RwCamera* cam = iCameraCreate(640, 480);
    RwRGBA bg = { 0, 0, 0, 0 };

    RwCameraClear(cam, &bg, 3);
    RwCameraBeginUpdate(cam);

    char text[] = "Game and Software, excluding Scooby-Doo materials, (C) 2002 THQ Inc. Used "
                  "under exclusive license by THQ Inc.\n\n\nDeveloped by Heavy Iron Studios_. "
                  "Heavy Iron Studios, THQ and their respective logos are trademarks and/or "
                  "registered trademarks of THQ Inc. All Rights Reserved.\n\n\nLICENSED BY "
                  "NINTENDO";
    xFontBox* box = xFontBoxCreate(0.0f, 0.0f, 640.0f, 480.0f, 1, color);
    xFontBoxSetJustification(box, 0x40);
    xFontBoxSetDrawSize(box, 17.0f, 22.6f);
    xFontBoxSetDrawSpacing(box, 15.5f, 22.6f);
    xFontBoxSetColor(box, color);
    xFontBoxSetInset(box, 30.0f, 30.0f, 120.0f, 20.0f);
    xFontBoxSetText(box, text);
    xFontBoxRender(box);
    xFontBoxDestroy(box);

    RwCameraEndUpdate(cam);
    RwCameraShowRaster(cam, 0, 1);
    iCameraDestroy(cam);

    signed int i = 120;
    do
    {
        iTRCDisk::CheckDVDAndResetState();
        iVSync();
    } while (i-- > 0);
}

static void zMainCheckINI()
{
    unsigned int size;
    void* buf = iFileLoad("SD2.INI", 0, &size);

    if (buf)
    {
        xIniFile* ini = xIniParse((char*)buf, size);
        char* str;

        if (xIniGetString(ini, "PATH", 0))
        {
            fn_800162D4();
        }

        str = xIniGetString(ini, "BOOT", 0);
        if (str)
        {
            char* start = globals.sceneStart;
            strcpy(start, "BOOT");
            if (strlen(str) == 4)
            {
                strcpy(start, str);
            }
        }

        globals.profile = xIniGetInt(ini, "Profile", 0);
        strncpy(globals.profFunc[0], xIniGetString(ini, "ProfFuncTriangle", ""), 128);
        globals.profFunc[0][127] = 0;
        strncpy(globals.profFunc[1], xIniGetString(ini, "ProfFuncSquare", ""), 128);
        globals.profFunc[1][127] = 0;
        strncpy(globals.profFunc[2], xIniGetString(ini, "ProfFuncLeft", ""), 128);
        globals.profFunc[2][127] = 0;
        strncpy(globals.profFunc[3], xIniGetString(ini, "ProfFuncRight", ""), 128);
        globals.profFunc[3][127] = 0;
        strncpy(globals.profFunc[4], xIniGetString(ini, "ProfFuncUp", ""), 128);
        globals.profFunc[4][127] = 0;
        strncpy(globals.profFunc[5], xIniGetString(ini, "ProfFuncDown", ""), 128);
        globals.profFunc[5][127] = 0;

        zMainParseINIGlobals(ini);
        xIniDestroy(ini);
    }
}

static void zMainLoop()
{
    unsigned int newGameSceneID;

    xMemPushBase();
    xUtil_idtag2string('BOOT', 0);
    xSTPreLoadScene('BOOT', 0);
    xUtil_idtag2string('BOOT', 0);
    xSTQueueSceneAssets('BOOT');
    xUtil_idtag2string('BOOT', 0);
    while (xSTLoadStep('BOOT') < 1.0f)
    {
        iTRCDisk::CheckDVDAndResetState();
        fn_800443AC();
    }
    xUtil_idtag2string('BOOT', 0);

    iShadowInit();
    zEntPlayer_InitCutsceneModelHack();

    xMemPushBase();
    xUtil_idtag2string('MNU4', 0);
    xSTPreLoadScene('MNU4', 0);
    xUtil_idtag2string('MNU4', 0);
    xSTQueueSceneAssets('MNU4');
    xUtil_idtag2string('MNU4', 0);
    while (xSTLoadStep('MNU4') < 1.0f)
    {
        iTRCDisk::CheckDVDAndResetState();
        fn_800443AC();
    }
    xUtil_idtag2string('MNU4', 0);

    static unsigned int menuModeID = 'MNU3';
    static unsigned int gameSceneID = (globals.sceneStart[0] << 24) |
                                      (globals.sceneStart[1] << 16) |
                                      (globals.sceneStart[2] << 8) | globals.sceneStart[3];

    newGameSceneID = gameSceneID;
    xUtil_idtag2string(gameSceneID, 0);
    xMemPushBase();

    while (1)
    {
        xSerialWipeMainBuffer();
        fn_800CD270();
        zCutSceneNamesTable_clearAll();
        fn_8004C84C();

        if (sShowMenuOnBoot)
        {
            zMenuInit(menuModeID);
            zMenuSetup();
            newGameSceneID = zMenuLoop();
            zMenuExit();
        }
        else
        {
            sShowMenuOnBoot = 1;
            zGameModeSwitch(eGameMode_Game);
            zGameStateSwitch(0);
        }

        if (newGameSceneID == 0)
        {
            zVarNewGame();
            iTimeSetGame(0.0f);
            gameSceneID = (globals.sceneStart[0] << 24) | (globals.sceneStart[1] << 16) |
                          (globals.sceneStart[2] << 8) | globals.sceneStart[3];
        }
        else
        {
            gameSceneID = newGameSceneID;
            iTimeSetGame(0.0f);
        }

        zGameInit(gameSceneID);
        zGameSetup();
        iProfileClear(gameSceneID);
        zGameLoop();
        zGameExit();
    }
}

static void zMainMemLvlChkCB()
{
    xMemGetBase();
    zSceneMemLvlChkCB();
}

static void zMainParseINIGlobals(xIniFile* ini)
{
    static float fbuf_SpeedSneak[16] = { 0.5f, 1.5f, 0.0f, 1.0f };
    static float fbuf_SpeedWalk[16] = { 2.0f, 4.0f, 0.0f, 1.0f };
    static float fbuf_SpeedRun[16] = { 4.0f, 7.0f, 0.0f, 1.0f };
    static float fbuf_SpeedAir[16] = { 3.0f, 6.0f, 0.0f, 1.0f };
    static float fbuf_SpeedHotsauce[16] = { 8.0f, 8.0f, 0.2f, 1.0f };
    static float fbuf_AnimSneak[12] = { 1.5f, 0.5f, 1.5f };
    static float fbuf_AnimWalk[12] = { 3.0f, 0.5f, 2.5f };
    static float fbuf_Jump[16] = { 2.0f, 0.7f, 0.35f };
    static float fbuf_Double[16] = { 1.0f, 0.7f, 0.35f };
    static float fbuf_Bounce[16] = { 1.5f, 0.2f, 0.2f };
    static float fbuf_Spring[16] = { 3.0f, 0.2f, 0.2f };
    static float fbuf_Toss[16] = { 4.0f, 0.5f };
    char* str;

    globals.player.s.AnalogMin = xIniGetInt(ini, "AnalogMin", 0x20);
    globals.player.s.AnalogMax = xIniGetInt(ini, "AnalogMax", 0x6E);
    str = xIniGetString(ini, "SpeedSneak", 0);
    xStrParseFloatList((float*)memcpy(globals.player.s.SpeedSneak, fbuf_SpeedSneak, sizeof(globals.player.s.SpeedSneak)), str, 4);
    str = xIniGetString(ini, "SpeedWalk", 0);
    xStrParseFloatList((float*)memcpy(globals.player.s.SpeedWalk, fbuf_SpeedWalk, sizeof(globals.player.s.SpeedWalk)), str, 4);
    str = xIniGetString(ini, "SpeedRun", 0);
    xStrParseFloatList((float*)memcpy(globals.player.s.SpeedRun, fbuf_SpeedRun, sizeof(globals.player.s.SpeedRun)), str, 4);
    str = xIniGetString(ini, "SpeedAir", 0);
    xStrParseFloatList((float*)memcpy(globals.player.s.SpeedAir, fbuf_SpeedAir, sizeof(globals.player.s.SpeedAir)), str, 4);
    str = xIniGetString(ini, "SpeedHotsauce", 0);
    xStrParseFloatList((float*)memcpy(globals.player.s.SpeedHotsauce, fbuf_SpeedHotsauce, sizeof(globals.player.s.SpeedHotsauce)), str, 4);
    str = xIniGetString(ini, "AnimSneak", 0);
    xStrParseFloatList((float*)memcpy(globals.player.s.AnimSneak, fbuf_AnimSneak, sizeof(globals.player.s.AnimSneak)), str, 3);
    str = xIniGetString(ini, "AnimWalk", 0);
    xStrParseFloatList((float*)memcpy(globals.player.s.AnimWalk, fbuf_AnimWalk, sizeof(globals.player.s.AnimWalk)), str, 3);

    globals.player.s.Gravity = xIniGetFloat(ini, "Gravity", 20.0f);
    globals.player.s.JumpGravity = xIniGetFloat(ini, "JumpGravity", 7.0f);
    globals.player.s.GravSmooth = xIniGetFloat(ini, "GravSmooth", 0.25f);
    globals.player.s.FloatSpeed = xIniGetFloat(ini, "FloatSpeed", 1.0f);
    globals.player.s.ButtsmashSpeed = xIniGetFloat(ini, "ButtsmashSpeed", 5.0f);

    str = xIniGetString(ini, "Jump", 0);
    xStrParseFloatList((float*)memcpy(&globals.player.s.Jump, fbuf_Jump, sizeof(globals.player.s.Jump)), str, 4);
    str = xIniGetString(ini, "Double", 0);
    xStrParseFloatList((float*)memcpy(&globals.player.s.Double, fbuf_Double, sizeof(globals.player.s.Double)), str, 4);
    str = xIniGetString(ini, "Bounce", 0);
    xStrParseFloatList((float*)memcpy(&globals.player.s.Bounce, fbuf_Bounce, sizeof(globals.player.s.Bounce)), str, 4);
    str = xIniGetString(ini, "Spring", 0);
    xStrParseFloatList((float*)memcpy(&globals.player.s.Spring, fbuf_Spring, sizeof(globals.player.s.Spring)), str, 4);
    str = xIniGetString(ini, "Toss", 0);
    xStrParseFloatList((float*)memcpy(&globals.player.s.Toss, fbuf_Toss, sizeof(globals.player.s.Toss)), str, 4);

    globals.player.s.HeadbuttSpeedMin = xIniGetFloat(ini, "HeadbuttSpeedMin", 3.0f);
    globals.player.s.HeadbuttSpeedMax = xIniGetFloat(ini, "HeadbuttSpeedMax", 10.0f);
    globals.player.s.HeadbuttAccel = xIniGetFloat(ini, "HeadbuttAccel", 5.0f);
    globals.player.s.HeadbuttTimeMin = xIniGetFloat(ini, "HeadbuttTimeMin", 0.75f);
    globals.player.s.HeadbuttTimeMax = xIniGetFloat(ini, "HeadbuttTimeMax", 2.0f);
    globals.player.s.HeadbuttTurn = xIniGetFloat(ini, "HeadbuttTurn", 2.5f);
    globals.player.s.FireDelay = xIniGetFloat(ini, "FireDelay", 0.3f);
    globals.player.s.FireRepeat = xIniGetFloat(ini, "FireRepeat", 0.6f);
    globals.player.s.FirePress = xIniGetFloat(ini, "FirePress", 0.3f);
    globals.player.s.FireTargetYaw = xIniGetFloat(ini, "FireTargetYaw", 15.0f);
    globals.player.s.FireTargetPitch = xIniGetFloat(ini, "FireTargetPitch", 30.0f);
    globals.player.s.FireTargetDist = xIniGetFloat(ini, "FireTargetDist", 0.7f);
    globals.player.s.FireTargetRadius = xIniGetFloat(ini, "FireTargetRadius", 0.25f);
    globals.player.s.FireTrack = xIniGetFloat(ini, "FireTrack", 0.0f);
    globals.player.s.FireGumSpeed = xIniGetFloat(ini, "FireGumSpeed", 10.0f);
    globals.player.s.FireSoapSpeed = xIniGetFloat(ini, "FireSoapSpeed", 10.0f);
    globals.player.s.FireGumTimer = xIniGetFloat(ini, "FireGumTimer", 3.0f);
    globals.player.s.FireSoapTimer = xIniGetFloat(ini, "FireSoapTimer", 3.0f);

    globals.player.s.AmmoCapacityGum = xIniGetInt(ini, "AmmoCapacityGum", 50);
    globals.player.s.AmmoCapacitySoap = xIniGetInt(ini, "AmmoCapacitySoap", 50);
    globals.player.s.AmmoSceneGum = xIniGetInt(ini, "AmmoSceneGum", 20);
    globals.player.s.AmmoSceneSoap = xIniGetInt(ini, "AmmoSceneSoap", 20);
    globals.player.Inv_AmmoMax[0] = globals.player.s.AmmoCapacityGum;
    globals.player.Inv_AmmoMax[1] = globals.player.s.AmmoCapacitySoap;

    globals.player.s.TakeDamage = xIniGetInt(ini, "TakeDamage", 0);
    globals.player.s.StartSlideAngle = xIniGetFloat(ini, "StartSlideAngle", 30.0f);
    globals.player.s.StopSlideAngle = xIniGetFloat(ini, "StopSlideAngle", 10.0f);
    globals.player.s.RotMatchMaxAngle = xIniGetFloat(ini, "RotMatchMaxAngle", 30.0f);
    globals.player.s.RotMatchMatchTime = xIniGetFloat(ini, "RotMatchMatchTime", 0.1f);
    globals.player.s.RotMatchRelaxTime = xIniGetFloat(ini, "RotMatchRelaxTime", 0.3f);
    globals.player.s.StartSlideAngle = (3.1415927f * globals.player.s.StartSlideAngle) / 180.0f;
    globals.player.s.StopSlideAngle = (3.1415927f * globals.player.s.StopSlideAngle) / 180.0f;
    globals.player.s.RotMatchMaxAngle = (3.1415927f * globals.player.s.RotMatchMaxAngle) / 180.0f;

    CalcJumpImpulse(&globals.player.s.Jump);
    CalcJumpImpulse(&globals.player.s.Double);
    CalcJumpImpulse(&globals.player.s.Bounce);
    CalcJumpImpulse(&globals.player.s.Spring);
    CalcJumpImpulse(&globals.player.s.Toss);

    globals.player.s.Initial_Specials |= xIniGetInt(ini, "eSPECIAL_BubbleGum", 1);
    globals.player.s.Initial_Specials |= xIniGetInt(ini, "eSPECIAL_SoapBar", 1) << 1;
    globals.player.s.Initial_Specials |= xIniGetInt(ini, "eSPECIAL_StickyBoots", 0) << 2;
    globals.player.s.Initial_Specials |= xIniGetInt(ini, "eSPECIAL_Plungers", 0) << 3;
    globals.player.s.Initial_Specials |= xIniGetInt(ini, "eSPECIAL_Slippers", 0) << 4;
    globals.player.s.Initial_Specials |= xIniGetInt(ini, "eSPECIAL_LampShade", 0) << 5;
    globals.player.s.Initial_Specials |= xIniGetInt(ini, "eSPECIAL_BlackKnight", 0) << 6;
    globals.player.s.Initial_Specials |= xIniGetInt(ini, "eSPECIAL_FlowerPot", 0) << 7;
    globals.player.s.Initial_Specials |= xIniGetInt(ini, "eSPECIAL_DivingHelmet", 0) << 8;
    globals.player.s.Initial_Specials |= xIniGetInt(ini, "eSPECIAL_Spring", 1) << 9;
    globals.player.s.Initial_Specials |= xIniGetInt(ini, "eSPECIAL_LightningBolt", 1) << 10;
    globals.player.s.Initial_Specials |= xIniGetInt(ini, "eSPECIAL_FootballHelmet", 1) << 11;
    globals.player.s.Initial_Specials |= xIniGetInt(ini, "eSPECIAL_Umbrella", 0) << 12;
    globals.player.s.Initial_Specials |= xIniGetInt(ini, "eSPECIAL_Shovel", 0) << 13;
    globals.player.s.Initial_Specials |= xIniGetInt(ini, "eSPECIAL_LightningBoltStun", 0) << 14;

    sShowMenuOnBoot = xIniGetInt(ini, "ShowMenuOnBoot", 1);
}

void main()
{
    fn_80016218();
    xMemRegisterBaseNotifyFunc(zMainMemLvlChkCB);
    memset(&globals, 0, sizeof(zGlobals));
    globals.sceneFirst = 1;
    fn_80044714();
    zMainCheckINI();
    fn_800162D0("scooby.elf", globals.profile);
    xUtilStartup();
    xSerialStartup(0x80, g_xser_sizeinfo);
    zDispatcher_Startup();
    zAssetStartup();
    xFontInit();
    xScrFxInit();
    xParMgrInit();
    globals.option_vibration = 1;
    fn_80015EAC("---PAD ENABLE\n");
    globals.pad0 = xPadEnable(0);
    globals.pad1 = gDebugPad = xPadEnable(1);
    xPadRumbleEnable(0, globals.option_vibration);
    xSGStartup();
    fn_8006D39C();
    zLobMaster_Startup();
    fn_800A9CF4();
    fn_80043E78();
    zMainFirstScreen(1);
    fn_80015EAC("----ENTERING MAIN LOOP\n");
    zMainLoop();
    fn_800A9CE4();
    zLobMaster_Shutdown();
    fn_8006D398();
    zAssetShutdown();
    zDispatcher_Shutdown();
    xSGShutdown();
    xSerialShutdown();
    xUtilShutdown();
    iSystemExit();
}

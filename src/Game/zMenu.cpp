#include "zGlobals.h"
#include "zScene.h"
#include "iTime.h"
#include "iDraw.h"
#include "xString.h"

class _zEnv;
class _xEnv;

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

enum eFMVFile
{
    eFMVFile_Force32 = 0x7FFFFFFF
};

class iTRCDisk
{
public:
    static signed int CheckDVDAndResetState();
};

#ifdef PC_PORT
// The GameCube OS keeps the bus clock in its globals page; ports can't map it.
#define GET_BUS_FREQUENCY() (162000000u)
#else
#define GET_BUS_FREQUENCY() (*(unsigned int*)0x800000F8)
#endif

void zGameModeSwitch(eGameMode modeNew);
eGameMode zGameModeGet();
void zGameStateSwitch(signed int theNewState);
signed int zGameStateGet();
void zGameSetOstrich(_GameOstrich o);
void zMusicNotify(signed int situation);
void zMusicUpdate(float dt);
void zMusicInit();
void zMusicKill();
char* zFMVFileGetName(eFMVFile fileEnum);
unsigned int zFMVPlay(char* filename, unsigned int buttons, float time);
void xScrFxFade(iColor_tag* base, iColor_tag* dest, float seconds, void (*callback)(),
                signed int hold, signed int unused);
signed int xPadUpdate(signed int idx, float time_passed);
void xDrawBegin();
void xDrawEnd();
void zSceneUpdate(float elapsedSec);
void zSceneRender();
void zSceneInit(unsigned int theSceneID);
void zSceneExit();
void zSceneSetup();
void xSndUpdate(float dt);
void xSndStopAll();
void xCameraUpdate(_tagxCamFrame* cam, float seconds);
void xCameraBegin(_tagxCamFrame* cam, signed int clear);
void xCameraEnd(_tagxCamFrame* cam, float seconds, signed int update_scrn_fx);
void xCameraShowRaster(_tagxCamFrame* cam);
void xCameraCreate(_tagxCamFrame* cam, unsigned int width, unsigned int height);
void xCameraDestroy(_tagxCamFrame* cam);
void xCameraAssignEnv(_tagxCamFrame* camera, _xEnv* env);
void xCameraRemoveEnv(_tagxCamFrame* camera);
void xFontRender();
void iVSync();
void iDebugScreenDump();
unsigned int zSaveLoad_LoadLoop();
void isrand(unsigned int seed);
unsigned int irand();
void zHud_Init();
void zHud_Setup(_xScene* sc);
void setupPlayer();
void zEnvStartingCamera(_zEnv* env);
void zEntEventAll(xBase* from, unsigned int fromEvent, unsigned int toEvent, float* toParam);
void xDebugUpdate();

extern "C" {
unsigned int VIGetRetraceCount();
// TODO: unidentified (iScrFX: number of black frames to draw)
void fn_80014A34(unsigned int frames);
// TODO: unidentified (xTRC: draws the TRC message)
// TODO: unidentified (xTRC: true while a TRC message is up)
unsigned int fn_800443F8();
// TODO: unidentified retrace counters (iSystem?)
extern volatile unsigned int gVsyncCount;
extern unsigned int gLastVsyncFlip;
}
void xTRCRender();

extern _zEnv* gCurEnv;
extern unsigned int gScreenDump;
extern unsigned int gFrameCount;
extern eGameMode gGameMode;

static signed int sFirstBoot = 1;
static float time_elapsed = 0.01f;
static float holdTmr = 10.0f;
static float time_last;
static float time_current;
static float sAttractMode_timer;
static signed int card;
static signed int sInMenu;

// Functions are in reverse DOL order (-inline deferred emits last to first).

signed int zMenuRunning()
{
    return sInMenu;
}

void zMenuInit(unsigned int theSceneID)
{
    sInMenu = 1;
    isrand(iTimeGet());
    iTimeGet();
    irand();
    zSceneInit(theSceneID);
    zHud_Init();
    xCameraCreate(&globals.camera, 640, 480);
    xCameraAssignEnv(&globals.camera, globals.sceneCur->env);
    zMusicInit();
}

void zMenuExit()
{
    zMusicKill();
    xCameraRemoveEnv(&globals.camera);
    xCameraDestroy(&globals.camera);
    zSceneExit();
    xSndStopAll();
    sInMenu = 0;
}

void zMenuSetup()
{
    zSceneSetup();
    zHud_Setup(globals.sceneCur);
    sAttractMode_timer = 30.0f;
    setupPlayer();
    zEnvStartingCamera(gCurEnv);
    xCameraUpdate(&globals.camera, -1.0f);
    zEntEventAll(0, 0, 0x57, 0);
    zEntEventAll(0, 0, 0x59, 0);
    zEntEvent(&globals.player.ent, 8);
}

void zMenuFMVPlay(char* filename, unsigned int buttons, float time);

#ifdef PC_PORT
extern "C" int pc_setting_int(const char* key, int def);

// "Press Start" on the title screen (its manager enabled, as the attract timer
// checks): the backdrop is 4:3 geometry, so a platform without a full-screen
// picture for it keeps that screen 4:3. The main menu after it may be wide.
extern "C" int pc_game_title_screen()
{
    if (zGameModeGet() != eGameMode_Title || !globals.sceneCur)
    {
        return 0;
    }
    _xEnt* mgr = (_xEnt*)zSceneFindObject(xStrHash("MNU3 PRESS START BUTTON MGR"));
    return mgr && (mgr->baseFlags & 1);
}
#endif

unsigned int zMenuLoop()
{
    unsigned int s;
    signed int ostrich_delay;

    if (sFirstBoot)
    {
        zGameModeSwitch(eGameMode_Intro);
        zGameStateSwitch(0);
        zMusicNotify(0);
    }
    else
    {
        zGameModeSwitch(eGameMode_Title);
        zGameStateSwitch(0);
        zMusicNotify(0);
    }

    time_last = 1.0f / float(GET_BUS_FREQUENCY() >> 2) * iTimeGet() - 1.0f / 60.0f;
    iColor_tag black = { 0, 0, 0, 255 };
    iColor_tag clear = { 0 };
    xScrFxFade(&black, &clear, 0.0f, 0, 1, 0);
    ostrich_delay = 10;

    do
    {
        if (zGameModeGet() == eGameMode_Intro)
        {
            switch (zGameStateGet())
            {
            case 0:
                fn_80014A34(1);
                zGameStateSwitch(3);
                xScrFxFade(&clear, &black, 0.0f, 0, 1, 0);
                break;
            case 3:
#ifdef PC_PORT
                // The player's game.skip_logos: straight to the title (the WB,
                // THQ, Heavy Iron and RenderWare logos; the story intro still plays).
                if (!pc_setting_int("game.skip_logos", 0))
#endif
                {
                zFMVPlay(zFMVFileGetName((eFMVFile)0x12), 0, 0.0f);
                zFMVPlay(zFMVFileGetName((eFMVFile)0x11), 0, 0.0f);
                zFMVPlay(zFMVFileGetName((eFMVFile)0), 0, 0.0f);
                zMenuFMVPlay(zFMVFileGetName((eFMVFile)0x1A), 0, 0.0f);
                }
                holdTmr = 10.0f;
                zGameModeSwitch(eGameMode_Title);
                zGameStateSwitch(1);
                xScrFxFade(&black, &clear, 0.1f, 0, 1, 0);
                break;
            }
        }

        time_current = 1.0f / float(GET_BUS_FREQUENCY() >> 2) * iTimeGet();
        time_elapsed = time_current - time_last;
        if (time_elapsed < 0.0f)
        {
            time_elapsed = 1.0f / 60.0f;
        }

        if (zGameModeGet() == eGameMode_Title)
        {
            if (!fn_800443F8())
            {
                sAttractMode_timer -= time_elapsed;
            }
            if (sAttractMode_timer < 0.0f)
            {
                if (fn_800443F8())
                {
                    sAttractMode_timer = 30.0f;
                }
                else
                {
                    _xEnt* title1 =
                        (_xEnt*)zSceneFindObject(xStrHash("MNU3 PRESS START BUTTON MGR"));
                    _xEnt* title2 = (_xEnt*)zSceneFindObject(xStrHash("MNU3 START NEWGAME"));
                    if ((title1->baseFlags & 1) || xEntIsVisible(title2))
                    {
                        zGameStateSwitch(1);
                    }
                }
            }

            if (zGameStateGet() == 1)
            {
                if (sFirstBoot)
                {
                    zMenuFMVPlay(zFMVFileGetName((eFMVFile)1), 0x10000, 0.1f);
                    sFirstBoot = 0;
                }
                else
                {
                    zMenuFMVPlay(zFMVFileGetName((eFMVFile)(irand() % 17)), -1, 0.1f);
                }
                zGameStateSwitch(0);
                sAttractMode_timer = 30.0f;
            }
            else
            {
                zMusicUpdate(time_elapsed);
            }

            if (globals.pad0->pressed)
            {
                sAttractMode_timer = 30.0f;
            }
        }

        time_last = time_current;
        xPadUpdate(0, time_elapsed);
        if (globals.pad1)
        {
            xPadUpdate(1, time_elapsed);
        }
        iTRCDisk::CheckDVDAndResetState();
        xDrawBegin();
        zSceneUpdate(time_elapsed);
        globals.player.ent.update(&globals.player.ent, globals.sceneCur, time_elapsed);
        xSndUpdate(time_elapsed);
        xCameraUpdate(&globals.camera, time_elapsed);
        xCameraBegin(&globals.camera, 1);
        zSceneRender();
        xFontRender();
        xDebugUpdate();
        xTRCRender();
        xDrawEnd();
        if (gScreenDump)
        {
            iDebugScreenDump();
        }
        xCameraEnd(&globals.camera, time_elapsed, 1);
#ifdef PC_PORT
        // One retrace per frame at 60 fps, two (30 fps) as on the GameCube.
        while ((gVsyncCount = VIGetRetraceCount()) - gLastVsyncFlip < 60 / (unsigned int)pc_game_fps())
#else
        while ((gVsyncCount = VIGetRetraceCount()) - gLastVsyncFlip < 2)
#endif
        {
            iVSync();
        }
        gLastVsyncFlip = gVsyncCount;
        xCameraShowRaster(&globals.camera);

        s = 0;
        if (gGameMode == eGameMode_Load)
        {
            float elapsed1 = float(iTimeGet()) - time_last;
            s = zSaveLoad_LoadLoop();
            if (s == '0000')
            {
                s = 0;
            }
            time_last = elapsed1 + float(iTimeGet());
        }

        gFrameCount++;
        if (ostrich_delay > 0)
        {
            ostrich_delay--;
        }
        else
        {
            zGameSetOstrich(eGameOstrich_InScene);
        }
    } while (gGameMode != eGameMode_Game);

    return s;
}

unsigned int zMenuGetBadCard()
{
    return card + 1;
}

#pragma auto_inline on
void zMenuFMVPlay(char* filename, unsigned int buttons, float time)
{
    zFMVPlay(filename, buttons, time);
    time_last = iTimeGet();
    time_current = iTimeGet();
    sAttractMode_timer = 30.0f;
}
#pragma auto_inline off

signed int zMenuIsFirstBoot()
{
    return sFirstBoot;
}

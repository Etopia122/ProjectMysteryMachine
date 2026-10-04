#include "zGlobals.h"
#include "zScene.h"
#include "xString.h"
#include "xSnd.h"
#include "iMath.h"
#include "iDraw.h"
#include "rwtypes.h"

class RwRaster;
class RwTexture
{
public:
    RwRaster* raster; // 0x0
};

class RwRGBAReal
{
public:
    float red;
    float green;
    float blue;
    float alpha;
};

class RwBBox
{
public:
    RwV3d sup;
    RwV3d inf;
};

class RwIm2DVertex
{
public:
    float x;                // 0x0
    float y;                // 0x4
    float z;                // 0x8
    RwRGBA emissiveColor;   // 0xC
    float u;                // 0x10
    float v;                // 0x14
    float recipZ;           // 0x18
};

extern "C" {
void* memset(void* dst, int c, unsigned int n);
char* strcpy(char* dst, const char* src);
int sprintf(char* buf, const char* fmt, ...);

void _rwFrameSyncDirty();
signed int RwFrameDestroy(RwFrame* frame);
RwFrame* RwFrameCreate();
signed int RpLightDestroy(RpLight* light);
RpLight* RpLightCreate(signed int type);
RpLight* RpLightSetColor(RpLight* light, const RwRGBAReal* color);
void* _rwObjectHasFrameSetFrame(void* object, RwFrame* frame);
RpWorld* RpWorldCreate(RwBBox* boundingBox);
signed int RpWorldDestroy(RpWorld* world);
RpWorld* RpWorldAddCamera(RpWorld* world, RwCamera* camera);
RpWorld* RpWorldRemoveCamera(RpWorld* world, RwCamera* camera);
RwCamera* RwCameraClear(RwCamera* camera, RwRGBA* colour, signed int clearMode);
RwCamera* RwCameraBeginUpdate(RwCamera* camera);
RwCamera* RwCameraEndUpdate(RwCamera* camera);
RwCamera* RwCameraShowRaster(RwCamera* camera, void* dev, unsigned int flags);
signed int RwRenderStateSet(signed int state, void* value);
float RwIm2DGetNearScreenZ();
signed int RwIm2DRenderIndexedPrimitive(signed int primType, RwIm2DVertex* vertices,
                                        signed int numVertices, unsigned short* indices,
                                        signed int numIndices);
unsigned int VIGetRetraceCount();

void fn_800056F8(_xVec3* pos);
void fn_80014A34(signed int mode);
void fn_80021B20();
}
void zEntPlayer_Init(_xEnt* ent, p2EntAsset* asset);
void zEntShaggy_SceneStart(zScene* sc);
void zScene_UpdateFlyToInterface(float dt);
void xTRCRender();

void zEntPickupCheat_allWarps();

class _zEnv;
class _xEnv;
class zCutsceneMgr;

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

enum eHudDropShadowMode
{
    eHudDropShadow_Force32 = 0x7FFFFFFF
};

class iTRCDisk
{
public:
    static signed int CheckDVDAndResetState();
};

class _tagTRCPadInfo
{
public:
    signed int pad;   // 0x0
    signed int id;    // 0x4
    signed int state; // 0x8
};

class zPortalAsset : public p2BaseAsset
{
public:
    unsigned int assetCameraID; // 0x8
    unsigned int assetMarkerID; // 0xC
    float ang;                  // 0x10
    unsigned int sceneID;       // 0x14
};

class _zPortal : public xBase
{
public:
    zPortalAsset* passet; // 0x10
};

class p2MarkerAsset
{
public:
    _xVec3 pos; // 0x0
};

#define GET_BUS_FREQUENCY() (*(unsigned int*)0x800000F8)

extern _zEnv* gCurEnv;
extern signed int gGameState;
extern eGameMode gGameMode;
extern _tagTRCPadInfo gTrcPad[4];
extern volatile unsigned int gVsyncCount;
extern unsigned int gFrameCount;

RwCamera* iCameraCreate(signed int width, signed int height);
void iCameraDestroy(RwCamera* camera);
long long iTimeGet();
float iTimeDiffSec(long long t0, long long t1);
void iTimeGameAdvance(float elapsed);
void iVSync();
void iPadStopRumble(_tagxPad* pad);
class iEnv;
class xSerial;
void iEnvEndRenderFX(iEnv* env);
_tagxPad* xPadUpdate(signed int idx, float time_passed);
void xPadDestroyRumbleChain(signed int idx);
void* xSTFindAsset(unsigned int aid, unsigned int* size);
void* xSTFindAssetByType(unsigned int type, signed int idx, unsigned int* size);
signed int xSTAssetCountByType(unsigned int type);
char* xSTAssetName(unsigned int aid);
void xDrawBegin();
void xDrawEnd();
void xFontRender();
void xprintf(char* fmt, ...);
void xCameraCreate(_tagxCamFrame* cam, unsigned int width, unsigned int height);
void xCameraDestroy(_tagxCamFrame* cam);
void xCameraAssignEnv(_tagxCamFrame* cam, _xEnv* env);
void xCameraRemoveEnv(_tagxCamFrame* cam);
void xCameraUpdate(_tagxCamFrame* cam, float dt);
void xCameraBegin(_tagxCamFrame* cam, signed int clear);
void xCameraEnd(_tagxCamFrame* cam, float seconds, signed int update_scrn_fx);
void xCameraShowRaster(_tagxCamFrame* cam);
void xCameraSetTarget(_tagxCamFrame* cam, _xVec3* tgt);
void xCameraSetTarget(_tagxCamFrame* cam, _xMat4x3* tgt);
void xCameraPickInitial(_tagxCamFrame* cam, _tagxCam* initial);
void xScrFxFade(iColor_tag* base, iColor_tag* dest, float seconds, void (*callback)(),
                signed int hold, signed int unused);
void xSndStopAll();
void xSndUpdate(float dt);
void xSndSetFrame(_xVec3* right, _xVec3* up, _xVec3* at, _xVec3* pos, float radius);
void xSndSetFrameFar(_xVec3* pos);
void xModel_SceneEnter(RpWorld* world);
void zFX_SceneEnter(RpWorld* world);
void zSceneInit(unsigned int theSceneID);
void zSceneSetup();
void zSceneExit();
void zSceneReset();
void zSceneLoad(zScene* ent, xSerial* s);
void zSceneUpdate(float dt);
void zSceneRender();
xBase* zSceneGetObject(signed int type, signed int idx);
unsigned int zSceneGetArea(unsigned int sceneID);
char* zSceneGetAreaname(unsigned int sceneID);
unsigned int zSceneGetRoominArea(unsigned int sceneID);
void zEnvStartingCamera(_zEnv* env);
void zEntEvent(char* to, unsigned int toEvent);
void zEntEventAll(xBase* from, unsigned int fromEvent, unsigned int toEvent, float* toParam);
void zEntEventAllOfType(unsigned int toEvent, unsigned int type);
void zEntPlayer_StoreCheckPoint(float* toParam);
void zEntShaggy_UpdateDep();
void zCutsceneMgrFinishLoad(xBase* mgr);
void zGameModeSwitch(eGameMode modeNew);
eGameMode zGameModeGet();
void zGameStateSwitch(signed int theNewState);
void zGameSetOstrich(_GameOstrich o);
_GameOstrich zGameGetOstrich();
void zGameExtras(float dt);
void zGameExtrasInit();
void zGameExtrasReset();
void zHud_Init();
void zHud_Setup(_xScene* sc);
void zHud_Update(_xScene* sc, float dt);
void zHud_DrawText(float x, float y, float w, float h, char* text, signed int justify, float size,
                   unsigned char alpha, eHudDropShadowMode shadow);
void zLaughInit();
void zLaughUpdate(float dt);
void zMusicInit();
void zMusicKill();
void zMusicNotify(signed int situation);
void zMusicUpdate(float dt);
signed int zMenuIsFirstBoot();
signed int zMenuRunning();
unsigned int zSaveLoad_SaveLoop();
void zVarUnlockEndGame();
void zVarUnlockAllMovies();

static unsigned int sPlayerMarkerStartID;
static unsigned int sPlayerMarkerStartCamID;
static float sPlayerStartAngle;
static signed int sPortalling;
static float sGameOverTimer;
float sTimeElapsed;
static long long sTimeLast;
static long long sTimeCurrent;
static RwIm2DVertex Vert[4];
static unsigned short Idx[6] = { 0, 1, 2, 0, 2, 3 };
unsigned int startPressed = 0xFFFFFFFF;
iColor_tag black = { 0, 0, 0, 255 };
iColor_tag clear = { 0, 0, 0, 0 };
long long t0;
long long t1;
unsigned int gLastVsyncFlip;
static RwCamera* sGameScreenTransCam;
static RpWorld* World;
static RpLight* DirectionalLight;
static unsigned int loadMeter;
static float sCheatTimer;
unsigned int sCheatUnlockCredits[14] = {
    0, 0, 0, 0, 0, 0, 0,
    0, 0x40000, 0x20000, 0x20000, 0x40000, 0x20000, 0x40000,
};
unsigned int sCheatUnlockAllMovies[14] = {
    0, 0, 0, 0, 0, 0x40000, 0x40000,
    0x40000, 0x20000, 0x20000, 0x20000, 0x40000, 0x20000, 0x40000,
};
unsigned int sCheatWarpGate[14] = {
    0, 0, 0, 0, 0x40000, 0x40000, 0x20000,
    0x40000, 0x40000, 0x20000, 0x40000, 0x20000, 0x20000, 0x20000,
};
unsigned int sCheatAllPowerups[14] = {
    0x20000, 0x40000, 0x20000, 0x40000, 0x20000, 0x40000, 0x40000,
    0x40000, 0x20000, 0x20000, 0x40000, 0x20000, 0x20000, 0x20000,
};
unsigned int sCheatUnused[14] = {
    0x40000, 0x40000, 0x40000, 0x40000, 0x40000, 0x40000, 0x40000,
    0x40000, 0x40000, 0x40000, 0x40000, 0x40000, 0x40000, 0x40000,
};
unsigned int sCheatPressed[14] = { 0 };

void zGameScreenTransitionUpdate(float percentComplete, char* msg, unsigned char* rgba);

#pragma auto_inline on

static _xMat4x3* xEntGetFrameMat(_xEnt* ent)
{
    return (_xMat4x3*)ent->model->Mat;
}
void zHudShowSaveSuccess();

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zGameInit(unsigned int theSceneID)
{
    isrand(iTimeGet());
    iTimeGet();
    irand();
    zSceneInit(theSceneID);
    xCameraCreate(&globals.camera, 640, 480);
    xCameraAssignEnv(&globals.camera, globals.sceneCur->env);
    zLaughInit();
    zMusicInit();
    globals.stats.gameTime = 0.0f;
    globals.stats.tokens = 0;
    globals.stats.tokens_tot = 0;
    globals.stats.snacks = 0;
    globals.stats.snacks_tot = 0;
    zHud_Init();
    zHud_Setup((_xScene*)globals.sceneCur);
}

void zGameExit()
{
    zGameExtrasReset();
    xPadDestroyRumbleChain(0);
    zMusicKill();
    xSndStopAll();
    xSndUpdate(0.0f);
    xCameraRemoveEnv(&globals.camera);
    xCameraDestroy(&globals.camera);
    zSceneExit();
}

void setupPlayer();

void zGameSetup()
{
    RpWorld* world;

    zSceneSetup();
    world = **(RpWorld***)globals.sceneCur->env;
    xModel_SceneEnter(world);
    zFX_SceneEnter(world);
    setupPlayer();
    zEntShaggy_SceneStart(globals.sceneCur);
    zEnvStartingCamera(gCurEnv);
    xCameraUpdate(&globals.camera, -1.0f);
    zSceneLoad(globals.sceneCur, 0);
    zMusicNotify(0);
    zEntEventAll(0, 0, 0x57, 0);
    zEntEventAll(0, 0, 0x59, 0);
    zGameExtrasInit();
}

static void zGameCheats(float dt);
signed int zGameIsNickCam2();
static void zGameUpdateMode();
signed int zGameIsPaused();

static signed int zGameLoopContinue()
{
    if (gGameMode == eGameMode_Game)
    {
        return (unsigned char)(gGameState == 1 || gGameState == 3 || gGameState == 4);
    }

    if (gGameMode == eGameMode_Save)
    {
        unsigned int res = zSaveLoad_SaveLoop();
        sTimeLast = iTimeGet();
        t1 = iTimeGet();
        if (res)
        {
            zHudShowSaveSuccess();
        }
    }
    return 1;
}

// TODO: matches, but .sdata2 has 0.1f right after zGameCheats' constants (before
// setupPlayer's); here it lands next to 1/60.
void zGameLoop()
{
    signed int ostrich_delay = 10;

    zGameStateSwitch(1);
    sTimeLast = iTimeGet() - (long long)((GET_BUS_FREQUENCY() >> 2) / 60.0f);
    if (globals.cmgr)
    {
        zCutsceneMgrFinishLoad((xBase*)globals.cmgr);
    }
    fn_80014A34(0);

    do
    {
        signed int paused;

        sTimeCurrent = iTimeGet();
        sTimeElapsed = iTimeDiffSec(sTimeLast, sTimeCurrent);
        if (sTimeElapsed < 0.0f)
        {
            sTimeElapsed = 1.0f / 60.0f;
        }
        else if (sTimeElapsed > 0.1f)
        {
            sTimeElapsed = 0.1f;
        }
        sTimeLast = sTimeCurrent;
        t0 = t1;
        t1 = iTimeGet();

        xPadUpdate(0, sTimeElapsed);
        iTRCDisk::CheckDVDAndResetState();
        globals.update_dt = sTimeElapsed;
        zGameCheats(sTimeElapsed);
        zGameExtras(sTimeElapsed);
        xDrawBegin();
        paused = zGameIsPaused();
        zSceneUpdate(sTimeElapsed);
        zEntShaggy_UpdateDep();
        if (!paused)
        {
            globals.player.ent.update(&globals.player.ent, globals.sceneCur, sTimeElapsed);
        }
        xSndSetFrame(&globals.camera.hi_cam.mat.right, &globals.camera.hi_cam.mat.up, &globals.camera.hi_cam.mat.at,
                     &globals.camera.hi_cam.mat.pos, 25.0f);
        xSndSetFrameFar(&xEntGetFrameMat(&globals.player.ent)->pos);
        xSndUpdate(sTimeElapsed);
        zHud_Update((_xScene*)globals.sceneCur, sTimeElapsed);
        xCameraUpdate(&globals.camera, sTimeElapsed);
        zScene_UpdateFlyToInterface(sTimeElapsed);
        xCameraBegin(&globals.camera, 1);
        fn_800056F8(&globals.camera.hi_cam.mat.pos);
        zSceneRender();
        xTRCRender();
        fn_80021B20();
        xDrawEnd();
        xCameraEnd(&globals.camera, sTimeElapsed, 1);
        iEnvEndRenderFX(0);
        while ((gVsyncCount = VIGetRetraceCount()) - gLastVsyncFlip < 2)
        {
            iVSync();
        }
        gLastVsyncFlip = gVsyncCount;
        xCameraShowRaster(&globals.camera);
        zLaughUpdate(sTimeElapsed);
        zMusicUpdate(sTimeElapsed);
        zGameUpdateMode();
        gFrameCount++;
        if (ostrich_delay > 0)
        {
            ostrich_delay--;
        }
        else
        {
            zGameSetOstrich(eGameOstrich_InScene);
        }
    } while (zGameLoopContinue());

    fn_80014A34(3);
}

signed int zGameIsPaused()
{
    if (gGameMode == eGameMode_Pause)
    {
        return 1;
    }
    if (gGameMode == eGameMode_Save)
    {
        return 1;
    }
    if (!zMenuRunning() && gTrcPad[0].state != 2)
    {
        return 1;
    }
    return 0;
}

static void zGamePause()
{
    if (gGameMode != eGameMode_Pause)
    {
        zGameModeSwitch(eGameMode_Pause);
        zGameStateSwitch(0);
        iPadStopRumble(globals.pad0);
        zEntEvent("techbutton6_click", 0x18);
    }
}

// TODO: 97.6%. The eGameMode_Load test should branch over a "b end" (bne/b) instead of beq.
void zGamePauseIfPossible()
{
    if (zGameGetOstrich() == eGameOstrich_InScene && !zMenuRunning() &&
        zGameModeGet() != eGameMode_Save)
    {
        if (zGameModeGet() == eGameMode_Load)
        {
            return;
        }
        signed int nick = zGameIsNickCam2();
        if (!nick)
        {
            zGamePause();
        }
    }
}

static signed int zGameOkToPause()
{
    if (!globals.player.ControlOn)
    {
        return 0;
    }
    if (globals.cmgr)
    {
        return 0;
    }
    return 1;
}

static void zGameUpdateMode()
{
    if (gGameMode != eGameMode_Game)
    {
        return;
    }

    if (gGameState == 1)
    {
        iTimeGameAdvance(sTimeElapsed);
        if (globals.pad0->pressed & 1)
        {
            signed int ok = zGameOkToPause();
            if (ok)
            {
                if (!startPressed)
                {
                    zGamePause();
                }
                else
                {
                    startPressed = 1;
                }
            }
        }
        else
        {
            startPressed = 0;
        }
    }
    else if (gGameState == 4)
    {
    }
    else if (gGameState == 5)
    {
        zPortalAsset* passet = globals.sceneCur->pendingPortal->passet;
        char* id = (char*)&passet->sceneID;
        unsigned int nextSceneID = (id[0] << 24) | (id[1] << 16) | (id[2] << 8) | id[3];

        if (passet->sceneID != globals.sceneCur->sceneID)
        {
            signed int vbl;

            sPlayerMarkerStartID = passet->assetMarkerID;
            sPlayerMarkerStartCamID = passet->assetCameraID;
            sPlayerStartAngle = passet->ang;
            sPortalling = 1;
            zGameExit();
            vbl = 30;
            if (nextSceneID == 'S006')
            {
                vbl = 120;
            }
            while (vbl-- > 0)
            {
                iVSync();
            }
            zGameInit(nextSceneID);
            zGameSetup();
            if (globals.cmgr)
            {
                zCutsceneMgrFinishLoad((xBase*)globals.cmgr);
            }
        }
        else if (sPlayerMarkerStartID)
        {
            unsigned int size;
            p2MarkerAsset* m = (p2MarkerAsset*)xSTFindAsset(sPlayerMarkerStartID, &size);
            if (m)
            {
                globals.player.ent.frame->mat.pos = m->pos;
            }
            sPlayerMarkerStartID = 0;
        }

        _tagxCam* cam = (_tagxCam*)zSceneFindObject(sPlayerMarkerStartCamID);
        if (!cam)
        {
            xBase* b = zSceneGetObject(7, 0);
            sPlayerMarkerStartCamID = b->id;
            for (signed int i = 0; i < 50; i++)
            {
            }
            for (signed int i = 0; i < 50; i++)
            {
            }
            xSTAssetName(b->id);
        }
        if (cam)
        {
            xCameraPickInitial(&globals.camera, cam);
            globals.player.cp.initCamID = sPlayerMarkerStartCamID;
            zEntPlayer_StoreCheckPoint(0);
        }
        sPlayerMarkerStartCamID = 0;
        sTimeLast = iTimeGet() - (long long)((GET_BUS_FREQUENCY() >> 2) / 60.0f);
        zGameStateSwitch(1);
    }
    else if (gGameState == 2)
    {
        zSceneReset();
        zGameStateSwitch(1);
    }
    else if (gGameState == 3)
    {
        if (0.0f == sGameOverTimer)
        {
            sGameOverTimer = 5.0f;
            xScrFxFade(&clear, &black, 4.5f, 0, 1, 1);
        }
        else
        {
            sGameOverTimer -= sTimeElapsed;
            xprintf("GAME OVER (%f secs)\n", sGameOverTimer);
            if (sGameOverTimer <= 0.0f)
            {
                sGameOverTimer = 0.0f;
                zGameStateSwitch(7);
            }
        }
    }
    else if (gGameState == 7)
    {
        sGameOverTimer = 0.0f;
    }
}

void zGameScreenTransitionBegin()
{
    zGameSetOstrich(eGameOstrich_Loading);
    sGameScreenTransCam = iCameraCreate(640, 480);
    if (sGameScreenTransCam)
    {
        DirectionalLight = RpLightCreate(1);
        if (DirectionalLight)
        {
            RwRGBAReal col;
            col.red = col.green = col.blue = 1.0f;
            col.alpha = 0.0f;
            RpLightSetColor(DirectionalLight, &col);
            RwFrame* frame = RwFrameCreate();
            _rwObjectHasFrameSetFrame(DirectionalLight, frame);
            RwBBox box;
            box.sup.z = box.sup.y = box.sup.x = 10000.0f;
            box.inf.z = box.inf.y = box.inf.x = -10000.0f;
            World = RpWorldCreate(&box);
            RpWorldAddCamera(World, sGameScreenTransCam);
        }
    }
}

void zGameScreenTransitionUpdate(float percentComplete, char* msg)
{
    if (!zMenuIsFirstBoot())
    {
        zGameScreenTransitionUpdate(percentComplete, msg, 0);
    }
}

void zGameScreenTransition_UIRender(float x1, float y1, float x2, float y2, signed int tex);

void zGameScreenTransitionUpdate(float percentComplete, char* msg, unsigned char* rgba)
{
    if (zMenuIsFirstBoot())
    {
        return;
    }

    RwRGBA back_col = { 0, 0, 0, 255 };
    if (rgba)
    {
        back_col.red = rgba[0];
        back_col.green = rgba[1];
        back_col.blue = rgba[2];
        back_col.alpha = rgba[3];
    }

    xPadUpdate(0, 1.0f);
    xDrawBegin();
    if (sGameScreenTransCam)
    {
        RwCameraClear(sGameScreenTransCam, &back_col, 3);
        RwCameraBeginUpdate(sGameScreenTransCam);
    }

    xprintf("Loading... %3.2f\n", 100.0f * percentComplete);
    if (msg)
    {
        xprintf(msg);
    }

    char meter[256] = "...";
    char str[256];
    switch ((loadMeter / 25) % 5)
    {
    case 0:
        strcpy(meter, "   ");
        break;
    case 1:
        strcpy(meter, ".  ");
        break;
    case 2:
        strcpy(meter, ".. ");
        break;
    case 3:
        strcpy(meter, "...");
        break;
    case 4:
        loadMeter = 0;
        break;
    }

    zGameScreenTransition_UIRender(0.0f, 0.0f, 640.0f, 480.0f, 0);
    zGameScreenTransition_UIRender(275.0f, 32.0f, 640.0f, 448.0f, 1);

    if (globals.sceneCur)
    {
        if (zSceneGetArea(globals.sceneCur->sceneID) == 0)
        {
            sprintf(str, "Loading%s", meter);
            zHud_DrawText(210.0f, 410.0f, 400.0f, 200.0f, str, 0x10, 1.5f, 200,
                          (eHudDropShadowMode)1);
        }
        else
        {
            sprintf(str, "Entering%s", meter);
            zHud_DrawText(210.0f, 410.0f, 300.0f, 60.0f, str, 0x10, 1.0f, 200,
                          (eHudDropShadowMode)1);
            if (globals.sceneCur->sceneID == 'H003')
            {
                sprintf(str, "%s", zSceneGetAreaname(globals.sceneCur->sceneID));
                zHud_DrawText(32.0f, 32.0f, 385.0f, 400.0f, str, 0x40, 1.2f, 200,
                              (eHudDropShadowMode)1);
            }
            else
            {
                unsigned int room = zSceneGetRoominArea(globals.sceneCur->sceneID);
                sprintf(str, "%s\n\nPart %d", zSceneGetAreaname(globals.sceneCur->sceneID),
                        room + 1);
                zHud_DrawText(32.0f, 80.0f, 385.0f, 400.0f, str, 0x40, 1.2f, 200,
                              (eHudDropShadowMode)1);
            }
        }
    }

    loadMeter++;
    xTRCRender();
    xFontRender();
    fn_80021B20();
    xDrawEnd();
    if (sGameScreenTransCam)
    {
        RwCameraEndUpdate(sGameScreenTransCam);
        RwCameraShowRaster(sGameScreenTransCam, 0, 1);
    }
}

void zGameScreenTransition_UIRender(float x1, float y1, float x2, float y2, signed int tex)
{
    float z;
    unsigned char r;
    unsigned char g;
    unsigned char b;

    if (tex)
    {
        RwTexture* texture = (RwTexture*)xSTFindAsset(xStrHash("loading screen bg"), 0);
        if (texture && texture->raster)
        {
            RwRenderStateSet(1, texture->raster);
        }
    }
    else
    {
        RwRenderStateSet(1, 0);
    }

    if (tex)
    {
        b = 0xFF;
        g = 0xFF;
        r = 0xFF;
    }
    else
    {
        b = 0;
        g = 0;
        r = 0;
    }

    z = RwIm2DGetNearScreenZ();

    Vert[0].x = x1;
    Vert[0].y = y1;
    Vert[0].z = z;
    Vert[0].u = 0.0f;
    Vert[0].v = 0.0f;
    Vert[1].x = x1;
    Vert[1].y = y2;
    Vert[1].z = z;
    Vert[1].u = 0.0f;
    Vert[1].v = 1.0f;
    Vert[2].x = x2;
    Vert[2].y = y2;
    Vert[2].z = z;
    Vert[2].u = 1.0f;
    Vert[2].v = 1.0f;
    Vert[3].x = x2;
    Vert[3].y = y1;
    Vert[3].z = z;
    Vert[3].u = 1.0f;
    Vert[3].v = 0.0f;

    Vert[0].emissiveColor.red = r;
    Vert[0].emissiveColor.green = g;
    Vert[0].emissiveColor.blue = b;
    Vert[0].emissiveColor.alpha = 0xFF;
    Vert[1].emissiveColor.red = r;
    Vert[1].emissiveColor.green = g;
    Vert[1].emissiveColor.blue = b;
    Vert[1].emissiveColor.alpha = 0xFF;
    Vert[2].emissiveColor.red = r;
    Vert[2].emissiveColor.green = g;
    Vert[2].emissiveColor.blue = b;
    Vert[2].emissiveColor.alpha = 0xFF;
    Vert[3].emissiveColor.red = r;
    Vert[3].emissiveColor.green = g;
    Vert[3].emissiveColor.blue = b;
    Vert[3].emissiveColor.alpha = 0xFF;

    RwRenderStateSet(0xE, 0);
    RwRenderStateSet(0xA, (void*)2);
    RwRenderStateSet(0xB, (void*)1);
    RwRenderStateSet(7, (void*)1);
    RwRenderStateSet(6, 0);
    RwRenderStateSet(8, 0);
    RwIm2DRenderIndexedPrimitive(3, Vert, 4, Idx, 6);
}

void zGameScreenTransitionEnd()
{
    _rwFrameSyncDirty();
    if (DirectionalLight)
    {
        RwFrame* frame = (RwFrame*)DirectionalLight->object.parent;
        if (frame)
        {
            RwFrameDestroy(frame);
        }
        RpLightDestroy(DirectionalLight);
        DirectionalLight = 0;
    }
    if (World)
    {
        if (sGameScreenTransCam)
        {
            RpWorldRemoveCamera(World, sGameScreenTransCam);
            iCameraDestroy(sGameScreenTransCam);
            sGameScreenTransCam = 0;
        }
        RpWorldDestroy(World);
        World = 0;
    }
}

void setupPlayer()
{
    unsigned int size;
    p2EntAsset* asset =
        (p2EntAsset*)xSTFindAssetByType('PLYR', xSTAssetCountByType('PLYR') - 1, 0);

    asset->baseType = 3;
    if (sPortalling)
    {
        asset->ang.x = (3.1415927f * sPlayerStartAngle) / 180.0f;
        sPortalling = 0;
    }
    asset->ang.y = 0.0f;
    asset->ang.z = 0.0f;
    zEntPlayer_Init(&globals.player.ent, asset);

    if (sPlayerMarkerStartID)
    {
        p2MarkerAsset* m = (p2MarkerAsset*)xSTFindAsset(sPlayerMarkerStartID, &size);
        if (m)
        {
            globals.player.ent.frame->mat.pos = m->pos;
            *(_xVec3*)&globals.player.ent.model->Mat->pos = m->pos;
            xCameraSetTarget(&globals.camera, &globals.player.ent.frame->mat.pos);
            xCameraSetTarget(&globals.camera, xEntGetFrameMat(&globals.player.ent));
        }
        sPlayerMarkerStartID = 0;
    }

    zEntEventAllOfType(0xC2, 0x1E);
}

static signed int TestCheat(unsigned int* cheat_info)
{
    signed int i;

    for (i = 13; cheat_info[i] != 0 && i >= 0; i--)
    {
        if (cheat_info[i] != sCheatPressed[i])
        {
            return 0;
        }
    }

    switch (irand() & 3)
    {
    case 0:
        xSndPlay(xStrHash("scooby_excite_01"), 0.5f, 0.5f, 0.0f, 0x80, 0x202, 0xDEADBEEF);
        break;
    case 1:
        xSndPlay(xStrHash("scooby_excite_04"), 0.5f, 0.5f, 0.0f, 0x80, 0x202, 0xDEADBEEF);
        break;
    case 2:
        xSndPlay(xStrHash("scooby_excite_09"), 0.5f, 0.5f, 0.0f, 0x80, 0x202, 0xDEADBEEF);
        break;
    case 3:
    default:
        xSndPlay(xStrHash("scooby_excite_10"), 0.5f, 0.5f, 0.0f, 0x80, 0x202, 0xDEADBEEF);
        break;
    }
    return 1;
}

// TODO: 99.9%. pressed gets r6 instead of r4.
static void zGameCheats(float dt)
{
    if (zGameModeGet() != eGameMode_Pause)
    {
        return;
    }

    if ((globals.pad0->on & 0x1100) == 0x1100)
    {
        unsigned int pressed = globals.pad0->pressed & 0xF0000;
        if (pressed)
        {
            for (signed int i = 1; i < 14; i++)
            {
                sCheatPressed[i - 1] = sCheatPressed[i];
            }
            sCheatPressed[13] = pressed;
            sCheatTimer = 0.35f;

            if (TestCheat(sCheatWarpGate))
            {
                zEntPickupCheat_allWarps();
            }
            if (TestCheat(sCheatAllPowerups))
            {
                globals.player.Inv_Specials |= 0x1;
                globals.player.Inv_Specials |= 0x2;
                globals.player.Inv_Specials |= 0x4;
                globals.player.Inv_Specials |= 0x10;
                globals.player.Inv_Specials |= 0x8;
                globals.player.Inv_Specials |= 0x20;
                globals.player.Inv_Specials |= 0x200;
                globals.player.Inv_Specials |= 0x400;
                globals.player.Inv_Specials |= 0x800;
                globals.player.Inv_Specials |= 0x1000;
                globals.player.Inv_Specials |= 0x2000;
                globals.player.Inv_Specials |= 0x4000;
                globals.player.Inv_Specials |= 0x40;
                globals.player.Inv_Specials |= 0x80;
                globals.player.Inv_Specials |= 0x100;
            }
            if (TestCheat(sCheatUnlockCredits))
            {
                zVarUnlockEndGame();
            }
            if (TestCheat(sCheatUnlockAllMovies))
            {
                zVarUnlockAllMovies();
            }
            sCheatTimer = 0.35f;
            return;
        }

        sCheatTimer -= dt;
        if (sCheatTimer <= 0.0f)
        {
            sCheatTimer = 0.35f;
            memset(sCheatPressed, 0, sizeof(sCheatPressed));
        }
    }
    else
    {
        sCheatTimer = 0.35f;
        memset(sCheatPressed, 0, sizeof(sCheatPressed));
    }
}

signed int zGameIsNickCam2()
{
    return xStrHash("NICK CAM 2") == globals.camera.hi_cam.asset->id;
}

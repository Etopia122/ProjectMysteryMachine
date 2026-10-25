#include "xBase.h"
#include "xMemMgr.h"
#include "zGlobals.h"
#include "zScene.h"
#include "iDraw.h"

extern "C" {
void* memset(void* dst, int c, unsigned int n);

void fn_800CD254();
void fn_800CD268(signed int enable);
}
void zVarDefeatMasterMind();
void zVarUnlockF001();
void zVarUnlockE001();
void zVarUnlockS001();
void zVarUnlockR001();
void zVarGiveMap();
signed int zVarEntryCB_VibrationOn();
signed int zVarEntryCB_SndFXVol();
signed int zVarEntryCB_SndMusicVol();
signed int zVarEntryCB_SndMasterVol();
signed int zVarEntryCB_SndMode();

class xSerial;

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

enum eFMVFile
{
    eFMVFile_Force32 = 0x7FFFFFFF
};

enum _tagSDRumbleType
{
    SDR_Force32 = 0x7FFFFFFF
};

char* xSceneID2Name(_xScene* sc, unsigned int id);
void xBaseLoad(xBase* ent, xSerial* s);
void xBaseSave(xBase* ent, xSerial* s);
void xPadRumbleEnable(signed int idx, signed int enable);
void xSndSetMasterVol(float vol, float unused);
void xSndSetStreamVol(float vol, float unused);
void xSndSetFXVol(float vol, float unused);
void xScrFxFade(iColor_tag* base, iColor_tag* dest, float seconds, void (*callback)(),
                signed int hold, signed int unused);
void iSndStereo(unsigned int stereo);
void zRumbleStart(signed int idx, _tagSDRumbleType type);
eGameMode zGameModeGet();
void zGameModeSwitch(eGameMode modeNew);
signed int zGameStateGet();
void zGameStateSwitch(signed int theNewState);
void zGameStateSwitchEvent(signed int event);
void zSaveLoad_DispatchCB(unsigned int toEvent, float* toParam);
void zEntPlayer_StoreCheckPoint(float* toParam);
void zHudShow();
void zHudHide();
void zLaughNotify(signed int situation);
void zMusicNotify(signed int situation);
void zMusicNotifyEvent(float* toParam, xBase* toParamWidget);
void zMusicKill();
char* zFMVFileGetName(eFMVFile fileEnum);
unsigned int zFMVPlay(char* filename, unsigned int buttons, float time);
void zMenuFMVPlay(char* filename, unsigned int buttons, float time);
unsigned int finishedGame();

enum en_DISPATCH_COMMAND
{
    ZDSP_CMD_INITDFLT = -1,
    ZDSP_CMD_CTRL_CFGGET = 0,
    ZDSP_CMD_CTRL_CFGSET = 1,
    ZDSP_CMD_CTRL_VIBEGET = 2,
    ZDSP_CMD_CTRL_VIBESET = 3,
    ZDSP_CMD_SNDMOD_GET = 4,
    ZDSP_CMD_SNDMOD_SET = 5,
    ZDSP_CMD_SNDVOL_GET = 6,
    ZDSP_CMD_SNDVOL_SET = 7,
    ZDSP_CMD_SNDVOL_INCR = 8,
    ZDSP_CMD_SNDVOL_DECR = 9,
    ZDSP_CMD_MUSVOL_GET = 10,
    ZDSP_CMD_MUSVOL_SET = 11,
    ZDSP_CMD_MUSVOL_INCR = 12,
    ZDSP_CMD_MUSVOL_DECR = 13,
    ZDSP_CMD_SFXVOL_GET = 14,
    ZDSP_CMD_SFXVOL_SET = 15,
    ZDSP_CMD_SFXVOL_INCR = 16,
    ZDSP_CMD_SFXVOL_DECR = 17,
    ZDSP_CMD_GSTATE_GET = 18,
    ZDSP_CMD_GSTATE_SET = 19,
    ZDSP_CMD_GMODE_GET = 20,
    ZDSP_CMD_GMODE_SET = 21,
    ZDSP_CHECKPNT_SET = 22,
    ZDSP_CMD_NOMORE = 23
};

class st_ZDISPATCH_DATA : public xBase
{
public:
    p2BaseAsset* rawass;   // 0x10
    signed int placeholder; // 0x14
};

class st_ZDISPATCH_CONTEXT
{
public:
    en_DISPATCH_COMMAND cmd; // 0x0
    void* indata;            // 0x4
    void* inxtra;            // 0x8
    void* result;            // 0xC
};

class st_ZDISPATCH_DEPOT
{
public:
    st_ZDISPATCH_DATA* raw_pool; // 0x0
    signed int raw_cnt;          // 0x4
};

#define CLAMP(v, lo, hi) (((lo) > ((v) < (hi) ? (v) : (hi))) ? (lo) : ((v) < (hi) ? (v) : (hi)))

static signed int g_zdsp_init;
static st_ZDISPATCH_DEPOT g_zdsp_depot = { 0, 0 };

static signed int ZDSP_elcb_event(xBase* from, xBase* xb, unsigned int toEvent, float* toParam,
                                  xBase* toParamWidget);
static signed int ZDSP_doCommand(st_ZDISPATCH_DATA* dspdata, st_ZDISPATCH_CONTEXT* cmdCtxt);
static void ZDSP_injectCmd(st_ZDISPATCH_DATA* dspdata, en_DISPATCH_COMMAND cmd, void* indata,
                           void* inxtra, void* result);
static void ZDSP_instInit(st_ZDISPATCH_DATA* dspdata, p2BaseAsset* bass);
void zDispatcher_InitDep(st_ZDISPATCH_DATA* dspdata, zScene* scene);

#pragma auto_inline on

// Functions are in reverse DOL order (-inline deferred emits last to first).

static void ZDSP_nop();

void zDispatcher_Startup()
{
}

void zDispatcher_Shutdown()
{
}

void zDispatcher_scenePrepare()
{
    st_ZDISPATCH_DEPOT* depot = &g_zdsp_depot;

    if (g_zdsp_init++ == 0)
    {
        memset(depot, 0, sizeof(st_ZDISPATCH_DEPOT));
    }
}

void zDispatcher_sceneFinish()
{
    st_ZDISPATCH_DEPOT* depot = &g_zdsp_depot;

    if (--g_zdsp_init == 0)
    {
        memset(depot, 0, sizeof(st_ZDISPATCH_DEPOT));
    }
}

st_ZDISPATCH_DATA* zDispatcher_memPool(signed int cnt)
{
    st_ZDISPATCH_DATA* pool;
    st_ZDISPATCH_DEPOT* depot = &g_zdsp_depot;

    if (cnt < 1)
    {
        return 0;
    }

    pool = (st_ZDISPATCH_DATA*)xMemAlloc(gActiveHeap, cnt * sizeof(st_ZDISPATCH_DATA));
    memset(pool, 0, cnt * 0x18);
    depot->raw_pool = pool;
    depot->raw_cnt = cnt;
    return pool;
}

st_ZDISPATCH_DATA* zDispatcher_getInst(st_ZDISPATCH_DATA* pool, signed int idx)
{
    return &pool[idx];
}

void zDispatcher_Init(st_ZDISPATCH_DATA* dspdata, p2BaseAsset* bass)
{
    ZDSP_instInit(dspdata, bass);
}

void zDispatcher_InitDep(st_ZDISPATCH_DATA* dspdata, zScene*)
{
    xSceneID2Name(globals.sceneCur, dspdata->id);
}

void zDispatcher_Save(st_ZDISPATCH_DATA* dspdata, xSerial* s)
{
    xBaseSave(dspdata, s);
}

void zDispatcher_Load(st_ZDISPATCH_DATA* dspdata, xSerial* s)
{
    xBaseLoad(dspdata, s);
}

static void ZDSP_instInit(st_ZDISPATCH_DATA* dspdata, p2BaseAsset* bass)
{
    xBaseInit(dspdata, bass);
    dspdata->rawass = bass;
    dspdata->eventFunc = ZDSP_elcb_event;
    if (dspdata->linkCount)
    {
        dspdata->link = (p2LinkAsset*)((p2BaseAsset*)dspdata->rawass + 1);
    }
    else
    {
        dspdata->link = 0;
    }
    xSceneID2Name(globals.sceneCur, dspdata->id);
}

static void ZDSP_injectCmd(st_ZDISPATCH_DATA* dspdata, en_DISPATCH_COMMAND cmd, void* indata,
                           void* inxtra, void* result)
{
    st_ZDISPATCH_CONTEXT cmdctxt = { ZDSP_CMD_INITDFLT };

    cmdctxt.cmd = cmd;
    cmdctxt.indata = indata;
    cmdctxt.inxtra = inxtra;
    cmdctxt.result = result;
    ZDSP_doCommand(dspdata, &cmdctxt);
}

// TODO: 99.6%. The SNDVOL_SET clamp keeps vol in f2 instead of f1 (an extra branch).
static signed int ZDSP_doCommand(st_ZDISPATCH_DATA* dspdata, st_ZDISPATCH_CONTEXT* cmdCtxt)
{
    en_DISPATCH_COMMAND cmd = cmdCtxt->cmd;
    signed int* indata = (signed int*)cmdCtxt->indata;
    signed int* result = (signed int*)cmdCtxt->result;
    float vol;
    signed int iv;
    static signed int warned = 0.0f;

    if (!warned)
    {
        warned = 1;
    }

    switch (cmd)
    {
    case ZDSP_CMD_CTRL_CFGGET:
        *result = 0;
        break;
    case ZDSP_CMD_CTRL_CFGSET:
        if (result)
        {
            *result = 0;
        }
        break;
    case ZDSP_CMD_CTRL_VIBEGET:
        *result = zVarEntryCB_VibrationOn();
        break;
    case ZDSP_CMD_CTRL_VIBESET:
        globals.option_vibration = *indata;
        xPadRumbleEnable(0, globals.option_vibration);
        zRumbleStart(0, (_tagSDRumbleType)0xE);
        break;
    case ZDSP_CMD_SNDMOD_GET:
        *result = zVarEntryCB_SndMode();
        break;
    case ZDSP_CMD_SNDMOD_SET:
        if (*indata == 0)
        {
            iSndStereo(0);
        }
        else if (*indata == 1)
        {
            iSndStereo(1);
        }
        if (result)
        {
            *result = zVarEntryCB_SndMode();
        }
        break;
    case ZDSP_CMD_SNDVOL_GET:
        *result = zVarEntryCB_SndMasterVol();
        break;
    case ZDSP_CMD_SNDVOL_SET:
        vol = 0.1f * *indata;
        vol = CLAMP(vol, 0.0f, 1.0f);
        xSndSetMasterVol(vol, vol);
        if (result)
        {
            *result = zVarEntryCB_SndMasterVol();
        }
        break;
    case ZDSP_CMD_SNDVOL_INCR:
        iv = zVarEntryCB_SndMasterVol();
        if (indata)
        {
            iv += *indata;
        }
        else
        {
            iv += 1;
        }
        vol = 0.1f * iv;
        vol = CLAMP(vol, 0.0f, 1.0f);
        xSndSetMasterVol(vol, vol);
        if (result)
        {
            *result = zVarEntryCB_SndMasterVol();
        }
        break;
    case ZDSP_CMD_SNDVOL_DECR:
        iv = zVarEntryCB_SndMasterVol();
        if (indata)
        {
            iv -= *indata;
        }
        else
        {
            iv -= 1;
        }
        vol = 0.1f * iv;
        vol = CLAMP(vol, 0.0f, 1.0f);
        xSndSetMasterVol(vol, vol);
        if (result)
        {
            *result = zVarEntryCB_SndMasterVol();
        }
        break;
    case ZDSP_CMD_MUSVOL_GET:
        *result = zVarEntryCB_SndMusicVol();
        break;
    case ZDSP_CMD_MUSVOL_SET:
        vol = 0.1f * *indata;
        vol = CLAMP(vol, 0.0f, 1.0f);
        xSndSetStreamVol(vol, vol);
        if (result)
        {
            *result = zVarEntryCB_SndMusicVol();
        }
        break;
    case ZDSP_CMD_MUSVOL_INCR:
        iv = zVarEntryCB_SndMusicVol();
        if (indata)
        {
            iv += *indata;
        }
        else
        {
            iv += 1;
        }
        vol = 0.1f * iv;
        vol = CLAMP(vol, 0.0f, 1.0f);
        xSndSetStreamVol(vol, vol);
        if (result)
        {
            *result = zVarEntryCB_SndMusicVol();
        }
        break;
    case ZDSP_CMD_MUSVOL_DECR:
        iv = zVarEntryCB_SndMusicVol();
        if (indata)
        {
            iv -= *indata;
        }
        else
        {
            iv -= 1;
        }
        vol = 0.1f * iv;
        vol = CLAMP(vol, 0.0f, 1.0f);
        xSndSetStreamVol(vol, vol);
        if (result)
        {
            *result = zVarEntryCB_SndMusicVol();
        }
        break;
    case ZDSP_CMD_SFXVOL_GET:
        *result = zVarEntryCB_SndFXVol();
        break;
    case ZDSP_CMD_SFXVOL_SET:
        vol = 0.1f * *indata;
        vol = CLAMP(vol, 0.0f, 1.0f);
        xSndSetFXVol(vol, vol);
        if (result)
        {
            *result = zVarEntryCB_SndFXVol();
        }
        break;
    case ZDSP_CMD_SFXVOL_INCR:
        iv = zVarEntryCB_SndFXVol();
        if (indata)
        {
            iv += *indata;
        }
        else
        {
            iv += 1;
        }
        vol = 0.1f * iv;
        vol = CLAMP(vol, 0.0f, 1.0f);
        xSndSetFXVol(vol, vol);
        if (result)
        {
            *result = zVarEntryCB_SndFXVol();
        }
        break;
    case ZDSP_CMD_SFXVOL_DECR:
        iv = zVarEntryCB_SndFXVol();
        if (indata)
        {
            iv -= *indata;
        }
        else
        {
            iv -= 1;
        }
        vol = 0.1f * iv;
        vol = CLAMP(vol, 0.0f, 1.0f);
        xSndSetFXVol(vol, vol);
        if (result)
        {
            *result = zVarEntryCB_SndFXVol();
        }
        break;
    case ZDSP_CMD_GSTATE_GET:
        *result = zGameStateGet();
        break;
    case ZDSP_CMD_GSTATE_SET:
        zGameStateSwitch(*indata);
        if (result)
        {
            *result = zGameStateGet();
        }
        break;
    case ZDSP_CMD_GMODE_GET:
        *result = zGameModeGet();
        break;
    case ZDSP_CMD_GMODE_SET:
        zGameModeSwitch((eGameMode)*indata);
        if (result)
        {
            *result = zGameModeGet();
        }
        break;
    case ZDSP_CHECKPNT_SET:
        break;
    }

    return 1;
}

// TODO: 90%. The target's compare tree keeps the empty case ranges 3-9 and 0xB-0x5A as
// their own entries (root at 0xAE); here they fold into the default (root at 0xC2).
// .sdata2 also wants 27.0f after doCommand's int-to-float constant.
static signed int ZDSP_elcb_event(xBase* from, xBase* xb, unsigned int toEvent, float* toParam,
                                  xBase* toParamWidget)
{
    st_ZDISPATCH_DATA* dspdata = (st_ZDISPATCH_DATA*)xb;

    switch (toEvent)
    {
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
    case 0x8:
    case 0x9:
        break;
    case 0xA:
    {
        p2BaseAsset* bass = dspdata->rawass;
        xBaseReset(dspdata, bass);
        memset(&dspdata->rawass, 0, sizeof(st_ZDISPATCH_DATA) - sizeof(xBase));
        zDispatcher_Init(dspdata, bass);
        zDispatcher_InitDep(dspdata, globals.sceneCur);
        break;
    }
    case 0xB:
    case 0xC:
    case 0xD:
    case 0xE:
    case 0xF:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x1F:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2A:
    case 0x2B:
    case 0x2C:
    case 0x2D:
    case 0x2E:
    case 0x2F:
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
    case 0x3A:
    case 0x3B:
    case 0x3C:
    case 0x3D:
    case 0x3E:
    case 0x3F:
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x46:
    case 0x47:
    case 0x48:
    case 0x49:
    case 0x4A:
    case 0x4B:
    case 0x4C:
    case 0x4D:
    case 0x4E:
    case 0x4F:
    case 0x50:
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
    case 0x56:
    case 0x57:
    case 0x58:
    case 0x59:
    case 0x5A:
        break;
    case 0x60:
    {
        signed int iv = 0;
        ZDSP_injectCmd(dspdata, ZDSP_CMD_CTRL_CFGSET, &iv, 0, 0);
        break;
    }
    case 0x61:
    {
        signed int iv = 1;
        ZDSP_injectCmd(dspdata, ZDSP_CMD_CTRL_CFGSET, &iv, 0, 0);
        break;
    }
    case 0x62:
    {
        signed int iv = 2;
        ZDSP_injectCmd(dspdata, ZDSP_CMD_CTRL_CFGSET, &iv, 0, 0);
        break;
    }
    case 0x63:
    {
        signed int iv = 3;
        ZDSP_injectCmd(dspdata, ZDSP_CMD_CTRL_CFGSET, &iv, 0, 0);
        break;
    }
    case 0x64:
    {
        signed int iv = 1;
        ZDSP_injectCmd(dspdata, ZDSP_CMD_CTRL_VIBESET, &iv, 0, 0);
        break;
    }
    case 0x65:
    {
        signed int iv = 0;
        ZDSP_injectCmd(dspdata, ZDSP_CMD_CTRL_VIBESET, &iv, 0, 0);
        break;
    }
    case 0x66:
    {
        signed int iv = 0;
        ZDSP_injectCmd(dspdata, ZDSP_CMD_SNDMOD_SET, &iv, 0, 0);
        break;
    }
    case 0x67:
    {
        signed int iv = 1;
        ZDSP_injectCmd(dspdata, ZDSP_CMD_SNDMOD_SET, &iv, 0, 0);
        break;
    }
    case 0x68:
        ZDSP_injectCmd(dspdata, ZDSP_CMD_SNDVOL_INCR, 0, 0, 0);
        break;
    case 0x69:
        ZDSP_injectCmd(dspdata, ZDSP_CMD_SNDVOL_DECR, 0, 0, 0);
        break;
    case 0x6A:
        ZDSP_injectCmd(dspdata, ZDSP_CMD_MUSVOL_INCR, 0, 0, 0);
        break;
    case 0x6B:
        ZDSP_injectCmd(dspdata, ZDSP_CMD_MUSVOL_DECR, 0, 0, 0);
        break;
    case 0x6C:
        ZDSP_injectCmd(dspdata, ZDSP_CMD_SFXVOL_INCR, 0, 0, 0);
        break;
    case 0x6D:
        ZDSP_injectCmd(dspdata, ZDSP_CMD_SFXVOL_DECR, 0, 0, 0);
        break;
    case 0xA4:
    case 0xA7:
        break;
    case 0x89:
    case 0x8A:
    case 0x8B:
    case 0x8C:
    case 0x8D:
    case 0x8E:
    case 0x8F:
    case 0x90:
    case 0x91:
    case 0x92:
    case 0x93:
    case 0x94:
    case 0x95:
    case 0x96:
    case 0x97:
    case 0x98:
    case 0x99:
    case 0x9A:
    case 0x9B:
    case 0x9C:
    case 0x9D:
    case 0x9E:
    case 0x9F:
    case 0xA0:
    case 0xA1:
    case 0xA2:
    case 0xA3:
    case 0xA5:
    case 0xA6:
        zGameStateSwitchEvent(toEvent);
        break;
    case 0xA8:
    case 0xA9:
    case 0xAA:
    case 0xAB:
    case 0xAC:
    case 0xAD:
        zSaveLoad_DispatchCB(toEvent, toParam);
        break;
    case 0x10C:
        zLaughNotify(0xD);
        break;
    case 0x10D:
        zMusicNotify(3);
        break;
    case 0x10E:
        zMusicNotify(3);
        break;
    case 0x10F:
        zMusicNotify(3);
        break;
    case 0x110:
        zMusicNotify(3);
        break;
    case 0x129:
        zMusicNotifyEvent(toParam, toParamWidget);
        break;
    case 0xC2:
        zEntPlayer_StoreCheckPoint(toParam);
        break;
    case 0x101:
        zVarGiveMap();
        break;
    case 0x102:
        zVarUnlockR001();
        break;
    case 0x103:
        zVarUnlockS001();
        break;
    case 0x104:
        zVarUnlockE001();
        break;
    case 0x105:
        zVarUnlockF001();
        break;
    case 0x106:
    case 0x107:
    case 0x108:
    case 0x109:
    case 0x10A:
    case 0x10B:
        break;
    case 0x11E:
        zHudShow();
        break;
    case 0x11F:
        zHudHide();
        break;
    case 0x120:
    {
        iColor_tag clear = { 0, 0, 0, 0 };
        iColor_tag black = { 0, 0, 0, 255 };
        xScrFxFade(&clear, &black, toParam[0], 0, 1, 1);
        break;
    }
    case 0x126:
        switch ((unsigned int)toParam[0])
        {
        case 0:
            zMenuFMVPlay(zFMVFileGetName((eFMVFile)0x13), 1, 0.1f);
            break;
        case 1:
            zMenuFMVPlay(zFMVFileGetName((eFMVFile)0x14), 1, 0.1f);
            break;
        case 2:
            zMenuFMVPlay(zFMVFileGetName((eFMVFile)0x25), 0, 0.0f);
            break;
        case 3:
            zMenuFMVPlay(zFMVFileGetName((eFMVFile)0x19), 0, 0.0f);
            zMusicKill();
            break;
        case 4:
            zFMVPlay(zFMVFileGetName((eFMVFile)0x19), 1, 0.1f);
            break;
        case 5:
            if (finishedGame())
            {
                zMenuFMVPlay(zFMVFileGetName((eFMVFile)0x19), 1, 0.1f);
            }
            else
            {
                zMenuFMVPlay(zFMVFileGetName((eFMVFile)0x18), 0x10001, 0.1f);
            }
            break;
        case 6:
            zFMVPlay(zFMVFileGetName((eFMVFile)(signed int)(27.0f + toParam[1] - 1.0f)), 1, 0.1f);
            break;
        }
        break;
    case 0x127:
        zVarDefeatMasterMind();
        break;
    case 0x130:
        fn_800CD268(1);
        fn_800CD254();
        break;
    case 0x131:
        fn_800CD268(0);
        break;
    }

    return 1;
}

static void ZDSP_nop()
{
}

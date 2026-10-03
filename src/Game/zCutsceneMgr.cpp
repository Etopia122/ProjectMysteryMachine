#include "xBase.h"
#include "xString.h"
#include "xserializer.h"
#include "zGlobals.h"
#include "zEnt.h"

class xCutscene;
class zCutsceneZbufferHack;

class zCutsceneMgrAsset : public p2BaseAsset
{
public:
    unsigned int cutsceneAssetID; // 0x8
    unsigned int flags;           // 0xC
    float interpSpeed;            // 0x10
    float startTime[15];          // 0x14
    float endTime[15];            // 0x50
    unsigned int emitID[15];      // 0x8C
};

class _zCutsceneMgr : public xBase
{
public:
    zCutsceneMgrAsset* tasset;   // 0x10
    xCutscene* csn;              // 0x14
    unsigned int stop;           // 0x18
    zCutsceneZbufferHack* zhack; // 0x1C
    unsigned int state;          // 0x20 (GameCube only)
};

// The GameCube build plays each cutscene as an FMV (fmvName).
class zCutSceneNames
{
public:
    char name[64];     // 0x0
    signed int played; // 0x40
    signed int skipOK; // 0x44
    char fmvName[64];  // 0x48
};

static zCutSceneNames zCutSceneNamesTable[14] = {
    { "cin_1", 0, 1, "cin1" },
    { "cin_2", 0, 1, "cin2" },
    { "cin_3", 0, 1, "cin3" },
    { "cin_4", 0, 1, "cin4" },
    { "cin_5", 0, 1, "cin5" },
    { "cin_6", 0, 1, "cin6" },
    { "cin_7", 0, 1, "cin7" },
    { "cin_8", 0, 1, "cin8" },
    { "cin_9", 0, 1, "cin9" },
    { "cin_10", 0, 1, "cin10" },
    { "cin_11", 0, 0, "cin11" },
    { "cin_mm_post_death", 0, 0, "cin_mmpd" },
    { "cin_mm_scare", 0, 1, "cin_mmsc" },
    { "cin_sh_barrel", 0, 1, "cin_shbr" },
};

static signed int gCutsceneSkipOK;

// Not yet named: an iCamera.cpp function and the FMV player.
extern "C" void fn_800072D4(float a, float b);
signed int xCutscene_Destroy(xCutscene* csn);
extern "C" void fn_800ABD9C(char* name, unsigned int flags, float time);

void zMusicPause();
void zMusicUnpause(signed int);

signed int zCutsceneMgrEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                               xBase* toParamWidget);

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zCutsceneMgrInit(void* b, void* tasset)
{
    _zCutsceneMgr* t = (_zCutsceneMgr*)b;

    xBaseInit(t, (p2BaseAsset*)tasset);
    t->eventFunc = zCutsceneMgrEventCB;
    t->tasset = (zCutsceneMgrAsset*)tasset;
    t->csn = 0;
    if (t->linkCount)
    {
        t->link = (p2LinkAsset*)(t->tasset + 1);
    }
    else
    {
        t->link = 0;
    }
}

void zCutsceneMgrSave(_zCutsceneMgr*, xSerial* s)
{
    for (unsigned int i = 0; i < 14; i++)
    {
        if (zCutSceneNamesTable[i].played)
        {
            s->Write_b1(1);
        }
        else
        {
            s->Write_b1(0);
        }
    }
}

void zCutsceneMgrLoad(_zCutsceneMgr*, xSerial* s)
{
    for (unsigned int i = 0; i < 14; i++)
    {
        s->Read_b1(&zCutSceneNamesTable[i].played);
    }
}

void zCutSceneNamesTable_clearAll()
{
    for (signed int i = 0; i < 14; i++)
    {
        zCutSceneNamesTable[i].played = 0;
    }
}

signed int zCutsceneMgrEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                               xBase* toParamWidget)
{
    _zCutsceneMgr* t = (_zCutsceneMgr*)to;

    switch (toEvent)
    {
    case 0x18:
    {
        unsigned int idx = 0;
        gCutsceneSkipOK = 1;
        for (unsigned int i = 0; i < 14; i++)
        {
            if (t->tasset->cutsceneAssetID == xStrHash(zCutSceneNamesTable[i].name))
            {
                idx = i;
                zCutSceneNamesTable[i].played = 1;
                gCutsceneSkipOK = zCutSceneNamesTable[i].skipOK;
                break;
            }
        }
        fn_800ABD9C(zCutSceneNamesTable[idx].fmvName,
                    zCutSceneNamesTable[idx].skipOK ? 0x10000 : 0, 3.0f);
        t->state |= 2;
        break;
    }
    case 0x13:
        t->stop = 1;
        break;
    case 0xBE:
        zMusicPause();
        if (!globals.cmgr)
        {
            t->stop = 0;
            globals.cmgr = t;
        }
        t->state = 1;
        break;
    case 0xBF:
        break;
    case 0xA:
        xBaseReset(t, t->tasset);
        if (t->csn)
        {
            xCutscene_Destroy(t->csn);
        }
        t->csn = 0;
        break;
    }

    return 1;
}

void zCutsceneMgrFinishLoad(xBase* to)
{
    _zCutsceneMgr* volatile t = (_zCutsceneMgr*)to;
}

void zCutsceneMgrUpdate(xBase* to, _xScene* sc, float dt)
{
    _zCutsceneMgr* t = (_zCutsceneMgr*)to;

    if (t->state & 1)
    {
        t->state &= ~1;
        zEntEvent(t, t, 0x18);
    }
    if (t->state & 2)
    {
        t->state &= 2;
        zEntEvent(t, t, 0x13);
    }

    if (t->stop)
    {
        signed int unpause;
        if (t->tasset->cutsceneAssetID == xStrHash("cin_2") ||
            t->tasset->cutsceneAssetID == xStrHash("cin_4") ||
            t->tasset->cutsceneAssetID == xStrHash("cin_6") ||
            t->tasset->cutsceneAssetID == xStrHash("cin_10"))
        {
            unpause = 1;
        }
        else
        {
            unpause = 0;
        }
        globals.cmgr = 0;
        zMusicUnpause(unpause);
        zEntEvent(t, t, 0xBF);
        zEntEvent(&globals.player.ent, 9);
        zEntEvent(&globals.player.ent, 3);
        fn_800072D4(0.0f, 0.0f);
        t->stop = 0;
    }
}

unsigned int zCutSceneNamesTable_getStatus(unsigned int number)
{
    return zCutSceneNamesTable[number - 1].played != 0;
}

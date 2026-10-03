#include "zGlobals.h"
#include "zScene.h"
#include "xAnim.h"
#include "xstransvc.h"

extern "C" {
int sprintf(char* s, const char* format, ...);
void* memcpy(void* dst, const void* src, unsigned int n);
void* malloc(unsigned int size);
// TODO: unidentified iSystem debug print (a stub that returns 0)
int fn_80015EAC(const char* format, ...);
// TODO: unidentified (iSnd: registers the sound info asset)
void fn_800D8570(void* sndInfo);
// TODO: unidentified iSystem flag set while the H003 world streams in
extern signed int lbl_8025E984;
}

class _xEnv;
class RpWorld;
class RpAtomic;
class RwRaster;
class RwStream;
class RwTexDictionary;

class RwTexture
{
public:
    RwRaster* raster;                    // 0x0
    unsigned char pad4[0x50 - 0x4];
    unsigned int filterAddressing;       // 0x50
    signed int refCount;                 // 0x54
};

class RwMemory
{
public:
    unsigned char* start;  // 0x0
    unsigned int length;   // 0x4
};

class RwChunkHeaderInfo
{
public:
    unsigned int type;       // 0x0
    unsigned int length;     // 0x4
    unsigned int version;    // 0x8
    unsigned int buildNum;   // 0xC
    signed int isComplex;    // 0x10
};

class st_PACKER_ASSETTYPE
{
public:
    unsigned int typetag;                                                     // 0x0
    unsigned int tflags;                                                      // 0x4
    signed int typalign;                                                      // 0x8
    void* (*readXForm)(void*, unsigned int, void*, unsigned int, unsigned int*); // 0xC
    void* (*writeXForm)(void*, unsigned int, void*, void*, unsigned int, unsigned int*); // 0x10
    signed int (*assetLoaded)(void*, unsigned int, void*, signed int);        // 0x14
    void* (*makeData)(void*, unsigned int, void*, signed int*, signed int*);  // 0x18
    void (*cleanup)(void*, unsigned int, void*);                             // 0x1C
    void (*assetUnloaded)(void*, unsigned int);                              // 0x20
    void (*writePeek)(void*, unsigned int, void*, char*);                    // 0x24
};

class xAnimEffect
{
public:
    xAnimEffect* Next;   // 0x0
    unsigned int Flags;  // 0x4
    float StartTime;     // 0x8
    float EndTime;       // 0xC
    unsigned int (*Callback)(unsigned int, class xAnimActiveEffect*, xAnimSingle*, void*); // 0x10
};

class xAnimActiveEffect
{
public:
    xAnimEffect* Effect; // 0x0
    unsigned int Handle; // 0x4
};

class xAnimState
{
public:
    xAnimState* Next;       // 0x0
    char* Name;             // 0x4
    unsigned int ID;        // 0x8
    unsigned int Flags;     // 0xC
    unsigned int UserFlags; // 0x10
    float Speed;            // 0x14
    xAnimFile* Data;        // 0x18
};

class xAnimTable
{
public:
    xAnimTable* Next;               // 0x0
    char* Name;                     // 0x4
    xAnimTransition* TransitionList; // 0x8
    xAnimState* StateList;          // 0xC
    unsigned int AnimIndex;         // 0x10
    unsigned int MorphIndex;        // 0x14
};

class xAnimTransition
{
public:
    xAnimTransition* Next; // 0x0
    xAnimState* Dest;      // 0x4
};

class zAnimFxSound
{
public:
    unsigned int ID;       // 0x0
    float vol;             // 0x4
    float pitch;           // 0x8
    unsigned int priority; // 0xC
    unsigned int flags;    // 0x10
    float radius;          // 0x14
};

class xAnimAssetTable
{
public:
    unsigned int Magic;         // 0x0
    unsigned int NumRaw;        // 0x4
    unsigned int NumFiles;      // 0x8
    unsigned int NumStates;     // 0xC
    unsigned int ConstructFunc; // 0x10
};

class xAnimAssetFile
{
public:
    unsigned int FileFlags;       // 0x0
    float Duration;               // 0x4
    float TimeOffset;             // 0x8
    unsigned short NumAnims[2];   // 0xC
    void** RawData;               // 0x10
    signed int Physics;           // 0x14
    signed int StartPose;         // 0x18
    signed int EndPose;           // 0x1C
};

class xAnimAssetState
{
public:
    unsigned int StateID;      // 0x0
    unsigned int FileIndex;    // 0x4
    unsigned int EffectCount;  // 0x8
    unsigned int EffectOffset; // 0xC
    float Speed;               // 0x10
};

class xAnimAssetEffect
{
public:
    unsigned int StateID;      // 0x0
    float StartTime;           // 0x4
    float EndTime;             // 0x8
    unsigned int Flags;        // 0xC
    unsigned int EffectType;   // 0x10
    unsigned int UserDataSize; // 0x14
};

class st_zEntVillain;

typedef unsigned int (*xAnimEffectCallback)(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*);

extern "C" {
RwStream* RwStreamOpen(signed int type, signed int accessType, const void* data);
signed int RwStreamClose(RwStream* stream, void* data);
signed int RwStreamFindChunk(RwStream* stream, unsigned int type, unsigned int* lengthOut,
                             unsigned int* versionOut);
RwStream* RwStreamReadChunkHeaderInfo(RwStream* stream, RwChunkHeaderInfo* chunkHeaderInfo);
RpWorld* RpWorldStreamRead(RwStream* stream);
RwTexDictionary* RwTexDictionaryStreamRead(RwStream* stream);
RwTexDictionary* RwTexDictionaryForAllTextures(RwTexDictionary* dict,
                                               RwTexture* (*fpCallBack)(RwTexture*, void*),
                                               void* pData);
RwTexture* RwTexDictionaryRemoveTexture(RwTexture* texture);
signed int RwTexDictionaryDestroy(RwTexDictionary* dict);
signed int RwTextureDestroy(RwTexture* texture);
}

void xSTStartup(st_PACKER_ASSETTYPE* handlers);
void xSTShutdown();
RpAtomic* iModelFileNew(void* buffer, unsigned int size);
void iModelUnload(RpAtomic* userdata);
void xEnvFree(_xEnv* env);
void xMovePointSplineDestroy(class _xMovePoint* m);
void xMorphSeqSetup(void* seq, void* (*FindAssetCB)(unsigned int, char*));
xAnimFile* xAnimFileNewBilinear(void** rawData, char* name, unsigned int flags,
                                xAnimFile** linkedList, unsigned int numX, unsigned int numY);
void xAnimFileSetTime(xAnimFile* data, float duration, float timeOffset);
void xAnimFileSetPhysics(xAnimFile* data, void* rawData);
void xAnimFileSetPoses(xAnimFile* data, void* startPose, float startTime, void* endPose,
                       float endTime);
xAnimState* xAnimTableAddFileID(xAnimTable* table, xAnimFile* file, unsigned int stateID);
xAnimState* xAnimTableGetStateID(xAnimTable* table, unsigned int ID);
xAnimEffect* xAnimStateNewEffect(xAnimState* state, unsigned int flags, float startTime,
                                 float endTime, xAnimEffectCallback callback,
                                 unsigned int userDataSize);
signed int zEntVillain_PlayAFXSound(st_zEntVillain* ent, zAnimFxSound* snd,
                                    unsigned int* sid_played);
unsigned int xSndPlay3D(unsigned int id, float vol, float pitch, unsigned int priority,
                        unsigned int flags, _xEnt* ent, float radius);
void xSndStop(unsigned int snd);

xAnimTable* zEntPlayer_AnimTable();
xAnimTable* zEntVillain_AnimTable_Standard();
xAnimTable* zEntPlayer_UmbrellaAnimTable();
xAnimTable* zEntPlayer_HeadAnimTable();
xAnimTable* zEntShaggy_Anim1();
xAnimTable* zEntPlayer_LampshadeAnimTable();
xAnimTable* zEntShaggy_Anim3();
xAnimTable* zEntShaggy_Anim2();
xAnimTable* zEntShaggy_Chimney();
xAnimTable* zEntVillain_AnimTable_BlackKnight();
xAnimTable* zEntVillain_AnimTable_GreenGhost();
xAnimTable* zEntVillain_AnimTable_RedBeard();
xAnimTable* zEntVillain_AnimTable_MasterMind();
xAnimTable* zEntVillain_AnimTable_BossSham();
xAnimTable* zEntVillain_AnimTable_Simple();
xAnimTable* zEntVillain_AnimTable_Gang();
xAnimTable* zEntVillain_AnimTable_Shark();
xAnimTable* zEntShaggy_Anim0();
xAnimTable* zEntShaggy_Anim8();
xAnimTable* zEntShaggy_Anim5();
xAnimTable* zEntVillain_AnimTable_Witch();
xAnimTable* zEntPlayer_TongueAnimTable();

extern unsigned int gxAnimUseGrowAlloc;

#define SCENE_ID(s) ((((s)[0] & ~0x20) << 24) | ((s)[1] << 16) | ((s)[2] << 8) | (s)[3])

static void* ATBL_Read(void*, unsigned int, void* indata, unsigned int insize,
                       unsigned int* outsize);
static void* RWTX_Read(void*, unsigned int, void* indata, unsigned int insize,
                       unsigned int* outsize);
static void* Model_Read(void*, unsigned int, void* indata, unsigned int insize,
                        unsigned int* outsize);
static void* BSP_Read(void*, unsigned int assetid, void* indata, unsigned int insize,
                      unsigned int* outsize);
static void* SndInfoRead(void*, unsigned int, void* indata, unsigned int insize,
                         unsigned int* outsize);
static void Model_Unload(void* userdata, unsigned int);
static void BSP_Unload(void*, unsigned int);
static void Anim_Unload(void*, unsigned int);
static void TextureRW3_Unload(void* userdata, unsigned int);
static void MovePoint_Unload(void* userdata, unsigned int);
static unsigned int dummyEffectCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*);
static unsigned int soundEffectCB(unsigned int cbenum, xAnimActiveEffect* acteffect,
                                  xAnimSingle* single, void* object);

static st_PACKER_ASSETTYPE assetTypeHandlers[56] = {
    { 'BSP ', 0, 0, BSP_Read, 0, 0, 0, 0, BSP_Unload, 0 },
    { 'TXD ' },
    { 'MODL', 0, 0, Model_Read, 0, 0, 0, 0, Model_Unload, 0 },
    { 'ANIM', 0, 0, 0, 0, 0, 0, 0, Anim_Unload, 0 },
    { 'RWTX', 0, 0, RWTX_Read, 0, 0, 0, 0, TextureRW3_Unload, 0 },
    { 'CAM ' },
    { 'PLYR' },
    { 'NPC ' },
    { 'ITEM' },
    { 'PKUP' },
    { 'TRIG' },
    { 'SDF ' },
    { 'TEX ' },
    { 'TXT ' },
    { 'ENV ' },
    { 'ATBL', 0, 0, ATBL_Read, 0, 0, 0, 0, 0, 0 },
    { 'MINF' },
    { 'PICK' },
    { 'PLAT' },
    { 'PEND' },
    { 'MRKR' },
    { 'MVPT', 0, 0, 0, 0, 0, 0, 0, MovePoint_Unload, 0 },
    { 'TIMR' },
    { 'CNTR' },
    { 'PORT' },
    { 'SND ' },
    { 'SNDS' },
    { 'GRUP' },
    { 'MPHT' },
    { 'SFX ' },
    { 'SNDI', 0, 0, SndInfoRead, 0, 0, 0, 0, 0, 0 },
    { 'HANG' },
    { 'SIMP' },
    { 'BUTN' },
    { 'SURF' },
    { 'DSTR' },
    { 'MAPR' },
    { 'GUST' },
    { 'VOLU' },
    { 'UI  ' },
    { 'UIFT' },
    { 'TEXT' },
    { 'COND' },
    { 'DPAT' },
    { 'PRJT' },
    { 'LOBM' },
    { 'FOG ' },
    { 'LITE' },
    { 'PARE' },
    { 'PARS' },
    { 'CSN ' },
    { 'CTOC' },
    { 'CSNM' },
    { 'EGEN' },
    { 'ALST' },
    { 0 },
};

static xAnimTable* (*tableFuncList[22])() = {
    zEntPlayer_AnimTable,
    zEntVillain_AnimTable_Standard,
    zEntPlayer_UmbrellaAnimTable,
    zEntPlayer_HeadAnimTable,
    zEntShaggy_Anim1,
    zEntPlayer_LampshadeAnimTable,
    zEntShaggy_Anim3,
    zEntShaggy_Anim2,
    zEntShaggy_Chimney,
    zEntVillain_AnimTable_BlackKnight,
    zEntVillain_AnimTable_GreenGhost,
    zEntVillain_AnimTable_RedBeard,
    zEntVillain_AnimTable_MasterMind,
    zEntVillain_AnimTable_BossSham,
    zEntVillain_AnimTable_Simple,
    zEntVillain_AnimTable_Gang,
    zEntVillain_AnimTable_Shark,
    zEntShaggy_Anim0,
    zEntShaggy_Anim8,
    zEntShaggy_Anim5,
    zEntVillain_AnimTable_Witch,
    zEntPlayer_TongueAnimTable,
};

static xAnimEffectCallback effectFuncList[2] = { dummyEffectCB, soundEffectCB };

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zAssetStartup()
{
    xSTStartup(assetTypeHandlers);
}

void zAssetShutdown()
{
    xSTShutdown();
}

static void* Model_Read(void*, unsigned int, void* indata, unsigned int insize,
                        unsigned int* outsize)
{
    RpAtomic* model = iModelFileNew(indata, insize);
    *outsize = 0x70;
    return model;
}

static void Model_Unload(void* userdata, unsigned int)
{
    if (userdata)
    {
        iModelUnload((RpAtomic*)userdata);
    }
}

static void* BSP_Read(void*, unsigned int assetid, void* indata, unsigned int insize,
                      unsigned int* outsize)
{
    RwMemory rwmem = { (unsigned char*)indata, insize };
    RwStream* stream;
    RpWorld* bsp;

    stream = RwStreamOpen(3, 1, &rwmem);
    if (!stream)
    {
        fn_80015EAC("BSP_Read RwStreamOpen failed\n");
    }

    if (!RwStreamFindChunk(stream, 0xB, 0, 0))
    {
        RwChunkHeaderInfo chunkHeaderInfo;
        RwStreamReadChunkHeaderInfo(stream, &chunkHeaderInfo);
        *outsize = 0;
        return 0;
    }

    if (globals.sceneCur->sceneID == SCENE_ID("H003"))
    {
        lbl_8025E984 = 1;
    }
    bsp = RpWorldStreamRead(stream);
    if (lbl_8025E984)
    {
        lbl_8025E984 = 0;
    }
    if (!bsp)
    {
        fn_80015EAC("BSP_Read RpWorldStreamRead failed\n");
    }
    RwStreamClose(stream, 0);
    fn_80015EAC("BSP_Read(BSP AID:0x%08X):%s\n", assetid, bsp ? "Ok" : "BAD!");
    *outsize = 4;
    return bsp;
}

static void BSP_Unload(void*, unsigned int)
{
    xEnvFree(globals.sceneCur->env);
}

static RwTexture* TexCB(RwTexture* texture, void* data)
{
    if (!*(RwTexture**)data)
    {
        *(RwTexture**)data = texture;
    }
    return texture;
}

static void* RWTX_Read(void*, unsigned int, void* indata, unsigned int insize,
                       unsigned int* outsize)
{
    RwTexture* tex = 0;

    if (insize)
    {
        RwMemory rwmem;
        rwmem.start = (unsigned char*)indata;
        rwmem.length = insize;
        RwStream* stream = RwStreamOpen(3, 1, &rwmem);
        if (stream)
        {
            if (!RwStreamFindChunk(stream, 0x16, 0, 0))
            {
                RwStreamClose(stream, 0);
            }
            else
            {
                RwTexDictionary* txd = RwTexDictionaryStreamRead(stream);
                RwStreamClose(stream, 0);
                if (txd)
                {
                    RwTexDictionaryForAllTextures(txd, TexCB, &tex);
                    if (!tex)
                    {
                        RwTexDictionaryDestroy(txd);
                    }
                    else
                    {
                        RwTexDictionaryRemoveTexture(tex);
                        RwTexDictionaryDestroy(txd);
                        tex->refCount++;
                        tex->filterAddressing = (tex->filterAddressing & ~0xFF) | 2;
                        *outsize = sizeof(RwTexture);
                        return tex;
                    }
                }
            }
        }
    }

    *outsize = insize;
    return 0;
}

static void TextureRW3_Unload(void* userdata, unsigned int)
{
    if (userdata)
    {
        ((RwTexture*)userdata)->refCount = 1;
        RwTextureDestroy((RwTexture*)userdata);
    }
}

static unsigned int dummyEffectCB(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*)
{
    return 0;
}

static unsigned int soundEffectCB(unsigned int cbenum, xAnimActiveEffect* acteffect,
                                  xAnimSingle* single, void* object)
{
    unsigned int sndhandle = 0;
    unsigned int vil_SID = 0;
    signed int vil_result;

    if (cbenum == 1)
    {
        _xEnt* ent_tmp = (_xEnt*)object;
        zAnimFxSound* snd = (zAnimFxSound*)(acteffect->Effect + 1);
        if (!ent_tmp)
        {
            vil_result = 0;
        }
        else if (ent_tmp->baseType == 2)
        {
            vil_result = zEntVillain_PlayAFXSound((st_zEntVillain*)ent_tmp, snd, &vil_SID);
        }
        else
        {
            vil_result = 0;
        }

        if (vil_result > 0)
        {
            sndhandle = vil_SID;
        }
        else
        {
            sndhandle = xSndPlay3D(snd->ID, 0.5f * snd->vol, snd->pitch, snd->priority,
                                   snd->flags, (_xEnt*)object, snd->radius);
        }
    }
    if (cbenum == 3)
    {
        xSndStop(acteffect->Handle);
    }
    return sndhandle;
}

static void* FindAssetCB(unsigned int ID, char*)
{
    unsigned int size;
    return xSTFindAsset(ID, &size);
}

// TODO (98.5%): only register coloring differs (indata/assetid/zaFile/zaRaw get
// different saved registers); declaration order has no effect.
static void* ATBL_Read(void*, unsigned int assetid, void* indata, unsigned int insize,
                       unsigned int* outsize)
{
    unsigned int i;
    unsigned int j;
    unsigned int debugNum = 0;
    unsigned int tmpsize;
    xAnimTable* table;
    xAnimState* astate;
    xAnimTransition* atran;
    xAnimAssetTable* zaTbl = (xAnimAssetTable*)indata;
    void** zaRaw = (void**)(zaTbl + 1);
    xAnimAssetFile* zaFile = (xAnimAssetFile*)(zaRaw + zaTbl->NumRaw);
    xAnimAssetState* zaState = (xAnimAssetState*)(zaFile + zaTbl->NumFiles);

    for (i = 0; i < zaTbl->NumRaw; i++)
    {
        zaRaw[i] = xSTFindAsset(*(unsigned int*)&zaRaw[i], &tmpsize);
    }

    for (i = 0; i < zaTbl->NumRaw; i++)
    {
        if (!zaRaw[i])
        {
            fn_80015EAC("WARNING:  Animation file not found on animtable id 0x%x\n", assetid);
            for (j = 0; j < zaTbl->NumRaw; j++)
            {
                if (zaRaw[j])
                {
                    zaRaw[i] = zaRaw[j];
                    break;
                }
            }
        }
    }

    for (i = 0; i < zaTbl->NumRaw; i++)
    {
        if (*(unsigned int*)zaRaw[i] == 'QSPM')
        {
            xMorphSeqSetup(zaRaw[i], FindAssetCB);
        }
    }

    for (i = 0; i < zaTbl->NumFiles; i++)
    {
        zaFile[i].RawData = (void**)((unsigned int)zaFile[i].RawData + (unsigned int)zaTbl);
        for (j = 0; j < (unsigned int)(zaFile[i].NumAnims[0] * zaFile[i].NumAnims[1]); j++)
        {
            zaFile[i].RawData[j] = zaRaw[(unsigned int)zaFile[i].RawData[j]];
        }
    }

    xAnimFile** fList = (xAnimFile**)zaFile;
    for (i = 0; i < zaTbl->NumFiles; i++)
    {
        fList[i] = xAnimFileNewBilinear(zaFile[i].RawData, "", zaFile[i].FileFlags, 0,
                                        zaFile[i].NumAnims[0], zaFile[i].NumAnims[1]);
        if (zaFile[i].TimeOffset >= 0.0f)
        {
            xAnimFileSetTime(fList[i], zaFile[i].Duration, zaFile[i].TimeOffset);
        }
        if (zaFile[i].Physics >= 0)
        {
            xAnimFileSetPhysics(fList[i], zaRaw[zaFile[i].Physics]);
        }
        else if (zaFile[i].StartPose >= 0)
        {
            xAnimFileSetPoses(fList[i], zaRaw[zaFile[i].StartPose], 0.0f,
                              zaRaw[zaFile[i].EndPose], 0.0f);
        }
    }

    if (zaTbl->ConstructFunc >= 22)
    {
        zaTbl->ConstructFunc = 3;
    }

    gxAnimUseGrowAlloc = 1;
    table = tableFuncList[zaTbl->ConstructFunc]();

    char tmpstr[32];
    for (i = 0; i < zaTbl->NumStates; i++)
    {
        astate = xAnimTableAddFileID(table, fList[zaState[i].FileIndex], zaState[i].StateID);
        if (!astate)
        {
            sprintf(tmpstr, "Debug%02d", debugNum++);
            astate = xAnimTableNewState(table, tmpstr, 0x20, 0x80000000, 1.0f, 0, 0, 0.0f, 0);
            atran = xAnimTableNewTransition(table, tmpstr, "", 0, 0, 0x10, 0, 0.0f, 0.0f, 0, 0,
                                            0.2f, 0);
            atran->Dest = table->StateList;
            xAnimTableAddFileID(table, fList[zaState[i].FileIndex], astate->ID);
        }
        astate->Speed = zaState[i].Speed;
    }

    xAnimFile* foundFile = 0;
    for (astate = table->StateList; astate; astate = astate->Next)
    {
        if (!foundFile && astate->Data)
        {
            foundFile = astate->Data;
        }
    }
    for (astate = table->StateList; astate; astate = astate->Next)
    {
        if (!astate->Data)
        {
            astate->Data = foundFile;
            astate->UserFlags |= 0x40000000;
        }
    }

    for (i = 0; i < zaTbl->NumStates; i++)
    {
        if (zaState[i].EffectCount)
        {
            xAnimState* state = xAnimTableGetStateID(table, zaState[i].StateID);
            xAnimAssetEffect* zaEffect =
                (xAnimAssetEffect*)((unsigned int)zaTbl + zaState[i].EffectOffset);
            if (state)
            {
                for (j = 0; j < zaState[i].EffectCount; j++)
                {
                    xAnimEffect* effect = xAnimStateNewEffect(
                        state, zaEffect->Flags, zaEffect->StartTime, zaEffect->EndTime,
                        effectFuncList[zaEffect->EffectType], zaEffect->UserDataSize);
                    memcpy(effect + 1, zaEffect + 1, zaEffect->UserDataSize);
                    zaEffect = (xAnimAssetEffect*)((unsigned int)zaEffect + zaEffect->UserDataSize) + 1;
                }
            }
        }
    }

    gxAnimUseGrowAlloc = 0;
    *outsize = sizeof(xAnimTable);
    return table;
}

static void Anim_Unload(void*, unsigned int)
{
}

static void MovePoint_Unload(void* userdata, unsigned int)
{
    xMovePointSplineDestroy((_xMovePoint*)userdata);
}

// TODO (98.1%): same parameter coloring difference as ATBL_Read.
static void* SndInfoRead(void*, unsigned int, void* indata, unsigned int insize,
                         unsigned int* outsize)
{
    void* dest = malloc(insize);
    if (!dest)
    {
        return dest;
    }
    memcpy(dest, indata, insize);
    fn_800D8570(dest);
    *outsize = insize;
    return dest;
}

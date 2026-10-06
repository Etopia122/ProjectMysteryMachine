// TODO: 99.9% (NonMatching). zSceneSwitch keeps passet in r7 instead of r3 around
// the byte swap and the camera calls, zSceneLoad has one register swap in the
// stats reset, zSceneRender in the sort-override loop's register allocation.
// zSceneVisitedDeadStripped and zSceneZeroDeadStripped must be stripped by the
// linker once this unit is Matching.
#define xCameraInit xCameraInit_header
#include "zScene.h"
#include "zGlobals.h"
#undef xCameraInit
#include "xserializer.h"
#include "xMath.h"
#include "xMath3.h"
#include "xBound.h"
#include "xCollide.h"
#include "xMemMgr.h"
#include "xString.h"

#ifndef NULL
#define NULL 0
#endif

#define eBaseTypePickup 4
#define SCENE_ID(s) ((((s)[0] & ~0x20) << 24) | ((s)[1] << 16) | ((s)[2] << 8) | (s)[3])

extern "C" {
void* memset(void* dst, int c, unsigned int n);
void* memcpy(void* dst, const void* src, unsigned int n);
char* strncpy(char* dst, const char* src, unsigned int n);
}

class _zEntPickup;
class _zEnv;
class _xRay3;
class _zParEmitter;

class _tagRain
{
public:
    signed int rain;
    float strength;
    _zParEmitter* rain_emitter;
    _zParEmitter* snow_emitter;
};

class _tagWind
{
public:
    float strength;
    float angle;
    _xVec3 dir;
};

class _tagClimate
{
public:
    _tagRain rain;
    _tagWind wind;
};

enum _tagSceneAreas
{
    eSceneArea_None
};

class zSceneLevel
{
public:
    char* desc;               // 0x0
    char name;                // 0x4
    signed int index;         // 0x8
    unsigned int totalClues;  // 0xC
};

class zSceneArea
{
public:
    char* desc;                    // 0x0
    _tagSceneAreas index;          // 0x4
    unsigned int totalSnacks;      // 0x8
    unsigned int constTotalSnacks; // 0xC
    unsigned int totalMTokens;     // 0x10
    unsigned int sceneIDs[4];      // 0x14
};

class zSceneObjectInstanceDesc
{
public:
    char* name;                // 0x0
    signed int baseType;       // 0x4
    unsigned int assetType;    // 0x8
    unsigned int sizeRuntime;  // 0xC
    unsigned int (*func)(zScene*, zSceneObjectInstanceDesc*, unsigned int); // 0x10
    void (*objectInitFunc)(void*, void*);                                   // 0x14
};

class IDBasePair
{
public:
    unsigned int id; // 0x0
    xBase* base;     // 0x4
};

// Render-order override for one entity in one scene (GameCube only).
// type 0: sort by distance to (a, b) in xz; 1: use a as the distance;
// 2: sort relative to the entity whose ID is stored in a, offset by b.
class zSceneSortEntry
{
public:
    unsigned int sceneID; // 0x0
    unsigned int id;      // 0x4
    signed int type;      // 0x8
    float a;              // 0xC
    float b;              // 0x10
};

union zSceneSortID
{
    unsigned int u;
    float f;
};

extern unsigned int gActiveHeap;
extern signed int gGameState;
extern signed int sCardCheckActive;

signed int zMenuRunning();
signed int zGameIsPaused();
void zEntPickup_UpdateFlyToInterface(_zEntPickup* p, unsigned int count, float dt);
unsigned int zVarEntryCB_eVareVarDefeatedMM();
char* xUtil_idtag2string(unsigned int id, signed int r);
char* xSTAssetName(unsigned int aid);

static signed int sMemDepthSceneStart = -1;
static signed int scobj_size = -1;
static signed int nidbps = -1;

static signed int roomCnt;
static signed int sRenderVar0;
static signed int sRenderVar1;
static signed int sRenderVar2;
static signed int sRenderVar3;
static signed int sRenderVar4;
static float sCardCheck0;
static float sCardCheck1;
static signed int sCardCheckEnabled;
static float sCardCheckSaved0;
static float sCardCheckSaved1;
_zEnv* gCurEnv;
unsigned int gTransitionSceneID;
static float sUpdateDt;
static unsigned int sPauseTimersHashed;
static signed char sPauseTimersHashedInit;
static IDBasePair* scobj_idbps;

_tagClimate gClimate;
static unsigned int sPauseTimerIDs[10];

class zEGenerator;
class _zEntButton;
class _zEntDestructObj;
class _zEntSimpleObj;
class zParEmitterAsset;
class p2EnvAsset;
class _zUIFont;

void zConditionalInit(void* b, void* asset);
void zCounterInit(void* b, void* asset);
void zCutsceneMgrInit(void* b, void* asset);
void zEGenerator_Init(zEGenerator* egen, p2EntAsset* asset);
void zEntButton_Init(_zEntButton* ent, p2EntAsset* asset);
void zEntDestructObj_Init(_zEntDestructObj* ent, p2EntAsset* asset);
void zEntHangable_Init(void* ent, void* asset);
void zEntPickupInit(void* ent, void* asset);
void zEntSimpleObj_Init(_zEntSimpleObj* ent, p2EntAsset* asset);
void zEntTriggerInit(void* ent, void* asset);
void zEnvInit(_zEnv* env, p2EnvAsset* asset);
void zFogInit(void* b, void* asset);
void zGroupInit(void* b, void* asset);
void zLightInit(void* b, void* asset);
void zParEmitterInit(xBase* b, zParEmitterAsset* asset);
void zParSysInit(void* b, void* asset);
void zPendulum_Init(void* ent, void* asset);
void zPlatform_Init(void* ent, void* asset);
void zPortalInit(void* b, void* asset);
void zSFXInit(void* b, void* asset);
void zScriptInit(void* b, void* asset);
void zTimerInit(void* b, void* asset);
void zUIFont_Init(_zUIFont* ent, p2EntAsset* asset);
void zUI_Init(void* ent, void* asset);

static unsigned int zSceneInitFunc_Dispatcher(zScene* s, zSceneObjectInstanceDesc* desc, unsigned int base_idx);
static unsigned int zSceneInitFunc_LobMaster(zScene* s, zSceneObjectInstanceDesc* desc, unsigned int base_idx);
static unsigned int zSceneInitFunc_Volume(zScene* s, zSceneObjectInstanceDesc* desc, unsigned int base_idx);
static unsigned int zSceneInitFunc_Gust(zScene* s, zSceneObjectInstanceDesc* desc, unsigned int base_idx);
static unsigned int zSceneInitFunc_Surface(zScene* s, zSceneObjectInstanceDesc* desc, unsigned int base_idx);
static unsigned int zSceneInitFunc_Camera(zScene* s, zSceneObjectInstanceDesc* desc, unsigned int base_idx);
static unsigned int zSceneInitFunc_Player(zScene* s, zSceneObjectInstanceDesc* desc, unsigned int base_idx);
static unsigned int zSceneInitFunc_Villain(zScene* s, zSceneObjectInstanceDesc* desc, unsigned int base_idx);
static unsigned int zSceneInitFunc_MovePoint(zScene* s, zSceneObjectInstanceDesc* desc, unsigned int base_idx);
static unsigned int zSceneInitFunc_Default(zScene* s, zSceneObjectInstanceDesc* desc, unsigned int base_idx);
static unsigned int zSceneInitFunc_DefaultEnt(zScene* s, zSceneObjectInstanceDesc* desc, unsigned int base_idx);

typedef void (*zSceneObjInitFunc)(void*, void*);

static zSceneLevel sLevelTable[13] = {
    { "Fishing Village", 'F', 0, 50 },
    { "Lighthouse", 'L', 1, 50 },
    { "Shipwreck", 'W', 2, 50 },
    { "Hedge Maze", 'E', 3, 50 },
    { "Cliffs & Caves", 'C', 4, 50 },
    { "Cemetery", 'G', 5, 50 },
    { "Secret Passages", 'P', 6, 50 },
    { "Cellars", 'B', 7, 50 },
    { "Super Secret Lab", 'S', 8, 50 },
    { "1st Floor", 'I', 9, 50 },
    { "Attic & Balconies", 'R', 10, 50 },
    { "Tower", 'O', 11, 50 },
    { "Hub", 'H', 12, 50 },
};

static zSceneArea sAreaTable[30] = {
    { "", (_tagSceneAreas)0, 999, 999, 999, { 0, 0, 0, 0 } },
    { "The Mystery Machine", (_tagSceneAreas)1, 118, 118, 1, { 'H001', 'H002', 'H003', 0 } },
    { "On Edge in the Hedge!", (_tagSceneAreas)2, 371, 371, 2, { 'E001', 'E002', 'E003', 'E004' } },
    { "It's a Mean Greenhouse, Scooby!", (_tagSceneAreas)3, 143, 143, 0, { 'E005', 'E006', 0, 0 } },
    { "Chills & Spills on Haunted Hill!", (_tagSceneAreas)4, 309, 309, 0, { 'E007', 'E008', 'E009', 0 } },
    { "Scared Stiff at Skull Cliff!", (_tagSceneAreas)5, 394, 394, 0, { 'C001', 'C002', 'C003', 'C004' } },
    { "Misbehavin'? Cause a Cave In", (_tagSceneAreas)6, 257, 257, 1, { 'C005', 'C006', 'C007', 0 } },
    { "A Tight Spot for a Grave Plot!", (_tagSceneAreas)7, 116, 116, 1, { 'G001', 'G002', 0, 0 } },
    { "A One Way Trip to the Crypt!", (_tagSceneAreas)8, 224, 224, 1, { 'G003', 'G004', 'G005', 'G006' } },
    { "Gloom & Doom Down in the Tomb!", (_tagSceneAreas)9, 264, 264, 1, { 'G007', 'G008', 'G009', 0 } },
    { "Shock on the Dock!", (_tagSceneAreas)10, 154, 154, 0, { 'F001', 'F003', 0, 0 } },
    { "Fishy Clues for Scooby-Doo?", (_tagSceneAreas)11, 386, 386, 2, { 'F004', 'F005', 'F006', 'F007' } },
    { "Fear on the Pier!", (_tagSceneAreas)12, 202, 202, 0, { 'F008', 'F009', 'F010', 0 } },
    { "Coast for Some Ghosts!", (_tagSceneAreas)13, 373, 373, 2, { 'L011', 'L013', 'L014', 0 } },
    { "Going Down Witch Way?", (_tagSceneAreas)14, 121, 121, 0, { 'L015', 0, 0, 0 } },
    { "Lighthouse Fright House!", (_tagSceneAreas)15, 269, 269, 0, { 'L017', 'L018', 'L019', 0 } },
    { "Wreck on the Deck!", (_tagSceneAreas)16, 268, 268, 1, { 'W020', 'W021', 'W022', 0 } },
    { "Aghast by the Mast!", (_tagSceneAreas)17, 375, 375, 1, { 'W023', 'W025', 'W026', 0 } },
    { "Shiver Your Timbers, Scooby!", (_tagSceneAreas)18, 117, 117, 0, { 'W027', 'W028', 0, 0 } },
    { "Clamor in the Manor!", (_tagSceneAreas)19, 220, 220, 1, { 'I001', 'I020', 'I021', 'I003' } },
    { "Mind Your Manors!", (_tagSceneAreas)20, 228, 228, 1, { 'I004', 'I005', 'I006', 0 } },
    { "All Scares Upstairs!", (_tagSceneAreas)21, 390, 390, 1, { 'R001', 'R020', 'R021', 'R003' } },
    { "Don't Look Down, Scooby-Doo!", (_tagSceneAreas)22, 163, 163, 0, { 'R004', 'R005', 'R006', 0 } },
    { "Panic in the Attic!", (_tagSceneAreas)23, 342, 342, 2, { 'O001', 'O002', 'O003', 'O004' } },
    { "A Dark and Stormy Knight!", (_tagSceneAreas)24, 240, 240, 0, { 'O005', 'O006', 'O008', 0 } },
    { "Who's Yella' in the Cellar?", (_tagSceneAreas)25, 205, 205, 1, { 'B001', 'B002', 'B003', 'B004' } },
    { "Creepy Crawlies in the Hallways", (_tagSceneAreas)26, 165, 165, 0, { 'P001', 'P002', 'P003', 0 } },
    { "Gusts Abound Underground", (_tagSceneAreas)27, 110, 110, 0, { 'P004', 'P005', 0, 0 } },
    { "Little Lab of Horrors", (_tagSceneAreas)28, 160, 160, 2, { 'S001', 'S002', 'S003', 0 } },
    { "Mastermind Unmasked!", (_tagSceneAreas)29, 64, 64, 0, { 'S004', 'S005', 0, 0 } },
};

static zSceneObjectInstanceDesc sInitTable[34] = {
    { "Trig", 1, 'TRIG', 0xEC, zSceneInitFunc_DefaultEnt, zEntTriggerInit },
    { "Move Point", 13, 'MVPT', 0x34, zSceneInitFunc_MovePoint, NULL },
    { "Pickup", 4, 'PKUP', 0x168, zSceneInitFunc_DefaultEnt, zEntPickupInit },
    { "Platform", 6, 'PLAT', 0x178, zSceneInitFunc_DefaultEnt, zPlatform_Init },
    { "Pendulum", 18, 'PEND', 0x164, zSceneInitFunc_DefaultEnt, zPendulum_Init },
    { "Hanger", 23, 'HANG', 0x144, zSceneInitFunc_DefaultEnt, zEntHangable_Init },
    { "Simple", 11, 'SIMP', 0xF4, zSceneInitFunc_DefaultEnt, (zSceneObjInitFunc)zEntSimpleObj_Init },
    { "DestructObj", 27, 'DSTR', 0x100, zSceneInitFunc_DefaultEnt, (zSceneObjInitFunc)zEntDestructObj_Init },
    { "Villain", 2, 'NPC ', 0x0, zSceneInitFunc_Villain, NULL },
    { "Player", 3, 'PLYR', 0xEC, zSceneInitFunc_Player, NULL },
    { "Timer", 14, 'TIMR', 0x1C, zSceneInitFunc_Default, zTimerInit },
    { "Counter", 22, 'CNTR', 0x18, zSceneInitFunc_Default, zCounterInit },
    { "SFX", 19, 'SFX ', 0x18, zSceneInitFunc_Default, zSFXInit },
    { "Group", 17, 'GRUP', 0x14, zSceneInitFunc_Default, zGroupInit },
    { "Portal", 16, 'PORT', 0x14, zSceneInitFunc_Default, zPortalInit },
    { "Camera", 7, 'CAM ', 0xA0, zSceneInitFunc_Camera, NULL },
    { "Button", 24, 'BUTN', 0x170, zSceneInitFunc_DefaultEnt, (zSceneObjInitFunc)zEntButton_Init },
    { "Surface", 26, 'SURF', 0x28, zSceneInitFunc_Surface, NULL },
    { "Gust", 28, 'GUST', 0x24, zSceneInitFunc_Gust, NULL },
    { "Volume", 29, 'VOLU', 0x14, zSceneInitFunc_Volume, NULL },
    { "Conditional", 31, 'COND', 0x14, zSceneInitFunc_Default, zConditionalInit },
    { "Lob Master", 35, 'LOBM', 0x0, zSceneInitFunc_LobMaster, NULL },
    { "Env", 5, 'ENV ', 0x14, zSceneInitFunc_Default, (zSceneObjInitFunc)zEnvInit },
    { "Dispatcher", 30, 'DPAT', 0x0, zSceneInitFunc_Dispatcher, NULL },
    { "UI", 32, 'UI  ', 0xF8, zSceneInitFunc_DefaultEnt, zUI_Init },
    { "UI Font", 33, 'UIFT', 0x100, zSceneInitFunc_Default, (zSceneObjInitFunc)zUIFont_Init },
    { "Fog", 36, 'FOG ', 0x14, zSceneInitFunc_Default, zFogInit },
    { "Light", 37, 'LITE', 0x64, zSceneInitFunc_Default, zLightInit },
    { "ParticleSystem", 39, 'PARS', 0x28, zSceneInitFunc_Default, zParSysInit },
    { "ParticleEmitter", 38, 'PARE', 0x50, zSceneInitFunc_Default, (zSceneObjInitFunc)zParEmitterInit },
    { "CutSceneMgr", 40, 'CSNM', 0x24, zSceneInitFunc_Default, zCutsceneMgrInit },
    { "EGenerator", 41, 'EGEN', 0x11C, zSceneInitFunc_DefaultEnt, (zSceneObjInitFunc)zEGenerator_Init },
    { "Script", 42, 'SCRP', 0x1C, zSceneInitFunc_Default, zScriptInit },
    { NULL, 0, 0, 0, NULL, NULL },
};

// Type 2 entries keep the ID of the entity to sort against in the float slot.
static const unsigned int sSortConst0 = 0xFFF2B7A3;
static const unsigned int sSortConst1 = 0xFFF2B7A4;
static const unsigned int sSortConst2 = 0xFFF2B7AA;

static zSceneSortEntry sSortTable[66] = {
    { SCENE_ID("b004"), 0x3A93B8D5, 1, 1000.0f, 0.0f },
    { SCENE_ID("f001"), 0x2AA813E6, 1, 10000.0f, 0.0f },
    { SCENE_ID("f004"), 0x22E5FF9E, 1, 10000.0f, 0.0f },
    { SCENE_ID("f004"), 0xE16CD658, 1, 10000.0f, 0.0f },
    { SCENE_ID("f007"), 0x05D17E38, 1, 100000.0f, 0.0f },
    { SCENE_ID("f007"), 0x1FB2A715, 1, 10000.0f, 0.0f },
    { SCENE_ID("f009"), 0x29133A86, 1, 100000.0f, 0.10000000149011612f },
    { SCENE_ID("f010"), 0x76D4796D, 2, -5.730950754535608e+36f, -0.10000000149011612f },
    { SCENE_ID("i001"), 0x676E3710, 1, 18.100000381469727f, 0.0f },
    { SCENE_ID("i001"), 0xFFF2B7A4, 1, 18.0f, 0.0f },
    { SCENE_ID("i001"), 0x2163A5AD, 1, 10000.0f, 0.0f },
    { SCENE_ID("i001"), 0xDC572112, 1, 9000.0f, 0.0f },
    { SCENE_ID("i003"), 0xA207A42F, 1, 5.0f, 0.0f },
    { SCENE_ID("i003"), 0x676E3710, 1, 7.0f, 0.0f },
    { SCENE_ID("i003"), 0xFFF2B7A4, 1, 10.0f, 0.0f },
    { SCENE_ID("i003"), 0xFFF2B7A5, 1, 5.0f, 0.0f },
    { SCENE_ID("i004"), 0x213E3E5C, 2, 14925528064.0f, 0.10000000149011612f },
    { SCENE_ID("i004"), 0x0016EBA0, 2, 14925528064.0f, 0.20000000298023224f },
    { SCENE_ID("i004"), 0xD660169D, 2, 14925528064.0f, 0.30000001192092896f },
    { SCENE_ID("i006"), 0x90E56560, 1, 10000.0f, 0.0f },
    { SCENE_ID("i006"), 0x9D7EF8C5, 1, 9999.0f, 0.0f },
    { SCENE_ID("i006"), 0x9DA14661, 2, -1.3661381146136485e-36f, 0.10000000149011612f },
    { SCENE_ID("i006"), 0x1EA6A37D, 2, *(float*)&sSortConst0, 0.10000000149011612f },
    { SCENE_ID("i020"), 0x00794266, 1, 10000.0f, 0.0f },
    { SCENE_ID("i020"), 0x0BBA9313, 1, 10000.0f, 0.0f },
    { SCENE_ID("i020"), 0x1FF4FC52, 2, *(float*)&sSortConst0, 0.10000000149011612f },
    { SCENE_ID("i020"), 0x8FD9188B, 1, 10000.0f, 0.0f },
    { SCENE_ID("i020"), 0x8FD9188D, 1, 10000.0f, 0.0f },
    { SCENE_ID("i021"), 0x9C17992C, 2, -1.4451877194954221e-10f, -0.10000000149011612f },
    { SCENE_ID("i021"), 0x0B0E0EAC, 1, 10000.0f, 0.0f },
    { SCENE_ID("i021"), 0x889C3413, 2, *(float*)&sSortConst0, 0.10000000149011612f },
    { SCENE_ID("l017"), 0xF933F817, 2, 3.753733324174391e-08f, -0.10000000149011612f },
    { SCENE_ID("l017"), 0xF933F818, 2, 7.52681181270809e-11f, -0.10000000149011612f },
    { SCENE_ID("l017"), 0xF933F819, 2, 3.2181434996898144e-15f, -0.10000000149011612f },
    { SCENE_ID("l017"), 0xF933F81A, 2, 0.00011711304250638932f, -0.10000000149011612f },
    { SCENE_ID("l017"), 0xF933F81B, 2, 4302963.5f, -0.10000000149011612f },
    { SCENE_ID("l017"), 0x105AB6CA, 1, 10000.0f, 0.0f },
    { SCENE_ID("o002"), 0x76D4796D, 1, 0.0f, 0.0f },
    { SCENE_ID("o003"), 0x5E6B898F, 2, -7.044735350625331e-10f, 0.10000000149011612f },
    { SCENE_ID("o003"), 0x5E6B8990, 2, -7.044735350625331e-10f, 0.10000000149011612f },
    { SCENE_ID("o003"), 0x5E6B8991, 2, -7.044735350625331e-10f, 0.10000000149011612f },
    { SCENE_ID("o003"), 0x5E6B8994, 1, 10000.0f, 0.0f },
    { SCENE_ID("o003"), 0x6C62EBA8, 2, -2.586839187301848e-09f, 0.10000000149011612f },
    { SCENE_ID("o003"), 0xB10F768D, 1, 10000.0f, 0.0f },
    { SCENE_ID("o003"), 0x5E6B8995, 2, -2.586839187301848e-09f, 0.20000000298023224f },
    { SCENE_ID("o003"), 0x5E6B8996, 2, 4.243065124488741e+18f, 0.0f },
    { SCENE_ID("o003"), 0x5E6B8997, 2, -9.768009734401062e-10f, 0.10000000149011612f },
    { SCENE_ID("o003"), 0x5107645D, 2, -9.768009734401062e-10f, 0.10000000149011612f },
    { SCENE_ID("o003"), 0x6BD9B53C, 2, -9.768009734401062e-10f, 0.10000000149011612f },
    { SCENE_ID("o004"), 0x4B6C4E03, 2, *(float*)&sSortConst1, 0.10000000149011612f },
    { SCENE_ID("o004"), 0x4B6C4E04, 2, *(float*)&sSortConst1, 0.10000000149011612f },
    { SCENE_ID("o004"), 0x4BF5846F, 2, *(float*)&sSortConst2, -0.10000000149011612f },
    { SCENE_ID("o004"), 0xF933F89E, 1, 32180446.0f, -0.10000000149011612f },
    { SCENE_ID("o004"), 0x1EA6A37A, 1, 20000.0f, 0.0f },
    { SCENE_ID("r001"), 0x0BBA9312, 0, 500.0f, 500.0f },
    { SCENE_ID("r001"), 0x8FD918AC, 1, 10000.0f, 0.0f },
    { SCENE_ID("r001"), 0xE50A53DD, 1, 2.0f, 0.0f },
    { SCENE_ID("r001"), 0xED663E44, 2, -1.431402426951332e-35f, 0.10000000149011612f },
    { SCENE_ID("r003"), 0x8598367B, 1, 5.0f, 0.0f },
    { SCENE_ID("r020"), 0x3E643406, 1, 10000.0f, 0.0f },
    { SCENE_ID("r021"), 0x303C778E, 0, 500.0f, 500.0f },
    { SCENE_ID("r021"), 0x0BBA9312, 0, 500.0f, 500.0f },
    { SCENE_ID("r021"), 0x0BBA9311, 0, 500.0f, 500.0f },
    { SCENE_ID("w027"), 0x7C5CE847, 0, 500.0f, 500.0f },
    { SCENE_ID("w027"), 0xAD92D5FD, 0, 500.0f, 500.0f },
    { SCENE_ID("w027"), 0x7C5CE7C4, 0, 500.0f, 500.0f },
};

static inline signed int zSceneGetLevelIndex(char c)
{
    for (signed int i = 0; i < 13; i++)
    {
        if (c == sLevelTable[i].name)
        {
            return sLevelTable[i].index;
        }
    }
    return -1;
}

static inline xBase* zSceneObjHashtableGet(unsigned int id)
{
    signed int k = id & (scobj_size - 1);
    for (signed int i = 0; i < scobj_size; i++)
    {
        IDBasePair* p = &scobj_idbps[k];
        if (p->id == id)
        {
            return p->base;
        }
        if (p->id == 0)
        {
            return NULL;
        }
        k++;
        if (k == scobj_size)
        {
            k = 0;
        }
    }
    return NULL;
}

unsigned int zSceneGetArea(unsigned int sceneID);
// ---- external declarations ----

char* zSceneGetName(xBase* b);
char* zSceneGetName(unsigned int gameID);
signed int zSceneSetup_serialTraverseCB(unsigned int clientID, xSerial* xser);
signed int zSceneCalcProgress_serialTraverseCB(unsigned int clientID, xSerial* xser);

#include "iDraw.h"

#include "iEnv.h"

#include "iSnd.h"

#include "xCamera.h"

#include "xFFX.h"

#include "xMovePoint.h"

#include "xQuickCull.h"

#include "xstransvc.h"

#include "xSurface.h"

#include "xEnv.h"

#include "zConditional.h"

#include "zEntProjectile.h"

#include "zCounter.h"

#include "zEnt.h"

#include "zTimer.h"

#include "zUI.h"

#include "zVar.h"

class p2EnvAsset : public p2BaseAsset
{
public:
    unsigned int bspAssetID;         // 0x8
    unsigned int startCameraAssetID; // 0xC
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

class zParSys : public xBase
{
public:
    char pad[0x18];
};

class _zEnv : public xBase
{
public:
    p2EnvAsset* easset; // 0x10
};

class iTRCDisk
{
public:
    static int CheckDVDAndResetState();
};

class _zPlatform;

class _zPendulum;

class _zMovePoint;

class _zEntHangable;

class _zFog;

class _zLight;

class _zGroup;

class zSFX;

class _zGust;

class _zScript;

class _zVolume;

class zEntTrigger;

class _zCutsceneMgr;

class st_ZDISPATCH_DATA;

class st_LOBMASTER_DATA;

class st_zEntVillain;

class p2MovePointAsset;

class zProjectileTypeAsset;

class xCamAsset;

class RpWorld;

extern "C" {
signed int sprintf(char* s, const char* format, ...);
char* strcat(char* dst, const char* src);
char* strcpy(char* dst, const char* src);
_zVolume* fn_80005AE8(unsigned short idx);
void fn_80005B08();
void fn_80005B58();
void fn_80005C34(xBase* b, xSerial* s);
void fn_80005C54(xBase* b, xSerial* s);
void fn_800162D8();
void fn_800A8F88(xBase* b, zScene* s);
}
st_zEntVillain* zEntVillain_NPCInst(unsigned int npcidx);
st_zEntVillain* zEntVillain_MemPool(unsigned int npccnt);
void zEntShaggy_SceneReset();

extern signed int lbl_8025E868;

extern _xScene* g_xSceneCur;

_tagxCam* xCameraInit(_tagxCamFrame* cam, unsigned int total);

extern signed int lbl_8025EEC8;

extern _tagClimate gClimate;

extern _zEnv* gCurEnv;

void xClimateInit(_tagClimate* climate);

void xClimateInitAsset(_tagClimate* climate, p2EnvAsset* asset);

void xCutscene_Init(void* toc);

_xBox* xEntGetAllEntsBox();

void xEntMotionDebugExit();

void xEntMotionDebugInit(unsigned short num);

void xEntSceneExit();

void xEntSceneInit();

void xModel_SceneExit(RpWorld* world);

void xPadDestroyRumbleChain(signed int idx);

void xPadRumbleEnable(signed int idx, signed int enable);

void xPartitionReset();

float xSTLoadStep(unsigned int);

signed int xSTPreLoadScene(unsigned int sid, void* userdata);

signed int xSTQueueSceneAssets(unsigned int sid);

signed int xSTSwitchScene(unsigned int sid, void* userdata, signed int (*progmon)(void*, float));

signed int xSTUnLoadScene(unsigned int sid);

void xSceneAddEnt(_xScene* sc, _xEnt* ent);

void xSceneInit(_xScene* sc, unsigned short num_trigs, unsigned short num_stats,
                unsigned short num_dyns, unsigned short num_npcs);

void xSceneLoad(_xScene* sc, xSerial* s);

void xSceneSave(_xScene* sc, xSerial* s);

void xSceneSetup(_xScene* sc);

void xScrFxFade(iColor_tag* base, iColor_tag* dest, float seconds, void (*callback)(),
                signed int hold, signed int arg5);

void xScrFxLetterbox(signed int enable);

void xSndParentDied(unsigned int pid);

void xSndStopAll();

void xSndUpdate(float dt);

void enableWarpPoint(unsigned int idx);

void zActionLineInit();

void zAnimListExit();

void zAnimListInit();

void zCameraInit(xBase* b, xCamAsset* asset);

void zCameraLoad(_tagxCam* ent, xSerial* s);

void zCameraSave(_tagxCam* ent, xSerial* s);

void zCutsceneMgrLoad(_zCutsceneMgr* mgr, xSerial* s);

void zCutsceneMgrSave(_zCutsceneMgr* mgr, xSerial* s);

void zDispatcher_Init(st_ZDISPATCH_DATA* dspdata, p2BaseAsset* bass);

void zDispatcher_InitDep(st_ZDISPATCH_DATA* dspdata, zScene* scene);

void zDispatcher_Load(st_ZDISPATCH_DATA* dspdata, xSerial* s);

void zDispatcher_Save(st_ZDISPATCH_DATA* dspdata, xSerial* s);

st_ZDISPATCH_DATA* zDispatcher_getInst(st_ZDISPATCH_DATA* pool, signed int idx);

st_ZDISPATCH_DATA* zDispatcher_memPool(signed int cnt);

void zDispatcher_sceneFinish();

void zDispatcher_scenePrepare();

void zEGenerator_Load(zEGenerator* ent, xSerial* s);

void zEGenerator_Reset(zEGenerator* egen, _xScene* sc);

void zEGenerator_Save(zEGenerator* ent, xSerial* s);

void zEGenerator_Setup(zEGenerator* egen, _xScene* sc);

void zEntBubble_SceneInit(zScene* s);

void zEntButton_Load(_zEntButton* ent, xSerial* s);

void zEntButton_Reset(_zEntButton* ent, _xScene* sc);

void zEntButton_Save(_zEntButton* ent, xSerial* s);

void zEntButton_Setup(_zEntButton* ent, _xScene* sc);

void zEntDestructObj_FindFX();

void zEntDestructObj_Load(_zEntDestructObj* ent, xSerial* s);

void zEntDestructObj_Save(_zEntDestructObj* ent, xSerial* s);

void zEntEventAll(xBase* from, unsigned int fromEvent, unsigned int toEvent, float* toParam);

void zEntHangable_Load(_zEntHangable* ent, xSerial* s);

void zEntHangable_Reset(_zEntHangable* ent);

void zEntHangable_Save(_zEntHangable* ent, xSerial* s);

void zEntHangable_SetupFX();

void zEntPickup_Load(_zEntPickup* ent, xSerial* s);

void zEntPickup_Reset(_zEntPickup* ent);

void zEntPickup_Save(_zEntPickup* ent, xSerial* s);

void zEntPickup_Setup();

void zEntPickup_Setup(_zEntPickup* p);

unsigned int zEntPickup_WarpIsOpen(unsigned int idx);

void zEntPlayerExit(_xEnt* ent);

void zEntPlayerReset(_xEnt* ent);

void zEntPlayer_LoadCheckPoint();

void zEntProjectile_prepSystem(signed int cnt, zScene* scn);

void zEntProjectile_registerType(zProjectileTypeAsset* asset);

void zEntProjectile_sceneFinish();

void zEntProjectile_scenePrepare();

void zEntProjectile_sceneReset();

void zEntSimpleObj_Load(_zEntSimpleObj* ent, xSerial* s);

void zEntSimpleObj_Save(_zEntSimpleObj* ent, xSerial* s);

void zEntSimpleObj_Setup(_zEntSimpleObj* ent);

void zEntTriggerLoad(zEntTrigger* ent, xSerial* s);

void zEntTriggerSave(zEntTrigger* ent, xSerial* s);

void zEntVillain_Load(st_zEntVillain* villain, xSerial* s);
void zEntVillain_Save(st_zEntVillain* villain, xSerial* s);
void zEntPlayer_Load(_xEnt* ent, xSerial* s);

void zEntVillain_Init(st_zEntVillain* villain, p2EntAsset* asset);

void zEntVillain_sceneFinish();

void zEntVillain_scenePrepare();

void zEnvLoad(_zEnv* ent, xSerial* s);

void zEnvSave(_zEnv* ent, xSerial* s);

void zEnvSetup(_zEnv* env);

void zFX_SceneExit(RpWorld* world);

void zFeetGetIDs();

void zFogLoad(_zFog* ent, xSerial* s);

void zFogSave(_zFog* ent, xSerial* s);

void zGameScreenTransitionBegin();

void zGameScreenTransitionEnd();

void zGameScreenTransitionUpdate(float percentComplete, char* msg);

void zGameScreenTransitionUpdate(float percentComplete, char* msg, unsigned char* rgba);

void zGroupLoad(_zGroup* ent, xSerial* s);

void zGroupSave(_zGroup* ent, xSerial* s);

_zGust* zGustGetGust(unsigned short n);

void zGustInit();

void zGustLoad(_zGust* ent, xSerial* s);

void zGustSave(_zGust* ent, xSerial* s);

void zGustSetup();

void zHologramInit();

void zLightDestroyAll();

void zLightLoad(_zLight* ent, xSerial* s);

void zLightResolveLinks();

void zLightSave(_zLight* ent, xSerial* s);

void zLightSetVolume(_zVolume* vol);

void zLightningInit();

void zLobMaster_Init(st_LOBMASTER_DATA* lobdata, p2BaseAsset* bass);

void zLobMaster_InitDep(st_LOBMASTER_DATA* lobdata, zScene* scene);

st_LOBMASTER_DATA* zLobMaster_getInst(st_LOBMASTER_DATA* pool, signed int idx);

st_LOBMASTER_DATA* zLobMaster_memPool(signed int cnt);

void zLobMaster_sceneFinish();

void zLobMaster_scenePrepare();

void zMovePointInit(_zMovePoint* m, p2MovePointAsset* asset);

void zMovePointLoad(_zMovePoint* ent, xSerial* s);

void zMovePointSave(_zMovePoint* ent, xSerial* s);

void zMovePointSetup(_zMovePoint* mvpt, zScene* scn);

_zMovePoint* zMovePoint_GetInst(signed int n);

_zMovePoint* zMovePoint_GetMemPool(signed int cnt);

void zMusicKill();

void zMusicNotify(signed int situation);

void zParCmdFindClipVolumes();

void zParEmitterSetup(_zParEmitter* pe);

void zParSysExit(zParSys* ps);

void zParSysSetup(zParSys* ps);

void zPendulum_Load(_zPendulum* ent, xSerial* s);

void zPendulum_Reset(_zPendulum* pend, _xScene* sc);

void zPendulum_Save(_zPendulum* ent, xSerial* s);

void zPendulum_Setup(_zPendulum* pend, _xScene* sc);

void zPlatform_Load(_zPlatform* ent, xSerial* s);

void zPlatform_Reset(_zPlatform* plat, _xScene* sc);

void zPlatform_Save(_zPlatform* ent, xSerial* s);

void zPlatform_Setup(_zPlatform* plat, _xScene* sc);

void zPortalLoad(_zPortal* ent, xSerial* s);

void zPortalSave(_zPortal* ent, xSerial* s);

void zRenderStateInit();

void zSFXLoad(zSFX* ent, xSerial* s);

void zSFXSave(zSFX* ent, xSerial* s);

void zSceneSave(zScene* ent, xSerial* s);

void zSceneLoad(zScene* ent, xSerial* s);

void zScriptLoad(_zScript* ent, xSerial* s);

void zScriptSave(_zScript* ent, xSerial* s);

void zSurfaceExit();

void zSurfaceInit();

void zSurfaceLoad(xSurface* ent, xSerial* s);

void zSurfaceSave(xSurface* ent, xSerial* s);

void zSurfaceSetup(xSurface* s);

void zUIFont_Load(_zUIFont* ent, xSerial* s);

void zUIFont_Save(_zUIFont* ent, xSerial* s);

static inline void zSceneSet(xBase* b, unsigned int idx)
{
    globals.sceneCur->base[idx] = b;
}
void zHud_sceneReset();

static inline void zSceneObjHashtableAdd(unsigned int id, xBase* base)
{
    signed int k = id & (scobj_size - 1);
    for (signed int i = 0; i < scobj_size; i++)
    {
        IDBasePair* p = &scobj_idbps[k];
        if (p->id == 0)
        {
            p->id = id;
            p->base = base;
            nidbps++;
            return;
        }
        k++;
        if (k == scobj_size)
        {
            k = 0;
        }
    }
}

static inline void zSceneObjHashtableExit()
{
    scobj_idbps = NULL;
    scobj_size = -1;
    nidbps = -1;
}

// Portal scene IDs may be stored byte-swapped; a valid ID ends in a digit.
static inline void HackSwapIt(char* buf, signed int size)
{
    char* end = &buf[size - 1];
    if (*end < '0' || *end > '9')
    {
        char* start = buf;
        for (signed int i = 0; i < size / 2; i++)
        {
            char t = *start;
            *start = *end;
            *end = t;
            start++;
            end--;
        }
    }
}


class xFontBox;

class _tagFeetSkidMark
{
public:
    char pad[0x24];
};

extern _tagFeetSkidMark sPlayerSkidMark[5];

extern "C" {
void fn_800167F0();
void fn_80009CCC(_xVec3* pts, signed int count, unsigned int texture, float w0, float w1);
void fn_8000A138(iEnv* env);
void fn_80032D28();
}
void zEntVillain_BonusRender();

enum _SDRenderState
{
    SDRS_Unknown = 0,
    SDRS_Default = 1,
    SDRS_OpaqueModels = 2,
    SDRS_AlphaModels = 3,
    SDRS_Bubble = 4,
    SDRS_Projectile = 5,
    SDRS_Font = 6,
    SDRS_HUD = 7,
    SDRS_Particles = 8,
    SDRS_Lightning = 9,
    SDRS_SkidMark = 10,
    SDRS_Tide = 11,
    SDRS_SkyBack = 12,
    SDRS_Total = -1
};

void zRenderState(_SDRenderState next);

void zLightAddLocalEnv();

void zLightRemoveLocalEnv();

void zEnvRender(_xEnv* env);

void xCameraEnd(_tagxCamFrame* cam, float seconds, signed int update_scrn_fx);

void xCameraBegin(_tagxCamFrame* cam, signed int clear);

void iEnvRenderFX(iEnv* env);

_xVec3* xBoundCenter(_xBound* bound);

void zEntPickup_RenderList(_zEntPickup* plist, unsigned int pcount);

unsigned int xShadowReceiveShadowSetup(_xEnt* ent);

void xShadowReceiveShadow(_xEnt* ent);

void zEntProjectile_poolRender(en_ZPRJ_CATEGORY cat);

void zLightningRender();

void zActionLineRender();

void zFeetRenderSkidMark(_tagFeetSkidMark* skid);

void zEntBubble_Render();

void zParSysRender(xBase* b);

void iShadowRender();

signed int zGameModeGet();

signed int zGameStateGet();

void zHud_Render();

void zUIRenderAll();

xFontBox* xFontBoxCreate(float x, float y, float w, float h, unsigned int fontID, iColor_tag color);

void xFontBoxDestroy(xFontBox* box);

void xFontBoxRender(xFontBox* box);

void xFontBoxSetText(xFontBox* box, char* text);

void xFontBoxSetDrawSize(xFontBox* box, float w, float h);

void xFontBoxSetDrawSpacing(xFontBox* box, float x, float y);

void xFontBoxSetBackdrop(xFontBox* box, iColor_tag color);

void xFontBoxSetInset(xFontBox* box, float l, float t, float r, float b);

void xFontBoxSetJustification(xFontBox* box, signed int just);

unsigned int zMenuGetBadCard();

void xFontRender();

void DrawScreenSizeRectangle();

static inline void zSceneRenderShadowReceivers()
{
    signed int i;
    _xEnt* ent2;
    _xEnt* ent;
    signed int j;
    zScene* s;
    s = globals.sceneCur;
    for (i = 0; i < s->num_stats; i++)
    {
        ent = s->stats[i];
        if ((ent->baseType == 6 || ent->baseType == 24 || ent->baseType == 27 ||
             ent->baseType == 11) &&
            xShadowReceiveShadowSetup(ent))
        {
            xShadowReceiveShadow(ent);
        }
    }
    for (j = 0; j < s->num_dyns; j++)
    {
        ent2 = s->dyns[j];
        if ((ent2->baseType == 6 || ent2->baseType == 24 || ent2->baseType == 27 ||
             ent2->baseType == 11) &&
            xShadowReceiveShadowSetup(ent2))
        {
            xShadowReceiveShadow(ent2);
        }
    }
}

static inline void zSceneRenderLevelFX()
{
    unsigned int sceneID = globals.sceneCur->sceneID;
    if (sceneID == SCENE_ID("b003"))
    {
        _xVec3 pts[14] = {
            { -10.935f, 32.565f, -31.772f },
            { -17.635f, 34.286f, -31.771f },
            { -24.938f, 33.703f, -31.795f },
            { -31.687f, 33.524f, -32.116f },
            { -38.701f, 33.005f, -32.116f },
            { -46.224f, 33.51f, -32.116f },
            { -61.621f, 36.247f, -46.187f },
            { -71.097f, 36.108f, -46.674f },
            { -76.251f, 36.426f, -54.49f },
            { -76.251f, 36.426f, -61.185f },
            { -69.825f, 36.541f, -68.408f },
            { -61.625f, 36.541f, -68.773f },
            { -51.77f, 29.418f, -102.253f },
            { -34.7f, 33.968f, -103.255f },
        };
        unsigned int* tex = (unsigned int*)xSTFindAsset(0x2AA06A3A, NULL);
        if (tex != NULL)
        {
            fn_80009CCC(pts, 14, *tex, 1.5f, 1.5f);
        }
    }
    else if (sceneID == SCENE_ID("e004"))
    {
        _xVec3 pts[15] = {
            { 336.775f, 10.53f, 68.837f },
            { 345.983f, 10.53f, 76.024f },
            { 369.36f, 10.596f, 75.283f },
            { 407.768f, 10.55f, 68.261f },
            { 440.203f, 10.592f, 86.134f },
            { 458.491f, 10.497f, 85.739f },
            { 475.495f, 10.071f, 93.686f },
            { 481.907f, 10.009f, 94.842f },
            { 482.091f, 9.982f, 104.687f },
            { 475.658f, 10.075f, 105.788f },
            { 467.437f, 10.51f, 113.27f },
            { 467.063f, 10.638f, 144.221f },
            { 442.756f, 10.297f, 144.6f },
            { 398.912f, 10.449f, 144.511f },
            { 378.616f, 10.219f, 144.498f },
        };
        unsigned int* tex = (unsigned int*)xSTFindAsset(0x6A38ADC5, NULL);
        if (tex != NULL)
        {
            fn_80009CCC(pts, 15, *tex, 3.6f, 3.6f);
        }
    }
    else if (sceneID == SCENE_ID("p003"))
    {
        _xVec3 pts0[5] = {
            { 57.448f, 13.165f, -49.734f },
            { 62.079f, 12.698f, -47.585f },
            { 61.519f, 14.91f, -57.24f },
            { 14.714f, 14.666f, -57.482f },
            { 17.973f, 17.225f, -66.808f },
        };
        _xVec3 pts1[4] = {
            { 59.459f, 17.985f, -45.893f },
            { 58.922f, 21.302f, -57.58f },
            { 17.133f, 21.198f, -57.649f },
            { 16.243f, 24.422f, -69.885f },
        };
        unsigned int* tex = (unsigned int*)xSTFindAsset(0xB0988A6C, NULL);
        if (tex != NULL)
        {
            fn_80009CCC(pts0, 5, *tex, 1.2f, 1.2f);
            fn_80009CCC(pts1, 4, *tex, 2.3f, 2.3f);
        }
    }
    else if (sceneID == SCENE_ID("p005"))
    {
        _xVec3 pts[3] = {
            { -39.211f, 10.394f, -3.671f },
            { -44.42f, 12.877f, -3.422f },
            { -49.24f, 12.468f, -3.422f },
        };
        unsigned int* tex = (unsigned int*)xSTFindAsset(0x10B4E293, NULL);
        if (tex != NULL)
        {
            fn_80009CCC(pts, 3, *tex, 2.5f, 2.5f);
        }
    }
}


extern float gFontWiggleTime;

void zEntPickup_SceneUpdate(float dt);

void xEntSetTimePassed(float dt);

void zCutsceneMgrUpdate(xBase* to, _xScene* sc, float dt);

void zUI_PreUpdate(_zUI* ent, _xScene* sc, float dt);

void zUIFont_PreUpdate(_zUIFont* ent, _xScene* sc, float dt);

void zTimerUpdate(xBase* to, _xScene* sc, float dt);

void zScriptUpdate(xBase* to, _xScene* sc, float dt);

void zFogUpdate(xBase* to, _xScene* sc, float dt);

void zParEmitterUpdate(xBase* to, _xScene* sc, float dt);

void zParSysUpdate(xBase* to, _xScene* sc, float dt);

void zLightUpdate(xBase* to, _xScene* sc, float dt);

void zSurfaceUpdate(xBase* to, _xScene* sc, float dt);

void zHologramUpdate(float dt);

void zActionLineUpdate(float dt);

void zLightningUpdate(float dt);

void zGustUpdateFX(float dt);

void xClimateUpdate(_tagClimate* climate, float dt);

void zLobMaster_timestep(zScene* s, float dt);

void zEntBubble_Cleanup();

void xFXUpdate(float dt);

void zGameStateSwitch(signed int theNewState);

static inline void zSceneEnableVisited(zScene* s)
{
    char name[64];
    strcpy(name, xUtil_idtag2string(s->sceneID, 0));
    name[1] = ' ';
    strcat(name, " VISITED");
    unsigned int id = xStrHash(name);
    for (signed int i = 0; i < s->num_base; i++)
    {
        xBase* b = s->base[i];
        if (b != NULL && b->baseType == 0x20 && id == b->id)
        {
            b->baseFlags |= 1;
        }
    }
}

static unsigned int zSceneInitFunc_DefaultEnt(zScene* s, zSceneObjectInstanceDesc* desc,
                                              unsigned int base_idx)
{
    unsigned char* block = NULL;
    unsigned int assetSize = 0;
    signed int count;
    unsigned int offset = desc->sizeRuntime;
    xBase* b;
    count = s->baseCount[desc->baseType];

    if (count)
    {
        block = (unsigned char*)xMemAlloc(gActiveHeap, count * offset);
        s->baseList[desc->baseType] = (xBase*)block;
    }
    for (signed int i = 0; i < count; i++)
    {
        void* asset = xSTFindAssetByType(desc->assetType, i, &assetSize);
        b = (xBase*)(block + i * offset);
        zSceneSet(b, base_idx);
        if (desc->objectInitFunc)
        {
            desc->objectInitFunc(b, asset);
        }
        xSceneAddEnt(s, (_xEnt*)s->base[base_idx]);
        zSceneObjHashtableAdd(b->id, b);
        base_idx++;
    }
    return base_idx;
}

static unsigned int zSceneInitFunc_Default(zScene* s, zSceneObjectInstanceDesc* desc,
                                           unsigned int base_idx)
{
    unsigned char* block = NULL;
    unsigned int assetSize = 0;
    signed int count;
    unsigned int offset = desc->sizeRuntime;
    xBase* b;
    count = s->baseCount[desc->baseType];

    if (count)
    {
        block = (unsigned char*)xMemAlloc(gActiveHeap, count * offset);
        s->baseList[desc->baseType] = (xBase*)block;
    }
    for (signed int i = 0; i < count; i++)
    {
        void* asset = xSTFindAssetByType(desc->assetType, i, &assetSize);
        b = (xBase*)(block + i * offset);
        zSceneSet(b, base_idx);
        if (desc->objectInitFunc)
        {
            desc->objectInitFunc(b, asset);
        }
        zSceneObjHashtableAdd(b->id, b);
        base_idx++;
    }
    return base_idx;
}

static unsigned int zSceneInitFunc_MovePoint(zScene* s, zSceneObjectInstanceDesc* desc,
                                             unsigned int base_idx)
{
    unsigned int assetSize = 0;
    signed int count = s->baseCount[desc->baseType];
    _zMovePoint* block = zMovePoint_GetMemPool(count);
    if (block != NULL)
    {
        s->baseList[desc->baseType] = (xBase*)block;
        for (signed int idx = 0; idx < count; idx++)
        {
            xBase* b = (xBase*)zMovePoint_GetInst(idx);
            p2MovePointAsset* asset =
                (p2MovePointAsset*)xSTFindAssetByType('MVPT', idx, &assetSize);
            zSceneSet(b, base_idx);
            zMovePointInit(zMovePoint_GetInst(idx), asset);
            zSceneObjHashtableAdd(b->id, b);
            base_idx++;
        }
    }
    return base_idx;
}

static unsigned int zSceneInitFunc_Villain(zScene* s, zSceneObjectInstanceDesc* desc,
                                           unsigned int base_idx)
{
    signed int count = s->baseCount[desc->baseType];
    if (count)
    {
        s->baseList[desc->baseType] = (xBase*)zEntVillain_MemPool(count);
        for (unsigned int idx = 0; idx < count; idx++)
        {
            st_zEntVillain* b = zEntVillain_NPCInst(idx);
            p2EntAsset* asset = (p2EntAsset*)xSTFindAssetByType('NPC ', idx, NULL);
            zSceneSet((xBase*)b, base_idx);
            zEntVillain_Init(b, asset);
            xSceneAddEnt(s, (_xEnt*)s->base[base_idx]);
            zSceneObjHashtableAdd(((xBase*)b)->id, (xBase*)b);
            base_idx++;
        }
    }
    return base_idx;
}

static unsigned int zSceneInitFunc_Player(zScene* s, zSceneObjectInstanceDesc* desc,
                                          unsigned int base_idx)
{
    signed int count = s->baseCount[desc->baseType];
    if (count)
    {
        s->baseList[desc->baseType] = (xBase*)xMemAlloc(gActiveHeap, count * 0xEC);
        for (signed int idx = 0; idx < count; idx++)
        {
            xBase* b = &globals.player.ent;
            zSceneSet(b, base_idx);
            if (idx == count - 1)
            {
                xSceneAddEnt(s, (_xEnt*)s->base[base_idx]);
            }
            globals.player.ent.id = *(unsigned int*)xSTFindAssetByType('PLYR', idx, NULL);
            zSceneObjHashtableAdd(b->id, b);
            base_idx++;
        }
    }
    return base_idx;
}

static unsigned int zSceneInitFunc_Camera(zScene* s, zSceneObjectInstanceDesc* desc,
                                          unsigned int base_idx)
{
    signed int count = s->baseCount[desc->baseType];
    if (count)
    {
#ifdef PC_PORT
        // xCameraInit returns void; the GameCube code relied on the camera array
        // being left in r3. Read it from the camera frame instead.
        xCameraInit(&globals.camera, count);
        _tagxCam* camBlock = globals.camera.cameras[0];
#else
        _tagxCam* camBlock = xCameraInit(&globals.camera, count);
#endif
        s->baseList[desc->baseType] = (xBase*)camBlock;
        for (signed int idx = 0; idx < count; idx++)
        {
            xBase* b = (xBase*)&camBlock[idx];
            xCamAsset* asset = (xCamAsset*)xSTFindAssetByType('CAM ', idx, NULL);
            zSceneSet(b, base_idx);
            xCameraSetAsset(&globals.camera, asset, idx);
            zCameraInit(b, asset);
            zSceneObjHashtableAdd(b->id, b);
            base_idx++;
        }
    }
    return base_idx;
}

static unsigned int zSceneInitFunc_Surface(zScene* s, zSceneObjectInstanceDesc* desc,
                                           unsigned int base_idx)
{
    signed int count = s->baseCount[desc->baseType];
    zSurfaceInit();
    if (count)
    {
        s->baseList[desc->baseType] = (xBase*)xSurfaceGetByIdx(0);
        for (signed int idx = 0; idx < count; idx++)
        {
            xBase* b = (xBase*)xSurfaceGetByIdx(idx);
            zSceneSet(b, base_idx);
            zSceneObjHashtableAdd(b->id, b);
            base_idx++;
        }
    }
    return base_idx;
}

static unsigned int zSceneInitFunc_Gust(zScene* s, zSceneObjectInstanceDesc* desc,
                                        unsigned int base_idx)
{
    signed int count = s->baseCount[desc->baseType];
    zGustInit();
    if (count)
    {
        s->baseList[desc->baseType] = (xBase*)zGustGetGust(0);
        for (signed int idx = 0; idx < count; idx++)
        {
            xBase* b = (xBase*)zGustGetGust(idx);
            zSceneSet(b, base_idx);
            zSceneObjHashtableAdd(b->id, b);
            base_idx++;
        }
    }
    return base_idx;
}

static unsigned int zSceneInitFunc_Volume(zScene* s, zSceneObjectInstanceDesc* desc,
                                          unsigned int base_idx)
{
    signed int count = s->baseCount[desc->baseType];
    fn_80005B58();
    if (count)
    {
        s->baseList[desc->baseType] = (xBase*)fn_80005AE8(0);
        for (signed int idx = 0; idx < count; idx++)
        {
            _zVolume* b = fn_80005AE8(idx);
            zLightSetVolume(b);
            zSceneSet((xBase*)b, base_idx);
            zSceneObjHashtableAdd(((xBase*)b)->id, (xBase*)b);
            base_idx++;
        }
    }
    return base_idx;
}

static unsigned int zSceneInitFunc_LobMaster(zScene* s, zSceneObjectInstanceDesc* desc,
                                             unsigned int base_idx)
{
    signed int count = s->baseCount[desc->baseType];
    if (count)
    {
        st_LOBMASTER_DATA* pool = zLobMaster_memPool(count);
        s->baseList[desc->baseType] = (xBase*)pool;
        for (signed int idx = 0; idx < count; idx++)
        {
            st_LOBMASTER_DATA* b = zLobMaster_getInst(pool, idx);
            p2BaseAsset* asset = (p2BaseAsset*)xSTFindAssetByType('LOBM', idx, NULL);
            zSceneSet((xBase*)b, base_idx);
            zLobMaster_Init(b, asset);
            zSceneObjHashtableAdd(((xBase*)b)->id, (xBase*)b);
            base_idx++;
        }
    }
    return base_idx;
}

static unsigned int zSceneInitFunc_Dispatcher(zScene* s, zSceneObjectInstanceDesc* desc,
                                              unsigned int base_idx)
{
    signed int count = s->baseCount[desc->baseType];
    if (count)
    {
        st_ZDISPATCH_DATA* pool = zDispatcher_memPool(count);
        s->baseList[desc->baseType] = (xBase*)pool;
        for (signed int idx = 0; idx < count; idx++)
        {
            st_ZDISPATCH_DATA* b = zDispatcher_getInst(pool, idx);
            p2BaseAsset* asset = (p2BaseAsset*)xSTFindAssetByType('DPAT', idx, NULL);
            zSceneSet((xBase*)b, base_idx);
            zDispatcher_Init(b, asset);
            zSceneObjHashtableAdd(((xBase*)b)->id, (xBase*)b);
            base_idx++;
        }
    }
    return base_idx;
}

void zSceneInit(unsigned int theSceneID)
{
    float pdone = 0.0f;
    unsigned char rgba_bkgrd[4] = { 15, 15, 15, 0 };
    zScene* s;
    signed int i;

    gTransitionSceneID = theSceneID;
    lbl_8025E868 = 0;

    char b[5] = {};
    sprintf(b, xUtil_idtag2string(theSceneID, 0));
    xStrupr(b);
    theSceneID = (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
    xUtil_idtag2string(theSceneID, 0);

    sMemDepthSceneStart = xMemPushBase();

    globals.stats.scares = 0;
    globals.stats.specials = 0;
    globals.stats.specials_tot = 0;
    globals.stats.tokens = 0;
    globals.stats.tokens_tot = 0;
    globals.stats.snacks = 0;
    globals.stats.snacks_tot = 0;
    globals.stats.chances = 0;
    globals.stats.chances_tot = 0;
    globals.stats.keys = 0;
    globals.stats.keys_tot = 0;
    globals.stats.clues = 0;
    globals.stats.clues_tot = 0;
    globals.stats.roomTime = 0.0f;
    for (i = 0; i < 13; i++)
    {
        globals.levelStats[i].clues_tot = sLevelTable[i].totalClues;
    }

    zHud_sceneReset();
    iTRCDisk::CheckDVDAndResetState();
    zGameScreenTransitionBegin();
    zGameScreenTransitionUpdate(pdone, "...preparing to preload\n");

    globals.sceneCur = (zScene*)xMemAlloc(gActiveHeap, sizeof(zScene));
    s = globals.sceneCur;
    xSceneInit(s, 205, 210, 230, 50);
    s->env = (_xEnv*)xMemAlloc(gActiveHeap, 0x28);
    s->sceneID = theSceneID;

    zGameScreenTransitionUpdate(pdone, "... scene preload ...\n");
    xUtil_idtag2string(theSceneID, 0);
    xSTPreLoadScene(theSceneID, NULL);
    xUtil_idtag2string(theSceneID, 0);
    xSTQueueSceneAssets(theSceneID);
    xUtil_idtag2string(theSceneID, 0);
    zGameScreenTransitionUpdate(pdone, "... scene asset queue ...\n");
    xUtil_idtag2string(theSceneID, 0);
    do
    {
        rgba_bkgrd[0] = 0;
        rgba_bkgrd[1] = 0;
        rgba_bkgrd[2] = 0;
        rgba_bkgrd[3] = 0;
        zGameScreenTransitionUpdate(pdone, "... scene loading ...\n", rgba_bkgrd);
        pdone = xSTLoadStep(theSceneID);
    } while (pdone < 1.0f);
    xUtil_idtag2string(theSceneID, 0);

    zGameScreenTransitionUpdate(pdone, "...initializing scene - sound\n");
    iSndInitSceneLoaded();
    xPadRumbleEnable(0, globals.option_vibration);
    xSTSwitchScene(theSceneID, NULL, NULL);
    globals.pickupTable = xSTFindAssetByType('PICK', 0, NULL);
    globals.sceneCur->resolvID = zSceneFindObject;
    globals.sceneCur->id2Name = zSceneGetName;
    globals.sceneCur->base2Name = zSceneGetName;
    g_xSceneCur = globals.sceneCur;

    zDispatcher_scenePrepare();
    zEntVillain_scenePrepare();
    zEntProjectile_scenePrepare();
    zLobMaster_scenePrepare();
    zAnimListInit();
    zGameScreenTransitionUpdate(100.0f, "...initializing scene - base types\n");

    for (signed int j = 0; j < 43; j++)
    {
        s->baseCount[j] = 0;
        s->baseList[j] = NULL;
    }
    s->num_base = 0;

    for (i = 0; sInitTable[i].name; i++)
    {
        unsigned int count = xSTAssetCountByType(sInitTable[i].assetType);
        s->baseCount[sInitTable[i].baseType] = count;
        s->num_base += count;
    }

    if (s->num_ents)
    {
        s->base = (xBase**)xMemAlloc(gActiveHeap, s->num_ents * sizeof(xBase*));
    }
    else
    {
        s->base = NULL;
    }

    scobj_idbps = (IDBasePair*)xMemAlloc(gActiveHeap, 2048 * sizeof(IDBasePair));
    memset(scobj_idbps, 0, 2048 * sizeof(IDBasePair));
    scobj_size = 2048;
    nidbps = 0;
    xFFXPoolInit(12);
    xFFXShakePoolInit(3);
    xFFXRotMatchPoolInit(1);
    xEntSceneInit();
    xEntMotionDebugInit(s->baseCount[6] + s->baseCount[18] + s->baseCount[24]);
    zLightSetVolume(NULL);
    xPartitionReset();

    unsigned int base_idx = 0;
    for (i = 0; sInitTable[i].name; i++)
    {
        if (sInitTable[i].func)
        {
            base_idx = sInitTable[i].func(s, &sInitTable[i], base_idx);
        }
    }

    zGameScreenTransitionEnd();

    signed int prjCount = xSTAssetCountByType('PRJT');
    if (prjCount)
    {
        zEntProjectile_prepSystem(prjCount, s);
        for (i = 0; i < prjCount; i++)
        {
            zProjectileTypeAsset* a = (zProjectileTypeAsset*)xSTFindAssetByType('PRJT', i, NULL);
            if (a != NULL)
            {
                zEntProjectile_registerType(a);
            }
        }
    }

    zLightResolveLinks();
    zRenderStateInit();
    zFeetGetIDs();
    zHologramInit();
    zLightningInit();
    zParCmdFindClipVolumes();
    zEntDestructObj_FindFX();
    zActionLineInit();
    xScrFxLetterbox(0);
    xClimateInit(&gClimate);
    iSndSuspendCD(0);
}

void zSceneExit()
{
    zScene* s = globals.sceneCur;
    if (s != NULL)
    {
        for (unsigned short i = 0; i < s->num_ents; i++)
        {
            xBase* b = s->base[i];
            xSndParentDied((unsigned int)b);
            if (b == NULL)
            {
                break;
            }
        }
    }
    xSndStopAll();
    xSndUpdate(0.0f);
    iSndWaitForDeadSounds();
    iSndSceneExit();
    iSndSuspendCD(1);
    fn_800162D8();

    RpWorld* world = s->env->geom->world;
    xModel_SceneExit(world);
    zFX_SceneExit(world);
    zEntEventAll(NULL, 0, 0x5A, NULL);
    zEntEventAll(NULL, 0, 0x58, NULL);
    lbl_8025E868 = 0;
    if (globals.sceneCur->sceneID != 'MNU3')
    {
        zSceneSave(s, NULL);
    }
    zEntPlayerExit(&globals.player.ent);
    zSurfaceExit();
    zLightDestroyAll();
    xEntSceneExit();
    xEntMotionDebugExit();
    zSceneObjHashtableExit();

    if (s->baseCount[39])
    {
        zParSys* ps = (zParSys*)s->baseList[39];
        for (signed int i = 0; i < (signed int)s->baseCount[39]; i++)
        {
            if (((xBase*)&ps[i])->baseFlags & 4)
            {
                zParSysExit(&ps[i]);
            }
        }
    }

    zAnimListExit();
    zLobMaster_sceneFinish();
    zEntProjectile_sceneFinish();
    zEntVillain_sceneFinish();
    zDispatcher_sceneFinish();
    xSTUnLoadScene(globals.sceneCur->sceneID);
    xMemPopBase(sMemDepthSceneStart);
    sMemDepthSceneStart = -1;
    xUtil_idtag2string(globals.sceneCur->sceneID, 0);
    globals.sceneCur = NULL;
}

void zSceneSwitch(_zPortal* p)
{
    globals.sceneCur->pendingPortal = p;
    zPortalAsset* passet = globals.sceneCur->pendingPortal->passet;

    HackSwapIt((char*)&passet->sceneID, 4);
    char* id = (char*)&passet->sceneID;
    unsigned int nextSceneID = (id[0] << 24) | (id[1] << 16) | (id[2] << 8) | id[3];

    if (nextSceneID == globals.sceneCur->sceneID)
    {
        unsigned int markerID = passet->assetMarkerID;
        unsigned int camID = passet->assetCameraID;
        unsigned int size;

        _tagxCam* cam = (_tagxCam*)zSceneFindObject(camID);
        if (cam != NULL)
        {
            xCameraPickInitial(&globals.camera, cam);
            globals.player.cp.initCamID = camID;
        }

        _xVec3* marker = (_xVec3*)xSTFindAsset(markerID, &size);
        if (marker != NULL)
        {
            xVec3Copy(&globals.player.ent.frame->mat.pos, marker);
            xVec3Copy(&((_xMat4x3*)globals.player.ent.model->Mat)->pos, marker);
            xCameraSetTarget(&globals.camera, &globals.player.ent.frame->mat.pos);
            xCameraSetTarget(&globals.camera, (_xMat4x3*)globals.player.ent.model->Mat);
        }
        globals.sceneCur->pendingPortal = NULL;
    }
}

void zSceneSave(zScene* ent, xSerial* xs)
{
    xSerial xser;
    signed int i;

    xs = &xser;
    xs->setClient('PLYR');
    xs->Write_b1(1);
    xs->Write(globals.player.Inv_Chances);
    xs->Write(globals.player.Inv_ScoobySnacks);
    xs->Write(globals.player.Inv_Clue);
    xs->Write(globals.player.Inv_Key);
    xs->Write(globals.player.Inv_Mtokens[0]);
    for (i = 0; i < 2; i++)
    {
        xs->Write(globals.player.Inv_Ammo[i]);
    }
    for (i = 0; i < 2; i++)
    {
        xs->Write(globals.player.Inv_AmmoMax[i]);
    }
    xs->Write(globals.player.Inv_Specials);
    xs->Write(globals.stats.snacks_tot);
    xs->Write(globals.stats.tokens_tot);
    for (i = 0; i < 30; i++)
    {
        xs->Write(globals.areaStats[i].snacks_tot);
        xs->Write(globals.areaStats[i].tokens_tot);
    }
    for (i = 0; i < 26; i++)
    {
        if (zEntPickup_WarpIsOpen(i))
        {
            xs->Write_b1(1);
        }
        else
        {
            xs->Write_b1(0);
        }
    }
    zCutsceneMgrSave(NULL, xs);
    zVarSave(xs);

    xs->setClient(ent->sceneID);
    xs->Write_b1(1);
    xSceneSave(ent, xs);
    xs->Write(++globals.stats.visited);
    xs->Write(globals.stats.scares_sum);
    xs->Write(globals.stats.deaths_sum);

    for (i = 0; i < globals.sceneCur->num_base; i++)
    {
        xBase* b = globals.sceneCur->base[i];
        if (b->baseFlags & 2)
        {
            switch (b->baseType)
            {
            case 1:
                zEntTriggerSave((zEntTrigger*)b, xs);
                break;
            case 2:
                zEntVillain_Save((st_zEntVillain*)b, xs);
                break;
            case 4:
                zEntPickup_Save((_zEntPickup*)b, xs);
                break;
            case 5:
                zEnvSave((_zEnv*)b, xs);
                break;
            case 36:
                zFogSave((_zFog*)b, xs);
                break;
            case 37:
                zLightSave((_zLight*)b, xs);
                break;
            case 6:
                zPlatform_Save((_zPlatform*)b, xs);
                break;
            case 7:
                zCameraSave((_tagxCam*)b, xs);
                break;
            case 11:
                zEntSimpleObj_Save((_zEntSimpleObj*)b, xs);
                break;
            case 13:
                zMovePointSave((_zMovePoint*)b, xs);
                break;
            case 14:
                zTimerSave((_zTimer*)b, xs);
                break;
            case 16:
                zPortalSave((_zPortal*)b, xs);
                break;
            case 17:
                zGroupSave((_zGroup*)b, xs);
                break;
            case 18:
                zPendulum_Save((_zPendulum*)b, xs);
                break;
            case 19:
                zSFXSave((zSFX*)b, xs);
                break;
            case 22:
                zCounterSave((_zCounter*)b, xs);
                break;
            case 23:
                zEntHangable_Save((_zEntHangable*)b, xs);
                break;
            case 24:
                zEntButton_Save((_zEntButton*)b, xs);
                break;
            case 26:
                zSurfaceSave((xSurface*)b, xs);
                break;
            case 27:
                zEntDestructObj_Save((_zEntDestructObj*)b, xs);
                break;
            case 28:
                zGustSave((_zGust*)b, xs);
                break;
            case 42:
                zScriptSave((_zScript*)b, xs);
                break;
            case 29:
                fn_80005C54(b, xs);
                break;
            case 30:
                zDispatcher_Save((st_ZDISPATCH_DATA*)b, xs);
                break;
            case 31:
                zConditionalSave((_zConditional*)b, xs);
                break;
            case 32:
                zUI_Save((_zUI*)b, xs);
                break;
            case 33:
                zUIFont_Save((_zUIFont*)b, xs);
                break;
            case 41:
                zEGenerator_Save((zEGenerator*)b, xs);
                break;
            }
        }
    }
}

void zSceneLoad(zScene* ent, xSerial* xs)
{
    xSerial xser;
    signed int val;
    signed int i;

    xs = &xser;
    xs->setClient('PLYR');
    val = 0;
    xs->Read_b1(&val);
    if (val)
    {
        xs->Read(&globals.player.Inv_Chances);
        xs->Read(&globals.player.Inv_ScoobySnacks);
        xs->Read(&globals.player.Inv_Clue);
        xs->Read(&globals.player.Inv_Key);
        xs->Read(&globals.player.Inv_Mtokens[0]);
        for (i = 0; i < 2; i++)
        {
            xs->Read(&globals.player.Inv_Ammo[i]);
        }
        for (i = 0; i < 2; i++)
        {
            xs->Read(&globals.player.Inv_AmmoMax[i]);
        }
        xs->Read(&globals.player.Inv_Specials);
        xs->Read(&globals.stats.snacks_tot);
        xs->Read(&globals.stats.tokens_tot);
        for (i = 0; i < 30; i++)
        {
            xs->Read(&globals.areaStats[i].snacks_tot);
            xs->Read(&globals.areaStats[i].tokens_tot);
        }
        for (i = 0; i < 26; i++)
        {
            signed int open = 0;
            xs->Read_b1(&open);
            if (open)
            {
                enableWarpPoint(i);
            }
        }
        zCutsceneMgrLoad(NULL, xs);
        zVarLoad(xs);
    }

    xs->setClient(ent->sceneID);
    val = 0;
    xs->Read_b1(&val);
    if (val)
    {
        xSceneLoad(ent, xs);
        xs->Read(&globals.stats.visited);
        xs->Read(&globals.stats.scares_sum);
        xs->Read(&globals.stats.deaths_sum);
        for (unsigned short j = 0; j < globals.sceneCur->num_ents; j++)
        {
            xBase* b = globals.sceneCur->base[j];
            if (b->baseFlags & 2)
            {
                switch (b->baseType)
                {
                case 1:
                    zEntTriggerLoad((zEntTrigger*)b, xs);
                    break;
                case 2:
                    zEntVillain_Load((st_zEntVillain*)b, xs);
                    break;
                case 3:
                    zEntPlayer_Load((_xEnt*)b, xs);
                    break;
                case 4:
                    zEntPickup_Load((_zEntPickup*)b, xs);
                    break;
                case 5:
                    zEnvLoad((_zEnv*)b, xs);
                    break;
                case 36:
                    zFogLoad((_zFog*)b, xs);
                    break;
                case 37:
                    zLightLoad((_zLight*)b, xs);
                    break;
                case 6:
                    zPlatform_Load((_zPlatform*)b, xs);
                    break;
                case 7:
                    zCameraLoad((_tagxCam*)b, xs);
                    break;
                case 11:
                    zEntSimpleObj_Load((_zEntSimpleObj*)b, xs);
                    break;
                case 13:
                    zMovePointLoad((_zMovePoint*)b, xs);
                    break;
                case 14:
                    zTimerLoad((_zTimer*)b, xs);
                    break;
                case 16:
                    zPortalLoad((_zPortal*)b, xs);
                    break;
                case 17:
                    zGroupLoad((_zGroup*)b, xs);
                    break;
                case 18:
                    zPendulum_Load((_zPendulum*)b, xs);
                    break;
                case 19:
                    zSFXLoad((zSFX*)b, xs);
                    break;
                case 22:
                    zCounterLoad((_zCounter*)b, xs);
                    break;
                case 23:
                    zEntHangable_Load((_zEntHangable*)b, xs);
                    break;
                case 24:
                    zEntButton_Load((_zEntButton*)b, xs);
                    break;
                case 26:
                    zSurfaceLoad((xSurface*)b, xs);
                    break;
                case 27:
                    zEntDestructObj_Load((_zEntDestructObj*)b, xs);
                    break;
                case 28:
                    zGustLoad((_zGust*)b, xs);
                    break;
                case 42:
                    zScriptLoad((_zScript*)b, xs);
                    break;
                case 29:
                    fn_80005C34(b, xs);
                    break;
                case 30:
                    zDispatcher_Load((st_ZDISPATCH_DATA*)b, xs);
                    break;
                case 31:
                    zConditionalLoad((_zConditional*)b, xs);
                    break;
                case 32:
                    zUI_Load((_zUI*)b, xs);
                    break;
                case 33:
                    zUIFont_Load((_zUIFont*)b, xs);
                    break;
                case 41:
                    zEGenerator_Load((zEGenerator*)b, xs);
                    break;
                }
            }
        }
    }
    else
    {
        globals.stats.roomTime_sum = 0.0f;
        globals.stats.visited = 1;
        globals.stats.timeRunning_sum = 0.0f;
        globals.stats.timeWalking_sum = 0.0f;
        globals.stats.scares_sum = 0;
        globals.stats.deaths_sum = 0;
        globals.stats.singleJumps_sum = 0;
        globals.stats.doubleJumps_sum = 0;
        globals.stats.gumShot_sum = 0;
        globals.stats.gumEmpty_sum = 0;
        globals.stats.soapShot_sum = 0;
        globals.stats.soapHit_sum = 0;
        globals.stats.soapEmpty_sum = 0;
        globals.stats.food_sum = 0;
        globals.stats.snackBoxes_sum = 0;
        globals.stats.enemyKills_sum = 0;
    }
}

void zSceneReset()
{
    xBase* b;
    zScene* s;
    signed int i;

    lbl_8025E868 = 0;
    s = globals.sceneCur;
    zMusicKill();
    xPadDestroyRumbleChain(0);
    globals.cmgr = NULL;
    zEntEventAll(NULL, 0, 0x5A, NULL);
    zEntEventAll(NULL, 0, 0x58, NULL);
    zSceneSave(s, NULL);
    xSndStopAll();
    xSndUpdate(0.0f);
    iSndWaitForDeadSounds();
    zEntProjectile_sceneReset();

    for (i = 0; i < s->num_base; i++)
    {
        b = s->base[i];
        if (b != NULL)
        {
            switch (b->baseType)
            {
            default:
                zEntEvent(NULL, 0, b, 10, NULL, NULL, 1);
                break;
            case 22:
                zCounterReset(b);
                break;
            case 3:
                zEntPlayerReset((_xEnt*)b);
                break;
            case 6:
                zPlatform_Reset((_zPlatform*)b, globals.sceneCur);
                break;
            case 18:
                zPendulum_Reset((_zPendulum*)b, globals.sceneCur);
                break;
            case 23:
                zEntHangable_Reset((_zEntHangable*)b);
                break;
            case 35:
                zEntEvent(b, 10);
                break;
            case 41:
                zEGenerator_Reset((zEGenerator*)b, globals.sceneCur);
                break;
            case 5:
                if (b != NULL)
                {
                    _tagxCam* cam = (_tagxCam*)zSceneFindObject(((_zEnv*)b)->easset->startCameraAssetID);
                    if (cam != NULL)
                    {
                        xCameraPickInitial(&globals.camera, cam);
                    }
                    xClimateInitAsset(&gClimate, ((_zEnv*)b)->easset);
                }
                break;
            }
        }
    }

    zMusicNotify(0);
    {
        zScene* sc = globals.sceneCur;
        for (i = 0; i < sc->num_act_ents; i++)
        {
            sc->act_ents[i]->updateTimer = 0.0f;
        }
    }

    zEntShaggy_SceneReset();
    zEntPlayer_LoadCheckPoint();
    zSceneLoad(globals.sceneCur, NULL);
    zSceneEnableVisited(s);
    xSerialTraverse(zSceneSetup_serialTraverseCB);
    zEntEventAll(NULL, 0, 0x57, NULL);
    zEntEventAll(NULL, 0, 0x59, NULL);
}

void zSceneSetup()
{
    zScene* s = globals.sceneCur;
    unsigned int size;

    globals.cmgr = NULL;
    lbl_8025EEC8 = 0;
    xSceneSetup(s);
    s->gravity = -50.0f;
    s->drag = 0.08f;
    s->flags = 5;
    zEntBubble_SceneInit(s);

    if (s->baseCount[28])
    {
        zGustSetup();
    }
    if (s->baseCount[29])
    {
        fn_80005B08();
    }

    gCurEnv = NULL;
    for (signed int i = 0; i < s->num_base; i++)
    {
        xBase* b = s->base[i];
        if (b != NULL)
        {
            switch (b->baseType)
            {
            case 5:
                gCurEnv = (_zEnv*)b;
                zEnvSetup(gCurEnv);
                xClimateInitAsset(&gClimate, ((_zEnv*)gCurEnv)->easset);
                break;
            case 2:
                fn_800A8F88(b, s);
                break;
            case 6:
                zPlatform_Reset((_zPlatform*)b, s);
                zPlatform_Setup((_zPlatform*)s->base[i], s);
                break;
            case 13:
                zMovePointSetup((_zMovePoint*)b, s);
                break;
            case 18:
                zPendulum_Reset((_zPendulum*)b, s);
                zPendulum_Setup((_zPendulum*)s->base[i], s);
                break;
            case 24:
                zEntButton_Reset((_zEntButton*)b, s);
                zEntButton_Setup((_zEntButton*)s->base[i], s);
                break;
            case 35:
                zLobMaster_InitDep((st_LOBMASTER_DATA*)b, s);
                break;
            case 11:
                zEntSimpleObj_Setup((_zEntSimpleObj*)b);
                break;
            case 30:
                zDispatcher_InitDep((st_ZDISPATCH_DATA*)b, s);
                break;
            case 41:
                zEGenerator_Setup((zEGenerator*)b, s);
                zEGenerator_Reset((zEGenerator*)s->base[i], s);
                break;
            case 4:
                zEntPickup_Setup((_zEntPickup*)b);
                zEntPickup_Reset((_zEntPickup*)s->base[i]);
                break;
            case 39:
                zParSysSetup((zParSys*)b);
                break;
            case 26:
                zSurfaceSetup((xSurface*)b);
                break;
            case 38:
                zParEmitterSetup((_zParEmitter*)b);
                break;
            case 42:
                break;
            }
        }
    }

    zEntPickup_Setup();
    zEntHangable_SetupFX();
    xCutscene_Init(xSTFindAssetByType('CTOC', 0, &size));

    for (signed int i = 0; i < s->num_base; i++)
    {
        xBase* b = s->base[i];
        if (b != NULL && b->baseType == 13)
        {
            xMovePointSplineSetup((_xMovePoint*)b);
        }
    }

    zSceneEnableVisited(s);
    xSerialTraverse(zSceneSetup_serialTraverseCB);
    xQuickCullInit(xEntGetAllEntsBox());
    sCardCheckEnabled = 0;
    iColor_tag black = { 0, 0, 0, 0xFF };
    iColor_tag clear = {};
    xScrFxFade(&black, &clear, 1.0f, NULL, 0, 1);
}


signed int zSceneSetup_serialTraverseCB(unsigned int clientID, xSerial* xser)
{
    signed int val = 0;
    char name[16];

    xser->Read_b1(&val);
    if (val == 0)
    {
        return 1;
    }

    sprintf(name, "%s", xUtil_idtag2string(clientID, 0));
    name[1] = ' ';
    strcat(name, " VISITED");
    xBase* b = zSceneObjHashtableGet(xStrHash(name));
    if (b != NULL)
    {
        b->baseFlags |= 1;
    }
    return 1;
}

void zSceneUpdate(float dt)
{
    signed int i;
    signed int pause = zGameIsPaused();

    sUpdateDt = dt;
    gFontWiggleTime += dt;
    zEntPickup_SceneUpdate(dt);
    xEntSetTimePassed(dt);
    if (globals.cmgr != NULL)
    {
        zCutsceneMgrUpdate((xBase*)globals.cmgr, globals.sceneCur, dt);
    }

    zScene* s = globals.sceneCur;
    xBase** b = s->base;

    if (s->baseCount[32])
    {
        _zUI* ui = (_zUI*)s->baseList[32];
        for (i = 0; i < (signed int)s->baseCount[32]; i++)
        {
            if (ui[i].baseFlags & 4)
            {
                zUI_PreUpdate(&ui[i], s, dt);
            }
        }
    }

    if (s->baseCount[33])
    {
        _zUIFont* uif = (_zUIFont*)s->baseList[33];
        for (i = 0; i < (signed int)s->baseCount[33]; i++)
        {
            if (uif[i].baseFlags & 4)
            {
                zUIFont_PreUpdate(&uif[i], s, dt);
            }
        }
    }

    for (i = 0; i < s->num_base; i++)
    {
        if (b[i]->baseFlags & 4)
        {
            switch (b[i]->baseType)
            {
            case 32:
            case 33:
                if (((_xEnt*)b[i])->update != NULL)
                {
                    ((_xEnt*)b[i])->update((_xEnt*)b[i], s, dt);
                }
                break;
            case 1:
            case 2:
            case 4:
            case 6:
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            case 18:
            case 23:
            case 24:
            case 27:
            case 41:
                if (((_xEnt*)b[i])->update != NULL && !pause)
                {
                    ((_xEnt*)b[i])->update((_xEnt*)b[i], s, dt);
                }
                break;
            case 14:
                if (pause)
                {
                    if (!sPauseTimersHashedInit)
                    {
                        sPauseTimersHashed = 0;
                        sPauseTimersHashedInit = 1;
                    }
                    if (!sPauseTimersHashed)
                    {
                        for (signed int j = 0; j < 10; j++)
                        {
                            char name[32];
                            sprintf(name, "pause_timer%d", j);
                            sPauseTimerIDs[j] = xStrHash(name);
                        }
                        sPauseTimersHashed = 1;
                    }
                    for (signed int j = 0; j < 10; j++)
                    {
                        if (b[i]->id == sPauseTimerIDs[j])
                        {
                            zTimerUpdate(b[i], s, dt);
                            break;
                        }
                    }
                }
                else
                {
                    zTimerUpdate(b[i], s, dt);
                }
                break;
            case 42:
                if (!pause)
                {
                    zScriptUpdate(b[i], s, dt);
                }
                break;
            case 36:
                if (!pause)
                {
                    zFogUpdate(b[i], s, dt);
                }
                break;
            case 38:
                if (!pause)
                {
                    zParEmitterUpdate(b[i], s, dt);
                }
                break;
            case 39:
                if (!pause)
                {
                    zParSysUpdate(b[i], s, dt);
                }
                break;
            case 37:
                if (!pause)
                {
                    zLightUpdate(b[i], s, dt);
                }
                break;
            case 26:
                if (!pause)
                {
                    zSurfaceUpdate(b[i], s, dt);
                }
                break;
            case 43:
                break;
            }
        }
    }

    if (!pause)
    {
        zHologramUpdate(dt);
        zActionLineUpdate(dt);
        zLightningUpdate(dt);
        zGustUpdateFX(dt);
        xClimateUpdate(&gClimate, dt);
        zLobMaster_timestep(s, dt);
        zEntProjectile_poolUpdate(ZPRJ_CAT_UNKNOWN, s, dt);

        _zEnt** bubbles = s->bubbles;
        for (i = 0; i < (signed int)s->num_bubbles; i++)
        {
            _zEnt* e = bubbles[i];
            if (e->baseFlags & 4)
            {
                e->update(e, s, dt);
            }
        }

        zEntBubble_Cleanup();
        xFXUpdate(dt);
        globals.stats.roomTime += dt;
        globals.stats.roomTime_sum += dt;

        if (s->pendingPortal != NULL)
        {
            if (s->pendingPortal->passet->sceneID == '600S' || s->sceneID == 'S006')
            {
                zGameStateSwitch(5);
            }
            else
            {
                zGameStateSwitch(4);
            }
        }
    }
}

static inline iColor_tag zSceneColor(unsigned char r, unsigned char g, unsigned char b,
                                     unsigned char a)
{
    iColor_tag c;
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    return c;
}

void zSceneRender()
{
    zScene* s = globals.sceneCur;
    signed int i;

    if (s->sceneID == SCENE_ID("MNU3"))
    {
        fn_800167F0();
    }

    zRenderState(SDRS_OpaqueModels);
    zLightAddLocalEnv();
    zEnvRender(s->env);
    zLightRemoveLocalEnv();

    if (s->env->geom != NULL && s->env->geom->fx != NULL)
    {
        xCameraEnd(&globals.camera, 0.0f, 0);
        iEnvBeginRenderFX(NULL);
        xCameraBegin(&globals.camera, 0);
        zLightAddLocalEnv();
        iEnvRenderFX(NULL);
        zLightRemoveLocalEnv();
    }

    _xEnt* ents[64] = {};
    float dists[64] = {};
    signed int numEnts = 0;

    zRenderState(SDRS_OpaqueModels);

    unsigned int renderPlayer;
    _tagxCamFrame* cam = &globals.camera;
    _xEnt** entptr = &s->act_ents[s->num_act_ents - 1];
    _xEnt** entlast = &s->act_ents[s->baseCount[1] + s->baseCount[4]];
    renderPlayer = 0;
    _xEnt* tide = NULL;
    unsigned int tideID = xStrHash("TIDE");

    while (entptr >= entlast)
    {
        _xEnt* ent = *entptr;
        if (ent->collType != 1)
        {
            if (ent->id == tideID)
            {
                tide = ent;
            }
            else if ((ent->collType == 0x10 && globals.player.Transparent > 0) || (ent->flags & 4) ||
                     ent->material_alpha < 255.0f || (ent->moreFlags & 1))
            {
                if (globals.cmgr != NULL && ent->collType != 0x10)
                {
                    ent->render(ent);
                }
                else
                {
                    _xVec3 center = *xBoundCenter(&ent->bound);
                    float dx = cam->hi_cam.mat.pos.x - center.x;
                    float dz = cam->hi_cam.mat.pos.z - center.z;
                    ents[numEnts] = ent;
                    dists[numEnts] = dx * dx + dz * dz;
                    numEnts++;
                }
            }
            else if (ent->collType == 0x10)
            {
                renderPlayer = 1;
            }
            else if (ent->render != NULL)
            {
                ent->render(ent);
            }
        }
        entptr--;
    }

    zScene* sc = globals.sceneCur;
    for (unsigned int j = 0; j < numEnts; j++)
    {
        _xEnt* ent = ents[j];
        for (signed int k = 0; k < 66; k++)
        {
            zSceneSortEntry* e = &sSortTable[k];
            if (sc->sceneID == e->sceneID && ent->id == e->id)
            {
                switch (e->type)
                {
                case 0:
                {
                    float dx = cam->hi_cam.mat.pos.x - e->a;
                    float dz = cam->hi_cam.mat.pos.z - e->b;
                    dists[j] = dx * dx + dz * dz;
                    break;
                }
                case 2:
                {
                    unsigned int* relID = (unsigned int*)&e->a;
                    for (unsigned int m = 0; m < numEnts; m++)
                    {
                        if (*relID == ents[m]->id)
                        {
                            dists[j] = dists[m] + e->b;
                            break;
                        }
                    }
                    break;
                }
                case 1:
                default:
                    dists[j] = e->a;
                    break;
                }
                break;
            }
        }
    }

    zEntPickup_RenderList((_zEntPickup*)s->baseList[4], s->baseCount[4]);

    if (renderPlayer == 1 && globals.player.Transparent <= 0)
    {
        globals.player.ent.render(&globals.player.ent);
        zSceneRenderShadowReceivers();
    }

    zRenderState(SDRS_OpaqueModels);
    zEntProjectile_poolRender(ZPRJ_CAT_UNKNOWN);
    zRenderState(SDRS_OpaqueModels);
    zEntVillain_BonusRender();
    zRenderState(SDRS_Lightning);
    zLightningRender();
    zActionLineRender();
    zRenderState(SDRS_SkidMark);
    for (i = 0; i < 5; i++)
    {
        zFeetRenderSkidMark(&sPlayerSkidMark[i]);
    }
    zRenderState(SDRS_Bubble);
    zEntBubble_Render();

    if (tide != NULL)
    {
        zRenderState(SDRS_Tide);
        tide->render(tide);
    }

    if (numEnts > 64)
    {
        numEnts = 64;
    }

    for (i = 0; i < numEnts; i++)
    {
        for (signed int j = 0; j < numEnts; j++)
        {
            if (dists[i] > dists[j])
            {
                float td = dists[i];
                _xEnt* te = ents[i];
                dists[i] = dists[j];
                ents[i] = ents[j];
                dists[j] = td;
                ents[j] = te;
            }
        }
    }

    zSceneRenderLevelFX();

    if (s->env->geom != NULL && s->env->geom->fx != NULL)
    {
        xCameraEnd(&globals.camera, 0.0f, 0);
        iEnvBeginRenderFX(NULL);
        xCameraBegin(&globals.camera, 0);
        zLightAddLocalEnv();
        fn_8000A138(NULL);
        zLightRemoveLocalEnv();
    }

    zRenderState(SDRS_Particles);
    {
        zParSys* ps = (zParSys*)s->baseList[39];
        signed int n = s->baseCount[39] - 1;
        while (n >= 0)
        {
            zParSysRender(ps);
            n--;
            ps++;
        }
    }

    zRenderState(SDRS_AlphaModels);
    iShadowRender();

    for (i = 0; i < numEnts; i++)
    {
        _xEnt* ent = ents[i];
        ent->render(ent);
        if (ent->baseType == 3)
        {
            zSceneRenderShadowReceivers();
        }
    }

    switch (zGameModeGet())
    {
    case 4:
        break;
    case 5:
    case 6:
    case 7:
    case 9:
    case 11:
        zRenderState(SDRS_HUD);
        zHud_Render();
        break;
    }

    zRenderState(SDRS_Font);
    zUIRenderAll();
    fn_80032D28();

    if (zGameModeGet() == 1 && (zGameStateGet() == 0 || zGameStateGet() == 1))
    {
        iColor_tag yellow = zSceneColor(255, 230, 0, 255);
        xFontBox* box = xFontBoxCreate(0.0f, 0.0f, 640.0f, 480.0f, 1, yellow);

        iColor_tag yellow2 = zSceneColor(255, 230, 0, 255);
        xFontBox* prompt = xFontBoxCreate(0.0f, 340.0f, 640.0f, 100.0f, 0, yellow2);

        signed int avail = sRenderVar1;
        char msg[256];
        char promptMsg[256];
        msg[0] = 0;
        promptMsg[0] = 0;

        if (zGameStateGet() == 0)
        {
            if (sRenderVar1 <= 0)
            {
                avail = 0;
            }
            sprintf(msg,
                    "Insufficient space on the Nintendo GameCube_ Memory Card in Slot %s.\n"
                    "At least $A000210068%d file%s and %d block%s needed to save game data.\n"
                    "($A220020026%d file%s and %d block%s available.)\n\nDo you want to start?",
                    zMenuGetBadCard() == 1 ? "A" : "B", sRenderVar3, sRenderVar3 == 1 ? "" : "s",
                    sRenderVar0 / sRenderVar2,
                    sRenderVar0 / sRenderVar2 == 1 ? "$C is" : "s$C are", sRenderVar4,
                    sRenderVar4 == 1 ? "" : "s", avail / sRenderVar2,
                    avail / sRenderVar2 == 1 ? "$C is" : "s$C are");
            xFontBoxSetText(box, msg);
            xFontBoxSetDrawSize(box, 18.0f, 24.0f);
            xFontBoxSetDrawSpacing(box, 17.0f, 24.0f);
            iColor_tag black = zSceneColor(0, 0, 0, 255);
            xFontBoxSetBackdrop(box, black);
            xFontBoxSetInset(box, 100.0f, 100.0f, 100.0f, 0.0f);
            xFontBoxSetJustification(box, 0x40);

            sprintf(promptMsg, "%s-Yes\t\t%s-No    ", "#", "}");
            xFontBoxSetText(prompt, promptMsg);
            xFontBoxSetDrawSize(prompt, 21.0f, 28.0f);
            xFontBoxSetDrawSpacing(prompt, 20.0f, 28.0f);
            iColor_tag black2 = zSceneColor(0, 0, 0, 255);
            xFontBoxSetBackdrop(prompt, black2);
            xFontBoxSetInset(prompt, 40.0f, 10.0f, 0.0f, 0.0f);
            xFontBoxSetJustification(prompt, 0x40);
        }
        else
        {
            xFontBoxSetText(box, NULL);
            iColor_tag black = zSceneColor(0, 0, 0, 255);
            xFontBoxSetBackdrop(box, black);
        }

        xFontBoxRender(box);
        xFontBoxDestroy(box);
        xFontBoxRender(prompt);
        xFontBoxDestroy(prompt);
    }

    zRenderState(SDRS_Font);
    xFontRender();

    if (zGameModeGet() == 1 && zGameStateGet() == 3)
    {
        DrawScreenSizeRectangle();
    }
}

// Stand-in for a function the linker strips; it keeps a 0.0f ahead of
// zSceneRender's constants in .sdata2.
float zSceneZeroDeadStripped()
{
    return 0.0f;
}

#pragma auto_inline on
xBase* zSceneFindObject(unsigned int gameID)
{
    return zSceneObjHashtableGet(gameID);
}
#pragma auto_inline reset

xBase* zSceneGetObject(signed int type, signed int idx)
{
    zScene* s = globals.sceneCur;
    if (s != NULL)
    {
        for (signed int i = 0; i < s->num_base; i++)
        {
            xBase* b = s->base[i];
            if (b != NULL && type == b->baseType)
            {
                if (idx == 0)
                {
                    return b;
                }
                idx--;
            }
        }
    }
    return NULL;
}

char* zSceneGetName(unsigned int gameID)
{
    xBase* b = zSceneObjHashtableGet(gameID);
    if (b != NULL)
    {
        return zSceneGetName(b);
    }
    return "";
}

#pragma auto_inline on
char* zSceneGetName(xBase* b)
{
    if (b != NULL)
    {
        char* n = xSTAssetName(b->id);
        if (n != NULL)
        {
            return n;
        }
    }
    return "";
}
#pragma auto_inline reset

void zSceneForAllBase(xBase* (*func)(xBase*, zScene*, void*), signed int baseType, void* data)
{
    zScene* s = globals.sceneCur;
    if (s != NULL)
    {
        for (unsigned short i = 0; i < s->num_ents; i++)
        {
            xBase* b = s->base[i];
            if (baseType == b->baseType)
            {
                if (func(b, s, data) == NULL)
                {
                    break;
                }
            }
        }
    }
}

void zSceneMemLvlChkCB()
{
    xMemGetBase();
}

#pragma auto_inline on
unsigned int zSceneGetArea(unsigned int sceneID)
{
    for (unsigned int i = 0; i < 30; i++)
    {
        for (signed int j = 0; j < 4; j++)
        {
            if (sceneID == sAreaTable[i].sceneIDs[j])
            {
                return i;
            }
        }
    }
    return 0;
}
#pragma auto_inline reset

unsigned int zSceneGetRoominArea(unsigned int sceneID)
{
    for (unsigned int i = 0; i < 30; i++)
    {
        for (signed int j = 0; j < 4; j++)
        {
            if (sceneID == sAreaTable[i].sceneIDs[j])
            {
                return j;
            }
        }
    }
    return -1;
}

char* zSceneGetAreaname(unsigned int sceneID)
{
    if (sceneID == 'O001')
    {
        return "Cower in the Tower!";
    }
    if (sceneID == 'H003')
    {
        return "Monster Gallery";
    }
    return sAreaTable[zSceneGetArea(sceneID)].desc;
}

// Stand-in for a function the linker strips; it keeps " VISITED" ahead of the
// area name strings in .rodata.
char* zSceneVisitedDeadStripped()
{
    return " VISITED";
}

unsigned int zSceneGetAreaConstTotalSnacks(unsigned int sceneID)
{
    return sAreaTable[zSceneGetArea(sceneID)].constTotalSnacks;
}

unsigned int zSceneGetAreaTotalSnacks(unsigned int sceneID)
{
    return sAreaTable[zSceneGetArea(sceneID)].totalSnacks;
}

unsigned int zSceneGetAreaTotalSnacks(_tagSceneAreas area)
{
    return sAreaTable[area].totalSnacks;
}

unsigned int zSceneGetAreaTotalTokens(unsigned int sceneID)
{
    return sAreaTable[zSceneGetArea(sceneID)].totalMTokens;
}

unsigned int zSceneGetAreaSnacksCollected(unsigned int sceneID)
{
    unsigned int area = zSceneGetArea(sceneID);
    unsigned int got = globals.areaStats[area].snacks_tot;
    if (got > sAreaTable[area].totalSnacks)
    {
        sAreaTable[area].totalSnacks = got;
    }
    return got;
}

unsigned int zSceneGetAreaTokensCollected(unsigned int sceneID)
{
    unsigned int area = zSceneGetArea(sceneID);
    unsigned int got = globals.areaStats[area].tokens_tot;
    unsigned int total = sAreaTable[area].totalMTokens;
    if (got > total)
    {
        return total;
    }
    return got;
}

void zSceneGameStatsTally_snack(unsigned int amount)
{
    unsigned int area = zSceneGetArea(globals.sceneCur->sceneID);
    globals.stats.snacks_tot += amount;
    globals.areaStats[area].snacks_tot += amount;
}

void zSceneGameStatsTally_token()
{
    unsigned int area = zSceneGetArea(globals.sceneCur->sceneID);
    globals.stats.tokens_tot++;
    globals.areaStats[area].tokens_tot++;
}

void zSceneGameStatsTally_scare()
{
    globals.stats.scares++;
    globals.stats.scares_sum++;
}

void zSceneGameStatsTally_death()
{
    globals.stats.deaths++;
    globals.stats.deaths_sum++;
}

void zSceneGameStatsTally_clue()
{
    char name[4] = {};
    globals.stats.clues++;
    strncpy(name, xStrupr(xUtil_idtag2string(globals.sceneCur->sceneID, 0)), 4);
    globals.levelStats[zSceneGetLevelIndex(name[0])].clues++;
}

signed int zRayHitsPlayer(_xRay3* ray)
{
    _xCollis coll;
    xRayHitsBound(ray, &globals.player.ent.bound, &coll);
    return coll.flags & 1;
}

unsigned int zSceneCalcProgress()
{
    unsigned int specials = 0;
    unsigned int snacks;
    unsigned int tokens;
    unsigned int mastermind = 0;
    signed int i;

    for (i = 0; i < 15; i++)
    {
        if (globals.player.Inv_Specials & (1 << i))
        {
            specials++;
        }
    }
    float pct_specials = specials / 13.0f;

    roomCnt = 0;
    xSerialTraverse(zSceneCalcProgress_serialTraverseCB);

    snacks = globals.stats.snacks_tot;
    float pct_rooms = roomCnt / 86.0f;
    float pct_snacks = snacks / 6748.0f;

    tokens = 0;
    for (i = 0; i < 21; i++)
    {
        if (globals.player.Inv_Mtokens[i >> 5] & (1 << (i & 31)))
        {
            tokens++;
        }
    }
    float pct_tokens = tokens / 21.0f;

    if (zVarEntryCB_eVareVarDefeatedMM())
    {
        mastermind = 2;
    }

    unsigned int pct = mastermind + (43.0f * pct_specials + 20.0f * pct_rooms +
                                     20.0f * pct_snacks + 15.0f * pct_tokens);
    if (pct == 0 && (specials || snacks || tokens))
    {
        pct++;
    }
    if (pct > 100)
    {
        pct = 100;
    }
    return pct;
}

signed int zSceneCalcProgress_serialTraverseCB(unsigned int clientID, xSerial* xser)
{
    signed int val = 0;
    if (clientID != 'PLYR')
    {
        xser->Read_b1(&val);
        if (val)
        {
            roomCnt++;
        }
    }
    return 1;
}

void zScene_UpdateFlyToInterface(float dt)
{
    zEntPickup_UpdateFlyToInterface((_zEntPickup*)globals.sceneCur->baseList[eBaseTypePickup],
                                    globals.sceneCur->baseCount[eBaseTypePickup], dt);
}

void zSceneStats_Clear()
{
    for (signed int i = 0; i < 30; i++)
    {
        globals.areaStats[i].tokens_tot = 0;
        globals.areaStats[i].snacks_tot = 0;
    }
}

extern "C" void fn_800CD268(signed int enable)
{
    sCardCheckEnabled = enable;
}

extern "C" void fn_800CD254()
{
    sCardCheckSaved0 = sCardCheck0;
    sCardCheckSaved1 = sCardCheck1;
}

extern "C" signed int fn_800CD24C()
{
    return sCardCheckEnabled;
}


// Is something (cutscene, pause, menu) blocking normal play? (GameCube only)
extern "C" signed int fn_800CD170()
{
    if (globals.sceneCur != NULL && globals.sceneCur->sceneID == SCENE_ID("h003") &&
        globals.player.ControlOn == 0)
    {
        return 1;
    }
    if (zMenuRunning())
    {
        return 1;
    }
    if (zGameIsPaused())
    {
        return 1;
    }
    if (sCardCheckActive != 0)
    {
        return 1;
    }
    return gGameState == 7;
}

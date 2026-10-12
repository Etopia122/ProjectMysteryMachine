// TODO: 99.1% (NonMatching). The remaining differences are register allocation:
// the inlined zHudInterp1d clamp evaluates fabs(step) and fabs(diff) into
// swapped float registers in the element update loops, zHudInterp2d keeps the
// squared length in a different register, zHud_Init does not reuse the loop
// counter's zero for the colour bytes, and the Setup callbacks swap the globals
// and item base registers. zHudAspectDeadStripped and zHudLayoutDeadStripped
// must be stripped by the linker once this unit is Matching.
#include "zGlobals.h"
#include "zScene.h"
#include "xMath.h"
#include "xMath3.h"
#include "xString.h"
#include "xstransvc.h"
#include "xFont.h"
#include "iDraw.h"
#include "xAnim.h"

#ifndef NULL
#define NULL 0
#endif

class xModelPool;

class _xVec2
{
public:
    float x; // 0x0
    float y; // 0x4
};

class RwIm2DVertex
{
public:
    float x;         // 0x0
    float y;         // 0x4
    float z;         // 0x8
    unsigned char r; // 0xC
    unsigned char g; // 0xD
    unsigned char b; // 0xE
    unsigned char a; // 0xF
    float u;         // 0x10
    float v;         // 0x14
    float w;         // 0x18
};

class RwCamera;

class RwGlobals
{
public:
    RwCamera* curCamera; // 0x0
};

extern "C" {
extern RwGlobals* RwEngineInstance;
signed int RwRenderStateSet(signed int state, void* value);
signed int RwIm2DRenderIndexedPrimitive(signed int primType, RwIm2DVertex* vertices,
                                        signed int numVertices, unsigned short* indices,
                                        signed int numIndices);
float RwIm2DGetNearScreenZ();
RwCamera* RwCameraClear(RwCamera* camera, void* color, signed int clearMode);
}

enum eHudDropShadowMode
{
    DROPSHADOW_NONE = 0,
    DROPSHADOW_NORMAL = 1
};

enum eHudInterpType
{
    eHudInterpTypeLinear = 0,
    eHudInterpTypeDecel = 1,
    eHudInterpTypeAccel = 2,
    eHudInterpTypeCount = 3
};

enum eHudElement
{
    eHudPlayerStatus = 0,
    eHudChances = 1,
    eHudSnacks = 2,
    eHudGum = 3,
    eHudSoap = 4,
    eHudMessage = 5,
    eHudGameStats = 6,
    eHudSoapbox = 7,
    eHudGumpack = 8,
    eHudHealthbar = 9,
    eHudCount = 10
};

enum eHudModel
{
    eHudModelBubblegumPiece = 0,
    eHudModelBubblegumPack = 1,
    eHudModelBubblegumMachine = 2,
    eHudModelPinkBunnySlippers = 3,
    eHudModelBarSoap = 4,
    eHudModelBoxSoap = 5,
    eHudModelBubblePowerUp = 6,
    eHudModelLampShade = 7,
    eHudModelFlowerPot = 8,
    eHudModelPlungerSuctionCup = 9,
    eHudModelBlackKnightHelmut = 10,
    eHudModelSpring = 11,
    eHudModelDivingHelmut = 12,
    eHudModelSkull = 13,
    eHudModelLightingBolt = 14,
    eHudModelFootballHelmut = 15,
    eHudModelUmbrella_closed = 16,
    eHudModelUmbrella_open = 17,
    eHudModelShovel = 18,
    eHudModelIndividualScoobySnack = 19,
    eHudModelScoobyCollar = 20,
    eHudModelMagnifyingGlass = 21,
    eHudModelHotSauce = 22,
    eHudModelSphere = 23,
    eHudModelHead0 = 24,
    eHudModelHead1 = 25,
    eHudModelHead2 = 26,
    eHudModelHead3 = 27,
    eHudModelHead4 = 28,
    eHudModelHead5 = 29,
    eHudModelSandwich1 = 30,
    eHudModelSandwich2 = 31,
    eHudModelSandwich3 = 32,
    eHudModelSandwich4 = 33,
    eHudModelSandwich5 = 34,
    eHudModelEnemyIcon = 35,
    eHudModelClue = 36,
    eHudModel_err = 37,
    eHudModelBubblegumPiece2 = 38,
    eHudModelBubblegumPiece3 = 39,
    eHudModelBubblegumPiece4 = 40,
    eHudModelBubblegumPiece5 = 41,
    eHudModelCount = 42
};

enum eHudStatusPos
{
    eHudStatusPosOnScreen = 0,
    eHudStatusPosOffScreenDefault = 1,
    eHudStatusPosOffScreenAlternate1 = 2,
    eHudStatusPosOffScreenAlternate2 = 3,
    eHudStatusPosNudgeUp = 4,
    eHudStatusPosNudgeDown = 5,
    eHudStatusPosNudgeLeft = 6,
    eHudStatusPosNudgeRight = 7,
    eHudStatusPosMiddleOfScreen = 8,
    eHudStatusPosOnScreenAlt1 = 9,
    eHudStatusPosCount = 10
};

enum eHudStatusSize
{
    eHudStatusSizeDefault = 0,
    eHudStatusSizeZero = 1,
    eHudStatusSizeSmall = 2,
    eHudStatusSizeBig = 3,
    eHudStatusSizeSquash = 4,
    eHudStatusSizeZeroHoriz = 5,
    eHudStatusSizeZeroVert = 6,
    eHudStatusSizeHyper = 7,
    eHudStatusSizeHyperReverse = 8,
    eHudStatusSizeCount = 9
};

enum eHudStatusAng
{
    eHudStatusAngStop = 0,
    eHudStatusAngIdle = 1,
    eHudStatusAngIdleReverse = 2,
    eHudStatusAngSlow = 3,
    eHudStatusAngSlowReverse = 4,
    eHudStatusAngFast = 5,
    eHudStatusAngFastReverse = 6,
    eHudStatusAngHyper = 7,
    eHudStatusAngHyperReverse = 8,
    eHudStatusAngTest1 = 9,
    eHudStatusAngTest2 = 10,
    eHudStatusAngCount = 11
};

enum eHudStatusColor
{
    eHudStatusColorSolid = 0,
    eHudStatusColorTransparent = 1,
    eHudStatusColorLightBlue = 2,
    eHudStatusColorOpaque1 = 3,
    eHudStatusColorOpaque2 = 4,
    eHudStatusColorOpaque3 = 5,
    eHudStatusColorCount = 6
};

class zHudColor
{
public:
    float rm; // 0x0
    float gm; // 0x4
    float bm; // 0x8
    float a;  // 0xC
};

class zHudFrame
{
public:
    _xVec3 pos;      // 0x0
    _xVec3 size;     // 0xC
    _xVec3 ang;      // 0x18
    _xVec3 dang;     // 0x24
    zHudColor color; // 0x30
};

class zHudStatusPos
{
public:
    float x; // 0x0
    float y; // 0x4
};

class zHudStatusAng
{
public:
    float x; // 0x0
    float y; // 0x4
    float z; // 0x8
};

class zHudStatusSize
{
public:
    float x; // 0x0
    float y; // 0x4
};

class zHudStatusColor
{
public:
    float rm;              // 0x0
    float gm;              // 0x4
    float bm;              // 0x8
    float a;               // 0xC
    eHudInterpType interp; // 0x10
};

class zHudItem
{
public:
    xModelInstance* model;                       // 0x0
    xAnimTable* atbl;                            // 0x4
    signed int renderType;                       // 0x8
    zHudFrame hframe;                            // 0xC
    eHudStatusPos statusPos;                     // 0x4C
    eHudStatusSize statusSize;                   // 0x50
    eHudStatusAng statusAng;                     // 0x54
    eHudStatusColor statusColor;                 // 0x58
    char text[64];                               // 0x5C
    signed int statusFX;                         // 0x9C
    float shake;                                 // 0xA0
    zHudStatusPos zHudStatusPosTable[10];        // 0xA4
    zHudStatusAng zHudStatusAngTable[11];        // 0xF4
    zHudStatusSize zHudStatusSizeTable[9];       // 0x178
    zHudStatusColor zHudStatusColorTable[6];     // 0x1C0
};

class _zHudElement
{
public:
    eHudElement hudType;                  // 0x0
    void (*hudInit)();                    // 0x4
    void (*hudSetup)(_xScene*);           // 0x8
    void (*hudUpdate)(_xScene*, float);   // 0xC
    void (*hudRender)();                  // 0x10
    unsigned int count;                   // 0x14
    char text[256];                       // 0x18
};

class zHudModel
{
public:
    eHudModel type;           // 0x0
    char* modelName;          // 0x4
    unsigned int modelNameID; // 0x8
};

enum _SDRenderState
{
    SDRS_Unknown = 0,
    SDRS_Unk1 = 1,
    SDRS_Unk7 = 7,
    SDRS_Force32 = 0x7FFFFFFF
};

void zRenderState(_SDRenderState next);
signed int zGameModeGet();
signed int xScrFxIsLetterbox();
void zEntEvent(char* to, unsigned int toEvent);

void zHudInterp2d(float dt, float* srcx, float* srcy, float dstx, float dsty, float speed,
                  signed int type);
void zHudHide();
void zHudShow();
void zHud_UIRender(char* asset, float xPos, float yPos, float w, float h, unsigned char alpha);
void zHud_DrawText(float x, float y, float w, float h, char* s, signed int just, float size,
                   unsigned char alpha, eHudDropShadowMode shadow);
void zHudElementsCB_PlayerStatusInit();
void zHudElementsCB_PlayerStatusSetup();
void zHudElementsCB_PlayerStatusUpdate(float dt);
void zHudElementsCB_PlayerStatusRender();
void zHudElementsCB_ChancesInit();
void zHudElementsCB_ChancesSetup();
void zHudElementsCB_ChancesUpdate(float dt);
void zHudElementsCB_ChancesRender();
void zHudElementsCB_SnacksInit();
void zHudElementsCB_SnacksSetup();
void zHudElementsCB_SnacksUpdate(float dt);
void zHudElementsCB_SnacksRender();
void zHudElementsCB_GumInit();
void zHudElementsCB_GumSetup();
void zHudElementsCB_GumUpdate(float dt);
void zHudElementsCB_GumRender();
void zHudElementsCB_SoapInit();
void zHudElementsCB_SoapSetup();
void zHudElementsCB_SoapUpdate(float dt);
void zHudElementsCB_SoapRender();
void zHudElementsCB_MessageInit();
void zHudElementsCB_MessageSetup();
void zHudElementsCB_MessageUpdate();
void zHudElementsCB_MessageRender();
void zHudElementsCB_GameStatsInit();
void zHudElementsCB_GameStatsSetup();
void zHudElementsCB_GameStatsUpdate(float dt);
void zHudElementsCB_GameStatsRender();
void zHudElementsCB_SoapboxInit();
void zHudElementsCB_SoapboxSetup();
void zHudElementsCB_SoapboxUpdate(float dt);
void zHudElementsCB_SoapboxRender();
void zHudElementsCB_GumpackInit();
void zHudElementsCB_GumpackSetup();
void zHudElementsCB_GumpackUpdate(float dt);
void zHudElementsCB_GumpackRender();
void zHudElementsCB_HealthbarInit();
void zHudElementsCB_HealthbarSetup();
void zHudElementsCB_HealthbarUpdate(float dt);
void zHudElementsCB_HealthbarRender();

static signed int sShakeSnackCounter;
static float widthAspect;
static float heightAspect;
static signed int sHudGameStatsTimer;
static float tmpRoomTime;
static float sHudDisplayTimer_plrStatus;
static float sHudDisplayTimer_snacks;
static float sHudDisplayTimer_gum;
static float sHudDisplayTimer_soap;
static float sHudDisplayTimer_healthBar;
static unsigned int oldHealthCnt;
static unsigned int oldHealthCnt2;
static signed int oldGumCnt;
static signed int oldGumCnt2;
static signed int oldSoapCnt;
static signed int oldSoapCnt2;
static float sHudDisplayTimer_allSnacksArea = -100.0f;
static float sHudDisplayTimer_saveDone = -100.0f;
static RwIm2DVertex Vert[4];
static unsigned short Idx[6] = { 0, 1, 2, 0, 2, 3 };
static zHudModel zHudModelTable[42] = {
    { eHudModelBubblegumPiece, "rhbg0001", 0 },
    { eHudModelBubblegumPack, "rbp00001", 0 },
    { eHudModelBubblegumMachine, "rbm00001", 0 },
    { eHudModelPinkBunnySlippers, "rbu00001", 0 },
    { eHudModelBarSoap, "rhbs0001", 0 },
    { eHudModelBoxSoap, "rhbo0001", 0 },
    { eHudModelBubblePowerUp, "rbb00001", 0 },
    { eHudModelLampShade, "rls00001", 0 },
    { eHudModelFlowerPot, "rfp00001", 0 },
    { eHudModelPlungerSuctionCup, "rps00001", 0 },
    { eHudModelBlackKnightHelmut, "rbk00001", 0 },
    { eHudModelSpring, "rsp00001", 0 },
    { eHudModelDivingHelmut, "rdh00001", 0 },
    { eHudModelSkull, "rsk00001", 0 },
    { eHudModelLightingBolt, "rlb00001", 0 },
    { eHudModelFootballHelmut, "rfh00001", 0 },
    { eHudModelUmbrella_closed, "rum00001", 0 },
    { eHudModelUmbrella_open, "rum00002", 0 },
    { eHudModelShovel, "rsh00001", 0 },
    { eHudModelIndividualScoobySnack, "rhss0001", 0 },
    { eHudModelScoobyCollar, "rhsc0003", 0 },
    { eHudModelMagnifyingGlass, "rma00001", 0 },
    { eHudModelHotSauce, "rhs00001", 0 },
    { eHudModelSphere, "sphere.dff", 0 },
    { eHudModelHead0, "rhsd0005", 0 },
    { eHudModelHead1, "rhsd0005", 0 },
    { eHudModelHead2, "rhsd0004", 0 },
    { eHudModelHead3, "rhsd0003", 0 },
    { eHudModelHead4, "rhsd0002", 0 },
    { eHudModelHead5, "rhsd0001", 0 },
    { eHudModelSandwich1, "rhsub_1", 0 },
    { eHudModelSandwich2, "rhsub_2", 0 },
    { eHudModelSandwich3, "rhsub_3", 0 },
    { eHudModelSandwich4, "rhsub_4", 0 },
    { eHudModelSandwich5, "rhsub_all", 0 },
    { eHudModelEnemyIcon, "rhss0001", 0 },
    { eHudModelClue, "rhtp0002", 0 },
    { eHudModel_err, "rhss0001", 0 },
    { eHudModelBubblegumPiece2, "rhbg0002", 0 },
    { eHudModelBubblegumPiece3, "rhbg0003", 0 },
    { eHudModelBubblegumPiece4, "rhbg0004", 0 },
    { eHudModelBubblegumPiece5, "rhbg0005", 0 },
};
static zHudItem sHudSnackArray[1];
static zHudItem sHudChancesArray[1];
static zHudItem sHudGumArray[5];
static zHudItem sHudSoapArray[6];
static zHudItem sHudSoapboxArray[1];
static zHudItem sHudGumpackArray[1];
static zHudItem sHudPlayerStatusArray[6];
static zHudItem sHudHealthbarArray[5];
static zHudItem sHudGameStatsArray[7];

static _zHudElement sHudElements[10] = {
    { eHudPlayerStatus, zHudElementsCB_PlayerStatusInit,
      (void (*)(_xScene*))zHudElementsCB_PlayerStatusSetup,
      (void (*)(_xScene*, float))zHudElementsCB_PlayerStatusUpdate,
      zHudElementsCB_PlayerStatusRender, globals.player.Health },
    { eHudChances, zHudElementsCB_ChancesInit, (void (*)(_xScene*))zHudElementsCB_ChancesSetup,
      (void (*)(_xScene*, float))zHudElementsCB_ChancesUpdate, zHudElementsCB_ChancesRender,
      globals.player.Inv_Chances },
    { eHudSnacks, zHudElementsCB_SnacksInit, (void (*)(_xScene*))zHudElementsCB_SnacksSetup,
      (void (*)(_xScene*, float))zHudElementsCB_SnacksUpdate, zHudElementsCB_SnacksRender,
      globals.player.Inv_ScoobySnacks },
    { eHudGum, zHudElementsCB_GumInit, (void (*)(_xScene*))zHudElementsCB_GumSetup,
      (void (*)(_xScene*, float))zHudElementsCB_GumUpdate, zHudElementsCB_GumRender,
      globals.player.Inv_Ammo[0] },
    { eHudSoap, zHudElementsCB_SoapInit, (void (*)(_xScene*))zHudElementsCB_SoapSetup,
      (void (*)(_xScene*, float))zHudElementsCB_SoapUpdate, zHudElementsCB_SoapRender,
      globals.player.Inv_Ammo[1] },
    { eHudMessage, zHudElementsCB_MessageInit, (void (*)(_xScene*))zHudElementsCB_MessageSetup,
      (void (*)(_xScene*, float))zHudElementsCB_MessageUpdate, zHudElementsCB_MessageRender },
    { eHudGameStats, zHudElementsCB_GameStatsInit,
      (void (*)(_xScene*))zHudElementsCB_GameStatsSetup,
      (void (*)(_xScene*, float))zHudElementsCB_GameStatsUpdate, zHudElementsCB_GameStatsRender },
    { eHudSoapbox, zHudElementsCB_SoapboxInit, (void (*)(_xScene*))zHudElementsCB_SoapboxSetup,
      (void (*)(_xScene*, float))zHudElementsCB_SoapboxUpdate, zHudElementsCB_SoapboxRender },
    { eHudGumpack, zHudElementsCB_GumpackInit, (void (*)(_xScene*))zHudElementsCB_GumpackSetup,
      (void (*)(_xScene*, float))zHudElementsCB_GumpackUpdate, zHudElementsCB_GumpackRender },
    { eHudHealthbar, zHudElementsCB_HealthbarInit,
      (void (*)(_xScene*))zHudElementsCB_HealthbarSetup,
      (void (*)(_xScene*, float))zHudElementsCB_HealthbarUpdate, zHudElementsCB_HealthbarRender },
};

static xFontBox* fontBox;
static iColor_tag clr = { 0xFF, 0xFF, 0x00, 0xC8 };
static iColor_tag clrDropShadow = { 0x52, 0x00, 0x8B, 0xC8 };
static float dynOffset;
static signed int sStatInit;

static inline unsigned int zHudModelGetID(unsigned int modelEnum)
{
    for (signed int i = 0; i < eHudModelCount; i++)
    {
        if (modelEnum == zHudModelTable[i].type)
        {
            return zHudModelTable[i].modelNameID;
        }
    }
    return 0;
}

#include "rwtypes.h"

class RpMaterial
{
public:
    void* texture; // 0x0
    RwRGBA color;  // 0x4
};

class RpMaterialList
{
public:
    RpMaterial** materials; // 0x0
    signed int numMaterials; // 0x4
    signed int space;        // 0x8
};

class RpGeometry
{
public:
    unsigned char pad0[0x20];
    RpMaterialList matList; // 0x20
};

class RpAtomic
{
public:
    unsigned char pad0[0x18];
    RpGeometry* geometry; // 0x18
};

class _xRect
{
public:
    float x; // 0x0
    float y; // 0x4
    float w; // 0x8
    float h; // 0xC
};

extern "C" {
void* memcpy(void* dst, const void* src, unsigned int n);
signed int sprintf(char* s, const char* format, ...);
void fn_80030494(signed int type, float alpha);
}

void zEntPlayer_Notify(signed int situation);

void xMat3x3Euler(_xMat3x3* m, float yaw, float pitch, float roll);

void xModelSetMaterialAlpha(xModelInstance* model, signed int alpha);

void xModelRender2D(xModelInstance* model, _xRect* r, _xVec3* from, _xVec3* to, float rot);

xModelInstance* zEntRecurseModelInfo(void* info, _xEnt* ent);

xModelInstance* xEntLoadModel(_xEnt* ent, RpAtomic* model);

class xAnimSingle
{
public:
    unsigned int SingleFlags; // 0x0
    xAnimState* State;        // 0x4
    float Time;               // 0x8
    float CurrentSpeed;       // 0xC
};

class xAnimTable
{
public:
    xAnimTable* Next;                // 0x0
    char* Name;                      // 0x4
    xAnimTransition* TransitionList; // 0x8
    xAnimState* StateList;           // 0xC
};

class xAnimPlay
{
public:
    xAnimPlay* Next;          // 0x0
    unsigned short NumSingle; // 0x4
    unsigned short BoneCount; // 0x6
    xAnimSingle* Single;      // 0x8
    void* Object;             // 0xC
    xAnimTable* Table;        // 0x10
};

signed int zGameStateGet();
void zGameStateSwitch(signed int theNewState);

void xScrFxFade(iColor_tag* base, iColor_tag* dest, float seconds, void (*callback)(),
                signed int hold, signed int unused);

void zEntEventAllOfType(unsigned int toEvent, unsigned int type);

void xModelUpdate(xModelInstance* model, float dt);

void xModelEval(xModelInstance* model);

void xAnimPlaySetState(xAnimSingle* single, xAnimState* state, float startTime);

xAnimPlay* xAnimPoolAlloc(xMemPool* pool, void* object, xAnimTable* table,
                          xModelInstance* modelInst);

static inline void zHudInterp1d(float dt, float* src, float dst, float speed, signed int type)
{
    float diff = dst - *src;
    if (diff > 0.0f)
    {
        *src += ((float)__fabs(speed * dt) > (float)__fabs(diff)) ? diff : speed * dt;
    }
    else
    {
        float ad = __fabs(diff);
        *src -= ((float)__fabs(speed * dt) > ad) ? ad : speed * dt;
    }
}

static inline xModelInstance* zHudLoadModel(unsigned int modelEnum)
{
    unsigned int size;
    void* data = xSTFindAsset(zHudModelGetID(modelEnum), &size);
    if (data == NULL)
    {
        data = xSTFindAsset(zHudModelGetID(eHudModel_err), &size);
    }
    if (*(unsigned int*)data == 'FNIM')
    {
        return zEntRecurseModelInfo(data, NULL);
    }
    return xEntLoadModel(NULL, (RpAtomic*)data);
}

void zHud_Init()
{
    widthAspect = 1.3333334f;
    heightAspect = 0.75f;
    clr.r = 0xFF;
    clr.g = 0xFF;
    clr.b = 0x00;
    clr.a = 0xC8;
    clrDropShadow.r = 0x52;
    clrDropShadow.g = 0x00;
    clrDropShadow.b = 0x8B;
    clrDropShadow.a = 0xC8;

    for (signed int i = 0; i < eHudModelCount; i++)
    {
        zHudModelTable[i].modelNameID = xStrHash(zHudModelTable[i].modelName);
    }

    for (signed int i = 0; i < eHudCount; i++)
    {
        sHudElements[i].hudInit();
    }

    sShakeSnackCounter = 0;
}

#ifdef PC_PORT
// The port's renderer: a second screen for the HUD (the New 3DS bottom screen).
extern "C" int pc_render_hud_second_screen();
extern "C" void pc_render_hud_screen(int on);
extern "C" int pc_platform_3ds(void);
extern "C" int pc_touch_read(int* x, int* y);
signed int zEntPickup_PCWarpAvailable();
signed int zEntPickup_PCWarpOpen();

// The New 3DS's warp button, on the bottom screen under the HUD (its 640x480
// space): touching it brings up the warp gate screen, once the player has
// walked by a warp gate (zEntPickup); the gates themselves no longer open it.
static const float sPCWarpX = 240.0f;
static const float sPCWarpY = 286.0f;
static const float sPCWarpW = 160.0f;
static const float sPCWarpH = 170.0f;
static signed int sPCWarpTouch; // touching last frame

static signed int zHud_PCWarpShown()
{
    return pc_platform_3ds() && pc_render_hud_second_screen() && zGameModeGet() == 11 && zGameStateGet() == 1 &&
           globals.cmgr == NULL && !xScrFxIsLetterbox() && globals.sceneCur != NULL &&
           globals.sceneCur->pendingPortal == NULL && zEntPickup_PCWarpAvailable();
}

static void zHud_PCWarpUpdate()
{
    int x, y;
    signed int touch = pc_touch_read(&x, &y);
    // A new touch inside the button.
    if (touch && !sPCWarpTouch && zHud_PCWarpShown() && x >= sPCWarpX && x < sPCWarpX + sPCWarpW &&
        y >= sPCWarpY && y < sPCWarpY + sPCWarpH)
    {
        zEntPickup_PCWarpOpen();
    }
    sPCWarpTouch = touch;
}

static void zHud_PCWarpRender()
{
    if (!zHud_PCWarpShown())
    {
        return;
    }
    if (xSTFindAsset(xStrHash("warp_icon"), NULL) != NULL)
    {
        zHud_UIRender("warp_icon", sPCWarpX + 24.0f, sPCWarpY, 112.0f, 112.0f, 0xFF);
    }
    zHud_DrawText(sPCWarpX - 40.0f, sPCWarpY + 120.0f, sPCWarpW + 80.0f, 48.0f, "WARP", 0x40, 1.0f, 0xFF,
                  DROPSHADOW_NORMAL);
}
#endif

void zHud_Setup(_xScene* sc)
{
    for (signed int i = 0; i < eHudCount; i++)
    {
        sHudElements[i].hudSetup(sc);
    }
}

void zHud_Update(_xScene* sc, float dt)
{
    for (signed int i = 0; i < eHudCount; i++)
    {
#ifdef PC_PORT
        // The update callbacks take only (float dt); on PowerPC the float arrives in f1
        // either way, on x86 it must be the first stack argument.
        ((void (*)(float))sHudElements[i].hudUpdate)(dt);
#else
        sHudElements[i].hudUpdate(sc, dt);
#endif
    }

    if (globals.pad0->on & 0x40000)
    {
        zHudShow();
    }

#ifdef PC_PORT
    // On its own screen the HUD stays up, as if Y were held (it still hides for
    // cutscenes below).
    if (pc_render_hud_second_screen())
    {
        zHudShow();
    }
    zHud_PCWarpUpdate();
#endif

    if (zGameModeGet() == 2 || xScrFxIsLetterbox() || globals.cmgr != NULL)
    {
        zHudHide();
    }

    if (zGameModeGet() == 7)
    {
        zHudShow();
    }

    if (sHudDisplayTimer_allSnacksArea < 0.0f && sHudDisplayTimer_allSnacksArea >= -10.0f)
    {
        zEntEvent("all snacks uif", 4);
        sHudDisplayTimer_allSnacksArea = -100.0f;
    }
    else if (sHudDisplayTimer_allSnacksArea == 2.5f)
    {
        zEntEvent("all snacks uif", 3);
    }
    sHudDisplayTimer_allSnacksArea -= dt;

    if (sHudDisplayTimer_saveDone < 0.0f && sHudDisplayTimer_saveDone >= -10.0f)
    {
        zEntEvent("save ok uif", 4);
        sHudDisplayTimer_saveDone = -100.0f;
    }
    else if (sHudDisplayTimer_saveDone == 1.5f)
    {
        zEntEvent("save ok uif", 3);
    }
    sHudDisplayTimer_saveDone -= dt;
}

void zHud_Render()
{
#ifdef PC_PORT
    pc_render_hud_screen(1);
#endif
    RwCameraClear(RwEngineInstance->curCamera, NULL, 2);
    for (signed int i = 0; i < eHudCount; i++)
    {
        sHudElements[i].hudRender();
    }
#ifdef PC_PORT
    zHud_PCWarpRender();
    pc_render_hud_screen(0);
#endif
}

void zHudElementsCB_PlayerStatusInit()
{
    unsigned int size;
    void* data;

    sHudPlayerStatusArray[0].model = zHudLoadModel(eHudModelHead0);
    sHudPlayerStatusArray[0].renderType = 1;
    sHudPlayerStatusArray[1].model = zHudLoadModel(eHudModelHead1);
    sHudPlayerStatusArray[1].renderType = 2;
    sHudPlayerStatusArray[2].model = zHudLoadModel(eHudModelHead2);
    sHudPlayerStatusArray[2].renderType = 3;
    sHudPlayerStatusArray[3].model = zHudLoadModel(eHudModelHead3);
    sHudPlayerStatusArray[3].renderType = 4;
    sHudPlayerStatusArray[4].model = zHudLoadModel(eHudModelHead4);
    sHudPlayerStatusArray[4].renderType = 5;
    sHudPlayerStatusArray[5].model = zHudLoadModel(eHudModelHead5);
    sHudPlayerStatusArray[5].renderType = 6;

    for (signed int i = 0; i < 6; i++)
    {
        zHudItem* item = &sHudPlayerStatusArray[i];

        switch (i)
        {
        case 0:
            data = xSTFindAsset(xStrHash("rhsd0001.anm"), &size);
            break;
        case 1:
            data = xSTFindAsset(xStrHash("rhsd0001.anm"), &size);
            break;
        case 2:
            data = xSTFindAsset(xStrHash("rhsd0001.anm"), &size);
            break;
        case 3:
            data = xSTFindAsset(xStrHash("rhsd0001.anm"), &size);
            break;
        case 4:
            data = xSTFindAsset(xStrHash("rhsd0001.anm"), &size);
            break;
        case 5:
            data = xSTFindAsset(xStrHash("rhsd0001.anm"), &size);
            break;
        }

        if (data != NULL)
        {
            xAnimFile* afile = xAnimFileNew(data, "", 0, NULL);
            item->atbl = xAnimTableNew("", NULL);
            xAnimTableNewState(item->atbl, "Idle", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
            xAnimTableAddFile(item->atbl, afile, "Idle");
            xAnimPoolAlloc(&globals.sceneCur->mempool, item, item->atbl, item->model);
        }
    }
}

void zHudElementsCB_PlayerStatusSetup()
{
    sHudDisplayTimer_plrStatus = 2.0f;
    oldHealthCnt = globals.player.Health;

    for (signed int i = 0; i < 6; i++)
    {
        zHudItem* item = &sHudPlayerStatusArray[i];

        item->zHudStatusPosTable[eHudStatusPosOnScreen].x = 44.0f;
        item->zHudStatusPosTable[eHudStatusPosOnScreen].y = 65.0f;
        item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].x = -44.0f;
        item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].y = -35.0f;
        item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].x = 100.0f;
        item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].y = 300.0f;
        item->zHudStatusSizeTable[eHudStatusSizeDefault].x = 80.0f;
        item->zHudStatusSizeTable[eHudStatusSizeDefault].y = 80.0f;
        item->zHudStatusSizeTable[eHudStatusSizeZero].x = 0.0f;
        item->zHudStatusSizeTable[eHudStatusSizeZero].y = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngStop].x = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngStop].y = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngStop].z = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngIdle].x = 1.5915494f;
        item->zHudStatusAngTable[eHudStatusAngIdle].y = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngIdle].z = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngTest1].x = 1.5915494f;
        item->zHudStatusAngTable[eHudStatusAngTest1].y = 15.915494f;
        item->zHudStatusAngTable[eHudStatusAngTest1].z = 0.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].rm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].gm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].bm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].a = 255.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].rm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].gm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].bm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].a = 0.0f;
        item->statusPos = eHudStatusPosOnScreen;
        item->statusAng = eHudStatusAngStop;
        item->statusSize = eHudStatusSizeDefault;
        item->statusColor = eHudStatusColorSolid;
        item->hframe.pos.x = item->zHudStatusPosTable[eHudStatusPosOnScreen].x;
        item->hframe.pos.y = item->zHudStatusPosTable[eHudStatusPosOnScreen].y;
        item->hframe.dang.x = item->zHudStatusAngTable[eHudStatusAngStop].x;
        item->hframe.dang.y = item->zHudStatusAngTable[eHudStatusAngStop].y;
        item->hframe.dang.z = item->zHudStatusAngTable[eHudStatusAngStop].z;
        item->hframe.size.x = item->zHudStatusSizeTable[eHudStatusSizeDefault].x;
        item->hframe.size.y = item->zHudStatusSizeTable[eHudStatusSizeDefault].y;
        item->hframe.color.a = item->zHudStatusColorTable[eHudStatusColorTransparent].a;

        if (item->model->Anim != NULL)
        {
            item->model->Anim->Single->Time = 0.0f;
            item->model->Anim->Single->CurrentSpeed = 1.0f;
            xAnimPlaySetState(item->model->Anim->Single, item->model->Anim->Table->StateList,
                              0.0f);
        }
    }
}

void zHudElementsCB_PlayerStatusUpdate(float dt)
{
    signed int i;

    if ((signed int)globals.player.Health < 6 && (signed int)globals.player.Health >= 0)
    {
        for (unsigned int j = 0; j < 6; j++)
        {
            if (j == globals.player.Health)
            {
                sHudPlayerStatusArray[globals.player.Health].statusSize = eHudStatusSizeDefault;
            }
            else
            {
                sHudPlayerStatusArray[j].statusSize = eHudStatusSizeZero;
            }
        }
    }

    if (globals.player.Health != oldHealthCnt)
    {
        oldHealthCnt = globals.player.Health;
        sHudDisplayTimer_plrStatus = 2.0f;
    }

    if (sHudDisplayTimer_plrStatus == 0.0f)
    {
        for (i = 0; i < 6; i++)
        {
            sHudPlayerStatusArray[i].statusColor = eHudStatusColorTransparent;
            sHudPlayerStatusArray[i].statusPos = eHudStatusPosOffScreenDefault;
        }
    }
    else
    {
        for (i = 0; i < 6; i++)
        {
            sHudPlayerStatusArray[i].statusColor = eHudStatusColorSolid;
            sHudPlayerStatusArray[i].statusPos = eHudStatusPosOnScreen;
        }
        sHudDisplayTimer_plrStatus -= dt;
        if (sHudDisplayTimer_plrStatus < 0.0f)
        {
            sHudDisplayTimer_plrStatus = 0.0f;
        }
    }

    for (i = 0; i < 6; i++)
    {
        zHudItem* item = &sHudPlayerStatusArray[i];

        switch (item->statusPos)
        {
        case eHudStatusPosOnScreen:
            zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                         item->zHudStatusPosTable[eHudStatusPosOnScreen].x,
                         item->zHudStatusPosTable[eHudStatusPosOnScreen].y, 1000.0f,
                         eHudInterpTypeDecel);
            break;
        case eHudStatusPosOffScreenDefault:
            zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                         item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].x,
                         item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].y, 750.0f,
                         eHudInterpTypeDecel);
            break;
        case eHudStatusPosMiddleOfScreen:
            zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                         item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].x,
                         item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].y, 250.0f,
                         eHudInterpTypeDecel);
            break;
        }

        switch (item->statusAng)
        {
        case eHudStatusAngStop:
            zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngStop].x,
                         1.0f, eHudInterpTypeLinear);
            break;
        case eHudStatusAngIdle:
            zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngIdle].x,
                         1.0f, eHudInterpTypeLinear);
            break;
        case eHudStatusAngTest1:
            zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngTest1].x,
                         1.0f, eHudInterpTypeLinear);
            zHudInterp1d(dt, &item->hframe.dang.y, item->zHudStatusAngTable[eHudStatusAngTest1].y,
                         1.0f, eHudInterpTypeLinear);
            zHudInterp1d(dt, &item->hframe.dang.z, item->zHudStatusAngTable[eHudStatusAngTest1].z,
                         1.0f, eHudInterpTypeLinear);
            break;
        case eHudStatusAngTest2:
            break;
        }

        switch (item->statusSize)
        {
        case eHudStatusSizeDefault:
            item->hframe.size.x = item->zHudStatusSizeTable[eHudStatusSizeDefault].x;
            item->hframe.size.y = item->zHudStatusSizeTable[eHudStatusSizeDefault].y;
            break;
        case eHudStatusSizeZero:
            item->hframe.size.x = item->zHudStatusSizeTable[eHudStatusSizeZero].x;
            item->hframe.size.y = item->zHudStatusSizeTable[eHudStatusSizeZero].y;
            break;
        }

        switch (item->statusColor)
        {
        case eHudStatusColorSolid:
            zHudInterp1d(dt, &item->hframe.color.a,
                         item->zHudStatusColorTable[eHudStatusColorSolid].a, 600.0f,
                         eHudInterpTypeLinear);
            break;
        case eHudStatusColorTransparent:
            zHudInterp1d(dt, &item->hframe.color.a,
                         item->zHudStatusColorTable[eHudStatusColorTransparent].a, 300.0f,
                         eHudInterpTypeLinear);
            break;
        }

        item->hframe.ang.x = item->hframe.dang.x * dt + item->hframe.ang.x;
        item->hframe.ang.y = item->hframe.dang.y * dt + item->hframe.ang.y;
        item->hframe.ang.z = item->hframe.dang.z * dt + item->hframe.ang.z;

        xModelUpdate(item->model, dt);
        xModelEval(item->model);
    }
}

void zHud_showHealthandStatus()
{
    sHudDisplayTimer_plrStatus = 2.0f;
    sHudDisplayTimer_healthBar = 2.0f;
}

void zHudElementsCB_PlayerStatusRender()
{
    zHud_UIRender("scooby_head_bg", sHudPlayerStatusArray[0].hframe.pos.x + -30.0f,
                  sHudPlayerStatusArray[0].hframe.pos.y + -50.0f, 65.0f * widthAspect, 85.0f,
                  sHudPlayerStatusArray[0].hframe.color.a);
    zRenderState(SDRS_Unk7);

    for (signed int i = 0; i < 6; i++)
    {
        zHudItem* item = &sHudPlayerStatusArray[i];
        if (item->hframe.color.a != 0.0f)
        {
            _xRect r = { 0.0f, 0.0f, 0.0f, 0.0f };
            _xMat4x3 frame;

            r.x = -(0.5f * item->hframe.size.x - item->hframe.pos.x);
            r.y = -(0.5f * item->hframe.size.y - item->hframe.pos.y);
            r.w = item->hframe.size.x * widthAspect;
            r.h = item->hframe.size.y;
            _xVec3 unused = { 0.0f, 0.0f, 0.0f };
            _xVec3 from = { -0.8f, 0.22f, 0.8f };
            _xVec3 to = { 0.0f, 0.0f, 0.0f };

            xMat3x3Euler((_xMat3x3*)&frame, item->hframe.ang.x, item->hframe.ang.y,
                         item->hframe.ang.z);
            frame.pos.x = 0.0f;
            frame.pos.y = 0.0f;
            frame.pos.z = 0.0f;
            frame.flags = 0;
            xModelSetMaterialAlpha(item->model, item->hframe.color.a);
            memcpy(item->model->Mat, &frame, sizeof(_xMat4x3));
            fn_80030494(item->renderType, item->hframe.color.a);
            xModelRender2D(item->model, &r, &from, &to, 1.5707964f);
        }
    }
}

void zHudElementsCB_ChancesInit()
{
}

void zHudElementsCB_ChancesSetup()
{
    sHudChancesArray[0].zHudStatusPosTable[eHudStatusPosOnScreen].x = 75.0f;
    sHudChancesArray[0].zHudStatusPosTable[eHudStatusPosOnScreen].y = 40.0f;
    sHudChancesArray[0].zHudStatusSizeTable[eHudStatusSizeDefault].x = 150.0f;
    sHudChancesArray[0].zHudStatusSizeTable[eHudStatusSizeDefault].y = 150.0f;
    sHudChancesArray[0].zHudStatusAngTable[eHudStatusAngStop].x = 0.0f;
    sHudChancesArray[0].zHudStatusAngTable[eHudStatusAngStop].y = 0.0f;
    sHudChancesArray[0].zHudStatusAngTable[eHudStatusAngStop].z = 0.0f;
    sHudChancesArray[0].zHudStatusAngTable[eHudStatusAngIdle].x = 1.5915494f;
    sHudChancesArray[0].zHudStatusAngTable[eHudStatusAngIdle].y = 0.0f;
    sHudChancesArray[0].zHudStatusAngTable[eHudStatusAngIdle].z = 0.0f;
    sHudChancesArray[0].zHudStatusAngTable[eHudStatusAngTest1].x = 1.5915494f;
    sHudChancesArray[0].zHudStatusAngTable[eHudStatusAngTest1].y = 15.915494f;
    sHudChancesArray[0].zHudStatusAngTable[eHudStatusAngTest1].z = 0.0f;
    sHudChancesArray[0].statusPos = eHudStatusPosOnScreen;
    sHudChancesArray[0].statusAng = eHudStatusAngIdle;
    sHudChancesArray[0].statusSize = eHudStatusSizeDefault;
    sHudChancesArray[0].hframe.pos.x = 75.0f;
    sHudChancesArray[0].hframe.pos.y = 40.0f;
    sHudChancesArray[0].hframe.dang.x = 0.0f;
    sHudChancesArray[0].hframe.dang.y = 0.0f;
    sHudChancesArray[0].hframe.dang.z = 0.0f;
    sHudChancesArray[0].hframe.size.x = 150.0f;
    sHudChancesArray[0].hframe.size.y = 150.0f;
}

void zHudElementsCB_ChancesUpdate(float dt)
{
    zHudItem* item = &sHudChancesArray[0];

    switch (item->statusPos)
    {
    case eHudStatusPosOnScreen:
        zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                     item->zHudStatusPosTable[eHudStatusPosOnScreen].x,
                     item->zHudStatusPosTable[eHudStatusPosOnScreen].y, 250.0f,
                     eHudInterpTypeDecel);
        break;
    case eHudStatusPosMiddleOfScreen:
        zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                     item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].x,
                     item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].y, 250.0f,
                     eHudInterpTypeDecel);
        break;
    }

    switch (item->statusAng)
    {
    case eHudStatusAngStop:
        zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngStop].x, 1.0f,
                     eHudInterpTypeLinear);
        break;
    case eHudStatusAngIdle:
        zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngIdle].x, 1.0f,
                     eHudInterpTypeLinear);
        break;
    case eHudStatusAngTest1:
        zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngTest1].x,
                     1.0f, eHudInterpTypeLinear);
        zHudInterp1d(dt, &item->hframe.dang.y, item->zHudStatusAngTable[eHudStatusAngTest1].y,
                     1.0f, eHudInterpTypeLinear);
        zHudInterp1d(dt, &item->hframe.dang.z, item->zHudStatusAngTable[eHudStatusAngTest1].z,
                     1.0f, eHudInterpTypeLinear);
        break;
    case eHudStatusAngTest2:
        break;
    }

    switch (item->statusSize)
    {
    case eHudStatusSizeDefault:
        zHudInterp1d(dt, &item->hframe.size.x, item->zHudStatusSizeTable[eHudStatusSizeDefault].x,
                     15.0f, eHudInterpTypeLinear);
        zHudInterp1d(dt, &item->hframe.size.y, item->zHudStatusSizeTable[eHudStatusSizeDefault].y,
                     10.0f, eHudInterpTypeLinear);
        break;
    case eHudStatusSizeZero:
    case eHudStatusSizeSmall:
        break;
    }

    item->hframe.ang.x = item->hframe.dang.x * dt + item->hframe.ang.x;
    item->hframe.ang.y = item->hframe.dang.y * dt + item->hframe.ang.y;
    item->hframe.ang.z = item->hframe.dang.z * dt + item->hframe.ang.z;
}

void zHudElementsCB_ChancesRender()
{
}

void zHudElementsCB_SnacksInit()
{
    unsigned int size;
    sHudSnackArray[0].model = zHudLoadModel(eHudModelIndividualScoobySnack);
    sHudSnackArray[0].renderType = 9;
}

void zHudElementsCB_SnacksSetup()
{
    sHudDisplayTimer_snacks = 2.0f;
    sHudSnackArray[0].zHudStatusPosTable[eHudStatusPosOnScreen].x = 520.0f;
    sHudSnackArray[0].zHudStatusPosTable[eHudStatusPosOnScreen].y = 32.0f;
    sHudSnackArray[0].zHudStatusPosTable[eHudStatusPosOnScreenAlt1].x = 496.0f;
    sHudSnackArray[0].zHudStatusPosTable[eHudStatusPosOnScreenAlt1].y = 32.0f;
    sHudSnackArray[0].zHudStatusPosTable[eHudStatusPosOffScreenDefault].x = 600.0f;
    sHudSnackArray[0].zHudStatusPosTable[eHudStatusPosOffScreenDefault].y = -68.0f;
    sHudSnackArray[0].zHudStatusSizeTable[eHudStatusSizeDefault].x = 70.0f;
    sHudSnackArray[0].zHudStatusSizeTable[eHudStatusSizeDefault].y = 70.0f;
    sHudSnackArray[0].zHudStatusAngTable[eHudStatusAngStop].x = 0.0f;
    sHudSnackArray[0].zHudStatusAngTable[eHudStatusAngStop].y = 0.0f;
    sHudSnackArray[0].zHudStatusAngTable[eHudStatusAngStop].z = 0.0f;
    sHudSnackArray[0].zHudStatusAngTable[eHudStatusAngIdle].x = 1.5915494f;
    sHudSnackArray[0].zHudStatusAngTable[eHudStatusAngIdle].y = 0.0f;
    sHudSnackArray[0].zHudStatusAngTable[eHudStatusAngIdle].z = 0.0f;
    sHudSnackArray[0].zHudStatusAngTable[eHudStatusAngTest1].x = 1.5915494f;
    sHudSnackArray[0].zHudStatusAngTable[eHudStatusAngTest1].y = 15.915494f;
    sHudSnackArray[0].zHudStatusAngTable[eHudStatusAngTest1].z = 0.0f;
    sHudSnackArray[0].zHudStatusColorTable[eHudStatusColorSolid].rm = 1.0f;
    sHudSnackArray[0].zHudStatusColorTable[eHudStatusColorSolid].gm = 1.0f;
    sHudSnackArray[0].zHudStatusColorTable[eHudStatusColorSolid].bm = 1.0f;
    sHudSnackArray[0].zHudStatusColorTable[eHudStatusColorSolid].a = 255.0f;
    sHudSnackArray[0].zHudStatusColorTable[eHudStatusColorTransparent].rm = 1.0f;
    sHudSnackArray[0].zHudStatusColorTable[eHudStatusColorTransparent].gm = 1.0f;
    sHudSnackArray[0].zHudStatusColorTable[eHudStatusColorTransparent].bm = 1.0f;
    sHudSnackArray[0].zHudStatusColorTable[eHudStatusColorTransparent].a = 0.0f;
    sHudSnackArray[0].statusPos = eHudStatusPosOnScreen;
    sHudSnackArray[0].statusAng = eHudStatusAngIdle;
    sHudSnackArray[0].statusSize = eHudStatusSizeDefault;
    sHudSnackArray[0].statusColor = eHudStatusColorSolid;
    sHudSnackArray[0].hframe.pos.x = 520.0f;
    sHudSnackArray[0].hframe.pos.y = 32.0f;
    sHudSnackArray[0].hframe.dang.x = 1.5915494f;
    sHudSnackArray[0].hframe.dang.y = 0.0f;
    sHudSnackArray[0].hframe.dang.z = 0.0f;
    sHudSnackArray[0].hframe.size.x = 70.0f;
    sHudSnackArray[0].hframe.size.y = 70.0f;
    sHudSnackArray[0].hframe.color.rm = 1.0f;
    sHudSnackArray[0].hframe.color.gm = 1.0f;
    sHudSnackArray[0].hframe.color.bm = 1.0f;
    sHudSnackArray[0].hframe.color.a = 0.0f;
}

void zHudElementsCB_SnacksUpdate(float dt)
{
    zHudItem* item = &sHudSnackArray[0];

    if (zGameModeGet() == 7)
    {
        item->statusAng = eHudStatusAngStop;
    }
    else
    {
        item->statusAng = eHudStatusAngIdle;
    }

    if (sHudDisplayTimer_snacks == 0.0f)
    {
        sHudSnackArray[0].statusColor = eHudStatusColorTransparent;
        sHudSnackArray[0].statusPos = eHudStatusPosOffScreenDefault;
    }
    else
    {
        if (globals.player.Inv_ScoobySnacks > 999)
        {
            sHudSnackArray[0].statusColor = eHudStatusColorSolid;
            sHudSnackArray[0].statusPos = eHudStatusPosOnScreenAlt1;
        }
        else
        {
            sHudSnackArray[0].statusColor = eHudStatusColorSolid;
            sHudSnackArray[0].statusPos = eHudStatusPosOnScreen;
        }
        sHudDisplayTimer_snacks -= dt;
        if (sHudDisplayTimer_snacks < 0.0f)
        {
            sHudDisplayTimer_snacks = 0.0f;
        }
    }

    switch (item->statusPos)
    {
    case eHudStatusPosOnScreen:
        zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                     item->zHudStatusPosTable[eHudStatusPosOnScreen].x,
                     item->zHudStatusPosTable[eHudStatusPosOnScreen].y, 1000.0f,
                     eHudInterpTypeDecel);
        break;
    case eHudStatusPosOffScreenDefault:
        zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                     item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].x,
                     item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].y, 750.0f,
                     eHudInterpTypeDecel);
        break;
    case eHudStatusPosOnScreenAlt1:
        zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                     item->zHudStatusPosTable[eHudStatusPosOnScreenAlt1].x,
                     item->zHudStatusPosTable[eHudStatusPosOnScreenAlt1].y, 750.0f,
                     eHudInterpTypeDecel);
        break;
    case eHudStatusPosMiddleOfScreen:
        zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                     item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].x,
                     item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].y, 250.0f,
                     eHudInterpTypeDecel);
        break;
    }

    switch (item->statusAng)
    {
    case eHudStatusAngStop:
        zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngStop].x, 1.0f,
                     eHudInterpTypeLinear);
        break;
    case eHudStatusAngIdle:
        zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngIdle].x, 1.0f,
                     eHudInterpTypeLinear);
        break;
    case eHudStatusAngTest1:
        zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngTest1].x,
                     1.0f, eHudInterpTypeLinear);
        zHudInterp1d(dt, &item->hframe.dang.y, item->zHudStatusAngTable[eHudStatusAngTest1].y,
                     1.0f, eHudInterpTypeLinear);
        zHudInterp1d(dt, &item->hframe.dang.z, item->zHudStatusAngTable[eHudStatusAngTest1].z,
                     1.0f, eHudInterpTypeLinear);
        break;
    }

    switch (item->statusSize)
    {
    case eHudStatusSizeDefault:
        zHudInterp1d(dt, &item->hframe.size.x, item->zHudStatusSizeTable[eHudStatusSizeDefault].x,
                     10.0f, eHudInterpTypeLinear);
        zHudInterp1d(dt, &item->hframe.size.y, item->zHudStatusSizeTable[eHudStatusSizeDefault].y,
                     10.0f, eHudInterpTypeLinear);
        break;
    }

    switch (item->statusColor)
    {
    case eHudStatusColorSolid:
        zHudInterp1d(dt, &item->hframe.color.a,
                     item->zHudStatusColorTable[eHudStatusColorSolid].a, 600.0f,
                     eHudInterpTypeLinear);
        break;
    case eHudStatusColorTransparent:
        zHudInterp1d(dt, &item->hframe.color.a,
                     item->zHudStatusColorTable[eHudStatusColorTransparent].a, 300.0f,
                     eHudInterpTypeLinear);
        break;
    }

    item->hframe.ang.x = item->hframe.dang.x * dt + item->hframe.ang.x;
    item->hframe.ang.y = item->hframe.dang.y * dt + item->hframe.ang.y;
    item->hframe.ang.z = item->hframe.dang.z * dt + item->hframe.ang.z;

    if (sShakeSnackCounter)
    {
        item->hframe.pos.x += item->shake;
        item->shake *= -0.8f;
    }
}

void zHud_ShakeSnackCounter()
{
    sShakeSnackCounter = 1;
    sHudSnackArray[0].shake = 5.0f;
}

void zHud_showSnack()
{
    sHudDisplayTimer_snacks = 2.0f;
}

void zHud_showSnackSpecial()
{
    sHudDisplayTimer_snacks = 3.0f;
}

void zHudShowAllSnacksArea()
{
    sHudDisplayTimer_allSnacksArea = 2.5f;
    zEntPlayer_Notify(0x20);
}

void zHudShowSaveSuccess()
{
    sHudDisplayTimer_saveDone = 1.5f;
}

void zHudElementsCB_SnacksRender()
{
    char str[256] = {};
    sprintf(str, "x%d", globals.player.Inv_ScoobySnacks);

    zHudItem* item = &sHudSnackArray[0];
    zHud_DrawText(item->hframe.pos.x, item->hframe.pos.y, 150.0f, 100.0f, str, 0x10, 1.0f,
                  item->hframe.color.a, DROPSHADOW_NORMAL);

    _xRect r = { 0.0f, 0.0f, 0.0f, 0.0f };
    _xMat4x3 frame;

    r.x = -(0.5f * item->hframe.size.x - item->hframe.pos.x) + -45.0f;
    r.w = item->hframe.size.x * widthAspect;
    r.y = -(0.5f * item->hframe.size.y - item->hframe.pos.y) + 22.0f;
    r.h = item->hframe.size.y;
    _xVec3 unused = { 0.0f, 0.0f, 0.0f };
    _xVec3 from = { 0.0f, 0.0f, 0.8f };
    _xVec3 to = { 0.0f, 0.0f, 0.0f };

    xMat3x3Euler((_xMat3x3*)&frame, item->hframe.ang.x, item->hframe.ang.y,
                 item->hframe.ang.z);
    frame.pos.x = 0.0f;
    frame.pos.y = 0.0f;
    frame.pos.z = 0.0f;
    frame.flags = 0;
    xModelSetMaterialAlpha(item->model, item->hframe.color.a);
    memcpy(item->model->Mat, &frame, sizeof(_xMat4x3));
    fn_80030494(item->renderType, item->hframe.color.a);
    xModelRender2D(item->model, &r, &from, &to, 1.5707964f);
}

void zHudElementsCB_GumInit()
{
    unsigned int size;

    sHudGumArray[0].model = zHudLoadModel(eHudModelBubblegumPiece);
    sHudGumArray[0].renderType = 8;
    sHudGumArray[1].model = zHudLoadModel(eHudModelBubblegumPiece5);
    sHudGumArray[1].renderType = 8;
    sHudGumArray[2].model = zHudLoadModel(eHudModelBubblegumPiece4);
    sHudGumArray[2].renderType = 8;
    sHudGumArray[3].model = zHudLoadModel(eHudModelBubblegumPiece3);
    sHudGumArray[3].renderType = 8;
    sHudGumArray[4].model = zHudLoadModel(eHudModelBubblegumPiece2);
    sHudGumArray[4].renderType = 8;

    for (signed int i = 0; i < 5; i++)
    {
        zHudItem* item = &sHudGumArray[i];
        void* data = xSTFindAsset(xStrHash("bat_fly.anm"), &size);
        if (data != NULL)
        {
            xAnimFile* afile = xAnimFileNew(data, "", 0, NULL);
            item->atbl = xAnimTableNew("", NULL);
            xAnimTableNewState(item->atbl, "Idle", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
            xAnimTableAddFile(item->atbl, afile, "Idle");
            xAnimPoolAlloc(&globals.sceneCur->mempool, item, item->atbl, item->model);
        }
    }
}

void zHudElementsCB_GumSetup()
{
    oldGumCnt = globals.player.Inv_Ammo[0];

    for (signed int i = 0; i < 5; i++)
    {
        zHudItem* item = &sHudGumArray[i];

        item->zHudStatusPosTable[eHudStatusPosOnScreen].x = 80.0f;
        item->zHudStatusPosTable[eHudStatusPosOnScreen].y = 435.0f;
        item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].x = 100.0f;
        item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].y = 300.0f;
        item->zHudStatusPosTable[eHudStatusPosOnScreenAlt1].x = 40.0f;
        item->zHudStatusPosTable[eHudStatusPosOnScreenAlt1].y = 435.0f;
        item->zHudStatusSizeTable[eHudStatusSizeDefault].x = 50.0f;
        item->zHudStatusSizeTable[eHudStatusSizeDefault].y = 50.0f;
        item->zHudStatusAngTable[eHudStatusAngStop].x = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngStop].y = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngStop].z = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngIdle].x = 1.5915494f;
        item->zHudStatusAngTable[eHudStatusAngIdle].y = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngIdle].z = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngTest1].x = 1.5915494f;
        item->zHudStatusAngTable[eHudStatusAngTest1].y = 15.915494f;
        item->zHudStatusAngTable[eHudStatusAngTest1].z = 0.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].rm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].gm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].bm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].a = 255.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].rm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].gm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].bm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].a = 0.0f;
        item->statusPos = eHudStatusPosOnScreen;
        item->statusAng = eHudStatusAngStop;
        item->statusSize = eHudStatusSizeDefault;
        item->statusColor = eHudStatusColorSolid;
        item->hframe.pos.x = item->zHudStatusPosTable[eHudStatusPosOnScreen].x;
        item->hframe.pos.y = item->zHudStatusPosTable[eHudStatusPosOnScreen].y;
        item->hframe.dang.x = item->zHudStatusAngTable[eHudStatusAngIdle].x;
        item->hframe.dang.y = item->zHudStatusAngTable[eHudStatusAngIdle].y;
        item->hframe.dang.z = item->zHudStatusAngTable[eHudStatusAngIdle].z;
        item->hframe.size.x = item->zHudStatusSizeTable[eHudStatusSizeDefault].x;
        item->hframe.size.y = item->zHudStatusSizeTable[eHudStatusSizeDefault].y;
        item->hframe.color.a = item->zHudStatusColorTable[eHudStatusColorTransparent].a;

        if (item->model->Anim != NULL)
        {
            item->model->Anim->Single->Time = 0.0f;
            item->model->Anim->Single->CurrentSpeed = 1.0f;
            xAnimPlaySetState(item->model->Anim->Single, item->model->Anim->Table->StateList,
                              0.0f);
        }
    }
}

void zHudElementsCB_GumUpdate(float dt)
{
    signed int i;

    if (!(globals.player.Inv_Specials & 1))
    {
        for (i = 0; i < 5; i++)
        {
            sHudGumArray[i].statusSize = eHudStatusSizeZero;
        }
    }
    else
    {
        signed int n = globals.player.Inv_Ammo[0] % 5;
        switch (n)
        {
        case 0:
            if (globals.player.Inv_Ammo[0] == 0)
            {
                for (i = 0; i < 5; i++)
                {
                    sHudGumArray[i].statusSize = eHudStatusSizeZero;
                }
            }
            else
            {
                sHudGumArray[0].statusSize = eHudStatusSizeDefault;
                for (i = 1; i < 5; i++)
                {
                    sHudGumArray[i].statusSize = eHudStatusSizeZero;
                }
            }
            break;
        case 1:
        case 2:
        case 3:
        case 4:
            for (i = 0; i < 5; i++)
            {
                if (i == n)
                {
                    sHudGumArray[n].statusSize = eHudStatusSizeDefault;
                }
                else
                {
                    sHudGumArray[i].statusSize = eHudStatusSizeZero;
                }
            }
            break;
        }
    }

    if (globals.player.Inv_Ammo[0] != oldGumCnt)
    {
        oldGumCnt = globals.player.Inv_Ammo[0];
        sHudDisplayTimer_gum = 1.5f;
    }

    if (sHudDisplayTimer_gum == 0.0f)
    {
        for (i = 0; i < 5; i++)
        {
            sHudGumArray[i].statusColor = eHudStatusColorTransparent;
        }
    }
    else
    {
        for (i = 0; i < 5; i++)
        {
            sHudGumArray[i].statusColor = eHudStatusColorSolid;
        }
        sHudDisplayTimer_gum -= dt;
        if (sHudDisplayTimer_gum < 0.0f)
        {
            sHudDisplayTimer_gum = 0.0f;
        }
    }

    for (i = 0; i < 5; i++)
    {
        zHudItem* item = &sHudGumArray[i];

        if ((globals.player.Inv_Ammo[0] - 1) / 5 > 0)
        {
            item->statusPos = eHudStatusPosOnScreen;
        }
        else
        {
            item->statusPos = eHudStatusPosOnScreenAlt1;
        }

        switch (item->statusPos)
        {
        case eHudStatusPosOnScreen:
            zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                         item->zHudStatusPosTable[eHudStatusPosOnScreen].x,
                         item->zHudStatusPosTable[eHudStatusPosOnScreen].y, 250.0f,
                         eHudInterpTypeDecel);
            break;
        case eHudStatusPosOnScreenAlt1:
            zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                         item->zHudStatusPosTable[eHudStatusPosOnScreenAlt1].x,
                         item->zHudStatusPosTable[eHudStatusPosOnScreenAlt1].y, 250.0f,
                         eHudInterpTypeDecel);
            break;
        case eHudStatusPosMiddleOfScreen:
            zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                         item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].x,
                         item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].y, 250.0f,
                         eHudInterpTypeDecel);
            break;
        }

        switch (item->statusAng)
        {
        case eHudStatusAngStop:
            zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngStop].x,
                         1.0f, eHudInterpTypeLinear);
            break;
        case eHudStatusAngIdle:
            zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngIdle].x,
                         1.0f, eHudInterpTypeLinear);
            break;
        case eHudStatusAngTest1:
            zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngTest1].x,
                         1.0f, eHudInterpTypeLinear);
            zHudInterp1d(dt, &item->hframe.dang.y, item->zHudStatusAngTable[eHudStatusAngTest1].y,
                         1.0f, eHudInterpTypeLinear);
            zHudInterp1d(dt, &item->hframe.dang.z, item->zHudStatusAngTable[eHudStatusAngTest1].z,
                         1.0f, eHudInterpTypeLinear);
            break;
        case eHudStatusAngTest2:
            break;
        }

        switch (item->statusSize)
        {
        case eHudStatusSizeDefault:
            item->hframe.size.x = item->zHudStatusSizeTable[eHudStatusSizeDefault].x;
            item->hframe.size.y = item->zHudStatusSizeTable[eHudStatusSizeDefault].y;
            break;
        case eHudStatusSizeZero:
            item->hframe.size.x = item->zHudStatusSizeTable[eHudStatusSizeZero].x;
            item->hframe.size.y = item->zHudStatusSizeTable[eHudStatusSizeZero].y;
            break;
        }

        switch (item->statusColor)
        {
        case eHudStatusColorSolid:
            zHudInterp1d(dt, &item->hframe.color.a,
                         item->zHudStatusColorTable[eHudStatusColorSolid].a, 300.0f,
                         eHudInterpTypeLinear);
            break;
        case eHudStatusColorTransparent:
            zHudInterp1d(dt, &item->hframe.color.a,
                         item->zHudStatusColorTable[eHudStatusColorTransparent].a, 120.0f,
                         eHudInterpTypeLinear);
            break;
        }

        item->hframe.ang.x = item->hframe.dang.x * dt + item->hframe.ang.x;
        item->hframe.ang.y = item->hframe.dang.y * dt + item->hframe.ang.y;
        item->hframe.ang.z = item->hframe.dang.z * dt + item->hframe.ang.z;

        xModelUpdate(item->model, dt);
        xModelEval(item->model);
    }
}

void zHudElementsCB_GumRender()
{
    if (globals.player.Inv_Specials & 1)
    {
        for (signed int i = 0; i < 5; i++)
        {
            zHudItem* item = &sHudGumArray[i];
            if (item->hframe.color.a != 0.0f)
            {
                char str[256] = {};
                sprintf(str, "%dx", globals.player.Inv_Ammo[0]);

                _xRect r = { 0.0f, 0.0f, 0.0f, 0.0f };
                _xMat4x3 frame;

                r.x = -(0.5f * item->hframe.size.x - item->hframe.pos.x);
                r.y = -(0.5f * item->hframe.size.y - item->hframe.pos.y);
                r.w = item->hframe.size.x * widthAspect;
                r.h = item->hframe.size.y;
                _xVec3 unused = { 0.0f, 0.0f, 0.0f };
                _xVec3 from = { 2.0f, 0.08f, 0.0f };
                _xVec3 to = { 0.0f, 0.0f, 0.0f };

                xMat3x3Euler((_xMat3x3*)&frame, item->hframe.ang.x, item->hframe.ang.y,
                             item->hframe.ang.z);
                frame.pos.x = 0.0f;
                frame.pos.y = 0.0f;
                frame.pos.z = 0.0f;
                frame.flags = 0;
                xModelSetMaterialAlpha(item->model, item->hframe.color.a);
                memcpy(item->model->Mat, &frame, sizeof(_xMat4x3));
                fn_80030494(item->renderType, item->hframe.color.a);
                xModelRender2D(item->model, &r, &from, &to, 1.5707964f);
            }
        }
    }
}

void zHudElementsCB_SoapInit()
{
    unsigned int size;

    sHudSoapArray[0].model = zHudLoadModel(eHudModelBarSoap);
    sHudSoapArray[0].renderType = 11;

    xModelInstance* bubble = zHudLoadModel(eHudModelSphere);
    for (signed int i = 1; i < 6; i++)
    {
        sHudSoapArray[i].model = bubble;
        sHudSoapArray[i].renderType = 12;
    }
}

void zHudElementsCB_SoapSetup()
{
    oldSoapCnt = globals.player.Inv_Ammo[1];
    sHudDisplayTimer_soap = 1.5f;

    sHudSoapArray[0].zHudStatusPosTable[eHudStatusPosOnScreen].x = 590.0f;
    sHudSoapArray[0].zHudStatusPosTable[eHudStatusPosOnScreen].y = 437.0f;
    sHudSoapArray[0].zHudStatusPosTable[eHudStatusPosOffScreenDefault].x = 586.0f;
    sHudSoapArray[0].zHudStatusPosTable[eHudStatusPosOffScreenDefault].y = 490.0f;
    sHudSoapArray[0].zHudStatusPosTable[eHudStatusPosMiddleOfScreen].x = 200.0f;
    sHudSoapArray[0].zHudStatusPosTable[eHudStatusPosMiddleOfScreen].y = 220.0f;
    sHudSoapArray[0].zHudStatusSizeTable[eHudStatusSizeDefault].x = 35.0f;
    sHudSoapArray[0].zHudStatusSizeTable[eHudStatusSizeDefault].y = 35.0f;
    sHudSoapArray[0].zHudStatusSizeTable[eHudStatusSizeZeroHoriz].x = 20.0f;
    sHudSoapArray[0].zHudStatusSizeTable[eHudStatusSizeZeroHoriz].y = 0.0f;
    sHudSoapArray[0].zHudStatusAngTable[eHudStatusAngStop].x = 0.0f;
    sHudSoapArray[0].zHudStatusAngTable[eHudStatusAngStop].y = 0.0f;
    sHudSoapArray[0].zHudStatusAngTable[eHudStatusAngStop].z = 0.0f;
    sHudSoapArray[0].zHudStatusAngTable[eHudStatusAngIdle].x = 1.5915494f;
    sHudSoapArray[0].zHudStatusAngTable[eHudStatusAngIdle].y = 0.0f;
    sHudSoapArray[0].zHudStatusAngTable[eHudStatusAngIdle].z = 0.0f;
    sHudSoapArray[0].zHudStatusAngTable[eHudStatusAngTest1].x = 1.5915494f;
    sHudSoapArray[0].zHudStatusAngTable[eHudStatusAngTest1].y = 15.915494f;
    sHudSoapArray[0].zHudStatusAngTable[eHudStatusAngTest1].z = 0.0f;
    sHudSoapArray[0].zHudStatusColorTable[eHudStatusColorSolid].rm = 1.0f;
    sHudSoapArray[0].zHudStatusColorTable[eHudStatusColorSolid].gm = 1.0f;
    sHudSoapArray[0].zHudStatusColorTable[eHudStatusColorSolid].bm = 1.0f;
    sHudSoapArray[0].zHudStatusColorTable[eHudStatusColorSolid].a = 255.0f;
    sHudSoapArray[0].zHudStatusColorTable[eHudStatusColorTransparent].rm = 1.0f;
    sHudSoapArray[0].zHudStatusColorTable[eHudStatusColorTransparent].gm = 1.0f;
    sHudSoapArray[0].zHudStatusColorTable[eHudStatusColorTransparent].bm = 1.0f;
    sHudSoapArray[0].zHudStatusColorTable[eHudStatusColorTransparent].a = 0.0f;
    sHudSoapArray[0].statusPos = eHudStatusPosOnScreen;
    sHudSoapArray[0].statusAng = eHudStatusAngStop;
    sHudSoapArray[0].statusSize = eHudStatusSizeDefault;
    sHudSoapArray[0].hframe.pos.x = 590.0f;
    sHudSoapArray[0].hframe.pos.y = 437.0f;
    sHudSoapArray[0].hframe.dang.x = 0.0f;
    sHudSoapArray[0].hframe.dang.y = 0.0f;
    sHudSoapArray[0].hframe.dang.z = 0.0f;
    sHudSoapArray[0].hframe.size.x = 35.0f;
    sHudSoapArray[0].hframe.size.y = 35.0f;

    for (signed int i = 1; i < 6; i++)
    {
        zHudItem* item = &sHudSoapArray[i];

        switch (i)
        {
        case 1:
            item->zHudStatusPosTable[eHudStatusPosOnScreen].x = 586.0f;
            item->zHudStatusPosTable[eHudStatusPosOnScreen].y = 419.0f;
            item->zHudStatusSizeTable[eHudStatusSizeDefault].x = 16.0f;
            item->zHudStatusSizeTable[eHudStatusSizeDefault].y = 16.0f;
            break;
        case 2:
            item->zHudStatusPosTable[eHudStatusPosOnScreen].x = 606.0f;
            item->zHudStatusPosTable[eHudStatusPosOnScreen].y = 415.0f;
            item->zHudStatusSizeTable[eHudStatusSizeDefault].x = 14.0f;
            item->zHudStatusSizeTable[eHudStatusSizeDefault].y = 14.0f;
            break;
        case 3:
            item->zHudStatusPosTable[eHudStatusPosOnScreen].x = 592.0f;
            item->zHudStatusPosTable[eHudStatusPosOnScreen].y = 403.0f;
            item->zHudStatusSizeTable[eHudStatusSizeDefault].x = 12.0f;
            item->zHudStatusSizeTable[eHudStatusSizeDefault].y = 12.0f;
            break;
        case 4:
            item->zHudStatusPosTable[eHudStatusPosOnScreen].x = 604.0f;
            item->zHudStatusPosTable[eHudStatusPosOnScreen].y = 398.0f;
            item->zHudStatusSizeTable[eHudStatusSizeDefault].x = 10.0f;
            item->zHudStatusSizeTable[eHudStatusSizeDefault].y = 10.0f;
            break;
        case 5:
            item->zHudStatusPosTable[eHudStatusPosOnScreen].x = 597.0f;
            item->zHudStatusPosTable[eHudStatusPosOnScreen].y = 388.0f;
            item->zHudStatusSizeTable[eHudStatusSizeDefault].x = 8.0f;
            item->zHudStatusSizeTable[eHudStatusSizeDefault].y = 8.0f;
            break;
        }

        item->zHudStatusColorTable[eHudStatusColorSolid].rm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].gm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].bm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].a = 255.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].rm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].gm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].bm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].a = 0.0f;
        item->zHudStatusColorTable[eHudStatusColorLightBlue].rm = 0.8f;
        item->zHudStatusColorTable[eHudStatusColorLightBlue].gm = 0.8f;
        item->zHudStatusColorTable[eHudStatusColorLightBlue].bm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorLightBlue].a = 100.0f;
        item->zHudStatusSizeTable[eHudStatusSizeZero].x = 0.0f;
        item->zHudStatusSizeTable[eHudStatusSizeZero].y = 0.0f;
        item->statusPos = eHudStatusPosOnScreen;
        item->statusAng = eHudStatusAngStop;
        item->statusSize = eHudStatusSizeDefault;
        item->statusColor = eHudStatusColorLightBlue;
        item->hframe.pos.x = item->zHudStatusPosTable[eHudStatusPosOnScreen].x;
        item->hframe.pos.y = item->zHudStatusPosTable[eHudStatusPosOnScreen].y;
        item->hframe.size.x = item->zHudStatusSizeTable[eHudStatusSizeDefault].x;
        item->hframe.size.y = item->zHudStatusSizeTable[eHudStatusSizeDefault].y;
    }
}

void zHudElementsCB_SoapUpdate(float dt)
{
    signed int i;
    unsigned int hasSoap = globals.player.Inv_Specials & 2;

    if (!hasSoap)
    {
        for (i = 0; i < 6; i++)
        {
            sHudSoapArray[i].statusSize = eHudStatusSizeZero;
        }
    }
    else
    {
        signed int n = globals.player.Inv_Ammo[1] % 5;
        switch (n)
        {
        case 0:
            if (globals.player.Inv_Ammo[1] == 0)
            {
                for (i = 1; i < 6; i++)
                {
                    sHudSoapArray[i].statusSize = eHudStatusSizeZero;
                }
                sHudSoapArray[0].statusPos = eHudStatusPosOffScreenDefault;
            }
            else
            {
                for (i = 1; i < 6; i++)
                {
                    sHudSoapArray[i].statusSize = eHudStatusSizeDefault;
                }
                sHudSoapArray[0].statusColor = eHudStatusColorSolid;
            }
            break;
        case 1:
        case 2:
        case 3:
        case 4:
            for (i = 1; i <= n; i++)
            {
                sHudSoapArray[i].statusSize = eHudStatusSizeDefault;
            }
            for (i = n + 1; i < 6; i++)
            {
                sHudSoapArray[i].statusSize = eHudStatusSizeZero;
            }
            sHudSoapArray[0].statusColor = eHudStatusColorSolid;
            break;
        }
    }

    if (globals.player.Inv_Ammo[1] != oldSoapCnt)
    {
        oldSoapCnt = globals.player.Inv_Ammo[1];
        sHudDisplayTimer_soap = 1.5f;
    }

    if (!hasSoap)
    {
        sHudSoapArray[0].statusColor = eHudStatusColorTransparent;
        sHudDisplayTimer_soap = 0.0f;
    }

    if (sHudDisplayTimer_soap == 0.0f)
    {
        for (i = 0; i < 6; i++)
        {
            sHudSoapArray[i].statusColor = eHudStatusColorTransparent;
        }
    }
    else
    {
        for (i = 0; i < 6; i++)
        {
            sHudSoapArray[i].statusColor = eHudStatusColorSolid;
        }
        sHudDisplayTimer_soap -= dt;
        if (sHudDisplayTimer_soap < 0.0f)
        {
            sHudDisplayTimer_soap = 0.0f;
        }
    }

    for (i = 0; i < 6; i++)
    {
        zHudItem* item = &sHudSoapArray[i];

        switch (item->statusPos)
        {
        case eHudStatusPosOnScreen:
            zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                         item->zHudStatusPosTable[eHudStatusPosOnScreen].x,
                         item->zHudStatusPosTable[eHudStatusPosOnScreen].y, 250.0f,
                         eHudInterpTypeDecel);
            break;
        case eHudStatusPosOffScreenDefault:
            zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                         item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].x,
                         item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].y, 250.0f,
                         eHudInterpTypeDecel);
            break;
        case eHudStatusPosMiddleOfScreen:
            zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                         item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].x,
                         item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].y, 250.0f,
                         eHudInterpTypeDecel);
            break;
        }

        switch (item->statusAng)
        {
        case eHudStatusAngStop:
            zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngStop].x,
                         1.0f, eHudInterpTypeLinear);
            break;
        case eHudStatusAngIdle:
            zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngIdle].x,
                         1.0f, eHudInterpTypeLinear);
            break;
        case eHudStatusAngTest1:
            zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngTest1].x,
                         1.0f, eHudInterpTypeLinear);
            zHudInterp1d(dt, &item->hframe.dang.y, item->zHudStatusAngTable[eHudStatusAngTest1].y,
                         1.0f, eHudInterpTypeLinear);
            zHudInterp1d(dt, &item->hframe.dang.z, item->zHudStatusAngTable[eHudStatusAngTest1].z,
                         1.0f, eHudInterpTypeLinear);
            break;
        case eHudStatusAngTest2:
            break;
        }

        switch (item->statusSize)
        {
        case eHudStatusSizeDefault:
        case eHudStatusSizeSquash:
            zHudInterp1d(dt, &item->hframe.size.x,
                         item->zHudStatusSizeTable[eHudStatusSizeDefault].x, 90.0f,
                         eHudInterpTypeLinear);
            zHudInterp1d(dt, &item->hframe.size.y,
                         item->zHudStatusSizeTable[eHudStatusSizeDefault].y, 90.0f,
                         eHudInterpTypeLinear);
            break;
        case eHudStatusSizeZero:
            zHudInterp1d(dt, &item->hframe.size.x, item->zHudStatusSizeTable[eHudStatusSizeZero].x,
                         90.0f, eHudInterpTypeLinear);
            zHudInterp1d(dt, &item->hframe.size.y, item->zHudStatusSizeTable[eHudStatusSizeZero].y,
                         90.0f, eHudInterpTypeLinear);
            break;
        case eHudStatusSizeZeroHoriz:
        {
            float d = item->zHudStatusSizeTable[eHudStatusSizeZeroHoriz].y - item->hframe.size.y;
            float s = dt * (d * 7.2f);
            if ((float)__fabs(s) > (float)__fabs(d))
            {
                s = d;
            }
            item->hframe.size.y += s;
            break;
        }
        }

        switch (item->statusColor)
        {
        case eHudStatusColorSolid:
            zHudInterp1d(dt, &item->hframe.color.a,
                         item->zHudStatusColorTable[eHudStatusColorSolid].a, 300.0f,
                         eHudInterpTypeLinear);
            break;
        case eHudStatusColorTransparent:
            zHudInterp1d(dt, &item->hframe.color.a,
                         item->zHudStatusColorTable[eHudStatusColorTransparent].a, 150.0f,
                         eHudInterpTypeLinear);
            break;
        case eHudStatusColorLightBlue:
        case eHudStatusColorOpaque2:
            item->hframe.color.a = item->zHudStatusColorTable[item->statusColor].a;
            break;
        }

        item->hframe.ang.x = item->hframe.dang.x * dt + item->hframe.ang.x;
        item->hframe.ang.y = item->hframe.dang.y * dt + item->hframe.ang.y;
        item->hframe.ang.z = item->hframe.dang.z * dt + item->hframe.ang.z;

        xModelSetMaterialAlpha(item->model, item->hframe.color.a);
    }
}

void zHudElementsCB_SoapRender()
{
    char str[256] = {};
    signed int i;

    sprintf(str, "%dx", globals.player.Inv_Ammo[1]);
    zHud_DrawText(sHudSoapArray[0].hframe.pos.x, sHudSoapArray[0].hframe.pos.y, 150.0f, 100.0f,
                  str, 0x20, 1.0f, 0xC8, DROPSHADOW_NORMAL);

    {
        _xRect r = { 0.0f, 0.0f, 0.0f, 0.0f };
        _xMat4x3 frame;

        r.x = -(0.5f * sHudSoapArray[0].hframe.size.x - sHudSoapArray[0].hframe.pos.x);
        r.y = -(0.5f * sHudSoapArray[0].hframe.size.y - sHudSoapArray[0].hframe.pos.y);
        r.w = sHudSoapArray[0].hframe.size.x * widthAspect;
        r.h = sHudSoapArray[0].hframe.size.y;
        _xVec3 unused = { 0.0f, 0.0f, 0.0f };
        _xVec3 from = { 0.0f, -0.35f, 0.4f };
        _xVec3 to = { 0.0f, 0.0f, 0.0f };

        xMat3x3Euler((_xMat3x3*)&frame, sHudSoapArray[0].hframe.ang.x,
                     sHudSoapArray[0].hframe.ang.y, sHudSoapArray[0].hframe.ang.z);
        frame.pos.x = 0.0f;
        frame.pos.y = 0.0f;
        frame.pos.z = 0.0f;
        frame.flags = 0;
        xModelSetMaterialAlpha(sHudSoapArray[0].model, sHudSoapArray[0].hframe.color.a);
        memcpy(sHudSoapArray[0].model->Mat, &frame, sizeof(_xMat4x3));
        fn_80030494(sHudSoapArray[0].renderType, sHudSoapArray[0].hframe.color.a);
        xModelRender2D(sHudSoapArray[0].model, &r, &from, &to, 1.5707964f);
    }

    for (i = 1; i < 6; i++)
    {
        zHudItem* item = &sHudSoapArray[i];
        if (item->hframe.color.a != 0.0f)
        {
            _xRect r = { 0.0f, 0.0f, 0.0f, 0.0f };
            _xMat4x3 frame;

            r.x = -(0.5f * item->hframe.size.x - item->hframe.pos.x);
            r.y = -(0.5f * item->hframe.size.y - item->hframe.pos.y);
            r.w = item->hframe.size.x * widthAspect;
            r.h = item->hframe.size.y;
            _xVec3 unused = { 0.0f, 0.0f, 0.0f };
            _xVec3 from = { 0.0f, 0.0f, 2.0f };
            _xVec3 to = { 0.0f, 0.0f, 0.0f };

            xMat3x3Euler((_xMat3x3*)&frame, item->hframe.ang.x, item->hframe.ang.y,
                         item->hframe.ang.z);
            frame.pos.x = 0.0f;
            frame.pos.y = 0.0f;
            frame.pos.z = 0.0f;
            frame.flags = 0;
            memcpy(item->model->Mat, &frame, sizeof(_xMat4x3));

            RwRGBA c = { 0x9E, 0xD7, 0xE0, 0x00 };
            c.alpha = item->hframe.color.a;
            item->model->Data->geometry->matList.materials[0]->color = c;

            fn_80030494(item->renderType, item->hframe.color.a);
            xModelRender2D(item->model, &r, &from, &to, 1.5707964f);
        }
    }
}

void zHudElementsCB_MessageInit()
{
}

void zHudElementsCB_MessageSetup()
{
}

void zHudElementsCB_MessageUpdate()
{
}

void zHudElementsCB_MessageRender()
{
}

void zHudElementsCB_SoapboxInit()
{
    unsigned int animSize;
    unsigned int size;

    sHudSoapboxArray[0].model = zHudLoadModel(eHudModelBoxSoap);
    sHudSoapboxArray[0].renderType = 13;
    zHudItem* item = &sHudSoapboxArray[0];

    void* data = xSTFindAsset(xStrHash("bat_fly.anm"), &animSize);
    if (data != NULL)
    {
        xAnimFile* afile = xAnimFileNew(data, "", 0, NULL);
        item->atbl = xAnimTableNew("", NULL);
        xAnimTableNewState(item->atbl, "Idle", 0x10, 0, 1.0f, NULL, NULL, 0.0f,
                           NULL);
        xAnimTableAddFile(item->atbl, afile, "Idle");
        xAnimPoolAlloc(&globals.sceneCur->mempool, item, item->atbl,
                       item->model);
    }
}

void zHudElementsCB_SoapboxSetup()
{
    oldSoapCnt2 = globals.player.Inv_Ammo[1];
    sHudDisplayTimer_soap = 1.5f;
    zHudItem* item = &sHudSoapboxArray[0];

    item->zHudStatusPosTable[eHudStatusPosOnScreen].x = 492.0f;
    item->zHudStatusPosTable[eHudStatusPosOnScreen].y = 475.0f;
    item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].x = 486.0f;
    item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].y = 575.0f;
    item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].x = 200.0f;
    item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].y = 220.0f;
    item->zHudStatusSizeTable[eHudStatusSizeDefault].x = 105.0f;
    item->zHudStatusSizeTable[eHudStatusSizeDefault].y = 105.0f;
    item->zHudStatusAngTable[eHudStatusAngStop].x = 0.0f;
    item->zHudStatusAngTable[eHudStatusAngStop].y = 0.0f;
    item->zHudStatusAngTable[eHudStatusAngStop].z = 0.0f;
    item->zHudStatusAngTable[eHudStatusAngIdle].x = 1.5915494f;
    item->zHudStatusAngTable[eHudStatusAngIdle].y = 0.0f;
    item->zHudStatusAngTable[eHudStatusAngIdle].z = 0.0f;
    item->zHudStatusAngTable[eHudStatusAngTest1].x = 1.5915494f;
    item->zHudStatusAngTable[eHudStatusAngTest1].y = 15.915494f;
    item->zHudStatusAngTable[eHudStatusAngTest1].z = 0.0f;
    item->zHudStatusColorTable[eHudStatusColorSolid].rm = 1.0f;
    item->zHudStatusColorTable[eHudStatusColorSolid].gm = 1.0f;
    item->zHudStatusColorTable[eHudStatusColorSolid].bm = 1.0f;
    item->zHudStatusColorTable[eHudStatusColorSolid].a = 255.0f;
    item->zHudStatusColorTable[eHudStatusColorTransparent].rm = 1.0f;
    item->zHudStatusColorTable[eHudStatusColorTransparent].gm = 1.0f;
    item->zHudStatusColorTable[eHudStatusColorTransparent].bm = 1.0f;
    item->zHudStatusColorTable[eHudStatusColorTransparent].a = 0.0f;
    item->statusPos = eHudStatusPosOnScreen;
    item->statusAng = eHudStatusAngStop;
    item->statusSize = eHudStatusSizeDefault;
    item->statusColor = eHudStatusColorSolid;

    if ((globals.player.Inv_Specials & 2) && (globals.player.Inv_Ammo[1] - 1) / 5 > 0)
    {
        item->hframe.pos.x = 492.0f;
        item->hframe.pos.y = 475.0f;
    }
    else
    {
        item->hframe.pos.x =
            item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].x;
        item->hframe.pos.y =
            item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].y;
    }

    item->hframe.dang.x = item->zHudStatusAngTable[eHudStatusAngStop].x;
    item->hframe.dang.y = item->zHudStatusAngTable[eHudStatusAngStop].y;
    item->hframe.dang.z = item->zHudStatusAngTable[eHudStatusAngStop].z;
    item->hframe.size.x =
        item->zHudStatusSizeTable[eHudStatusSizeDefault].x;
    item->hframe.size.y =
        item->zHudStatusSizeTable[eHudStatusSizeDefault].y;
    item->hframe.color.a =
        item->zHudStatusColorTable[eHudStatusColorTransparent].a;

    if (item->model->Anim != NULL)
    {
        item->model->Anim->Single->Time = 0.0f;
        item->model->Anim->Single->CurrentSpeed = 1.0f;
        xAnimPlaySetState(item->model->Anim->Single,
                          item->model->Anim->Table->StateList, 0.0f);
    }
}

void zHudElementsCB_SoapboxUpdate(float dt)
{
    zHudItem* item = &sHudSoapboxArray[0];
    unsigned int hasSoap = globals.player.Inv_Specials & 2;

    if (!hasSoap)
    {
        item->statusPos = eHudStatusPosOffScreenDefault;
    }
    else
    {
        item->statusPos = eHudStatusPosOffScreenDefault;
    }

    if (hasSoap && (globals.player.Inv_Ammo[1] - 1) / 5 > 0)
    {
        item->statusPos = eHudStatusPosOnScreen;
    }
    else
    {
        item->statusPos = eHudStatusPosOffScreenDefault;
    }

    if (globals.player.Inv_Ammo[1] != oldSoapCnt2)
    {
        oldSoapCnt2 = globals.player.Inv_Ammo[1];
    }

    if (!(globals.player.Inv_Specials & 2))
    {
        sHudSoapboxArray[0].statusColor = eHudStatusColorTransparent;
    }
    else if (sHudDisplayTimer_soap == 0.0f)
    {
        sHudSoapboxArray[0].statusColor = eHudStatusColorTransparent;
    }
    else
    {
        sHudSoapboxArray[0].statusColor = eHudStatusColorSolid;
    }

    switch (item->statusPos)
    {
    case eHudStatusPosOnScreen:
        zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                     item->zHudStatusPosTable[eHudStatusPosOnScreen].x,
                     item->zHudStatusPosTable[eHudStatusPosOnScreen].y, 2500.0f,
                     eHudInterpTypeDecel);
        break;
    case eHudStatusPosOffScreenDefault:
        zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                     item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].x,
                     item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].y, 40.0f,
                     eHudInterpTypeAccel);
        break;
    case eHudStatusPosMiddleOfScreen:
        zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                     item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].x,
                     item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].y, 250.0f,
                     eHudInterpTypeDecel);
        break;
    }

    switch (item->statusAng)
    {
    case eHudStatusAngStop:
        zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngStop].x, 1.0f,
                     eHudInterpTypeLinear);
        break;
    case eHudStatusAngIdle:
        zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngIdle].x, 1.0f,
                     eHudInterpTypeLinear);
        break;
    case eHudStatusAngTest1:
        zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngTest1].x,
                     1.0f, eHudInterpTypeLinear);
        zHudInterp1d(dt, &item->hframe.dang.y, item->zHudStatusAngTable[eHudStatusAngTest1].y,
                     1.0f, eHudInterpTypeLinear);
        zHudInterp1d(dt, &item->hframe.dang.z, item->zHudStatusAngTable[eHudStatusAngTest1].z,
                     1.0f, eHudInterpTypeLinear);
        break;
    case eHudStatusAngTest2:
        break;
    }

    switch (item->statusSize)
    {
    case eHudStatusSizeDefault:
        zHudInterp1d(dt, &item->hframe.size.x, item->zHudStatusSizeTable[eHudStatusSizeDefault].x,
                     10.0f, eHudInterpTypeLinear);
        zHudInterp1d(dt, &item->hframe.size.y, item->zHudStatusSizeTable[eHudStatusSizeDefault].y,
                     10.0f, eHudInterpTypeLinear);
        break;
    case eHudStatusSizeZero:
        item->hframe.size.x = item->zHudStatusSizeTable[eHudStatusSizeZero].x;
        item->hframe.size.y = item->zHudStatusSizeTable[eHudStatusSizeZero].y;
        break;
    }

    switch (item->statusColor)
    {
    case eHudStatusColorSolid:
    case eHudStatusColorLightBlue:
    case eHudStatusColorOpaque2:
        zHudInterp1d(dt, &item->hframe.color.a,
                     item->zHudStatusColorTable[eHudStatusColorSolid].a, 300.0f,
                     eHudInterpTypeLinear);
        break;
    case eHudStatusColorTransparent:
        zHudInterp1d(dt, &item->hframe.color.a,
                     item->zHudStatusColorTable[eHudStatusColorTransparent].a, 150.0f,
                     eHudInterpTypeLinear);
        break;
    }

    item->hframe.ang.x = item->hframe.dang.x * dt + item->hframe.ang.x;
    item->hframe.ang.y = item->hframe.dang.y * dt + item->hframe.ang.y;
    item->hframe.ang.z = item->hframe.dang.z * dt + item->hframe.ang.z;

    xModelUpdate(item->model, dt);
    xModelEval(item->model);
}

void zHudElementsCB_SoapboxRender()
{
    zHudItem* item = &sHudSoapboxArray[0];
    if (globals.player.Inv_Specials & 2)
    {
        _xRect r = { 0.0f, 0.0f, 0.0f, 0.0f };
        _xMat4x3 frame;

        r.x = 23.0f + -(0.5f * item->hframe.size.x - item->hframe.pos.x);
        r.w = item->hframe.size.x * widthAspect;
        r.y = 17.0f + -(0.5f * item->hframe.size.y - item->hframe.pos.y);
        r.h = item->hframe.size.y;
        _xVec3 unused = { 0.0f, 0.0f, 0.0f };
        _xVec3 from = { -1.3f, 0.3f, 2.0f };
        _xVec3 to = { 0.0f, 0.0f, 0.0f };

        xMat3x3Euler((_xMat3x3*)&frame, item->hframe.ang.x,
                     item->hframe.ang.y, item->hframe.ang.z);
        frame.pos.x = 0.0f;
        frame.pos.y = 0.0f;
        frame.pos.z = 0.0f;
        frame.flags = 0;
        xModelSetMaterialAlpha(item->model, item->hframe.color.a);
        memcpy(item->model->Mat, &frame, sizeof(_xMat4x3));

        if (globals.player.Inv_Specials & 2)
        {
            fn_80030494(item->renderType, item->hframe.color.a);
            xModelRender2D(item->model, &r, &from, &to, 1.5707964f);
        }

        if (globals.player.Inv_Specials & 2)
        {
            char str[256] = {};
            signed int offset[2] = { -20, -70 };
            sprintf(str, "%d", (globals.player.Inv_Ammo[1] - 1) / 5);
            zHud_DrawText(item->hframe.pos.x + offset[0],
                          item->hframe.pos.y + offset[1], 90.0f, 100.0f, str, 0x40,
                          1.0f, item->hframe.color.a, DROPSHADOW_NORMAL);
        }
    }
}

void zHudElementsCB_GumpackInit()
{
    unsigned int animSize;
    unsigned int size;

    sHudGumpackArray[0].model = zHudLoadModel(eHudModelSphere);
    sHudGumpackArray[0].renderType = 7;
    zHudItem* item = &sHudGumpackArray[0];

    void* data = xSTFindAsset(xStrHash("bat_fly.anm"), &animSize);
    if (data != NULL)
    {
        xAnimFile* afile = xAnimFileNew(data, "", 0, NULL);
        item->atbl = xAnimTableNew("", NULL);
        xAnimTableNewState(item->atbl, "Idle", 0x10, 0, 1.0f, NULL, NULL, 0.0f,
                           NULL);
        xAnimTableAddFile(item->atbl, afile, "Idle");
        xAnimPoolAlloc(&globals.sceneCur->mempool, item, item->atbl,
                       item->model);
    }
}

void zHudElementsCB_GumpackSetup()
{
    oldGumCnt2 = globals.player.Inv_Ammo[0];
    sHudDisplayTimer_gum = 1.5f;
    zHudItem* item = &sHudGumpackArray[0];

    item->zHudStatusPosTable[eHudStatusPosOnScreen].x = 1.0f;
    item->zHudStatusPosTable[eHudStatusPosOnScreen].y = 480.0f;
    item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].x = 1.0f;
    item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].y = 580.0f;
    item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].x = 200.0f;
    item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].y = 220.0f;
    item->zHudStatusSizeTable[eHudStatusSizeDefault].x = 60.0f;
    item->zHudStatusSizeTable[eHudStatusSizeDefault].y = 60.0f;
    item->zHudStatusAngTable[eHudStatusAngStop].x = 0.0f;
    item->zHudStatusAngTable[eHudStatusAngStop].y = 0.0f;
    item->zHudStatusAngTable[eHudStatusAngStop].z = 0.0f;
    item->zHudStatusAngTable[eHudStatusAngIdle].x = 1.5915494f;
    item->zHudStatusAngTable[eHudStatusAngIdle].y = 0.0f;
    item->zHudStatusAngTable[eHudStatusAngIdle].z = 0.0f;
    item->zHudStatusAngTable[eHudStatusAngTest1].x = 1.5915494f;
    item->zHudStatusAngTable[eHudStatusAngTest1].y = 15.915494f;
    item->zHudStatusAngTable[eHudStatusAngTest1].z = 0.0f;
    item->zHudStatusColorTable[eHudStatusColorSolid].rm = 1.0f;
    item->zHudStatusColorTable[eHudStatusColorSolid].gm = 1.0f;
    item->zHudStatusColorTable[eHudStatusColorSolid].bm = 1.0f;
    item->zHudStatusColorTable[eHudStatusColorSolid].a = 255.0f;
    item->zHudStatusColorTable[eHudStatusColorTransparent].rm = 1.0f;
    item->zHudStatusColorTable[eHudStatusColorTransparent].gm = 1.0f;
    item->zHudStatusColorTable[eHudStatusColorTransparent].bm = 1.0f;
    item->zHudStatusColorTable[eHudStatusColorTransparent].a = 0.0f;
    item->statusPos = eHudStatusPosOnScreen;
    item->statusAng = eHudStatusAngStop;
    item->statusSize = eHudStatusSizeDefault;
    item->statusColor = eHudStatusColorSolid;

    if ((globals.player.Inv_Specials & 1) && (globals.player.Inv_Ammo[0] - 1) / 5 > 0)
    {
        item->hframe.pos.x = 1.0f;
        item->hframe.pos.y = 480.0f;
    }
    else
    {
        item->hframe.pos.x =
            item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].x;
        item->hframe.pos.y =
            item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].y;
    }

    item->hframe.dang.x = item->zHudStatusAngTable[eHudStatusAngStop].x;
    item->hframe.dang.y = item->zHudStatusAngTable[eHudStatusAngStop].y;
    item->hframe.dang.z = item->zHudStatusAngTable[eHudStatusAngStop].z;
    item->hframe.size.x =
        item->zHudStatusSizeTable[eHudStatusSizeDefault].x;
    item->hframe.size.y =
        item->zHudStatusSizeTable[eHudStatusSizeDefault].y;
    item->hframe.color.a =
        item->zHudStatusColorTable[eHudStatusColorTransparent].a;

    if (item->model->Anim != NULL)
    {
        item->model->Anim->Single->Time = 0.0f;
        item->model->Anim->Single->CurrentSpeed = 1.0f;
        xAnimPlaySetState(item->model->Anim->Single,
                          item->model->Anim->Table->StateList, 0.0f);
    }
}

void zHudElementsCB_GumpackUpdate(float dt)
{
    zHudItem* item = &sHudGumpackArray[0];

    if (!(globals.player.Inv_Specials & 1))
    {
        item->statusSize = eHudStatusSizeZero;
    }
    else
    {
        item->statusSize = eHudStatusSizeDefault;
    }

    if ((globals.player.Inv_Ammo[0] - 1) / 5 > 0)
    {
        item->statusPos = eHudStatusPosOnScreen;
    }
    else
    {
        item->statusPos = eHudStatusPosOffScreenDefault;
    }

    if (globals.player.Inv_Ammo[0] != oldGumCnt2)
    {
        oldGumCnt2 = globals.player.Inv_Ammo[0];
        sHudDisplayTimer_gum = 1.5f;
    }

    if (sHudDisplayTimer_gum == 0.0f)
    {
        item->statusColor = eHudStatusColorTransparent;
    }
    else
    {
        item->statusColor = eHudStatusColorSolid;
        sHudDisplayTimer_gum -= dt;
        if (sHudDisplayTimer_gum < 0.0f)
        {
            sHudDisplayTimer_gum = 0.0f;
        }
    }

    switch (item->statusPos)
    {
    case eHudStatusPosOnScreen:
        zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                     item->zHudStatusPosTable[eHudStatusPosOnScreen].x,
                     item->zHudStatusPosTable[eHudStatusPosOnScreen].y, 2500.0f,
                     eHudInterpTypeDecel);
        break;
    case eHudStatusPosOffScreenDefault:
        zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                     item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].x,
                     item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].y, 40.0f,
                     eHudInterpTypeAccel);
        break;
    case eHudStatusPosMiddleOfScreen:
        zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                     item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].x,
                     item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].y, 250.0f,
                     eHudInterpTypeDecel);
        break;
    }

    switch (item->statusAng)
    {
    case eHudStatusAngStop:
        zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngStop].x, 1.0f,
                     eHudInterpTypeLinear);
        break;
    case eHudStatusAngIdle:
        zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngIdle].x, 1.0f,
                     eHudInterpTypeLinear);
        break;
    case eHudStatusAngTest1:
        zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngTest1].x,
                     1.0f, eHudInterpTypeLinear);
        zHudInterp1d(dt, &item->hframe.dang.y, item->zHudStatusAngTable[eHudStatusAngTest1].y,
                     1.0f, eHudInterpTypeLinear);
        zHudInterp1d(dt, &item->hframe.dang.z, item->zHudStatusAngTable[eHudStatusAngTest1].z,
                     1.0f, eHudInterpTypeLinear);
        break;
    case eHudStatusAngTest2:
        break;
    }

    switch (item->statusSize)
    {
    case eHudStatusSizeDefault:
        zHudInterp1d(dt, &item->hframe.size.x, item->zHudStatusSizeTable[eHudStatusSizeDefault].x,
                     150.0f, eHudInterpTypeLinear);
        zHudInterp1d(dt, &item->hframe.size.y, item->zHudStatusSizeTable[eHudStatusSizeDefault].y,
                     150.0f, eHudInterpTypeLinear);
        break;
    case eHudStatusSizeZero:
        item->hframe.size.x = item->zHudStatusSizeTable[eHudStatusSizeZero].x;
        item->hframe.size.y = item->zHudStatusSizeTable[eHudStatusSizeZero].y;
        break;
    }

    switch (item->statusColor)
    {
    case eHudStatusColorSolid:
    case eHudStatusColorOpaque2:
        zHudInterp1d(dt, &item->hframe.color.a,
                     item->zHudStatusColorTable[eHudStatusColorSolid].a, 300.0f,
                     eHudInterpTypeLinear);
        break;
    case eHudStatusColorTransparent:
        zHudInterp1d(dt, &item->hframe.color.a,
                     item->zHudStatusColorTable[eHudStatusColorTransparent].a, 120.0f,
                     eHudInterpTypeLinear);
        break;
    }

    item->hframe.ang.x = item->hframe.dang.x * dt + item->hframe.ang.x;
    item->hframe.ang.y = item->hframe.dang.y * dt + item->hframe.ang.y;
    item->hframe.ang.z = item->hframe.dang.z * dt + item->hframe.ang.z;

    xModelUpdate(item->model, dt);
    xModelEval(item->model);
}

void zHudElementsCB_GumpackRender()
{
    zHudItem* item = &sHudGumpackArray[0];
    _xRect r = { 0.0f, 0.0f, 0.0f, 0.0f };
    _xMat4x3 frame;

    r.x = -(0.5f * item->hframe.size.x - item->hframe.pos.x) + 33.0f;
    r.y = -(0.5f * item->hframe.size.y - item->hframe.pos.y) + 21.0f;
    r.w = item->hframe.size.x * widthAspect;
    r.h = item->hframe.size.y;
    _xVec3 unused = { 0.0f, 0.0f, 0.0f };
    _xVec3 from = { 1.8f, 0.0f, 0.0f };
    _xVec3 to = { 0.0f, 0.0f, 0.0f };

    xMat3x3Euler((_xMat3x3*)&frame, item->hframe.ang.x,
                 item->hframe.ang.y, item->hframe.ang.z);
    frame.pos.x = 0.0f;
    frame.pos.y = 0.0f;
    frame.pos.z = 0.0f;
    frame.flags = 0;
    memcpy(item->model->Mat, &frame, sizeof(_xMat4x3));

    RwRGBA c = { 0xF5, 0xBC, 0xBC, 0x00 };
    c.alpha = sHudGumpackArray[0].hframe.color.a;
    sHudGumpackArray[0].model->Data->geometry->matList.materials[0]->color = c;

    if (globals.player.Inv_Specials & 1)
    {
        fn_80030494(item->renderType, item->hframe.color.a);
        xModelRender2D(item->model, &r, &from, &to, 1.5707964f);
    }

    if (globals.player.Inv_Specials & 1)
    {
        char str[256] = {};
        signed int offset[2] = { 17, -70 };
        sprintf(str, "%d", (globals.player.Inv_Ammo[0] - 1) / 5);
        zHud_DrawText(item->hframe.pos.x + offset[0],
                      item->hframe.pos.y + offset[1], 90.0f, 70.0f, str, 0x40, 1.0f,
                      item->hframe.color.a, DROPSHADOW_NORMAL);
    }
}

void zHudElementsCB_HealthbarInit()
{
    unsigned int size;
    void* data;
    signed int i;

    xModelInstance* collar = zHudLoadModel(eHudModelScoobyCollar);
    for (i = 0; i < 5; i++)
    {
        sHudHealthbarArray[i].model = collar;
        sHudHealthbarArray[i].renderType = 14;
    }

    // NOTE: the original sets up the animations on the player status items here.
    for (i = 0; i < 5; i++)
    {
        zHudItem* item = &sHudPlayerStatusArray[i];

        switch (i)
        {
        case 0:
            data = xSTFindAsset(xStrHash("rhsa0001.anm"), &size);
            break;
        case 1:
            data = xSTFindAsset(xStrHash("rhsa0001.anm"), &size);
            break;
        case 2:
            data = xSTFindAsset(xStrHash("rhsa0001.anm"), &size);
            break;
        case 3:
            data = xSTFindAsset(xStrHash("rhsa0001.anm"), &size);
            break;
        case 4:
            data = xSTFindAsset(xStrHash("rhsa0001.anm"), &size);
            break;
        }

        if (data != NULL)
        {
            xAnimFile* afile = xAnimFileNew(data, "", 0, NULL);
            item->atbl = xAnimTableNew("", NULL);
            xAnimTableNewState(item->atbl, "Idle", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
            xAnimTableAddFile(item->atbl, afile, "Idle");
            xAnimPoolAlloc(&globals.sceneCur->mempool, item, item->atbl, item->model);
        }
    }
}

void zHudElementsCB_HealthbarSetup()
{
    oldHealthCnt2 = globals.player.Health;

    for (signed int i = 0; i < 5; i++)
    {
        zHudItem* item = &sHudHealthbarArray[i];

        item->zHudStatusPosTable[eHudStatusPosOnScreen].x = 17.0f * i + 20.0f;
        item->zHudStatusPosTable[eHudStatusPosOnScreen].y = 115.0f;
        item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].x =
            item->zHudStatusPosTable[eHudStatusPosOnScreen].x - 100.0f;
        item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].y = 115.0f;
        item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].x = 300.0f;
        item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].y = 200.0f;
        item->zHudStatusSizeTable[eHudStatusSizeDefault].x = 17.0f;
        item->zHudStatusSizeTable[eHudStatusSizeDefault].y = 17.0f;
        item->zHudStatusAngTable[eHudStatusAngStop].x = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngStop].y = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngStop].z = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngIdle].x = 1.5915494f;
        item->zHudStatusAngTable[eHudStatusAngIdle].y = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngIdle].z = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngFast].x = 6.3661976f;
        item->zHudStatusAngTable[eHudStatusAngFast].y = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngFast].z = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngTest1].x = 1.5915494f;
        item->zHudStatusAngTable[eHudStatusAngTest1].y = 15.915494f;
        item->zHudStatusAngTable[eHudStatusAngTest1].z = 0.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].rm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].gm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].bm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].a = 255.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].rm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].gm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].bm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].a = 0.0f;
        item->statusPos = eHudStatusPosOnScreen;
        item->statusAng = eHudStatusAngStop;
        item->statusSize = eHudStatusSizeDefault;
        item->statusColor = eHudStatusColorSolid;
        item->hframe.pos.x = item->zHudStatusPosTable[eHudStatusPosOnScreen].x;
        item->hframe.pos.y = item->zHudStatusPosTable[eHudStatusPosOnScreen].y;
        item->hframe.dang.x = item->zHudStatusAngTable[eHudStatusAngStop].x;
        item->hframe.dang.y = item->zHudStatusAngTable[eHudStatusAngStop].y;
        item->hframe.dang.z = item->zHudStatusAngTable[eHudStatusAngStop].z;
        item->hframe.size.x = item->zHudStatusSizeTable[eHudStatusSizeDefault].x;
        item->hframe.size.y = item->zHudStatusSizeTable[eHudStatusSizeDefault].y;
        item->hframe.color.a = item->zHudStatusColorTable[eHudStatusColorTransparent].a;

        if (item->model->Anim != NULL)
        {
            item->model->Anim->Single->Time = 0.0f;
            item->model->Anim->Single->CurrentSpeed = 1.0f;
            xAnimPlaySetState(item->model->Anim->Single, item->model->Anim->Table->StateList,
                              0.0f);
        }
    }
}

void zHudElementsCB_HealthbarUpdate(float dt)
{
    signed int i;
    signed int j;
    signed int n = globals.player.Health - 1;

    switch (n)
    {
    case -1:
        for (i = 0; i < 5; i++)
        {
            sHudHealthbarArray[i].statusSize = eHudStatusSizeZero;
        }
        break;
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        for (i = 0; i < 5; i++)
        {
            for (j = 0; j <= n; j++)
            {
                sHudHealthbarArray[j].statusSize = eHudStatusSizeDefault;
            }
            for (j = n + 1; j < 5; j++)
            {
                sHudHealthbarArray[j].statusSize = eHudStatusSizeZero;
            }
        }
        break;
    }

    if (globals.player.Health != oldHealthCnt2)
    {
        oldHealthCnt2 = globals.player.Health;
        sHudDisplayTimer_healthBar = 2.0f;
    }

    if (sHudDisplayTimer_healthBar == 0.0f)
    {
        for (i = 0; i < 5; i++)
        {
            sHudHealthbarArray[i].statusColor = eHudStatusColorTransparent;
            sHudHealthbarArray[i].statusPos = eHudStatusPosOffScreenDefault;
        }
    }
    else
    {
        for (i = 0; i < 5; i++)
        {
            sHudHealthbarArray[i].statusColor = eHudStatusColorSolid;
            sHudHealthbarArray[i].statusPos = eHudStatusPosOnScreen;
        }
        sHudDisplayTimer_healthBar -= dt;
        if (sHudDisplayTimer_healthBar < 0.0f)
        {
            sHudDisplayTimer_healthBar = 0.0f;
        }
    }

    dynOffset = (float)(5 - globals.player.Health) * sHudHealthbarArray[0].hframe.size.x * 0.5f;

    for (i = 0; i < 5; i++)
    {
        zHudItem* item = &sHudHealthbarArray[i];

        item->zHudStatusPosTable[eHudStatusPosOnScreen].x = 17.0f * i + (20.0f + dynOffset);

        switch (item->statusPos)
        {
        case eHudStatusPosOnScreen:
            zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                         item->zHudStatusPosTable[eHudStatusPosOnScreen].x,
                         item->zHudStatusPosTable[eHudStatusPosOnScreen].y, 1000.0f,
                         eHudInterpTypeDecel);
            break;
        case eHudStatusPosOffScreenDefault:
            zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                         item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].x,
                         item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].y, 125.0f,
                         eHudInterpTypeLinear);
            break;
        case eHudStatusPosMiddleOfScreen:
            zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                         item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].x,
                         item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].y, 250.0f,
                         eHudInterpTypeDecel);
            break;
        }

        switch (item->statusAng)
        {
        case eHudStatusAngStop:
        case eHudStatusAngIdleReverse:
        case eHudStatusAngSlow:
        case eHudStatusAngSlowReverse:
        case eHudStatusAngFastReverse:
        case eHudStatusAngHyper:
        case eHudStatusAngHyperReverse:
        case eHudStatusAngTest2:
            zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngStop].x,
                         1.0f, eHudInterpTypeLinear);
            break;
        case eHudStatusAngIdle:
            zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngIdle].x,
                         1.0f, eHudInterpTypeLinear);
            break;
        case eHudStatusAngFast:
            zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngFast].x,
                         1.0f, eHudInterpTypeLinear);
            break;
        case eHudStatusAngTest1:
            zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngTest1].x,
                         1.0f, eHudInterpTypeLinear);
            zHudInterp1d(dt, &item->hframe.dang.y, item->zHudStatusAngTable[eHudStatusAngTest1].y,
                         1.0f, eHudInterpTypeLinear);
            zHudInterp1d(dt, &item->hframe.dang.z, item->zHudStatusAngTable[eHudStatusAngTest1].z,
                         1.0f, eHudInterpTypeLinear);
            break;
        }

        switch (item->statusSize)
        {
        case eHudStatusSizeDefault:
            zHudInterp1d(dt, &item->hframe.size.x,
                         item->zHudStatusSizeTable[eHudStatusSizeDefault].x, 50.0f,
                         eHudInterpTypeLinear);
            zHudInterp1d(dt, &item->hframe.size.y,
                         item->zHudStatusSizeTable[eHudStatusSizeDefault].y, 50.0f,
                         eHudInterpTypeLinear);
            break;
        case eHudStatusSizeZero:
            zHudInterp1d(dt, &item->hframe.size.x, item->zHudStatusSizeTable[eHudStatusSizeZero].x,
                         50.0f, eHudInterpTypeLinear);
            zHudInterp1d(dt, &item->hframe.size.y, item->zHudStatusSizeTable[eHudStatusSizeZero].y,
                         50.0f, eHudInterpTypeLinear);
            break;
        }

        switch (item->statusColor)
        {
        case eHudStatusColorSolid:
            zHudInterp1d(dt, &item->hframe.color.a,
                         item->zHudStatusColorTable[eHudStatusColorSolid].a, 450.0f,
                         eHudInterpTypeLinear);
            break;
        case eHudStatusColorTransparent:
            zHudInterp1d(dt, &item->hframe.color.a,
                         item->zHudStatusColorTable[eHudStatusColorTransparent].a, 600.0f,
                         eHudInterpTypeLinear);
            break;
        }

        item->hframe.ang.x = item->hframe.dang.x * dt + item->hframe.ang.x;
        item->hframe.ang.y = item->hframe.dang.y * dt + item->hframe.ang.y;
        item->hframe.ang.z = item->hframe.dang.z * dt + item->hframe.ang.z;

        xModelUpdate(item->model, dt);
        xModelEval(item->model);
    }
}

void zHudElementsCB_HealthbarRender()
{
    for (signed int i = 0; i < 5; i++)
    {
        zHudItem* item = &sHudHealthbarArray[i];
        if (item->hframe.color.a != 0.0f)
        {
            _xRect r = { 0.0f, 0.0f, 0.0f, 0.0f };
            _xMat4x3 frame;

            r.x = -(0.5f * item->hframe.size.x - item->hframe.pos.x);
            r.y = -(0.5f * item->hframe.size.y - item->hframe.pos.y);
            r.w = item->hframe.size.x * widthAspect;
            r.h = item->hframe.size.y;
            _xVec3 unused = { 0.0f, 0.0f, 0.0f };
            _xVec3 from = { 0.0f, 0.0f, 1.0f };
            _xVec3 to = { 0.0f, 0.0f, 0.0f };

            xMat3x3Euler((_xMat3x3*)&frame, item->hframe.ang.x, item->hframe.ang.y,
                         item->hframe.ang.z);
            frame.pos.x = 0.0f;
            frame.pos.y = 0.0f;
            frame.pos.z = 0.0f;
            frame.flags = 0;
            xModelSetMaterialAlpha(item->model, item->hframe.color.a);
            memcpy(item->model->Mat, &frame, sizeof(_xMat4x3));
            fn_80030494(item->renderType, item->hframe.color.a);
            xModelRender2D(item->model, &r, &from, &to, 1.5707964f);
        }
    }
}

void zHudElementsCB_GameStatsInit()
{
    unsigned int size;
    void* data;

    sHudGameStatsArray[4].model = zHudLoadModel(eHudModelIndividualScoobySnack);
    sHudGameStatsArray[4].renderType = 10;
    sHudGameStatsArray[5].model = zHudLoadModel(eHudModelClue);
    sHudGameStatsArray[5].renderType = 15;
    sHudGameStatsArray[6].model = zHudLoadModel(eHudModelIndividualScoobySnack);
    sHudGameStatsArray[6].renderType = 10;

    // NOTE: the original sets up the animations on the player status items here.
    for (signed int i = 4; i < 6; i++)
    {
        zHudItem* item = &sHudPlayerStatusArray[i];

        switch (i)
        {
        case 4:
            data = xSTFindAsset(xStrHash("rhsd0001.anm"), &size);
            break;
        case 5:
            data = xSTFindAsset(xStrHash("rhsd0001.anm"), &size);
            break;
        case 6:
            data = xSTFindAsset(xStrHash("rhsd0001.anm"), &size);
            break;
        }

        if (data != NULL)
        {
            xAnimFile* afile = xAnimFileNew(data, "", 0, NULL);
            item->atbl = xAnimTableNew("", NULL);
            xAnimTableNewState(item->atbl, "Idle", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL);
            xAnimTableAddFile(item->atbl, afile, "Idle");
            xAnimPoolAlloc(&globals.sceneCur->mempool, item, item->atbl, item->model);
        }
    }
}

void zHudElementsCB_GameStatsSetup()
{
    for (signed int i = 0; i < 7; i++)
    {
        zHudItem* item = &sHudGameStatsArray[i];

        if (i >= 0 && i < 4)
        {
            item->zHudStatusPosTable[eHudStatusPosOnScreen].x = 260.0f;
            item->zHudStatusPosTable[eHudStatusPosOnScreen].y = 60.0f * i + 150.0f;
        }
        else
        {
            switch (i)
            {
            case 4:
                item->zHudStatusPosTable[eHudStatusPosOnScreen].x = 205.0f;
                item->zHudStatusPosTable[eHudStatusPosOnScreen].y = 60.0f * (i - 4) + 170.0f;
                break;
            case 5:
                item->zHudStatusPosTable[eHudStatusPosOnScreen].x = 205.0f;
                item->zHudStatusPosTable[eHudStatusPosOnScreen].y = 60.0f * (i - 4) + 170.0f;
                break;
            case 6:
                item->zHudStatusPosTable[eHudStatusPosOnScreen].x = 205.0f;
                item->zHudStatusPosTable[eHudStatusPosOnScreen].y = 60.0f * (i - 4) + 170.0f;
                break;
            }
        }

        switch (i)
        {
        case 0:
        case 2:
            item->zHudStatusPosTable[eHudStatusPosOffScreenAlternate1].x = 20.0f;
            item->zHudStatusPosTable[eHudStatusPosOffScreenAlternate1].y = 60.0f * i + 150.0f;
            break;
        case 1:
        case 3:
            item->zHudStatusPosTable[eHudStatusPosOffScreenAlternate1].x = 500.0f;
            item->zHudStatusPosTable[eHudStatusPosOffScreenAlternate1].y = 60.0f * i + 150.0f;
            break;
        case 4:
            item->zHudStatusPosTable[eHudStatusPosOffScreenAlternate1].x = -35.0f;
            item->zHudStatusPosTable[eHudStatusPosOffScreenAlternate1].y = 170.0f;
            break;
        case 5:
            item->zHudStatusPosTable[eHudStatusPosOffScreenAlternate1].x = 445.0f;
            item->zHudStatusPosTable[eHudStatusPosOffScreenAlternate1].y = 230.0f;
            break;
        case 6:
            item->zHudStatusPosTable[eHudStatusPosOffScreenAlternate1].x = -35.0f;
            item->zHudStatusPosTable[eHudStatusPosOffScreenAlternate1].y = 290.0f;
            break;
        }

        switch (i)
        {
        case 4:
            item->zHudStatusSizeTable[eHudStatusSizeDefault].x = 80.0f;
            item->zHudStatusSizeTable[eHudStatusSizeDefault].y = 80.0f;
            break;
        case 5:
            item->zHudStatusSizeTable[eHudStatusSizeDefault].x = 30.0f;
            item->zHudStatusSizeTable[eHudStatusSizeDefault].y = 30.0f;
            break;
        case 6:
            item->zHudStatusSizeTable[eHudStatusSizeDefault].x = 80.0f;
            item->zHudStatusSizeTable[eHudStatusSizeDefault].y = 80.0f;
            break;
        default:
            item->zHudStatusSizeTable[eHudStatusSizeDefault].x = 70.0f;
            item->zHudStatusSizeTable[eHudStatusSizeDefault].y = 70.0f;
            break;
        }

        item->zHudStatusAngTable[eHudStatusAngStop].x = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngStop].y = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngStop].z = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngIdle].x = 1.5915494f;
        item->zHudStatusAngTable[eHudStatusAngIdle].y = 0.0f;
        item->zHudStatusAngTable[eHudStatusAngIdle].z = 0.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].rm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].gm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].bm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorSolid].a = 200.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].rm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].gm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].bm = 1.0f;
        item->zHudStatusColorTable[eHudStatusColorTransparent].a = 0.0f;
        item->statusPos = eHudStatusPosOffScreenAlternate1;
        item->statusAng = eHudStatusAngIdle;
        item->statusSize = eHudStatusSizeDefault;
        item->statusColor = eHudStatusColorTransparent;
        item->hframe.pos.x = item->zHudStatusPosTable[eHudStatusPosOffScreenAlternate1].x;
        item->hframe.pos.y = item->zHudStatusPosTable[eHudStatusPosOffScreenAlternate1].y;
        item->hframe.size.x = item->zHudStatusSizeTable[eHudStatusSizeDefault].x;
        item->hframe.size.y = item->zHudStatusSizeTable[eHudStatusSizeDefault].y;
        item->hframe.color.a = item->zHudStatusColorTable[eHudStatusColorTransparent].a;
    }
}

void zHudElementsCB_GameStatsUpdate(float dt)
{
    if (zGameStateGet() == 4)
    {
        if (sStatInit == 0)
        {
            sStatInit = 1;
            iColor_tag black = {};
            iColor_tag fade = { 0x00, 0x00, 0x00, 0xFF };
            xScrFxFade(&black, &fade, 0.9f, NULL, 1, 1);
            zEntEventAllOfType(8, 3);
        }

        if (sHudGameStatsTimer == 0)
        {
            sHudGameStatsTimer = 30;
        }
        else
        {
            sHudGameStatsTimer = sHudGameStatsTimer - dt;
            if (sHudGameStatsTimer <= 0)
            {
                sHudGameStatsTimer = 0;
                zGameStateSwitch(5);
            }
        }

        if (tmpRoomTime == 0.0f)
        {
            tmpRoomTime = globals.stats.roomTime;
        }
    }
    else
    {
        sStatInit = 0;
    }

    for (signed int i = 0; i < 7; i++)
    {
        zHudItem* item = &sHudGameStatsArray[i];

        switch (item->statusPos)
        {
        case eHudStatusPosOnScreen:
            zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                         item->zHudStatusPosTable[eHudStatusPosOnScreen].x,
                         item->zHudStatusPosTable[eHudStatusPosOnScreen].y, 500.0f,
                         eHudInterpTypeDecel);
            break;
        case eHudStatusPosOffScreenDefault:
            zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                         item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].x,
                         item->zHudStatusPosTable[eHudStatusPosOffScreenDefault].y, 500.0f,
                         eHudInterpTypeDecel);
            break;
        case eHudStatusPosOffScreenAlternate1:
            zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                         item->zHudStatusPosTable[eHudStatusPosOffScreenAlternate1].x,
                         item->zHudStatusPosTable[eHudStatusPosOffScreenAlternate1].y, 500.0f,
                         eHudInterpTypeDecel);
            break;
        case eHudStatusPosOffScreenAlternate2:
            zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                         item->zHudStatusPosTable[eHudStatusPosOffScreenAlternate2].x,
                         item->zHudStatusPosTable[eHudStatusPosOffScreenAlternate2].y, 500.0f,
                         eHudInterpTypeDecel);
            break;
        case eHudStatusPosMiddleOfScreen:
            zHudInterp2d(dt, &item->hframe.pos.x, &item->hframe.pos.y,
                         item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].x,
                         item->zHudStatusPosTable[eHudStatusPosMiddleOfScreen].y, 250.0f,
                         eHudInterpTypeDecel);
            break;
        }

        switch (item->statusAng)
        {
        case eHudStatusAngStop:
            zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngStop].x,
                         1.0f, eHudInterpTypeLinear);
            break;
        case eHudStatusAngIdle:
            zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngIdle].x,
                         1.0f, eHudInterpTypeLinear);
            break;
        case eHudStatusAngTest1:
            zHudInterp1d(dt, &item->hframe.dang.x, item->zHudStatusAngTable[eHudStatusAngTest1].x,
                         1.0f, eHudInterpTypeLinear);
            zHudInterp1d(dt, &item->hframe.dang.y, item->zHudStatusAngTable[eHudStatusAngTest1].y,
                         1.0f, eHudInterpTypeLinear);
            zHudInterp1d(dt, &item->hframe.dang.z, item->zHudStatusAngTable[eHudStatusAngTest1].z,
                         1.0f, eHudInterpTypeLinear);
            break;
        }

        switch (item->statusSize)
        {
        case eHudStatusSizeDefault:
            zHudInterp1d(dt, &item->hframe.size.x,
                         item->zHudStatusSizeTable[eHudStatusSizeDefault].x, 90.0f,
                         eHudInterpTypeLinear);
            zHudInterp1d(dt, &item->hframe.size.y,
                         item->zHudStatusSizeTable[eHudStatusSizeDefault].y, 90.0f,
                         eHudInterpTypeLinear);
            break;
        case eHudStatusSizeZero:
            zHudInterp1d(dt, &item->hframe.size.x, item->zHudStatusSizeTable[eHudStatusSizeZero].x,
                         90.0f, eHudInterpTypeLinear);
            zHudInterp1d(dt, &item->hframe.size.y, item->zHudStatusSizeTable[eHudStatusSizeZero].y,
                         90.0f, eHudInterpTypeLinear);
            break;
        }

        switch (item->statusColor)
        {
        case eHudStatusColorSolid:
        case eHudStatusColorLightBlue:
        case eHudStatusColorOpaque2:
            zHudInterp1d(dt, &item->hframe.color.a,
                         item->zHudStatusColorTable[eHudStatusColorSolid].a, 300.0f,
                         eHudInterpTypeLinear);
            break;
        case eHudStatusColorTransparent:
            zHudInterp1d(dt, &item->hframe.color.a,
                         item->zHudStatusColorTable[eHudStatusColorTransparent].a, 300.0f,
                         eHudInterpTypeLinear);
            break;
        }

        switch (i)
        {
        case 0:
            sprintf(item->text, "x %d%/%d", (signed int)(float)globals.stats.snacks,
                    (signed int)(float)globals.stats.snacks_tot);
            break;
        case 1:
            sprintf(item->text, "x %d%/%d", (signed int)(float)globals.stats.clues,
                    (signed int)(float)globals.stats.clues_tot);
            break;
        case 2:
            sprintf(item->text, "x %d%/%d", (signed int)(float)globals.stats.enemyKills,
                    (signed int)(float)globals.stats.enemyCount);
            break;
        case 3:
            sprintf(item->text, "Time - %dh %dm %ds", (signed int)(tmpRoomTime / 3600.0f),
                    (signed int)(tmpRoomTime / 60.0f), (signed int)tmpRoomTime % 60);
            break;
        }

        item->hframe.ang.x = item->hframe.dang.x * dt + item->hframe.ang.x;
        item->hframe.ang.y = item->hframe.dang.y * dt + item->hframe.ang.y;
        item->hframe.ang.z = item->hframe.dang.z * dt + item->hframe.ang.z;
    }
}

void zHudElementsCB_GameStatsRender()
{
    if (zGameStateGet() == 4)
    {
        for (signed int i = 0; i < 7; i++)
        {
            zHudItem* item = &sHudGameStatsArray[i];
            if (item->hframe.color.a != 0.0f)
            {
                if (i != 3)
                {
                    zHud_DrawText(item->hframe.pos.x, item->hframe.pos.y, 300.0f, 50.0f,
                                  item->text, 0x10, 1.0f, item->hframe.color.a,
                                  DROPSHADOW_NORMAL);
                }

                if (i == 4 || i == 5 || i == 6)
                {
                    _xRect r = { 0.0f, 0.0f, 0.0f, 0.0f };
                    _xMat4x3 frame;

                    r.x = -(0.5f * item->hframe.size.x - item->hframe.pos.x);
                    r.y = -(0.5f * item->hframe.size.y - item->hframe.pos.y);
                    r.w = item->hframe.size.x * widthAspect;
                    r.h = item->hframe.size.y;
                    _xVec3 unused = { 0.0f, 0.0f, 0.0f };
                    _xVec3 from = { 0.0f, 0.0f, 0.8f };
                    _xVec3 to = { 0.0f, 0.0f, 0.0f };

                    xMat3x3Euler((_xMat3x3*)&frame, item->hframe.ang.x, item->hframe.ang.y,
                                 item->hframe.ang.z);
                    frame.pos.x = 0.0f;
                    frame.pos.y = 0.0f;
                    frame.pos.z = 0.0f;
                    frame.flags = 0;
                    xModelSetMaterialAlpha(item->model, item->hframe.color.a);
                    memcpy(item->model->Mat, &frame, sizeof(_xMat4x3));
                    fn_80030494(item->renderType, item->hframe.color.a);
                    xModelRender2D(item->model, &r, &from, &to, 1.5707964f);
                }
            }
        }
    }
}

// Stand-in for a function the linker strips; it keeps 15.0f, 20.0f and 5.0f
// between zHud_DrawText's and zHudElementsCB_GameStatsRender's constants in
// .sdata2.
void zHudLayoutDeadStripped(float* v)
{
    v[0] = 15.0f;
    v[1] = 20.0f;
    v[2] = 5.0f;
}

#pragma auto_inline on
void zHud_DrawText(float x, float y, float w, float h, char* s, signed int just, float size,
                   unsigned char alpha, eHudDropShadowMode shadow)
{
    float drawSizeX = 24.0f * size;
    float drawSizeY = 32.0f * size;

    clr.a = alpha;
    clrDropShadow.a = alpha;

    if (shadow == DROPSHADOW_NORMAL)
    {
        fontBox = xFontBoxCreate(x + -3.0f, y + 3.0f, w, h, 0, clrDropShadow);
        xFontBoxSetJustification(fontBox, just);
        xFontBoxSetDrawSize(fontBox, drawSizeX, drawSizeY);
        xFontBoxSetDrawSpacing(fontBox, drawSizeX, drawSizeY);
        xFontBoxSetColor(fontBox, clrDropShadow);
        xFontBoxSetText(fontBox, s);
        xFontBoxRender(fontBox);
        xFontBoxDestroy(fontBox);
    }

    fontBox = xFontBoxCreate(x, y, w, h, 0, clr);
    xFontBoxSetJustification(fontBox, just);
    xFontBoxSetDrawSize(fontBox, drawSizeX, drawSizeY);
    xFontBoxSetDrawSpacing(fontBox, drawSizeX, drawSizeY);
    xFontBoxSetColor(fontBox, clr);
    xFontBoxSetText(fontBox, s);
    xFontBoxRender(fontBox);
    xFontBoxDestroy(fontBox);
}
#pragma auto_inline reset

// Stand-in for a function the linker strips; it keeps 0.75f and the int to
// float conversion constant ahead of zHud_DrawText's constants in .sdata2.
float zHudAspectDeadStripped(signed int x)
{
    return 0.75f * x;
}

void zHud_UIRender(char* asset, float xPos, float yPos, float w, float h, unsigned char alpha)
{
    char unused[256] = {};
    void* texture = xSTFindAsset(xStrHash(asset), NULL);
    if (texture != NULL && *(void**)texture != NULL)
    {
        RwRenderStateSet(1, *(void**)texture);
    }

    float x2 = xPos + w;
    float y2 = yPos + h;
    float z = RwIm2DGetNearScreenZ();
    unsigned char r = 0xFF;
    unsigned char g = 0xFF;
    unsigned char b = 0xFF;

    Vert[0].r = r;
    Vert[0].g = g;
    Vert[0].b = b;
    Vert[0].a = 0xFF;
    Vert[1].r = r;
    Vert[1].g = g;
    Vert[1].b = b;
    Vert[1].a = 0xFF;
    Vert[2].r = r;
    Vert[2].g = g;
    Vert[2].b = b;
    Vert[2].a = 0xFF;
    Vert[3].r = r;
    Vert[3].g = g;
    Vert[3].b = b;
    Vert[3].a = 0xFF;

    Vert[0].x = xPos;
    Vert[0].y = yPos;
    Vert[0].z = z;
    Vert[0].u = 0.0f;
    Vert[0].v = 0.0f;
    Vert[1].x = xPos;
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
    Vert[3].y = yPos;
    Vert[3].z = z;
    Vert[3].u = 1.0f;
    Vert[3].v = 0.0f;

    Vert[0].r = r;
    Vert[0].g = g;
    Vert[0].b = b;
    Vert[0].a = alpha;
    Vert[1].r = r;
    Vert[1].g = g;
    Vert[1].b = b;
    Vert[1].a = alpha;
    Vert[2].r = r;
    Vert[2].g = g;
    Vert[2].b = b;
    Vert[2].a = alpha;
    Vert[3].r = r;
    Vert[3].g = g;
    Vert[3].b = b;
    Vert[3].a = alpha;

    zRenderState(SDRS_Unk1);
    RwRenderStateSet(0xE, (void*)0);
    RwRenderStateSet(0xA, (void*)5);
    RwRenderStateSet(0xB, (void*)6);
    RwRenderStateSet(0xC, (void*)1);
    RwRenderStateSet(7, (void*)1);
    RwRenderStateSet(8, (void*)0);
    RwIm2DRenderIndexedPrimitive(3, Vert, 4, Idx, 6);
}

void zHud_sceneReset()
{
    tmpRoomTime = 0.0f;
}

#pragma auto_inline on
void zHudShow()
{
    sHudDisplayTimer_plrStatus = 2.0f;
    sHudDisplayTimer_snacks = 2.0f;
    sHudDisplayTimer_gum = 1.5f;
    sHudDisplayTimer_soap = 1.5f;
    sHudDisplayTimer_healthBar = 2.0f;
}
#pragma auto_inline reset

#pragma auto_inline on
void zHudHide()
{
    sHudDisplayTimer_plrStatus = 0.0f;
    sHudDisplayTimer_snacks = 0.0f;
    sHudDisplayTimer_gum = 0.0f;
    sHudDisplayTimer_soap = 0.0f;
    sHudDisplayTimer_healthBar = 0.0f;
}
#pragma auto_inline reset

void zHudInterp2d(float dt, float* srcx, float* srcy, float dstx, float dsty, float speed,
                  signed int type)
{
    if (0.0f != speed)
    {
        if (*srcx != dstx || *srcy != dsty)
        {
                float dx = dstx - *srcx;
                float dy = dsty - *srcy;
                _xVec2 t = {};
                t.x = dx;
                t.y = dy;
                _xVec2 tNorm = {};
                float dx2 = dx * dx;
                float dy2 = dy * dy;
                float d2 = dx2 + dy2;
                if (d2 == 1.0f)
                {
                    if (&tNorm != &t)
                    {
                        tNorm = t;
                    }
                }
                else if (d2 == 0.0f)
                {
                    if (&tNorm != &t)
                    {
                        tNorm = t;
                    }
                }
                else
                {
                    d2 = xsqrt(d2);
                    float inv = 1.0f / d2;
                    tNorm.x = t.x * inv;
                    tNorm.y = t.y * inv;
                }

                switch (type)
                {
                case eHudInterpTypeLinear:
                {
                    float mx = dt * (tNorm.x * speed);
                    if ((float)__fabs(mx) > (float)__fabs(dx))
                    {
                        mx = dx;
                    }
                    *srcx += mx;
                    float my = dt * (tNorm.y * speed);
                    if ((float)__fabs(my) > (float)__fabs(dy))
                    {
                        my = dy;
                    }
                    *srcy += my;
                    break;
                }
                case eHudInterpTypeDecel:
                {
                    float mx;
                    if ((float)__fabs(dx) < 0.1f)
                    {
                        mx = dx;
                    }
                    else
                    {
                        mx = dt * ((0.05f * dx * speed) / 10.0f);
                    }
                    *srcx += mx;
                    float my;
                    if ((float)__fabs(dy) < 0.1f)
                    {
                        my = dy;
                    }
                    else
                    {
                        my = dt * ((0.05f * dy * speed) / 10.0f);
                    }
                    *srcy += my;
                    break;
                }
                case eHudInterpTypeAccel:
                {
                    float k = 10000.0f / (dx * dx + dy * dy);
                    float mx;
                    if (dt * (speed * (float)__fabs(tNorm.x * k)) > (float)__fabs(dx))
                    {
                        mx = dx;
                    }
                    else
                    {
                        mx = dt * (speed * (tNorm.x * (float)__fabs(k)));
                    }
                    *srcx += mx;
                    float my;
                    if (dt * (speed * (float)__fabs(tNorm.y * k)) > (float)__fabs(dy))
                    {
                        my = dy;
                    }
                    else
                    {
                        my = dt * (speed * (tNorm.y * (float)__fabs(k)));
                    }
                    *srcy += my;
                    break;
                }
                }
        }
    }
    else
    {
        *srcx = dstx;
        *srcy = dsty;
    }
}

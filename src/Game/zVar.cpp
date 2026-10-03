// TODO: .data/.rodata/.sbss each end 4 bytes short of the split (trailing alignment padding).

#include "zVar.h"
#include "zGlobals.h"
#include "xserializer.h"
#include "zScene.h"
#include "xSnd.h"
#include "iSnd.h"

extern "C" {
int sprintf(char* s, const char* format, ...);
char* strncpy(char* dst, const char* src, unsigned int n);
int atoi(const char* s);
signed int fn_800C9698(char* buffer);
signed int fn_800C96C0(char* buffer);
signed int fn_800C96E8(char* buffer);
signed int fn_800C9710(char* buffer);
unsigned int fn_800D8C14();
}

signed int zSaveLoad_getcard();
signed int zSaveLoad_getgame();

#ifndef NULL
#define NULL 0
#endif

#define SCENE_ID(s) ((((s)[0] & ~0x20) << 24) | ((s)[1] << 16) | ((s)[2] << 8) | (s)[3])

enum _tagSceneAreas
{
    eSceneArea_None = 0,
};

class st_XSAVEGAME_DATA;
class zSaveLoadGame
{
public:
    char label[64]; // 0x0
    char date[32]; // 0x40
    unsigned char pad[4];
};


unsigned int zCutSceneNamesTable_getStatus(unsigned int idx);
unsigned int zEntPickup_WarpIsOpen(unsigned int idx);
unsigned int zSceneGetAreaTotalSnacks(_tagSceneAreas area);
unsigned int zSceneGetAreaTotalSnacks(unsigned int sceneID);
unsigned int zSceneGetAreaTotalTokens(unsigned int sceneID);
unsigned int zSceneGetAreaSnacksCollected(unsigned int sceneID);
unsigned int zSceneGetAreaTokensCollected(unsigned int sceneID);
char* zSceneGetAreaname(unsigned int sceneID);
char* xUtil_idtag2string(unsigned int srctag, signed int bufidx);
signed int iGetCurrFormattedTime(char* buffer);
signed int iGetCurrFormattedDate(char* buffer);
// Declared with an int return here; zVar's call sites compare the full register.
signed int xSGGameIsEmpty(st_XSAVEGAME_DATA* xsgdata, signed int gidx);
unsigned int xStrHash(const char* str);
signed int xStricmp(char* a, char* b);
void zSaveLoad_BuildName(char* buffer, signed int slot);

extern zSaveLoadGame zSaveLoadGameTable[];

class zVarMagicEntry
{
public:
    unsigned int varNameID;
    char* varName;
    char* (*varCB)(char*);
};

static unsigned int hasMap;
static unsigned int R001Unlocked;
static unsigned int S001Unlocked;
static unsigned int E001Unlocked;
static unsigned int F001Unlocked;
static unsigned int MMDefeated;
static unsigned int endgame;
static unsigned int allmovies;
static unsigned int selSceneID;
static unsigned int sSnackGateValue;
unsigned int zVarEntryCB_eVarCin14();
unsigned int zVarEntryCB_eVarCin13();
unsigned int zVarEntryCB_eVarCin12();
unsigned int zVarEntryCB_eVarCin11();
unsigned int zVarEntryCB_eVarCin10();
unsigned int zVarEntryCB_eVarCin9();
unsigned int zVarEntryCB_eVarCin8();
unsigned int zVarEntryCB_eVarCin7();
unsigned int zVarEntryCB_eVarCin6();
unsigned int zVarEntryCB_eVarCin5();
unsigned int zVarEntryCB_eVarCin4();
unsigned int zVarEntryCB_eVarCin3();
unsigned int zVarEntryCB_eVarCin2();
unsigned int zVarEntryCB_eVarCin1();
unsigned int zVarEntryCB_eVarAllSnacksFound();
unsigned int zVarEntryCB_eVareVarDefeatedMM();
unsigned int zVarEntryCB_eVarWarpH003();
unsigned int zVarEntryCB_eVarWarpW026();
unsigned int zVarEntryCB_eVarWarpW022();
unsigned int zVarEntryCB_eVarWarpS003();
unsigned int zVarEntryCB_eVarWarpR003();
unsigned int zVarEntryCB_eVarWarpP005();
unsigned int zVarEntryCB_eVarWarpP003();
unsigned int zVarEntryCB_eVarWarpO006();
unsigned int zVarEntryCB_eVarWarpO004();
unsigned int zVarEntryCB_eVarWarpL018();
unsigned int zVarEntryCB_eVarWarpL015();
unsigned int zVarEntryCB_eVarWarpL014();
unsigned int zVarEntryCB_eVarWarpI006();
unsigned int zVarEntryCB_eVarWarpI003();
unsigned int zVarEntryCB_eVarWarpH001();
unsigned int zVarEntryCB_eVarWarpG008();
unsigned int zVarEntryCB_eVarWarpG005();
unsigned int zVarEntryCB_eVarWarpG001();
unsigned int zVarEntryCB_eVarWarpF010();
unsigned int zVarEntryCB_eVarWarpF007();
unsigned int zVarEntryCB_eVarWarpF003();
unsigned int zVarEntryCB_eVarWarpE009();
unsigned int zVarEntryCB_eVarWarpE006();
unsigned int zVarEntryCB_eVarWarpE004();
unsigned int zVarEntryCB_eVarWarpC004();
unsigned int zVarEntryCB_eVarWarpB004();
unsigned int zVarEntryCB_eVarMToken21();
unsigned int zVarEntryCB_eVarMToken20();
unsigned int zVarEntryCB_eVarMToken19();
unsigned int zVarEntryCB_eVarMToken18();
unsigned int zVarEntryCB_eVarMToken17();
unsigned int zVarEntryCB_eVarMToken16();
unsigned int zVarEntryCB_eVarMToken15();
unsigned int zVarEntryCB_eVarMToken14();
unsigned int zVarEntryCB_eVarMToken13();
unsigned int zVarEntryCB_eVarMToken12();
unsigned int zVarEntryCB_eVarMToken11();
unsigned int zVarEntryCB_eVarMToken10();
unsigned int zVarEntryCB_eVarMToken9();
unsigned int zVarEntryCB_eVarMToken8();
unsigned int zVarEntryCB_eVarMToken7();
unsigned int zVarEntryCB_eVarMToken6();
unsigned int zVarEntryCB_eVarMToken5();
unsigned int zVarEntryCB_eVarMToken4();
unsigned int zVarEntryCB_eVarMToken3();
unsigned int zVarEntryCB_eVarMToken2();
unsigned int zVarEntryCB_eVarMToken1();
unsigned int zVarEntryCB_CurrentRoom();
unsigned int zVarEntryCB_PuzzlePieceH();
unsigned int zVarEntryCB_PuzzlePieceO();
unsigned int zVarEntryCB_PuzzlePieceR();
unsigned int zVarEntryCB_PuzzlePieceI();
unsigned int zVarEntryCB_PuzzlePieceS();
unsigned int zVarEntryCB_PuzzlePieceB();
unsigned int zVarEntryCB_PuzzlePieceP();
unsigned int zVarEntryCB_PuzzlePieceG();
unsigned int zVarEntryCB_PuzzlePieceC();
unsigned int zVarEntryCB_PuzzlePieceE();
unsigned int zVarEntryCB_PuzzlePieceW();
unsigned int zVarEntryCB_PuzzlePieceL();
unsigned int zVarEntryCB_PuzzlePieceF();
void zVarUnlockAllMovies();
void zVarUnlockEndGame();
unsigned int finishedGame();
unsigned int zVarEntryCB_UnlockedF001();
unsigned int zVarEntryCB_UnlockedE001();
unsigned int zVarEntryCB_UnlockedS001();
unsigned int zVarEntryCB_UnlockedR001();
unsigned int zVarEntryCB_MapItem();
unsigned int zVarEntryCB_IsSneaking();
unsigned int zVarEntryCB_CurrentSceneLetter();
char* zVMCB_AltButtonLabel(char* buffer);
char* zVMCB_ActionButtonLabel(char* buffer);
char* zVMCB_CancelButtonLabel(char* buffer);
char* zVMCB_MCActionFailMessage(char* buffer);
char* zVMCB_MCActionMessage(char* buffer);
char* zVMCB_FormatPromptMessage(char* buffer);
char* zVMCB_SpaceNeeded(char* buffer);
char* zVMCB_SpaceAvailable(char* buffer);
char* zVMCB_NoCardMessage(char* buffer);
char* zVMCB_CurrentTime(char* buffer);
char* zVMCB_CurrentDate(char* buffer);
char* zVMCB_SnackGateValue(char* buffer);
char* zVMCB_SelectedArea(char* buffer);
char* zVMCB_AreaStatsTokens(char* buffer);
char* zVMCB_AreaStatsSnacks(char* buffer);
char* zVMCB_CurrentArea(char* buffer);
char* zVMCB_TokenWorldStats(char* buffer);
char* zVMCB_SnackWorldStats(char* buffer);
char* zVMCB_TokenAreaStats(char* buffer);
char* zVMCB_SnackAreaStats(char* buffer);
char* zVMCB_TokenRoomStats(char* buffer);
char* zVMCB_SnackRoomStats(char* buffer);
char* zVMCB_CurrentPosition(char* buffer);
char* zVMCB_MCSelectedCard(char* buffer);
char* zVMCB_MCSelectedGame(char* buffer);
char* zVMCB_GameSlot7(char* buffer);
char* zVMCB_GameSlot6(char* buffer);
char* zVMCB_GameSlot5(char* buffer);
char* zVMCB_GameSlot4(char* buffer);
char* zVMCB_GameSlot3(char* buffer);
char* zVMCB_GameSlot2(char* buffer);
char* zVMCB_GameSlot1(char* buffer);
char* zVMCB_GameSlot0(char* buffer);
char* zVMCB_Scene(char* buffer);
char* zVMCB_SoundFXVolume(char* buffer);
char* zVMCB_SoundMusicVolume(char* buffer);
char* zVMCB_PlayerChancesCount(char* buffer);
char* zVMCB_PlayerScoobySnackCount_Int(char* buffer);
void zVarDefeatMasterMind();
void zVarSetSnackGateValue(unsigned int amount);
void zVarSetArea(unsigned int sceneID);
void zVarUnlockF001();
void zVarUnlockE001();
void zVarUnlockS001();
void zVarUnlockR001();
void zVarGiveMap();
unsigned int zVarEntryCB_VibrationOn();
unsigned int zVarEntryCB_MCAvailable();
unsigned int zVarEntryCB_PCSpecialLightningBoltStun();
unsigned int zVarEntryCB_PCSpecialShovel();
unsigned int zVarEntryCB_PCSpecialUmbrella();
unsigned int zVarEntryCB_PCSpecialFootballHelmet();
unsigned int zVarEntryCB_PCSpecialLightningBolt();
unsigned int zVarEntryCB_PCSpecialSpring();
unsigned int zVarEntryCB_PCSpecialDivingHelmet();
unsigned int zVarEntryCB_PCSpecialFlowerPot();
unsigned int zVarEntryCB_PCSpecialBlackKnight();
unsigned int zVarEntryCB_PCSpecialLampShade();
unsigned int zVarEntryCB_PCSpecialSlippers();
unsigned int zVarEntryCB_PCSpecialPlungers();
unsigned int zVarEntryCB_PCSpecialStickyBoots();
unsigned int zVarEntryCB_PCSpecialSoapBar();
unsigned int zVarEntryCB_PCSpecialBubbleGum();
unsigned int zVarEntryCB_SndFXVol();
unsigned int zVarEntryCB_SndMusicVol();
unsigned int zVarEntryCB_SndMasterVol();
unsigned int zVarEntryCB_SndMode();
unsigned int zVarEntryCB_PCScoobySnacks();
unsigned int zVarEntryCB_PCChances();
void zVarLoad(xSerial* s);
void zVarSave(xSerial* s);
void zVarNewGame();
void zVarInit(zVarEntry* table);
void zVarMagicProcessText(char* src, char* dst);

zVarEntry zVarEntryTable[108] = {
    { 0, 0, "Chances", zVarEntryCB_PCChances },
    { 1, 0, "Scooby Snacks", zVarEntryCB_PCScoobySnacks },
    { 2, 0, "Sound mode", zVarEntryCB_SndMode },
    { 3, 0, "Master Volume", zVarEntryCB_SndMasterVol },
    { 19, 0, "Music Volume", zVarEntryCB_SndMusicVol },
    { 4, 0, "SFX Volume", zVarEntryCB_SndFXVol },
    { 5, 0, "Bubble gum", zVarEntryCB_PCSpecialBubbleGum },
    { 6, 0, "Soap bar", zVarEntryCB_PCSpecialSoapBar },
    { 7, 0, "Sticky boots", zVarEntryCB_PCSpecialStickyBoots },
    { 8, 0, "Plungers", zVarEntryCB_PCSpecialPlungers },
    { 9, 0, "Slippers", zVarEntryCB_PCSpecialSlippers },
    { 10, 0, "Lamp shade", zVarEntryCB_PCSpecialLampShade },
    { 11, 0, "Black knight", zVarEntryCB_PCSpecialBlackKnight },
    { 12, 0, "Flower pos", zVarEntryCB_PCSpecialFlowerPot },
    { 13, 0, "Diving helmet", zVarEntryCB_PCSpecialDivingHelmet },
    { 14, 0, "Spring", zVarEntryCB_PCSpecialSpring },
    { 15, 0, "Lightning bolt", zVarEntryCB_PCSpecialLightningBolt },
    { 16, 0, "Football helmet", zVarEntryCB_PCSpecialFootballHelmet },
    { 17, 0, "Umbrella", zVarEntryCB_PCSpecialUmbrella },
    { 18, 0, "Shovel", zVarEntryCB_PCSpecialShovel },
    { 20, 0, "Memory Card Available", zVarEntryCB_MCAvailable },
    { 21, 0, "Vibration is on", zVarEntryCB_VibrationOn },
    { 22, 0, "Letter of scene", zVarEntryCB_CurrentSceneLetter },
    { 23, 0, "Room", zVarEntryCB_CurrentRoom },
    { 24, 0, "IsSneaking", zVarEntryCB_IsSneaking },
    { 25, 0, "Map Item Recieved", zVarEntryCB_MapItem },
    { 26, 0, "R001 Unlocked", zVarEntryCB_UnlockedR001 },
    { 27, 0, "S001 Unlocked", zVarEntryCB_UnlockedS001 },
    { 28, 0, "F Piece Collected", zVarEntryCB_PuzzlePieceF },
    { 29, 0, "L Piece Collected", zVarEntryCB_PuzzlePieceL },
    { 30, 0, "W Piece Collected", zVarEntryCB_PuzzlePieceW },
    { 31, 0, "E Piece Collected", zVarEntryCB_PuzzlePieceE },
    { 32, 0, "C Piece Collected", zVarEntryCB_PuzzlePieceC },
    { 33, 0, "G Piece Collected", zVarEntryCB_PuzzlePieceG },
    { 34, 0, "P Piece Collected", zVarEntryCB_PuzzlePieceP },
    { 35, 0, "B Piece Collected", zVarEntryCB_PuzzlePieceB },
    { 36, 0, "S Piece Collected", zVarEntryCB_PuzzlePieceS },
    { 37, 0, "I Piece Collected", zVarEntryCB_PuzzlePieceI },
    { 38, 0, "R Piece Collected", zVarEntryCB_PuzzlePieceR },
    { 39, 0, "O Piece Collected", zVarEntryCB_PuzzlePieceO },
    { 40, 0, "H Piece Collected", zVarEntryCB_PuzzlePieceH },
    { 41, 0, "F001 Unlocked", zVarEntryCB_UnlockedF001 },
    { 42, 0, "E001 Unlocked", zVarEntryCB_UnlockedE001 },
    { 43, 0, "Monster Token 1", zVarEntryCB_eVarMToken1 },
    { 44, 0, "Monster Token 2", zVarEntryCB_eVarMToken2 },
    { 45, 0, "Monster Token 3", zVarEntryCB_eVarMToken3 },
    { 46, 0, "Monster Token 4", zVarEntryCB_eVarMToken4 },
    { 47, 0, "Monster Token 5", zVarEntryCB_eVarMToken5 },
    { 48, 0, "Monster Token 6", zVarEntryCB_eVarMToken6 },
    { 49, 0, "Monster Token 7", zVarEntryCB_eVarMToken7 },
    { 50, 0, "Monster Token 8", zVarEntryCB_eVarMToken8 },
    { 51, 0, "Monster Token 9", zVarEntryCB_eVarMToken9 },
    { 52, 0, "Monster Token 10", zVarEntryCB_eVarMToken10 },
    { 53, 0, "Monster Token 11", zVarEntryCB_eVarMToken11 },
    { 54, 0, "Monster Token 12", zVarEntryCB_eVarMToken12 },
    { 55, 0, "Monster Token 13", zVarEntryCB_eVarMToken13 },
    { 56, 0, "Monster Token 14", zVarEntryCB_eVarMToken14 },
    { 57, 0, "Monster Token 15", zVarEntryCB_eVarMToken15 },
    { 58, 0, "Monster Token 16", zVarEntryCB_eVarMToken16 },
    { 59, 0, "Monster Token 17", zVarEntryCB_eVarMToken17 },
    { 60, 0, "Monster Token 18", zVarEntryCB_eVarMToken18 },
    { 61, 0, "Monster Token 19", zVarEntryCB_eVarMToken19 },
    { 62, 0, "Monster Token 20", zVarEntryCB_eVarMToken20 },
    { 63, 0, "Monster Token 21", zVarEntryCB_eVarMToken21 },
    { 64, 0, "Warp Point B4", zVarEntryCB_eVarWarpB004 },
    { 65, 0, "Warp Point C4", zVarEntryCB_eVarWarpC004 },
    { 66, 0, "Warp Point E4", zVarEntryCB_eVarWarpE004 },
    { 67, 0, "Warp Point E6", zVarEntryCB_eVarWarpE006 },
    { 68, 0, "Warp Point E9", zVarEntryCB_eVarWarpE009 },
    { 69, 0, "Warp Point F3", zVarEntryCB_eVarWarpF003 },
    { 70, 0, "Warp Point F7", zVarEntryCB_eVarWarpF007 },
    { 71, 0, "Warp Point F10", zVarEntryCB_eVarWarpF010 },
    { 88, 0, "Warp Point G1", zVarEntryCB_eVarWarpG001 },
    { 72, 0, "Warp Point G5", zVarEntryCB_eVarWarpG005 },
    { 73, 0, "Warp Point G8", zVarEntryCB_eVarWarpG008 },
    { 74, 0, "Warp Point H1", zVarEntryCB_eVarWarpH001 },
    { 75, 0, "Warp Point I3", zVarEntryCB_eVarWarpI003 },
    { 76, 0, "Warp Point I6", zVarEntryCB_eVarWarpI006 },
    { 77, 0, "Warp Point L14", zVarEntryCB_eVarWarpL014 },
    { 87, 0, "Warp Point L15", zVarEntryCB_eVarWarpL015 },
    { 78, 0, "Warp Point L18", zVarEntryCB_eVarWarpL018 },
    { 79, 0, "Warp Point O4", zVarEntryCB_eVarWarpO004 },
    { 80, 0, "Warp Point O6", zVarEntryCB_eVarWarpO006 },
    { 81, 0, "Warp Point P3", zVarEntryCB_eVarWarpP003 },
    { 82, 0, "Warp Point P5", zVarEntryCB_eVarWarpP005 },
    { 83, 0, "Warp Point R3", zVarEntryCB_eVarWarpR003 },
    { 84, 0, "Warp Point S3", zVarEntryCB_eVarWarpS003 },
    { 85, 0, "Warp Point W22", zVarEntryCB_eVarWarpW022 },
    { 86, 0, "Warp Point W26", zVarEntryCB_eVarWarpW026 },
    { 89, 0, "Lightning bolt stun", zVarEntryCB_PCSpecialLightningBoltStun },
    { 90, 0, "Defeated Mastermind", zVarEntryCB_eVareVarDefeatedMM },
    { 91, 0, "Warp Point H3", zVarEntryCB_eVarWarpH003 },
    { 92, 0, "AllSnacksBonus open", zVarEntryCB_eVarAllSnacksFound },
    { 93, 0, "Cinematic 1 Unlocked", zVarEntryCB_eVarCin1 },
    { 94, 0, "Cinematic 2 Unlocked", zVarEntryCB_eVarCin2 },
    { 95, 0, "Cinematic 3 Unlocked", zVarEntryCB_eVarCin3 },
    { 96, 0, "Cinematic 4 Unlocked", zVarEntryCB_eVarCin4 },
    { 97, 0, "Cinematic 5 Unlocked", zVarEntryCB_eVarCin5 },
    { 98, 0, "Cinematic 6 Unlocked", zVarEntryCB_eVarCin6 },
    { 99, 0, "Cinematic 7 Unlocked", zVarEntryCB_eVarCin7 },
    { 100, 0, "Cinematic 8 Unlocked", zVarEntryCB_eVarCin8 },
    { 101, 0, "Cinematic 9 Unlocked", zVarEntryCB_eVarCin9 },
    { 102, 0, "Cinematic 10 Unlocked", zVarEntryCB_eVarCin10 },
    { 103, 0, "Cinematic 11 Unlocked", zVarEntryCB_eVarCin11 },
    { 104, 0, "Cinematic 12 Unlocked", zVarEntryCB_eVarCin12 },
    { 105, 0, "Cinematic 13 Unlocked", zVarEntryCB_eVarCin13 },
    { 106, 0, "Cinematic 14 Unlocked", zVarEntryCB_eVarCin14 },
    { 0, 0, NULL, NULL },
};

zVarMagicEntry sVarMagicEntryTrable[] = {
    { 0, "PlayerChancesCount", zVMCB_PlayerChancesCount },
    { 0, "PlayerScoobySnackCount_Int", zVMCB_PlayerScoobySnackCount_Int },
    { 0, "SoundMusicVolume", zVMCB_SoundMusicVolume },
    { 0, "SoundFXVolume", zVMCB_SoundFXVolume },
    { 0, "CurrentScene", zVMCB_Scene },
    { 0, "GameSlot0", zVMCB_GameSlot0 },
    { 0, "GameSlot1", zVMCB_GameSlot1 },
    { 0, "GameSlot2", zVMCB_GameSlot2 },
    { 0, "GameSlot3", zVMCB_GameSlot3 },
    { 0, "GameSlot4", zVMCB_GameSlot4 },
    { 0, "GameSlot5", zVMCB_GameSlot5 },
    { 0, "GameSlot6", zVMCB_GameSlot6 },
    { 0, "GameSlot7", zVMCB_GameSlot7 },
    { 0, "MCSelectedGame", zVMCB_MCSelectedGame },
    { 0, "MCSelectedCard", zVMCB_MCSelectedCard },
    { 0, "PlayerPosition", zVMCB_CurrentPosition },
    { 0, "SnackRoomStats", zVMCB_SnackRoomStats },
    { 0, "TokenRoomStats", zVMCB_TokenRoomStats },
    { 0, "SnackAreaStats", zVMCB_SnackAreaStats },
    { 0, "TokenAreaStats", zVMCB_TokenAreaStats },
    { 0, "SnackWorldStats", zVMCB_SnackWorldStats },
    { 0, "TokenWorldStats", zVMCB_TokenWorldStats },
    { 0, "CurrentArea", zVMCB_CurrentArea },
    { 0, "AreaStatsSnacks", zVMCB_AreaStatsSnacks },
    { 0, "AreaStatsTokens", zVMCB_AreaStatsTokens },
    { 0, "SelectedArea", zVMCB_SelectedArea },
    { 0, "SnackGateValue", zVMCB_SnackGateValue },
    { 0, "CurrentDate", zVMCB_CurrentDate },
    { 0, "CurrentTime", zVMCB_CurrentTime },
    { 0, "NoCardMessage", zVMCB_NoCardMessage },
    { 0, "SpaceAvailable", zVMCB_SpaceAvailable },
    { 0, "SpaceNeeded", zVMCB_SpaceNeeded },
    { 0, "FormatPromptMessage", zVMCB_FormatPromptMessage },
    { 0, "MCActionMessage", zVMCB_MCActionMessage },
    { 0, "MCACtionFailMessage", zVMCB_MCActionFailMessage },
    { 0, "CancelButtonLabel", zVMCB_CancelButtonLabel },
    { 0, "ActionButtonLabel", zVMCB_ActionButtonLabel },
    { 0, "AltButtonLabel", zVMCB_AltButtonLabel },
    { 0, NULL, NULL },
};

static signed int sVarMagicInit;
char txt[64];

static inline double xround(double x)
{
    return 0.5 + x;
}

static inline unsigned char zVarIsLower(char c)
{
    unsigned char lower = 0;
    if (c >= 'a' && c <= 'z')
    {
        lower = 1;
    }
    return lower;
}

static inline signed int zVarToUpper(char c)
{
    if (zVarIsLower(c))
    {
        return c - 0x20;
    }
    return c;
}

static inline unsigned int zVarPlayerAnimFlags()
{
    return *(unsigned int*)(*(char**)(*(char**)(*(char**)((char*)globals.player.ent.model + 0xC) + 8) + 4) + 0x10);
}

static inline void zVarGameSlotInfo(signed int i)
{
    char date[0x20] = "";
    char time[0x20] = "";
    char ampm[0x20] = "";
    char biggerbuf[256] = "";
    signed int hour;

    strncpy(date, zSaveLoadGameTable[i].date, 5);
    date[2] = '/';
    sprintf(biggerbuf, "%s/%c%c", date, zSaveLoadGameTable[i].date[8], zSaveLoadGameTable[i].date[9]);
    strncpy(date, biggerbuf, 0x20);
    date[0x20 - 1] = '\0';

    sprintf(time, "%c%c", zSaveLoadGameTable[i].date[0xB], zSaveLoadGameTable[i].date[0xC]);
    hour = atoi(time);
    if (hour >= 12)
    {
        strncpy(ampm, "PM", 3);
        if (hour != 12)
        {
            hour -= 12;
        }
    }
    else
    {
        strncpy(ampm, "AM", 3);
        if (hour == 0)
        {
            hour = 12;
        }
    }

    sprintf(time, "%d:%c%c %s", hour, zSaveLoadGameTable[i].date[0xE], zSaveLoadGameTable[i].date[0xF], ampm);
    sprintf(date, "%s %s", date, time);
    time[0x20 - 1] = '\0';
    date[0x20 - 1] = '\0';

    if (xStricmp(zSaveLoadGameTable[i].label, "Empty") == 0)
    {
        sprintf(txt, "Empty");
    }
    else
    {
        zSaveLoad_BuildName(txt, i);
    }
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

static inline void zVarMagicInit()
{
    for (zVarMagicEntry* e = sVarMagicEntryTrable; e->varName; e++)
    {
        e->varNameID = xStrHash(e->varName);
    }
    sVarMagicInit = 1;
}

void zVarMagicProcessText(char* src, char* dst)
{
    char name[256];

    if (!sVarMagicInit)
    {
        zVarMagicInit();
    }

    while (*src)
    {
        if (src[0] == '$' && src[1] == 'V')
        {
            char* n = name;
            src += 2;
            while (*src && *src != '$')
            {
                *n++ = *src++;
            }
            *n = '\0';
            if (*src == '$')
            {
                src++;
            }

            unsigned int id = xStrHash(name);
            for (zVarMagicEntry* e = sVarMagicEntryTrable; e->varName; e++)
            {
                if (e->varNameID == id)
                {
                    dst = e->varCB(dst);
                    break;
                }
            }
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

void zVarInit(zVarEntry* table)
{
    for (signed int i = 0; i < 107; i++)
    {
        table[i].varNameID = xStrHash(table[i].varName);
    }
}

void zVarNewGame()
{
    hasMap = 0;
    R001Unlocked = 0;
    S001Unlocked = 0;
    E001Unlocked = 0;
    F001Unlocked = 0;
    MMDefeated = 0;
    allmovies = 0;
}

void zVarSave(xSerial* s)
{
    s->Write_b1((unsigned char)(hasMap != 0));
    s->Write_b1((unsigned char)(R001Unlocked != 0));
    s->Write_b1((unsigned char)(S001Unlocked != 0));
    s->Write_b1((unsigned char)(E001Unlocked != 0));
    s->Write_b1((unsigned char)(F001Unlocked != 0));
    s->Write_b1((unsigned char)(MMDefeated != 0));
}

void zVarLoad(xSerial* s)
{
    s->Read_b1(&hasMap);
    s->Read_b1(&R001Unlocked);
    s->Read_b1(&S001Unlocked);
    s->Read_b1(&E001Unlocked);
    s->Read_b1(&F001Unlocked);
    s->Read_b1(&MMDefeated);
}

unsigned int zVarEntryCB_PCChances()
{
    return globals.player.Inv_Chances;
}

unsigned int zVarEntryCB_PCScoobySnacks()
{
    return globals.player.Inv_ScoobySnacks;
}

unsigned int zVarEntryCB_SndMode()
{
    return fn_800D8C14();
}

// TODO: 96.8%/97.9% - same instructions, but the float registers are numbered differently
// (target: sum in f3, 0.5f in f2, 10.0 in f1).
unsigned int zVarEntryCB_SndMasterVol()
{
    double d = (gSnd.mastervol.left + gSnd.mastervol.right) * 0.5f * 10.0;
    d = 0.5 + d;
    return (unsigned int)d;
}

// TODO: 96.8%/97.9% - same instructions, but the float registers are numbered differently
// (target: sum in f3, 0.5f in f2, 10.0 in f1).
unsigned int zVarEntryCB_SndMusicVol()
{
    double d = (gSnd.streamvol.left + gSnd.streamvol.right) * 0.5f * 10.0;
    d = 0.5 + d;
    return (unsigned int)d;
}

// TODO: 96.8%/97.9% - same instructions, but the float registers are numbered differently
// (target: sum in f3, 0.5f in f2, 10.0 in f1).
unsigned int zVarEntryCB_SndFXVol()
{
    double d = (gSnd.fxvol.left + gSnd.fxvol.right) * 0.5f * 10.0;
    d = 0.5 + d;
    return (unsigned int)d;
}

unsigned int zVarEntryCB_PCSpecialBubbleGum()
{
    return globals.player.Inv_Specials & 1;
}

unsigned int zVarEntryCB_PCSpecialSoapBar()
{
    return (globals.player.Inv_Specials >> 1) & 1;
}

unsigned int zVarEntryCB_PCSpecialStickyBoots()
{
    return (globals.player.Inv_Specials >> 2) & 1;
}

unsigned int zVarEntryCB_PCSpecialPlungers()
{
    return (globals.player.Inv_Specials >> 3) & 1;
}

unsigned int zVarEntryCB_PCSpecialSlippers()
{
    return (globals.player.Inv_Specials >> 4) & 1;
}

unsigned int zVarEntryCB_PCSpecialLampShade()
{
    return (globals.player.Inv_Specials >> 5) & 1;
}

unsigned int zVarEntryCB_PCSpecialBlackKnight()
{
    return (globals.player.Inv_Specials >> 6) & 1;
}

unsigned int zVarEntryCB_PCSpecialFlowerPot()
{
    return (globals.player.Inv_Specials >> 7) & 1;
}

unsigned int zVarEntryCB_PCSpecialDivingHelmet()
{
    return (globals.player.Inv_Specials >> 8) & 1;
}

unsigned int zVarEntryCB_PCSpecialSpring()
{
    return (globals.player.Inv_Specials >> 9) & 1;
}

unsigned int zVarEntryCB_PCSpecialLightningBolt()
{
    return (globals.player.Inv_Specials >> 10) & 1;
}

unsigned int zVarEntryCB_PCSpecialFootballHelmet()
{
    return (globals.player.Inv_Specials >> 11) & 1;
}

unsigned int zVarEntryCB_PCSpecialUmbrella()
{
    return (globals.player.Inv_Specials >> 12) & 1;
}

unsigned int zVarEntryCB_PCSpecialShovel()
{
    return (globals.player.Inv_Specials >> 13) & 1;
}

unsigned int zVarEntryCB_PCSpecialLightningBoltStun()
{
    return (globals.player.Inv_Specials >> 14) & 1;
}

unsigned int zVarEntryCB_MCAvailable()
{
    if (xSGGameIsEmpty(NULL, 0) == 1)
    {
        if (xSGGameIsEmpty(NULL, 1) == 1)
        {
            return 0;
        }
        if (xSGGameIsEmpty(NULL, 1) == 0)
        {
            return 2;
        }
    }
    else if (xSGGameIsEmpty(NULL, 0) == 0)
    {
        if (xSGGameIsEmpty(NULL, 1) == 1)
        {
            return 1;
        }
        if (xSGGameIsEmpty(NULL, 1) == 0)
        {
            return 3;
        }
    }
    return 0;
}

unsigned int zVarEntryCB_VibrationOn()
{
    return globals.option_vibration & 1;
}

void zVarGiveMap()
{
    hasMap = 1;
}

void zVarUnlockR001()
{
    R001Unlocked = 1;
}

void zVarUnlockS001()
{
    S001Unlocked = 1;
}

void zVarUnlockE001()
{
    E001Unlocked = 1;
}

void zVarUnlockF001()
{
    F001Unlocked = 1;
}

void zVarSetArea(unsigned int sceneID)
{
    selSceneID = sceneID;
}

void zVarSetSnackGateValue(unsigned int amount)
{
    sSnackGateValue = amount;
}

void zVarDefeatMasterMind()
{
    MMDefeated = 1;
    endgame = 1;
}

char* zVMCB_PlayerScoobySnackCount_Int(char* buffer)
{
    return buffer + sprintf(buffer, "%d", globals.player.Inv_ScoobySnacks);
}

char* zVMCB_PlayerChancesCount(char* buffer)
{
    return buffer + sprintf(buffer, "%d", globals.player.Inv_Chances);
}

// TODO: 96.8%/97.9% - same instructions, but the float registers are numbered differently
// (target: sum in f3, 0.5f in f2, 10.0 in f1).
char* zVMCB_SoundMusicVolume(char* buffer)
{
    double d = (gSnd.streamvol.left + gSnd.streamvol.right) * 0.5f * 10.0;
    d = 0.5 + d;
    return buffer + sprintf(buffer, "%d", (unsigned int)d);
}

// TODO: 96.8%/97.9% - same instructions, but the float registers are numbered differently
// (target: sum in f3, 0.5f in f2, 10.0 in f1).
char* zVMCB_SoundFXVolume(char* buffer)
{
    double d = (gSnd.fxvol.left + gSnd.fxvol.right) * 0.5f * 10.0;
    d = 0.5 + d;
    return buffer + sprintf(buffer, "%d", (unsigned int)d);
}

char* zVMCB_Scene(char* buffer)
{
    return buffer + sprintf(buffer, "%s", xUtil_idtag2string(globals.sceneCur->sceneID, 0));
}

char* zVMCB_GameSlot0(char* buffer)
{
    zVarGameSlotInfo(0);
    return buffer + sprintf(buffer, "%s", txt);
}

char* zVMCB_GameSlot1(char* buffer)
{
    zVarGameSlotInfo(1);
    return buffer + sprintf(buffer, "%s", txt);
}

char* zVMCB_GameSlot2(char* buffer)
{
    zVarGameSlotInfo(2);
    return buffer + sprintf(buffer, "%s", txt);
}

char* zVMCB_GameSlot3(char* buffer)
{
    zVarGameSlotInfo(3);
    return buffer + sprintf(buffer, "%s", txt);
}

char* zVMCB_GameSlot4(char* buffer)
{
    zVarGameSlotInfo(4);
    return buffer + sprintf(buffer, "%s", txt);
}

char* zVMCB_GameSlot5(char* buffer)
{
    zVarGameSlotInfo(5);
    return buffer + sprintf(buffer, "%s", txt);
}

char* zVMCB_GameSlot6(char* buffer)
{
    zVarGameSlotInfo(6);
    return buffer + sprintf(buffer, "%s", txt);
}

char* zVMCB_GameSlot7(char* buffer)
{
    zVarGameSlotInfo(7);
    return buffer + sprintf(buffer, "%s", txt);
}

char* zVMCB_MCSelectedGame(char* buffer)
{
    return buffer + sprintf(buffer, "%d", zSaveLoad_getgame() + 1);
}

char* zVMCB_MCSelectedCard(char* buffer)
{
    signed int len = 0;
    switch (zSaveLoad_getcard())
    {
    case 0:
        len = sprintf(buffer, "%s", "A");
        break;
    case 1:
        len = sprintf(buffer, "%s", "B");
        break;
    }
    return buffer + len;
}

char* zVMCB_CurrentPosition(char* buffer)
{
    _xVec3* pos = (_xVec3*)&globals.player.ent.frame->mat.pos;
    return buffer + sprintf(buffer, "%d,%d,%d", (signed int)(pos->x + 0.5f), (signed int)(pos->y + 0.5f),
                            (signed int)(pos->z + 0.5f));
}

char* zVMCB_SnackRoomStats(char* buffer)
{
    return buffer;
}

char* zVMCB_TokenRoomStats(char* buffer)
{
    return buffer;
}

char* zVMCB_SnackAreaStats(char* buffer)
{
    return buffer;
}

char* zVMCB_TokenAreaStats(char* buffer)
{
    return buffer;
}

char* zVMCB_SnackWorldStats(char* buffer)
{
    return buffer;
}

char* zVMCB_TokenWorldStats(char* buffer)
{
    return buffer;
}

char* zVMCB_CurrentArea(char* buffer)
{
    return buffer + sprintf(buffer, "%s", zSceneGetAreaname(globals.sceneCur->sceneID));
}

char* zVMCB_AreaStatsSnacks(char* buffer)
{
    if (selSceneID == 'H003')
    {
        return buffer;
    }
    return buffer + sprintf(buffer, "%d/%d", zSceneGetAreaSnacksCollected(selSceneID), zSceneGetAreaTotalSnacks(selSceneID));
}

char* zVMCB_AreaStatsTokens(char* buffer)
{
    if (selSceneID == 'H003')
    {
        return buffer;
    }
    return buffer + sprintf(buffer, "%d/%d", zSceneGetAreaTokensCollected(selSceneID), zSceneGetAreaTotalTokens(selSceneID));
}

char* zVMCB_SelectedArea(char* buffer)
{
    return buffer + sprintf(buffer, "%s", zSceneGetAreaname(selSceneID));
}

char* zVMCB_SnackGateValue(char* buffer)
{
    return buffer + sprintf(buffer, "%d", sSnackGateValue);
}

char* zVMCB_CurrentDate(char* buffer)
{
    return buffer + iGetCurrFormattedDate(buffer);
}

char* zVMCB_CurrentTime(char* buffer)
{
    return buffer + iGetCurrFormattedTime(buffer);
}

char* zVMCB_NoCardMessage(char* buffer)
{
    return buffer + fn_800C9710(buffer);
}

char* zVMCB_SpaceAvailable(char* buffer)
{
    return buffer + sprintf(buffer, " ");
}

char* zVMCB_SpaceNeeded(char* buffer)
{
    return buffer + fn_800C9710(buffer);
}

char* zVMCB_FormatPromptMessage(char* buffer)
{
    return buffer + fn_800C9710(buffer);
}

char* zVMCB_MCActionMessage(char* buffer)
{
    return buffer + fn_800C9710(buffer);
}

char* zVMCB_MCActionFailMessage(char* buffer)
{
    return buffer + fn_800C9710(buffer);
}

char* zVMCB_CancelButtonLabel(char* buffer)
{
    return buffer + fn_800C96E8(buffer);
}

char* zVMCB_ActionButtonLabel(char* buffer)
{
    return buffer + fn_800C96C0(buffer);
}

char* zVMCB_AltButtonLabel(char* buffer)
{
    return buffer + fn_800C9698(buffer);
}

unsigned int zVarEntryCB_CurrentSceneLetter()
{
    char name[16];
    sprintf(name, "%s", xUtil_idtag2string(globals.sceneCur->sceneID, 0));
    return zVarToUpper(name[0]) - '@';
}

unsigned int zVarEntryCB_IsSneaking()
{
    unsigned char sneaking = 0;
    if (globals.player.Sneak && (zVarPlayerAnimFlags() & 0x1E) == 2)
    {
        sneaking = 1;
    }
    return sneaking;
}

unsigned int zVarEntryCB_MapItem()
{
    return hasMap;
}

unsigned int zVarEntryCB_UnlockedR001()
{
    return R001Unlocked;
}

unsigned int zVarEntryCB_UnlockedS001()
{
    return S001Unlocked;
}

unsigned int zVarEntryCB_UnlockedE001()
{
    return E001Unlocked;
}

unsigned int zVarEntryCB_UnlockedF001()
{
    return F001Unlocked;
}

unsigned int finishedGame()
{
    return endgame;
}

void zVarUnlockEndGame()
{
    endgame = 1;
}

void zVarUnlockAllMovies()
{
    allmovies = 1;
}

unsigned int zVarEntryCB_PuzzlePieceF()
{
    return globals.levelStats[0].clues >= globals.levelStats[0].clues_tot;
}

unsigned int zVarEntryCB_PuzzlePieceL()
{
    return globals.levelStats[1].clues >= globals.levelStats[1].clues_tot;
}

unsigned int zVarEntryCB_PuzzlePieceW()
{
    return globals.levelStats[2].clues >= globals.levelStats[2].clues_tot;
}

unsigned int zVarEntryCB_PuzzlePieceE()
{
    return globals.levelStats[3].clues >= globals.levelStats[3].clues_tot;
}

unsigned int zVarEntryCB_PuzzlePieceC()
{
    return globals.levelStats[4].clues >= globals.levelStats[4].clues_tot;
}

unsigned int zVarEntryCB_PuzzlePieceG()
{
    return globals.levelStats[5].clues >= globals.levelStats[5].clues_tot;
}

unsigned int zVarEntryCB_PuzzlePieceP()
{
    return globals.levelStats[6].clues >= globals.levelStats[6].clues_tot;
}

unsigned int zVarEntryCB_PuzzlePieceB()
{
    return globals.levelStats[7].clues >= globals.levelStats[7].clues_tot;
}

unsigned int zVarEntryCB_PuzzlePieceS()
{
    return globals.levelStats[8].clues >= globals.levelStats[8].clues_tot;
}

unsigned int zVarEntryCB_PuzzlePieceI()
{
    return globals.levelStats[9].clues >= globals.levelStats[9].clues_tot;
}

unsigned int zVarEntryCB_PuzzlePieceR()
{
    return globals.levelStats[10].clues >= globals.levelStats[10].clues_tot;
}

unsigned int zVarEntryCB_PuzzlePieceO()
{
    return globals.levelStats[11].clues >= globals.levelStats[11].clues_tot;
}

unsigned int zVarEntryCB_PuzzlePieceH()
{
    return globals.levelStats[12].clues >= globals.levelStats[12].clues_tot;
}

unsigned int zVarEntryCB_CurrentRoom()
{
    char name[16];
    sprintf(name, "%s", xUtil_idtag2string(globals.sceneCur->sceneID, 0));
    signed int room = (name[2] - '0') * 10 + (name[3] - '0');
    return (zVarToUpper(name[0]) - '@') * 100 + room;
}

unsigned int zVarEntryCB_eVarMToken1()
{
    return globals.player.Inv_Mtokens[0] & 1;
}

unsigned int zVarEntryCB_eVarMToken2()
{
    return (globals.player.Inv_Mtokens[0] >> 1) & 1;
}

unsigned int zVarEntryCB_eVarMToken3()
{
    return (globals.player.Inv_Mtokens[0] >> 2) & 1;
}

unsigned int zVarEntryCB_eVarMToken4()
{
    return (globals.player.Inv_Mtokens[0] >> 3) & 1;
}

unsigned int zVarEntryCB_eVarMToken5()
{
    return (globals.player.Inv_Mtokens[0] >> 4) & 1;
}

unsigned int zVarEntryCB_eVarMToken6()
{
    return (globals.player.Inv_Mtokens[0] >> 5) & 1;
}

unsigned int zVarEntryCB_eVarMToken7()
{
    return (globals.player.Inv_Mtokens[0] >> 6) & 1;
}

unsigned int zVarEntryCB_eVarMToken8()
{
    return (globals.player.Inv_Mtokens[0] >> 7) & 1;
}

unsigned int zVarEntryCB_eVarMToken9()
{
    return (globals.player.Inv_Mtokens[0] >> 8) & 1;
}

unsigned int zVarEntryCB_eVarMToken10()
{
    return (globals.player.Inv_Mtokens[0] >> 9) & 1;
}

unsigned int zVarEntryCB_eVarMToken11()
{
    return (globals.player.Inv_Mtokens[0] >> 10) & 1;
}

unsigned int zVarEntryCB_eVarMToken12()
{
    return (globals.player.Inv_Mtokens[0] >> 11) & 1;
}

unsigned int zVarEntryCB_eVarMToken13()
{
    return (globals.player.Inv_Mtokens[0] >> 12) & 1;
}

unsigned int zVarEntryCB_eVarMToken14()
{
    return (globals.player.Inv_Mtokens[0] >> 13) & 1;
}

unsigned int zVarEntryCB_eVarMToken15()
{
    return (globals.player.Inv_Mtokens[0] >> 14) & 1;
}

unsigned int zVarEntryCB_eVarMToken16()
{
    return (globals.player.Inv_Mtokens[0] >> 15) & 1;
}

unsigned int zVarEntryCB_eVarMToken17()
{
    return (globals.player.Inv_Mtokens[0] >> 16) & 1;
}

unsigned int zVarEntryCB_eVarMToken18()
{
    return (globals.player.Inv_Mtokens[0] >> 17) & 1;
}

unsigned int zVarEntryCB_eVarMToken19()
{
    return (globals.player.Inv_Mtokens[0] >> 18) & 1;
}

unsigned int zVarEntryCB_eVarMToken20()
{
    return (globals.player.Inv_Mtokens[0] >> 19) & 1;
}

unsigned int zVarEntryCB_eVarMToken21()
{
    return (globals.player.Inv_Mtokens[0] >> 20) & 1;
}

unsigned int zVarEntryCB_eVarWarpB004()
{
    return zEntPickup_WarpIsOpen(0);
}

unsigned int zVarEntryCB_eVarWarpC004()
{
    return zEntPickup_WarpIsOpen(1);
}

unsigned int zVarEntryCB_eVarWarpE004()
{
    return zEntPickup_WarpIsOpen(2);
}

unsigned int zVarEntryCB_eVarWarpE006()
{
    return zEntPickup_WarpIsOpen(3);
}

unsigned int zVarEntryCB_eVarWarpE009()
{
    return zEntPickup_WarpIsOpen(4);
}

unsigned int zVarEntryCB_eVarWarpF003()
{
    return zEntPickup_WarpIsOpen(5);
}

unsigned int zVarEntryCB_eVarWarpF007()
{
    return zEntPickup_WarpIsOpen(6);
}

unsigned int zVarEntryCB_eVarWarpF010()
{
    return zEntPickup_WarpIsOpen(7);
}

unsigned int zVarEntryCB_eVarWarpG001()
{
    return zEntPickup_WarpIsOpen(24);
}

unsigned int zVarEntryCB_eVarWarpG005()
{
    return zEntPickup_WarpIsOpen(8);
}

unsigned int zVarEntryCB_eVarWarpG008()
{
    return zEntPickup_WarpIsOpen(9);
}

unsigned int zVarEntryCB_eVarWarpH001()
{
    return zEntPickup_WarpIsOpen(10);
}

unsigned int zVarEntryCB_eVarWarpI003()
{
    return zEntPickup_WarpIsOpen(11);
}

unsigned int zVarEntryCB_eVarWarpI006()
{
    return zEntPickup_WarpIsOpen(12);
}

unsigned int zVarEntryCB_eVarWarpL014()
{
    return zEntPickup_WarpIsOpen(13);
}

unsigned int zVarEntryCB_eVarWarpL015()
{
    return zEntPickup_WarpIsOpen(23);
}

unsigned int zVarEntryCB_eVarWarpL018()
{
    return zEntPickup_WarpIsOpen(14);
}

unsigned int zVarEntryCB_eVarWarpO004()
{
    return zEntPickup_WarpIsOpen(15);
}

unsigned int zVarEntryCB_eVarWarpO006()
{
    return zEntPickup_WarpIsOpen(16);
}

unsigned int zVarEntryCB_eVarWarpP003()
{
    return zEntPickup_WarpIsOpen(17);
}

unsigned int zVarEntryCB_eVarWarpP005()
{
    return zEntPickup_WarpIsOpen(18);
}

unsigned int zVarEntryCB_eVarWarpR003()
{
    return zEntPickup_WarpIsOpen(19);
}

unsigned int zVarEntryCB_eVarWarpS003()
{
    return zEntPickup_WarpIsOpen(20);
}

unsigned int zVarEntryCB_eVarWarpW022()
{
    return zEntPickup_WarpIsOpen(21);
}

unsigned int zVarEntryCB_eVarWarpW026()
{
    return zEntPickup_WarpIsOpen(22);
}

unsigned int zVarEntryCB_eVarWarpH003()
{
    return zEntPickup_WarpIsOpen(25);
}

unsigned int zVarEntryCB_eVareVarDefeatedMM()
{
    return MMDefeated;
}

unsigned int zVarEntryCB_eVarAllSnacksFound()
{
    unsigned int count = 0;
    for (signed int i = 0; i < 30; i++)
    {
        if (globals.areaStats[i].snacks_tot >= zSceneGetAreaTotalSnacks((_tagSceneAreas)i))
        {
            count++;
        }
    }
    return count;
}

unsigned int zVarEntryCB_eVarCin1()
{
    if (allmovies)
    {
        return 1;
    }
    return zCutSceneNamesTable_getStatus(1);
}

unsigned int zVarEntryCB_eVarCin2()
{
    if (allmovies)
    {
        return 1;
    }
    return zCutSceneNamesTable_getStatus(2);
}

unsigned int zVarEntryCB_eVarCin3()
{
    if (allmovies)
    {
        return 1;
    }
    return zCutSceneNamesTable_getStatus(3);
}

unsigned int zVarEntryCB_eVarCin4()
{
    if (allmovies)
    {
        return 1;
    }
    return zCutSceneNamesTable_getStatus(4);
}

unsigned int zVarEntryCB_eVarCin5()
{
    if (allmovies)
    {
        return 1;
    }
    return zCutSceneNamesTable_getStatus(5);
}

unsigned int zVarEntryCB_eVarCin6()
{
    if (allmovies)
    {
        return 1;
    }
    return zCutSceneNamesTable_getStatus(6);
}

unsigned int zVarEntryCB_eVarCin7()
{
    if (allmovies)
    {
        return 1;
    }
    return zCutSceneNamesTable_getStatus(7);
}

unsigned int zVarEntryCB_eVarCin8()
{
    if (allmovies)
    {
        return 1;
    }
    return zCutSceneNamesTable_getStatus(8);
}

unsigned int zVarEntryCB_eVarCin9()
{
    if (allmovies)
    {
        return 1;
    }
    return zCutSceneNamesTable_getStatus(9);
}

unsigned int zVarEntryCB_eVarCin10()
{
    if (allmovies)
    {
        return 1;
    }
    return zCutSceneNamesTable_getStatus(10);
}

unsigned int zVarEntryCB_eVarCin11()
{
    return MMDefeated;
}

unsigned int zVarEntryCB_eVarCin12()
{
    if (allmovies)
    {
        return 1;
    }
    return zCutSceneNamesTable_getStatus(12);
}

unsigned int zVarEntryCB_eVarCin13()
{
    if (allmovies)
    {
        return 1;
    }
    return zCutSceneNamesTable_getStatus(13);
}

unsigned int zVarEntryCB_eVarCin14()
{
    if (allmovies)
    {
        return 1;
    }
    return zCutSceneNamesTable_getStatus(14);
}

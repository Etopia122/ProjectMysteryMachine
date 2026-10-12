#include "zEnt.h"

// zGameStateSwitch and the table search are inlined into zGameStateSwitchEvent
// but zGameModeSwitch is not: auto-inlining is enabled only around
// zGameStateSwitch's definition (the table search is declared inline).

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

void xSndPauseAll(unsigned int pause_effects, unsigned int pause_streams);
void zEntEventAllOfType(unsigned int toEvent, unsigned int type);
extern signed int startPressed;

signed int gGameState = 6;
eGameMode gGameMode;
_GameOstrich gGameOstrich;

static unsigned int sGameMode_DispatchTable[12] = { 0 };
static unsigned int sGameState_DispatchTable[8] = { 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0, 0 };
static unsigned int sPauseState_DispatchTable[2] = { 0x80, 0x81 };
static unsigned int sSaveState_DispatchTable[3] = { 0x7C, 0x7D, 0x7E };
static unsigned int sOptionsState_DispatchTable[1] = { 0x7A };
static unsigned int sLoadState_DispatchTable[3] = { 0x76, 0x77, 0x78 };
static unsigned int sTitleState_DispatchTable[2] = { 0x73, 0x74 };
static unsigned int sIntroState_DispatchTable[4] = { 0x6E, 0x6F, 0x70, 0x71 };
static unsigned int sGameState_DoDispatchTable[8] = { 0x9E, 0x9F, 0xA0, 0xA1, 0x128, 0xA2, 0xA3, 0xA6 };
static unsigned int sPauseState_DoDispatchTable[2] = { 0x9B, 0x9C };
static unsigned int sSaveState_DoDispatchTable[3] = { 0x97, 0x98, 0x99 };
static unsigned int sOptionsState_DoDispatchTable[1] = { 0x95 };
static unsigned int sLoadState_DoDispatchTable[3] = { 0x91, 0x92, 0x93 };
static unsigned int sTitleState_DoDispatchTable[2] = { 0x8E, 0x8F };
static unsigned int sIntroState_DoDispatchTable[4] = { 0x89, 0x8A, 0x8B, 0x8C };

void zGameModeSwitch(eGameMode modeNew);
void zGameStateSwitch(signed int theNewState);

// Functions are in reverse DOL order (-inline deferred emits last to first).

signed int zGameStateGet()
{
    return gGameState;
}

signed int zGameModeGet()
{
    return gGameMode;
}

#ifdef PC_PORT
// For the port's renderer: in a level (playing, paused, saving, the warp and
// gallery screens), where the view may be wider than 4:3; the intro, title
// and front-end menus are made for 4:3.
extern "C" int pc_game_wide_view()
{
    return gGameMode >= eGameMode_Save && gGameMode != eGameMode_Options && gGameMode < eGameMode_Count;
}
#endif

_GameOstrich zGameGetOstrich()
{
    return gGameOstrich;
}

void zGameSetOstrich(_GameOstrich o)
{
    gGameOstrich = o;
}

static inline signed int zGameStateFindEvent(unsigned int* eventList, signed int eventCount,
                                      signed int targetMode, signed int targetEvent,
                                      signed int* new_mode, signed int* new_state)
{
    signed int i;
    for (i = 0; i < eventCount; i++)
    {
        if (targetEvent == eventList[i])
        {
            *new_mode = targetMode;
            *new_state = i;
            return 1;
        }
    }
    return 0;
}

void zGameStateSwitchEvent(signed int event)
{
    signed int old_mode = gGameMode;
    signed int old_state = gGameState;
    signed int new_mode = -1;
    signed int new_state = -1;

    if (zGameStateFindEvent(sGameState_DoDispatchTable, 8, eGameMode_Game, event, &new_mode,
                            &new_state) ||
        zGameStateFindEvent(sPauseState_DoDispatchTable, 2, eGameMode_Pause, event, &new_mode,
                            &new_state) ||
        zGameStateFindEvent(sSaveState_DoDispatchTable, 3, eGameMode_Save, event, &new_mode,
                            &new_state) ||
        zGameStateFindEvent(sOptionsState_DoDispatchTable, 1, eGameMode_Options, event,
                            &new_mode, &new_state) ||
        zGameStateFindEvent(sLoadState_DoDispatchTable, 3, eGameMode_Load, event, &new_mode,
                            &new_state) ||
        zGameStateFindEvent(sTitleState_DoDispatchTable, 2, eGameMode_Title, event, &new_mode,
                            &new_state) ||
        zGameStateFindEvent(sIntroState_DoDispatchTable, 4, eGameMode_Intro, event, &new_mode,
                            &new_state))
    {
    }

    if (new_mode != old_mode)
    {
        zGameModeSwitch((eGameMode)new_mode);
    }

    if (new_mode != old_mode || new_state != old_state)
    {
        zGameStateSwitch(new_state);
    }
}

#pragma auto_inline on
void zGameStateSwitch(signed int theNewState)
{
    signed int stateOld = gGameState;
    unsigned int theEvent = 0;

    gGameState = theNewState;
    if (theNewState == 1 && stateOld == 0)
    {
        startPressed = 1;
    }

    switch (gGameMode)
    {
    case eGameMode_Intro:
        theEvent = sIntroState_DispatchTable[theNewState];
        break;
    case eGameMode_Title:
        theEvent = sTitleState_DispatchTable[theNewState];
        break;
    case eGameMode_Load:
        theEvent = sLoadState_DispatchTable[theNewState];
        break;
    case eGameMode_Options:
        theEvent = sOptionsState_DispatchTable[theNewState];
        break;
    case eGameMode_Save:
        theEvent = sSaveState_DispatchTable[theNewState];
        break;
    case eGameMode_Pause:
        theEvent = sPauseState_DispatchTable[theNewState];
        break;
    case eGameMode_Game:
        theEvent = sGameState_DispatchTable[theNewState];
        break;
    }

    if (theEvent)
    {
        zEntEventAllOfType(theEvent, 0x1E);
    }
}

#pragma auto_inline off
void zGameModeSwitch(eGameMode modeNew)
{
    eGameMode modeOld = gGameMode;

    if ((modeOld == eGameMode_Game && modeNew == eGameMode_Pause) ||
        (modeOld == eGameMode_Game && modeNew == eGameMode_Save))
    {
        xSndPauseAll(1, 1);
    }
    else if ((modeOld == eGameMode_Save || modeOld == eGameMode_Pause ||
              modeOld == eGameMode_Load) &&
             modeNew == eGameMode_Game)
    {
        xSndPauseAll(0, 0);
    }

    gGameMode = modeNew;
    zEntEventAllOfType(sGameMode_DispatchTable[modeNew], 0x1E);
}

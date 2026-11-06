#include "xString.h"
#include "xSnd.h"
#include "iMath.h"
#include "zGlobals.h"
#include "zScene.h"

extern "C" void fn_80015EAC(const char* fmt, ...);
void xprintf(char* fmt, ...);

class zMusicSituation
{
public:
    signed int track;      // 0x0
    unsigned int priority; // 0x4
    unsigned int countMax; // 0x8
    float delay;           // 0xC
    float punchDelay;      // 0x10
    float elapsedTime;     // 0x14
    unsigned int count;    // 0x18
    unsigned int sndid;    // 0x1C
    signed int situation;  // 0x20
    signed int music_enum; // 0x24
    signed int game_state; // 0x28
};

class zMusicTrackInfo
{
public:
    unsigned int snd_id;        // 0x0
    unsigned int loop;          // 0x4
    zMusicSituation* situation; // 0x8
    unsigned int assetID;       // 0xC
    float lastVol;              // 0x10
};

extern signed int gGameMode;

#define SCENE_ID(s) ((((s)[0] & ~0x20) << 24) | ((s)[1] << 16) | ((s)[2] << 8) | (s)[3])

static unsigned int sMusicPaused;
static unsigned int sMusicSoundID[45][2];
static signed int sMusicLastEnum[2] = { 0 };
static float sMusicTimer[2] = { 0 };
static zMusicSituation* sMusicQueueData[2];
static zMusicSituation sMusicInfo[7] = {
    { 0, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 0.0f, 1.0f, 0.0f, 0, 0, 1, 0, 0 },
    { 0, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0, 2, 0, 0 },
    { 0, 0, 0, 0.0f, 2.0f, 0.0f, 0, 0, 3, 0, 0 },
    { 0, 0, 0, 0.0f, 1.0f, 0.0f, 0, 0, 4, 0, 0 },
    { 0, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0, 5, 0, 0 },
    { 1, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0, 6, 0, 0 },
};
static zMusicTrackInfo sMusicTrack[2];

static signed int zMusicDo(signed int track);

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zMusicInit()
{
    sMusicPaused = 0;
    for (signed int i = 0; i < 2; i++)
    {
        sMusicTrack[i].snd_id = 0;
        sMusicTrack[i].loop = 0;
        sMusicTrack[i].situation = 0;
    }

    sMusicSoundID[0][0] = xStrHash("BLACK KNIGHT-L");
    sMusicSoundID[0][1] = 1;
    sMusicSoundID[1][0] = xStrHash("REDBEARD-L");
    sMusicSoundID[1][1] = 1;
    sMusicSoundID[2][0] = xStrHash("GREEN GHOST-L");
    sMusicSoundID[2][1] = 1;
    sMusicSoundID[3][0] = xStrHash("MASTERMIND TEMP-L");
    sMusicSoundID[3][1] = 1;
    sMusicSoundID[4][0] = xStrHash("Scooby 03 loop-L");
    sMusicSoundID[4][1] = 1;
    sMusicSoundID[5][0] = xStrHash("Scooby 04 loop-L");
    sMusicSoundID[5][1] = 1;
    sMusicSoundID[6][0] = xStrHash("Scooby 05 loop-L");
    sMusicSoundID[6][1] = 1;
    sMusicSoundID[7][0] = xStrHash("Scooby 06 loop-L");
    sMusicSoundID[7][1] = 1;
    sMusicSoundID[8][0] = xStrHash("Scooby 07 loop-L");
    sMusicSoundID[8][1] = 1;
    sMusicSoundID[9][0] = xStrHash("Scooby 07 loop-L");
    sMusicSoundID[9][1] = 1;
    sMusicSoundID[10][0] = xStrHash("Scooby 09 loop-L");
    sMusicSoundID[10][1] = 1;
    sMusicSoundID[11][0] = xStrHash("Scooby 10 loop-L");
    sMusicSoundID[11][1] = 1;
    sMusicSoundID[12][0] = xStrHash("Scooby 11 loop-L");
    sMusicSoundID[12][1] = 1;
    sMusicSoundID[13][0] = xStrHash("Scooby 12 loop-L");
    sMusicSoundID[13][1] = 1;
    sMusicSoundID[14][0] = xStrHash("ACTION LOOP 01-L");
    sMusicSoundID[14][1] = 1;
    sMusicSoundID[15][0] = xStrHash("ACTION LOOP 02-L");
    sMusicSoundID[15][1] = 1;
    sMusicSoundID[16][0] = xStrHash("ACTION LOOP 03-L");
    sMusicSoundID[16][1] = 1;
    sMusicSoundID[17][0] = xStrHash("ACTION LOOP 04-L");
    sMusicSoundID[17][1] = 1;
    sMusicSoundID[18][0] = xStrHash("ACTION LOOP 05-L");
    sMusicSoundID[18][1] = 1;
    sMusicSoundID[19][0] = xStrHash("ACTION LOOP 06-L");
    sMusicSoundID[19][1] = 1;
    sMusicSoundID[20][0] = xStrHash("ACTION LOOP 07-L");
    sMusicSoundID[20][1] = 1;
    sMusicSoundID[21][0] = xStrHash("CINEMATIC NEMESIS 1-L");
    sMusicSoundID[21][0] = 0;
    sMusicSoundID[22][0] = xStrHash("CINEMATIC NEMESIS 2-L");
    sMusicSoundID[22][0] = 0;
    sMusicSoundID[23][0] = xStrHash("CINEMATIC NEMESIS 3-L");
    sMusicSoundID[23][0] = 0;
    sMusicSoundID[24][0] = xStrHash("Cinematic Mood 1-L");
    sMusicSoundID[24][1] = 1;
    sMusicSoundID[25][0] = xStrHash("Cinematic Mood 2-L");
    sMusicSoundID[25][1] = 1;
    sMusicSoundID[26][0] = xStrHash("Cinematic Mood 3-L");
    sMusicSoundID[26][1] = 1;
    sMusicSoundID[27][0] = xStrHash("Cinematic Generic 1-L");
    sMusicSoundID[27][1] = 1;
    sMusicSoundID[28][0] = xStrHash("Cinematic Silly-L");
    sMusicSoundID[28][1] = 1;
    sMusicSoundID[29][0] = xStrHash("Cinematic Van-L");
    sMusicSoundID[29][1] = 1;
    sMusicSoundID[30][0] = xStrHash("Comedic Duo loop-L");
    sMusicSoundID[30][1] = 1;
    sMusicSoundID[31][0] = xStrHash("Mystery loop-L");
    sMusicSoundID[31][1] = 1;
    sMusicSoundID[32][0] = xStrHash("Jeopardy loop-L");
    sMusicSoundID[32][1] = 1;
    sMusicSoundID[21][0] = xStrHash("Cinematic Nemesis 1-L");
    sMusicSoundID[21][1] = 0;
    sMusicSoundID[22][0] = xStrHash("Cinematic Nemesis 2-L");
    sMusicSoundID[22][1] = 0;
    sMusicSoundID[23][0] = xStrHash("Cinematic Nemesis 3-L");
    sMusicSoundID[23][1] = 0;
    sMusicSoundID[33][0] = xStrHash("theramin 01");
    sMusicSoundID[33][1] = 0;
    sMusicSoundID[34][0] = xStrHash("theramin 02");
    sMusicSoundID[34][1] = 0;
    sMusicSoundID[35][0] = xStrHash("theramin 03");
    sMusicSoundID[35][1] = 0;
    sMusicSoundID[36][0] = xStrHash("theramin 04");
    sMusicSoundID[36][1] = 0;
    sMusicSoundID[37][0] = xStrHash("theramin 05");
    sMusicSoundID[37][1] = 0;
    sMusicSoundID[38][0] = xStrHash("theramin 06");
    sMusicSoundID[38][1] = 0;
    sMusicSoundID[39][0] = xStrHash("theramin 07");
    sMusicSoundID[39][1] = 0;
    sMusicSoundID[40][0] = xStrHash("theramin 08");
    sMusicSoundID[40][1] = 0;
    sMusicSoundID[41][0] = xStrHash("theramin 09");
    sMusicSoundID[41][1] = 0;
    sMusicSoundID[42][0] = xStrHash("theramin 10");
    sMusicSoundID[42][1] = 0;
    sMusicSoundID[43][0] = xStrHash("theramin 11");
    sMusicSoundID[43][1] = 0;
    sMusicSoundID[44][0] = xStrHash("theramin 12");
    sMusicSoundID[44][1] = 0;


    for (signed int i = 0; i < 7; i++)
    {
        sMusicInfo[i].elapsedTime = sMusicInfo[i].delay;
        sMusicInfo[i].count = 0;
    }
}

static signed int zMusicDo(signed int track)
{
    float vol = 0.33f;
    signed int snd_enum = irand() % 45;

    switch (sMusicQueueData[track]->situation)
    {
    case 3:
        vol = 1.0f;
        if (globals.sceneCur->sceneID == SCENE_ID("O008"))
        {
            snd_enum = 0;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("W028"))
        {
            snd_enum = 1;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("G009"))
        {
            snd_enum = 2;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("S005"))
        {
            snd_enum = 3;
        }
        break;
    case 5:
        snd_enum = sMusicQueueData[track]->music_enum;
        break;
    case 6:
        snd_enum = sMusicQueueData[track]->music_enum;
        break;
    case 0:
        if (globals.sceneCur->sceneID == SCENE_ID("H003"))
        {
            snd_enum = 31;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("G009"))
        {
            snd_enum = 11;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("O008"))
        {
            snd_enum = 11;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("W028"))
        {
            snd_enum = 11;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("S005"))
        {
            snd_enum = 11;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("MNU4") || globals.sceneCur->sceneID == SCENE_ID("MNU3"))
        {
            snd_enum = 13;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("H001"))
        {
            snd_enum = 11;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("H002"))
        {
            snd_enum = 19;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("I001"))
        {
            snd_enum = 31;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("I020"))
        {
            snd_enum = 6;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("I021"))
        {
            snd_enum = 5;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("I003"))
        {
            snd_enum = 12;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("I004"))
        {
            snd_enum = 31;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("I005"))
        {
            snd_enum = 12;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("I006"))
        {
            snd_enum = 10;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("R001"))
        {
            snd_enum = 11;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("R020"))
        {
            snd_enum = 7;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("R021"))
        {
            snd_enum = 30;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("R003"))
        {
            snd_enum = 12;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("R004"))
        {
            snd_enum = 20;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("R005"))
        {
            snd_enum = 20;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("R005"))
        {
            snd_enum = 4;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("o001"))
        {
            snd_enum = 6;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("o002"))
        {
            snd_enum = 5;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("o003"))
        {
            snd_enum = 32;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("o004"))
        {
            snd_enum = 17;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("o005"))
        {
            snd_enum = 11;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("o006"))
        {
            snd_enum = 12;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("o008"))
        {
            snd_enum = 24;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("f001"))
        {
            snd_enum = 10;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("f003"))
        {
            snd_enum = 30;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("f009"))
        {
            snd_enum = 4;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("f010"))
        {
            snd_enum = 8;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("e001"))
        {
            snd_enum = 31;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("e002"))
        {
            snd_enum = 5;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("e003"))
        {
            snd_enum = 11;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("e004"))
        {
            snd_enum = 28;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("e005"))
        {
            snd_enum = 32;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("e006"))
        {
            snd_enum = 10;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("e007"))
        {
            snd_enum = 30;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("e008"))
        {
            snd_enum = 6;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("e009"))
        {
            snd_enum = 8;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("c001"))
        {
            snd_enum = 7;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("c002"))
        {
            snd_enum = 12;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("c003"))
        {
            snd_enum = 19;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("c004"))
        {
            snd_enum = 32;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("c005"))
        {
            snd_enum = 31;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("c006"))
        {
            snd_enum = 5;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("c007"))
        {
            snd_enum = 6;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("g001"))
        {
            snd_enum = 11;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("g002"))
        {
            snd_enum = 18;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("g003"))
        {
            snd_enum = 32;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("g004"))
        {
            snd_enum = 30;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("g005"))
        {
            snd_enum = 10;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("g006"))
        {
            snd_enum = 19;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("g007"))
        {
            snd_enum = 7;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("g008"))
        {
            snd_enum = 31;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("f004"))
        {
            snd_enum = 12;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("f005"))
        {
            snd_enum = 7;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("f005"))
        {
            snd_enum = 7;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("f006"))
        {
            snd_enum = 13;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("f007"))
        {
            snd_enum = 32;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("f008"))
        {
            snd_enum = 31;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("l011"))
        {
            snd_enum = 5;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("l013"))
        {
            snd_enum = 8;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("l014"))
        {
            snd_enum = 4;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("l015"))
        {
            snd_enum = 10;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("l015"))
        {
            snd_enum = 10;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("l017"))
        {
            snd_enum = 20;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("l018"))
        {
            snd_enum = 30;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("l019"))
        {
            snd_enum = 6;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("w020"))
        {
            snd_enum = 4;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("w021"))
        {
            snd_enum = 8;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("w022"))
        {
            snd_enum = 7;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("w023"))
        {
            snd_enum = 32;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("w025"))
        {
            snd_enum = 5;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("w026"))
        {
            snd_enum = 19;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("w027"))
        {
            snd_enum = 12;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("p001"))
        {
            snd_enum = 12;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("p002"))
        {
            snd_enum = 6;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("p003"))
        {
            snd_enum = 11;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("p004"))
        {
            snd_enum = 30;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("p005"))
        {
            snd_enum = 14;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("b001"))
        {
            snd_enum = 16;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("b002"))
        {
            snd_enum = 32;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("b003"))
        {
            snd_enum = 4;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("b003"))
        {
            snd_enum = 10;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("b004"))
        {
            snd_enum = 12;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("s001"))
        {
            snd_enum = 15;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("s002"))
        {
            snd_enum = 17;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("s003"))
        {
            snd_enum = 19;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("s004"))
        {
            snd_enum = 20;
        }
        else if (globals.sceneCur->sceneID == SCENE_ID("s005"))
        {
            snd_enum = 31;
        }
        else
        {
            snd_enum = irand() % 9 + 4;
        }
        break;
    case 2:
        snd_enum = 14;
        break;
    case 4:
        break;
    case 1:
    default:
        snd_enum = irand() % 45;
        break;
    }

    if (sMusicTrack[track].snd_id != 0)
    {
        xSndStop(sMusicTrack[track].snd_id);
        sMusicTrack[track].snd_id = 0;
    }

    sMusicTrack[track].loop = sMusicSoundID[snd_enum][1];
    sMusicTrack[track].snd_id =
        xSndPlay(sMusicSoundID[snd_enum][0], vol, vol, 0.0f, 0xFFFF,
                 (sMusicTrack[track].loop ? 0x100 : 0) | 0x604, 0);
    xprintf("zMusicDo %d\n", sMusicQueueData[track]->sndid);

    if (sMusicTrack[track].snd_id != 0)
    {
        sMusicTrack[track].assetID = sMusicSoundID[snd_enum][0];
        sMusicTrack[track].lastVol = vol;
        if (sMusicQueueData[track] != 0)
        {
            sMusicQueueData[track]->sndid = sMusicTrack[track].snd_id;
            sMusicQueueData[track]->elapsedTime = 0.0f;
            sMusicQueueData[track]->count++;
            sMusicQueueData[track] = 0;
        }
        sMusicLastEnum[track] = snd_enum;
        return 1;
    }
    return 0;
}

void zMusicNotify(signed int situation)
{
    zMusicSituation* s;

    if (sMusicPaused)
    {
        return;
    }

    s = &sMusicInfo[situation];
    if ((s->countMax == 0 || s->count < s->countMax) && !(s->delay > s->elapsedTime))
    {
        sMusicTimer[s->track] = s->punchDelay;
        sMusicQueueData[s->track] = s;
        sMusicQueueData[s->track]->game_state = gGameMode == 11;
    }
}

void zMusicNotifyEvent(float* toParam, xBase*)
{
    zMusicSituation* s;

    if (sMusicPaused)
    {
        return;
    }

    s = &sMusicInfo[(0.0f == toParam[1]) ? 5 : 6];
    if (sMusicQueueData[s->track] == 0 && (s->countMax == 0 || s->count < s->countMax) &&
        !(s->delay > s->elapsedTime))
    {
        sMusicTimer[s->track] = s->punchDelay;
        sMusicQueueData[s->track] = s;
        sMusicQueueData[s->track]->game_state = gGameMode == 11;
        sMusicQueueData[s->track]->music_enum = toParam[0];
    }
}

void zMusicUpdate(float dt)
{
    signed int i;

    if (sMusicPaused)
    {
        return;
    }

    for (i = 0; i < 7; i++)
    {
        sMusicInfo[i].elapsedTime += dt;
    }

    for (i = 0; i < 2; i++)
    {
        if (sMusicTimer[i] || sMusicQueueData[i])
        {
            if ((gGameMode == 11) == sMusicQueueData[i]->game_state)
            {
                if (sMusicTimer[i] > 0.0f)
                {
                    sMusicTimer[i] -= dt;
                    if (sMusicTimer[i] < 0.0f)
                    {
                        sMusicTimer[i] = 0.0f;
                    }
                }
                if (0.0f == sMusicTimer[i] && sMusicQueueData[i] != 0)
                {
                    zMusicDo(i);
                }
            }
        }
    }
}

void zMusicKill()
{
    for (signed int i = 0; i < 2; i++)
    {
        if (sMusicTrack[i].snd_id != 0)
        {
            xSndStop(sMusicTrack[i].snd_id);
            sMusicTrack[i].snd_id = 0;
            sMusicTrack[i].loop = 0;
            sMusicTrack[i].situation = 0;
        }
    }
}

void zMusicPause()
{
    if (sMusicPaused)
    {
        return;
    }

    for (signed int i = 0; i < 2; i++)
    {
        if (sMusicTrack[i].snd_id != 0)
        {
            fn_80015EAC("pausing track %d (sndid= %d)\n", i, sMusicTrack[i].snd_id);
            xSndStop(sMusicTrack[i].snd_id);
        }
    }
    sMusicPaused = 1;
}

void zMusicUnpause(signed int kill)
{
    if (!sMusicPaused)
    {
        return;
    }

    for (signed int i = 0; i < 2; i++)
    {
        if (sMusicTrack[i].snd_id != 0)
        {
            if (kill)
            {
                sMusicTrack[i].snd_id = 0;
            }
            else
            {
                sMusicTrack[i].snd_id =
                    xSndPlay(sMusicTrack[i].assetID, sMusicTrack[i].lastVol,
                             sMusicTrack[i].lastVol, 0.0f, 0xFFFF,
                             (sMusicTrack[i].loop ? 0x100 : 0) | 0x604, 0);
                fn_80015EAC("unpausing track %d (sndid= %d)\n", i, sMusicTrack[i].snd_id);
            }
        }
    }
    sMusicPaused = 0;
}

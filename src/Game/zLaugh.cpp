#include "xSnd.h"
#include "xString.h"

extern "C" int sprintf(char* s, const char* format, ...);

class zLaughSituation
{
public:
    unsigned int countMax;
    float delay;
    float punchDelay;
    float elapsedTime;
    unsigned int count;
};

static signed int sLastLaughEnum;
static unsigned int sLaughSoundID[17];
static float sLaughTimer;
static zLaughSituation* sLaughQueueData;
static zLaughSituation sLaughInfo[15] = {
    { 0, 120.0f, 1.0f, 0.0f, 0 },          { 0, 120.0f, 1.0f, 0.0f, 0 },
    { 0, 120.0f, 1.0f, 0.0f, 0 },          { 0, 120.0f, 1.0f, 0.0f, 0 },
    { 0, 120.0f, 1.0f, 0.0f, 0 },          { 0, 120.0f, 1.0f, 0.0f, 0 },
    { 0xFFFFFFFF, 120.0f, 1.0f, 0.0f, 0 }, { 0, 120.0f, 1.0f, 0.0f, 0 },
    { 0, 120.0f, 1.0f, 0.0f, 0 },          { 0, 120.0f, 1.0f, 0.0f, 0 },
    { 0, 120.0f, 1.0f, 0.0f, 0 },          { 0, 120.0f, 1.0f, 0.0f, 0 },
    { 0, 120.0f, 1.0f, 0.0f, 0 },          { 0, 2.0f, 0.0f, 0.0f, 0 },
    { 0, 120.0f, 1.0f, 0.0f, 0 },
};
static signed int sLastRefLaugh;

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zLaughInit()
{
    char str[32];
    signed int i;

    for (i = 0; i < 17; i++)
    {
        sprintf(str, "laugh_track%02d", i + 1);
        sLaughSoundID[i] = xStrHash(str);
    }
    for (i = 0; i < 15; i++)
    {
        sLaughInfo[i].elapsedTime = sLaughInfo[i].delay;
        sLaughInfo[i].count = 0;
    }
}

static inline signed int zLaughNextLaughEnum()
{
    if (sLastRefLaugh >= 17)
    {
        sLastRefLaugh = 0;
    }
    for (signed int i = 0; i < 17; i++)
    {
        if (iSndLookup(sLaughSoundID[sLastRefLaugh]))
        {
            return sLastRefLaugh++;
        }
        if (++sLastRefLaugh >= 17)
        {
            sLastRefLaugh = 0;
        }
    }
    return -1;
}

void zLaughNotify(signed int situation)
{
    zLaughSituation* s = &sLaughInfo[situation];

    if (sLaughQueueData)
    {
        return;
    }
    if (s->countMax == 0xFFFFFFFF)
    {
        return;
    }
    if (s->countMax && s->count >= s->countMax)
    {
        return;
    }
    if (s->delay > s->elapsedTime)
    {
        return;
    }
    sLaughQueueData = s;
    sLaughTimer = s->punchDelay;
}

void zLaughUpdate(float dt)
{
    signed int i;

    for (i = 0; i < 15; i++)
    {
        sLaughInfo[i].elapsedTime += dt;
    }

    if (sLaughTimer || sLaughQueueData)
    {
        if (sLaughTimer > 0.0f)
        {
            sLaughTimer -= dt;
            if (sLaughTimer < 0.0f)
            {
                sLaughTimer = 0.0f;
            }
        }

        if (0.0f == sLaughTimer && sLaughQueueData)
        {
            signed int laugh = zLaughNextLaughEnum();

            if (laugh >= 0 &&
                xSndPlay(sLaughSoundID[laugh], 0.5f, 0.5f, 0.0f, 0x80, 0x202, 0))
            {
                if (sLaughQueueData)
                {
                    sLaughQueueData->elapsedTime = 0.0f;
                    sLaughQueueData->count++;
                    sLaughQueueData = 0;
                }
                sLastLaughEnum = laugh;
            }
            else
            {
                if (sLaughQueueData)
                {
                    sLaughQueueData->elapsedTime = 0.0f;
                }
                sLaughQueueData = 0;
            }
        }
    }
}

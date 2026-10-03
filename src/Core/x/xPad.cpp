#include "zGlobals.h"
#include "zScene.h"

extern "C" void* memset(void* dst, int c, unsigned int n);

signed int iPadInit();
_tagxPad* iPadEnable(_tagxPad* pad, signed short port);
signed int iPadUpdate(_tagxPad* pad, unsigned int* on);
void iPadRumbleFx(_tagxPad* p, _tagxRumble* r, float time_passed);
void iPadStopRumble(_tagxPad* pad);
void iPadStartRumble(_tagxPad* pad, _tagxRumble* rumble);
void iPadKill();
signed int zGameIsPaused();
signed int zMenuRunning();

extern "C" {
// TODO: unidentified game-state checks
unsigned int fn_800CD24C();
signed int fn_800CD170();
}

enum _tagRumbleType
{
    eRumble_Off = 0,
    eRumbleForceU32 = 0x7FFFFFFF
};

#define SCENE_ID(s) ((((s)[0] & ~0x20) << 24) | ((s)[1] << 16) | ((s)[2] << 8) | (s)[3])

static _tagxRumble mRumbleList[32];
_tagxPad mPad[4];
_tagxPad* gDebugPad;
_tagxPad* gPlayerPad;

static inline void xPadAnalogIsDigital(signed int idx, signed int enable)
{
    if (idx == 0)
    {
        _tagxPad* pad = &mPad[idx];
        if (enable)
        {
            pad->flags |= 0x10;
        }
        else
        {
            pad->flags &= ~0x10;
        }
        if (pad->al2d_timer >= 0.35f)
        {
            pad->al2d_timer = 0.35f;
        }
        if (pad->ar2d_timer >= 0.35f)
        {
            pad->ar2d_timer = 0.35f;
        }
        if (pad->d_timer >= 0.35f)
        {
            pad->d_timer = 0.35f;
        }
    }
}

static inline void xPadDestroyRumbleChain(_tagxPad* pad)
{
    iPadStopRumble(pad);
    _tagxRumble* r = pad->rumble_head.next;
    while (r)
    {
        memset(r, 0, sizeof(_tagxRumble));
        r = r->next;
    }
    pad->rumble_head.next = 0;
}

static inline _tagxRumble* xPadGetRumbleSlot()
{
    for (signed int i = 0; i < 32; i++)
    {
        if (!mRumbleList[i].active)
        {
            memset(&mRumbleList[i], 0, sizeof(_tagxRumble));
            return &mRumbleList[i];
        }
    }
    return 0;
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

signed int xPadInit()
{
    memset(mPad, 0, sizeof(mPad));
    memset(mRumbleList, 0, sizeof(mRumbleList));
    if (!iPadInit())
    {
        return 0;
    }
    gPlayerPad = mPad;
    return 1;
}

_tagxPad* xPadEnable(signed int idx)
{
    _tagxPad* p = &mPad[idx];
    if (p->state != 0)
    {
        return p;
    }
    if (idx != 0)
    {
        return p;
    }
    _tagxPad* ret = iPadEnable(p, idx);
    if (p->flags & 4)
    {
        p->flags |= 8;
    }
    return ret;
}

void xPadRumbleEnable(signed int idx, signed int enable)
{
    _tagxPad* p = &mPad[idx];
    if (enable)
    {
        if (p->flags & 4)
        {
            p->flags |= 8;
        }
    }
    else if (p->flags & 8)
    {
        p->flags ^= 8;
        xPadDestroyRumbleChain(p);
    }
}

signed int xPadUpdate(signed int idx, float time_passed)
{
    _tagxPad* p;
    _tagxRumble* r;
    unsigned int new_on;

    if (idx != 0)
    {
        return 0;
    }

    if (fn_800CD24C())
    {
        xPadAnalogIsDigital(idx, 0);
    }
    else if (zMenuRunning() || zGameIsPaused() ||
             (globals.sceneCur && globals.sceneCur->sceneID == SCENE_ID("h003")))
    {
        xPadAnalogIsDigital(idx, 1);
    }
    else
    {
        xPadAnalogIsDigital(idx, 0);
    }

    p = &mPad[idx];
    if ((p->flags & 4) && (p->flags & 8))
    {
        r = p->rumble_head.next;
        while (r)
        {
            r->seconds -= time_passed;
            if (r->seconds <= 0.0f)
            {
                r->active = 0;
                if (!r->next)
                {
                    p->rumble_head.next = 0;
                    r = 0;
                    iPadStopRumble(p);
                }
                else
                {
                    p->rumble_head.next = r->next;
                    r = p->rumble_head.next;
                    iPadStartRumble(p, r);
                }
            }
            else
            {
                iPadRumbleFx(p, r, time_passed);
                break;
            }
        }
    }

    if (!iPadUpdate(p, &new_on))
    {
        return 1;
    }

    if (p->port == 0 && fn_800CD170())
    {
        unsigned int swapped = new_on & ~0xC0000;
        if (new_on & 0x40000)
        {
            swapped |= 0x80000;
        }
        if (new_on & 0x80000)
        {
            swapped |= 0x40000;
        }
        new_on = swapped;
    }

    if (p->flags & 0x10)
    {
        if (p->flags & 1)
        {
            unsigned int fake_dpad = 0;
            if (p->analog1.x >= 50)
            {
                fake_dpad |= 0x20;
            }
            else if (p->analog1.x <= -50)
            {
                fake_dpad |= 0x80;
            }
            if (p->analog1.y >= 50)
            {
                fake_dpad |= 0x40;
            }
            else if (p->analog1.y <= -50)
            {
                fake_dpad |= 0x10;
            }
            if (fake_dpad == 0)
            {
                p->al2d_timer = 0.0f;
            }
            else
            {
                p->al2d_timer -= time_passed;
                if (p->al2d_timer <= 0.0f)
                {
                    new_on |= fake_dpad;
                    p->al2d_timer = 0.35f;
                }
            }
        }
        if (p->flags & 2)
        {
            if (p->analog2.x > -50 && p->analog2.x < 50 && p->analog2.y > -50 &&
                p->analog2.y < 50)
            {
                p->ar2d_timer = 0.0f;
            }
            else
            {
                p->ar2d_timer -= time_passed;
                if (p->ar2d_timer <= 0.0f)
                {
                    p->ar2d_timer = 0.35f;
                    if (p->analog2.x >= 50)
                    {
                        new_on |= 0x20;
                    }
                    else if (p->analog2.x <= -50)
                    {
                        new_on |= 0x80;
                    }
                    if (p->analog2.y >= 50)
                    {
                        new_on |= 0x40;
                    }
                    else if (p->analog2.y <= -50)
                    {
                        new_on |= 0x10;
                    }
                }
            }
        }
    }

    p->pressed = new_on & ~p->on;
    p->released = p->on & ~new_on;
    p->on = new_on;

    if (p->flags & 0x10)
    {
        if (!(p->on & 0x10) && !(p->on & 0x40) && !(p->on & 0x80) && !(p->on & 0x20))
        {
            p->d_timer = 0.0f;
        }
        else
        {
            p->d_timer -= time_passed;
            if (p->d_timer <= 0.0f)
            {
                p->d_timer = 0.35f;
                if (p->on & 0x10)
                {
                    p->pressed |= 0x10;
                }
                else if (p->on & 0x40)
                {
                    p->pressed |= 0x40;
                }
                if (p->on & 0x80)
                {
                    p->pressed |= 0x80;
                }
                else if (p->on & 0x20)
                {
                    p->pressed |= 0x20;
                }
            }
        }
    }
    return 1;
}

void xPadKill()
{
    iPadKill();
}

void xPadDestroyRumbleChain(signed int idx)
{
    xPadDestroyRumbleChain(&mPad[idx]);
}

signed int xPadAddRumble(signed int idx, _tagRumbleType type, float time, signed int replace,
                         unsigned int fxflags)
{
    signed int appended;
    _tagxPad* pad = &mPad[idx];
    _tagxRumble* r;
    _tagxRumble* last_r;

    if (!(pad->flags & 4))
    {
        return 0;
    }
    if (!(pad->flags & 8))
    {
        return 0;
    }

    appended = 1;
    if (replace)
    {
        xPadDestroyRumbleChain(pad);
        appended = 0;
    }

    r = &pad->rumble_head;
    last_r = r;
    if (!r->next)
    {
        appended = 0;
    }
    while (r)
    {
        last_r = r;
        r = r->next;
    }

    r = xPadGetRumbleSlot();
    if (!r)
    {
        return 0;
    }
    r->type = type;
    r->seconds = time;
    r->active = 1;
    r->fxflags = fxflags;
    last_r->next = r;
    last_r->next->next = 0;
    if (!appended)
    {
        iPadStartRumble(pad, r);
    }
    return 1;
}

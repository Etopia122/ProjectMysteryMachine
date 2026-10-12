// GameCube controller API (PAD) on the New 3DS buttons.
//
// Circle Pad = stick, C-stick = C-stick, A/B/X/Y as on GameCube, Start = Start,
// L/R = L/R triggers, ZR = Z, D-pad = D-pad. ZL is free for the port's own use.
#include <3ds.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_platform.h"

extern "C" {

struct PADStatus
{
    unsigned short button;
    signed char stickX, stickY, substickX, substickY;
    unsigned char triggerLeft, triggerRight, analogA, analogB;
    signed char err;
};

enum {
    PAD_BUTTON_LEFT = 0x0001, PAD_BUTTON_RIGHT = 0x0002, PAD_BUTTON_DOWN = 0x0004, PAD_BUTTON_UP = 0x0008,
    PAD_TRIGGER_Z = 0x0010, PAD_TRIGGER_R = 0x0020, PAD_TRIGGER_L = 0x0040, PAD_BUTTON_A = 0x0100,
    PAD_BUTTON_B = 0x0200, PAD_BUTTON_X = 0x0400, PAD_BUTTON_Y = 0x0800, PAD_BUTTON_START = 0x1000,
};
enum { PAD_ERR_NONE = 0, PAD_ERR_NO_CONTROLLER = -1 };

int PADInit() { return 1; }
int PADReset(unsigned int mask) { return 1; }
int PADRecalibrate(unsigned int mask) { return 1; }
void PADControlMotor(int chan, unsigned int command) {}

// Circle Pad and C-stick report about +-156; scale to the GameCube's +-127.
static signed char axis8(int v)
{
    int x = v * 127 / 150;
    return (signed char)(x > 127 ? 127 : x < -127 ? -127 : x);
}

// The touch screen (the bottom screen, where the HUD is), in the HUD's 640x480
// space. hidScanInput runs once per frame in pc_events_pump.
extern "C" int pc_touch_read(int* x, int* y)
{
    // Debug: PC_AUTOTOUCH=<seconds>:<x>:<y> touches the screen pixel (x, y) for
    // 0.3 seconds from then.
    static const char* autoTouch = getenv("PC_AUTOTOUCH");
    if (autoTouch)
    {
        float at = 0;
        int tx = 0, ty = 0;
        unsigned int now = pc_ticks_ms();
        if (sscanf(autoTouch, "%f:%d:%d", &at, &tx, &ty) == 3 && now >= (unsigned int)(at * 1000) &&
            now < (unsigned int)(at * 1000) + 300)
        {
            *x = tx * 2;
            *y = ty * 2;
            return 1;
        }
    }
    if (pc_debug_menu_active() || !(hidKeysHeld() & KEY_TOUCH))
    {
        return 0;
    }
    touchPosition t;
    hidTouchRead(&t);
    *x = t.px * 2;
    *y = t.py * 2;
    return 1;
}

unsigned int PADRead(PADStatus* status)
{
    memset(status, 0, sizeof(PADStatus) * 4);
    for (int i = 1; i < 4; i++)
    {
        status[i].err = PAD_ERR_NO_CONTROLLER;
    }
    if (pc_debug_menu_active())
    {
        return 0x80000000; // the debug menu has the buttons
    }
    // hidScanInput runs once per frame in pc_events_pump.
    u32 k = hidKeysHeld();
    unsigned short b = 0;
    if (k & KEY_A) b |= PAD_BUTTON_A;
    if (k & KEY_B) b |= PAD_BUTTON_B;
    if ((k & KEY_X) && !(k & KEY_ZL)) b |= PAD_BUTTON_X; // ZL+X/Y are debug keys (main.cpp)
    if ((k & KEY_Y) && !(k & KEY_ZL)) b |= PAD_BUTTON_Y;
    if (k & KEY_START) b |= PAD_BUTTON_START;
    if (k & KEY_L) b |= PAD_TRIGGER_L;
    if (k & KEY_R) b |= PAD_TRIGGER_R;
    if (k & KEY_ZR) b |= PAD_TRIGGER_Z;
    if (k & KEY_DUP) b |= PAD_BUTTON_UP;
    if (k & KEY_DDOWN) b |= PAD_BUTTON_DOWN;
    if (k & KEY_DLEFT) b |= PAD_BUTTON_LEFT;
    if (k & KEY_DRIGHT) b |= PAD_BUTTON_RIGHT;
    circlePosition cp, cs;
    hidCircleRead(&cp);
    hidCstickRead(&cs);
    int sx = axis8(cp.dx), sy = axis8(cp.dy);
    pc_autopad(&b, &sx, &sy);
    PADStatus& p = status[0];
    p.button = b;
    p.stickX = (signed char)sx;
    p.stickY = (signed char)sy;
    p.substickX = axis8(cs.dx);
    p.substickY = axis8(cs.dy);
    p.triggerLeft = (b & PAD_TRIGGER_L) ? 255 : 0;
    p.triggerRight = (b & PAD_TRIGGER_R) ? 255 : 0;
    p.analogA = (b & PAD_BUTTON_A) ? 255 : 0;
    p.analogB = (b & PAD_BUTTON_B) ? 255 : 0;
    p.err = PAD_ERR_NONE;
    return 0x80000000;
}

// Clamp to the ranges a real GameCube controller reports after PADClamp:
// sticks to radius 72 (C-stick 59) past a dead zone, triggers 0-150.
static void clamp_stick(signed char* px, signed char* py, int deadzone, int max)
{
    int x = *px, y = *py;
    float len = sqrtf((float)(x * x + y * y));
    if (len <= deadzone)
    {
        *px = *py = 0;
        return;
    }
    float scaled = len - deadzone;
    if (scaled > max)
    {
        scaled = (float)max;
    }
    float k = scaled / len;
    *px = (signed char)(x * k);
    *py = (signed char)(y * k);
}

void PADClamp(PADStatus* status)
{
    for (int i = 0; i < 4; i++)
    {
        PADStatus& p = status[i];
        if (p.err != PAD_ERR_NONE)
        {
            continue;
        }
        clamp_stick(&p.stickX, &p.stickY, 15, 72);
        clamp_stick(&p.substickX, &p.substickY, 15, 59);
        p.triggerLeft = p.triggerLeft <= 30 ? 0 : (unsigned char)(p.triggerLeft > 180 ? 150 : p.triggerLeft - 30);
        p.triggerRight = p.triggerRight <= 30 ? 0 : (unsigned char)(p.triggerRight > 180 ? 150 : p.triggerRight - 30);
    }
}

}

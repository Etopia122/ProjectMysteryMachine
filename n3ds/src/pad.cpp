// GameCube controller API (PAD) on the New 3DS buttons.
//
// Circle Pad = stick, C-stick = C-stick, A/B/X/Y as on GameCube, Start = Start,
// L/R = L/R triggers, ZR = Z, D-pad = D-pad. ZL is free for the port's own use.
#include <3ds.h>
#include <math.h>
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

unsigned int PADRead(PADStatus* status)
{
    memset(status, 0, sizeof(PADStatus) * 4);
    for (int i = 1; i < 4; i++)
    {
        status[i].err = PAD_ERR_NO_CONTROLLER;
    }
    // hidScanInput runs once per frame in pc_events_pump.
    u32 k = hidKeysHeld();
    unsigned short b = 0;
    if (k & KEY_A) b |= PAD_BUTTON_A;
    if (k & KEY_B) b |= PAD_BUTTON_B;
    if (k & KEY_X) b |= PAD_BUTTON_X;
    if (k & KEY_Y) b |= PAD_BUTTON_Y;
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

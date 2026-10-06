// GameCube controller API (PAD) on SDL2: first game controller and the keyboard
// both drive port 0.
//
// Keyboard: WASD/arrows = stick, IJKL = C-stick, Space = A, Shift/J = B,
// E = X, R = Y, Enter = Start, Q = L, Tab = R, F = Z, number pad 8/2/4/6 = D-pad.
#include "pc_platform.h"

#include <SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static SDL_GameController* s_ctrl;

static void open_controller()
{
    if (s_ctrl && SDL_GameControllerGetAttached(s_ctrl))
    {
        return;
    }
    s_ctrl = NULL;
    for (int i = 0; i < SDL_NumJoysticks(); i++)
    {
        if (SDL_IsGameController(i))
        {
            s_ctrl = SDL_GameControllerOpen(i);
            if (s_ctrl)
            {
                pc_log("[pad] using controller: %s", SDL_GameControllerName(s_ctrl));
                break;
            }
        }
    }
}

int PADInit()
{
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
    open_controller();
    return 1;
}

int PADReset(unsigned int mask) { return 1; }
int PADRecalibrate(unsigned int mask) { return 1; }

void PADControlMotor(int chan, unsigned int command)
{
    if (s_ctrl && chan == 0)
    {
        if (command == 1)
        {
            SDL_GameControllerRumble(s_ctrl, 0x6000, 0x6000, 100);
        }
        else
        {
            SDL_GameControllerRumble(s_ctrl, 0, 0, 0);
        }
    }
}

static signed char axis8(Sint16 v)
{
    int x = v / 256;
    return (signed char)(x > 127 ? 127 : x < -127 ? -127 : x);
}

unsigned int PADRead(PADStatus* status)
{
    memset(status, 0, sizeof(PADStatus) * 4);
    for (int i = 1; i < 4; i++)
    {
        status[i].err = PAD_ERR_NO_CONTROLLER;
    }
    SDL_PumpEvents();
    open_controller();
    PADStatus& p = status[0];
    const Uint8* k = SDL_GetKeyboardState(NULL);
    int sx = 0, sy = 0, cx = 0, cy = 0;
    if (k[SDL_SCANCODE_A] || k[SDL_SCANCODE_LEFT]) sx -= 127;
    if (k[SDL_SCANCODE_D] || k[SDL_SCANCODE_RIGHT]) sx += 127;
    if (k[SDL_SCANCODE_W] || k[SDL_SCANCODE_UP]) sy += 127;
    if (k[SDL_SCANCODE_S] || k[SDL_SCANCODE_DOWN]) sy -= 127;
    if (k[SDL_SCANCODE_J]) cx -= 127;
    if (k[SDL_SCANCODE_L]) cx += 127;
    if (k[SDL_SCANCODE_I]) cy += 127;
    if (k[SDL_SCANCODE_K]) cy -= 127;
    unsigned short b = 0;
    if (k[SDL_SCANCODE_SPACE]) b |= PAD_BUTTON_A;
    if (k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT]) b |= PAD_BUTTON_B;
    if (k[SDL_SCANCODE_E]) b |= PAD_BUTTON_X;
    if (k[SDL_SCANCODE_R]) b |= PAD_BUTTON_Y;
    if (k[SDL_SCANCODE_RETURN]) b |= PAD_BUTTON_START;
    if (k[SDL_SCANCODE_Q]) b |= PAD_TRIGGER_L;
    if (k[SDL_SCANCODE_TAB]) b |= PAD_TRIGGER_R;
    if (k[SDL_SCANCODE_F]) b |= PAD_TRIGGER_Z;
    if (k[SDL_SCANCODE_KP_8]) b |= PAD_BUTTON_UP;
    if (k[SDL_SCANCODE_KP_2]) b |= PAD_BUTTON_DOWN;
    if (k[SDL_SCANCODE_KP_4]) b |= PAD_BUTTON_LEFT;
    if (k[SDL_SCANCODE_KP_6]) b |= PAD_BUTTON_RIGHT;

    unsigned char tl = (b & PAD_TRIGGER_L) ? 255 : 0, tr = (b & PAD_TRIGGER_R) ? 255 : 0;
    if (s_ctrl)
    {
        SDL_GameController* c = s_ctrl;
        // Xbox-style layout: A=A, B=B, X=X, Y=Y; right shoulder = Z.
        if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_A)) b |= PAD_BUTTON_A;
        if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_B)) b |= PAD_BUTTON_B;
        if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_X)) b |= PAD_BUTTON_X;
        if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_Y)) b |= PAD_BUTTON_Y;
        if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_START)) b |= PAD_BUTTON_START;
        if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) b |= PAD_TRIGGER_Z;
        if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_UP)) b |= PAD_BUTTON_UP;
        if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_DOWN)) b |= PAD_BUTTON_DOWN;
        if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) b |= PAD_BUTTON_LEFT;
        if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) b |= PAD_BUTTON_RIGHT;
        int lx = axis8(SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX));
        int ly = -axis8(SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY));
        int rx = axis8(SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTX));
        int ry = -axis8(SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTY));
        if (abs(lx) > abs(sx)) sx = lx;
        if (abs(ly) > abs(sy)) sy = ly;
        if (abs(rx) > abs(cx)) cx = rx;
        if (abs(ry) > abs(cy)) cy = ry;
        int lt = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 128;
        int rt = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 128;
        if (lt > tl) tl = (unsigned char)lt;
        if (rt > tr) tr = (unsigned char)rt;
        if (lt > 200) b |= PAD_TRIGGER_L;
        if (rt > 200) b |= PAD_TRIGGER_R;
    }
    // Test automation: PC_AUTOPAD="sec:hexmask[:sx:sy],..." holds buttons/stick from that time on.
    static const char* script = getenv("PC_AUTOPAD");
    if (script)
    {
        static Uint32 t0 = SDL_GetTicks();
        float now = (SDL_GetTicks() - t0) / 1000.0f;
        unsigned int mask = 0;
        int ax = 0, ay = 0;
        for (const char* q = script; q && *q;)
        {
            float t;
            unsigned int m;
            int x = 0, y = 0, n = 0;
            if (sscanf(q, "%f:%x%n", &t, &m, &n) < 2)
            {
                break;
            }
            const char* r = q + n;
            int n2 = 0;
            if (sscanf(r, ":%d:%d%n", &x, &y, &n2) == 2)
            {
                r += n2;
            }
            if (t <= now)
            {
                mask = m;
                ax = x;
                ay = y;
            }
            q = strchr(r, ',');
            q = q ? q + 1 : NULL;
        }
        b |= (unsigned short)mask;
        if (ax) sx = ax;
        if (ay) sy = ay;
    }
    p.button = b;
    p.stickX = (signed char)sx;
    p.stickY = (signed char)sy;
    p.substickX = (signed char)cx;
    p.substickY = (signed char)cy;
    p.triggerLeft = tl;
    p.triggerRight = tr;
    p.analogA = (b & PAD_BUTTON_A) ? 255 : 0;
    p.analogB = (b & PAD_BUTTON_B) ? 255 : 0;
    p.err = PAD_ERR_NONE;
    return 0x80000000;
}

// Clamp to the ranges a real GameCube controller reports after PADClamp:
// sticks to an octagon of radius 72 (C-stick 59) past a dead zone, triggers 0-150.
static void clamp_stick(signed char* px, signed char* py, int deadzone, int max)
{
    int x = *px, y = *py;
    float len = sqrtf((float)(x * x + y * y));
    if (len <= deadzone)
    {
        *px = *py = 0;
        return;
    }
    float scaled = (len - deadzone);
    float maxlen = (float)max;
    if (scaled > maxlen)
    {
        scaled = maxlen;
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

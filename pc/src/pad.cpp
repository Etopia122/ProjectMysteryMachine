// GameCube controller API (PAD) on SDL2: first game controller and the keyboard
// both drive port 0.
//
// Bindings come from scooby.ini ([keyboard] and [controller], written by the
// launcher); the defaults: WASD/arrows = stick, IJKL = C-stick, Space = A,
// Shift = B, E = X, R = Y, Enter = Start, Q = L, Tab = R, F = Z, number pad
// 8/2/4/6 = D-pad; on a controller A/B/X/Y as labelled, triggers = L/R, right
// shoulder = Z, left stick = stick, right stick = C-stick.
#include "pc_platform.h"

#include <SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

extern "C" {
int pc_debug_menu_active();

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

// One GameCube input and what drives it. Setting names are the launcher's.
struct PadAction
{
    const char* name;      // [keyboard]/[controller] key in scooby.ini
    unsigned short mask;   // button bit; 0 for stick directions
    int stick, dx, dy;     // stick directions: 1 main, 2 C-stick, and the direction
    const char* keys;      // default keys (SDL key names, comma-separated)
    const char* pad;       // default controller button or axis (SDL names)
};

static const PadAction kActions[] = {
    { "stick_up", 0, 1, 0, 1, "W,Up", "" },
    { "stick_down", 0, 1, 0, -1, "S,Down", "" },
    { "stick_left", 0, 1, -1, 0, "A,Left", "" },
    { "stick_right", 0, 1, 1, 0, "D,Right", "" },
    { "cstick_up", 0, 2, 0, 1, "I", "" },
    { "cstick_down", 0, 2, 0, -1, "K", "" },
    { "cstick_left", 0, 2, -1, 0, "J", "" },
    { "cstick_right", 0, 2, 1, 0, "L", "" },
    { "a", PAD_BUTTON_A, 0, 0, 0, "Space", "a" },
    { "b", PAD_BUTTON_B, 0, 0, 0, "Left Shift,Right Shift", "b" },
    { "x", PAD_BUTTON_X, 0, 0, 0, "E", "x" },
    { "y", PAD_BUTTON_Y, 0, 0, 0, "R", "y" },
    { "start", PAD_BUTTON_START, 0, 0, 0, "Return", "start" },
    { "l", PAD_TRIGGER_L, 0, 0, 0, "Q", "lefttrigger" },
    { "r", PAD_TRIGGER_R, 0, 0, 0, "Tab", "righttrigger" },
    { "z", PAD_TRIGGER_Z, 0, 0, 0, "F", "rightshoulder" },
    { "dpad_up", PAD_BUTTON_UP, 0, 0, 0, "Keypad 8", "dpup" },
    { "dpad_down", PAD_BUTTON_DOWN, 0, 0, 0, "Keypad 2", "dpdown" },
    { "dpad_left", PAD_BUTTON_LEFT, 0, 0, 0, "Keypad 4", "dpleft" },
    { "dpad_right", PAD_BUTTON_RIGHT, 0, 0, 0, "Keypad 6", "dpright" },
};
static const int kNumActions = sizeof(kActions) / sizeof(kActions[0]);

struct PadBinding
{
    std::vector<SDL_Scancode> keys;
    SDL_GameControllerButton button = SDL_CONTROLLER_BUTTON_INVALID;
    SDL_GameControllerAxis axis = SDL_CONTROLLER_AXIS_INVALID;
};
static PadBinding s_bind[kNumActions];
static bool s_swapSticks; // controller.sticks = swapped: the right stick moves Scooby
static float s_mouseCamera; // keyboard.mouse_camera: mouse movement turns the C-stick (its sensitivity; 0 off)

static std::string trim(const std::string& t)
{
    size_t a = t.find_first_not_of(' '), b = t.find_last_not_of(' ');
    return a == std::string::npos ? std::string() : t.substr(a, b - a + 1);
}

static void load_bindings()
{
    static bool loaded;
    if (loaded)
    {
        return;
    }
    loaded = true;
    for (int i = 0; i < kNumActions; i++)
    {
        std::string key = std::string("keyboard.") + kActions[i].name;
        std::string list = pc_setting_str(key.c_str(), kActions[i].keys);
        size_t pos = 0;
        for (;;)
        {
            size_t comma = list.find(',', pos);
            std::string name = trim(list.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos));
            SDL_Scancode sc = name.empty() ? SDL_SCANCODE_UNKNOWN : SDL_GetScancodeFromName(name.c_str());
            if (sc != SDL_SCANCODE_UNKNOWN)
            {
                s_bind[i].keys.push_back(sc);
            }
            else if (!name.empty())
            {
                pc_log("[pad] %s: unknown key \"%s\"", key.c_str(), name.c_str());
            }
            if (comma == std::string::npos)
            {
                break;
            }
            pos = comma + 1;
        }
        if (kActions[i].mask)
        {
            key = std::string("controller.") + kActions[i].name;
            std::string name = trim(pc_setting_str(key.c_str(), kActions[i].pad));
            s_bind[i].button = SDL_GameControllerGetButtonFromString(name.c_str());
            if (s_bind[i].button == SDL_CONTROLLER_BUTTON_INVALID)
            {
                s_bind[i].axis = SDL_GameControllerGetAxisFromString(name.c_str());
            }
        }
    }
    s_swapSticks = !strcmp(pc_setting_str("controller.sticks", "normal"), "swapped");
    s_mouseCamera = pc_setting_int("keyboard.mouse_camera", 0) ? pc_setting_float("keyboard.mouse_sensitivity", 1.0f) : 0.0f;
}

// The mouse as the C-stick (keyboard.mouse_camera): the cursor is captured
// while the game window has the keyboard and the debug menu is closed; how
// far it moves in a frame is how far the C-stick is pushed.
static void mouse_camera(int* cx, int* cy)
{
    if (s_mouseCamera <= 0.0f)
    {
        return;
    }
    bool capture = SDL_GetKeyboardFocus() != NULL && !pc_debug_menu_active();
    if ((SDL_GetRelativeMouseMode() == SDL_TRUE) != capture)
    {
        SDL_SetRelativeMouseMode(capture ? SDL_TRUE : SDL_FALSE);
        SDL_GetRelativeMouseState(NULL, NULL); // drop the jump from before
        return;
    }
    if (!capture)
    {
        return;
    }
    int dx = 0, dy = 0;
    SDL_GetRelativeMouseState(&dx, &dy);
    int mx = (int)(dx * 8.0f * s_mouseCamera), my = (int)(-dy * 8.0f * s_mouseCamera);
    if (abs(mx) > abs(*cx))
    {
        *cx = mx > 127 ? 127 : mx < -127 ? -127 : mx;
    }
    if (abs(my) > abs(*cy))
    {
        *cy = my > 127 ? 127 : my < -127 ? -127 : my;
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
    if (pc_debug_menu_active())
    {
        return 0x80000000; // the debug menu has the keyboard and controller
    }
    load_bindings();
    const Uint8* k = SDL_GetKeyboardState(NULL);
    int sx = 0, sy = 0, cx = 0, cy = 0;
    unsigned short b = 0;
    unsigned char tl = 0, tr = 0;
    for (int i = 0; i < kNumActions; i++)
    {
        const PadAction& a = kActions[i];
        bool down = false;
        for (SDL_Scancode sc : s_bind[i].keys)
        {
            down |= k[sc] != 0;
        }
        if (s_ctrl && a.mask)
        {
            if (s_bind[i].button != SDL_CONTROLLER_BUTTON_INVALID)
            {
                down |= SDL_GameControllerGetButton(s_ctrl, s_bind[i].button) != 0;
            }
            else if (s_bind[i].axis != SDL_CONTROLLER_AXIS_INVALID)
            {
                // An axis as a button: pressed past most of its travel; L and
                // R also take its analog value.
                int v = SDL_GameControllerGetAxis(s_ctrl, s_bind[i].axis) / 128;
                v = v < 0 ? -v : v;
                down |= v > 200;
                if (a.mask == PAD_TRIGGER_L && v > tl)
                {
                    tl = (unsigned char)v;
                }
                if (a.mask == PAD_TRIGGER_R && v > tr)
                {
                    tr = (unsigned char)v;
                }
            }
        }
        if (!down)
        {
            continue;
        }
        b |= a.mask;
        if (a.stick == 1)
        {
            sx += 127 * a.dx;
            sy += 127 * a.dy;
        }
        else if (a.stick == 2)
        {
            cx += 127 * a.dx;
            cy += 127 * a.dy;
        }
    }
    if (b & PAD_TRIGGER_L)
    {
        tl = 255;
    }
    if (b & PAD_TRIGGER_R)
    {
        tr = 255;
    }
    mouse_camera(&cx, &cy);
    // Two keys for one direction (A and Left) count once.
    sx = sx > 127 ? 127 : sx < -127 ? -127 : sx;
    sy = sy > 127 ? 127 : sy < -127 ? -127 : sy;
    cx = cx > 127 ? 127 : cx < -127 ? -127 : cx;
    cy = cy > 127 ? 127 : cy < -127 ? -127 : cy;
    if (s_ctrl)
    {
        SDL_GameController* c = s_ctrl;
        SDL_GameControllerAxis mx = s_swapSticks ? SDL_CONTROLLER_AXIS_RIGHTX : SDL_CONTROLLER_AXIS_LEFTX;
        SDL_GameControllerAxis my = s_swapSticks ? SDL_CONTROLLER_AXIS_RIGHTY : SDL_CONTROLLER_AXIS_LEFTY;
        SDL_GameControllerAxis ox = s_swapSticks ? SDL_CONTROLLER_AXIS_LEFTX : SDL_CONTROLLER_AXIS_RIGHTX;
        SDL_GameControllerAxis oy = s_swapSticks ? SDL_CONTROLLER_AXIS_LEFTY : SDL_CONTROLLER_AXIS_RIGHTY;
        int lx = axis8(SDL_GameControllerGetAxis(c, mx));
        int ly = -axis8(SDL_GameControllerGetAxis(c, my));
        int rx = axis8(SDL_GameControllerGetAxis(c, ox));
        int ry = -axis8(SDL_GameControllerGetAxis(c, oy));
        if (abs(lx) > abs(sx)) sx = lx;
        if (abs(ly) > abs(sy)) sy = ly;
        if (abs(rx) > abs(cx)) cx = rx;
        if (abs(ry) > abs(cy)) cy = ry;
    }
    int ax = sx, ay = sy;
    pc_autopad(&b, &ax, &ay);
    sx = ax;
    sy = ay;
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

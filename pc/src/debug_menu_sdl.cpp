// PC input for the debug menu (pc/src/debug_menu.cpp): F1 or the controller's
// Back button opens it; arrows/Enter/Esc or the D-pad/A/B drive it. F3 shows
// the coordinates, F4 saves them, F5 returns to the saved spot.
#include "pc_platform.h"

#include <SDL.h>

extern "C" const char* pc_debug_key_hint(int which)
{
    return which == PC_HINT_MENU ? "F1" : which == PC_HINT_SAVE ? "F4" : "F5";
}

extern "C" void pc_set_clipboard(const char* text)
{
    SDL_SetClipboardText(text);
}

// Handles one SDL event; returns true if it was consumed.
bool pc_debug_menu_event(const SDL_Event& e)
{
    if (e.type == SDL_KEYDOWN)
    {
        SDL_Keycode k = e.key.keysym.sym;
        if (!e.key.repeat)
        {
            switch (k)
            {
            case SDLK_F1: pc_debug_menu_toggle(); return true;
            case SDLK_F3: pc_debug_coords_toggle(); return true;
            case SDLK_F4: pc_debug_coords_save(); return true;
            case SDLK_F5: pc_debug_coords_return(); return true;
            default: break;
            }
        }
        switch (k)
        {
        case SDLK_UP: return pc_debug_menu_key(PC_MENU_UP);
        case SDLK_DOWN: return pc_debug_menu_key(PC_MENU_DOWN);
        case SDLK_LEFT: return pc_debug_menu_key(PC_MENU_LEFT);
        case SDLK_RIGHT: return pc_debug_menu_key(PC_MENU_RIGHT);
        case SDLK_RETURN: case SDLK_SPACE: return pc_debug_menu_key(PC_MENU_OK);
        case SDLK_ESCAPE: case SDLK_BACKSPACE: return pc_debug_menu_key(PC_MENU_BACK);
        default: return pc_debug_menu_active();
        }
    }
    if (e.type == SDL_CONTROLLERBUTTONDOWN)
    {
        switch (e.cbutton.button)
        {
        case SDL_CONTROLLER_BUTTON_BACK: pc_debug_menu_toggle(); return true;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return pc_debug_menu_key(PC_MENU_UP);
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return pc_debug_menu_key(PC_MENU_DOWN);
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return pc_debug_menu_key(PC_MENU_LEFT);
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return pc_debug_menu_key(PC_MENU_RIGHT);
        case SDL_CONTROLLER_BUTTON_A: return pc_debug_menu_key(PC_MENU_OK);
        case SDL_CONTROLLER_BUTTON_B: return pc_debug_menu_key(PC_MENU_BACK);
        default: return pc_debug_menu_active();
        }
    }
    return false;
}

// Debug menu and position readout (keyboard F1/F3/F4/F5, controller Back).
//
// F1 (or the controller's Back/Select button) opens a menu: warp to any level,
// all upgrades, unlock warp gates, refill, snacks, infinite health, unlock
// extras, coordinate readout. Up/Down choose, Left/Right pick a level's scene,
// Enter/Space (A) selects, Esc/Backspace (B) goes back. Game input is ignored
// while the menu is open.
// F3 toggles the scene/position readout, F4 saves the position to coords.txt
// next to the exe (and the clipboard), F5 teleports back to it.
#include "pc_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

extern "C" {
// Game side (src/Game/zPCDebug.cpp, zScene.cpp).
int pc_dbg_warp(unsigned int sceneID);
void pc_dbg_all_upgrades();
void pc_dbg_all_warps();
void pc_dbg_unlock_extras();
void pc_dbg_refill();
void pc_dbg_add_snacks(unsigned int n);
int pc_dbg_toggle_infinite_health();
void pc_dbg_frame();
void pc_dbg_event(const char* name, unsigned int event);
const char* pc_dbg_force_boss_damage();
void pc_dbg_ent_info(const char* name);
void pc_dbg_hurt();
int pc_debug_area(int i, const char** name, const unsigned int** scenes);
unsigned int pc_debug_scene_id();
void pc_debug_player_pos(float* out);
void pc_debug_teleport(float x, float y, float z);
void pc_render_set_overlay(const char* text);
}

namespace {

enum Page { MAIN, LEVELS };
enum Item { WARP, UPGRADES, GATES, REFILL, SNACKS, INFHEALTH, BOSSDMG, EXTRAS, COORDS, CLOSE, ITEM_COUNT };

bool s_open;
Page s_page = MAIN;
int s_cursor;          // main menu row
int s_levelCursor = 1; // area row (area 0 is empty)
int s_sceneSel[32];    // chosen scene per area
bool s_infHealth;
bool s_showCoords; // PC_COORDS=1 starts with the readout on (read on the first frame:
                   // the 3DS sets its environment in main)
std::string s_status;
unsigned int s_statusUntil;

float s_savedPos[3];
bool s_hasSaved;

void scene_text(unsigned int id, char* out)
{
    for (int i = 0; i < 4; i++)
    {
        char c = (char)(id >> (24 - i * 8));
        out[i] = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ? c : '-';
    }
    out[4] = 0;
}

void status(const char* msg)
{
    s_status = msg;
    s_statusUntil = pc_ticks_ms() + 2500;
    pc_log("[debug] %s", msg);
}

int area_count()
{
    const char* n;
    const unsigned int* s;
    int i = 0;
    while (pc_debug_area(i, &n, &s))
    {
        i++;
    }
    return i;
}

int area_scene_count(int area)
{
    const char* n;
    const unsigned int* s;
    if (!pc_debug_area(area, &n, &s))
    {
        return 0;
    }
    int c = 0;
    while (c < 4 && s[c])
    {
        c++;
    }
    return c;
}

void save_coords()
{
    char scene[5], line[128];
    float p[3];
    scene_text(pc_debug_scene_id(), scene);
    pc_debug_player_pos(p);
    memcpy(s_savedPos, p, sizeof(p));
    s_hasSaved = true;
    snprintf(line, sizeof(line), "%s %.2f,%.2f,%.2f", scene, p[0], p[1], p[2]);
    std::string path = std::string(pc_base_path()) + "coords.txt";
    if (FILE* f = fopen(path.c_str(), "a"))
    {
        fprintf(f, "%s\n", line);
        fclose(f);
    }
    pc_set_clipboard(line);
    pc_log("[coords] %s (saved to coords.txt)", line);
    status("POSITION SAVED TO COORDS.TXT");
}

void activate()
{
    switch (s_cursor)
    {
    case WARP:
        s_page = LEVELS;
        break;
    case UPGRADES:
        pc_dbg_all_upgrades();
        status("ALL UPGRADES GIVEN");
        break;
    case GATES:
        pc_dbg_all_warps();
        status("ALL WARP GATES UNLOCKED");
        break;
    case REFILL:
        pc_dbg_refill();
        status("HEALTH, CHANCES AND AMMO REFILLED");
        break;
    case SNACKS:
        pc_dbg_add_snacks(100);
        status("100 SCOOBY SNACKS ADDED");
        break;
    case INFHEALTH:
        s_infHealth = pc_dbg_toggle_infinite_health() != 0;
        status(s_infHealth ? "INFINITE HEALTH ON" : "INFINITE HEALTH OFF");
        break;
    case BOSSDMG:
        status(pc_dbg_force_boss_damage());
        break;
    case EXTRAS:
        pc_dbg_unlock_extras();
        status("MOVIES AND CREDITS UNLOCKED");
        break;
    case COORDS:
        s_showCoords = !s_showCoords;
        break;
    case CLOSE:
        s_open = false;
        break;
    }
}

void warp_selected()
{
    const char* name;
    const unsigned int* scenes;
    if (!pc_debug_area(s_levelCursor, &name, &scenes))
    {
        return;
    }
    unsigned int id = scenes[s_sceneSel[s_levelCursor]];
    char sc[5];
    scene_text(id, sc);
    if (id && pc_dbg_warp(id))
    {
        std::string msg = std::string("WARPING TO ") + sc;
        status(msg.c_str());
        s_open = false;
        s_page = MAIN;
    }
    else
    {
        status("CAN ONLY WARP DURING GAMEPLAY");
    }
}

enum Key { K_UP, K_DOWN, K_LEFT, K_RIGHT, K_OK, K_BACK };

void navigate(Key k)
{
    if (s_page == MAIN)
    {
        if (k == K_UP) s_cursor = (s_cursor + ITEM_COUNT - 1) % ITEM_COUNT;
        if (k == K_DOWN) s_cursor = (s_cursor + 1) % ITEM_COUNT;
        if (k == K_OK) activate();
        if (k == K_BACK) s_open = false;
        return;
    }
    int n = area_count();
    if (k == K_UP) s_levelCursor = s_levelCursor > 1 ? s_levelCursor - 1 : n - 1;
    if (k == K_DOWN) s_levelCursor = s_levelCursor < n - 1 ? s_levelCursor + 1 : 1;
    int sc = area_scene_count(s_levelCursor);
    if (sc > 0 && k == K_LEFT) s_sceneSel[s_levelCursor] = (s_sceneSel[s_levelCursor] + sc - 1) % sc;
    if (sc > 0 && k == K_RIGHT) s_sceneSel[s_levelCursor] = (s_sceneSel[s_levelCursor] + 1) % sc;
    if (k == K_OK) warp_selected();
    if (k == K_BACK) s_page = MAIN;
}

void build_overlay()
{
    std::string t;
    char buf[160];
    if (s_open && s_page == MAIN)
    {
        t = std::string("DEBUG MENU  (") + pc_debug_key_hint(PC_HINT_MENU) + " CLOSE)\n\n";
        const char* labels[ITEM_COUNT] = {
            "WARP TO LEVEL",          "ALL UPGRADES",           "UNLOCK ALL WARP GATES",
            "REFILL HEALTH AND AMMO", "ADD 100 SCOOBY SNACKS",  s_infHealth ? "INFINITE HEALTH: ON" : "INFINITE HEALTH: OFF",
            "FORCE BOSS DAMAGE",
            "UNLOCK MOVIES, CREDITS", s_showCoords ? "COORDINATES: ON" : "COORDINATES: OFF", "CLOSE",
        };
        for (int i = 0; i < ITEM_COUNT; i++)
        {
            snprintf(buf, sizeof(buf), "%s %s\n", i == s_cursor ? ">" : " ", labels[i]);
            t += buf;
        }
    }
    else if (s_open && s_page == LEVELS)
    {
        t = "WARP TO LEVEL  (LEFT/RIGHT: SCENE)\n\n";
        int n = area_count();
        const int rows = 14;
        int first = s_levelCursor - rows / 2;
        if (first < 1) first = 1;
        if (first > n - rows) first = n - rows > 1 ? n - rows : 1;
        for (int i = first; i < n && i < first + rows; i++)
        {
            const char* name;
            const unsigned int* scenes;
            pc_debug_area(i, &name, &scenes);
            char sc[5];
            scene_text(scenes[s_sceneSel[i]], sc);
            snprintf(buf, sizeof(buf), "%s %-32.32s %s\n", i == s_levelCursor ? ">" : " ", name, sc);
            t += buf;
        }
    }
    if (s_showCoords)
    {
        char scene[5];
        float p[3];
        scene_text(pc_debug_scene_id(), scene);
        pc_debug_player_pos(p);
        snprintf(buf, sizeof(buf), "%sSCENE %s  X %.2f  Y %.2f  Z %.2f\n%s SAVE  %s RETURN\n", t.empty() ? "" : "\n",
                 scene, p[0], p[1], p[2], pc_debug_key_hint(PC_HINT_SAVE), pc_debug_key_hint(PC_HINT_RETURN));
        t += buf;
    }
    if (!s_status.empty() && pc_ticks_ms() < s_statusUntil)
    {
        t += (t.empty() ? "" : "\n") + s_status;
    }
    while (!t.empty() && t.back() == '\n')
    {
        t.pop_back();
    }
    pc_render_set_overlay(t.c_str());
}

} // namespace

extern "C" int pc_debug_menu_active() { return s_open; }

// Platform input (pc/src/debug_menu_sdl.cpp on PC, n3ds/src/main.cpp on 3DS).
extern "C" void pc_debug_menu_toggle()
{
    s_open = !s_open;
    s_page = MAIN;
    if (s_open)
    {
        // Opening the menu is using a cheat: unlocks from here on stay off the
        // leaderboards (pc/docs/achievements.md).
        pc_achievements_casual("debug");
    }
}

extern "C" void pc_debug_coords_toggle()
{
    s_showCoords = !s_showCoords;
}

extern "C" void pc_debug_coords_save()
{
    save_coords();
}

extern "C" void pc_debug_coords_return()
{
    if (s_hasSaved)
    {
        pc_debug_teleport(s_savedPos[0], s_savedPos[1], s_savedPos[2]);
        status("RETURNED TO SAVED POSITION");
    }
}

// A menu key; returns nonzero if the open menu took it.
extern "C" int pc_debug_menu_key(int key)
{
    if (!s_open)
    {
        return 0;
    }
    navigate((Key)key);
    return 1;
}

// Once per presented frame.
extern "C" void pc_debug_menu_frame()
{
    static bool started;
    if (!started)
    {
        started = true;
        s_showCoords = getenv("PC_COORDS") != NULL;
    }
    // Testing: PC_MENU=main|levels opens the menu; PC_WARP=<sec>:<scene> warps.
    static const char* menu = getenv("PC_MENU");
    if (menu)
    {
        s_open = true;
        s_page = strcmp(menu, "levels") ? MAIN : LEVELS;
        menu = NULL;
    }
    // Testing: PC_EVENT=<sec>:NAME:event[;NAME:event...] sends events to objects.
    static const char* ev = getenv("PC_EVENT");
    if (ev && pc_ticks_ms() >= (unsigned int)(atof(ev) * 1000))
    {
        std::string list = strchr(ev, ':') ? strchr(ev, ':') + 1 : "";
        ev = NULL;
        size_t start = 0;
        while (start < list.size())
        {
            size_t end = list.find(';', start);
            std::string item = list.substr(start, end == std::string::npos ? std::string::npos : end - start);
            size_t colon = item.rfind(':');
            if (colon != std::string::npos)
            {
                std::string name = item.substr(0, colon);
                unsigned int id = (unsigned int)atoi(item.c_str() + colon + 1);
                pc_log("[debug] event %u -> %s", id, name.c_str());
                pc_dbg_event(name.c_str(), id);
            }
            if (end == std::string::npos) break;
            start = end + 1;
        }
    }
    // Testing: PC_BOSSDMG=<sec>[,<sec>...] forces boss damage at those times.
    static const char* bossdmg = getenv("PC_BOSSDMG");
    if (bossdmg && pc_ticks_ms() >= (unsigned int)(atof(bossdmg) * 1000))
    {
        status(pc_dbg_force_boss_damage());
        bossdmg = strchr(bossdmg, ',');
        if (bossdmg) bossdmg++;
    }
    // Testing: PC_HURT=<sec>[,<sec>...] damages Scooby at those times.
    static const char* hurt = getenv("PC_HURT");
    if (hurt && pc_ticks_ms() >= (unsigned int)(atof(hurt) * 1000))
    {
        pc_dbg_hurt();
        hurt = strchr(hurt, ',');
        if (hurt) hurt++;
    }
    // Testing: PC_ENTINFO=<sec>:NAME[,<sec>:NAME] logs an entity's state.
    static const char* entinfo = getenv("PC_ENTINFO");
    if (entinfo && pc_ticks_ms() >= (unsigned int)(atof(entinfo) * 1000))
    {
        const char* c = strchr(entinfo, ':');
        const char* next = strchr(entinfo, ',');
        std::string name = c ? std::string(c + 1, next ? next : c + 1 + strlen(c + 1)) : "";
        pc_dbg_ent_info(name.c_str());
        entinfo = next ? next + 1 : NULL;
    }
    static const char* warp = getenv("PC_WARP");
    if (warp && pc_ticks_ms() >= (unsigned int)(atof(warp) * 1000))
    {
        const char* c = strchr(warp, ':');
        if (c && strlen(c + 1) == 4 &&
            pc_dbg_warp((unsigned int)c[1] << 24 | (unsigned int)c[2] << 16 | (unsigned int)c[3] << 8 | (unsigned char)c[4]))
        {
            warp = NULL;
        }
    }
    pc_dbg_frame();
    build_overlay();
}

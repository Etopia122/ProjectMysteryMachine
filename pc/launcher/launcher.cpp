// Project: Mystery Machine - the PC port of Scooby-Doo! Night of 100 Frights.
//
// Extracts the game's files from the player's own disc image (USA, GIHE78)
// into data/ beside the program, edits scooby.ini (display, graphics and
// control settings, read by pc/src/settings.cpp) and starts scooby.exe.
// Native Win32; SDL2 only to read controller buttons for remapping.
#define SDL_MAIN_HANDLED
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <wincodec.h>

#include <SDL.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

// ---------------------------------------------------------------- strings
static std::wstring widen(const std::string& s)
{
    if (s.empty())
    {
        return std::wstring();
    }
    int n = MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), NULL, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

// The game opens files with the ANSI API, so paths in scooby.ini are ANSI.
static std::string narrow(const std::wstring& w)
{
    if (w.empty())
    {
        return std::string();
    }
    int n = WideCharToMultiByte(CP_ACP, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
    std::string s(n, 0);
    WideCharToMultiByte(CP_ACP, 0, w.c_str(), (int)w.size(), &s[0], n, NULL, NULL);
    return s;
}

static std::string trim(const std::string& t)
{
    size_t a = t.find_first_not_of(" \t\r\n"), b = t.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? std::string() : t.substr(a, b - a + 1);
}

static std::wstring exe_dir()
{
    wchar_t path[MAX_PATH];
    DWORD n = GetModuleFileNameW(NULL, path, MAX_PATH);
    while (n > 0 && path[n - 1] != L'\\' && path[n - 1] != L'/')
    {
        n--;
    }
    return std::wstring(path, n);
}

static bool file_exists(const std::wstring& p)
{
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static bool dir_exists(const std::wstring& p)
{
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

// A folder holding the extracted disc: its root has boot.HIP and sd2.ini.
static bool is_game_dir(const std::wstring& d)
{
    std::wstring s = d;
    if (!s.empty() && s.back() != L'\\' && s.back() != L'/')
    {
        s += L'\\';
    }
    return file_exists(s + L"boot.HIP") && file_exists(s + L"sd2.ini");
}

// ---------------------------------------------------------------- settings
// scooby.ini as sections of key = value, kept in file order; unknown keys
// survive a save.
struct Ini
{
    std::vector<std::string> sections;
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> keys;

    std::string get(const std::string& sec, const std::string& key, const std::string& def) const
    {
        auto it = keys.find(sec);
        if (it != keys.end())
        {
            for (const auto& kv : it->second)
            {
                if (kv.first == key)
                {
                    return kv.second;
                }
            }
        }
        return def;
    }

    void set(const std::string& sec, const std::string& key, const std::string& value)
    {
        if (!keys.count(sec))
        {
            sections.push_back(sec);
        }
        auto& list = keys[sec];
        for (auto& kv : list)
        {
            if (kv.first == key)
            {
                kv.second = value;
                return;
            }
        }
        list.push_back(std::make_pair(key, value));
    }

    void load(const std::wstring& path)
    {
        FILE* f = _wfopen(path.c_str(), L"r");
        if (!f)
        {
            return;
        }
        char line[1024];
        std::string sec;
        while (fgets(line, sizeof(line), f))
        {
            std::string s = trim(line);
            if (s.empty() || s[0] == ';' || s[0] == '#')
            {
                continue;
            }
            if (s[0] == '[')
            {
                size_t close = s.find(']');
                sec = s.substr(1, close == std::string::npos ? std::string::npos : close - 1);
                for (char& c : sec)
                {
                    c = (char)tolower((unsigned char)c);
                }
                if (!keys.count(sec))
                {
                    sections.push_back(sec);
                    keys[sec];
                }
                continue;
            }
            size_t eq = s.find('=');
            if (eq == std::string::npos)
            {
                continue;
            }
            std::string k = trim(s.substr(0, eq));
            for (char& c : k)
            {
                c = (char)tolower((unsigned char)c);
            }
            set(sec, k, trim(s.substr(eq + 1)));
        }
        fclose(f);
    }

    bool save(const std::wstring& path) const
    {
        FILE* f = _wfopen(path.c_str(), L"w");
        if (!f)
        {
            return false;
        }
        fputs("; Project: Mystery Machine (Scooby-Doo! Night of 100 Frights) settings, written by the launcher.\n"
              "; Key names are SDL's; controller inputs are SDL game controller buttons or axes.\n",
              f);
        static const char* order[] = { "game", "display", "graphics", "keyboard", "controller", "debug" };
        std::vector<std::string> secs(order, order + 6);
        for (const std::string& s : sections)
        {
            if (std::find(secs.begin(), secs.end(), s) == secs.end())
            {
                secs.push_back(s);
            }
        }
        for (const std::string& s : secs)
        {
            auto it = keys.find(s);
            if (it == keys.end() || it->second.empty())
            {
                continue;
            }
            fprintf(f, "\n[%s]\n", s.c_str());
            for (const auto& kv : it->second)
            {
                fprintf(f, "%s = %s\n", kv.first.c_str(), kv.second.c_str());
            }
        }
        fclose(f);
        return true;
    }
};

// ---------------------------------------------------------------- controls
// The GameCube inputs, as pc/src/pad.cpp names them, with the same defaults.
struct Action
{
    const char* name;
    const wchar_t* label;
    const char* keys;
    const char* pad; // NULL: analog stick direction (the controller's stick)
};

static const Action kActions[] = {
    { "stick_up", L"Move forward", "W,Up", NULL },
    { "stick_down", L"Move back", "S,Down", NULL },
    { "stick_left", L"Move left", "A,Left", NULL },
    { "stick_right", L"Move right", "D,Right", NULL },
    { "cstick_up", L"Camera (C-stick) up", "I", NULL },
    { "cstick_down", L"Camera (C-stick) down", "K", NULL },
    { "cstick_left", L"Camera (C-stick) left", "J", NULL },
    { "cstick_right", L"Camera (C-stick) right", "L", NULL },
    { "a", L"A button", "Space", "a" },
    { "b", L"B button", "Left Shift,Right Shift", "b" },
    { "x", L"X button", "E", "x" },
    { "y", L"Y button", "R", "y" },
    { "start", L"Start (pause)", "Return", "start" },
    { "l", L"L trigger", "Q", "lefttrigger" },
    { "r", L"R trigger", "Tab", "righttrigger" },
    { "z", L"Z button", "F", "rightshoulder" },
    { "dpad_up", L"D-pad up", "Keypad 8", "dpup" },
    { "dpad_down", L"D-pad down", "Keypad 2", "dpdown" },
    { "dpad_left", L"D-pad left", "Keypad 4", "dpleft" },
    { "dpad_right", L"D-pad right", "Keypad 6", "dpright" },
};
static const int kNumActions = sizeof(kActions) / sizeof(kActions[0]);

// Windows keyboard scan codes (set 1; 0x100 = extended) to SDL's.
static SDL_Scancode sdl_scancode(int scan)
{
    static const struct { int scan; SDL_Scancode sdl; } map[] = {
        { 0x01, SDL_SCANCODE_ESCAPE }, { 0x02, SDL_SCANCODE_1 }, { 0x03, SDL_SCANCODE_2 }, { 0x04, SDL_SCANCODE_3 },
        { 0x05, SDL_SCANCODE_4 }, { 0x06, SDL_SCANCODE_5 }, { 0x07, SDL_SCANCODE_6 }, { 0x08, SDL_SCANCODE_7 },
        { 0x09, SDL_SCANCODE_8 }, { 0x0A, SDL_SCANCODE_9 }, { 0x0B, SDL_SCANCODE_0 }, { 0x0C, SDL_SCANCODE_MINUS },
        { 0x0D, SDL_SCANCODE_EQUALS }, { 0x0E, SDL_SCANCODE_BACKSPACE }, { 0x0F, SDL_SCANCODE_TAB },
        { 0x10, SDL_SCANCODE_Q }, { 0x11, SDL_SCANCODE_W }, { 0x12, SDL_SCANCODE_E }, { 0x13, SDL_SCANCODE_R },
        { 0x14, SDL_SCANCODE_T }, { 0x15, SDL_SCANCODE_Y }, { 0x16, SDL_SCANCODE_U }, { 0x17, SDL_SCANCODE_I },
        { 0x18, SDL_SCANCODE_O }, { 0x19, SDL_SCANCODE_P }, { 0x1A, SDL_SCANCODE_LEFTBRACKET },
        { 0x1B, SDL_SCANCODE_RIGHTBRACKET }, { 0x1C, SDL_SCANCODE_RETURN }, { 0x1D, SDL_SCANCODE_LCTRL },
        { 0x1E, SDL_SCANCODE_A }, { 0x1F, SDL_SCANCODE_S }, { 0x20, SDL_SCANCODE_D }, { 0x21, SDL_SCANCODE_F },
        { 0x22, SDL_SCANCODE_G }, { 0x23, SDL_SCANCODE_H }, { 0x24, SDL_SCANCODE_J }, { 0x25, SDL_SCANCODE_K },
        { 0x26, SDL_SCANCODE_L }, { 0x27, SDL_SCANCODE_SEMICOLON }, { 0x28, SDL_SCANCODE_APOSTROPHE },
        { 0x29, SDL_SCANCODE_GRAVE }, { 0x2A, SDL_SCANCODE_LSHIFT }, { 0x2B, SDL_SCANCODE_BACKSLASH },
        { 0x2C, SDL_SCANCODE_Z }, { 0x2D, SDL_SCANCODE_X }, { 0x2E, SDL_SCANCODE_C }, { 0x2F, SDL_SCANCODE_V },
        { 0x30, SDL_SCANCODE_B }, { 0x31, SDL_SCANCODE_N }, { 0x32, SDL_SCANCODE_M }, { 0x33, SDL_SCANCODE_COMMA },
        { 0x34, SDL_SCANCODE_PERIOD }, { 0x35, SDL_SCANCODE_SLASH }, { 0x36, SDL_SCANCODE_RSHIFT },
        { 0x37, SDL_SCANCODE_KP_MULTIPLY }, { 0x38, SDL_SCANCODE_LALT }, { 0x39, SDL_SCANCODE_SPACE },
        { 0x3A, SDL_SCANCODE_CAPSLOCK }, { 0x3B, SDL_SCANCODE_F1 }, { 0x3C, SDL_SCANCODE_F2 },
        { 0x3D, SDL_SCANCODE_F3 }, { 0x3E, SDL_SCANCODE_F4 }, { 0x3F, SDL_SCANCODE_F5 }, { 0x40, SDL_SCANCODE_F6 },
        { 0x41, SDL_SCANCODE_F7 }, { 0x42, SDL_SCANCODE_F8 }, { 0x43, SDL_SCANCODE_F9 }, { 0x44, SDL_SCANCODE_F10 },
        { 0x45, SDL_SCANCODE_NUMLOCKCLEAR }, { 0x46, SDL_SCANCODE_SCROLLLOCK }, { 0x47, SDL_SCANCODE_KP_7 },
        { 0x48, SDL_SCANCODE_KP_8 }, { 0x49, SDL_SCANCODE_KP_9 }, { 0x4A, SDL_SCANCODE_KP_MINUS },
        { 0x4B, SDL_SCANCODE_KP_4 }, { 0x4C, SDL_SCANCODE_KP_5 }, { 0x4D, SDL_SCANCODE_KP_6 },
        { 0x4E, SDL_SCANCODE_KP_PLUS }, { 0x4F, SDL_SCANCODE_KP_1 }, { 0x50, SDL_SCANCODE_KP_2 },
        { 0x51, SDL_SCANCODE_KP_3 }, { 0x52, SDL_SCANCODE_KP_0 }, { 0x53, SDL_SCANCODE_KP_PERIOD },
        { 0x56, SDL_SCANCODE_NONUSBACKSLASH }, { 0x57, SDL_SCANCODE_F11 }, { 0x58, SDL_SCANCODE_F12 },
        { 0x11C, SDL_SCANCODE_KP_ENTER }, { 0x11D, SDL_SCANCODE_RCTRL }, { 0x135, SDL_SCANCODE_KP_DIVIDE },
        { 0x138, SDL_SCANCODE_RALT }, { 0x147, SDL_SCANCODE_HOME }, { 0x148, SDL_SCANCODE_UP },
        { 0x149, SDL_SCANCODE_PAGEUP }, { 0x14B, SDL_SCANCODE_LEFT }, { 0x14D, SDL_SCANCODE_RIGHT },
        { 0x14F, SDL_SCANCODE_END }, { 0x150, SDL_SCANCODE_DOWN }, { 0x151, SDL_SCANCODE_PAGEDOWN },
        { 0x152, SDL_SCANCODE_INSERT }, { 0x153, SDL_SCANCODE_DELETE }, { 0x15B, SDL_SCANCODE_LGUI },
        { 0x15C, SDL_SCANCODE_RGUI }, { 0x15D, SDL_SCANCODE_APPLICATION },
    };
    for (const auto& m : map)
    {
        if (m.scan == scan)
        {
            return m.sdl;
        }
    }
    return SDL_SCANCODE_UNKNOWN;
}

// ---------------------------------------------------------------- disc extraction
enum { WM_EXTRACT_PROGRESS = WM_APP + 1, WM_EXTRACT_DONE };

struct ExtractJob
{
    std::wstring iso, out;
    HWND notify;
};

static unsigned int be32(const unsigned char* p) { return (unsigned)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]; }

static void delete_tree(const std::wstring& dir)
{
    if (!dir_exists(dir))
    {
        return;
    }
    std::wstring from = dir + L'\0'; // double-null terminated
    SHFILEOPSTRUCTW op = {};
    op.wFunc = FO_DELETE;
    op.pFrom = from.c_str();
    op.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    SHFileOperationW(&op);
}

static bool make_dirs(const std::wstring& path)
{
    if (path.empty() || dir_exists(path))
    {
        return true;
    }
    size_t slash = path.find_last_of(L"\\/");
    if (slash != std::wstring::npos && !make_dirs(path.substr(0, slash)))
    {
        return false;
    }
    return CreateDirectoryW(path.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

// Checks the image: a plain GameCube disc of the USA release. "" when fine.
static std::wstring check_disc(FILE* f)
{
    unsigned char h[0x440];
    if (fread(h, 1, sizeof(h), f) != sizeof(h))
    {
        return L"This file is too small to be a GameCube disc image.";
    }
    if (!memcmp(h, "RVZ\x01", 4) || !memcmp(h, "WIA\x01", 4) || !memcmp(h, "CISO", 4) || be32(h) == 0x01C00BB1)
    {
        return L"This disc image is compressed (RVZ, WIA, CISO or GCZ). Convert it to a plain .iso first: in "
               L"Dolphin, right-click the game, Convert File..., format ISO.";
    }
    if (be32(h + 0x1C) != 0xC2339F3D)
    {
        return L"This file is not a GameCube disc image.";
    }
    if (memcmp(h, "GIH", 3) || memcmp(h + 4, "78", 2))
    {
        return L"This is a disc of another game. The launcher needs Scooby-Doo! Night of 100 Frights.";
    }
    if (h[3] != 'E')
    {
        return L"This is the European or Japanese release of the game; the PC port is made from the USA release "
               L"(disc ID GIHE78) and needs its files.";
    }
    return L"";
}

// Copies the disc's files into job->out (through job->out + ".partial", so a
// stopped extraction leaves no half folder); "" when it worked. Progress goes
// to job->notify as permille.
static std::wstring extract_disc(const ExtractJob* job)
{
    std::wstring err;
    std::wstring tmp = job->out + L".partial";
    FILE* f = _wfopen(job->iso.c_str(), L"rb");
    std::vector<unsigned char> fst;
    unsigned int n = 0;
    if (!f)
    {
        err = L"Can't open the disc image.";
    }
    else
    {
        err = check_disc(f);
    }
    if (err.empty())
    {
        unsigned char b[8];
        fseek(f, 0x424, SEEK_SET);
        fread(b, 1, 8, f);
        unsigned int off = be32(b), size = be32(b + 4);
        fst.resize(size);
        if (size < 12 || _fseeki64(f, off, SEEK_SET) != 0 || fread(fst.data(), 1, size, f) != size)
        {
            err = L"The disc image's file table can't be read; the image may be incomplete.";
        }
        else
        {
            n = be32(&fst[8]);
            if ((unsigned long long)n * 12 > size)
            {
                err = L"The disc image's file table is damaged.";
            }
        }
    }
    unsigned long long total = 0, done = 0;
    if (err.empty())
    {
        for (unsigned int i = 1; i < n; i++)
        {
            if (!(fst[i * 12] & 1))
            {
                total += be32(&fst[i * 12 + 8]);
            }
        }
        delete_tree(tmp);
        if (!make_dirs(tmp))
        {
            err = L"Can't create the folder " + tmp + L".";
        }
    }
    if (err.empty())
    {
        // Walk the file table: directories end at their 'next' index.
        std::vector<std::pair<unsigned int, std::wstring>> stack;
        stack.push_back(std::make_pair(n, tmp + L"\\"));
        std::vector<unsigned char> buf(1 << 20);
        int lastPermille = -1;
        size_t strt = (size_t)n * 12;
        for (unsigned int i = 1; i < n && err.empty(); i++)
        {
            while (stack.size() > 1 && i >= stack.back().first)
            {
                stack.pop_back();
            }
            const unsigned char* e = &fst[i * 12];
            size_t nameOff = strt + (be32(e) & 0xFFFFFF);
            std::string raw;
            while (nameOff < fst.size() && fst[nameOff])
            {
                raw += (char)fst[nameOff++];
            }
            int wn = MultiByteToWideChar(932, 0, raw.c_str(), (int)raw.size(), NULL, 0);
            std::wstring name(wn, 0);
            MultiByteToWideChar(932, 0, raw.c_str(), (int)raw.size(), &name[0], wn);
            std::wstring path = stack.back().second + name;
            if (e[0] & 1)
            {
                if (!make_dirs(path))
                {
                    err = L"Can't create the folder " + path + L".";
                }
                stack.push_back(std::make_pair(be32(e + 8), path + L"\\"));
                continue;
            }
            unsigned int pos = be32(e + 4), left = be32(e + 8);
            FILE* o = _wfopen(path.c_str(), L"wb");
            if (!o || _fseeki64(f, pos, SEEK_SET) != 0)
            {
                err = L"Can't write " + path + L".";
                if (o)
                {
                    fclose(o);
                }
                break;
            }
            while (left)
            {
                size_t chunk = left < buf.size() ? left : buf.size();
                if (fread(buf.data(), 1, chunk, f) != chunk)
                {
                    err = L"The disc image ends early; it may be an incomplete dump.";
                    break;
                }
                if (fwrite(buf.data(), 1, chunk, o) != chunk)
                {
                    err = L"Writing " + path + L" failed (is the disk full?).";
                    break;
                }
                left -= (unsigned int)chunk;
                done += chunk;
                int permille = total ? (int)(done * 1000 / total) : 0;
                if (permille != lastPermille && job->notify)
                {
                    lastPermille = permille;
                    PostMessageW(job->notify, WM_EXTRACT_PROGRESS, permille, 0);
                }
            }
            fclose(o);
        }
    }
    if (f)
    {
        fclose(f);
    }
    if (err.empty())
    {
        // Swap the finished folder in for any older one.
        delete_tree(job->out);
        if (!MoveFileW(tmp.c_str(), job->out.c_str()))
        {
            err = L"Can't rename " + tmp + L" to " + job->out + L".";
        }
        else if (!is_game_dir(job->out))
        {
            err = L"The files were copied, but boot.HIP or sd2.ini is missing from them.";
        }
    }
    else
    {
        delete_tree(tmp);
    }
    return err;
}

static DWORD WINAPI extract_thread(void* param)
{
    ExtractJob* job = (ExtractJob*)param;
    std::wstring err = extract_disc(job);
    PostMessageW(job->notify, WM_EXTRACT_DONE, err.empty(), (LPARAM) new std::wstring(err));
    delete job;
    return 0;
}

// ---------------------------------------------------------------- window
enum
{
    ID_NAV_GAME = 100,
    ID_NAV_DISPLAY,
    ID_NAV_GRAPHICS,
    ID_NAV_CONTROLS,
    // Game files
    ID_GAME_INFO, ID_ISO_LABEL, ID_ISO_PATH, ID_ISO_BROWSE, ID_EXTRACT, ID_PROGRESS, ID_GAME_STATUS, ID_GAME_FOLDER, ID_SKIP_LOGOS, ID_GAME_NOTE, ID_LOGS,
    // Display
    ID_RES_LABEL, ID_RES, ID_MODE_LABEL, ID_MODE, ID_ASPECT_LABEL, ID_ASPECT, ID_VSYNC, ID_MSAA_LABEL, ID_MSAA,
    ID_DISPLAY_NOTE, ID_HUD_LABEL, ID_HUD,
    // Graphics
    ID_DRAW_LABEL, ID_DRAW, ID_DRAW_VALUE, ID_FX_LABEL, ID_FX, ID_FX_VALUE, ID_FOV_LABEL, ID_FOV, ID_FOV_VALUE,
    ID_BRIGHT_LABEL, ID_BRIGHT, ID_BRIGHT_VALUE, ID_FILTER_LABEL, ID_FILTER, ID_SHADOW_LABEL, ID_SHADOW, ID_FPS_LABEL,
    ID_FPS, ID_GRAPHICS_NOTE,
    // Controls
    ID_BINDS, ID_SET_KEY, ID_ADD_KEY, ID_SET_PAD, ID_CLEAR, ID_DEFAULTS, ID_SWAP_STICKS, ID_CAPTURE, ID_DEBUG_MENU,
    // Bottom
    ID_STATUS, ID_SAVE, ID_PLAY,
};

enum { PAGE_GAME, PAGE_DISPLAY, PAGE_GRAPHICS, PAGE_CONTROLS, PAGE_COUNT };

static const int kNavIds[PAGE_COUNT] = { ID_NAV_GAME, ID_NAV_DISPLAY, ID_NAV_GRAPHICS, ID_NAV_CONTROLS };

struct Control
{
    int id, page;
    HWND hwnd;
};

static HWND s_main;
static HFONT s_font;
static HFONT s_titleFont, s_pageFont, s_eyebrowFont, s_navFont, s_smallFont;
static int s_dpi = 96;
static std::vector<Control> s_controls;
static Ini s_ini;
static std::wstring s_dir, s_iniPath;
static bool s_extracting;
static std::vector<std::pair<int, int>> s_resolutions;
static int s_page = PAGE_GAME;
static int s_hotButton;
static HBRUSH s_cardBrush, s_fieldBrush, s_footerBrush;

// The launcher's palette: a plum night sky, parchment cards, Mystery Machine
// teal and orange. The chrome is drawn; only the wordmark and mascot are
// bitmaps, and both are compiled into the exe.
static const COLORREF kBg = RGB(18, 15, 29);
static const COLORREF kHeaderA = RGB(18, 58, 58);
static const COLORREF kHeaderB = RGB(46, 24, 70);
static const COLORREF kSidebar = RGB(31, 24, 47);
static const COLORREF kSidebarEdge = RGB(58, 45, 82);
static const COLORREF kCard = RGB(247, 240, 226);
static const COLORREF kField = RGB(255, 251, 243);
static const COLORREF kLine = RGB(218, 203, 178);
static const COLORREF kInk = RGB(37, 30, 48);
static const COLORREF kMuted = RGB(104, 91, 79);
static const COLORREF kAccent = RGB(241, 146, 44);
static const COLORREF kAccentDark = RGB(199, 107, 20);
static const COLORREF kTeal = RGB(46, 166, 145);
static const COLORREF kTrack = RGB(223, 210, 189);
static const COLORREF kFooterText = RGB(213, 201, 184);

// Remapping state: the row being captured, and how.
enum { CAPTURE_NONE, CAPTURE_SET_KEY, CAPTURE_ADD_KEY, CAPTURE_PAD };
static int s_capture = CAPTURE_NONE, s_captureRow = -1;
static SDL_GameController* s_pad;
static std::vector<std::string> s_keys(kNumActions), s_pads(kNumActions);
static Uint8 s_padBase[SDL_CONTROLLER_BUTTON_MAX];

static int px(int v) { return MulDiv(v, s_dpi, 96); }

static HFONT make_font(int points, int weight)
{
    return CreateFontW(-MulDiv(points, s_dpi, 72), 0, 0, 0, weight, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                       DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

static HWND item(int id)
{
    for (const Control& c : s_controls)
    {
        if (c.id == id)
        {
            return c.hwnd;
        }
    }
    return NULL;
}

static LRESULT CALLBACK hot_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR);

static HWND add(int page, int id, const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y, int w, int h,
                DWORD ex = 0)
{
    // Buttons and checkboxes are owner-drawn. An owner-drawn checkbox no longer
    // toggles itself, so WM_COMMAND flips those four check states by hand.
    // Only real buttons are converted: BS_PUSHBUTTON is 0, so testing the style
    // alone also caught statics added with style 0 (SS_LEFT) and turned them
    // into SS_SIMPLE labels, which neither wrap nor use the card background.
    DWORD type = style & 0xF;
    if (!wcscmp(cls, L"BUTTON") &&
        (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON || type == BS_AUTOCHECKBOX))
    {
        style = (style & ~0xF) | BS_OWNERDRAW;
    }
    HWND hwnd = CreateWindowExW(ex, cls, text, WS_CHILD | WS_CLIPSIBLINGS | style, px(x), px(y), px(w), px(h),
                                s_main, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    SendMessageW(hwnd, WM_SETFONT, (WPARAM)s_font, TRUE);
    if ((style & 0xF) == BS_OWNERDRAW)
    {
        SetWindowSubclass(hwnd, hot_proc, (UINT_PTR)id, 0);
    }
    Control c = { id, page, hwnd };
    s_controls.push_back(c);
    return hwnd;
}

static HBRUSH brush_for(HBRUSH* slot, COLORREF color)
{
    if (!*slot)
    {
        *slot = CreateSolidBrush(color);
    }
    return *slot;
}
static HBRUSH card_brush() { return brush_for(&s_cardBrush, kCard); }
static HBRUSH field_brush() { return brush_for(&s_fieldBrush, kField); }
static HBRUSH footer_brush() { return brush_for(&s_footerBrush, kBg); }

static void set_text(int id, const std::wstring& t) { SetWindowTextW(item(id), t.c_str()); }

static void status(const std::wstring& t) { set_text(ID_STATUS, t); }

// The game writes scooby.log and crash-*.log to
// %APPDATA%\ProjectMysteryMachine\logs (pc/src/main.cpp); this opens the same
// folder, creating it first so the button always shows somewhere.
static std::wstring log_dir()
{
    wchar_t appdata[MAX_PATH];
    if (GetEnvironmentVariableW(L"APPDATA", appdata, MAX_PATH) && appdata[0])
    {
        return std::wstring(appdata) + L"\\ProjectMysteryMachine\\logs";
    }
    return exe_dir() + L"logs";
}

static void open_log_folder()
{
    std::wstring dir = log_dir();
    std::wstring parent = dir.substr(0, dir.find_last_of(L'\\'));
    CreateDirectoryW(parent.c_str(), NULL);
    CreateDirectoryW(dir.c_str(), NULL);
    if ((INT_PTR)ShellExecuteW(s_main, L"open", dir.c_str(), NULL, NULL, SW_SHOWNORMAL) <= 32)
    {
        MessageBoxW(s_main, (L"Can't open " + dir + L".").c_str(), L"Logs", MB_ICONERROR);
    }
}

static void show_page(int page)
{
    if (page < 0 || page >= PAGE_COUNT)
    {
        page = PAGE_GAME;
    }
    s_page = page;
    for (const Control& c : s_controls)
    {
        if (c.page >= 0)
        {
            ShowWindow(c.hwnd, c.page == page ? SW_SHOW : SW_HIDE);
        }
    }
    for (int i = 0; i < PAGE_COUNT; i++)
    {
        InvalidateRect(item(kNavIds[i]), NULL, TRUE);
    }
    InvalidateRect(s_main, NULL, FALSE);
}

static int combo_add(int id, const wchar_t* text)
{
    return (int)SendMessageW(item(id), CB_ADDSTRING, 0, (LPARAM)text);
}
static int combo_get(int id) { return (int)SendMessageW(item(id), CB_GETCURSEL, 0, 0); }
static void combo_set(int id, int i) { SendMessageW(item(id), CB_SETCURSEL, i, 0); }
// Owner-drawn buttons do not keep BM_SETCHECK state, so the four toggles live
// here. load_settings fills them and save_settings reads them.
static bool s_checkVsync = true, s_checkSkipLogos, s_checkSwapSticks, s_checkDebugMenu;

static bool& check_state(int id)
{
    switch (id)
    {
    case ID_VSYNC: return s_checkVsync;
    case ID_SKIP_LOGOS: return s_checkSkipLogos;
    case ID_SWAP_STICKS: return s_checkSwapSticks;
    case ID_DEBUG_MENU: return s_checkDebugMenu;
    }
    static bool unused;
    return unused;
}

static bool check_get(int id) { return check_state(id); }
static void check_set(int id, bool on)
{
    check_state(id) = on;
    InvalidateRect(item(id), NULL, TRUE);
}

// ---------------------------------------------------------------- theme drawing
static bool is_checkbox_id(int id)
{
    return id == ID_VSYNC || id == ID_SKIP_LOGOS || id == ID_SWAP_STICKS || id == ID_DEBUG_MENU;
}

static bool is_slider_id(int id)
{
    return id == ID_DRAW || id == ID_FX || id == ID_FOV || id == ID_BRIGHT;
}

static bool is_note_id(int id)
{
    return id == ID_GAME_INFO || id == ID_GAME_NOTE || id == ID_GAME_STATUS || id == ID_DISPLAY_NOTE ||
           id == ID_GRAPHICS_NOTE || id == ID_CAPTURE;
}

static void fill_round(HDC dc, RECT r, int radius, COLORREF fill, COLORREF border)
{
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

static void draw_line(HDC dc, int x1, int y1, int x2, int y2, COLORREF color, int width = 1)
{
    HPEN pen = CreatePen(PS_SOLID, width, color);
    HGDIOBJ old = SelectObject(dc, pen);
    MoveToEx(dc, x1, y1, NULL);
    LineTo(dc, x2, y2);
    SelectObject(dc, old);
    DeleteObject(pen);
}

static void draw_text(HDC dc, const wchar_t* text, RECT r, HFONT font, COLORREF color,
                      UINT flags = DT_LEFT | DT_VCENTER | DT_SINGLELINE)
{
    HGDIOBJ old = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextW(dc, text, -1, &r, flags);
    SelectObject(dc, old);
}

// ------------------------------------------------------------- launcher art
// The wordmark and the Great Dane badge are PNG resources compiled into the
// exe and decoded with WIC, so the launcher is still a single portable exe.
enum { IDR_WORDMARK_PNG = 2, IDR_MASCOT_PNG = 3, IDR_VAN_PNG = 4 };

struct Image
{
    HBITMAP handle;
    int w, h;
};

static Image load_png_image(int id)
{
    Image image = { NULL, 0, 0 };
    HRSRC res = FindResourceW(NULL, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!res)
    {
        return image;
    }
    DWORD size = SizeofResource(NULL, res);
    const void* data = LockResource(LoadResource(NULL, res));
    if (!data || !size)
    {
        return image;
    }

    IWICImagingFactory* factory = NULL;
    IWICStream* stream = NULL;
    IWICBitmapDecoder* decoder = NULL;
    IWICBitmapFrameDecode* frame = NULL;
    IWICFormatConverter* converter = NULL;
    UINT w = 0, h = 0;

    if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                                   IID_IWICImagingFactory, (void**)&factory)) &&
        SUCCEEDED(factory->CreateStream(&stream)) &&
        SUCCEEDED(stream->InitializeFromMemory((BYTE*)data, size)) &&
        SUCCEEDED(factory->CreateDecoderFromStream(stream, NULL, WICDecodeMetadataCacheOnLoad,
                                                   &decoder)) &&
        SUCCEEDED(decoder->GetFrame(0, &frame)) &&
        SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
        SUCCEEDED(converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
                                       NULL, 0.0, WICBitmapPaletteTypeCustom)) &&
        SUCCEEDED(converter->GetSize(&w, &h)))
    {
        BITMAPINFO info = {};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = (LONG)w;
        info.bmiHeader.biHeight = -(LONG)h;  // top-down, matching the PNG
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        void* bits = NULL;
        HDC screen = GetDC(NULL);
        HBITMAP bitmap = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &bits, NULL, 0);
        ReleaseDC(NULL, screen);
        if (bitmap && bits &&
            SUCCEEDED(converter->CopyPixels(NULL, w * 4, w * 4 * h, (BYTE*)bits)))
        {
            image.handle = bitmap;
            image.w = (int)w;
            image.h = (int)h;
        }
        else if (bitmap)
        {
            DeleteObject(bitmap);
        }
    }

    if (converter) converter->Release();
    if (frame) frame->Release();
    if (decoder) decoder->Release();
    if (stream) stream->Release();
    if (factory) factory->Release();
    return image;
}

// Alpha-blend a loaded image, optionally clipped to rounded corners (the same
// radius convention as fill_round).
static void draw_image(HDC dc, const Image& image, RECT r, int radius)
{
    if (!image.handle)
    {
        return;
    }
    HDC src = CreateCompatibleDC(dc);
    HGDIOBJ old = SelectObject(src, image.handle);
    int saved = SaveDC(dc);
    HRGN clip = NULL;
    if (radius > 0)
    {
        clip = CreateRoundRectRgn(r.left, r.top, r.right + 1, r.bottom + 1, radius * 2, radius * 2);
        SelectClipRgn(dc, clip);
    }
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    AlphaBlend(dc, r.left, r.top, r.right - r.left, r.bottom - r.top, src, 0, 0, image.w, image.h,
               blend);
    RestoreDC(dc, saved);
    if (clip)
    {
        DeleteObject(clip);
    }
    SelectObject(src, old);
    DeleteDC(src);
}

static void draw_paw(HDC dc, int cx, int cy, int unit, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    Ellipse(dc, cx - unit * 3 / 2, cy - unit / 2, cx + unit * 3 / 2, cy + unit * 3 / 2);
    Ellipse(dc, cx - unit * 5 / 2, cy - unit * 3 / 2, cx - unit * 3 / 2, cy - unit / 2);
    Ellipse(dc, cx - unit * 5 / 4, cy - unit * 2, cx - unit / 4, cy - unit);
    Ellipse(dc, cx + unit / 4, cy - unit * 2, cx + unit * 5 / 4, cy - unit);
    Ellipse(dc, cx + unit * 3 / 2, cy - unit * 3 / 2, cx + unit * 5 / 2, cy - unit / 2);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

static void draw_van(HDC dc, int x, int y, int w, int h)
{
    int bodyTop = y + h / 3;
    RECT body = { x, bodyTop, x + w, y + h - 5 };
    fill_round(dc, body, h / 4, kTeal, RGB(28, 116, 101));
    RECT roof = { x + w / 6, y + 4, x + w - w / 8, bodyTop + 4 };
    fill_round(dc, roof, h / 4, kTeal, RGB(28, 116, 101));
    RECT stripe = { x + 5, y + h / 2, x + w - 5, y + h / 2 + 4 };
    HBRUSH orange = CreateSolidBrush(kAccent);
    FillRect(dc, &stripe, orange);
    DeleteObject(orange);
    RECT window = { x + w / 5, y + 9, x + w / 2, bodyTop - 2 };
    fill_round(dc, window, 3, RGB(43, 58, 74), RGB(43, 58, 74));
    HBRUSH wheel = CreateSolidBrush(RGB(27, 24, 33));
    HPEN wheelPen = CreatePen(PS_SOLID, 1, RGB(27, 24, 33));
    HGDIOBJ ob = SelectObject(dc, wheel), op = SelectObject(dc, wheelPen);
    Ellipse(dc, x + w / 5, y + h - 9, x + w / 5 + 14, y + h + 5);
    Ellipse(dc, x + w - w / 3, y + h - 9, x + w - w / 3 + 14, y + h + 5);
    SelectObject(dc, ob);
    SelectObject(dc, op);
    DeleteObject(wheel);
    DeleteObject(wheelPen);
}

// The hand-drawn Great Dane: only used if the mascot PNG cannot be loaded.
static void draw_drawn_dog(HDC dc, int x, int y, int s)
{
    RECT outer = { x, y, x + s, y + s };
    fill_round(dc, outer, s / 5, RGB(20, 52, 52), kAccent);
    RECT inner = { x + s / 14, y + s / 14, x + s - s / 14, y + s - s / 14 };
    fill_round(dc, inner, s / 6, RGB(28, 70, 67), RGB(238, 190, 112));

    auto ellipse = [&](COLORREF fill, COLORREF border, int l, int t, int r, int b) {
        HBRUSH brush = CreateSolidBrush(fill);
        HPEN pen = CreatePen(PS_SOLID, 1, border);
        HGDIOBJ oldBrush = SelectObject(dc, brush);
        HGDIOBJ oldPen = SelectObject(dc, pen);
        Ellipse(dc, x + s * l / 100, y + s * t / 100, x + s * r / 100, y + s * b / 100);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);
    };

    // Floppy ears and the long Great Dane head.
    ellipse(RGB(75, 44, 28), RGB(75, 44, 28), 14, 24, 39, 84);
    ellipse(RGB(75, 44, 28), RGB(75, 44, 28), 61, 24, 86, 84);
    ellipse(RGB(166, 104, 58), RGB(111, 66, 36), 24, 17, 76, 79);
    // Two darker patches.
    ellipse(RGB(92, 55, 34), RGB(92, 55, 34), 28, 24, 41, 35);
    ellipse(RGB(92, 55, 34), RGB(92, 55, 34), 59, 22, 72, 33);
    // A narrower, longer muzzle and nose.
    ellipse(RGB(218, 163, 113), RGB(158, 99, 57), 37, 46, 63, 84);
    ellipse(RGB(31, 26, 24), RGB(31, 26, 24), 44, 58, 56, 68);
    // Eyes: white, then dark pupils.
    ellipse(RGB(252, 249, 238), RGB(91, 59, 38), 35, 39, 47, 52);
    ellipse(RGB(252, 249, 238), RGB(91, 59, 38), 53, 39, 65, 52);
    ellipse(RGB(26, 22, 21), RGB(26, 22, 21), 39, 44, 44, 50);
    ellipse(RGB(26, 22, 21), RGB(26, 22, 21), 56, 44, 61, 50);
    // A small mouth line under the nose, then the collar over the neck.
    draw_line(dc, x + s * 50 / 100, y + s * 68 / 100, x + s * 50 / 100, y + s * 75 / 100,
              RGB(100, 59, 34), s > 48 ? 2 : 1);
    RECT collar = { x + s * 27 / 100, y + s * 72 / 100, x + s * 73 / 100, y + s * 82 / 100 };
    fill_round(dc, collar, s / 30, RGB(43, 139, 202), RGB(28, 98, 150));
    ellipse(RGB(246, 197, 67), RGB(183, 132, 28), 45, 77, 55, 87);
}

// The header badge: the Great Dane portrait inside the orange-ringed plate.
static void draw_dog_logo(HDC dc, int x, int y, int s, const Image& mascot)
{
    if (!mascot.handle)
    {
        draw_drawn_dog(dc, x, y, s);
        return;
    }
    RECT outer = { x, y, x + s, y + s };
    fill_round(dc, outer, s / 5, kAccent, kAccent);
    int rim = (s * 37 + 500) / 1000;
    if (rim < 2)
    {
        rim = 2;
    }
    RECT inner = { x + rim, y + rim, x + s - rim, y + s - rim };
    fill_round(dc, inner, s / 6, RGB(20, 52, 52), RGB(20, 52, 52));
    int pad = (s * 6 + 50) / 100;
    RECT hole = { x + pad, y + pad, x + s - pad, y + s - pad };
    fill_round(dc, hole, s / 9, RGB(28, 70, 67), RGB(28, 70, 67));
    // Keep the portrait square and let it sit on the plate's floor, so the
    // straight bottom of the crop reads as the frame edge rather than a cut.
    int air = (s * 3 + 50) / 100;
    int side = (hole.right - hole.left) - 2 * air;
    RECT art = { hole.left + air, hole.bottom - side, hole.right - air, hole.bottom };
    draw_image(dc, mascot, art, s / 9);
}

static void draw_nav_icon(HDC dc, int page, int x, int y, COLORREF color)
{
    HPEN pen = CreatePen(PS_SOLID, 2, color);
    HBRUSH brush = CreateSolidBrush(color);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    switch (page)
    {
    case PAGE_GAME:
        SelectObject(dc, GetStockObject(NULL_BRUSH));
        Ellipse(dc, x + 1, y + 1, x + 19, y + 19);
        Ellipse(dc, x + 7, y + 7, x + 13, y + 13);
        break;
    case PAGE_DISPLAY:
        SelectObject(dc, GetStockObject(NULL_BRUSH));
        RoundRect(dc, x + 1, y + 2, x + 19, y + 15, 2, 2);
        draw_line(dc, x + 10, y + 15, x + 10, y + 19, color, 2);
        draw_line(dc, x + 6, y + 19, x + 14, y + 19, color, 2);
        break;
    case PAGE_GRAPHICS:
        for (int i = 0; i < 3; i++)
        {
            int yy = y + 4 + i * 6;
            draw_line(dc, x + 1, yy, x + 19, yy, color, 2);
            int kx = x + 5 + (i == 1 ? 8 : 3);
            Ellipse(dc, kx - 2, yy - 3, kx + 2, yy + 3);
        }
        break;
    default:
        RoundRect(dc, x + 1, y + 4, x + 19, y + 17, 6, 6);
        SelectObject(dc, GetStockObject(NULL_BRUSH));
        draw_line(dc, x + 6, y + 10, x + 10, y + 10, color, 2);
        draw_line(dc, x + 8, y + 8, x + 8, y + 12, color, 2);
        Ellipse(dc, x + 13, y + 8, x + 15, y + 10);
        Ellipse(dc, x + 15, y + 11, x + 17, y + 13);
        break;
    }
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

static void draw_background(HDC dc, int w, int h)
{
    RECT all = { 0, 0, w, h };
    HBRUSH bg = CreateSolidBrush(kBg);
    FillRect(dc, &all, bg);
    DeleteObject(bg);

    // Header: a deep teal-to-plum night sky.
    TRIVERTEX v[2] = {
        { 0, 0, (COLOR16)(GetRValue(kHeaderA) << 8), (COLOR16)(GetGValue(kHeaderA) << 8),
          (COLOR16)(GetBValue(kHeaderA) << 8), 0xFF00 },
        { w, px(112), (COLOR16)(GetRValue(kHeaderB) << 8), (COLOR16)(GetGValue(kHeaderB) << 8),
          (COLOR16)(GetBValue(kHeaderB) << 8), 0xFF00 },
    };
    GRADIENT_RECT gradient = { 0, 1 };
    GradientFill(dc, v, 2, &gradient, 1, GRADIENT_FILL_RECT_H);

    // A few low-contrast paw prints, the Great Dane badge and the Mystery Machine.
    draw_paw(dc, px(704), px(28), px(9), RGB(35, 40, 51));
    draw_paw(dc, px(732), px(97), px(5), RGB(40, 37, 57));
    static const Image van = load_png_image(IDR_VAN_PNG);
    if (van.handle)
    {
        int vanH = px(76);
        int vanW = MulDiv(vanH, van.w, van.h);
        RECT art = { w - px(26) - vanW, px(14), w - px(26), px(14) + vanH };
        draw_image(dc, van, art, 0);
    }
    else
    {
        draw_van(dc, px(786), px(20), px(100), px(54));
    }
    static const Image mascot = load_png_image(IDR_MASCOT_PNG);
    draw_dog_logo(dc, px(24), px(16), px(82), mascot);

    static const Image wordmark = load_png_image(IDR_WORDMARK_PNG);
    if (wordmark.handle)
    {
        // The wordmark sits on a parchment sticker so the black lettering keeps
        // its contrast against the dark header.
        int artH = px(44);
        int artW = MulDiv(artH, wordmark.w, wordmark.h);
        RECT plate = { px(112), px(8), px(112) + artW + px(26), px(8) + artH + px(12) };
        fill_round(dc, plate, px(14), kCard, RGB(231, 219, 198));
        RECT art = { plate.left + px(13), plate.top + px(6), plate.left + px(13) + artW,
                     plate.top + px(6) + artH };
        draw_image(dc, wordmark, art, 0);
    }
    else
    {
        RECT title = { px(122), px(12), px(640), px(64) };
        RECT titleShadow = title;
        OffsetRect(&titleShadow, px(2), px(3));
        draw_text(dc, L"PROJECT: MYSTERY MACHINE", titleShadow, s_titleFont, RGB(16, 14, 25));
        draw_text(dc, L"PROJECT: MYSTERY MACHINE", title, s_titleFont, kAccent);
    }
    RECT subtitle = { px(124), px(66), px(640), px(92) };
    draw_text(dc, L"SCOOBY-DOO! NIGHT OF 100 FRIGHTS   /   PC", subtitle, s_smallFont,
              RGB(240, 228, 205));

    RECT badge = { px(646), px(30), px(758), px(56) };
    fill_round(dc, badge, 12, RGB(26, 57, 62), kTeal);
    RECT badgeText = badge;
    draw_text(dc, L"PC EDITION", badgeText, s_eyebrowFont, RGB(235, 247, 239),
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Sidebar.
    RECT side = { px(24), px(124), px(204), px(600) };
    fill_round(dc, side, px(16), kSidebar, kSidebarEdge);
    RECT label = { px(40), px(142), px(190), px(164) };
    draw_text(dc, L"PROJECT HQ", label, s_eyebrowFont, RGB(192, 174, 220),
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    draw_paw(dc, px(156), px(548), px(13), RGB(51, 40, 72));
    RECT edition = { px(40), px(574), px(188), px(594) };
    draw_text(dc, L"PC PORT  /  NO GAME DATA", edition, s_eyebrowFont, RGB(127, 112, 151),
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Page card with a soft drop shadow.
    RECT shadow = { px(226), px(129), px(900), px(623) };
    fill_round(dc, shadow, px(18), RGB(11, 9, 17), RGB(11, 9, 17));
    RECT card = { px(222), px(124), px(896), px(618) };
    fill_round(dc, card, px(18), kCard, RGB(228, 214, 191));
    draw_paw(dc, px(846), px(582), px(10), RGB(238, 228, 210));

    static const wchar_t* kickers[PAGE_COUNT] = {
        L"CASE FILE 01   /   THE MISSING DISC",
        L"CASE FILE 02   /   SCREEN SETUP",
        L"CASE FILE 03   /   LOOK & FEEL",
        L"CASE FILE 04   /   GHOST-PROOF CONTROLS",
    };
    static const wchar_t* titles[PAGE_COUNT] = { L"Game files", L"Display", L"Graphics", L"Controls" };
    static const wchar_t* subtitles[PAGE_COUNT] = {
        L"Bring your own disc. The launcher takes it from there.",
        L"Make the mystery look its best on your screen.",
        L"Dial in the fog, shadows and frame rate.",
        L"Pick up every clue with your keyboard or controller.",
    };
    RECT kicker = { px(248), px(140), px(870), px(158) };
    draw_text(dc, kickers[s_page], kicker, s_eyebrowFont, kAccentDark,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT pageTitle = { px(248), px(158), px(870), px(188) };
    draw_text(dc, titles[s_page], pageTitle, s_pageFont, kInk, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    RECT pageSub = { px(248), px(188), px(870), px(208) };
    draw_text(dc, subtitles[s_page], pageSub, s_smallFont, kMuted, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Footer.
    draw_line(dc, px(24), px(628), px(896), px(628), RGB(58, 45, 78), 1);
    RECT footer = { px(24), px(668), px(220), px(690) };
    draw_text(dc, L"PROJECT: MYSTERY MACHINE  /  PC", footer, s_eyebrowFont, RGB(133, 117, 151),
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}

static void draw_owner_button(const DRAWITEMSTRUCT* d)
{
    if (!d)
    {
        return;
    }
    HDC dc = d->hDC;
    RECT r = d->rcItem;
    bool disabled = (d->itemState & ODS_DISABLED) != 0;
    bool pressed = (d->itemState & ODS_SELECTED) != 0;
    wchar_t text[512] = L"";
    GetWindowTextW(d->hwndItem, text, 512);
    int id = (int)d->CtlID;

    if (id >= ID_NAV_GAME && id <= ID_NAV_CONTROLS)
    {
        HBRUSH bg = CreateSolidBrush(kSidebar);
        FillRect(dc, &r, bg);
        DeleteObject(bg);
        InflateRect(&r, -px(2), -px(2));
        bool active = id - ID_NAV_GAME == s_page;
        bool hot = s_hotButton == id && !active && !disabled;
        fill_round(dc, r, px(10), active ? kAccent : (hot ? RGB(54, 42, 77) : kSidebar),
                   active ? kAccent : (hot ? RGB(78, 61, 105) : kSidebar));
        if (active)
        {
            RECT bar = { r.left, r.top + px(10), r.left + px(4), r.bottom - px(10) };
            HBRUSH orange = CreateSolidBrush(RGB(255, 217, 150));
            FillRect(dc, &bar, orange);
            DeleteObject(orange);
        }
        draw_nav_icon(dc, id - ID_NAV_GAME, r.left + px(14), r.top + (r.bottom - r.top - px(20)) / 2,
                      active ? kInk : kTeal);
        RECT tr = { r.left + px(44), r.top, r.right - px(8), r.bottom };
        draw_text(dc, text, tr, s_navFont, active ? kInk : (disabled ? RGB(110, 96, 132) : RGB(240, 229, 210)),
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        if (d->itemState & ODS_FOCUS)
        {
            RECT f = r;
            InflateRect(&f, -px(4), -px(4));
            DrawFocusRect(dc, &f);
        }
        return;
    }

    if (is_checkbox_id(id))
    {
        HBRUSH bg = CreateSolidBrush(kCard);
        FillRect(dc, &r, bg);
        DeleteObject(bg);
        bool checked = check_get(id);
        int box = px(18);
        int top = r.top + (r.bottom - r.top - box) / 2;
        RECT boxR = { r.left + px(2), top, r.left + px(2) + box, top + box };
        fill_round(dc, boxR, px(5), checked ? (disabled ? RGB(198, 188, 172) : kTeal) : kField,
                   checked ? (disabled ? RGB(198, 188, 172) : RGB(31, 127, 111)) : kLine);
        if (checked)
        {
            HPEN pen = CreatePen(PS_SOLID, px(2), disabled ? RGB(120, 112, 101) : kField);
            HGDIOBJ old = SelectObject(dc, pen);
            MoveToEx(dc, boxR.left + px(4), boxR.top + px(9), NULL);
            LineTo(dc, boxR.left + px(7), boxR.top + px(13));
            LineTo(dc, boxR.left + px(14), boxR.top + px(5));
            SelectObject(dc, old);
            DeleteObject(pen);
        }
        RECT tr = { boxR.right + px(10), r.top, r.right - px(2), r.bottom };
        draw_text(dc, text, tr, s_font, disabled ? kMuted : kInk,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        return;
    }

    bool play = id == ID_PLAY;
    bool extract = id == ID_EXTRACT;
    bool save = id == ID_SAVE;
    COLORREF fill = (play || extract) ? kAccent : (save ? RGB(31, 88, 82) : kField);
    COLORREF border = (play || extract) ? kAccentDark : (save ? kTeal : kLine);
    COLORREF ink = (save ? RGB(242, 249, 242) : kInk);
    if (disabled)
    {
        fill = RGB(226, 216, 198);
        border = RGB(204, 193, 175);
        ink = RGB(147, 134, 119);
    }
    else if (pressed)
    {
        fill = (play || extract) ? RGB(210, 118, 25) : (save ? RGB(24, 70, 66) : RGB(239, 229, 212));
    }
    else if (s_hotButton == id && !save)
    {
        fill = (play || extract) ? RGB(247, 163, 62) : RGB(255, 253, 247);
        border = (play || extract) ? kAccentDark : RGB(198, 178, 148);
    }
    HBRUSH behind = CreateSolidBrush(save || play ? kBg : kCard);
    FillRect(dc, &d->rcItem, behind);
    DeleteObject(behind);
    InflateRect(&r, -px(1), -px(1));
    fill_round(dc, r, px(10), fill, border);
    RECT tr = r;
    InflateRect(&tr, -px(8), 0);
    draw_text(dc, text, tr, s_font, ink, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    if ((d->itemState & ODS_FOCUS) && !(d->itemState & ODS_NOFOCUSRECT))
    {
        RECT f = r;
        InflateRect(&f, -px(4), -px(4));
        DrawFocusRect(dc, &f);
    }
}

static LRESULT slider_custom_draw(const NMCUSTOMDRAW* cd)
{
    if (cd->dwDrawStage == CDDS_PREPAINT)
    {
        FillRect(cd->hdc, &cd->rc, card_brush());
        return CDRF_NOTIFYITEMDRAW;
    }
    if (cd->dwDrawStage == CDDS_ITEMPREPAINT)
    {
        if (cd->dwItemSpec == TBCD_CHANNEL)
        {
            RECT r = cd->rc;
            int cy = (r.top + r.bottom) / 2;
            RECT track = { r.left, cy - px(2), r.right, cy + px(2) };
            fill_round(cd->hdc, track, px(2), kTrack, kTrack);
            return CDRF_SKIPDEFAULT;
        }
        if (cd->dwItemSpec == TBCD_THUMB)
        {
            int cx = (cd->rc.left + cd->rc.right) / 2;
            int cy = (cd->rc.top + cd->rc.bottom) / 2;
            HBRUSH brush = CreateSolidBrush(kTeal);
            HPEN pen = CreatePen(PS_SOLID, px(2), kField);
            HGDIOBJ ob = SelectObject(cd->hdc, brush), op = SelectObject(cd->hdc, pen);
            Ellipse(cd->hdc, cx - px(7), cy - px(7), cx + px(7), cy + px(7));
            SelectObject(cd->hdc, ob);
            SelectObject(cd->hdc, op);
            DeleteObject(brush);
            DeleteObject(pen);
            return CDRF_SKIPDEFAULT;
        }
        if (cd->dwItemSpec == TBCD_TICS)
        {
            return CDRF_SKIPDEFAULT;
        }
    }
    return CDRF_DODEFAULT;
}

static LRESULT CALLBACK hot_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR)
{
    switch (msg)
    {
    case WM_MOUSEMOVE:
        if (s_hotButton != (int)id)
        {
            int old = s_hotButton;
            s_hotButton = (int)id;
            if (old)
            {
                InvalidateRect(item(old), NULL, TRUE);
            }
            InvalidateRect(hwnd, NULL, TRUE);
        }
        {
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
        }
        break;
    case WM_MOUSELEAVE:
        if (s_hotButton == (int)id)
        {
            s_hotButton = 0;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    }
    return DefSubclassProc(hwnd, msg, wp, lp);
}

static void use_dark_titlebar(HWND hwnd)
{
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark)); // DWMWA_USE_IMMERSIVE_DARK_MODE
    COLORREF caption = kBg;
    DwmSetWindowAttribute(hwnd, 35, &caption, sizeof(caption)); // DWMWA_CAPTION_COLOR
    COLORREF textColor = RGB(246, 236, 216);
    DwmSetWindowAttribute(hwnd, 36, &textColor, sizeof(textColor)); // DWMWA_TEXT_COLOR
    int corner = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, 33, &corner, sizeof(corner)); // DWMWA_WINDOW_CORNER_PREFERENCE
}

// ---------------------------------------------------------------- game files page
// Where the game will find its files (the same order as pc/src/dvd.cpp).
static std::wstring find_game_dir()
{
    std::string set = s_ini.get("game", "data", "");
    if (!set.empty())
    {
        return widen(set);
    }
    const std::wstring candidates[] = { s_dir + L"data", s_dir + L"game\\files", s_dir + L"..\\..\\game\\files" };
    for (const std::wstring& c : candidates)
    {
        if (is_game_dir(c))
        {
            return c;
        }
    }
    return L"";
}

static void update_game_status()
{
    std::wstring d = find_game_dir();
    if (!d.empty() && is_game_dir(d))
    {
        wchar_t full[MAX_PATH];
        GetFullPathNameW(d.c_str(), MAX_PATH, full, NULL);
        set_text(ID_GAME_STATUS, std::wstring(L"Game files found in ") + full + L". You're ready to play.");
    }
    else if (!d.empty())
    {
        set_text(ID_GAME_STATUS, L"The chosen game folder " + d + L" doesn't hold the game's files. Extract them "
                                 L"from your disc image or choose another folder.");
    }
    else
    {
        set_text(ID_GAME_STATUS, L"No game files yet. Choose your disc image and press Extract game files.");
    }
}

static void browse_iso()
{
    wchar_t file[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = s_main;
    ofn.lpstrFilter = L"GameCube disc images (*.iso;*.gcm)\0*.iso;*.gcm\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"Choose your Scooby-Doo! Night of 100 Frights disc image";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameW(&ofn))
    {
        set_text(ID_ISO_PATH, file);
        EnableWindow(item(ID_EXTRACT), TRUE);
    }
}

static void start_extract()
{
    wchar_t iso[MAX_PATH];
    GetWindowTextW(item(ID_ISO_PATH), iso, MAX_PATH);
    if (!iso[0] || s_extracting)
    {
        return;
    }
    FILE* f = _wfopen(iso, L"rb");
    std::wstring err = f ? check_disc(f) : L"Can't open the disc image.";
    if (f)
    {
        fclose(f);
    }
    if (!err.empty())
    {
        MessageBoxW(s_main, err.c_str(), L"Extract game files", MB_ICONWARNING);
        return;
    }
    ExtractJob* job = new ExtractJob;
    job->iso = iso;
    job->out = s_dir + L"data";
    job->notify = s_main;
    s_extracting = true;
    for (int id : { ID_EXTRACT, ID_ISO_BROWSE, ID_GAME_FOLDER, ID_PLAY })
    {
        EnableWindow(item(id), FALSE);
    }
    SendMessageW(item(ID_PROGRESS), PBM_SETPOS, 0, 0);
    set_text(ID_GAME_STATUS, L"Extracting the game's files (about 1.1 GB)...");
    HANDLE t = CreateThread(NULL, 0, extract_thread, job, 0, NULL);
    if (t)
    {
        CloseHandle(t);
    }
}

static void extract_done(bool ok, const std::wstring& err)
{
    s_extracting = false;
    for (int id : { ID_EXTRACT, ID_ISO_BROWSE, ID_GAME_FOLDER, ID_PLAY })
    {
        EnableWindow(item(id), TRUE);
    }
    if (ok)
    {
        // The extracted folder is found by default; drop an older custom one.
        s_ini.set("game", "data", "");
        SendMessageW(item(ID_PROGRESS), PBM_SETPOS, 1000, 0);
        update_game_status();
        status(L"Game files extracted.");
    }
    else
    {
        SendMessageW(item(ID_PROGRESS), PBM_SETPOS, 0, 0);
        update_game_status();
        MessageBoxW(s_main, err.c_str(), L"Extract game files", MB_ICONWARNING);
    }
}

static void choose_game_folder()
{
    BROWSEINFOW bi = {};
    bi.hwndOwner = s_main;
    bi.lpszTitle = L"Choose a folder holding the game's extracted disc files (boot.HIP, sd2.ini, fmv...)";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (!pidl)
    {
        return;
    }
    wchar_t path[MAX_PATH];
    if (SHGetPathFromIDListW(pidl, path))
    {
        if (is_game_dir(path))
        {
            s_ini.set("game", "data", narrow(path));
            update_game_status();
        }
        else
        {
            MessageBoxW(s_main, L"That folder doesn't hold the game's files (boot.HIP and sd2.ini are missing).",
                        L"Game folder", MB_ICONWARNING);
        }
    }
    CoTaskMemFree(pidl);
}

// ---------------------------------------------------------------- display and graphics pages
static const struct { const wchar_t* label; const char* value; } kModes[] = {
    { L"Windowed", "windowed" },
    { L"Borderless fullscreen", "borderless" },
    { L"Fullscreen (exclusive)", "fullscreen" },
};
static const struct { const wchar_t* label; const char* value; } kAspects[] = {
    { L"Fit the window (widescreen)", "auto" },
    { L"4:3 (original)", "4:3" },
    { L"16:9", "16:9" },
    { L"16:10", "16:10" },
    { L"21:9", "21:9" },
};
static const int kMsaa[] = { 0, 2, 4, 8 };
static const struct { const wchar_t* label; int trilinear, aniso; } kFilters[] = {
    { L"Original (bilinear)", 0, 1 },
    { L"Trilinear", 1, 1 },
    { L"Anisotropic 4x", 1, 4 },
    { L"Anisotropic 8x", 1, 8 },
    { L"Anisotropic 16x", 1, 16 },
};

static void fill_resolutions()
{
    DEVMODEW dm = {};
    dm.dmSize = sizeof(dm);
    for (DWORD i = 0; EnumDisplaySettingsW(NULL, i, &dm); i++)
    {
        std::pair<int, int> r((int)dm.dmPelsWidth, (int)dm.dmPelsHeight);
        if (r.first >= 640 && r.second >= 480 &&
            std::find(s_resolutions.begin(), s_resolutions.end(), r) == s_resolutions.end())
        {
            s_resolutions.push_back(r);
        }
    }
    // Without a setting: the desktop's resolution.
    std::string dw = std::to_string(GetSystemMetrics(SM_CXSCREEN)), dh = std::to_string(GetSystemMetrics(SM_CYSCREEN));
    int w = atoi(s_ini.get("display", "width", dw).c_str()), h = atoi(s_ini.get("display", "height", dh).c_str());
    std::pair<int, int> cur(w, h);
    if (std::find(s_resolutions.begin(), s_resolutions.end(), cur) == s_resolutions.end())
    {
        s_resolutions.push_back(cur);
    }
    std::sort(s_resolutions.begin(), s_resolutions.end(),
              [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                  return a.first * a.second > b.first * b.second ||
                         (a.first * a.second == b.first * b.second && a.first > b.first);
              });
    for (size_t i = 0; i < s_resolutions.size(); i++)
    {
        wchar_t t[64];
        swprintf(t, 64, L"%d x %d", s_resolutions[i].first, s_resolutions[i].second);
        combo_add(ID_RES, t);
        if (s_resolutions[i] == cur)
        {
            combo_set(ID_RES, (int)i);
        }
    }
}

static int slider(int id) { return (int)SendMessageW(item(id), TBM_GETPOS, 0, 0); }

static void update_distance_labels()
{
    wchar_t t[32];
    swprintf(t, 32, L"%.2gx", slider(ID_DRAW) / 4.0);
    set_text(ID_DRAW_VALUE, t);
    swprintf(t, 32, L"%.2gx", slider(ID_FX) / 4.0);
    set_text(ID_FX_VALUE, t);
    swprintf(t, 32, L"+%d\u00B0", slider(ID_FOV));
    set_text(ID_FOV_VALUE, t);
    swprintf(t, 32, L"%d%%", slider(ID_BRIGHT) * 5);
    set_text(ID_BRIGHT_VALUE, t);
}

static void update_mode_dependent()
{
    // Borderless always uses the desktop's resolution.
    EnableWindow(item(ID_RES), combo_get(ID_MODE) != 1);
}

// ---------------------------------------------------------------- controls page
static std::wstring key_text(const std::string& keys)
{
    std::wstring t;
    for (wchar_t c : widen(keys))
    {
        t += c;
        if (c == L',')
        {
            t += L' ';
        }
    }
    return keys.empty() ? L"(none)" : t;
}

// SDL's controller input names, as players know them (Xbox-style labels).
static std::wstring pad_name(const std::string& n)
{
    static const struct { const char* sdl; const wchar_t* label; } names[] = {
        { "a", L"A (bottom)" }, { "b", L"B (right)" }, { "x", L"X (left)" }, { "y", L"Y (top)" },
        { "back", L"Back / View" }, { "guide", L"Guide" }, { "start", L"Start / Menu" },
        { "leftstick", L"Left stick press" }, { "rightstick", L"Right stick press" },
        { "leftshoulder", L"Left bumper" }, { "rightshoulder", L"Right bumper" }, { "dpup", L"D-pad up" },
        { "dpdown", L"D-pad down" }, { "dpleft", L"D-pad left" }, { "dpright", L"D-pad right" },
        { "misc1", L"Share / Capture" }, { "touchpad", L"Touchpad" }, { "paddle1", L"Paddle 1" },
        { "paddle2", L"Paddle 2" }, { "paddle3", L"Paddle 3" }, { "paddle4", L"Paddle 4" },
        { "lefttrigger", L"Left trigger" }, { "righttrigger", L"Right trigger" },
        { "leftx", L"Left stick sideways" }, { "lefty", L"Left stick up/down" },
        { "rightx", L"Right stick sideways" }, { "righty", L"Right stick up/down" },
    };
    for (const auto& m : names)
    {
        if (n == m.sdl)
        {
            return m.label;
        }
    }
    return widen(n);
}

static std::wstring pad_text(int row)
{
    if (!kActions[row].pad)
    {
        return check_get(ID_SWAP_STICKS) ? (row < 4 ? L"Right stick" : L"Left stick")
                                         : (row < 4 ? L"Left stick" : L"Right stick");
    }
    return s_pads[row].empty() ? L"(none)" : pad_name(s_pads[row]);
}

static void refresh_binds()
{
    HWND lv = item(ID_BINDS);
    for (int i = 0; i < kNumActions; i++)
    {
        std::wstring k = key_text(s_keys[i]), p = pad_text(i);
        ListView_SetItemText(lv, i, 1, (LPWSTR)k.c_str());
        ListView_SetItemText(lv, i, 2, (LPWSTR)p.c_str());
    }
}

static int selected_row() { return ListView_GetNextItem(item(ID_BINDS), -1, LVNI_SELECTED); }

static void open_pad()
{
    if (s_pad && SDL_GameControllerGetAttached(s_pad))
    {
        return;
    }
    s_pad = NULL;
    for (int i = 0; i < SDL_NumJoysticks(); i++)
    {
        if (SDL_IsGameController(i) && (s_pad = SDL_GameControllerOpen(i)) != NULL)
        {
            break;
        }
    }
}

static void end_capture(const std::wstring& msg)
{
    s_capture = CAPTURE_NONE;
    s_captureRow = -1;
    KillTimer(s_main, 1);
    set_text(ID_CAPTURE, msg);
    refresh_binds();
}

static void begin_capture(int kind)
{
    int row = selected_row();
    if (row < 0)
    {
        set_text(ID_CAPTURE, L"Choose an action in the list first.");
        return;
    }
    if (kind == CAPTURE_PAD && !kActions[row].pad)
    {
        set_text(ID_CAPTURE, L"Moving and the camera use the controller's analog sticks; use Swap analog sticks "
                             L"to exchange them.");
        return;
    }
    s_capture = kind;
    s_captureRow = row;
    if (kind == CAPTURE_PAD)
    {
        open_pad();
        if (!s_pad)
        {
            end_capture(L"No controller found. Connect one and try again.");
            return;
        }
        SDL_JoystickUpdate();
        for (int b = 0; b < SDL_CONTROLLER_BUTTON_MAX; b++)
        {
            s_padBase[b] = SDL_GameControllerGetButton(s_pad, (SDL_GameControllerButton)b);
        }
        SetTimer(s_main, 1, 30, NULL);
        set_text(ID_CAPTURE, std::wstring(L"Press a controller button or trigger for \"") + kActions[row].label +
                                 L"\" (Esc cancels)...");
    }
    else
    {
        set_text(ID_CAPTURE, std::wstring(L"Press a key for \"") + kActions[row].label + L"\" (Esc cancels)...");
    }
    SetFocus(item(ID_BINDS));
}

// A key press while capturing: true when it was used.
static bool capture_key(MSG& msg)
{
    if (s_capture == CAPTURE_NONE || (msg.message != WM_KEYDOWN && msg.message != WM_SYSKEYDOWN))
    {
        return false;
    }
    int scan = (int)((msg.lParam >> 16) & 0xFF) | ((msg.lParam & (1 << 24)) ? 0x100 : 0);
    if (scan == 0x01)
    {
        end_capture(L"Cancelled.");
        return true;
    }
    if (s_capture == CAPTURE_PAD)
    {
        return true; // waiting for the controller
    }
    SDL_Scancode sc = sdl_scancode(scan);
    if (sc == SDL_SCANCODE_UNKNOWN)
    {
        set_text(ID_CAPTURE, L"That key can't be used; press another (Esc cancels).");
        return true;
    }
    std::string name = SDL_GetScancodeName(sc);
    std::string& keys = s_keys[s_captureRow];
    if (s_capture == CAPTURE_SET_KEY)
    {
        keys = name;
    }
    else if (("," + keys + ",").find("," + name + ",") == std::string::npos)
    {
        keys = keys.empty() ? name : keys + "," + name;
    }
    end_capture(std::wstring(L"\"") + kActions[s_captureRow].label + L"\" is now " + key_text(keys) + L".");
    return true;
}

static void poll_pad_capture()
{
    if (s_capture != CAPTURE_PAD)
    {
        return;
    }
    SDL_JoystickUpdate(); // also finds controllers plugged in since (not SDL_PumpEvents: it
                          // would dispatch this window's messages)
    open_pad();
    if (!s_pad)
    {
        return;
    }
    const char* name = NULL;
    for (int b = 0; b < SDL_CONTROLLER_BUTTON_MAX && !name; b++)
    {
        Uint8 down = SDL_GameControllerGetButton(s_pad, (SDL_GameControllerButton)b);
        if (down && !s_padBase[b])
        {
            name = SDL_GameControllerGetStringForButton((SDL_GameControllerButton)b);
        }
        s_padBase[b] = down;
    }
    if (!name)
    {
        // Triggers (and a stick pushed all the way, as a button).
        for (int a = 0; a < SDL_CONTROLLER_AXIS_MAX && !name; a++)
        {
            int v = SDL_GameControllerGetAxis(s_pad, (SDL_GameControllerAxis)a);
            bool trigger = a == SDL_CONTROLLER_AXIS_TRIGGERLEFT || a == SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
            if ((trigger && v > 20000) || (!trigger && (v > 30000 || v < -30000)))
            {
                name = SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)a);
            }
        }
    }
    if (name)
    {
        s_pads[s_captureRow] = name;
        end_capture(std::wstring(L"\"") + kActions[s_captureRow].label + L"\" is now " + pad_name(name) +
                    L" on the controller.");
    }
}

// ---------------------------------------------------------------- load / save
static void load_settings()
{
    s_ini = Ini();
    s_ini.load(s_iniPath);
    // Display
    std::string mode = s_ini.get("display", "mode", "borderless");
    combo_set(ID_MODE, 1);
    for (int i = 0; i < 3; i++)
    {
        if (mode == kModes[i].value)
        {
            combo_set(ID_MODE, i);
        }
    }
    std::string aspect = s_ini.get("display", "aspect", "auto");
    combo_set(ID_ASPECT, 0);
    for (int i = 0; i < 5; i++)
    {
        if (aspect == kAspects[i].value)
        {
            combo_set(ID_ASPECT, i);
        }
    }
    check_set(ID_VSYNC, s_ini.get("display", "vsync", "1") != "0");
    combo_set(ID_HUD, s_ini.get("display", "hud", "wide") == "classic" ? 1 : 0);
    combo_set(ID_FPS, s_ini.get("graphics", "fps", "60") == "30" ? 0 : 1);
    int msaa = atoi(s_ini.get("display", "msaa", "4").c_str());
    combo_set(ID_MSAA, 0);
    for (int i = 0; i < 4; i++)
    {
        if (msaa == kMsaa[i])
        {
            combo_set(ID_MSAA, i);
        }
    }
    // Graphics
    double draw = atof(s_ini.get("graphics", "draw_distance", "1").c_str());
    double fx = atof(s_ini.get("graphics", "effects_distance", "1").c_str());
    SendMessageW(item(ID_DRAW), TBM_SETPOS, TRUE, (LPARAM)(int)(draw * 4 + 0.5));
    SendMessageW(item(ID_FX), TBM_SETPOS, TRUE, (LPARAM)(int)(fx * 4 + 0.5));
    SendMessageW(item(ID_FOV), TBM_SETPOS, TRUE, (LPARAM)atoi(s_ini.get("graphics", "fov", "0").c_str()));
    double bright = atof(s_ini.get("graphics", "brightness", "1").c_str());
    SendMessageW(item(ID_BRIGHT), TBM_SETPOS, TRUE, (LPARAM)(int)(bright * 20 + 0.5));
    int shadow = atoi(s_ini.get("graphics", "shadow_res", "1024").c_str());
    combo_set(ID_SHADOW, shadow >= 2048 ? 3 : shadow >= 1024 ? 2 : shadow >= 512 ? 1 : 0);
    check_set(ID_SKIP_LOGOS, s_ini.get("game", "skip_logos", "0") == "1");
    int tri = atoi(s_ini.get("graphics", "trilinear", "1").c_str());
    int aniso = atoi(s_ini.get("graphics", "anisotropy", "16").c_str());
    combo_set(ID_FILTER, 0);
    for (int i = 0; i < 5; i++)
    {
        if (tri == kFilters[i].trilinear && (aniso == kFilters[i].aniso || (i == 1 && aniso <= 1 && tri)))
        {
            combo_set(ID_FILTER, i);
        }
    }
    // Controls
    for (int i = 0; i < kNumActions; i++)
    {
        s_keys[i] = s_ini.get("keyboard", kActions[i].name, kActions[i].keys);
        s_pads[i] = kActions[i].pad ? s_ini.get("controller", kActions[i].name, kActions[i].pad) : "";
    }
    check_set(ID_SWAP_STICKS, s_ini.get("controller", "sticks", "normal") == "swapped");
    check_set(ID_DEBUG_MENU, s_ini.get("debug", "menu", "0") == "1");
    update_distance_labels();
    update_mode_dependent();
}

static bool save_settings()
{
    int r = combo_get(ID_RES);
    if (r >= 0 && r < (int)s_resolutions.size())
    {
        s_ini.set("display", "width", std::to_string(s_resolutions[r].first));
        s_ini.set("display", "height", std::to_string(s_resolutions[r].second));
    }
    s_ini.set("display", "mode", kModes[combo_get(ID_MODE)].value);
    s_ini.set("display", "aspect", kAspects[combo_get(ID_ASPECT)].value);
    s_ini.set("display", "vsync", check_get(ID_VSYNC) ? "1" : "0");
    s_ini.set("display", "msaa", std::to_string(kMsaa[combo_get(ID_MSAA)]));
    s_ini.set("display", "hud", combo_get(ID_HUD) == 1 ? "classic" : "wide");
    s_ini.set("graphics", "fps", combo_get(ID_FPS) == 0 ? "30" : "60");
    char t[32];
    snprintf(t, sizeof(t), "%.2f", SendMessageW(item(ID_DRAW), TBM_GETPOS, 0, 0) / 4.0);
    s_ini.set("graphics", "draw_distance", t);
    snprintf(t, sizeof(t), "%.2f", SendMessageW(item(ID_FX), TBM_GETPOS, 0, 0) / 4.0);
    s_ini.set("graphics", "effects_distance", t);
    s_ini.set("graphics", "fov", std::to_string(slider(ID_FOV)));
    snprintf(t, sizeof(t), "%.2f", slider(ID_BRIGHT) / 20.0);
    s_ini.set("graphics", "brightness", t);
    static const int kShadow[] = { 256, 512, 1024, 2048 };
    s_ini.set("graphics", "shadow_res", std::to_string(kShadow[combo_get(ID_SHADOW) < 0 ? 0 : combo_get(ID_SHADOW)]));
    s_ini.set("game", "skip_logos", check_get(ID_SKIP_LOGOS) ? "1" : "0");
    int f = combo_get(ID_FILTER);
    s_ini.set("graphics", "trilinear", std::to_string(kFilters[f].trilinear));
    s_ini.set("graphics", "anisotropy", std::to_string(kFilters[f].aniso));
    for (int i = 0; i < kNumActions; i++)
    {
        s_ini.set("keyboard", kActions[i].name, s_keys[i]);
        if (kActions[i].pad)
        {
            s_ini.set("controller", kActions[i].name, s_pads[i]);
        }
    }
    s_ini.set("controller", "sticks", check_get(ID_SWAP_STICKS) ? "swapped" : "normal");
    s_ini.set("debug", "menu", check_get(ID_DEBUG_MENU) ? "1" : "0");
    if (!s_ini.save(s_iniPath))
    {
        MessageBoxW(s_main, (L"Can't write " + s_iniPath + L". Is the folder read-only?").c_str(), L"Settings",
                    MB_ICONWARNING);
        return false;
    }
    status(L"Settings saved.");
    return true;
}

static void play()
{
    std::wstring d = find_game_dir();
    if (d.empty() || !is_game_dir(d))
    {
        show_page(PAGE_GAME);
        MessageBoxW(s_main, L"The game's files aren't there yet: extract them from your disc image first.",
                    L"Play", MB_ICONINFORMATION);
        return;
    }
    if (!save_settings())
    {
        return;
    }
    std::wstring exe = s_dir + L"scooby.exe";
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    std::wstring cmd = L"\"" + exe + L"\"";
    if (!CreateProcessW(exe.c_str(), &cmd[0], NULL, NULL, FALSE, 0, NULL, s_dir.c_str(), &si, &pi))
    {
        MessageBoxW(s_main, (L"Can't start " + exe + L".").c_str(), L"Play", MB_ICONERROR);
        return;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    DestroyWindow(s_main);
}

// ---------------------------------------------------------------- layout
static void create_controls()
{
    const int L = 248, T = 216, W = 604;
    const int CARD_RIGHT = L + W;

    // Sidebar: four case files, always visible.
    static const wchar_t* navNames[PAGE_COUNT] = { L"Game files", L"Display", L"Graphics", L"Controls" };
    for (int i = 0; i < PAGE_COUNT; i++)
    {
        add(-1, kNavIds[i], L"BUTTON", navNames[i], WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            34, 178 + 50 * i, 162, 42);
    }

    // Game files.
    add(PAGE_GAME, ID_GAME_INFO, L"STATIC",
        L"This release contains no game data. The launcher copies the files from your own USA GameCube disc "
        L"(GIHE78) into the data folder beside the program. Choose a plain .iso or .gcm image; compressed "
        L"RVZ, WIA, CISO and GCZ images must be converted to ISO first.",
        0, L, T, W, 58);
    add(PAGE_GAME, ID_ISO_LABEL, L"STATIC", L"Disc image:", 0, L, T + 70, 160, 20);
    add(PAGE_GAME, ID_ISO_PATH, L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | ES_READONLY, L, T + 90, 466, 28,
        WS_EX_CLIENTEDGE);
    add(PAGE_GAME, ID_ISO_BROWSE, L"BUTTON", L"Browse...", WS_TABSTOP | BS_PUSHBUTTON, L + 478, T + 89, 126, 30);
    HWND ex = add(PAGE_GAME, ID_EXTRACT, L"BUTTON", L"Extract game files", WS_TABSTOP | BS_PUSHBUTTON, L, T + 132,
                  190, 34);
    EnableWindow(ex, FALSE);
    HWND prog = add(PAGE_GAME, ID_PROGRESS, PROGRESS_CLASSW, L"", 0, L + 204, T + 137, W - 204, 22);
    SendMessageW(prog, PBM_SETRANGE32, 0, 1000);
    SetWindowTheme(prog, L"", L"");
    SendMessageW(prog, PBM_SETBKCOLOR, 0, (LPARAM)kField);
    SendMessageW(prog, PBM_SETBARCOLOR, 0, (LPARAM)kTeal);
    add(PAGE_GAME, ID_GAME_STATUS, L"STATIC", L"", 0, L, T + 180, W, 44);
    add(PAGE_GAME, ID_GAME_FOLDER, L"BUTTON", L"Use a folder I already extracted...",
        WS_TABSTOP | BS_PUSHBUTTON, L, T + 244, 300, 32);
    add(PAGE_GAME, ID_LOGS, L"BUTTON", L"Open log folder", WS_TABSTOP | BS_PUSHBUTTON, L + 320, T + 244, 180, 32);
    add(PAGE_GAME, ID_SKIP_LOGOS, L"BUTTON", L"Skip the company logo movies when the game starts",
        WS_TABSTOP | BS_AUTOCHECKBOX, L, T + 292, 460, 24);
    add(PAGE_GAME, ID_GAME_NOTE, L"STATIC",
        L"Your disc image is only read locally. Nothing is uploaded, and the extracted files stay in data/ "
        L"beside the launcher.",
        0, L, T + 326, W, 40);

    // Display.
    struct Row { int label; int combo; const wchar_t* text; };
    static const Row rows[] = {
        { ID_RES_LABEL, ID_RES, L"Resolution:" },
        { ID_MODE_LABEL, ID_MODE, L"Window mode:" },
        { ID_ASPECT_LABEL, ID_ASPECT, L"Aspect ratio:" },
        { ID_MSAA_LABEL, ID_MSAA, L"Anti-aliasing:" },
        { ID_HUD_LABEL, ID_HUD, L"HUD layout:" },
    };
    for (int i = 0; i < 5; i++)
    {
        int y = T + 8 + 40 * i;
        add(PAGE_DISPLAY, rows[i].label, L"STATIC", rows[i].text, 0, L, y + 6, 180, 22);
        HWND cb = add(PAGE_DISPLAY, rows[i].combo, WC_COMBOBOXW, L"", WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                      L + 190, y, 280, 240);
        SetWindowTheme(cb, L"Explorer", NULL);
    }
    add(PAGE_DISPLAY, ID_VSYNC, L"BUTTON", L"Vertical sync (no tearing)", WS_TABSTOP | BS_AUTOCHECKBOX,
        L + 190, T + 208, 300, 24);
    add(PAGE_DISPLAY, ID_DISPLAY_NOTE, L"STATIC",
        L"Widescreen shows more of each level at the sides without stretching the picture, and the HUD moves "
        L"out to the screen's corners (or stays in the middle with the original layout). The title screen and "
        L"movies keep their 4:3 shape. Borderless fullscreen uses your desktop's resolution; Alt+Enter "
        L"switches between a window and fullscreen while playing.",
        0, L, T + 250, W, 92);
    for (const wchar_t* m : { kModes[0].label, kModes[1].label, kModes[2].label })
    {
        combo_add(ID_MODE, m);
    }
    for (const auto& a : kAspects)
    {
        combo_add(ID_ASPECT, a.label);
    }
    for (const wchar_t* m : { L"Off", L"2x", L"4x", L"8x" })
    {
        combo_add(ID_MSAA, m);
    }
    combo_add(ID_HUD, L"Widescreen (corners)");
    combo_add(ID_HUD, L"Original (4:3 middle)");

    // Graphics: one slider row each, then three dropdowns and a note.
    struct Slider { int label, bar, value; const wchar_t* text; int lo, hi, tic; } sliders[] = {
        { ID_DRAW_LABEL, ID_DRAW, ID_DRAW_VALUE, L"Draw distance:", 4, 16, 2 },
        { ID_FX_LABEL, ID_FX, ID_FX_VALUE, L"Effects distance:", 4, 16, 2 },
        { ID_FOV_LABEL, ID_FOV, ID_FOV_VALUE, L"Field of view:", 0, 30, 5 },
        { ID_BRIGHT_LABEL, ID_BRIGHT, ID_BRIGHT_VALUE, L"Brightness:", 10, 40, 2 },
    };
    for (int i = 0; i < 4; i++)
    {
        int y = T + 8 + 38 * i;
        add(PAGE_GRAPHICS, sliders[i].label, L"STATIC", sliders[i].text, 0, L, y + 7, 180, 22);
        HWND tb = add(PAGE_GRAPHICS, sliders[i].bar, TRACKBAR_CLASSW, L"", WS_TABSTOP | TBS_AUTOTICKS,
                      L + 190, y, 300, 30);
        add(PAGE_GRAPHICS, sliders[i].value, L"STATIC", L"", 0, L + 516, y + 7, 84, 22);
        SendMessageW(tb, TBM_SETRANGE, TRUE, MAKELPARAM(sliders[i].lo, sliders[i].hi));
        SendMessageW(tb, TBM_SETTICFREQ, sliders[i].tic, 0);
        SendMessageW(tb, TBM_SETPAGESIZE, 0, sliders[i].tic);
        SetWindowTheme(tb, L"", L"");
    }
    struct Combo { int label, combo; const wchar_t* text; } combos[] = {
        { ID_FILTER_LABEL, ID_FILTER, L"Texture filtering:" },
        { ID_SHADOW_LABEL, ID_SHADOW, L"Shadow quality:" },
        { ID_FPS_LABEL, ID_FPS, L"Frame rate:" },
    };
    for (int i = 0; i < 3; i++)
    {
        int y = T + 164 + 38 * i;
        add(PAGE_GRAPHICS, combos[i].label, L"STATIC", combos[i].text, 0, L, y + 5, 180, 22);
        HWND cb = add(PAGE_GRAPHICS, combos[i].combo, WC_COMBOBOXW, L"",
                      WS_TABSTOP | CBS_DROPDOWNLIST, L + 190, y, 280, 220);
        SetWindowTheme(cb, L"Explorer", NULL);
    }
    add(PAGE_GRAPHICS, ID_GRAPHICS_NOTE, L"STATIC",
        L"Draw distance pushes the far edge of the view and the fog back (some places expect fog to hide what "
        L"lies beyond); effects distance is how far particle effects still show. The game has no model levels "
        L"of detail, so these are its distance settings. 60 fps keeps jumps, speeds and timings as on the "
        L"GameCube; Field of view adds degrees to the game's camera.",
        0, L, T + 286, W, 92);
    for (const auto& f : kFilters)
    {
        combo_add(ID_FILTER, f.label);
    }
    for (const wchar_t* t : { L"Original (256)", L"High (512)", L"Very high (1024)", L"Ultra (2048)" })
    {
        combo_add(ID_SHADOW, t);
    }
    combo_add(ID_FPS, L"30 fps (original)");
    combo_add(ID_FPS, L"60 fps");

    // Controls.
    HWND lv = add(PAGE_CONTROLS, ID_BINDS, WC_LISTVIEWW, L"",
                  WS_TABSTOP | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                  L, T, W, 224);
    ListView_SetExtendedListViewStyle(lv, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    ListView_SetBkColor(lv, kField);
    ListView_SetTextBkColor(lv, kField);
    ListView_SetTextColor(lv, kInk);
    const wchar_t* cols[] = { L"Action", L"Keyboard", L"Controller" };
    const int widths[] = { 200, 220, 184 };
    for (int i = 0; i < 3; i++)
    {
        LVCOLUMNW c = {};
        c.mask = LVCF_TEXT | LVCF_WIDTH;
        c.pszText = (LPWSTR)cols[i];
        c.cx = px(widths[i]);
        ListView_InsertColumn(lv, i, &c);
    }
    for (int i = 0; i < kNumActions; i++)
    {
        LVITEMW it = {};
        it.mask = LVIF_TEXT;
        it.iItem = i;
        it.pszText = (LPWSTR)kActions[i].label;
        ListView_InsertItem(lv, &it);
    }
    add(PAGE_CONTROLS, ID_SET_KEY, L"BUTTON", L"Set key", WS_TABSTOP | BS_PUSHBUTTON, L, T + 236, 100, 30);
    add(PAGE_CONTROLS, ID_ADD_KEY, L"BUTTON", L"Add key", WS_TABSTOP | BS_PUSHBUTTON, L + 106, T + 236, 100, 30);
    add(PAGE_CONTROLS, ID_SET_PAD, L"BUTTON", L"Set button", WS_TABSTOP | BS_PUSHBUTTON, L + 212, T + 236, 110, 30);
    add(PAGE_CONTROLS, ID_CLEAR, L"BUTTON", L"Clear", WS_TABSTOP | BS_PUSHBUTTON, L + 328, T + 236, 80, 30);
    add(PAGE_CONTROLS, ID_DEFAULTS, L"BUTTON", L"Defaults", WS_TABSTOP | BS_PUSHBUTTON, L + 414, T + 236, 190, 30);
    add(PAGE_CONTROLS, ID_SWAP_STICKS, L"BUTTON",
        L"Swap analog sticks (right stick moves, left stick is the camera)",
        WS_TABSTOP | BS_AUTOCHECKBOX, L, T + 278, W, 24);
    add(PAGE_CONTROLS, ID_DEBUG_MENU, L"BUTTON",
        L"Enable the debug menu (F1 or the controller's Back button)",
        WS_TABSTOP | BS_AUTOCHECKBOX, L, T + 304, W, 24);
    add(PAGE_CONTROLS, ID_CAPTURE, L"STATIC",
        L"Pick an action, then Set key, Add key (for a second key) or Set button (controller). "
        L"Double-click sets a key.",
        0, L, T + 352, 340, 40);

    // Footer.
    add(-1, ID_STATUS, L"STATIC", L"Ready when you are.", WS_VISIBLE | SS_ENDELLIPSIS, L, 674, 390, 20);
    add(-1, ID_SAVE, L"BUTTON", L"Save settings", WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 656, 636, 118, 38);
    add(-1, ID_PLAY, L"BUTTON", L"Play", WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 786, 636, 110, 38);
}

// ---------------------------------------------------------------- window procedure
static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        HDC mem = CreateCompatibleDC(dc);
        HBITMAP bmp = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
        HGDIOBJ old = SelectObject(mem, bmp);
        draw_background(mem, rc.right, rc.bottom);
        BitBlt(dc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
        SelectObject(mem, old);
        DeleteObject(bmp);
        DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DRAWITEM:
        draw_owner_button((const DRAWITEMSTRUCT*)lp);
        return TRUE;
    case WM_NOTIFY:
    {
        NMHDR* nm = (NMHDR*)lp;
        if (nm->code == NM_CUSTOMDRAW && is_slider_id((int)nm->idFrom))
        {
            return slider_custom_draw((const NMCUSTOMDRAW*)lp);
        }
        if (nm->idFrom == ID_BINDS && nm->code == NM_DBLCLK)
        {
            begin_capture(CAPTURE_SET_KEY);
        }
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) >= ID_NAV_GAME && LOWORD(wp) <= ID_NAV_CONTROLS)
        {
            show_page(LOWORD(wp) - ID_NAV_GAME);
            return 0;
        }
        switch (LOWORD(wp))
        {
        case ID_ISO_BROWSE: browse_iso(); break;
        case ID_EXTRACT: start_extract(); break;
        case ID_GAME_FOLDER: choose_game_folder(); break;
        case ID_LOGS: open_log_folder(); break;
        case ID_MODE:
            if (HIWORD(wp) == CBN_SELCHANGE)
            {
                update_mode_dependent();
            }
            break;
        case ID_VSYNC:
            check_set(ID_VSYNC, !check_get(ID_VSYNC));
            InvalidateRect(item(ID_VSYNC), NULL, TRUE);
            break;
        case ID_SKIP_LOGOS:
            check_set(ID_SKIP_LOGOS, !check_get(ID_SKIP_LOGOS));
            InvalidateRect(item(ID_SKIP_LOGOS), NULL, TRUE);
            break;
        case ID_DEBUG_MENU:
            check_set(ID_DEBUG_MENU, !check_get(ID_DEBUG_MENU));
            InvalidateRect(item(ID_DEBUG_MENU), NULL, TRUE);
            break;
        case ID_SET_KEY: begin_capture(CAPTURE_SET_KEY); break;
        case ID_ADD_KEY: begin_capture(CAPTURE_ADD_KEY); break;
        case ID_SET_PAD: begin_capture(CAPTURE_PAD); break;
        case ID_CLEAR:
        {
            int row = selected_row();
            if (row >= 0)
            {
                s_keys[row].clear();
                if (kActions[row].pad)
                {
                    s_pads[row].clear();
                }
                refresh_binds();
            }
            break;
        }
        case ID_DEFAULTS:
            for (int i = 0; i < kNumActions; i++)
            {
                s_keys[i] = kActions[i].keys;
                s_pads[i] = kActions[i].pad ? kActions[i].pad : "";
            }
            check_set(ID_SWAP_STICKS, false);
            InvalidateRect(item(ID_SWAP_STICKS), NULL, TRUE);
            refresh_binds();
            set_text(ID_CAPTURE, L"All controls are back to their defaults.");
            break;
        case ID_SWAP_STICKS:
            check_set(ID_SWAP_STICKS, !check_get(ID_SWAP_STICKS));
            InvalidateRect(item(ID_SWAP_STICKS), NULL, TRUE);
            refresh_binds();
            break;
        case ID_SAVE: save_settings(); break;
        case ID_PLAY: play(); break;
        }
        return 0;
    case WM_HSCROLL:
        update_distance_labels();
        return 0;
    case WM_TIMER:
        poll_pad_capture();
        return 0;
    case WM_CTLCOLORSTATIC:
    {
        HWND ctl = (HWND)lp;
        int id = GetDlgCtrlID(ctl);
        if (id == ID_STATUS)
        {
            SetTextColor((HDC)wp, kFooterText);
            SetBkColor((HDC)wp, kBg);
            SetBkMode((HDC)wp, TRANSPARENT);
            return (LRESULT)footer_brush();
        }
        wchar_t cls[32] = L"";
        GetClassNameW(ctl, cls, 32);
        bool field = !wcscmp(cls, L"ComboBox") || !wcscmp(cls, L"Edit");
        SetTextColor((HDC)wp, is_note_id(id) ? kMuted : kInk);
        SetBkColor((HDC)wp, field ? kField : kCard);
        SetBkMode((HDC)wp, TRANSPARENT);
        return (LRESULT)(field ? field_brush() : card_brush());
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
        SetTextColor((HDC)wp, kInk);
        SetBkColor((HDC)wp, kField);
        return (LRESULT)field_brush();
    case WM_CTLCOLORBTN:
        SetTextColor((HDC)wp, kInk);
        SetBkColor((HDC)wp, kCard);
        return (LRESULT)card_brush();
    case WM_EXTRACT_PROGRESS:
        SendMessageW(item(ID_PROGRESS), PBM_SETPOS, wp, 0);
        return 0;
    case WM_EXTRACT_DONE:
    {
        std::wstring* err = (std::wstring*)lp;
        extract_done(wp != 0, *err);
        delete err;
        return 0;
    }
    case WM_CLOSE:
        if (s_extracting &&
            MessageBoxW(hwnd, L"The game's files are still being extracted. Stop and quit?", L"Quit",
                        MB_YESNO | MB_ICONQUESTION) != IDYES)
        {
            return 0;
        }
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// scooby_launcher --extract <disc image> [folder]: extracts without the window
// (default folder: data beside the program); exit code 0 when it worked.
static int extract_command(int argc, wchar_t** argv)
{
    AttachConsole(ATTACH_PARENT_PROCESS);
    ExtractJob job;
    job.iso = argv[2];
    job.out = argc > 3 ? std::wstring(argv[3]) : exe_dir() + L"data";
    job.notify = NULL;
    std::wstring err = extract_disc(&job);
    FILE* out = _wfopen(L"CONOUT$", L"w");
    if (out)
    {
        std::string text = err.empty() ? "extracted to " + narrow(job.out) : narrow(err);
        fprintf(out, "%s\n", text.c_str());
        fclose(out);
    }
    return err.empty() ? 0 : 1;
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int show)
{
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv && argc >= 3 && !wcscmp(argv[1], L"--extract"))
    {
        return extract_command(argc, argv);
    }
    SetProcessDPIAware();
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_TAB_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES |
                                                  ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_Init(SDL_INIT_GAMECONTROLLER);

    s_dir = exe_dir();
    s_iniPath = s_dir + L"scooby.ini";
    HDC dc = GetDC(NULL);
    s_dpi = GetDeviceCaps(dc, LOGPIXELSY);
    ReleaseDC(NULL, dc);
    s_font = make_font(9, FW_NORMAL);
    s_titleFont = make_font(24, FW_HEAVY);
    s_pageFont = make_font(17, FW_BOLD);
    s_eyebrowFont = make_font(8, FW_BOLD);
    s_navFont = make_font(10, FW_SEMIBOLD);
    s_smallFont = make_font(8, FW_NORMAL);

    WNDCLASSW wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
    wc.lpszClassName = L"ProjectMysteryMachineLauncher";
    RegisterClassW(&wc);
    RECT r = { 0, 0, px(920), px(700) };
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
    AdjustWindowRect(&r, style, FALSE);
    int ww = r.right - r.left, wh = r.bottom - r.top;
    s_main = CreateWindowExW(0, wc.lpszClassName, L"Project: Mystery Machine - Launcher", style,
                             (GetSystemMetrics(SM_CXSCREEN) - ww) / 2,
                             (GetSystemMetrics(SM_CYSCREEN) - wh) / 2,
                             ww, wh, NULL, NULL, inst, NULL);
    use_dark_titlebar(s_main);
    create_controls();
    s_ini.load(s_iniPath); // the resolution list includes the saved one
    fill_resolutions();
    load_settings();
    refresh_binds();
    update_game_status();
    // The first page: the game files until there are some (--page <n> picks one).
    int page = is_game_dir(find_game_dir()) ? PAGE_DISPLAY : PAGE_GAME;
    if (argv && argc >= 3 && !wcscmp(argv[1], L"--page"))
    {
        page = _wtoi(argv[2]) % PAGE_COUNT;
    }
    show_page(page);
    if (!file_exists(s_dir + L"scooby.exe"))
    {
        status(L"scooby.exe isn't next to the launcher.");
    }
    ShowWindow(s_main, show);
    UpdateWindow(s_main);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        if (capture_key(msg))
        {
            continue;
        }
        // The Play button is owner-drawn, so it is no longer a dialog default
        // button: keep Enter as the shortcut for Play unless a combo box is open.
        if (!s_extracting && msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN)
        {
            wchar_t cls[32] = L"";
            HWND focus = GetFocus();
            if (focus)
            {
                GetClassNameW(focus, cls, 32);
            }
            if (wcscmp(cls, L"ComboBox"))
            {
                play();
                continue;
            }
        }
        if (!IsDialogMessageW(s_main, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    if (s_pad)
    {
        SDL_GameControllerClose(s_pad);
    }
    SDL_Quit();
    return 0;
}

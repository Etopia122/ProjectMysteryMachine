// Scooby-Doo! Night of 100 Frights PC launcher.
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
        fputs("; Scooby-Doo! Night of 100 Frights (PC) settings, written by the launcher.\n"
              "; Key names are SDL's; controller inputs are SDL game controller buttons or axes.\n",
              f);
        static const char* order[] = { "game", "display", "graphics", "keyboard", "controller" };
        std::vector<std::string> secs(order, order + 5);
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
    ID_TABS = 100,
    // Game files
    ID_GAME_INFO, ID_ISO_LABEL, ID_ISO_PATH, ID_ISO_BROWSE, ID_EXTRACT, ID_PROGRESS, ID_GAME_STATUS, ID_GAME_FOLDER,
    // Display
    ID_RES_LABEL, ID_RES, ID_MODE_LABEL, ID_MODE, ID_ASPECT_LABEL, ID_ASPECT, ID_VSYNC, ID_MSAA_LABEL, ID_MSAA,
    ID_DISPLAY_NOTE,
    // Graphics
    ID_DRAW_LABEL, ID_DRAW, ID_DRAW_VALUE, ID_DRAW_NOTE, ID_FX_LABEL, ID_FX, ID_FX_VALUE, ID_FX_NOTE, ID_FILTER_LABEL,
    ID_FILTER, ID_FILTER_NOTE,
    // Controls
    ID_BINDS, ID_SET_KEY, ID_ADD_KEY, ID_SET_PAD, ID_CLEAR, ID_DEFAULTS, ID_SWAP_STICKS, ID_CAPTURE,
    // Bottom
    ID_STATUS, ID_SAVE, ID_PLAY,
};

enum { PAGE_GAME, PAGE_DISPLAY, PAGE_GRAPHICS, PAGE_CONTROLS, PAGE_COUNT };

struct Control
{
    int id, page;
    HWND hwnd;
};

static HWND s_main;
static HFONT s_font;
static int s_dpi = 96;
static std::vector<Control> s_controls;
static Ini s_ini;
static std::wstring s_dir, s_iniPath;
static bool s_extracting;
static std::vector<std::pair<int, int>> s_resolutions;

// Remapping state: the row being captured, and how.
enum { CAPTURE_NONE, CAPTURE_SET_KEY, CAPTURE_ADD_KEY, CAPTURE_PAD };
static int s_capture = CAPTURE_NONE, s_captureRow = -1;
static SDL_GameController* s_pad;
static std::vector<std::string> s_keys(kNumActions), s_pads(kNumActions);
static Uint8 s_padBase[SDL_CONTROLLER_BUTTON_MAX];

static int px(int v) { return MulDiv(v, s_dpi, 96); }

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

static HWND add(int page, int id, const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y, int w, int h,
                DWORD ex = 0)
{
    HWND hwnd = CreateWindowExW(ex, cls, text, WS_CHILD | style, px(x), px(y), px(w), px(h), s_main,
                                (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    SendMessageW(hwnd, WM_SETFONT, (WPARAM)s_font, TRUE);
    Control c = { id, page, hwnd };
    s_controls.push_back(c);
    return hwnd;
}

static void set_text(int id, const std::wstring& t) { SetWindowTextW(item(id), t.c_str()); }

static void status(const std::wstring& t) { set_text(ID_STATUS, t); }

static void show_page(int page)
{
    for (const Control& c : s_controls)
    {
        if (c.page >= 0)
        {
            ShowWindow(c.hwnd, c.page == page ? SW_SHOW : SW_HIDE);
        }
    }
}

static int combo_add(int id, const wchar_t* text)
{
    return (int)SendMessageW(item(id), CB_ADDSTRING, 0, (LPARAM)text);
}
static int combo_get(int id) { return (int)SendMessageW(item(id), CB_GETCURSEL, 0, 0); }
static void combo_set(int id, int i) { SendMessageW(item(id), CB_SETCURSEL, i, 0); }
static bool check_get(int id) { return SendMessageW(item(id), BM_GETCHECK, 0, 0) == BST_CHECKED; }
static void check_set(int id, bool on) { SendMessageW(item(id), BM_SETCHECK, on ? BST_CHECKED : BST_UNCHECKED, 0); }

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

static void update_distance_labels()
{
    int ids[2][2] = { { ID_DRAW, ID_DRAW_VALUE }, { ID_FX, ID_FX_VALUE } };
    for (auto& p : ids)
    {
        int v = (int)SendMessageW(item(p[0]), TBM_GETPOS, 0, 0);
        wchar_t t[32];
        swprintf(t, 32, L"%.2gx", v / 4.0);
        set_text(p[1], t);
    }
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
    char t[32];
    snprintf(t, sizeof(t), "%.2f", SendMessageW(item(ID_DRAW), TBM_GETPOS, 0, 0) / 4.0);
    s_ini.set("graphics", "draw_distance", t);
    snprintf(t, sizeof(t), "%.2f", SendMessageW(item(ID_FX), TBM_GETPOS, 0, 0) / 4.0);
    s_ini.set("graphics", "effects_distance", t);
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
        TabCtrl_SetCurSel(item(ID_TABS), PAGE_GAME);
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
    HWND tabs = add(-1, ID_TABS, WC_TABCONTROLW, L"", WS_VISIBLE | WS_CLIPSIBLINGS, 10, 10, 560, 420);
    const wchar_t* names[] = { L"Game files", L"Display", L"Graphics", L"Controls" };
    for (int i = 0; i < PAGE_COUNT; i++)
    {
        TCITEMW ti = {};
        ti.mask = TCIF_TEXT;
        ti.pszText = (LPWSTR)names[i];
        TabCtrl_InsertItem(tabs, i, &ti);
    }
    const int L = 26, T = 48; // page origin inside the tab control

    // Game files
    add(PAGE_GAME, ID_GAME_INFO, L"STATIC",
        L"The game's files come from your own copy of Scooby-Doo! Night of 100 Frights for GameCube (the USA "
        L"release, disc ID GIHE78). Choose a disc image you made from it (.iso or .gcm) and the launcher copies "
        L"the game's files into the data folder beside the program. You only need to do this once.",
        0, L, T, 528, 64);
    add(PAGE_GAME, ID_ISO_LABEL, L"STATIC", L"Disc image:", 0, L, T + 80, 100, 20);
    add(PAGE_GAME, ID_ISO_PATH, L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | ES_READONLY, L, T + 100, 420, 24,
        WS_EX_CLIENTEDGE);
    add(PAGE_GAME, ID_ISO_BROWSE, L"BUTTON", L"Browse...", WS_TABSTOP | BS_PUSHBUTTON, L + 428, T + 99, 100, 26);
    HWND ex = add(PAGE_GAME, ID_EXTRACT, L"BUTTON", L"Extract game files", WS_TABSTOP | BS_PUSHBUTTON, L, T + 136,
                  160, 30);
    EnableWindow(ex, FALSE);
    HWND prog = add(PAGE_GAME, ID_PROGRESS, PROGRESS_CLASSW, L"", 0, L + 170, T + 140, 358, 22);
    SendMessageW(prog, PBM_SETRANGE32, 0, 1000);
    add(PAGE_GAME, ID_GAME_STATUS, L"STATIC", L"", 0, L, T + 184, 528, 60);
    add(PAGE_GAME, ID_GAME_FOLDER, L"BUTTON", L"Use a folder I already extracted...", WS_TABSTOP | BS_PUSHBUTTON, L,
        T + 254, 240, 28);

    // Display
    add(PAGE_DISPLAY, ID_RES_LABEL, L"STATIC", L"Resolution:", 0, L, T + 4, 150, 20);
    add(PAGE_DISPLAY, ID_RES, WC_COMBOBOXW, L"", WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, L + 160, T, 220, 300);
    add(PAGE_DISPLAY, ID_MODE_LABEL, L"STATIC", L"Window mode:", 0, L, T + 44, 150, 20);
    add(PAGE_DISPLAY, ID_MODE, WC_COMBOBOXW, L"", WS_TABSTOP | CBS_DROPDOWNLIST, L + 160, T + 40, 220, 200);
    add(PAGE_DISPLAY, ID_ASPECT_LABEL, L"STATIC", L"Aspect ratio:", 0, L, T + 84, 150, 20);
    add(PAGE_DISPLAY, ID_ASPECT, WC_COMBOBOXW, L"", WS_TABSTOP | CBS_DROPDOWNLIST, L + 160, T + 80, 220, 200);
    add(PAGE_DISPLAY, ID_MSAA_LABEL, L"STATIC", L"Anti-aliasing:", 0, L, T + 124, 150, 20);
    add(PAGE_DISPLAY, ID_MSAA, WC_COMBOBOXW, L"", WS_TABSTOP | CBS_DROPDOWNLIST, L + 160, T + 120, 220, 200);
    add(PAGE_DISPLAY, ID_VSYNC, L"BUTTON", L"Vertical sync (no tearing)", WS_TABSTOP | BS_AUTOCHECKBOX, L + 160,
        T + 160, 300, 22);
    add(PAGE_DISPLAY, ID_DISPLAY_NOTE, L"STATIC",
        L"Widescreen shows more of each level at the sides without stretching the picture. The title screen, "
        L"menus and movies keep their original 4:3 shape. Borderless fullscreen uses your desktop's "
        L"resolution; Alt+Enter switches between a window and fullscreen while playing.",
        0, L, T + 204, 528, 80);
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

    // Graphics
    add(PAGE_GRAPHICS, ID_DRAW_LABEL, L"STATIC", L"Draw distance:", 0, L, T + 6, 150, 20);
    HWND draw = add(PAGE_GRAPHICS, ID_DRAW, TRACKBAR_CLASSW, L"", WS_TABSTOP | TBS_AUTOTICKS, L + 160, T, 300, 30);
    add(PAGE_GRAPHICS, ID_DRAW_VALUE, L"STATIC", L"", 0, L + 470, T + 6, 60, 20);
    add(PAGE_GRAPHICS, ID_DRAW_NOTE, L"STATIC",
        L"How far you can see: pushes the far edge of the view and the fog back. Some places were built "
        L"expecting fog to hide what lies beyond.",
        0, L + 160, T + 34, 368, 44);
    add(PAGE_GRAPHICS, ID_FX_LABEL, L"STATIC", L"Effects distance:", 0, L, T + 96, 150, 20);
    HWND fx = add(PAGE_GRAPHICS, ID_FX, TRACKBAR_CLASSW, L"", WS_TABSTOP | TBS_AUTOTICKS, L + 160, T + 90, 300, 30);
    add(PAGE_GRAPHICS, ID_FX_VALUE, L"STATIC", L"", 0, L + 470, T + 96, 60, 20);
    add(PAGE_GRAPHICS, ID_FX_NOTE, L"STATIC",
        L"How far away particle effects (smoke, sparkles, rain) still show. The game has no model levels of "
        L"detail; this and draw distance are its distance settings.",
        0, L + 160, T + 124, 368, 44);
    add(PAGE_GRAPHICS, ID_FILTER_LABEL, L"STATIC", L"Texture filtering:", 0, L, T + 190, 150, 20);
    add(PAGE_GRAPHICS, ID_FILTER, WC_COMBOBOXW, L"", WS_TABSTOP | CBS_DROPDOWNLIST, L + 160, T + 186, 220, 200);
    add(PAGE_GRAPHICS, ID_FILTER_NOTE, L"STATIC",
        L"Trilinear and anisotropic filtering keep distant and sloped textures sharp without shimmering.", 0,
        L + 160, T + 218, 368, 40);
    for (HWND tb : { draw, fx })
    {
        SendMessageW(tb, TBM_SETRANGE, TRUE, MAKELPARAM(4, 16));
        SendMessageW(tb, TBM_SETTICFREQ, 2, 0);
        SendMessageW(tb, TBM_SETPAGESIZE, 0, 2);
    }
    for (const auto& f : kFilters)
    {
        combo_add(ID_FILTER, f.label);
    }

    // Controls
    HWND lv = add(PAGE_CONTROLS, ID_BINDS, WC_LISTVIEWW, L"",
                  WS_TABSTOP | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, L, T, 528, 230);
    ListView_SetExtendedListViewStyle(lv, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    const wchar_t* cols[] = { L"Action", L"Keyboard", L"Controller" };
    const int widths[] = { 170, 190, 145 };
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
    add(PAGE_CONTROLS, ID_SET_KEY, L"BUTTON", L"Set key", WS_TABSTOP | BS_PUSHBUTTON, L, T + 238, 96, 28);
    add(PAGE_CONTROLS, ID_ADD_KEY, L"BUTTON", L"Add key", WS_TABSTOP | BS_PUSHBUTTON, L + 102, T + 238, 96, 28);
    add(PAGE_CONTROLS, ID_SET_PAD, L"BUTTON", L"Set button", WS_TABSTOP | BS_PUSHBUTTON, L + 204, T + 238, 96, 28);
    add(PAGE_CONTROLS, ID_CLEAR, L"BUTTON", L"Clear", WS_TABSTOP | BS_PUSHBUTTON, L + 306, T + 238, 80, 28);
    add(PAGE_CONTROLS, ID_DEFAULTS, L"BUTTON", L"Defaults", WS_TABSTOP | BS_PUSHBUTTON, L + 392, T + 238, 136, 28);
    add(PAGE_CONTROLS, ID_SWAP_STICKS, L"BUTTON", L"Swap analog sticks (right stick moves, left stick is the camera)",
        WS_TABSTOP | BS_AUTOCHECKBOX, L, T + 274, 528, 22);
    add(PAGE_CONTROLS, ID_CAPTURE, L"STATIC",
        L"Pick an action, then Set key, Add key (for a second key) or Set button (controller). Double-click "
        L"sets a key.",
        0, L, T + 302, 528, 40);

    // Bottom
    add(-1, ID_STATUS, L"STATIC", L"", WS_VISIBLE, 14, 446, 330, 20);
    add(-1, ID_SAVE, L"BUTTON", L"Save settings", WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 350, 440, 110, 32);
    add(-1, ID_PLAY, L"BUTTON", L"Play", WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 466, 440, 104, 32);
}

// ---------------------------------------------------------------- window procedure
static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_NOTIFY:
    {
        NMHDR* nm = (NMHDR*)lp;
        if (nm->idFrom == ID_TABS && nm->code == TCN_SELCHANGE)
        {
            show_page(TabCtrl_GetCurSel(nm->hwndFrom));
        }
        else if (nm->idFrom == ID_BINDS && nm->code == NM_DBLCLK)
        {
            begin_capture(CAPTURE_SET_KEY);
        }
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case ID_ISO_BROWSE: browse_iso(); break;
        case ID_EXTRACT: start_extract(); break;
        case ID_GAME_FOLDER: choose_game_folder(); break;
        case ID_MODE:
            if (HIWORD(wp) == CBN_SELCHANGE)
            {
                update_mode_dependent();
            }
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
            refresh_binds();
            set_text(ID_CAPTURE, L"All controls are back to their defaults.");
            break;
        case ID_SWAP_STICKS: refresh_binds(); break;
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
        // Labels on the tab pages sit on its white body.
        HWND ctl = (HWND)lp;
        if (GetDlgCtrlID(ctl) != ID_STATUS)
        {
            SetBkMode((HDC)wp, TRANSPARENT);
            return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
        }
        break;
    }
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
    NONCLIENTMETRICSW ncm = {};
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    s_font = CreateFontIndirectW(&ncm.lfMessageFont);

    WNDCLASSW wc = {};
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
    wc.lpszClassName = L"ScoobyLauncher";
    RegisterClassW(&wc);
    RECT r = { 0, 0, px(580), px(484) };
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&r, style, FALSE);
    s_main = CreateWindowExW(0, wc.lpszClassName, L"Scooby-Doo! Night of 100 Frights - Launcher", style,
                             CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, NULL, NULL, inst, NULL);
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
    TabCtrl_SetCurSel(item(ID_TABS), page);
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

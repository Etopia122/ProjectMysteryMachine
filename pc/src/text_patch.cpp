// Text the ports change as the game loads it (TEXT assets, from
// PKR_xform_asset): the ports have no Nintendo GameCube memory cards (saves are
// files, with no card to choose), and the New 3DS has its own buttons (the
// GameCube's Z is ZR there).
#include "pc_platform.h"

#include <stdint.h>
#include <string.h>
#include <string>
#include <unordered_map>

namespace {

struct Rule
{
    const char* from;
    const char* to;
    bool n3dsOnly;
};

// Whole texts, as they are in the game.
const Rule s_texts[] = {
    { "Insert a Nintendo GameCube^ Memory Card.", "No save data was found.", false },
    { "No Scooby games were found.  Please insert another Nintendo GameCube^ Memory Card.",
      "No saved games were found.", false },
    { "No game files were found on the Nintendo GameCube^ Memory Card in Memory Card Slot $VMCSelectedCard$.",
      "No saved games were found.", false },
    { "WARNING:  Accessing Memory Card.  Do not remove.", "Accessing save data.  Please do not turn off the power.",
      false },
    { "Are you sure you wish to format Nintendo GameCube^ Memory Card in Memory Card Slot $VMCSelectedCard$?  Press "
      "$D#$C to confirm.",
      "Are you sure you wish to erase all save data?  Press $D#$C to confirm.", false },
    { "Are you sure you wish to format Nintendo GameCube^ Memory Card in Memory Card Slot $VMCSelectedCard$? ",
      "Are you sure you wish to erase all save data? ", false },
    { "Scooby-Doo game already exists in Memory Card Slot $VMCSelectedCard$. Overwrite previous data?",
      "A saved game already exists in this slot. Overwrite previous data?", false },
    { "$A255000000Format failed!$C\r\nThe Nintendo GameCube^ Memory Card in Slot $VMCSelectedCard$ is damaged and "
      "cannot be used.",
      "$A255000000Erasing failed!$C\r\nThe save data is damaged and cannot be used.", false },
    { "There is insufficient space on the Memory Card.  Do you want to start? $D#$C-Yes  $D}$C-No\r\n",
      "There is not enough space to save.  Do you want to start? $D#$C-Yes  $D}$C-No\r\n", false },
    { "Memory Card Slot A", "Save Data", false },
    { "Memory Card Slot B", "Save Data", false },
    { "Configure Controller", "Configure Controls", true },
    { "Press $D#$C to Configure Controller", "Press $D#$C to Configure Controls", true },
    { "Controller Configuration", "Control Configuration", true },
};

// Parts of any text (after the whole-text rules).
const Rule s_parts[] = {
    { "Z + R Button", "ZR + R Buttons", true },
    { "Z Button", "ZR Button", true },
};

// Patched texts by original, kept for the run (assets point at them).
std::unordered_map<std::string, std::string>& s_patched()
{
    static std::unordered_map<std::string, std::string> m;
    return m;
}

} // namespace

// asset: a TEXT asset (u32 length including the terminating NUL, then the
// text) already in host byte order. Returns a replacement asset, or NULL.
extern "C" void* pc_text_patch(const void* asset)
{
    uint32_t len;
    memcpy(&len, asset, 4);
    if (len == 0 || len > 4096)
    {
        return NULL;
    }
    const char* text = (const char*)asset + 4;
    std::string orig(text, strnlen(text, len));
    std::unordered_map<std::string, std::string>& patched = s_patched();
    std::unordered_map<std::string, std::string>::iterator it = patched.find(orig);
    if (it != patched.end())
    {
        return it->second.empty() ? NULL : (void*)it->second.data();
    }
    bool n3ds = pc_platform_3ds() != 0;
    std::string s = orig;
    for (const Rule& r : s_texts)
    {
        if ((!r.n3dsOnly || n3ds) && s == r.from)
        {
            s = r.to;
            break;
        }
    }
    for (const Rule& r : s_parts)
    {
        if (r.n3dsOnly && !n3ds)
        {
            continue;
        }
        size_t from = 0, at;
        while ((at = s.find(r.from, from)) != std::string::npos)
        {
            s.replace(at, strlen(r.from), r.to);
            from = at + strlen(r.to);
        }
    }
    std::string& out = patched[orig];
    if (s == orig)
    {
        return NULL; // out stays empty: unchanged
    }
    uint32_t n = (uint32_t)s.size() + 1;
    out.assign((const char*)&n, 4);
    out.append(s);
    out.push_back('\0');
    return (void*)out.data();
}

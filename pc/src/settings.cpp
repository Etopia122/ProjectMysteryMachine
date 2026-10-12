// Player settings: scooby.ini in the program's folder (pc_base_path), which
// the launcher writes. Plain "[section]" headers and "key = value" lines;
// settings are looked up as "section.key". A missing file or key gives the
// caller's default, so the game runs the same without one. PC_SETTINGS=<file>
// reads another file instead.
#include "pc_platform.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>

static std::map<std::string, std::string>& settings()
{
    static std::map<std::string, std::string> s_map;
    static bool s_loaded;
    if (s_loaded)
    {
        return s_map;
    }
    s_loaded = true;
    // PC_SETTINGS=<file> reads another file (test runs).
    const char* env = getenv("PC_SETTINGS");
    std::string path = env ? std::string(env) : std::string(pc_base_path()) + "scooby.ini";
    FILE* f = fopen(path.c_str(), "r");
    if (!f)
    {
        return s_map;
    }
    char line[512];
    std::string section;
    while (fgets(line, sizeof(line), f))
    {
        char* s = line;
        while (isspace((unsigned char)*s))
        {
            s++;
        }
        char* end = s + strlen(s);
        while (end > s && isspace((unsigned char)end[-1]))
        {
            *--end = 0;
        }
        if (!*s || *s == ';' || *s == '#')
        {
            continue;
        }
        if (*s == '[')
        {
            char* close = strchr(s, ']');
            section.assign(s + 1, close ? close : end);
            continue;
        }
        char* eq = strchr(s, '=');
        if (!eq)
        {
            continue;
        }
        char* k = eq;
        while (k > s && isspace((unsigned char)k[-1]))
        {
            k--;
        }
        char* v = eq + 1;
        while (isspace((unsigned char)*v))
        {
            v++;
        }
        std::string key = section.empty() ? std::string(s, k) : section + "." + std::string(s, k);
        for (char& c : key)
        {
            c = (char)tolower((unsigned char)c);
        }
        s_map[key] = v;
    }
    fclose(f);
    pc_log("[settings] %s: %d settings", path.c_str(), (int)s_map.size());
    return s_map;
}

extern "C" const char* pc_setting_str(const char* key, const char* def)
{
    std::map<std::string, std::string>& m = settings();
    std::map<std::string, std::string>::const_iterator it = m.find(key);
    return it != m.end() ? it->second.c_str() : def;
}

extern "C" int pc_setting_int(const char* key, int def)
{
    const char* v = pc_setting_str(key, NULL);
    return v && *v ? atoi(v) : def;
}

extern "C" float pc_setting_float(const char* key, float def)
{
    const char* v = pc_setting_str(key, NULL);
    return v && *v ? (float)atof(v) : def;
}

// Pictures the New 3DS port shows that come from the player's own art, not the
// repository: converted by n3ds/tools/make_bottom_bg.py ("SBG1": u32 width,
// u32 height, RGBA), read from the CIA's RomFS or else sdmc:/scooby.
//   title_bg.rgba   the title screen's backdrop, 400x240 for the whole top
//                   screen (in place of the game's 4:3 "text_title")
//   bottom_bg.rgba  the bottom screen's background (gpu_c3d.cpp)
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_platform.h"

static unsigned char* load_file(const char* path, int* w, int* h)
{
    FILE* f = fopen(path, "rb");
    if (!f)
    {
        return NULL;
    }
    char magic[4];
    uint32_t wh[2] = { 0, 0 };
    unsigned char* px = NULL;
    bool ok = fread(magic, 1, 4, f) == 4 && !memcmp(magic, "SBG1", 4) && fread(wh, 4, 2, f) == 2 && wh[0] > 0 &&
              wh[1] > 0 && wh[0] <= 1024 && wh[1] <= 1024 && (px = (unsigned char*)malloc((size_t)wh[0] * wh[1] * 4)) &&
              fread(px, 4, (size_t)wh[0] * wh[1], f) == (size_t)wh[0] * wh[1];
    fclose(f);
    if (!ok)
    {
        free(px);
        pc_log("[n3ds] %s: not a picture from make_bottom_bg.py", path);
        return NULL;
    }
    *w = (int)wh[0];
    *h = (int)wh[1];
    pc_log("[n3ds] picture %s (%dx%d)", path, *w, *h);
    return px;
}

// A picture by file name (malloc'd RGBA), from the RomFS or the SD card.
extern "C" unsigned char* n3ds_load_picture(const char* file, int* w, int* h)
{
    char path[96];
    snprintf(path, sizeof(path), "romfs:/%s", file);
    if (unsigned char* px = load_file(path, w, h))
    {
        return px;
    }
    snprintf(path, sizeof(path), "sdmc:/scooby/%s", file);
    return load_file(path, w, h);
}

extern "C" unsigned char* pc_texture_replace(const char* name, int* w, int* h, int* fullscreen)
{
    if (name && !strcmp(name, "text_title"))
    {
        // The title screen's backdrop: drawn over the whole top screen.
        unsigned char* px = n3ds_load_picture("title_bg.rgba", w, h);
        *fullscreen = px != NULL;
        return px;
    }
    return NULL;
}

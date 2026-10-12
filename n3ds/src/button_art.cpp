// New 3DS button art in place of the GameCube's: the textures that picture the
// GameCube's buttons get round 3DS-style buttons (light grey, dark letter, as
// the 3DS's own menus show them) drawn over them as they load
// (pc_texture_patch, from the RenderWare texture loader and iFontInit).
#include <math.h>
#include <string.h>

#include "pc_platform.h"

namespace {

struct Seg
{
    float x0, y0, x1, y1;
};

// Letters as strokes in a unit box (x right, y down).
const int MAX_SEGS = 40;
struct Glyph
{
    Seg s[MAX_SEGS];
    int n;
};

void add(Glyph& g, float x0, float y0, float x1, float y1)
{
    if (g.n < MAX_SEGS)
    {
        g.s[g.n++] = Seg{ x0, y0, x1, y1 };
    }
}

// Half circle on the right of (cx, cy), radius r, from the top down.
void add_bowl(Glyph& g, float cx, float cy, float r)
{
    const int steps = 10;
    for (int i = 0; i < steps; i++)
    {
        float a0 = -1.5707964f + 3.1415927f * i / steps, a1 = -1.5707964f + 3.1415927f * (i + 1) / steps;
        add(g, cx + r * cosf(a0), cy + r * sinf(a0), cx + r * cosf(a1), cy + r * sinf(a1));
    }
}

Glyph glyph(char c)
{
    Glyph g;
    g.n = 0;
    switch (c)
    {
    case 'A':
        add(g, 0.18f, 0.95f, 0.5f, 0.05f);
        add(g, 0.5f, 0.05f, 0.82f, 0.95f);
        add(g, 0.31f, 0.63f, 0.69f, 0.63f);
        break;
    case 'B':
        add(g, 0.25f, 0.05f, 0.25f, 0.95f);
        add(g, 0.25f, 0.05f, 0.52f, 0.05f);
        add_bowl(g, 0.52f, 0.27f, 0.22f);
        add(g, 0.52f, 0.49f, 0.25f, 0.49f);
        add(g, 0.25f, 0.49f, 0.55f, 0.49f);
        add_bowl(g, 0.55f, 0.72f, 0.23f);
        add(g, 0.55f, 0.95f, 0.25f, 0.95f);
        break;
    case 'X':
        add(g, 0.2f, 0.05f, 0.8f, 0.95f);
        add(g, 0.8f, 0.05f, 0.2f, 0.95f);
        break;
    case 'Y':
        add(g, 0.2f, 0.05f, 0.5f, 0.5f);
        add(g, 0.8f, 0.05f, 0.5f, 0.5f);
        add(g, 0.5f, 0.5f, 0.5f, 0.95f);
        break;
    }
    return g;
}

float seg_dist(float px, float py, const Seg& s)
{
    float dx = s.x1 - s.x0, dy = s.y1 - s.y0;
    float l2 = dx * dx + dy * dy;
    float t = l2 > 0 ? ((px - s.x0) * dx + (py - s.y0) * dy) / l2 : 0;
    t = t < 0 ? 0 : t > 1 ? 1 : t;
    float ex = s.x0 + t * dx - px, ey = s.y0 + t * dy - py;
    return sqrtf(ex * ex + ey * ey);
}

void blend(unsigned char* p, float r, float g, float b, float a)
{
    if (a <= 0)
    {
        return;
    }
    float da = p[3] / 255.0f;
    float oa = a + da * (1 - a);
    if (oa <= 0)
    {
        return;
    }
    p[0] = (unsigned char)((r * a + p[0] * da * (1 - a)) / oa + 0.5f);
    p[1] = (unsigned char)((g * a + p[1] * da * (1 - a)) / oa + 0.5f);
    p[2] = (unsigned char)((b * a + p[2] * da * (1 - a)) / oa + 0.5f);
    p[3] = (unsigned char)(oa * 255 + 0.5f);
}

// A round button with its letter in the size x size square at (x0, y0),
// 4x4 samples a pixel.
void draw_button(unsigned char* rgba, int w, int h, int x0, int y0, int size, char letter)
{
    Glyph gl = glyph(letter);
    const float R = 0.47f, ring = 0.07f, stroke = 0.062f;
    const float lx = 0.29f, ly = 0.27f, ls = 0.46f; // the letter's box in the button
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            int px = x0 + x, py = y0 + y;
            if (px < 0 || py < 0 || px >= w || py >= h)
            {
                continue;
            }
            float body = 0, rim = 0, ink = 0;
            for (int sy = 0; sy < 4; sy++)
            {
                for (int sx = 0; sx < 4; sx++)
                {
                    float u = (x + (sx + 0.5f) / 4) / size, v = (y + (sy + 0.5f) / 4) / size;
                    float d = sqrtf((u - 0.5f) * (u - 0.5f) + (v - 0.5f) * (v - 0.5f));
                    if (d > R)
                    {
                        continue;
                    }
                    if (d > R - ring)
                    {
                        rim += 1;
                        continue;
                    }
                    float gu = (u - lx) / ls, gv = (v - ly) / ls;
                    float best = 1e9f;
                    for (int i = 0; i < gl.n; i++)
                    {
                        float sd = seg_dist(gu, gv, gl.s[i]);
                        best = sd < best ? sd : best;
                    }
                    if (best * ls < stroke)
                    {
                        ink += 1;
                    }
                    else
                    {
                        body += 1;
                    }
                }
            }
            unsigned char* p = rgba + ((size_t)py * w + px) * 4;
            // Body shaded from light at the top to a little darker below.
            float shade = 242 - 40.0f * y / size;
            blend(p, 120, 120, 124, rim / 16);
            blend(p, shade, shade, shade + 2, body / 16);
            blend(p, 58, 58, 62, ink / 16);
        }
    }
}

void fill(unsigned char* rgba, int w, int h, int x0, int y0, int x1, int y1, unsigned char r, unsigned char g,
          unsigned char b, unsigned char a)
{
    for (int y = y0; y < y1 && y < h; y++)
    {
        for (int x = x0; x < x1 && x < w; x++)
        {
            unsigned char* p = rgba + ((size_t)y * w + x) * 4;
            p[0] = r, p[1] = g, p[2] = b, p[3] = a;
        }
    }
}

} // namespace

extern "C" void pc_texture_patch(const char* name, unsigned char* rgba, int w, int h)
{
    if (!name || !rgba)
    {
        return;
    }
    if ((!strcmp(name, "buttons_all") || !strcmp(name, "buttons_all_mnu3")) && w == h && w >= 16)
    {
        // Quadrants: Y X over A B (iFontDrawButton's button glyphs, the menus' labels).
        int q = w / 2;
        fill(rgba, w, h, 0, 0, w, h, 0, 0, 0, 0);
        draw_button(rgba, w, h, 0, 0, q, 'Y');
        draw_button(rgba, w, h, q, 0, q, 'X');
        draw_button(rgba, w, h, 0, q, q, 'A');
        draw_button(rgba, w, h, q, q, q, 'B');
    }
    else if (!strcmp(name, "text_labels2") && w == 128 && h == 128)
    {
        // The main menu's "Exit" (Y) and "Choose" (A) labels have the buttons in
        // their artwork, at the left of each line, on a flat panel colour.
        fill(rgba, w, h, 0, 0, 17, 17, 178, 174, 156, 255);
        fill(rgba, w, h, 0, 17, 17, 34, 178, 174, 156, 255);
        draw_button(rgba, w, h, 1, 0, 16, 'Y');
        draw_button(rgba, w, h, 1, 17, 16, 'A');
    }
}

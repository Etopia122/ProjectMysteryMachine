#ifndef XFONT_H
#define XFONT_H

#include "iDraw.h"

class RwTexture;
class RwRaster;

class iFontSpacing
{
public:
    unsigned char offset;
    unsigned char width;
};

class iFontInfo
{
public:
    RwTexture* texture; // 0x0
    RwRaster* raster;   // 0x4
};

class xFontInfo
{
public:
    unsigned short id;          // 0x0
    unsigned short flags;       // 0x2
    unsigned short char_width;  // 0x4
    unsigned short char_height; // 0x6
    float oo_tex_width;         // 0x8
    float oo_tex_height;        // 0xC
    float u;                    // 0x10
    float v;                    // 0x14
    unsigned short layout;      // 0x18
    unsigned short pad;         // 0x1A
    unsigned char lookup[128];  // 0x1C
    iFontSpacing pspc[128];     // 0x9C
    char* characters;           // 0x19C
    iFontInfo ps;               // 0x1A0
};

class xFontBox
{
public:
    signed int flags;          // 0x0
    float x;                   // 0x4
    float y;                   // 0x8
    float w;                   // 0xC
    float h;                   // 0x10
    unsigned int font;         // 0x14
    iColor_tag color;          // 0x18
    iColor_tag saved_color;    // 0x1C
    float draw_width;          // 0x20
    float draw_height;         // 0x24
    float x_spacing;           // 0x28
    float y_spacing;           // 0x2C
    float depth;               // 0x30
    float inset_left;          // 0x34
    float inset_right;         // 0x38
    float inset_top;           // 0x3C
    float inset_bottom;        // 0x40
    char* text;                // 0x44
    iColor_tag backdrop_color; // 0x48
};

class xFontGlobals_t
{
public:
    float dpx;          // 0x0
    float dpy;          // 0x4
    unsigned int draw;  // 0x8
    float aspect_ratio; // 0xC
};

extern xFontInfo gFontList[4];
extern xFontBox gBoxList[4];
extern unsigned int gDBfont;
extern xFontGlobals_t gFontGlobals;
extern float gFontWiggle;
extern float gFontWiggleTime;

void xFontInit();
void xFontRender();
xFontBox* xFontBoxCreate(float x, float y, float w, float h, unsigned int id, iColor_tag color);
void xFontBoxSetDrawSize(xFontBox* box, float draw_width, float draw_height);
char* xFontBoxRender(xFontBox* box);
void xFontBoxDestroy(xFontBox* box);
void xFontBoxSetText(xFontBox* box, char* text);
void xFontBoxSetFont(xFontBox* box, unsigned int id);
void xFontBoxSetColor(xFontBox* box, iColor_tag color);
void xFontBoxSetDrawSpacing(xFontBox* box, float x_spacing, float y_spacing);
void xFontBoxSetInset(xFontBox* box, float inset_left, float inset_right, float inset_top,
                      float inset_bottom);
void xFontBoxSetJustification(xFontBox* box, signed int mode);
void xFontBoxSetBackdrop(xFontBox* box, iColor_tag color);

void iFontInit();
void iFontDrawChar(char c, iColor_tag color, xFontBox* box, float depth);
void iFontDrawBackdrop(xFontBox* box);
void iFontRestoreRenderState();
void iFontSetCharRenderState(xFontBox* box);
void iFontSetBackdropRenderState(xFontBox* box);
void iFontSaveRenderState();

#endif

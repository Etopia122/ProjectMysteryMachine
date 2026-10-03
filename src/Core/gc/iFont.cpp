#include "xString.h"
#include "xstransvc.h"
#include "xMath.h"
#include "rwtypes.h"
#include "xFont.h"

extern "C" void* memset(void* dst, int c, unsigned int n);

class RwRaster
{
public:
    RwRaster* parent;          // 0x0
    unsigned char* cpPixels;   // 0x4
    unsigned char* palette;    // 0x8
    signed int width;          // 0xC
    signed int height;         // 0x10
};

class RwTexture
{
public:
    RwRaster* raster; // 0x0
    unsigned char pad4[0x50 - 0x4];
    unsigned int filterAddressing; // 0x50
};

class RwImage
{
public:
    signed int flags;        // 0x0
    signed int width;        // 0x4
    signed int height;       // 0x8
    signed int depth;        // 0xC
    signed int stride;       // 0x10
    unsigned char* cpPixels; // 0x14
    RwRGBA* palette;         // 0x18
};

class RwIm2DVertex
{
public:
    float x;      // 0x0
    float y;      // 0x4
    float z;      // 0x8
    unsigned char r; // 0xC
    unsigned char g; // 0xD
    unsigned char b; // 0xE
    unsigned char a; // 0xF
    float u;      // 0x10
    float v;      // 0x14
    float w;      // 0x18
};

class RxRenderStateVector
{
public:
    unsigned int Flags;      // 0x0
    signed int ShadeMode;    // 0x4
    signed int SrcBlend;     // 0x8
    signed int DestBlend;    // 0xC
    RwRaster* TextureRaster; // 0x10
    signed int AddressModeU; // 0x14
    signed int AddressModeV; // 0x18
    signed int FilterMode;   // 0x1C
    RwRGBA BorderColor;      // 0x20
    signed int FogType;      // 0x24
    RwRGBA FogColor;         // 0x28
    unsigned char* FogTable; // 0x2C
};

class RwCameraClip
{
public:
    unsigned char pad0[0x80];
    float nearPlane; // 0x80
};

class RwGlobals
{
public:
    RwCameraClip* curCamera; // 0x0
};

extern "C" {
extern RwGlobals* RwEngineInstance;
signed int RwRenderStateSet(signed int state, void* value);
signed int RwIm2DRenderPrimitive(signed int primType, RwIm2DVertex* vertices, signed int numVertices);
float RwIm2DGetNearScreenZ();
RwImage* RwImageCreate(signed int width, signed int height, signed int depth);
RwImage* RwImageAllocatePixels(RwImage* image);
RwImage* RwImageSetFromRaster(RwImage* image, RwRaster* raster);
signed int RwImageDestroy(RwImage* image);
signed int fn_8018B9A0(RxRenderStateVector* rsvp);
signed int fn_800CD170();
}


static float sButtonUV[4][2] = {
    { 0.5f, 0.0f },
    { 0.5f, 0.5f },
    { 0.0f, 0.0f },
    { 0.0f, 0.5f },
};

static RwIm2DVertex sCharVerts[120];
static RxRenderStateVector rsv;
static RwIm2DVertex sButtonVerts[4];

static RwIm2DVertex* im2d_vert;
static signed int im2d_vert_ct;
float gFontWiggle;
float gFontWiggleTime;
static float rcz;
static float nsz;
static RwRaster* sRaster;
static RwRaster* sButtonRaster;

void iFontSaveRenderState()
{
    fn_8018B9A0(&rsv);
    rcz = 1.0f / RwEngineInstance->curCamera->nearPlane;
    nsz = RwIm2DGetNearScreenZ();
}

void iFontSetBackdropRenderState(xFontBox*)
{
    RwRenderStateSet(0xE, (void*)0);
    RwRenderStateSet(0xA, (void*)5);
    RwRenderStateSet(0xB, (void*)6);
    RwRenderStateSet(0xC, (void*)1);
    RwRenderStateSet(0x1, (void*)0);
}

void iFontSetCharRenderState(xFontBox* box)
{
    xFontInfo* font = &gFontList[box->font];

    RwRenderStateSet(0xE, (void*)0);
    RwRenderStateSet(0xA, (void*)5);
    RwRenderStateSet(0xB, (void*)6);
    RwRenderStateSet(0xC, (void*)1);
    RwRenderStateSet(0x1, font->ps.raster);
    RwRenderStateSet(0x7, (void*)1);
    RwRenderStateSet(0x8, (void*)0);

    sRaster = font->ps.raster;
    im2d_vert = sCharVerts;
}

void iFontRestoreRenderState()
{
    if (im2d_vert_ct > 0)
    {
        RwIm2DRenderPrimitive(3, im2d_vert, im2d_vert_ct);
        im2d_vert_ct = 0;
    }

    RwRenderStateSet(0xE, (void*)((rsv.Flags >> 4) & 1));
    RwRenderStateSet(0xA, (void*)rsv.SrcBlend);
    RwRenderStateSet(0xB, (void*)rsv.DestBlend);
    RwRenderStateSet(0xC, (void*)((rsv.Flags >> 3) & 1));
    RwRenderStateSet(0x1, rsv.TextureRaster);
    RwRenderStateSet(0x7, (void*)rsv.ShadeMode);
    RwRenderStateSet(0x8, (void*)((rsv.Flags >> 2) & 1));

    im2d_vert = 0;
}

void iFontDrawBackdrop(xFontBox* box)
{
    RwIm2DVertex vert[4];

    vert[0].x = box->x;
    vert[0].y = box->y + box->h;
    vert[0].z = nsz;
    vert[0].r = box->backdrop_color.r;
    vert[0].g = box->backdrop_color.g;
    vert[0].b = box->backdrop_color.b;
    vert[0].a = box->backdrop_color.a;

    vert[1].x = box->x + box->w;
    vert[1].y = box->y + box->h;
    vert[1].z = nsz;
    vert[1].r = box->backdrop_color.r;
    vert[1].g = box->backdrop_color.g;
    vert[1].b = box->backdrop_color.b;
    vert[1].a = box->backdrop_color.a;

    vert[2].x = box->x;
    vert[2].y = box->y;
    vert[2].z = nsz;
    vert[2].r = box->backdrop_color.r;
    vert[2].g = box->backdrop_color.g;
    vert[2].b = box->backdrop_color.b;
    vert[2].a = box->backdrop_color.a;

    vert[3].x = box->x + box->w;
    vert[3].y = box->y;
    vert[3].z = nsz;
    vert[3].r = box->backdrop_color.r;
    vert[3].g = box->backdrop_color.g;
    vert[3].b = box->backdrop_color.b;
    vert[3].a = box->backdrop_color.a;

    RwIm2DRenderPrimitive(4, vert, 4);
}

static void iFontDrawButton(signed int c, float x, float y, float w, float h)
{
    RwIm2DVertex* vert = sButtonVerts;
    float u;
    float v;
    float d = 0.49999991f;

    x += 0.5f * w;
    y += 0.5f * h;
    x -= 0.5f * h;
    y -= 0.5f * h;

    if (fn_800CD170())
    {
        if (c == 'U')
        {
            c = 'V';
        }
        else if (c == 'V')
        {
            c = 'U';
        }
    }

    u = sButtonUV[c - 'T'][0];
    v = sButtonUV[c - 'T'][1];

    RwRenderStateSet(0x1, sButtonRaster);

    vert[0].x = x;
    vert[0].y = y + h;
    vert[0].z = nsz;
    vert[0].u = u;
    vert[0].v = v + d;
    vert[0].r = 0xFF;
    vert[0].g = 0xFF;
    vert[0].b = 0xFF;
    vert[0].a = 0xFF;

    vert[1].x = x + h;
    vert[1].y = y + h;
    vert[1].z = nsz;
    vert[1].u = u + d;
    vert[1].v = v + d;
    vert[1].r = 0xFF;
    vert[1].g = 0xFF;
    vert[1].b = 0xFF;
    vert[1].a = 0xFF;

    vert[2].x = x;
    vert[2].y = y;
    vert[2].z = nsz;
    vert[2].u = u;
    vert[2].v = v;
    vert[2].r = 0xFF;
    vert[2].g = 0xFF;
    vert[2].b = 0xFF;
    vert[2].a = 0xFF;

    vert[3].x = x + h;
    vert[3].y = y;
    vert[3].z = nsz;
    vert[3].u = u + d;
    vert[3].v = v;
    vert[3].r = 0xFF;
    vert[3].g = 0xFF;
    vert[3].b = 0xFF;
    vert[3].a = 0xFF;

    RwIm2DRenderPrimitive(4, sButtonVerts, 4);
    RwRenderStateSet(0x1, sRaster);
}

void iFontDrawChar(char c, iColor_tag color, xFontBox* box, float)
{
    float x;
    float y;
    float dx;
    float dy;
    float u;
    float v;
    float du;
    float dv;
    unsigned int i;
    xFontInfo* font = &gFontList[box->font];
    unsigned int button;

    if ((font->flags & 0x2) && c >= 'a' && c <= 'z')
    {
        c ^= 0x20;
    }

    if (c == '^')
    {
        c = '~';
        button = 0;
        i = font->lookup['~'] & 0x7F;
    }
    else
    {
        i = font->lookup[c];
        if (i == 0xFF)
        {
            return;
        }

        button = i & 0x80;
        i &= 0x7F;
        if (button && box->font == 0)
        {
            i += 0x54;
        }
    }

    u = font->oo_tex_width * (font->u + font->char_width * (i % font->layout));
    v = font->oo_tex_height * (font->v + font->char_height * (i / font->layout));
    du = (font->char_width - 0.5f) * font->oo_tex_width;
    dv = (font->char_height - 0.5f) * font->oo_tex_height;
    x = gFontGlobals.dpx;
    y = gFontGlobals.dpy;
    dx = box->draw_width;
    dy = box->draw_height;

    if (!(font->flags & 0x4))
    {
        dx *= (float)font->pspc[c].width / (float)font->char_width;
        u += font->pspc[c].offset * font->oo_tex_width;
        du = font->pspc[c].width * font->oo_tex_width;
    }

    if (gFontWiggle != 0.0f)
    {
        y += gFontWiggle * icos(10.0f * gFontWiggleTime + (x + y) / 10.0f);
    }

    float sx = font->oo_tex_width * (font->oo_tex_width * box->draw_width);
    float sy = font->oo_tex_height * (font->oo_tex_height * box->draw_height);

    if (x < 0.0f)
    {
        if (x + dx < 0.0f)
        {
            return;
        }

        float n = -x;
        x = 0.0f;
        dx -= n;
        u += n * sx;
        du -= n * sx;
    }
    else if (x + dx >= 640.0f)
    {
        if (x > 640.0f)
        {
            return;
        }

        float n = (x + dx) - 640.0f;
        dx -= n;
        du -= n * sx;
    }

    if (y < 0.0f)
    {
        if (y + dy < 0.0f)
        {
            return;
        }

        float n = -y;
        y = 0.0f;
        dy -= n;
        v += n * sy;
        dv -= n * sy;
    }
    else if (y + dy >= 480.0f)
    {
        if (y > 480.0f)
        {
            return;
        }

        float n = (y + dy) - 480.0f;
        dy -= n;
        dv -= n * sy;
    }

    if (button)
    {
        float _x = x - 0.24f * dx;
        float _y = y - 0.12f * dy;
        float _dx = 0.48f * dx + dx;
        float _dy = 0.32f * dy + dy;
        float wave = 1.0f + isin(5.0f * gFontWiggleTime);
        float pulse_w = 0.025f * _dx * wave;
        float pulse_h = 0.025f * _dy * wave;
        _x -= pulse_w;
        _dx += 2.0f * pulse_w;
        _y -= pulse_h;
        _dy += 2.0f * pulse_h;
        iFontDrawButton(i, _x, _y, _dx, _dy);
        return;
    }

    if (sizeof(sCharVerts) / sizeof(RwIm2DVertex) - im2d_vert_ct < 6)
    {
        RwIm2DRenderPrimitive(3, im2d_vert, im2d_vert_ct);
        im2d_vert_ct = 0;
    }

    RwIm2DVertex* vert = &im2d_vert[im2d_vert_ct];

    vert[0].x = x;
    vert[0].y = y + dy;
    vert[0].z = nsz;
    vert[0].u = u;
    vert[0].v = v + dv;
    vert[0].r = color.r;
    vert[0].g = color.g;
    vert[0].b = color.b;
    vert[0].a = color.a;

    vert[1].x = x + dx;
    vert[1].y = y + dy;
    vert[1].z = nsz;
    vert[1].u = u + du;
    vert[1].v = v + dv;
    vert[1].r = color.r;
    vert[1].g = color.g;
    vert[1].b = color.b;
    vert[1].a = color.a;

    vert[2].x = x;
    vert[2].y = y;
    vert[2].z = nsz;
    vert[2].u = u;
    vert[2].v = v;
    vert[2].r = color.r;
    vert[2].g = color.g;
    vert[2].b = color.b;
    vert[2].a = color.a;

    vert[3] = vert[2];
    vert[4] = vert[1];

    vert[5].x = x + dx;
    vert[5].y = y;
    vert[5].z = nsz;
    vert[5].u = u + du;
    vert[5].v = v;
    vert[5].r = color.r;
    vert[5].g = color.g;
    vert[5].b = color.b;
    vert[5].a = color.a;

    im2d_vert_ct += 6;
}

static unsigned int iFontInitFont(RwTexture* texture, char* characters, unsigned int layout,
                                  unsigned short u, unsigned short v, unsigned short char_width,
                                  unsigned short char_height, unsigned short flags)
{
    char t;
    char* s;
    unsigned int n;
    unsigned int id;
    xFontInfo* font;
    unsigned int tex_width;
    unsigned int tex_height;
    unsigned char val;
    RwImage* image;
    unsigned char* imp;
    unsigned char c;
    unsigned char uu;
    unsigned char vv;
    unsigned char minu;
    unsigned char maxu;
    unsigned int ou;
    unsigned int ov;

    id = 0;
    font = gFontList;
    tex_width = texture->raster->width;
    tex_height = texture->raster->height;

    for (unsigned int n = 4; n != 0; n--)
    {
        if (!font->flags)
        {
            break;
        }
        id++;
        font++;
    }

    font->ps.texture = texture;
    font->ps.texture->filterAddressing = (font->ps.texture->filterAddressing & ~0xFF) | 2;
    font->ps.raster = font->ps.texture->raster;

    memset(font->lookup, 0xFF, sizeof(font->lookup));

    n = 0;
    for (s = characters; (t = *s) != '\0'; s++, n++)
    {
        if ((unsigned char)t < 0x80)
        {
            val = n;
            if (t == '{' || t == '}' || t == '~' || t == '#')
            {
                val |= 0x80;
            }
            font->lookup[t] = val;
        }
    }

    for (t = 0; (unsigned int)t < 0x80; t++)
    {
        font->pspc[t].offset = 0;
        font->pspc[t].width = char_width;
    }

    image = RwImageCreate(256, 256, 32);
    if (image)
    {
        RwImageAllocatePixels(image);
        RwImageSetFromRaster(image, font->ps.raster);

        imp = image->cpPixels;
        for (s = characters; *s != '\0'; s++)
        {
            c = *s;
            val = font->lookup[c] & 0x7F;
            minu = 0xFF;
            maxu = 0;
            ou = u + char_width * (val % layout);
            ov = v + char_height * (val / layout);

            for (vv = 0; vv < char_height; vv++)
            {
                for (uu = 0; uu < char_width; uu++)
                {
                    if (imp[((ov + vv) * 256 + ou + uu) * 4 + 3])
                    {
                        minu = (minu < uu) ? minu : uu;
                        maxu = (maxu > uu) ? maxu : uu;
                    }
                }
            }

            font->pspc[c].offset = minu;
            font->pspc[c].width = maxu + 1 - minu;

            if ((flags & 0x2) && c >= 'A' && c <= 'Z')
            {
                font->pspc[c | 0x20].offset = minu;
                font->pspc[c | 0x20].width = maxu + 1 - minu;
            }
        }

        RwImageDestroy(image);
    }

    font->pspc[' '].offset = 0;
    font->pspc[' '].width = char_width >> 1;
    font->pspc['>'].offset = 0;
    font->pspc['<'].offset = 0;
    font->pspc['>'].width = 18;
    font->pspc['<'].width = 18;

    font->oo_tex_width = 1.0f / tex_width;
    font->oo_tex_height = 1.0f / tex_height;
    font->char_width = char_width;
    font->char_height = char_height;
    font->layout = layout;
    font->characters = characters;
    font->flags |= 0x1;
    font->flags |= flags;
    font->u = u;
    font->v = v;

    return id;
}

void iFontInit()
{
    RwTexture* font_texture = (RwTexture*)xSTFindAsset(xStrHash("font"), 0);

    iFontInitFont(font_texture,
                  "~{}#ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789?!.,-:_\"'&()<>/%", 14, 0, 132,
                  18, 22, 2);
    iFontInitFont(font_texture,
                  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789?!.,;:+-=/&()%\"'_<>*[]~{}#",
                  14, 0, 0, 18, 22, 0);
    gDBfont = iFontInitFont(font_texture, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ,./*+-=:;%<>[]|()_",
                            18, 148, 232, 6, 8, 6);

    RwTexture* button_texture = (RwTexture*)xSTFindAsset(xStrHash("buttons_all"), 0);
    sButtonRaster = button_texture->raster;
}

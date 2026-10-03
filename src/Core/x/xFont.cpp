#include "xFont.h"
#include "xMemMgr.h"

char* xUtil_idtag2string(unsigned int srctag, signed int bufidx);
signed int xSTPreLoadScene(unsigned int sid, void* userdata);
signed int xSTQueueSceneAssets(unsigned int sid);
float xSTLoadStep(unsigned int sid);

xFontInfo gFontList[4];
xFontBox gBoxList[4];
unsigned int gDBfont;
xFontGlobals_t gFontGlobals = { 0 };
static char* blankText = "";

static inline iColor_tag xFontColor(unsigned char r, unsigned char g, unsigned char b,
                                    unsigned char a)
{
    iColor_tag c;
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    return c;
}

void xFontInit()
{
    xMemPushBase();
    xUtil_idtag2string('FONT', 0);
    xSTPreLoadScene('FONT', 0);
    xUtil_idtag2string('FONT', 0);
    xSTQueueSceneAssets('FONT');
    xUtil_idtag2string('FONT', 0);

    while (xSTLoadStep('FONT') < 1.0f)
    {
    }

    xUtil_idtag2string('FONT', 0);

    unsigned int i;
    for (i = 0; i < 4; i++)
    {
        gFontList[i].flags = 0;
    }

    for (i = 0; i < 4; i++)
    {
        gBoxList[i].flags = 0;
    }

    iFontInit();
}

void xFontRender()
{
    unsigned int i;
    xFontBox* box = gBoxList;
    for (i = 0; i < 4; i++)
    {
        if (box->flags & 0x1)
        {
            xFontBoxRender(box);
        }
        box++;
    }
}

void xFontBoxSetDrawSize(xFontBox* box, float draw_width, float draw_height)
{
    box->draw_width = (640.0f * draw_width) / 640.0f;
    box->draw_height = (480.0f * draw_height) / 480.0f;
}

xFontBox* xFontBoxCreate(float x, float y, float w, float h, unsigned int id, iColor_tag color)
{
    xFontBox* box = gBoxList;
    for (unsigned int i = 0; i < 4; i++)
    {
        if (box->flags == 0)
        {
            break;
        }
        box++;
    }

    xFontInfo* font = &gFontList[id];

    box->x = (640.0f * x) / 640.0f;
    box->y = (480.0f * y) / 480.0f;
    box->w = (640.0f * w) / 640.0f;
    box->h = (480.0f * h) / 480.0f;
    box->color = color;
    box->saved_color = color;
    box->font = id;
    box->depth = 1.0f;
    box->inset_left = 4.0f;
    box->inset_right = 4.0f;
    box->inset_top = 4.0f;
    box->inset_bottom = 4.0f;
    box->x_spacing = (float)(font->char_width * 640) / 640.0f;
    box->y_spacing = (float)(font->char_height * 480) / 480.0f;
    box->draw_width = (float)(font->char_width * 640) / 640.0f;
    box->draw_height = (float)(font->char_height * 480) / 480.0f;
    box->flags = 0x11;

    return box;
}

static inline signed int xFontReadNumber(char** s)
{
    signed int n = 0;
    if (**s < '0' || **s > '9')
    {
        n = **s;
    }
    else
    {
        for (signed int i = 0; i < 1; i++)
        {
            n = n * 10 + *(*s)++ - '0';
        }
    }
    return n;
}

static char* ProcessWord(xFontBox* box, char* text, float* width, float* height, float maxw)
{
    char* s;
    float dx;
    unsigned int fontID;
    iColor_tag color;
    xFontInfo* font;

    fontID = box->font;
    font = &gFontList[fontID];
    s = text;
    color = box->color;
    *width = 0.0f;
    *height = 0.0f;

    for (;;)
    {
        if (*s == '$')
        {
            char c = s[1];
            s += 2;
            switch (c)
            {
            case 'B':
            {
                signed int n = xFontReadNumber(&s);
                fontID = n;
                break;
            }
            case 'A':
            {
                signed int r = 0;
                for (signed int i = 0; i < 3; i++)
                {
                    r = r * 10 + *s++ - '0';
                }
                signed int g = 0;
                for (signed int i = 0; i < 3; i++)
                {
                    g = g * 10 + *s++ - '0';
                }
                signed int b = 0;
                for (signed int i = 0; i < 3; i++)
                {
                    b = b * 10 + *s++ - '0';
                }
                color = xFontColor(r, g, b, 0xFF);
                break;
            }
            case 'C':
                color = box->saved_color;
                break;
            case 'D':
            {
                signed int n = xFontReadNumber(&s);

                switch (n)
                {
                case 0:
                case '#':
                    color = xFontColor(0x0C, 0xB3, 0x5F, 0xFF);
                    break;
                case 1:
                case '{':
                    color = xFontColor(0xCD, 0x07, 0x00, 0xFF);
                    break;
                case 2:
                case '}':
                case '~':
                    color = xFontColor(0x99, 0xBD, 0xEE, 0xFF);
                    break;
                default:
                    color = xFontColor(0xFF, 0xFF, 0xFF, 0xFF);
                    break;
                }
                break;
            }
            }
        }

        if (font->flags & 0x4)
        {
            dx = box->x_spacing;
        }
        else
        {
            dx = (unsigned int)(font->pspc[*s].width + 3);
            dx *= box->x_spacing / font->char_width;
        }

        char c = *s;
        if (c == '\n' || c == '@' || c == '\0')
        {
            break;
        }
        if (c == ' ' || c == '\t')
        {
            break;
        }

        if (maxw != 0.0f && *width + dx > maxw)
        {
            break;
        }

        if (gFontGlobals.draw)
        {
            iFontDrawChar(*s, color, box, box->depth);
            gFontGlobals.dpx += dx;
        }

        *width += dx;
        *height = (*height > box->y_spacing) ? *height : box->y_spacing;
        s++;
    }

    if (gFontGlobals.draw)
    {
        box->font = fontID;
        box->color = color;
    }

    return s;
}

static char* JustifyLine(xFontBox* box, char* text, float maxwidth, float maxheight)
{
    char* end;
    float width;
    float height;
    float whitespace_width;
    float testwidth;
    float testheight;
    char* s;
    char* t;
    xFontInfo* font;

    font = &gFontList[box->font];
    if (!text)
    {
        return 0;
    }

    height = 0.0f;
    width = 0.0f;
    gFontGlobals.draw = 0;
    s = end = text;

    while (width <= maxwidth && height <= maxheight && s)
    {
        if (*s == '\0')
        {
            break;
        }
        if (*s == '\n' || *s == '@')
        {
            end = s + 1;
            break;
        }

        whitespace_width = 0.0f;
        while (*s == ' ' || *s == '\t')
        {
            if (font->flags & 0x4)
            {
                if (*s == ' ')
                {
                    whitespace_width += box->x_spacing;
                }
                if (*s == '\t')
                {
                    whitespace_width += 8.0f * box->x_spacing;
                }
            }
            else
            {
                float space = ((unsigned int)(font->pspc[' '].width + 3) * box->x_spacing) / font->char_width;
                if (*s == ' ')
                {
                    whitespace_width += space;
                }
                if (*s == '\t')
                {
                    whitespace_width += 8.0f * space;
                }
            }
            s++;
        }

        if (*s == '\0')
        {
            break;
        }
        if (*s == '\n' || *s == '@')
        {
            end = s + 1;
            break;
        }

        if (width == 0.0f)
        {
            whitespace_width = 0.0f;
        }

        t = ProcessWord(box, s, &testwidth, &testheight, 0.0f);
        if (testwidth + (width + whitespace_width) > maxwidth)
        {
            if (testwidth > maxwidth)
            {
                end = ProcessWord(box, s, &testwidth, &testheight,
                                  maxwidth - (width + whitespace_width));
            }
            break;
        }

        if (testheight > maxheight)
        {
            break;
        }

        width += whitespace_width + testwidth;
        s = end = t;
        height = (height > testheight) ? height : testheight;
    }

    if (end == text)
    {
        return text;
    }

    if (box->flags & 0x20)
    {
        gFontGlobals.dpx += maxwidth - width;
    }

    if (box->flags & 0x40)
    {
        gFontGlobals.dpx += 0.5f * (maxwidth - width);
    }

    width = 0.0f;
    s = text;
    while (s != end)
    {
        if (*s == '\n' || *s == '@')
        {
            break;
        }

        whitespace_width = 0.0f;
        while (*s == ' ' || *s == '\t')
        {
            if (font->flags & 0x4)
            {
                if (*s == ' ')
                {
                    whitespace_width += box->x_spacing;
                }
                if (*s == '\t')
                {
                    whitespace_width += 8.0f * box->x_spacing;
                }
            }
            else
            {
                float space = ((unsigned int)(font->pspc[' '].width + 3) * box->x_spacing) / font->char_width;
                if (*s == ' ')
                {
                    whitespace_width += space;
                }
                if (*s == '\t')
                {
                    whitespace_width += 8.0f * space;
                }
            }
            s++;
        }

        if (width == 0.0f)
        {
            whitespace_width = 0.0f;
        }

        gFontGlobals.draw = 0;
        gFontGlobals.dpx += whitespace_width;
        ProcessWord(box, s, &testwidth, &testheight, 0.0f);
        gFontGlobals.draw = 1;

        if (testwidth + (width + whitespace_width) > maxwidth)
        {
            if (testwidth > maxwidth)
            {
                ProcessWord(box, s, &testwidth, &testheight, maxwidth - (width + whitespace_width));
            }
            break;
        }

        s = ProcessWord(box, s, &testwidth, &testheight, 0.0f);
        width += testwidth + whitespace_width;
    }

    return end;
}

char* xFontBoxRender(xFontBox* box)
{
    char* s;
    char* ret;

    iFontSaveRenderState();

    if (box->flags & 0x100)
    {
        iFontSetBackdropRenderState(box);
        iFontDrawBackdrop(box);
    }

    if (!box->text)
    {
        return 0;
    }

    float minx = box->x + box->inset_left;
    float maxx = box->x + box->w - box->inset_right;
    float miny = box->y + box->inset_top;
    float maxy = box->y + box->h - box->inset_bottom;

    if (maxx < 0.0f || maxy < 0.0f || minx > 640.0f || miny > 480.0f)
    {
        return 0;
    }

    s = box->text;
    gFontGlobals.dpx = minx;
    gFontGlobals.dpy = miny;
    iFontSetCharRenderState(box);

    maxx -= minx;
    while (s)
    {
        if (*s == '\0')
        {
            ret = 0;
            break;
        }

        char* t = JustifyLine(box, s, maxx, maxy - gFontGlobals.dpy);
        if (t == s)
        {
            ret = s;
            break;
        }

        gFontGlobals.dpx = minx;
        s = t;
        gFontGlobals.dpy += box->y_spacing;
    }

    iFontRestoreRenderState();
    return ret;
}

void xFontBoxDestroy(xFontBox* box)
{
    box->flags = 0;
}

void xFontBoxSetText(xFontBox* box, char* text)
{
    if (!text)
    {
        text = blankText;
    }
    box->text = text;
}

void xFontBoxSetFont(xFontBox* box, unsigned int id)
{
    xFontInfo* font = &gFontList[id];
    box->font = id;
    box->x_spacing = (float)(font->char_width * 640) / 640.0f;
    box->y_spacing = (float)(font->char_height * 480) / 480.0f;
}

void xFontBoxSetColor(xFontBox* box, iColor_tag color)
{
    box->color = color;
    box->saved_color = color;
}

void xFontBoxSetDrawSpacing(xFontBox* box, float x_spacing, float y_spacing)
{
    box->x_spacing = (640.0f * x_spacing) / 640.0f;
    box->y_spacing = (480.0f * y_spacing) / 480.0f;
}

void xFontBoxSetInset(xFontBox* box, float inset_left, float inset_right, float inset_top,
                      float inset_bottom)
{
    box->inset_left = (640.0f * inset_left) / 640.0f;
    box->inset_right = (640.0f * inset_right) / 640.0f;
    box->inset_top = (480.0f * inset_top) / 480.0f;
    box->inset_bottom = (480.0f * inset_bottom) / 480.0f;
}

void xFontBoxSetJustification(xFontBox* box, signed int mode)
{
    box->flags &= ~0x10;
    box->flags &= ~0x20;
    box->flags &= ~0x40;
    box->flags |= mode;
}

void xFontBoxSetBackdrop(xFontBox* box, iColor_tag color)
{
    box->flags |= 0x100;
    box->backdrop_color = color;
}

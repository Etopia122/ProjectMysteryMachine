#include "zUI.h"
#include "zGlobals.h"
#include "iDraw.h"
#include "xString.h"
#include "xstransvc.h"

class xFontBox;

xFontBox* xFontBoxCreate(float x, float y, float w, float h, unsigned int id, iColor_tag color);
void xFontBoxSetInset(xFontBox* box, float inset_left, float inset_right, float inset_top,
                      float inset_bottom);
void xFontBoxSetDrawSpacing(xFontBox* box, float x_spacing, float y_spacing);
void xFontBoxSetDrawSize(xFontBox* box, float draw_width, float draw_height);
void xFontBoxSetJustification(xFontBox* box, signed int mode);
void xFontBoxSetBackdrop(xFontBox* box, iColor_tag color);
void xFontBoxSetText(xFontBox* box, char* text);
char* xFontBoxRender(xFontBox* box);
void xFontBoxDestroy(xFontBox* box);
void zVarMagicProcessText(char* in, char* out);
void xEntUpdate(_xEnt* ent, _xScene* sc, float dt);

extern "C" {
// TODO: unidentified save-slot check
unsigned int fn_800C9738(signed int slot);
// TODO: unidentified xFont global (text scale while rendering)
extern float gFontWiggle;
}

void zUIFont_Render(_xEnt* e);
void zUIFont_Update(_zUIFont* ent, _xScene* sc, float dt);
signed int zUIFontEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                          xBase* toParamWidget);

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zUIFont_Init(_zUIFont* ent, p2EntAsset* asset)
{
    unsigned short hack;

    zUI_Init(ent, asset);
    ent->render = zUIFont_Render;
    ent->fasset = (zUIFontAsset*)asset;
    ent->update = (void (*)(_xEnt*, _xScene*, float))zUIFont_Update;
    ent->eventFunc = zUIFontEventCB;
    if (ent->linkCount)
    {
        ent->link = (p2LinkAsset*)(ent->fasset + 1);
    }

    hack = 0;
    ent->uiFontFlags = ent->fasset->uiFontFlags;
    if (ent->id == xStrHash("MNU3 PRESS START"))
    {
        hack |= 1;
    }
    if (ent->id == xStrHash("MNU3 PRESS START"))
    {
        hack |= 2;
    }
    ent->uiFontHackFlags = hack;
    ent->uiButton = 0;
    if (ent->flags & 0x40)
    {
        ent->flags ^= 0x40;
    }

    switch (ent->fasset->mode)
    {
    case 1:
    case 2:
        ent->fasset->pos.x = 50.0f;
        ent->fasset->dim[0] = 540;
    case 3:
    case 4:
    case 5:
    case 6:
        ent->fasset->fontID = 1;
        ent->fasset->color[0] = 255;
        ent->fasset->color[1] = 230;
        ent->fasset->color[2] = 0;
        ent->fasset->color[3] = 255;
        break;
    }
}

void zUIFont_Save(_zUIFont* ent, xSerial* s)
{
    zUI_Save(ent, s);
}

void zUIFont_Load(_zUIFont* ent, xSerial* s)
{
    zUI_Load(ent, s);
}

void zUIFont_PreUpdate(_zUIFont* ent)
{
    ent->uiButton = 0;
    if (globals.pad0 && globals.pad0->pressed && (ent->uiFlags & 8) &&
        ((ent->uiFlags & 2) || (ent->uiFlags & 1)))
    {
        unsigned int pressed = globals.pad0->pressed;
        if (pressed & 0x10)
        {
            ent->uiButton |= 0x10;
        }
        else if (pressed & 0x40)
        {
            ent->uiButton |= 0x40;
        }
        else if (pressed & 0x80)
        {
            ent->uiButton |= 0x80;
        }
        else if (pressed & 0x20)
        {
            ent->uiButton |= 0x20;
        }
        if (globals.pad0->pressed & 1)
        {
            ent->uiButton |= 1;
        }
        if (globals.pad0->pressed & 2)
        {
            ent->uiButton |= 2;
        }
        if (globals.pad0->pressed & 0x1000)
        {
            ent->uiButton |= 0x1000;
        }
        if (globals.pad0->pressed & 0x2000)
        {
            ent->uiButton |= 0x2000;
        }
        if (globals.pad0->pressed & 0x100)
        {
            ent->uiButton |= 0x100;
        }
        if (globals.pad0->pressed & 0x200)
        {
            ent->uiButton |= 0x200;
        }
        if (globals.pad0->pressed & 0x10000)
        {
            ent->uiButton |= 0x10000;
        }
        if (globals.pad0->pressed & 0x20000)
        {
            ent->uiButton |= 0x20000;
        }
        if (globals.pad0->pressed & 0x40000)
        {
            ent->uiButton |= 0x40000;
        }
        if (globals.pad0->pressed & 0x80000)
        {
            ent->uiButton |= 0x80000;
        }
        if (ent->uiButton)
        {
            ent->uiFlags |= 0x40;
        }
    }
}

void zUIFont_Update(_zUIFont* ent, _xScene* sc, float dt)
{
    xEntUpdate(ent, sc, dt);
    if (ent->uiButton)
    {
        if (ent->uiButton & 0x10)
        {
            zEntEvent(ent, ent, 0x49);
            if ((ent->id == xStrHash("LD GAMESLOT 0") || ent->id == xStrHash("LD GAMESLOT 2")) &&
                !fn_800C9738(0) && fn_800C9738(1) && !fn_800C9738(2))
            {
                zEntEvent(ent, ent, 0x4A);
            }
        }
        else if (ent->uiButton & 0x40)
        {
            zEntEvent(ent, ent, 0x4A);
            if ((ent->id == xStrHash("LD GAMESLOT 0") || ent->id == xStrHash("LD GAMESLOT 2")) &&
                !fn_800C9738(0) && fn_800C9738(1) && !fn_800C9738(2))
            {
                zEntEvent(ent, ent, 0x49);
            }
        }
        else if (ent->uiButton & 0x80)
        {
            zEntEvent(ent, ent, 0x4C);
        }
        else if (ent->uiButton & 0x20)
        {
            zEntEvent(ent, ent, 0x4B);
        }
        else if (ent->uiButton & 1)
        {
            zEntEvent(ent, ent, 0x47);
        }
        else if (ent->uiButton & 2)
        {
            zEntEvent(ent, ent, 0x48);
        }
        else if (ent->uiButton & 0x1000)
        {
            zEntEvent(ent, ent, 0x45);
            ent->uiButton = 0x1000;
        }
        else if (ent->uiButton & 0x2000)
        {
            zEntEvent(ent, ent, 0x46);
            ent->uiButton = 0x2000;
        }
        else if (ent->uiButton & 0x100)
        {
            zEntEvent(ent, ent, 0x43);
        }
        else if (ent->uiButton & 0x200)
        {
            zEntEvent(ent, ent, 0x44);
        }
        else if (ent->uiButton & 0x10000)
        {
            zEntEvent(ent, ent, 0x3F);
        }
        else if (ent->uiButton & 0x20000)
        {
            zEntEvent(ent, ent, 0x41);
        }
        else if (ent->uiButton & 0x40000)
        {
            zEntEvent(ent, ent, 0x42);
        }
        else if (ent->uiButton & 0x80000)
        {
            zEntEvent(ent, ent, 0x40);
        }
        ent->uiFlags &= ~0x40;
    }
}

signed int zUIFontEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                          xBase* toParamWidget)
{
    signed int rval = 1;
    _zUIFont* s = (_zUIFont*)to;

    switch (toEvent)
    {
    case 3:
        s->flags |= 1;
        switch (s->fasset->mode)
        {
        case 3:
        case 4:
        case 5:
            zEntEvent("mnu4 xtocontinue", toEvent);
            break;
        case 6:
            break;
        }
        break;
    case 4:
        s->flags &= ~1;
        switch (s->fasset->mode)
        {
        case 3:
        case 4:
        case 5:
            zEntEvent("mnu4 xtocontinue", toEvent);
            break;
        case 6:
            break;
        }
        break;
    case 10:
        zUI_Reset(s);
        s->uiFontFlags = s->fasset->uiFontFlags;
        s->uiButton = 0;
        if (s->flags & 0x40)
        {
            s->flags ^= 0x40;
        }
        break;
    case 0x4D:
        s->uiFontFlags |= 8;
        break;
    case 0x4E:
        if (s->uiFontFlags & 8)
        {
            s->uiFontFlags ^= 8;
        }
        break;
    default:
        rval = zUIEventCB(from, to, toEvent, toParam, toParamWidget);
        break;
    }
    return rval;
}

void zUIFont_Render(_xEnt* e)
{
    _zUIFont* ent = (_zUIFont*)e;
    char processedText[512];
    iColor_tag c;

    if (!xEntIsVisible(e))
    {
        return;
    }

    c.r = ent->fasset->color[0];
    c.g = ent->fasset->color[1];
    c.b = ent->fasset->color[2];
    c.a = ent->fasset->color[3];
    if (ent->uiFontFlags & 0x10)
    {
        if (ent->uiFontFlags & 0x80)
        {
            ent->uiFontFlags ^= 0x80;
        }
        if ((ent->uiFontFlags & 0x20) && !(ent->uiFlags & 2))
        {
            ent->uiFontFlags |= 0x80;
            c.a >>= 1;
        }
        if ((ent->uiFontFlags & 0x40) && !(ent->uiFlags & 8))
        {
            ent->uiFontFlags |= 0x80;
            c.a >>= 1;
        }
    }

    if (ent->uiFlags & 4)
    {
        xFontBox* b = xFontBoxCreate(ent->fasset->pos.x, ent->fasset->pos.y, ent->fasset->dim[0],
                                     ent->fasset->dim[1], ent->fasset->fontID, c);
        if (b)
        {
            xFontBoxSetDrawSpacing(b, ent->fasset->space[0], ent->fasset->space[1]);
            xFontBoxSetInset(b, ent->fasset->inset[2], ent->fasset->inset[3], ent->fasset->inset[0],
                             ent->fasset->inset[1]);
            unsigned int jflags = (ent->uiFontFlags & 1) ? 0x10 : 0;
            jflags |= (ent->uiFontFlags & 2) ? 0x20 : 0;
            jflags |= (ent->uiFontFlags & 4) ? 0x40 : 0;
            xFontBoxSetJustification(b, jflags);
            xFontBoxSetDrawSize(b, ent->fasset->cdim[0], ent->fasset->cdim[1]);
            if (ent->uiFontFlags & 8)
            {
                c.r = ent->fasset->bcolor[0];
                c.g = ent->fasset->bcolor[1];
                c.b = ent->fasset->bcolor[2];
                c.a = ent->fasset->bcolor[3];
                xFontBoxSetBackdrop(b, c);
            }
            char* text = (char*)xSTFindAsset(ent->fasset->textAssetID, 0);
            if (text)
            {
                zVarMagicProcessText(text + 4, processedText);
                xFontBoxSetText(b, processedText);
                if (((ent->uiFontHackFlags & 1) && (ent->uiFlags & 2)) ||
                    (ent->uiFontHackFlags & 2) ||
                    ((ent->uiFontFlags & 0x200) && (ent->uiFlags & 2)))
                {
                    gFontWiggle = 1.0f;
                }
                else
                {
                    gFontWiggle = 0.0f;
                }
                xFontBoxRender(b);
                gFontWiggle = 0.0f;
            }
            xFontBoxDestroy(b);
        }
    }
}

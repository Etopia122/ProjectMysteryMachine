#include "zUI.h"
#include "zGlobals.h"
#include "xMath.h"
#include "xString.h"
#include "xstransvc.h"

extern "C" {
int sprintf(char* s, const char* format, ...);
char* strncpy(char* dst, const char* src, unsigned int n);
void qsort(void* base, unsigned int n, unsigned int size, int (*cmp)(const void*, const void*));
}

class RwIm2DVertex
{
public:
    float x;                // 0x0
    float y;                // 0x4
    float z;                // 0x8
    unsigned char color[4]; // 0xC
    float u;                // 0x10
    float v;                // 0x14
    float recipZ;           // 0x18
};

class RwRaster;

class RwTexture
{
public:
    RwRaster* raster; // 0x0
};

class _xRect
{
public:
    float x; // 0x0
    float y; // 0x4
    float w; // 0x8
    float h; // 0xC
};

class zSceneLevel
{
public:
    char* desc;              // 0x0
    char name;               // 0x4
    signed int index;        // 0x8
    unsigned int totalClues; // 0xC
};

enum _SDRenderState
{
    SDRS_Unknown = 0,
    SDRS_Default = 1,
    SDRS_Force32 = 0x7FFFFFFF
};

extern "C" {
signed int RwRenderStateSet(signed int state, void* value);
signed int RwIm2DRenderIndexedPrimitive(signed int primType, RwIm2DVertex* vertices,
                                        signed int numVertices, unsigned short* indices,
                                        signed int numIndices);
float RwIm2DGetNearScreenZ();
// TODO: unidentified zVar setter (the current level's scene id)
void fn_800D77CC(unsigned int id);
}

void zEntInit(_zEnt* ent, p2EntAsset* asset, unsigned int type);
void xEntUpdate(_xEnt* ent, _xScene* sc, float dt);
void xEntRender(_xEnt* ent);
void zSceneForAllBase(xBase* (*func)(xBase*, zScene*, void*), signed int baseType, void* data);
void zRenderState(_SDRenderState next);
signed int xScrFxIsLetterbox();
signed int zGameIsPaused();
void xModelRender2D(xModelInstance* model, _xRect* r, _xVec3* from, _xVec3* to, float fov);

extern float gFontWiggleTime;

static unsigned int sSortedCount;
_zUI* sSorted[768];
static RwIm2DVertex Vertex[4];
static unsigned short Index[6] = { 0, 1, 2, 0, 2, 3 };
static unsigned int cKeyUIid1off = xStrHash("KEY OFF 1 UI");
static unsigned int cKeyUIid2off = xStrHash("KEY OFF 2 UI");
static unsigned int cKeyUIid3off = xStrHash("KEY OFF 3 UI");
static unsigned int cKeyUIid4off = xStrHash("KEY OFF 4 UI");
static unsigned int cKeyUIid1on = xStrHash("KEY ON 1 UI");
static unsigned int cKeyUIid2on = xStrHash("KEY ON 2 UI");
static unsigned int cKeyUIid3on = xStrHash("KEY ON 3 UI");
static unsigned int cKeyUIid4on = xStrHash("KEY ON 4 UI");
static zSceneLevel sLevelTable2[14] = {
    { "Fishing Village", 'F', 0, 50 },
    { "Lighthouse", 'L', 1, 50 },
    { "Shipwreck", 'W', 2, 50 },
    { "Hedge Maze", 'E', 3, 50 },
    { "Cliffs & Caves", 'C', 4, 50 },
    { "Cemetery", 'G', 5, 50 },
    { "Secret Passages", 'P', 6, 50 },
    { "Cellars", 'B', 7, 50 },
    { "Super Secret Lab", 'S', 8, 50 },
    { "1st Floor", 'I', 9, 50 },
    { "Attic & Balconies", 'R', 10, 50 },
    { "Tower", 'O', 11, 50 },
    { "Hub", 'H', 12, 50 },
};
static float ushift;

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zUI_Init(void* ent, void* asset)
{
    zUI_Init((_zUI*)ent, (p2EntAsset*)asset);
}

// TODO (99.6%): the original schedules the zUI_Update address before zUIEventCB's.
#pragma auto_inline on
void zUI_Init(_zUI* ent, p2EntAsset* asset)
{
    zUIAsset* sasset = (zUIAsset*)asset;

    zEntInit(ent, asset, 'UI  ');
    ent->sasset = sasset;
    ent->pflags = 0;
    ent->penby |= 0x10;
    ent->chkby = 0;
    ent->update = (void (*)(_xEnt*, _xScene*, float))zUI_Update;
    ent->move = 0;
    ent->eventFunc = zUIEventCB;
    if (ent->linkCount)
    {
        ent->link = (p2LinkAsset*)(sasset + 1);
    }
    ent->eventFunc = zUIEventCB;
    ent->uiFlags = sasset->uiFlags;
    if (ent->uiFlags & 4)
    {
        ent->render = 0;
    }
    zEntReset(ent);
    sSortedCount = 0;

    cKeyUIid1off = xStrHash("KEY OFF 1 UI");
    cKeyUIid2off = xStrHash("KEY OFF 2 UI");
    cKeyUIid3off = xStrHash("KEY OFF 3 UI");
    cKeyUIid4off = xStrHash("KEY OFF 4 UI");
    cKeyUIid1on = xStrHash("KEY ON 1 UI");
    cKeyUIid2on = xStrHash("KEY ON 2 UI");
    cKeyUIid3on = xStrHash("KEY ON 3 UI");
    cKeyUIid4on = xStrHash("KEY ON 4 UI");
}
#pragma auto_inline off

void zUI_Save(_zUI* ent, xSerial* s)
{
    zEntSave(ent, s);
}

void zUI_Load(_zUI* ent, xSerial* s)
{
    zEntLoad(ent, s);
}

// TODO (99.4%): the original epilogue restores r31 before r0 (BFBB's zUI_Reset has
// the same unsolved difference).
#pragma auto_inline on
void zUI_Reset(_zUI* ent)
{
    zEntReset(ent);
    ent->pflags = 0;
    ent->penby |= 0x10;
    ent->chkby = 0;
    ent->uiFlags = ent->sasset->uiFlags;
    sSortedCount = 0;
}
#pragma auto_inline off

void zUI_PreUpdate(_zUI* ent)
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

void zUI_Update(_zUI* ent, _xScene* sc, float dt)
{
    xEntUpdate(ent, sc, dt);
    if (ent->uiButton)
    {
        if (ent->uiButton & 0x10)
        {
            zEntEvent(ent, ent, 0x49);
        }
        else if (ent->uiButton & 0x40)
        {
            zEntEvent(ent, ent, 0x4A);
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

xBase* zUIRenderIteratorInsert(xBase* b, zScene* sc, void* data)
{
    sSorted[sSortedCount++] = (_zUI*)b;
    return b;
}

signed int iRenderQSort_Face(void* arg1, void* arg2)
{
    _zUI* f1 = *(_zUI**)arg1;
    _zUI* f2 = *(_zUI**)arg2;

    if (f1->sasset->pos.z < f2->sasset->pos.z)
    {
        return 1;
    }
    if (f1->sasset->pos.z > f2->sasset->pos.z)
    {
        return -1;
    }
    return 0;
}

void zUIRenderAll()
{
    ushift += 0.05f;
    if (ushift >= 2.0f)
    {
        ushift -= 2.0f;
    }

    if (!sSortedCount)
    {
        zSceneForAllBase(zUIRenderIteratorInsert, 0x20, 0);
        zSceneForAllBase(zUIRenderIteratorInsert, 0x21, 0);
        if (sSortedCount > 1)
        {
            qsort(sSorted, sSortedCount, sizeof(_zUI*),
                  (int (*)(const void*, const void*))iRenderQSort_Face);
        }
    }

    for (signed int i = 0; i < (signed int)sSortedCount; i++)
    {
        if (xEntIsVisible(sSorted[i]))
        {
            if (sSorted[i]->baseType == 0x20)
            {
                zUI_Render(sSorted[i]);
            }
            else if (zGameIsPaused())
            {
                if (!(((zUIFontAsset*)sSorted[i]->sasset)->uiFontFlags & 0x100))
                {
                    zUIFont_Render(sSorted[i]);
                }
            }
            else
            {
                zUIFont_Render(sSorted[i]);
            }
        }
    }
}

#define RwIm2DVertexSetIntRGBA(v, r, g, b, a)                                                     \
    ((v)->color[0] = (r), (v)->color[1] = (g), (v)->color[2] = (b), (v)->color[3] = (a))

void zUI_Render(_xEnt* ent)
{
    _zUI* ui = (_zUI*)ent;

    if (xScrFxIsLetterbox())
    {
        if (ent->id == cKeyUIid1off || ent->id == cKeyUIid2off || ent->id == cKeyUIid3off ||
            ent->id == cKeyUIid4off || ent->id == cKeyUIid1on || ent->id == cKeyUIid2on ||
            ent->id == cKeyUIid3on || ent->id == cKeyUIid4on)
        {
            return;
        }
    }

    if (ui->uiFlags & 4)
    {
        if (xEntIsVisible(ui))
        {
            if (ui->sasset->textureID)
            {
                RwTexture* texture = (RwTexture*)xSTFindAsset(ui->sasset->textureID, 0);
                if (texture && texture->raster)
                {
                    RwRenderStateSet(1, texture->raster);
                }

                float w = 640.0f;
                float h = 480.0f;
                float u1 = ui->sasset->uva[0];
                float v1 = ui->sasset->uva[1];
                float u2 = ui->sasset->uvb[0];
                float v2 = ui->sasset->uvb[1];
                float u3 = ui->sasset->uvc[0];
                float v3 = ui->sasset->uvc[1];
                float u4 = ui->sasset->uvd[0];
                float v4 = ui->sasset->uvd[1];
                float x1 = w * ui->sasset->pos.x / w;
                float y1 = h * ui->sasset->pos.y / h;
                float x2 = w * (ui->sasset->pos.x + ui->sasset->dim[0]) / w;
                float y2 = h * (ui->sasset->pos.y + ui->sasset->dim[1]) / h;

                static unsigned int hack_pulse1 = xStrHash("paws_map");
                static unsigned int hack_pulse2 = xStrHash("paws_corner");
                static unsigned int hack_pulse3 = xStrHash("sd_tag_map");
                if (ui->sasset->textureID == hack_pulse1 || ui->sasset->textureID == hack_pulse2 ||
                    ui->sasset->textureID == hack_pulse3)
                {
                    float pulse = 1.0f + isin(15.0f * gFontWiggleTime);
                    float pulse_w = pulse * (0.05f * (x2 - x1));
                    float pulse_h = pulse * (0.05f * (y2 - y1));
                    x1 -= pulse_w;
                    x2 += pulse_w;
                    y1 -= pulse_h;
                    y2 += pulse_h;
                }

                static unsigned int hack_token1 = xStrHash("warp token ui");
                static unsigned int hack_token2 = xStrHash("warp snack ui");
                if (ent->id == hack_token1 || ent->id == hack_token2)
                {
                    float wave = 0.5f * isin(5.0f * gFontWiggleTime + x1);
                    float pulse_w = wave * (x2 - x1);
                    float cx = w * ui->sasset->pos.x / w;
                    x1 = cx - pulse_w;
                    x2 = cx + pulse_w;
                }

                float z = RwIm2DGetNearScreenZ();

                RwIm2DVertexSetIntRGBA(&Vertex[0], 0xFF, 0xFF, 0xFF, 0xFF);
                RwIm2DVertexSetIntRGBA(&Vertex[1], 0xFF, 0xFF, 0xFF, 0xFF);
                RwIm2DVertexSetIntRGBA(&Vertex[2], 0xFF, 0xFF, 0xFF, 0xFF);
                RwIm2DVertexSetIntRGBA(&Vertex[3], 0xFF, 0xFF, 0xFF, 0xFF);

                Vertex[0].x = x1;
                Vertex[0].y = y1;
                Vertex[0].z = z;
                Vertex[0].u = u1;
                Vertex[0].v = v1;

                Vertex[1].x = x1;
                Vertex[1].y = y2;
                Vertex[1].z = z;
                Vertex[1].u = u4;
                Vertex[1].v = v4;

                Vertex[2].x = x2;
                Vertex[2].y = y2;
                Vertex[2].z = z;
                Vertex[2].u = u3;
                Vertex[2].v = v3;

                Vertex[3].x = x2;
                Vertex[3].y = y1;
                Vertex[3].z = z;
                Vertex[3].u = u2;
                Vertex[3].v = v2;

                RwIm2DVertexSetIntRGBA(&Vertex[0], 0xFF, 0xFF, 0xFF, 0xFF);
                RwIm2DVertexSetIntRGBA(&Vertex[1], 0xFF, 0xFF, 0xFF, 0xFF);
                RwIm2DVertexSetIntRGBA(&Vertex[2], 0xFF, 0xFF, 0xFF, 0xFF);
                RwIm2DVertexSetIntRGBA(&Vertex[3], 0xFF, 0xFF, 0xFF, 0xFF);

                zRenderState(SDRS_Default);
                RwRenderStateSet(14, (void*)0);
                RwRenderStateSet(10, (void*)5);
                RwRenderStateSet(11, (void*)6);
                RwRenderStateSet(12, (void*)1);
                RwRenderStateSet(7, (void*)1);
                RwRenderStateSet(8, (void*)0);
                RwIm2DRenderIndexedPrimitive(3, Vertex, 4, Index, 6);
            }
            else if (ui->model)
            {
                _xVec3* pos = &((_xMat4x3*)ui->model->Mat)->pos;
                _xVec3 relfrom = { 0.0f, 1.0f, 5.0f };
                _xVec3 relto = { 0.0f, 0.0f, 0.0f };
                _xVec3 from;
                _xVec3 to;
                _xRect win1 = { 0.0f, 0.0f, 0.0f, 0.0f };

                win1.x = 640.0f * ui->sasset->pos.x / 640.0f;
                win1.y = 480.0f * ui->sasset->pos.y / 480.0f;
                win1.w = 640.0f * (ui->sasset->pos.x + ui->sasset->dim[0]) / 640.0f;
                win1.h = 480.0f * (ui->sasset->pos.y + ui->sasset->dim[1]) / 480.0f;
                from.x = relfrom.x + pos->x;
                from.y = relfrom.y + pos->y;
                from.z = relfrom.z + pos->z;
                to.x = relto.x + pos->x;
                to.y = relto.y + pos->y;
                to.z = relto.z + pos->z;
                xModelRender2D(ui->model, &win1, &from, &to, 1.5707964f);
            }
        }
    }
    else
    {
        xEntRender(ui);
    }
}

signed int zUIEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                      xBase* toParamWidget)
{
    _zUI* s = (_zUI*)to;

    switch (toEvent)
    {
    case 3:
        s->flags |= 1;
        break;
    case 4:
        s->flags &= ~1;
        break;
    case 10:
        zUI_Reset(s);
        break;
    case 0x4F:
    {
        s->uiFlags |= 2;
        for (signed int i = 0; i < 13; i++)
        {
            char level = sLevelTable2[i].name;
            for (signed int j = 0; j < 30; j++)
            {
                char uiName[256];
                sprintf(uiName, "%c %02d SELECTED", level, j);
                if (s->id == xStrHash(uiName))
                {
                    zEntEvent(s, s, 3);
                    char scene[5] = { 0 };
                    strncpy(scene, uiName, 4);
                    scene[1] = '0';
                    fn_800D77CC((scene[0] << 24) | (scene[1] << 16) | (scene[2] << 8) | scene[3]);
                }
            }
        }
        break;
    }
    case 0x50:
    {
        if (s->uiFlags & 2)
        {
            s->uiFlags ^= 2;
        }
        for (signed int i = 0; i < 13; i++)
        {
            char level = sLevelTable2[i].name;
            for (signed int j = 0; j < 30; j++)
            {
                char uiName[256];
                sprintf(uiName, "%c %02d SELECTED", level, j);
                if (s->id == xStrHash(uiName))
                {
                    zEntEvent(s, s, 4);
                }
            }
        }
        break;
    }
    case 0x51:
        s->uiFlags |= 8;
        if (s->uiFlags & 0x10)
        {
            zEntEvent(s, s, 3);
        }
        break;
    case 0x52:
        if (s->uiFlags & 8)
        {
            s->uiFlags ^= 8;
        }
        if (s->uiFlags & 0x20)
        {
            zEntEvent(s, s, 4);
        }
        break;
    case 0x5E:
        zEntEvent(s, s, 0x51);
        zEntEvent(s, s, 0x4F);
        break;
    case 0x5F:
        zEntEvent(s, s, 0x52);
        zEntEvent(s, s, 0x50);
        break;
    }
    return 1;
}

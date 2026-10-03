#include "iDraw.h"
#include "rwtypes.h"

// RenderWare GameCube immediate-mode 2D vertex.
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

extern "C" {
signed int RwRenderStateSet(signed int state, void* value);
float RwIm2DGetNearScreenZ();
signed int RwIm2DRenderIndexedPrimitive(signed int primType, RwIm2DVertex* vertices,
                                        signed int numVertices, unsigned short* indices,
                                        signed int numIndices);
}

#define RwIm2DVertexSetScreenX(vert, scrnx) ((vert)->x = (scrnx))
#define RwIm2DVertexSetScreenY(vert, scrny) ((vert)->y = (scrny))
#define RwIm2DVertexSetScreenZ(vert, scrnz) ((vert)->z = (scrnz))
#define RwIm2DVertexSetIntRGBA(vert, r, g, b, a)                                                   \
    {                                                                                              \
        (vert)->color[0] = (r);                                                                    \
        (vert)->color[1] = (g);                                                                    \
        (vert)->color[2] = (b);                                                                    \
        (vert)->color[3] = (a);                                                                    \
    }

void iScrFxDrawFade(RwCamera* cam, iColor_tag* col);

// Frames of black still to draw (set after loads to hide garbage).
static unsigned int sBlackFrames;

// Functions are in reverse DOL order (-inline deferred emits last to first).

// TODO: unidentified names (GameCube-only).
extern "C" void fn_80014A34(unsigned int frames)
{
    sBlackFrames = frames;
}

extern "C" void fn_800149F0()
{
    if (sBlackFrames)
    {
        sBlackFrames--;
        iColor_tag black = { 0, 0, 0, 255 };
        iScrFxDrawFade(0, &black);
    }
}

void iScrFxBegin()
{
    RwRenderStateSet(9, (void*)1);
    RwRenderStateSet(14, (void*)0);
    RwRenderStateSet(6, (void*)0);
    RwRenderStateSet(8, (void*)0);
    RwRenderStateSet(1, (void*)0);
    RwRenderStateSet(12, (void*)1);
    RwRenderStateSet(10, (void*)5);
    RwRenderStateSet(11, (void*)6);
}

void iScrFxEnd()
{
    RwRenderStateSet(14, (void*)0);
    RwRenderStateSet(6, (void*)1);
    RwRenderStateSet(8, (void*)1);
    RwRenderStateSet(1, (void*)0);
    RwRenderStateSet(12, (void*)0);
    RwRenderStateSet(10, (void*)2);
    RwRenderStateSet(11, (void*)1);
}

void iScrFxDrawFade(RwCamera* cam, iColor_tag* col)
{
    unsigned short indices[4] = { 0, 1, 2, 3 };
    RwIm2DVertex v[4];
    float nearZ = RwIm2DGetNearScreenZ();

    RwIm2DVertexSetScreenX(&v[0], 0.0f);
    RwIm2DVertexSetScreenY(&v[0], 0.0f);
    RwIm2DVertexSetScreenZ(&v[0], nearZ);
    RwIm2DVertexSetScreenX(&v[1], 640.0f);
    RwIm2DVertexSetScreenY(&v[1], 0.0f);
    RwIm2DVertexSetScreenZ(&v[1], nearZ);
    RwIm2DVertexSetScreenX(&v[2], 0.0f);
    RwIm2DVertexSetScreenY(&v[2], 480.0f);
    RwIm2DVertexSetScreenZ(&v[2], nearZ);
    RwIm2DVertexSetScreenX(&v[3], 640.0f);
    RwIm2DVertexSetScreenY(&v[3], 480.0f);
    RwIm2DVertexSetScreenZ(&v[3], nearZ);
    RwIm2DVertexSetIntRGBA(&v[0], col->r, col->g, col->b, col->a);
    RwIm2DVertexSetIntRGBA(&v[1], col->r, col->g, col->b, col->a);
    RwIm2DVertexSetIntRGBA(&v[2], col->r, col->g, col->b, col->a);
    RwIm2DVertexSetIntRGBA(&v[3], col->r, col->g, col->b, col->a);
    RwIm2DRenderIndexedPrimitive(4, v, 4, indices, 4);
}

void iScrFxDrawLetterbox(RwCamera* cam, signed int y_offset)
{
    unsigned short indices[4] = { 0, 1, 2, 3 };
    RwIm2DVertex v[4];
    float nearZ = RwIm2DGetNearScreenZ();

    // Every z goes to v[3] in the original.
    RwIm2DVertexSetScreenX(&v[0], 0.0f);
    RwIm2DVertexSetScreenY(&v[0], 0.0f);
    RwIm2DVertexSetScreenZ(&v[3], nearZ);
    RwIm2DVertexSetScreenX(&v[1], 640.0f);
    RwIm2DVertexSetScreenY(&v[1], 0.0f);
    RwIm2DVertexSetScreenZ(&v[3], nearZ);
    RwIm2DVertexSetScreenX(&v[2], 0.0f);
    RwIm2DVertexSetScreenY(&v[2], y_offset);
    RwIm2DVertexSetScreenZ(&v[3], nearZ);
    RwIm2DVertexSetScreenX(&v[3], 640.0f);
    RwIm2DVertexSetScreenY(&v[3], y_offset);
    RwIm2DVertexSetScreenZ(&v[3], nearZ);
    RwIm2DVertexSetIntRGBA(&v[0], 0, 0, 0, 255);
    RwIm2DVertexSetIntRGBA(&v[1], 0, 0, 0, 255);
    RwIm2DVertexSetIntRGBA(&v[2], 0, 0, 0, 255);
    RwIm2DVertexSetIntRGBA(&v[3], 0, 0, 0, 255);
    RwIm2DRenderIndexedPrimitive(4, v, 4, indices, 4);

    RwIm2DVertexSetScreenX(&v[0], 0.0f);
    RwIm2DVertexSetScreenY(&v[0], 480.0f - y_offset);
    RwIm2DVertexSetScreenX(&v[1], 640.0f);
    RwIm2DVertexSetScreenY(&v[1], 480.0f - y_offset);
    RwIm2DVertexSetScreenX(&v[2], 0.0f);
    RwIm2DVertexSetScreenY(&v[2], 480.0f);
    RwIm2DVertexSetScreenX(&v[3], 640.0f);
    RwIm2DVertexSetScreenY(&v[3], 480.0f);
    RwIm2DRenderIndexedPrimitive(4, v, 4, indices, 4);
}

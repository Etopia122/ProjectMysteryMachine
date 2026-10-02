#include "xMath3.h"
#include "iDraw.h"

static iColor_tag sgDrawColor = { 0xFF, 0xFF, 0xFF, 0xFF };

static inline iColor_tag iColorMake(unsigned char r, unsigned char g, unsigned char b,
                                    unsigned char a)
{
    iColor_tag c;
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    return c;
}

static inline void xDrawSetColorFields(iColor_tag color)
{
    sgDrawColor.r = color.r;
    sgDrawColor.g = color.g;
    sgDrawColor.b = color.b;
    sgDrawColor.a = color.a;
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xDrawBegin()
{
    iDrawBegin();
}

void xDrawEnd()
{
    iDrawEnd();
}

void xDrawSetColor(iColor_tag color)
{
    iColor_tag c = color;
    *(unsigned int*)&sgDrawColor = *(unsigned int*)&c;
}

void xDrawSetColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    xDrawSetColorFields(iColorMake(r, g, b, a));
}

// TODO (96%): the original reuses one register for the constant 1 in
// indices[1] and the numLines argument; this version loads it twice.
void xDrawLine(_xVec3* a, _xVec3* b)
{
    RwV3d verts[2];
    unsigned int indices[2];
    float bz, by, bx, az, ay, ax;

    ax = a->x;
    ay = a->y;
    az = a->z;
    bx = b->x;
    by = b->y;
    bz = b->z;

    indices[0] = 0;
    indices[1] = 1;
    verts[0].x = ax;
    verts[0].y = ay;
    verts[0].z = az;
    verts[1].x = bx;
    verts[1].y = by;
    verts[1].z = bz;
    iQueueLineList(sgDrawColor, verts, indices, 0, 2, 2, 1);
}

class RwIm2DVertex
{
public:
    float x;
    float y;
    float z;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
    float u;
    float v;
    float recipZ;
};

extern "C" {
float RwIm2DGetNearScreenZ();
signed int RwIm2DRenderLine(RwIm2DVertex* vertices, signed int numVertices, signed int vert1,
                            signed int vert2);
}

// GameCube only (not in the PS2 build); name is a guess.
void xDrawLine2D(float x1, float y1, float x2, float y2)
{
    RwIm2DVertex verts[2];
    float z = RwIm2DGetNearScreenZ();

    verts[0].x = x1;
    verts[0].y = y1;
    verts[0].z = z;
    verts[0].r = sgDrawColor.r;
    verts[0].g = sgDrawColor.g;
    verts[0].b = sgDrawColor.b;
    verts[0].a = sgDrawColor.a;
    verts[1].x = x2;
    verts[1].y = y2;
    verts[1].z = z;
    verts[1].r = sgDrawColor.r;
    verts[1].g = sgDrawColor.g;
    verts[1].b = sgDrawColor.b;
    verts[1].a = sgDrawColor.a;
    RwIm2DRenderLine(verts, 2, 0, 1);
}

#include "xMath3.h"
#include "iDraw.h"

static iColor_tag sgDrawColor = { 0xFF, 0xFF, 0xFF, 0xFF };

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
    iColor_tag c;
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    iColor_tag tmp = c;
    sgDrawColor.r = tmp.r;
    sgDrawColor.g = tmp.g;
    sgDrawColor.b = tmp.b;
    sgDrawColor.a = tmp.a;
}

// TODO: the original loads all six coordinates before storing any of them
// (struct-copy style scheduling); this version interleaves loads and stores.
void xDrawLine(_xVec3* a, _xVec3* b)
{
    RwV3d verts[2];
    unsigned int indices[2];

    verts[0].x = a->x;
    verts[0].y = a->y;
    verts[0].z = a->z;
    verts[1].x = b->x;
    verts[1].y = b->y;
    verts[1].z = b->z;
    indices[0] = 0;
    indices[1] = 1;
    iQueueLineList(sgDrawColor, verts, indices, 0, 2, 2, 1);
}

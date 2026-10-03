#include "xMath3.h"
#include "xString.h"
#include "xstransvc.h"
#include "rwtypes.h"

class RwRaster;

class RwTexture
{
public:
    RwRaster* raster;
};

class RxObjSpace3DVertex
{
public:
    RwV3d objVertex;
    RwV3d objNormal;
    RwRGBA color;
    float u;
    float v;
};

extern "C" {
signed int RwRenderStateSet(signed int state, void* value);
void* RwIm3DTransform(RxObjSpace3DVertex* pVerts, unsigned int numVerts, RwMatrixTag* ltm,
                      unsigned int flags);
signed int RwIm3DRenderPrimitive(signed int primType);
signed int RwIm3DEnd();
}

class _tagActionLine
{
public:
    unsigned int flags;
    _xVec3 pos[4];
    float time_left;
};

static _tagActionLine* sActionLine[8];
static RwRaster* sActionLineRaster;

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zActionLineInit()
{
    RwTexture* tex;

    for (signed int i = 0; i < 8; i++)
    {
        sActionLine[i] = 0;
    }
    sActionLineRaster = 0;
    tex = (RwTexture*)xSTFindAsset(xStrHash("ACTIONLINES"), 0);
    if (tex)
    {
        sActionLineRaster = tex->raster;
    }
}

void zActionLineUpdate(float seconds)
{
    _tagActionLine* l;

    for (signed int i = 0; i < 8; i++)
    {
        l = sActionLine[i];
        if (l && (l->flags & 1))
        {
            l->time_left -= seconds;
            if (l->time_left <= 0.0f)
            {
                l->flags &= ~1;
            }
        }
    }
}

static inline void RwIm3DVertexSetPos(RxObjSpace3DVertex* vert, float x, float y, float z)
{
    vert->objVertex.x = x;
    vert->objVertex.y = y;
    vert->objVertex.z = z;
}

// TODO (99.8%): the original loads pos y, z, x in that order; this loads z, y, x.
static inline void RenderActionLine(_tagActionLine* l)
{
    static RxObjSpace3DVertex sStripVert[4];
    RxObjSpace3DVertex* vert = sStripVert;

    for (signed int i = 0; i < 4; i++)
    {
        RwIm3DVertexSetPos(vert, l->pos[i].x, l->pos[i].y, l->pos[i].z);
        vert->u = 0.0f;
        vert->v = 0.0f;
        vert->color.red = 255;
        vert->color.green = 255;
        vert->color.blue = 255;
        vert->color.alpha = 128;
        vert++;
    }
    sStripVert[2].u = 1.0f;
    sStripVert[3].u = 1.0f;
    sStripVert[3].v = 1.0f;
    if (RwIm3DTransform(sStripVert, 4, 0, 0x19))
    {
        RwIm3DRenderPrimitive(4);
        RwIm3DEnd();
    }
}

void zActionLineRender()
{
    RwRenderStateSet(1, sActionLineRaster);
    for (signed int i = 0; i < 8; i++)
    {
        if (sActionLine[i] && (sActionLine[i]->flags & 1))
        {
            RenderActionLine(sActionLine[i]);
        }
    }
}

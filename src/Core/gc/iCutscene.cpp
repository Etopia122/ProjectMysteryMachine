#include "xMath3.h"
#include "rwtypes.h"

class RwRaster;

class RwIm3DVertex
{
public:
    RwV3d objVertex;    // 0x0
    RwV3d objNormal;    // 0xC
    unsigned int color; // 0x18
    float u;            // 0x1C
    float v;            // 0x20
};

class RwCameraView
{
public:
    RwObject object; // 0x0 (parent is the camera's frame)
};

class RwGlobals
{
public:
    RwCameraView* curCamera; // 0x0
};

extern "C" {
extern RwGlobals* RwEngineInstance;
signed int RwRenderStateSet(signed int state, void* value);
void* RwIm3DTransform(RwIm3DVertex* pVerts, unsigned int numVerts, RwMatrixTag* ltm,
                      unsigned int flags);
signed int RwIm3DRenderPrimitive(signed int primType);
signed int RwIm3DEnd();
}

#define RwCameraGetCurrentCamera() (RwEngineInstance->curCamera)
#define RwCameraGetFrame(cam) ((RwFrame*)(cam)->object.parent)
#define RwFrameGetMatrix(frame) (&(frame)->modelling)

#define SPRITE_COLOR 0x80808080
#define SPRITE_UV_MAX 0.999f

// TODO: GameCube-only (no PS2 equivalent); the name is unknown. Draws a
// camera-facing textured quad of size w x h at each point (zScene callers).
extern "C" void fn_80009CCC(_xVec3* pts, unsigned int count, RwRaster* raster, float w, float h)
{
    RwIm3DVertex verts[64 * 6];
    RwIm3DVertex* v;
    RwMatrixTag* mat = RwFrameGetMatrix(RwCameraGetFrame(RwCameraGetCurrentCamera()));

    if (!pts || !mat || !raster)
    {
        return;
    }

    v = verts;
    _xVec3 right;
    _xVec3 up;
    _xVec3 at;
    xVec3SMul(&right, (_xVec3*)&mat->right, 0.5f * w);
    xVec3SMul(&up, (_xVec3*)&mat->up, -0.5f * h);
    xVec3SMul(&at, (_xVec3*)&mat->at, -0.25f);

    for (unsigned int i = 0; i < count; i++)
    {
        v[0].objVertex.x = pts->x - right.x - up.x;
        v[0].objVertex.y = pts->y - right.y - up.y;
        v[0].objVertex.z = pts->z - right.z - up.z;
        v[0].color = SPRITE_COLOR;
        v[0].u = 0.0f;
        v[0].v = 0.0f;
        xVec3AddTo((_xVec3*)&v[0].objVertex, &at);

        v[1].objVertex.x = pts->x + right.x - up.x;
        v[1].objVertex.y = pts->y + right.y - up.y;
        v[1].objVertex.z = pts->z + right.z - up.z;
        v[1].color = SPRITE_COLOR;
        v[1].u = SPRITE_UV_MAX;
        v[1].v = 0.0f;
        xVec3AddTo((_xVec3*)&v[1].objVertex, &at);

        v[2].objVertex.x = up.x + (pts->x - right.x);
        v[2].objVertex.y = up.y + (pts->y - right.y);
        v[2].objVertex.z = up.z + (pts->z - right.z);
        v[2].color = SPRITE_COLOR;
        v[2].u = 0.0f;
        v[2].v = SPRITE_UV_MAX;
        xVec3AddTo((_xVec3*)&v[2].objVertex, &at);

        v[3].objVertex.x = up.x + (pts->x - right.x);
        v[3].objVertex.y = up.y + (pts->y - right.y);
        v[3].objVertex.z = up.z + (pts->z - right.z);
        v[3].color = SPRITE_COLOR;
        v[3].u = 0.0f;
        v[3].v = SPRITE_UV_MAX;
        xVec3AddTo((_xVec3*)&v[3].objVertex, &at);

        v[4].objVertex.x = pts->x + right.x - up.x;
        v[4].objVertex.y = pts->y + right.y - up.y;
        v[4].objVertex.z = pts->z + right.z - up.z;
        v[4].color = SPRITE_COLOR;
        v[4].u = SPRITE_UV_MAX;
        v[4].v = 0.0f;
        xVec3AddTo((_xVec3*)&v[4].objVertex, &at);

        v[5].objVertex.x = up.x + (pts->x + right.x);
        v[5].objVertex.y = up.y + (pts->y + right.y);
        v[5].objVertex.z = up.z + (pts->z + right.z);
        v[5].color = SPRITE_COLOR;
        v[5].u = SPRITE_UV_MAX;
        v[5].v = SPRITE_UV_MAX;
        xVec3AddTo((_xVec3*)&v[5].objVertex, &at);

        pts++;
        v += 6;
    }

    RwRenderStateSet(1, raster);
    RwRenderStateSet(12, (void*)0);
    RwRenderStateSet(10, (void*)5);
    RwRenderStateSet(11, (void*)6);
    RwRenderStateSet(6, (void*)1);
    RwRenderStateSet(8, (void*)0);
    RwIm3DTransform(verts, count * 6, 0, 9);
    RwIm3DRenderPrimitive(3);
    RwIm3DEnd();
    RwRenderStateSet(10, (void*)5);
    RwRenderStateSet(11, (void*)6);
    RwRenderStateSet(6, (void*)1);
    RwRenderStateSet(8, (void*)1);
}

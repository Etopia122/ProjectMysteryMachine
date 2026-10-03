#include "iParMgr.h"
#include "xMath3.h"
#include "xParGroup.h"
#include "xstransvc.h"
#include "zGlobals.h"
#include "rwtypes.h"

extern "C" void* memcpy(void* dst, const void* src, unsigned int n);

class RwRaster;

class RwTexture
{
public:
    RwRaster* raster;
};

class RxObjSpace3DVertex
{
public:
    RwV3d objVertex;        // 0x0
    RwV3d objNormal;        // 0xC
    unsigned char color[4]; // 0x18
    float u;                // 0x1C
    float v;                // 0x20
};

class xVec4
{
public:
    float x;
    float y;
    float z;
    float w;
};

class tagiRenderArrays
{
public:
    unsigned short m_index[960];      // 0x0
    RxObjSpace3DVertex m_vertex[480]; // 0x780
    float m_vertexTZ[480];            // 0x4B00
};

class tagiRenderInput
{
public:
    unsigned short* m_index;      // 0x0
    RxObjSpace3DVertex* m_vertex; // 0x4
    float* m_vertexTZ;            // 0x8
    unsigned int m_mode;          // 0xC
    signed int m_vertexType;      // 0x10
    signed int m_vertexTypeSize;  // 0x14
    signed int m_indexCount;      // 0x18
    signed int m_vertexCount;     // 0x1C
    _xMat4x3 m_camViewMatrix;     // 0x20
    xVec4 m_camViewR;             // 0x60
    xVec4 m_camViewU;             // 0x70
};

class xParCmdTex
{
public:
    unsigned int type;
    unsigned char enabled;
    unsigned char mode;
    unsigned char pad[2];
    float x1;
    float y1;
    float x2;
    float y2;
    unsigned char birthMode;
    unsigned char rows;
    unsigned char cols;
    unsigned char unit_count;
    float unit_width;
    float unit_height;
};

class xParSysAsset
{
public:
    unsigned char pad[0x10];
    unsigned int texture; // 0x10
};

class zParSys
{
public:
    unsigned char pad[0x10];
    xParSysAsset* tasset; // 0x10
};

class RwGlobals
{
public:
    void* curCamera;
};

extern "C" {
extern RwGlobals* RwEngineInstance;
signed int RwRenderStateSet(signed int state, void* value);
void* RwIm3DTransform(RxObjSpace3DVertex* pVerts, unsigned int numVerts, RwMatrixTag* ltm,
                      unsigned int flags);
signed int RwIm3DRenderIndexedPrimitive(signed int primType, unsigned short* indices,
                                        signed int numIndices);
signed int RwIm3DRenderPrimitive(signed int primType);
signed int RwIm3DEnd();
}

tagiRenderArrays gRenderArr;
tagiRenderInput gRenderBuffer;

static inline void xVec3Inv(_xVec3* o, const _xVec3* v)
{
    xVec3SMul(o, v, -1.0f);
}

static inline void iRenderSetCameraViewMatrix(_xMat4x3* m)
{
    if (m == 0 && *(RwCamera**)&globals.camera != 0)
    {
        gRenderBuffer.m_camViewMatrix =
            *(_xMat4x3*)((char*)((RwObject*)RwEngineInstance->curCamera)->parent + 0x10);
    }
    else
    {
        gRenderBuffer.m_camViewMatrix = *m;
    }
    xVec3Inv((_xVec3*)&gRenderBuffer.m_camViewR, &gRenderBuffer.m_camViewMatrix.right);
    xVec3Inv((_xVec3*)&gRenderBuffer.m_camViewU, &gRenderBuffer.m_camViewMatrix.up);
}

static inline void iRenderTrianglesImmediate(signed int vertType, signed int vertTypeSize,
                                             void* data, signed int dataSize,
                                             unsigned short* index, signed int indexSize)
{
    if (RwIm3DTransform((RxObjSpace3DVertex*)data, dataSize, 0, 1) != 0)
    {
        if (indexSize != 0)
        {
            RwIm3DRenderIndexedPrimitive(3, index, indexSize);
        }
        else
        {
            RwIm3DRenderPrimitive(3);
        }
        RwIm3DEnd();
    }
}

static inline void iRenderFlush()
{
    if (gRenderBuffer.m_vertexCount > 0)
    {
        iRenderTrianglesImmediate(gRenderBuffer.m_vertexType, gRenderBuffer.m_vertexTypeSize,
                                  gRenderBuffer.m_vertex, gRenderBuffer.m_vertexCount,
                                  gRenderBuffer.m_index, gRenderBuffer.m_indexCount);
    }
    gRenderBuffer.m_vertexCount = 0;
    gRenderBuffer.m_indexCount = 0;
}

static inline void iRenderSetTexture(void* data)
{
    RwTexture* texture = (RwTexture*)xSTFindAsset(((zParSys*)data)->tasset->texture, 0);
    if (texture != 0 && texture->raster != 0)
    {
        RwRenderStateSet(1, texture->raster);
    }
}

static inline void iRenderSetTextureOrNull(void* data)
{
    RwTexture* texture = (RwTexture*)xSTFindAsset(((zParSys*)data)->tasset->texture, 0);
    if (texture != 0)
    {
        if (texture->raster != 0)
        {
            RwRenderStateSet(1, texture->raster);
        }
    }
    else
    {
        RwRenderStateSet(1, 0);
    }
}

static inline void iRenderSetColor(RxObjSpace3DVertex* v, unsigned char r, unsigned char g,
                                   unsigned char b, unsigned char a)
{
    unsigned char* _col = v->color;
    _col[0] = r;
    _col[1] = g;
    _col[2] = b;
    _col[3] = a;
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void iParMgrInit()
{
    gRenderBuffer.m_mode = 0;
    gRenderBuffer.m_indexCount = 0;
    gRenderBuffer.m_vertexCount = 0;
    gRenderBuffer.m_vertexTypeSize = sizeof(RxObjSpace3DVertex);
    gRenderBuffer.m_index = gRenderArr.m_index;
    gRenderBuffer.m_vertex = gRenderArr.m_vertex;
    gRenderBuffer.m_vertexTZ = gRenderArr.m_vertexTZ;
}

void iParMgrUpdate(float)
{
}

void iParMgrRender()
{
}

// TODO: 92%. Scheduling of the corner computations.
void iParMgrRenderParSys_Sprite(void* data, xParGroup* ps)
{
    xPar* idx = ps->m_root;
    signed int indexCount;
    signed int vertexCount;
    unsigned short* i3d;
    RxObjSpace3DVertex* v3d;
    xParCmdTex* tex;
    xPar* p;
    float u1, u2, v1, v2;

    iRenderSetCameraViewMatrix(0);
    iRenderSetTexture(data);

    tex = ps->m_cmdTex;
    i3d = gRenderBuffer.m_index;
    indexCount = 0;
    v3d = gRenderBuffer.m_vertex;
    vertexCount = 0;
    while (idx != 0)
    {
        p = idx;
        idx = idx->m_next;

        float size = 0.5f * p->m_size;
        float rx = size * gRenderBuffer.m_camViewR.x;
        float ry = size * gRenderBuffer.m_camViewR.y;
        float rz = size * gRenderBuffer.m_camViewR.z;
        float ux = size * gRenderBuffer.m_camViewU.x;
        float uy = size * gRenderBuffer.m_camViewU.y;
        float uz = size * gRenderBuffer.m_camViewU.z;
        float lx = p->m_pos.x - rx;
        float hx = p->m_pos.x + rx;
        float ly = p->m_pos.y - ry;
        float hy = p->m_pos.y + ry;
        float lz = p->m_pos.z - rz;
        float hz = p->m_pos.z + rz;

        iRenderSetColor(&v3d[0], p->m_c[0], p->m_c[1], p->m_c[2], p->m_c[3]);
        iRenderSetColor(&v3d[1], p->m_c[0], p->m_c[1], p->m_c[2], p->m_c[3]);
        iRenderSetColor(&v3d[2], p->m_c[0], p->m_c[1], p->m_c[2], p->m_c[3]);
        iRenderSetColor(&v3d[3], p->m_c[0], p->m_c[1], p->m_c[2], p->m_c[3]);
        v3d[0].objVertex.x = lx - ux;
        v3d[0].objVertex.y = ly - uy;
        v3d[0].objVertex.z = lz - uz;
        v3d[1].objVertex.x = ux + hx;
        v3d[1].objVertex.y = uy + hy;
        v3d[1].objVertex.z = uz + hz;
        v3d[2].objVertex.x = hx - ux;
        v3d[2].objVertex.y = hy - uy;
        v3d[2].objVertex.z = hz - uz;
        v3d[3].objVertex.x = ux + lx;
        v3d[3].objVertex.y = uy + ly;
        v3d[3].objVertex.z = uz + lz;

        if (tex != 0)
        {
            u1 = p->m_texIdx[0] * tex->unit_width + tex->x1;
            v1 = p->m_texIdx[1] * tex->unit_height + tex->y1;
            u2 = (p->m_texIdx[0] + 1) * tex->unit_width + tex->x1;
            v2 = (p->m_texIdx[1] + 1) * tex->unit_height + tex->y1;
            v3d[0].u = u1;
            v3d[0].v = v1;
            v3d[1].u = u2;
            v3d[1].v = v2;
            v3d[2].u = u2;
            v3d[2].v = v1;
            v3d[3].u = u1;
            v3d[3].v = v2;
        }
        else
        {
            v3d[0].u = 0.0f;
            v3d[0].v = 0.0f;
            v3d[1].u = 1.0f;
            v3d[1].v = 1.0f;
            v3d[2].u = 1.0f;
            v3d[2].v = 0.0f;
            v3d[3].u = 0.0f;
            v3d[3].v = 1.0f;
        }

        i3d[0] = vertexCount;
        i3d[1] = vertexCount + 1;
        i3d[2] = vertexCount + 2;
        i3d[3] = vertexCount + 3;
        i3d[4] = vertexCount;
        i3d[5] = vertexCount + 1;
        indexCount += 6;
        v3d += 4;
        i3d += 6;
        vertexCount += 4;

        if (indexCount > 954 || vertexCount > 476)
        {
            i3d = gRenderBuffer.m_index;
            gRenderBuffer.m_indexCount = indexCount;
            v3d = gRenderBuffer.m_vertex;
            gRenderBuffer.m_vertexCount = vertexCount;
            iRenderFlush();
            indexCount = 0;
            vertexCount = 0;
        }
    }

    gRenderBuffer.m_indexCount = indexCount;
    gRenderBuffer.m_vertexCount = vertexCount;
    iRenderFlush();
}

// TODO: 81%. Corner/uv scheduling differs.
static void iRenderPushQuadStreak(xPar* p, xParCmdTex* tex)
{
    static unsigned short i3d[6] = { 0, 1, 2, 0, 2, 3 };
    static RxObjSpace3DVertex v3d[4];
    unsigned short* dst;
    float u1, u2, v1, v2;
    float size;

    if (gRenderBuffer.m_indexCount + 6 > 960)
    {
        iRenderFlush();
    }
    if (gRenderBuffer.m_vertexCount + 4 > 480)
    {
        iRenderFlush();
    }

    float px = p->m_pos.x;
    float py = p->m_pos.y;
    float pz = p->m_pos.z;
    float tx = px - 5.0f * p->m_vel.x;
    float ty = py - 5.0f * p->m_vel.y;
    size = p->m_size;
    float tz = pz - 5.0f * p->m_vel.z;
    float sx = size * gRenderBuffer.m_camViewR.x;
    float sy = size * gRenderBuffer.m_camViewR.y;
    float sz = size * gRenderBuffer.m_camViewR.z;

    iRenderSetColor(&v3d[0], p->m_c[0], p->m_c[1], p->m_c[2], p->m_c[3]);
    iRenderSetColor(&v3d[1], p->m_c[0], p->m_c[1], p->m_c[2], p->m_c[3]);
    iRenderSetColor(&v3d[2], p->m_c[0], p->m_c[1], p->m_c[2], p->m_c[3]);
    iRenderSetColor(&v3d[3], p->m_c[0], p->m_c[1], p->m_c[2], p->m_c[3]);
    v3d[0].objVertex.x = px - sx;
    v3d[0].objVertex.y = py - sy;
    v3d[0].objVertex.z = pz - sz;
    v3d[1].objVertex.x = tx - sx;
    v3d[1].objVertex.y = ty - sy;
    v3d[1].objVertex.z = tz - sz;
    v3d[2].objVertex.x = tx + sx;
    v3d[2].objVertex.y = ty + sy;
    v3d[2].objVertex.z = tz + sz;
    v3d[3].objVertex.x = px + sx;
    v3d[3].objVertex.y = py + sy;
    v3d[3].objVertex.z = pz + sz;

    if (tex != 0)
    {
        u1 = p->m_texIdx[0] * tex->unit_width + tex->x1;
        v1 = p->m_texIdx[1] * tex->unit_height + tex->y1;
        u2 = (p->m_texIdx[0] + 1) * tex->unit_width + tex->x1;
        v2 = (p->m_texIdx[1] + 1) * tex->unit_height + tex->y1;
        v3d[0].u = u1;
        v3d[0].v = v2;
        v3d[1].u = u1;
        v3d[1].v = v1;
        v3d[2].u = u2;
        v3d[2].v = v1;
        v3d[3].u = u2;
        v3d[3].v = v2;
    }
    else
    {
        v3d[0].u = 0.0f;
        v3d[0].v = 1.0f;
        v3d[1].u = 0.0f;
        v3d[1].v = 0.0f;
        v3d[2].u = 1.0f;
        v3d[2].v = 0.0f;
        v3d[3].u = 0.0f;
        v3d[3].v = 1.0f;
    }

    dst = &gRenderBuffer.m_index[gRenderBuffer.m_indexCount];
    for (signed int i = 0; i < 6; i++)
    {
        dst[i] = gRenderBuffer.m_vertexCount + i3d[i];
    }
    memcpy((char*)gRenderBuffer.m_vertex +
               gRenderBuffer.m_vertexTypeSize * gRenderBuffer.m_vertexCount,
           v3d, gRenderBuffer.m_vertexTypeSize * 4);
    gRenderBuffer.m_indexCount += 6;
    gRenderBuffer.m_vertexCount += 4;
}

// TODO: 87%. Prologue scheduling and corner computations differ.
static void iRenderPushFlat(xPar* p, xParCmdTex* tex)
{
    static unsigned short i3d[6] = { 0, 1, 2, 0, 2, 3 };
    static RxObjSpace3DVertex v3d[4];
    unsigned short* dst;
    float u1, u2, v1, v2;
    float size;
    float yaw;
    _xMat3x3 groundmat;

    if (gRenderBuffer.m_indexCount + 6 > 960)
    {
        iRenderFlush();
    }
    if (gRenderBuffer.m_vertexCount + 4 > 480)
    {
        iRenderFlush();
    }

    unsigned char r = p->m_c[0];
    unsigned char g = p->m_c[1];
    unsigned char b = p->m_c[2];
    unsigned char a = p->m_c[3];
    size = 0.5f * p->m_size;
    yaw = 0.0f;
    if (p->m_rotdeg[0] != 0)
    {
        yaw = 6.2831855f * (p->m_rotdeg[0] / 255.0f);
    }
    xMat3x3Euler(&groundmat, yaw, 0.0f, 0.0f);

    float rx = groundmat.right.x * size;
    float rz = groundmat.right.z * size;
    float ax = groundmat.at.x * size;
    float az = groundmat.at.z * size;
    float lx = p->m_pos.x - rx;
    float hx = p->m_pos.x + rx;
    float lz = p->m_pos.z - rz;
    float hz = p->m_pos.z + rz;

    iRenderSetColor(&v3d[0], r, g, b, a);
    iRenderSetColor(&v3d[1], r, g, b, a);
    iRenderSetColor(&v3d[2], r, g, b, a);
    iRenderSetColor(&v3d[3], r, g, b, a);
    v3d[0].objVertex.x = lx - ax;
    v3d[0].objVertex.y = p->m_pos.y;
    v3d[0].objVertex.z = lz - az;
    v3d[1].objVertex.x = hx - ax;
    v3d[1].objVertex.y = p->m_pos.y;
    v3d[1].objVertex.z = hz - az;
    v3d[2].objVertex.x = ax + hx;
    v3d[2].objVertex.y = p->m_pos.y;
    v3d[2].objVertex.z = az + hz;
    v3d[3].objVertex.x = ax + lx;
    v3d[3].objVertex.y = p->m_pos.y;
    v3d[3].objVertex.z = az + lz;

    if (tex != 0)
    {
        u1 = p->m_texIdx[0] * tex->unit_width + tex->x1;
        v1 = p->m_texIdx[1] * tex->unit_height + tex->y1;
        u2 = (p->m_texIdx[0] + 1) * tex->unit_width + tex->x1;
        v2 = (p->m_texIdx[1] + 1) * tex->unit_height + tex->y1;
        v3d[0].u = u1;
        v3d[0].v = v1;
        v3d[1].u = u2;
        v3d[1].v = v1;
        v3d[2].u = u2;
        v3d[2].v = v2;
        v3d[3].u = u1;
        v3d[3].v = v2;
    }
    else
    {
        v3d[0].u = 0.0f;
        v3d[0].v = 0.0f;
        v3d[1].u = 1.0f;
        v3d[1].v = 0.0f;
        v3d[2].u = 1.0f;
        v3d[2].v = 1.0f;
        v3d[3].u = 0.0f;
        v3d[3].v = 1.0f;
    }

    dst = &gRenderBuffer.m_index[gRenderBuffer.m_indexCount];
    for (signed int i = 0; i < 6; i++)
    {
        dst[i] = gRenderBuffer.m_vertexCount + i3d[i];
    }
    memcpy((char*)gRenderBuffer.m_vertex +
               gRenderBuffer.m_vertexTypeSize * gRenderBuffer.m_vertexCount,
           v3d, gRenderBuffer.m_vertexTypeSize * 4);
    gRenderBuffer.m_indexCount += 6;
    gRenderBuffer.m_vertexCount += 4;
}

// TODO: 90%. The original copies the tail point (fmr) before adding the side
// vector; PS2 lists no vector locals here.
void iParMgrRenderParSys_Streak(void* data, xParGroup* ps)
{
    xPar* idx = ps->m_root;
    RxObjSpace3DVertex* v3d;

    iRenderSetCameraViewMatrix(0);
    iRenderSetTextureOrNull(data);

    v3d = gRenderBuffer.m_vertex;
    for (; idx != 0; idx = idx->m_next)
    {
        float tx = idx->m_pos.x - 5.0f * idx->m_vel.x;
        float ty = idx->m_pos.y - 5.0f * idx->m_vel.y;
        float size = idx->m_size;
        float tz = idx->m_pos.z - 5.0f * idx->m_vel.z;
        float sx = size * gRenderBuffer.m_camViewR.x;
        float sy = size * gRenderBuffer.m_camViewR.y;
        float sz = size * gRenderBuffer.m_camViewR.z;

        v3d[0].objVertex.x = tx + sx;
        v3d[0].objVertex.y = ty + sy;
        v3d[0].objVertex.z = tz + sz;
        v3d[1].objVertex.x = tx - sx;
        v3d[1].objVertex.y = ty - sy;
        v3d[1].objVertex.z = tz - sz;
        v3d[2].objVertex.x = idx->m_pos.x;
        v3d[2].objVertex.y = idx->m_pos.y;
        v3d[2].objVertex.z = idx->m_pos.z;
        iRenderSetColor(&v3d[0], idx->m_c[0], idx->m_c[1], idx->m_c[2], idx->m_c[3]);
        iRenderSetColor(&v3d[1], idx->m_c[0], idx->m_c[1], idx->m_c[2], idx->m_c[3]);
        iRenderSetColor(&v3d[2], idx->m_c[0], idx->m_c[1], idx->m_c[2], idx->m_c[3]);
        v3d[2].u = 0.5f;
        v3d[2].v = 1.0f;
        v3d[1].u = 0.0f;
        v3d[1].v = 0.0f;
        v3d[0].u = 1.0f;
        v3d[0].v = 0.0f;
        v3d += 3;

        gRenderBuffer.m_vertexCount += 3;
        if (gRenderBuffer.m_vertexCount + 3 > 477)
        {
            iRenderFlush();
            v3d = gRenderBuffer.m_vertex;
        }
    }

    iRenderFlush();
}

// TODO: 90%. Same as Streak (the two differ only in register allocation).
void iParMgrRenderParSys_InvStreak(void* data, xParGroup* ps)
{
    xPar* idx = ps->m_root;
    RxObjSpace3DVertex* v3d;

    iRenderSetCameraViewMatrix(0);
    iRenderSetTextureOrNull(data);

    v3d = gRenderBuffer.m_vertex;
    for (; idx != 0; idx = idx->m_next)
    {
        float tx = idx->m_pos.x - 5.0f * idx->m_vel.x;
        float ty = idx->m_pos.y - 5.0f * idx->m_vel.y;
        float size = idx->m_size;
        float tz = idx->m_pos.z - 5.0f * idx->m_vel.z;
        float sx = size * gRenderBuffer.m_camViewR.x;
        float sy = size * gRenderBuffer.m_camViewR.y;
        float sz = size * gRenderBuffer.m_camViewR.z;

        v3d[0].objVertex.x = tx + sx;
        v3d[0].objVertex.y = ty + sy;
        v3d[0].objVertex.z = tz + sz;
        v3d[1].objVertex.x = tx - sx;
        v3d[1].objVertex.y = ty - sy;
        v3d[1].objVertex.z = tz - sz;
        v3d[2].objVertex.x = idx->m_pos.x;
        v3d[2].objVertex.y = idx->m_pos.y;
        v3d[2].objVertex.z = idx->m_pos.z;
        iRenderSetColor(&v3d[0], idx->m_c[0], idx->m_c[1], idx->m_c[2], idx->m_c[3]);
        iRenderSetColor(&v3d[1], idx->m_c[0], idx->m_c[1], idx->m_c[2], idx->m_c[3]);
        iRenderSetColor(&v3d[2], idx->m_c[0], idx->m_c[1], idx->m_c[2], idx->m_c[3]);
        v3d[2].u = 0.5f;
        v3d[2].v = 1.0f;
        v3d[1].u = 0.0f;
        v3d[1].v = 0.0f;
        v3d[0].u = 1.0f;
        v3d[0].v = 0.0f;
        v3d += 3;

        gRenderBuffer.m_vertexCount += 3;
        if (gRenderBuffer.m_vertexCount + 3 > 477)
        {
            iRenderFlush();
            v3d = gRenderBuffer.m_vertex;
        }
    }

    iRenderFlush();
}

void iParMgrRenderParSys_QuadStreak(void* data, xParGroup* ps)
{
    xPar* idx = ps->m_root;

    iRenderSetCameraViewMatrix(0);
    iRenderSetTextureOrNull(data);

    for (; idx != 0; idx = idx->m_next)
    {
        iRenderPushQuadStreak(idx, ps->m_cmdTex);
    }
    iRenderFlush();
}

void iParMgrRenderParSys_Static(void*, xParGroup*)
{
}

// TODO: 95.6%. Frame size and corner scheduling.
void iParMgrRenderParSys_Ground(void* data, xParGroup* ps)
{
    static unsigned short i3d[6] = { 0, 1, 2, 3, 0, 1 };
    static RxObjSpace3DVertex v3d[4];
    xPar* idx = ps->m_root;
    xParCmdTex* tex;
    _xMat3x3 groundmat;

    iRenderSetCameraViewMatrix(0);
    iRenderSetTexture(data);

    for (; idx != 0; idx = idx->m_next)
    {
        float u1, u2, v1, v2;
        float angx, angy, angz;
        unsigned short* dst;

        tex = ps->m_cmdTex;
        if (gRenderBuffer.m_indexCount + 6 > 960 || gRenderBuffer.m_vertexCount + 4 > 480)
        {
            iRenderFlush();
        }

        unsigned char r = idx->m_c[0];
        unsigned char g = idx->m_c[1];
        unsigned char b = idx->m_c[2];
        unsigned char a = idx->m_c[3];
        float size = 0.5f * idx->m_size;
        angx = angy = angz = 0.0f;
        if (idx->m_rotdeg[0] != 0)
        {
            angx = 6.2831855f * (idx->m_rotdeg[0] / 255.0f);
        }
        if (idx->m_rotdeg[1] != 0)
        {
            angy = 6.2831855f * (idx->m_rotdeg[1] / 255.0f);
        }
        if (idx->m_rotdeg[2] != 0)
        {
            angz = 6.2831855f * (idx->m_rotdeg[2] / 255.0f);
        }
        xMat3x3Euler(&groundmat, angx, angy, angz);

        float rx = groundmat.right.x * size;
        float ry = groundmat.right.y * size;
        float rz = groundmat.right.z * size;
        float ax = groundmat.at.x * size;
        float ay = groundmat.at.y * size;
        float az = groundmat.at.z * size;
        float lx = idx->m_pos.x - rx;
        float ly = idx->m_pos.y - ry;
        float hx = idx->m_pos.x + rx;
        float hy = idx->m_pos.y + ry;
        float lz = idx->m_pos.z - rz;
        float hz = idx->m_pos.z + rz;

        iRenderSetColor(&v3d[0], r, g, b, a);
        iRenderSetColor(&v3d[1], r, g, b, a);
        iRenderSetColor(&v3d[2], r, g, b, a);
        iRenderSetColor(&v3d[3], r, g, b, a);
        v3d[0].objVertex.x = lx - ax;
        v3d[0].objVertex.y = ly - ay;
        v3d[0].objVertex.z = lz - az;
        v3d[1].objVertex.x = ax + hx;
        v3d[1].objVertex.y = ay + hy;
        v3d[1].objVertex.z = az + hz;
        v3d[2].objVertex.x = hx - ax;
        v3d[2].objVertex.y = hy - ay;
        v3d[2].objVertex.z = hz - az;
        v3d[3].objVertex.x = ax + lx;
        v3d[3].objVertex.y = ay + ly;
        v3d[3].objVertex.z = az + lz;

        if (tex != 0)
        {
            u1 = idx->m_texIdx[0] * tex->unit_width + tex->x1;
            v1 = idx->m_texIdx[1] * tex->unit_height + tex->y1;
            u2 = (idx->m_texIdx[0] + 1) * tex->unit_width + tex->x1;
            v2 = (idx->m_texIdx[1] + 1) * tex->unit_height + tex->y1;
            v3d[0].u = u1;
            v3d[0].v = v1;
            v3d[1].u = u2;
            v3d[1].v = v2;
            v3d[2].u = u2;
            v3d[2].v = v1;
            v3d[3].u = u1;
            v3d[3].v = v2;
        }
        else
        {
            v3d[0].u = 0.0f;
            v3d[0].v = 0.0f;
            v3d[1].u = 1.0f;
            v3d[1].v = 1.0f;
            v3d[2].u = 1.0f;
            v3d[2].v = 0.0f;
            v3d[3].u = 0.0f;
            v3d[3].v = 1.0f;
        }

        dst = &gRenderBuffer.m_index[gRenderBuffer.m_indexCount];
        for (signed int i = 0; i < 6; i++)
        {
            dst[i] = gRenderBuffer.m_vertexCount + i3d[i];
        }
        memcpy((char*)gRenderBuffer.m_vertex +
                   gRenderBuffer.m_vertexTypeSize * gRenderBuffer.m_vertexCount,
               v3d, gRenderBuffer.m_vertexTypeSize * 4);
        gRenderBuffer.m_indexCount += 6;
        gRenderBuffer.m_vertexCount += 4;
    }

    iRenderFlush();
}

void iParMgrRenderParSys_Flat(void* data, xParGroup* ps)
{
    xPar* idx = ps->m_root;

    iRenderSetCameraViewMatrix(0);
    iRenderSetTexture(data);

    for (; idx != 0; idx = idx->m_next)
    {
        iRenderPushFlat(idx, ps->m_cmdTex);
    }
    iRenderFlush();
}

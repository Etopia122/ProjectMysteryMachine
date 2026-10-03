#include "xMemMgr.h"
#include "rwtypes.h"

extern "C" void* memset(void* dst, int c, unsigned int n);

class RpMorphTarget
{
public:
    void* parentGeom;          // 0x0
    float boundingSphere[4];   // 0x4
    RwV3d* verts;              // 0x14
    RwV3d* normals;            // 0x18
};

class RpGeometry
{
public:
    RwObject object;             // 0x0
    unsigned int flags;          // 0x8
    unsigned short lockedSinceLastInst; // 0xC
    signed short refCount;       // 0xE
    signed int numTriangles;     // 0x10
    signed int numVertices;      // 0x14
    unsigned char pad18[0x68 - 0x18];
    RpMorphTarget* morphTarget;  // 0x68
};

class RpAtomic
{
public:
    unsigned char pad0[0x18];
    RpGeometry* geometry; // 0x18
};

class RpUserDataArray
{
public:
    char* name;               // 0x0
    signed int format;        // 0x4
    signed int numElements;   // 0x8
    void* data;               // 0xC
};

// The per-geometry cache of the last morph, kept in a user data array.
class DirtyMorph
{
public:
    unsigned int count;        // 0x0
    float scale;               // 0x4
    signed short weight[4];    // 0x8
    signed short* v_array[4];  // 0x10
};

extern "C" {
RpGeometry* RpGeometryLock(RpGeometry* geometry, signed int lockMode);
RpGeometry* RpGeometryUnlock(RpGeometry* geometry);
signed int RpGeometryGetUserDataArrayCount(const RpGeometry* geometry);
signed int RpGeometryAddUserDataArray(RpGeometry* geometry, char* name, signed int format,
                                      signed int numElements);
RpUserDataArray* RpGeometryGetUserDataArray(const RpGeometry* geometry, signed int data);
}

void iModelRender(RpAtomic* model, RwMatrixTag* mat);

static RpGeometry* s_geom;
static RpMorphTarget* s_tgt;
static float* s_alloc;
static float* s_vTemp;
static float* s_nTemp;
static unsigned int s_numV;

static inline void FastS16unpack(float* dest, signed short* v, signed int count, float scale)
{
    for (signed int i = 0; i < count; i++)
    {
        dest[i] = scale * v[i];
    }
}

static inline void FastS16weight2(float* dest, signed short** v_array, signed short* weight,
                                  signed int count, float scale)
{
    signed short* a = v_array[0];
    signed short* b = v_array[1];
    float s0 = scale * weight[0];
    float s1 = scale * weight[1];

    for (signed int i = 0; i < count; i++)
    {
        dest[i] = s0 * a[i] + s1 * b[i];
    }
}

static inline void FastS16weight4(float* dest, signed short** v_array, signed short* weight,
                                  signed int count, float scale)
{
    signed short* a0 = v_array[0];
    signed short* a1 = v_array[1];
    signed short* a2 = v_array[2];
    signed short* a3 = v_array[3];
    float s0 = scale * weight[0];
    float s1 = scale * weight[1];
    float s2 = scale * weight[2];
    float s3 = scale * weight[3];

    for (signed int i = 0; i < count; i++)
    {
        dest[i] = s0 * a0[i] + s1 * a1[i] + s2 * a2[i] + s3 * a3[i];
    }
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

// TODO (98.6%): saved registers for useNormals/dorender/lockMode come out as
// r27/r25/r26 instead of r25/r26/r27, and s_numV is loaded before the
// int-to-float constant in the normals pass.
static void MorphCommon(RpAtomic* model, RwMatrixTag* mat, signed short** v_array,
                        signed short* weight, unsigned int normals, float scale,
                        signed int dorender)
{
    signed short* va[4];
    signed short wa[4];
    unsigned int i;
    unsigned int a;
    signed int wsum;
    RwV3d* vold;
    RwV3d* nold;
    signed int lockMode;
    RpUserDataArray* usr;
    DirtyMorph* dm;
    unsigned char useNormals;

    s_geom = model->geometry;
    nold = 0;
    s_tgt = s_geom->morphTarget;
    s_numV = s_geom->numVertices;
    s_alloc = 0;
    s_nTemp = 0;

    useNormals = normals && (s_geom->object.flags & 0x10);

    vold = s_tgt->verts;
    lockMode = (useNormals ? 4 : 0) | 2;
    if (useNormals)
    {
        nold = s_tgt->normals;
    }

    a = 0;
    wsum = 0;
    for (i = 0; i < 4; i++)
    {
        if (v_array[i] && weight[i])
        {
            va[a] = v_array[i];
            wa[a] = weight[i];
            wsum += weight[i];
            a++;
        }
    }

    usr = RpGeometryGetUserDataArray(s_geom, 0);
    if (usr)
    {
        dm = (DirtyMorph*)usr->data;

        s_vTemp = (float*)((char*)dm + sizeof(DirtyMorph));
        while ((unsigned int)s_vTemp & 0xF)
        {
            s_vTemp++;
        }
        s_tgt->verts = (RwV3d*)s_vTemp;

        if (useNormals)
        {
            s_nTemp = s_vTemp + s_numV * 3;
            while ((unsigned int)s_nTemp & 0xF)
            {
                s_nTemp++;
            }
            s_tgt->normals = (RwV3d*)s_nTemp;
        }

        if (dm->count == a && dm->weight[0] == wa[0] && dm->v_array[0] == va[0] &&
            dm->scale == scale)
        {
            for (i = 1; i < a; i++)
            {
                if (dm->weight[i] != wa[i] || va[i] != dm->v_array[i])
                {
                    break;
                }
            }

            if (a == i)
            {
                if (dorender)
                {
                    iModelRender(model, mat);
                }
                s_tgt->verts = vold;
                if (nold)
                {
                    s_tgt->normals = nold;
                }
                return;
            }
        }

        dm->count = a;
        dm->scale = scale;
        for (i = 0; i < a; i++)
        {
            dm->weight[i] = wa[i];
            dm->v_array[i] = v_array[i];
        }

        RpGeometryLock(s_geom, lockMode);
    }

    if (a == 3)
    {
        va[3] = va[2];
        wa[3] = 0;
    }

    if (!usr)
    {
        if (s_numV == 0)
        {
            s_vTemp = 0;
        }
        else
        {
            s_vTemp = (float*)xMemPushTemp(s_numV * 3 * sizeof(float) + 16);
            s_alloc = s_vTemp;
            while ((unsigned int)s_vTemp & 0xF)
            {
                s_vTemp++;
            }
        }

        if (useNormals && (s_geom->object.flags & 0x10))
        {
            if (s_numV == 0)
            {
                s_nTemp = 0;
            }
            else
            {
                s_nTemp = (float*)xMemPushTemp(s_numV * 3 * sizeof(float) + 16);
                if (!s_alloc)
                {
                    s_alloc = s_nTemp;
                }
                while ((unsigned int)s_nTemp & 0xF)
                {
                    s_nTemp++;
                }
            }
        }
    }

    if (a == 1)
    {
        FastS16unpack(s_vTemp, va[0], s_numV * 3, scale * wsum);
    }
    else if (a == 2)
    {
        FastS16weight2(s_vTemp, va, wa, s_numV * 3, scale);
    }
    else
    {
        FastS16weight4(s_vTemp, va, wa, s_numV * 3, scale);
    }

    if (s_nTemp)
    {
        scale = 1.0f / (16384.0f * wsum);
        unsigned int stride = ((s_numV * 3 + 7) * 2) & ~0xF;

        for (i = 0; i < a; i++)
        {
            va[i] = (signed short*)((char*)va[i] + stride);
        }

        if (a == 1)
        {
            FastS16unpack(s_nTemp, va[0], s_numV * 3, scale * wsum);
        }
        else if (a == 2)
        {
            FastS16weight2(s_nTemp, va, wa, s_numV * 3, scale);
        }
        else
        {
            FastS16weight4(s_nTemp, va, wa, s_numV * 3, scale);
        }
    }

    if (usr)
    {
        RpGeometryUnlock(s_geom);
        if (dorender)
        {
            iModelRender(model, mat);
        }
        s_tgt->verts = vold;
        if (useNormals)
        {
            s_tgt->normals = nold;
        }
    }
    else if (dorender)
    {
        RpGeometryLock(s_geom, lockMode);
        vold = s_tgt->verts;
        s_tgt->verts = (RwV3d*)s_vTemp;
        if (useNormals)
        {
            nold = s_tgt->normals;
            s_tgt->normals = (RwV3d*)s_nTemp;
        }
        RpGeometryUnlock(s_geom);
        iModelRender(model, mat);
        s_tgt->verts = vold;
        if (useNormals)
        {
            s_tgt->normals = nold;
        }
    }
}

void iMorphOptimize(RpAtomic* model, signed int normals)
{
    RpGeometry* geom;
    unsigned char hasNormals = (normals != 0);
    geom = model->geometry;

    if (RpGeometryGetUserDataArrayCount(geom) == 0)
    {
        signed int usridx = RpGeometryAddUserDataArray(
            geom, "MORPHSTATE", 1, ((hasNormals + 1) * geom->numVertices) * sizeof(RwV3d) + 56);
        RpUserDataArray* usr = RpGeometryGetUserDataArray(geom, usridx);
        memset(usr->data, 0, 0x20);
    }
}

void iMorphRender(RpAtomic* model, RwMatrixTag* mat, signed short** v_array, signed short* weight,
                  unsigned int normals, float scale)
{
    MorphCommon(model, mat, v_array, weight, normals, scale, 1);
    if (s_alloc)
    {
        xMemPopTemp(s_alloc);
        s_alloc = 0;
    }
}

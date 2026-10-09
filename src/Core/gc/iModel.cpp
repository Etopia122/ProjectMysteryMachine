#ifdef PC_PORT
#include <stdlib.h>
#endif
#include "iModel.h"
#include "rwtypes.h"


class RwSphere
{
public:
    RwV3d center; // 0x0
    float radius; // 0xC
};

class RpMorphTarget
{
public:
    unsigned char pad0[0x14];
    RwV3d* verts; // 0x14
};

class RpGeometry
{
public:
    unsigned char pad0[0x14];
    signed int numVertices; // 0x14
    unsigned char pad18[0x68 - 0x18];
    RpMorphTarget* morphTarget; // 0x68
};

class RpClump;

class RpAtomic
{
public:
    RwObject object;                 // 0x0
    unsigned char pad8[0x18 - 0x8];
    RpGeometry* geometry;            // 0x18
    RwSphere boundingSphere;         // 0x1C
    RwSphere worldBoundingSphere;    // 0x2C
    RpClump* clump;                  // 0x3C
    unsigned char pad40[0x48 - 0x40];
    RpAtomic* (*renderCallBack)(RpAtomic*); // 0x48
};

class RpClump
{
public:
    RwObject object; // 0x0
};

class RpHAnimNodeInfo
{
public:
    signed int nodeID;    // 0x0
    signed int nodeIndex; // 0x4
    signed int flags;     // 0x8
    void* pFrame;         // 0xC
};

class RpHAnimHierarchy
{
public:
    signed int flags;              // 0x0
    signed int numNodes;           // 0x4
    unsigned char pad8[0x28 - 0x8];
    RwMatrixTag* pMatrixArray;     // 0x28
    unsigned char pad2C[0x30 - 0x2C];
    RpHAnimNodeInfo* pNodeInfo;    // 0x30
};

class RpSkin;
class RwStream;
class RwCamera;

class RwMemory
{
public:
    unsigned char* start;  // 0x0
    unsigned int length;   // 0x4
};

class RwGlobals
{
public:
    RwCamera* curCamera; // 0x0
};

extern "C" {
extern RwGlobals* RwEngineInstance;
RwV3d* RwV3dTransformPoints(RwV3d* pointsOut, const RwV3d* pointsIn, signed int numPoints,
                            const RwMatrixTag* matrix);
RpHAnimHierarchy* RpHAnimFrameGetHierarchy(RwFrame* frame);
signed int RpHAnimFrameSetHierarchy(RwFrame* frame, RpHAnimHierarchy* hierarchy);
RwFrame* RwFrameForAllChildren(RwFrame* frame, RwFrame* (*callBack)(RwFrame*, void*), void* data);
RwFrame* RwFrameGetRoot(const RwFrame* frame);
signed int RwFrameDestroyHierarchy(RwFrame* frame);
RwMatrixTag* RwMatrixUpdate(RwMatrixTag* matrix);
RwMatrixTag* RwMatrixMultiply(RwMatrixTag* matrixOut, const RwMatrixTag* matrixIn1,
                              const RwMatrixTag* matrixIn2);
signed int RwCameraFrustumTestSphere(const RwCamera* camera, const RwSphere* sphere);
RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry);
unsigned int RpSkinGetNumBones(RpSkin* skin);
const RwMatrixTag* RpSkinGetSkinToBoneMatrices(RpSkin* skin);
const float* RpSkinGetVertexBoneWeights(RpSkin* skin);
const unsigned int* RpSkinGetVertexBoneIndices(RpSkin* skin);
RpAtomic* RpSkinAtomicSetHAnimHierarchy(RpAtomic* atomic, RpHAnimHierarchy* hierarchy);
RpHAnimHierarchy* RpHAnimHierarchyCreate(signed int numNodes, unsigned int* nodeFlags,
                                         signed int* nodeIDs, signed int flags,
                                         signed int maxInterpKeyFrameSize);
RwStream* RwStreamOpen(signed int type, signed int accessType, const void* pData);
signed int RwStreamClose(RwStream* stream, void* pData);
signed int RwStreamFindChunk(RwStream* stream, unsigned int type, unsigned int* lengthOut,
                             unsigned int* versionOut);
RpClump* RpClumpStreamRead(RwStream* stream);
RpClump* RpClumpForAllAtomics(RpClump* clump, RpAtomic* (*callback)(RpAtomic*, void*),
                              void* pData);
signed int RpClumpDestroy(RpClump* clump);
}

void xMat4x3Mul(_xMat4x3* o, _xMat4x3* a, _xMat4x3* b);
void xQuatToMat(_xQuat* q, _xMat3x3* m);

extern unsigned char* giAnimScratch;

static unsigned int gLastAtomicCount;
static RpAtomic* gLastAtomicList[256];

#pragma auto_inline on
RwFrame* GetChildFrameHierarchy(RwFrame* frame, void* data)
{
    RpHAnimHierarchy* hierarchy = RpHAnimFrameGetHierarchy(frame);
    if (hierarchy == 0)
    {
        RwFrameForAllChildren(frame, GetChildFrameHierarchy, data);
        return frame;
    }
    *(RpHAnimHierarchy**)data = hierarchy;
    return 0;
}
#pragma auto_inline off

static inline RpHAnimHierarchy* GetHierarchy(RpAtomic* model)
{
    RpHAnimHierarchy* hierarchy = 0;
    GetChildFrameHierarchy((RwFrame*)model->object.parent, &hierarchy);
    return hierarchy;
}

void iModelInit()
{
}

static RpAtomic* FindAtomicCallback(RpAtomic* model, void*)
{
    RpHAnimHierarchy* hierarchy = GetHierarchy(model);
    RpSkin* skin = RpSkinGeometryGetSkin(model->geometry);

    if (skin && !hierarchy)
    {
        hierarchy = RpHAnimHierarchyCreate(RpSkinGetNumBones(skin), 0, 0, 0x4000, 0x24);
        RpHAnimFrameSetHierarchy((RwFrame*)model->object.parent, hierarchy);
    }
    if (hierarchy && skin)
    {
        RpSkinAtomicSetHAnimHierarchy(model, hierarchy);
    }
    if (hierarchy)
    {
        hierarchy->flags = 0x4000;
    }
    if (gLastAtomicCount < 256)
    {
        gLastAtomicList[gLastAtomicCount++] = model;
    }

    RwFrameGetRoot((RwFrame*)model->object.parent);
    return model;
}

static inline RpAtomic* iModelStreamRead(RwStream* stream)
{
    if (!stream)
    {
        return 0;
    }

    if (!RwStreamFindChunk(stream, 0x10, 0, 0))
    {
        RwStreamClose(stream, 0);
        return 0;
    }

    RpClump* clump = RpClumpStreamRead(stream);
    RwStreamClose(stream, 0);
    if (!clump)
    {
        return 0;
    }

    gLastAtomicCount = 0;
    RpClumpForAllAtomics(clump, FindAtomicCallback, 0);
    return gLastAtomicList[0];
}

RpAtomic* iModelFileNew(void* buffer, unsigned int size)
{
    RwMemory rwmem;
    rwmem.start = (unsigned char*)buffer;
    rwmem.length = size;

    return iModelStreamRead(RwStreamOpen(3, 1, &rwmem));
}

void iModelUnload(RpAtomic* userdata)
{
    RpClump* clump = userdata->clump;
    RwFrame* frame = (RwFrame*)clump->object.parent;

    if (frame)
    {
        RwFrame* root = RwFrameGetRoot(frame);
        if (root)
        {
            frame = root;
        }
        RwFrameDestroyHierarchy(frame);
        clump->object.parent = 0;
    }
    if (clump)
    {
        RpClumpDestroy(clump);
    }
}

static RpAtomic* NextAtomicCallback(RpAtomic* atomic, void* data)
{
    if (*(RpAtomic**)data == atomic)
    {
        *(RpAtomic**)data = 0;
    }
    else if (*(RpAtomic**)data == 0)
    {
        *(RpAtomic**)data = atomic;
    }
    return atomic;
}

RpAtomic* iModelFile_RWMultiAtomic(RpAtomic* model)
{
    if (!model)
    {
        return 0;
    }

    RpClump* clump = model->clump;
    RpAtomic* nextModel = model;
    RpClumpForAllAtomics(clump, NextAtomicCallback, &nextModel);
    return nextModel;
}

unsigned int iModelNumBones(RpAtomic* model)
{
    RpHAnimHierarchy* hierarchy = GetHierarchy(model);
    if (!hierarchy)
    {
        return 0;
    }
    return hierarchy->numNodes;
}

#pragma auto_inline on
void iModelQuatToMat(_xQuat* quat, _xVec3* tran, RwMatrixTag* mat)
{
    quat->s = -quat->s;
    xQuatToMat(quat, (_xMat3x3*)mat);
    quat->s = -quat->s;
    mat->pos.x = tran->x;
    mat->pos.y = tran->y;
    mat->pos.z = tran->z;
}
#pragma auto_inline off

// TODO: 96%, the push onto the matrix stack copies with a different load/store
// interleaving than the other matrix copies.
void iModelAnimMatrices(RpAtomic* model, _xQuat* quat, _xVec3* tran, RwMatrixTag* mat)
{
    RpHAnimHierarchy* hierarchy = GetHierarchy(model);

    if (hierarchy)
    {
        RwMatrixTag qmat;
        RwMatrixTag tmp;
        RwMatrixTag matrixStack[33];
        RwMatrixTag* pMatrixArray;
        RpHAnimNodeInfo* pCurrentFrame;
        signed int pCurrentFrameFlags;
        signed int i;
        signed int numFrames;
        RwMatrixTag* pMat;

        pMat = mat;
        matrixStack[0].at.z = 1.0f;
        matrixStack[0].up.y = 1.0f;
        matrixStack[0].right.x = 1.0f;
        matrixStack[0].up.x = 0.0f;
        matrixStack[0].right.z = 0.0f;
        matrixStack[0].right.y = 0.0f;
        matrixStack[0].at.y = 0.0f;
        matrixStack[0].at.x = 0.0f;
        matrixStack[0].up.z = 0.0f;
        matrixStack[0].pos.z = 0.0f;
        matrixStack[0].pos.y = 0.0f;
        matrixStack[0].pos.x = 0.0f;
        matrixStack[0].flags |= 0x20003;

        matrixStack[1] = matrixStack[0];
        pMatrixArray = &matrixStack[2];

        numFrames = hierarchy->numNodes;
        pCurrentFrame = hierarchy->pNodeInfo;
        pCurrentFrameFlags = pCurrentFrame->flags;

        for (i = 0; i < numFrames; i++)
        {
            if (pCurrentFrameFlags & 2)
            {
                *pMatrixArray++ = matrixStack[0];
            }

            iModelQuatToMat(quat, tran, &qmat);
            RwMatrixMultiply(&tmp, &qmat, &matrixStack[0]);
            *pMat = tmp;

            RwMatrixTag* pMatrixStackTop;
            if (pCurrentFrameFlags & 1)
            {
                pMatrixStackTop = --pMatrixArray;
            }
            else
            {
                pMatrixStackTop = &tmp;
            }
            matrixStack[0] = *pMatrixStackTop;

            pMat++;
            quat++;
            tran++;
            pCurrentFrameFlags = (++pCurrentFrame)->flags;
        }
    }
}

void iModelRender(RpAtomic* model, RwMatrixTag* mat)
{
    RpHAnimHierarchy* hierarchy = GetHierarchy(model);
    RwMatrixTag* pAnimOldMatrix;

    if (hierarchy)
    {
        pAnimOldMatrix = hierarchy->pMatrixArray;
        hierarchy->pMatrixArray = mat + 1;
    }

    RwFrame* frame = (RwFrame*)model->object.parent;
    frame->ltm = *mat;
    RwMatrixUpdate(&frame->ltm);
    model->renderCallBack(model);

    if (hierarchy)
    {
        hierarchy->pMatrixArray = pAnimOldMatrix;
    }
}

static inline float xsqr(float x)
{
    return x * x;
}

static inline float xsqrt(float x)
{
    volatile float y;
    if (x > 0.0f)
    {
        double guess = __frsqrte((double)x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        y = (float)(x * guess);
        return y;
    }
    return x;
}

#define MAX2(a, b) ((a) >= (b) ? (a) : (b))

// TODO: 97%, the MAX result shares the wrong register (up instead of at).
signed int iModelCull(RpAtomic* model, RwMatrixTag* mat)
{
    RwCamera* cam = RwEngineInstance->curCamera;
    RwSphere sph;

    RwV3dTransformPoints(&sph.center, &model->boundingSphere.center, 1, mat);

#pragma fp_contract off
    float up = mat->up.x * mat->up.x + mat->up.y * mat->up.y + mat->up.z * mat->up.z;
    float at = mat->at.x * mat->at.x + mat->at.y * mat->at.y + mat->at.z * mat->at.z;
    float right =
        mat->right.x * mat->right.x + mat->right.y * mat->right.y + mat->right.z * mat->right.z;
#pragma fp_contract on

    sph.radius = model->boundingSphere.radius * xsqrt(MAX2(right, MAX2(up, at)));
    model->worldBoundingSphere = sph;

#ifdef PC_PORT
    // Debug: PC_NOMODELCULL=1 draws every model regardless of the camera.
    static int nocull = -1;
    if (nocull < 0)
    {
        nocull = getenv("PC_NOMODELCULL") ? 1 : 0;
    }
    if (nocull)
    {
        return 0;
    }
#endif
    return (unsigned char)(RwCameraFrustumTestSphere(cam, &sph) == 0);
}

unsigned int iModelVertCount(RpAtomic* model)
{
    return model->geometry->numVertices;
}

static inline void SkinXform(_xVec3* dest, const _xVec3* vert, const RwMatrixTag* mat,
                             const RwMatrixTag* skinmat, const float* wt, const unsigned int* idx,
                             unsigned int count)
{
    unsigned int catMatFlags[2] = { 0, 0 };
    RwMatrixTag* catmat = (RwMatrixTag*)giAnimScratch;
    const RwMatrixTag* rootmat = mat;
    mat++;

    while (count != 0)
    {
        for (unsigned int i = 0; i < 4; i++)
        {
            unsigned int midx = (*idx >> (i << 3)) & 0xFF;
            unsigned int flagidx = ((*idx >> (i << 3)) >> 5) & 0x7;
            unsigned int bit = 1 << ((*idx >> (i << 3)) & 0x1F);
            if (!(bit & catMatFlags[flagidx]))
            {
                xMat4x3Mul((_xMat4x3*)(catmat + midx), (_xMat4x3*)(skinmat + midx),
                           (_xMat4x3*)(mat + midx));
                catMatFlags[flagidx] |= bit;
            }
        }

        _xVec3 accumV;
        accumV.x = accumV.y = accumV.z = 0.0f;

        RwMatrixTag* pMatrix;
        const float* fwt = wt;
        unsigned int wtidx = *idx;
        unsigned int maxwt = 4;

        while (*fwt && maxwt)
        {
            pMatrix = catmat + (wtidx & 0xFF);

            accumV.x += *fwt * (pMatrix->right.x * vert->x + pMatrix->up.x * vert->y +
                                pMatrix->at.x * vert->z + pMatrix->pos.x);
            accumV.y += *fwt * (pMatrix->right.y * vert->x + pMatrix->up.y * vert->y +
                                pMatrix->at.y * vert->z + pMatrix->pos.y);
            accumV.z += *fwt * (pMatrix->right.z * vert->x + pMatrix->up.z * vert->y +
                                pMatrix->at.z * vert->z + pMatrix->pos.z);

            maxwt--;
            wtidx >>= 8;
            fwt++;
        }

        dest->x = rootmat->right.x * accumV.x + rootmat->up.x * accumV.y +
                  rootmat->at.x * accumV.z + rootmat->pos.x;
        dest->y = rootmat->right.y * accumV.x + rootmat->up.y * accumV.y +
                  rootmat->at.y * accumV.z + rootmat->pos.y;
        dest->z = rootmat->right.z * accumV.x + rootmat->up.z * accumV.y +
                  rootmat->at.z * accumV.z + rootmat->pos.z;

        vert++;
        idx++;
        wt += 4;
        count--;
        dest++;
    }
}

// TODO: 97%, parameter registers are coloured differently.
unsigned int iModelVertEval(RpAtomic* model, unsigned int index, unsigned int count,
                            RwMatrixTag* mat, _xVec3* vert, _xVec3* dest)
{
    RpGeometry* geom = model->geometry;

    if (!vert)
    {
        unsigned int numVerts = geom->numVertices;
        if (index >= numVerts || count == 0)
        {
            return 0;
        }
        count = (count < numVerts - index) ? count : (numVerts - index);
        vert = (_xVec3*)(geom->morphTarget->verts + index);
    }

    RpSkin* skin = RpSkinGeometryGetSkin(geom);
    if (skin)
    {
        SkinXform(dest, vert, mat, RpSkinGetSkinToBoneMatrices(skin),
                  RpSkinGetVertexBoneWeights(skin) + index * 4,
                  RpSkinGetVertexBoneIndices(skin) + index, count);
    }
    else
    {
        RwV3dTransformPoints((RwV3d*)dest, (RwV3d*)vert, count, mat);
    }
    return count;
}

static unsigned int iModelTagInternal(iModelTag* tag, RpAtomic* model, float x, float y, float z,
                                      signed int closeV)
{
    RpGeometry* geom = model->geometry;
    RwV3d* vert = geom->morphTarget->verts;
    signed int numV = geom->numVertices;
    float closeDistSqr = 1.0e9f;

    if (closeV < 0 || closeV > numV)
    {
        closeV = 0;
        for (signed int v = 0; v < numV; v++)
        {
#pragma fp_contract off
            float distSqr = (vert[v].x - x) * (vert[v].x - x) + (vert[v].y - y) * (vert[v].y - y) +
                            (vert[v].z - z) * (vert[v].z - z);
#pragma fp_contract on
            if (distSqr < closeDistSqr)
            {
                closeV = v;
                closeDistSqr = distSqr;
            }
        }
        if (tag)
        {
            tag->v.x = x;
            tag->v.y = y;
            tag->v.z = z;
        }
    }
    else
    {
        if (tag)
        {
            tag->v.x = vert[closeV].x;
            tag->v.y = vert[closeV].y;
            tag->v.z = vert[closeV].z;
        }
    }

    if (tag)
    {
        RpSkin* skin = RpSkinGeometryGetSkin(model->geometry);
        if (skin)
        {
            const float* wt = RpSkinGetVertexBoneWeights(skin) + closeV * 4;
            tag->matidx = RpSkinGetVertexBoneIndices(skin)[closeV];
            tag->wt[0] = wt[0];
            tag->wt[1] = wt[1];
            tag->wt[2] = wt[2];
            tag->wt[3] = wt[3];
        }
        else
        {
            tag->matidx = 0;
            tag->wt[0] = 0.0f;
            tag->wt[1] = 0.0f;
            tag->wt[2] = 0.0f;
            tag->wt[3] = 0.0f;
        }
    }

    return closeV;
}

unsigned int iModelTagIndex(iModelTag* tag, RpAtomic* model, signed int v)
{
    return iModelTagInternal(tag, model, 0.0f, 0.0f, 0.0f, v);
}

unsigned int iModelTagSetup(iModelTag* tag, RpAtomic* model, float x, float y, float z)
{
    return iModelTagInternal(tag, model, x, y, z, -1);
}

void iModelTagEval(RpAtomic* model, iModelTag* tag, RwMatrixTag* mat, _xVec3* dest)
{
    if (tag->wt[0])
    {
        SkinXform(dest, &tag->v, mat,
                  RpSkinGetSkinToBoneMatrices(RpSkinGeometryGetSkin(model->geometry)), tag->wt,
                  &tag->matidx, 1);
    }
    else
    {
        RwV3dTransformPoints((RwV3d*)dest, (RwV3d*)&tag->v, 1, mat);
    }
}

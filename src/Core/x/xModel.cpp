#include "xEnt.h"
#include "xMemMgr.h"
#include "zGlobals.h"
#include "rwtypes.h"

extern "C" void* memcpy(void* dst, const void* src, unsigned int n);

class RwRaster;
class RpWorld;
class xMorphSeqFile;

class _xRect
{
public:
    float x; // 0x0
    float y; // 0x4
    float w; // 0x8
    float h; // 0xC
};

class RwV2d
{
public:
    float x; // 0x0
    float y; // 0x4
};

class RwVideoMode
{
public:
    signed int width;  // 0x0
    signed int height; // 0x4
    signed int depth;  // 0x8
    signed int flags;  // 0xC
};

class RwCamera
{
public:
    RwObject object;       // 0x0
    unsigned char pad8[0x60 - 0x8];
    RwRaster* frameBuffer; // 0x60
    RwRaster* zBuffer;     // 0x64
};

class RwGlobals
{
public:
    RwCamera* curCamera; // 0x0
};

class xModelPool
{
public:
    xModelPool* Next;         // 0x0
    unsigned int NumMatrices; // 0x4
    xModelInstance* List;     // 0x8
};

class xAnimFile
{
public:
    xAnimFile* Next;        // 0x0
    char* Name;             // 0x4
    unsigned int ID;        // 0x8
    unsigned int FileFlags; // 0xC
    float Duration;         // 0x10
    float TimeOffset;       // 0x14
    unsigned char pad18[0x3C - 0x18];
    void** RawData;         // 0x3C
};

class xAnimState
{
public:
    unsigned char pad0[0x18];
    xAnimFile* Data; // 0x18
};

class xAnimSingle
{
public:
    unsigned int SingleFlags; // 0x0
    xAnimState* State;        // 0x4
    float Time;               // 0x8
    unsigned char padC[0x60 - 0xC];
};

class xAnimPlay
{
public:
    xAnimPlay* Next;          // 0x0
    unsigned short NumSingle; // 0x4
    unsigned short BoneCount; // 0x6
    xAnimSingle* Single;      // 0x8
};

extern "C" {
extern RwGlobals* RwEngineInstance;
RwCamera* RwCameraCreate();
signed int RwCameraDestroy(RwCamera* camera);
RwCamera* RwCameraBeginUpdate(RwCamera* camera);
RwCamera* RwCameraEndUpdate(RwCamera* camera);
RwCamera* RwCameraSetViewWindow(RwCamera* camera, const RwV2d* viewWindow);
RwCamera* RwCameraSetViewOffset(RwCamera* camera, const RwV2d* offset);
RwRaster* RwRasterCreate(signed int width, signed int height, signed int depth, signed int flags);
signed int RwRasterDestroy(RwRaster* raster);
RwFrame* RwFrameCreate();
signed int RwFrameDestroy(RwFrame* frame);
void* _rwObjectHasFrameSetFrame(void* object, RwFrame* frame);
signed int _rwFrameSyncDirty();
signed int RwEngineGetCurrentVideoMode();
RwVideoMode* RwEngineGetVideoModeInfo(RwVideoMode* modeinfo, signed int modeIndex);
RpWorld* RpWorldAddCamera(RpWorld* world, RwCamera* camera);
RpWorld* RpWorldRemoveCamera(RpWorld* world, RwCamera* camera);
// TODO: unidentified (xAnim: returns an anim player to its pool)
void fn_80016A30(xAnimPlay* play);
}

void iModelInit();
unsigned int iModelNumBones(RpAtomic* model);
signed int iModelCull(RpAtomic* model, RwMatrixTag* mat);
void iModelRender(RpAtomic* model, RwMatrixTag* mat);
void iModelResetMaterial(RpAtomic* model);
void iModelMaterialMul(RpAtomic* model, float rm, float gm, float bm);
void iModelSetMaterialAlpha(RpAtomic* model, signed int alpha);
void iModelSetMaterialTextureRestore(RpAtomic* model);
void iModelSetMaterialTexture(RpAtomic* model, void* texture);
void iCameraUpdatePos(RwCamera* cam, _xMat4x3* pos);
void iCameraSetFOV(RwCamera* cam, float fov);
void xMorphRender(xMorphSeqFile* seq, RwMatrixTag* mat, float time);
void xAnimPlayEval(xAnimPlay* play);
void xAnimPlayUpdate(xAnimPlay* play, float timeDelta);
void xAnimPlayChooseTransition(xAnimPlay* play);
void xMat4x3Mul(_xMat4x3* o, _xMat4x3* a, _xMat4x3* b);
void xMat3x3Euler(_xMat3x3* m, float yaw, float pitch, float roll);
void xModelRender(xModelInstance* modelInst);

static xModelPool* sxModelPoolList;
static RwCamera* subcamera;
static signed int sRender2DMode;
static signed int sRender2DEnabled;

static inline void CameraDestroy(RwCamera* camera)
{
    if (camera)
    {
        _rwFrameSyncDirty();
        RwFrame* frame = (RwFrame*)camera->object.parent;
        if (frame)
        {
            _rwObjectHasFrameSetFrame(camera, 0);
            RwFrameDestroy(frame);
        }
        if (camera->frameBuffer)
        {
            RwRasterDestroy(camera->frameBuffer);
            camera->frameBuffer = 0;
        }
        if (camera->zBuffer)
        {
            RwRasterDestroy(camera->zBuffer);
            camera->zBuffer = 0;
        }
        RwCameraDestroy(camera);
    }
}

static inline RwCamera* CameraCreate(signed int width, signed int height, signed int zbuffer)
{
    RwCamera* camera = RwCameraCreate();
    if (camera)
    {
        _rwObjectHasFrameSetFrame(camera, RwFrameCreate());
        camera->frameBuffer = RwRasterCreate(width, height, 0, 2);
        if (zbuffer)
        {
            camera->zBuffer = RwRasterCreate(width, height, 0, 1);
        }
        if (camera->object.parent && camera->frameBuffer && (!zbuffer || camera->zBuffer))
        {
            return camera;
        }
    }
    CameraDestroy(camera);
    return 0;
}

static inline float xAnimFileRawTime(xAnimFile* data, float time)
{
    if ((data->FileFlags & 0x1000) || ((data->FileFlags & 0x2000) && time > 0.5f * data->Duration))
    {
        return data->TimeOffset + data->Duration - time;
    }
    return data->TimeOffset + time;
}

static inline void xModelEvalSingle(xModelInstance* modelInst)
{
    unsigned short flags = modelInst->Flags;
    if (flags & 2)
    {
        if (modelInst->Anim)
        {
            xAnimPlayEval(modelInst->Anim);
        }

        xModelInstance* dad = modelInst->Parent;
        if (dad)
        {
            if (flags & 8)
            {
                memcpy(modelInst->Mat, dad->Mat, (modelInst->BoneCount + 1) * sizeof(RwMatrixTag));
            }
            else if (flags & 0x10)
            {
                signed int i;
                unsigned char* remap = modelInst->BoneRemap;
                memcpy(modelInst->Mat, dad->Mat, sizeof(RwMatrixTag));
                for (i = 0; i < modelInst->BoneCount; i++, remap++)
                {
                    memcpy(&modelInst->Mat[i + 1], &dad->Mat[*remap + 1], sizeof(RwMatrixTag));
                }
            }
            else if (flags & 0x20)
            {
                if (dad->BoneCount && modelInst->BoneIndex)
                {
                    xMat4x3Mul((_xMat4x3*)modelInst->Mat,
                               (_xMat4x3*)&dad->Mat[modelInst->BoneIndex + 1],
                               (_xMat4x3*)dad->Mat);
                }
                else
                {
                    memcpy(modelInst->Mat, dad->Mat, sizeof(RwMatrixTag));
                }
                if (flags & 0x40)
                {
                    xMat4x3Mul((_xMat4x3*)modelInst->Mat,
                               (_xMat4x3*)&modelInst->Mat[modelInst->BoneCount + 1],
                               (_xMat4x3*)modelInst->Mat);
                }
            }
        }
    }
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xModelInit()
{
    iModelInit();
    sxModelPoolList = 0;
    if (!subcamera)
    {
        subcamera = CameraCreate(0, 0, 1);
    }
}

void xModelPoolInit(unsigned int count, unsigned int numMatrices)
{
    signed int i;
    unsigned char* buffer;
    RwMatrixTag* mat;
    xModelPool* pool;
    xModelPool* curr;
    xModelPool** prev;
    xModelInstance* inst;

    if (numMatrices < 1)
    {
        numMatrices = 1;
    }

    buffer = (unsigned char*)(((unsigned int)xMemAlloc(gActiveHeap, count * numMatrices * sizeof(RwMatrixTag) + sizeof(xModelPool) + count * sizeof(xModelInstance) + 15) + 15) & ~15);
    mat = (RwMatrixTag*)buffer;
    buffer += count * numMatrices * sizeof(RwMatrixTag);
    pool = (xModelPool*)buffer;
    buffer += sizeof(xModelPool);
    inst = (xModelInstance*)buffer;

    for (i = 0; i < (signed int)count; i++)
    {
        inst[i].Next = &inst[i + 1];
        inst[i].Pool = pool;
        inst[i].Mat = mat;
        mat += numMatrices;
    }
    inst[count - 1].Next = 0;

    pool->NumMatrices = numMatrices;
    pool->List = inst;

    curr = sxModelPoolList;
    prev = &sxModelPoolList;
    while (curr && numMatrices < curr->NumMatrices)
    {
        prev = &curr->Next;
        curr = curr->Next;
    }
    pool->Next = curr;
    *prev = pool;
}

xModelInstance* xModelInstanceAlloc(RpAtomic* data, void* object, unsigned short flags,
                                    unsigned char boneIndex, unsigned char* boneRemap)
{
    signed int i;
    unsigned int boneCount;
    unsigned int matCount;
    xModelPool* curr;
    xModelPool* found = 0;
    xModelInstance* dude;

    boneCount = iModelNumBones(data);
    matCount = 1 + boneCount + ((flags >> 6) & 1);

    for (curr = sxModelPoolList; curr; curr = curr->Next)
    {
        if (curr->List && matCount <= curr->NumMatrices)
        {
            found = curr;
        }
    }

    dude = found->List;
    found->List = dude->Next;

    dude->Next = 0;
    dude->Parent = 0;
    dude->Anim = 0;
    dude->Data = data;
    dude->Object = object;
    dude->Flags = flags | 3;
    dude->BoneCount = boneCount;
    dude->BoneIndex = boneIndex;
    dude->BoneRemap = boneRemap;
    dude->Scale.x = 0.0f;
    dude->Scale.y = 0.0f;
    dude->Scale.z = 0.0f;

    for (i = 0; i < (signed int)matCount; i++)
    {
        memcpy(&dude->Mat[i], &g_I3, sizeof(RwMatrixTag));
    }

    return dude;
}

#pragma auto_inline on
static xModelInstance* FindChild(xModelInstance* model)
{
    xModelInstance* child = model->Next;
    while (child)
    {
        if (child->Parent == model)
        {
            return child;
        }
        child = child->Next;
    }
    return 0;
}

void xModelInstanceFree(xModelInstance* model)
{
    xModelInstance* prev;
    xModelInstance* curr;

    while (prev = FindChild(model), prev != 0)
    {
        xModelInstanceFree(prev);
    }

    prev = model->Parent;
    if (prev)
    {
        for (curr = prev->Next; curr && curr != model; curr = curr->Next)
        {
            prev = curr;
        }
        prev->Next = curr->Next;
    }

    if (model->Anim)
    {
        fn_80016A30(model->Anim);
        model->Anim = 0;
    }

    model->Next = model->Pool->List;
    model->Pool->List = model;
}
#pragma auto_inline off

void xModelInstanceAttach(xModelInstance* inst, xModelInstance* parent)
{
    xModelInstance* curr = parent;
    while (curr->Next)
    {
        curr = curr->Next;
    }
    curr->Next = inst;
    inst->Parent = parent;
}

void xModelUpdate(xModelInstance* modelInst, float timeDelta)
{
    for (; modelInst; modelInst = modelInst->Next)
    {
        if (modelInst->Anim && (modelInst->Flags & 4))
        {
            if (modelInst->Flags & 0x100)
            {
                xAnimPlayChooseTransition(modelInst->Anim);
            }
            xAnimPlayUpdate(modelInst->Anim, timeDelta);
        }
    }
}

void xModelEval(xModelInstance* modelInst)
{
    while (modelInst)
    {
        xModelEvalSingle(modelInst);
        modelInst = modelInst->Next;
    }
}

void xModelRenderSingle(xModelInstance* modelInst)
{
    _xMat3x3 tmpmat;

    if (modelInst->Flags & 1)
    {
        if (0.0f != modelInst->Scale.x)
        {
            tmpmat = *(_xMat3x3*)modelInst->Mat;
            modelInst->Mat->right.x *= modelInst->Scale.x;
            modelInst->Mat->right.y *= modelInst->Scale.x;
            modelInst->Mat->right.z *= modelInst->Scale.x;
            modelInst->Mat->up.x *= modelInst->Scale.y;
            modelInst->Mat->up.y *= modelInst->Scale.y;
            modelInst->Mat->up.z *= modelInst->Scale.y;
            modelInst->Mat->at.x *= modelInst->Scale.z;
            modelInst->Mat->at.y *= modelInst->Scale.z;
            modelInst->Mat->at.z *= modelInst->Scale.z;
        }

        if (!iModelCull(modelInst->Data, modelInst->Mat))
        {
            if (modelInst->Flags & 0x80)
            {
                xAnimPlay* a = modelInst->Anim;
                for (unsigned short i = 0; i < a->NumSingle; i++)
                {
                    if (a->Single[i].SingleFlags & 0x8000)
                    {
                        if (a->Single[i].State)
                        {
                            xMorphRender(
                                (xMorphSeqFile*)a->Single[i].State->Data->RawData[0],
                                modelInst->Mat,
                                xAnimFileRawTime(a->Single[i].State->Data, a->Single[i].Time));
                        }
                        break;
                    }
                }
            }
            else
            {
                iModelRender(modelInst->Data, modelInst->Mat);
            }
        }

        if (modelInst->Scale.x)
        {
            *(_xMat3x3*)modelInst->Mat = tmpmat;
        }
    }
}

void xModelRender(xModelInstance* modelInst)
{
    while (modelInst)
    {
        xModelRenderSingle(modelInst);
        modelInst = modelInst->Next;
    }
}

// TODO: GameCube-only; the name is unknown. Picks the camera preset that the
// next xModelRender2D uses.
extern "C" void fn_80030494(signed int mode, float enable)
{
    sRender2DMode = mode;
    sRender2DEnabled = enable > 0.0f ? 1 : 0;
}

void xModelRender2D(xModelInstance* modelInst, _xRect* sr, _xVec3* from, _xVec3* to, float fov)
{
    float dist = 0.0f;
    float viewscaleX = 1.0f;
    float viewscaleY = viewscaleX;
    float yaw = 3.1415927f;
    _xMat4x3 cammat;
    RwVideoMode mode;
    RwV2d offset;
    RwV2d vw;

    if (!sRender2DEnabled)
    {
        return;
    }

    switch (sRender2DMode)
    {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        dist = 6.3001f;
        viewscaleX = 0.4893799f;
        viewscaleY = 0.3772403f;
        yaw = 3.5f;
        break;
    case 9:
        dist = 5.750003f;
        viewscaleX = 0.8769528f;
        viewscaleY = 0.3797596f;
        yaw = 0.0f;
        break;
    case 7:
        dist = 11.874952f;
        viewscaleX = 0.4462602f;
        viewscaleY = 0.3378909f;
        break;
    case 8:
        dist = 19.99983f;
        viewscaleX = 0.3664008f;
        viewscaleY = 0.4427503f;
        break;
    case 14:
        dist = 33.12471f;
        viewscaleX = 0.4910994f;
        viewscaleY = 0.3686507f;
        break;
    case 11:
        dist = 5.0f;
        viewscaleX = 0.6303097f;
        viewscaleY = 0.50421f;
        break;
    case 12:
        dist = 55.626087f;
        viewscaleX = 0.4766497f;
        viewscaleY = 0.3861006f;
        break;
    case 13:
        dist = 9.899982f;
        viewscaleX = 0.7654892f;
        viewscaleY = 0.3862704f;
        break;
    case 15:
        break;
    }

    sRender2DMode = 0;

    RwCamera* camera = RwEngineInstance->curCamera;
    if (sr->w <= 0.0f)
    {
        return;
    }
    if (sr->h <= 0.0f)
    {
        return;
    }
    if (sr->x < -sr->w)
    {
        return;
    }
    if (sr->y < -sr->h)
    {
        return;
    }
    if (sr->x > 640.0f + sr->w)
    {
        return;
    }
    if (sr->y > 480.0f + sr->y)
    {
        return;
    }

    if (camera)
    {
        RwCameraEndUpdate(camera);
    }

    memcpy(&cammat, &g_I3, sizeof(_xMat4x3));
    _xMat4x3* objmat = (_xMat4x3*)modelInst->Mat;
    objmat->pos.x = 0.0f;
    objmat->pos.y = 0.0f;
    objmat->pos.z = dist;
    if (0.0f != yaw)
    {
        xMat3x3Euler((_xMat3x3*)objmat, yaw, 0.0f, 0.0f);
    }
    iCameraUpdatePos(camera, &cammat);

    RwEngineGetVideoModeInfo(&mode, RwEngineGetCurrentVideoMode());
    if (mode.flags & 1)
    {
        vw.x = 0.5f;
        vw.y = 0.375f;
    }
    else
    {
        vw.x = 0.5f;
        vw.y = 0.375f;
    }
    RwCameraSetViewWindow(camera, &vw);

    float x = sr->x;
    float y = sr->y;
    offset.x = (x - 320.0f) / 320.0f * viewscaleX;
    offset.y = -((y - 240.0f) / 240.0f) * viewscaleY;
    RwCameraSetViewOffset(camera, &offset);
    RwCameraBeginUpdate(camera);
    xModelRender(modelInst);
    RwCameraEndUpdate(camera);
    offset.x = 0.0f;
    offset.y = 0.0f;
    RwCameraSetViewOffset(camera, &offset);
    if (camera)
    {
        RwCameraBeginUpdate(camera);
    }
    iCameraUpdatePos(*(RwCamera**)&globals.camera, &globals.camera.mat);
    iCameraSetFOV(*(RwCamera**)&globals.camera, *(float*)((unsigned char*)&globals.camera + 0x110));
}

// Dead-stripped by the linker: only its constants survive (they open the
// unit's .sdata2). The original function is unknown.
void xModelScreenSize(float* width, float* height)
{
    *width = 640.0f;
    *height = 480.0f;
}

void xModelSetMaterialTexture(xModelInstance* modelInst, void* texture)
{
    iModelSetMaterialTexture(modelInst->Data, texture);
}

void xModelSetMaterialTextureRestore(xModelInstance* modelInst)
{
    iModelSetMaterialTextureRestore(modelInst->Data);
}

void xModelSetMaterialAlpha(xModelInstance* modelInst, signed int alpha)
{
    iModelSetMaterialAlpha(modelInst->Data, alpha);
}

void xModelMaterialMul(xModelInstance* modelInst, float rm, float gm, float bm)
{
    iModelMaterialMul(modelInst->Data, rm, gm, bm);
}

void xModelResetMaterial(xModelInstance* modelInst)
{
    iModelResetMaterial(modelInst->Data);
}

void xModel_SceneEnter(RpWorld* world)
{
    RpWorldAddCamera(world, subcamera);
}

void xModel_SceneExit(RpWorld* world)
{
    RpWorldRemoveCamera(world, subcamera);
}

// TODO: 99.9% (NonMatching). Remaining: register allocation in xAnimFileNew
// (inlined NewBilinear), xAnimPlayEval, xAnimFileEval's root loop and
// AddTransition; xAnimFileSetPoses has one extra branch; SingleUpdate keeps a
// dead block for MWCC's inline budget (see the comment there).
#include "xAnim.h"
#include "xMath.h"
#include "xMath3.h"
#include "xMemMgr.h"
#include "xString.h"

#ifndef NULL
#define NULL 0
#endif

extern "C" {
void* memset(void* dst, int c, unsigned int n);
signed int printf(const char* format, ...);
double floor(double x);
}

class xModelInstance;
class xMorphSeqFile;
class xAnimActiveEffect;

class xAnimEffect
{
public:
    xAnimEffect* Next;   // 0x0
    unsigned int Flags;  // 0x4
    float StartTime;     // 0x8
    float EndTime;       // 0xC
    unsigned int (*Callback)(unsigned int, xAnimActiveEffect*, xAnimSingle*, void*); // 0x10
};

class xAnimActiveEffect
{
public:
    xAnimEffect* Effect; // 0x0
    unsigned int Handle; // 0x4
};

class xAnimTransition
{
public:
    xAnimTransition* Next;                       // 0x0
    xAnimState* Dest;                            // 0x4
    xAnimTransitionConditionalCallback Conditional; // 0x8
    xAnimTransitionCallback Callback;            // 0xC
    unsigned int Flags;                          // 0x10
    unsigned int UserFlags;                      // 0x14
    float SrcTime;                               // 0x18
    float DestTime;                              // 0x1C
    unsigned short Priority;                     // 0x20
    unsigned short QueuePriority;                // 0x22
    float BlendRecip;                            // 0x24
    unsigned short* BlendOffset;                 // 0x28
};

class xAnimTransitionList
{
public:
    xAnimTransitionList* Next; // 0x0
    xAnimTransition* T;        // 0x4
};

class xAnimFile
{
public:
    xAnimFile* Next;          // 0x0
    char* Name;               // 0x4
    unsigned int ID;          // 0x8
    unsigned int FileFlags;   // 0xC
    float Duration;           // 0x10
    float TimeOffset;         // 0x14
    float PhysBase[4];        // 0x18
    float PhysDelta[4];       // 0x28
    unsigned short BoneCount; // 0x38
    unsigned char NumAnims[2]; // 0x3A
    void** RawData;           // 0x3C
    void* Physics;            // 0x40
};

class xAnimState
{
public:
    xAnimState* Next;             // 0x0
    char* Name;                   // 0x4
    unsigned int ID;              // 0x8
    unsigned int Flags;           // 0xC
    unsigned int UserFlags;       // 0x10
    float Speed;                  // 0x14
    xAnimFile* Data;              // 0x18
    xAnimEffect* Effects;         // 0x1C
    xAnimTransitionList* Default; // 0x20
    xAnimTransitionList* List;    // 0x24
    float* BoneBlend;             // 0x28
    float* TimeSnap;              // 0x2C
    float FadeRecip;              // 0x30
    unsigned short* FadeOffset;   // 0x34
};

class xAnimPlay;

class xAnimSingle
{
public:
    unsigned int SingleFlags;      // 0x0
    xAnimState* State;             // 0x4
    float Time;                    // 0x8
    float CurrentSpeed;            // 0xC
    float BilinearLerp[2];         // 0x10
    xAnimEffect* Effect;           // 0x18
    unsigned int ActiveCount;      // 0x1C
    xAnimActiveEffect* ActiveList; // 0x20
    xAnimPlay* Play;               // 0x24
    float LastTime;                // 0x28
    _xVec3 LastTranslation;        // 0x2C
    float LastYaw;                 // 0x38
    unsigned int pad;              // 0x3C
    _xQuat LastQuat;               // 0x40
    xAnimTransition* Sync;         // 0x50
    xAnimTransition* Tran;         // 0x54
    xAnimSingle* Blend;            // 0x58
    float BlendFactor;             // 0x5C
};

class xAnimTable
{
public:
    xAnimTable* Next;                // 0x0
    char* Name;                      // 0x4
    xAnimTransition* TransitionList; // 0x8
    xAnimState* StateList;           // 0xC
    unsigned int AnimIndex;          // 0x10
    unsigned int MorphIndex;         // 0x14
};

class xAnimPlay
{
public:
    xAnimPlay* Next;              // 0x0
    unsigned short NumSingle;     // 0x4
    unsigned short BoneCount;     // 0x6
    xAnimSingle* Single;          // 0x8
    void* Object;                 // 0xC
    xAnimTable* Table;            // 0x10
    _xVec3 Translate[2];          // 0x14
    unsigned int pad1;            // 0x2C
    _xQuat Quat[2];               // 0x30
    float Yaw;                    // 0x50
    xMemPool* Pool;               // 0x54
    xModelInstance* ModelInst;    // 0x58
    unsigned int pad2;            // 0x5C
};

static xMemPool sxAnimTempTranPool;
unsigned int gxAnimUseGrowAlloc;
extern unsigned char* giAnimScratch;

void iAnimInit();
void iAnimEval(void* RawData, float time, unsigned int flags, _xVec3* tran, _xQuat* quat);
float iAnimDuration(void* RawData);
unsigned int iAnimBoneCount(void* RawData);
void iAnimBlend(float BlendFactor, float BlendRecip, unsigned short* BlendTimeOffset,
                float* BoneTable, unsigned int BoneCount, _xVec3* src1, _xQuat* src1quat,
                _xVec3* src2, _xQuat* src2quat, _xVec3* tparr, _xQuat* qparr);
float xMorphSeqDuration(xMorphSeqFile* seq);
void xQuatMul(_xQuat* o, _xQuat* a, _xQuat* b);

static void CalcPhys(xAnimFile* data);
static void CalcPhysFromKeys(xAnimFile* data, float* k0, float* k1, float* d0, float* d1);
float iurand();
void xAnimFileEval(xAnimFile* data, float time, float* bilinear, unsigned int flags, _xVec3* tran,
                   _xQuat* quat, float* yaw);
void xAnimPlaySetState(xAnimSingle* single, xAnimState* state, float startTime);
static void CalcLastAndDeltas(xAnimSingle* single, _xVec3* tran, _xQuat* quat, float* yaw,
                              unsigned int lastOnly);
static void LoopUpdate(xAnimSingle* single, _xVec3* tran, _xQuat* quat, float* yaw);
static void StopUpdate(xAnimSingle* single, _xVec3* tran, _xQuat* quat, float* yaw);
static void EffectSingleLoop(xAnimSingle* single);
static void EffectSingleRun(xAnimSingle* single);
static void EffectSingleStart(xAnimSingle* single);
void xAnimPlayStartTransition(xAnimPlay* play, xAnimTransition* transition);

// Yaw of a root rotation: rotate +Z and take the angle in the XZ plane.
static inline float xAnimAbsf(float x)
{
    return __fabs(x);
}

static inline float xAnimQuatYaw(_xQuat* q)
{
    _xQuat z = { 0.0f, 0.0f, 1.0f, 0.0f };
    _xQuat r;
    xQuatMul(&r, q, &z);
    if (xAnimAbsf(r.v.x) + xAnimAbsf(r.v.z) < 0.0001f)
    {
        return 0.0f;
    }
    return xAngleClamp(atan2(r.v.x, r.v.z));
}

static inline float xAnimFileRawTime(xAnimFile* data)
{
    if (data->FileFlags & 0x8000)
    {
        return xMorphSeqDuration((xMorphSeqFile*)*data->RawData);
    }
    return iAnimDuration(*data->RawData);
}

xAnimState* xAnimTableGetStateID(xAnimTable* table, unsigned int ID);
xAnimState* xAnimTableGetState(xAnimTable* table, char* name);
xAnimState* xAnimTableAddFileID(xAnimTable* table, xAnimFile* file, unsigned int stateID);
void xAnimTableAddTransition(xAnimTable* table, xAnimTransition* tran, char* source);

void xAnimFileEval(xAnimFile* data, float time, float* bilinear, unsigned int flags, _xVec3* tran,
                   _xQuat* quat, float* yaw);

// ---- effect helpers ----

static inline void EffectActiveInsert(xAnimSingle* single, xAnimActiveEffect* active)
{
    unsigned int index = 0;
    unsigned int count = single->ActiveCount;
    xAnimActiveEffect* curr = single->ActiveList;

    while (index < count && curr->Effect != NULL)
    {
        curr++;
        index++;
    }

    *curr = *active;

    if (index < count - 1)
    {
        (curr + 1)->Effect = NULL;
    }
}

static inline void EffectActiveRemove(xAnimActiveEffect* active, unsigned int index,
                                      unsigned int count)
{
    count--;

    if (index == count || (active + 1)->Effect == NULL)
    {
        active->Effect = NULL;
    }
    else
    {
        xAnimActiveEffect* curr = active;

        while (index < count && (curr + 1)->Effect != NULL)
        {
            curr++;
            index++;
        }

        *active = *curr;
        curr->Effect = NULL;
    }
}

static inline unsigned int EffectPlaying(xAnimSingle* single, xAnimEffect* effect)
{
    unsigned int index = 0;

    while (index < single->ActiveCount && single->ActiveList[index].Effect != NULL)
    {
        if (single->ActiveList[index].Effect == effect)
        {
            return 1;
        }
        index++;
    }

    return 0;
}

static inline void EffectSingleStop(xAnimSingle* single)
{
    if (single->State == NULL || single->LastTime == -1.0f)
    {
        return;
    }

    for (unsigned int i = 0; i < single->ActiveCount && single->ActiveList[i].Effect != NULL; i++)
    {
        if (single->ActiveList[i].Effect->Flags & 2)
        {
            single->ActiveList[i].Effect->Callback(3, &single->ActiveList[i], single,
                                                   single->Play->Object);
        }
    }

    single->ActiveList->Effect = NULL;

    for (xAnimEffect* effect = single->Effect; effect != NULL; effect = effect->Next)
    {
        xAnimActiveEffect tempActive;
        if ((effect->Flags & 9) == 9)
        {
            tempActive.Effect = effect;
            tempActive.Handle = effect->Callback(1, &tempActive, single, single->Play->Object);
            if (effect->Flags & 2)
            {
                effect->Callback(3, &tempActive, single, single->Play->Object);
            }
        }
    }

    single->Effect = NULL;
}


class RpAtomic;

class xModelPool;

class RwMatrixTag
{
public:
    unsigned char pad[0x40];
};

class xModelInstance
{
public:
    xModelInstance* Next;     // 0x0
    xModelInstance* Parent;   // 0x4
    xModelPool* Pool;         // 0x8
    xAnimPlay* Anim;          // 0xC
    RpAtomic* Data;           // 0x10
    void* Object;             // 0x14
    unsigned short Flags;     // 0x18
    unsigned char BoneCount;  // 0x1A
    unsigned char BoneIndex;  // 0x1B
    unsigned char* BoneRemap; // 0x1C
    RwMatrixTag* Mat;         // 0x20
    _xVec3 Scale;             // 0x24
};

extern "C" {
void* memcpy(void* dst, const void* src, unsigned int n);

struct _MSL_FILE
{
    unsigned char pad[0x50];
};

#ifndef PC_PORT
extern _MSL_FILE __files[3];
signed int fprintf(_MSL_FILE* stream, const char* format, ...);
#endif
}

void iModelAnimMatrices(RpAtomic* model, _xQuat* quat, _xVec3* tran, RwMatrixTag* mat);

void xQuatSlerp(_xQuat* q, _xQuat* a, _xQuat* b, float t);

void xAnimPlaySetup(xAnimPlay* play, void* object, xAnimTable* table, xModelInstance* modelInst);

void xAnimPoolCB(xMemPool* pool, void* data);

static void SingleUpdate(xAnimSingle* single, float timeDelta, _xVec3* tran, _xQuat* quat,
                         float* yaw, unsigned int);

static inline void EffectSingleDuration(xAnimSingle* single)
{
    float time = single->Time;
    xAnimActiveEffect* alist = single->ActiveList;
    unsigned int index = 0;
    unsigned int count = single->ActiveCount;

    while (index < count && alist->Effect != NULL)
    {
        if (!(alist->Effect->Flags & 0x20) && alist->Effect->EndTime <= time)
        {
            alist->Effect->Callback(3, alist, single, single->Play->Object);
            EffectActiveRemove(alist, index, count);
        }
        else
        {
            alist->Effect->Callback(2, alist, single, single->Play->Object);
            alist++;
            index++;
        }
    }
}

static inline void TransitionTimeInit(xAnimSingle* single, xAnimTransition* tran)
{
    if (tran->Flags & 0x20)
    {
        xAnimFile* destData = tran->Dest->Data;
        if ((single->State->Data->FileFlags ^ destData->FileFlags) & 0x1000)
        {
            single->Time = destData->Duration - single->Time;
        }
    }
    else
    {
        single->Time = ((tran->Dest->Flags & 0x100) && tran->DestTime == 0.0f) ?
                           single->State->Data->Duration * iurand() :
                           tran->DestTime;
    }
    single->LastTime = single->Time;
}

static inline void SingleEval(xAnimSingle* single, _xVec3* tran, _xQuat* quat)
{
    if (single->Play->BoneCount > 1)
    {
        xAnimFileEval(single->State->Data, single->Time, single->BilinearLerp, 2, tran + 1,
                      quat + 1, NULL);

        if (single->Blend != NULL && single->Blend->State != NULL)
        {
            _xQuat* qbuf = (_xQuat*)(giAnimScratch + 0xE40);
            _xVec3* vbuf = (_xVec3*)((unsigned char*)qbuf + 0x410);

            xAnimFileEval(single->Blend->State->Data, single->Blend->Time,
                          single->Blend->BilinearLerp, 2, vbuf, qbuf, NULL);

            iAnimBlend(single->BlendFactor, single->Tran->BlendRecip, single->Tran->BlendOffset,
                       NULL, single->Play->BoneCount - 1, vbuf, qbuf, tran + 1, quat + 1,
                       tran + 1, quat + 1);
        }
    }
}

#define ADD_4_BITS(x) (((x) & 1) + (((x) >> 1) & 1) + (((x) >> 2) & 1) + (((x) >> 3) & 1))

static void EffectSingleLoop(xAnimSingle* single);

static void EffectSingleRun(xAnimSingle* single);

static inline float CalcRecipBlendMax(unsigned short* offset)
{
    float max = 0.0f;
    while (offset[0] != 0xFFFF)
    {
        float t;
        if (offset[1] == 0)
        {
            t = 0.0f;
        }
        else
        {
            t = 1.0f / (offset[1] / 1024.0f);
        }
        t = offset[0] / 1024.0f + t;
        if (t > max)
        {
            max = t;
        }
        offset += 2;
    }
    if (max == 0.0f)
    {
        return 0.0f;
    }
    return 1.0f / max;
}

static inline unsigned int StateHasTransition(xAnimState* state, xAnimTransition* tran)
{
    xAnimTransitionList* curr = state->List;
    while (curr != NULL)
    {
        if (curr->T == tran)
        {
            return 1;
        }
        curr = curr->Next;
    }
    return 0;
}

static inline unsigned int DefaultHasTransition(xAnimState* state, xAnimTransition* tran,
                                                unsigned int* count)
{
    unsigned int add = 1;
    xAnimTransitionList* curr = state->Default;
    while (curr != NULL)
    {
        if (curr->T == tran)
        {
            return 1;
        }
        if (curr->T->Conditional == tran->Conditional && curr->T->UserFlags == tran->UserFlags)
        {
            add = 0;
        }
        curr = curr->Next;
    }
    *count += add;
    return 0;
}

static inline unsigned int DefaultOverride(xAnimState* state, xAnimTransition* tran)
{
    xAnimTransitionList* curr = state->Default;
    while (curr != NULL)
    {
        if (curr->T->Conditional == tran->Conditional && curr->T->UserFlags == tran->UserFlags)
        {
            curr->T = tran;
            return 1;
        }
        curr = curr->Next;
    }
    return 0;
}

xAnimFile* xAnimFileNewBilinear(void** rawData, char* name, unsigned int flags,
                                xAnimFile** linkedList, unsigned int numX, unsigned int numY);

void xAnimInit()
{
    iAnimInit();
    memset(&sxAnimTempTranPool, 0, sizeof(xMemPool));
}

void xAnimTempTransitionInit(unsigned int count)
{
    xMemPoolSetup(&sxAnimTempTranPool,
                  xMemAlloc(gActiveHeap, count * sizeof(xAnimTransition)), 0, 0, NULL,
                  sizeof(xAnimTransition), count, count / 2);
}

xAnimTransition* xAnimTempTransitionAlloc()
{
    xAnimTransition* tran = (xAnimTransition*)xMemPoolAlloc(&sxAnimTempTranPool);
    tran->Flags = 2;
    return tran;
}

static void CalcPhysFromKeys(xAnimFile* data, float* k0, float* k1, float* d0, float* d1)
{
    unsigned int flags = data->FileFlags;

    if (flags & 0x1000)
    {
        float* tk = k0;
        float* td = d0;
        k0 = k1;
        d0 = d1;
        k1 = tk;
        d1 = td;
    }
    if (flags & 0x2000)
    {
        k1 = k0;
        d1 = d0;
    }

    for (signed int i = 0; i < 4; i++)
    {
        if (data->FileFlags & (0x10 << i))
        {
            data->PhysBase[i] = d0[i];
            data->PhysDelta[i] = 0.0f;
        }
        else if (data->FileFlags & (0x100 << i))
        {
            data->PhysBase[i] = d0[i];
            float delta = d1[i] - d0[i];
            if (i == 3)
            {
                while (delta > PI)
                {
                    delta -= 2.0f * PI;
                }
                while (delta < -PI)
                {
                    delta += 2.0f * PI;
                }
            }
            data->PhysDelta[i] = delta / data->Duration;
        }
        else
        {
            data->PhysBase[i] = d0[i] - k0[i];
            if (i == 3)
            {
                while (data->PhysBase[i] > 2.0f * PI)
                {
                    data->PhysBase[i] -= 2.0f * PI;
                }
                while (data->PhysBase[i] < 0.0f)
                {
                    data->PhysBase[i] += 2.0f * PI;
                }
            }
            float delta = (d1[i] - d0[i]) - (k1[i] - k0[i]);
            if (i == 3)
            {
                while (delta > PI)
                {
                    delta -= 2.0f * PI;
                }
                while (delta < -PI)
                {
                    delta += 2.0f * PI;
                }
            }
            data->PhysDelta[i] = delta / data->Duration;
        }
    }
}

static void CalcPhys(xAnimFile* data)
{
    if (data->Physics != NULL)
    {
        return;
    }

    if (data->FileFlags & 0x8000)
    {
        for (signed int i = 0; i < 4; i++)
        {
            data->PhysBase[i] = 0.0f;
            data->PhysDelta[i] = 0.0f;
        }
        return;
    }

    void* raw = *data->RawData;
    float k1[4];
    float k2[4];
    float k0[4];
    float d0[4];
    float d1[4];
    _xQuat quat;

    iAnimEval(raw, 0.0f, 1, (_xVec3*)k0, &quat);
    k0[3] = xAnimQuatYaw(&quat);
    iAnimEval(raw, data->TimeOffset, 1, (_xVec3*)k1, &quat);
    k1[3] = xAnimQuatYaw(&quat);
    iAnimEval(raw, data->TimeOffset + data->Duration, 1, (_xVec3*)k2, &quat);
    k2[3] = xAnimQuatYaw(&quat);

    for (signed int i = 0; i < 4; i++)
    {
        if (data->FileFlags & (1 << i))
        {
            d0[i] = k1[i];
        }
        else
        {
            d0[i] = k1[i] - k0[i];
        }
        d1[i] = d0[i] + k2[i] - k1[i];
        if (i == 3)
        {
            while (d1[i] > 2.0f * PI)
            {
                d1[i] -= 2.0f * PI;
            }
            while (d1[i] < 0.0f)
            {
                d1[i] += 2.0f * PI;
            }
        }
    }

    CalcPhysFromKeys(data, k1, k2, d0, d1);
}

#pragma auto_inline on
xAnimFile* xAnimFileNewBilinear(void** rawData, char* name, unsigned int flags,
                                xAnimFile** linkedList, unsigned int numX, unsigned int numY)
{
    xAnimFile* afile;

    afile = gxAnimUseGrowAlloc ?
                (xAnimFile*)xMemGrowAlloc(gActiveHeap, numX * numY * sizeof(void*) + 0x44) :
                (xAnimFile*)xMemAlloc(gActiveHeap, numX * numY * sizeof(void*) + 0x44);

    if (numX > 1 || numY > 1)
    {
        flags |= 0x4000;
    }
    if (*(unsigned int*)*rawData == 'QSPM')
    {
        flags |= 0x8000;
    }

    if (linkedList != NULL)
    {
        afile->Next = *linkedList;
        *linkedList = afile;
    }
    else
    {
        afile->Next = NULL;
    }

    afile->RawData = (void**)((unsigned char*)afile + 0x44);
    for (signed int i = 0; i < (signed int)(numX * numY); i++)
    {
        afile->RawData[i] = rawData[i];
    }

    afile->Name = name;
    afile->ID = xStrHash(name);
    afile->FileFlags = flags;
    afile->Duration = (afile->FileFlags & 0x2000) ? 2.0f * xAnimFileRawTime(afile) :
                                                     xAnimFileRawTime(afile);
    afile->TimeOffset = 0.0f;
    afile->BoneCount = (flags & 0x8000) ? 0 : iAnimBoneCount(*rawData);
    afile->NumAnims[0] = numX;
    afile->NumAnims[1] = numY;
    afile->Physics = NULL;
    CalcPhys(afile);
    return afile;
}
#pragma auto_inline reset

xAnimFile* xAnimFileNew(void* rawData, char* name, unsigned int flags, xAnimFile** linkedList)
{
    return xAnimFileNewBilinear(&rawData, name, flags, linkedList, 1, 1);
}

void xAnimFileSetPhysics(xAnimFile* data, void* physics)
{
    data->Physics = physics;
}

void xAnimFileSetTime(xAnimFile* data, float duration, float timeOffset)
{
    float rawDuration = xAnimFileRawTime(data);

    if (timeOffset > rawDuration - 0.1f)
    {
        timeOffset = rawDuration - 0.1f;
    }
    data->TimeOffset = timeOffset;
    if (duration > rawDuration - timeOffset)
    {
        duration = rawDuration - timeOffset;
    }
    data->Duration = (data->FileFlags & 0x2000) ? 2.0f * duration : duration;
    CalcPhys(data);
}

void xAnimFileSetPoses(xAnimFile* data, void* startPose, float startTime, void* endPose,
                       float endTime)
{
    float k0[4];
    float k1[4];
    float p0[4];
    float p1[4];
    float d0[4];
    float d1[4];
    _xQuat quat;

    if (data->Physics == NULL)
    {
        if (data->FileFlags & 0x8000)
        {
            return;
        }
        {
            void* raw = *data->RawData;

            iAnimEval(raw, data->TimeOffset, 1, (_xVec3*)k0, &quat);
            k0[3] = xAnimQuatYaw(&quat);
            iAnimEval(raw, data->TimeOffset + data->Duration, 1, (_xVec3*)k1, &quat);
            k1[3] = xAnimQuatYaw(&quat);
            iAnimEval(startPose, startTime, 1, (_xVec3*)p0, &quat);
            p0[3] = xAnimQuatYaw(&quat);
            iAnimEval(endPose, endTime, 1, (_xVec3*)p1, &quat);
            p1[3] = xAnimQuatYaw(&quat);

            for (signed int i = 0; i < 4; i++)
            {
                d0[i] = k0[i] - p0[i];
                d1[i] = k1[i] - p1[i];
                if (data->FileFlags & (1 << i))
                {
                    d0[i] += p0[i];
                    d1[i] += p0[i];
                }
            }

            CalcPhysFromKeys(data, k0, k1, d0, d1);
        }
    }
}


#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, a, b) (MAX((a), MIN((x), (b))))

static inline float xAnimFileEvalTime(xAnimFile* data, float time)
{
    if ((data->FileFlags & 0x1000) || ((data->FileFlags & 0x2000) && time > 0.5f * data->Duration))
    {
        return data->TimeOffset + data->Duration - time;
    }
    return data->TimeOffset + time;
}

void xAnimFileEval(xAnimFile* data, float time, float* bilinear, unsigned int flags, _xVec3* tran,
                   _xQuat* quat, float* yaw)
{
    signed int i;
    unsigned int numBones;
    float bilerp[2];
    unsigned int biindex[2];
    unsigned int biplus[2];
    _xQuat* q0;
    _xVec3* t0;
    _xQuat* q1;
    _xVec3* t1;
    unsigned int fileFlags = data->FileFlags;
    float t = CLAMP(time, 0.0f, data->Duration);
    float evalTime = xAnimFileEvalTime(data, t);

    if (fileFlags & 0x8000)
    {
        if (flags & 4)
        {
            if (data->Physics != NULL)
            {
                iAnimEval(data->Physics, evalTime, 1, tran, quat);
                xAnimQuatYaw(quat);
            }
            else
            {
                tran->x = 0.0f;
                tran->y = 0.0f;
                tran->z = 0.0f;
                quat->v.x = 0.0f;
                quat->v.y = 0.0f;
                quat->v.z = 0.0f;
                quat->s = 1.0f;
                *yaw = 0.0f;
            }
        }
        return;
    }

    if (flags & 1)
    {
        numBones = 1;
    }
    else
    {
        numBones = data->BoneCount;
    }
    if (flags & 2)
    {
        numBones--;
    }
    if (numBones == 0)
    {
        return;
    }

    if (flags & 4)
    {
        tran++;
        quat++;
    }

    if (bilinear != NULL && (fileFlags & 0x4000))
    {
        for (i = 0; i < 2; i++)
        {
            float f = CLAMP(bilinear[i], 0.0f, data->NumAnims[i] - 1);
            float fl = floor(f);
            bilerp[i] = f - fl;
            biindex[i] = fl;
            biplus[i] = biindex[i] + 1 < data->NumAnims[i] ? biindex[i] + 1 : biindex[i];
        }

        q0 = (_xQuat*)(giAnimScratch + 0x1560);
        t0 = (_xVec3*)((unsigned char*)q0 + 0x410);
        if (bilerp[0] && bilerp[1])
        {
            q1 = (_xQuat*)(giAnimScratch + 0x1C80);
            t1 = (_xVec3*)((unsigned char*)q1 + 0x410);
            iAnimEval(data->RawData[biindex[0] + biindex[1] * data->NumAnims[0]], evalTime, flags,
                      tran, quat);
            iAnimEval(data->RawData[biplus[0] + biindex[1] * data->NumAnims[0]], evalTime, flags,
                      t0, q0);
            iAnimBlend(bilerp[0], 1.0f, NULL, NULL, numBones, tran, quat, t0, q0, tran, quat);
            iAnimEval(data->RawData[biindex[0] + biplus[1] * data->NumAnims[0]], evalTime, flags,
                      t0, q0);
            iAnimEval(data->RawData[biplus[0] + biplus[1] * data->NumAnims[0]], evalTime, flags,
                      t1, q1);
            iAnimBlend(bilerp[0], 1.0f, NULL, NULL, numBones, t0, q0, t1, q1, t0, q0);
            iAnimBlend(bilerp[1], 1.0f, NULL, NULL, numBones, tran, quat, t0, q0, tran, quat);
        }
        else if (bilerp[0])
        {
            iAnimEval(data->RawData[biindex[0] + biindex[1] * data->NumAnims[0]], evalTime, flags,
                      tran, quat);
            iAnimEval(data->RawData[biplus[0] + biindex[1] * data->NumAnims[0]], evalTime, flags,
                      t0, q0);
            iAnimBlend(bilerp[0], 1.0f, NULL, NULL, numBones, tran, quat, t0, q0, tran, quat);
        }
        else if (bilerp[1])
        {
            iAnimEval(data->RawData[biindex[0] + biindex[1] * data->NumAnims[0]], evalTime, flags,
                      tran, quat);
            iAnimEval(data->RawData[biindex[0] + biplus[1] * data->NumAnims[0]], evalTime, flags,
                      t0, q0);
            iAnimBlend(bilerp[0], 1.0f, NULL, NULL, numBones, tran, quat, t0, q0, tran, quat);
        }
        else
        {
            iAnimEval(data->RawData[biindex[0] + biindex[1] * data->NumAnims[0]], evalTime, flags,
                      tran, quat);
        }
    }
    else
    {
        iAnimEval(*data->RawData, evalTime, flags, tran, quat);
    }

    if (flags & 4)
    {
        float rootYaw;
        if (data->Physics != NULL)
        {
            iAnimEval(data->Physics, evalTime, 1, tran - 1, quat - 1);
            rootYaw = xAnimQuatYaw(quat - 1);
        }
        else
        {
            float* root = (float*)(tran - 1);
            for (i = 0; i < 3; i++)
            {
                if (fileFlags & (0x10 << i))
                {
                    root[i] = data->PhysBase[i];
                }
                else if (fileFlags & (0x100 << i))
                {
                    root[i] = t * data->PhysDelta[i] + data->PhysBase[i];
                }
                else
                {
                    root[i] = t * data->PhysDelta[i] + data->PhysBase[i] + root[(unsigned int)i + 3];
                }
            }

            if (fileFlags & 0x80)
            {
                rootYaw = data->PhysBase[3];
            }
            else if (fileFlags & 0x800)
            {
                rootYaw = t * data->PhysDelta[3] + data->PhysBase[3];
            }
            else
            {
                rootYaw = t * data->PhysBase[3] + data->PhysBase[3] + xAnimQuatYaw(quat);
            }

            (quat - 1)->v.x = 0.0f;
            (quat - 1)->v.y = isin(rootYaw);
            (quat - 1)->v.z = 0.0f;
            (quat - 1)->s = icos(rootYaw);
        }

        tran->x -= (tran - 1)->x;
        tran->y -= (tran - 1)->y;
        tran->z -= (tran - 1)->z;

        _xQuat inv;
        inv.v.x = -(quat - 1)->v.x;
        inv.v.y = -(quat - 1)->v.y;
        inv.v.z = -(quat - 1)->v.z;
        inv.s = (quat - 1)->s;
        xQuatMul(quat, quat, &inv);

        if (yaw != NULL)
        {
            *yaw = rootYaw;
        }
    }
}

xAnimEffect* xAnimStateNewEffect(xAnimState* state, unsigned int flags, float startTime,
                                 float endTime,
                                 unsigned int (*callback)(unsigned int, xAnimActiveEffect*,
                                                          xAnimSingle*, void*),
                                 unsigned int userDataSize)
{
    xAnimEffect* curr;
    xAnimEffect** prev;
    xAnimEffect* effect;

    effect = gxAnimUseGrowAlloc ? (xAnimEffect*)xMemGrowAlloc(gActiveHeap, userDataSize + sizeof(xAnimEffect)) :
                               (xAnimEffect*)xMemAlloc(gActiveHeap, userDataSize + sizeof(xAnimEffect));

    effect->Flags = flags;
    effect->StartTime = startTime;
    effect->EndTime = endTime;
    effect->Callback = callback;

    prev = &state->Effects;
    curr = state->Effects;
    while (curr != NULL && startTime > curr->StartTime)
    {
        prev = &curr->Next;
        curr = curr->Next;
    }

    effect->Next = curr;
    *prev = effect;
    return effect;
}

xAnimTable* xAnimTableNew(char* name, xAnimTable** linkedList)
{
    xAnimTable* table = (xAnimTable*)xMemAlloc(gActiveHeap, sizeof(xAnimTable));

    if (linkedList != NULL)
    {
        table->Next = *linkedList;
        *linkedList = table;
    }
    else
    {
        table->Next = NULL;
    }

    table->Name = name;
    table->TransitionList = NULL;
    table->StateList = NULL;
    table->AnimIndex = 0;
    table->MorphIndex = 0;
    return table;
}

xAnimState* xAnimTableNewState(xAnimTable* table, char* name, unsigned int flags,
                               unsigned int userFlags, float speed, float* boneBlend,
                               float* timeSnap, float fadeRecip, unsigned short* fadeOffset)
{
    xAnimState* state;

    state = gxAnimUseGrowAlloc ? (xAnimState*)xMemGrowAlloc(gActiveHeap, sizeof(xAnimState)) :
                               (xAnimState*)xMemAlloc(gActiveHeap, sizeof(xAnimState));

    if (table->StateList == NULL)
    {
        state->Next = NULL;
        table->StateList = state;
    }
    else
    {
        state->Next = table->StateList->Next;
        table->StateList->Next = state;
    }

    state->Name = name;
    state->ID = xStrHash(name);
    state->Flags = flags;
    state->UserFlags = userFlags;
    state->Speed = speed;
    state->Data = NULL;
    state->Effects = NULL;
    state->Default = NULL;
    state->List = NULL;
    state->BoneBlend = boneBlend;
    state->TimeSnap = timeSnap;
    state->FadeRecip = fadeOffset != NULL ? CalcRecipBlendMax(fadeOffset) : fadeRecip;
    state->FadeOffset = fadeOffset;
    return state;
}

void xAnimTableAddTransition(xAnimTable* table, xAnimTransition* tran, char* source)
{
    unsigned int flags = tran->Flags;
    unsigned int i;
    unsigned char* buffer = giAnimScratch;
    xAnimState** stateList = (xAnimState**)(giAnimScratch + 0x400);
    unsigned int allocCount;
    unsigned int stateCount;
    xAnimTransitionList* tlist;

    stateCount = 0;
    allocCount = 0;
    char* stateName = xStrTokBuffer(source, " ,\t\n\r", buffer);
    while (stateName != NULL)
    {
        xAnimState* state = xAnimTableGetState(table, stateName);
        if (state != NULL)
        {
            if (flags & 0x10)
            {
                if (!DefaultHasTransition(state, tran, &stateCount))
                {
                    stateList[allocCount] = state;
                    allocCount++;
                }
            }
            else if (!StateHasTransition(state, tran))
            {
                stateList[allocCount] = state;
                allocCount++;
                stateCount++;
            }
        }
        stateName = xStrTokBuffer(NULL, " ,\t\n\r", buffer);
    }

    if (stateCount)
    {
        tlist = gxAnimUseGrowAlloc ?
                    (xAnimTransitionList*)xMemGrowAlloc(gActiveHeap,
                                                        stateCount * sizeof(xAnimTransitionList)) :
                    (xAnimTransitionList*)xMemAlloc(gActiveHeap,
                                                    stateCount * sizeof(xAnimTransitionList));
    }

    if (flags & 0x10)
    {
        for (i = 0; i < allocCount; i++)
        {
            if (!DefaultOverride(stateList[i], tran))
            {
                if (tran->Conditional == NULL && stateList[i]->Default != NULL)
                {
                    xAnimTransitionList* last = stateList[i]->Default;
                    while (last->Next != NULL)
                    {
                        last = last->Next;
                    }
                    tlist->T = tran;
                    tlist->Next = NULL;
                    last->Next = tlist;
                    tlist++;
                }
                else
                {
                    tlist->T = tran;
                    tlist->Next = stateList[i]->Default;
                    stateList[i]->Default = tlist;
                    tlist++;
                }
            }
        }
    }
    else
    {
        for (i = 0; i < allocCount; i++)
        {
            tlist->T = tran;
            tlist->Next = stateList[i]->List;
            stateList[i]->List = tlist;
            tlist++;
        }
    }
}

xAnimTransition* xAnimTableNewTransition(xAnimTable* table, char* source, char* dest,
                                         xAnimTransitionConditionalCallback conditional,
                                         xAnimTransitionCallback callback, unsigned int flags,
                                         unsigned int userFlags, float srcTime, float destTime,
                                         unsigned short priority, unsigned short queuePriority,
                                         float blendRecip, unsigned short* blendOffset)
{
    xAnimTransition* tran;

    tran = gxAnimUseGrowAlloc ? (xAnimTransition*)xMemGrowAlloc(gActiveHeap, sizeof(xAnimTransition)) :
                               (xAnimTransition*)xMemAlloc(gActiveHeap, sizeof(xAnimTransition));

    if (blendRecip != 0.0f)
    {
        blendRecip = 1.0f / blendRecip;
    }

    tran->Next = table->TransitionList;
    table->TransitionList = tran;
    tran->Dest = xAnimTableGetState(table, dest);
    tran->Conditional = conditional;
    tran->Callback = callback;
    tran->Flags = flags;
    tran->UserFlags = userFlags;
    tran->SrcTime = srcTime;
    tran->DestTime = destTime;
    tran->Priority = priority;
    tran->QueuePriority = queuePriority;
    tran->BlendRecip = blendOffset != NULL ? CalcRecipBlendMax(blendOffset) : blendRecip;
    tran->BlendOffset = blendOffset;

    xAnimTableAddTransition(table, tran, source);
    return tran;
}

void xAnimTableAddFile(xAnimTable* table, xAnimFile* file, char* states)
{
    unsigned char* buffer = giAnimScratch;
    char* stateName = xStrTokBuffer(states, " ,\t\n\r", buffer);
    while (stateName != NULL)
    {
        xAnimTableAddFileID(table, file, xStrHash(stateName));
        stateName = xStrTokBuffer(NULL, " ,\t\n\r", buffer);
    }
}

#pragma auto_inline on
xAnimState* xAnimTableAddFileID(xAnimTable* table, xAnimFile* file, unsigned int stateID)
{
    xAnimState* state = xAnimTableGetStateID(table, stateID);
    if (state != NULL)
    {
        state->Data = file;
        if (file->FileFlags & 0x8000)
        {
            table->MorphIndex |= 1 << (state->Flags & 0xF);
        }
        else
        {
            table->AnimIndex |= 1 << (state->Flags & 0xF);
        }
    }
    return state;
}
#pragma auto_inline reset

#pragma auto_inline on
xAnimState* xAnimTableGetStateID(xAnimTable* table, unsigned int ID)
{
    xAnimState* curr = table->StateList;
    while (curr != NULL)
    {
        if (curr->ID == ID)
        {
            return curr;
        }
        curr = curr->Next;
    }
    return NULL;
}
#pragma auto_inline reset

void xAnimTableRemoveEffectByCB(xAnimTable* table,
                                unsigned int (*callback)(unsigned int, xAnimActiveEffect*,
                                                         xAnimSingle*, void*))
{
    xAnimEffect* prev;
    xAnimEffect* curr;
    xAnimState* state;

    for (state = table->StateList; state != NULL; state = state->Next)
    {
        prev = NULL;
        curr = state->Effects;
        while (curr != NULL)
        {
            if (curr->Callback == callback)
            {
                if (prev != NULL)
                {
                    prev->Next = curr->Next;
                }
                else
                {
                    prev = NULL;
                    state->Effects = curr->Next;
                }
                curr = curr->Next;
            }
            else
            {
                prev = curr;
                curr = curr->Next;
            }
        }
    }
}


#pragma auto_inline on
xAnimState* xAnimTableGetState(xAnimTable* table, char* name)
{
    return xAnimTableGetStateID(table, xStrHash(name));
}
#pragma auto_inline reset

#pragma auto_inline on
static void EffectSingleStart(xAnimSingle* single)
{
    xAnimEffect* effect = single->State->Effects;
    xAnimActiveEffect tempActive;
    float time = single->Time;
    unsigned int flags;

    while (effect != NULL && effect->StartTime < time)
    {
        flags = effect->Flags;

        if (flags & 1)
        {
            if (flags & 4)
            {
                tempActive.Effect = effect;
                tempActive.Handle = effect->Callback(1, &tempActive, single, single->Play->Object);

                flags = effect->Flags;

                if (flags & 2)
                {
                    if (effect->EndTime <= time)
                    {
                        effect->Callback(3, &tempActive, single, single->Play->Object);
                    }
                    else
                    {
                        EffectActiveInsert(single, &tempActive);
                    }
                }
                else if (flags & 0x20)
                {
                    EffectActiveInsert(single, &tempActive);
                }
            }
            else if ((flags & 2) && !(flags & 0x10) && effect->EndTime > time)
            {
                tempActive.Effect = effect;
                tempActive.Handle = effect->Callback(1, &tempActive, single, single->Play->Object);

                EffectActiveInsert(single, &tempActive);
            }
        }

        effect = effect->Next;
    }

    single->Effect = effect;
}
#pragma auto_inline reset

#pragma auto_inline on
static void EffectSingleRun(xAnimSingle* single)
{
    xAnimEffect* effect = single->Effect;
    xAnimActiveEffect tempActive;
    float time = single->Time;
    unsigned int flags;

    while (effect != NULL && effect->StartTime <= time)
    {
        flags = effect->Flags;

        if ((flags & 1) && (!(flags & 0x20) || !EffectPlaying(single, effect)))
        {
            tempActive.Effect = effect;
            tempActive.Handle = effect->Callback(1, &tempActive, single, single->Play->Object);

            if (flags & 2)
            {
                if (effect->EndTime <= time)
                {
                    effect->Callback(3, &tempActive, single, single->Play->Object);
                }
                else
                {
                    EffectActiveInsert(single, &tempActive);
                }
            }
            else if (flags & 0x20)
            {
                EffectActiveInsert(single, &tempActive);
            }
        }

        effect = effect->Next;
    }

    single->Effect = effect;
}
#pragma auto_inline reset

static void EffectSingleLoop(xAnimSingle* single)
{
    EffectSingleRun(single);

    xAnimActiveEffect* alist = single->ActiveList;
    unsigned int index = 0;
    unsigned int count = single->ActiveCount;
    while (index < count && alist->Effect != NULL)
    {
        if (!(alist->Effect->Flags & 0x20))
        {
            alist->Effect->Callback(3, alist, single, single->Play->Object);
            EffectActiveRemove(alist, index, count);
        }
        else
        {
            alist++;
            index++;
        }
    }

    xAnimEffect* effect = single->State->Effects;
    while (effect != NULL && effect->StartTime < 0.0f)
    {
        effect = effect->Next;
    }
    single->Effect = effect;
}

#pragma auto_inline on
static void CalcLastAndDeltas(xAnimSingle* single, _xVec3* tran, _xQuat* quat, float* yaw,
                              unsigned int lastOnly)
{
    float lyaw[2];
    _xQuat inv;
    _xVec3 ltran[2];
    _xQuat lquat[2];

    xAnimFileEval(single->State->Data, single->Time, single->BilinearLerp, 5, ltran, lquat, lyaw);

    if (!lastOnly)
    {
        tran->x = ltran[0].x - single->LastTranslation.x;
        tran->y = ltran[0].y - single->LastTranslation.y;
        tran->z = ltran[0].z - single->LastTranslation.z;

        inv.v.x = -single->LastQuat.v.x;
        inv.v.y = -single->LastQuat.v.y;
        inv.v.z = -single->LastQuat.v.z;
        inv.s = single->LastQuat.s;
        xQuatMul(quat, &inv, &lquat[0]);

        *yaw = lyaw[0] - single->LastYaw;
        while (*yaw > PI)
        {
            *yaw -= 2.0f * PI;
        }
        while (*yaw < -PI)
        {
            *yaw += 2.0f * PI;
        }

        if (!(single->SingleFlags & 0x8000))
        {
            tran[1] = ltran[1];
            quat[1] = lquat[1];
            yaw[1] = lyaw[1];
        }
    }

    single->LastTranslation = ltran[0];
    single->LastQuat = lquat[0];
    single->LastYaw = lyaw[0];
}
#pragma auto_inline reset

static void StopUpdate(xAnimSingle* single, _xVec3* tran, _xQuat* quat, float* yaw)
{
    float duration = single->State->Data->Duration;
    if (single->Time > duration)
    {
        single->Time = duration;
        single->CurrentSpeed = 0.0f;
    }
    CalcLastAndDeltas(single, tran, quat, yaw, 0);
}

static void LoopUpdate(xAnimSingle* single, _xVec3* tran, _xQuat* quat, float* yaw)
{
    float time = single->Time;
    xAnimFile* data = single->State->Data;
    float duration = data->Duration;

    if (time > duration)
    {
        _xVec3 ltran[2];
        _xQuat lquat[2];
        float lyaw[2];

        single->Time = duration;
        float remain = time - duration;
        CalcLastAndDeltas(single, ltran, lquat, lyaw, 0);
        EffectSingleLoop(single);
        single->Time = 0.0f;
        CalcLastAndDeltas(single, NULL, NULL, NULL, 1);
        single->Time = remain;
        CalcLastAndDeltas(single, tran, quat, yaw, 0);
        tran->x += ltran[0].x;
        tran->y += ltran[0].y;
        tran->z += ltran[0].z;
        xQuatMul(quat, quat, &lquat[0]);
        *yaw += lyaw[0];
    }
    else
    {
        CalcLastAndDeltas(single, tran, quat, yaw, 0);
    }
}


void xAnimPlaySetState(xAnimSingle* single, xAnimState* state, float startTime)
{
    EffectSingleStop(single);
    if (single->Blend != NULL)
    {
        EffectSingleStop(single->Blend);
        single->Blend->State = NULL;
    }

    single->State = state;
    if (state == NULL)
    {
        return;
    }

    single->Time = ((state->Flags & 0x100) && startTime == 0.0f) ?
                       state->Data->Duration * iurand() :
                       startTime;
    single->CurrentSpeed = state->Speed;
    single->BilinearLerp[0] = 0.0f;
    single->BilinearLerp[1] = 0.0f;
    single->Effect = NULL;
    memset(single->ActiveList, 0, single->ActiveCount * sizeof(xAnimActiveEffect));
    single->LastTime = -1.0f;
    memset(&single->LastTranslation, 0, sizeof(_xVec3));
    memset(&single->LastQuat, 0, sizeof(_xQuat));
    single->LastYaw = 0.0f;
    single->Sync = NULL;

    if (single->Tran != NULL && (single->Tran->Flags & 2))
    {
        xMemPoolFree(&sxAnimTempTranPool, single->Tran);
    }

    single->Tran = NULL;
    single->BlendFactor = 0.0f;
}

static void SingleUpdate(xAnimSingle* single, float timeDelta, _xVec3* tran, _xQuat* quat,
                         float* yaw, unsigned int)
{
    unsigned int foundBlendstop = 0;
    unsigned int firstStep = 1;
    xAnimTransition* foundTransition = NULL;
    xAnimSingle* bl = NULL;
    float tranDelta = 0.0f;
    float blendDelta = 0.0f;
    float singleTime;
    _xVec3 btran[2];
    _xVec3 ltran[2];
    _xQuat bquat[2];
    _xQuat lquat[2];
    float byaw[2];
    float lyaw[2];

    if (single->State == NULL)
    {
        return;
    }

    // Stand-in for code the original compiled out: it only makes MWCC's
    // auto-inliner see this function as large enough to stop inlining
    // CalcLastAndDeltas, EffectSingleStart and EffectSingleRun here.
    if (0)
    {
        EffectSingleStop(single);
        EffectSingleStop(single);
    }

    void* object = single->Play->Object;
    if (single->LastTime == -1.0f)
    {
        CalcLastAndDeltas(single, NULL, NULL, NULL, 1);
        EffectSingleStart(single);
    }

    single->LastTime = single->Time;
    singleTime = timeDelta * single->CurrentSpeed + single->Time;

    if (single->Blend != NULL && single->Blend->State != NULL)
    {
        bl = single->Blend;
        if (bl->LastTime == -1.0f)
        {
            CalcLastAndDeltas(bl, NULL, NULL, NULL, 1);
            EffectSingleStart(bl);
        }
        bl->LastTime = bl->Time;
    }

    float duration = single->State->Data->Duration;
    if (single->Sync != NULL)
    {
        float timeCmp;
        if ((timeCmp = single->Sync->SrcTime) != 0.0f)
        {
            if (timeCmp > duration)
            {
                timeCmp = duration;
            }
            if (single->LastTime <= timeCmp && singleTime >= timeCmp)
            {
                foundTransition = single->Sync;
                tranDelta = (singleTime - timeCmp) / single->CurrentSpeed;
                timeDelta = timeDelta - tranDelta;
                if (timeDelta < 0.0f)
                {
                    timeDelta = 0.0f;
                }
            }
            else
            {
                timeCmp += duration;
                if (single->LastTime <= timeCmp && singleTime >= timeCmp)
                {
                    foundTransition = single->Sync;
                    tranDelta = (singleTime - timeCmp) / single->CurrentSpeed;
                    timeDelta = timeDelta - tranDelta;
                    if (timeDelta < 0.0f)
                    {
                        timeDelta = 0.0f;
                    }
                }
            }
        }
        else if (bl == NULL)
        {
            foundTransition = single->Sync;
            tranDelta = timeDelta;
            timeDelta = 0.0f;
        }
    }
    else if ((single->State->Flags & 0x30) == 0x20)
    {
        float timeCmp = single->State->Default->T->SrcTime;

        if (timeCmp == 0.0f || timeCmp > duration)
        {
            timeCmp = duration;
        }

        if (singleTime >= timeCmp && (!(single->State->Default->T->Flags & 4) || bl == NULL))
        {
            xAnimTransitionList* curr = single->State->Default;
            while (curr != NULL && curr->T->Conditional != NULL &&
                   curr->T->Conditional(curr->T, single, object) == 0)
            {
                curr = curr->Next;
            }

            if (curr == NULL)
            {
                fprintf(PC_STDERR_OR(&__files[2]), "State \"%s\" no default conditionals true!\n",
                        single->State->Name);
                curr = single->State->Default;
            }

            foundTransition = curr->T;
            if (single->LastTime < timeCmp)
            {
                tranDelta = (singleTime - timeCmp) / single->CurrentSpeed;
                timeDelta = timeDelta - tranDelta;
                if (timeDelta < 0.0f)
                {
                    timeDelta = 0.0f;
                }
            }
            else
            {
                tranDelta = timeDelta;
                timeDelta = 0.0f;
            }
        }
    }

    if (single->BlendFactor != 0.0f)
    {
        float recip;
        if (single->Tran != NULL)
        {
            recip = single->Tran->BlendRecip;
        }
        else
        {
            recip = single->State->FadeRecip;
        }

        if (recip * (single->BlendFactor + timeDelta) > 1.0f)
        {
            foundBlendstop = 1;
            blendDelta = (single->BlendFactor + timeDelta) - (1.0f / recip);
            timeDelta = timeDelta - blendDelta;
            if (timeDelta < 0.0f)
            {
                timeDelta = 0.0f;
            }
            if (blendDelta < 0.0f)
            {
                blendDelta = 0.01f;
            }
        }
    }

    do
    {
        if (!firstStep)
        {
            if (foundBlendstop)
            {
                single->BlendFactor = 0.0f;
                if (bl != NULL)
                {
                    EffectSingleStop(bl);
                    bl->State = NULL;
                    bl = NULL;

                    if (single->Tran != NULL && (single->Tran->Flags & 2))
                    {
                        xMemPoolFree(&sxAnimTempTranPool, single->Tran);
                    }
                    single->Tran = NULL;
                }
                else if (single->Tran != NULL)
                {
                    if (single->Tran != NULL && (single->Tran->Flags & 2))
                    {
                        xMemPoolFree(&sxAnimTempTranPool, single->Tran);
                    }
                    single->Tran = NULL;
                }
                else
                {
                    EffectSingleStop(single);
                    single->State = NULL;
                    return;
                }
                timeDelta = blendDelta;
                foundBlendstop = 0;
            }
            else
            {
                if (bl != NULL)
                {
                    EffectSingleStop(bl);
                    bl->State = NULL;
                    bl = NULL;
                    single->BlendFactor = 0.0f;
                }

                if (foundTransition->BlendRecip == 0.0f || single->Blend == NULL)
                {
                    EffectSingleStop(single);
                    if (single->Tran != NULL && (single->Tran->Flags & 2))
                    {
                        xMemPoolFree(&sxAnimTempTranPool, single->Tran);
                    }
                    single->Tran = NULL;
                }
                else
                {
                    bl = single->Blend;
                    bl->State = single->State;
                    bl->Time = single->Time;
                    bl->CurrentSpeed = single->CurrentSpeed;
                    bl->BilinearLerp[0] = single->BilinearLerp[0];
                    bl->BilinearLerp[1] = single->BilinearLerp[1];
                    bl->Effect = single->Effect;
                    bl->LastTime = single->LastTime;
                    bl->LastTranslation = single->LastTranslation;
                    bl->LastQuat = single->LastQuat;
                    bl->LastYaw = single->LastYaw;
                    memcpy(bl->ActiveList, single->ActiveList,
                           single->ActiveCount * sizeof(xAnimActiveEffect));
                    single->ActiveList->Effect = NULL;

                    if (single->Tran != NULL && (single->Tran->Flags & 2))
                    {
                        xMemPoolFree(&sxAnimTempTranPool, single->Tran);
                    }
                    single->Tran = foundTransition;
                    single->BlendFactor = 1e-7f;
                }

                TransitionTimeInit(single, foundTransition);
                single->State = foundTransition->Dest;
                single->CurrentSpeed = single->State->Speed;
                single->BilinearLerp[0] = 0.0f;
                single->BilinearLerp[1] = 0.0f;
                single->Sync = NULL;
                CalcLastAndDeltas(single, NULL, NULL, NULL, 1);
                EffectSingleStart(single);

                if (foundTransition->Callback != NULL)
                {
                    foundTransition->Callback(foundTransition, single, single->Play->Object);
                }
                timeDelta = tranDelta;
                foundTransition = NULL;
            }
        }

        single->Time += timeDelta * single->CurrentSpeed;
        if (single->BlendFactor != 0.0f)
        {
            single->BlendFactor += timeDelta;
        }

        if ((single->State->Flags & 0x30) == 0x10)
        {
            LoopUpdate(single, ltran, lquat, lyaw);
        }
        else
        {
            StopUpdate(single, ltran, lquat, lyaw);
        }
        EffectSingleRun(single);

        if (bl != NULL)
        {
            if ((bl->State->Flags & 0x30) == 0x10)
            {
                LoopUpdate(bl, btran, bquat, byaw);
            }
            else
            {
                StopUpdate(bl, btran, bquat, byaw);
            }
            EffectSingleRun(bl);

            if ((float)__fabs(btran[0].x) > (float)__fabs(ltran[0].x))
            {
                ltran[0].x = btran[0].x;
            }
            if ((float)__fabs(btran[0].y) > (float)__fabs(ltran[0].y))
            {
                ltran[0].y = btran[0].y;
            }
            if ((float)__fabs(btran[0].z) > (float)__fabs(ltran[0].z))
            {
                ltran[0].z = btran[0].z;
            }
            if ((float)__fabs(byaw[0]) > (float)__fabs(lyaw[0]))
            {
                lyaw[0] = byaw[0];
            }
            if (bquat[0].v.x * bquat[0].v.x + bquat[0].v.y * bquat[0].v.y +
                    bquat[0].v.z * bquat[0].v.z + bquat[0].s * bquat[0].s >
                lquat[0].v.x * lquat[0].v.x + lquat[0].v.y * lquat[0].v.y +
                    lquat[0].v.z * lquat[0].v.z + lquat[0].s * lquat[0].s)
            {
                lquat[0] = bquat[0];
            }
        }

        if (firstStep)
        {
            firstStep = 0;
            tran[0] = ltran[0];
            quat[0] = lquat[0];
            *yaw = lyaw[0];
        }
        else
        {
            tran->x += ltran[0].x;
            tran->y += ltran[0].y;
            tran->z += ltran[0].z;
            xQuatMul(quat, quat, &lquat[0]);
            *yaw += lyaw[0];
        }
    } while (foundBlendstop || foundTransition != NULL);

    if (single->Tran == NULL && single->BlendFactor == 0.0f)
    {
        if ((single->State->Flags & 0x30) == 0x30)
        {
            if (single->State->Flags & 0x200)
            {
                if (single->Time >= duration)
                {
                    single->BlendFactor = 1e-7f;
                }
            }
            else if (single->State->FadeRecip * (duration - single->Time) < 1.0f)
            {
                single->BlendFactor = 1e-7f;
            }
        }
    }

    EffectSingleDuration(single);
    if (bl != NULL)
    {
        EffectSingleDuration(bl);
    }

    if (bl != NULL)
    {
        float lerp = single->BlendFactor * single->Tran->BlendRecip;
        if (lerp > 1.0f)
        {
            lerp = 1.0f;
        }
        tran[1].x = lerp * (ltran[1].x - btran[1].x) + btran[1].x;
        tran[1].y = lerp * (ltran[1].y - btran[1].y) + btran[1].y;
        tran[1].z = lerp * (ltran[1].z - btran[1].z) + btran[1].z;
        xQuatSlerp(&quat[1], &bquat[1], &lquat[1], lerp);
    }
    else
    {
        tran[1] = ltran[1];
        quat[1] = lquat[1];
    }
}

void xAnimPlaySetup(xAnimPlay* play, void* object, xAnimTable* table, xModelInstance* modelInst)
{
    play->BoneCount = modelInst->BoneCount;
    play->Object = object;
    play->Table = table;
    memset(play->Translate, 0, sizeof(play->Translate));
    memset(play->Quat, 0, sizeof(play->Quat));
    play->Yaw = 0.0f;
    play->ModelInst = modelInst;
    modelInst->Anim = play;
    modelInst->Flags |= 0x104;
    if (table->MorphIndex != 0)
    {
        modelInst->Flags |= 0x80;
    }

    for (signed int i = 0; i < play->NumSingle; i++)
    {
        play->Single[i].SingleFlags = ((1 << i) & table->MorphIndex) ? 0x8000 : 1;
        play->Single[i].State = NULL;
        play->Single[i].Tran = NULL;
        if (play->Single[i].Blend != NULL)
        {
            play->Single[i].Blend->State = NULL;
        }
    }

    xAnimPlaySetState(play->Single, table->StateList, 0.0f);
}

void xAnimPlayChooseTransition(xAnimPlay* play)
{
    unsigned int i;
    void* object = play->Object;
    xAnimTransition** list = (xAnimTransition**)giAnimScratch;
    xAnimTransition** found;
    xAnimTransitionList* curr;

    memset(list, 0, play->NumSingle * sizeof(xAnimTransition*));

    for (i = 0; i < play->NumSingle; i++)
    {
        if (play->Single[i].State != NULL)
        {
            curr = play->Single[i].State->List;
            while (curr != NULL)
            {
                if (curr->T->Conditional(curr->T, &play->Single[i], object))
                {
                    found = &list[curr->T->Dest->Flags & 0xF];
                    if (*found == NULL || curr->T->Priority > (*found)->Priority)
                    {
                        *found = curr->T;
                    }
                }
                curr = curr->Next;
            }
        }
    }

    for (i = 0; i < play->NumSingle; i++)
    {
        if (list[i] != NULL &&
            (play->Single[i].Sync == NULL ||
             list[i]->Priority > play->Single[i].Sync->QueuePriority))
        {
            xAnimPlayStartTransition(play, list[i]);
        }
    }
}

void xAnimPlayStartTransition(xAnimPlay* play, xAnimTransition* transition)
{
    xAnimSingle* single = &play->Single[transition->Dest->Flags & 0xF];
    xAnimSingle* bl = single->Blend;

    if (transition->SrcTime != 0.0f ||
        ((transition->Flags & 4) && bl != NULL && bl->State != NULL))
    {
        single->Sync = transition;
        return;
    }

    if (bl != NULL && bl->State != NULL)
    {
        EffectSingleStop(bl);
        bl->State = NULL;
        single->BlendFactor = 0.0f;
    }

    if (transition->BlendRecip == 0.0f || single->Blend == NULL)
    {
        EffectSingleStop(single);
        if (single->Tran != NULL && (single->Tran->Flags & 2))
        {
            xMemPoolFree(&sxAnimTempTranPool, single->Tran);
        }
        single->Tran = NULL;
    }
    else
    {
        if (single->State != NULL)
        {
            bl->State = single->State;
            bl->Time = single->Time;
            bl->CurrentSpeed = single->CurrentSpeed;
            bl->BilinearLerp[0] = single->BilinearLerp[0];
            bl->BilinearLerp[1] = single->BilinearLerp[1];
            bl->Effect = single->Effect;
            bl->LastTime = single->LastTime;
            bl->LastTranslation = single->LastTranslation;
            bl->LastQuat = single->LastQuat;
            bl->LastYaw = single->LastYaw;
            memcpy(bl->ActiveList, single->ActiveList,
                   single->ActiveCount * sizeof(xAnimActiveEffect));
            single->ActiveList->Effect = NULL;
        }

        if (single->Tran != NULL && (single->Tran->Flags & 2))
        {
            xMemPoolFree(&sxAnimTempTranPool, single->Tran);
        }
        single->Tran = transition;
        single->BlendFactor = 1e-7f;
    }

    TransitionTimeInit(single, transition);
    single->State = transition->Dest;
    single->CurrentSpeed = single->State->Speed;
    single->BilinearLerp[0] = 0.0f;
    single->BilinearLerp[1] = 0.0f;
    single->Sync = NULL;
    CalcLastAndDeltas(single, NULL, NULL, NULL, 1);
    EffectSingleStart(single);

    if (transition->Callback != NULL)
    {
        transition->Callback(transition, single, single->Play->Object);
    }
}

void xAnimPlayUpdate(xAnimPlay* play, float timeDelta)
{
    _xVec3 tran[2];
    _xQuat quat[2];
    float yaw[2];

    for (unsigned int i = 0; i < play->NumSingle; i++)
    {
        SingleUpdate(&play->Single[i], timeDelta, tran, quat, yaw, 1);
        if (i == 0)
        {
            play->Translate[0] = tran[0];
            play->Translate[1] = tran[1];
            play->Quat[0] = quat[0];
            play->Quat[1] = quat[1];
            play->Yaw = yaw[0];
        }
    }
}

void xAnimPlayEval(xAnimPlay* play)
{
    unsigned int i;
    unsigned int bone;
    _xQuat* quatresult = (_xQuat*)giAnimScratch;
    _xVec3* tranresult = (_xVec3*)((unsigned char*)quatresult + 0x410);

    if (play->BoneCount > 1)
    {
        _xQuat* quatblend = (_xQuat*)((unsigned char*)quatresult + 0x720);
        _xVec3* tranblend = (_xVec3*)((unsigned char*)quatblend + 0x410);

        SingleEval(play->Single, tranresult, (_xQuat*)giAnimScratch);

        for (i = 1; i < play->NumSingle; i++)
        {
            xAnimSingle* si = &play->Single[i];
            if (si->State == NULL || (si->SingleFlags & 0x8000))
            {
                continue;
            }

            float blendF = 1.0f;
            float blendR = 1.0f;
            unsigned short* blendO = NULL;
            SingleEval(si, tranblend, quatblend);

            if ((si->Blend == NULL || si->Blend->State == NULL) && si->BlendFactor)
            {
                if (si->Tran != NULL)
                {
                    blendF = si->BlendFactor;
                    blendR = si->Tran->BlendRecip;
                    blendO = si->Tran->BlendOffset;
                }
                else
                {
                    blendF = -si->BlendFactor;
                    blendR = si->State->FadeRecip;
                    blendO = si->State->FadeOffset;
                }
            }

            if ((si->SingleFlags & 3) == 2)
            {
                iAnimBlend(blendF, blendR, blendO, si->State->BoneBlend, play->BoneCount - 1,
                           tranblend + 1, quatblend + 1, NULL, NULL, tranblend + 1,
                           quatblend + 1);

                for (bone = 1; bone < play->BoneCount; bone++)
                {
                    tranresult[bone].x += tranblend[bone].x;
                    tranresult[bone].y += tranblend[bone].y;
                    tranresult[bone].z += tranblend[bone].z;
                    xQuatMul(&quatresult[bone], &quatresult[bone], &quatblend[bone]);
                }
            }
            else
            {
                iAnimBlend(blendF, blendR, blendO, si->State->BoneBlend, play->BoneCount - 1,
                           tranresult + 1, quatresult + 1, tranblend + 1, quatblend + 1,
                           tranresult + 1, quatresult + 1);
            }
        }
    }

    tranresult[0] = play->Translate[1];
    quatresult[0] = play->Quat[1];
    iModelAnimMatrices(play->ModelInst->Data, quatresult, tranresult, &play->ModelInst->Mat[1]);
}

void xAnimPoolCB(xMemPool* pool, void* data)
{
    signed int i;
    xAnimPlay* clone = (xAnimPlay*)data;
    xAnimPlay* play = (xAnimPlay*)pool->Buffer;
    xAnimSingle* clonesingle = (xAnimSingle*)(clone + 1);
    xAnimSingle* currsingle;
    xAnimActiveEffect* curract;

    clone->NumSingle = play->NumSingle;
    clone->Single = clonesingle;
    clonesingle += play->NumSingle;

    for (i = 0; i < play->NumSingle; i++)
    {
        clone->Single[i].Play = clone;
        if (play->Single[i].Blend != NULL)
        {
            clone->Single[i].Blend = clonesingle;
            clonesingle->Blend = NULL;
            clonesingle->Play = clone;
            clonesingle++;
        }
        else
        {
            clone->Single[i].Blend = NULL;
        }
    }

    curract = (xAnimActiveEffect*)clonesingle;

    for (i = 0; i < clone->NumSingle; i++)
    {
        clonesingle = &clone->Single[i];
        currsingle = &play->Single[i];

        while (clonesingle != NULL)
        {
            clonesingle->ActiveCount = currsingle->ActiveCount;
            if (currsingle->ActiveCount != 0)
            {
                clonesingle->ActiveList = curract;
                curract += currsingle->ActiveCount;
            }
            else
            {
                clonesingle->ActiveList = NULL;
            }
            currsingle = currsingle->Blend;
            clonesingle = clonesingle->Blend;
        }
    }

    clone->Pool = pool;
}

void xAnimPoolInit(xMemPool* pool, unsigned int count, unsigned int singles,
                   unsigned int blendFlags, unsigned int effectMax)
{
    effectMax += effectMax & 1;

    unsigned int size =
        (effectMax * sizeof(xAnimActiveEffect) + sizeof(xAnimSingle)) *
            (ADD_4_BITS((blendFlags & 0xFFFF) & ((signed int)(1 << singles) - 1 >> 0x0)) +
             ADD_4_BITS((blendFlags & 0xFFFF) & ((signed int)(1 << singles) - 1 >> 0x4)) +
             ADD_4_BITS((blendFlags & 0xFFFF) & ((signed int)(1 << singles) - 1 >> 0x8)) +
             ADD_4_BITS((blendFlags & 0xFFFF) & ((signed int)(1 << singles) - 1 >> 0xC)) +
             singles) +
        sizeof(xAnimPlay);

    unsigned int i;
    void* buffer = xMemAlloc(gActiveHeap, count * size);

    xAnimPlay* play = (xAnimPlay*)buffer;
    play->NumSingle = singles;

    xAnimSingle* currsingle;
    play->Single = currsingle = (xAnimSingle*)(play + 1);
    currsingle += singles;

    for (i = 0; i < singles; i++)
    {
        if (blendFlags & (1 << i))
        {
            play->Single[i].Blend = currsingle;
            currsingle->Blend = NULL;
            currsingle++;
        }
        else
        {
            play->Single[i].Blend = NULL;
        }
    }

    xAnimActiveEffect* curract = (xAnimActiveEffect*)currsingle;
    for (i = 0; i < play->NumSingle; i++)
    {
        currsingle = &play->Single[i];
        while (currsingle != NULL)
        {
            currsingle->ActiveCount = effectMax;
            if (effectMax != 0)
            {
                currsingle->ActiveList = curract;
                curract += effectMax;
            }
            else
            {
                currsingle->ActiveList = NULL;
            }
            currsingle = currsingle->Blend;
        }
    }

    play->Pool = pool;
    xMemPoolSetup(pool, buffer, 0, 1, xAnimPoolCB, size, count, count / 2);
}

xAnimPlay* xAnimPoolAlloc(xMemPool* pool, void* object, xAnimTable* table,
                          xModelInstance* modelInst)
{
    xAnimPlay* play = (xAnimPlay*)xMemPoolAlloc(pool);
    xAnimPlaySetup(play, object, table, modelInst);
    return play;
}

void xAnimPoolFree(xAnimPlay* play)
{
    for (unsigned int i = 0; i < play->NumSingle; i++)
    {
        EffectSingleStop(&play->Single[i]);
        if (play->Single[i].Blend != NULL)
        {
            EffectSingleStop(play->Single[i].Blend);
        }
    }

    xMemPoolFree(play->Pool, play);
}

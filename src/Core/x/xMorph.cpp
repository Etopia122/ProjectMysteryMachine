#include "xMath3.h"
#include "rwtypes.h"

extern "C" unsigned int strlen(const char* s);

class RpAtomic;

void iModelRender(RpAtomic* model, RwMatrixTag* mat);
void iMorphRender(RpAtomic* model, RwMatrixTag* mat, signed short** v_array, signed short* weight,
                  unsigned int normals, float scale);
void iMorphOptimize(RpAtomic* model, signed int normals);

class xMorphSeqFile
{
public:
    unsigned int Magic;
    unsigned int Flags;
    unsigned int TimeCount;
    unsigned int ModelCount;
};

class xMorphFrame
{
public:
    RpAtomic* Model;
    float RecipTime;
    float Scale;
    unsigned short Flags;
    unsigned short NumVerts;
    signed short* Targets[4];
    signed short WeightStart[4];
    signed short WeightEnd[4];
};

class xMorphTargetFile
{
public:
    unsigned int Magic;
    unsigned short NumTargets;
    unsigned short NumVerts;
    unsigned int Flags;
    float Scale;
    _xVec3 Center;
    float Radius;
};

// Functions are in reverse DOL order (-inline deferred emits last to first).

xMorphSeqFile* xMorphSeqSetup(void* data, void* (*FindAssetCB)(unsigned int, char*))
{
    signed int i;
    signed int j;
    RpAtomic* model;
    xMorphTargetFile* mphtgt;
    xMorphSeqFile* header;
    float* timelist;
    xMorphFrame* framelist;
    unsigned int* assetlist;
    char* namelist;
    void* assetPtr;
    unsigned int skipsize;

    header = (xMorphSeqFile*)data;
    timelist = (float*)(header + 1);
    framelist = (xMorphFrame*)(timelist + header->TimeCount);
    assetlist = (unsigned int*)(framelist + header->TimeCount);
    namelist = (char*)(assetlist + header->ModelCount * 2);

    if (header->Flags & 0x80000000)
    {
        return header;
    }

    header->Flags |= 0x80000000;
    if (!(header->Flags & 0x1))
    {
        namelist = 0;
    }

    for (i = 0; i < (signed int)header->ModelCount * 2; i++)
    {
        if (assetlist[i])
        {
            assetPtr = FindAssetCB(assetlist[i], namelist);
            assetlist[i] = (unsigned int)assetPtr;
        }
        if (namelist)
        {
            namelist = strlen(namelist) + 1 + namelist;
        }
    }

    for (i = 0; i < (signed int)header->ModelCount; i++)
    {
        if (assetlist[i * 2 + 1])
        {
            model = (RpAtomic*)assetlist[i * 2];
            mphtgt = (xMorphTargetFile*)assetlist[i * 2 + 1];
            iMorphOptimize(model, mphtgt->Flags & 0x1);
        }
    }

    for (i = 0; i < (signed int)header->TimeCount; i++)
    {
        model = (RpAtomic*)assetlist[(unsigned int)framelist[i].Model * 2];
        mphtgt = (xMorphTargetFile*)assetlist[(unsigned int)framelist[i].Model * 2 + 1];
        framelist[i].Model = model;
        for (j = 0; j < 4; j++)
        {
            if ((unsigned int)framelist[i].Targets[j] + 0x10000 == 0xFFFF)
            {
                framelist[i].Targets[j] = 0;
            }
            else
            {
                skipsize = (mphtgt->NumVerts * 3 + 7) & ~7;
                if (mphtgt->Flags & 0x1)
                {
                    skipsize *= 2;
                }
                if (j == 0)
                {
                    framelist[i].Flags = mphtgt->Flags;
                    framelist[i].NumVerts = mphtgt->NumVerts;
                    framelist[i].Scale *= mphtgt->Scale;
                }
                framelist[i].Targets[j] =
                    (signed short*)((unsigned char*)(mphtgt + 1) +
                                    skipsize * (unsigned int)framelist[i].Targets[j] * 2);
            }
        }
    }

    return header;
}

void xMorphRender(xMorphSeqFile* seq, RwMatrixTag* mat, float time)
{
    float lerp;
    unsigned int tidx;
    unsigned int tcount;
    signed short weight[4];
    signed short* wptr;
    float* times;
    xMorphFrame* frame;

    times = (float*)(seq + 1);
    tcount = seq->TimeCount;
    frame = (xMorphFrame*)(times + seq->TimeCount);

    if (time < 0.0f)
    {
        time = 0.0f;
    }
    if (time > times[tcount - 1])
    {
        time = times[tcount - 1];
    }

    tidx = (tcount - 1) & 0x3;
    while (times[tidx] < time)
    {
        tidx += 4;
    }
    while (tidx && time < times[tidx])
    {
        tidx--;
    }

    frame += tidx;

    if (!frame->Targets[0])
    {
        iModelRender(frame->Model, mat);
    }
    else
    {
        lerp = frame->RecipTime * (time - times[tidx]);
        if (lerp == 0.0f)
        {
            wptr = frame->WeightStart;
        }
        else
        {
            weight[0] = (signed int)(lerp * (frame->WeightEnd[0] - frame->WeightStart[0])) +
                        frame->WeightStart[0];
            weight[1] = (signed int)(lerp * (frame->WeightEnd[1] - frame->WeightStart[1])) +
                        frame->WeightStart[1];
            weight[2] = (signed int)(lerp * (frame->WeightEnd[2] - frame->WeightStart[2])) +
                        frame->WeightStart[2];
            weight[3] = (signed int)(lerp * (frame->WeightEnd[3] - frame->WeightStart[3])) +
                        frame->WeightStart[3];
            wptr = weight;
        }

        iMorphRender(frame->Model, mat, frame->Targets, wptr, frame->Flags & 0x1, frame->Scale);
    }
}

float xMorphSeqDuration(xMorphSeqFile* seq)
{
    return ((float*)(seq + 1))[seq->TimeCount - 1];
}

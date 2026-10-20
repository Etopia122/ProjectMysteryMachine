#define XBASE_ENABLE_OUTOFLINE
#include "zEnt.h"
#include "zGlobals.h"
#include "zScene.h"
#include "xAnim.h"
#include "xBound.h"
#include "xString.h"
#include "xstransvc.h"

extern "C" {
double atof(const char* s);
int atoi(const char* s);
int sprintf(char* s, const char* fmt, ...);
char* strcpy(char* dst, const char* src);
}

class xSerial;
class RpAtomic;

class zAssetModelParam
{
public:
    unsigned int HashID;      // 0x0
    unsigned char WordLength; // 0x4
    unsigned char String[3];  // 0x5
};

class xModelAssetInst
{
public:
    unsigned int ModelID;   // 0x0
    unsigned short Flags;   // 0x4
    unsigned char Parent;   // 0x6
    unsigned char Bone;     // 0x7
    float RotPos[12];       // 0x8
};

class xModelAssetInfo
{
public:
    unsigned int Magic;        // 0x0
    unsigned int NumModelInst; // 0x4
    unsigned int AnimTableID;  // 0x8
};

class xAnimState
{
public:
    xAnimState* Next; // 0x0
    char* Name;       // 0x4
};

class xAnimSingle
{
public:
    unsigned int SingleFlags; // 0x0
    xAnimState* State;        // 0x4
    float Time;               // 0x8
    float CurrentSpeed;       // 0xC
};

class xAnimPlay
{
public:
    xAnimPlay* Next;      // 0x0
    unsigned short NumSingle; // 0x4
    unsigned short BoneCount; // 0x6
    xAnimSingle* Single;  // 0x8
};

class zShadowParams
{
public:
    unsigned int type; // 0x0
    float at;          // 0x4
    float rad;         // 0x8
};

xAnimState* xAnimTableGetState(xAnimTable* table, char* name);
xAnimPlay* xAnimPoolAlloc(xMemPool* pool, void* object, xAnimTable* table,
                          xModelInstance* modelInst);
void xEntInit(_xEnt* ent, p2EntAsset* asset);
void xEntInitForType(_xEnt* ent);
void xEntSetup(_xEnt* ent);
void xEntSave(_xEnt* ent, xSerial* s);
void xEntLoad(_xEnt* ent, xSerial* s);
void xEntReset(_xEnt* ent);
void xEntLoadModel(_xEnt* ent, RpAtomic* imodel);
xModelInstance* xModelInstanceAlloc(RpAtomic* data, void* object, unsigned short flags,
                                    unsigned char boneIndex, unsigned char* boneRemap);
void xModelInstanceAttach(xModelInstance* inst, xModelInstance* parent);
RpAtomic* iModelFile_RWMultiAtomic(RpAtomic* model);
signed int xStrParseFloatList(float* dest, char* strbuf, signed int max);
signed int zAnimListGetNumUsed(unsigned int animID);
xAnimTable* zAnimListGetTable(unsigned int animID);
signed int irand();
float iurand();

static zShadowParams sShadowParams[27] = {
    { 3, 0.2f, 1.25f },  { 4, -0.2f, 1.4f },  { 7, -0.4f, 1.4f },  { 9, 0.4f, 1.25f },
    { 15, 0.0f, 1.5f },  { 17, 0.0f, 1.25f }, { 18, 0.0f, 1.5f },  { 19, 0.25f, 1.5f },
    { 20, 1.0f, 1.75f }, { 21, 0.5f, 1.25f }, { 22, 0.0f, 1.25f }, { 24, 0.0f, 1.25f },
    { 26, 0.0f, 1.75f }, { 46, 0.25f, 1.5f }, { 12, 0.0f, 1.5f },  { 28, 0.0f, 1.5f },
    { 29, 0.0f, 1.5f },  { 30, 0.0f, 1.5f },  { 31, 0.0f, 1.5f },  { 32, 0.0f, 1.5f },
    { 33, 0.0f, 1.5f },  { 34, 0.0f, 1.5f },  { 35, 0.0f, 1.5f },  { 36, 0.0f, 1.5f },
    { 37, 0.0f, 1.5f },  { 999, 0.0f, 1.5f }, { 999, 0.0f, 1.5f },
};

void zEntParseModelInfo(_xEnt* ent, unsigned int assetID);

#pragma auto_inline on
void xBaseDisable(xBase* xb)
{
    xb->baseFlags &= ~1;
}

void xBaseEnable(xBase* xb)
{
    xb->baseFlags |= 1;
}

unsigned int xBaseIsEnabled(xBase* xb)
{
    return xb->baseFlags & 1;
}
#pragma auto_inline off

void zEntInit(_zEnt* ent, p2EntAsset* asset, unsigned int type)
{
    xEntInit(ent, asset);

    if (type == 'PLYR')
    {
        ent->collType = 0x10;
        ent->collLev = 4;
        ent->bound.type = 1;
        zEntParseModelInfo(ent, asset->modelInfoID);
    }
    else if (type == 'NPC ')
    {
        ent->collType = 0x8;
        ent->collLev = 4;
        ent->bound.type = 1;
        zEntParseModelInfo(ent, asset->modelInfoID);
    }
    else if (type == 'ITEM')
    {
        ent->collType = 0x2;
        ent->collLev = 4;
        ent->bound.type = 1;
        ent->eventFunc = 0;
        zEntParseModelInfo(ent, asset->modelInfoID);
    }
    else if (type == 'PKUP')
    {
        ent->collType = 0;
        ent->bound.type = 0;
    }
    else if (type == 'PLAT')
    {
        ent->collType = 0x4;
        if (asset->moreFlags & 0x2)
        {
            ent->collLev = 5;
        }
        else
        {
            ent->collLev = 4;
        }
        zEntParseModelInfo(ent, asset->modelInfoID);
        ent->bound.type = 4;
        ent->bound.mat = (_xMat4x3*)ent->model->Mat;
    }
    else if (type == 'PEND')
    {
        ent->collType = 0x4;
        if (asset->moreFlags & 0x2)
        {
            ent->collLev = 5;
        }
        else
        {
            ent->collLev = 4;
        }
        zEntParseModelInfo(ent, asset->modelInfoID);
        ent->bound.type = 4;
        ent->bound.mat = (_xMat4x3*)ent->model->Mat;
    }
    else if (type == 'TRIG')
    {
        ent->collType = 0x1;
        ent->bound.type = 0;
    }
    else if (type == 'HANG')
    {
        ent->collType = 0x4;
        ent->collLev = 4;
        ent->bound.type = 1;
        zEntParseModelInfo(ent, asset->modelInfoID);
    }
    else if (type == 'SIMP')
    {
        ent->collType = 0x2;
        if (asset->moreFlags & 0x2)
        {
            ent->collLev = 5;
        }
        else
        {
            ent->collLev = 4;
        }
        ent->bound.type = 4;
        zEntParseModelInfo(ent, asset->modelInfoID);
        ent->bound.mat = (_xMat4x3*)ent->model->Mat;
    }
    else if (type == 'UI  ')
    {
        ent->collType = 0;
        ent->bound.type = 0;
        if (ent->baseType != 0x21 && asset->modelInfoID)
        {
            unsigned int size;
            RpAtomic* imodel = (RpAtomic*)xSTFindAsset(asset->modelInfoID, &size);
            xEntLoadModel(ent, imodel);
            if (ent->model)
            {
                ent->bound.mat = (_xMat4x3*)ent->model->Mat;
            }
        }
    }
    else if (type == 'BUTN')
    {
        ent->collType = 0x4;
        if (asset->moreFlags & 0x2)
        {
            ent->collLev = 5;
        }
        else
        {
            ent->collLev = 4;
        }
        ent->bound.type = 4;
        zEntParseModelInfo(ent, asset->modelInfoID);
        ent->bound.mat = (_xMat4x3*)ent->model->Mat;
    }
    else if (type == 'DSTR')
    {
        ent->collType = 0x4;
        if (asset->moreFlags & 0x2)
        {
            ent->collLev = 5;
        }
        else
        {
            ent->collLev = 4;
        }
        ent->bound.type = 4;
        zEntParseModelInfo(ent, asset->modelInfoID);
        ent->bound.mat = (_xMat4x3*)ent->model->Mat;
    }
    else if (type == 'EGEN')
    {
        ent->collType = 0x2;
        if (asset->moreFlags & 0x2)
        {
            ent->collLev = 5;
        }
        else
        {
            ent->collLev = 4;
        }
        ent->bound.type = 4;
        zEntParseModelInfo(ent, asset->modelInfoID);
        ent->bound.mat = (_xMat4x3*)ent->model->Mat;
    }

    if (asset->animListID)
    {
        if (zAnimListGetNumUsed(asset->animListID) > 0)
        {
            ent->atbl = zAnimListGetTable(asset->animListID);
            xAnimPoolAlloc(&globals.sceneCur->mempool, ent, ent->atbl, ent->model);

            xAnimState* ast = xAnimTableGetState(ent->atbl, "idle");
            if (ast)
            {
                xAnimSingle* single = ((xAnimPlay*)ent->model->Anim)->Single;
                single->State = ast;
                single->Time = 0.0f;
                single->CurrentSpeed = 1.0f;
            }
        }
        else
        {
            ent->atbl = 0;
        }
    }
    else
    {
        ent->atbl = 0;
    }

    xEntInitForType(ent);
}

void zEntSetup(_zEnt* ent)
{
    xEntSetup(ent);
}

void zEntSave(_zEnt* ent, xSerial* s)
{
    xEntSave(ent, s);
}

void zEntLoad(_zEnt* ent, xSerial* s)
{
    xEntLoad(ent, s);
}

void zEntReset(_zEnt* ent)
{
    xEntReset(ent);
}

void zEntEventAll(xBase* from, unsigned int fromEvent, unsigned int toEvent, float* toParam)
{
    zScene* sc = globals.sceneCur;
    for (unsigned short i = 0; i < sc->num_ents; i++)
    {
        zEntEvent(from, fromEvent, sc->base[i], toEvent, toParam, 0, 0);
    }
}

void zEntEventAllOfType(xBase* from, unsigned int fromEvent, unsigned int toEvent, float* toParam,
                        unsigned int type)
{
    zScene* sc = globals.sceneCur;
    if (sc)
    {
        for (unsigned short i = 0; i < sc->num_ents; i++)
        {
            xBase* base = sc->base[i];
            if (type == base->baseType)
            {
                zEntEvent(from, fromEvent, base, toEvent, toParam, 0, 0);
            }
        }
    }
}

void zEntEventAllOfType(unsigned int toEvent, unsigned int type)
{
    zEntEventAllOfType(0, 0, toEvent, 0, type);
}

void zEntEvent(char* to, unsigned int toEvent)
{
    xBase* base = zSceneFindObject(xStrHash(to));
    if (base)
    {
        zEntEvent(base, toEvent);
    }
}

void zEntEvent(xBase* to, unsigned int toEvent)
{
    zEntEvent(0, 0, to, toEvent, 0, 0, 0);
}

void zEntEvent(xBase* to, unsigned int toEvent, float* toParam)
{
    zEntEvent(0, 0, to, toEvent, toParam, 0, 0);
}

void zEntEvent(xBase* to, unsigned int toEvent, float* toParam, xBase* toParamWidget)
{
    zEntEvent(0, 0, to, toEvent, toParam, toParamWidget, 0);
}

void zEntEvent(xBase* from, xBase* to, unsigned int toEvent)
{
    zEntEvent(from, 0, to, toEvent, 0, 0, 0);
}

// TODO: 98.4%, the parameters land in different saved registers.
#pragma auto_inline on
void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to, unsigned int toEvent,
               float* toParam, xBase* toParamWidget, signed int forceEvent)
{
    xBase* dest;
    signed int i;
    p2LinkAsset* link;
    xBase* widget;
    float* param;

    if (toEvent == 2)
    {
        xBaseDisable(to);
    }
    else if (toEvent == 1)
    {
        xBaseEnable(to);
    }

    if (to->eventFunc && (xBaseIsEnabled(to) || forceEvent))
    {
        to->eventFunc(from, to, toEvent, toParam, toParamWidget);
    }

    if (xBaseIsEnabled(to) && to->linkCount)
    {
        link = to->link;
        for (i = 0; i < to->linkCount; i++, link++)
        {
            if (toEvent == link->srcEvent &&
                (!link->chkAssetID || (from && link->chkAssetID == from->id)))
            {
                dest = zSceneFindObject(link->dstAssetID);
                if (dest)
                {
                    widget =
                        link->paramWidgetAssetID ? zSceneFindObject(link->paramWidgetAssetID) : 0;
                    param = link->param;
                    zEntEvent(to, toEvent, dest, link->dstEvent, param, widget, 0);
                }
            }
        }
    }
}

xModelInstance* zEntRecurseModelInfo(void* info, _xEnt* ent)
{
    xModelAssetInfo* zinfo;
    unsigned int i;
    RpAtomic* imodel;
    unsigned int bufsize;
    xModelInstance* tempInst[64];
    xModelAssetInst* zinst;

    zinfo = (xModelAssetInfo*)info;
    zinst = (xModelAssetInst*)(zinfo + 1);
    for (i = 0; i < zinfo->NumModelInst; i++)
    {
        imodel = (RpAtomic*)xSTFindAsset(zinst[i].ModelID, &bufsize);
        if (*(unsigned int*)imodel == 'FNIM')
        {
            tempInst[i] = zEntRecurseModelInfo(imodel, ent);
            if (i != 0)
            {
                tempInst[i]->Flags |= zinst[i].Flags;
                tempInst[i]->BoneIndex = zinst[i].Bone;
                xModelInstanceAttach(tempInst[i], tempInst[zinst[i].Parent]);
            }
        }
        else if (i == 0)
        {
            tempInst[i] = xModelInstanceAlloc(imodel, ent, 0, 0, 0);
            if (ent->baseType != 3)
            {
                while ((imodel = iModelFile_RWMultiAtomic(imodel)) != 0)
                {
                    xModelInstanceAttach(xModelInstanceAlloc(imodel, ent, 0x8, 0, 0), tempInst[i]);
                }
            }
        }
        else
        {
            tempInst[i] = xModelInstanceAlloc(imodel, ent, zinst[i].Flags, zinst[i].Bone, 0);
            xModelInstanceAttach(tempInst[i], tempInst[zinst[i].Parent]);
        }
    }

    if (zinfo->AnimTableID)
    {
        xAnimTable* table = (xAnimTable*)xSTFindAsset(zinfo->AnimTableID, &bufsize);
        tempInst[0]->Anim =
            (xAnimPlay*)xAnimPoolAlloc(&globals.sceneCur->mempool, ent, table, tempInst[0]);
    }

    return tempInst[0];
}
#pragma auto_inline off

void zEntParseModelInfo(_xEnt* ent, unsigned int assetID)
{
    unsigned int bufsize;
    void* info = xSTFindAsset(assetID, &bufsize);
    if (*(unsigned int*)info == 'FNIM')
    {
        ent->model = zEntRecurseModelInfo(info, ent);
    }
    else
    {
        xEntLoadModel(ent, (RpAtomic*)info);
    }
}

void zEntAnimEvent(_zEnt* ent, unsigned int animEvent, float* animParam)
{
    xAnimSingle* single = ((xAnimPlay*)ent->model->Anim)->Single;

    switch (animEvent)
    {
    case 0xC3:
    case 0xC4:
    {
        if (!animParam)
        {
            break;
        }

        signed int anum = (signed int)*animParam - 1;
        if (anum < 0 || anum >= 10 || !ent->atbl)
        {
            break;
        }

        char name[12];
        if (animEvent == 0xC4)
        {
            sprintf(name, "loop%d", anum);
        }
        else
        {
            sprintf(name, "stop%d", anum);
        }

        xAnimState* ast = xAnimTableGetState(ent->atbl, name);
        if (ast)
        {
            single->State = ast;
            single->Time = 0.0f;
            single->CurrentSpeed = 1.0f;
        }
        break;
    }
    case 0xC5:
    {
        char name[12];
        strcpy(name, single->State->Name);
        name[0] = 's';
        name[1] = 't';
        single->State = xAnimTableGetState(ent->atbl, name);
        break;
    }
    case 0xC6:
        single->CurrentSpeed = 0.0f;
        break;
    case 0xC7:
        single->CurrentSpeed = 1.0f;
        break;
    case 0xC8:
        if (single->CurrentSpeed)
        {
            single->CurrentSpeed = 0.0f;
        }
        else
        {
            single->CurrentSpeed = 1.0f;
        }
        break;
    case 0xC9:
    {
        if (!animParam)
        {
            break;
        }

        signed int amin = (signed int)animParam[0] - 1;
        signed int amax = (signed int)animParam[1] - 1;
        if (amin < 0 || amin > amax || amax >= 10 || !ent->atbl)
        {
            break;
        }

        char name[12];
        sprintf(name, "stop%d", amin + irand() % (amax - amin + 1));

        xAnimState* ast = xAnimTableGetState(ent->atbl, name);
        if (ast)
        {
            single->State = ast;
            single->Time = 0.0f;
            single->CurrentSpeed = 1.0f;
        }
        break;
    }
    case 0xCA:
    {
        if (!animParam)
        {
            break;
        }

        signed int anum = (signed int)animParam[0] - 1;
        float prob = 0.01f * animParam[1];
        if (anum < 0 || anum >= 10)
        {
            break;
        }

        if (iurand() < prob && ent->atbl)
        {
            char name[12];
            sprintf(name, "stop%d", anum);

            xAnimState* ast = xAnimTableGetState(ent->atbl, name);
            if (ast)
            {
                single->State = ast;
                single->Time = 0.0f;
                single->CurrentSpeed = 1.0f;
            }
        }
        break;
    }
    }
}


zAssetModelParam* zEntGetModelParams(unsigned int assetID, unsigned int* size)
{
    unsigned int bufsize;
    xModelAssetInfo* minf = (xModelAssetInfo*)xSTFindAsset(assetID, &bufsize);

    if (minf->Magic == 'FNIM')
    {
        *size = bufsize - (minf->NumModelInst * sizeof(xModelAssetInst) + sizeof(xModelAssetInfo));
        if (*size)
        {
            return (zAssetModelParam*)((char*)minf + minf->NumModelInst * sizeof(xModelAssetInst) +
                                       sizeof(xModelAssetInfo));
        }
    }
    else
    {
        *size = 0;
    }

    return 0;
}

#pragma auto_inline on
char* zParamGetString(zAssetModelParam* param, unsigned int size, char* tok, char* def)
{
    unsigned int hash = xStrHash(tok);

    while (param && size)
    {
        if (param->HashID == hash)
        {
            return (char*)param->String;
        }

        size -= param->WordLength * 4 + 8;
        param = (zAssetModelParam*)((char*)param + (param->WordLength * 4 + 8));
    }
    return def;
}
#pragma auto_inline off

signed int zParamGetInt(zAssetModelParam* param, unsigned int size, char* tok, signed int def)
{
    char* str = zParamGetString(param, size, tok, 0);
    if (str)
    {
        return atoi(str);
    }
    return def;
}

float zParamGetFloat(zAssetModelParam* param, unsigned int size, char* tok, float def)
{
    char* str = zParamGetString(param, size, tok, 0);
    if (str)
    {
        return atof(str);
    }
    return def;
}

signed int zParamGetVector(zAssetModelParam* param, unsigned int size, char* tok, _xVec3 def,
                           _xVec3* result)
{
    _xVec3 vec = def;
    char* str = zParamGetString(param, size, tok, 0);
    float fl[3] = { 0.0f, 0.0f, 0.0f };
    signed int list = 0;

    xVec3Copy(result, &vec);
    if (str)
    {
        list = xStrParseFloatList(fl, str, 3);
        if (list > 0)
        {
            result->x = fl[0];
        }
        if (list > 1)
        {
            result->y = fl[1];
        }
        if (list > 2)
        {
            result->z = fl[2];
        }
    }
    return list;
}

// TODO: 96.6%, the original builds the radius in f0 and copies it to f7.
void zEntGetShadowParams(_xEnt* ent, _xVec3* center, float* radius)
{
    float r;
    RwMatrixTag* mat;
    float rad;
    float cx;
    float cy;
    float cz;
    zShadowParams* sp;
    unsigned int i;
    float s;

    r = ent->bound.sph.r;
    mat = ent->model->Mat;
    rad = (ent->bound.sph.r < 0.01f) ? 2.0f : 2.4f * ent->bound.sph.r;
    cx = ent->bound.sph.center.x;
    cy = ent->bound.sph.center.y;
    cz = ent->bound.sph.center.z;

    if (ent->baseType == 2)
    {
        sp = sShadowParams;
        for (i = 0; i < 27; i++)
        {
            if (ent->subType == sp->type)
            {
                rad *= sp->rad;
                s = sp->at;
                cx += mat->at.x * s;
                cy += mat->at.y * s;
                cz += mat->at.z * s;
                break;
            }
            sp++;
        }
    }

    *radius = rad;
    center->x = cx;
    center->y = cy;
    center->z = cz;
}

#include "zEnt.h"
#include "zGlobals.h"
#include "zScene.h"
#include "xAnim.h"
#include "xstransvc.h"

extern "C" void* memset(void* dst, int c, unsigned int n);

class _tagLightning;
class _xMovePoint;

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
    xAnimPlay* Next;          // 0x0
    unsigned short NumSingle; // 0x4
    unsigned short BoneCount; // 0x6
    xAnimSingle* Single;      // 0x8
};

class zEGenAsset : public p2EntAsset
{
public:
    _xVec3 src_dpos;           // 0x50
    unsigned char damage_type; // 0x5C
    unsigned char flags;       // 0x5D
    float ontime;              // 0x60
    unsigned int onAnimID;     // 0x64
};

class zEGenerator : public _zEnt
{
public:
    unsigned short flags;    // 0xEC
    unsigned short num_dsts; // 0xEE
    float tmr;               // 0xF0
    xAnimFile* afile;        // 0xF4
    _xVec3 src_pos;          // 0xF8
    _xVec3 dst_pos;          // 0x104
    xBase* dst;              // 0x110
    _tagLightning* lfx[2];   // 0x114
};

enum _tagLightningType
{
    eLightningLine,
    eLightningRotating,
    eLightningForce32 = 0x7FFFFFFF
};

class _tagLightningAdd
{
public:
    _tagLightningType type;    // 0x0
    float setup_degrees;       // 0x4
    float move_degrees;        // 0x8
    float rot_radius;          // 0xC
    _xVec3* start;             // 0x10
    _xVec3* end;               // 0x14
    signed int total_points;   // 0x18
    float time;                // 0x1C
    float arc_height;          // 0x20
    float thickness;           // 0x24
    unsigned char r;           // 0x28
    unsigned char g;           // 0x29
    unsigned char b;           // 0x2A
    unsigned char alpha_pad;   // 0x2B
    float rand_radius;         // 0x2C
    unsigned int flags;        // 0x30
};

class xMemPool;

void xEntUpdate(_xEnt* ent, _xScene* sc, float dt);
void xEntRender(_xEnt* ent);
_tagLightning* zLightningAdd(_tagLightningAdd* add);
void zLightningKill(_tagLightning* l);
void zLightningModifyEndpoints(_tagLightning* l, _xVec3* start, _xVec3* end);
_xVec3* xMovePointGetPos(_xMovePoint* m);
unsigned int irand();
float iurand();
xAnimFile* xAnimFileNew(void* rawData, char* name, unsigned int flags, xAnimFile** linkedList);
xAnimTable* xAnimTableNew(char* name, xAnimTable** linkedList);
void zEntSetup(_zEnt* ent);

xAnimPlay* xAnimPoolAlloc(xMemPool* pool, void* object, xAnimTable* table,
                          xModelInstance* modelInst);

#define SCENE_ID(s) ((((s)[0] & ~0x20) << 24) | ((s)[1] << 16) | ((s)[2] << 8) | (s)[3])

void zEGenerator_Update(zEGenerator* egen, _xScene* sc, float dt);
void zEGenerator_Render(zEGenerator* egen);
void zEGenerator_TurnOn(zEGenerator* egen);
void zEGenerator_Reset(zEGenerator* egen, _xScene* sc);
signed int zEGeneratorEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                              xBase* toParamWidget);

inline void zEGenerator_TurnOff(zEGenerator* egen)
{
    egen->flags &= 0xFFFE;
    if (egen->afile)
    {
        egen->model->Anim->Single->CurrentSpeed = 0.0f;
    }
    for (signed int i = 0; i < 2; i++)
    {
        if (egen->lfx[i])
        {
            zLightningKill(egen->lfx[i]);
            egen->lfx[i] = 0;
        }
    }
}

inline void zEGenerator_ToggleOn(zEGenerator* egen)
{
    if (egen->flags & 1)
    {
        zEGenerator_TurnOff(egen);
    }
    else
    {
        zEGenerator_TurnOn(egen);
    }
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zEGenerator_Init(zEGenerator* egen, p2EntAsset* asset)
{
    unsigned int size;
    zEGenAsset* zasset = (zEGenAsset*)asset;

    zEntInit(egen, asset, 'EGEN');
    if (egen->linkCount)
    {
        egen->link = (p2LinkAsset*)(zasset + 1);
    }
    else
    {
        egen->link = 0;
    }
    egen->update = (void (*)(_xEnt*, _xScene*, float))zEGenerator_Update;
    egen->move = 0;
    egen->eventFunc = zEGeneratorEventCB;
    egen->render = (void (*)(_xEnt*))zEGenerator_Render;
    egen->afile = 0;

    if (zasset->onAnimID)
    {
        void* buf = xSTFindAsset(zasset->onAnimID, &size);
        if (buf)
        {
            egen->afile = xAnimFileNew(buf, "", 0, 0);
            egen->atbl = xAnimTableNew("", 0);
            xAnimTableNewState(egen->atbl, "On", 0x10, 0, 1.0f, 0, 0, 0.0f, 0);
            xAnimTableAddFile(egen->atbl, egen->afile, "On");
            xAnimPoolAlloc(&globals.sceneCur->mempool, egen, egen->atbl, egen->model);
        }
    }
}

void zEGenerator_Setup(zEGenerator* egen, _xScene*)
{
    zEntSetup(egen);
    egen->num_dsts = 0;
    for (signed int i = 0; i < egen->linkCount; i++)
    {
        p2LinkAsset* la = &egen->link[i];
        xBase* b = zSceneFindObject(la->dstAssetID);
        if (b && la->dstEvent == 0xC0)
        {
            if (b->baseType == 0xD)
            {
                egen->num_dsts++;
            }
            else if (b->baseType == 6)
            {
                egen->num_dsts++;
            }
        }
    }
}

void zEGenerator_Save(zEGenerator* ent, xSerial* s)
{
    zEntSave(ent, s);
}

void zEGenerator_Load(zEGenerator* ent, xSerial* s)
{
    zEntLoad(ent, s);
}

#pragma auto_inline on
void zEGenerator_Reset(zEGenerator* egen, _xScene*)
{
    zEntReset(egen);
    zEGenAsset* zasset = (zEGenAsset*)egen->asset;
    egen->flags = zasset->flags;
    if (egen->flags & 1)
    {
        zEGenerator_TurnOn(egen);
    }
    else
    {
        zEGenerator_TurnOff(egen);
    }
    if (zasset->moreFlags & 4)
    {
        egen->chkby = 0;
    }
    else
    {
        egen->chkby = 0x18;
    }
}
#pragma auto_inline off

void zEGenerator_Update(zEGenerator* egen, _xScene* sc, float dt)
{
    xEntUpdate(egen, sc, dt);

    _xEnt* ent = (_xEnt*)egen->dst;
    if (ent && ent->baseType == 6)
    {
        xVec3Copy(&egen->dst_pos, &((_xMat4x3*)ent->model->Mat)->pos);
        if (egen->lfx[0])
        {
            zLightningModifyEndpoints(egen->lfx[0], &egen->src_pos, &egen->dst_pos);
        }
        if (egen->lfx[1])
        {
            zLightningModifyEndpoints(egen->lfx[1], &egen->src_pos, &egen->dst_pos);
        }
    }

    if (egen->tmr > 0.0f)
    {
        egen->tmr -= dt;
        if (egen->tmr <= 0.0f)
        {
            zEGenerator_TurnOff(egen);
            zEntEvent(egen, 0x27);
        }
    }
}

void zEGenerator_Render(zEGenerator* egen)
{
    xEntRender(egen);
}

void zEGenerator_TurnOn(zEGenerator* egen)
{
    _tagLightningAdd add;

    egen->flags |= 1;
    zEGenAsset* zasset = (zEGenAsset*)egen->asset;
    if (egen->afile)
    {
        egen->model->Anim->Single->CurrentSpeed = 1.0f;
    }
    egen->tmr = zasset->ontime;
    xVec3Add(&egen->src_pos, &((_xMat4x3*)egen->model->Mat)->pos, &zasset->src_dpos);

    if (egen->num_dsts)
    {
        unsigned short itgt = irand() % egen->num_dsts;
        unsigned short imp = 0;
        for (signed int i = 0; i < egen->linkCount; i++)
        {
            p2LinkAsset* link = &egen->link[i];
            xBase* b = zSceneFindObject(link->dstAssetID);
            if (b && link->dstEvent == 0xC0)
            {
                if (imp == itgt)
                {
                    if (b->baseType == 0xD)
                    {
                        xVec3Copy(&egen->dst_pos, xMovePointGetPos((_xMovePoint*)b));
                        egen->dst = b;
                        break;
                    }
                    else if (b->baseType == 6)
                    {
                        xVec3Copy(&egen->dst_pos, &((_xMat4x3*)((_xEnt*)b)->model->Mat)->pos);
                        egen->dst = b;
                        break;
                    }
                }
                else
                {
                    imp++;
                }
            }
        }
    }

    for (signed int i = 0; i < 2; i++)
    {
        if (egen->lfx[i])
        {
            zLightningKill(egen->lfx[i]);
            egen->lfx[i] = 0;
        }
    }

    memset(&add, 0, sizeof(_tagLightningAdd));
    add.type = eLightningLine;
    add.total_points = 16;
    add.start = &egen->src_pos;
    add.end = &egen->dst_pos;
    add.thickness = 0.15f;
    add.r = 0xFF;
    add.g = 0xFF;
    add.b = 0xFF;
    add.rand_radius = 5.0f;
    add.flags = 0x10;
    if (globals.sceneCur->sceneID == SCENE_ID("G009"))
    {
        add.thickness = 1.0f;
        add.r = 0x55;
        add.g = 0xF3;
        add.b = 0x55;
        add.rand_radius = 10.0f;
    }
    egen->lfx[0] = zLightningAdd(&add);

    add.type = eLightningRotating;
    add.setup_degrees = 90.0f * iurand() + 20.0f;
    add.move_degrees = 360.0f * iurand() + 180.0f;
    if (irand() & 1)
    {
        add.move_degrees = -add.move_degrees;
    }
    add.rot_radius = 0.5f;
    add.total_points = 16;
    add.thickness = 0.1f;
    add.r = 0x50;
    add.g = 0x50;
    add.b = 0xFF;
    add.rand_radius = 3.0f;
    add.flags = 0x18;
    if (globals.sceneCur->sceneID == SCENE_ID("G009"))
    {
        add.thickness = 0.5f;
        add.r = 0x14;
        add.g = 0xF3;
        add.b = 0x14;
        add.rot_radius = 1.5f;
    }
    egen->lfx[1] = zLightningAdd(&add);
}

signed int zEGeneratorEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                              xBase* toParamWidget)
{
    zEGenerator* egen = (zEGenerator*)to;

    switch (toEvent)
    {
    case 0x26:
        zEGenerator_TurnOn(egen);
        break;
    case 0x27:
        zEGenerator_TurnOff(egen);
        break;
    case 0xF:
        zEGenerator_ToggleOn(egen);
        break;
    case 0xA:
        zEGenerator_Reset(egen, globals.sceneCur);
        break;
    case 3:
        egen->_xEnt::flags |= 1;
        break;
    case 4:
        egen->_xEnt::flags &= 0xFE;
        break;
    case 0x55:
        egen->_xEnt::flags |= 1;
    case 0x53:
        egen->chkby = 0x18;
        break;
    case 0x56:
        egen->_xEnt::flags &= 0xFE;
    case 0x54:
        egen->chkby = 0;
        break;
    }
    return 1;
}

#include "xBase.h"
#include "xMemMgr.h"
#include "xParGroup.h"
#include "zScene.h"

class zParSysAsset : public p2BaseAsset
{
public:
    unsigned int type;                // 0x8
    unsigned int parentParSysID;      // 0xC
    unsigned int textureID;           // 0x10
    unsigned char parFlags;           // 0x14
    unsigned char priority;           // 0x15
    unsigned short maxPar;            // 0x16
    unsigned char renderFunc;         // 0x18
    unsigned char renderSrcBlendMode; // 0x19
    unsigned char renderDstBlendMode; // 0x1A
    unsigned char cmdCount;           // 0x1B
    unsigned int cmdSize;             // 0x1C
};

class xParCmdAsset
{
public:
    unsigned int type;     // 0x0
    unsigned char enabled; // 0x4
    unsigned char mode;    // 0x5
    unsigned char pad[2];  // 0x6
};

class xParCmdTex : public xParCmdAsset
{
public:
    float x1;                  // 0x8
    float y1;                  // 0xC
    float x2;                  // 0x10
    float y2;                  // 0x14
    unsigned char birthMode;   // 0x18
    unsigned char rows;        // 0x19
    unsigned char cols;        // 0x1A
    unsigned char unit_count;  // 0x1B
    float unit_width;          // 0x1C
    float unit_height;         // 0x20
};

class xParCmd
{
public:
    unsigned int flag; // 0x0
};

class zParCmd : public xParCmd
{
public:
    xParCmdAsset* tasset; // 0x4
};

class _zParGroup : public xParGroup
{
};

class zParSys : public xBase
{
public:
    zParSysAsset* tasset;  // 0x10
    unsigned int cmdCount; // 0x14
    zParCmd* cmd;          // 0x18
    zParSys* parent;       // 0x1C
    _zParGroup* group;     // 0x20
    unsigned char visible; // 0x24
};

class zParSysInfo
{
public:
    signed int type;                   // 0x0
    void (*func)(void*, xParGroup*);   // 0x4
};

enum _SDRenderState
{
    SDRS_Unknown = 0,
    SDRS_Particles = 8,
    SDRS_Force32 = 0x7FFFFFFF
};

void zRenderState(_SDRenderState next);
void zRenderStateSetSrcBlendMode(signed int mode);
void zRenderStateSetDstBlendMode(signed int mode);
void zRenderStateSetTexture(unsigned int textureID);
void (*zParCmdGetUpdateFunc(unsigned int parType))(zParCmd*, xParGroup*, float);
unsigned int zParCmdGetSize(unsigned int parType);

void iParMgrRenderParSys_Sprite(void* data, xParGroup* ps);
void iParMgrRenderParSys_Streak(void* data, xParGroup* ps);
void iParMgrRenderParSys_Flat(void* data, xParGroup* ps);
void iParMgrRenderParSys_Static(void* data, xParGroup* ps);
void iParMgrRenderParSys_Ground(void* data, xParGroup* ps);
void iParMgrRenderParSys_QuadStreak(void* data, xParGroup* ps);
void iParMgrRenderParSys_InvStreak(void* data, xParGroup* ps);

signed int zParSysEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                          xBase* toParamWidget);

static zParSysInfo sParSysInfo[7] = {
    { 0, iParMgrRenderParSys_Sprite },     { 1, iParMgrRenderParSys_Streak },
    { 2, iParMgrRenderParSys_Flat },       { 3, iParMgrRenderParSys_Static },
    { 4, iParMgrRenderParSys_Ground },     { 5, iParMgrRenderParSys_QuadStreak },
    { 6, iParMgrRenderParSys_InvStreak },
};

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zParSysInit(xBase* b, zParSysAsset* tasset);

void zParSysInit(void* b, void* tasset)
{
    zParSysInit((xBase*)b, (zParSysAsset*)tasset);
}

void zParSysInit(xBase* b, zParSysAsset* tasset)
{
    zParSys* s = (zParSys*)b;
    xParCmdAsset* cmdAsset;
    signed int i;

    xBaseInit(s, tasset);
    s->eventFunc = zParSysEventCB;
    s->tasset = tasset;
    if (s->linkCount)
    {
        s->link = (p2LinkAsset*)((unsigned char*)(s->tasset + 1) + tasset->cmdSize);
    }
    else
    {
        s->link = 0;
    }

    s->visible = tasset->parFlags & 1;
    s->cmdCount = tasset->cmdCount;
    if (s->cmdCount)
    {
        s->cmd = (zParCmd*)xMemAlloc(gActiveHeap, s->cmdCount * sizeof(zParCmd));
    }
    else
    {
        s->cmd = 0;
    }

    cmdAsset = (xParCmdAsset*)(tasset + 1);
    for (i = 0; i < (signed int)s->cmdCount; i++)
    {
        s->cmd[i].flag = 1;
        s->cmd[i].tasset = cmdAsset;
        cmdAsset = (xParCmdAsset*)((unsigned char*)cmdAsset + zParCmdGetSize(cmdAsset->type));
    }

    s->group = (_zParGroup*)xMemAlloc(gActiveHeap, sizeof(_zParGroup));
    xParGroupInit(s->group);
    xParGroupSetPriority(s->group, tasset->priority);
    xParGroupRegister(s->group);
    xParGroupSetAging(s->group, (unsigned char)(((tasset->parFlags >> 1) & 1) ^ 1));
    xParGroupSetVisibility(s->group, s->visible);
    xParGroupSetBack2Life(s->group, (unsigned char)(((tasset->parFlags >> 2) & 1) ^ 1));
    s->parent = 0;

    for (i = 0; i < (signed int)s->cmdCount; i++)
    {
        if (s->cmd[i].tasset->type == 12)
        {
            s->group->m_cmdTex = (xParCmdTex*)s->cmd[i].tasset;
            xParCmdTex* tex = s->group->m_cmdTex;
            tex->unit_count = tex->rows * tex->cols;
            tex->unit_width = (tex->x2 - tex->x1) / tex->cols;
            tex->unit_height = (tex->y2 - tex->y1) / tex->rows;
            break;
        }
    }

    s->group->draw = sParSysInfo[s->tasset->renderFunc].func;
}

void zParSysSetup(zParSys* t)
{
    if (t && t->tasset && t->tasset->parentParSysID)
    {
        t->parent = (zParSys*)zSceneFindObject(t->tasset->parentParSysID);
    }
}

void zParSysExit(zParSys* t)
{
    if (t->group)
    {
        xParGroupKillAllParticles(t->group);
        xParGroupUnregister(t->group);
    }
}

signed int zParSysEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                          xBase* toParamWidget)
{
    zParSys* t = (zParSys*)to;

    switch (toEvent)
    {
    case 10:
        xBaseReset(t, t->tasset);
        if (t->group)
        {
            xParGroupKillAllParticles(t->group);
            t->visible = t->tasset->parFlags & 1;
            xParGroupSetAging(t->group, (unsigned char)(((t->tasset->parFlags >> 1) & 1) ^ 1));
            xParGroupSetVisibility(t->group, t->visible);
            xParGroupSetBack2Life(t->group, (unsigned char)(((t->tasset->parFlags >> 2) & 1) ^ 1));
        }
        break;
    case 3:
        t->visible = 1;
        if (t->group)
        {
            xParGroupSetVisibility(t->group, t->visible);
        }
        break;
    case 4:
        t->visible = 0;
        if (t->group)
        {
            xParGroupSetVisibility(t->group, t->visible);
        }
        break;
    case 0x26:
        if (t->group)
        {
            xParGroupSetActive(t->group, 1);
        }
        break;
    case 0x27:
        if (t->group)
        {
            xParGroupSetActive(t->group, 0);
        }
        break;
    }
    return 1;
}

static inline void zParSysCmdUpdate(zParSys* s, _zParGroup* g, float dt)
{
    for (signed int i = 0; i < (signed int)s->cmdCount; i++)
    {
        zParCmd* cmd = &s->cmd[i];
        if (cmd && cmd->tasset)
        {
            void (*update)(zParCmd*, xParGroup*, float) = zParCmdGetUpdateFunc(cmd->tasset->type);
            if (update)
            {
                update(cmd, g, dt);
            }
        }
    }
}

static void zParGroupUpdateR(zParSys* s, _zParGroup* g, float dt);



void zParSysUpdate(xBase* to, float dt)
{
    zParSys* s = (zParSys*)to;
    zParSys* parent = s->parent;

    for (_zParGroup* g = s->group; g; g = (_zParGroup*)g->m_next)
    {
        if (g->m_active)
        {
            if (parent)
            {
                zParGroupUpdateR(parent, g, dt);
            }
            if (g->m_alive)
            {
                zParSysCmdUpdate(s, g, dt);
            }
        }
        xParGroupAnimate(g, dt);
    }
}

#pragma auto_inline on
static void zParGroupUpdateR(zParSys* s, _zParGroup* g, float dt)
{
    if (s->parent)
    {
        zParGroupUpdateR(s->parent, g, dt);
    }
    if (s->group->m_active)
    {
        for (signed int i = 0; i < (signed int)s->cmdCount; i++)
        {
            zParCmd* cmd = &s->cmd[i];
            if (cmd && cmd->tasset)
            {
                void (*update)(zParCmd*, xParGroup*, float) =
                    zParCmdGetUpdateFunc(cmd->tasset->type);
                if (update)
                {
                    update(cmd, g, dt);
                }
            }
        }
    }
}
#pragma auto_inline off

void zParSysRender(xBase* b)
{
    _zParGroup* g;
    zParSys* s = (zParSys*)b;

    zRenderState(SDRS_Particles);
    for (g = s->group; g; g = (_zParGroup*)g->m_next)
    {
        if (g->m_active && g->m_visible && g->m_alive && !g->m_culled && g->draw)
        {
            zRenderStateSetSrcBlendMode(s->tasset->renderSrcBlendMode);
            zRenderStateSetDstBlendMode(s->tasset->renderDstBlendMode);
            zRenderStateSetTexture(s->tasset->textureID);
            g->draw(s, g);
        }
    }
}

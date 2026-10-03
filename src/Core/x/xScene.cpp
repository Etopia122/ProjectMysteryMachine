#include "xScene.h"
#include "xEnt.h"
#include "xMemMgr.h"
#include "xserializer.h"

class _xEnv;

void xAnimInit();
void xAnimPoolInit(xMemPool* pool, unsigned int count, unsigned int singles,
                   unsigned int blendFlags, unsigned int effectMax);
void xAnimTempTransitionInit(unsigned int count);
void xModelInit();
void xModelPoolInit(unsigned int count, unsigned int numMatrices);
void xCollideInit(_xScene* sc);
void xCollideFastInit(_xScene* sc);
void xEnvSetup(_xEnv* env);
void xQuickCullForRay(xQCData* q, _xRay3* r);
unsigned int xRayHitsBoundFast(const _xRay3* r, const _xBound* b);
unsigned int iRayHitsEnv(_xRay3* r, _xEnv* env, _xCollis* coll);

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xSceneInit(_xScene* sc, unsigned short num_trigs, unsigned short num_stats,
                unsigned short num_dyns, unsigned short num_npcs)
{
    sc->flags = 0;
    sc->num_trigs_allocd = num_trigs;
    sc->trigs = (_xEnt**)xMemAlloc(gActiveHeap, (unsigned int)num_trigs * sizeof(_xEnt*));
    sc->num_stats_allocd = num_stats;
    sc->stats = (_xEnt**)xMemAlloc(gActiveHeap, (unsigned int)num_stats * sizeof(_xEnt*));
    sc->num_dyns_allocd = num_dyns;
    sc->dyns = (_xEnt**)xMemAlloc(gActiveHeap, (unsigned int)num_dyns * sizeof(_xEnt*));
    sc->num_npcs_allocd = num_npcs;
    sc->npcs = (_xEnt**)xMemAlloc(gActiveHeap, (unsigned int)num_npcs * sizeof(_xEnt*));
    sc->num_ents_allocd = num_trigs + num_stats + num_dyns + num_npcs;
    sc->num_act_ents = 0;
    sc->act_ents = (_xEnt**)xMemAlloc(gActiveHeap, sc->num_ents_allocd * sizeof(_xEnt*));
    sc->num_nact_ents = 0;
    sc->nact_ents = (_xEnt**)xMemAlloc(gActiveHeap, sc->num_ents_allocd * sizeof(_xEnt*));
    sc->num_ents = 0;
    sc->num_trigs = 0;
    sc->num_stats = 0;
    sc->num_dyns = 0;
    sc->num_npcs = 0;
    sc->resolvID = 0;
    xAnimInit();
    xModelInit();
    xAnimPoolInit(&sc->mempool, 50, 1, 1, 4);
    xModelPoolInit(num_npcs + 24, 64);
    xModelPoolInit(num_npcs + 24 + num_dyns, 8);
    xModelPoolInit(num_stats + (num_npcs + 24 + num_dyns), 1);
    xAnimTempTransitionInit(16);
    xCollideInit(sc);
    xCollideFastInit(sc);
}

void xSceneSave(_xScene* sc, xSerial* s)
{
}

void xSceneLoad(_xScene* sc, xSerial* s)
{
}

void xSceneSetup(_xScene* sc)
{
    xEnvSetup(sc->env);
}

void xSceneAddEnt(_xScene* sc, _xEnt* ent)
{
    if (ent->collType == 1)
    {
        sc->trigs[sc->num_trigs++] = ent;
    }
    else if (ent->collType == 2)
    {
        sc->stats[sc->num_stats++] = ent;
    }
    else if (ent->collType == 4)
    {
        sc->dyns[sc->num_dyns++] = ent;
    }
    else if (ent->collType == 8)
    {
        sc->npcs[sc->num_npcs++] = ent;
    }
    sc->act_ents[sc->num_act_ents++] = ent;
}

xBase* xSceneResolvID(_xScene* sc, unsigned int id)
{
    if (sc->resolvID)
    {
        return sc->resolvID(id);
    }
    return 0;
}

char* xSceneID2Name(_xScene* sc, unsigned int id)
{
    if (sc->id2Name)
    {
        return sc->id2Name(id);
    }
    return 0;
}

void xSceneForAllTriggers(_xScene* sc, _xEnt* (*func)(_xEnt*, _xScene*, void*), void* data)
{
    for (unsigned short i = 0; i < sc->num_trigs; i++)
    {
        if (!func(sc->trigs[i], sc, data))
        {
            break;
        }
    }
}

void xSceneForAllStatics(_xScene* sc, _xEnt* (*func)(_xEnt*, _xScene*, void*), void* data)
{
    for (unsigned short i = 0; i < sc->num_stats; i++)
    {
        if (!func(sc->stats[i], sc, data))
        {
            break;
        }
    }
}

void xSceneForAllDynamics(_xScene* sc, _xEnt* (*func)(_xEnt*, _xScene*, void*), void* data)
{
    for (unsigned short i = 0; i < sc->num_dyns; i++)
    {
        if (!func(sc->dyns[i], sc, data))
        {
            break;
        }
    }
}

void xSceneForAllNPCs(_xScene* sc, _xEnt* (*func)(_xEnt*, _xScene*, void*), void* data)
{
    for (unsigned short i = 0; i < sc->num_npcs; i++)
    {
        if (!func(sc->npcs[i], sc, data))
        {
            break;
        }
    }
}

void xRayHitsScene(_xScene* sc, _xRay3* r, _xCollis* coll)
{
    xQCData q;
    _xCollis c;
    unsigned short i;

    xQuickCullForRay(&q, r);
    coll->dist = 1e19f;

    for (i = 0; i < sc->num_stats; i++)
    {
        _xEnt* ent = sc->stats[i];
        xRayHitsBound(r, &q, &ent->bound, &c);
        if ((c.flags & 1) && c.dist < coll->dist)
        {
            coll->dist = c.dist;
            coll->oid = ent->id;
            coll->optr = ent;
        }
    }

    for (i = 0; i < sc->num_dyns; i++)
    {
        _xEnt* ent = sc->dyns[i];
        xRayHitsBound(r, &q, &ent->bound, &c);
        if ((c.flags & 1) && c.dist < coll->dist)
        {
            coll->dist = c.dist;
            coll->oid = ent->id;
            coll->optr = ent;
        }
    }

    for (i = 0; i < sc->num_npcs; i++)
    {
        _xEnt* ent = sc->npcs[i];
        xRayHitsBound(r, &q, &ent->bound, &c);
        if ((c.flags & 1) && c.dist < coll->dist)
        {
            coll->dist = c.dist;
            coll->oid = ent->id;
            coll->optr = ent;
        }
    }

    iRayHitsEnv(r, sc->env, &c);
    if (c.dist < coll->dist)
    {
        coll->dist = c.dist;
        coll->oid = c.oid;
        coll->optr = 0;
    }

    if (coll->dist < 1e19f)
    {
        coll->flags |= 1;
    }
    else
    {
        coll->flags &= ~1;
    }
}

void xRayHitsCameraScene(_xScene* sc, _xRay3* r, void (*camera_func)(_xEnt*))
{
    xQCData q;
    unsigned short i;

    xQuickCullForRay(&q, r);

    for (i = 0; i < sc->num_stats; i++)
    {
        _xEnt* ent = sc->stats[i];
        if ((ent->moreFlags & 1) && xQuickCullIsects(&q, &ent->bound.qcd) &&
            xRayHitsBoundFast(r, &ent->bound))
        {
            camera_func(ent);
        }
    }

    for (i = 0; i < sc->num_dyns; i++)
    {
        _xEnt* ent = sc->dyns[i];
        if ((ent->moreFlags & 1) && xQuickCullIsects(&q, &ent->bound.qcd) &&
            xRayHitsBoundFast(r, &ent->bound))
        {
            camera_func(ent);
        }
    }
}

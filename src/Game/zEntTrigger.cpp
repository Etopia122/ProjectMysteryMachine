#include "zEnt.h"
#include "zScene.h"
#include "rwtypes.h"

class _xBox
{
public:
    _xVec3 upper;
    _xVec3 lower;
};

class _xSphere
{
public:
    _xVec3 center;
    float r;
};

class _xCylinder
{
public:
    _xVec3 center;
    float r;
    float h;
};

class _xIsect
{
public:
    unsigned int flags;
    float penned;
    float contained;
    float lapped;
    _xVec3 point;
    _xVec3 norm;
    float dist;
};

class p2EntTriggerAsset
{
public:
    _xVec3 p[4];
    _xVec3 direction;
    unsigned int flags;
};

class zEntTrigger : public _zEnt
{
};

void iBoxIsectVec(_xBox* b, _xVec3* v, _xIsect* isx);
void iCylinderIsectVec(_xCylinder* c, _xVec3* v, _xIsect* isx);
void iSphereIsectVec(_xSphere* s, _xVec3* v, _xIsect* isx);

void zEntTriggerUpdate(zEntTrigger* trig, _xScene* sc, float dt);
signed int zEntTriggerEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                              xBase* toParamWidget);

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zEntTriggerInit(void* ent, void* asset)
{
    zEntTrigger* trig = (zEntTrigger*)ent;

    zEntInit(trig, (p2EntAsset*)asset, 'TRIG');
    trig->move = 0;
    trig->update = (void (*)(_xEnt*, _xScene*, float))zEntTriggerUpdate;
    trig->eventFunc = zEntTriggerEventCB;
    if (trig->linkCount)
    {
        trig->link = (p2LinkAsset*)((unsigned char*)trig->asset + sizeof(p2EntAsset) +
                                    sizeof(p2EntTriggerAsset));
    }
    else
    {
        trig->link = 0;
    }
}

// TODO (97.8%): the original computes each `penned <= 0.0f` result in r0
// and then copies it to collide (r27); here it goes straight to r27.
void zEntTriggerUpdate(zEntTrigger* trig, _xScene* sc, float dt)
{
    if (xBaseIsEnabled(trig))
    {
        unsigned int i;
        p2LinkAsset* link = trig->link;
        p2EntTriggerAsset* tasset = (p2EntTriggerAsset*)(trig->asset + 1);

        for (i = 0; i < trig->linkCount; i++)
        {
            if (link->chkAssetID && link->srcEvent >= 0xD4 && link->srcEvent <= 0xD7)
            {
                _xIsect isect;
                signed int collide = 0;

                if (link->chkAssetID)
                {
                    _xEnt* chkEnt = (_xEnt*)zSceneFindObject(link->chkAssetID);
                    if (chkEnt)
                    {
                        _xVec3* chkPos;
                        if (chkEnt->bound.type == 1)
                        {
                            chkPos = (_xVec3*)chkEnt->bound.data;
                        }
                        else
                        {
                            chkPos = (_xVec3*)&chkEnt->model->Mat->pos;
                        }

                        switch (trig->subType)
                        {
                        case 0:
                        {
                            _xBox box;
                            box.upper = tasset->p[1];
                            box.lower = tasset->p[0];
                            iBoxIsectVec(&box, chkPos, &isect);
                            collide = (isect.penned <= 0.0f);
                            break;
                        }
                        case 1:
                        {
                            _xSphere sphere;
                            _xIsect isect;
                            sphere.center = tasset->p[0];
                            sphere.r = tasset->p[1].x;
                            iSphereIsectVec(&sphere, chkPos, &isect);
                            collide = (isect.penned <= 0.0f);
                            break;
                        }
                        case 2:
                        {
                            _xCylinder cylinder;
                            _xIsect isect;
                            cylinder.center = tasset->p[0];
                            cylinder.r = tasset->p[1].x;
                            cylinder.h = tasset->p[1].y;
                            iCylinderIsectVec(&cylinder, chkPos, &isect);
                            collide = (isect.penned <= 0.0f);
                            break;
                        }
                        case 3:
                        {
                            _xSphere sphere;
                            _xIsect isect;
                            sphere.center = tasset->p[0];
                            sphere.r = tasset->p[1].x;
                            iSphereIsectVec(&sphere, chkPos, &isect);
                            collide = (isect.penned <= 0.0f);
                            break;
                        }
                        case 4:
                        case 5:
                            break;
                        }
                    }
                }

                if (collide)
                {
                    if (link->srcEvent == 0xD4)
                    {
                        link->srcEvent = 0xD6;
                        zEntEvent(trig, 0xD4, zSceneFindObject(link->dstAssetID), link->dstEvent,
                                  link->param, zSceneFindObject(link->paramWidgetAssetID), 0);
                    }
                    if (link->srcEvent == 0xD5)
                    {
                        link->srcEvent = 0xD7;
                    }
                }
                else
                {
                    if (link->srcEvent == 0xD7)
                    {
                        link->srcEvent = 0xD5;
                        zEntEvent(trig, 0xD5, zSceneFindObject(link->dstAssetID), link->dstEvent,
                                  link->param, zSceneFindObject(link->paramWidgetAssetID), 0);
                    }
                    if (link->srcEvent == 0xD6)
                    {
                        link->srcEvent = 0xD4;
                    }
                }
            }
            link++;
        }
    }
}

signed int zEntTriggerEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                              xBase* toParamWidget)
{
    switch (toEvent)
    {
    case 0xA:
        zEntReset((zEntTrigger*)to);
        break;
    }
    return 1;
}

void zEntTriggerSave(zEntTrigger* ent, xSerial* s)
{
    zEntSave(ent, s);
}

void zEntTriggerLoad(zEntTrigger* ent, xSerial* s)
{
    zEntLoad(ent, s);
}

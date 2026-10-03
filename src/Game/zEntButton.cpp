#include "zEnt.h"
#include "zGlobals.h"
#include "xEntMotion.h"

class zEntButtonAsset
{
public:
    unsigned int actMethod;
    signed int initButtonState;
    signed int isReset;
    signed int isSpringBB;
    unsigned char buttonFlags;
};

class _zEntButton : public _zEnt
{
public:
    zEntButtonAsset* basset; // 0xEC
    _xEntMotion motion;      // 0xF0
    unsigned int state;      // 0x15C
    float speed;             // 0x160
    unsigned int oldState;   // 0x164
    signed int oldMotState;  // 0x168
    unsigned int counter;    // 0x16C
};

// mech.state of the motion's union (0x34 into it)
#define MECH_STATE(m) ((m).mech.state)

void zEntButton_Update(_zEntButton* ent, _xScene* sc, float dt);
void zEntButton_Move(_zEntButton* ent, _xScene* s, float dt, _xEntFrame* frame);
signed int zEntButtonEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                             xBase* toParamWidget);

// Functions are in reverse DOL order (-inline deferred emits last to first).

// TODO (99.3%): the original materialises the Update/Move/EventCB addresses in
// that order; this version loads EventCB's first.
void zEntButton_Init(_zEntButton* ent, p2EntAsset* asset)
{
    zEntButtonAsset* basset;
    p2EntMotionAsset* masset;

    zEntInit(ent, asset, 'BUTN');
    basset = (zEntButtonAsset*)(asset + 1);
    ent->basset = basset;
    masset = (p2EntMotionAsset*)(basset + 1);

    ent->pflags |= 1;
    ent->penby |= 0x10;
    ent->chkby |= 0x10;
    ent->update = (void (*)(_xEnt*, _xScene*, float))zEntButton_Update;
    ent->move = (void (*)(_xEnt*, _xScene*, float, _xEntFrame*))zEntButton_Move;
    ent->eventFunc = zEntButtonEventCB;
    if (ent->linkCount)
    {
        ent->link = (p2LinkAsset*)(masset + 1);
    }
    else
    {
        ent->link = 0;
    }
    ent->eventFunc = zEntButtonEventCB;

    ent->state = 0;
    ent->oldState = ent->state;
    ent->counter = 1000;

    xEntReset(ent);
    xEntMotionInit(&ent->motion, ent, masset);
    xEntMotionRun(&ent->motion);
    ent->oldMotState = MECH_STATE(ent->motion);
}

void zEntButton_Move(_zEntButton* ent, _xScene* s, float dt, _xEntFrame* frame)
{
    xEntMotionMove(&ent->motion, s, dt, frame);
}

void zEntButton_Setup(_zEntButton* ent, _xScene* sc)
{
    xEntMotionSetup(&ent->motion, sc);
}

void zEntButton_Save(_zEntButton* ent, xSerial* s)
{
    zEntSave(ent, s);
}

void zEntButton_Load(_zEntButton* ent, xSerial* s)
{
    zEntLoad(ent, s);
}

#pragma auto_inline on
void zEntButton_Reset(_zEntButton* ent, _xScene* sc)
{
    xEntReset(ent);
    xEntMotionReset(&ent->motion, sc);

    ent->pflags |= 1;
    ent->penby |= 0x10;
    if (ent->asset->flags & 1)
    {
        ent->flags |= 1;
    }
    ent->chkby |= 0x10;

    ent->state = 0;
    xEntMotionStop(&ent->motion);
    ent->oldState = 5;
    ent->oldMotState = 6;
}
#pragma auto_inline off

void zEntButton_Update(_zEntButton* ent, _xScene* sc, float dt)
{
    xEntUpdate(ent, sc, dt);

    if (MECH_STATE(ent->motion) != ent->oldMotState)
    {
        switch (MECH_STATE(ent->motion))
        {
        case 0:
            if (ent->state == 4)
            {
                ent->state = 0;
            }
            break;
        case 2:
            if (!(ent->motion.flags & 4))
            {
                ent->state = 3;
            }
            break;
        case 3:
            ent->state = 4;
            break;
        case 4:
        case 5:
        case 6:
            break;
        }
    }
    ent->oldMotState = MECH_STATE(ent->motion);

    if (ent->state != ent->oldState)
    {
        switch (ent->state)
        {
        case 1:
            xEntMotionRun(&ent->motion);
            break;
        case 3:
            xEntMotionStop(&ent->motion);
            break;
        case 4:
            xEntMotionRun(&ent->motion);
            break;
        case 0:
            if (MECH_STATE(ent->motion) == 0)
            {
                xEntMotionStop(&ent->motion);
            }
            break;
        }
        ent->oldState = ent->state;
    }

    if (ent->basset->isReset)
    {
        ent->counter++;
    }
    if (ent->counter == 80)
    {
        zEntEvent(ent, ent, 0x38);
    }
}

unsigned int zEntButton_Headbutt(_zEntButton* ent)
{
    return (ent->basset->actMethod == 2) ? 4 : 0;
}

unsigned int zEntButton_Stand(_zEntButton* ent)
{
    return ent->basset->actMethod == 0;
}

unsigned int zEntButton_ButtBounce(_zEntButton* ent)
{
    if (ent->basset->actMethod == 1)
    {
        return ent->basset->isSpringBB ? 3 : 2;
    }
    return 0;
}

unsigned int zEntButton_CheckState(_zEntButton* ent)
{
    return ent->state;
}

signed int zEntButtonEventCB(xBase* from, xBase* to, unsigned int toEvent, float* toParam,
                             xBase* toParamWidget)
{
    _zEntButton* s = (_zEntButton*)to;

    switch (toEvent)
    {
    case 0x3:
    case 0x55:
        s->chkby |= 0x10;
        s->flags |= 1;
        break;
    case 0x56:
    case 0x4:
        s->chkby &= 0xEF;
        s->flags &= 0xFE;
        break;
    case 0xA:
        zEntButton_Reset(s, (_xScene*)globals.sceneCur);
        break;
    case 0x37:
        if (s->state == 0)
        {
            s->state = 1;
            s->counter = 0;
        }
        break;
    case 0x38:
        if (s->state != 0)
        {
            if (s->state != 0)
            {
                s->state = 4;
            }
        }
        break;
    case 0xF:
        if (s->state != 0)
        {
            zEntEvent(s, s, 0x38);
        }
        else
        {
            zEntEvent(s, s, 0x37);
        }
        break;
    case 0xC3:
    case 0xC4:
    case 0xC5:
    case 0xC6:
    case 0xC7:
    case 0xC8:
    case 0xC9:
    case 0xCA:
        zEntAnimEvent(s, toEvent, toParam);
        break;
    }

    return 1;
}

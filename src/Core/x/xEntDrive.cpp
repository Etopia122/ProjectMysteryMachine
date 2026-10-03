#include "xEntDrive.h"


// Functions are in reverse DOL order (-inline deferred emits last to first).

void xEntDriveInit(_xEntDrive* drv, _xEnt* driven)
{
    drv->flags = 0;
    drv->driven = driven;
    drv->driver = 0;
    drv->s = 0.0f;
    drv->tm = 0.0f;
    drv->tmr = 0.0f;
    drv->odriver = 0;
    drv->os = 0.0f;
    drv->otm = 0.0f;
    drv->otmr = 0.0f;
}

void xEntDriveMount(_xEntDrive* drv, _xEnt* driver, float mt)
{
    _xVec3 euler;

    if (driver == drv->odriver && drv->os)
    {
        drv->driver = driver;
        drv->s = drv->os;
        drv->tm = mt;
        drv->tmr = mt * (1.0f - drv->s);
        drv->odriver = 0;
        drv->os = 0.0f;
        drv->otm = 0.0f;
        drv->otmr = 0.0f;
    }
    else
    {
        drv->driver = driver;
        drv->s = 0.0f;
        drv->tm = mt;
        drv->tmr = mt;
    }

    if (drv->flags & 1)
    {
        xMat3x3GetEuler((_xMat3x3*)&drv->driver->frame->mat, &euler);
        drv->yaw = euler.x;
    }

    xVec3Copy(&drv->q, &drv->driven->frame->mat.pos);
    xMat4x3Tolocal(&drv->p, &drv->driver->frame->mat, &drv->q);
}

void xEntDriveDismount(_xEntDrive* drv, float dmt)
{
    if (drv->driver)
    {
        drv->odriver = drv->driver;
        drv->os = drv->s;
        drv->otm = dmt;
        drv->otmr = dmt * drv->os;
        drv->driver = 0;
        drv->s = 0.0f;
        drv->tm = 0.0f;
        drv->tmr = 0.0f;
        xVec3Copy(&drv->q, &drv->driven->frame->mat.pos);
        xMat4x3Tolocal(&drv->op, &drv->odriver->frame->mat, &drv->q);
    }
}

void xEntDriveUpdate(_xEntDrive* drv, float dt)
{
    _xMat3x3 rot;
    _xVec3 euler;
    _xVec3 newq;

    if (!drv->odriver && !drv->driver)
    {
        return;
    }

    if (drv->otmr > 0.0f)
    {
        drv->otmr -= dt;
        if (drv->otmr <= 0.0f)
        {
            drv->os = 0.0f;
            drv->otmr = 0.0f;
        }
        else
        {
            drv->os = drv->otmr / drv->otm;
        }
    }

    if (drv->tmr > 0.0f)
    {
        drv->tmr -= dt;
        if (drv->tmr <= 0.0f)
        {
            drv->s = 1.0f;
            drv->tmr = 0.0f;
        }
        else
        {
            drv->s = 1.0f - drv->tmr / drv->tm;
        }
    }

    if (!drv->os && !drv->s)
    {
        return;
    }

    if (drv->s && (drv->flags & 1))
    {
        xMat3x3GetEuler((_xMat3x3*)&drv->driver->frame->mat, &euler);
        xMat3x3RotY(&rot, drv->s * (euler.x - drv->yaw));
        xMat3x3Mul((_xMat3x3*)&drv->driven->frame->mat, (_xMat3x3*)&drv->driven->frame->mat, &rot);
        drv->yaw = euler.x;
    }

    if (drv->os)
    {
        xMat4x3Toworld(&newq, &drv->odriver->frame->mat, &drv->op);
        xVec3Sub(&drv->driven->frame->dpos, &newq, &drv->q);
        xVec3SMulBy(&drv->driven->frame->dpos, drv->os);
        xVec3AddTo(&drv->driven->frame->mat.pos, &drv->driven->frame->dpos);
    }

    if (drv->s)
    {
        xMat4x3Toworld(&newq, &drv->driver->frame->mat, &drv->p);
        xVec3Sub(&drv->driven->frame->dpos, &newq, &drv->q);
        xVec3SMulBy(&drv->driven->frame->dpos, drv->s);
        xVec3AddTo(&drv->driven->frame->mat.pos, &drv->driven->frame->dpos);
    }

    xVec3Copy(&drv->q, &drv->driven->frame->mat.pos);
    if (drv->os)
    {
        xMat4x3Tolocal(&drv->op, &drv->odriver->frame->mat, &drv->q);
    }
    if (drv->s)
    {
        xMat4x3Tolocal(&drv->p, &drv->driver->frame->mat, &drv->q);
    }
}

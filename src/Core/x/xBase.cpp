#include "xBase.h"
#include "xserializer.h"

void xBaseInit(xBase* xb, p2BaseAsset* asset)
{
    xb->id = asset->id;
    xb->baseType = asset->baseType;
    xb->baseFlags = asset->baseFlags;
    xb->linkCount = asset->linkCount;
    xb->link = 0;
    xBaseValidate(xb);
}

void xBaseSetup()
{
}

void xBaseSave(xBase* ent, xSerial* s)
{
    if (xBaseIsEnabled(ent))
    {
        s->Write_b1(1);
    }
    else
    {
        s->Write_b1(0);
    }
}

void xBaseLoad(xBase* ent, xSerial* s)
{
    signed int b = 0;
    s->Read_b1(&b);
    if (b)
    {
        xBaseEnable(ent);
    }
    else
    {
        xBaseDisable(ent);
    }
}

void xBaseReset(xBase* xb, p2BaseAsset* asset)
{
    xb->baseFlags = asset->baseFlags;
    xBaseValidate(xb);
}

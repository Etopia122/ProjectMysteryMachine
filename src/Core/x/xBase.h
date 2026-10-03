#ifndef XBASE_H
#define XBASE_H

class xSerial;

class p2LinkAsset
{
public:
    unsigned short srcEvent;
    unsigned short dstEvent;
    unsigned int dstAssetID;
    float param[4];
    unsigned int paramWidgetAssetID;
    unsigned int chkAssetID;
};

class p2BaseAsset
{
public:
    unsigned int id;
    unsigned char baseType;
    unsigned char linkCount;
    unsigned short baseFlags;
};

class xBase
{
public:
    unsigned int id;
    unsigned char baseType;
    unsigned char linkCount;
    unsigned short baseFlags;
    p2LinkAsset* link;
    signed int (*eventFunc)(xBase*, xBase*, unsigned int, float*, xBase*);
};

void xBaseInit(xBase* xb, p2BaseAsset* asset);
void xBaseSetup();
void xBaseSave(xBase* ent, xSerial* s);
void xBaseLoad(xBase* ent, xSerial* s);
void xBaseReset(xBase* xb, p2BaseAsset* asset);

inline void xBaseValidate(xBase* xb)
{
    xb->baseFlags |= 4;
}

// zEnt.cpp defines these as ordinary functions (they are emitted there out of line)
#ifndef XBASE_ENABLE_OUTOFLINE
inline unsigned int xBaseIsEnabled(xBase* xb)
{
    return xb->baseFlags & 1;
}

inline void xBaseEnable(xBase* xb)
{
    xb->baseFlags |= 1;
}

inline void xBaseDisable(xBase* xb)
{
    xb->baseFlags &= ~1;
}
#else
unsigned int xBaseIsEnabled(xBase* xb);
void xBaseEnable(xBase* xb);
void xBaseDisable(xBase* xb);
#endif

#endif

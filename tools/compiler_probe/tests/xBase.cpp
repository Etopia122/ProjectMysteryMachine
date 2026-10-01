// Probe source for xBase.cpp (types from the PS2 DWARF dump), written from
// the GameCube disassembly. Functions are in source order (Init first); the
// DOL has them in reverse.

struct p2LinkAsset;
struct st_SERIAL_CLIENTINFO;

class xSerial
{
public:
    unsigned int idtag;
    signed int baseoff;
    st_SERIAL_CLIENTINFO* ctxtdata;
    signed int warned;
    signed int curele;
    signed int bitidx;
    signed int bittally;

    signed int Write_b1(signed int bits);
    signed int Read_b1(signed int* bits);
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

class p2BaseAsset
{
public:
    unsigned int id;
    unsigned char baseType;
    unsigned char linkCount;
    unsigned short baseFlags;
};

inline void xBaseValidate(xBase* xb)
{
    xb->baseFlags |= 4;
}

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

#ifndef RWTYPES_H
#define RWTYPES_H

// Minimal RenderWare type declarations (layouts from the PS2 DWARF).

class RwV3d
{
public:
    float x;
    float y;
    float z;
};

class RwMatrixTag
{
public:
    RwV3d right;
    unsigned int flags;
    RwV3d up;
    unsigned int pad1;
    RwV3d at;
    unsigned int pad2;
    RwV3d pos;
    unsigned int pad3;
};

class RwLLLink
{
public:
    RwLLLink* next;
    RwLLLink* prev;
};

class RwObject
{
public:
    unsigned char type;
    unsigned char subType;
    unsigned char flags;
    unsigned char privateFlags;
    void* parent;
};

class RwFrame
{
public:
    RwObject object;
    RwLLLink inDirtyListLink;
    RwMatrixTag modelling;
    RwMatrixTag ltm;
};

class RpLight
{
public:
    RwObject object;
};

class RwRGBAReal;
class RpWorld;
class RwCamera;

#endif

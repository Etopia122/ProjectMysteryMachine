#ifndef RWSDK_BAIM3D_INT_H
#define RWSDK_BAIM3D_INT_H

/* Immediate mode 3D vertex pool shared by RwIm3D and the Imm* nodes. */

#include "rwsdk/p2core_int.h"

#define rwIM3D_VERTEXUV 0x01
#define rwIM3D_ALLOPAQUE 0x02
#define rwIM3D_NOCLIP 0x04
#define rwIM3D_VERTEXXYZ 0x08
#define rwIM3D_VERTEXRGBA 0x10

#define rwPRIMTYPENAPRIMTYPE 0
#define rwPRIMTYPELINELIST 1
#define rwPRIMTYPEPOLYLINE 2
#define rwPRIMTYPETRILIST 3
#define rwPRIMTYPETRISTRIP 4
#define rwPRIMTYPETRIFAN 5
#define rwPRIMTYPEPOINTLIST 6

typedef RwUInt16 RxVertexIndex;

typedef struct RxObjSpace3DVertex {
    /* 0x00 */ RwV3d objVertex;
    /* 0x0C */ RwV3d objNormal;
    /* 0x18 */ RwRGBA color;
    /* 0x1C */ RwReal u;
    /* 0x20 */ RwReal v;
} RxObjSpace3DVertex;

typedef struct rwIm3DPoolStash {
    /* 0x00 */ RwUInt32 flags;
    /* 0x04 */ RwMatrix* ltm;
    /* 0x08 */ RwUInt32 numVerts;
    /* 0x0C */ RxObjSpace3DVertex* objVerts;
    /* 0x10 */ void* camVerts;
    /* 0x14 */ void* devVerts;
    /* 0x18 */ void* meshState;
    /* 0x1C */ void* renderState;
    /* 0x20 */ RxPipeline* pipeline;
    /* 0x24 */ RwInt32 primType;
    /* 0x28 */ RxVertexIndex* indices;
    /* 0x2C */ RwUInt32 numIndices;
} rwIm3DPoolStash;

typedef struct rwIm3DPool {
    /* 0x00 */ RwUInt16 numElements;
    /* 0x02 */ RwUInt16 pad;
    /* 0x04 */ void* elements;
    /* 0x08 */ RwInt32 stride;
    /* 0x0C */ rwIm3DPoolStash stash;
} rwIm3DPool;

#endif

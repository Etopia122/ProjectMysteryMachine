#ifndef RWSDK_RPWORLD_INT_H
#define RWSDK_RPWORLD_INT_H

/* World plugin object layouts shared by the world source files. */

#include "rwsdk/rwcore.h"
#include "rwsdk/p2core_int.h"

#define rpWORLD 7
#define rpWORLDMAXBSPDEPTH 64
#define rpWORLDTRISTRIP 0x01
#define rpWORLDSINGLEMALLOC 0x01
#define rpWORLDRENDERFRONT2BACK 1
#define rpWORLDRENDERBACK2FRONT 2
#define rpATOMICCOLLISIONTEST 0x01
#define rpATOMICRENDER 0x04
#define rpLIGHTPOSITIONINGSTART 0x80
#define rpLIGHTLIGHTATOMICS 0x01
#define rpGEOMETRYLIGHT 0x20

#define rwSECTORATOMIC -1
#define rwSECTORBUILD -2

typedef struct RpMaterial RpMaterial;
typedef struct RpBuildMesh RpBuildMesh;
typedef struct RpMeshHeader RpMeshHeader;
typedef struct RwResEntry RwResEntry;

typedef struct RwSurfaceProperties {
    RwReal ambient;
    RwReal specular;
    RwReal diffuse;
} RwSurfaceProperties;

typedef struct RwTexCoords {
    RwReal u;
    RwReal v;
} RwTexCoords;

typedef struct RpMaterialList {
    RpMaterial** materials;
    RwInt32 numMaterials;
    RwInt32 space;
} RpMaterialList;

typedef struct RpPolygon {
    RwUInt16 matIndex;
    RwUInt16 vertIndex[3];
} RpPolygon;

typedef struct RpSector {
    RwInt32 type;
} RpSector;

typedef struct RpPlaneSector {
    /* 0x00 */ RwInt32 type;
    /* 0x04 */ RwReal value;
    /* 0x08 */ RpSector* leftSubTree;
    /* 0x0C */ RpSector* rightSubTree;
    /* 0x10 */ RwReal leftValue;
    /* 0x14 */ RwReal rightValue;
} RpPlaneSector;

typedef struct RpWorldSector {
    /* 0x00 */ RwInt32 type;
    /* 0x04 */ RpPolygon* polygons;
    /* 0x08 */ RwV3d* vertices;
    /* 0x0C */ RwV3d* normals;
    /* 0x10 */ RwTexCoords* texCoords[8];
    /* 0x30 */ RwRGBA* preLitLum;
    /* 0x34 */ RwResEntry* repEntry;
    /* 0x38 */ RwLinkList collAtomicsInWorldSector;
    /* 0x40 */ RwLinkList noCollAtomicsInWorldSector;
    /* 0x48 */ RwLinkList lightsInWorldSector;
    /* 0x50 */ RwBBox boundingBox;
    /* 0x68 */ RwBBox tightBoundingBox;
    /* 0x80 */ void* unk80;
    /* 0x84 */ RpMeshHeader* mesh;
    /* 0x88 */ RxPipeline* pipeline;
    /* 0x8C */ RwUInt16 matListWindowBase;
    /* 0x8E */ RwUInt16 numVertices;
    /* 0x90 */ RwUInt16 numPolygons;
    /* 0x92 */ RwUInt16 pad;
} RpWorldSector;

typedef RpWorldSector* (*RpWorldSectorCallBackRender)(RpWorldSector* worldSector);
typedef RpWorldSector* (*RpWorldSectorCallBack)(RpWorldSector* worldSector, void* data);

typedef struct RpWorld {
    /* 0x00 */ RwObject object;
    /* 0x08 */ RwUInt32 flags;
    /* 0x0C */ RwInt32 renderOrder;
    /* 0x10 */ RpMaterialList matList;
    /* 0x1C */ RpSector* rootSector;
    /* 0x20 */ RwInt32 numTexCoordSets;
    /* 0x24 */ RwInt32 numClumpsInWorld;
    /* 0x28 */ RwLLLink* currentClumpLink;
    /* 0x2C */ RwLinkList clumpList;
    /* 0x34 */ RwLinkList lightList;
    /* 0x3C */ RwLinkList directionalLightList;
    /* 0x44 */ RwV3d worldOrigin;
    /* 0x50 */ RwBBox boundingBox;
    /* 0x68 */ RwSurfaceProperties surfaceProps;
    /* 0x74 */ RpWorldSectorCallBackRender renderCallBack;
    /* 0x78 */ RxPipeline* pipeline;
} RpWorld;

typedef struct RwSphere {
    RwV3d center;
    RwReal radius;
} RwSphere;

typedef struct RpInterpolator {
    /* 0x00 */ RwInt32 flags;
    /* 0x04 */ RwInt16 startMorphTarget;
    /* 0x06 */ RwInt16 endMorphTarget;
    /* 0x08 */ RwReal time;
    /* 0x0C */ RwReal recipTime;
    /* 0x10 */ RwReal position;
} RpInterpolator;

typedef struct RpMorphTarget {
    /* 0x00 */ struct RpGeometry* parentGeom;
    /* 0x04 */ RwSphere boundingSphere;
    /* 0x14 */ RwV3d* verts;
    /* 0x18 */ RwV3d* normals;
} RpMorphTarget;

typedef struct RpAtomic {
    /* 0x00 */ RwObjectHasFrame object;
    /* 0x14 */ RwResEntry* repEntry;
    /* 0x18 */ struct RpGeometry* geometry;
    /* 0x1C */ RwSphere boundingSphere;
    /* 0x2C */ RwSphere worldBoundingSphere;
    /* 0x3C */ struct RpClump* clump;
    /* 0x40 */ RwLLLink inClumpLink;
    /* 0x48 */ struct RpAtomic* (*renderCallBack)(struct RpAtomic* atomic);
    /* 0x4C */ RpInterpolator interpolator;
    /* 0x60 */ RwUInt16 renderFrame;
    /* 0x62 */ RwUInt16 pad;
    /* 0x64 */ RwLinkList llWorldSectorsInAtomic;
    /* 0x6C */ RxPipeline* pipeline;
} RpAtomic;

typedef struct RpLight {
    /* 0x00 */ RwObjectHasFrame object;
    /* 0x14 */ RwReal radius;
    /* 0x18 */ RwReal color[4];
    /* 0x28 */ RwReal minusCosAngle;
    /* 0x2C */ RwLinkList WorldSectorsInLight;
    /* 0x34 */ RwLLLink inWorld;
    /* 0x3C */ RwUInt16 lightFrame;
    /* 0x3E */ RwUInt16 pad;
} RpLight;

typedef struct RpClump {
    /* 0x00 */ RwObject object;
    /* 0x08 */ RwLinkList atomicList;
    /* 0x10 */ RwLinkList lightList;
    /* 0x18 */ RwLinkList cameraList;
    /* 0x20 */ RwLLLink inWorldLink;
    /* 0x28 */ void* callback;
} RpClump;

typedef struct RpTriangle {
    RwUInt16 vertIndex[3];
    RwInt16 matIndex;
} RpTriangle;

typedef struct RpGeometry {
    /* 0x00 */ RwObject object;
    /* 0x08 */ RwUInt32 flags;
    /* 0x0C */ RwUInt16 lockedSinceLastInst;
    /* 0x0E */ RwInt16 refCount;
    /* 0x10 */ RwInt32 numTriangles;
    /* 0x14 */ RwInt32 numVertices;
    /* 0x18 */ RwInt32 numMorphTargets;
    /* 0x1C */ RwInt32 numTexCoordSets;
    /* 0x20 */ RpMaterialList matList;
    /* 0x2C */ RpTriangle* triangles;
    /* 0x30 */ RwRGBA* preLitLum;
    /* 0x34 */ RwTexCoords* texCoords[8];
    /* 0x54 */ RwSurfaceProperties surfaceProps;
    /* 0x60 */ RpMeshHeader* mesh;
    /* 0x64 */ RwResEntry* repEntry;
    /* 0x68 */ RpMorphTarget* morphTarget;
} RpGeometry;

typedef struct RwPlane {
    RwV3d normal;
    RwReal distance;
} RwPlane;

typedef struct RwFrustumPlane {
    RwPlane plane;
    RwUInt8 closestX;
    RwUInt8 closestY;
    RwUInt8 closestZ;
    RwUInt8 pad;
} RwFrustumPlane;

typedef struct RwCamera {
    /* 0x000 */ RwObjectHasFrame object;
    /* 0x014 */ RwInt32 projectionType;
    /* 0x018 */ struct RwCamera* (*beginUpdate)(struct RwCamera* camera);
    /* 0x01C */ struct RwCamera* (*endUpdate)(struct RwCamera* camera);
    /* 0x020 */ RwMatrix viewMatrix;
    /* 0x060 */ RwUInt8 pad60[0x98 - 0x60];
    /* 0x098 */ RwFrustumPlane frustumPlanes[6];
    /* 0x110 */ RwBBox frustumBoundBox;
    /* 0x128 */ RwV3d frustumCorners[8];
} RwCamera;

typedef RpLight* (*RpLightCallBack)(RpLight* light, void* data);

typedef struct RpTie {
    RwLLLink lAtomicInWorldSector;
    RpAtomic* apAtom;
    RwLLLink lWorldSectorInAtomic;
    RpWorldSector* worldSector;
} RpTie;

typedef struct RpLightTie {
    RwLLLink lightInWorldSector;
    RpLight* light;
    RwLLLink WorldSectorInLight;
    RpWorldSector* sect;
} RpLightTie;

#define SETCOORD(v, offset, value) (*(RwReal*)((RwUInt8*)&(v) + (offset)) = (value))
#define GETCOORD(v, offset) (*(RwReal*)((RwUInt8*)&(v) + (offset)))

#endif

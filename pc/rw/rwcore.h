#ifndef PC_RWCORE_H
#define PC_RWCORE_H

// PC implementation of the RenderWare 3.x subset Scooby-Doo uses.
// Structure layouts follow the game's RenderWare build (PS2 DWARF, which the
// game code also uses), because game code reads these structures directly.
// Only the PC platform layer includes this header; game code has its own declarations.

#include <stddef.h>
#include <stdint.h>

typedef int32_t RwInt32;
typedef uint32_t RwUInt32;
typedef int16_t RwInt16;
typedef uint16_t RwUInt16;
typedef uint8_t RwUInt8;
typedef float RwReal;
typedef int32_t RwBool;

#define RW_ASSERT_LAYOUT(T, f, off) static_assert(offsetof(T, f) == off, #T "." #f)
#define RW_ASSERT_SIZE(T, sz) static_assert(sizeof(T) == sz, "sizeof " #T)

extern "C" {

// ---------------------------------------------------------------- basics
struct RwV2d { float x, y; };
struct RwV3d { float x, y, z; };
struct RwRGBA { RwUInt8 red, green, blue, alpha; };
struct RwRGBAReal { float red, green, blue, alpha; };
struct RwTexCoords { float u, v; };
struct RwSphere { RwV3d center; float radius; };
struct RwBBox { RwV3d sup; RwV3d inf; };
struct RwPlane { RwV3d normal; float distance; };
struct RwLine { RwV3d start, end; };

struct RwLLLink { RwLLLink* next; RwLLLink* prev; };
struct RwLinkList { RwLLLink link; };

struct RwMatrix
{
    RwV3d right; RwUInt32 flags;
    RwV3d up; RwUInt32 pad1;
    RwV3d at; RwUInt32 pad2;
    RwV3d pos; RwUInt32 pad3;
};
RW_ASSERT_SIZE(RwMatrix, 0x40);
typedef RwMatrix RwMatrixTag;

enum RwOpCombineType { rwCOMBINEREPLACE = 0, rwCOMBINEPRECONCAT = 1, rwCOMBINEPOSTCONCAT = 2 };
enum { rwMATRIXTYPENORMAL = 1, rwMATRIXTYPEORTHOGONAL = 2, rwMATRIXTYPEORTHONORMAL = 3,
       rwMATRIXINTERNALIDENTITY = 0x20000 };

struct RwObject
{
    RwUInt8 type, subType, flags, privateFlags;
    void* parent;
};
RW_ASSERT_SIZE(RwObject, 8);

struct RwObjectHasFrame;
typedef RwObjectHasFrame* (*RwObjectHasFrameSyncFunction)(RwObjectHasFrame*);
struct RwObjectHasFrame
{
    RwObject object;
    RwLLLink lFrame;
    RwObjectHasFrameSyncFunction sync;
};
RW_ASSERT_SIZE(RwObjectHasFrame, 0x14);

// Object type ids (RwObject.type)
enum { rwFRAME = 0, rpATOMIC = 1, rpCLUMP = 2, rpLIGHT = 3, rwCAMERA = 4, rwTEXDICTIONARY = 6 };

// ---------------------------------------------------------------- frames
struct RwFrame
{
    RwObject object;
    RwLLLink inDirtyListLink;
    RwMatrix modelling;
    RwMatrix ltm;
    RwLinkList objectList;
    RwFrame* child;
    RwFrame* next;
    RwFrame* root;
};
RW_ASSERT_LAYOUT(RwFrame, ltm, 0x50);
RW_ASSERT_LAYOUT(RwFrame, root, 0xA0);
// RwFrame.object.privateFlags
enum { rwFRAMEPRIVATEHIERARCHYSYNCLTM = 0x01, rwFRAMEPRIVATEHIERARCHYSYNCOBJ = 0x02,
       rwFRAMEPRIVATESUBTREESYNCLTM = 0x04, rwFRAMEPRIVATESUBTREESYNCOBJ = 0x08,
       rwFRAMEPRIVATESTATIC = 0x10 };

// ---------------------------------------------------------------- rasters & textures
struct RwRaster
{
    RwRaster* parent;
    RwUInt8* cpPixels;
    RwUInt8* palette;
    RwInt32 width, height, depth, stride;
    RwInt16 nOffsetX, nOffsetY;
    RwUInt8 cType, cFlags, privateFlags, cFormat;
    RwUInt8* originalPixels;
    RwInt32 originalWidth, originalHeight, originalStride;
};
RW_ASSERT_SIZE(RwRaster, 0x34);
enum { rwRASTERTYPENORMAL = 0, rwRASTERTYPEZBUFFER = 1, rwRASTERTYPECAMERA = 2,
       rwRASTERTYPETEXTURE = 4, rwRASTERTYPECAMERATEXTURE = 5, rwRASTERTYPEMASK = 7,
       rwRASTERDONTALLOCATE = 0x80 };

struct RwTexDictionary;
struct RwTexture
{
    RwRaster* raster;
    RwTexDictionary* dict;
    RwLLLink lInDictionary;
    char name[32];
    char mask[32];
    RwUInt32 filterAddressing; // filter: bits 0-7, address U: 8-11, address V: 12-15 (RW 3.3)
    RwInt32 refCount;
};
RW_ASSERT_LAYOUT(RwTexture, filterAddressing, 0x50);
RW_ASSERT_LAYOUT(RwTexture, refCount, 0x54);
RW_ASSERT_SIZE(RwTexture, 0x58);
inline int rwTexFilter(const RwTexture* t) { return t->filterAddressing & 0xFF; }
inline int rwTexAddrU(const RwTexture* t) { return (t->filterAddressing >> 8) & 0xF; }
inline int rwTexAddrV(const RwTexture* t) { return (t->filterAddressing >> 12) & 0xF; }

struct RwTexDictionary
{
    RwObject object;
    RwLinkList texturesInDict;
    RwLLLink lInInstance;
};

struct RwImage
{
    RwInt32 flags, width, height, depth, stride;
    RwUInt8* cpPixels;
    RwRGBA* palette;
};

// ---------------------------------------------------------------- camera
struct RwFrustumPlane { RwPlane plane; RwUInt8 closestX, closestY, closestZ, pad; };
struct RwCamera;
typedef RwCamera* (*RwCameraBeginUpdateFunc)(RwCamera*);
struct RwCamera
{
    RwObjectHasFrame object;
    RwInt32 projectionType;
    RwCameraBeginUpdateFunc beginUpdate;
    RwCameraBeginUpdateFunc endUpdate;
    RwMatrix viewMatrix;
    RwRaster* frameBuffer;
    RwRaster* zBuffer;
    RwV2d viewWindow;
    RwV2d recipViewWindow;
    RwV2d viewOffset;
    float nearPlane, farPlane, fogPlane, zScale, zShift;
    RwUInt16 renderFrame, pad;
    RwFrustumPlane frustumPlanes[6];
    RwBBox frustumBoundBox;
    RwV3d frustumCorners[8];
};
RW_ASSERT_LAYOUT(RwCamera, frameBuffer, 0x60);
RW_ASSERT_LAYOUT(RwCamera, nearPlane, 0x80);
RW_ASSERT_LAYOUT(RwCamera, frustumCorners, 0x128);
enum { rwPERSPECTIVE = 1, rwPARALLEL = 2 };
enum { rwCAMERACLEARIMAGE = 1, rwCAMERACLEARZ = 2, rwCAMERACLEARSTENCIL = 4 };

// ---------------------------------------------------------------- streams
struct RwStreamMemory { RwUInt32 position, nSize; RwUInt8* memBlock; };
struct RwStreamFile { void* fpFile; };
struct RwStream
{
    RwInt32 type;
    RwInt32 accessType;
    RwInt32 position;
    union { RwStreamMemory memory; RwStreamFile file; RwUInt8 pad[0x14]; } Type;
    RwInt32 rwOwned;
};
RW_ASSERT_SIZE(RwStream, 0x24);
enum { rwSTREAMNASTREAM = 0, rwSTREAMFILE = 1, rwSTREAMFILENAME = 2, rwSTREAMMEMORY = 3, rwSTREAMCUSTOM = 4 };
enum { rwSTREAMREAD = 1, rwSTREAMWRITE = 2, rwSTREAMAPPEND = 3 };

struct RwChunkHeaderInfo
{
    RwUInt32 type, length, version, buildNum;
    RwInt32 isComplex;
};
struct RwMemory { RwUInt8* start; RwUInt32 length; };

// Chunk ids
enum {
    rwID_STRUCT = 0x01, rwID_STRING = 0x02, rwID_EXTENSION = 0x03, rwID_CAMERA = 0x05, rwID_TEXTURE = 0x06,
    rwID_MATERIAL = 0x07, rwID_MATLIST = 0x08, rwID_ATOMICSECT = 0x09, rwID_PLANESECT = 0x0A,
    rwID_WORLD = 0x0B, rwID_FRAMELIST = 0x0E, rwID_GEOMETRY = 0x0F, rwID_CLUMP = 0x10, rwID_LIGHT = 0x12,
    rwID_ATOMIC = 0x14, rwID_TEXTURENATIVE = 0x15, rwID_TEXDICTIONARY = 0x16, rwID_GEOMETRYLIST = 0x1A,
    rwID_SKINPLUGIN = 0x116, rwID_HANIMPLUGIN = 0x11E, rwID_USERDATAPLUGIN = 0x11F, rwID_MATERIALEFFECTSPLUGIN = 0x120,
    rwID_COLLISPLUGIN = 0x11D, rwID_BINMESHPLUGIN = 0x50E, rwID_NATIVEDATAPLUGIN = 0x510, rwID_FRAMENAME = 0x253F2FE,
};

// ---------------------------------------------------------------- geometry
struct RwSurfaceProperties { float ambient, specular, diffuse; };
struct RxPipeline;
struct RpMaterial
{
    RwTexture* texture;
    RwRGBA color;
    RxPipeline* pipeline;
    RwSurfaceProperties surfaceProps;
    RwInt16 refCount, pad;
};
RW_ASSERT_SIZE(RpMaterial, 0x1C);

struct RpMaterialList { RpMaterial** materials; RwInt32 numMaterials, space; };
struct RpTriangle { RwUInt16 vertIndex[3]; RwInt16 matIndex; };
struct RpGeometry;
struct RpMorphTarget
{
    RpGeometry* parentGeom;
    RwSphere boundingSphere;
    RwV3d* verts;
    RwV3d* normals;
};
RW_ASSERT_SIZE(RpMorphTarget, 0x1C);

struct RpMeshHeader
{
    RwUInt32 flags;
    RwUInt16 numMeshes, serialNum;
    RwUInt32 totalIndicesInMesh;
    RwUInt32 firstMeshOffset;
};
struct RpMesh { RwUInt16* indices; RwUInt32 numIndices; RpMaterial* material; };
enum { rpMESHHEADERTRISTRIP = 1, rpMESHHEADERTRIFAN = 2, rpMESHHEADERLINELIST = 4, rpMESHHEADERPOLYLINE = 8 };

struct RwResEntry
{
    RwLLLink link;
    RwInt32 size;
    void* owner;
    RwResEntry** ownerRef;
    void (*destroyNotify)(RwResEntry*);
};
RW_ASSERT_SIZE(RwResEntry, 0x18);

struct RpGeometry
{
    RwObject object;
    RwUInt32 flags;
    RwUInt16 lockedSinceLastInst;
    RwInt16 refCount;
    RwInt32 numTriangles, numVertices, numMorphTargets, numTexCoordSets;
    RpMaterialList matList;
    RpTriangle* triangles;
    RwRGBA* preLitLum;
    RwTexCoords* texCoords[8];
    RwSurfaceProperties ignoredSurfaceProps;
    RpMeshHeader* mesh;
    RwResEntry* repEntry;
    RpMorphTarget* morphTarget;
};
RW_ASSERT_LAYOUT(RpGeometry, matList, 0x20);
RW_ASSERT_LAYOUT(RpGeometry, texCoords, 0x34);
RW_ASSERT_LAYOUT(RpGeometry, morphTarget, 0x68);
enum { rpGEOMETRYTRISTRIP = 0x01, rpGEOMETRYPOSITIONS = 0x02, rpGEOMETRYTEXTURED = 0x04,
       rpGEOMETRYPRELIT = 0x08, rpGEOMETRYNORMALS = 0x10, rpGEOMETRYLIGHT = 0x20,
       rpGEOMETRYMODULATEMATERIALCOLOR = 0x40, rpGEOMETRYTEXTURED2 = 0x80,
       rpGEOMETRYNATIVE = 0x01000000 };

struct RpInterpolator
{
    RwInt32 flags;
    RwInt16 startMorphTarget, endMorphTarget;
    float time, recipTime, position;
};

struct RpClump;
struct RpAtomic;
typedef RpAtomic* (*RpAtomicCallBackRender)(RpAtomic*);
struct RpAtomic
{
    RwObjectHasFrame object;
    RwResEntry* repEntry;
    RpGeometry* geometry;
    RwSphere boundingSphere;
    RwSphere worldBoundingSphere;
    RpClump* clump;
    RwLLLink inClumpLink;
    RpAtomicCallBackRender renderCallBack;
    RpInterpolator interpolator;
    RwUInt16 renderFrame, pad;
    RwLinkList llWorldSectorsInAtomic;
    RxPipeline* pipeline;
};
RW_ASSERT_LAYOUT(RpAtomic, geometry, 0x18);
RW_ASSERT_LAYOUT(RpAtomic, renderCallBack, 0x48);
RW_ASSERT_SIZE(RpAtomic, 0x70);
enum { rpATOMICCOLLISIONTEST = 1, rpATOMICRENDER = 4 };

struct RpClump
{
    RwObject object;
    RwLinkList atomicList;
    RwLLLink inWorldLink;
    RpClump* (*callback)(RpClump*, void*);
    RwUInt16 renderFrame, pad;
};
RW_ASSERT_SIZE(RpClump, 0x20);

struct RpLight
{
    RwObjectHasFrame object;
    float radius;
    RwRGBAReal color;
    float minusCosAngle;
    RwLinkList WorldSectorsInLight;
    RwLLLink inWorld;
    RwUInt16 lightFrame, pad;
};
RW_ASSERT_SIZE(RpLight, 0x40);
enum { rpLIGHTDIRECTIONAL = 1, rpLIGHTAMBIENT = 2, rpLIGHTPOINT = 0x80, rpLIGHTSPOT = 0x81, rpLIGHTSPOTSOFT = 0x82 };
enum { rpLIGHTLIGHTATOMICS = 1, rpLIGHTLIGHTWORLD = 2 };


// ---------------------------------------------------------------- worlds
struct RpPolygon { RwUInt16 matIndex; RwUInt16 vertIndex[3]; };
struct RpVertexNormal { int8_t x, y, z; RwUInt8 pad; };
struct RpSector { RwInt32 type; };
struct RpPlaneSector
{
    RwInt32 type;
    float value;
    RpSector* leftSubTree;
    RpSector* rightSubTree;
    float leftValue, rightValue;
};
struct RpWorldSector
{
    RwInt32 type;
    RpPolygon* polygons;
    RwV3d* vertices;
    RpVertexNormal* normals;
    RwTexCoords* texCoords[8];
    RwRGBA* preLitLum;
    RwResEntry* repEntry;
    RwLinkList collAtomicsInWorldSector;
    RwLinkList noCollAtomicsInWorldSector;
    RwLinkList lightsInWorldSector;
    RwBBox boundingBox;
    RwBBox tightBoundingBox;
    void* colSectorRoot;
    RpMeshHeader* mesh;
    RxPipeline* pipeline;
    RwUInt16 matListWindowBase, numVertices, numPolygons, pad;
};
RW_ASSERT_LAYOUT(RpWorldSector, preLitLum, 0x30);
RW_ASSERT_LAYOUT(RpWorldSector, mesh, 0x84);
RW_ASSERT_SIZE(RpWorldSector, 0x94);
enum { rpWORLDSECTOR_TYPE = -1 }; // RpSector.type of an atomic (leaf) sector

struct RpWorld
{
    RwObject object;
    RwUInt32 flags;
    RwInt32 renderOrder;
    RpMaterialList matList;
    RpSector* rootSector;
    RwInt32 numTexCoordSets;
    RwInt32 numClumpsInWorld;
    RwLLLink* currentClumpLink;
    RwLinkList clumpList;
    RwLinkList lightList;
    RwLinkList directionalLightList;
    RwV3d worldOrigin;
    RwBBox boundingBox;
    RwSurfaceProperties surfaceProps;
    RpWorldSector* (*renderCallBack)(RpWorldSector*);
    RxPipeline* pipeline;
};
RW_ASSERT_LAYOUT(RpWorld, worldOrigin, 0x44);
RW_ASSERT_SIZE(RpWorld, 0x7C);

// ---------------------------------------------------------------- plugins
struct RpHAnimNodeInfo { RwInt32 nodeID, nodeIndex, flags; RwFrame* pFrame; };
struct RpHAnimHierarchy
{
    RwInt32 flags, numNodes;
    void* pCurrentAnim;
    float currentTime;
    void* pNextFrame;
    void* pAnimCallBack;
    void* pAnimCallBackData;
    float animCallBackTime;
    void* pAnimLoopCallBack;
    void* pAnimLoopCallBackData;
    RwMatrix* pMatrixArray;
    void* pMatrixArrayUnaligned;
    RpHAnimNodeInfo* pNodeInfo;
    RwFrame* parentFrame;
    RwInt32 maxKeyFrameSize, currentKeyFrameSize;
    void* keyFrameToMatrixCB;
    void* keyFrameBlendCB;
    void* keyFrameInterpolateCB;
    void* keyFrameAddCB;
    RpHAnimHierarchy* parentHierarchy;
    RwInt32 offsetInParent, rootParentOffset;
};
RW_ASSERT_LAYOUT(RpHAnimHierarchy, pMatrixArray, 0x28);
RW_ASSERT_SIZE(RpHAnimHierarchy, 0x5C);

struct RpUserDataArray { char* name; RwInt32 format; RwInt32 numElements; void* data; };
enum { rpNAUSERDATAFORMAT = 0, rpINTUSERDATA = 1, rpREALUSERDATA = 2, rpSTRINGUSERDATA = 3 };

struct RtQuat { RwV3d imag; float real; };
struct RtQuatSlerpCache { RtQuat raFrom, raTo; float omega; RwInt32 nearlyZeroOm; };

struct RpIntersection
{
    union { RwLine line; RwV3d point; RwSphere sphere; RwBBox box; void* object; } t;
    RwInt32 type;
};
enum { rpINTERSECTNONE = 0, rpINTERSECTLINE = 1, rpINTERSECTPOINT = 2, rpINTERSECTSPHERE = 3,
       rpINTERSECTBOX = 4, rpINTERSECTATOMIC = 5 };
struct RpCollisionTriangle { RwV3d normal; RwV3d point; RwInt32 index; RwV3d* vertices[3]; };

struct RxRenderStateVector
{
    RwUInt32 Flags;
    RwInt32 ShadeMode, SrcBlend, DestBlend;
    RwRaster* TextureRaster;
    RwInt32 AddressModeU, AddressModeV, FilterMode;
    RwRGBA BorderColor;
    RwInt32 FogType;
    RwRGBA FogColor;
    RwUInt8* FogTable;
};

// ---------------------------------------------------------------- immediate mode (GameCube vertex layouts)
struct RwIm2DVertex
{
    float x, y, z;
    RwRGBA emissiveColor;
    float u, v;
    float recipZ;
};
RW_ASSERT_SIZE(RwIm2DVertex, 0x1C);
struct RwIm3DVertex
{
    RwV3d objVertex;
    RwV3d objNormal;
    RwRGBA color;
    float u, v;
};
RW_ASSERT_SIZE(RwIm3DVertex, 0x24);
enum { rwPRIMTYPENAPRIMTYPE = 0, rwPRIMTYPELINELIST = 1, rwPRIMTYPEPOLYLINE = 2, rwPRIMTYPETRILIST = 3,
       rwPRIMTYPETRISTRIP = 4, rwPRIMTYPETRIFAN = 5, rwPRIMTYPEPOINTLIST = 6 };

// Render states
enum {
    rwRENDERSTATETEXTURERASTER = 1, rwRENDERSTATETEXTUREADDRESS = 2, rwRENDERSTATETEXTUREADDRESSU = 3,
    rwRENDERSTATETEXTUREADDRESSV = 4, rwRENDERSTATETEXTUREPERSPECTIVE = 5, rwRENDERSTATEZTESTENABLE = 6,
    rwRENDERSTATESHADEMODE = 7, rwRENDERSTATEZWRITEENABLE = 8, rwRENDERSTATETEXTUREFILTER = 9,
    rwRENDERSTATESRCBLEND = 10, rwRENDERSTATEDESTBLEND = 11, rwRENDERSTATEVERTEXALPHAENABLE = 12,
    rwRENDERSTATEBORDERCOLOR = 13, rwRENDERSTATEFOGENABLE = 14, rwRENDERSTATEFOGCOLOR = 15,
    rwRENDERSTATEFOGTYPE = 16, rwRENDERSTATEFOGDENSITY = 17, rwRENDERSTATECULLMODE = 20,
    rwRENDERSTATESTENCILENABLE = 21, rwRENDERSTATEALPHATESTFUNCTION = 29,
    rwRENDERSTATEALPHATESTFUNCTIONREF = 30,
};

// ---------------------------------------------------------------- engine globals
typedef void* (*RwFreeListAllocFn)(void* freelist);
typedef void* (*RwFreeListFreeFn)(void* freelist, void* p);

struct RwFileFunctions
{
    RwInt32 (*rwfexist)(const char*);
    void* (*rwfopen)(const char*, const char*);
    RwInt32 (*rwfclose)(void*);
    RwUInt32 (*rwfread)(void*, RwUInt32, RwUInt32, void*);
    RwUInt32 (*rwfwrite)(const void*, RwUInt32, RwUInt32, void*);
    char* (*rwfgets)(char*, RwInt32, void*);
    RwInt32 (*rwfputs)(const char*, void*);
    RwInt32 (*rwfeof)(void*);
    RwInt32 (*rwfseek)(void*, long, int);
    RwInt32 (*rwfflush)(void*);
    RwInt32 (*rwftell)(void*);
};
RW_ASSERT_SIZE(RwFileFunctions, 0x2C);

struct RwStringFunctions
{
    void* vecSprintf;
    void* vecVsprintf;
    char* (*vecStrcpy)(char*, const char*);
    char* (*vecStrncpy)(char*, const char*, size_t);
    char* (*vecStrcat)(char*, const char*);
    char* (*vecStrncat)(char*, const char*, size_t);
    char* (*vecStrrchr)(const char*, int);
    char* (*vecStrchr)(const char*, int);
    char* (*vecStrstr)(const char*, const char*);
    int (*vecStrcmp)(const char*, const char*);
    int (*vecStricmp)(const char*, const char*);
    size_t (*vecStrlen)(const char*);
    char* (*vecStrUpr)(char*);
    char* (*vecStrLwr)(char*);
    char* (*vecStrtok)(char*, const char*);
    void* vecSscanf;
};
RW_ASSERT_SIZE(RwStringFunctions, 0x40);

struct RwMemoryFunctions
{
    void* (*rwmalloc)(size_t);
    void (*rwfree)(void*);
    void* (*rwrealloc)(void*, size_t);
    void* (*rwcalloc)(size_t, size_t);
};

struct RwGlobals
{
    RwCamera* curCamera;
    void* curWorld;
    RwUInt16 renderFrame, lightFrame;
    RwUInt16 pad[2];
    RwUInt8 dOpenDevice[0x38];
    void* stdFunc[29];
    RwLinkList dirtyFrameList;
    RwFileFunctions fileFuncs;
    RwStringFunctions stringFuncs;
    RwMemoryFunctions memoryFuncs;
    RwFreeListAllocFn memoryAlloc;
    RwFreeListFreeFn memoryFree;
    void* metrics;
    RwInt32 engineStatus;
    RwUInt32 resArenaInitSize;
};
RW_ASSERT_LAYOUT(RwGlobals, dirtyFrameList, 0xBC);
RW_ASSERT_LAYOUT(RwGlobals, fileFuncs, 0xC4);
RW_ASSERT_LAYOUT(RwGlobals, stringFuncs, 0xF0);
RW_ASSERT_LAYOUT(RwGlobals, memoryAlloc, 0x140);
RW_ASSERT_LAYOUT(RwGlobals, memoryFree, 0x144);

struct RwModuleInfo { RwInt32 globalsOffset; RwInt32 numInstances; };

// The engine instance: RwGlobals followed by module/plugin globals.
extern RwUInt8* RwEngineInstance;

} // extern "C"

// ---------------------------------------------------------------- internal helpers (C++)
inline void rwLinkListInitialize(RwLinkList* l) { l->link.next = l->link.prev = &l->link; }
inline bool rwLinkListEmpty(const RwLinkList* l) { return l->link.next == &l->link; }
inline void rwLinkListAddLLLink(RwLinkList* l, RwLLLink* k)
{
    k->next = l->link.next;
    k->prev = &l->link;
    l->link.next->prev = k;
    l->link.next = k;
}
inline void rwLinkListRemoveLLLink(RwLLLink* k)
{
    k->prev->next = k->next;
    k->next->prev = k->prev;
}
#define rwLLLinkGetData(link, type, entry) ((type*)((RwUInt8*)(link) - offsetof(type, entry)))

inline RwGlobals* rwGlobals() { return (RwGlobals*)RwEngineInstance; }

void* rwMalloc(size_t size);
void* rwCalloc(size_t n, size_t size);
void rwFree(void* p);

#endif // PC_RWCORE_H

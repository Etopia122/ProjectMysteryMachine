/*
 * GameCube native geometry streaming: the instanced vertex arrays and display
 * lists hanging off a resource entry are written with their pointers made
 * relative to the data block, and fixed up again after reading.
 */

#include "rwsdk/rpworld_int.h"

#define rwPLUGIN_ID 2 /* world */

#define E_RW_BADVERSION 0x80000004

#define rpGEOMETRYNATIVE 0x01000000

#define rxGCNATIVEPLATFORM 6

#define rxGCINSTFLAG_POSINDIRECT 0x02
#define rxGCINSTFLAG_CLRINDIRECT 0x08
#define rxGCINSTFLAG_NRMINDIRECT 0x10
#define rxGCINSTFLAG_TEXINDIRECT 0x84

typedef struct rxGCResEntry {
    /* 0x00 */ void* next;
    /* 0x04 */ void* prev;
    /* 0x08 */ RwInt32 size;
    /* 0x0C */ void* owner;
    /* 0x10 */ struct rxGCResEntry** ownerRef;
    /* 0x14 */ void (*destroyNotify)(struct rxGCResEntry* resEntry);
} rxGCResEntry;

typedef struct rxGCInstanceData {
    /* 0x00 */ RwUInt32 pad00;
    /* 0x04 */ RwUInt16 serialNum;
    /* 0x06 */ RwUInt16 pad06;
    /* 0x08 */ RwUInt8 pad08[0x14 - 0x08];
    /* 0x14 */ RwUInt32 flags;
    /* 0x18 */ RwUInt8* positions;
    /* 0x1C */ RwUInt8* normals;
    /* 0x20 */ RwUInt8* colors;
    /* 0x24 */ RwUInt8* texCoords[8];
} rxGCInstanceData;

typedef struct rxGCMeshData {
    /* 0x00 */ RwUInt32 size;
    /* 0x04 */ RwUInt8* displayList;
} rxGCMeshData;

typedef struct rxGCMeshHeader {
    /* 0x00 */ RwUInt32 flags;
    /* 0x04 */ RwUInt16 numMeshes;
} rxGCMeshHeader;

#define ALIGN32(p) ((RwUInt8*)(((RwUInt32)(p) + 31) & ~31))

#define GCRESENTRYDATA(resEntry) ((rxGCInstanceData*)((RwUInt8*)(resEntry) + sizeof(rxGCResEntry)))
#define GCMESHES(inst) ((rxGCMeshData*)((RwUInt8*)(inst) + sizeof(rxGCInstanceData)))

RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length);
RwStream* RwStreamWrite(RwStream* stream, const void* buffer, RwUInt32 length);
RpWorld* RpWorldSectorGetWorld(const RpWorldSector* worldSector);
RwInt32 RpWorldSectorGetNumVertices(const RpWorldSector* worldSector);
void* _rpGameCubeVtxFmtGetDefault(void);
void* RpGameCubeGeometryGetVtxFmt(const RpGeometry* geometry);
void* RpGameCubeWorldGetVtxFmt(const RpWorld* world);
void _rxGCResEntryWaitDone(rxGCResEntry* resEntry);
void DCFlushRange(void* addr, RwUInt32 nBytes);
void GXInvalidateVtxCache(void);

extern RwUInt16 lbl_8025E800;

static RwInt32 DlNativePlatform = rxGCNATIVEPLATFORM;

void* DlNativeDataStreamRead(RwStream* stream, void* owner, rxGCResEntry** resEntryRef, RwUInt32 numMeshes,
                             RwInt32 numVertices)
{
    RwUInt32 count;
    RwUInt8* data;
    rxGCInstanceData* inst;
    rxGCMeshData* meshes;
    RwInt32 i;
    RwUInt32 version;
    RwUInt32 length;
    RwUInt32 headerSize;
    RwUInt32 dataSize;
    RwInt32 platform;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &length, &version)) {
        return NULL;
    }

    if (version < rwLIBRARYBASEVERSION || version > rwLIBRARYCURRENTVERSION) {
        RWERROR((E_RW_BADVERSION));
        return NULL;
    }

    if (!RwStreamReadInt32(stream, &platform, sizeof(platform))) {
        return NULL;
    }
    if (platform != rxGCNATIVEPLATFORM) {
        return NULL;
    }
    if (!RwStreamReadInt32(stream, (RwInt32*)&headerSize, sizeof(headerSize))) {
        return NULL;
    }
    if (!RwStreamReadInt32(stream, (RwInt32*)&dataSize, sizeof(dataSize))) {
        return NULL;
    }

    *resEntryRef = (rxGCResEntry*)RwMalloc((RwUInt32)ALIGN32(sizeof(rxGCResEntry) + headerSize) + 32 + dataSize);

    inst = GCRESENTRYDATA(*resEntryRef);
    if (RwStreamRead(stream, inst, headerSize) != headerSize) {
        return NULL;
    }

    data = ALIGN32((RwUInt8*)GCMESHES(inst) + numMeshes * sizeof(rxGCMeshData));
    if (RwStreamRead(stream, data, dataSize) != dataSize) {
        return NULL;
    }

    (*resEntryRef)->next = NULL;
    (*resEntryRef)->prev = NULL;
    (*resEntryRef)->owner = owner;
    (*resEntryRef)->size = length;
    (*resEntryRef)->ownerRef = resEntryRef;
    (*resEntryRef)->destroyNotify = _rxGCResEntryWaitDone;

    inst->serialNum = lbl_8025E800;

    if (inst->positions && !(inst->flags & rxGCINSTFLAG_POSINDIRECT)) {
        inst->positions = data + (RwUInt32)inst->positions;
    }
    if (inst->normals && !(inst->flags & rxGCINSTFLAG_NRMINDIRECT)) {
        inst->normals = data + (RwUInt32)inst->normals;
    }
    if (inst->colors) {
        inst->colors = data + (RwUInt32)inst->colors;
    }
    for (i = 0; inst->texCoords[i] && i < 8 && !(inst->flags & rxGCINSTFLAG_TEXINDIRECT); i++) {
        inst->texCoords[i] = data + (RwUInt32)inst->texCoords[i];
    }
    for (meshes = GCMESHES(inst), count = numMeshes; count; count--) {
        meshes->displayList = data + (RwUInt32)meshes->displayList;
        meshes++;
    }

    DCFlushRange(GCRESENTRYDATA(*resEntryRef), (*resEntryRef)->size);
    GXInvalidateVtxCache();
    return owner;
}

static inline RwInt32 DlNativeDataSize(rxGCResEntry* resEntry, RwUInt32 numMeshes)
{
    RwInt32 size = sizeof(rxGCInstanceData) + numMeshes * sizeof(rxGCMeshData) + rwCHUNKHEADERSIZE + 3 * sizeof(RwInt32);
    RwUInt8* data = ALIGN32((rxGCMeshData*)((RwUInt8*)resEntry + sizeof(rxGCResEntry) + sizeof(rxGCInstanceData)) + numMeshes);

    size += resEntry->size - (data - (RwUInt8*)GCRESENTRYDATA(resEntry));
    return size;
}

RwStream* DlNativeDataStreamWrite(RwStream* stream, rxGCResEntry* resEntry, void* vtxFmt, RwUInt32 numMeshes,
                                  RwInt32 numVertices)
{
    rxGCInstanceData* inst = GCRESENTRYDATA(resEntry);
    rxGCMeshData* meshes = GCMESHES(inst);
    RwUInt32 headerSize = sizeof(rxGCInstanceData) + numMeshes * sizeof(rxGCMeshData);
    RwInt32 size = DlNativeDataSize(resEntry, numMeshes) - rwCHUNKHEADERSIZE;
    RwUInt8* data = ALIGN32((RwUInt8*)meshes + numMeshes * sizeof(rxGCMeshData));
    RwUInt32 dataSize;
    rxGCMeshData* mesh = meshes;
    RwUInt32 count = numMeshes;
    RwInt32 i;

    if (vtxFmt == NULL) {
        vtxFmt = _rpGameCubeVtxFmtGetDefault();
    }

    if (inst->positions && !(inst->flags & rxGCINSTFLAG_POSINDIRECT)) {
        inst->positions = (RwUInt8*)(inst->positions - data);
    }
    if (inst->normals && !(inst->flags & rxGCINSTFLAG_NRMINDIRECT)) {
        inst->normals = (RwUInt8*)(inst->normals - data);
    }
    if (inst->colors && !(inst->flags & rxGCINSTFLAG_CLRINDIRECT)) {
        inst->colors = (RwUInt8*)(inst->colors - data);
    }
    for (i = 0; inst->texCoords[i] && i < 8 && !(inst->flags & rxGCINSTFLAG_TEXINDIRECT); i++) {
        inst->texCoords[i] = (RwUInt8*)(inst->texCoords[i] - data);
    }
    for (; count; count--) {
        mesh->displayList = (RwUInt8*)(mesh->displayList - data);
        mesh++;
    }

    if (!_rwStreamWriteVersionedChunkHeader(stream, rwID_STRUCT, size, rwLIBRARYCURRENTVERSION, rwLIBRARYBUILD)) {
        return NULL;
    }
    if (!RwStreamWriteInt32(stream, &DlNativePlatform, sizeof(DlNativePlatform))) {
        return NULL;
    }
    if (!RwStreamWriteInt32(stream, (RwInt32*)&headerSize, sizeof(headerSize))) {
        return NULL;
    }
    dataSize = resEntry->size - (data - (RwUInt8*)GCRESENTRYDATA(resEntry));
    if (!RwStreamWriteInt32(stream, (RwInt32*)&dataSize, sizeof(dataSize))) {
        return NULL;
    }
    if (!RwStreamWrite(stream, inst, headerSize)) {
        return NULL;
    }
    if (!RwStreamWrite(stream, data, dataSize)) {
        return NULL;
    }

    if (inst->positions && !(inst->flags & rxGCINSTFLAG_POSINDIRECT)) {
        inst->positions = data + (RwUInt32)inst->positions;
    }
    if (inst->normals && !(inst->flags & rxGCINSTFLAG_NRMINDIRECT)) {
        inst->normals = data + (RwUInt32)inst->normals;
    }
    if (inst->colors) {
        inst->colors = data + (RwUInt32)inst->colors;
    }
    for (i = 0; inst->texCoords[i] && i < 8 && !(inst->flags & rxGCINSTFLAG_TEXINDIRECT); i++) {
        inst->texCoords[i] = data + (RwUInt32)inst->texCoords[i];
    }
    mesh = meshes;
    for (count = numMeshes; count; count--) {
        mesh->displayList = data + (RwUInt32)mesh->displayList;
        mesh++;
    }

    DCFlushRange(GCRESENTRYDATA(resEntry), resEntry->size);
    GXInvalidateVtxCache();
    return stream;
}


RwInt32 _rpGeometryNativeSize(const RpGeometry* geometry)
{
    if ((geometry->flags & rpGEOMETRYNATIVE) && geometry->repEntry) {
        return DlNativeDataSize((rxGCResEntry*)geometry->repEntry, ((rxGCMeshHeader*)geometry->mesh)->numMeshes);
    }
    return 0;
}

RwStream* _rpGeometryNativeWrite(RwStream* stream, const RpGeometry* geometry)
{
    if (geometry->flags & rpGEOMETRYNATIVE) {
        DlNativeDataStreamWrite(stream, (rxGCResEntry*)geometry->repEntry, RpGameCubeGeometryGetVtxFmt(geometry),
                                ((rxGCMeshHeader*)geometry->mesh)->numMeshes, geometry->numVertices);
    }
    return stream;
}

void* _rpGeometryNativeRead(RwStream* stream, RpGeometry* geometry)
{
    return DlNativeDataStreamRead(stream, geometry, (rxGCResEntry**)&geometry->repEntry,
                                  ((rxGCMeshHeader*)geometry->mesh)->numMeshes, geometry->numVertices);
}

RwInt32 _rpWorldSectorNativeSize(const RpWorldSector* sector)
{
    const RpWorld* world = RpWorldSectorGetWorld(sector);

    if ((world->flags & rpGEOMETRYNATIVE) && sector->repEntry) {
        return DlNativeDataSize((rxGCResEntry*)sector->repEntry, ((rxGCMeshHeader*)sector->mesh)->numMeshes);
    }
    return 0;
}

RwStream* _rpWorldSectorNativeWrite(RwStream* stream, const RpWorldSector* sector)
{
    const RpWorld* world = RpWorldSectorGetWorld(sector);

    if (world->flags & rpGEOMETRYNATIVE) {
        DlNativeDataStreamWrite(stream, (rxGCResEntry*)sector->repEntry, RpGameCubeWorldGetVtxFmt(world),
                                ((rxGCMeshHeader*)sector->mesh)->numMeshes, RpWorldSectorGetNumVertices(sector));
    }
    return stream;
}

void* _rpWorldSectorNativeRead(RwStream* stream, RpWorldSector* sector)
{
    return DlNativeDataStreamRead(stream, sector, (rxGCResEntry**)&sector->repEntry,
                                  ((rxGCMeshHeader*)sector->mesh)->numMeshes, RpWorldSectorGetNumVertices(sector));
}

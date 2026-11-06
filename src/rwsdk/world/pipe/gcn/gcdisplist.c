/*
 * GameCube all-in-one helpers: building each mesh's indexed display list
 * and loading RenderWare lights into GX hardware light objects.
 */

#include "rwsdk/gcallinone_int.h"

#define rpMESHHEADERTRISTRIP 0x0001

#define rpLIGHTDIRECTIONAL 1
#define rpLIGHTAMBIENT 2
#define rpLIGHTPOINT 0x80
#define rpLIGHTSPOT 0x81
#define rpLIGHTSPOTSOFT 0x82

#define GX_INDEX8 2
#define GX_INDEX16 3

#define GX_DA_MEDIUM 2
#define GX_SP_COS 1
#define GX_SP_COS2 2

typedef struct GXLightObj {
    RwUInt32 dummy[16];
} GXLightObj;

extern RwMatrix lbl_8025E304;

RwMatrix* RwFrameGetLTM(RwFrame* frame);
RwV3d* RwV3dTransformPoints(RwV3d* pointsOut, const RwV3d* pointsIn, RwInt32 numPoints, const RwMatrix* matrix);
RwV3d* RwV3dTransformVectors(RwV3d* vectorsOut, const RwV3d* vectorsIn, RwInt32 numPoints, const RwMatrix* matrix);
RwReal RpLightGetConeAngle(const RpLight* light);

void DCInvalidateRange(void* addr, RwUInt32 nBytes);
void GXBeginDisplayList(void* list, RwUInt32 size);
RwUInt32 GXEndDisplayList(void);
void GXResetWriteGatherPipe(void);
void GXInitLightPos(GXLightObj* lt, RwReal x, RwReal y, RwReal z);
void GXInitLightDir(GXLightObj* lt, RwReal nx, RwReal ny, RwReal nz);
void GXInitLightColor(GXLightObj* lt, GXColor color);
void GXInitLightDistAttn(GXLightObj* lt, RwReal refDistance, RwReal refBrightness, RwInt32 distFunc);
void GXInitLightAttnA(GXLightObj* lt, RwReal a0, RwReal a1, RwReal a2);
void GXInitLightSpot(GXLightObj* lt, RwReal cutoff, RwInt32 spotFunc);
void GXLoadLightObjImm(GXLightObj* lt, RwUInt32 light);

static inline void GXPosition1x8(const RwUInt8 index)
{
    GXWGFifo.u8 = index;
}

static inline void GXPosition1x16(const RwUInt16 index)
{
    GXWGFifo.u16 = index;
}

static GXLightObj lightObjs[8];

/* The GX light id of each hardware light slot. */
static RwUInt32 lightMasks[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

/* Bytes in a mesh's display list: the GXBegin header, then an index per
 * vertex attribute per index, padded so an 8 bit list never fills a whole
 * number of 32 byte blocks. */
static inline RwUInt32 GCMeshDisplayListSize(RwUInt32 numIndices, RwInt32 numTexCoords, RwInt32 indexSize,
                                             RwUInt32 normals, RwUInt32 prelit, RwUInt32 textured)
{
    RwUInt32 count = numIndices;
    RwUInt32 size;

    if (normals) {
        count += numIndices;
    }
    if (prelit) {
        count += numIndices;
    }
    if (textured) {
        count += numIndices * numTexCoords;
    }

    size = indexSize * count + 3;
    if (indexSize == 1 && ((size + 31) & ~31) == size) {
        size += 32;
    }

    return (size + 31) & ~31;
}

RwUInt32 _rxGCDisplayListsSize(RpMeshHeader* meshHeader, RwInt32 numTexCoords, RwInt32 indexSize, RwUInt32 flags)
{
    RwUInt32 numMeshes;
    RwUInt32 normals;
    RwUInt32 prelit;
    RwUInt32 textured;
    RpMesh* mesh;
    RwUInt32 total;

    numMeshes = meshHeader->numMeshes;
    mesh = (RpMesh*)(meshHeader + 1);
    normals = flags & rxGEOMETRY_NORMALS;
    prelit = flags & rxGEOMETRY_PRELIT;
    textured = flags & (rxGEOMETRY_TEXTURED | rxGEOMETRY_TEXTURED2);
    total = 0;

    while (numMeshes--) {
        total += GCMeshDisplayListSize(mesh->numIndices, numTexCoords, indexSize, normals, prelit, textured);
        mesh++;
    }

    return total;
}

void _rxGCBuildDisplayLists(RxGameCubeDisplayList* displayLists, RwUInt8* dst, RpMeshHeader* meshHeader, RwInt32 numTexCoords,
                 RwInt32 indexSize, RwUInt32 flags)
{
    GXPrimitive prim;
    RwUInt32 numAttribs;
    RwInt32 i;
    RwInt32 descType;
    RpMesh* mesh;
    RwUInt32 normals;
    RwUInt32 prelit;
    RwUInt32 textured;
    RwUInt32 numMeshes;
    RxGameCubeDisplayList* dl;

    prim = GX_TRIANGLES;
    if (meshHeader->flags & rpMESHHEADERTRISTRIP) {
        prim = GX_TRIANGLESTRIP;
    }
    descType = GX_INDEX16;
    if (indexSize == 1) {
        descType = GX_INDEX8;
    }

    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, descType);
    numAttribs = 1;
    normals = flags & rxGEOMETRY_NORMALS;
    if (normals) {
        GXSetVtxDesc(GX_VA_NRM, descType);
        numAttribs = 2;
    }
    prelit = flags & rxGEOMETRY_PRELIT;
    if (prelit) {
        GXSetVtxDesc(GX_VA_CLR0, descType);
        numAttribs++;
    }
    for (i = 0; i < numTexCoords; i++) {
        GXSetVtxDesc(GX_VA_TEX0 + i, descType);
        numAttribs++;
    }
    textured = flags & (rxGEOMETRY_TEXTURED | rxGEOMETRY_TEXTURED2);
    numAttribs = (RwUInt8)numAttribs;

    numMeshes = meshHeader->numMeshes;
    dl = displayLists;
    mesh = (RpMesh*)(meshHeader + 1);
    while (numMeshes--) {
        RwUInt16* indices = mesh->indices;
        RwUInt32 numIndices = mesh->numIndices;
        RwUInt32 size =
            GCMeshDisplayListSize(numIndices, numTexCoords, indexSize, normals, prelit, textured) + 32;

        DCInvalidateRange(dst, size);
        GXBeginDisplayList(dst, size);
        GXResetWriteGatherPipe();
        GXBegin(prim, GX_VTXFMT0, numIndices);

        if (indexSize == 1) {
            while (numIndices--) {
                RwUInt8 index = *indices;
                RwInt32 j;

                for (j = 0; j < (RwInt32)numAttribs; j++) {
                    GXPosition1x8(index);
                }
                indices++;
            }
        } else {
            while (numIndices--) {
                RwInt16 index = *indices;
                RwInt32 j;

                for (j = 0; j < (RwInt32)numAttribs; j++) {
                    GXPosition1x16(index);
                }
                indices++;
            }
        }

        size = GXEndDisplayList();
        dl->list = dst;
        dst += size;
        dl->size = size;
        dl++;
        mesh++;
    }
}

void _rxGCSetupGlobalLights(RwUInt8 lightFlags, RxGameCubeRenderData* data)
{
    RpWorld* world;
    GXColor color;
    RwV3d dir;
    RwInt32 n;
    RwLLLink* cur;
    GXLightObj* obj;
    RwLLLink* end;
    RpLight* light;

    world = (RpWorld*)RWSRCGLOBAL(curWorld);
    end = rwLinkListGetTerminator(&world->directionalLightList);
    cur = rwLinkListGetFirstLLLink(&world->directionalLightList);
    while (cur != end) {
        light = rwLLLinkGetData(cur, RpLight, inWorld);
        if (light && (light->object.object.flags & lightFlags)) {
            if ((RwInt32)light->object.object.subType == rpLIGHTDIRECTIONAL) {
                n = data->numLights;
                RwV3dTransformVectors(&dir, &RwFrameGetLTM((RwFrame*)light->object.object.parent)->at, 1,
                                      &lbl_8025E304);
                obj = &lightObjs[n];
                GXInitLightPos(obj, -dir.x * -1048576.0f, dir.y * -1048576.0f, -dir.z * -1048576.0f);
                color.r = light->color[0] * 255.0f;
                color.g = light->color[1] * 255.0f;
                color.b = light->color[2] * 255.0f;
                color.a = 0;
                GXInitLightColor(obj, color);
                GXLoadLightObjImm(obj, lightMasks[n]);
                data->lightMask0 |= lightMasks[data->numLights];
                data->numLights++;
            } else {
                data->ambient[0] += light->color[0];
                data->ambient[1] += light->color[1];
                data->ambient[2] += light->color[2];
                data->ambientLight = TRUE;
            }
        }
        cur = rwLLLinkGetNext(cur);
    }
}

void _rxGCSetupLocalLight(RpLight* light, RxGameCubeRenderData* data)
{
    GXColor color;
    RwMatrix* ltm;
    RwV3d dir;
    RwV3d pos;

    color.r = light->color[0] * 255.0f;
    color.g = light->color[1] * 255.0f;
    color.b = light->color[2] * 255.0f;
    color.a = 0;
    GXInitLightColor(&lightObjs[data->numLights], color);

    ltm = RwFrameGetLTM((RwFrame*)light->object.object.parent);
    RwV3dTransformPoints(&pos, &ltm->pos, 1, &lbl_8025E304);
    GXInitLightPos(&lightObjs[data->numLights], -pos.x, pos.y, -pos.z);
    GXInitLightDistAttn(&lightObjs[data->numLights], light->radius * 0.5f, 0.5f, GX_DA_MEDIUM);

    switch (light->object.object.subType) {
    case rpLIGHTDIRECTIONAL:
    case rpLIGHTAMBIENT:
        break;
    case rpLIGHTPOINT:
        GXInitLightAttnA(&lightObjs[data->numLights], 1.0f, 0.0f, 0.0f);
        break;
    case rpLIGHTSPOT:
        RwV3dTransformVectors(&dir, &ltm->at, 1, &lbl_8025E304);
        GXInitLightDir(&lightObjs[data->numLights], -dir.x, dir.y, -dir.z);
        GXInitLightSpot(&lightObjs[data->numLights], RpLightGetConeAngle(light) * 57.29578f, GX_SP_COS);
        break;
    case rpLIGHTSPOTSOFT:
        RwV3dTransformVectors(&dir, &ltm->at, 1, &lbl_8025E304);
        GXInitLightDir(&lightObjs[data->numLights], -dir.x, dir.y, -dir.z);
        GXInitLightSpot(&lightObjs[data->numLights], RpLightGetConeAngle(light) * 57.29578f, GX_SP_COS2);
        break;
    }

    GXLoadLightObjImm(&lightObjs[data->numLights], lightMasks[data->numLights]);
    data->lightMask1 |= lightMasks[data->numLights];
    data->numLights++;
}

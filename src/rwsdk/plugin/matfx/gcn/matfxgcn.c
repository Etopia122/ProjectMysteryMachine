/*
 * GameCube material effects pipelines: the all-in-one atomic and world sector
 * pipelines with MatFX instance and render callbacks. Environment maps are
 * drawn as a second, additive pass with a normal based texture matrix, dual
 * textures as a second blended pass over the second texture coordinate set,
 * and bump maps by offsetting the texture coordinates along the light
 * direction projected into each vertex's tangent space.
 */

#include "rwsdk/gcallinone_int.h"

#define rwID_MATERIALEFFECTSPLUGIN 0x120

#define rpMATFXEFFECTNULL 0
#define rpMATFXEFFECTBUMPMAP 1
#define rpMATFXEFFECTENVMAP 2
#define rpMATFXEFFECTBUMPENVMAP 3
#define rpMATFXEFFECTDUAL 4

#define rpMAXPASS 2

#define rpGEOMETRYTRISTRIP 0x01
#define rpGEOMETRYNORMALS 0x10

#define rpGEOMETRYLOCKPOLYGONS 0x01
#define rpGEOMETRYLOCKVERTICES 0x02
#define rpGEOMETRYLOCKNORMALS 0x04
#define rpGEOMETRYLOCKTEXCOORDS1 0x10

#define rpINTERPOLATORDIRTYINSTANCE 0x01

#define rwRENDERSTATESRCBLEND 10
#define rwRENDERSTATEDESTBLEND 11

#define rwBLENDZERO 1
#define rwBLENDONE 2
#define rwBLENDSRCALPHA 5
#define rwBLENDINVSRCALPHA 6

#define GX_TG_NRM 1
#define GX_TEXMTX0 30
#define GX_MTX2x4 1

struct RwResEntry {
    /* 0x00 */ RwLLLink link;
    /* 0x08 */ RwInt32 size;
    /* 0x0C */ void* owner;
    /* 0x10 */ RwResEntry** ownerRef;
    /* 0x14 */ void (*destroyNotify)(RwResEntry* resEntry);
};

typedef struct MatFXBumpMapData {
    /* 0x00 */ RwFrame* frame;
    /* 0x04 */ RwTexture* bumpedTexture;
    /* 0x08 */ RwTexture* bumpTexture;
    /* 0x0C */ RwReal coef;
    /* 0x10 */ RwReal invBumpWidth;
} MatFXBumpMapData;

typedef struct MatFXEnvMapData {
    /* 0x00 */ RwFrame* frame;
    /* 0x04 */ RwTexture* texture;
    /* 0x08 */ RwReal coef;
    /* 0x0C */ RwInt32 useFrameBufferAlpha;
} MatFXEnvMapData;

typedef struct MatFXDualData {
    /* 0x00 */ RwTexture* texture;
    /* 0x04 */ RwInt32 srcBlendMode;
    /* 0x08 */ RwInt32 dstBlendMode;
} MatFXDualData;

typedef union MatFXEffectUnion {
    MatFXBumpMapData bumpMap;
    MatFXEnvMapData envMap;
    MatFXDualData dual;
} MatFXEffectUnion;

typedef struct MatFXEffectData {
    /* 0x00 */ MatFXEffectUnion data;
    /* 0x14 */ RwInt32 flag;
} MatFXEffectData;

typedef struct rpMatFXMaterialData {
    /* 0x00 */ MatFXEffectData data[rpMAXPASS];
    /* 0x30 */ RwInt32 flags;
} rpMatFXMaterialData;

extern RwInt32 MatFXMaterialDataOffset;
extern RwMatrix lbl_8025E304;

#define MATFXMATERIALGETDATA(material) (*(rpMatFXMaterialData**)((RwUInt8*)(material) + MatFXMaterialDataOffset))

#define ALIGN32(x) (((RwUInt32)(x) + 31) & ~31)

void* memset(void* dst, int c, unsigned long n);
void DCFlushRange(void* addr, RwUInt32 nBytes);
void GXLoadTexMtxImm(RwReal mtx[3][4], RwUInt32 id, RwInt32 type);
RwBool RwRenderStateSet(RwInt32 state, void* value);
RwBool RwRenderStateGet(RwInt32 state, void* value);
RwMatrix* RwMatrixMultiply(RwMatrix* matrixOut, const RwMatrix* matrixIn1, const RwMatrix* matrixIn2);
RwMatrix* RwMatrixInvert(RwMatrix* matrixOut, const RwMatrix* matrixIn);
RwV3d* RwV3dTransformVectors(RwV3d* vectorsOut, const RwV3d* vectorsIn, RwInt32 numPoints, const RwMatrix* matrix);
RwReal RwV3dNormalize(RwV3d* out, const RwV3d* in);
RwReal _rwInvSqrt(RwReal num);
RwResEntry* RwResourcesAllocateResEntry(void* owner, RwResEntry** ownerRef, RwInt32 size,
                                        void (*destroyNotify)(RwResEntry* resEntry));
RpGameCubeVtxFmt* RpGameCubeVtxFmtCreate(void);
void RpGameCubeVtxFmtDestroy(RpGameCubeVtxFmt* fmt);
void RpGameCubeVtxFmtSetNormal(RpGameCubeVtxFmt* fmt, RwUInt8 compType, RwUInt8 frac);
void RpGameCubeGeometrySetVtxFmt(RpGeometry* geometry, RpGameCubeVtxFmt* fmt);
RxNodeDefinition* RxNodeDefinitionGetGameCubeAtomicAllInOne(void);
RxNodeDefinition* RxNodeDefinitionGetGameCubeWorldSectorAllInOne(void);
RxPipelineNode* RxPipelineFindNodeByName(RxPipeline* pipeline, const RwChar* name, RxPipelineNode* start,
                                         RwInt32* nodeIndex);
void _rxGameCubeAllInOneSetInstanceCallBack(RxPipelineNode* node, RxGameCubeAllInOneCallBack callback);
RxGameCubeAllInOneCallBack _rxGameCubeAllInOneGetInstanceCallBack(RxPipelineNode* node);
void _rxGameCubeAllInOneSetReinstanceCallBack(RxPipelineNode* node, RxGameCubeAllInOneCallBack callback);
RxGameCubeAllInOneCallBack _rxGameCubeAllInOneGetReinstanceCallBack(RxPipelineNode* node);
void RxGameCubeAllInOneSetRenderCallBack(RxPipelineNode* node, RxGameCubeAllInOneCallBack callback);
void* RpMaterialGetMultiTexture(const RpMaterial* material, RwUInt32 platform);
void _rpGameCubeMTMeshRenderCallBack(RxGameCubeDisplayList* dl, RpMaterial* material, void* object,
                                     RxGameCubeRenderData* data, RwMatrix* ltm);
RwBool _rpGameCubeMTPipeDataQueryNBTs(RxGameCubeRenderData* data);
void fn_80141D20(RxGameCubeRenderData* data, RpGameCubeVtxFmt* fmt, RwInt32 numVerts);
RwTexture* _rpMatFXTextureMaskCreate(const RwTexture* baseTexture, const RwTexture* effectTexture);

void* _rpGCMatFXAtomicReinstanceCallBack(void* object, RxGameCubeRenderData* data);
void* _rpGCMatFXSectorInstanceCallBack(void* object, RxGameCubeRenderData* data);

static RxGameCubeAllInOneCallBack AtomicInstanceCB;
static RxGameCubeAllInOneCallBack AtomicReinstanceCB;
static RxGameCubeAllInOneCallBack SectorInstanceCB;
static RxPipeline* MatFXWorldSectorPipeline;
static RxPipeline* MatFXAtomicPipeline;
static RpGameCubeVtxFmt* MatFXVtxFmt;

static RwUInt8 BumpMapOwner[0x70];

static GXColor EnvMapColor = { 255, 255, 255, 255 };
static GXColor BumpEnvMapColor = { 255, 255, 255, 255 };

/*
 * Offsets each vertex's texture coordinates by the light direction in its
 * tangent space, writing the result to a separate texture coordinate array.
 */
static void BumpMapCalcTexCoords(RwTexCoords* outCoords, void* object, MatFXBumpMapData* bumpMapData, RpMesh* mesh,
                                 RwV3d* positions, RwV3d* normals, RwTexCoords* texCoords, RwInt32 numVerts,
                                 RwUInt32 flags)
{
    RwUInt16* index;
    RwReal by;
    RwReal tx;
    RwReal bz;
    RwReal ty;
    RwInt32 i1;
    RwReal dv1;
    RwV3d* lightDir;
    RwReal dv2;
    RwBool isStrip;
    RwUInt8* done;
    RwReal dvv;
    RwInt32 i;
    RwReal factor;
    RwFrame* frame;
    RwInt32 minIndex;
    RwV3d e1;
    RwInt32 i2;
    RwTexCoords* t1;
    RwInt32 maxIndex;
    RwV3d e2;
    RwV3d* p2;
    RwTexCoords* t2;
    RwUInt16* indices;
    RwReal du;
    RwReal tz;
    RwV3d* p0;
    RwV3d* n;
    RwReal recip;
    RwReal bx;
    RwMatrix inverse;
    RwTexCoords* t0;
    RwV3d dir;
    RwV3d* p1;

    done = (RwUInt8*)RwMalloc(numVerts);
    memset(done, 0, numVerts);
    if (done) {
        minIndex = 0x7FFFFFFF;
        maxIndex = 0x80000000;
        factor = bumpMapData->coef * bumpMapData->invBumpWidth;

        frame = bumpMapData->frame;
        if (!frame) {
            frame = (RwFrame*)((RwCamera*)RWSRCGLOBAL(curCamera))->object.object.parent;
        }
        lightDir = &RwFrameGetLTM(frame)->at;
        if (((RwObject*)object)->type == rpATOMIC) {
            RwMatrixInvert(&inverse, RwFrameGetLTM((RwFrame*)((RpAtomic*)object)->object.object.parent));
            RwV3dTransformVectors(&dir, lightDir, 1, &inverse);
        } else {
            dir = *lightDir;
        }
        RwV3dNormalize(&dir, &dir);

        indices = mesh->indices;
        isStrip = flags & rpGEOMETRYTRISTRIP;
        for (i = 0, index = indices; i < mesh->numIndices; index++, i++) {
            if (!done[*index]) {
                if (isStrip) {
                    if (i < 2) {
                        if (i % 2) {
                            i1 = i + 2;
                            i2 = i + 1;
                        } else {
                            i1 = i + 1;
                            i2 = i + 2;
                        }
                    } else {
                        if (i % 2) {
                            i1 = i - 1;
                            i2 = i - 2;
                        } else {
                            i1 = i - 2;
                            i2 = i - 1;
                        }
                    }
                    p0 = &positions[*index];
                    n = &normals[*index];
                    t0 = &texCoords[*index];
                    p1 = &positions[indices[i1]];
                    t1 = &texCoords[indices[i1]];
                    p2 = &positions[indices[i2]];
                    t2 = &texCoords[indices[i2]];
                } else {
                    RwInt32 base = (i / 3) * 3;

                    t0 = &texCoords[*index];
                    p0 = &positions[*index];
                    n = &normals[*index];
                    i1 = indices[base + (i + 1) % 3];
                    i2 = indices[base + (i + 2) % 3];
                    p1 = &positions[i1];
                    t1 = &texCoords[i1];
                    p2 = &positions[i2];
                    t2 = &texCoords[i2];
                }

                e1.x = p1->x - p0->x;
                e2.x = p2->x - p0->x;
                e1.y = p1->y - p0->y;
                e2.y = p2->y - p0->y;
                dv1 = t1->v - t0->v;
                dv2 = t2->v - t0->v;
                e1.z = p1->z - p0->z;
                e2.z = p2->z - p0->z;
                tx = e1.x * dv1 - e2.x * dv2;
                ty = e1.y * dv1 - e2.y * dv2;
                tz = e1.z * dv1 - e2.z * dv2;
                recip = _rwInvSqrt(tx * tx + ty * ty + tz * tz);
                tx *= recip;
                ty *= recip;
                tz *= recip;

                bx = ty * n->z - tz * n->y;
                by = tz * n->x - tx * n->z;
                bz = tx * n->y - ty * n->x;

                du = tx * dir.x + ty * dir.y + tz * dir.z;
                dvv = bx * dir.x + by * dir.y + bz * dir.z;

                outCoords[*index].u = t0->u - du * factor;
                outCoords[*index].v = t0->v - dvv * factor;

                minIndex = (*index < minIndex) ? *index : minIndex;
                maxIndex = (*index > maxIndex) ? *index : maxIndex;
                done[*index] = TRUE;
            }
        }

        DCFlushRange(&outCoords[minIndex], (maxIndex - minIndex + 1) * sizeof(RwTexCoords));
        GXInvalidateVtxCache();
    }
    RwFree(done);
}

static void BumpMapResEntryDestroy(RwResEntry* resEntry)
{
    if (*(RwUInt16*)(resEntry + 1) == lbl_8025E800) {
        GXSetDrawSync(lbl_8025E800);
        lbl_8025E800 = (lbl_8025E800 + 1) % 0xE000;
    }
    while (!_rwDlTokenDone(*(RwUInt16*)(resEntry + 1))) {
    }
}

static void* _rpGCMatFXAtomicInstanceCallBack(void* object, RxGameCubeRenderData* data)
{
    if (_rpGameCubeMTPipeDataQueryNBTs(data)) {
        RpGeometry* geometry = ((RpAtomic*)object)->geometry;
        RpGameCubeVtxFmt* fmt = RpGameCubeGeometryGetVtxFmt(geometry);

        if (!fmt) {
            RpGameCubeGeometrySetVtxFmt(geometry, MatFXVtxFmt);
            fmt = MatFXVtxFmt;
        }
        if (!AtomicInstanceCB(object, data)) {
            return NULL;
        }
        fn_80141D20(data, fmt, geometry->numVertices);
        return object;
    }
    return AtomicInstanceCB(object, data);
}

void* _rpGCMatFXAtomicReinstanceCallBack(void* object, RxGameCubeRenderData* data)
{
    RpAtomic* atomic = (RpAtomic*)object;
    RpGeometry* geometry = atomic->geometry;
    RpGameCubeVtxFmt* fmt = RpGameCubeGeometryGetVtxFmt(geometry);
    RwBool updateNBTs = FALSE;

    if (fmt && fmt->nbt && (geometry->flags & rpGEOMETRYNORMALS)) {
        RwUInt32 locked = geometry->lockedSinceLastInst & (rpGEOMETRYLOCKPOLYGONS | rpGEOMETRYLOCKVERTICES | rpGEOMETRYLOCKNORMALS | rpGEOMETRYLOCKTEXCOORDS1);
        RwUInt32 morphing = FALSE;

        if (geometry->numMorphTargets != 1 && (atomic->interpolator.flags & rpINTERPOLATORDIRTYINSTANCE)) {
            morphing = TRUE;
        }
        updateNBTs = (morphing || locked);
    }

    if (!AtomicReinstanceCB(object, data)) {
        return NULL;
    }
    if (updateNBTs) {
        fn_80141D20(data, fmt, geometry->numVertices);
    }
    return object;
}

void* _rpGCMatFXSectorInstanceCallBack(void* object, RxGameCubeRenderData* data)
{
    RpGameCubeVtxFmt* fmt;

    if (!SectorInstanceCB(object, data)) {
        return NULL;
    }
    fmt = RpGameCubeWorldGetVtxFmt((RpWorld*)RWSRCGLOBAL(curWorld));
    if (fmt && fmt->nbt) {
        fn_80141D20(data, fmt, ((RpWorldSector*)object)->numVertices);
    }
    return object;
}

static inline void MatFXTextureSet(RwTexture* texture)
{
    RwInt32 mode;

    _rwDlTextureSet(texture, 0);
    mode = 1;
    if (texture && texture->raster && (RASTEREXTFROMRASTER(texture->raster->parent)->flags & rwDLRASTERHASALPHA)) {
        mode = 3;
    }
    _rwDlTextureRasterFlush(mode);
}

static void* _rpGCMatFXRenderCallBack(void* object, RxGameCubeRenderData* data)
{
    RwResEntry* repEntry;
    RwUInt32 geomFlags;
    RwBool resetLighting;
    RpMeshHeader* meshHeader;
    RwInt32 numVerts;
    RxGameCubeDisplayList* dl;
    RwBool resetTev;
    RwResEntry* bumpEntry;
    RwMatrix* ltm;
    RwTexCoords* bumpCoords;
    RwInt32 numMeshes;
    RpMesh* mesh;

    repEntry = data->repEntry;
    bumpEntry = NULL;
    bumpCoords = NULL;
    resetTev = TRUE;
    resetLighting = TRUE;

    dl = RESENTRYDATA(repEntry)->displayLists;
    RESENTRYDATA(repEntry)->token = lbl_8025E800;
    if (((RwObject*)object)->type == rpATOMIC) {
        RpGeometry* geometry = ((RpAtomic*)object)->geometry;

        numVerts = geometry->numVertices;
        geomFlags = geometry->flags;
        meshHeader = geometry->mesh;
        ltm = RwFrameGetLTM((RwFrame*)((RpAtomic*)object)->object.object.parent);
        _rxGCSetVtxDescs(RpGameCubeGeometryGetVtxFmt(((RpAtomic*)object)->geometry), data);
    } else {
        numVerts = RpWorldSectorGetNumVertices((RpWorldSector*)object);
        meshHeader = ((RpWorldSector*)object)->mesh;
        ltm = NULL;
        geomFlags = ((RpWorld*)RWSRCGLOBAL(curWorld))->flags;
        _rxGCSetVtxDescs(RpGameCubeWorldGetVtxFmt((RpWorld*)RWSRCGLOBAL(curWorld)), data);
    }
    _rwDlTransformSetup(ltm, (data->flags & rxGEOMETRY_NORMALS) >> 4);

    numMeshes = meshHeader->numMeshes;
    mesh = (RpMesh*)(meshHeader + 1);
    while (numMeshes--) {
        rpMatFXMaterialData* matfxData;
        void* multiTexture;

        if (resetLighting) {
            _rxGCSetupLighting(data);
            resetLighting = FALSE;
        }
        _rxGCSetupMaterialColor(data, mesh->material);

        multiTexture = RpMaterialGetMultiTexture(mesh->material, 6);
        matfxData = MATFXMATERIALGETDATA(mesh->material);
        if (multiTexture) {
            _rpGameCubeMTMeshRenderCallBack(dl, mesh->material, object, data, ltm);
            resetTev = TRUE;
        } else if (matfxData) {
            if (resetTev) {
                _rxGCDefaultRenderCallback(data);
                resetTev = FALSE;
            }

            switch (matfxData->flags) {
            case rpMATFXEFFECTENVMAP: {
                MatFXEnvMapData* envMapData = &MATFXMATERIALGETDATA(mesh->material)->data[0].data.envMap;
                RwInt32 srcBlend;
                RwInt32 destBlend;

                RwRenderStateGet(rwRENDERSTATESRCBLEND, &srcBlend);
                RwRenderStateGet(rwRENDERSTATEDESTBLEND, &destBlend);

                _rwDlTextureSet(mesh->material->texture, 0);
                _rwDlTextureRasterFlush(1);
                GXCallDisplayList(dl->list, dl->size);

                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
                GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
                GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
                EnvMapColor.a = 255.0f * envMapData->coef;
                GXSetChanMatColor(GX_COLOR0A0, EnvMapColor);
                resetLighting = TRUE;

                GXSetNumTexGens(1);
                GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_NRM, GX_TEXMTX0, GX_FALSE, GX_PTIDENTITY);
                GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);
                GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
                resetTev = TRUE;

                _rwDlTextureSet(envMapData->texture, 0);
                {
                    RwReal texMtx[3][4];
                    RwMatrix inverse;
                    RwMatrix combined;
                    RwMatrix* matrix;

                    matrix = envMapData->frame ? (RwMatrixInvert(&inverse, RwFrameGetLTM(envMapData->frame)), &inverse)
                                               : &lbl_8025E304;
                    if (ltm) {
                        RwMatrixMultiply(&combined, ltm, matrix);
                        matrix = &combined;
                    }
                    texMtx[0][0] = -0.5f * matrix->right.x;
                    texMtx[0][1] = -0.5f * matrix->up.x;
                    texMtx[0][2] = -0.5f * matrix->at.x;
                    texMtx[0][3] = 0.5f;
                    texMtx[1][0] = -0.5f * matrix->right.y;
                    texMtx[1][1] = -0.5f * matrix->up.y;
                    texMtx[1][2] = -0.5f * matrix->at.y;
                    texMtx[1][3] = 0.5f;
                    GXLoadTexMtxImm(texMtx, GX_TEXMTX0, GX_MTX2x4);
                }
                GXCallDisplayList(dl->list, dl->size);

                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcBlend);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)destBlend);
                break;
            }
            case rpMATFXEFFECTDUAL: {
                MatFXDualData* dualData = &MATFXMATERIALGETDATA(mesh->material)->data[0].data.dual;
                RwInt32 srcBlend;
                RwInt32 destBlend;

                RwRenderStateGet(rwRENDERSTATESRCBLEND, &srcBlend);
                RwRenderStateGet(rwRENDERSTATEDESTBLEND, &destBlend);

                MatFXTextureSet(mesh->material->texture);
                GXCallDisplayList(dl->list, dl->size);

                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)dualData->srcBlendMode);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dualData->dstBlendMode);
                if (data->flags & rxGEOMETRY_TEXTURED2) {
                    GXSetArray(GX_VA_TEX0, RESENTRYDATA(repEntry)->texData[1], RESENTRYDATA(repEntry)->texStride[1]);
                }
                MatFXTextureSet(dualData->texture);
                GXCallDisplayList(dl->list, dl->size);
                if (data->flags & rxGEOMETRY_TEXTURED2) {
                    GXSetArray(GX_VA_TEX0, RESENTRYDATA(repEntry)->texData[0], RESENTRYDATA(repEntry)->texStride[0]);
                }

                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcBlend);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)destBlend);
                break;
            }
            case rpMATFXEFFECTBUMPMAP: {
                MatFXBumpMapData* bumpMapData = &MATFXMATERIALGETDATA(mesh->material)->data[0].data.bumpMap;
                RwInt32 srcBlend;
                RwInt32 destBlend;

                RwRenderStateGet(rwRENDERSTATESRCBLEND, &srcBlend);
                RwRenderStateGet(rwRENDERSTATEDESTBLEND, &destBlend);
                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDINVSRCALPHA);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

                _rwDlTextureSet(bumpMapData->bumpedTexture, 0);
                _rwDlTextureRasterFlush(1);
                GXCallDisplayList(dl->list, dl->size);

                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

                if (!bumpEntry) {
                    RwUInt32 size = numVerts * sizeof(RwTexCoords);

                    size += 6;
                    bumpEntry = RwResourcesAllocateResEntry(BumpMapOwner, NULL, size, BumpMapResEntryDestroy);
                    *(RwUInt16*)(bumpEntry + 1) = lbl_8025E800;
                    bumpCoords = (RwTexCoords*)ALIGN32((RwUInt16*)(bumpEntry + 1) + 1);
                }
                BumpMapCalcTexCoords(bumpCoords, object, bumpMapData, mesh, RESENTRYDATA(repEntry)->posData,
                                     RESENTRYDATA(repEntry)->nrmData, RESENTRYDATA(repEntry)->texData[0], numVerts,
                                     geomFlags);
                GXSetArray(GX_VA_TEX0, bumpCoords, sizeof(RwTexCoords));
                GXCallDisplayList(dl->list, dl->size);
                GXSetArray(GX_VA_TEX0, RESENTRYDATA(repEntry)->texData[0], sizeof(RwTexCoords));

                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcBlend);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)destBlend);
                break;
            }
            case rpMATFXEFFECTBUMPENVMAP: {
                MatFXBumpMapData* bumpMapData = &MATFXMATERIALGETDATA(mesh->material)->data[0].data.bumpMap;
                MatFXEnvMapData* envMapData = &MATFXMATERIALGETDATA(mesh->material)->data[1].data.envMap;
                RwInt32 srcBlend;
                RwInt32 destBlend;

                RwRenderStateGet(rwRENDERSTATESRCBLEND, &srcBlend);
                RwRenderStateGet(rwRENDERSTATEDESTBLEND, &destBlend);
                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDINVSRCALPHA);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);

                _rwDlTextureSet(bumpMapData->bumpedTexture, 0);
                _rwDlTextureRasterFlush(1);
                GXCallDisplayList(dl->list, dl->size);

                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

                if (!bumpEntry) {
                    RwUInt32 size = numVerts * sizeof(RwTexCoords);

                    size += 6;
                    bumpEntry = RwResourcesAllocateResEntry(BumpMapOwner, NULL, size, BumpMapResEntryDestroy);
                    *(RwUInt16*)(bumpEntry + 1) = lbl_8025E800;
                    bumpCoords = (RwTexCoords*)ALIGN32((RwUInt16*)(bumpEntry + 1) + 1);
                }
                BumpMapCalcTexCoords(bumpCoords, object, bumpMapData, mesh, RESENTRYDATA(repEntry)->posData,
                                     RESENTRYDATA(repEntry)->nrmData, RESENTRYDATA(repEntry)->texData[0], numVerts,
                                     geomFlags);
                GXSetArray(GX_VA_TEX0, bumpCoords, sizeof(RwTexCoords));
                GXCallDisplayList(dl->list, dl->size);
                GXSetArray(GX_VA_TEX0, RESENTRYDATA(repEntry)->texData[0], sizeof(RwTexCoords));

                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
                GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
                GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
                BumpEnvMapColor.a = 255.0f * envMapData->coef;
                GXSetChanMatColor(GX_COLOR0A0, BumpEnvMapColor);
                resetLighting = TRUE;

                GXSetNumTexGens(1);
                GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_NRM, GX_TEXMTX0, GX_FALSE, GX_PTIDENTITY);
                GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);
                GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
                resetTev = TRUE;

                _rwDlTextureSet(envMapData->texture, 0);
                {
                    RwReal texMtx[3][4];
                    RwMatrix inverse;
                    RwMatrix combined;
                    RwMatrix* matrix;

                    matrix = envMapData->frame ? (RwMatrixInvert(&inverse, RwFrameGetLTM(envMapData->frame)), &inverse)
                                               : &lbl_8025E304;
                    if (ltm) {
                        RwMatrixMultiply(&combined, ltm, matrix);
                        matrix = &combined;
                    }
                    texMtx[0][0] = -0.5f * matrix->right.x;
                    texMtx[0][1] = -0.5f * matrix->up.x;
                    texMtx[0][2] = -0.5f * matrix->at.x;
                    texMtx[0][3] = 0.5f;
                    texMtx[1][0] = -0.5f * matrix->right.y;
                    texMtx[1][1] = -0.5f * matrix->up.y;
                    texMtx[1][2] = -0.5f * matrix->at.y;
                    texMtx[1][3] = 0.5f;
                    GXLoadTexMtxImm(texMtx, GX_TEXMTX0, GX_MTX2x4);
                }
                GXCallDisplayList(dl->list, dl->size);

                RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcBlend);
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)destBlend);
                break;
            }
            default:
                MatFXTextureSet(mesh->material->texture);
                GXCallDisplayList(dl->list, dl->size);
                break;
            }
        } else {
            if (resetTev) {
                _rxGCDefaultRenderCallback(data);
                resetTev = FALSE;
            }
            MatFXTextureSet(mesh->material->texture);
            GXCallDisplayList(dl->list, dl->size);
        }
        dl++;
        mesh++;
    }

    return object;
}

static inline RxPipeline* MatFXAtomicPipelineCreate(void)
{
    RxPipeline* pipe = RxPipelineCreate();

    if (pipe) {
        RxLockedPipe* lpipe;

        pipe->pluginId = rwID_MATERIALEFFECTSPLUGIN;
        lpipe = RxPipelineLock(pipe);
        if (lpipe) {
            RxNodeDefinition* nodeDef = RxNodeDefinitionGetGameCubeAtomicAllInOne();
            RxPipelineNode* node;

            RxLockedPipeUnlock(RxLockedPipeAddFragment(lpipe, NULL, nodeDef, NULL));
            node = RxPipelineFindNodeByName(pipe, nodeDef->name, NULL, NULL);
            AtomicInstanceCB = _rxGameCubeAllInOneGetInstanceCallBack(node);
            AtomicReinstanceCB = _rxGameCubeAllInOneGetReinstanceCallBack(node);
            _rxGameCubeAllInOneSetInstanceCallBack(node, _rpGCMatFXAtomicInstanceCallBack);
            _rxGameCubeAllInOneSetReinstanceCallBack(node, _rpGCMatFXAtomicReinstanceCallBack);
            RxGameCubeAllInOneSetRenderCallBack(node, _rpGCMatFXRenderCallBack);
            return pipe;
        }
        _rxPipelineDestroy(pipe);
    }
    return NULL;
}

static inline RxPipeline* MatFXWorldSectorPipelineCreate(void)
{
    RxPipeline* pipe = RxPipelineCreate();

    if (pipe) {
        RxLockedPipe* lpipe;

        pipe->pluginId = rwID_MATERIALEFFECTSPLUGIN;
        lpipe = RxPipelineLock(pipe);
        if (lpipe) {
            RxNodeDefinition* nodeDef = RxNodeDefinitionGetGameCubeWorldSectorAllInOne();
            RxPipelineNode* node;

            RxLockedPipeUnlock(RxLockedPipeAddFragment(lpipe, NULL, nodeDef, NULL));
            node = RxPipelineFindNodeByName(pipe, nodeDef->name, NULL, NULL);
            SectorInstanceCB = _rxGameCubeAllInOneGetInstanceCallBack(node);
            _rxGameCubeAllInOneSetInstanceCallBack(node, _rpGCMatFXSectorInstanceCallBack);
            RxGameCubeAllInOneSetRenderCallBack(node, _rpGCMatFXRenderCallBack);
            return pipe;
        }
        _rxPipelineDestroy(pipe);
    }
    return NULL;
}

RwBool _rpMatFXPipelinesCreate(void)
{
    MatFXVtxFmt = RpGameCubeVtxFmtCreate();
    RpGameCubeVtxFmtSetNormal(MatFXVtxFmt, GX_F32, 1);
    MatFXAtomicPipeline = MatFXAtomicPipelineCreate();
    MatFXWorldSectorPipeline = MatFXWorldSectorPipelineCreate();
    return TRUE;
}

RwBool _rpMatFXPipelinesDestroy(void)
{
    if (MatFXVtxFmt) {
        RpGameCubeVtxFmtDestroy(MatFXVtxFmt);
        MatFXVtxFmt = NULL;
    }
    if (MatFXAtomicPipeline) {
        _rxPipelineDestroy(MatFXAtomicPipeline);
        MatFXAtomicPipeline = NULL;
    }
    if (MatFXWorldSectorPipeline) {
        _rxPipelineDestroy(MatFXWorldSectorPipeline);
        MatFXWorldSectorPipeline = NULL;
    }
    return TRUE;
}

RpAtomic* _rpMatFXPipelineAtomicSetup(RpAtomic* atomic)
{
    atomic->pipeline = MatFXAtomicPipeline;
    return atomic;
}

RpWorldSector* _rpMatFXPipelineWorldSectorSetup(RpWorldSector* worldSector)
{
    worldSector->pipeline = MatFXWorldSectorPipeline;
    return worldSector;
}

void _rpMatFXSetupEnvMapMaterial(RpMaterial* material)
{
}

void _rpMatFXSetupBumpMapMaterial(RpMaterial* material)
{
}

void _rpMatFXSetupDualMaterial(RpMaterial* material)
{
}

RwBool _rpMatFXSetupDualRenderState(MatFXDualData* dualData, RwInt32 renderState)
{
    return TRUE;
}

RwTexture* _rpMatFXSetupBumpMapTexture(const RwTexture* baseTexture, const RwTexture* effectTexture)
{
    return _rpMatFXTextureMaskCreate(baseTexture, effectTexture);
}

/*
 * Materials: reference counted surface descriptions with an optional texture.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 2 /* world */

#define rwID_TEXTURE 0x06

typedef struct RwSurfaceProperties {
    RwReal ambient;
    RwReal specular;
    RwReal diffuse;
} RwSurfaceProperties;

typedef struct RpMaterial {
    RwTexture* texture;
    RwRGBA color;
    void* pipeline;
    RwSurfaceProperties surfaceProps;
    RwInt16 refCount;
    RwInt16 pad;
} RpMaterial;

typedef struct _rpMaterial {
    RwInt32 flags;
    RwRGBA color;
    RwInt32 unused;
    RwBool textured;
    RwSurfaceProperties surfaceProps;
} _rpMaterial;

RwPluginRegistry materialTKList = { sizeof(RpMaterial), sizeof(RpMaterial), 0, 0, NULL, NULL };
static RwSurfaceProperties defaultSurfaceProperties = { 1.0f, 1.0f, 1.0f };

RwModuleInfo materialModule;

#define RWMATERIALGLOBAL(var) (RWPLUGINOFFSET(RwFreeList*, RwEngineInstance, materialModule.globalsOffset)[0])

void* memset(void* dst, int c, unsigned long n);
RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length);
void* RwMemNative32(void* mem, RwUInt32 size);
void* RwMemFloat32ToReal(void* mem, RwUInt32 size);
RwInt32 _rwPluginRegistryAddPluginStream(RwPluginRegistry* reg, RwUInt32 pluginID,
                                         RwPluginDataChunkReadCallBack readCB,
                                         RwPluginDataChunkWriteCallBack writeCB,
                                         RwPluginDataChunkGetSizeCallBack getSizeCB);
const RwPluginRegistry* _rwPluginRegistryReadDataChunks(const RwPluginRegistry* reg, RwStream* stream,
                                                        void* object);
RwBool RwTextureDestroy(RwTexture* texture);
RwTexture* RwTextureStreamRead(RwStream* stream);
RpMaterial* RpMaterialSetTexture(RpMaterial* material, RwTexture* texture);

void RpMaterialSetDefaultSurfaceProperties(const RwSurfaceProperties* surfaceProps) {
    if (!surfaceProps) {
        defaultSurfaceProperties.ambient = (RwReal)1.0;
        defaultSurfaceProperties.diffuse = (RwReal)1.0;
        defaultSurfaceProperties.specular = (RwReal)1.0;
        return;
    }

    defaultSurfaceProperties = *surfaceProps;
}

void* _rpMaterialOpen(void* instance, RwInt32 offset, RwInt32 size) {
    materialModule.globalsOffset = offset;

    RWMATERIALGLOBAL(materialFreeList) = RwFreeListCreate(materialTKList.sizeOfStruct, 20, 0);
    if (!RWMATERIALGLOBAL(materialFreeList)) {
        return NULL;
    }

    materialModule.numInstances++;

    return instance;
}

void* _rpMaterialClose(void* instance, RwInt32 offset, RwInt32 size) {
    if (RWMATERIALGLOBAL(materialFreeList)) {
        RwFreeListDestroy(RWMATERIALGLOBAL(materialFreeList));
        RWMATERIALGLOBAL(materialFreeList) = NULL;
    }

    materialModule.numInstances--;

    return instance;
}

RwBool RpMaterialDestroy(RpMaterial* material) {
    if (material->refCount == 1) {
        _rwPluginRegistryDeInitObject(&materialTKList, material);
        RpMaterialSetTexture(material, NULL);
        RwFreeListFree(RWMATERIALGLOBAL(materialFreeList), material);
    } else {
        material->refCount--;
    }

    return TRUE;
}

RpMaterial* RpMaterialSetTexture(RpMaterial* material, RwTexture* texture) {
    if (texture) {
        texture->refCount++;
    }

    if (material->texture) {
        RwTextureDestroy(material->texture);
    }

    material->texture = texture;

    return material;
}

RwInt32 RpMaterialRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                                 RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return _rwPluginRegistryAddPlugin(&materialTKList, size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 RpMaterialRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                       RwPluginDataChunkWriteCallBack writeCB,
                                       RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return _rwPluginRegistryAddPluginStream(&materialTKList, pluginID, readCB, writeCB, getSizeCB);
}

static inline RpMaterial* MaterialCreate(void) {
    RpMaterial* material = (RpMaterial*)RwFreeListAlloc(RWMATERIALGLOBAL(materialFreeList));
    RwRGBA white;

    if (!material) {
        return NULL;
    }

    material->refCount = 1;

    white.red = 255;
    white.green = 255;
    white.blue = 255;
    white.alpha = 255;

    material->color = white;
    material->texture = NULL;
    material->pipeline = NULL;
    material->surfaceProps = defaultSurfaceProperties;

    _rwPluginRegistryInitObject(&materialTKList, material);

    return material;
}

RpMaterial* RpMaterialStreamRead(RwStream* stream) {
    RwUInt32 size;
    RwUInt32 version;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version)) {
        return NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION) {
        _rpMaterial mat;
        RwRGBA color;
        RpMaterial* material;

        memset(&mat, 0, sizeof(mat));
        if (RwStreamRead(stream, &mat, size) != size) {
            return NULL;
        }

        /* The colour is four bytes, not a word: keep it out of the swap. */
        color = mat.color;
        RwMemNative32(&mat, sizeof(mat));
        mat.color = color;

        material = MaterialCreate();
        if (!material) {
            return NULL;
        }

        material->color = mat.color;

        if (size <= (RwUInt32)((RwUInt8*)&mat.surfaceProps - (RwUInt8*)&mat)) {
            material->surfaceProps = defaultSurfaceProperties;
        } else {
            RwMemFloat32ToReal(&mat.surfaceProps, sizeof(mat.surfaceProps));
            material->surfaceProps = mat.surfaceProps;
        }

        material->texture = NULL;

        if (mat.textured) {
            if (!RwStreamFindChunk(stream, rwID_TEXTURE, NULL, &version)) {
                RpMaterialDestroy(material);
                return NULL;
            }

            if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION) {
                material->texture = RwTextureStreamRead(stream);
            } else {
                RpMaterialDestroy(material);
                RWERROR((E_RW_BADVERSION));
                return NULL;
            }
        }

        if (!_rwPluginRegistryReadDataChunks(&materialTKList, stream, material)) {
            RpMaterialDestroy(material);
            return NULL;
        }

        return material;
    }

    RWERROR((E_RW_BADVERSION));
    return NULL;
}

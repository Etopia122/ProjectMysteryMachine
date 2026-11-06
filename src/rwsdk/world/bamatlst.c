/*
 * Material lists: the per-geometry array of referenced materials.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 2 /* world */

#define rwID_MATERIAL 0x07
#define rpMATERIALLISTGRANULARITY 20

typedef struct RpMaterial RpMaterial;

/* Only the reference count is touched here. */
#define RpMaterialAddRef(material) (((RwInt16*)(material))[0x18 / 2]++)

typedef struct RpMaterialList {
    RpMaterial** materials;
    RwInt32 numMaterials;
    RwInt32 space;
} RpMaterialList;

#define RwRealloc(_p, _s) ((RWSRCGLOBAL(memoryFuncs).rwrealloc)((_p), (_s)))

RwStream* RwStreamReadInt32(RwStream* stream, RwInt32* ints, RwUInt32 numBytes);
RwBool RpMaterialDestroy(RpMaterial* material);
RpMaterial* RpMaterialStreamRead(RwStream* stream);

RpMaterialList* _rpMaterialListDeinitialize(RpMaterialList* matList) {
    if (matList->materials) {
        RpMaterial** materials = matList->materials;
        RwInt32 numMaterials = matList->numMaterials;
        RwInt32 i;

        for (i = 0; i < numMaterials; i++) {
            RpMaterialDestroy(materials[i]);
            materials[i] = NULL;
        }

        RwFree(materials);
        matList->materials = NULL;
    }

    matList->numMaterials = 0;
    matList->space = 0;

    return matList;
}

RpMaterialList* _rpMaterialListInitialize(RpMaterialList* matList) {
    matList->space = 0;
    matList->materials = NULL;
    matList->numMaterials = 0;

    return matList;
}

RpMaterial* _rpMaterialListGetMaterial(const RpMaterialList* matList, RwInt32 matIndex) {
    return matList->materials[matIndex];
}

RwInt32 _rpMaterialListFindMaterialIndex(const RpMaterialList* matList, const RpMaterial* material) {
    RwInt32 i = matList->numMaterials;

    while (i-- > 0) {
        if (matList->materials[i] == material) {
            break;
        }
    }

    return i;
}

static inline RpMaterialList* MaterialListSetSize(RpMaterialList* matList, RwInt32 size) {
    if (matList->space < size) {
        RwUInt32 memSize = size * sizeof(RpMaterial*);
        RpMaterial** materials;

        if (matList->materials) {
            materials = (RpMaterial**)RwRealloc(matList->materials, memSize);
        } else {
            materials = (RpMaterial**)RwMalloc(memSize);
        }

        if (!materials) {
            RWERROR((E_RW_NOMEM, memSize));
            return NULL;
        }

        matList->materials = materials;
        matList->space = size;
    }

    return matList;
}

static inline RwInt32 MaterialListAppendMaterial(RpMaterialList* matList, RpMaterial* material) {
    if (matList->space > matList->numMaterials) {
        matList->materials[matList->numMaterials] = material;
        RpMaterialAddRef(material);

        return matList->numMaterials++;
    } else {
        RwUInt32 memSize = (matList->space + rpMATERIALLISTGRANULARITY) * sizeof(RpMaterial*);
        RpMaterial** materials;

        if (matList->materials) {
            materials = (RpMaterial**)RwRealloc(matList->materials, memSize);
        } else {
            materials = (RpMaterial**)RwMalloc(memSize);
        }

        if (!materials) {
            RWERROR((E_RW_NOMEM, memSize));
            return -1;
        }

        matList->materials = materials;
        matList->space += rpMATERIALLISTGRANULARITY;

        matList->materials[matList->numMaterials] = material;
        RpMaterialAddRef(material);

        return matList->numMaterials++;
    }
}

RpMaterialList* _rpMaterialListStreamRead(RwStream* stream, RpMaterialList* matList) {
    RwInt32 len;
    RwUInt32 size;
    RwUInt32 version;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version)) {
        return NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION) {
        RwInt32* matIndices;
        RwInt32 i;
        RwBool status = (RwStreamReadInt32(stream, &len, sizeof(len)) != NULL);

        if (!status) {
            return NULL;
        }

        _rpMaterialListInitialize(matList);

        if (!len) {
            return matList;
        }

        if (!MaterialListSetSize(matList, len)) {
            _rpMaterialListDeinitialize(matList);
            return NULL;
        }

        matIndices = (RwInt32*)RwMalloc(len * sizeof(RwInt32));
        status = (RwStreamReadInt32(stream, matIndices, len * sizeof(RwInt32)) != NULL);
        if (!status) {
            RwFree(matIndices);
            _rpMaterialListDeinitialize(matList);
            return NULL;
        }

        for (i = 0; i < len; i++) {
            RpMaterial* material;

            if (matIndices[i] < 0) {
                if (!RwStreamFindChunk(stream, rwID_MATERIAL, NULL, &version)) {
                    RwFree(matIndices);
                    _rpMaterialListDeinitialize(matList);
                    return NULL;
                }

                if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION) {
                    material = RpMaterialStreamRead(stream);
                    if (!material) {
                        RwFree(matIndices);
                        _rpMaterialListDeinitialize(matList);
                        return NULL;
                    }
                } else {
                    RWERROR((E_RW_BADVERSION));
                    RwFree(matIndices);
                    _rpMaterialListDeinitialize(matList);
                    return NULL;
                }
            } else {
                material = matList->materials[matIndices[i]];
                RpMaterialAddRef(material);
            }

            MaterialListAppendMaterial(matList, material);
            RpMaterialDestroy(material);
        }

        RwFree(matIndices);

        return matList;
    }

    RWERROR((E_RW_BADVERSION));
    return NULL;
}

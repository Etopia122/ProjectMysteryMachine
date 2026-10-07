/*
 * User data plugin: named arrays of integers, reals or strings attached to
 * geometries, world sectors, frames, cameras, lights, materials and
 * textures, with stream support.
 */

#include "rwsdk/rwcore.h"

#define rwID_USERDATAPLUGIN 0x11F

typedef enum RpUserDataFormat {
    rpNAUSERDATAFORMAT = 0,
    rpINTUSERDATA = 1,
    rpREALUSERDATA = 2,
    rpSTRINGUSERDATA = 3,
} RpUserDataFormat;

typedef struct RpUserDataArray {
    RwChar* name;
    RpUserDataFormat format;
    RwInt32 numElements;
    void* data;
} RpUserDataArray;

typedef struct RpUserDataList {
    RwInt32 numElements;
    RpUserDataArray* userData;
} RpUserDataList;


typedef struct RpGeometry RpGeometry;

#define rwstrlen(_s) (RWSRCGLOBAL(stringFuncs).rwstrlen(_s))
#define rwstrcpy(_d, _s) (RWSRCGLOBAL(stringFuncs).rwstrcpy((_d), (_s)))

#define rwstrdup(_dst, _src)                                              \
    do {                                                                  \
        (_dst) = NULL;                                                    \
        if (_src) {                                                       \
            (_dst) = (RwChar*)RwMalloc(rwstrlen(_src) + 1);               \
            if (_dst) {                                                   \
                rwstrcpy((_dst), (_src));                                 \
            }                                                             \
        }                                                                 \
    } while (0)

#define USERDATALIST(object, offset) ((RpUserDataList*)((RwUInt8*)(object) + (offset)))

/* .sbss is laid out in reverse declaration order. */
static RwModuleInfo userDataModule;
static RwInt32 textureStreamOffset;
static RwInt32 textureOffset;
static RwInt32 materialStreamOffset;
static RwInt32 materialOffset;
static RwInt32 lightStreamOffset;
static RwInt32 lightOffset;
static RwInt32 cameraStreamOffset;
static RwInt32 cameraOffset;
static RwInt32 frameStreamOffset;
static RwInt32 frameOffset;
static RwInt32 worldSectorStreamOffset;
static RwInt32 worldSectorOffset;
static RwInt32 geometryStreamOffset;
static RwInt32 geometryOffset;

RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length);
RwStream* RwStreamWrite(RwStream* stream, const void* buffer, RwUInt32 length);
void* memcpy(void* dst, const void* src, unsigned long n);

RwInt32 RpGeometryRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                                 RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpGeometryRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                       RwPluginDataChunkWriteCallBack writeCB,
                                       RwPluginDataChunkGetSizeCallBack getSizeCB);
RwInt32 RpWorldSectorRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                                    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpWorldSectorRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                          RwPluginDataChunkWriteCallBack writeCB,
                                          RwPluginDataChunkGetSizeCallBack getSizeCB);
RwInt32 RwFrameRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RwFrameRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                    RwPluginDataChunkWriteCallBack writeCB,
                                    RwPluginDataChunkGetSizeCallBack getSizeCB);
RwInt32 RwCameraRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                               RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RwCameraRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                     RwPluginDataChunkWriteCallBack writeCB,
                                     RwPluginDataChunkGetSizeCallBack getSizeCB);
RwInt32 RpLightRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpLightRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                    RwPluginDataChunkWriteCallBack writeCB,
                                    RwPluginDataChunkGetSizeCallBack getSizeCB);
RwInt32 RpMaterialRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                                 RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RpMaterialRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                       RwPluginDataChunkWriteCallBack writeCB,
                                       RwPluginDataChunkGetSizeCallBack getSizeCB);
RwInt32 RwTextureRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB,
                                RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB);
RwInt32 RwTextureRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                      RwPluginDataChunkWriteCallBack writeCB,
                                      RwPluginDataChunkGetSizeCallBack getSizeCB);

RwInt32 RpUserDataGetFormatSize(RpUserDataFormat format);

void* UserDataOpen(void* instance, RwInt32 offset, RwInt32 size) {
    userDataModule.numInstances++;
    return instance;
}

void* UserDataClose(void* instance, RwInt32 offset, RwInt32 size) {
    userDataModule.numInstances--;
    return instance;
}

RwStream* UserDataStreamRead(RpUserDataArray* userData, RwStream* stream) {
    RwInt32 nameLength;

    if (!RwStreamReadInt32(stream, &nameLength, sizeof(RwInt32))) {
        return NULL;
    }

    if (nameLength > 0) {
        userData->name = (RwChar*)RwMalloc(nameLength);
        if (!userData->name) {
            return NULL;
        }

        if (!RwStreamRead(stream, userData->name, nameLength)) {
            return NULL;
        }
    }

    if (!RwStreamReadInt32(stream, (RwInt32*)&userData->format, sizeof(RwInt32))) {
        return NULL;
    }

    if (!RwStreamReadInt32(stream, &userData->numElements, sizeof(RwInt32))) {
        return NULL;
    }

    switch (userData->format) {
    case rpINTUSERDATA: {
        RwInt32* intData;
        RwInt32 i;

        userData->data = RwMalloc(userData->numElements * sizeof(RwInt32));
        intData = (RwInt32*)userData->data;
        if (!intData) {
            return NULL;
        }

        for (i = 0; i < userData->numElements; i++) {
            if (!RwStreamReadInt32(stream, intData, sizeof(RwInt32))) {
                return NULL;
            }

            intData++;
        }
        break;
    }
    case rpREALUSERDATA: {
        RwInt32 i;
        RwReal* realData;

        userData->data = RwMalloc(userData->numElements * sizeof(RwReal));
        realData = (RwReal*)userData->data;
        if (!realData) {
            return NULL;
        }

        for (i = 0; i < userData->numElements; i++) {
            if (!RwStreamReadReal(stream, realData, sizeof(RwReal))) {
                return NULL;
            }

            realData++;
        }
        break;
    }
    case rpSTRINGUSERDATA: {
        RwInt32 i;
        RwChar** stringData;

        userData->data = RwMalloc(userData->numElements * sizeof(RwChar*));
        stringData = (RwChar**)userData->data;
        if (!stringData) {
            return NULL;
        }

        for (i = 0; i < userData->numElements; i++) {
            if (!RwStreamReadInt32(stream, &nameLength, sizeof(RwInt32))) {
                return NULL;
            }

            if (nameLength > 0) {
                *stringData = (RwChar*)RwMalloc(nameLength);
                if (!*stringData) {
                    return NULL;
                }

                if (!RwStreamRead(stream, *stringData, nameLength)) {
                    return NULL;
                }
            } else {
                *stringData = NULL;
            }

            stringData++;
        }
        break;
    }
    default:
        return NULL;
    }

    return stream;
}

RwStream* UserDataStreamWrite(const RpUserDataArray* userData, RwStream* stream) {
    RwInt32 nameLength;

    nameLength = userData->name ? rwstrlen(userData->name) + 1 : 0;
    if (!RwStreamWriteInt32(stream, &nameLength, sizeof(RwInt32))) {
        return NULL;
    }

    if (nameLength > 0) {
        if (!RwStreamWrite(stream, userData->name, nameLength)) {
            return NULL;
        }
    }

    if (!RwStreamWriteInt32(stream, (const RwInt32*)&userData->format, sizeof(RwInt32))) {
        return NULL;
    }

    if (!RwStreamWriteInt32(stream, &userData->numElements, sizeof(RwInt32))) {
        return NULL;
    }

    switch (userData->format) {
    case rpINTUSERDATA: {
        const RwInt32* intData = (const RwInt32*)userData->data;
        RwInt32 i;

        for (i = 0; i < userData->numElements; i++) {
            if (!RwStreamWriteInt32(stream, intData, sizeof(RwInt32))) {
                return NULL;
            }

            intData++;
        }
        break;
    }
    case rpREALUSERDATA: {
        RwInt32 i;
        const RwReal* realData = (const RwReal*)userData->data;

        for (i = 0; i < userData->numElements; i++) {
            if (!RwStreamWriteReal(stream, realData, sizeof(RwReal))) {
                return NULL;
            }

            realData++;
        }
        break;
    }
    case rpSTRINGUSERDATA: {
        RwInt32 i;
        RwChar** stringData = (RwChar**)userData->data;

        for (i = 0; i < userData->numElements; i++) {
            nameLength = *stringData ? rwstrlen(*stringData) + 1 : 0;
            if (!RwStreamWriteInt32(stream, &nameLength, sizeof(RwInt32))) {
                return NULL;
            }

            if (nameLength > 0) {
                if (!RwStreamWrite(stream, *stringData, nameLength)) {
                    return NULL;
                }
            }

            stringData++;
        }
        break;
    }
    default:
        return NULL;
    }

    return stream;
}

static inline void UserDataListDestroy(RpUserDataList* list) {
    RwChar** stringData;
    RpUserDataArray* userData;
    RwInt32 j;
    RwInt32 i;

    if (list->userData) {
        for (i = 0; i < list->numElements; i++) {
            userData = &list->userData[i];

            if (userData->name) {
                RwFree(userData->name);
            }

            if (userData->format == rpSTRINGUSERDATA) {
                stringData = (RwChar**)userData->data;

                for (j = 0; j < userData->numElements; j++) {
                    if (*stringData) {
                        RwFree(*stringData);
                    }

                    stringData++;
                }
            }

            if (userData->data) {
                RwFree(userData->data);
            }
        }

        RwFree(list->userData);
    }

    list->userData = NULL;
    list->numElements = 0;
}

void UserDataListCopy(RpUserDataList* dst, const RpUserDataList* src) {
    RwChar** srcStrings;
    RwInt32 j;
    const RpUserDataArray* srcData;
    RwChar** dstStrings;
    RpUserDataArray* dstData;
    RwInt32 size;
    RwInt32 i;

    UserDataListDestroy(dst);

    dst->numElements = src->numElements;
    if (dst->numElements > 0) {
        dst->userData = (RpUserDataArray*)RwMalloc(dst->numElements * sizeof(RpUserDataArray));

        for (i = 0; i < dst->numElements; i++) {
            srcData = &src->userData[i];
            dstData = &dst->userData[i];

            dstData->format = srcData->format;
            dstData->numElements = srcData->numElements;

            if (srcData->name) {
                rwstrdup(dstData->name, srcData->name);
            }

            if (srcData->data) {
                size = dstData->numElements * RpUserDataGetFormatSize(dstData->format);

                dstData->data = RwMalloc(size);

                if (dstData->format == rpSTRINGUSERDATA) {
                    srcStrings = (RwChar**)srcData->data;
                    dstStrings = (RwChar**)dstData->data;

                    for (j = 0; j < dstData->numElements; j++) {
                        if (!*srcStrings) {
                            *dstStrings = NULL;
                        } else {
                            rwstrdup(*dstStrings, *srcStrings);
                        }

                        srcStrings++;
                        dstStrings++;
                    }
                } else {
                    memcpy(dstData->data, srcData->data, size);
                }
            }
        }
    }
}

RwInt32 UserDataListAddElement(RpUserDataList* list, const RwChar* name, RpUserDataFormat format,
                               RwInt32 numElements) {
    RpUserDataArray* userData;
    RwInt32 index = -1;
    RwInt32 i;

    for (i = 0; i < list->numElements; i++) {
        if (!list->userData[i].data) {
            index = i;
        }
    }

    if (index == -1) {
        if (list->userData) {
            RpUserDataArray* newData =
                (RpUserDataArray*)RwMalloc((list->numElements + 1) * sizeof(RpUserDataArray));

            if (!newData) {
                return -1;
            }

            memcpy(newData, list->userData, list->numElements * sizeof(RpUserDataArray));
            RwFree(list->userData);
            list->userData = newData;
        } else {
            list->userData = (RpUserDataArray*)RwMalloc((list->numElements + 1) * sizeof(RpUserDataArray));
            if (!list->userData) {
                return -1;
            }
        }

        index = list->numElements;
        list->numElements++;
    }

    userData = &list->userData[index];

    userData->data = RwMalloc(numElements * RpUserDataGetFormatSize(format));
    if (!userData->data) {
        return -1;
    }

    rwstrdup(userData->name, name);
    userData->format = format;
    userData->numElements = numElements;

    return index;
}

void* UserDataObjectConstruct(void* object, RwInt32 offset, RwInt32 size) {
    RpUserDataList* list = USERDATALIST(object, offset);

    list->numElements = 0;
    list->userData = NULL;

    return object;
}

void* UserDataObjectDestruct(void* object, RwInt32 offset, RwInt32 size) {
    UserDataListDestroy(USERDATALIST(object, offset));
    return object;
}

void* UserDataObjectCopy(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size) {
    UserDataListCopy(USERDATALIST(dstObject, offset), USERDATALIST(srcObject, offset));
    return dstObject;
}

static inline RwStream* UserDataListStreamRead(RwStream* stream, RpUserDataList* list) {
    RwInt32 numElements;
    RwInt32 i;

    if (list) {
        if (RwStreamReadInt32(stream, &numElements, sizeof(RwInt32))) {
            list->numElements = numElements;
            list->userData = (RpUserDataArray*)RwMalloc(list->numElements * sizeof(RpUserDataArray));

            for (i = 0; i < list->numElements; i++) {
                stream = UserDataStreamRead(&list->userData[i], stream);
            }
        }
    }

    return stream;
}

RwStream* UserDataObjectStreamRead(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offset,
                                   RwInt32 size) {
    UserDataListStreamRead(stream, USERDATALIST(object, offset));
    return stream;
}

static inline RwStream* UserDataListStreamWrite(RwStream* stream, const RpUserDataList* list) {
    RwInt32 i;

    if (list && list->numElements > 0) {
        if (RwStreamWriteInt32(stream, &list->numElements, sizeof(RwInt32))) {
            for (i = 0; i < list->numElements; i++) {
                stream = UserDataStreamWrite(&list->userData[i], stream);
            }
        }
    }

    return stream;
}

RwStream* UserDataObjectStreamWrite(RwStream* stream, RwInt32 binaryLength, const void* object, RwInt32 offset,
                                    RwInt32 size) {
    UserDataListStreamWrite(stream, USERDATALIST(object, offset));
    return stream;
}

static inline RwInt32 UserDataArrayGetSize(const RpUserDataArray* userData) {
    RwChar** stringData;
    RwInt32 j;
    RwInt32 size;

    size = 0;
    if (userData) {
        size = sizeof(RwInt32);
        if (userData->name) {
            size += rwstrlen(userData->name) + 1;
        }

        size += sizeof(RwInt32) + sizeof(RwInt32);

        switch (userData->format) {
        case rpINTUSERDATA:
            size += userData->numElements * sizeof(RwInt32);
            break;
        case rpREALUSERDATA:
            size += userData->numElements * sizeof(RwReal);
            break;
        case rpSTRINGUSERDATA:
            stringData = (RwChar**)userData->data;

            for (j = 0; j < userData->numElements; j++) {
                size += sizeof(RwInt32);
                if (*stringData) {
                    RwInt32 len = rwstrlen(*stringData) + 1;

                    size += len;
                }

                stringData++;
            }
            break;
        }
    }

    return size;
}

static inline RwInt32 UserDataListGetSize(const RpUserDataList* list) {
    RwInt32 i;
    RwInt32 totalSize = 0;

    if (list && list->numElements > 0) {
        totalSize += sizeof(RwInt32);
        for (i = 0; i < list->numElements; i++) {
            totalSize += UserDataArrayGetSize(&list->userData[i]);
        }
    }

    return totalSize;
}

RwInt32 UserDataObjectGetSize(const void* object, RwInt32 offset, RwInt32 size) {
    return UserDataListGetSize(USERDATALIST(object, offset));
}

RwInt32 RpGeometryAddUserDataArray(RpGeometry* geometry, RwChar* name, RpUserDataFormat format,
                                   RwInt32 numElements) {
    return UserDataListAddElement(USERDATALIST(geometry, geometryOffset), name, format,
                                  numElements);
}

RwInt32 RpGeometryGetUserDataArrayCount(const RpGeometry* geometry) {
    RwInt32 count = 0;
    RwInt32 i;
    const RpUserDataList* list = USERDATALIST(geometry, geometryOffset);

    for (i = 0; i < list->numElements; i++) {
        if (list->userData[i].data) {
            count++;
        }
    }

    return count;
}

RpUserDataArray* RpGeometryGetUserDataArray(const RpGeometry* geometry, RwInt32 data) {
    const RpUserDataList* list = USERDATALIST(geometry, geometryOffset);

    if (data < list->numElements) {
        return &list->userData[data];
    }

    return NULL;
}

RwInt32 RpUserDataGetFormatSize(RpUserDataFormat format) {
    switch (format) {
    case rpINTUSERDATA:
        return sizeof(RwInt32);
    case rpREALUSERDATA:
        return sizeof(RwReal);
    case rpSTRINGUSERDATA:
        return sizeof(RwChar*);
    default:
        return 0;
    }
}

RwBool RpUserDataPluginAttach(void) {
    if (RwEngineRegisterPlugin(0, rwID_USERDATAPLUGIN, UserDataOpen, UserDataClose) < 0) {
        return FALSE;
    }

    geometryOffset =
        RpGeometryRegisterPlugin(sizeof(RpUserDataList), rwID_USERDATAPLUGIN, UserDataObjectConstruct,
                                 UserDataObjectDestruct, UserDataObjectCopy);
    if (geometryOffset < 0) {
        return FALSE;
    }

    geometryStreamOffset = RpGeometryRegisterPluginStream(
        rwID_USERDATAPLUGIN, UserDataObjectStreamRead, UserDataObjectStreamWrite, UserDataObjectGetSize);
    if (geometryStreamOffset < 0) {
        return FALSE;
    }

    worldSectorOffset =
        RpWorldSectorRegisterPlugin(sizeof(RpUserDataList), rwID_USERDATAPLUGIN, UserDataObjectConstruct,
                                    UserDataObjectDestruct, UserDataObjectCopy);
    if (geometryOffset < 0) {
        return FALSE;
    }

    worldSectorStreamOffset = RpWorldSectorRegisterPluginStream(
        rwID_USERDATAPLUGIN, UserDataObjectStreamRead, UserDataObjectStreamWrite, UserDataObjectGetSize);
    if (geometryStreamOffset < 0) {
        return FALSE;
    }

    frameOffset =
        RwFrameRegisterPlugin(sizeof(RpUserDataList), rwID_USERDATAPLUGIN, UserDataObjectConstruct,
                              UserDataObjectDestruct, UserDataObjectCopy);
    if (frameOffset < 0) {
        return FALSE;
    }

    frameStreamOffset = RwFrameRegisterPluginStream(
        rwID_USERDATAPLUGIN, UserDataObjectStreamRead, UserDataObjectStreamWrite, UserDataObjectGetSize);
    if (frameStreamOffset < 0) {
        return FALSE;
    }

    cameraOffset =
        RwCameraRegisterPlugin(sizeof(RpUserDataList), rwID_USERDATAPLUGIN, UserDataObjectConstruct,
                               UserDataObjectDestruct, UserDataObjectCopy);
    if (cameraOffset < 0) {
        return FALSE;
    }

    cameraStreamOffset = RwCameraRegisterPluginStream(
        rwID_USERDATAPLUGIN, UserDataObjectStreamRead, UserDataObjectStreamWrite, UserDataObjectGetSize);
    if (cameraStreamOffset < 0) {
        return FALSE;
    }

    lightOffset =
        RpLightRegisterPlugin(sizeof(RpUserDataList), rwID_USERDATAPLUGIN, UserDataObjectConstruct,
                              UserDataObjectDestruct, UserDataObjectCopy);
    if (lightOffset < 0) {
        return FALSE;
    }

    lightStreamOffset = RpLightRegisterPluginStream(
        rwID_USERDATAPLUGIN, UserDataObjectStreamRead, UserDataObjectStreamWrite, UserDataObjectGetSize);
    if (lightStreamOffset < 0) {
        return FALSE;
    }

    materialOffset =
        RpMaterialRegisterPlugin(sizeof(RpUserDataList), rwID_USERDATAPLUGIN, UserDataObjectConstruct,
                                 UserDataObjectDestruct, UserDataObjectCopy);
    if (materialOffset < 0) {
        return FALSE;
    }

    materialStreamOffset = RpMaterialRegisterPluginStream(
        rwID_USERDATAPLUGIN, UserDataObjectStreamRead, UserDataObjectStreamWrite, UserDataObjectGetSize);
    if (materialStreamOffset < 0) {
        return FALSE;
    }

    textureOffset =
        RwTextureRegisterPlugin(sizeof(RpUserDataList), rwID_USERDATAPLUGIN, UserDataObjectConstruct,
                                UserDataObjectDestruct, UserDataObjectCopy);
    if (textureOffset < 0) {
        return FALSE;
    }

    textureStreamOffset = RwTextureRegisterPluginStream(
        rwID_USERDATAPLUGIN, UserDataObjectStreamRead, UserDataObjectStreamWrite, UserDataObjectGetSize);
    if (textureStreamOffset < 0) {
        return FALSE;
    }

    return TRUE;
}

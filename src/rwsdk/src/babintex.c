/*
 * Binary texture streaming: string chunks, textures and texture dictionaries.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 1 /* core */

#define rwID_STRING 0x02
#define rwID_TEXTURE 0x06
#define rwID_UNICODESTRING 0x13
#define rwID_TEXTURENATIVE 0x15

#define rwRASTERFORMATAUTOMIPMAP 0x1000

typedef struct _rwStreamTexture {
    RwUInt32 filterAndAddress;
} _rwStreamTexture;

typedef struct _rwStreamTexDictionary {
    RwInt32 numTextures;
} _rwStreamTexDictionary;

static const RwChar nullString[] = "";

extern RwPluginRegistry textureTKList;
extern RwPluginRegistry texDictTKList;

void* memset(void* dst, int c, unsigned long n);
RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length);
RwStream* RwStreamWrite(RwStream* stream, const void* buffer, RwUInt32 length);
void* RwMemNative32(void* mem, RwUInt32 size);
void* RwMemLittleEndian32(void* mem, RwUInt32 size);
RwInt32 _rwPluginRegistryAddPluginStream(RwPluginRegistry* reg, RwUInt32 pluginID,
                                         RwPluginDataChunkReadCallBack readCB,
                                         RwPluginDataChunkWriteCallBack writeCB,
                                         RwPluginDataChunkGetSizeCallBack getSizeCB);
const RwPluginRegistry* _rwPluginRegistryReadDataChunks(const RwPluginRegistry* reg, RwStream* stream,
                                                        void* object);
const RwPluginRegistry* _rwPluginRegistryWriteDataChunks(const RwPluginRegistry* reg, RwStream* stream,
                                                         const void* object);
const RwPluginRegistry* _rwPluginRegistrySkipDataChunks(const RwPluginRegistry* reg, RwStream* stream);
RwInt32 _rwPluginRegistryGetSize(const RwPluginRegistry* reg, const void* object);

RwBool RwTextureSetMipmapping(RwBool enable);
RwBool RwTextureGetMipmapping(void);
RwBool RwTextureSetAutoMipmapping(RwBool enable);
RwBool RwTextureGetAutoMipmapping(void);
RwTexture* RwTextureRead(const RwChar* name, const RwChar* maskName);
RwBool RwTextureDestroy(RwTexture* texture);
RwTexDictionary* RwTexDictionaryCreate(void);
RwBool RwTexDictionaryDestroy(RwTexDictionary* dict);
const RwTexDictionary* RwTexDictionaryForAllTextures(const RwTexDictionary* dict, RwTextureCallBack fpCallBack,
                                                     void* pData);
RwTexture* RwTexDictionaryAddTexture(RwTexDictionary* dict, RwTexture* texture);

RwUInt32 _rwStringStreamGetSize(const RwChar* string) {
    if (!string) {
        string = nullString;
    }

    return (RWSRCGLOBAL(stringFuncs).rwstrlen(string) + 1 + 3) & ~3;
}

const RwChar* _rwStringStreamWrite(const RwChar* string, RwStream* stream) {
    RwUInt32 size;

    if (!string) {
        string = nullString;
    }

    size = _rwStringStreamGetSize(string);

    if (!RwStreamWriteChunkHeader(stream, rwID_STRING, size)) {
        return NULL;
    }

    if (!RwStreamWrite(stream, string, size)) {
        return NULL;
    }

    return string;
}

static inline RwChar* StringStreamReadAscii(RwChar* string, RwStream* stream, RwUInt32 length) {
    RwChar* dst;

    if (!string) {
        string = (RwChar*)RwMalloc(length);
        if (!string) {
            RWERROR((E_RW_NOMEM, length));
            return NULL;
        }
    }

    dst = string;
    while (length) {
        RwChar buffer[64];
        RwUInt32 chunk = 64;
        RwUInt32 i;

        if (length <= 64) {
            chunk = length;
        }

        if (RwStreamRead(stream, buffer, chunk) != chunk) {
            return NULL;
        }

        length -= chunk;
        for (i = 0; i < chunk; i++) {
            dst[i] = buffer[i];
        }
        dst += chunk;
    }

    return string;
}

static inline RwChar* StringStreamReadUnicode(RwChar* string, RwStream* stream, RwUInt32 length) {
    RwBool allocated = FALSE;
    RwChar* dst;

    if (!string) {
        string = (RwChar*)RwMalloc(length);
        if (!string) {
            RWERROR((E_RW_NOMEM, length));
            return NULL;
        }
        allocated = TRUE;
    }

    dst = string;
    while (length) {
        RwUInt16 buffer[64];
        RwUInt32 chunk = 128;
        RwUInt32 count;
        RwUInt32 i;

        if (length <= 128) {
            chunk = length;
        }

        if (RwStreamRead(stream, buffer, chunk) != chunk) {
            if (allocated) {
                RwFree(string);
            }
            return NULL;
        }

        count = chunk >> 1;
        length -= chunk;
        for (i = 0; i < count; i++) {
            dst[i] = (RwChar)buffer[i];
        }
        dst += count;
    }

    return string;
}

RwChar* _rwStringStreamFindAndRead(RwChar* string, RwStream* stream) {
    RwUInt32 type;
    RwUInt32 length;
    RwUInt32 version;

    while (_rwStreamReadChunkHeader(stream, &type, &length, &version, NULL)) {
        RwBool versionOK = (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION);

        if (!versionOK) {
            RWERROR((E_RW_BADVERSION));
            return NULL;
        }

        if (type == rwID_STRING) {
            return StringStreamReadAscii(string, stream, length);
        } else if (type == rwID_UNICODESTRING) {
            return StringStreamReadUnicode(string, stream, length);
        } else if (!RwStreamSkip(stream, length)) {
            return NULL;
        }
    }

    return NULL;
}

RwInt32 RwTextureRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                      RwPluginDataChunkWriteCallBack writeCB,
                                      RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return _rwPluginRegistryAddPluginStream(&textureTKList, pluginID, readCB, writeCB, getSizeCB);
}

RwUInt32 RwTextureStreamGetSize(const RwTexture* texture) {
    RwUInt32 size = sizeof(_rwStreamTexture) + rwCHUNKHEADERSIZE;

    size += _rwStringStreamGetSize(texture->name) + rwCHUNKHEADERSIZE;
    size += _rwStringStreamGetSize(texture->mask) + rwCHUNKHEADERSIZE;
    size += _rwPluginRegistryGetSize(&textureTKList, texture) + rwCHUNKHEADERSIZE;

    return size;
}

const RwTexture* RwTextureStreamWrite(const RwTexture* texture, RwStream* stream) {
    _rwStreamTexture texFiltAddr;

    if (!RwStreamWriteChunkHeader(stream, rwID_TEXTURE, RwTextureStreamGetSize(texture))) {
        return NULL;
    }

    if (!RwStreamWriteChunkHeader(stream, rwID_STRUCT, sizeof(_rwStreamTexture))) {
        return NULL;
    }

    texFiltAddr.filterAndAddress =
        ((RwUInt8)(texture->raster && !(texture->raster->cFormat & (rwRASTERFORMATAUTOMIPMAP >> 8))) << 16) |
        (texture->filterAddressing & 0xFFFF);
    RwMemLittleEndian32(&texFiltAddr, sizeof(texFiltAddr));

    if (!RwStreamWrite(stream, &texFiltAddr, sizeof(texFiltAddr))) {
        return NULL;
    }

    if (!_rwStringStreamWrite(texture->name, stream)) {
        return NULL;
    }

    if (!_rwStringStreamWrite(texture->mask, stream)) {
        return NULL;
    }

    if (!_rwPluginRegistryWriteDataChunks(&textureTKList, stream, texture)) {
        return NULL;
    }

    return texture;
}

RwTexture* RwTextureStreamRead(RwStream* stream) {
    RwUInt32 size;
    RwUInt32 version;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version)) {
        return NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION) {
        RwChar textureName[128];
        RwChar textureMask[128];
        _rwStreamTexture texFiltAddr;
        RwUInt32 filtering;
        RwUInt32 flags;
        RwBool mipmapState;
        RwBool autoMipmapState;
        RwTexture* texture;

        memset(&texFiltAddr, 0, sizeof(texFiltAddr));
        if (RwStreamRead(stream, &texFiltAddr, size) != size) {
            return NULL;
        }

        RwMemNative32(&texFiltAddr, sizeof(texFiltAddr));

        filtering = texFiltAddr.filterAndAddress & 0xFF;
        if (!((texFiltAddr.filterAndAddress >> 12) & 0xF)) {
            texFiltAddr.filterAndAddress |= ((texFiltAddr.filterAndAddress >> 8) & 0xF) << 12;
        }
        flags = (texFiltAddr.filterAndAddress >> 16) & 0xFF;

        mipmapState = RwTextureGetMipmapping();
        autoMipmapState = RwTextureGetAutoMipmapping();

        if (filtering == 3 || filtering == 4 || filtering == 5 || filtering == 6) {
            RwTextureSetMipmapping(TRUE);
            if (flags & 1) {
                RwTextureSetAutoMipmapping(FALSE);
            } else {
                RwTextureSetAutoMipmapping(TRUE);
            }
        } else {
            RwTextureSetMipmapping(FALSE);
            RwTextureSetAutoMipmapping(FALSE);
        }

        if (!_rwStringStreamFindAndRead(textureName, stream)) {
            RwTextureSetMipmapping(mipmapState);
            RwTextureSetAutoMipmapping(autoMipmapState);
            return NULL;
        }

        if (!_rwStringStreamFindAndRead(textureMask, stream)) {
            RwTextureSetMipmapping(mipmapState);
            RwTextureSetAutoMipmapping(autoMipmapState);
            return NULL;
        }

        texture = RwTextureRead(textureName, textureMask);
        if (!texture) {
            _rwPluginRegistrySkipDataChunks(&textureTKList, stream);
            RwTextureSetMipmapping(mipmapState);
            RwTextureSetAutoMipmapping(autoMipmapState);
            return NULL;
        }

        if (texture->refCount == 1) {
            texture->filterAddressing = texFiltAddr.filterAndAddress & 0xFFFF;
        }

        RwTextureSetMipmapping(mipmapState);
        RwTextureSetAutoMipmapping(autoMipmapState);

        if (!_rwPluginRegistryReadDataChunks(&textureTKList, stream, texture)) {
            return NULL;
        }

        return texture;
    }

    RWERROR((E_RW_BADVERSION));
    return NULL;
}

static RwTexture* destroyTexture(RwTexture* texture, void* pData) {
    RwTextureDestroy(texture);

    return texture;
}

RwTexDictionary* RwTexDictionaryStreamRead(RwStream* stream) {
    RwUInt32 size;
    RwUInt32 version;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version)) {
        return NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION) {
        _rwStreamTexDictionary binTexDict;
        RwTexture* texture;
        RwTexDictionary* texDict;

        memset(&binTexDict, 0, sizeof(binTexDict));
        if (RwStreamRead(stream, &binTexDict, size) != size) {
            return NULL;
        }

        RwMemNative32(&binTexDict, sizeof(binTexDict));

        texDict = RwTexDictionaryCreate();
        if (!texDict) {
            return NULL;
        }

        while (binTexDict.numTextures--) {
            if (!RwStreamFindChunk(stream, rwID_TEXTURENATIVE, &size, &version)) {
                RwTexDictionaryForAllTextures(texDict, destroyTexture, NULL);
                RwTexDictionaryDestroy(texDict);
                return NULL;
            }

            if (version < rwLIBRARYBASEVERSION || version > rwLIBRARYCURRENTVERSION) {
                RwTexDictionaryForAllTextures(texDict, destroyTexture, NULL);
                RwTexDictionaryDestroy(texDict);
                RWERROR((E_RW_BADVERSION));
                return NULL;
            }

            if (!RWSRCGLOBAL(stdFunc)[rwSTANDARDNATIVETEXTUREREAD](stream, &texture, size)) {
                RwTexDictionaryForAllTextures(texDict, destroyTexture, NULL);
                RwTexDictionaryDestroy(texDict);
                return NULL;
            }

            if (!texture) {
                RwTexDictionaryForAllTextures(texDict, destroyTexture, NULL);
                RwTexDictionaryDestroy(texDict);
                return NULL;
            }

            if (!_rwPluginRegistryReadDataChunks(&textureTKList, stream, texture)) {
                RwTexDictionaryForAllTextures(texDict, destroyTexture, NULL);
                RwTexDictionaryDestroy(texDict);
                return NULL;
            }

            RwTexDictionaryAddTexture(texDict, texture);
        }

        if (!_rwPluginRegistryReadDataChunks(&texDictTKList, stream, texDict)) {
            RwTexDictionaryForAllTextures(texDict, destroyTexture, NULL);
            RwTexDictionaryDestroy(texDict);
            return NULL;
        }

        return texDict;
    }

    RWERROR((E_RW_BADVERSION));
    return NULL;
}

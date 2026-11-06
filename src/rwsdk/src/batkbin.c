/*
 * Plugin data chunks: each plugin can stream its own extension chunk inside
 * an object's rwID_EXTENSION chunk.
 */

#include "rwsdk/rwcore.h"
#include "rwsdk/batkreg.h"

#define rwPLUGIN_ID 1 /* core */

static inline RwPluginRegEntry* PluginRegistryFindEntry(const RwPluginRegistry* reg, RwUInt32 pluginID) {
    RwPluginRegEntry* entry = reg->firstRegEntry;

    while (entry) {
        if (entry->pluginID == pluginID) {
            break;
        }
        entry = entry->nextRegEntry;
    }

    return entry;
}

RwInt32 _rwPluginRegistryAddPluginStream(RwPluginRegistry* reg, RwUInt32 pluginID,
                                         RwPluginDataChunkReadCallBack readCB,
                                         RwPluginDataChunkWriteCallBack writeCB,
                                         RwPluginDataChunkGetSizeCallBack getSizeCB) {
    RwPluginRegEntry* entry = PluginRegistryFindEntry(reg, pluginID);

    if (entry) {
        entry->readCB = readCB;
        entry->writeCB = writeCB;
        entry->getSizeCB = getSizeCB;
        return entry->offset;
    }

    return -1;
}

RwInt32 _rwPluginRegistryAddPlgnStrmlwysCB(RwPluginRegistry* reg, RwUInt32 pluginID,
                                           RwPluginDataChunkAlwaysCallBack alwaysCB) {
    RwPluginRegEntry* entry = PluginRegistryFindEntry(reg, pluginID);

    if (entry) {
        entry->alwaysCB = alwaysCB;
        return entry->offset;
    }

    return -1;
}

RwInt32 _rwPluginRegistryAddPlgnStrmRightsCB(RwPluginRegistry* reg, RwUInt32 pluginID,
                                             RwPluginDataChunkRightsCallBack rightsCB) {
    RwPluginRegEntry* entry = PluginRegistryFindEntry(reg, pluginID);

    if (entry) {
        entry->rightsCB = rightsCB;
        return entry->offset;
    }

    return -1;
}

const RwPluginRegistry* _rwPluginRegistryReadDataChunks(const RwPluginRegistry* reg, RwStream* stream,
                                                        void* object) {
    RwUInt32 length;
    RwUInt32 version;

    if (!RwStreamFindChunk(stream, rwID_EXTENSION, &length, &version)) {
        return NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION) {
        RwPluginRegEntry* entry;

        while (length) {
            RwUInt32 type;
            RwUInt32 size;

            if (!_rwStreamReadChunkHeader(stream, &type, &size, NULL, NULL)) {
                return NULL;
            }

            entry = reg->firstRegEntry;
            while (entry) {
                if (entry->pluginID == type) {
                    break;
                }
                entry = entry->nextRegEntry;
            }
            if (entry && entry->readCB) {
                if (!entry->readCB(stream, size, object, entry->offset, entry->size)) {
                    return NULL;
                }
            } else {
                if (!RwStreamSkip(stream, size)) {
                    return NULL;
                }
            }

            length -= size + rwCHUNKHEADERSIZE;
        }

        entry = reg->firstRegEntry;
        while (entry) {
            if (entry->alwaysCB) {
                if (!entry->alwaysCB(object, entry->offset, entry->size)) {
                    return NULL;
                }
            }
            entry = entry->nextRegEntry;
        }

        return reg;
    }

    RWERROR((E_RW_BADVERSION));
    return NULL;
}

const RwPluginRegistry* _rwPluginRegistryInvokeRights(const RwPluginRegistry* reg, RwUInt32 id, void* obj,
                                                      RwUInt32 extraData) {
    RwPluginRegEntry* entry = PluginRegistryFindEntry(reg, id);

    if (entry && entry->rightsCB) {
        if (entry->rightsCB(obj, entry->offset, entry->size, extraData)) {
            return reg;
        }
        return NULL;
    }

    return NULL;
}

RwInt32 _rwPluginRegistryGetSize(const RwPluginRegistry* reg, const void* object) {
    RwInt32 size = 0;
    RwPluginRegEntry* entry = reg->firstRegEntry;

    while (entry) {
        if (entry->getSizeCB) {
            RwInt32 pluginSize = entry->getSizeCB(object, entry->offset, entry->size);

            if (pluginSize > 0) {
                size += pluginSize + rwCHUNKHEADERSIZE;
            }
        }
        entry = entry->nextRegEntry;
    }

    return size;
}

const RwPluginRegistry* _rwPluginRegistryWriteDataChunks(const RwPluginRegistry* reg, RwStream* stream,
                                                         const void* object) {
    RwPluginRegEntry* entry;

    if (!RwStreamWriteChunkHeader(stream, rwID_EXTENSION, _rwPluginRegistryGetSize(reg, object))) {
        return NULL;
    }

    entry = reg->firstRegEntry;
    while (entry) {
        if (entry->getSizeCB && entry->writeCB) {
            RwInt32 size = entry->getSizeCB(object, entry->offset, entry->size);

            if (size > 0) {
                if (!RwStreamWriteChunkHeader(stream, entry->pluginID, size)) {
                    return NULL;
                }
                if (!entry->writeCB(stream, size, object, entry->offset, entry->size)) {
                    return NULL;
                }
            }
        }
        entry = entry->nextRegEntry;
    }

    return reg;
}

const RwPluginRegistry* _rwPluginRegistrySkipDataChunks(const RwPluginRegistry* reg, RwStream* stream) {
    RwUInt32 length;

    if (!RwStreamFindChunk(stream, rwID_EXTENSION, &length, NULL)) {
        return NULL;
    }

    while (length) {
        RwUInt32 size;

        if (!_rwStreamReadChunkHeader(stream, NULL, &size, NULL, NULL)) {
            return NULL;
        }
        if (!RwStreamSkip(stream, size)) {
            return NULL;
        }

        length -= size + rwCHUNKHEADERSIZE;
    }

    return reg;
}

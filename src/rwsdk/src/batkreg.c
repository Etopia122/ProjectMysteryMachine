/*
 * Plugin registry: lets plugins reserve space in an object type and hook its
 * construction, destruction and copying.
 */

#include "rwsdk/rwcore.h"
#include "rwsdk/batkreg.h"

#define rwPLUGIN_ID 1 /* core */

/* Declared in reverse: .sbss holds them in the opposite order. */
static RwFreeList* pluginEntryFreeList;
static RwUInt32 numRegistries;
static RwPluginRegistry** registryList;

RwBool _rwPluginRegistryOpen(void) {
    pluginEntryFreeList = RwFreeListCreate(sizeof(RwPluginRegEntry), 20, 0);
    if (!pluginEntryFreeList) {
        return FALSE;
    }

    numRegistries = 0;
    return TRUE;
}

static void rwDestroyEntry(void* pMem, void* pData) {
    RwPluginRegEntry* entry = (RwPluginRegEntry*)pMem;

    if (entry->parentRegistry->firstRegEntry) {
        entry->parentRegistry->sizeOfStruct = entry->parentRegistry->origSizeOfStruct;
        entry->parentRegistry->firstRegEntry = NULL;
        entry->parentRegistry->lastRegEntry = NULL;
    }

    RwFreeListFree((RwFreeList*)pData, entry);
}

RwBool _rwPluginRegistryClose(void) {
    if (pluginEntryFreeList) {
        RwFreeListForAllUsed(pluginEntryFreeList, rwDestroyEntry, pluginEntryFreeList);

        if (RWSRCGLOBAL(memoryAlloc) != _rwFreeListAllocReal) {
            RwUInt32 i;

            for (i = 0; i < numRegistries; i++) {
                RwPluginRegistry* reg = registryList[i];
                RwPluginRegEntry* entry = reg->firstRegEntry;
                RwPluginRegistry* parent = NULL;

                if (entry) {
                    parent = entry->parentRegistry;
                }

                while (entry) {
                    RwPluginRegEntry* next = entry->nextRegEntry;

                    RwFreeListFree(NULL, entry);
                    entry = next;
                }

                if (parent && parent->firstRegEntry) {
                    parent->sizeOfStruct = parent->origSizeOfStruct;
                    parent->firstRegEntry = NULL;
                    parent->lastRegEntry = NULL;
                }
            }

            if (registryList) {
                RwFree(registryList);
                registryList = NULL;
            }
        }

        RwFreeListDestroy(pluginEntryFreeList);
        pluginEntryFreeList = NULL;
    }

    return TRUE;
}

static void* PluginDefaultConstructor(void* object, RwInt32 offset, RwInt32 size) {
    return object;
}

static void* PluginDefaultDestructor(void* object, RwInt32 offset, RwInt32 size) {
    return object;
}

static void* PluginDefaultCopy(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size) {
    return dstObject;
}

RwInt32 _rwPluginRegistryGetPluginOffset(const RwPluginRegistry* reg, RwUInt32 pluginID) {
    RwPluginRegEntry* entry = reg->firstRegEntry;

    while (entry) {
        if (entry->pluginID == pluginID) {
            return entry->offset;
        }
        entry = entry->nextRegEntry;
    }

    return -1;
}

RwInt32 _rwPluginRegistryAddPlugin(RwPluginRegistry* reg, RwInt32 size, RwUInt32 pluginID,
                                   RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB,
                                   RwPluginObjectCopy copyCB) {
    RwPluginRegEntry* entry;
    RwInt32 newSize;

    if (!pluginEntryFreeList) {
        return -1;
    }

    if (_rwGetNumEngineInstances()) {
        RWERROR((E_RW_PLUGININIT));
        return -1;
    }

    if (RWSRCGLOBAL(memoryAlloc) != _rwFreeListAllocReal) {
        RwUInt32 i;

        for (i = 0; i < numRegistries; i++) {
            if (reg == registryList[i]) {
                break;
            }
        }

        if (numRegistries == i) {
            RwPluginRegistry** newList = (RwPluginRegistry**)RwMalloc((numRegistries + 1) * sizeof(RwPluginRegistry*));
            RwUInt32 j = 0;

            if (registryList) {
                while (j < numRegistries) {
                    newList[j] = registryList[j];
                    j++;
                }
                RwFree(registryList);
                registryList = NULL;
            }

            newList[j] = reg;
            registryList = newList;
            numRegistries++;
        }
    }

    entry = reg->firstRegEntry;
    while (entry) {
        if (entry->pluginID == pluginID) {
            RWERROR((E_RW_PLUGININIT));
            return entry->offset;
        }
        entry = entry->nextRegEntry;
    }

    newSize = reg->sizeOfStruct + ((size + 3) & ~3);
    if (reg->maxSizeOfStruct && newSize > reg->maxSizeOfStruct) {
        return -1;
    }

    entry = (RwPluginRegEntry*)RwFreeListAlloc(pluginEntryFreeList);
    if (entry) {
        entry->offset = reg->sizeOfStruct;
        reg->sizeOfStruct = newSize;
        entry->size = size;
        entry->pluginID = pluginID;
        entry->readCB = NULL;
        entry->writeCB = NULL;
        entry->getSizeCB = NULL;
        entry->alwaysCB = NULL;
        entry->rightsCB = NULL;
        entry->constructCB = constructCB ? constructCB : PluginDefaultConstructor;
        entry->destructCB = destructCB ? destructCB : PluginDefaultDestructor;
        entry->copyCB = copyCB ? copyCB : PluginDefaultCopy;
        entry->errStrCB = NULL;
        entry->nextRegEntry = NULL;
        entry->prevRegEntry = NULL;
        entry->parentRegistry = reg;

        if (!reg->firstRegEntry) {
            reg->firstRegEntry = entry;
            reg->lastRegEntry = entry;
        } else {
            reg->lastRegEntry->nextRegEntry = entry;
            entry->prevRegEntry = reg->lastRegEntry;
            reg->lastRegEntry = entry;
        }

        return entry->offset;
    }

    return -1;
}

const RwPluginRegistry* _rwPluginRegistryInitObject(const RwPluginRegistry* reg, void* object) {
    RwPluginRegEntry* entry = reg->firstRegEntry;

    while (entry) {
        if (!entry->constructCB(object, entry->offset, entry->size)) {
            entry = entry->prevRegEntry;
            while (entry) {
                entry->destructCB(object, entry->offset, entry->size);
                entry = entry->prevRegEntry;
            }
            return NULL;
        }
        entry = entry->nextRegEntry;
    }

    return reg;
}

const RwPluginRegistry* _rwPluginRegistryDeInitObject(const RwPluginRegistry* reg, void* object) {
    RwPluginRegEntry* entry = reg->lastRegEntry;

    while (entry) {
        entry->destructCB(object, entry->offset, entry->size);
        entry = entry->prevRegEntry;
    }

    return reg;
}

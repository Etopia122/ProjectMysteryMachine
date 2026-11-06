/*
 * Resources: the arena that caches instanced data. Entries live on two
 * lists, those used this frame and those that can be thrown out when the
 * arena runs short.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 1 /* core */

#define E_RW_RESOURCES 0x0C

typedef struct RwResEntry RwResEntry;
typedef void (*RwResEntryDestroyNotify)(RwResEntry* resEntry);

struct RwResEntry {
    RwLLLink link;
    RwInt32 size;
    void* owner;
    RwResEntry** ownerRef;
    RwResEntryDestroyNotify destroyNotify;
};

typedef struct rwResourcesGlobals {
    RwUInt32 maxSize;
    RwUInt32 currentSize;
    RwUInt32 reusageSize;
    void* memHeap;
    RwLinkList entriesA;
    RwLinkList entriesB;
    RwLinkList* freeEntries;
    RwLinkList* usedEntries;
} rwResourcesGlobals;

RwBool _rwResHeapInit(void* mem, RwUInt32 size);
RwBool _rwResHeapClose(void* heap);
void _rwResHeapFree(void* alloc);
void* _rwResHeapAlloc(void* mem, RwUInt32 size);

RwBool RwResourcesEmptyArena(void);

RwModuleInfo resourcesModule;

#define RWRESOURCESGLOBALS() RWPLUGINOFFSET(rwResourcesGlobals, RwEngineInstance, resourcesModule.globalsOffset)
#define RWRESOURCESGLOBAL(var) (RWRESOURCESGLOBALS()->var)

static inline rwResourcesGlobals* ResourcesInitialize(rwResourcesGlobals* globals, RwUInt32 size) {
    if (size) {
        globals->memHeap = RwMalloc(size);
        if (!globals->memHeap) {
            RWERROR((E_RW_NOMEM, size));
            return NULL;
        }

        if (!_rwResHeapInit(globals->memHeap, size)) {
            RwFree(globals->memHeap);
            RWERROR((E_RW_RESOURCES, 0));
            return NULL;
        }
    } else {
        globals->memHeap = NULL;
    }

    rwLinkListInitialize(&globals->entriesA);
    rwLinkListInitialize(&globals->entriesB);
    globals->usedEntries = &globals->entriesA;
    globals->freeEntries = &globals->entriesB;
    globals->maxSize = size;
    globals->currentSize = 0;
    globals->reusageSize = 0;

    return globals;
}

void* _rwResourcesOpen(void* instance, RwInt32 offset, RwInt32 size) {
    rwResourcesGlobals* globals;

    resourcesModule.globalsOffset = offset;

    globals = ResourcesInitialize(RWRESOURCESGLOBALS(), RWSRCGLOBAL(resArenaInitSize));
    if (!globals) {
        return NULL;
    }

    resourcesModule.numInstances++;

    return instance;
}

void* _rwResourcesClose(void* instance, RwInt32 offset, RwInt32 size) {
    RwResourcesEmptyArena();

    _rwResHeapClose(RWRESOURCESGLOBAL(memHeap));
    RwFree(RWRESOURCESGLOBAL(memHeap));
    RWRESOURCESGLOBAL(memHeap) = NULL;

    resourcesModule.numInstances--;

    return instance;
}

RwBool RwResourcesFreeResEntry(RwResEntry* entry) {
    if (entry->destroyNotify) {
        entry->destroyNotify(entry);
    }

    if (entry->ownerRef) {
        *entry->ownerRef = NULL;
    }

    if (entry->link.next) {
        rwLLLinkRemove(&entry->link);
        RWRESOURCESGLOBAL(currentSize) -= entry->size;
        _rwResHeapFree(entry);
    } else {
        RwFree(entry);
    }

    return TRUE;
}

void _rwResourcesPurge(void) {
    rwResourcesGlobals* globals = RWRESOURCESGLOBALS();
    RwLinkList* usedEntries = globals->usedEntries;
    RwLinkList* freeEntries = globals->freeEntries;

    if (freeEntries->link.next != &freeEntries->link) {
        if (usedEntries->link.next == &usedEntries->link) {
            usedEntries->link.next = freeEntries->link.next;
            usedEntries->link.next->prev = &usedEntries->link;
            usedEntries->link.prev = freeEntries->link.prev;
            usedEntries->link.prev->next = &usedEntries->link;
            rwLinkListInitialize(freeEntries);
        } else {
            RwLLLink* first = freeEntries->link.next;
            RwLLLink* last = usedEntries->link.prev;

            last->next = first;
            first->prev = last;
            last = freeEntries->link.prev;
            last->next = &usedEntries->link;
            usedEntries->link.prev = last;
            rwLinkListInitialize(freeEntries);
        }
    }

    RWRESOURCESGLOBAL(usedEntries) = freeEntries;
    RWRESOURCESGLOBAL(freeEntries) = usedEntries;
    RWRESOURCESGLOBAL(reusageSize) = 0;
}

RwResEntry* RwResourcesAllocateResEntry(void* owner, RwResEntry** ownerRef, RwInt32 size,
                                        RwResEntryDestroyNotify destroyNotify) {
    RwBool failed = FALSE;

    while (!failed) {
        RwResEntry* entry = (RwResEntry*)_rwResHeapAlloc(RWRESOURCESGLOBAL(memHeap), size + sizeof(RwResEntry));

        if (entry) {
            rwLinkListAddLLLink(RWRESOURCESGLOBAL(usedEntries), &entry->link);
            entry->owner = owner;
            entry->size = size;
            entry->ownerRef = ownerRef;
            entry->destroyNotify = destroyNotify;
            RWRESOURCESGLOBAL(currentSize) += size;

            if (ownerRef) {
                *ownerRef = entry;
            }

            return entry;
        } else {
            rwResourcesGlobals* globals = RWRESOURCESGLOBALS();
            RwResEntry* victim;

            if (rwLinkListGetLastLLLink(globals->freeEntries) != rwLinkListGetTerminator(globals->freeEntries)) {
                victim = (RwResEntry*)rwLinkListGetLastLLLink(globals->freeEntries);
                RwResourcesFreeResEntry(victim);
            } else {
                victim = (RwResEntry*)globals->usedEntries->link.prev;
                if (&victim->link != &globals->usedEntries->link) {
                    globals->reusageSize += victim->size;
                    RwResourcesFreeResEntry(victim);
                } else {
                    failed = TRUE;
                }
            }
        }
    }

    if (ownerRef) {
        *ownerRef = NULL;
    }

    RWERROR((E_RW_RESOURCES, size));

    return NULL;
}

RwBool RwResourcesSetArenaSize(RwUInt32 size) {
    rwResourcesGlobals* globals;

    if (!resourcesModule.numInstances) {
        RWSRCGLOBAL(resArenaInitSize) = size;
        return TRUE;
    }

    globals = RWRESOURCESGLOBALS();
    globals->maxSize = size;

    RwResourcesEmptyArena();
    _rwResHeapClose(globals->memHeap);
    RwFree(globals->memHeap);

    globals->memHeap = RwMalloc(size);
    if (!globals->memHeap) {
        globals->maxSize = 0;
        RWERROR((E_RW_NOMEM, size));
        return FALSE;
    }

    if (!_rwResHeapInit(globals->memHeap, size)) {
        RwFree(globals->memHeap);
        RWERROR((E_RW_RESOURCES, 0));
        return FALSE;
    }

    return TRUE;
}

RwUInt32 RwResourcesGetArenaSize(void) {
    return RWRESOURCESGLOBAL(maxSize);
}

RwBool RwResourcesEmptyArena(void) {
    rwResourcesGlobals* globals = RWRESOURCESGLOBALS();
    RwLLLink* cur;
    RwLLLink* end;

    /* Chain the two lists together and walk them both in one go. */
    globals->entriesA.link.prev->next = globals->entriesB.link.next;

    globals = RWRESOURCESGLOBALS();
    cur = globals->entriesA.link.next;
    end = &globals->entriesB.link;
    while (cur != end) {
        RwResEntry* entry = (RwResEntry*)cur;

        cur = cur->next;
        RwResourcesFreeResEntry(entry);
    }

    rwLinkListInitialize(&RWRESOURCESGLOBAL(entriesA));
    rwLinkListInitialize(&RWRESOURCESGLOBAL(entriesB));
    RWRESOURCESGLOBAL(reusageSize) = 0;

    return TRUE;
}

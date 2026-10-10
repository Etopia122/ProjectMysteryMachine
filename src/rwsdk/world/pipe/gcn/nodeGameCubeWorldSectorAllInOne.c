/*
 * GameCubeWorldSectorAllInOne.csl: the whole world sector pipeline in one
 * node. World geometry is static, so a sector is instanced once into display
 * lists and GX vertex arrays, then lit and drawn through the callbacks.
 */

#include "rwsdk/gcallinone_int.h"

#define rpWORLDNATIVE 0x01000000
#define rpWORLDNATIVEINSTANCE 0x02000000

#define GX_INDEX8 2
#define GX_INDEX16 3

struct RwResEntry {
    /* 0x00 */ RwLLLink link;
    /* 0x08 */ RwInt32 size;
    /* 0x0C */ void* owner;
    /* 0x10 */ RwResEntry** ownerRef;
    /* 0x14 */ void (*destroyNotify)(RwResEntry* resEntry);
};

typedef struct rwResourcesGlobals {
    /* 0x00 */ RwUInt32 maxSize;
    /* 0x04 */ RwUInt32 currentSize;
    /* 0x08 */ RwUInt32 reusageSize;
    /* 0x0C */ void* memHeap;
    /* 0x10 */ RwLinkList entriesA;
    /* 0x18 */ RwLinkList entriesB;
    /* 0x20 */ RwLinkList* freeEntries;
    /* 0x24 */ RwLinkList* usedEntries;
} rwResourcesGlobals;

extern RwModuleInfo resourcesModule;

#define RWRESOURCESGLOBAL(var) \
    (RWPLUGINOFFSET(rwResourcesGlobals, RwEngineInstance, resourcesModule.globalsOffset)->var)

#define RwResourcesUseResEntry(_ntry)                                          \
    do {                                                                       \
        if ((_ntry)->link.next) {                                              \
            rwLLLinkRemove(&(_ntry)->link);                                    \
            rwLinkListAddLLLink(RWRESOURCESGLOBAL(usedEntries), &(_ntry)->link); \
        }                                                                      \
    } while (0)

extern volatile RwUInt16 lbl_8025F890;

RpGameCubeVtxFmt* RpGameCubeWorldGetVtxFmt(RpWorld* world);
RwResEntry* RwResourcesAllocateResEntry(void* owner, RwResEntry** ownerRef, RwInt32 size,
                                        void (*destroyNotify)(RwResEntry* resEntry));
RwBool RwResourcesFreeResEntry(RwResEntry* entry);
void _rxGCSetupGlobalLights(RwInt32 reset);
void _rxGCSetupLocalLight(RpLight* light, RxGameCubeRenderData* data);
RwUInt32 _rxGCDisplayListsSize(RpMeshHeader* meshHeader, RwInt32 numTexCoords, RwInt32 indexSize, RwUInt32 flags);
void _rxGCBuildDisplayLists(RxGameCubeDisplayList* displayLists, RwUInt8* dst, RpMeshHeader* meshHeader, RwInt32 numTexCoords,
                 RwInt32 indexSize, RwUInt32 flags);

void DCFlushRange(void* addr, RwUInt32 nBytes);
void* memset(void* dst, RwInt32 c, RwUInt32 n);

RpWorldSector* _rxGCWorldSectorAllInOneLighting(RpWorldSector* sector, RxGameCubeRenderData* data)
{
    RwLLLink* cur;
    RwLLLink* end;

    data->lightMask0 = 0;
    data->lightMask1 = 0;
    data->ambient[0] = 0.0f;
    data->ambient[1] = 0.0f;
    data->ambient[2] = 0.0f;
    data->ambient[3] = 1.0f;
    data->ambientLight = FALSE;
    data->numLights = 0;

    if (((RpWorld*)RWSRCGLOBAL(curWorld))->flags & rpWORLDLIGHT) {
        _rxGCSetupGlobalLights(2);
        RWSRCGLOBAL(lightFrame)++;

        end = rwLinkListGetTerminator(&sector->lightsInWorldSector);
        cur = rwLinkListGetFirstLLLink(&sector->lightsInWorldSector);
        while (cur != end) {
            RpLightTie* tie = rwLLLinkGetData(cur, RpLightTie, lightInWorldSector);

            if (tie->light && (tie->light->object.object.flags & rpLIGHTLIGHTWORLD)) {
                _rxGCSetupLocalLight(tie->light, data);
            }
            cur = rwLLLinkGetNext(cur);
        }
    }

    if (data->ambientLight) {
        if (data->ambient[0] > 1.0f) {
            data->ambient[0] = 1.0f;
        }
        if (data->ambient[1] > 1.0f) {
            data->ambient[1] = 1.0f;
        }
        if (data->ambient[2] > 1.0f) {
            data->ambient[2] = 1.0f;
        }
    }

    return sector;
}

void* _rxGCWorldSectorAllInOneInstance(void* object, RxGameCubeRenderData* data)
{
    RwUInt32 dlArraySize;
    RpWorldSector* sector;
    RwUInt32 dlSize;
    RpWorld* world;
    RwUInt8* dst;
    RwUInt32 size;
    RpMeshHeader* meshHeader;
    RxGameCubeInstanceData* inst;

    RwUInt32 numVerts;
    RwUInt32 flags;
    RwUInt32 arrayFlags;
    sector = (RpWorldSector*)object;
    world = (RpWorld*)RWSRCGLOBAL(curWorld);
    numVerts = sector->numVertices;
    meshHeader = sector->mesh;
    flags = world->flags | rpGEOMETRYPOSITIONS;
    arrayFlags = flags & ~data->pvtData->unk10;
    dlArraySize = meshHeader->numMeshes * sizeof(RxGameCubeDisplayList);
    dlSize = _rxGCDisplayListsSize(meshHeader, world->numTexCoordSets, (numVerts < 0xFF) ? 1 : 2, flags);

    size = (dlArraySize + (sizeof(RxGameCubeInstanceData) - sizeof(RxGameCubeDisplayList)) + 31) & ~31;
    size += dlSize;
    size = ((size + 31) & ~31) +
           _rxGCVertexArraySize(RpGameCubeWorldGetVtxFmt(world), arrayFlags, numVerts, world->numTexCoordSets);

    if (world->flags & rpWORLDNATIVEINSTANCE) {
        data->repEntry = (RwResEntry*)RwMalloc(size + sizeof(RwResEntry) + 32);
        data->repEntry->link.next = NULL;
        data->repEntry->link.prev = NULL;
        data->repEntry->owner = sector;
        data->repEntry->size = size + 32;
        data->repEntry->ownerRef = &sector->repEntry;
        data->repEntry->destroyNotify = _rxGCResEntryWaitDone;
        sector->repEntry = data->repEntry;
    } else {
        data->repEntry = RwResourcesAllocateResEntry(sector, &sector->repEntry, size + 32, _rxGCResEntryWaitDone);
    }

    inst = RESENTRYDATA(data->repEntry);
    memset(inst, 0, sizeof(RxGameCubeInstanceData) - sizeof(RxGameCubeDisplayList));
    inst->serialNum = meshHeader->serialNum;
    inst->descType = (numVerts < 0xFF) ? GX_INDEX8 : GX_INDEX16;
    inst->token = lbl_8025F890;
    inst->unk14 = data->pvtData->unk10;

    dst = (RwUInt8*)inst + dlArraySize;
    dst = (RwUInt8*)(((RwUInt32)dst + (sizeof(RxGameCubeInstanceData) - sizeof(RxGameCubeDisplayList)) + 31) & ~31);
    _rxGCBuildDisplayLists(inst->displayLists, dst, meshHeader, world->numTexCoordSets, (numVerts < 0xFF) ? 1 : 2, flags);
    dst = (RwUInt8*)(((RwUInt32)dst + dlSize + 31) & ~31);
    _rxGCInstanceWorldSectorArrays(sector, inst, RpGameCubeWorldGetVtxFmt(world), arrayFlags, world->numTexCoordSets,
                                   dst);

    DCFlushRange(RESENTRYDATA(data->repEntry), data->repEntry->size);

    return sector;
}

RwBool _rxGCWorldSectorAllInOneNode(RxPipelineNodeInstance* self, const RxPipelineNodeParam* params)
{
    RxGameCubeRenderData data;
    RwUInt32 flags;
    RpWorldSector* sector = (RpWorldSector*)params->dataParam;
    RxGameCubeAllInOnePvtData* pvt = (RxGameCubeAllInOnePvtData*)self->privateData;

    data.pvtData = pvt;
    flags = ((RpWorld*)RWSRCGLOBAL(curWorld))->flags;
    if (!(flags & rpWORLDNATIVE)) {
        RpMeshHeader* meshHeader;

        if (!sector->numVertices) {
            return TRUE;
        }
        meshHeader = sector->mesh;
        if (!meshHeader->numMeshes) {
            return TRUE;
        }

        data.repEntry = sector->repEntry;
        data.meshHeader = meshHeader;
        data.flags = flags;

        if (data.repEntry && RESENTRYDATA(data.repEntry)->serialNum != meshHeader->serialNum) {
            RwResourcesFreeResEntry(data.repEntry);
            data.repEntry = NULL;
        }

        if (data.repEntry) {
            RwResourcesUseResEntry(data.repEntry);
        } else if (pvt->instanceCB && pvt->instanceCB(sector, &data) != sector) {
            return FALSE;
        }
    } else {
        data.repEntry = sector->repEntry;
        data.meshHeader = sector->mesh;
        data.flags = flags;
    }

    if (pvt->lightingCB && pvt->lightingCB(sector, &data) != sector) {
        return FALSE;
    }
    if (pvt->renderCB && pvt->renderCB(sector, &data) != sector) {
        return FALSE;
    }

    return TRUE;
}

RwBool _rxGCWorldSectorAllInOnePipelineInit(RxPipelineNode* node)
{
    RxGameCubeAllInOnePvtData* pvt = (RxGameCubeAllInOnePvtData*)node->privateData;

    pvt->instanceCB = _rxGCWorldSectorAllInOneInstance;
    pvt->reinstanceCB = NULL;
    pvt->lightingCB = (RxGameCubeAllInOneCallBack)_rxGCWorldSectorAllInOneLighting;
    pvt->renderCB = _rxGCDefaultRender;
    pvt->unk10 = 0;

    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetGameCubeWorldSectorAllInOne(void)
{
    static RwChar _GameCubeWorldSectorAllInOne_csl[] = "GamerCubeWorldSectorAllInOne.csl";

    static RxNodeDefinition nodeGameCubeWorldSectorAllInOneCSL = {
        _GameCubeWorldSectorAllInOne_csl,
        { _rxGCWorldSectorAllInOneNode, NULL, NULL, _rxGCWorldSectorAllInOnePipelineInit, NULL, NULL, NULL },
        { 0, NULL, NULL, 0, NULL },
        sizeof(RxGameCubeAllInOnePvtData),
        0,
        0,
    };

    return &nodeGameCubeWorldSectorAllInOneCSL;
}

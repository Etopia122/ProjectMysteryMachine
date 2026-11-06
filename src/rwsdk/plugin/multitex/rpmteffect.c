/*
 * Multi-texture effects: reference counted, named effect descriptions kept
 * in dictionaries. Effects not found in a dictionary are loaded from
 * "<effect path><name>" through the owning platform's stream reader.
 */

#include "rwsdk/rpworld_int.h"

#define rwPLUGIN_ID 0x120 /* reported under material effects */

#define rwID_MTEFFECTNATIVE 0x20

#define rwSTREAMFILENAME 2
#define rwSTREAMREAD 1

#define rpMTEFFECTMAXPLATFORMS 9
#define rpMTEFFECTNAMELENGTH 32
#define rpMTEFFECTPATHLENGTH 0x100

typedef struct RpMTEffect RpMTEffect;
typedef struct RpMTEffectDict RpMTEffectDict;

typedef RpMTEffect* (*RpMTEffectStreamReadCallBack)(RwStream* stream, RwInt32 platform, RwUInt32 version,
                                                    RwUInt32 length);
typedef void* RpMTEffectStreamWriteCallBack;
typedef void* RpMTEffectStreamGetSizeCallBack;
typedef void (*RpMTEffectDestroyCallBack)(RpMTEffect* effect);

struct RpMTEffect {
    /* 0x00 */ RwInt32 platform;
    /* 0x04 */ RwUInt32 refCount;
    /* 0x08 */ RwChar name[rpMTEFFECTNAMELENGTH];
    /* 0x28 */ RwLLLink inDict;
};

struct RpMTEffectDict {
    /* 0x00 */ RwLinkList effectList;
    /* 0x08 */ RwLLLink lInInstance;
};

typedef struct RpMTEffectPlatformInfo {
    /* 0x00 */ RwInt32 platform;
    /* 0x04 */ RpMTEffectDestroyCallBack destroyCB;
    /* 0x08 */ RpMTEffectStreamReadCallBack readCB;
    /* 0x0C */ RpMTEffectStreamWriteCallBack writeCB;
    /* 0x10 */ RpMTEffectStreamGetSizeCallBack getSizeCB;
} RpMTEffectPlatformInfo;

typedef struct rwMTEffectGlobals {
    /* 0x00 */ RwLinkList dictList;
    /* 0x08 */ RpMTEffectDict* currentDict;
    /* 0x0C */ RwUInt32 pathSize;
    /* 0x10 */ RwChar* path;
    /* 0x14 */ RwChar* fileName;
} rwMTEffectGlobals;

extern RwModuleInfo MultiTextureModule;

#define RWMTEFFECTGLOBAL(var) (RWPLUGINOFFSET(rwMTEffectGlobals, RwEngineInstance, MultiTextureModule.globalsOffset)->var)

#define MTEFFECTFROMDICTLINK(link) ((RpMTEffect*)((RwUInt8*)(link) - 0x28))
#define DICTFROMINSTANCELINK(link) ((RpMTEffectDict*)((RwUInt8*)(link) - 0x8))

void* memset(void* dst, int c, unsigned long n);
RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length);
void* RwMemNative32(void* mem, RwUInt32 size);
RwStream* RwStreamOpen(RwInt32 type, RwInt32 accessType, const void* data);
RwBool RwStreamClose(RwStream* stream, void* data);
RwChar* _rwStringStreamFindAndRead(RwChar* string, RwStream* stream);

RpMTEffectDict* RpMTEffectDictCreate(void);
void RpMTEffectDictDestroy(RpMTEffectDict* dict);
RpMTEffectDict* RpMTEffectDictAddEffect(RpMTEffectDict* dict, RpMTEffect* effect);
RpMTEffect* RpMTEffectDictRemoveEffect(RpMTEffect* effect);
void RpMTEffectDestroy(RpMTEffect* effect);
RpMTEffect* RpMTEffectSetName(RpMTEffect* effect, const RwChar* name);
void RpMTEffectAddRef(RpMTEffect* effect);

static RpMTEffectPlatformInfo MTEffectPlatformInfo[rpMTEFFECTMAXPLATFORMS];

static RpMTEffectDict* MTEffectDefaultDict;

RwBool _rpMTEffectSystemInit(void)
{
    memset(MTEffectPlatformInfo, 0, sizeof(MTEffectPlatformInfo));
    return TRUE;
}

RwBool _rpMTEffectRegisterPlatform(RwInt32 platform, RpMTEffectStreamReadCallBack readCB,
                                   RpMTEffectStreamWriteCallBack writeCB, RpMTEffectStreamGetSizeCallBack getSizeCB,
                                   RpMTEffectDestroyCallBack destroyCB)
{
    RpMTEffectPlatformInfo* info = &MTEffectPlatformInfo[platform];

    info->platform = platform;
    info->readCB = readCB;
    info->writeCB = writeCB;
    info->getSizeCB = getSizeCB;
    info->destroyCB = destroyCB;
    return TRUE;
}

RwBool _rpMTEffectOpen(void)
{
    RwChar* path;

    RWMTEFFECTGLOBAL(dictList).link.next = &RWMTEFFECTGLOBAL(dictList).link;
    RWMTEFFECTGLOBAL(dictList).link.prev = &RWMTEFFECTGLOBAL(dictList).link;

    MTEffectDefaultDict = RpMTEffectDictCreate();
    if (MTEffectDefaultDict == NULL) {
        return FALSE;
    }
    RWMTEFFECTGLOBAL(currentDict) = MTEffectDefaultDict;

    path = (RwChar*)RwMalloc(2 * rpMTEFFECTPATHLENGTH + rpMTEFFECTNAMELENGTH);
    if (path == NULL) {
        RpMTEffectDictDestroy(RWMTEFFECTGLOBAL(currentDict));
        RWERROR((E_RW_NOMEM, 2 * rpMTEFFECTPATHLENGTH + rpMTEFFECTNAMELENGTH));
        return FALSE;
    }

    memset(path, 0, 2 * rpMTEFFECTPATHLENGTH + rpMTEFFECTNAMELENGTH);
    RWMTEFFECTGLOBAL(path) = path;
    RWMTEFFECTGLOBAL(fileName) = path + rpMTEFFECTPATHLENGTH;
    RWMTEFFECTGLOBAL(pathSize) = rpMTEFFECTPATHLENGTH;
    return TRUE;
}

RwBool _rpMTEffectClose(void)
{
    RwLLLink* cur;
    RwLinkList* dictList;

    if (RWMTEFFECTGLOBAL(path)) {
        RwFree(RWMTEFFECTGLOBAL(path));
        RWMTEFFECTGLOBAL(path) = NULL;
        RWMTEFFECTGLOBAL(fileName) = NULL;
        RWMTEFFECTGLOBAL(pathSize) = 0;
    }

    dictList = &RWMTEFFECTGLOBAL(dictList);
    for (cur = dictList->link.next; cur != &dictList->link; cur = cur->next) {
        if (DICTFROMINSTANCELINK(cur) == MTEffectDefaultDict) {
            RpMTEffectDictDestroy(MTEffectDefaultDict);
            MTEffectDefaultDict = NULL;
            break;
        }
    }
    return TRUE;
}

static inline RpMTEffect* MTEffectInit(RpMTEffect* effect, RwInt32 platform)
{
    memset(effect, 0, sizeof(RpMTEffect));
    effect->platform = platform;
    effect->refCount = 1;
    effect->inDict.prev = NULL;
    effect->inDict.next = NULL;

    if (platform && RWMTEFFECTGLOBAL(currentDict)) {
        RpMTEffectDictAddEffect(RWMTEFFECTGLOBAL(currentDict), effect);
    }
    return effect;
}

RpMTEffect* _rpMTEffectInit(RpMTEffect* effect, RwInt32 platform)
{
    return MTEffectInit(effect, platform);
}

RpMTEffectDict* RpMTEffectDictCreate(void)
{
    RpMTEffectDict* dict = (RpMTEffectDict*)RwMalloc(sizeof(RpMTEffectDict));

    if (dict == NULL) {
        RWERROR((E_RW_NOMEM, sizeof(RpMTEffectDict)));
        return NULL;
    }

    dict->effectList.link.next = &dict->effectList.link;
    dict->effectList.link.prev = &dict->effectList.link;

    dict->lInInstance.next = RWMTEFFECTGLOBAL(dictList).link.next;
    dict->lInInstance.prev = &RWMTEFFECTGLOBAL(dictList).link;
    RWMTEFFECTGLOBAL(dictList).link.next->prev = &dict->lInInstance;
    RWMTEFFECTGLOBAL(dictList).link.next = &dict->lInInstance;
    return dict;
}

void RpMTEffectDictDestroy(RpMTEffectDict* dict)
{
    RwLLLink* cur;

    if (dict == RWMTEFFECTGLOBAL(currentDict)) {
        RWMTEFFECTGLOBAL(currentDict) = NULL;
    }

    cur = dict->effectList.link.next;
    while (cur != &dict->effectList.link) {
        RpMTEffect* effect = MTEFFECTFROMDICTLINK(cur);

        cur = cur->next;
        RpMTEffectDictRemoveEffect(effect);
    }

    dict->lInInstance.prev->next = dict->lInInstance.next;
    dict->lInInstance.next->prev = dict->lInInstance.prev;

    RwFree(dict);
}

RpMTEffectDict* RpMTEffectDictAddEffect(RpMTEffectDict* dict, RpMTEffect* effect)
{
    if (effect->inDict.next) {
        effect->inDict.prev->next = effect->inDict.next;
        effect->inDict.next->prev = effect->inDict.prev;
        RpMTEffectDestroy(effect);
    }

    effect->inDict.next = dict->effectList.link.next;
    effect->inDict.prev = &dict->effectList.link;
    dict->effectList.link.next->prev = &effect->inDict;
    dict->effectList.link.next = &effect->inDict;

    RpMTEffectAddRef(effect);
    return dict;
}

RpMTEffect* RpMTEffectDictRemoveEffect(RpMTEffect* effect)
{
    if (effect->inDict.next) {
        effect->inDict.prev->next = effect->inDict.next;
        effect->inDict.next->prev = effect->inDict.prev;
        RpMTEffectDestroy(effect);
    }
    return effect;
}

RpMTEffect* RpMTEffectCreateDummy(void)
{
    RpMTEffect* effect = (RpMTEffect*)RwMalloc(sizeof(RpMTEffect));

    if (effect == NULL) {
        RWERROR((E_RW_NOMEM, sizeof(RpMTEffect)));
        return NULL;
    }

    return MTEffectInit(effect, 0);
}

void RpMTEffectDestroy(RpMTEffect* effect)
{
    effect->refCount--;
    if (effect->refCount == 0) {
        RpMTEffectDictRemoveEffect(effect);

        {
            RpMTEffectPlatformInfo* info;

            if (effect->platform && (info = &MTEffectPlatformInfo[effect->platform])->destroyCB) {
                info->destroyCB(effect);
            } else {
                RwFree(effect);
            }
        }
    }
}

static inline RpMTEffect* MTEffectDictFindNamedEffect(const RpMTEffectDict* dict, const RwChar* name)
{
    const RwLLLink* cur;

    for (cur = dict->effectList.link.next; cur != &dict->effectList.link; cur = cur->next) {
        RpMTEffect* effect = MTEFFECTFROMDICTLINK(cur);

        if (!RWSRCGLOBAL(stringFuncs).rwstrcmp(effect->name, name)) {
            return effect;
        }
    }
    return NULL;
}

RpMTEffect* RpMTEffectFind(const RwChar* name)
{
    rwMTEffectGlobals* globals = RWPLUGINOFFSET(rwMTEffectGlobals, RwEngineInstance, MultiTextureModule.globalsOffset);
    RwChar* fileName;
    RpMTEffect* effect = NULL;
    RwChar effectName[rpMTEFFECTNAMELENGTH];
    RwStream* stream;
    RwUInt32 length, version;
    RwInt32 platform;
    RpMTEffectPlatformInfo* info;

    if (globals->currentDict) {
        effect = MTEffectDictFindNamedEffect(globals->currentDict, name);
    } else {
        RwLLLink* cur;

        for (cur = globals->dictList.link.next; cur != &globals->dictList.link; cur = cur->next) {
            effect = MTEffectDictFindNamedEffect(DICTFROMINSTANCELINK(cur), name);
            if (effect) {
                break;
            }
        }
    }

    if (effect) {
        RpMTEffectAddRef(effect);
        return effect;
    }

    fileName = RWMTEFFECTGLOBAL(fileName);
    RWSRCGLOBAL(stringFuncs).rwstrcpy(fileName, RWMTEFFECTGLOBAL(path));
    RWSRCGLOBAL(stringFuncs).rwstrncat(fileName, name, rpMTEFFECTNAMELENGTH - 1);

    stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, fileName);
    if (stream == NULL) {
        return NULL;
    }

    if (!RwStreamFindChunk(stream, rwID_MTEFFECTNATIVE, NULL, NULL)) {
        RwStreamClose(stream, NULL);
        return NULL;
    }

    if (!RwStreamFindChunk(stream, rwID_STRUCT, NULL, NULL) || !RwStreamRead(stream, &platform, sizeof(RwInt32))) {
        effect = NULL;
    } else {
        RwMemNative32(&platform, sizeof(RwInt32));
        info = &MTEffectPlatformInfo[platform];

        if (info->readCB == NULL) {
            effect = NULL;
        } else if (!_rwStringStreamFindAndRead(effectName, stream)) {
            effect = NULL;
        } else if (!RwStreamFindChunk(stream, rwID_EXTENSION, &length, &version)) {
            effect = NULL;
        } else {
            effect = info->readCB(stream, platform, version, length);
            if (effect == NULL) {
                effect = NULL;
            } else {
                RpMTEffectSetName(effect, effectName);
            }
        }
    }

    RwStreamClose(stream, NULL);
    return effect;
}

RpMTEffect* RpMTEffectSetName(RpMTEffect* effect, const RwChar* name)
{
    RWSRCGLOBAL(stringFuncs).rwstrncpy(effect->name, name, rpMTEFFECTNAMELENGTH - 1);
    return effect;
}

void RpMTEffectAddRef(RpMTEffect* effect)
{
    effect->refCount++;
}

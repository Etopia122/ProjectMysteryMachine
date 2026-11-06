/*
 * Streams: one interface over files, memory blocks and custom callbacks.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 1 /* core */

#define E_RW_ENDOFSTREAM 0x05
#define E_RW_BADACCESS 0x0D
#define E_RW_BADSTREAMTYPE 0x0E
#define E_RW_BADOPEN 0x80000002
#define E_RW_READ 0x8000001A
#define E_RW_WRITE 0x8000001C

#define rwSTREAMBLOCKSIZE 512

typedef enum RwStreamType {
    rwNASTREAM,
    rwSTREAMFILE,
    rwSTREAMFILENAME,
    rwSTREAMMEMORY,
    rwSTREAMCUSTOM,
} RwStreamType;

typedef enum RwStreamAccessType {
    rwNASTREAMACCESS,
    rwSTREAMREAD,
    rwSTREAMWRITE,
    rwSTREAMAPPEND,
} RwStreamAccessType;

typedef struct RwMemory {
    RwUInt8* start;
    RwUInt32 length;
} RwMemory;

typedef struct RwStreamMemory {
    RwUInt32 position;
    RwUInt32 nSize;
    RwUInt8* memBlock;
} RwStreamMemory;

typedef struct RwStreamFile {
    void* fpFile;
} RwStreamFile;

typedef struct RwStreamCustom {
    RwBool (*sfnclose)(void* data);
    RwUInt32 (*sfnread)(void* data, void* buffer, RwUInt32 length);
    RwBool (*sfnwrite)(void* data, const void* buffer, RwUInt32 length);
    RwBool (*sfnskip)(void* data, RwUInt32 offset);
    void* data;
} RwStreamCustom;

typedef union RwStreamUnion {
    RwStreamMemory memory;
    RwStreamFile file;
    RwStreamCustom custom;
} RwStreamUnion;

struct RwStream {
    RwStreamType type;
    RwStreamAccessType accessType;
    RwInt32 position;
    RwStreamUnion Type;
    RwBool rwOwned;
};

void* memcpy(void* dst, const void* src, unsigned long n);

RwModuleInfo streamModule;

typedef struct rwStreamGlobals {
    RwFreeList* streamFreeList;
} rwStreamGlobals;

#define RWSTREAMGLOBAL(var) (RWPLUGINOFFSET(rwStreamGlobals, RwEngineInstance, streamModule.globalsOffset)->var)
#define RwRealloc(_p, _s) ((RWSRCGLOBAL(memoryFuncs).rwrealloc)((_p), (_s)))

void* _rwStreamModuleOpen(void* instance, RwInt32 offset, RwInt32 size) {
    streamModule.globalsOffset = offset;

    RWSTREAMGLOBAL(streamFreeList) = RwFreeListCreate(sizeof(RwStream), 10, 0);
    if (!RWSTREAMGLOBAL(streamFreeList)) {
        return NULL;
    }

    streamModule.numInstances++;

    return instance;
}

void* _rwStreamModuleClose(void* instance, RwInt32 offset, RwInt32 size) {
    if (RWSTREAMGLOBAL(streamFreeList)) {
        RwFreeListDestroy(RWSTREAMGLOBAL(streamFreeList));
    }

    streamModule.numInstances--;

    return instance;
}

RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length) {
    switch (stream->type) {
    case rwSTREAMFILE:
    case rwSTREAMFILENAME: {
        void* fp = stream->Type.file.fpFile;
        RwUInt32 bytesRead = RWSRCGLOBAL(fileFuncs).rwfread(buffer, 1, length, fp);

        if (bytesRead != length) {
            if (RWSRCGLOBAL(fileFuncs).rwfeof(fp)) {
                RWERROR((E_RW_ENDOFSTREAM));
            } else {
                RWERROR((E_RW_READ));
            }
        }

        return bytesRead;
    }
    case rwSTREAMMEMORY:
        if (length > stream->Type.memory.nSize - stream->Type.memory.position) {
            length = stream->Type.memory.nSize - stream->Type.memory.position;
            RWERROR((E_RW_ENDOFSTREAM));
        }

        memcpy(buffer, stream->Type.memory.memBlock + stream->Type.memory.position, length);
        stream->Type.memory.position += length;

        return length;
    case rwSTREAMCUSTOM:
        return stream->Type.custom.sfnread(stream->Type.custom.data, buffer, length);
    default:
        RWERROR((E_RW_BADSTREAMTYPE));
        return 0;
    }
}

RwStream* RwStreamWrite(RwStream* stream, const void* buffer, RwUInt32 length) {
    switch (stream->type) {
    case rwSTREAMFILE:
    case rwSTREAMFILENAME: {
        RwUInt32 bytesWritten = RWSRCGLOBAL(fileFuncs).rwfwrite(buffer, 1, length, stream->Type.file.fpFile);

        if (bytesWritten != length) {
            RWERROR((E_RW_WRITE));
            return NULL;
        }

        return stream;
    }
    case rwSTREAMMEMORY:
        if (!stream->Type.memory.memBlock) {
            stream->Type.memory.memBlock = (RwUInt8*)RwMalloc(rwSTREAMBLOCKSIZE);
            if (!stream->Type.memory.memBlock) {
                RWERROR((E_RW_NOMEM, rwSTREAMBLOCKSIZE));
                return NULL;
            }

            stream->Type.memory.nSize = rwSTREAMBLOCKSIZE;
        }

        if (stream->Type.memory.nSize - stream->Type.memory.position < length) {
            RwUInt32 newSize = (length < rwSTREAMBLOCKSIZE) ? stream->Type.memory.nSize + rwSTREAMBLOCKSIZE
                                                            : length + stream->Type.memory.nSize;
            RwUInt8* newBlock = (RwUInt8*)RwRealloc(stream->Type.memory.memBlock, newSize);

            if (!newBlock) {
                RWERROR((E_RW_NOMEM, newSize - stream->Type.memory.nSize));
                return NULL;
            }

            stream->Type.memory.memBlock = newBlock;
            stream->Type.memory.nSize = newSize;
        }

        memcpy(stream->Type.memory.memBlock + stream->Type.memory.position, buffer, length);
        stream->Type.memory.position += length;

        return stream;
    case rwSTREAMCUSTOM:
        if (stream->Type.custom.sfnwrite(stream->Type.custom.data, buffer, length)) {
            return stream;
        }

        return NULL;
    default:
        RWERROR((E_RW_BADSTREAMTYPE));
        return NULL;
    }
}

RwStream* RwStreamSkip(RwStream* stream, RwUInt32 offset) {
    if (!offset) {
        return stream;
    }

    switch (stream->type) {
    case rwSTREAMFILE:
    case rwSTREAMFILENAME: {
        void* fp = stream->Type.file.fpFile;

        if (RWSRCGLOBAL(fileFuncs).rwfseek(fp, offset, 1)) {
            if (RWSRCGLOBAL(fileFuncs).rwfeof(fp)) {
                RWERROR((E_RW_ENDOFSTREAM));
            }

            return NULL;
        }

        return stream;
    }
    case rwSTREAMMEMORY:
        if (stream->Type.memory.position + offset > stream->Type.memory.nSize) {
            stream->Type.memory.position = stream->Type.memory.nSize;
            RWERROR((E_RW_ENDOFSTREAM));
            return NULL;
        }

        stream->Type.memory.position += offset;

        return stream;
    case rwSTREAMCUSTOM:
        if (stream->Type.custom.sfnskip(stream->Type.custom.data, offset)) {
            return stream;
        }

        return NULL;
    default:
        RWERROR((E_RW_BADSTREAMTYPE));
        return NULL;
    }
}

RwBool RwStreamClose(RwStream* stream, void* pData) {
    RwBool result;

    switch (stream->type) {
    case rwSTREAMFILE:
        result = TRUE;
        break;
    case rwSTREAMFILENAME:
        result = !RWSRCGLOBAL(fileFuncs).rwfclose(stream->Type.file.fpFile);
        break;
    case rwSTREAMMEMORY:
        if (stream->accessType != rwSTREAMREAD && pData) {
            ((RwMemory*)pData)->start = stream->Type.memory.memBlock;
            ((RwMemory*)pData)->length = stream->Type.memory.position;
        }

        result = TRUE;
        break;
    case rwSTREAMCUSTOM:
        if (stream->Type.custom.sfnclose) {
            stream->Type.custom.sfnclose(stream->Type.custom.data);
        }

        result = TRUE;
        break;
    default:
        RWERROR((E_RW_BADSTREAMTYPE));
        return FALSE;
    }

    if (stream->rwOwned) {
        RwFreeListFree(RWSTREAMGLOBAL(streamFreeList), stream);
    }

    return result;
}

static inline RwStream* StreamOpenFilename(RwStream* stream, RwStreamAccessType accessType, const RwChar* name) {
    void* fp = NULL;

    switch (accessType) {
    case rwSTREAMREAD:
        fp = RWSRCGLOBAL(fileFuncs).rwfopen(name, "rb");
        break;
    case rwSTREAMWRITE:
        fp = RWSRCGLOBAL(fileFuncs).rwfopen(name, "wb");
        break;
    case rwSTREAMAPPEND:
        fp = RWSRCGLOBAL(fileFuncs).rwfopen(name, "ab");
        break;
    default:
        RWERROR((E_RW_BADACCESS));
        break;
    }

    if (fp) {
        stream->Type.file.fpFile = fp;
        return stream;
    }

    RWERROR((E_RW_BADOPEN, name));
    return NULL;
}

static inline RwStream* StreamOpenMemory(RwStream* stream, RwStreamAccessType accessType, const RwMemory* memory) {
    RwStream* result = NULL;

    switch (accessType) {
    case rwSTREAMREAD:
        stream->Type.memory.position = 0;
        result = stream;
        stream->Type.memory.nSize = memory->length;
        stream->Type.memory.memBlock = memory->start;
        break;
    case rwSTREAMWRITE:
        stream->Type.memory.position = 0;
        result = stream;
        stream->Type.memory.nSize = 0;
        stream->Type.memory.memBlock = NULL;
        break;
    case rwSTREAMAPPEND:
        stream->Type.memory.position = memory->length;
        result = stream;
        stream->Type.memory.nSize = memory->length;
        stream->Type.memory.memBlock = memory->start;
        break;
    default:
        RWERROR((E_RW_BADACCESS));
        break;
    }

    return result;
}

static inline RwStream* _rwStreamInitialize(RwStream* stream, RwBool rwOwned, RwStreamType type,
                                            RwStreamAccessType accessType, const void* pData) {
    stream->type = type;
    stream->accessType = accessType;
    stream->rwOwned = rwOwned;

    switch (type) {
    case rwSTREAMFILE:
        if (RWSRCGLOBAL(fileFuncs).rwftell((void*)pData) == -1) {
            return NULL;
        }

        stream->Type.file.fpFile = (void*)pData;
        return stream;
    case rwSTREAMFILENAME:
        return StreamOpenFilename(stream, accessType, (const RwChar*)pData);
    case rwSTREAMMEMORY:
        return StreamOpenMemory(stream, accessType, (const RwMemory*)pData);
    case rwSTREAMCUSTOM:
        memcpy(&stream->Type.custom, pData, sizeof(RwStreamCustom));
        return stream;
    default:
        RWERROR((E_RW_BADSTREAMTYPE));
        return NULL;
    }
}

RwStream* RwStreamOpen(RwStreamType type, RwStreamAccessType accessType, const void* pData) {
    RwStream* stream = (RwStream*)RwFreeListAlloc(RWSTREAMGLOBAL(streamFreeList));

    if (!stream || !_rwStreamInitialize(stream, TRUE, type, accessType, pData)) {
        RwFreeListFree(RWSTREAMGLOBAL(streamFreeList), stream);
        stream = NULL;
    }

    return stream;
}

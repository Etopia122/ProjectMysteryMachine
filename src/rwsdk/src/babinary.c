/*
 * Binary streams: chunk headers and endian-correct reads and writes. RW files
 * are little endian, so the GameCube byte-swaps every 32-bit word.
 */

#include "rwsdk/rwcore.h"

#define rwPLUGIN_ID 1 /* core */

#define E_RW_READ 0x8000001A

void* memcpy(void* dst, const void* src, unsigned long n);
RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length);
RwStream* RwStreamWrite(RwStream* stream, const void* buffer, RwUInt32 length);

typedef struct RwChunkHeaderInfo {
    RwUInt32 type;
    RwUInt32 length;
    RwUInt32 version;
    RwUInt32 buildNum;
    RwBool isComplex;
} RwChunkHeaderInfo;

typedef struct _rwChunkHeader {
    RwUInt32 type;
    RwUInt32 length;
    RwUInt32 libraryID;
} _rwChunkHeader;

#define RWLIBRARYIDPACK(_version, _buildNum) \
    (((((_version) - 0x30000) & 0x3FF00) << 14) | (((_version) & 0x3F) << 16) | ((_buildNum) & 0xFFFF))
#define RWLIBRARYIDUNPACKVERSION(_id) (((((_id) >> 14) & 0x3FF00) + 0x30000) | (((_id) >> 16) & 0x3F))
#define RWLIBRARYIDUNPACKBUILDNUM(_id) ((_id) & 0xFFFF)

#pragma dont_inline on
static RwBool ChunkIsComplex(RwChunkHeaderInfo* chunkHeaderInfo) {
    switch (chunkHeaderInfo->type) {
    case 0x01:
        return FALSE;
    case 0x02:
        return FALSE;
    case 0x03:
        return FALSE;
    case 0x05:
        return TRUE;
    case 0x06:
        return TRUE;
    case 0x07:
        return TRUE;
    case 0x08:
        return TRUE;
    case 0x09:
        return TRUE;
    case 0x0A:
        return TRUE;
    case 0x0B:
        return TRUE;
    case 0x0D:
        return FALSE;
    case 0x0E:
        return TRUE;
    case 0x0F:
        return TRUE;
    case 0x10:
        return TRUE;
    case 0x12:
        return TRUE;
    case 0x13:
        return FALSE;
    case 0x14:
        return TRUE;
    case 0x1A:
        return TRUE;
    }

    return FALSE;
}
#pragma dont_inline reset

void* RwMemNative32(void* mem, RwUInt32 size);

RwBool _rwStreamReadChunkHeader(RwStream* stream, RwUInt32* type, RwUInt32* length, RwUInt32* version,
                                RwUInt32* buildNum) {
    _rwChunkHeader chunkHdr;

    RwBool status = (RwStreamRead(stream, &chunkHdr, sizeof(chunkHdr)) == sizeof(chunkHdr));

    if (!status) {
        RWERROR((E_RW_READ));
        return FALSE;
    }

    RwMemNative32(&chunkHdr, sizeof(chunkHdr));

    if (type) {
        *type = chunkHdr.type;
    }
    if (length) {
        *length = chunkHdr.length;
    }

    if (!(chunkHdr.libraryID & 0xFFFF0000)) {
        if (version) {
            *version = chunkHdr.libraryID << 8;
        }
        if (buildNum) {
            *buildNum = 0;
        }
    } else {
        if (version) {
            *version = RWLIBRARYIDUNPACKVERSION(chunkHdr.libraryID);
        }
        if (buildNum) {
            *buildNum = RWLIBRARYIDUNPACKBUILDNUM(chunkHdr.libraryID);
        }
    }

    return TRUE;
}

void* RwMemLittleEndian32(void* mem, RwUInt32 size);

RwStream* _rwStreamWriteVersionedChunkHeader(RwStream* stream, RwInt32 type, RwInt32 size, RwUInt32 version,
                                             RwUInt32 buildNum) {
    _rwChunkHeader chunkHdr;

    chunkHdr.type = type;
    chunkHdr.length = size;
    chunkHdr.libraryID = RWLIBRARYIDPACK(version, buildNum);

    RwMemLittleEndian32(&chunkHdr, sizeof(chunkHdr));

    return RwStreamWrite(stream, &chunkHdr, sizeof(chunkHdr));
}

RwBool RwStreamFindChunk(RwStream* stream, RwUInt32 type, RwUInt32* lengthOut, RwUInt32* versionOut) {
    RwUInt32 readType;
    RwUInt32 readLength;
    RwUInt32 readVersion;

    while (_rwStreamReadChunkHeader(stream, &readType, &readLength, &readVersion, NULL)) {
        if (readType == type) {
            if (readVersion < rwLIBRARYBASEVERSION) {
                RWERROR((E_RW_BADVERSION));
                return FALSE;
            }
            if (readVersion > rwLIBRARYCURRENTVERSION) {
                RWERROR((E_RW_BADVERSION));
                return FALSE;
            }

            if (lengthOut) {
                *lengthOut = readLength;
            }
            if (versionOut) {
                *versionOut = readVersion;
            }
            return TRUE;
        }

        if (!RwStreamSkip(stream, readLength)) {
            return FALSE;
        }
    }

    return FALSE;
}

void* RwMemLittleEndian32(void* mem, RwUInt32 size) {
    RwUInt32* p = (RwUInt32*)mem;

    size >>= 2;
    while (size--) {
        *p = (*p >> 24) | ((*p >> 8) & 0xFF00) | ((*p << 8) & 0xFF0000) | (*p << 24);
        p++;
    }

    return mem;
}

void* RwMemLittleEndian16(void* mem, RwUInt32 size) {
    RwUInt16* p = (RwUInt16*)mem;
    RwUInt32 n = size >> 1;

    while (n--) {
        *p = (RwUInt16)((*p >> 8) | (*p << 8));
        p++;
    }

    return mem;
}

void* RwMemNative32(void* mem, RwUInt32 size) {
    RwUInt32* p = (RwUInt32*)mem;

    size >>= 2;
    while (size--) {
        *p = (*p >> 24) | ((*p >> 8) & 0xFF00) | ((*p << 8) & 0xFF0000) | (*p << 24);
        p++;
    }

    return mem;
}

void* RwMemNative16(void* mem, RwUInt32 size) {
    RwUInt16* p = (RwUInt16*)mem;
    RwUInt32 n = size >> 1;

    while (n--) {
        *p = (RwUInt16)((*p >> 8) | (*p << 8));
        p++;
    }

    return mem;
}

/* RwReal is a 32-bit float here, so the conversions only walk the buffer. */
void* RwMemRealToFloat32(void* mem, RwUInt32 size) {
    RwReal* p = (RwReal*)mem;

    size >>= 2;
    while (size--) {
        *p = *p;
        p++;
    }

    return mem;
}

void* RwMemFloat32ToReal(void* mem, RwUInt32 size) {
    RwReal* p = (RwReal*)mem;

    size >>= 2;
    while (size--) {
        *p = *p;
        p++;
    }

    return mem;
}

RwStream* RwStreamWriteReal(RwStream* stream, const RwReal* reals, RwUInt32 numBytes) {
    RwUInt8 buffer[256];

    while (numBytes) {
        RwUInt32 bytesToWrite = (numBytes >= sizeof(buffer)) ? sizeof(buffer) : numBytes;

        memcpy(buffer, reals, bytesToWrite);
        RwMemRealToFloat32(buffer, bytesToWrite);
        RwMemLittleEndian32(buffer, bytesToWrite);

        if (!RwStreamWrite(stream, buffer, bytesToWrite)) {
            return NULL;
        }

        numBytes -= bytesToWrite;
        reals = (const RwReal*)((const RwUInt8*)reals + bytesToWrite);
    }

    return stream;
}

RwStream* RwStreamWriteInt32(RwStream* stream, const RwInt32* ints, RwUInt32 numBytes) {
    RwUInt8 buffer[256];

    while (numBytes) {
        RwUInt32 bytesToWrite = (numBytes >= sizeof(buffer)) ? sizeof(buffer) : numBytes;

        memcpy(buffer, ints, bytesToWrite);
        RwMemLittleEndian32(buffer, bytesToWrite);

        if (!RwStreamWrite(stream, buffer, bytesToWrite)) {
            return NULL;
        }

        numBytes -= bytesToWrite;
        ints = (const RwInt32*)((const RwUInt8*)ints + bytesToWrite);
    }

    return stream;
}

RwStream* RwStreamReadReal(RwStream* stream, RwReal* reals, RwUInt32 numBytes) {
    if (!RwStreamRead(stream, reals, numBytes)) {
        RWERROR((E_RW_READ));
        return NULL;
    }

    RwMemNative32(reals, numBytes);
    RwMemFloat32ToReal(reals, numBytes);

    return stream;
}

RwStream* RwStreamReadInt32(RwStream* stream, RwInt32* ints, RwUInt32 numBytes) {
    if (!RwStreamRead(stream, ints, numBytes)) {
        RWERROR((E_RW_READ));
        return NULL;
    }

    RwMemNative32(ints, numBytes);

    return stream;
}

RwStream* RwStreamReadChunkHeaderInfo(RwStream* stream, RwChunkHeaderInfo* chunkHeaderInfo) {
    RwUInt32 type;
    RwUInt32 length;
    RwUInt32 version;
    RwUInt32 buildNum;

    if (!_rwStreamReadChunkHeader(stream, &type, &length, &version, &buildNum)) {
        return NULL;
    }

    chunkHeaderInfo->type = type;
    chunkHeaderInfo->length = length;
    chunkHeaderInfo->version = version;
    chunkHeaderInfo->buildNum = buildNum;
    chunkHeaderInfo->isComplex = ChunkIsComplex(chunkHeaderInfo);

    return stream;
}

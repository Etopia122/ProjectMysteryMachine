// RenderWare streams: memory and file (through the engine's file functions,
// which the game replaces with its own disc file system).
#include "rw_internal.h"
#include "pc_platform.h"

#include <stdio.h>
#include <string.h>

extern "C" {

RwStream* RwStreamOpen(RwInt32 type, RwInt32 accessType, const void* pData)
{
    RwStream* s = (RwStream*)rwCalloc(1, sizeof(RwStream));
    s->type = type;
    s->accessType = accessType;
    s->rwOwned = 1;
    if (type == rwSTREAMMEMORY)
    {
        const RwMemory* m = (const RwMemory*)pData;
        s->Type.memory.memBlock = m->start;
        s->Type.memory.nSize = m->length;
        s->Type.memory.position = 0;
    }
    else if (type == rwSTREAMFILENAME)
    {
        const char* mode = accessType == rwSTREAMREAD ? "rb" : accessType == rwSTREAMWRITE ? "wb" : "ab";
        s->Type.file.fpFile = rwGlobals()->fileFuncs.rwfopen((const char*)pData, mode);
        if (!s->Type.file.fpFile)
        {
            rwFree(s);
            return NULL;
        }
    }
    else if (type == rwSTREAMFILE)
    {
        s->Type.file.fpFile = (void*)pData;
    }
    else
    {
        pc_log("[rw] stream type %d not supported", type);
        rwFree(s);
        return NULL;
    }
    return s;
}

RwInt32 RwStreamClose(RwStream* s, void* pData)
{
    if (s->type == rwSTREAMFILENAME && s->Type.file.fpFile)
    {
        rwGlobals()->fileFuncs.rwfclose(s->Type.file.fpFile);
    }
    if (s->type == rwSTREAMMEMORY && pData)
    {
        RwMemory* m = (RwMemory*)pData;
        m->start = s->Type.memory.memBlock;
        m->length = s->Type.memory.nSize;
    }
    rwFree(s);
    return 1;
}

RwUInt32 RwStreamRead(RwStream* s, void* buffer, RwUInt32 length)
{
    if (s->type == rwSTREAMMEMORY)
    {
        RwStreamMemory& m = s->Type.memory;
        RwUInt32 left = m.nSize > m.position ? m.nSize - m.position : 0;
        if (length > left)
        {
            length = left;
        }
        memcpy(buffer, m.memBlock + m.position, length);
        m.position += length;
        s->position += length;
        return length;
    }
    RwUInt32 n = rwGlobals()->fileFuncs.rwfread(buffer, 1, length, s->Type.file.fpFile);
    s->position += n;
    return n;
}

RwStream* RwStreamSkip(RwStream* s, RwUInt32 offset)
{
    if (s->type == rwSTREAMMEMORY)
    {
        RwStreamMemory& m = s->Type.memory;
        m.position += offset;
        if (m.position > m.nSize)
        {
            m.position = m.nSize;
        }
    }
    else
    {
        rwGlobals()->fileFuncs.rwfseek(s->Type.file.fpFile, (long)offset, SEEK_CUR);
    }
    s->position += offset;
    return s;
}

static bool read_header(RwStream* s, RwChunkHeaderInfo* info)
{
    RwUInt8 h[12];
    if (RwStreamRead(s, h, 12) != 12)
    {
        return false;
    }
    info->type = rd32(h);
    info->length = rd32(h + 4);
    RwUInt32 libId = rd32(h + 8);
    if (libId & 0xFFFF0000)
    {
        info->version = ((libId >> 14) & 0x3FF00) + 0x30000 | ((libId >> 16) & 0x3F);
        info->buildNum = libId & 0xFFFF;
    }
    else
    {
        info->version = libId << 8;
        info->buildNum = 0;
    }
    info->isComplex = 0;
    return true;
}

RwStream* RwStreamReadChunkHeaderInfo(RwStream* s, RwChunkHeaderInfo* info)
{
    return read_header(s, info) ? s : NULL;
}

RwInt32 RwStreamFindChunk(RwStream* s, RwUInt32 type, RwUInt32* lengthOut, RwUInt32* versionOut)
{
    RwChunkHeaderInfo info;
    while (read_header(s, &info))
    {
        if (info.type == type)
        {
            if (lengthOut)
            {
                *lengthOut = info.length;
            }
            if (versionOut)
            {
                *versionOut = info.version;
            }
            return 1;
        }
        RwStreamSkip(s, info.length);
    }
    return 0;
}

}

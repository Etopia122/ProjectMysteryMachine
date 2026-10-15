/*
 * Bink GameCube file IO: double-buffered asynchronous DVD reads behind the
 * BINKIO interface. Compiled with SN ProDG (GCC 2.95).
 */

#include "bink/binkint.h"

typedef struct DVDFileInfo
{
    U32 cb[11];     /* 0x00: command block */
    U32 startAddr;  /* 0x2C */
    U32 length;     /* 0x30 */
    void* callback; /* 0x34 */
    U32 pad;        /* 0x38 */
} DVDFileInfo;

typedef struct BINKFILE BINKFILE;
typedef void (*DVDCallback)(S32 result, DVDFileInfo* fi);

/* BINKIO with the file reader's private data after the public part. */
struct BINKFILE
{
    void* ReadHeader;          /* 0x00 */
    void* ReadFrame;           /* 0x04 */
    void* GetBufferSize;       /* 0x08 */
    void* SetInfo;             /* 0x0C */
    void* Idle;                /* 0x10 */
    void* Close;               /* 0x14 */
    void* BGControl;           /* 0x18 */
    volatile U32 ReadError;    /* 0x1C */
    volatile U32 DoingARead;   /* 0x20 */
    volatile U32 BytesRead;    /* 0x24 */
    volatile U32 Working;      /* 0x28 */
    volatile U32 TotalTime;    /* 0x2C */
    volatile U32 ForegroundTime; /* 0x30 */
    volatile U32 IdleTime;     /* 0x34 */
    volatile U32 ThreadTime;   /* 0x38 */
    volatile U32 BufSize;      /* 0x3C */
    volatile U32 BufHighUsed;  /* 0x40 */
    volatile U32 CurBufSize;   /* 0x44 */
    volatile U32 CurBufUsed;   /* 0x48 */
    DVDFileInfo fi;            /* 0x4C */
    volatile U32 fileoffset;          /* 0x88: next byte to read ahead */
    U32 skip;                  /* 0x8C: alignment bytes at the buffer start */
    U32 bufpos;                /* 0x90: file offset of bufreadpos */
    U8* bufreadpos;            /* 0x94 */
    volatile U32 bufempty;            /* 0x98 */
    U8* buffer;                /* 0x9C */
    U8* bufend;                /* 0xA0 */
    U8* bufwritepos;           /* 0xA4 */
    U32 dontclose;             /* 0xA8 */
    U32 startpos;              /* 0xAC */
    U32 filesize;              /* 0xB0 */
    U32 simulate;              /* 0xB4 */
    S32 simtime;               /* 0xB8 */
    U32 cancelling;            /* 0xBC */
    U32 wasworking;            /* 0xC0 */
    U32 readstart;             /* 0xC4 */
    BINKFILE* self;            /* 0xC8 */
    U8 pad[0x13C - 0xCC];
};

S32 DVDReadAsyncPrio(DVDFileInfo* fi, void* addr, S32 length, S32 offset, DVDCallback cb, S32 prio);
S32 DVDGetCommandBlockStatus(void* cb);
S32 DVDCancel(void* cb);
S32 DVDClose(DVDFileInfo* fi);
S32 DVDConvertPathToEntrynum(const char* path);
S32 DVDFastOpen(S32 entry, DVDFileInfo* fi);
U32 RADTimerRead(void);
U32 mult64anddiv(U32 a, U32 b, U32 c);
void intelendian(void* dest, U32 size);

void* memmove(void* dest, const void* src, U32 n);

void ReadKickoff(BINKFILE* bf);

void intelendian(void* dest, U32 size)
{
    U32 n = (size + 3) >> 2;
    U32* p = (U32*)dest;

    while (n--)
    {
        *p = radlwbrx(p, 0);
        p++;
    }
}

U32 radreadngc(DVDFileInfo* fi, U32 offset, U8* dest, U32 bytes)
{
    U32 total, amt, max;
    S32 st;
    U8 buf[1024 + 31];
    U8* aligned = (U8*)(((U32)buf + 31) & ~31);

    max = (U32)(buf + sizeof(buf) - aligned) & ~31;
    total = 0;
    while (bytes)
    {
        if (bytes >= max)
            amt = max;
        else
            amt = (bytes + 31) & ~31;
        DVDReadAsyncPrio(fi, aligned, amt, offset, 0, 0);
        for (;;)
        {
            st = DVDGetCommandBlockStatus(fi);
            switch (st)
            {
            case 0:
                goto done;
            case -1:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
                return 0;
            }
        }
    done:
        if (amt > bytes)
            amt = bytes;
        memcpy(dest, aligned, amt);
        dest += amt;
        total += amt;
        offset += amt;
        bytes -= amt;
    }
    return total;
}

U32 BinkFileReadHeader(BINKFILE* bf, S32 offset, void* dest, U32 size)
{
    U32 amt, left;

    if (offset != -1 && bf->fileoffset != (U32)offset)
        bf->fileoffset = offset;
    amt = radreadngc(&bf->fi, bf->fileoffset + bf->startpos, dest, size);
    bf->fileoffset += amt;
    bf->bufpos = bf->fileoffset;
    left = bf->filesize - bf->bufpos;
    bf->CurBufSize = (left < bf->BufSize) ? left : bf->BufSize;
    intelendian(dest, amt);
    return amt;
}

void dosimulate(BINKFILE* bf, U32 amt, U32 start)
{
    U32 t, now;

    t = mult64anddiv(amt, 1000, bf->simulate);
    now = RADTimerRead();
    bf->simtime += t - (now - start);
    while (bf->simtime > 0)
    {
        do
        {
            t = RADTimerRead();
        } while ((S32)(t - now) < bf->simtime);
        bf->simtime -= t - now;
        now = t;
    }
}

void DVDReadCallback(S32 result, DVDFileInfo* fi)
{
    BINKFILE* bf = *(BINKFILE**)((U8*)fi + 0x7C);
    U32 t;

    if (bf->cancelling == 0)
    {
        if (result != 0x1000 && (bf->filesize - bf->fileoffset) + bf->skip != (U32)result)
        {
            bf->DoingARead = 0;
            bf->ReadError = 1;
        }
        else
        {
            if (bf->skip)
            {
                memmove(bf->bufwritepos, bf->bufwritepos + bf->skip, bf->filesize - bf->fileoffset);
                result -= bf->skip;
                bf->skip = 0;
            }
            bf->BytesRead += result;
            bf->bufwritepos += result;
            bf->fileoffset += result;
            if (bf->bufwritepos >= bf->bufend)
                bf->bufwritepos = bf->buffer;
            bf->bufempty -= result;
            bf->CurBufUsed += result;
            if (bf->CurBufUsed > bf->BufHighUsed)
                bf->BufHighUsed = bf->CurBufUsed;
            if (bf->simulate)
                dosimulate(bf, result, bf->readstart);
            t = RADTimerRead() - bf->readstart;
            bf->readstart = t;
            bf->TotalTime += t;
            if (bf->wasworking || bf->Working)
                bf->ThreadTime += t;
            else
                bf->IdleTime += t;
            bf->DoingARead = 0;
            ReadKickoff(bf);
        }
    }
}

void ReadKickoff(BINKFILE* bf)
{
    S32 st;
    U32 left;

    st = DVDGetCommandBlockStatus(&bf->fi);
    left = bf->filesize - bf->fileoffset;
    switch (st)
    {
    case -1:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 11:
        bf->ReadError = 1;
        return;
    }
    if (bf->cancelling == 0 && (st == 0 || st == 10))
    {
        if (bf->bufempty >= 0x1000 && left != 0)
        {
            bf->wasworking = bf->Working;
            bf->readstart = RADTimerRead();
            bf->DoingARead = 1;
            if (left > 0x1000)
            {
                left = 0x1000;
            }
            else if (left & 31)
            {
                bf->skip = -left & 31;
                left += bf->skip;
            }
            DVDReadAsyncPrio(&bf->fi, bf->bufwritepos, left, (bf->fileoffset - bf->skip) + bf->startpos,
                             DVDReadCallback, 0);
        }
        else
        {
            bf->CurBufSize = bf->CurBufUsed;
        }
    }
}

S32 BinkFileIdle(BINKFILE* bf)
{
    if (bf->ReadError)
        return 0;
    switch (DVDGetCommandBlockStatus(&bf->fi))
    {
    case 0:
        ReadKickoff(bf);
        break;
    case 1:
    case 2:
    case 10:
        break;
    case -1:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 11:
        bf->ReadError = 1;
        break;
    }
    return bf->DoingARead;
}

void CancelReadRequests(BINKFILE* bf)
{
    bf->cancelling = 1;
    DVDCancel(&bf->fi);
    while ((U32)(DVDGetCommandBlockStatus(&bf->fi) - 1) < 2)
        ;
    bf->DoingARead = 0;
    bf->cancelling = 0;
}

U32 BinkFileReadFrame(BINKFILE* bf, U32 frame, U32 offset, U8* dest, U32 size)
{
    U32 total = 0;
    U32 start, t, amt, wrap, rs;
    S32 level;
    U32 d;
    U8* orig = dest;

    if (bf->ReadError)
        return 0;
    {
        start = RADTimerRead();
        if (offset != (U32)-1 && bf->bufpos != offset)
        {
            if (offset > bf->bufpos && offset <= bf->fileoffset)
            {
                level = OSDisableInterrupts();
                d = offset - bf->bufpos;
                bf->bufempty += d;
                bf->bufpos = offset;
                bf->bufreadpos += d;
                bf->CurBufUsed -= d;
                if (bf->bufreadpos > bf->bufend)
                    bf->bufreadpos -= bf->BufSize;
                OSRestoreInterrupts(level);
            }
            else
            {
                CancelReadRequests(bf);
                level = OSDisableInterrupts();
                bf->fileoffset = offset;
                bf->bufpos = offset;
                bf->bufempty = bf->BufSize;
                bf->CurBufUsed = 0;
                bf->bufreadpos = bf->buffer;
                bf->bufwritepos = bf->buffer;
                OSRestoreInterrupts(level);
            }
        }
        if (bf->buffer == 0)
        {
            rs = RADTimerRead();
            d = radreadngc(&bf->fi, bf->fileoffset, orig, size);
            if (d < size)
                bf->ReadError = 1;
            bf->fileoffset += d;
            bf->bufpos += d;
            bf->BytesRead += d;
            total = d;
            if (bf->simulate)
                dosimulate(bf, total, rs);
            t = RADTimerRead();
            bf->TotalTime += t - rs;
            bf->ForegroundTime += t - start;
        }
        else
        {
            while (size && bf->ReadError == 0)
            {
                ReadKickoff(bf);
                amt = bf->CurBufUsed;
                if (amt)
                {
                    if (amt > size)
                        amt = size;
                    size -= amt;
                    total += amt;
                    bf->bufpos += amt;
                    wrap = bf->bufend - bf->bufreadpos;
                    if (wrap <= amt)
                    {
                        memcpy(dest, bf->bufreadpos, wrap);
                        dest += wrap;
                        amt -= wrap;
                        bf->bufreadpos = bf->buffer;
                        level = OSDisableInterrupts();
                        bf->CurBufUsed -= wrap;
                        bf->bufempty += wrap;
                        OSRestoreInterrupts(level);
                    }
                    if (amt)
                    {
                        memcpy(dest, bf->bufreadpos, amt);
                        dest += amt;
                        bf->bufreadpos += amt;
                        level = OSDisableInterrupts();
                        bf->CurBufUsed -= amt;
                        bf->bufempty += amt;
                        OSRestoreInterrupts(level);
                    }
                }
            }
            bf->ForegroundTime += RADTimerRead() - start;
        }
        t = bf->filesize - bf->bufpos;
        bf->CurBufSize = (t < bf->BufSize) ? t : bf->BufSize;
        if (bf->CurBufUsed + 0x1000 > bf->CurBufSize)
            bf->CurBufSize = bf->CurBufUsed;
        intelendian(orig, total);
    }
    return total;
}

U32 BinkFileGetBufferSize(BINKFILE* bf, U32 size)
{
    return (size + 0xFFF) & ~0xFFF;
}

void BinkFileSetInfo(BINKFILE* bf, U8* buf, U32 size, U32 filesize, U32 simulate)
{
    S32 level = OSDisableInterrupts();
    size = (size / 4096) * 4096;
    bf->bufempty = size;
    bf->BufSize = size;
    bf->filesize = filesize;
    bf->simulate = simulate;
    bf->self = bf;
    bf->buffer = buf;
    bf->bufreadpos = buf;
    bf->bufwritepos = buf;
    bf->CurBufUsed = 0;
    bf->bufend = buf + size;
    OSRestoreInterrupts(level);
}

void BinkFileClose(BINKFILE* bf)
{
    CancelReadRequests(bf);
    if (bf->dontclose == 0)
        DVDClose(&bf->fi);
}

S32 BinkFileOpen(BINKFILE* bf, const char* name, U32 flags)
{
    memset(bf, 0, sizeof(BINKFILE));
    if (flags & 0x800000)
    {
        bf->fi = *(DVDFileInfo*)name;
        bf->dontclose = 1;
        bf->startpos = bf->fi.startAddr;
    }
    else
    {
        S32 entry = DVDConvertPathToEntrynum(name);
        if (entry == -1 || !DVDFastOpen(entry, &bf->fi))
            return 0;
    }
    bf->ReadHeader = BinkFileReadHeader;
    bf->ReadFrame = BinkFileReadFrame;
    bf->GetBufferSize = BinkFileGetBufferSize;
    bf->SetInfo = BinkFileSetInfo;
    bf->Idle = BinkFileIdle;
    bf->Close = BinkFileClose;
    return 1;
}

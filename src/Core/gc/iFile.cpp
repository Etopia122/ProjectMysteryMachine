extern "C" {
char* strcpy(char* dst, const char* src);
}

class DVDCommandBlock
{
public:
    DVDCommandBlock* next;        // 0x0
    DVDCommandBlock* prev;        // 0x4
    unsigned int command;         // 0x8
    signed int state;             // 0xC
    unsigned int offset;          // 0x10
    unsigned int length;          // 0x14
    void* addr;                   // 0x18
    unsigned int currTransferSize; // 0x1C
    unsigned int transferredSize; // 0x20
    void* id;                     // 0x24
    void* callback;               // 0x28
    void* userData;               // 0x2C
};

class DVDFileInfo
{
public:
    DVDCommandBlock cb;    // 0x0
    unsigned int startAddr; // 0x30
    unsigned int length;    // 0x34
    void* callback;         // 0x38
};

class tag_iFile
{
public:
    unsigned int flags;   // 0x0
    char path[128];       // 0x4
    signed int entrynum;  // 0x84
    DVDFileInfo fileInfo; // 0x88
    signed int unkC4;     // 0xC4
    signed int asynckey;  // 0xC8
    signed int unknown[7]; // 0xCC
    signed int offset;    // 0xE8
};

class tag_xFile
{
public:
    char relname[16]; // 0x0
    tag_iFile ps;     // 0x10
};

typedef void (*DVDCallback)(long result, DVDFileInfo* fileInfo);

extern "C" {
signed int DVDConvertPathToEntrynum(const char* pathPtr);
int DVDFastOpen(signed int entrynum, DVDFileInfo* fileInfo);
int DVDClose(DVDFileInfo* fileInfo);
signed int DVDReadPrio(DVDFileInfo* fileInfo, void* addr, signed int length, signed int offset,
                       signed int prio);
int DVDReadAsyncPrio(DVDFileInfo* fileInfo, void* addr, signed int length, signed int offset,
                     DVDCallback callback, signed int prio);
signed int DVDGetCommandBlockStatus(const DVDCommandBlock* block);
void* OSAllocFromHeap(signed int heap, unsigned int size);
// TODO: unidentified xTRC function, polled while waiting for a read
void fn_800443AC();
}

extern volatile signed int __OSCurrHeap;

class iTRCDisk
{
public:
    static int CheckDVDAndResetState();
};

class file_queue_entry
{
public:
    tag_xFile* file;               // 0x0
    void* buf;                     // 0x4
    unsigned int size;             // 0x8
    unsigned int offset;           // 0xC
    signed int stat;               // 0x10
    void (*callback)(signed int);  // 0x14
    unsigned int asynckey;         // 0x18
};

file_queue_entry file_queue[4];

static unsigned int tbuffer[1024 + 8];
static unsigned int* buffer32;
volatile unsigned int iFileSyncAsyncReadActive;

static void async_cb(long result, DVDFileInfo* fileInfo);
signed int iFileReadAsync(tag_xFile* file, void* buf, unsigned int aSize,
                          void (*callback)(signed int));

#define OSRoundUp32B(x) (((unsigned int)(x) + 31) & ~31)

// Functions are in reverse DOL order (-inline deferred emits last to first).

void iFileInit()
{
    buffer32 = (unsigned int*)OSRoundUp32B(tbuffer);
}

void iFileExit()
{
}

unsigned int* iFileLoad(char* name, unsigned int* buffer, unsigned int* size)
{
    char fullpath[128];
    DVDFileInfo fileInfo;
    signed int fd;
    signed int locsize;

    strcpy(fullpath, name);
    fd = DVDConvertPathToEntrynum(fullpath);
    if (fd == -1 || !DVDFastOpen(fd, &fileInfo))
    {
        return 0;
    }

    locsize = fileInfo.length;
    if (!buffer)
    {
        buffer = (unsigned int*)OSAllocFromHeap(__OSCurrHeap, OSRoundUp32B(locsize));
    }
    locsize = OSRoundUp32B(locsize);
    DVDReadPrio(&fileInfo, buffer, locsize, 0, 2);
    DVDClose(&fileInfo);
    if (size)
    {
        *size = locsize;
    }
    return buffer;
}

#pragma auto_inline on
unsigned int iFileOpen(char* name, signed int flags, tag_xFile* file)
{
    tag_iFile* ps = &file->ps;

    if (flags & 4)
    {
        strcpy(ps->path, name);
    }
    else
    {
        strcpy(ps->path, name);
    }

    ps->entrynum = DVDConvertPathToEntrynum(ps->path);
    if (ps->entrynum == -1)
    {
        return 1;
    }
    if (!DVDFastOpen(ps->entrynum, &ps->fileInfo))
    {
        ps->entrynum = -1;
        return 1;
    }

    ps->unkC4 = 0;
    ps->flags = 1;
    ps->offset = 0;
    return 0;
}
#pragma auto_inline off

signed int iFileSeek(tag_xFile* file, signed int offset, signed int whence)
{
    tag_iFile* ps = &file->ps;
    signed int new_pos;

    switch (whence)
    {
    case 0:
        new_pos = offset;
        break;
    case 1:
        if (DVDGetCommandBlockStatus(&ps->fileInfo.cb) == 1)
        {
            return -1;
        }
        new_pos = offset + ps->offset;
        break;
    case 2:
        new_pos = ps->fileInfo.length - offset;
        if (new_pos < 0)
        {
            new_pos = 0;
        }
        break;
    default:
        new_pos = offset;
        break;
    }

    ps->offset = new_pos;
    return ps->offset;
}

static void ifilereadCB(signed int)
{
    iFileSyncAsyncReadActive = 0;
}

// TODO (98.9%): volatile registers differ in the inlined iFileReadAsync.
unsigned int iFileRead(tag_xFile* file, void* buf, unsigned int size)
{
    iFileSyncAsyncReadActive = 1;
    iFileReadAsync(file, buf, size, ifilereadCB);
    while (iFileSyncAsyncReadActive)
    {
        iTRCDisk::CheckDVDAndResetState();
        fn_800443AC();
    }
    return size;
}

// TODO (93%): the original keeps the if/else for the read length; here the
// compiler loads 0x8000 first and overwrites it.
static void async_cb(long result, DVDFileInfo* fileInfo)
{
    file_queue_entry* entry = &file_queue[(signed int)fileInfo->cb.userData];
    signed int r7 = -1;

    switch (result)
    {
    case -1:
    case -3:
        break;
    case -2:
    default:
        if (result >= 0)
        {
            r7 = result;
        }
        break;
    }

    if (r7 < 0)
    {
        entry->stat = 4;
        if (entry->callback)
        {
            entry->callback((signed int)entry->file);
        }
    }
    else if (r7 + entry->offset >= entry->size)
    {
        entry->stat = 3;
        if (entry->callback)
        {
            entry->callback((signed int)entry->file);
        }
    }
    else
    {
        entry->offset += r7;
        entry->stat = 2;

        signed int length;
        if (entry->size - entry->offset < 0x8000)
        {
            length = (entry->size - entry->offset + 3) & ~3;
        }
        else
        {
            length = 0x8000;
        }
        DVDReadAsyncPrio(&entry->file->ps.fileInfo, (char*)entry->buf + entry->offset, length,
                         entry->file->ps.offset + entry->offset, async_cb, 2);
    }
}

// TODO (97.8%): volatile registers differ (queue entry r10/r9).
#pragma auto_inline on
signed int iFileReadAsync(tag_xFile* file, void* buf, unsigned int aSize,
                          void (*callback)(signed int))
{
    static signed int fopcount = 1;
    tag_iFile* ps = &file->ps;
    signed int i;
    signed int id;

    for (i = 0; i < 4; i++)
    {
        if (file_queue[i].stat != 1 && file_queue[i].stat != 2)
        {
            id = fopcount++ << 2;

            file_queue[i].file = file;
            file_queue[i].buf = buf;
            file_queue[i].size = aSize;
            file_queue[i].offset = 0;
            file_queue[i].stat = 1;
            file_queue[i].callback = callback;
            file_queue[i].asynckey = id + i;

            aSize = (aSize < 32) ? (aSize + 3) & ~3 : 32;

            ps->fileInfo.cb.userData = (void*)i;
            DVDReadAsyncPrio(&ps->fileInfo, buf, aSize, ps->offset, async_cb, 2);
            return id + i;
        }
    }

    return -1;
}
#pragma auto_inline off

signed int iFileReadAsyncStatus(signed int key, signed int* amtToFar)
{
    if (key != file_queue[key & 3].asynckey)
    {
        return 5;
    }
    if (amtToFar)
    {
        *amtToFar = file_queue[key & 3].offset;
    }
    return file_queue[key & 3].stat;
}

// TODO: unidentified (returns 0)
extern "C" signed int fn_8000A5A4()
{
    return 0;
}

unsigned int iFileClose(tag_xFile* file)
{
    tag_iFile* ps = &file->ps;
    signed int ret;

    ret = DVDClose(&file->ps.fileInfo);
    ret = DVDClose(&file->ps.fileInfo);
    if (!ret)
    {
        return 1;
    }
    ps->flags = 0;
    return 0;
}

unsigned int iFileGetSize(tag_xFile* file)
{
    return file->ps.fileInfo.length;
}

unsigned int iFileFind(char* name, tag_xFile* file)
{
    return iFileOpen(name, 0, file);
}

void iFileGetInfo(tag_xFile* file, unsigned int* addr, unsigned int* length)
{
    if (addr)
    {
        *addr = file->ps.fileInfo.startAddr;
    }
    if (length)
    {
        *length = file->ps.fileInfo.length;
    }
}

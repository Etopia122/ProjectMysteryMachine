extern "C" {
void* memset(void* dst, int c, unsigned int n);
void* memcpy(void* dst, const void* src, unsigned int n);
int strcmp(const char* a, const char* b);
char* strncpy(char* dst, const char* src, unsigned int n);
}

enum en_FIOERRCODES
{
    FIOERR_NONE,
    FIOERR_READFAIL,
    FIOERR_WRITEFAIL,
    FIOERR_SEEKFAIL,
    FIOERR_USERABORT
};

enum en_BIO_ASYNC_ERRCODES
{
    BINIO_ASYNC_FAIL = -1,
    BINIO_ASYNC_NOOP,
    BINIO_ASYNC_INPROG,
    BINIO_ASYNC_DONE
};

enum IFILE_READSECTOR_STATUS
{
    IFILE_RDSTAT_NOOP,
    IFILE_RDSTAT_INPROG,
    IFILE_RDSTAT_QUEUED,
    IFILE_RDSTAT_DONE,
    IFILE_RDSTAT_FAIL,
    IFILE_RDSTAT_EXPIRED
};

class st_FILELOADINFO
{
public:
    void (*destroy)(st_FILELOADINFO*);
    signed int (*readBytes)(st_FILELOADINFO*, char*, signed int);
    signed int (*readMShorts)(st_FILELOADINFO*, signed short*, signed int);
    signed int (*readMLongs)(st_FILELOADINFO*, signed int*, signed int);
    signed int (*readMFloats)(st_FILELOADINFO*, float*, signed int);
    signed int (*readMDoubles)(st_FILELOADINFO*, double*, signed int);
    signed int (*readIShorts)(st_FILELOADINFO*, signed short*, signed int);
    signed int (*readILongs)(st_FILELOADINFO*, signed int*, signed int);
    signed int (*readIFloats)(st_FILELOADINFO*, float*, signed int);
    signed int (*readIDoubles)(st_FILELOADINFO*, double*, signed int);
    signed int (*skipBytes)(st_FILELOADINFO*, signed int);
    signed int (*seekSpot)(st_FILELOADINFO*, signed int);
    void (*setDoubleBuf)(st_FILELOADINFO*, char*, signed int);
    void (*discardDblBuf)(st_FILELOADINFO*);
    signed int (*asyncIRead)(st_FILELOADINFO*, signed int, char*, signed int, signed int);
    signed int (*asyncMRead)(st_FILELOADINFO*, signed int, char*, signed int, signed int);
    en_BIO_ASYNC_ERRCODES (*asyncReadStatus)(st_FILELOADINFO*);
    unsigned int lockid;      // 0x44
    en_FIOERRCODES error;     // 0x48
    unsigned int basesector;  // 0x4C
    void* privdata;           // 0x50
    void* xtradata;           // 0x54
    void* asyndata;           // 0x58
    signed int filesize;      // 0x5C
    signed int remain;        // 0x60
    signed int position;      // 0x64
};

// GameCube layout (0x100 bytes); only user_data is used here.
class tag_xFile
{
public:
    char relname[16];
    unsigned char ps[0xEC];
    void* user_data; // 0xFC
};

class st_BINIO_XTRADATA
{
public:
    char* dbl_buf;
    signed int dblbuf_size;
    signed int dbl_beg;
    signed int dbl_end;
    signed int dbl_amt;
    unsigned int fpos;
    char* asyn_data;
    signed int asyn_amt;
    signed int asyn_elesize;
    signed int asyn_ismot;
    en_BIO_ASYNC_ERRCODES asyn_status;
    unsigned int pad[3];
    signed int gcaskey; // 0x38
};

unsigned int iFileOpen(char* name, signed int flags, tag_xFile* file);
signed int iFileSeek(tag_xFile* file, signed int offset, signed int whence);
unsigned int iFileRead(tag_xFile* file, void* buf, unsigned int size);
unsigned int iFileClose(tag_xFile* file);
unsigned int iFileGetSize(tag_xFile* file);
unsigned int iFileFind(char* name, tag_xFile* file);
void iFileGetInfo(tag_xFile* file, unsigned int* starting_sector, unsigned int* size_in_bytes);
signed int iFileReadAsync(tag_xFile* file, void* buf, unsigned int size, void (*callback)(signed int));
IFILE_READSECTOR_STATUS iFileReadAsyncStatus(signed int key, signed int* amtsofar);

static unsigned int g_loadlock = 0xFFFFFF00;
static st_FILELOADINFO g_loadinst[8] = { 0 };
static tag_xFile g_xfload[8] = { 0 };
static st_BINIO_XTRADATA g_xtraload[8] = { 0 };
static st_BINIO_XTRADATA* g_async_context;

static void LoadDestroy(st_FILELOADINFO* fli);
static signed int SkipBytes(st_FILELOADINFO* fli, signed int fwd);
static signed int ReadSeek(st_FILELOADINFO* fli, signed int pos);
static void SetBuffer(st_FILELOADINFO* fli, char* dblbuffer, signed int bufsize);
static void DiscardBuffer(st_FILELOADINFO* fli);
static signed int ReadBytes(st_FILELOADINFO* fli, char* data, signed int count);
static signed int ReadMShorts(st_FILELOADINFO* fli, signed short* data, signed int count);
static signed int ReadMLongs(st_FILELOADINFO* fli, signed int* data, signed int count);
static signed int ReadMFloats(st_FILELOADINFO* fli, float* data, signed int count);
static signed int ReadMDoubles(st_FILELOADINFO* fli, double* data, signed int count);
static signed int ReadIShorts(st_FILELOADINFO* fli, signed short* data, signed int count);
static signed int ReadILongs(st_FILELOADINFO* fli, signed int* data, signed int count);
static signed int ReadIFloats(st_FILELOADINFO* fli, float* data, signed int count);
static signed int ReadIDoubles(st_FILELOADINFO* fli, double* data, signed int count);
static signed int AsyncMRead(st_FILELOADINFO* fli, signed int offset, char* data, signed int size,
                             signed int n);
static signed int AsyncIRead(st_FILELOADINFO* fli, signed int offset, char* data, signed int size,
                             signed int n);
static en_BIO_ASYNC_ERRCODES AsyncReadStatus(st_FILELOADINFO* fli);
static tag_xFile* BFD_open(char* filename, char* mode, unsigned int lockid, signed int,
                           void* xtradata);
static signed int BFD_read(void* data, signed int elesize, signed int elecnt, tag_xFile* bffp,
                           void* xtradata);
static void BFD_cb_asyncread(signed int);
static signed int BFD_AsyncRead(st_FILELOADINFO* fli, signed int pos, void* data, signed int size,
                                signed int n, signed int endian);

#pragma auto_inline on
static void BFD_close(tag_xFile* bffp, void* xtradata)
{
    st_BINIO_XTRADATA* xtra = (st_BINIO_XTRADATA*)xtradata;

    iFileClose(bffp);

    xtra->fpos = 0;
    xtra->dbl_buf = 0;
    xtra->dblbuf_size = 0;
    xtra->dbl_beg = 0;
    xtra->dbl_end = 0;
    xtra->dbl_amt = 0;
}

static signed int BFD_seek(tag_xFile* bffp, signed int offset, signed int whence, void* xtradata)
{
    st_BINIO_XTRADATA* xtra = (st_BINIO_XTRADATA*)xtradata;
    signed int failed = iFileSeek(bffp, offset, whence) == -1;

    xtra->fpos = offset;

    return failed;
}

static signed int BFD_getLength(tag_xFile* bffp, void* xtradata)
{
    return iFileGetSize(bffp);
}

static signed int BFD_startSector(char* filename)
{
    tag_xFile file = { 0 };
    unsigned int addr = 0;
    unsigned int length = 0;

    if (!iFileFind(filename, &file))
    {
        iFileGetInfo(&file, &addr, &length);
    }

    return addr;
}

static en_BIO_ASYNC_ERRCODES BFD_AsyncReadStatus(st_FILELOADINFO* fli)
{
    en_BIO_ASYNC_ERRCODES status;
    st_BINIO_XTRADATA* xtra = (st_BINIO_XTRADATA*)fli->xtradata;

    if (!g_async_context)
    {
        status = BINIO_ASYNC_NOOP;
    }
    else if (xtra != g_async_context)
    {
        status = BINIO_ASYNC_INPROG;
    }
    else
    {
        if (xtra->asyn_status == BINIO_ASYNC_INPROG)
        {
            signed int amtsofar = 0;

            switch (iFileReadAsyncStatus(xtra->gcaskey, &amtsofar))
            {
            case IFILE_RDSTAT_INPROG:
            case IFILE_RDSTAT_QUEUED:
                break;
            case IFILE_RDSTAT_DONE:
                xtra->asyn_status = BINIO_ASYNC_DONE;
                break;
            case IFILE_RDSTAT_NOOP:
            case IFILE_RDSTAT_FAIL:
            case IFILE_RDSTAT_EXPIRED:
            default:
                xtra->asyn_status = BINIO_ASYNC_FAIL;
                break;
            }
        }

        status = xtra->asyn_status;

        if (status == BINIO_ASYNC_FAIL || status == BINIO_ASYNC_DONE)
        {
            xtra->asyn_status = BINIO_ASYNC_NOOP;
            g_async_context = 0;
        }
    }

    return status;
}

static signed int ReadRaw(st_FILELOADINFO* fli, void* data, signed int size, signed int count)
{
    tag_xFile* file = (tag_xFile*)fli->privdata;

    if (fli->error != FIOERR_NONE)
    {
        return 0;
    }

    signed int amt = (size * count > fli->remain) ? fli->remain / size : count;

    if (amt)
    {
        signed int n = BFD_read(data, size, amt, file, fli->xtradata);

        if (n != amt)
        {
            fli->error = FIOERR_READFAIL;
        }

        fli->remain -= size * amt;
        fli->position += size * amt;
    }

    return amt;
}

static void Swap2(char* d, signed int n)
{
    char t;

    while (n--)
    {
        t = d[0];
        d[0] = d[1];
        d[1] = t;

        d += 2;
    }
}

static void Swap4(char* d, signed int n)
{
    char t;

    while (n--)
    {
        t = d[0];
        d[0] = d[3];
        d[3] = t;

        t = d[1];
        d[1] = d[2];
        d[2] = t;

        d += 4;
    }
}

static void Swap8(char* d, signed int n)
{
    char t;

    while (n--)
    {
        t = d[0];
        d[0] = d[7];
        d[7] = t;

        t = d[1];
        d[1] = d[6];
        d[6] = t;

        t = d[2];
        d[2] = d[5];
        d[5] = t;

        t = d[3];
        d[3] = d[4];
        d[4] = t;

        d += 8;
    }
}
#pragma auto_inline off

// Functions are in reverse DOL order (-inline deferred emits last to first).

st_FILELOADINFO* xBinioLoadCreate(char* filename)
{
    st_FILELOADINFO* fli = 0;
    tag_xFile* tmp_fp;
    signed int i;
    signed int uselock = -1;

    for (i = 0; i < 8; i++)
    {
        if (!(g_loadlock & (1 << i)))
        {
            g_loadlock |= 1 << i;
            uselock = i;
            fli = &g_loadinst[i];
            break;
        }
    }

    if (fli)
    {
        memset(fli, 0, sizeof(st_FILELOADINFO));

        fli->lockid = uselock & ~(uselock >> 31);
        fli->xtradata = &g_xtraload[uselock];
        fli->destroy = LoadDestroy;
        fli->readBytes = ReadBytes;
        fli->readMShorts = ReadMShorts;
        fli->readMLongs = ReadMLongs;
        fli->readMFloats = ReadMFloats;
        fli->readMDoubles = ReadMDoubles;
        fli->readIShorts = ReadIShorts;
        fli->readILongs = ReadILongs;
        fli->readIFloats = ReadIFloats;
        fli->readIDoubles = ReadIDoubles;
        fli->asyncIRead = AsyncIRead;
        fli->asyncMRead = AsyncMRead;
        fli->asyncReadStatus = AsyncReadStatus;
        fli->skipBytes = SkipBytes;
        fli->seekSpot = ReadSeek;
        fli->setDoubleBuf = SetBuffer;
        fli->discardDblBuf = DiscardBuffer;
        fli->error = FIOERR_NONE;
        fli->remain = 0;
        fli->basesector = 0;
        fli->filesize = 0;
        fli->position = 0;
        fli->basesector = 0;

        tmp_fp = BFD_open(filename, "rb", uselock, 0, fli->xtradata);

        if (tmp_fp)
        {
            fli->basesector = BFD_startSector(filename);
            fli->privdata = tmp_fp;
            fli->filesize = BFD_getLength(tmp_fp, fli->xtradata);
            fli->remain = fli->filesize;
        }
        else
        {
            LoadDestroy(fli);
            fli = 0;
        }
    }

    return fli;
}

#pragma auto_inline on
static void LoadDestroy(st_FILELOADINFO* fli)
{
    tag_xFile* fp = (tag_xFile*)fli->privdata;

    if (fp)
    {
        BFD_close(fp, fli->xtradata);
    }

    unsigned int lockid = fli->lockid;

    memset(fli, 0, sizeof(st_FILELOADINFO));

    g_loadlock &= ~(1 << lockid);
}
#pragma auto_inline off

static signed int SkipBytes(st_FILELOADINFO* fli, signed int fwd)
{
    tag_xFile* file = (tag_xFile*)fli->privdata;

    if (fli->error != FIOERR_NONE)
    {
        return 0;
    }

    if (fwd == 0)
    {
        return 1;
    }

    if (fwd < 0)
    {
        fwd = 0;
    }

    if (BFD_seek(file, fli->position + fwd, 0, fli->xtradata) != 0)
    {
        fli->error = FIOERR_SEEKFAIL;
    }
    else
    {
        fli->remain -= fwd;
        fli->position += fwd;
    }

    if (fli->error != FIOERR_NONE)
    {
        return 0;
    }

    return 1;
}

static signed int ReadSeek(st_FILELOADINFO* fli, signed int pos)
{
    tag_xFile* file = (tag_xFile*)fli->privdata;

    if (fli->error != FIOERR_NONE)
    {
        return 0;
    }

    pos = pos < fli->filesize ? pos : fli->filesize;

    if (BFD_seek(file, pos, 0, fli->xtradata) != 0)
    {
        fli->error = FIOERR_SEEKFAIL;
    }
    else
    {
        fli->position = pos;
        fli->remain = fli->filesize - pos;
    }

    if (fli->error != FIOERR_NONE)
    {
        return 0;
    }

    return 1;
}

static void SetBuffer(st_FILELOADINFO* fli, char* dblbuffer, signed int bufsize)
{
    st_BINIO_XTRADATA* xtra = (st_BINIO_XTRADATA*)fli->xtradata;

    if (xtra)
    {
        xtra->dbl_buf = dblbuffer;
        xtra->dblbuf_size = bufsize;
        xtra->dbl_beg = 0;
        xtra->dbl_end = 0;
        xtra->dbl_amt = 0;
    }
}

static void DiscardBuffer(st_FILELOADINFO* fli)
{
    st_BINIO_XTRADATA* xtra = (st_BINIO_XTRADATA*)fli->xtradata;

    if (xtra)
    {
        xtra->dbl_buf = 0;
        xtra->dblbuf_size = 0;
        xtra->dbl_beg = 0;
        xtra->dbl_end = 0;
        xtra->dbl_amt = 0;
    }
}

static signed int ReadBytes(st_FILELOADINFO* fli, char* data, signed int count)
{
    return ReadRaw(fli, data, 1, count);
}

#ifdef PC_PORT
// Little-endian host: big-endian ("Motorola") values need swapping, Intel ones don't.
#define PC_SWAP_M(fn, d, n) fn((char*)(d), n)
#define PC_SWAP_I(fn, d, n)
#else
#define PC_SWAP_M(fn, d, n)
#define PC_SWAP_I(fn, d, n) fn((char*)(d), n)
#endif

static signed int ReadMShorts(st_FILELOADINFO* fli, signed short* data, signed int count)
{
    signed int act = ReadRaw(fli, data, sizeof(signed short), count);
    PC_SWAP_M(Swap2, data, act);
    return act;
}

static signed int ReadMLongs(st_FILELOADINFO* fli, signed int* data, signed int count)
{
    signed int act = ReadRaw(fli, data, sizeof(signed int), count);
    PC_SWAP_M(Swap4, data, act);
    return act;
}

static signed int ReadMFloats(st_FILELOADINFO* fli, float* data, signed int count)
{
    signed int act = ReadRaw(fli, data, sizeof(float), count);
    PC_SWAP_M(Swap4, data, act);
    return act;
}

static signed int ReadMDoubles(st_FILELOADINFO* fli, double* data, signed int count)
{
    signed int act = ReadRaw(fli, data, sizeof(double), count);
    PC_SWAP_M(Swap8, data, act);
    return act;
}

static signed int ReadIShorts(st_FILELOADINFO* fli, signed short* data, signed int count)
{
    signed int act = ReadRaw(fli, data, sizeof(signed short), count);
    PC_SWAP_I(Swap2, data, act);
    return act;
}

static signed int ReadILongs(st_FILELOADINFO* fli, signed int* data, signed int count)
{
    signed int act = ReadRaw(fli, data, sizeof(signed int), count);
    PC_SWAP_I(Swap4, data, act);
    return act;
}

static signed int ReadIFloats(st_FILELOADINFO* fli, float* data, signed int count)
{
    signed int act = ReadRaw(fli, data, sizeof(float), count);
    PC_SWAP_I(Swap4, data, act);
    return act;
}

static signed int ReadIDoubles(st_FILELOADINFO* fli, double* data, signed int count)
{
    signed int act = ReadRaw(fli, data, sizeof(double), count);
    PC_SWAP_I(Swap8, data, act);
    return act;
}

static signed int AsyncMRead(st_FILELOADINFO* fli, signed int offset, char* data, signed int size,
                             signed int n)
{
    return BFD_AsyncRead(fli, offset, data, size, n, 1);
}

static signed int AsyncIRead(st_FILELOADINFO* fli, signed int offset, char* data, signed int size,
                             signed int n)
{
    return BFD_AsyncRead(fli, offset, data, size, n, 0);
}

static en_BIO_ASYNC_ERRCODES AsyncReadStatus(st_FILELOADINFO* fli)
{
    return BFD_AsyncReadStatus(fli);
}

static tag_xFile* BFD_open(char* filename, char* mode, unsigned int lockid, signed int,
                           void* xtradata)
{
    st_BINIO_XTRADATA* xtra = (st_BINIO_XTRADATA*)xtradata;
    tag_xFile* bffp;
    signed int xfflg = 1;

    if (strcmp(mode, "rb") == 0)
    {
        xfflg = 1;
        bffp = &g_xfload[lockid];
    }
    else if (strcmp(mode, "wb") == 0)
    {
        bffp = 0;
        xfflg = 2;
    }
    else
    {
        bffp = 0;
    }

    if (bffp)
    {
        strncpy(bffp->relname, filename, 15);
        bffp->relname[15] = '\0';

        if (iFileOpen(filename, xfflg, bffp))
        {
            bffp = 0;
        }
        else
        {
            xtra->fpos = 0;
            xtra->dbl_buf = 0;
            xtra->dblbuf_size = 0;
            xtra->dbl_beg = 0;
            xtra->dbl_end = 0;
            xtra->dbl_amt = 0;
        }
    }

    return bffp;
}

static signed int BFD_read(void* data, signed int elesize, signed int elecnt, tag_xFile* bffp,
                           void* xtradata)
{
    st_BINIO_XTRADATA* xtra = (st_BINIO_XTRADATA*)xtradata;
    char* dest = (char*)data;
    signed int readbeg;
    signed int refill = 0;
    signed int remain;
    signed int actual;
    unsigned int holdpos;
    unsigned int numBytes = 0;
    unsigned int safety = 0;
    signed int total = elesize * elecnt;

    if (total == 0)
    {
        return 0;
    }

    if (!xtra->dbl_buf || xtra->dblbuf_size < 1 || total > xtra->dblbuf_size)
    {
        iFileSeek(bffp, xtra->fpos, 0);
        numBytes = iFileRead(bffp, dest, total);
        xtra->fpos += numBytes;
    }
    else
    {
        remain = total;

        while (1)
        {
            if (xtra->fpos < xtra->dbl_beg)
            {
                refill = 1;
                xtra->dbl_beg = xtra->fpos - (xtra->fpos % xtra->dblbuf_size);
                xtra->dbl_end = xtra->dbl_beg + xtra->dblbuf_size;
                xtra->dbl_amt = 0;
            }
            else if (xtra->fpos >= xtra->dbl_end)
            {
                refill = 1;
                xtra->dbl_beg = xtra->fpos - (xtra->fpos % xtra->dblbuf_size);
                xtra->dbl_end = xtra->dbl_beg + xtra->dblbuf_size;
                xtra->dbl_amt = 0;
            }

            if (refill)
            {
                holdpos = xtra->fpos;
                iFileSeek(bffp, xtra->dbl_beg, 0);
                xtra->fpos = holdpos;
                xtra->dbl_amt = iFileRead(bffp, xtra->dbl_buf, xtra->dblbuf_size);
            }

            readbeg = xtra->fpos - xtra->dbl_beg;

            if (xtra->dbl_amt > readbeg)
            {
                actual = xtra->dbl_amt - readbeg;
                if (remain < actual)
                {
                    actual = remain;
                }

                memcpy(dest + numBytes, xtra->dbl_buf + readbeg, actual);

                numBytes += actual;
                remain -= actual;
                xtra->fpos += actual;
            }

            if (numBytes == total || xtra->dbl_amt < 1 || safety++ > 60000)
            {
                break;
            }
        }
    }

    return (signed int)numBytes / elesize;
}

static void BFD_cb_asyncread(signed int)
{
}

static signed int BFD_AsyncRead(st_FILELOADINFO* fli, signed int pos, void* data, signed int size,
                                signed int n, signed int endian)
{
    tag_xFile* file = (tag_xFile*)fli->privdata;
    st_BINIO_XTRADATA* xtra = (st_BINIO_XTRADATA*)fli->xtradata;

    if (fli->error != FIOERR_NONE)
    {
        return 0;
    }

    if (g_async_context)
    {
        return 0;
    }

    g_async_context = xtra;

    xtra->asyn_status = BINIO_ASYNC_INPROG;
    xtra->asyn_data = (char*)data;
    xtra->asyn_elesize = size;
    xtra->asyn_amt = n;
    xtra->asyn_ismot = 1;

    iFileSeek(file, pos, 0);
    file->user_data = fli;

    signed int result = iFileReadAsync(file, data, size * n, BFD_cb_asyncread);
    signed int rc;

    if (result < 0)
    {
        rc = 0;
    }
    else
    {
        xtra->gcaskey = result;
        rc = 1;
    }

    if (rc == 0)
    {
        g_async_context = 0;
        xtra->asyn_status = BINIO_ASYNC_NOOP;
    }

    return rc;
}

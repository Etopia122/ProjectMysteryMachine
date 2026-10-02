extern "C" void* memset(void* dst, int c, unsigned int n);

enum en_READ_ASYNC_STATUS
{
    HIP_RDSTAT_NONE = -1,
    HIP_RDSTAT_INPROG = 0,
    HIP_RDSTAT_SUCCESS = 1,
    HIP_RDSTAT_FAILED = 2,
    HIP_RDSTAT_NOBYPASS = 3,
    HIP_RDSTAT_NOASYNC = 4,
};

enum en_BIO_ASYNC_ERRCODES
{
    BINIO_ASYNC_FAIL = -1,
    BINIO_ASYNC_NOOP = 0,
    BINIO_ASYNC_INPROG = 1,
    BINIO_ASYNC_DONE = 2,
    BINIO_ASYNC_FORCEENUMSIZEINT = 2147483647,
};

enum en_FIOERRCODES
{
    FIOERR_NONE = 0,
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
    unsigned int lockid;
    en_FIOERRCODES error;
    unsigned int basesector;
    void* privdata;
    void* xtradata;
    void* asyndata;
    signed int filesize;
    signed int remain;
    signed int position;
};

class st_HIPLOADBLOCK
{
public:
    signed int endpos;
    unsigned int blk_id;
    signed int blk_remain;
    signed int flags;
};

class st_HIPLOADDATA
{
public:
    st_FILELOADINFO* fli;
    signed int lockid;
    signed int bypass;
    signed int bypass_recover;
    unsigned int base_sector;
    signed int use_async;
    en_READ_ASYNC_STATUS asyn_stat;
    signed int pos;
    signed int top;
    signed int readTop;
    st_HIPLOADBLOCK stk[8];
};

class st_HIPLOADFUNCS
{
public:
    st_HIPLOADDATA* (*create)(char*, char*, signed int);
    void (*destroy)(st_HIPLOADDATA*);
    unsigned int (*basesector)(st_HIPLOADDATA*);
    unsigned int (*enter)(st_HIPLOADDATA*);
    void (*exit)(st_HIPLOADDATA*);
    signed int (*readBytes)(st_HIPLOADDATA*, char*, signed int);
    signed int (*readShorts)(st_HIPLOADDATA*, signed short*, signed int);
    signed int (*readLongs)(st_HIPLOADDATA*, signed int*, signed int);
    signed int (*readFloats)(st_HIPLOADDATA*, float*, signed int);
    signed int (*readString)(st_HIPLOADDATA*, char*);
    signed int (*setBypass)(st_HIPLOADDATA*, signed int, signed int);
    void (*setSpot)(st_HIPLOADDATA*, signed int);
    en_READ_ASYNC_STATUS (*pollRead)(st_HIPLOADDATA*);
};

st_FILELOADINFO* xBinioLoadCreate(char* filename);

static st_HIPLOADDATA* HIPLCreate(char* filename, char* dblbuf, signed int bufsize);
static void HIPLDestroy(st_HIPLOADDATA* lddata);
static unsigned int HIPLBaseSector(st_HIPLOADDATA* lddata);
static signed int HIPLSetBypass(st_HIPLOADDATA* lddata, signed int enable, signed int use_async);
static void HIPLSetSpot(st_HIPLOADDATA* lddata, signed int spot);
static unsigned int HIPLBlockEnter(st_HIPLOADDATA* lddata);
static void HIPLBlockExit(st_HIPLOADDATA* lddata);
static signed int HIPLBlockRead(st_HIPLOADDATA* lddata, void* data, signed int cnt, signed int size);
static en_READ_ASYNC_STATUS HIPLPollRead(st_HIPLOADDATA* lddata);
static signed int HIPLReadBytes(st_HIPLOADDATA* lddata, char* data, signed int cnt);
static signed int HIPLReadShorts(st_HIPLOADDATA* lddata, signed short* data, signed int cnt);
static signed int HIPLReadLongs(st_HIPLOADDATA* lddata, signed int* data, signed int cnt);
static signed int HIPLReadFloats(st_HIPLOADDATA* lddata, float* data, signed int cnt);
static signed int HIPLReadString(st_HIPLOADDATA* lddata, char* buf);

static unsigned int g_loadlock;
static st_HIPLOADDATA g_hiploadinst[8] = { 0 };

static st_HIPLOADFUNCS g_map_HIPL_funcmap = {
    HIPLCreate,     HIPLDestroy,   HIPLBaseSector, HIPLBlockEnter,  HIPLBlockExit,
    HIPLReadBytes,  HIPLReadShorts, HIPLReadLongs, HIPLReadFloats,  HIPLReadString,
    HIPLSetBypass,  HIPLSetSpot,   HIPLPollRead,
};

static inline signed int HIPLReadAsync(st_HIPLOADDATA* lddata, signed int pos, char* data,
                                       signed int cnt, signed int size)
{
    signed int rc;

    lddata->asyn_stat = HIP_RDSTAT_NONE;
    rc = lddata->fli->asyncMRead(lddata->fli, pos, data, cnt, size);
    lddata->asyn_stat = HIP_RDSTAT_INPROG;
    return rc;
}

static inline signed int HIPLBypassRead(st_HIPLOADDATA* lddata, void* data, signed int cnt,
                                        signed int size)
{
    signed int got = 0;

    if (!lddata->bypass)
    {
        return 0;
    }

    if (lddata->use_async)
    {
        return HIPLReadAsync(lddata, lddata->pos, (char*)data, cnt, size);
    }

    if (cnt == 0)
    {
        return 0;
    }

    if (cnt == 0)
    {
        got = 0;
    }
    else if (size == 1)
    {
        got = lddata->fli->readBytes(lddata->fli, (char*)data, cnt);
    }
    else if (size == 2)
    {
        got = lddata->fli->readMShorts(lddata->fli, (signed short*)data, cnt);
    }
    else if (size == 4)
    {
        got = lddata->fli->readMLongs(lddata->fli, (signed int*)data, cnt);
    }

    return got * size;
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

st_HIPLOADFUNCS* get_HIPLFuncs()
{
    return &g_map_HIPL_funcmap;
}

static st_HIPLOADDATA* HIPLCreate(char* filename, char* dblbuf, signed int bufsize)
{
    st_HIPLOADDATA* lddata = 0;
    st_FILELOADINFO* fli;
    st_HIPLOADBLOCK* tmp_blk;
    signed int i;
    signed int uselock = -1;

    for (i = 0; i < 8; i++)
    {
        if (!(g_loadlock & (1 << i)))
        {
            g_loadlock |= 1 << i;
            uselock = i;
            lddata = &g_hiploadinst[i];
            break;
        }
    }

    if (lddata)
    {
        memset(lddata, 0, sizeof(st_HIPLOADDATA));

        lddata->lockid = uselock;
        lddata->top = -1;
        lddata->base_sector = 0;
        lddata->use_async = 0;
        lddata->asyn_stat = HIP_RDSTAT_NONE;
        lddata->bypass = 0;
        lddata->bypass_recover = -1;
        lddata->pos = 0;
        lddata->readTop = 0;

        for (i = 0; i < 8; i++)
        {
            tmp_blk = &lddata->stk[i];
            tmp_blk->endpos = 0;
            tmp_blk->blk_id = 0;
            tmp_blk->blk_remain = 0;
            tmp_blk->flags = 0;
        }

        fli = xBinioLoadCreate(filename);
        if (fli)
        {
            lddata->fli = fli;
            lddata->base_sector = fli->basesector;
            if (dblbuf && bufsize > 0)
            {
                fli->setDoubleBuf(fli, dblbuf, bufsize);
            }
        }
        else
        {
            HIPLDestroy(lddata);
            lddata = 0;
        }
    }

    return lddata;
}

#pragma auto_inline on
static void HIPLDestroy(st_HIPLOADDATA* lddata)
{
    signed int lockid;

    if (lddata)
    {
        if (lddata->fli)
        {
            lddata->fli->destroy(lddata->fli);
        }
        lockid = lddata->lockid;
        memset(lddata, 0, sizeof(st_HIPLOADDATA));
        g_loadlock &= ~(1 << lockid);
    }
}
#pragma auto_inline off

static unsigned int HIPLBaseSector(st_HIPLOADDATA* lddata)
{
    return lddata->base_sector;
}

static signed int HIPLSetBypass(st_HIPLOADDATA* lddata, signed int enable, signed int use_async)
{
    lddata->fli->discardDblBuf(lddata->fli);

    if (enable && lddata->bypass)
    {
        return 0;
    }
    if (!enable && !lddata->bypass)
    {
        return 0;
    }

    if (enable)
    {
        lddata->bypass = 1;
        lddata->use_async = use_async;
        lddata->bypass_recover = lddata->fli->position;
    }
    else
    {
        lddata->fli->seekSpot(lddata->fli, lddata->bypass_recover);
        lddata->bypass = 0;
        lddata->use_async = 0;
        lddata->bypass_recover = -1;
    }

    return 1;
}

static void HIPLSetSpot(st_HIPLOADDATA* lddata, signed int spot)
{
    if (lddata->bypass)
    {
        lddata->pos = spot;
        lddata->fli->seekSpot(lddata->fli, spot);
    }
}

static unsigned int HIPLBlockEnter(st_HIPLOADDATA* lddata)
{
    st_HIPLOADBLOCK* top;
    unsigned int cid = 0;
    signed int size = 0;

    if (lddata->bypass)
    {
        return 0;
    }

    if (lddata->top >= 0 && lddata->stk[lddata->top].blk_remain <= 0)
    {
        return 0;
    }

    if (HIPLReadLongs(lddata, (signed int*)&cid, -1) == 0)
    {
        cid = 0;
    }
    else
    {
        HIPLReadLongs(lddata, &size, -1);

        if (lddata->top >= 0)
        {
            lddata->stk[lddata->top].blk_remain -= size;
        }

        top = &lddata->stk[++lddata->top];
        top->blk_id = cid;
        top->blk_remain = size;
        top->endpos = lddata->pos + top->blk_remain + (top->blk_remain & 1);
        top->flags = 0;
    }

    return cid;
}

static void HIPLBlockExit(st_HIPLOADDATA* lddata)
{
    st_HIPLOADBLOCK* top;

    if (!lddata->bypass)
    {
        top = &lddata->stk[lddata->top--];
        lddata->fli->skipBytes(lddata->fli, top->endpos - lddata->pos);
        lddata->pos = top->endpos;
    }
}

static signed int HIPLBlockRead(st_HIPLOADDATA* lddata, void* data, signed int cnt, signed int size)
{
    st_HIPLOADBLOCK* top;
    signed int got = 0;
    signed int left = 0;
    signed int head = 0;

    if (lddata->bypass)
    {
        return 0;
    }

    if (cnt == 0)
    {
        return 0;
    }

    if (lddata->top < 0)
    {
        top = 0;
    }
    else
    {
        top = &lddata->stk[lddata->top];
        left = top->blk_remain / size;
    }

    if (cnt < 0)
    {
        cnt = -cnt;
        head = 1;
        if (top && cnt > left)
        {
            cnt = left;
        }
    }

    if (!head && left < cnt)
    {
        cnt = left;
    }

    if (cnt == 0)
    {
        got = 0;
    }
    else if (size == 1)
    {
        got = lddata->fli->readBytes(lddata->fli, (char*)data, cnt);
    }
    else if (size == 2)
    {
        got = lddata->fli->readMShorts(lddata->fli, (signed short*)data, cnt);
    }
    else if (size == 4)
    {
        got = lddata->fli->readMLongs(lddata->fli, (signed int*)data, cnt);
    }

    got *= size;
    lddata->pos += got;
    if (top)
    {
        top->blk_remain -= got;
    }

    return got;
}

static en_READ_ASYNC_STATUS HIPLPollRead(st_HIPLOADDATA* lddata)
{
    en_READ_ASYNC_STATUS rdstat = HIP_RDSTAT_INPROG;
    en_BIO_ASYNC_ERRCODES pollstat;

    if (!lddata->bypass)
    {
        return HIP_RDSTAT_NOBYPASS;
    }

    if (!lddata->use_async)
    {
        return HIP_RDSTAT_NOASYNC;
    }

    pollstat = lddata->fli->asyncReadStatus(lddata->fli);

    switch (pollstat)
    {
    case BINIO_ASYNC_INPROG:
        rdstat = HIP_RDSTAT_INPROG;
        break;
    case BINIO_ASYNC_DONE:
        rdstat = HIP_RDSTAT_SUCCESS;
        lddata->asyn_stat = HIP_RDSTAT_NONE;
        break;
    case BINIO_ASYNC_FAIL:
        rdstat = HIP_RDSTAT_FAILED;
        lddata->asyn_stat = HIP_RDSTAT_NONE;
        break;
    default:
        lddata->asyn_stat = HIP_RDSTAT_NONE;
        break;
    }

    return rdstat;
}

static signed int HIPLReadBytes(st_HIPLOADDATA* lddata, char* data, signed int cnt)
{
    if (lddata->bypass)
    {
        return HIPLBypassRead(lddata, data, cnt, sizeof(char));
    }
    else
    {
        return HIPLBlockRead(lddata, data, cnt, sizeof(char));
    }
}

static signed int HIPLReadShorts(st_HIPLOADDATA* lddata, signed short* data, signed int cnt)
{
    signed int got;

    if (lddata->bypass)
    {
        got = HIPLBypassRead(lddata, data, cnt, sizeof(signed short));
    }
    else
    {
        got = HIPLBlockRead(lddata, data, cnt, sizeof(signed short));
    }

    got /= sizeof(signed short);
    return got;
}

#pragma auto_inline on
static signed int HIPLReadLongs(st_HIPLOADDATA* lddata, signed int* data, signed int cnt)
{
    signed int got;

    if (lddata->bypass)
    {
        got = HIPLBypassRead(lddata, data, cnt, sizeof(signed int));
    }
    else
    {
        got = HIPLBlockRead(lddata, data, cnt, sizeof(signed int));
    }

    got /= sizeof(signed int);
    return got;
}
#pragma auto_inline off

static signed int HIPLReadFloats(st_HIPLOADDATA* lddata, float* data, signed int cnt)
{
    signed int got;

    if (lddata->bypass)
    {
        got = HIPLBypassRead(lddata, data, cnt, sizeof(float));
    }
    else
    {
        got = HIPLBlockRead(lddata, data, cnt, sizeof(float));
    }

    got /= sizeof(float);
    return got;
}

static signed int HIPLReadString(st_HIPLOADDATA* lddata, char* buf)
{
    signed int n = 0;
    char pad = 0;

    if (lddata->bypass)
    {
        while (HIPLBypassRead(lddata, &buf[n], 1, sizeof(char)))
        {
            if (buf[n] == '\0')
            {
                if (!(n & 1))
                {
                    HIPLBypassRead(lddata, &pad, 1, sizeof(char));
                }
                break;
            }
            n++;
        }
    }
    else
    {
        while (HIPLBlockRead(lddata, &buf[n], 1, sizeof(char)))
        {
            if (buf[n] == '\0')
            {
                if (!(n & 1))
                {
                    HIPLBlockRead(lddata, &pad, 1, sizeof(char));
                }
                break;
            }
            n++;
        }
    }

    return n;
}

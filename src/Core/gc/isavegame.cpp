#include "isavegame.h"
#include "xMemMgr.h"
#include "zGlobals.h"

extern "C" {
void* memcpy(void* dst, const void* src, unsigned int n);
void* memset(void* dst, int c, unsigned int n);
int memcmp(const void* a, const void* b, unsigned int n);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, unsigned int n);
unsigned int strlen(const char* s);
int sprintf(char* s, const char* format, ...);

typedef void (*CARDCallback)(signed long chan, signed long result);

typedef struct OSCalendarTime
{
    int sec;
    int min;
    int hour;
    int mday;
    int mon;
    int year;
    int wday;
    int yday;
    int msec;
    int usec;
} OSCalendarTime;

void CARDInit();
signed long CARDGetResultCode(signed long chan);
signed long CARDGetXferredBytes(signed long chan);
signed long CARDFreeBlocks(signed long chan, signed long* byteNotUsed, signed long* filesNotUsed);
signed long CARDGetEncoding(signed long chan, unsigned short* encode);
signed long CARDGetSectorSize(signed long chan, unsigned long* size);
signed long CARDCheckEx(signed long chan, signed long* xferBytes);
signed long CARDProbeEx(signed long chan, signed long* memSize, signed long* sectorSize);
signed long CARDMount(signed long chan, void* workArea, CARDCallback detachCallback);
signed long CARDUnmount(signed long chan);
signed long CARDFormat(signed long chan);
// The older SDK's CARDOpen/CARDClose; the TWW CARDOpen.o object already defines those names.
signed long fn_80135484(signed long chan, const char* fileName, CARDFileInfo* fileInfo);
signed long fn_8013558C(CARDFileInfo* fileInfo);
#define CARDOpen fn_80135484
#define CARDClose fn_8013558C
signed long CARDCreate(signed long chan, const char* fileName, unsigned long size,
                       CARDFileInfo* fileInfo);
signed long CARDReadAsync(CARDFileInfo* fileInfo, void* addr, signed long length,
                          signed long offset, CARDCallback callback);
signed long CARDRead(CARDFileInfo* fileInfo, void* addr, signed long length, signed long offset);
signed long CARDWriteAsync(CARDFileInfo* fileInfo, const void* addr, signed long length,
                           signed long offset, CARDCallback callback);
signed long CARDDelete(signed long chan, const char* fileName);
signed long CARDGetStatus(signed long chan, signed long fileNo, CARDStat* stat);
signed long CARDSetStatus(signed long chan, signed long fileNo, CARDStat* stat);
signed long CARDSetAttributes(signed long chan, signed long fileNo, unsigned char attr);

long long OSGetTime();
void OSTicksToCalendarTime(long long ticks, OSCalendarTime* td);
void OSFreeToHeap(int heap, void* ptr);
extern volatile int __OSCurrHeap;

void fn_80016768();
}

unsigned int* iFileLoad(char* name, unsigned int* buffer, unsigned int* size);
void xSndUpdate(float dt);

#ifndef NULL
#define NULL 0
#endif

#define CARD_RESULT_READY 0
#define CARD_RESULT_BUSY -1
#define CARD_RESULT_WRONGDEVICE -2
#define CARD_RESULT_NOCARD -3
#define CARD_RESULT_NOFILE -4
#define CARD_RESULT_IOERROR -5
#define CARD_RESULT_BROKEN -6
#define CARD_RESULT_EXIST -7
#define CARD_RESULT_NOENT -8
#define CARD_RESULT_INSSPACE -9
#define CARD_RESULT_NOPERM -10
#define CARD_RESULT_ENCODING -13
#define CARD_RESULT_FATAL_ERROR -128

#define ALIGN_THING(x, n) (n + x - 1 & -x)

class st_ISG_TPL_TEX
{
public:
    class UnkIn
    {
    public:
        unsigned int unk_0;
        unsigned int unk_4;
        void* unk_8;
        unsigned int unk_c;
        unsigned int unk_10[4];
        unsigned char unk_20;
        unsigned char unk_21;
        unsigned char unk_22;
        unsigned char unk_23;
    };
    class UnkOut
    {
    public:
        unsigned char unk_0;
        unsigned char unk_1;
        unsigned char unk_2;
        unsigned char unk_3;
        unsigned int unk_4;
        void* unk_8;
    };

    UnkIn* unk_0;
    UnkOut* unk_4;
};

class st_ISG_TPL_TEXPALETTE
{
public:
    unsigned int magic;
    unsigned int count;
    st_ISG_TPL_TEX* unk_8;
};

class IconData
{
public:
    char game_name[0x20];
    char buf0[0x20];
    char buf1[0x1800];
    char buf2[4][0x800];
    char pad[0x200];
    char footer[0x200];
};

static char cardwork[2][0xA000] __attribute__((aligned(32)));

static signed int g_isginit;
static st_ISG_TPL_TEXPALETTE* g_rawicon;
static st_ISG_TPL_TEXPALETTE* g_rawbanr;
static unsigned int g_iconsize;
static unsigned int g_banrsize;

static st_ISGSESSION g_isgdata_MAIN = { 0 };
static signed int g_legalSectSize[] = { 0x2000, 0, -1 };

#pragma auto_inline on

// Functions are in reverse DOL order (-inline deferred emits last to first).
// TODO: .rodata starts with the strings of a dead-stripped function ("AB------",
// "is damaged and cannot be used", ... and a 0x40-byte zero initializer) that is not
// reconstructed yet. Near-misses: mc_format/fopen switch trees (extra blt), SaveFile and
// ReadLeader register swaps.

static signed int iSG_tpl_unpack(st_ISG_TPL_TEXPALETTE* tpl);
static signed int iSG_load_icondata();
static signed int iSG_get_fmoddate(st_ISG_MEMCARD_DATA* mcdata, const char* fname, int* sec,
                                   int* min, int* hr, int* mon, int* day, int* yr);
static void iSG_cb_unmount(signed long chan, signed long result);
static signed int iSG_mc_isGCcard(st_ISG_MEMCARD_DATA* mcdata, signed int* param2,
                                  signed int* param3);
static signed int iSG_mc_format(st_ISG_MEMCARD_DATA* mcdata, signed int, signed int* canRecover);
static signed int iSG_mc_fopen(st_ISG_MEMCARD_DATA* mcdata, const char* fname, signed int fsize,
                               en_ISG_IOMODE mode, en_ASYNC_OPERR* operr);
static signed int iSG_isSpaceForFile(st_ISG_MEMCARD_DATA* mcdata, signed int fsize,
                                     const char* fname, signed int* bytesNeeded,
                                     signed int* availOnDisk, signed int* needFile);
static signed int iSG_find_gamefile(st_ISG_MEMCARD_DATA* mcdata, signed int which, char* name,
                                    signed int* count);
static signed int iSG_curKosher(CARDStat* stat, CARDFileInfo* info);
static char* iSG_bfr_icondata(char* buf, CARDStat* stat, char* label, signed int sectorSize);
static signed int iSG_mcqa_fread(st_ISG_MEMCARD_DATA* mcdata, char* buf, signed int len,
                                 signed int offset);
static signed int iSG_mcqa_fwrite(st_ISG_MEMCARD_DATA* mcdata, char* buf, signed int len);

static signed int iSG_start_your_engines()
{
    CARDInit();
    return 1;
}

static void iSG_discard_icondata()
{
    OSFreeToHeap(__OSCurrHeap, g_rawicon);
    OSFreeToHeap(__OSCurrHeap, g_rawbanr);
    g_rawicon = NULL;
    g_iconsize = 0;
    g_rawbanr = NULL;
    g_banrsize = 0;
}

static signed int iSG_cubeicon_size(signed int slot, signed int sectorSize)
{
    if ((unsigned int)slot > 1)
    {
        return -1;
    }
    signed int t = ALIGN_THING(sectorSize, 0x200);
    return -t & sizeof(IconData) + t - 1;
}

static signed int iSG_mc_exists(signed int slot)
{
    signed int ret = 0;
    signed long memSize = 0;
    signed long sectorSize = 0;

    if (slot < -1)
    {
        return 0;
    }

    signed long result;
    do
    {
        result = CARDProbeEx(slot, &memSize, &sectorSize);
    } while (result == CARD_RESULT_BUSY);

    if (result == CARD_RESULT_READY)
    {
        ret = 1;
    }
    return ret;
}

static signed int iSG_mc_isformatted(st_ISG_MEMCARD_DATA* mcdata)
{
    signed int result = 0;
    signed int rc = 0;
    signed long xferBytes = 0;

    if (mcdata->unk_0 == 0)
    {
        rc = 0;
    }
    else if (mcdata->unk_12c)
    {
        rc = 0;
    }
    else
    {
        signed long result;
        do
        {
            result = CARDCheckEx(mcdata->chan, &xferBytes);
        } while (result == CARD_RESULT_BUSY);

        if (result == CARD_RESULT_READY)
        {
            rc = 1;
        }
        else if (result == CARD_RESULT_BROKEN)
        {
            rc = 0;
        }
        else
        {
            rc = 0;
        }
    }
    return rc;
}

static signed int iSG_mc_tryRepair(st_ISG_MEMCARD_DATA* mcdata)
{
    signed long xferBytes = 0;

    if (mcdata->unk_0 != 0 && mcdata->unk_12c == 0)
    {
        signed long result;
        do
        {
            result = CARDCheckEx(mcdata->chan, &xferBytes);
        } while (result == CARD_RESULT_BUSY);

        if (result != CARD_RESULT_READY && result != CARD_RESULT_ENCODING &&
            result != CARD_RESULT_BROKEN)
        {
            mcdata->unk_12c = 1;
        }
    }
    return 0;
}

static signed int iSG_mc_mount(signed int slot)
{
    signed int ret = 0;
    signed long result;
    do
    {
        result = CARDMount(slot, cardwork[slot], iSG_cb_unmount);
    } while (result == CARD_RESULT_BUSY);

    if (result == CARD_RESULT_READY)
    {
        ret = 1;
    }
    else if (result == CARD_RESULT_BROKEN || result == CARD_RESULT_ENCODING)
    {
        ret = 1;
    }
    return ret;
}

static signed int iSG_mc_settgt(st_ISG_MEMCARD_DATA* mcdata, signed int slot)
{
    if (iSG_mc_mount(slot))
    {
        mcdata->unk_0 = 1;
        mcdata->chan = slot;
        iSG_mc_tryRepair(mcdata);
        CARDGetSectorSize(slot, (unsigned long*)&mcdata->sectorSize);
    }
    else
    {
        mcdata->unk_0 = 0;
    }
    return mcdata->unk_0;
}

static signed int iSG_mc_fclose(st_ISG_MEMCARD_DATA* mcdata)
{
    while (CARDClose(&mcdata->finfo) == CARD_RESULT_BUSY)
    {
    }
    memset(&mcdata->finfo, 0, sizeof(CARDFileInfo));
    memset(&mcdata->fstat, 0, sizeof(CARDStat));
    return 1;
}

static signed int iSG_get_finfo(st_ISG_MEMCARD_DATA* mcdata, const char* dpath)
{
    signed int rc = 0;
    en_ASYNC_OPERR operr = ISG_OPERR_NONE;

    if (iSG_mc_fopen(mcdata, dpath, -1, ISG_IOMODE_READ, &operr))
    {
        rc = 1;
        memcpy(&mcdata->unk_b0, &mcdata->fstat, sizeof(CARDStat));
        memcpy(&mcdata->unk_9c, &mcdata->finfo, sizeof(CARDFileInfo));
        iSG_mc_fclose(mcdata);
    }
    return rc;
}

static signed int iSG_get_fsize(st_ISG_MEMCARD_DATA* mcdata, const char* fname)
{
    signed int rc = -1;
    if (iSG_get_finfo(mcdata, fname))
    {
        rc = mcdata->unk_b0.length;
    }
    if (rc < 0)
    {
        rc = -1;
    }
    return rc;
}

static signed int iSG_mc_fdel(st_ISG_MEMCARD_DATA* mcdata, const char* fname)
{
    signed long result;
    do
    {
        result = CARDDelete(mcdata->chan, fname);
    } while (result == CARD_RESULT_BUSY);

    signed int ret;
    if (result == CARD_RESULT_NOFILE)
    {
        ret = 1;
    }
    else if (result == CARD_RESULT_READY)
    {
        ret = 1;
    }
    else
    {
        ret = 0;
        switch (result)
        {
        case CARD_RESULT_IOERROR:
            mcdata->unk_12c = 1;
            break;
        case CARD_RESULT_NOPERM:
        case CARD_RESULT_FATAL_ERROR:
        case CARD_RESULT_WRONGDEVICE:
        case CARD_RESULT_NOCARD:
            break;
        }
    }
    return ret;
}

signed int iSGStartup()
{
    if (g_isginit++ != 0)
    {
        return g_isginit;
    }

    iSG_start_your_engines();
    iSG_load_icondata();
    return g_isginit;
}

signed int iSGShutdown()
{
    iSG_discard_icondata();
    return 1;
}

char* iSGMakeName(en_NAMEGEN_TYPE type, const char* base, signed int idx)
{
    static signed int rotate = 0.0f;
    static char rotatebuf[8][32] = { 0 };

    const char* fmt_sd = "%s%02d";
    char* use_buf = rotatebuf[rotate++];
    if (rotate == 8)
    {
        rotate = 0;
    }

    *use_buf = '\0';
    switch (type)
    {
    case ISG_NGTYP_GAMEFILE:
        if (base != NULL)
        {
            sprintf(use_buf, fmt_sd, base, idx);
        }
        else
        {
            sprintf(use_buf, fmt_sd, "ScoobyDoo", idx);
        }
        break;
    case ISG_NGTYP_GAMEDIR:
    case ISG_NGTYP_CONFIG:
    case ISG_NGTYP_ICONTHUM:
        break;
    }

    return use_buf;
}

st_ISGSESSION* iSGSessionBegin(void* cltdata, void (*chgfunc)(void*, en_CHGCODE))
{
    fn_80016768();
    memset(&g_isgdata_MAIN, 0, sizeof(st_ISGSESSION));
    g_isgdata_MAIN.slot = -1;
    return &g_isgdata_MAIN;
}

void iSGSessionEnd(st_ISGSESSION* isgdata)
{
    fn_80016768();
    for (signed int i = 0; i < ISG_NUM_SLOTS; i++)
    {
        if (isgdata->mcdata[i].unk_0)
        {
            while (CARDUnmount(i) == CARD_RESULT_BUSY)
            {
            }
            isgdata->mcdata[i].unk_0 = 0;
        }
    }
    memset(isgdata, 0, sizeof(st_ISGSESSION));
}

signed int iSGTgtCount(st_ISGSESSION* isgdata, signed int* max)
{
    signed long memSize = 0;
    signed long sectorSize = 0;
    signed int ret = 0;

    if (max != NULL)
    {
        *max = 2;
    }

    for (signed int i = 0; i < ISG_NUM_SLOTS; i++)
    {
        fn_80016768();
        signed long result;
        do
        {
            result = CARDProbeEx(i, &memSize, &sectorSize);
        } while (result == CARD_RESULT_BUSY);
        if (result == CARD_RESULT_READY)
        {
            ret++;
        }
    }
    return ret;
}

signed int iSGTgtPhysSlotIdx(st_ISGSESSION* isgdata, signed int tidx)
{
    signed int idx = -1;
    if (iSG_mcidx2slot(tidx, &idx, NULL))
    {
        return idx;
    }
    return -1;
}

signed int iSGCheckMemoryCard(st_ISGSESSION* isgdata, signed int index)
{
    signed long memSize = 0;
    signed long sectorSize = 0;
    signed int ret = 0;

    fn_80016768();
    signed long result;
    do
    {
        result = CARDProbeEx(index, &memSize, &sectorSize);
    } while (result == CARD_RESULT_BUSY);

    if (result == CARD_RESULT_READY)
    {
        ret |= 3;
    }
    else if (result == CARD_RESULT_NOCARD)
    {
        ret = 0;
    }
    else if (result == CARD_RESULT_WRONGDEVICE)
    {
        ret |= 1;
    }
    else if (result == CARD_RESULT_FATAL_ERROR)
    {
        ret = 4;
    }
    else
    {
        ret = 0;
    }
    return ret;
}

char* iSGTgtName(st_ISGSESSION* isgdata, signed int tidx, signed int kind)
{
    static char tgtname[64] = { 0 };
    signed int slot = 0;
    char letters[] = "AB---";

    switch (kind)
    {
    case 5:
    case 6:
    case 7:
        slot = tidx;
        break;
    default:
        if (iSG_mcidx2slot(tidx, &slot, NULL) == 0)
        {
            slot = 3;
        }
        break;
    }

    switch (kind)
    {
    case 0:
    case 6:
        sprintf(tgtname, "Slot %c", letters[slot]);
        break;
    case 1:
        sprintf(tgtname, "Memory Card Slot %c", letters[slot]);
        break;
    case 2:
        sprintf(tgtname, "Nintendo GameCube Memory Card in Slot %c", letters[slot]);
        break;
    case 3:
    case 5:
        sprintf(tgtname, "Nintendo GameCube%s Memory Card in Slot %c", "^", letters[slot]);
        break;
    case 4:
    case 7:
        sprintf(tgtname, "Nintendo GameCube%s Memory Card", "^");
        break;
    default:
        sprintf(tgtname, "Memory Card Slot %c", letters[slot]);
        break;
    }
    return tgtname;
}

signed int iSGTgtFormat(st_ISGSESSION* isgdata, signed int tgtidx, signed int async,
                        signed int* canRecover)
{
    signed int slot = 0;

    fn_80016768();
    if (iSG_mc_exists(isgdata->slot) == 0)
    {
        return 0;
    }

    iSG_mcidx2slot(tgtidx, &slot, NULL);
    if (slot != isgdata->slot)
    {
        return 0;
    }

    st_ISG_MEMCARD_DATA* data = &isgdata->mcdata[isgdata->slot];
    if (data->unk_0 == 0)
    {
        return 0;
    }

    if (iSG_mc_isGCcard(data, NULL, NULL) == 0)
    {
        return 0;
    }

    return iSG_mc_format(data, 0, canRecover) != 0;
}

#pragma auto_inline off

unsigned int iSGTgtState(st_ISGSESSION* isgdata, signed int tgtidx, const char* dpath)
{
    signed int isSectSizeValid = 0;
    unsigned int state = 0;
    signed int slot = 0;
    signed int x = 0;
    signed int y = 0;

    fn_80016768();
    iSG_mcidx2slot(tgtidx, &slot, NULL);
    if (slot != isgdata->slot)
    {
        iSGTgtSetActive(isgdata, tgtidx);
    }

    if (isgdata->slot < 0)
    {
        return 0x1000000;
    }

    st_ISG_MEMCARD_DATA* data = &isgdata->mcdata[isgdata->slot];
    if (data->unk_0 == 0)
    {
        return 0;
    }

    if (iSG_mc_exists(isgdata->slot))
    {
        state |= 1;
    }
    else
    {
        return 0;
    }

    if (data->unk_12c != 0)
    {
        return state | 0x1000000;
    }

    signed int rc = iSG_mc_isGCcard(data, &x, &y);
    if (x != 0)
    {
        state |= 0x4000000;
    }
    if (y != 0)
    {
        state |= 0x2000000;
    }
    if (rc == 0)
    {
        return state;
    }

    if (g_legalSectSize[0] > 0)
    {
        for (signed int i = 0; g_legalSectSize[i] > 0; i++)
        {
            if (data->sectorSize == g_legalSectSize[i])
            {
                isSectSizeValid = 1;
                break;
            }
        }
        if (isSectSizeValid == 0)
        {
            state |= 0x8000004;
            return state & ~4;
        }
    }

    if (iSG_mc_isformatted(data) == 0)
    {
        return state | 4;
    }
    return state | 0xE;
}

#pragma auto_inline on

signed int iSGTgtFreeSpace(st_ISGSESSION* isgdata, signed int tgtidx, signed int* bytes,
                           signed int* files)
{
    signed long byteNotUsed = 0;
    signed long filesNotUsed = 0;

    if (bytes != NULL)
    {
        *bytes = 0;
    }
    if (files != NULL)
    {
        *files = 0;
    }

    if (isgdata->slot < 0)
    {
        return 0;
    }

    st_ISG_MEMCARD_DATA* data = &isgdata->mcdata[isgdata->slot];
    fn_80016768();
    if (iSGTgtState(isgdata, tgtidx, NULL) == 0)
    {
        return 0;
    }

    CARDFreeBlocks(isgdata->slot, &byteNotUsed, &filesNotUsed);
    if (files != NULL)
    {
        *files = filesNotUsed;
    }
    if (bytes != NULL)
    {
        *bytes = ALIGN_THING(data->sectorSize, 0x200);
    }
    return byteNotUsed;
}

signed int iSGTgtSetActive(st_ISGSESSION* isgdata, signed int tgtidx)
{
    signed int ret = 0;
    signed int slot = 0;
    iSG_mcidx2slot(tgtidx, &slot, NULL);
    st_ISG_MEMCARD_DATA* data = &isgdata->mcdata[slot];
    fn_80016768();
    if (data->unk_0)
    {
        ret = 1;
    }
    else if (iSG_mc_settgt(data, slot))
    {
        ret = 1;
        isgdata->slot = slot;
    }
    return ret;
}

signed int iSGTgtHaveRoom(st_ISGSESSION* isgdata, signed int tidx, signed int fsize,
                          const char* dpath, const char* fname, signed int* bytesNeeded,
                          signed int* availOnDisk, signed int* needFile)
{
    if (isgdata->slot < 0)
    {
        return 0;
    }

    st_ISG_MEMCARD_DATA* data = &isgdata->mcdata[isgdata->slot];
    if (data->unk_0 == 0)
    {
        return 0;
    }

    if (fname == NULL)
    {
        for (signed int i = 0; i < ISG_NUM_FILES; i++)
        {
            fn_80016768();
            fname = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, i);
            if (iSG_get_finfo(data, fname) != 0)
            {
                break;
            }
        }
    }

    return iSG_isSpaceForFile(data, fsize, fname, bytesNeeded, availOnDisk, needFile);
}

signed int iSGGameFileCount(st_ISGSESSION* isgdata)
{
    if (isgdata->slot < 0)
    {
        return 0;
    }

    st_ISG_MEMCARD_DATA* data = &isgdata->mcdata[isgdata->slot];
    if (data->unk_0 == 0)
    {
        return 0;
    }

    fn_80016768();
    signed int count = 0;
    iSG_find_gamefile(data, -1, NULL, &count);
    return count;
}

signed int iSGFileSize(st_ISGSESSION* isgdata, const char* fname)
{
    if (isgdata->slot < 0)
    {
        return -1;
    }

    st_ISG_MEMCARD_DATA* data = &isgdata->mcdata[isgdata->slot];
    fn_80016768();
    signed int ret = iSG_get_fsize(data, fname);
    if (ret >= 0)
    {
        ret -= iSG_cubeicon_size(data->chan, data->sectorSize);
        if (ret < 0)
        {
            ret = -1;
        }
    }
    return ret;
}

char* iSGFileModDate(st_ISGSESSION* isgdata, const char* fname)
{
    static char datestr[64] = { 0 };
    int sec = 0;
    int min = 0;
    int hr = 0;
    int mon = 0;
    int day = 0;
    int yr = 0;

    if (iSG_get_fmoddate(&isgdata->mcdata[isgdata->slot], fname, &sec, &min, &hr, &mon, &day,
                         &yr))
    {
        sprintf(datestr, "%02d/%02d/%04d %02d:%02d:%02d", mon, day, yr, hr, min, sec);
    }
    else
    {
        sprintf(datestr, "<Unknown Modification>");
    }
    return datestr;
}

void iSGMakeTimeStamp(char* str)
{
    OSCalendarTime calendar_time = { 0 };
    OSTicksToCalendarTime(OSGetTime(), &calendar_time);
    sprintf(str, "%02d/%02d/%04d %02d:%02d:%02d", calendar_time.mon + 1, calendar_time.mday,
            calendar_time.year, calendar_time.hour, calendar_time.min, calendar_time.sec);
}

static signed int iSG_fileKosher(st_ISG_MEMCARD_DATA* mcdata, const char* fname, signed int fix,
                                 signed int* fixed)
{
    signed int rc;
    en_ASYNC_OPERR operr = ISG_OPERR_NONE;

    if (fixed)
    {
        *fixed = 0;
    }

    if (iSG_mc_fopen(mcdata, fname, -1, ISG_IOMODE_READ, &operr) == 0)
    {
        return -1;
    }

    signed int ret = iSG_curKosher(&mcdata->fstat, &mcdata->finfo);
    iSG_mc_fclose(mcdata);

    if (ret == 0)
    {
        rc = 0;
        if (fix && iSG_mc_fdel(mcdata, fname))
        {
            *fixed = 1;
        }
    }
    else
    {
        rc = 1;
    }
    return rc;
}

#pragma auto_inline off

signed int iSGCheckForCorruptFiles(st_ISGSESSION* isgdata, signed int fix, signed int* firstBad,
                                   signed int* firstFixed)
{
    signed int fixed = 0;
    signed int ret = 1;

    if (firstBad != NULL)
    {
        *firstBad = -1;
    }

    if (isgdata->slot < 0)
    {
        return 1;
    }

    st_ISG_MEMCARD_DATA* mcdata = &isgdata->mcdata[isgdata->slot];
    for (signed int i = 0; i < ISG_NUM_FILES; i++)
    {
        char* name = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, i);
        if (iSG_fileKosher(mcdata, name, fix, &fixed) == 0)
        {
            if (firstBad != NULL && *firstBad < 0)
            {
                *firstBad = i;
            }
            if (fixed != 0 && firstFixed != NULL && *firstFixed < 0)
            {
                *firstFixed = i;
            }
            ret = 0;
            break;
        }
    }

    if (fix)
    {
        if (iSGCheckForCorruptFiles(isgdata, 0, NULL, NULL) > 0)
        {
            ret = 1;
        }
        else
        {
            ret = 0;
        }
    }
    return ret;
}

#pragma auto_inline on

signed int iSGSelectGameDir(st_ISGSESSION* isgdata, const char* dname)
{
    if (isgdata->slot < 0)
    {
        return 0;
    }

    st_ISG_MEMCARD_DATA* data = &isgdata->mcdata[isgdata->slot];
    if (data->unk_0 == 0)
    {
        return 0;
    }

    if (iSG_mc_isGCcard(data, NULL, NULL) == 0)
    {
        return 0;
    }

    return iSG_mc_isformatted(data) != 0;
}

signed int iSGSetupGameDir(st_ISGSESSION* isgdata, const char* dname, signed int force_iconfix)
{
    return 1;
}

static void iSG_upd_icostat(CARDStat*, CARDStat* stat)
{
    stat->commentAddr = 0;
    stat->bannerFormat = (stat->bannerFormat & ~3) | 2;
    stat->iconAddr = 0x40;
    stat->bannerFormat = stat->bannerFormat & ~4;

    for (signed int i = 0; i < 8; i++)
    {
        stat->iconFormat = (stat->iconFormat & ~(3 << (2 * i))) | (2 << (2 * i));
        stat->iconSpeed = (stat->iconSpeed & ~(3 << (2 * i))) | (2 << (2 * i));
    }
    stat->iconFormat &= ~0x300;
    stat->iconSpeed &= ~0x300;
}

signed int iSGSaveFile(st_ISGSESSION* isgdata, const char* fname, char* data, signed int n,
                       signed int async, char* label)
{
    signed int writeret = 0;
    en_ASYNC_OPERR operr = ISG_OPERR_NONE;
    CARDStat statA = { 0 };
    CARDStat statB = { 0 };

    if (isgdata->slot < 0)
    {
        isgdata->unk_26c = ISG_OPSTAT_FAILURE;
        isgdata->unk_268 = ISG_OPERR_NOCARD;
        return 0;
    }

    st_ISG_MEMCARD_DATA* mcdata = &isgdata->mcdata[isgdata->slot];
    if (mcdata->unk_12c != 0)
    {
        isgdata->unk_26c = ISG_OPSTAT_FAILURE;
        isgdata->unk_268 = ISG_OPERR_DAMAGE;
        return 0;
    }

    fn_80016768();
    signed int iconsize = iSG_cubeicon_size(isgdata->slot, mcdata->sectorSize);
    signed int sectorsize200 = ALIGN_THING(mcdata->sectorSize, 0x200);
    iconsize += ALIGN_THING(sectorsize200, n);

    signed int allocsize = iconsize + 0x1F;
    void* alloc = xMemPushTemp(allocsize);
    memset(alloc, 0, allocsize);
    char* icondata = (char*)((unsigned int)alloc + 0x1F & 0xFFFFFFE0);

    memcpy(iSG_bfr_icondata(icondata, &statA, label, mcdata->sectorSize), data, n);
    fn_80016768();

    if (iSG_mc_fopen(mcdata, fname, iconsize, ISG_IOMODE_WRITE, &operr) != 0)
    {
        if (iSG_mcqa_fwrite(mcdata, icondata, iconsize) != 0)
        {
            writeret = 1;
        }
        else
        {
            writeret = 0;
            operr = ISG_OPERR_SVWRITE;
        }

        if (writeret != 0)
        {
            iSG_upd_icostat(&statA, &mcdata->fstat);
            signed long result;
            do
            {
                result = CARDSetStatus(mcdata->chan, mcdata->finfo.fileNo, &mcdata->fstat);
            } while (result == CARD_RESULT_BUSY);

            if (result != CARD_RESULT_READY)
            {
                writeret = 0;
            }
        }

        if (writeret != 0)
        {
            while (CARDSetAttributes(mcdata->chan, mcdata->finfo.fileNo, 4) == CARD_RESULT_BUSY)
            {
            }
        }

        while (CARDClose(&mcdata->finfo) == CARD_RESULT_BUSY)
        {
        }
        memcpy(&statB, &mcdata->fstat, sizeof(CARDStat));
        memset(&mcdata->finfo, 0, sizeof(CARDFileInfo));
        memset(&mcdata->fstat, 0, sizeof(CARDStat));
    }
    xMemPopTemp(alloc);

    if (writeret == 0 && mcdata->unk_12c == 0)
    {
        iSG_mc_fdel(mcdata, fname);
    }

    fn_80016768();

    isgdata->unk_268 = ISG_OPERR_NONE;
    if (writeret != 0)
    {
        isgdata->unk_26c = ISG_OPSTAT_SUCCESS;
    }
    else
    {
        isgdata->unk_26c = ISG_OPSTAT_FAILURE;
        if (operr != ISG_OPERR_NONE)
        {
            isgdata->unk_268 = operr;
        }
        else
        {
            isgdata->unk_268 = ISG_OPERR_UNKNOWN;
        }
    }
    return writeret;
}

signed int iSGLoadFile(st_ISGSESSION* isgdata, const char* fname, char* databuf, signed int async)
{
    return iSGReadLeader(isgdata, fname, databuf, iSGFileSize(isgdata, fname), async);
}

#pragma auto_inline off

signed int iSGReadLeader(st_ISGSESSION* isgdata, const char* fname, char* databuf,
                         signed int numbytes, signed int async)
{
    signed int bufsize;
    char* readbuf;
    signed int readret = 0;
    st_ISG_MEMCARD_DATA* data;
    void* alloc = NULL;
    en_ASYNC_OPERR operr = ISG_OPERR_NONE;

    if (isgdata->slot < 0)
    {
        isgdata->unk_26c = ISG_OPSTAT_FAILURE;
        isgdata->unk_268 = ISG_OPERR_NOCARD;
        return 0;
    }

    data = &isgdata->mcdata[isgdata->slot];
    if (data->unk_12c != 0)
    {
        isgdata->unk_26c = ISG_OPSTAT_FAILURE;
        isgdata->unk_268 = ISG_OPERR_DAMAGE;
        return 0;
    }

    fn_80016768();
    signed int iconsize = iSG_cubeicon_size(data->chan, data->sectorSize);
    signed int sectorsize200 = ALIGN_THING(data->sectorSize, 0x200);
    if ((signed int)databuf % 32 != 0 || numbytes % sectorsize200 != 0)
    {
        signed int tmpsize = (numbytes + 0x1FF & ~0x1FF);
        signed int allocsize = tmpsize + 0x1F;
        alloc = xMemPushTemp(allocsize);
        memset(alloc, 0, allocsize);
        bufsize = tmpsize;
        readbuf = (char*)((unsigned int)alloc + 0x1F & ~0x1F);
    }
    else
    {
        readbuf = databuf;
        bufsize = numbytes;
    }

    fn_80016768();

    if (iSG_mc_fopen(data, fname, -1, ISG_IOMODE_READ, &operr) != 0)
    {
        readret = iSG_mcqa_fread(data, readbuf, bufsize, iconsize) ? 1 : 0;
        iSG_mc_fclose(data);
    }

    if (readret != 0 && alloc != NULL)
    {
        memcpy(databuf, readbuf, numbytes);
    }
    if (alloc != NULL)
    {
        xMemPopTemp(alloc);
    }

    isgdata->unk_268 = ISG_OPERR_NONE;
    if (readret != 0)
    {
        isgdata->unk_26c = ISG_OPSTAT_SUCCESS;
    }
    else
    {
        isgdata->unk_26c = ISG_OPSTAT_FAILURE;
        if (data->unk_12c != 0)
        {
            isgdata->unk_268 = ISG_OPERR_DAMAGE;
        }
        else
        {
            isgdata->unk_268 = ISG_OPERR_OTHER;
        }
    }
    return readret;
}

en_ASYNC_OPSTAT iSGPollStatus(st_ISGSESSION* isgdata, en_ASYNC_OPCODE* curop, signed int block)
{
    return isgdata->unk_26c;
}

en_ASYNC_OPERR iSGOpError(st_ISGSESSION* isgdata, char* errmsg)
{
    static char* errmsgs[0x16] = {
        "No current error",
        "No operation in async queue",
        "Too many async ops queued simultaneously",
        "Init Failed",
        "Unable to access Save Game Directory",
        "Access Error - no card ?!? (eg yanked out)",
        "Access Error - no room on card (file handles free bytes, etc)",
        "Access Error - card is damaged or something bad",
        "Access Error - file being loaded appears to be corrupt (I-Level)",
        "Access Error - general problem",
        "Save Error - Not enough free space to save file",
        "Save Error - during initalization (async queue)",
        "Save Error - during write",
        "Save Error - opening file",
        "Load Error - during initalization (async queue)",
        "Load Error - during read",
        "Load Error - opening file",
        "Target problem (general error)",
        "Target Error - media removed or changed",
        "Target Error - Not ready for I/O (unformatted?)",
        "Operation encountered unknown error",
        NULL,
    };

    if (errmsg == NULL)
    {
        return isgdata->unk_268;
    }

    if (isgdata->unk_268 < ISG_OPERR_NOMORE)
    {
        strncpy(errmsg, errmsgs[isgdata->unk_268], 0x80);
    }
    else
    {
        strncpy(errmsg, errmsgs[ISG_OPERR_UNKNOWN], 0x80);
    }
    errmsg[0x7F] = '\0';
    return isgdata->unk_268;
}

signed int iSG_mcidx2slot(signed int idx, signed int* out_slot, signed int* ready)
{
    signed int cardReady[ISG_NUM_SLOTS] = {};
    signed int ret = 0;
    signed int found = 0;
    signed long memSize = 0;
    signed long sectorSize = 0;
    *out_slot = -1;

    for (signed int i = 0; i < ISG_NUM_SLOTS; i++)
    {
        signed long result;
        do
        {
            result = CARDProbeEx(i, &memSize, &sectorSize);
        } while (result == CARD_RESULT_BUSY);

        if (result == CARD_RESULT_READY)
        {
            cardReady[i] = 1;
        }
    }

    for (signed int i = 0; i < ISG_NUM_SLOTS; i++)
    {
        if (cardReady[i])
        {
            if (found == idx)
            {
                *out_slot = i;
                ret = 1;
                break;
            }
            found++;
        }
    }

    if (ready)
    {
        for (signed int i = 0; i < ISG_NUM_SLOTS; i++)
        {
            ready[i] = cardReady[i];
        }
    }

    return ret;
}

static signed int iSG_mc_isGCcard(st_ISG_MEMCARD_DATA* mcdata, signed int* param2,
                                  signed int* param3)
{
    signed int rc = 0;
    signed long xferBytes = 0;
    unsigned short encoding = 0;
    signed long memSize = 0;
    signed long sectorSize = 0;

    if (param2)
    {
        *param2 = 0;
    }
    if (param3)
    {
        *param3 = 0;
    }

    if (mcdata->unk_0 == 0)
    {
        return 0;
    }
    if (mcdata->unk_12c)
    {
        return 0;
    }

    signed long result;
    do
    {
        result = CARDProbeEx(mcdata->chan, &memSize, &sectorSize);
    } while (result == CARD_RESULT_BUSY);

    if (result == CARD_RESULT_READY)
    {
        rc = 1;
    }

    if (rc != 0)
    {
        do
        {
            result = CARDCheckEx(mcdata->chan, &xferBytes);
        } while (result == CARD_RESULT_BUSY);

        if (result == CARD_RESULT_READY)
        {
            rc = 1;
        }
        else if (result == CARD_RESULT_BROKEN)
        {
            rc = 2;
        }
        else if (result == CARD_RESULT_ENCODING)
        {
            rc = 3;
            if (param2)
            {
                *param2 = 1;
            }
        }
        else
        {
            rc = 0;
            if (result == CARD_RESULT_ENCODING && param2)
            {
                *param2 = 1;
            }
            if (result == CARD_RESULT_IOERROR && param3)
            {
                *param3 = 1;
            }
        }
    }

    if (rc == 1)
    {
        do
        {
            result = CARDGetEncoding(mcdata->chan, &encoding);
        } while (result == CARD_RESULT_BUSY);

        if (result == CARD_RESULT_READY && encoding && param2)
        {
            *param2 = 1;
        }
    }

    return rc;
}

static signed int iSG_isSpaceForFile(st_ISG_MEMCARD_DATA* mcdata, signed int fsize,
                                     const char* fname, signed int* bytesNeeded,
                                     signed int* availOnDisk, signed int* needFile)
{
    signed int rc = 0;
    signed long byteNotUsed = 0;
    signed long filesNotUsed = 0;

    if (mcdata->unk_0 == 0)
    {
        return 0;
    }

    signed int len = iSG_cubeicon_size(mcdata->chan, mcdata->sectorSize);
    len = len + fsize;
    len = (len + 0x1FF) & ~0x1FF;

    signed long result;
    do
    {
        result = CARDFreeBlocks(mcdata->chan, &byteNotUsed, &filesNotUsed);
    } while (result == CARD_RESULT_BUSY);

    if (result == CARD_RESULT_READY)
    {
        if (availOnDisk)
        {
            *availOnDisk = byteNotUsed;
        }
        if (bytesNeeded)
        {
            *bytesNeeded = len;
        }
    }
    else
    {
        return 0;
    }
    if (needFile)
    {
        *needFile = 1;
    }

    if (iSG_get_finfo(mcdata, fname))
    {
        if (needFile)
        {
            *needFile = *needFile - 1 & ~-(0 > *needFile - 1);
        }
        if (len <= mcdata->unk_b0.length + byteNotUsed)
        {
            rc = 1;
        }
    }
    else if (len <= byteNotUsed && filesNotUsed > 0)
    {
        rc = 1;
    }

    return rc;
}

static signed int iSG_find_gamefile(st_ISG_MEMCARD_DATA* mcdata, signed int which, char* name,
                                    signed int* count)
{
    signed int ret = 0;
    signed int found = 0;

    if (mcdata->unk_0 == 0)
    {
        return 0;
    }

    if (name != NULL)
    {
        *name = '\0';
    }

    for (signed int i = 0; i < ISG_NUM_FILES; i++)
    {
        char* fname = iSGMakeName(ISG_NGTYP_GAMEFILE, NULL, i);
        if (iSG_get_finfo(mcdata, fname))
        {
            found++;
            if (name != NULL && found - 1 == which)
            {
                strcpy(name, fname);
                ret = 1;
                if (count == NULL)
                {
                    break;
                }
            }
        }
    }

    if (count != NULL)
    {
        *count = found;
        ret++;
    }
    return ret;
}

static signed int iSG_curKosher(CARDStat* stat, CARDFileInfo* info)
{
    signed int rc = 1;

    if ((stat->iconAddr < 1 || stat->iconAddr > 0x7FFFFFFF) &&
        (stat->commentAddr < 1 || stat->commentAddr > 0x7FFFFFFF))
    {
        rc = 0;
    }
    else
    {
        char stuff[0x200] = { 0 };
        sprintf(stuff, "SCOOBY-DOO:NIGHTOF100FRIGHTS::RyanNeilDan");
        void* alloc = xMemPushTemp(0x3E00 + 0x1F);
        char* buf = (char*)((unsigned int)alloc + 0x1F & ~0x1F);

        signed long result;
        do
        {
            result = CARDRead(info, buf, 0x3E00, 0);
        } while (result == CARD_RESULT_BUSY);

        if (result == CARD_RESULT_READY && memcmp(buf + 0x3A40, stuff, strlen(stuff) - 1))
        {
            rc = 0;
        }

        xMemPopTemp(alloc);
    }
    return rc;
}

#pragma auto_inline on

static signed int iSG_get_fmoddate(st_ISG_MEMCARD_DATA* mcdata, const char* fname, int* sec,
                                   int* min, int* hr, int* mon, int* day, int* yr)
{
    signed int rc = 1;
    OSCalendarTime time = { 0 };

    if (iSG_get_finfo(mcdata, fname) == 0)
    {
        rc = 0;
    }
    else
    {
        long long t = mcdata->unk_b0.time;
        t = t * (*(unsigned long*)0x800000F8 / 4);
        OSTicksToCalendarTime(t, &time);

        if (sec)
        {
            *sec = time.sec;
        }
        if (min)
        {
            *min = time.min;
        }
        if (hr)
        {
            *hr = time.hour;
        }
        if (mon)
        {
            *mon = time.mon + 1;
        }
        if (day)
        {
            *day = time.mday;
        }
        if (yr)
        {
            *yr = time.year;
        }
    }
    return rc;
}

static signed int iSG_load_icondata()
{
    g_rawicon = (st_ISG_TPL_TEXPALETTE*)iFileLoad("/SD2GCIcon.tpl", NULL, &g_iconsize);
    g_rawbanr = (st_ISG_TPL_TEXPALETTE*)iFileLoad("/SD2GCBanner.tpl", NULL, &g_banrsize);
    iSG_tpl_unpack(g_rawicon);
    iSG_tpl_unpack(g_rawbanr);
    return 1;
}

#pragma auto_inline off

static char* iSG_bfr_icondata(char* buf, CARDStat* stat, char* label, signed int sectorSize)
{
    IconData data = { 0 };
    static st_ISG_TPL_TEXPALETTE* ico_pal;
    static st_ISG_TPL_TEX* ico_desc;
    static unsigned int i = 0.0f;

    sprintf(data.footer, "SCOOBY-DOO:NIGHTOF100FRIGHTS::RyanNeilDan");
    strncpy(data.game_name, "Night of 100 Frights", sizeof(data.game_name));
    data.game_name[0x1F] = '\0';

    if (label)
    {
        strncpy(data.buf0, label, sizeof(data.buf0));
    }
    data.buf0[0x1F] = '\0';

    ico_pal = g_rawbanr;
    ico_desc = &ico_pal->unk_8[0];
    memcpy(data.buf1, ico_desc->unk_0->unk_8, sizeof(data.buf1));

    ico_pal = g_rawicon;
    for (i = 0; i < 4; i++)
    {
        ico_desc = &ico_pal->unk_8[i];
        memcpy(data.buf2[i], ico_desc->unk_0->unk_8, sizeof(data.buf2[i]));
    }

    memcpy(buf, &data, sizeof(data));

    signed int t = ALIGN_THING(sectorSize, 0x200);
    return buf + (t + (sizeof(IconData) - 1) & -t);
}

static signed int iSG_tpl_unpack(st_ISG_TPL_TEXPALETTE* tpl)
{
    if (tpl->magic != 0x0020AF30)
    {
        return 0;
    }

    tpl->unk_8 = (st_ISG_TPL_TEX*)((unsigned int)tpl + (unsigned int)tpl->unk_8);
    for (unsigned int i = 0; i < tpl->count; i++)
    {
        st_ISG_TPL_TEX* tex = &tpl->unk_8[i];
        if (tex->unk_0)
        {
            tex->unk_0 = (st_ISG_TPL_TEX::UnkIn*)((unsigned int)tpl + (unsigned int)tex->unk_0);
            if (tex->unk_0->unk_23 == 0)
            {
                tex->unk_0->unk_8 = (void*)((unsigned int)tpl + (unsigned int)tex->unk_0->unk_8);
                tex->unk_0->unk_23 = 1;
            }
        }
        if (tex->unk_4)
        {
            tex->unk_4 = (st_ISG_TPL_TEX::UnkOut*)((unsigned int)tpl + (unsigned int)tex->unk_4);
            if (tex->unk_4->unk_2 == 0)
            {
                tex->unk_4->unk_8 = tpl;
                tex->unk_4->unk_2 = 1;
            }
        }
    }
    return 1;
}

static void iSG_cb_unmount(signed long chan, signed long result)
{
    st_ISGSESSION* session = &g_isgdata_MAIN;
    if (chan == session->slot)
    {
        session->slot = -1;
    }
    memset(&session->mcdata[chan], 0, sizeof(st_ISG_MEMCARD_DATA));
}

static signed int iSG_mc_format(st_ISG_MEMCARD_DATA* mcdata, signed int, signed int* canRecover)
{
    if (mcdata->unk_12c != 0)
    {
        if (canRecover != NULL)
        {
            *canRecover = 0;
        }
        return 0;
    }

    if (canRecover != NULL)
    {
        *canRecover = 1;
    }

    signed long result;
    do
    {
        result = CARDFormat(mcdata->chan);
    } while (result == CARD_RESULT_BUSY);

    signed int ret;
    if (result == CARD_RESULT_READY)
    {
        ret = 1;
    }
    else
    {
        ret = 0;
        switch (result)
        {
        case CARD_RESULT_IOERROR:
            mcdata->unk_12c = 1;
            if (canRecover != NULL)
            {
                *canRecover = 0;
            }
            break;
        case CARD_RESULT_WRONGDEVICE:
        case CARD_RESULT_NOFILE:
        case CARD_RESULT_FATAL_ERROR:
            break;
        }
    }
    return ret;
}

static void iSG_cb_asyndone(signed long, signed long);

static signed int iSG_mc_fopen(st_ISG_MEMCARD_DATA* mcdata, const char* fname, signed int fsize,
                               en_ISG_IOMODE mode, en_ASYNC_OPERR* operr)
{
    signed int ret = 0;
    CARDFileInfo* finfo = &mcdata->finfo;
    CARDStat* stat = &mcdata->fstat;

    if (operr != NULL)
    {
        *operr = ISG_OPERR_NONE;
    }

    if (mode == ISG_IOMODE_READ)
    {
        signed long result;
        do
        {
            result = CARDOpen(mcdata->chan, fname, finfo);
        } while (result == CARD_RESULT_BUSY);

        if (result == CARD_RESULT_READY)
        {
            ret = 1;
        }
    }
    else
    {
        iSG_mc_fdel(mcdata, fname);
        if (mcdata->unk_12c == 0)
        {
            signed long result;
            do
            {
                result = CARDCreate(mcdata->chan, fname, fsize, finfo);
            } while (result == CARD_RESULT_BUSY);

            if (result == CARD_RESULT_READY)
            {
                ret = 1;
                mcdata->unk_98 = 0;
            }
            else if (operr != NULL)
            {
                switch (result)
                {
                case CARD_RESULT_WRONGDEVICE:
                case CARD_RESULT_NOCARD:
                    *operr = ISG_OPERR_NOCARD;
                    break;
                case CARD_RESULT_IOERROR:
                case CARD_RESULT_BROKEN:
                    mcdata->unk_12c = 1;
                    *operr = ISG_OPERR_DAMAGE;
                    break;
                case CARD_RESULT_NOENT:
                case CARD_RESULT_INSSPACE:
                    *operr = ISG_OPERR_NOROOM;
                    break;
                default:
                    *operr = ISG_OPERR_OTHER;
                    break;
                case CARD_RESULT_READY:
                case CARD_RESULT_BUSY:
                case CARD_RESULT_EXIST:
                case CARD_RESULT_NOPERM:
                    break;
                }
            }
        }
    }

    if (ret != 0)
    {
        signed long result;
        do
        {
            result = CARDGetStatus(mcdata->chan, finfo->fileNo, stat);
        } while (result == CARD_RESULT_BUSY);

        if (result == CARD_RESULT_READY)
        {
            ret = 1;
        }
    }

    if (ret != 0)
    {
        if (memcmp(stat->gameName, "GIHE", 4) != 0 || memcmp(stat->company, "78", 2) != 0)
        {
            iSG_mc_fclose(mcdata);
            ret = 0;
        }
    }

    return ret;
}

static void iSG_cb_asyndone(signed long, signed long)
{
}

static signed int iSG_mcqa_fread(st_ISG_MEMCARD_DATA* mcdata, char* buf, signed int len,
                                 signed int offset)
{
    signed int x = 1000;
    signed long result = CARDGetResultCode(mcdata->chan);
    do
    {
        result = CARDGetResultCode(mcdata->chan);
    } while (result == CARD_RESULT_BUSY);

    if (result != CARD_RESULT_READY)
    {
        return 0;
    }

    CARDGetXferredBytes(mcdata->chan);
    do
    {
        result = CARDReadAsync(&mcdata->finfo, buf, len, offset, iSG_cb_asyndone);
    } while (result == CARD_RESULT_BUSY);

    if (result != CARD_RESULT_READY)
    {
        return 0;
    }

    result = CARD_RESULT_BUSY;
    do
    {
        if (x++ > 500)
        {
            xSndUpdate(globals.update_dt);
            CARDGetXferredBytes(mcdata->chan);
            result = CARDGetResultCode(mcdata->chan);
            x = 0;
        }
        fn_80016768();
    } while (result == CARD_RESULT_BUSY);
    return result == 0;
}

static signed int iSG_mcqa_fwrite(st_ISG_MEMCARD_DATA* mcdata, char* buf, signed int len)
{
    signed int ret = 0;
    signed int x = 1000;
    CARDStat* fstat = &mcdata->fstat;
    signed long result;
    do
    {
        result = CARDGetResultCode(mcdata->chan);
    } while (result == CARD_RESULT_BUSY);

    if (result != CARD_RESULT_READY)
    {
        return 0;
    }

    CARDGetXferredBytes(mcdata->chan);
    signed long asynresult;
    do
    {
        asynresult = CARDWriteAsync(&mcdata->finfo, buf, len, mcdata->unk_98, iSG_cb_asyndone);
    } while (asynresult == CARD_RESULT_BUSY);

    if (asynresult != CARD_RESULT_READY)
    {
        return 0;
    }

    result = CARD_RESULT_BUSY;
    do
    {
        if (x++ > 500)
        {
            result = CARDGetResultCode(mcdata->chan);
            xSndUpdate(globals.update_dt);
            CARDGetXferredBytes(mcdata->chan);
            x = 0;
        }
        fn_80016768();
    } while (result == CARD_RESULT_BUSY);

    if (asynresult == CARD_RESULT_READY)
    {
        mcdata->unk_98 += len;

        do
        {
            result = CARDGetStatus(mcdata->chan, mcdata->finfo.fileNo, fstat);
        } while (result == CARD_RESULT_BUSY);

        if (result == CARD_RESULT_READY)
        {
            ret = 1;
        }
    }
    return ret;
}

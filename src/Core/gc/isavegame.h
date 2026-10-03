#ifndef ISAVEGAME_H
#define ISAVEGAME_H

extern "C" {

typedef struct CARDFileInfo
{
    signed long chan;
    signed long fileNo;
    signed long offset;
    signed long length;
    unsigned short iBlock;
    unsigned short __padding;
} CARDFileInfo;

typedef struct CARDStat
{
    char fileName[32];
    unsigned long length;
    unsigned long time;
    unsigned char gameName[4];
    unsigned char company[2];
    unsigned char bannerFormat;
    unsigned char __padding;
    unsigned long iconAddr;
    unsigned short iconFormat;
    unsigned short iconSpeed;
    unsigned long commentAddr;
    unsigned long offsetBanner;
    unsigned long offsetBannerTlut;
    unsigned long offsetIcon[8];
    unsigned long offsetIconTlut;
    unsigned long offsetData;
} CARDStat;
}

enum en_ISG_IOMODE
{
    ISG_IOMODE_READ = 1,
    ISG_IOMODE_WRITE = 2,
    ISG_IOMODE_APPEND = 3,
};

enum en_ASYNC_OPCODE
{
    ISG_OPER_NOOP = 0,
    ISG_OPER_INIT = 1,
    ISG_OPER_SAVE = 2,
    ISG_OPER_LOAD = 3,
};

enum en_ASYNC_OPSTAT
{
    ISG_OPSTAT_FAILURE = -1,
    ISG_OPSTAT_INPROG = 0,
    ISG_OPSTAT_SUCCESS = 1,
};

enum en_ASYNC_OPERR
{
    ISG_OPERR_NONE = 0,
    ISG_OPERR_NOOPER = 1,
    ISG_OPERR_MULTIOPER = 2,
    ISG_OPERR_INITFAIL = 3,
    ISG_OPERR_GAMEDIR = 4,
    ISG_OPERR_NOCARD = 5,
    ISG_OPERR_NOROOM = 6,
    ISG_OPERR_DAMAGE = 7,
    ISG_OPERR_CORRUPT = 8,
    ISG_OPERR_OTHER = 9,
    ISG_OPERR_SVNOSPACE = 10,
    ISG_OPERR_SVINIT = 11,
    ISG_OPERR_SVWRITE = 12,
    ISG_OPERR_SVOPEN = 13,
    ISG_OPERR_LDINIT = 14,
    ISG_OPERR_LDREAD = 15,
    ISG_OPERR_LDOPEN = 16,
    ISG_OPERR_TGTERR = 17,
    ISG_OPERR_TGTREM = 18,
    ISG_OPERR_TGTPREP = 19,
    ISG_OPERR_UNKNOWN = 20,
    ISG_OPERR_NOMORE = 21,
};

enum en_CHGCODE
{
    ISG_CHG_NONE = 0,
    ISG_CHG_TARGET = 1,
    ISG_CHG_GAMELIST = 2,
};

enum en_NAMEGEN_TYPE
{
    ISG_NGTYP_GAMEDIR = 0,
    ISG_NGTYP_GAMEFILE = 1,
    ISG_NGTYP_CONFIG = 2,
    ISG_NGTYP_ICONTHUM = 3,
};

class st_ISG_MEMCARD_DATA
{
public:
    signed int unk_0; // 0x0 mounted
    signed int chan; // 0x4
    signed int sectorSize; // 0x8
    CARDFileInfo finfo; // 0xC
    CARDStat fstat; // 0x20
    signed int unk_pad5[3]; // 0x8C
    signed int unk_98; // 0x98 write offset
    CARDFileInfo unk_9c; // 0x9C
    CARDStat unk_b0; // 0xB0
    signed int unk_pad6[4]; // 0x11C
    signed int unk_12c; // 0x12C damaged
};

#define ISG_NUM_SLOTS 2
#define ISG_NUM_FILES 8

class st_ISGSESSION
{
public:
    st_ISG_MEMCARD_DATA mcdata[ISG_NUM_SLOTS]; // 0x0
    signed int slot; // 0x260
    signed int unk_264;
    en_ASYNC_OPERR unk_268; // 0x268
    en_ASYNC_OPSTAT unk_26c; // 0x26C
    signed int unk_270;
    signed int unk_274;
};

signed int iSGStartup();
signed int iSGShutdown();
char* iSGMakeName(en_NAMEGEN_TYPE type, const char* base, signed int idx);
st_ISGSESSION* iSGSessionBegin(void* cltdata, void (*chgfunc)(void*, en_CHGCODE));
void iSGSessionEnd(st_ISGSESSION* isgdata);
signed int iSGTgtCount(st_ISGSESSION* isgdata, signed int* max);
signed int iSGTgtPhysSlotIdx(st_ISGSESSION* isgdata, signed int tidx);
signed int iSGCheckMemoryCard(st_ISGSESSION* isgdata, signed int index);
char* iSGTgtName(st_ISGSESSION* isgdata, signed int tidx, signed int kind);
signed int iSGTgtFormat(st_ISGSESSION* isgdata, signed int tgtidx, signed int async,
                        signed int* canRecover);
unsigned int iSGTgtState(st_ISGSESSION* isgdata, signed int tgtidx, const char* dpath);
signed int iSGTgtFreeSpace(st_ISGSESSION* isgdata, signed int tgtidx, signed int* bytes,
                           signed int* files);
signed int iSGTgtSetActive(st_ISGSESSION* isgdata, signed int tgtidx);
signed int iSGTgtHaveRoom(st_ISGSESSION* isgdata, signed int tidx, signed int fsize,
                          const char* dpath, const char* fname, signed int* bytesNeeded,
                          signed int* availOnDisk, signed int* needFile);
signed int iSGGameFileCount(st_ISGSESSION* isgdata);
signed int iSGFileSize(st_ISGSESSION* isgdata, const char* fname);
char* iSGFileModDate(st_ISGSESSION* isgdata, const char* fname);
void iSGMakeTimeStamp(char* str);
signed int iSGCheckForCorruptFiles(st_ISGSESSION* isgdata, signed int fix, signed int* firstBad,
                                   signed int* firstFixed);
signed int iSGSelectGameDir(st_ISGSESSION* isgdata, const char* dname);
signed int iSGSetupGameDir(st_ISGSESSION* isgdata, const char* dname, signed int force_iconfix);
signed int iSGSaveFile(st_ISGSESSION* isgdata, const char* fname, char* data, signed int n,
                       signed int async, char* label);
signed int iSGLoadFile(st_ISGSESSION* isgdata, const char* fname, char* databuf,
                       signed int async);
signed int iSGReadLeader(st_ISGSESSION* isgdata, const char* fname, char* databuf,
                         signed int numbytes, signed int async);
en_ASYNC_OPSTAT iSGPollStatus(st_ISGSESSION* isgdata, en_ASYNC_OPCODE* curop, signed int block);
en_ASYNC_OPERR iSGOpError(st_ISGSESSION* isgdata, char* errmsg);
signed int iSG_mcidx2slot(signed int idx, signed int* out_slot, signed int* ready);

#endif

#ifndef XSAVEGAME_H
#define XSAVEGAME_H

class st_ISGSESSION;

class st_XSAVEGAME_READCONTEXT
{
};

class st_XSAVEGAME_WRITECONTEXT
{
};

enum en_SAVEGAME_MODE
{
    XSG_MODE_LOAD = 10,
    XSG_MODE_SAVE = 11,
};

enum en_XSGASYNC_STATUS
{
    XSG_ASTAT_NOOP = 0,
    XSG_ASTAT_INPROG = 1,
    XSG_ASTAT_SUCCESS = 2,
    XSG_ASTAT_FAILED = 3,
};

enum en_XSG_WHYFAIL
{
    XSG_WHYERR_NONE = 0,
    XSG_WHYERR_NOCARD = 1,
    XSG_WHYERR_NOROOM = 2,
    XSG_WHYERR_DAMAGE = 3,
    XSG_WHYERR_OTHER = 4,
};

class st_XSAVEGAME_DATA;

class st_XSAVEGAME_CLIENT
{
public:
    unsigned int idtag; // 0x0
    signed int (*cltinfo)(void*, st_XSAVEGAME_DATA*, signed int*, signed int*); // 0x4
    signed int (*cltproc)(void*, st_XSAVEGAME_DATA*, st_XSAVEGAME_WRITECONTEXT*); // 0x8
    signed int (*cltload)(void*, st_XSAVEGAME_DATA*, st_XSAVEGAME_READCONTEXT*, unsigned int,
                          signed int); // 0xC
    void* cltdata; // 0x10
    signed int needamt; // 0x14
    signed int maxamt; // 0x18
    signed int realamt; // 0x1C
    char* buf_sizepos; // 0x20
    char* buf_maxpos; // 0x24
    signed int blokact; // 0x28
    signed int blokmax; // 0x2C
    char* blokpos; // 0x30
    char* readpos; // 0x34
    signed int readamt; // 0x38
    signed int readremain; // 0x3C
};

class st_XSAVEGAME_DATA
{
public:
    signed int gfile_idx; // 0x0
    en_SAVEGAME_MODE mode; // 0x4
    unsigned int stage; // 0x8
    signed int gslot; // 0xC
    char label[64]; // 0x10
    signed int thumIndex; // 0x50
    signed long long playtime; // 0x58
    char* membuf; // 0x60
    signed int memsize; // 0x64
    char* buf_curpos; // 0x68
    char* buf_sizespot; // 0x6C
    char* buf_cksmspot; // 0x70
    signed int totamt; // 0x74
    unsigned int chksum; // 0x78
    unsigned int upd_tally; // 0x7C
    signed int cltneed; // 0x80
    signed int cltmax; // 0x84
    signed int chdrneed; // 0x88
    signed int stkcnt; // 0x8C
    st_XSAVEGAME_CLIENT cltstk[128]; // 0x90
    st_XSAVEGAME_CLIENT dfltloadclt; // 0x2090
    unsigned int file_chksum; // 0x20D0
    unsigned int read_chksum; // 0x20D4
    signed int readsize; // 0x20D8
    char* loadbuf; // 0x20DC
    signed int loadsize; // 0x20E0
    char* walkpos; // 0x20E4
    signed int walkremain; // 0x20E8
    st_ISGSESSION* isgsess; // 0x20EC
};

class st_XSAVEGAME_LEADER
{
public:
    char gameLabel[64]; // 0x0
    signed int thumIconIndex; // 0x40
    signed long long gametime; // 0x48
};

signed int xSGStartup();
signed int xSGShutdown();
st_XSAVEGAME_DATA* xSGInit(en_SAVEGAME_MODE mode);
signed int xSGDone(st_XSAVEGAME_DATA* xsgdata);
signed int xSGTgtCount(st_XSAVEGAME_DATA* xsgdata, signed int* max);
signed int xSGTgtPhysSlotIdx(st_XSAVEGAME_DATA* xsgdata, signed int tidx);
char* xSGTgtName(st_XSAVEGAME_DATA* xsgdata, signed int tidx, signed int kind);
signed int xSGTgtIsFormat(st_XSAVEGAME_DATA* xsgdata, signed int tidx, signed int* badEncode);
signed int xSGTgtFormatTgt(st_XSAVEGAME_DATA* xsgdata, signed int tidx, signed int* canRecover);
signed int xSGTgtFreeSpace(st_XSAVEGAME_DATA* xsgdata, signed int tidx, signed int* bytes,
                           signed int* files);
signed int xSGTgtSelect(st_XSAVEGAME_DATA* xsgdata, signed int tidx);
signed int xSGTgtHasGameDir(st_XSAVEGAME_DATA* xsgdata, signed int tidx);
signed int xSGTgtHaveRoom(st_XSAVEGAME_DATA* xsgdata, signed int tidx, signed int fsize,
                          signed int slotidx, signed int* bytesNeeded, signed int* availOnDisk,
                          signed int* needFile);
void xSGGameSet(st_XSAVEGAME_DATA* xsgdata, signed int gidx);
unsigned char xSGGameIsEmpty(st_XSAVEGAME_DATA* xsgdata, signed int gidx);
char* xSGGameModDate(st_XSAVEGAME_DATA* xsgdata, signed int gidx);
char* xSGGameLabel(st_XSAVEGAME_DATA* xsgdata, signed int gidx);
signed int xSGGameThumIndex(st_XSAVEGAME_DATA* xsgdata, signed int gidx);
signed int xSGAddSaveClient(st_XSAVEGAME_DATA* xsgdata, unsigned int clttag, void* cltdata,
                            signed int (*infofunc)(void*, st_XSAVEGAME_DATA*, signed int*,
                                                   signed int*),
                            signed int (*procfunc)(void*, st_XSAVEGAME_DATA*,
                                                   st_XSAVEGAME_WRITECONTEXT*));
signed int xSGAddLoadClient(st_XSAVEGAME_DATA* xsgdata, unsigned int clttag, void* cltdata,
                            signed int (*loadfunc)(void*, st_XSAVEGAME_DATA*,
                                                   st_XSAVEGAME_READCONTEXT*, unsigned int,
                                                   signed int));
signed int xSGSetup(st_XSAVEGAME_DATA* xsgdata);
signed int xSGSetup(st_XSAVEGAME_DATA* xsgdata, signed int gidx, char* label, signed int thumIdx,
                    signed long long playtime);
signed int xSGProcess(st_XSAVEGAME_DATA* xsgdata);
signed int xSGWrapup(st_XSAVEGAME_DATA* xsgdata);
en_XSGASYNC_STATUS xSGAsyncStatus(st_XSAVEGAME_DATA* xsgdata, signed int block,
                                  en_XSG_WHYFAIL* whyFail, char* errmsg);
signed int xSGWriteData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt, char* data,
                        signed int elesiz, signed int n);
signed int xSGWriteStrLen(char* str);
signed int xSGWriteData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt, char* data,
                        signed int n);
signed int xSGWriteData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt,
                        signed int* data, signed int n);
signed int xSGWriteData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt,
                        unsigned int* data, signed int n);
signed int xSGWriteData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt, float* data,
                        signed int n);
signed int xSGReadData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt, char* buff,
                       signed int elesiz, signed int n);
signed int xSGReadData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt, char* buff,
                       signed int n);
signed int xSGReadData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt,
                       signed int* buff, signed int n);
signed int xSGReadData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt,
                       unsigned int* buff, signed int n);
signed int xSGReadData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt, float* buff,
                       signed int n);

#endif

#include "xserializer.h"
#include "xMemMgr.h"

extern "C" void* memset(void* dst, int c, unsigned int n);

enum en_SAVEGAME_MODE
{
    XSG_MODE_LOAD = 10,
    XSG_MODE_SAVE = 11
};

char* xUtil_idtag2string(unsigned int srctag, signed int bufidx);

signed int xSGAddSaveClient(st_XSAVEGAME_DATA* xsgdata, unsigned int clttag, void* cltdata,
                            signed int (*infofunc)(void*, st_XSAVEGAME_DATA*, signed int*,
                                                   signed int*),
                            signed int (*procfunc)(void*, st_XSAVEGAME_DATA*,
                                                   st_XSAVEGAME_WRITECONTEXT*));
signed int xSGAddLoadClient(st_XSAVEGAME_DATA* xsgdata, unsigned int clttag, void* cltdata,
                            signed int (*loadfunc)(void*, st_XSAVEGAME_DATA*,
                                                   st_XSAVEGAME_READCONTEXT*, unsigned int,
                                                   signed int));
signed int xSGWriteData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt, signed int* data,
                        signed int n);
signed int xSGWriteData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_WRITECONTEXT* wctxt, char* data,
                        signed int n);
signed int xSGReadData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt, signed int* buff,
                       signed int n);
signed int xSGReadData(st_XSAVEGAME_DATA* xsgdata, st_XSAVEGAME_READCONTEXT* rctxt, char* buff,
                       signed int n);

static signed int g_serinit;
static st_XSERIAL_DATA_PRIV g_xserdata = { 0 };
static signed int g_tbl_onbit[32] = { 0 };
static signed int g_tbl_clear[32] = { 0 };

static signed int xSER_ord_test(void* key, void* elt);
static signed int xSER_ord_compare(void* e1, void* e2);
static void xSER_init_buffers(signed int count, st_SERIAL_PERCID_SIZE* sizeinfo);

static inline void xSER_init_tables()
{
    for (signed int i = 0; i < 32; i++)
    {
        g_tbl_onbit[i] = 1 << i;
        g_tbl_clear[i] = ~(1 << i);
    }
}

static inline st_SERIAL_CLIENTINFO* XSER_get_client(unsigned int idtag)
{
    st_XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    st_SERIAL_CLIENTINFO* clt;
    signed int idx = XOrdLookup(&xsd->cltlist, (void*)idtag, xSER_ord_test);

    if (idx < 0)
    {
        clt = xsd->cltnext++;
        clt->idtag = idtag;
        XOrdInsert(&xsd->cltlist, clt, xSER_ord_compare);
    }
    else
    {
        clt = (st_SERIAL_CLIENTINFO*)xsd->cltlist.list[idx];
    }

    return clt;
}

inline void xSerial::prepare(unsigned int idtag)
{
    st_SERIAL_CLIENTINFO* clt = XSER_get_client(idtag);
    this->idtag = clt->idtag;
    baseoff = clt->trueoff;
    ctxtdata = clt;
    warned = 0;
    curele = 0;
    bitidx = 0;
    bittally = 0;
}

inline void xSerial::wrbit(signed int is_on)
{
    st_SERIAL_CLIENTINFO* clt = ctxtdata;

    if (bittally + 1 > clt->actsize * 8)
    {
        warned = 1;
        return;
    }

    clt->membuf[curele] &= g_tbl_clear[bitidx];
    if (is_on)
    {
        clt->membuf[curele] |= g_tbl_onbit[bitidx];
    }

    bitidx++;
    if (bitidx == 32)
    {
        curele++;
        bitidx = 0;
    }
    bittally++;
}

inline signed int xSerial::rdbit()
{
    st_SERIAL_CLIENTINFO* clt = ctxtdata;
    signed int is_on;

    if (bittally + 1 > clt->actsize * 8)
    {
        warned = 1;
        return 0;
    }

    is_on = (ctxtdata->membuf[curele] & g_tbl_onbit[bitidx]) != 0;

    bitidx++;
    if (bitidx == 32)
    {
        curele++;
        bitidx = 0;
    }
    bittally++;

    return is_on;
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

signed int xSerialStartup(signed int count, st_SERIAL_PERCID_SIZE* sizeinfo)
{
    if (g_serinit++ == 0)
    {
        memset(&g_xserdata, 0, sizeof(st_XSERIAL_DATA_PRIV));
        xSER_init_tables();
        xSER_init_buffers(count, sizeinfo);
    }
    return g_serinit;
}

signed int xSerialShutdown()
{
    return --g_serinit;
}

// TODO (97%): register assignment differs (the original keeps &g_xserdata in
// r26 and &cltlist in r31).
void xSerialTraverse(signed int (*func)(unsigned int, xSerial*))
{
    st_XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    xSerial xser;

    for (signed int i = 0; i < xsd->cltlist.cnt; i++)
    {
        st_SERIAL_CLIENTINFO* clt = (st_SERIAL_CLIENTINFO*)xsd->cltlist.list[i];
        xser.setClient(clt->idtag);
        if (!func(clt->idtag, &xser))
        {
            break;
        }
    }
}

#pragma auto_inline on
xSerial::xSerial()
{
}

xSerial::~xSerial()
{
    if (ctxtdata)
    {
        xUtil_idtag2string(idtag, 0);
    }
}
#pragma auto_inline off

#pragma auto_inline on
void xSerial::setClient(unsigned int idtag)
{
    prepare(idtag);
}
#pragma auto_inline off

signed int xSerial::Write(char* data, signed int elesize, signed int n)
{
    signed int nbit;

    if (n == 0)
    {
        return 0;
    }

    nbit = n > 0 ? n * elesize * 8 : -n;

    if (n < 0)
    {
        signed int bidx = 0;
        for (signed int i = 0; i < nbit; i++)
        {
            wrbit(*(signed int*)data & g_tbl_onbit[bidx]);
            if (++bidx == 32)
            {
                bidx = 0;
                data += 4;
            }
        }
    }
    else
    {
        signed int bidx = 0;
        for (signed int i = 0; i < nbit; i++)
        {
            wrbit(*data & g_tbl_onbit[bidx]);
            if (++bidx == 8)
            {
                bidx = 0;
                data++;
            }
        }
    }

    return nbit;
}

signed int xSerial::Write_b1(signed int bits)
{
    return Write((char*)&bits, 4, -1);
}

signed int xSerial::Write_b2(signed int bits)
{
    return Write((char*)&bits, 4, -2);
}

signed int xSerial::Write_b7(unsigned int bits)
{
    return Write((char*)&bits, 4, -7);
}

signed int xSerial::Write(unsigned char data)
{
    return Write((char*)&data, 1, 1);
}

signed int xSerial::Write(signed short data)
{
    return Write((char*)&data, 2, 1);
}

signed int xSerial::Write(signed int data)
{
    return Write((char*)&data, 4, 1);
}

signed int xSerial::Write(unsigned int data)
{
    return Write((char*)&data, 4, 1);
}

signed int xSerial::Write(float data)
{
    return Write((char*)&data, 4, 1);
}

signed int xSerial::Read(char* buf, signed int elesize, signed int n)
{
    signed int nbit = n > 0 ? n * elesize * 8 : -n;

    if (n < 0)
    {
        signed int bidx = 0;
        for (signed int i = 0; i < nbit; i++)
        {
            if (rdbit())
            {
                *(signed int*)buf |= g_tbl_onbit[bidx];
            }
            else
            {
                *(signed int*)buf &= g_tbl_clear[bidx];
            }

            if (++bidx == 32)
            {
                bidx = 0;
                buf += 4;
            }
        }
    }
    else
    {
        signed int bidx = 0;
        for (signed int i = 0; i < nbit; i++)
        {
            if (rdbit())
            {
                *buf |= (char)g_tbl_onbit[bidx];
            }
            else
            {
                *buf &= (char)g_tbl_clear[bidx];
            }

            if (++bidx == 8)
            {
                bidx = 0;
                buf++;
            }
        }
    }

    return nbit;
}

signed int xSerial::Read_b1(signed int* bits)
{
    return Read((char*)bits, 4, -1);
}

signed int xSerial::Read_b1(unsigned int* bits)
{
    return Read((char*)bits, 4, -1);
}

signed int xSerial::Read_b2(signed int* bits)
{
    return Read((char*)bits, 4, -2);
}

signed int xSerial::Read_b7(unsigned int* bits)
{
    return Read((char*)bits, 4, -7);
}

signed int xSerial::Read(unsigned char* buf)
{
    return Read((char*)buf, 1, 1);
}

signed int xSerial::Read(signed short* buf)
{
    return Read((char*)buf, 2, 1);
}

signed int xSerial::Read(signed int* buf)
{
    return Read((char*)buf, 4, 1);
}

signed int xSerial::Read(unsigned int* buf)
{
    return Read((char*)buf, 4, 1);
}

signed int xSerial::Read(float* buf)
{
    return Read((char*)buf, 4, 1);
}

void xSerialWipeMainBuffer()
{
    memset(g_xserdata.bitbuf, 0, g_xserdata.buf_bytcnt);
}

static void xSER_init_buffers(signed int count, st_SERIAL_PERCID_SIZE* sizeinfo)
{
    st_XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    signed int i = 0;
    signed int tally = 0;
    signed int sicnt = 0;
    st_SERIAL_PERCID_SIZE* sitmp;
    st_SERIAL_CLIENTINFO* tmp_clt;

    XOrdInit(&xsd->cltlist, count);

    xsd->cltbuf = (st_SERIAL_CLIENTINFO*)xMemAlloc(gActiveHeap, count * sizeof(st_SERIAL_CLIENTINFO));
    memset(xsd->cltbuf, 0, count * 16);
    xsd->cltnext = xsd->cltbuf;

    sitmp = sizeinfo;
    while (sitmp->idtag != 0)
    {
        tally += (sitmp->needsize + 3) & ~3;
        sicnt++;
        sitmp++;
    }
    tally += (count - sicnt) * 160;

    xsd->bitbuf = (signed int*)xMemAlloc(gActiveHeap, tally);
    memset(xsd->bitbuf, 0, tally);
    xsd->buf_bytcnt = tally;

    sitmp = sizeinfo;
    tally = 0;
    tmp_clt = xsd->cltnext;
    while (sitmp->idtag != 0)
    {
        tmp_clt->idtag = sitmp->idtag;
        tmp_clt->trueoff = tally;
        tmp_clt->actsize = (sitmp->needsize + 3) & ~3;
        tmp_clt->membuf = xsd->bitbuf + tally / 4;
        XOrdAppend(&xsd->cltlist, tmp_clt);
        tally += tmp_clt->actsize;
        sitmp++;
        tmp_clt++;
    }
    XOrdSort(&xsd->cltlist, xSER_ord_compare);
    xsd->cltnext = tmp_clt;

    for (i = sicnt; i < count; i++)
    {
        tmp_clt->idtag = 0;
        tmp_clt->trueoff = tally;
        tmp_clt->actsize = 160;
        tmp_clt->membuf = xsd->bitbuf + tally / 4;
        tally += tmp_clt->actsize;
        tmp_clt++;
    }
}

static signed int xSER_ord_compare(void* e1, void* e2)
{
    signed int result;
    if (*(unsigned int*)e1 < *(unsigned int*)e2)
    {
        result = -1;
    }
    else if (*(unsigned int*)e1 > *(unsigned int*)e2)
    {
        result = 1;
    }
    else
    {
        result = 0;
    }
    return result;
}

static signed int xSER_ord_test(void* key, void* elt)
{
    signed int result;
    if ((unsigned int)key < *(unsigned int*)elt)
    {
        result = -1;
    }
    else if ((unsigned int)key > *(unsigned int*)elt)
    {
        result = 1;
    }
    else
    {
        result = 0;
    }
    return result;
}

static signed int xSER_xsgclt_svinfo_ver(void*, st_XSAVEGAME_DATA*, signed int* cur_space,
                                         signed int* max_fullgame);
static signed int xSER_xsgclt_svproc_ver(void*, st_XSAVEGAME_DATA* xsg,
                                         st_XSAVEGAME_WRITECONTEXT* wctxt);
static signed int xSER_xsgclt_ldproc_ver(void*, st_XSAVEGAME_DATA* xsg,
                                         st_XSAVEGAME_READCONTEXT* rctxt, unsigned int, signed int);
static signed int xSER_xsgclt_svinfo_clt(void* cltdata, st_XSAVEGAME_DATA*, signed int* cur_space,
                                         signed int* max_fullgame);
static signed int xSER_xsgclt_svproc_clt(void* cltdata, st_XSAVEGAME_DATA* xsg,
                                         st_XSAVEGAME_WRITECONTEXT* wctxt);
static signed int xSER_xsgclt_ldproc_clt(void*, st_XSAVEGAME_DATA* xsg,
                                         st_XSAVEGAME_READCONTEXT* rctxt, unsigned int idtag,
                                         signed int);
static signed int xSER_xsgclt_svinfo_fill(void*, st_XSAVEGAME_DATA*, signed int* cur_space,
                                          signed int* max_fullgame);
static signed int xSER_xsgclt_svproc_fill(void*, st_XSAVEGAME_DATA* xsg,
                                          st_XSAVEGAME_WRITECONTEXT* wctxt);
static signed int xSER_xsgclt_ldproc_fill(void*, st_XSAVEGAME_DATA* xsg,
                                          st_XSAVEGAME_READCONTEXT* rctxt, unsigned int,
                                          signed int);

signed int xSerial_svgame_register(st_XSAVEGAME_DATA* sgctxt, en_SAVEGAME_MODE mode)
{
    st_XSERIAL_DATA_PRIV* xsd = &g_xserdata;

    xsd->flg_info &= ~1;

    if (mode == XSG_MODE_SAVE)
    {
        xSGAddSaveClient(sgctxt, 'SVID', xsd, xSER_xsgclt_svinfo_ver,
                         xSER_xsgclt_svproc_ver);
        for (signed int i = 0; i < xsd->cltlist.cnt; i++)
        {
            xSGAddSaveClient(sgctxt, *(unsigned int*)xsd->cltlist.list[i],
                             xsd->cltlist.list[i], xSER_xsgclt_svinfo_clt,
                             xSER_xsgclt_svproc_clt);
        }
        xSGAddSaveClient(sgctxt, 'SFIL', &g_xserdata, xSER_xsgclt_svinfo_fill,
                         xSER_xsgclt_svproc_fill);
    }
    else if (mode == XSG_MODE_LOAD)
    {
        xSGAddLoadClient(sgctxt, 'SVID', &g_xserdata, xSER_xsgclt_ldproc_ver);
        xSGAddLoadClient(sgctxt, 0, &g_xserdata, xSER_xsgclt_ldproc_clt);
        xSGAddLoadClient(sgctxt, 'SFIL', &g_xserdata, xSER_xsgclt_ldproc_fill);
    }

    return 1;
}

static signed int xSER_xsgclt_svinfo_ver(void*, st_XSAVEGAME_DATA*, signed int* cur_space,
                                         signed int* max_fullgame)
{
    *cur_space = 4;
    *max_fullgame = 4;
    return 1;
}

static signed int xSER_xsgclt_svproc_ver(void*, st_XSAVEGAME_DATA* xsg,
                                         st_XSAVEGAME_WRITECONTEXT* wctxt)
{
    signed int ver = 1;
    xSGWriteData(xsg, wctxt, &ver, 1);
    return 1;
}

static signed int xSER_xsgclt_ldproc_ver(void*, st_XSAVEGAME_DATA* xsg,
                                         st_XSAVEGAME_READCONTEXT* rctxt, unsigned int, signed int)
{
    st_XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    signed int ver = 0;
    xSGReadData(xsg, rctxt, &ver, 1);
    if (ver != 1)
    {
        xsd->flg_info |= 1;
    }
    return 1;
}

static signed int xSER_xsgclt_svinfo_clt(void* cltdata, st_XSAVEGAME_DATA*, signed int* cur_space,
                                         signed int* max_fullgame)
{
    *cur_space = ((st_SERIAL_CLIENTINFO*)cltdata)->actsize;
    *max_fullgame = ((st_SERIAL_CLIENTINFO*)cltdata)->actsize;
    return 1;
}

static signed int xSER_xsgclt_svproc_clt(void* cltdata, st_XSAVEGAME_DATA* xsg,
                                         st_XSAVEGAME_WRITECONTEXT* wctxt)
{
    st_SERIAL_CLIENTINFO* clt = XSER_get_client(*(unsigned int*)cltdata);
    xSGWriteData(xsg, wctxt, (char*)clt->membuf, clt->actsize);
    return 1;
}

static signed int xSER_xsgclt_ldproc_clt(void*, st_XSAVEGAME_DATA* xsg,
                                         st_XSAVEGAME_READCONTEXT* rctxt, unsigned int idtag,
                                         signed int)
{
    if (g_xserdata.flg_info & 1)
    {
        return 1;
    }

    st_SERIAL_CLIENTINFO* clt = XSER_get_client(idtag);
    xSGReadData(xsg, rctxt, (char*)clt->membuf, clt->actsize);
    return 1;
}

// TODO (99.5%): the original loads buf_bytcnt into r4 rather than r0.
static signed int xSER_xsgclt_svinfo_fill(void*, st_XSAVEGAME_DATA*, signed int* cur_space,
                                          signed int* max_fullgame)
{
    st_XSERIAL_DATA_PRIV* xsd = &g_xserdata;
    signed int used = 0;
    for (signed int i = 0; i < xsd->cltlist.cnt; i++)
    {
        used += ((st_SERIAL_CLIENTINFO*)xsd->cltlist.list[i])->actsize;
    }
    used = xsd->buf_bytcnt - used + 8;
    *cur_space = used;
    *max_fullgame = used;
    return 1;
}

static signed int xSER_xsgclt_svproc_fill(void*, st_XSAVEGAME_DATA* xsg,
                                          st_XSAVEGAME_WRITECONTEXT* wctxt)
{
    char filbuf[9] = { 'R', 'y', 'a', 'n', 'N', 'e', 'i', 'l', '\0' };
    xSGWriteData(xsg, wctxt, filbuf, 8);
    return 1;
}

static signed int xSER_xsgclt_ldproc_fill(void*, st_XSAVEGAME_DATA* xsg,
                                          st_XSAVEGAME_READCONTEXT* rctxt, unsigned int, signed int)
{
    char filbuf[9] = { 0 };
    if (g_xserdata.flg_info & 1)
    {
        return 1;
    }
    xSGReadData(xsg, rctxt, filbuf, 8);
    return 1;
}

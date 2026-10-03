#ifndef XSERIALIZER_H
#define XSERIALIZER_H

#include "xordarray.h"

class st_XSAVEGAME_DATA;
class st_XSAVEGAME_READCONTEXT;
class st_XSAVEGAME_WRITECONTEXT;

class st_SERIAL_CLIENTINFO
{
public:
    unsigned int idtag;
    signed int* membuf;
    signed int trueoff;
    signed int actsize;
};

class st_SERIAL_PERCID_SIZE
{
public:
    unsigned int idtag;
    signed int needsize;
};

class st_XSERIAL_DATA_PRIV
{
public:
    signed int flg_info;
    signed int* bitbuf;
    signed int buf_bytcnt;
    st_SERIAL_CLIENTINFO* cltbuf;
    st_SERIAL_CLIENTINFO* cltnext;
    st_XORDEREDARRAY cltlist;
};

class xSerial
{
public:
    unsigned int idtag;
    signed int baseoff;
    st_SERIAL_CLIENTINFO* ctxtdata;
    signed int warned;
    signed int curele;
    signed int bitidx;
    signed int bittally;

    xSerial();
    ~xSerial();
    void operator delete(void*)
    {
    }

    void setClient(unsigned int idtag);
    signed int Write_b1(signed int bits);
    signed int Write_b2(signed int bits);
    signed int Write_b7(unsigned int bits);
    signed int Write(char* data, signed int elesize, signed int n);
    signed int Write(unsigned char data);
    signed int Write(signed short data);
    signed int Write(signed int data);
    signed int Write(unsigned int data);
    signed int Write(float data);
    signed int Read_b1(signed int* bits);
    signed int Read_b1(unsigned int* bits);
    signed int Read_b2(signed int* bits);
    signed int Read_b7(unsigned int* bits);
    signed int Read(char* buf, signed int elesize, signed int n);
    signed int Read(unsigned char* buf);
    signed int Read(signed short* buf);
    signed int Read(signed int* buf);
    signed int Read(unsigned int* buf);
    signed int Read(float* buf);

private:
    void wrbit(signed int is_on);
    signed int rdbit();
    void prepare(unsigned int idtag);
};

signed int xSerialStartup(signed int count, st_SERIAL_PERCID_SIZE* sizeinfo);
signed int xSerialShutdown();
void xSerialTraverse(signed int (*func)(unsigned int, xSerial*));
void xSerialWipeMainBuffer();

#endif

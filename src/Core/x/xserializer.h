#ifndef XSERIALIZER_H
#define XSERIALIZER_H

struct st_SERIAL_CLIENTINFO;

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
};

#endif

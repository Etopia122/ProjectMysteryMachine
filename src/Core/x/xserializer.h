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
    signed int Read_b1(signed int* bits);
};

#endif

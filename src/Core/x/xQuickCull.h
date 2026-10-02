#ifndef XQUICKCULL_H
#define XQUICKCULL_H

class _xBound;

class xQCData
{
public:
    unsigned int xmask;
    unsigned int ymask;
    unsigned int zmask;
};

void xQuickCullForBound(xQCData* q, _xBound* b);
signed int xQuickCullIsects(xQCData* a, xQCData* b);

#endif

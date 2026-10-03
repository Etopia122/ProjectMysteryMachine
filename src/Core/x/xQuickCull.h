#ifndef XQUICKCULL_H
#define XQUICKCULL_H

class _xBound;
class _xBox;
class _xRay3;

class xQCData
{
public:
    unsigned int xmask;
    unsigned int ymask;
    unsigned int zmask;
};

void xQuickCullInit(const _xBox* box);
void xQuickCullForBound(xQCData* q, _xBound* b);
void xQuickCullForRay(xQCData* q, _xRay3* r);
signed int xQuickCullIsects(const xQCData* a, const xQCData* b);

#endif

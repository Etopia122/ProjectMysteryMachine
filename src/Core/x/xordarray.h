#ifndef XORDARRAY_H
#define XORDARRAY_H

class st_XORDEREDARRAY
{
public:
    void** list;
    signed int cnt;
    signed int max;
    signed int warnlvl;
};

typedef signed int (*XOrdCompareCallback)(void*, void*);
typedef signed int (*XOrdTestCallback)(void*, void*);

void XOrdInit(st_XORDEREDARRAY* array, signed int max);
void XOrdDone(st_XORDEREDARRAY* array);
void XOrdAppend(st_XORDEREDARRAY* array, void* elt);
void XOrdInsert(st_XORDEREDARRAY* array, void* elt, XOrdCompareCallback compare);
signed int XOrdLookup(st_XORDEREDARRAY* array, void* key, XOrdTestCallback test);
void XOrdSort(st_XORDEREDARRAY* array, XOrdCompareCallback test);

#endif

#include "xordarray.h"
#include "xMemMgr.h"

// Functions are in reverse DOL order (-inline deferred emits last to first).

void XOrdInit(st_XORDEREDARRAY* array, signed int max)
{
    signed int cnt;

    array->list = (void**)xMemAlloc(gActiveHeap, max << 2);
    array->cnt = 0;
    array->max = max;
    array->warnlvl = 0.95f * max;
    if (array->warnlvl == array->max)
    {
        cnt = array->max - 1;
        array->warnlvl = (0 > cnt) ? 0 : cnt;
    }
}

void XOrdDone(st_XORDEREDARRAY* array)
{
    array->list = 0;
    array->cnt = 0;
    array->max = 0;
    array->warnlvl = 0;
}

void XOrdAppend(st_XORDEREDARRAY* array, void* elt)
{
    if (array->cnt >= array->max)
    {
        return;
    }
    array->list[array->cnt++] = elt;
}

void XOrdInsert(st_XORDEREDARRAY* array, void* elt, XOrdCompareCallback compare)
{
    signed int i;

    if (array->cnt >= array->max)
    {
        return;
    }
    array->cnt++;

    for (i = array->cnt - 1; i > 0; i--)
    {
        if (compare(array->list[i - 1], elt) <= 0)
        {
            array->list[i] = elt;
            return;
        }
        array->list[i] = array->list[i - 1];
    }

    array->list[0] = elt;
}

signed int XOrdLookup(st_XORDEREDARRAY* array, void* key, XOrdTestCallback test)
{
    signed int da_idx = -1;
    signed int k0 = 0;
    signed int k1 = array->cnt;
    signed int k;
    signed int v;

    while (k1 > k0)
    {
        k = (k0 + k1) / 2;
        v = test(key, array->list[k]);
        if (v == 0)
        {
            da_idx = k;
            break;
        }
        if (v > 0)
        {
            k0 = k + 1;
        }
        else
        {
            k1 = k;
        }
    }

    return da_idx;
}

// Sedgewick's shell sort, written for a 1-based array: list[0] is never
// compared (preserved from the original).
void XOrdSort(st_XORDEREDARRAY* array, XOrdCompareCallback test)
{
    void** list = array->list;
    signed int num = array->cnt;
    signed int i;
    signed int j;
    signed int h;
    void* v;

    for (h = 1; h <= num; h = 3 * h + 1)
    {
    }

    while (h != 1)
    {
        h /= 3;
        for (i = h + 1; i < num; i++)
        {
            v = list[i];
            j = i;
            while (test(list[j - h], v) > 0)
            {
                list[j] = list[j - h];
                j -= h;
                if (j <= h)
                {
                    break;
                }
            }
            list[j] = v;
        }
    }
}

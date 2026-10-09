/*
 * Radix exchange sort over arrays of fixed size records keyed by an unsigned
 * 32-bit integer. Large ranges are split bit by bit with an explicit stack,
 * then a sentinel insertion sort finishes the nearly sorted array.
 */

#include "rwsdk/rwcore.h"

#define rxSORTSTACKSIZE 32
#define rxSORTINSERTIONTHRESHOLD 5

#define SORTKEY(_rec, _keyOffset) (*(RwUInt32*)((_rec) + (_keyOffset)))

typedef struct rxSortRange {
    RwUInt8* lo;
    RwUInt8* hi;
    RwUInt32 bit;
} rxSortRange;

static inline void SortSwap(RwUInt8* a, RwUInt8* b, RwUInt32 size)
{
    RwUInt32* pa = (RwUInt32*)a;
    RwUInt32* pb = (RwUInt32*)b;
    RwUInt32 i;

    for (i = size; i >= 4; i -= 4) {
        RwUInt32 tmp = *pb;
        *pb++ = *pa;
        *pa++ = tmp;
    }
}

void _repartition(RwUInt8* lo, RwUInt8* hi, RwUInt32 size, RwUInt32 keyOffset, RwUInt32 bit)
{
    rxSortRange stack[rxSORTSTACKSIZE];
    RwUInt8* origLo;
    RwUInt32 threshold = size * rxSORTINSERTIONTHRESHOLD;
    rxSortRange* sp;
    RwUInt8* origHi;
    RwUInt32* pa;
    RwUInt32* pb;
    RwUInt32 i;
    RwUInt32 tmp;

    stack[0].lo = lo;
    stack[0].hi = hi;
    stack[0].bit = bit;
    sp = &stack[1];

    do {
        sp--;
        lo = sp->lo;
        hi = sp->hi;
        bit = sp->bit;

        for (;;) {
            origLo = lo;
            origHi = hi;

            do {
                while (!(SORTKEY(lo, keyOffset) & bit)) {
                    lo += size;
                    if (lo > hi) {
                        goto partitioned;
                    }
                }
                while (SORTKEY(hi, keyOffset) & bit) {
                    hi -= size;
                    if (lo > hi) {
                        goto partitioned;
                    }
                }
                pa = (RwUInt32*)lo;
                pb = (RwUInt32*)hi;
                for (i = size; i >= 4; i -= 4) {
                    tmp = *pb;
                    *pb++ = *pa;
                    *pa++ = tmp;
                }
                lo += size;
                hi -= size;
            } while (lo <= hi);

        partitioned:
            bit >>= 1;
            if (bit == 0) {
                break;
            }

            if (origHi >= hi + size + threshold) {
                sp->lo = hi + size;
                sp->hi = origHi;
                sp->bit = bit;
                sp++;
            }

            hi = lo - size;
            lo = origLo;
            if (hi < origLo + threshold) {
                break;
            }
        }
    } while (sp != stack);
}

void _rx_rxRadixExchangeSort(RwUInt8* base, RwUInt32 num, RwUInt32 size, RwUInt32 keyOffset, RwUInt32 minKey,
                             RwUInt32 maxKey)
{
    RwUInt32 i;
    RwUInt8* cur;
    RwUInt32 recKey;
    RwUInt8* rec;
    RwUInt32 key;
    RwUInt32 n;
    RwUInt32 minIndex;

    if (base != NULL && keyOffset + sizeof(RwUInt32) <= size && minKey < maxKey) {
        if (num > rxSORTINSERTIONTHRESHOLD) {
            RwInt32 topBit;

            if (maxKey != 0) {
                topBit = 0;
                while (maxKey >>= 1) {
                    topBit++;
                }
            } else {
                topBit = -1;
            }
            _repartition(base, base + (num - 1) * size, size, keyOffset, 1 << topBit);
        }

        if (num > 1) {
            i = 4;
            if (num - 1 < 4) {
                i = num - 1;
            }

            minIndex = i;
            key = SORTKEY(base + i * size, keyOffset);
            i--;
            do {
                RwUInt32 k = SORTKEY(base + i * size, keyOffset);
                if (k < key) {
                    key = k;
                    minIndex = i;
                }
            } while (i-- != 0);

            if (minIndex != 0) {
                RwUInt32* pa = (RwUInt32*)base;
                RwUInt32* pb = (RwUInt32*)(base + minIndex * size);
                RwUInt32 j;

                for (j = size; j >= 4; j -= 4) {
                    RwUInt32 tmp = *pb;
                    *pb++ = *pa;
                    *pa++ = tmp;
                }
            }

            n = num;
            rec = base;
            while (rec += size, --n) {
                recKey = SORTKEY(rec, keyOffset);
                cur = rec;
                while (cur -= size, SORTKEY(cur, keyOffset) > recKey) {
                    SortSwap(cur, cur + size, size);
                }
            }
        }
    }
}

#include "xQuickCull.h"

signed int xQuickCullIsects(xQCData* a, xQCData* b)
{
    return (a->xmask & b->xmask) | (a->ymask & b->ymask) | (a->zmask & b->zmask);
}

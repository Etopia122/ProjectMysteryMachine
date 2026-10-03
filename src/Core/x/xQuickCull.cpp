#include "xQuickCull.h"

signed int xQuickCullIsects(const xQCData* a, const xQCData* b)
{
    return (a->xmask & b->xmask) | (a->ymask & b->ymask) | (a->zmask & b->zmask);
}

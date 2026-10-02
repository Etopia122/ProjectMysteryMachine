class xQCData
{
public:
    unsigned int xmask;
    unsigned int ymask;
    unsigned int zmask;
};

signed int xQuickCullIsects(xQCData* a, xQCData* b)
{
    return (a->xmask & b->xmask) | (a->ymask & b->ymask) | (a->zmask & b->zmask);
}

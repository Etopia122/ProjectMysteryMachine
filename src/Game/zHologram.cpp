#include "xMemMgr.h"

class _xEnt;

void xEntSetMaterialMul(_xEnt* ent, float r, float g, float b);

class _tagHologram
{
public:
    unsigned int flags;
    _xEnt* ent;
    float src_r;
    float src_g;
    float src_b;
    float time_passed;
    signed int cur_burst;
    signed int alpha_max;
};

class _HoloBurst
{
public:
    float r;
    float g;
    float b;
    float total_time;
};

static _tagHologram* sHologram[16];

static const _HoloBurst cHologramColBurst[23] = {
    { 1.0f, 1.0f, 1.0f, 0.0f },   { -1.0f, -1.0f, 5.0f, 0.01f }, { 1.0f, 1.0f, 1.0f, 0.0f },
    { -1.0f, -1.0f, 5.0f, 0.01f }, { 1.0f, 1.0f, 1.0f, 0.0f },   { -1.0f, -1.0f, 5.0f, 0.01f },
    { 1.0f, 1.0f, 1.0f, 0.0f },   { -1.0f, -1.0f, 5.0f, 0.01f }, { 1.0f, 1.0f, 1.0f, 0.0f },
    { -1.0f, -1.0f, 5.0f, 0.01f }, { 1.0f, 1.0f, 1.0f, 0.0f },   { -1.0f, -1.0f, 5.0f, 0.01f },
    { 1.0f, 1.0f, 1.0f, 0.0f },   { -1.0f, -1.0f, 5.0f, 0.01f }, { 1.0f, 1.0f, 1.0f, 0.0f },
    { -1.0f, -1.0f, 5.0f, 0.01f }, { 1.0f, 1.0f, 1.0f, 0.0f },   { -1.0f, -1.0f, 5.0f, 0.03f },
    { 1.0f, 1.0f, 1.0f, 0.33f },  { -1.0f, -1.0f, 5.0f, 0.01f }, { 1.0f, 1.0f, 1.0f, 0.33f },
    { -1.0f, -1.0f, 5.0f, 0.01f }, { 1.0f, 1.0f, 1.0f, 0.33f },
};

static inline _tagHologram* FindFreeHologram()
{
    for (signed int i = 0; i < 16; i++)
    {
        if (sHologram[i])
        {
            if (!(sHologram[i]->flags & 1))
            {
                return sHologram[i];
            }
        }
        else
        {
            sHologram[i] = (_tagHologram*)xMemAlloc(gActiveHeap, sizeof(_tagHologram));
            return sHologram[i];
        }
    }
    return 0;
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void zHologramInit()
{
    for (signed int i = 0; i < 16; i++)
    {
        sHologram[i] = 0;
    }
}

void zHologramAdd(_xEnt* ent)
{
    _tagHologram* h = FindFreeHologram();

    if (h)
    {
        h->flags |= 1;
        h->ent = ent;
        h->src_r = cHologramColBurst[0].r;
        h->src_g = cHologramColBurst[0].g;
        h->src_b = cHologramColBurst[0].b;
        h->time_passed = 0.0f;
        h->alpha_max = 1;
        h->cur_burst = 1;
    }
}

static inline void UpdateHologram(_tagHologram* h, float seconds)
{
    float bset;
    float gset;
    float rset;
    const _HoloBurst* burst;

    if (h->cur_burst >= 23)
    {
        h->flags &= ~1;
        h->alpha_max = 1;
        return;
    }

    burst = &cHologramColBurst[h->cur_burst];
    h->time_passed += seconds;
    if (h->time_passed >= burst->total_time)
    {
        rset = burst->r;
        gset = burst->g;
        bset = burst->b;
        h->src_r = rset;
        h->src_g = gset;
        h->src_b = bset;
        h->time_passed = 0.0f;
        h->cur_burst++;
    }
    else
    {
        rset = h->time_passed * ((burst->r - h->src_r) / burst->total_time);
        gset = h->time_passed * ((burst->g - h->src_g) / burst->total_time);
        bset = h->time_passed * ((burst->b - h->src_b) / burst->total_time);
        rset += h->src_r;
        gset += h->src_g;
        bset += h->src_b;
    }
    h->alpha_max = (unsigned char)!h->alpha_max;
    xEntSetMaterialMul(h->ent, rset, gset, bset);
}

void zHologramUpdate(float seconds)
{
    for (signed int i = 0; i < 16; i++)
    {
        if (sHologram[i] && (sHologram[i]->flags & 1))
        {
            UpdateHologram(sHologram[i], seconds);
        }
    }
}

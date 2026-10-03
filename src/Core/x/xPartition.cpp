#include "xBound.h"
#include "xMemMgr.h"
#include "iEnv.h"
#include "rwtypes.h"

extern "C" void* memset(void* dst, int c, unsigned int n);

class _xEnv
{
public:
    iEnv* geom;
};

class _zVolume;

class RwBBoxLayout
{
public:
    _xVec3 sup;
    _xVec3 inf;
};

class RpWorldLayout
{
public:
    unsigned char pad0[0x50];
    RwBBoxLayout boundingBox; // 0x50
};

class _tagPartLink
{
public:
    void* data;
    _tagPartLink* next;
};

class _tagPartSpace
{
public:
    signed int total;
    _tagPartLink head;
};

class _tagPartition
{
public:
    _xVec3 min;           // 0x0
    _xVec3 max;           // 0xC
    _xVec3 space_dim;     // 0x18
    signed int total_x;   // 0x24
    signed int total_y;   // 0x28
    signed int total_z;   // 0x2C
    _tagPartSpace* space; // 0x30
    _tagPartSpace global; // 0x34
};

// Not yet named: returns a volume's bound.
extern "C" _xBound* fn_80005AFC(_zVolume* vol);

signed int xPartitionGetTrueIdx(_tagPartition* part, signed int x, signed int y, signed int z);
_tagPartSpace* xPartitionGetSpace(_tagPartition* p, signed int true_idx);

static inline void PartitionSpaceReset(_tagPartSpace* space)
{
    memset(space, 0, sizeof(_tagPartSpace));
}

static inline void PartitionSpaceInsert(_tagPartSpace* space, void* data)
{
    space->total++;
    _tagPartLink* head = &space->head;
    while (head->next)
    {
        head = head->next;
    }
    head->next = (_tagPartLink*)xMemAlloc(gActiveHeap, sizeof(_tagPartLink));
    head->next->data = data;
    head->next->next = 0;
}

static inline signed int PartitionGetIdx(_tagPartition* part, _xVec3* pos)
{
    if (pos->x < part->min.x || pos->x > part->max.x || pos->y < part->min.y ||
        pos->y > part->max.y || pos->z < part->min.z || pos->z > part->max.z)
    {
        return -1;
    }
    return xPartitionGetTrueIdx(part, (signed int)((pos->x - part->min.x) / part->space_dim.x),
                                (signed int)((pos->y - part->min.y) / part->space_dim.y),
                                (signed int)((pos->z - part->min.z) / part->space_dim.z));
}

static inline void xPartitionSpaceMove(_tagPartSpace* src, _tagPartSpace* dest, unsigned int data)
{
    _tagPartLink* dest_tail = &dest->head;
    while (dest_tail->next)
    {
        dest_tail = dest_tail->next;
    }

    _tagPartLink* cur = &src->head;
    _tagPartLink* prev = cur;
    while ((unsigned int)cur->data != data)
    {
        prev = cur;
        cur = cur->next;
    }

    prev->next = cur->next;
    dest_tail->next = cur;
    dest_tail->next->next = 0;
    src->total--;
    dest->total++;
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xPartitionReset()
{
}

// TODO (94%; also costs World/Volume/Insert/Update their last percent): the
// original computes tx * z before tx * tz and keeps total_z in r3.
#pragma auto_inline on
signed int xPartitionGetTrueIdx(_tagPartition* part, signed int x, signed int y, signed int z)
{
    signed int tx = part->total_x;
    return y * (tx * part->total_z) + tx * z + x;
}
#pragma auto_inline off

void xPartitionVolume(_tagPartition* part, _zVolume* volume, signed int x_spaces, signed int y_spaces,
                      signed int z_spaces)
{
    memset(part, 0, sizeof(_tagPartition));

    _xBound* bound = fn_80005AFC(volume);
    part->min = bound->box.box.lower;
    part->max = bound->box.box.upper;
    _xVec3 size;
    xVec3Sub(&size, &part->max, &part->min);
    part->total_x = x_spaces;
    part->total_y = y_spaces;
    part->total_z = z_spaces;
    part->space_dim.x = size.x / part->total_x;
    part->space_dim.y = size.y / part->total_y;
    part->space_dim.z = size.z / part->total_z;
    part->space = (_tagPartSpace*)xMemAlloc(
        gActiveHeap, part->total_z * (part->total_y * (part->total_x * sizeof(_tagPartSpace))));

    for (signed int z = 0; z < z_spaces; z++)
    {
        for (signed int y = 0; y < y_spaces; y++)
        {
            for (signed int x = 0; x < x_spaces; x++)
            {
                PartitionSpaceReset(&part->space[xPartitionGetTrueIdx(part, x, y, z)]);
            }
        }
    }

    PartitionSpaceReset(&part->global);
}

void xPartitionWorld(_tagPartition* part, _xEnv* env, signed int x_spaces, signed int y_spaces,
                     signed int z_spaces)
{
    memset(part, 0, sizeof(_tagPartition));

    RpWorldLayout* world = (RpWorldLayout*)env->geom->world;
    part->min = world->boundingBox.inf;
    part->max = world->boundingBox.sup;
    _xVec3 size;
    xVec3Sub(&size, &part->max, &part->min);
    part->total_x = x_spaces;
    part->total_y = y_spaces;
    part->total_z = z_spaces;
    part->space_dim.x = size.x / part->total_x;
    part->space_dim.y = size.y / part->total_y;
    part->space_dim.z = size.z / part->total_z;
    part->space = (_tagPartSpace*)xMemAlloc(
        gActiveHeap, part->total_z * (part->total_y * (part->total_x * sizeof(_tagPartSpace))));

    for (signed int z = 0; z < z_spaces; z++)
    {
        for (signed int y = 0; y < y_spaces; y++)
        {
            for (signed int x = 0; x < x_spaces; x++)
            {
                PartitionSpaceReset(&part->space[xPartitionGetTrueIdx(part, x, y, z)]);
            }
        }
    }

    PartitionSpaceReset(&part->global);
}

signed int xPartitionInsert(_tagPartition* part, void* insert_data, _xVec3* insert_pos)
{
    if (insert_pos->x < part->min.x || insert_pos->x > part->max.x ||
        insert_pos->y < part->min.y || insert_pos->y > part->max.y ||
        insert_pos->z < part->min.z || insert_pos->z > part->max.z)
    {
        PartitionSpaceInsert(&part->global, insert_data);
        return -1;
    }

    signed int idx =
        xPartitionGetTrueIdx(part, (signed int)((insert_pos->x - part->min.x) / part->space_dim.x),
                             (signed int)((insert_pos->y - part->min.y) / part->space_dim.y),
                             (signed int)((insert_pos->z - part->min.z) / part->space_dim.z));
    PartitionSpaceInsert(&part->space[idx], insert_data);
    return idx;
}

signed int xPartitionUpdate(_tagPartition* part, void* data, signed int old_idx,
                            _xVec3* current_pos)
{
    signed int new_idx = PartitionGetIdx(part, current_pos);
    if (old_idx == new_idx)
    {
        return old_idx;
    }

    _tagPartSpace* src = old_idx == -1 ? &part->global : xPartitionGetSpace(part, old_idx);
    _tagPartSpace* dest = new_idx == -1 ? &part->global : xPartitionGetSpace(part, new_idx);
    xPartitionSpaceMove(src, dest, (unsigned int)data);
    return new_idx;
}

#pragma auto_inline on
_tagPartSpace* xPartitionGetSpace(_tagPartition* p, signed int true_idx)
{
    return &p->space[true_idx];
}
#pragma auto_inline off

void xPartitionGetVirtualIdx(_tagPartition* part, _xVec3* insert_pos, signed int* x, signed int* y,
                             signed int* z)
{
    if (x)
    {
        *x = (insert_pos->x - part->min.x) / part->space_dim.x;
    }
    if (y)
    {
        *y = (insert_pos->y - part->min.y) / part->space_dim.y;
    }
    if (z)
    {
        *z = (insert_pos->z - part->min.z) / part->space_dim.z;
    }
}

void xPartitionDump()
{
}

void xPartitionGetLoop(_tagPartition* p, signed int step, signed int* xi, signed int* yi,
                       signed int* zi, signed int xmid, signed int ymid, signed int zmid,
                       signed int* xend, signed int* yend, signed int* zend)
{
    if (xi)
    {
        *xi = xmid - step;
        if (*xi < 0)
        {
            *xi = 0;
        }
        *xend = xmid + step;
        if (*xend >= p->total_x)
        {
            *xend = p->total_x - 1;
        }
    }
    if (yi)
    {
        *yi = ymid - step;
        if (*yi < 0)
        {
            *yi = 0;
        }
        *yend = ymid + step;
        if (*yend >= p->total_y)
        {
            *yend = p->total_y - 1;
        }
    }
    if (zi)
    {
        *zi = zmid - step;
        if (*zi < 0)
        {
            *zi = 0;
        }
        *zend = zmid + step;
        if (*zend >= p->total_z)
        {
            *zend = p->total_z - 1;
        }
    }
}

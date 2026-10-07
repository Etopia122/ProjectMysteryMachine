// RenderWare matrices, vectors and frame hierarchies.
#include "rwcore.h"

#include <math.h>
#include <string.h>

// ---------------------------------------------------------------- matrix helpers
static void mat_identity(RwMatrix* m)
{
    memset(m, 0, sizeof(*m));
    m->right.x = m->up.y = m->at.z = 1.0f;
    m->flags = rwMATRIXTYPEORTHONORMAL | rwMATRIXINTERNALIDENTITY;
}

// out = a * b (row vectors: transform by a, then by b), as RwMatrixMultiply.
static void mat_mul(RwMatrix* out, const RwMatrix* a, const RwMatrix* b)
{
    RwMatrix r;
    const RwV3d* rows[3] = { &a->right, &a->up, &a->at };
    RwV3d* orows[3] = { &r.right, &r.up, &r.at };
    for (int i = 0; i < 3; i++)
    {
        const RwV3d* v = rows[i];
        orows[i]->x = v->x * b->right.x + v->y * b->up.x + v->z * b->at.x;
        orows[i]->y = v->x * b->right.y + v->y * b->up.y + v->z * b->at.y;
        orows[i]->z = v->x * b->right.z + v->y * b->up.z + v->z * b->at.z;
    }
    const RwV3d* p = &a->pos;
    r.pos.x = p->x * b->right.x + p->y * b->up.x + p->z * b->at.x + b->pos.x;
    r.pos.y = p->x * b->right.y + p->y * b->up.y + p->z * b->at.y + b->pos.y;
    r.pos.z = p->x * b->right.z + p->y * b->up.z + p->z * b->at.z + b->pos.z;
    r.flags = 0;
    r.pad1 = r.pad2 = r.pad3 = 0;
    *out = r;
}

// Objects attached to a destroyed frame are left without one (as in RenderWare).
static void detach_objects(RwFrame* f)
{
    RwLLLink* end = &f->objectList.link;
    for (RwLLLink* l = end->next; l && l != end;)
    {
        RwLLLink* n = l->next;
        RwObjectHasFrame* o = rwLLLinkGetData(l, RwObjectHasFrame, lFrame);
        o->object.parent = NULL;
        o->lFrame.next = o->lFrame.prev = NULL;
        l = n;
    }
    rwLinkListInitialize(&f->objectList);
}

extern "C" {

RwMatrix* RwMatrixMultiply(RwMatrix* out, const RwMatrix* a, const RwMatrix* b)
{
    mat_mul(out, a, b);
    return out;
}

RwMatrix* RwMatrixUpdate(RwMatrix* m)
{
    m->flags &= ~rwMATRIXINTERNALIDENTITY;
    return m;
}

RwMatrix* RwMatrixInvert(RwMatrix* dst, const RwMatrix* src)
{
    // General 3x3 inverse plus translation.
    float a = src->right.x, b = src->right.y, c = src->right.z;
    float d = src->up.x, e = src->up.y, f = src->up.z;
    float g = src->at.x, h = src->at.y, i = src->at.z;
    float A = e * i - f * h, B = -(d * i - f * g), C = d * h - e * g;
    float det = a * A + b * B + c * C;
    float inv = det != 0.0f ? 1.0f / det : 0.0f;
    RwMatrix r;
    r.right.x = A * inv;
    r.up.x = B * inv;
    r.at.x = C * inv;
    r.right.y = -(b * i - c * h) * inv;
    r.up.y = (a * i - c * g) * inv;
    r.at.y = -(a * h - b * g) * inv;
    r.right.z = (b * f - c * e) * inv;
    r.up.z = -(a * f - c * d) * inv;
    r.at.z = (a * e - b * d) * inv;
    const RwV3d& p = src->pos;
    r.pos.x = -(p.x * r.right.x + p.y * r.up.x + p.z * r.at.x);
    r.pos.y = -(p.x * r.right.y + p.y * r.up.y + p.z * r.at.y);
    r.pos.z = -(p.x * r.right.z + p.y * r.up.z + p.z * r.at.z);
    r.flags = src->flags & ~rwMATRIXINTERNALIDENTITY;
    r.pad1 = r.pad2 = r.pad3 = 0;
    *dst = r;
    return dst;
}

static RwMatrix* combine(RwMatrix* m, const RwMatrix* t, RwInt32 op)
{
    if (op == rwCOMBINEREPLACE)
    {
        *m = *t;
    }
    else if (op == rwCOMBINEPRECONCAT)
    {
        mat_mul(m, t, m);
    }
    else
    {
        mat_mul(m, m, t);
    }
    return m;
}

RwMatrix* RwMatrixScale(RwMatrix* m, const RwV3d* s, RwInt32 op)
{
    RwMatrix t;
    mat_identity(&t);
    t.right.x = s->x;
    t.up.y = s->y;
    t.at.z = s->z;
    t.flags = 0;
    return combine(m, &t, op);
}

RwMatrix* RwMatrixTranslate(RwMatrix* m, const RwV3d* v, RwInt32 op)
{
    RwMatrix t;
    mat_identity(&t);
    t.pos = *v;
    t.flags = 0;
    return combine(m, &t, op);
}

static void rotation(RwMatrix* t, const RwV3d* axis, float degrees)
{
    float len = sqrtf(axis->x * axis->x + axis->y * axis->y + axis->z * axis->z);
    float x = len > 0 ? axis->x / len : 0, y = len > 0 ? axis->y / len : 0, z = len > 0 ? axis->z / len : 1;
    float r = degrees * (3.14159265358979f / 180.0f);
    float c = cosf(r), s = sinf(r), ic = 1.0f - c;
    mat_identity(t);
    t->right.x = x * x * ic + c;
    t->right.y = x * y * ic + z * s;
    t->right.z = x * z * ic - y * s;
    t->up.x = y * x * ic - z * s;
    t->up.y = y * y * ic + c;
    t->up.z = y * z * ic + x * s;
    t->at.x = z * x * ic + y * s;
    t->at.y = z * y * ic - x * s;
    t->at.z = z * z * ic + c;
    t->flags = rwMATRIXTYPEORTHONORMAL;
}

RwMatrix* RwMatrixRotate(RwMatrix* m, const RwV3d* axis, float angle, RwInt32 op)
{
    RwMatrix t;
    rotation(&t, axis, angle);
    return combine(m, &t, op);
}

float RwV3dLength(const RwV3d* v) { return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z); }

RwV3d* RwV3dTransformPoints(RwV3d* out, const RwV3d* in, RwInt32 n, const RwMatrix* m)
{
    for (RwInt32 i = 0; i < n; i++)
    {
        RwV3d p = in[i];
        out[i].x = p.x * m->right.x + p.y * m->up.x + p.z * m->at.x + m->pos.x;
        out[i].y = p.x * m->right.y + p.y * m->up.y + p.z * m->at.y + m->pos.y;
        out[i].z = p.x * m->right.z + p.y * m->up.z + p.z * m->at.z + m->pos.z;
    }
    return out;
}

RwV3d* RwV3dTransformVectors(RwV3d* out, const RwV3d* in, RwInt32 n, const RwMatrix* m)
{
    for (RwInt32 i = 0; i < n; i++)
    {
        RwV3d p = in[i];
        out[i].x = p.x * m->right.x + p.y * m->up.x + p.z * m->at.x;
        out[i].y = p.x * m->right.y + p.y * m->up.y + p.z * m->at.y;
        out[i].z = p.x * m->right.z + p.y * m->up.z + p.z * m->at.z;
    }
    return out;
}

// ---------------------------------------------------------------- frames
static void frame_mark_dirty(RwFrame* f)
{
    RwFrame* root = f->root;
    if (!(root->object.privateFlags & (rwFRAMEPRIVATEHIERARCHYSYNCLTM | rwFRAMEPRIVATEHIERARCHYSYNCOBJ)))
    {
        rwLinkListAddLLLink(&rwGlobals()->dirtyFrameList, &root->inDirtyListLink);
    }
    root->object.privateFlags |= rwFRAMEPRIVATEHIERARCHYSYNCLTM | rwFRAMEPRIVATEHIERARCHYSYNCOBJ;
    f->object.privateFlags |= rwFRAMEPRIVATESUBTREESYNCLTM | rwFRAMEPRIVATESUBTREESYNCOBJ;
}

RwFrame* RwFrameCreate()
{
    RwFrame* f = (RwFrame*)rwCalloc(1, sizeof(RwFrame) + 0x40); // + room for plugin data
    f->object.type = rwFRAME;
    mat_identity(&f->modelling);
    mat_identity(&f->ltm);
    rwLinkListInitialize(&f->objectList);
    f->root = f;
    f->inDirtyListLink.next = f->inDirtyListLink.prev = NULL;
    return f;
}

static void frame_set_root(RwFrame* f, RwFrame* root)
{
    for (; f; f = f->next)
    {
        f->root = root;
        frame_set_root(f->child, root);
    }
}

RwFrame* RwFrameRemoveChild(RwFrame* child)
{
    RwFrame* parent = (RwFrame*)child->object.parent;
    if (!parent)
    {
        return child;
    }
    if (parent->child == child)
    {
        parent->child = child->next;
    }
    else
    {
        for (RwFrame* s = parent->child; s; s = s->next)
        {
            if (s->next == child)
            {
                s->next = child->next;
                break;
            }
        }
    }
    child->next = NULL;
    child->object.parent = NULL;
    frame_set_root(child, child);
    child->next = NULL;
    frame_mark_dirty(child);
    return child;
}

RwFrame* RwFrameAddChild(RwFrame* parent, RwFrame* child)
{
    if (child->object.parent)
    {
        RwFrameRemoveChild(child);
    }
    if (child->object.privateFlags & (rwFRAMEPRIVATEHIERARCHYSYNCLTM | rwFRAMEPRIVATEHIERARCHYSYNCOBJ) &&
        child->inDirtyListLink.next)
    {
        rwLinkListRemoveLLLink(&child->inDirtyListLink);
        child->inDirtyListLink.next = child->inDirtyListLink.prev = NULL;
        child->object.privateFlags &= ~(rwFRAMEPRIVATEHIERARCHYSYNCLTM | rwFRAMEPRIVATEHIERARCHYSYNCOBJ);
    }
    child->next = parent->child;
    parent->child = child;
    child->object.parent = parent;
    frame_set_root(child, parent->root);
    child->next = child->next; // keep sibling chain
    frame_mark_dirty(child);
    return parent;
}

RwInt32 RwFrameDestroy(RwFrame* f)
{
    if (f->object.parent)
    {
        RwFrameRemoveChild(f);
    }
    if (f->inDirtyListLink.next)
    {
        rwLinkListRemoveLLLink(&f->inDirtyListLink);
    }
    for (RwFrame* c = f->child; c;)
    {
        RwFrame* n = c->next;
        c->object.parent = NULL;
        c->next = NULL;
        frame_set_root(c, c);
        c = n;
    }
    detach_objects(f);
    rwFree(f);
    return 1;
}

static void destroy_tree(RwFrame* f)
{
    while (f)
    {
        RwFrame* n = f->next;
        destroy_tree(f->child);
        f->child = NULL;
        f->object.parent = NULL;
        if (f->inDirtyListLink.next)
        {
            rwLinkListRemoveLLLink(&f->inDirtyListLink);
        }
        detach_objects(f);
        rwFree(f);
        f = n;
    }
}

RwInt32 RwFrameDestroyHierarchy(RwFrame* f)
{
    if (f->object.parent)
    {
        RwFrameRemoveChild(f);
    }
    destroy_tree(f->child);
    f->child = NULL;
    RwFrameDestroy(f);
    return 1;
}

RwFrame* RwFrameForAllChildren(RwFrame* f, RwFrame* (*cb)(RwFrame*, void*), void* data)
{
    for (RwFrame* c = f->child; c;)
    {
        RwFrame* n = c->next;
        if (!cb(c, data))
        {
            break;
        }
        c = n;
    }
    return f;
}

RwFrame* RwFrameGetRoot(const RwFrame* f) { return f->root; }

RwFrame* RwFrameTransform(RwFrame* f, const RwMatrix* m, RwInt32 op)
{
    combine(&f->modelling, m, op);
    frame_mark_dirty(f);
    return f;
}

RwFrame* RwFrameRotate(RwFrame* f, const RwV3d* axis, float angle, RwInt32 op)
{
    RwMatrixRotate(&f->modelling, axis, angle, op);
    frame_mark_dirty(f);
    return f;
}

RwFrame* RwFrameTranslate(RwFrame* f, const RwV3d* v, RwInt32 op)
{
    RwMatrixTranslate(&f->modelling, v, op);
    frame_mark_dirty(f);
    return f;
}

RwFrame* RwFrameOrthoNormalize(RwFrame* f)
{
    RwMatrix* m = &f->modelling;
    RwV3d* r = &m->right;
    RwV3d* u = &m->up;
    RwV3d* a = &m->at;
    float l = RwV3dLength(a);
    if (l > 0) { a->x /= l; a->y /= l; a->z /= l; }
    // right = up x at, up = at x right
    RwV3d nr = { u->y * a->z - u->z * a->y, u->z * a->x - u->x * a->z, u->x * a->y - u->y * a->x };
    l = RwV3dLength(&nr);
    if (l > 0) { nr.x /= l; nr.y /= l; nr.z /= l; }
    *r = nr;
    RwV3d nu = { a->y * r->z - a->z * r->y, a->z * r->x - a->x * r->z, a->x * r->y - a->y * r->x };
    *u = nu;
    m->flags = rwMATRIXTYPEORTHONORMAL;
    frame_mark_dirty(f);
    return f;
}

RwMatrix* RwFrameGetLTM(RwFrame* f);

static void sync_objects(RwFrame* f)
{
    RwLLLink* end = &f->objectList.link;
    for (RwLLLink* l = end->next; l != end; l = l->next)
    {
        RwObjectHasFrame* o = rwLLLinkGetData(l, RwObjectHasFrame, lFrame);
        if (o->sync)
        {
            o->sync(o);
        }
    }
}

static void sync_tree(RwFrame* f, const RwMatrix* parentLtm, bool dirty)
{
    for (; f; f = f->next)
    {
        bool d = dirty || (f->object.privateFlags & rwFRAMEPRIVATESUBTREESYNCLTM);
        if (d)
        {
            if (parentLtm)
            {
                mat_mul(&f->ltm, &f->modelling, parentLtm);
            }
            else
            {
                f->ltm = f->modelling;
            }
            sync_objects(f);
        }
        f->object.privateFlags &= ~(rwFRAMEPRIVATESUBTREESYNCLTM | rwFRAMEPRIVATESUBTREESYNCOBJ);
        sync_tree(f->child, &f->ltm, d);
    }
}

static void sync_root(RwFrame* root)
{
    bool d = root->object.privateFlags & rwFRAMEPRIVATESUBTREESYNCLTM;
    if (d)
    {
        root->ltm = root->modelling;
        sync_objects(root);
    }
    root->object.privateFlags &= ~(rwFRAMEPRIVATESUBTREESYNCLTM | rwFRAMEPRIVATESUBTREESYNCOBJ |
                                   rwFRAMEPRIVATEHIERARCHYSYNCLTM | rwFRAMEPRIVATEHIERARCHYSYNCOBJ);
    sync_tree(root->child, &root->ltm, d);
}

RwInt32 _rwFrameSyncDirty()
{
    RwLinkList* list = &rwGlobals()->dirtyFrameList;
    while (!rwLinkListEmpty(list))
    {
        RwLLLink* l = list->link.next;
        rwLinkListRemoveLLLink(l);
        l->next = l->prev = NULL;
        RwFrame* root = rwLLLinkGetData(l, RwFrame, inDirtyListLink);
        sync_root(root);
    }
    return 1;
}

RwMatrix* RwFrameGetLTM(RwFrame* f)
{
    if (f->root->object.privateFlags & rwFRAMEPRIVATEHIERARCHYSYNCLTM)
    {
        _rwFrameSyncDirty();
    }
    return &f->ltm;
}

RwFrame* RwFrameUpdateObjects(RwFrame* f)
{
    frame_mark_dirty(f);
    return f;
}

void* _rwObjectHasFrameSetFrame(void* object, RwFrame* frame)
{
    RwObjectHasFrame* o = (RwObjectHasFrame*)object;
    if (o->object.parent)
    {
        rwLinkListRemoveLLLink(&o->lFrame);
    }
    o->object.parent = frame;
    if (frame)
    {
        rwLinkListAddLLLink(&frame->objectList, &o->lFrame);
        RwFrameUpdateObjects(frame);
    }
    return object;
}

}

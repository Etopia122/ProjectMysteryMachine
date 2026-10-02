#include "xLinkList.h"

inline void xListInit(st_ELIST* list)
{
    list->head = (st_ENODE*)&list->tail;
    list->tail = 0;
    list->tailprev = (st_ENODE*)list;
}

// Functions are in reverse order of the DOL: with -inline deferred, CW
// emits a TU's functions last-to-first.

void NewList(st_ELIST* list)
{
    xListInit(list);
}

void AddHead(st_ELIST* list, st_ENODE* node)
{
    node->next = list->head;
    node->prev = (st_ENODE*)list;
    list->head->prev = node;
    list->head = node;
}

void AddTail(st_ELIST* list, st_ENODE* node)
{
    st_ENODE* pred = list->tailprev;
    if (pred == 0)
    {
        pred = (st_ENODE*)list;
    }
    node->next = pred->next;
    node->prev = pred;
    pred->next->prev = node;
    pred->next = node;
}

st_ENODE* RemHead(st_ELIST* list)
{
    st_ENODE* node = list->head;
    if (node->next == 0)
    {
        return 0;
    }
    node->next->prev = node->prev;
    node->prev->next = node->next;
    return node;
}

void MoveList(st_ELIST* to, st_ELIST* from)
{
    if (from->head->next == 0)
    {
        xListInit(to);
        return;
    }

    to->head = from->head;
    to->tail = from->tail;
    to->tailprev = from->tailprev;
    to->head->prev = (st_ENODE*)to;
    to->tailprev->next = (st_ENODE*)&to->tail;
    xListInit(from);
}

#ifndef XLINKLIST_H
#define XLINKLIST_H

// Exec-style doubly linked list: the list header doubles as the head and
// tail sentinel nodes (head/tail and tail/tailprev overlap st_ENODE).
class st_ENODE
{
public:
    st_ENODE* next;
    st_ENODE* prev;
};

class st_ELIST
{
public:
    st_ENODE* head;
    st_ENODE* tail;
    st_ENODE* tailprev;
};

void MoveList(st_ELIST* to, st_ELIST* from);
st_ENODE* RemHead(st_ELIST* list);
void AddTail(st_ELIST* list, st_ENODE* node);
void AddHead(st_ELIST* list, st_ENODE* node);
void NewList(st_ELIST* list);

#endif

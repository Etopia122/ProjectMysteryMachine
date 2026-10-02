#ifndef XANIM_H
#define XANIM_H

class xAnimFile;
class xAnimTable;
class xAnimState;
class xAnimTransition;
class xAnimSingle;

typedef unsigned int (*xAnimTransitionConditionalCallback)(xAnimTransition*, xAnimSingle*, void*);
typedef unsigned int (*xAnimTransitionCallback)(xAnimTransition*, xAnimSingle*, void*);

xAnimFile* xAnimFileNew(void* rawData, char* name, unsigned int flags, xAnimFile** linkedList);
xAnimTable* xAnimTableNew(char* name, xAnimTable** linkedList);
xAnimState* xAnimTableNewState(xAnimTable* table, char* name, unsigned int flags,
                               unsigned int userFlags, float speed, float* boneBlend,
                               float* timeSnap, float fadeRecip, unsigned short* fadeOffset);
xAnimTransition* xAnimTableNewTransition(xAnimTable* table, char* source, char* dest,
                                         xAnimTransitionConditionalCallback conditional,
                                         xAnimTransitionCallback callback, unsigned int flags,
                                         unsigned int userFlags, float srcTime, float destTime,
                                         unsigned short priority, unsigned short queuePriority,
                                         float blendRecip, unsigned short* blendOffset);
void xAnimTableAddFile(xAnimTable* table, xAnimFile* file, char* states);

#endif

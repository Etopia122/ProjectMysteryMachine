#ifndef XSND_H
#define XSND_H

#include "xMath3.h"

class _xEnt;

unsigned int xSndPlay(unsigned int id, float volL, float volR, float pitch, unsigned int priority,
                      unsigned int flags, unsigned int parentID);
unsigned int xSndPlay3D(unsigned int id, float vol, float pitch, unsigned int priority,
                        unsigned int flags, _xEnt* parent, float radius);
unsigned int xSndPlay3D(unsigned int id, float vol, float pitch, unsigned int priority,
                        unsigned int flags, _xVec3* pos, float radius);
void xSndPause(unsigned int snd, unsigned int pause);
void xSndStop(unsigned int snd);
void xSndParentDied(unsigned int pid);
void xSndStopChildren(unsigned int pid);

// GameCube: looks a sound up by asset ID (named after BFBB's iSndLookup)
void* iSndLookup(unsigned int id);

#endif

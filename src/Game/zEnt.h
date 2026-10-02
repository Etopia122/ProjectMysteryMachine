#ifndef ZENT_H
#define ZENT_H

#include "xEnt.h"

class xAnimTable;
class xSerial;

class _zEnt : public _xEnt
{
public:
    xAnimTable* atbl; // 0xE8
};

void zEntInit(_zEnt* ent, p2EntAsset* asset, unsigned int type);
void zEntSetup(_zEnt* ent);
void zEntReset(_zEnt* ent);
void zEntAnimEvent(_zEnt* ent, unsigned int animEvent, float* animParam);
void zEntSave(_zEnt* ent, xSerial* s);
void zEntLoad(_zEnt* ent, xSerial* s);

void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to, unsigned int toEvent, float* toParam,
               xBase* toParamWidget, signed int forceEvent);
void zEntEvent(xBase* from, xBase* to, unsigned int toEvent);
void zEntEvent(xBase* to, unsigned int toEvent, float* toParam, xBase* toParamWidget);
void zEntEvent(xBase* to, unsigned int toEvent, float* toParam);
void zEntEvent(xBase* to, unsigned int toEvent);
void zEntEvent(char* to, unsigned int toEvent);

#endif

#ifndef ZENT_H
#define ZENT_H

class xBase;

void zEntEvent(xBase* from, xBase* to, unsigned int toEvent);
void zEntEvent(xBase* to, unsigned int toEvent, float* toParam, xBase* toParamWidget);
void zEntEvent(xBase* to, unsigned int toEvent, float* toParam);
void zEntEvent(xBase* to, unsigned int toEvent);
void zEntEvent(char* to, unsigned int toEvent);

#endif

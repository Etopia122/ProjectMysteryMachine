#ifndef XENV_H
#define XENV_H

#include "iEnv.h"

class _xEnv
{
public:
    iEnv* geom;
    iEnv ienv; // offset 0x4 on GameCube (0x10 on PS2)
};

extern _xEnv* gCurXEnv;

void xEnvRender(_xEnv* env);
void xEnvFree(_xEnv* env);
void xEnvSetup(_xEnv* env);
void xEnvLoadBsp(_xEnv* env, void* data, unsigned int datasize, signed int dataType);

#endif

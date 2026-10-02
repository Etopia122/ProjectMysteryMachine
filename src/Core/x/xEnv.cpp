#include "xEnv.h"

_xEnv* gCurXEnv;

// Functions are in reverse DOL order (-inline deferred emits last to first).

void xEnvLoadBsp(_xEnv* env, void* data, unsigned int datasize, signed int dataType)
{
    if (dataType == 0)
    {
        env->geom = &env->ienv;
    }
    iEnvLoad(env->geom, data, datasize, dataType);
    gCurXEnv = env;
}

void xEnvSetup(_xEnv* env)
{
    iEnvSetup(env->geom);
    gCurXEnv = env;
}

void xEnvFree(_xEnv* env)
{
    if (env->geom)
    {
        iEnvFree(env->geom);
        env->geom = 0;
    }
}

void xEnvRender(_xEnv* env)
{
    if (env->geom)
    {
        iEnvRender(env->geom);
    }
}

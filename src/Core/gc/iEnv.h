#ifndef IENV_H
#define IENV_H

class iEnv
{
public:
    unsigned int data[12];
};

void iEnvRender(iEnv* env);
void iEnvSetup(iEnv* env);
void iEnvFree(iEnv* env);
void iEnvLoad(iEnv* env, void* data, unsigned int datasize, signed int dataType);

#endif

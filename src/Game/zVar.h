#ifndef ZVAR_H
#define ZVAR_H

class zVarEntry
{
public:
    unsigned int entry;
    unsigned int varNameID;
    char* varName;
    unsigned int (*varCB)();
};

extern zVarEntry zVarEntryTable[];

void zVarInit(zVarEntry* table);

#endif

#ifndef ZVAR_H
#define ZVAR_H

class xSerial;

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
void zVarNewGame();
void zVarSave(xSerial* s);
void zVarLoad(xSerial* s);
void zVarMagicProcessText(char* src, char* dst);
void zVarGiveMap();
void zVarUnlockR001();
void zVarUnlockS001();
void zVarUnlockE001();
void zVarUnlockF001();
void zVarSetArea(unsigned int sceneID);
void zVarSetSnackGateValue(unsigned int amount);
void zVarDefeatMasterMind();
void zVarUnlockEndGame();
void zVarUnlockAllMovies();
unsigned int finishedGame();

#endif

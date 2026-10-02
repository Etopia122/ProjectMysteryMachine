#ifndef XINI_H
#define XINI_H

class xIniValue
{
public:
    char* tok;
    char* val;
};

class xIniSection
{
public:
    char* sec;
    signed int first;
    signed int count;
};

class xIniFile
{
public:
    signed int NumValues;
    signed int NumSections;
    xIniValue* Values;
    xIniSection* Sections;
    void* mem;
    char name[256];
    char pathname[256];
};

xIniFile* xIniParse(char* buf, signed int len);
void xIniDestroy(xIniFile* ini);
signed int xIniGetInt(xIniFile* ini, char* tok, signed int def);
float xIniGetFloat(xIniFile* ini, char* tok, float def);
char* xIniGetString(xIniFile* ini, char* tok, char* def);

#endif

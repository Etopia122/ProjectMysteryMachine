#include "xIni.h"
#include "xString.h"

extern "C" {
void* malloc(unsigned int size);
void free(void* ptr);
unsigned int strlen(const char* s);
char* strncpy(char* dst, const char* src, unsigned int n);
char* strstr(const char* s, const char* sub);
int atoi(const char* s);
double atof(const char* s);
}

static inline char* TrimWhitespace(char* string)
{
    for (; *string == ' ' || *string == '\t'; string++)
    {
    }
    if (*string == '\0')
    {
        return string;
    }

    char* substring = &string[strlen(string) - 1];
    for (; *substring == ' ' || *substring == '\t'; substring--)
    {
    }
    substring[1] = '\0';

    return string;
}

static inline signed int xIniGetIndex(xIniFile* ini, char* tok)
{
    for (signed int i = 0; i < ini->NumValues; i++)
    {
        if (xStricmp(ini->Values[i].tok, tok) == 0)
        {
            return i;
        }
    }
    return -1;
}

// Functions are in reverse DOL order (-inline deferred emits last to first).

xIniFile* xIniParse(char* buf, signed int len)
{
    signed int i;
    signed int ccr = 1;
    signed int clf = 1;
    signed int copen = 0;
    signed int lastCRLF = -1;
    for (i = 0; i < len; i++)
    {
        switch (buf[i])
        {
        case '\n':
            lastCRLF = i;
            clf++;
            break;
        case '\r':
            lastCRLF = i;
            ccr++;
            break;
        case '[':
            copen++;
            break;
        }
    }

    if (clf > ccr)
    {
        ccr = clf;
    }

    signed int sectionAlloc = copen;
    signed int valueAlloc = ccr;

    xIniFile* ini = (xIniFile*)malloc(sizeof(xIniFile) + (valueAlloc * sizeof(xIniValue)) +
                                      (sectionAlloc * sizeof(xIniSection)) + (len - lastCRLF));
    ini->mem = 0;
    ini->NumValues = 0;
    ini->NumSections = 0;
    ini->Values = (xIniValue*)(ini + 1);
    ini->Sections = (xIniSection*)(ini->Values + valueAlloc);

    char* lastLine = (char*)(ini->Sections + sectionAlloc);
    strncpy(lastLine, buf + (lastCRLF + 1), len - (lastCRLF + 1));
    lastLine[len - (lastCRLF + 1)] = '\0';

    if (lastCRLF >= 0)
    {
        buf[lastCRLF] = '\0';
    }
    else
    {
        buf[0] = '\0';
    }

    char* ltoken;
    char* line = xStrTok(buf, "\n\r", &ltoken);
    if (line == 0)
    {
        line = xStrTok(lastLine, "\n\r", &ltoken);
        lastLine = 0;
    }

    while (line != 0)
    {
        line = TrimWhitespace(line);
        if (*line != '#' && *line != '\0')
        {
            if (*line == '[')
            {
                char* c = strstr(line, "]");
                if (c != 0)
                {
                    *c = '\0';
                    c = TrimWhitespace(line + 1);
                    if (*c != '\0')
                    {
                        ini->Sections[ini->NumSections].sec = c;
                        ini->Sections[ini->NumSections].first = ini->NumValues;
                        ini->Sections[ini->NumSections].count = 0;
                        ini->NumSections++;
                    }
                }
            }
            else
            {
                char* c = strstr(line, "=");
                if (c != 0)
                {
                    *c = '\0';
                    char* tok = TrimWhitespace(line);
                    if (*tok != '\0')
                    {
                        line = c + 1;
                        c = strstr(line, "#");
                        if (c != 0)
                        {
                            *c = '\0';
                        }
                        char* val = TrimWhitespace(line);
                        ini->Values[ini->NumValues].tok = tok;
                        ini->Values[ini->NumValues].val = val;
                        ini->NumValues++;
                        if (ini->NumSections != 0)
                        {
                            ini->Sections[ini->NumSections - 1].count++;
                        }
                    }
                }
            }
        }

        line = xStrTok(0, "\n\r", &ltoken);
        if (line == 0 && lastLine != 0)
        {
            line = xStrTok(lastLine, "\n\r", &ltoken);
            lastLine = 0;
        }
    }

    return ini;
}

void xIniDestroy(xIniFile* ini)
{
    free(ini->mem);
    free(ini);
}

signed int xIniGetInt(xIniFile* ini, char* tok, signed int def)
{
    signed int index = xIniGetIndex(ini, tok);
    if (index == -1)
    {
        return def;
    }
    return atoi(ini->Values[index].val);
}

float xIniGetFloat(xIniFile* ini, char* tok, float def)
{
    signed int index = xIniGetIndex(ini, tok);
    if (index == -1)
    {
        return def;
    }
    return atof(ini->Values[index].val);
}

#include "xString.h"

extern "C" double atof(const char*);

// Functions are in reverse DOL order (-inline deferred emits last to first).

inline signed int xToUpper(char c)
{
    unsigned char lower = 0;
    if (c >= 'a' && c <= 'z')
    {
        lower = 1;
    }
    return lower ? c - 0x20 : c;
}

unsigned int xStrHash(char* str)
{
    unsigned int hash = 0;
    char c;
    while ((c = *str) != 0)
    {
        str++;
        hash = (char)(c - (c & (c >> 1) & 0x20)) + hash * 131;
    }
    return hash;
}

char* xStrTok(char* string, char* control, char** nextoken)
{
    unsigned char* str;
    unsigned char* ctrl = (unsigned char*)control;
    unsigned char map[32];
    signed int count;

    for (count = 0; count < 32; count++)
    {
        map[count] = 0;
    }

    do
    {
        map[*ctrl >> 3] |= (unsigned char)(1 << (*ctrl & 7));
    } while (*ctrl++);

    str = string ? (unsigned char*)string : (unsigned char*)*nextoken;

    while ((map[*str >> 3] & (1 << (*str & 7))) && *str)
    {
        str++;
    }

    string = (char*)str;

    for (; *str; str++)
    {
        if (map[*str >> 3] & (1 << (*str & 7)))
        {
            *str++ = 0;
            break;
        }
    }

    *nextoken = (char*)str;

    if (string == (char*)str)
    {
        return 0;
    }
    return string;
}

char* xStrTokBuffer(char* string, char* control, void* buffer)
{
    unsigned char* str;
    unsigned char* ctrl = (unsigned char*)control;
    xStrTokBuf* buf = (xStrTokBuf*)buffer;
    char* out = buf->token;
    unsigned char map[32];
    signed int count;

    for (count = 0; count < 32; count++)
    {
        map[count] = 0;
    }

    do
    {
        map[*ctrl >> 3] |= (unsigned char)(1 << (*ctrl & 7));
    } while (*ctrl++);

    str = string ? (unsigned char*)string : buf->next;

    while ((map[*str >> 3] & (1 << (*str & 7))) && *str)
    {
        str++;
    }

    string = (char*)str;

    for (; *str; str++)
    {
        if (map[*str >> 3] & (1 << (*str & 7)))
        {
            str++;
            break;
        }
        *out++ = *str;
    }

    *out = 0;
    buf->next = str;

    if (string == (char*)str)
    {
        return 0;
    }
    return buf->token;
}

signed int xStricmp(char* string1, char* string2)
{
    // TODO: nearly matching; the loop compare and final comparison use the
    // operands in a different register order than the original.
    signed int done = 0;

    while (xToUpper(*string2) == xToUpper(*string1) && !done)
    {
        if (*string1 == 0 || *string2 == 0)
        {
            done = 1;
        }
        else
        {
            string1++;
            string2++;
        }
    }

    if (*string1 == *string2)
    {
        return 0;
    }
    return (xToUpper(*string2) > xToUpper(*string1)) ? -1 : 1;
}

char* xStrupr(char* string)
{
    char* p = string;
    while (*p)
    {
        *p = xToUpper(*p);
        p++;
    }
    return string;
}

signed int xStrParseFloatList(float* dest, char* strbuf, signed int max)
{
    char* str;
    signed int index;
    signed int digits;
    signed int negate;
    char* numstart;
    char savech;

    if ((str = strbuf) == 0)
    {
        return 0;
    }

    for (index = 0; *str != 0 && index < max; index++)
    {
        while (*str == '\t' || *str == ' ' || *str == '[' || *str == ']' || *str == '{' ||
               *str == '}' || *str == '(' || *str == ')' || *str == '+' || *str == ',' ||
               *str == ':' || *str == ';')
        {
            str++;
        }

        if (*str == 0)
        {
            return index;
        }

        if (*str == '-')
        {
            negate = 1;
            str++;
            while (*str == '\t' || *str == ' ')
            {
                str++;
            }
        }
        else
        {
            negate = 0;
        }

        numstart = str;
        digits = 0;
        while ((*str >= '0' && *str <= '9') || *str == '.' || *str == 'E' || *str == 'e' ||
               *str == 'f')
        {
            if (*str >= '0' && *str <= '9')
            {
                digits++;
            }
            str++;
        }

        if (digits == 0)
        {
            return index;
        }

        savech = *str;
        *str = 0;
        dest[index] = atof(numstart);
        if (negate)
        {
            dest[index] = -dest[index];
        }
        *str = savech;
    }

    return index;
}

#include "inireader.h"
#include <errno.h>
#include <limits.h>

typedef enum {
    STR2INT_SUCCESS,
    STR2INT_OVERFLOW,
    STR2INT_UNDERFLOW,
    STR2INT_INCONVERTIBLE
} str2int_errno;

str2int_errno _str2int(int* out, char* s, int base) {
    char* end;
    if (s[0] == '\0' || isspace(s[0]))
        return STR2INT_INCONVERTIBLE;
    errno = 0;
    long l = strtol(s, &end, base);
    /* Both checks are needed because INT_MAX == LONG_MAX is possible. */
    if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
        return STR2INT_OVERFLOW;
    if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
        return STR2INT_UNDERFLOW;
    if (*end != '\0')
        return STR2INT_INCONVERTIBLE;
    *out = l;
    return STR2INT_SUCCESS;
}

int str2int(char* s, int base) {
    int result;
    _str2int(&result, s, base);
    return result;
}

void SetIniPath(const char* szFileName)
{
    inireader.iniName = szFileName;

    static char buf[2000];
    inireader.iniBufSize = sizeof(buf);
    inireader.iniBuf = &buf;
    
    SceUID fileId = sceIoOpen(inireader.iniName, PSP_O_RDONLY, 0777);
    if (fileId < 0)
    {
        return;
    }
    sceIoRead(fileId, inireader.iniBuf, inireader.iniBufSize);
    sceIoClose(fileId);
}

int ReadInteger(char* szSection, char* szKey, int iDefaultValue)
{
    int result = iDefaultValue;
    if (rini_get_key(szSection, szKey, inireader.iniBuf, inireader.iniBufSize, &result, sizeof(result), INT_VAL))
    {
        return result;
    }
    else
    {
        return iDefaultValue;
    }
}

float ReadFloat(char* szSection, char* szKey, float fltDefaultValue)
{
    int BufferSize = 30;
    char Buffer[BufferSize];
    if (rini_get_key(szSection, szKey, inireader.iniBuf, inireader.iniBufSize, Buffer, BufferSize, STRING_VAL))
    {
        return (float)strtod(Buffer, NULL);
    }
    else
    {
        return fltDefaultValue;
    }
}

bool ReadBoolean(char* szSection, char* szKey, bool bDefaultValue)
{
    bool result = bDefaultValue;
    if (rini_get_key(szSection, szKey, inireader.iniBuf, inireader.iniBufSize, &result, sizeof(result), BOOL_VAL))
    {
        return result;
    }
    else
    {
        return bDefaultValue;
    }
}

char* ReadString(char* szSection, char* szKey, char* szDefaultValue, char* Buffer, int BufferSize)
{
    if (rini_get_key(szSection, szKey, inireader.iniBuf, inireader.iniBufSize, Buffer, BufferSize, STRING_VAL))
    {
        while (*Buffer == ' ')
            *Buffer++;
        return Buffer;
    }
    else
    {
        return szDefaultValue;
    }
}

struct inireader_t inireader =
{
    .iniName = 0,
    .iniBuf = 0,
    .iniBufSize = 0,
    .SetIniPath = SetIniPath,
    .ReadInteger = ReadInteger,
    .ReadFloat = ReadFloat,
    .ReadBoolean = ReadBoolean,
    .ReadString = ReadString,
};
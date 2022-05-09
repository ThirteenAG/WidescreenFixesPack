#include "inireader_alt.h"
#include "minIni.h"
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

char iniReaderIniName[255];
void SetIniPath(const char* szFileName)
{
    strcpy(iniReaderIniName, szFileName);
    inireader.iniName = iniReaderIniName;
}

int ReadInteger(char* szSection, char* szKey, int iDefaultValue)
{
    return ini_getl(szSection, szKey, iDefaultValue, inireader.iniName);
}

float ReadFloat(char* szSection, char* szKey, float fltDefaultValue)
{
    return ini_getf(szSection, szKey, fltDefaultValue, inireader.iniName);
}

_Bool ReadBoolean(char* szSection, char* szKey, _Bool bolDefaultValue)
{
    return ini_getbool(szSection, szKey, bolDefaultValue, inireader.iniName);
}

char* ReadString(char* szSection, char* szKey, char* szDefaultValue, char* Buffer, int BufferSize)
{
    ini_gets(szSection, szKey, szDefaultValue, Buffer, BufferSize, inireader.iniName);
    return Buffer;
}

void WriteInteger(char* szSection, char* szKey, int iValue)
{
    ini_putl(szSection, szKey, iValue, inireader.iniName);
}

void WriteFloat(char* szSection, char* szKey, float fltValue)
{
    ini_putf(szSection, szKey, fltValue, inireader.iniName);
}

void WriteBoolean(char* szSection, char* szKey, _Bool bolValue)
{
    ini_putl(szSection, szKey, bolValue, inireader.iniName);
}

void WriteString(char* szSection, char* szKey, char* szValue)
{
    ini_puts(szSection, szKey, szValue, inireader.iniName);
}

struct inireader_t inireader =
{
    .iniName = 0,
    .SetIniPath = SetIniPath,
    .ReadInteger = ReadInteger,
    .ReadFloat = ReadFloat,
    .ReadBoolean = ReadBoolean,
    .ReadString = ReadString,
    .WriteInteger = WriteInteger,
    .WriteFloat = WriteFloat,
    .WriteBoolean = WriteBoolean,
    .WriteString = WriteString
};
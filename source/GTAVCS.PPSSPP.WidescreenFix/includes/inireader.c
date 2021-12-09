#include "inireader.h"
#include "minIni.h"

void SetIniPath(const char* szFileName)
{
    inireader.iniName = szFileName;
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

char* ReadString(char* szSection, char* szKey, char* szDefaultValue)
{
    return 0; //TODO
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
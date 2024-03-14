#include "inireader_alt.h"
#include "minIni.h"
#include <errno.h>
#include <limits.h>
#include <string.h>

char IniName[255];
void Set_Ini_Path(const char* szFileName)
{
    strcpy(IniName, szFileName);
    ini._iniName = IniName;
}

int Read_Integer(char* szSection, char* szKey, int iDefaultValue)
{
    return ini_getl(szSection, szKey, iDefaultValue, ini._iniName);
}

float Read_Float(char* szSection, char* szKey, float fltDefaultValue)
{
    return ini_getf(szSection, szKey, fltDefaultValue, ini._iniName);
}

_Bool Read_Boolean(char* szSection, char* szKey, _Bool bolDefaultValue)
{
    return ini_getbool(szSection, szKey, bolDefaultValue, ini._iniName);
}

char* Read_String(char* szSection, char* szKey, char* szDefaultValue, char* Buffer, int BufferSize)
{
    ini_gets(szSection, szKey, szDefaultValue, Buffer, BufferSize, ini._iniName);
    return Buffer;
}

void Write_Integer(char* szSection, char* szKey, int iValue)
{
    ini_putl(szSection, szKey, iValue, ini._iniName);
}

void Write_Float(char* szSection, char* szKey, float fltValue)
{
    ini_putf(szSection, szKey, fltValue, ini._iniName);
}

void Write_Boolean(char* szSection, char* szKey, _Bool bolValue)
{
    ini_putl(szSection, szKey, bolValue, ini._iniName);
}

void Write_String(char* szSection, char* szKey, char* szValue)
{
    ini_puts(szSection, szKey, szValue, ini._iniName);
}

struct ini_t ini =
{
    ._iniName = 0,
    .Set_Ini_Path = Set_Ini_Path,
    .Read_Integer = Read_Integer,
    .Read_Float = Read_Float,
    .Read_Boolean = Read_Boolean,
    .Read_String = Read_String,
    .Write_Integer = Write_Integer,
    .Write_Float = Write_Float,
    .Write_Boolean = Write_Boolean,
    .Write_String = Write_String
};
#include "inireader.h"

void SetIniPath(const char* szFileContent, size_t size)
{
    inireader.iniBuf = szFileContent;
    inireader.iniBufSize = size;
}

int ReadInteger(char* szSection, char* szKey, int iDefaultValue)
{
    // hex numbers are not supported
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

float ratof(char* arr)
{
    float val = 0;
    int afterdot = 0;
    float scale = 1;
    int neg = 0;

    while (*arr == ' ') {
        arr++;
    }

    if (*arr == '-') {
        arr++;
        neg = 1;
    }
    while (*arr) {
        if (afterdot) {
            scale = scale / 10;
            val = val + (*arr - '0') * scale;
        }
        else {
            if (*arr == '.')
                afterdot++;
            else
                val = val * 10.0f + (*arr - '0');
        }
        arr++;
    }
    if (neg) return -val;
    else    return  val;
}

float ReadFloat(char* szSection, char* szKey, float fltDefaultValue)
{
    static char Buffer[30];
    if (rini_get_key(szSection, szKey, inireader.iniBuf, inireader.iniBufSize, &Buffer, sizeof(Buffer), STRING_VAL))
    {
        return ratof(Buffer);
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
    .iniBuf = 0,
    .iniBufSize = 0,
    .SetIniPath = SetIniPath,
    .ReadInteger = ReadInteger,
    .ReadFloat = ReadFloat,
    .ReadBoolean = ReadBoolean,
    .ReadString = ReadString,
};
#ifndef H_INIREADER
#define H_INIREADER
#include "rini.h"

struct inireader_t {
    int iniBuf;
    int iniBufSize;
    void (*SetIniPath)(const char* szFileContent, size_t size);
    int (*ReadInteger)(char* szSection, char* szKey, int iDefaultValue);
    float (*ReadFloat)(char* szSection, char* szKey, float fltDefaultValue);
    bool (*ReadBoolean)(char* szSection, char* szKey, bool bolDefaultValue);
    char* (*ReadString)(char* szSection, char* szKey, char* szDefaultValue, char* Buffer, int BufferSize);
};
extern struct inireader_t inireader;
#endif

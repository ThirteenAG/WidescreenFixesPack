#include "rini.h"
#include <pspuser.h>
#include <stdlib.h>

int str2int(char* s, int base);

struct inireader_t {
	const char* iniName;
	int iniBuf;
	int iniBufSize;
	void (*SetIniPath)(const char* szFileName);
	int (*ReadInteger)(char* szSection, char* szKey, int iDefaultValue);
	float (*ReadFloat)(char* szSection, char* szKey, float fltDefaultValue);
	bool (*ReadBoolean)(char* szSection, char* szKey, bool bolDefaultValue);
	char* (*ReadString)(char* szSection, char* szKey, char* szDefaultValue, char* Buffer, int BufferSize);
};
extern struct inireader_t inireader;
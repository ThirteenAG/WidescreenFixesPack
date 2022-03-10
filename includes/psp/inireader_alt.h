#pragma once
#include <psptypes.h>
#include <stdlib.h>

int str2int(char* s, int base);

struct inireader_t {
	const char* iniName;
	void (*SetIniPath)(const char* szFileName);
	int (*ReadInteger)(char* szSection, char* szKey, int iDefaultValue);
	float (*ReadFloat)(char* szSection, char* szKey, float fltDefaultValue);
	_Bool (*ReadBoolean)(char* szSection, char* szKey, _Bool bolDefaultValue);
	char* (*ReadString)(char* szSection, char* szKey, char* szDefaultValue, char* Buffer, int BufferSize);
	void (*WriteInteger)(char* szSection, char* szKey, int iValue);
	void (*WriteFloat)(char* szSection, char* szKey, float fltValue);
	void (*WriteBoolean)(char* szSection, char* szKey, _Bool bolValue);
	void (*WriteString)(char* szSection, char* szKey, char* szValue);
};
extern struct inireader_t inireader;
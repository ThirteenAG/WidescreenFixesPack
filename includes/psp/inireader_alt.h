#pragma once
#include <psptypes.h>
#include <stdlib.h>

struct ini_t {
	const char* _iniName;
	void (*Set_Ini_Path)(const char* szFileName);
	int (*Read_Integer)(char* szSection, char* szKey, int iDefaultValue);
	float (*Read_Float)(char* szSection, char* szKey, float fltDefaultValue);
	_Bool (*Read_Boolean)(char* szSection, char* szKey, _Bool bolDefaultValue);
	char* (*Read_String)(char* szSection, char* szKey, char* szDefaultValue, char* Buffer, int BufferSize);
	void (*Write_Integer)(char* szSection, char* szKey, int iValue);
	void (*Write_Float)(char* szSection, char* szKey, float fltValue);
	void (*Write_Boolean)(char* szSection, char* szKey, _Bool bolValue);
	void (*Write_String)(char* szSection, char* szKey, char* szValue);
};
extern struct ini_t ini;
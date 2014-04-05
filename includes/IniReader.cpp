#include "stdafx.h"
#include "..\includes\IniReader.h"
#include <iostream>
#include <Windows.h>
using namespace std;
#pragma warning(disable:4996)
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

CIniReader::CIniReader(char* szFileName)
{
	char			moduleName[MAX_PATH];
	char			dllPath[MAX_PATH];
	char			iniName[MAX_PATH];
	char*			tempPointer;

	GetModuleFileName((HINSTANCE)&__ImageBase, moduleName, MAX_PATH);
	tempPointer = strrchr(moduleName, '.');
	*tempPointer = '\0';
	tempPointer = strrchr(moduleName, '\\');
	strncpy(iniName, tempPointer + 1, 255);
	strcat(iniName, ".ini");
	strncpy(dllPath, moduleName, (tempPointer - moduleName + 1));
	dllPath[tempPointer - moduleName + 1] = '\0';
	if (strcmp(szFileName, "") == 0) //if (szFileName == "")
	{
	strcat(dllPath, iniName);
	} else {
	strcat(dllPath, szFileName);
	}

 memset(m_szFileName, 0x00, 255);
 memcpy(m_szFileName, dllPath, strlen(dllPath));
}
int CIniReader::ReadInteger(char* szSection, char* szKey, int iDefaultValue)
{
 int iResult = GetPrivateProfileInt(szSection,  szKey, iDefaultValue, m_szFileName); 
 return iResult;
}
float CIniReader::ReadFloat(char* szSection, char* szKey, float fltDefaultValue)
{
 char szResult[255];
 char szDefault[255];
 float fltResult;
 _snprintf(szDefault, 255, "%f",fltDefaultValue);
 GetPrivateProfileString(szSection,  szKey, szDefault, szResult, 255, m_szFileName); 
 fltResult = (float)atof(szResult);
 return fltResult;
}
bool CIniReader::ReadBoolean(char* szSection, char* szKey, bool bolDefaultValue)
{
 char szResult[255];
 char szDefault[255];
 bool bolResult;
 _snprintf(szDefault, 255, "%s", bolDefaultValue ? "True" : "False");
 GetPrivateProfileString(szSection, szKey, szDefault, szResult, 255, m_szFileName); 
 bolResult =  (strcmp(szResult, "True") == 0 || 
		strcmp(szResult, "true") == 0) ? true : false;
 return bolResult;
}
char* CIniReader::ReadString(char* szSection, char* szKey, const char* szDefaultValue)
{
 char* szResult = new char[255];
 memset(szResult, 0x00, 255);
 GetPrivateProfileString(szSection,  szKey, 
		szDefaultValue, szResult, 255, m_szFileName); 
 return szResult;
}
void CIniReader::WriteInteger(char* szSection, char* szKey, int iValue)
{
	char szValue[255];
	_snprintf(szValue, 255, "%d", iValue);
	WritePrivateProfileString(szSection, szKey, szValue, m_szFileName);
}
void CIniReader::WriteFloat(char* szSection, char* szKey, float fltValue)
{
	char szValue[255];
	_snprintf(szValue, 255, "%f", fltValue);
	WritePrivateProfileString(szSection, szKey, szValue, m_szFileName);
}
void CIniReader::WriteBoolean(char* szSection, char* szKey, bool bolValue)
{
	char szValue[255];
	_snprintf(szValue, 255, "%s", bolValue ? "True" : "False");
	WritePrivateProfileString(szSection, szKey, szValue, m_szFileName);
}
void CIniReader::WriteString(char* szSection, char* szKey, char* szValue)
{
	WritePrivateProfileString(szSection, szKey, szValue, m_szFileName);
}
#ifndef INIREADER_H
#define INIREADER_H
#include <iostream>
#include <Windows.h>
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

class CIniReader
{
private:
	char m_szFileName[MAX_PATH];

public:
	CIniReader()
	{
		SetIniPath("");
	}

	CIniReader(char* szFileName)
	{
		SetIniPath(szFileName);
	}

public:
	char* GetIniPath()
	{
		return m_szFileName;
	}

	void SetIniPath()
	{
		SetIniPath("");
	}

	void SetIniPath(char* szFileName)
	{
		char  moduleName[MAX_PATH];
		char  iniPath[MAX_PATH];
		char  iniName[MAX_PATH];
		char* ptr;

		GetModuleFileName((HINSTANCE)&__ImageBase, moduleName, MAX_PATH);
		*strrchr(moduleName, '.') = '\0';
		ptr = strrchr(moduleName, '\\');
		strcpy_s(iniName, ptr + 1);
		strcat_s(iniName, ".ini");
		strncpy_s(iniPath, moduleName, (ptr - moduleName + 1));
		iniPath[ptr - moduleName + 1] = '\0';

		if (strchr(szFileName, ':') != NULL)
		{
			strncpy_s(m_szFileName, szFileName, sizeof(m_szFileName));
			return;
		}
		else if (strcmp(szFileName, "") == 0)
		{
			strcat_s(iniPath, iniName);
		}
		else 
		{
			strcat_s(iniPath, szFileName);
		}

		strncpy_s(m_szFileName, iniPath, sizeof(m_szFileName));
	}

	int ReadInteger(char* szSection, char* szKey, int iDefaultValue)
	{
		int iResult = GetPrivateProfileInt(szSection, szKey, iDefaultValue, m_szFileName);
		return iResult;
	}

	float ReadFloat(char* szSection, char* szKey, float fltDefaultValue)
	{
		char szResult[255];
		char szDefault[255];
		float fltResult;
		_snprintf_s(szDefault, 255, "%f", fltDefaultValue);
		GetPrivateProfileString(szSection, szKey, szDefault, szResult, 255, m_szFileName);
		fltResult = (float)atof(szResult);
		return fltResult;
	}

	bool ReadBoolean(char* szSection, char* szKey, bool bolDefaultValue)
	{
		char szResult[255];
		char szDefault[255];
		bool bolResult;
		_snprintf_s(szDefault, 255, "%s", bolDefaultValue ? "True" : "False");
		GetPrivateProfileString(szSection, szKey, szDefault, szResult, 255, m_szFileName);
		bolResult = (strcmp(szResult, "True") == 0 || strcmp(szResult, "true") == 0) ? true : false;
		return bolResult;
	}

	char* ReadString(char* szSection, char* szKey, const char* szDefaultValue)
	{
		char* szResult = new char[255];
		memset(szResult, 0x00, 255);
		GetPrivateProfileString(szSection, szKey, szDefaultValue, szResult, 255, m_szFileName);
		return szResult;
	}

	void WriteInteger(char* szSection, char* szKey, int iValue)
	{
		char szValue[255];
		_snprintf_s(szValue, 255, "%s%d", " ", iValue);
		WritePrivateProfileString(szSection, szKey, szValue, m_szFileName);
	}

	void WriteFloat(char* szSection, char* szKey, float fltValue)
	{
		char szValue[255];
		_snprintf_s(szValue, 255, "%s%f", " ", fltValue);
		WritePrivateProfileString(szSection, szKey, szValue, m_szFileName);
	}

	void WriteBoolean(char* szSection, char* szKey, bool bolValue)
	{
		char szValue[255];
		_snprintf_s(szValue, 255, "%s%s", " ", bolValue ? "True" : "False");
		WritePrivateProfileString(szSection, szKey, szValue, m_szFileName);
	}

	void WriteString(char* szSection, char* szKey, char* szValue)
	{
		WritePrivateProfileString(szSection, szKey, szValue, m_szFileName);
	}
};

#endif //INIREADER_H
#include "..\includes\stdafx.h"
#include "tchar.h"
#include "Strsafe.h"

HWND hWnd;

bool bFixHUD, bFixFOV;
int ResX;
int ResY;
HKEY hKey;
DWORD _REG, jmpAddr1, jmpAddr2, jmpAddr3, jmpAddr4, jmpAddr5, jmpAddr6;
int nGameSpeed;
float fAspectRatio;
float fHudOffset, fHUDOffset1;

#define _USE_MATH_DEFINES
#include "math.h"
#define DEGREE_TO_RADIAN(fAngle) \
	((fAngle)* (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) \
	((fAngle)* 180.0f / (float)M_PI)
#define SCREEN_AR_NARROW			(4.0f / 3.0f)	// 640.0f / 480.0f
#define SCREEN_FOV_HORIZONTAL		48.0f
#define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))	// Default is 75.0f.
float fScreenFieldOfViewVStd = SCREEN_FOV_VERTICAL;
float fDynamicScreenFieldOfViewScale;
float fFOVFactor;

double SetAR()
{
	return fAspectRatio * ((*(float*)0x750FAC) / (4.0f / 3.0f));
}

void __declspec(naked) CenterText()
{
	_asm
	{
		fadd    st, st(1)
		fstp    dword ptr[esp + 28h]
		mov _REG, ecx
		mov     ecx, [esp + 28h]
		mov		fHUDOffset1, ecx
	}
	fHUDOffset1 += fHudOffset;
	_asm
	{
		mov     ecx, fHUDOffset1
		mov     dword ptr[esp + 28h], ecx
		mov     ecx, _REG
		jmp     jmpAddr1
	}
}

void __declspec(naked) CenterHUD()
{
	_asm
	{
		fadd    st, st(1)
		fstp    dword ptr[esp + 28h]
		mov		_REG, ecx
		mov     ecx, [esp + 28h]
		mov		fHUDOffset1, ecx
	}
	fHUDOffset1 += fHudOffset;
	_asm
	{
		mov     ecx, fHUDOffset1
		mov     dword ptr[esp + 28h], ecx
		mov     ecx, _REG
		jmp     jmpAddr2
	}
}

void __declspec(naked) CenterHUD2()
{
	_asm
	{
		fadd    st, st(1)
		fstp    dword ptr[esp + 18h]
		mov		_REG, ecx
		mov     ecx, [esp + 18h]
		mov		fHUDOffset1, ecx
	}
	fHUDOffset1 += fHudOffset;
	_asm
	{
		mov     ecx, fHUDOffset1
		mov     dword ptr[esp + 18h], ecx
		mov     ecx, _REG
		jmp     jmpAddr3
	}
}

void __declspec(naked) CenterHUD3()
{
	_asm
	{
		fadd    st, st(1)
		fstp    dword ptr[esp + 14h]
		mov		_REG, ecx
	}
	__asm	mov     ecx, [esp + 14h]
	__asm	mov		fHUDOffset1, ecx
	fHUDOffset1 += fHudOffset;
	__asm	mov     ecx, fHUDOffset1
	__asm	mov     dword ptr[esp + 14h], ecx

	__asm	mov     ecx, [esp + 18h]
	__asm	mov		fHUDOffset1, ecx
	fHUDOffset1 += fHudOffset;
	__asm	mov     ecx, fHUDOffset1
	__asm	mov     dword ptr[esp + 18h], ecx
	_asm
	{
		mov     ecx, _REG
		jmp     jmpAddr5
	}
}

void __declspec(naked) CenterHUD4()
{
	_asm
	{
		fadd    dword ptr[esp + 20h]
		fstp    dword ptr[esp + 60h]
		mov		_REG, ecx
	}
	__asm	mov     ecx, [esp + 0x5C]
	__asm	mov		fHUDOffset1, ecx
	fHUDOffset1 += fHudOffset;
	__asm mov     ecx, fHUDOffset1
	__asm mov     dword ptr[esp + 0x5C], ecx


	__asm	mov     ecx, [esp + 0x6C]
	__asm	mov		fHUDOffset1, ecx
	fHUDOffset1 += fHudOffset;
	__asm mov     ecx, fHUDOffset1
	__asm mov     dword ptr[esp + 0x6C], ecx


	__asm	mov     ecx, [esp + 0x7C]
	__asm	mov		fHUDOffset1, ecx
	fHUDOffset1 += fHudOffset;
	__asm mov     ecx, fHUDOffset1
	__asm mov     dword ptr[esp + 0x7C], ecx


	__asm	mov     ecx, [esp + 0x8C]
	__asm	mov		fHUDOffset1, ecx
	fHUDOffset1 += fHudOffset;
	__asm mov     ecx, fHUDOffset1
	__asm mov     dword ptr[esp + 0x8C], ecx

	_asm
	{
		mov     ecx, _REG
		jmp     jmpAddr4
	}
}

void Init()
{
	CIniReader iniReader("");
	ResX = iniReader.ReadInteger("MAIN", "ResX", 0);
	ResY = iniReader.ReadInteger("MAIN", "ResY", 0);
	bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
	bFixFOV = iniReader.ReadFloat("MAIN", "FixFOV", 1) != 0;
	nGameSpeed = iniReader.ReadInteger("MAIN", "GameSpeed", 30);

	if (!ResX || !ResY) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		ResX = info.rcMonitor.right - info.rcMonitor.left;
		ResY = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	DWORD dwDisp;
	RegCreateKeyExA(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Activision\\Gun\\Settings"), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp);
	if (dwDisp == REG_CREATED_NEW_KEY || dwDisp == REG_OPENED_EXISTING_KEY)
	{
		char data[20];
		char res[20];

		sprintf_s(res, "%d", ResX);
		strcat_s(data, res);
		strcat_s(data, "x");
		sprintf_s(res, "%d", ResY);
		strcat_s(data, res);
		strcat_s(data, "\0");

		if (!RegSetValueEx(hKey, TEXT("Resolution"), NULL, REG_SZ, (LPBYTE)data, _tcslen(data) * sizeof(TCHAR)) == ERROR_SUCCESS)
			MessageBox(0, "Error setting the value of the registry key.", "GUN", 0);

		RegCloseKey(hKey);
	}
	else
	{
		MessageBox(0, "Error creating the registry subkey.", "GUN", 0);
	}

	Sleep(1000); //steam

	fAspectRatio = static_cast<float>(ResX) / static_cast<float>(ResY);
	injector::MakeJMP(0x4BA680, SetAR);

	if (bFixHUD)
	{
		static float fHudScaleX = 1.0f / static_cast<float>(ResX) * (static_cast<float>(ResY) / 480.0f);
		injector::WriteMemory<float>(0x6814CC, fHudScaleX, true);

		fHudOffset = (static_cast<float>(ResX) - static_cast<float>(ResY) * (4.0f / 3.0f)) / 2.0f;

		jmpAddr1 = 0x4DC226 + 6;
		injector::MakeJMP(0x4DC226, CenterText, true); //text

		jmpAddr2 = 0x4F12DB + 6;
		injector::MakeJMP(0x4F12DB, CenterHUD, true); //graphics

		jmpAddr3 = 0x4F12E9 + 6;
		injector::MakeJMP(0x4F12E9, CenterHUD2, true); //graphics

		jmpAddr4 = 0x4F1BD7 + 8;
		injector::MakeJMP(0x4F1BD7, CenterHUD4, true); //graphics

		jmpAddr5 = 0x4F0D06 + 6;
		injector::MakeJMP(0x4F0D06, CenterHUD3, true); //graphics

		injector::WriteMemory<uchar>(0x4FC837, 0xC0, true); //main menu cursor fix
		injector::WriteMemory<uchar>(0x4FC847, 0xC0, true);
	}

	if (bFixFOV)
	{
		fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * (static_cast<float>(ResX) / static_cast<float>(ResY)))) * (1.0f / SCREEN_FOV_HORIZONTAL);
		fFOVFactor = fDynamicScreenFieldOfViewScale * 114.59155f;
		injector::WriteMemory(0x498BA3 + 0x2, &fFOVFactor, true);
	}

	if (nGameSpeed)
	{
		injector::WriteMemory(0x52E7CA, nGameSpeed, true);
	}
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
	}
	return TRUE;
}
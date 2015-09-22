#include "stdafx.h"
#include "stdio.h"
#include "..\includes\injector\injector.hpp"
#include "..\includes\IniReader.h"

HWND hWnd;
HINSTANCE hExecutableInstance;

#define _USE_MATH_DEFINES
#include "math.h"
#define DEGREE_TO_RADIAN(fAngle) \
	((fAngle)* (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) \
	((fAngle)* 180.0f / (float)M_PI)
#define SCREEN_AR_NARROW			(4.0f / 3.0f)	// 640.0f / 480.0f
#define SCREEN_FOV_HORIZONTAL		75.0f
#define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))	// Default is 75.0f.
float fScreenFieldOfViewVStd = SCREEN_FOV_VERTICAL;
float fDynamicScreenFieldOfViewScale;

struct Screen
{
	int Width;
	int Height;
	float fWidth;
	float fHeight;
	float fFieldOfView;
	float fAspectRatio;
	float fHudOffset;
	float fHudOffsetRight;
	float fFMVoffsetStartX;
	float fFMVoffsetEndX;
	float fFMVoffsetStartY;
	float fFMVoffsetEndY;
} Screen;

HMODULE D3DDrv, WinDrv, Engine, Core;
DWORD hookJmpAddr, hookJmpAddr2, hookJmpAddr3, hookJmpAddr4, hookJmpAddr5, hookJmpAddr6;
DWORD epJump, dword_10173E5C;
DWORD nForceShadowBufferSupport, nFMVWidescreenMode;
char* UserIni;



void Init()
{
	CIniReader iniReader("");
	Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
	Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);

	if (Screen.Width == -1 || Screen.Height == -1)
		return;

	if (!Screen.Width || !Screen.Height) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		Screen.Width = info.rcMonitor.right - info.rcMonitor.left;
		Screen.Height = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	Screen.fWidth = static_cast<float>(Screen.Width);
	Screen.fHeight = static_cast<float>(Screen.Height);
	Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

	char ResX[20];
	char ResY[20];
	_snprintf(ResX, 20, "%d", Screen.Width);
	_snprintf(ResY, 20, "%d", Screen.Height);

	CIniReader iniWriter(UserIni);
	iniWriter.WriteString("WinDrv.WindowsClient", "WindowedViewportX", ResX);
	iniWriter.WriteString("WinDrv.WindowsClient", "WindowedViewportY", ResY);
	iniWriter.WriteString("WinDrv.WindowsClient", "FullscreenViewportX", ResX);
	iniWriter.WriteString("WinDrv.WindowsClient", "FullscreenViewportY", ResY);

	_asm
	{
		PUSH 60h
		PUSH 0x1093E4D0
		jmp		epJump
	}
}



DWORD WINAPI Thread(LPVOID)
{
	while (true)
	{
		Sleep(0);
		D3DDrv = GetModuleHandle("D3DDrv");
		WinDrv = GetModuleHandle("WinDrv");
		Engine = GetModuleHandle("Engine");
		if (D3DDrv && WinDrv && Engine)
			break;
	}



	return 0;
}

DWORD WINAPI SteamHandler(LPVOID)
{
	while (*(unsigned char*)0x10926D65 != 0x6A)
		Sleep(0);

	injector::MakeJMP(0x10926D65, Init, true);
	epJump = 0x10926D65 + 7;
	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		DWORD fAttr = GetFileAttributes("SplinterCell4.ini");
		if ((fAttr != INVALID_FILE_ATTRIBUTES) && !(fAttr & FILE_ATTRIBUTE_DIRECTORY))
		{
			UserIni = ".\\SplinterCell4.ini";
		}
		else
		{
			UserIni = "..\\SplinterCell4.ini";
		}

		hExecutableInstance = GetModuleHandle(NULL);
		IMAGE_NT_HEADERS* ntHeader = (IMAGE_NT_HEADERS*)((DWORD)hExecutableInstance + ((IMAGE_DOS_HEADER*)hExecutableInstance)->e_lfanew);
		BYTE* ep = (BYTE*)((DWORD)hExecutableInstance + ntHeader->OptionalHeader.AddressOfEntryPoint);
		if (*(BYTE*)ep == 0x53)
		{
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&SteamHandler, NULL, 0, NULL);
		}
		else
		{
			injector::MakeJMP(ep, Init, true);
			epJump = (DWORD)ep + 7;
		}
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
	}
	return TRUE;
}
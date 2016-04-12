#include "..\includes\stdafx.h"

HWND hWnd;
HINSTANCE hExecutableInstance;
BYTE originalCode[5];
BYTE* originalEP;

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

DWORD hookJmpAddr, hookJmpAddr4, hookJmpAddr5;
DWORD epJump;
float HUDScaleX, TextScaleX;
wchar_t* UserIni;
char pszPath[MAX_PATH];

void SetRes()
{
	wcstombs(pszPath, UserIni, wcslen(UserIni));
	char ResX[20];
	char ResY[20];
	_snprintf(ResX, 20, "%d", Screen.Width);
	_snprintf(ResY, 20, "%d", Screen.Height);
	WritePrivateProfileString("WinDrv.WindowsClient", "WindowedViewportX", ResX, pszPath);
	WritePrivateProfileString("WinDrv.WindowsClient", "WindowedViewportY", ResY, pszPath);
	WritePrivateProfileString("WinDrv.WindowsClient", "FullscreenViewportX", ResX, pszPath);
	WritePrivateProfileString("WinDrv.WindowsClient", "FullscreenViewportY", ResY, pszPath);
}

void __declspec(naked) SetRes_Hook()
{
	_asm
	{
		LEA ECX, [ESP + 20h]
		PUSH ECX
		STOSW
		mov UserIni, edi
		call SetRes
		jmp	    hookJmpAddr
	}
}

float __EDX;
void __declspec(naked) UGameEngine_Draw_Hook1()
{
	_asm
	{
		mov  edx, [ecx + 2BCh]
		mov  __EDX, edx
	}
	__EDX *= fDynamicScreenFieldOfViewScale;
	__asm   mov edx, __EDX
	__asm	jmp	 hookJmpAddr4
}

float __ECX;
void __declspec(naked) UGameEngine_Draw_Hook2()
{
	_asm
	{
		mov  ecx, [eax + 374h]
		mov  __ECX, ecx
	}
	__ECX *= fDynamicScreenFieldOfViewScale;
	__asm   mov ecx, __ECX
	__asm	jmp	 hookJmpAddr5
}

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

	injector::MakeJMP(0x10CD09AF, SetRes_Hook, true);
	hookJmpAddr = 0x10CD09B6;

	// return to the original EP
	*(DWORD*)originalEP = *(DWORD*)&originalCode;
	*(BYTE*)(originalEP + 4) = originalCode[4];
	_asm
	{
		jmp originalEP
	}
}

DWORD WINAPI Thread(LPVOID)
{
	injector::MakeInline<0x10CC622C, 0x10CC6232>([](injector::reg_pack& regs)
	{
		*(DWORD*)(regs.ebx + 0x58) = regs.ebp;
		*(DWORD*)(regs.ebx + 0x5C) = regs.edi;
		Screen.Width = regs.ebp;
		Screen.Height = regs.edi;
		Screen.fWidth = static_cast<float>(Screen.Width);
		Screen.fHeight = static_cast<float>(Screen.Height);
		Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

		HUDScaleX = (1.0f / Screen.fWidth * (Screen.fHeight / 480.0f)) * 2.0f;
		injector::WriteMemory(0x1120B6BC, HUDScaleX, true);
		TextScaleX = ((4.0f / 3.0f) / Screen.fAspectRatio) * 2.0f;
		Screen.fHudOffset = TextScaleX / 2.0f;

		fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);

		//FMV
		//Screen.fFMVoffsetStartX = ((Screen.fHeight * Screen.fAspectRatio) - (Screen.fHeight * (4.0f / 3.0f))) / 2.0f;
		//injector::WriteMemory(0x10C863D6 + 0x4, Screen.fFMVoffsetStartX, true);
		//actually it scales perfectly as is.
	});

	//HUD
	injector::MakeInline<0x10ADB0BC, 0x10ADB0C7>([](injector::reg_pack& regs)
	{
		injector::WriteMemory(0x1120B6BC, HUDScaleX, true);
		*(DWORD*)(regs.esp + 0x8) = *(WORD*)(regs.eax + 0x86);
	});
	injector::WriteMemory(0x10ADADBE + 0x2, &Screen.fHudOffset, true);

	//TEXT
	injector::WriteMemory(0x10B149CE + 0x2, &TextScaleX, true);
	injector::WriteMemory(0x10B14BAD + 0x2, &Screen.fHudOffset, true);

	//FOV
	injector::MakeJMP(0x10A3E67F, UGameEngine_Draw_Hook1, true);
	injector::MakeJMP(0x10A3E8A0, UGameEngine_Draw_Hook2, true);
	hookJmpAddr4 = 0x10A3E685;
	hookJmpAddr5 = 0x10A3E8A6;
	return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		hExecutableInstance = GetModuleHandle(NULL);
		IMAGE_NT_HEADERS* ntHeader = (IMAGE_NT_HEADERS*)((DWORD)hExecutableInstance + ((IMAGE_DOS_HEADER*)hExecutableInstance)->e_lfanew);
		BYTE* ep = (BYTE*)((DWORD)hExecutableInstance + ntHeader->OptionalHeader.AddressOfEntryPoint);
		// back up original code
		*(DWORD*)&originalCode = *(DWORD*)ep;
		originalCode[4] = *(ep + 4);
		originalEP = ep;

		injector::MakeJMP(ep, Init, true);

		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
	}
	return TRUE;
}
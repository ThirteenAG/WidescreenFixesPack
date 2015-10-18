#include "..\includes\stdafx.h"

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
DWORD epJump;
char* UserIni;
float HUDScaleX, TextScaleX;
int FilmstripScaleX, FilmstripOffset;
float f1_0 = 1.0f;

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

void __declspec(naked) Text_Hook()
{
	_asm
	{
		movss xmm2, TextScaleX
		divss   xmm1, xmm2
		movss   dword ptr [esp + 14h], xmm1
		jmp	 hookJmpAddr3
	}
}

void __declspec(naked) HUD_Hook()
{
	_asm
	{
		movss xmm0, Screen.fHudOffset
		subss   xmm3, xmm0
		addss   xmm3, xmm4
		movss xmm0, f1_0
		jmp	 hookJmpAddr4
	}
}


void __declspec(naked) RenderFilmstrip_Hook()
{
	_asm
	{
		mov     ecx, [ebx + 4]
		mov     ecx, FilmstripScaleX
		add     ecx, ecx
		jmp		hookJmpAddr5
	}
}

void __declspec(naked) RenderFilmstrip_Hook2()
{
	_asm
	{
		mov		edx, FilmstripOffset
		mov		[esp + 10h], edx
		fild    dword ptr [esp + 10h]
		jmp		hookJmpAddr2
	}
}


float __EDX;
void __declspec(naked) UGameEngine_Draw_Hook()
{
	_asm
	{
		mov  edx, [ecx + 628h]
		mov  __EDX, edx
	}
	__EDX *= fDynamicScreenFieldOfViewScale;
	__asm   mov edx, __EDX
	__asm	jmp	 hookJmpAddr6
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

	DWORD pfInitDraw = (DWORD)GetProcAddress(Engine, "?InitDraw@ImageUnreal@magma@@SAXXZ");
	DWORD pWidthScale = (DWORD)GetProcAddress(Engine, "?m_widthScale@ImageUnreal@magma@@0MA");
	DWORD pfImageUnrealDraw = (DWORD)GetProcAddress(Engine, "?Draw@ImageUnreal@magma@@UAEXXZ");

	HUDScaleX = 2.0f / (600.0f * Screen.fAspectRatio);
	injector::MakeNOP(pfInitDraw + 0x153, 8, true);
	injector::WriteMemory<float>(pWidthScale, HUDScaleX, true);

	TextScaleX = 600.0f * Screen.fAspectRatio;
	Screen.fHudOffset = ((4.0f / 3.0f) / Screen.fAspectRatio);

	injector::MakeJMP((0x1030678D - 0x10300000) + (unsigned char*)Engine, Text_Hook, true);
	hookJmpAddr3 = (0x10306797 - 0x10300000) + (DWORD)((unsigned char*)Engine);

	injector::WriteMemory((0x103069A5 + 0x4 - 0x10300000) + (unsigned char*)Engine, &Screen.fHudOffset, true);

	injector::MakeJMP(pfImageUnrealDraw + 0x2EC, HUD_Hook, true);
	hookJmpAddr4 = pfImageUnrealDraw + 0x2EC + 0x8;

	//Minimap
	DWORD pfRenderFilmstrip = (DWORD)GetProcAddress(D3DDrv, "?RenderFilmstrip@UD3DRenderDevice@@UAE_NXZ");

	FilmstripScaleX = static_cast<int>(Screen.fWidth / (1280.0f / (368.0 * ((4.0 / 3.0) / (Screen.fAspectRatio)))));
	FilmstripOffset = static_cast<int>((((Screen.fWidth / 2.0f) - ((Screen.fHeight * (4.0f / 3.0f)) / 2.0f)) * 2.0f) + ((float)FilmstripScaleX / 5.25f));

	injector::MakeJMP(pfRenderFilmstrip + 0x38C, RenderFilmstrip_Hook, true);
	hookJmpAddr5 = pfRenderFilmstrip + 0x38C + 0x5;

	injector::MakeJMP(pfRenderFilmstrip + 0x350, RenderFilmstrip_Hook2, true);
	hookJmpAddr2 = pfRenderFilmstrip + 0x350 + 0x8;

	//FOV
	DWORD pfDraw = (DWORD)GetProcAddress(Engine, "?Draw@UGameEngine@@UAEXPAVUViewport@@HPAEPAH@Z");
	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);

	for (size_t i = (0xD47 - 0x100); i < (0xD47 + 0x100); i++)
	{
		if (*(DWORD*)(pfDraw + i) == 0x0628918B && *(DWORD*)(pfDraw + i + 4) == 0x8B520000)
		{
			injector::MakeJMP(pfDraw + i /*0xD47*/, UGameEngine_Draw_Hook, true);
			hookJmpAddr6 = pfDraw + i /*0xD47*/ + 0x6;
			break;
		}
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
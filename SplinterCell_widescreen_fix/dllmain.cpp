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

HMODULE D3DDrv, WinDrv, Engine;
DWORD hookJmpAddr, hookJmpAddr2, hookJmpAddr3, hookJmpAddr4;
DWORD dword_1003DBA0;
DWORD dword_1003DBB4;
DWORD dword_1003DBA4;
DWORD epJump;
DWORD nForceShadowBufferSupport, nFMVWidescreenMode;
char* UserIni;
bool bIsSteam;

void __declspec(naked) UD3DRenderDevice_SetRes_Hook()
{
	_asm
	{
		mov     eax, dword ptr [dword_1003DBA0]
		mov		edx, Screen.Width
		mov     dword ptr ds:[eax], edx // 640
		mov     eax, dword ptr[dword_1003DBB4]
		mov     dword ptr ds:[eax], ecx
		mov     ecx, [esp + 1Ch]
		mov     eax, dword ptr[dword_1003DBA4]
		mov		ecx, Screen.Height
		mov     dword ptr ds:[eax], ecx // 480
		jmp	    hookJmpAddr
	}
}

void __declspec(naked) UWindowsViewport_ResizeViewport_Hook()
{
	_asm
	{
		mov		edi, Screen.Width
		mov		[ebp + 7Ch], edi //640
		test    esi, esi
		mov		[ebp + 194h], ecx
		mov     ebx, Screen.Height
		mov		[ebp + 80h], ebx //480
		jmp	    hookJmpAddr2
	}
}

DWORD __esp;
float offset1, offset2;
DWORD Color;
void CenterHud()
{
	offset1 = *(float*)(__esp + 4);
	offset2 = *(float*)(__esp + 4 + 8);
	Color = *(DWORD*)(__esp + 0x2C);

	/*if (offset1 >= 780.0f && offset1 <= 925.0f) //898.50 912.00 916.50 910.50 906.0 904.5 780.00 792.00 787.50 915.0//925 - everything cept interaction menu
	{
		offset2 += 150.0f;
		offset1 += 150.0f;
	}*/

	offset1 += Screen.fHudOffset;
	offset2 += Screen.fHudOffset;

	if (Color == 0xFE000000)
	{
		offset2 *= 0.0f; // hiding cutscene borders
		offset1 *= 0.0f;
	}
}

void __declspec(naked) FCanvasUtil_DrawTile_Hook()
{
	_asm
	{
		mov  __esp, esp
		call CenterHud
		mov  eax, offset1
		mov  [esp+4], eax;
		mov  eax, offset2
		mov  [esp + 4 + 8], eax;
		jmp	 hookJmpAddr3
	}
}

float __ECX;
void __declspec(naked) UGameEngine_Draw_Hook()
{
	_asm
	{
		mov  ecx, [eax + 2B0h]
		mov  __ECX, ecx
	}
	__ECX *= fDynamicScreenFieldOfViewScale;

	__asm mov ecx, __ECX
	__asm	jmp	 hookJmpAddr4
}

void Init()
{
	CIniReader iniReader("");
	Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
	Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
	nForceShadowBufferSupport = iniReader.ReadInteger("MAIN", "ForceShadowBufferSupport", 0);
	nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 0);

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

	CIniReader iniWriter(UserIni);
	char szRes[50];
	sprintf(szRes, "%dx%d", Screen.Width, Screen.Height);
	iniWriter.WriteString("Engine.EPCGameOptions", "Resolution", szRes);
	_asm
	{
		push    ebp
		mov     ebp, esp
		push    0xFF
		jmp		epJump
	}
}

DWORD WINAPI Thread(LPVOID)
{
	if (bIsSteam == true)
	{
		while (*(BYTE*)0x10918F1A != 0x55)
		{
			Sleep(0);
		}
		injector::MakeJMP(0x10918F1A, Init, true);
		epJump = (DWORD)0x10918F1A + 5;
	}

	while (true)
	{
		Sleep(0);
		D3DDrv = GetModuleHandle("D3DDrv");
		WinDrv = GetModuleHandle("WinDrv");
		Engine = GetModuleHandle("Engine");
		if (D3DDrv && WinDrv && Engine)
			break;
	}

	dword_1003DBA0 = /*(DWORD)D3DDrv + 0x3DBA0*/injector::ReadMemory<DWORD>((DWORD)GetProcAddress(D3DDrv, "?SetRes@UD3DRenderDevice@@UAEHPAVUViewport@@HHH@Z") + 0x5A8 + 0x2, true);
	dword_1003DBB4 = /*(DWORD)D3DDrv + 0x3DBB4*/injector::ReadMemory<DWORD>((DWORD)GetProcAddress(D3DDrv, "?SetRes@UD3DRenderDevice@@UAEHPAVUViewport@@HHH@Z") + 0x5A8 + 0x2 + 0x6, true);
	dword_1003DBA4 = /*(DWORD)D3DDrv + 0x3DBA4*/injector::ReadMemory<DWORD>((DWORD)GetProcAddress(D3DDrv, "?SetRes@UD3DRenderDevice@@UAEHPAVUViewport@@HHH@Z") + 0x5A8 + 0x2 + 0x6 + 0xA, true);
	injector::MakeJMP(/*(DWORD)D3DDrv + 0xCE48*/(DWORD)GetProcAddress(D3DDrv, "?SetRes@UD3DRenderDevice@@UAEHPAVUViewport@@HHH@Z") + 0x5A8, UD3DRenderDevice_SetRes_Hook, true);
	hookJmpAddr = /*(DWORD)D3DDrv + 0xCE5E*/(DWORD)GetProcAddress(D3DDrv, "?SetRes@UD3DRenderDevice@@UAEHPAVUViewport@@HHH@Z") + 0x5A8 + 0x16;
	
	DWORD pfResizeViewport = injector::ReadMemory<DWORD>((DWORD)GetProcAddress(WinDrv, "?ResizeViewport@UWindowsViewport@@UAEHKHH@Z") + 0x1, true) + (DWORD)GetProcAddress(WinDrv, "?ResizeViewport@UWindowsViewport@@UAEHKHH@Z") + 5;
	injector::MakeJMP(/*(DWORD)WinDrv + 0xAB8B*/ pfResizeViewport + 0x6DB, UWindowsViewport_ResizeViewport_Hook, true);
	hookJmpAddr2 = /*(DWORD)WinDrv + 0xAB9C*/pfResizeViewport + 0x6DB + 0x11;

	DWORD pfDrawTile = injector::ReadMemory<DWORD>((DWORD)GetProcAddress(Engine, "?DrawTile@UCanvas@@UAEXPAVUMaterial@@MMMMMMMMMVFPlane@@1@Z") + 0x1, true) + (DWORD)GetProcAddress(Engine, "?DrawTile@UCanvas@@UAEXPAVUMaterial@@MMMMMMMMMVFPlane@@1@Z") + 5;
	DWORD pfDrawString = injector::ReadMemory<DWORD>((DWORD)GetProcAddress(Engine, "?DrawStringC@UCanvas@@UAEHPBGHH@Z") + 0x1, true) + (DWORD)GetProcAddress(Engine, "?DrawStringC@UCanvas@@UAEHPBGHH@Z") + 5;
	DWORD pfsub_103C9750 = injector::ReadMemory<DWORD>(pfDrawString + 0xA8 + 0x1, true) + pfDrawString + 0xA8 + 5;
	static float HUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
	injector::WriteMemory<float>(/*(DWORD)Engine + 0x1E9F78*/injector::ReadMemory<DWORD>(pfDrawTile + 0x284 + 0x2, true), HUDScaleX, true);

	injector::MakeCALL(/*(DWORD)Engine + 0xC8ECE*/pfDrawTile + 0x50E, FCanvasUtil_DrawTile_Hook, true);
	injector::MakeCALL(/*(DWORD)Engine + 0xC9B7C*/pfsub_103C9750 + 0x42C, FCanvasUtil_DrawTile_Hook, true);
	injector::MakeCALL(/*(DWORD)Engine + 0xC9DE1*/pfsub_103C9750 + 0x691, FCanvasUtil_DrawTile_Hook, true);
	hookJmpAddr3 = /*(DWORD)Engine + 0x157200*/injector::ReadMemory<DWORD>((DWORD)GetProcAddress(Engine, "?DrawTile@FCanvasUtil@@QAEXMMMMMMMMMPAVUMaterial@@VFColor@@@Z") + 0x1, true) + (DWORD)GetProcAddress(Engine, "?DrawTile@FCanvasUtil@@QAEXMMMMMMMMMPAVUMaterial@@VFColor@@@Z") + 5;

	//FMV
	injector::WriteMemory<float>(/*(DWORD)D3DDrv + 0x31860*/injector::ReadMemory<DWORD>((DWORD)GetProcAddress(D3DDrv, "?DisplayVideo@UD3DRenderDevice@@UAEXPAVUCanvas@@PAX@Z") + 0x55D, true), (1.0f / 640.0f) / ((Screen.fWidth / 640.0f)), true); //X
	injector::WriteMemory<float>(/*(DWORD)D3DDrv + 0x31864*/injector::ReadMemory<DWORD>((DWORD)GetProcAddress(D3DDrv, "?DisplayVideo@UD3DRenderDevice@@UAEXPAVUCanvas@@PAX@Z") + 0x55D + 0x12, true), (1.0f / 480.0f) / ((Screen.fHeight / 480.0f)), true); //Y

	//HUD
	Screen.fHudOffset = (Screen.fWidth - Screen.fHeight * (4.0f/3.0f)) / 2.0f;	

	//FOV
	DWORD pfDraw = injector::ReadMemory<DWORD>((DWORD)GetProcAddress(Engine, "?Draw@UGameEngine@@UAEXPAVUViewport@@HPAEPAH@Z") + 0x1, true) + (DWORD)GetProcAddress(Engine, "?Draw@UGameEngine@@UAEXPAVUViewport@@HPAEPAH@Z") + 5;
	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);
	injector::MakeJMP(/*(DWORD)Engine + 0xEC5F4*/pfDraw + 0x104, UGameEngine_Draw_Hook, true);
	hookJmpAddr4 = /*(DWORD)Engine + 0xEC5FA*/pfDraw + 0x104 + 0x6;

	//Shadows
	if (nForceShadowBufferSupport)
	{
		DWORD pfSupportsShadowBuffer = (DWORD)GetProcAddress(D3DDrv, "?SupportsShadowBuffer@UD3DRenderDevice@@UAEHXZ");
		injector::WriteMemory<unsigned short>(pfSupportsShadowBuffer + 0x10, 0xE990, true);
		injector::WriteMemory(pfSupportsShadowBuffer + 0x113, &nForceShadowBufferSupport, true);
	}
	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		DWORD fAttr = GetFileAttributes("SplinterCellUser.ini");
		if ((fAttr != INVALID_FILE_ATTRIBUTES) && !(fAttr & FILE_ATTRIBUTE_DIRECTORY))
		{
			UserIni = ".\\SplinterCellUser.ini";
		}
		else
		{
			UserIni = "..\\SplinterCellUser.ini";
		}

		hExecutableInstance = GetModuleHandle(NULL);
		IMAGE_NT_HEADERS* ntHeader = (IMAGE_NT_HEADERS*)((DWORD)hExecutableInstance + ((IMAGE_DOS_HEADER*)hExecutableInstance)->e_lfanew);
		BYTE* ep = (BYTE*)((DWORD)hExecutableInstance + ntHeader->OptionalHeader.AddressOfEntryPoint);

		if (*(BYTE*)ep == 0x53)
		{
			bIsSteam = true;
		}
		else
		{
			injector::MakeJMP(ep, Init, true);
			epJump = (DWORD)ep + 5;
		}
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
	}
	return TRUE;
}


#include "stdafx.h"
#include "stdio.h"
#include "..\includes\injector\injector.hpp"
#include "..\includes\IniReader.h"

HWND hWnd;
void ProgramRestart(LPTSTR lpszTitle);

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
} Screen;

HMODULE D3DDrv, WinDrv, Engine;
DWORD hookJmpAddr, hookJmpAddr2, hookJmpAddr3, hookJmpAddr4;
DWORD dword_1003DBA0;
DWORD dword_1003DBB4;
DWORD dword_1003DBA4;

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
float temp1 = 940.0f, temp2 = 1111.0f;
char xxx[100];
void CenterHud()
{
	offset1 = *(float*)(__esp + 4);
	offset2 = *(float*)(__esp + 4 + 8);
	Color = *(DWORD*)(__esp + 0x2C);

	if (offset1 >= 780.0f && offset1 <= 925.0f) //898.50 912.00 916.50 910.50 906.0 904.5 780.00 792.00 787.50 915.0
		//925 - everything cept interaction menu
	{
		//sprintf(xxx, "%f", offset1);
		//MessageBox(0, 0, xxx, 0);
		offset2 += 150.0f;
		offset1 += 150.0f;
	}

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

template<uintptr_t addr>
void TestHook()
{
	using func_hook = injector::function_hooker_thiscall<addr, int(int _this, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, int a10, int a11, int a12)>;
	injector::make_static_hook<func_hook>([](func_hook::func_type Test, int _this, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, int a10, int a11, int a12)
	{
	a2 += 150.1f;
	a4 += 150.1f;
		Test(_this, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
		return 1;
	});
}

DWORD WINAPI Thread(LPVOID)
{
	CIniReader iniReader("");
	Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
	Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);

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

	while (true)
	{
		Sleep(0);
		D3DDrv = GetModuleHandle("D3DDrv");
		WinDrv = GetModuleHandle("WinDrv");
		Engine = GetModuleHandle("Engine");
		if (D3DDrv && WinDrv && Engine)
			break;
	}

	injector::MakeJMP((DWORD)D3DDrv + 0xCE48, UD3DRenderDevice_SetRes_Hook, true);
	hookJmpAddr = (DWORD)D3DDrv + 0xCE5E;
	dword_1003DBA0 = (DWORD)D3DDrv + 0x3DBA0;
	dword_1003DBB4 = (DWORD)D3DDrv + 0x3DBB4;
	dword_1003DBA4 = (DWORD)D3DDrv + 0x3DBA4;

	injector::MakeJMP((DWORD)WinDrv + 0xAB8B, UWindowsViewport_ResizeViewport_Hook, true);
	hookJmpAddr2 = (DWORD)WinDrv + 0xAB9C;

	static float HUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
	injector::WriteMemory<float>((DWORD)Engine + 0x1E9F78, HUDScaleX, true);

	injector::MakeCALL((DWORD)Engine + 0xC8ECE, FCanvasUtil_DrawTile_Hook, true);
	injector::MakeCALL((DWORD)Engine + 0xC9B7C, FCanvasUtil_DrawTile_Hook, true);
	injector::MakeCALL((DWORD)Engine + 0xC9DE1, FCanvasUtil_DrawTile_Hook, true);
	hookJmpAddr3 = (DWORD)Engine + 0x157200;

	injector::WriteMemory((DWORD)GetModuleHandle("Engine") + 0xD9419, 0x0001BAE9, true); //skipping resolution reading from SplinterCellUser.ini
	injector::WriteMemory<unsigned short>((DWORD)GetModuleHandle("Engine") + 0xD9419 + 0x4, 0x9000, true); //because it causes black screen after loading save.

	//FMV
	injector::WriteMemory<float>((DWORD)D3DDrv + 0x31860, 0.0f, true); //Y
	injector::WriteMemory<float>((DWORD)D3DDrv + 0x31864, 0.0f, true); //X

	//HUD
	Screen.fHudOffset = (Screen.fWidth - Screen.fHeight * (4.0f/3.0f)) / 2.0f;	

	//FOV
	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(fScreenFieldOfViewVStd * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);
	injector::MakeJMP((DWORD)Engine + 0xEC5F4, UGameEngine_Draw_Hook, true);
	hookJmpAddr4 = (DWORD)Engine + 0xEC5FA;

	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		DWORD fAttr = GetFileAttributes(".\\SplinterCellUser.ini");
		char* UserIni;
		if ((fAttr != INVALID_FILE_ATTRIBUTES) && !(fAttr & FILE_ATTRIBUTE_DIRECTORY))
		{
			UserIni = ".\\SplinterCellUser.ini";
		}
		else
		{
			UserIni = "..\\SplinterCellUser.ini";
		}
		CIniReader iniWriter(UserIni);
		char* Res = iniWriter.ReadString("Engine.EPCGameOptions", "Resolution", "");

		if (strcmp(Res, "") != 0)
		{
			iniWriter.WriteString("Engine.EPCGameOptions", "Resolution", "");
			ProgramRestart("SC Widescreen Fix");
		}

		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
	}
	return TRUE;
}

void ProgramRestart(LPTSTR lpszTitle)
{
	// Just doing this so you can tell that is a new process
	TCHAR szBuffer[512];
	if (strlen(lpszTitle) > 500) lpszTitle[500] = 0;
	wsprintf(szBuffer, TEXT("%s - %08X"), lpszTitle, GetCurrentProcessId());

	// If they answer yes, launch the new process
	if (MessageBox(HWND_DESKTOP, TEXT("Widescreen fix detected that custom resolution is set in SplinterCellUser.ini. It has been removed to make the fix work properly, the game should be restarted for the changes to take effect. Restart now?"),
		szBuffer, MB_ICONQUESTION | MB_YESNO | MB_SETFOREGROUND | MB_APPLMODAL) == IDYES)
	{
		TCHAR szPath[MAX_PATH + 1];
		PROCESS_INFORMATION pi;
		STARTUPINFO si;

		GetStartupInfo(&si);
		GetModuleFileName(NULL, szPath, MAX_PATH);
		static LPTSTR lpszRestartMutex = TEXT("NapalmSelfRestart");
		HANDLE hRestartMutex = CreateMutex(NULL, TRUE, lpszRestartMutex);

		if (CreateProcess(szPath, GetCommandLine(), NULL, NULL,
			FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi) == 0)
		{
			MessageBox(HWND_DESKTOP, TEXT("Failed to restart program.\n"
				"Please try manually."), TEXT("Error"), MB_ICONERROR);
		}
		else {
			Sleep(1000);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
}
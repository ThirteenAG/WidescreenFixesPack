#define _SCL_SECURE_NO_WARNINGS
#include "stdafx.h"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"
#define _USE_MATH_DEFINES
#include "math.h"
#include <cstdio>
#include <windows.h>
#include <tlhelp32.h>

float height_multipl;
float height_stretch;
float width_stretch;
float fDynamicScreenFieldOfViewScale;

float aspect_ratio;
float FOV_ptr;
float hud_stretch;
float ini_fov;
int DisableGamepadInput;
int RunJoyToKey;

int* g_Width = (int *)0x829584;
int* g_Height = (int *)0x829588;
float* g_some = (float *)0x7D3450;
float* g_fov = (float *)0x715C94;

#define width (float)*g_Width
#define height (float)*g_Height
#define some (float)*g_some
#define FOV (float)*g_fov

#define DEGREE_TO_RADIAN(fAngle) \
	((fAngle)* (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) \
	((fAngle)* 180.0f / (float)M_PI)
#define SCREEN_FOV_HORIZONTAL		0.7f
#define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / (4.0f / 3.0f))))

void getDynamicScreenFieldOfView()
{
	if (ini_fov)
	{
	fDynamicScreenFieldOfViewScale = ini_fov;
	} else {
	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * aspect_ratio));
	}
}

DWORD jmpAddr;
void __declspec(naked)setScreenFieldOfView()
{
	_asm
	{
		    fdiv dword ptr ds:[0x715C94]
				fmul dword ptr ds : [fDynamicScreenFieldOfViewScale]
			fstp dword ptr[esp]
			fld dword ptr[esp + 4]
			fdiv dword ptr ds:[0x715C94]
				fmul dword ptr ds : [fDynamicScreenFieldOfViewScale]
			mov jmpAddr, 0x475C4F
			jmp jmpAddr
	}
}

bool ProcessRunning(const char* name)
{
	HANDLE SnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (SnapShot == INVALID_HANDLE_VALUE)
		return false;

	PROCESSENTRY32 procEntry;
	procEntry.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(SnapShot, &procEntry))
		return false;

	do
	{
		if (strcmp(procEntry.szExeFile, name) == 0)
			return true;
	} while (Process32Next(SnapShot, &procEntry));

	return false;
}

STARTUPINFO si;
PROCESS_INFORMATION pi;
void StartJ2K() {
	if (!ProcessRunning("JoyToKey.exe")) {
		DWORD dwExitCode = 0;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_SHOWMINNOACTIVE;
		ZeroMemory(&pi, sizeof(pi));
		CreateProcess("JoyToKey.exe", NULL, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
	}
	return;
}

DWORD WINAPI Thread(LPVOID)
{
	Sleep(10);
	CIniReader iniReader("");
	ini_fov = iniReader.ReadFloat("MAIN", "FOV", 0.0f);
	DisableGamepadInput = iniReader.ReadInteger("MAIN", "DisableGamepadInput", 0);
	RunJoyToKey = iniReader.ReadInteger("MAIN", "RunJoyToKey", 0);

	if (DisableGamepadInput)
	{
		CPatch::Nop(0x491DDE, 6);
		CPatch::Nop(0x491DE7, 6);
		CPatch::Nop(0x491DED, 10);
	}

	if (RunJoyToKey)
	{
		StartJ2K();
	}

	aspect_ratio = width / height;

		//CPatch::SetPointer(0x475BC0 + 0x89 + 0x2, &FOV_ptr);
		//CPatch::SetPointer(0x475BC0 + 0x7C + 0x2, &FOV_ptr);

		height_multipl = 640.0f / aspect_ratio; // HUD PATCH
		hud_stretch = height / ((height / height_multipl) / (some) * width);
		CPatch::SetFloat(0x7D3458, hud_stretch);

		height_stretch = 0.6999999881f * (aspect_ratio / (4.0f / 3.0f));

		CPatch::SetPointer(0x475BE2 + 0x2, &height_stretch);
		CPatch::SetPointer(0x475C09 + 0x2, &height_stretch);
		CPatch::SetPointer(0x476246 + 0x1, &height_stretch);

		width_stretch = 0.546875f;
		CPatch::SetPointer(0x475C1B + 0x2, &width_stretch);

		//Disable menu aspect ratio switch
		CPatch::Nop(0x5DAA20, 9);
		CPatch::SetChar(0x5DAA30, 0xC3u); //ret

		//if ( *(float *)&height_stretch > (double)flt_715AA8 (1.9) )
		CPatch::SetChar(0x475BF3, 0xEBu);

		//setFOV();
		getDynamicScreenFieldOfView();
		CPatch::RedirectJump(0x475C3C, setScreenFieldOfView);
	return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
	}
	
		if (reason == DLL_PROCESS_DETACH)
		{
			LPDWORD ExitCode;
			TerminateProcess(pi.hProcess, GetExitCodeProcess(pi.hProcess, ExitCode));
		}
	return TRUE;
}
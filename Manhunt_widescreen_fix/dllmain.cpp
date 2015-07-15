#include "stdafx.h"
#include "stdio.h"
#include <windows.h>
#include "..\includes\injector\injector.hpp"
#include "..\includes\IniReader.h"

HWND hWnd;
float fFOVFactor;
bool bDisableGamepadInput;
char* szCustomUserFilesDirectoryInGameDir;

struct Screen
{
	int Width;
	int Height;
	float fWidth;
	float fHeight;
	float fUnk1;
	float FieldOfView;
	float fAspectRatio;
} Screen;

DWORD jmpAddr;
void __declspec(naked)setScreenFieldOfView()
{
	_asm
	{
		    fdiv dword ptr ds:[0x715C94]
				fmul dword ptr ds : [fFOVFactor]
			fstp dword ptr[esp]
			fld dword ptr[esp + 4]
			fdiv dword ptr ds:[0x715C94]
				fmul dword ptr ds : [fFOVFactor]
			mov jmpAddr, 0x475C4F
			jmp jmpAddr
	}
}

char* __cdecl InitUserDirectories()
{
	CreateDirectory(szCustomUserFilesDirectoryInGameDir, NULL);
	return szCustomUserFilesDirectoryInGameDir;
}

void ApplyMemoryPatches()
{
	Screen.Width = *(int *)0x829584;
	Screen.Height = *(int *)0x829588;
	Screen.fUnk1 = *(float *)0x7D3450;
	Screen.FieldOfView = *(float *)0x715C94;

	//Disable menu aspect ratio switch
	injector::MakeNOP(0x5DAA20, 9, true);
	injector::WriteMemory<unsigned char>(0x5DAA30, 0xC3, true); //ret

	Screen.fWidth = static_cast<float>(Screen.Width);
	Screen.fHeight = static_cast<float>(Screen.Height);
	Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

	float fHudHorScale = static_cast<float>(Screen.fHeight / ((Screen.fHeight / (640.0f / Screen.fAspectRatio)) / (Screen.fUnk1) * Screen.fWidth));
	injector::WriteMemory(0x7D3458, fHudHorScale, true);

	static float fHudVerScale = 0.7f * (Screen.fAspectRatio / (4.0f / 3.0f));
	injector::WriteMemory(0x475BE2 + 0x2, &fHudVerScale, true);
	injector::WriteMemory(0x475C09 + 0x2, &fHudVerScale, true);
	injector::WriteMemory(0x476246 + 0x1, &fHudVerScale, true);

	injector::WriteMemory<unsigned char>(0x475BF3, 0xEB);
	static float fScreenWidthScale = 0.546875f;
	injector::WriteMemory(0x475C1B + 0x2, &fScreenWidthScale, true);

	if (fFOVFactor)
		injector::MakeJMP(0x475C3C, setScreenFieldOfView, true);
}

DWORD WINAPI Thread(LPVOID)
{
	while (*(DWORD*)0x5E2579 !=0xE44972E8)
		Sleep(0);

	CIniReader iniReader("");
	fFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 0.0f);
	bDisableGamepadInput = iniReader.ReadInteger("MAIN", "DisableGamepadInput", 0) != 0;
	szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MAIN", "CustomUserFilesDirectoryInGameDir", "");

	injector::MakeCALL(0x5E2579, ApplyMemoryPatches, true);

	if (bDisableGamepadInput)
	{
		injector::MakeNOP(0x491DDE, 6, true);
		injector::MakeNOP(0x491DE7, 6, true);
		injector::MakeNOP(0x491DED, 10, true);
	}

	if (strncmp(szCustomUserFilesDirectoryInGameDir, "0", 1) != 0)
	{
		injector::MakeCALL(0x605E0B, InitUserDirectories, true);
	}

	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
	}
	return TRUE;
}
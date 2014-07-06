#include "stdafx.h"
#include "iostream"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"

HWND hWnd;
WORD wres_x;
WORD wres_y;
DWORD jmpAddr;

float fTextAlignment;

void __declspec(naked)patch_res()
{
	_asm
	{
			MOVSX EAX, wres_x //WORD PTR DS : [ECX + 968h]
			MOV DWORD PTR DS : [EDX], EAX
			MOVSX EAX, wres_y //WORD PTR DS : [ECX + 96Ah]

			mov jmpAddr, 0x4ABE40
			jmp jmpAddr
	}
}

void __declspec(naked)patch_res_steam()
{
	_asm
	{
			MOVSX EAX, wres_x //WORD PTR DS : [ECX + 968h]
			MOV DWORD PTR DS : [EDX], EAX
			MOVSX EAX, wres_y //WORD PTR DS : [ECX + 96Ah]

			mov jmpAddr, 0x4ABF00
			jmp jmpAddr
	}
}

DWORD WINAPI Init(LPVOID)
{
	CIniReader iniReader("");
	int res_x = iniReader.ReadInteger("MAIN", "X", 0);
	int res_y = iniReader.ReadInteger("MAIN", "Y", 0);
	//int HUD_PATCH = iniReader.ReadInteger("MAIN", "HUD_PATCH", 0);

	if (!res_x || !res_y) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		res_x = info.rcMonitor.right - info.rcMonitor.left;
		res_y = info.rcMonitor.bottom - info.rcMonitor.top;
	}
	wres_x = static_cast<WORD>(res_x);
	wres_y = static_cast<WORD>(res_y);

	if (*(DWORD *)0x4ABE30 == 0x6881BF0F)
	{
		CPatch::SetFloat(0x5755E1 + 0x4, ((1.0f / (480.0f * (static_cast<float>(res_x) / static_cast<float>(res_y)))) * 2.0f));
		fTextAlignment = (1.0f / (480.0f * (static_cast<float>(res_x) / static_cast<float>(res_y))) * 2.0f) * ((480.0f * (4.0f / 3.0f)) / 2.0f);
		CPatch::SetPointer(0x575671 + 0x2, &fTextAlignment);

		CPatch::RedirectJump(0x4ABE30, patch_res);
	} else 
		{
		while (true)
		{
			Sleep(0);
			if (*(DWORD*)0x401000 == 0x245C8B53) break; //steam
		}
		CPatch::SetFloat(0x575761 + 0x4, ((1.0f / (480.0f * (static_cast<float>(res_x) / static_cast<float>(res_y)))) * 2.0f));
		fTextAlignment = (1.0f / (480.0f * (static_cast<float>(res_x) / static_cast<float>(res_y))) * 2.0f) * ((480.0f * (4.0f / 3.0f)) / 2.0f);
		CPatch::SetPointer(0x5757F1 + 0x2, &fTextAlignment);

		CPatch::RedirectJump(0x4ABEF0, patch_res_steam);
		}
	return 0;
}





BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
	}
	return TRUE;
}


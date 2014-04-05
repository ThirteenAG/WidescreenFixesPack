#include "stdafx.h"
#include "stdio.h"
#include <windows.h>
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"

HWND hWnd;

int hud_patch;
int res_x;
int res_y;
DWORD WINAPI hud_handler(LPVOID);

void Init()
{
	CIniReader iniReader("nfsu2_res.ini");
	res_x = iniReader.ReadInteger("MAIN", "X", 0);
	res_y = iniReader.ReadInteger("MAIN", "Y", 0);
	hud_patch = iniReader.ReadInteger("MAIN", "HUD_PATCH", 0);

	if (!res_x || !res_y) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		res_x = info.rcMonitor.right - info.rcMonitor.left;
		res_y = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	//game
	CPatch::SetInt(0x5BF62F, res_x);
	CPatch::SetInt(0x5BF64C, res_x);
	CPatch::SetInt(0x5BF674, res_x);
	CPatch::SetInt(0x5BF691, res_x);

	CPatch::SetInt(0x5BF635, res_y);
	CPatch::SetInt(0x5BF65D, res_y);
	CPatch::SetInt(0x5BF67A, res_y);
	CPatch::SetInt(0x5BF6A2, res_y);


		CPatch::SetInt(0x7FF77C, res_x);
		CPatch::SetInt(0x86E77C, res_x);
		CPatch::SetInt(0x86E7B4, res_x);
		CPatch::SetInt(0x86F870, res_x);
		CPatch::SetInt(0x870980, res_x);

		CPatch::SetInt(0x7FF780, res_y);
		CPatch::SetInt(0x86E780, res_y);
		CPatch::SetInt(0x86E7B8, res_y);
		CPatch::SetInt(0x86F874, res_y);
		CPatch::SetInt(0x870984, res_y);

		//HUD
		if (hud_patch)
		{
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&hud_handler, NULL, 0, NULL);
		}
}


DWORD WINAPI hud_handler(LPVOID)
{
	float hud_multiplier_x = 1.0f / (float)res_x * 2.0f;
	float hud_multiplier_y = 1.0f / (float)res_y * 2.0f;

	float orig_multiplier_x = 1.0f / 640.0f * 2.0f;
	float orig_multiplier_y = 1.0f / 480.0f * 2.0f;

	while (true)
	{
		Sleep(0);
		if ((char)*(DWORD*)0x816204 == 0) 
		{
			CPatch::SetFloat(0x79AC10, hud_multiplier_x);
			CPatch::SetFloat(0x79AC14, hud_multiplier_y);
		}
		else 
		{
			CPatch::SetFloat(0x79AC10, orig_multiplier_x);
			CPatch::SetFloat(0x79AC14, orig_multiplier_y);
		}
	}
	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}


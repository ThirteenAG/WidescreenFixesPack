#include "stdafx.h"
#include "stdio.h"
#include <windows.h>
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"

HWND hWnd;

int hud_patch;
int res_x;
int res_y;
float FOV_hor, FOV_ver, HUD_scale;
DWORD WINAPI hud_handler(LPVOID);

void Init()
{
	CIniReader iniReader("nfsu_res.ini");
	res_x = iniReader.ReadInteger("MAIN", "X", 0);
	res_y = iniReader.ReadInteger("MAIN", "Y", 0);
	hud_patch = iniReader.ReadInteger("MAIN", "HUD_PATCH", 0);
	FOV_hor = iniReader.ReadFloat("MAIN", "FOV_hor", 1.1f);
	FOV_ver = iniReader.ReadFloat("MAIN", "FOV_ver", 0.90909088f);
	HUD_scale = iniReader.ReadFloat("MAIN", "HUD_scale", 1.0f);

	if (!res_x || !res_y) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		res_x = info.rcMonitor.right - info.rcMonitor.left;
		res_y = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	//menu
	CPatch::SetInt(0x408A25, res_x);
	CPatch::SetInt(0x408A2A, res_y);

	CPatch::SetInt(0x4494C5, res_x);
	CPatch::SetInt(0x4494CA, res_y);

	//game
	CPatch::SetInt(0x408783, res_x);
	CPatch::SetInt(0x408796, res_x);
	CPatch::SetInt(0x4087AF, res_x);
	CPatch::SetInt(0x4087C8, res_x);
	CPatch::SetInt(0x4087E1, res_x);
	//CPatch::SetInt(0x4087FA, res_x);

	CPatch::SetInt(0x408788, res_y);
	CPatch::SetInt(0x40879B, res_y);
	CPatch::SetInt(0x4087B4, res_y);
	CPatch::SetInt(0x4087CD, res_y);
	CPatch::SetInt(0x4087E6, res_y);
	//CPatch::SetInt(0x4087FF, res_y);

	if (hud_patch)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&hud_handler, NULL, 0, NULL);
	}

	if (FOV_hor && FOV_ver)
	{
		CPatch::SetFloat(0x6B7C70, FOV_hor);
		CPatch::SetFloat(0x6B7C74, FOV_hor);
		CPatch::SetFloat(0x6B7C6C, FOV_ver);
	}

}


DWORD WINAPI hud_handler(LPVOID)
{
	float hud_multiplier_x = (1.0f / (float)res_x * 2.0f) * HUD_scale;
	float hud_multiplier_y = (1.0f / (float)res_y * 2.0f) * HUD_scale;

	float orig_multiplier_x = 1.0f / 640.0f * 2.0f;
	float orig_multiplier_y = 1.0f / 480.0f * 2.0f;

	while (true)
	{
		Sleep(0);
		/*!!!!!!!!/*if (!((int)*(DWORD*)0x701038 == res_y)) {
			patch(0x701034, &res_x, 4);
			patch(0x71A96C, &res_x, 4);
			patch(0x71AA44, &res_x, 4);
			patch(0x71AA60, &res_x, 4);
			patch(0x71AA98, &res_x, 4);

			patch(0x701038, &res_y, 4);
			patch(0x71A970, &res_y, 4);
			patch(0x71AA48, &res_y, 4);
			patch(0x71AA64, &res_y, 4);
			patch(0x71AA9C, &res_y, 4);
		}*//**/

		/*changePointer(0x408410+0x103, &res_x, 2);
		changePointer(0x408830+0x115, &res_x, 2);
		changePointer(0x40A890+0xA6, &res_x, 2);
		changePointer(0x40AA99, &res_x, 1);
		changePointer(0x40CB80, &res_x, 1);
		changePointer(0x40CB80+0xF7, &res_x, 2);
		changePointer(0x40DCB0+0x2B, &res_x, 1);
		changePointer(0x42F020+0x86, &res_x, 2);




		changePointer(0x408410+0x10A, &res_y, 2);
		changePointer(0x408830+0x11B, &res_y, 2);
		changePointer(0x40A890+0x9B, &res_y, 2);
		changePointer(0x40AA20+0x7E, &res_y, 2);
		changePointer(0x40CB80+0x11, &res_y, 1);
		changePointer(0x40CB80+0xEC, &res_y, 2);
		changePointer(0x42F020+0x7E, &res_y, 2); */
		//Sleep(1000);
		//nop(0x40872D, 6);
		//nop(0x4086BD, 5);
		//nop(0x408727, 6);
		//nop(0x40AAA4, 3);
		//nop(0x40856D, 3);
		//nop(0x40AAA4, 3);
		//nop(0x40AAA4, 3);

		//nop(0x408754, 6);
		//nop(0x4086BD, 5);
		//nop(0x408733, 6);
		//nop(0x40AAAC, 3);
		//nop(0x408570, 3);
		//nop(0x40AAAC, 3);
		//nop(0x40AAAC, 3);
		//HUD

			if ((unsigned int)*(DWORD*)0x713D00 == 0) 
			{
				CPatch::SetFloat(0x6CC914, hud_multiplier_x);
				CPatch::SetFloat(0x6CCBA4, hud_multiplier_y);
			}
			else 
			{
				CPatch::SetFloat(0x6CC914, orig_multiplier_x);
				CPatch::SetFloat(0x6CCBA4, orig_multiplier_y);
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


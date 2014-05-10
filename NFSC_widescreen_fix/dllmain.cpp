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
	CIniReader iniReader("nfsc_res.ini");
	res_x = iniReader.ReadInteger("MAIN", "X", 0);
	res_y = iniReader.ReadInteger("MAIN", "Y", 0);
	hud_patch = iniReader.ReadInteger("MAIN", "HUD_PATCH", 0);
	FOV_hor = iniReader.ReadFloat("MAIN", "FOV_hor", 1.15f);
	FOV_ver = iniReader.ReadFloat("MAIN", "FOV_ver", 0.5f);
	HUD_scale = iniReader.ReadFloat("MAIN", "HUD_scale", 1.0f);

	if (!res_x || !res_y) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		res_x = info.rcMonitor.right - info.rcMonitor.left;
		res_y = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	//game
	CPatch::SetInt(0x712ADD + 0x2, res_x);
	CPatch::SetInt(0x712AE3 + 0x2, res_y);
	CPatch::SetInt(0x712AF4 + 0x2, res_x);
	CPatch::SetInt(0x712AFA + 0x2, res_y);
	CPatch::SetInt(0x712B0B + 0x2, res_x);
	CPatch::SetInt(0x712B11 + 0x2, res_y);
	CPatch::SetInt(0x712B22 + 0x2, res_x);
	CPatch::SetInt(0x712B28 + 0x2, res_y);
	CPatch::SetInt(0x712B39 + 0x2, res_x);
	CPatch::SetInt(0x712B3F + 0x2, res_y);
	CPatch::SetInt(0x712B50 + 0x2, res_x);
	CPatch::SetInt(0x712B56 + 0x2, res_y);
	CPatch::SetInt(0x712BBD + 0x2, res_x);
	CPatch::SetInt(0x712BC3 + 0x2, res_y);
	CPatch::SetInt(0x0712BD4 + 0x2, res_x);
	CPatch::SetInt(0x0712BDA + 0x2, res_y);
	CPatch::SetInt(0x0712BEB + 0x2, res_x);
	CPatch::SetInt(0x0712BF1 + 0x2, res_y);
	CPatch::SetInt(0x0712C02 + 0x2, res_x);
	CPatch::SetInt(0x0712C08 + 0x2, res_y);
	CPatch::SetInt(0x0712C19 + 0x2, res_x);
	CPatch::SetInt(0x0712C1F + 0x2, res_y);
	CPatch::SetInt(0x0712C30 + 0x2, res_x);
	CPatch::SetInt(0x0712C36 + 0x2, res_y);


		CPatch::SetInt(0xA63F80, res_x);
		CPatch::SetInt(0xAB04E4, res_x);
		CPatch::SetInt(0xAB0500, res_x);
		CPatch::SetInt(0xAB0538, res_x);
		CPatch::SetInt(0xAB05A8, res_x);
		CPatch::SetInt(0xAB06FC, res_x);
		CPatch::SetInt(0xAB0AC8, res_x);

		CPatch::SetInt(0xA63F84, res_y);
		CPatch::SetInt(0xAB04E8, res_y);
		CPatch::SetInt(0xAB0504, res_y);
		CPatch::SetInt(0xAB053C, res_y);
		CPatch::SetInt(0xAB05AC, res_y);
		CPatch::SetInt(0xAB0700, res_y);
		CPatch::SetInt(0xAB0ACC, res_y);

		CPatch::SetFloat(0x9E9B68, (float)res_x);
		CPatch::SetFloat(0x9E8F84, (float)res_y);

		//HUD
		if (hud_patch)
		{
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&hud_handler, NULL, 0, NULL);
		}

		if (FOV_hor && FOV_ver)
		{
			CPatch::SetPointer(0x71B883 + 2, &FOV_hor);
			CPatch::SetPointer(0x71B8AE + 2, &FOV_ver);
			CPatch::SetPointer(0x71B8EC + 2, &FOV_ver);
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
			if (!((unsigned int)*(DWORD*)0xA8F2DD == 0)) 
			{
				CPatch::SetFloat(0x9E8F8C, hud_multiplier_x);
				CPatch::SetFloat(0x9E8F88, hud_multiplier_y);
			}
			else 
			{
				CPatch::SetFloat(0x9E8F8C, orig_multiplier_x);
				CPatch::SetFloat(0x9E8F88, orig_multiplier_y);
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


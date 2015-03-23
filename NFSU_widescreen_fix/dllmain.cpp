#include "stdafx.h"
#include "stdio.h"
#include <windows.h>
#include "..\includes\injector\injector.hpp"
#include "..\includes\IniReader.h"

HWND hWnd;

bool HudFix;
bool DisableCutsceneBorders;
int ResX;
int ResY;
float horFOV, verFOV, HUDscale;
DWORD WINAPI hud_handler(LPVOID);

void Init()
{
	CIniReader iniReader("nfsu_res.ini");
	ResX = iniReader.ReadInteger("MAIN", "ResX", 0);
	ResY = iniReader.ReadInteger("MAIN", "ResY", 0);
	HudFix = iniReader.ReadInteger("MAIN", "HudFix", 1) == 1;
	horFOV = iniReader.ReadFloat("MAIN", "horFOV", 0.0f);
	verFOV = iniReader.ReadFloat("MAIN", "verFOV", 0.0f);
	HUDscale = iniReader.ReadFloat("MAIN", "HUDscale", 1.0f);
	DisableCutsceneBorders = iniReader.ReadInteger("MAIN", "DisableCutsceneBorders", 1) == 1;

	if (!HUDscale) { HUDscale = 1.0f; }

	if (!ResX || !ResY) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		ResX = info.rcMonitor.right - info.rcMonitor.left;
		ResY = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	//menu
	injector::WriteMemory(0x408A25, ResX, true);
	injector::WriteMemory(0x408A2A, ResY, true);

	injector::WriteMemory(0x4494C5, ResX, true);
	injector::WriteMemory(0x4494CA, ResY, true);

	//game
	injector::WriteMemory(0x408783, ResX, true);
	injector::WriteMemory(0x408796, ResX, true);
	injector::WriteMemory(0x4087AF, ResX, true);
	injector::WriteMemory(0x4087C8, ResX, true);
	injector::WriteMemory(0x4087E1, ResX, true);
	//injector::WriteMemory(0x4087FA, ResX, true);

	injector::WriteMemory(0x408788, ResY, true);
	injector::WriteMemory(0x40879B, ResY, true);
	injector::WriteMemory(0x4087B4, ResY, true);
	injector::WriteMemory(0x4087CD, ResY, true);
	injector::WriteMemory(0x4087E6, ResY, true);
	//injector::WriteMemory(0x4087FF, ResY, true);

		//HUD
		if (HudFix)
		{
			//CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&hud_handler, NULL, 0, NULL);

			float hud_multiplier_x = (1.0f / ResX * (ResY / 480.0f)) * 2.0f;
			float hud_position_x = 640.0f / (640.0f * hud_multiplier_x);

			injector::WriteMemory<float>(0x6CC914, hud_multiplier_x, true);
			//CPatch::SetFloat(0x79AC14, hud_multiplier_y);

			injector::WriteMemory<float>(0x400000 + 0xF1E2D, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0xF20A3, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0xF232F, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0xF26E5, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0xF28AF, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0xF3147, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x14646B, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x2CC910, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x2EADCC, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x2EFC48, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x379CC0, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x379E20, hud_position_x, true);


			//injector::WriteMemory<float>(0x797D58, hud_position_x * 2.0f);
		}

		if (horFOV && verFOV)
		{
			injector::WriteMemory<float>(0x6B7C70, horFOV, true);
			injector::WriteMemory<float>(0x6B7C74, horFOV, true);
			injector::WriteMemory<float>(0x6B7C6C, verFOV, true);
		}

}


DWORD WINAPI hud_handler(LPVOID)
{
	float hud_multiplier_x = (1.0f / (float)ResX * 2.0f) * HUDscale;
	float hud_multiplier_y = (1.0f / (float)ResY * 2.0f) * HUDscale;

	float orig_multiplier_x = 1.0f / 640.0f * 2.0f;
	float orig_multiplier_y = 1.0f / 480.0f * 2.0f;

	while (true)
	{
		Sleep(0);
		if ((unsigned int)*(DWORD*)0x816204 == 0) 
		{
			injector::WriteMemory<float>(0x79AC10, hud_multiplier_x);
			injector::WriteMemory<float>(0x79AC14, hud_multiplier_y);
		}
		else 
		{
			injector::WriteMemory<float>(0x79AC10, orig_multiplier_x);
			injector::WriteMemory<float>(0x79AC14, orig_multiplier_y);
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


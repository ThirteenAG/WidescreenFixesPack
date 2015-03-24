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
	CIniReader iniReader("nfsu2_res.ini");
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
	if (*(DWORD*)0x5BF62F == 640u)
	{
		//game
		injector::WriteMemory(0x5BF62F, ResX, true);
		injector::WriteMemory(0x5BF646, ResX, true);
		injector::WriteMemory(0x5BF65D, ResX, true);
		injector::WriteMemory(0x5BF674, ResX, true);
		injector::WriteMemory(0x5BF68B, ResX, true);
		injector::WriteMemory(0x5BF6A2, ResX, true);

		injector::WriteMemory(0x5BF635, ResY, true);
		injector::WriteMemory(0x5BF64C, ResY, true);
		injector::WriteMemory(0x5BF663, ResY, true);
		injector::WriteMemory(0x5BF67A, ResY, true);
		injector::WriteMemory(0x5BF691, ResY, true);
		injector::WriteMemory(0x5BF6A8, ResY, true);


		injector::WriteMemory(0x7FF77C, ResX, true);
		injector::WriteMemory(0x86E77C, ResX, true);
		injector::WriteMemory(0x86E7B4, ResX, true);
		injector::WriteMemory(0x86F870, ResX, true);
		injector::WriteMemory(0x870980, ResX, true);

		injector::WriteMemory(0x7FF780, ResY, true);
		injector::WriteMemory(0x86E780, ResY, true);
		injector::WriteMemory(0x86E7B8, ResY, true);
		injector::WriteMemory(0x86F874, ResY, true);
		injector::WriteMemory(0x870984, ResY, true);

		//HUD
		if (HudFix)
		{
			//CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&hud_handler, NULL, 0, NULL);

			float hud_multiplier_x = (1.0f / ResX * (ResY / 480.0f)) * 2.0f;
			float hud_position_x = 640.0f / (640.0f * hud_multiplier_x);

			injector::WriteMemory<float>(0x79AC10, hud_multiplier_x, true);
			//CPatch::SetFloat(0x79AC14, hud_multiplier_y);

			injector::WriteMemory<float>(0x51B3CB, hud_position_x, true);
			injector::WriteMemory<float>(0x5368C8, hud_position_x, true);
			injector::WriteMemory<float>(0x536A99, hud_position_x, true);
			injector::WriteMemory<float>(0x536CC9, hud_position_x, true);
			injector::WriteMemory<float>(0x537011, hud_position_x, true);
			injector::WriteMemory<float>(0x48B640, hud_position_x, true);
			injector::WriteMemory<float>(0x50B4F5, hud_position_x, true);
			injector::WriteMemory<float>(0x797D50, hud_position_x, true);

			injector::WriteMemory<float>(0x797D58, hud_position_x * 2.0f, true);

			injector::WriteMemory<float>(0x5CBEF5, (float)ResX, true); // borders
			injector::WriteMemory<float>(0x5CBF05, (float)ResX, true);
			injector::WriteMemory<float>(0x5CBE89, (float)ResX, true);
			injector::WriteMemory<float>(0x5CBEA1, (float)ResX, true);
		}

		if (DisableCutsceneBorders)
		{
			injector::WriteMemory<float>(0x5CBEF5, 0.0f, true); // borders
			injector::WriteMemory<float>(0x5CBF05, 0.0f, true);
			injector::WriteMemory<float>(0x5CBE89, 0.0f, true);
			injector::WriteMemory<float>(0x5CBEA1, 0.0f, true);
		}

		if (horFOV && verFOV)
		{
			injector::WriteMemory<float>(0x7A27DC, horFOV, true);
			injector::WriteMemory<float>(0x7A27E0, horFOV, true);
			injector::WriteMemory<float>(0x7A27D8, verFOV, true);
		}
	}
	else
	{
		//game
		injector::WriteMemory(0x5BF62F - 0x400, ResX, true);
		injector::WriteMemory(0x5BF646 - 0x400, ResX, true);
		injector::WriteMemory(0x5BF65D - 0x400, ResX, true);
		injector::WriteMemory(0x5BF674 - 0x400, ResX, true);
		injector::WriteMemory(0x5BF68B - 0x400, ResX, true);
		injector::WriteMemory(0x5BF6A2 - 0x400, ResX, true);
									   
		injector::WriteMemory(0x5BF635 - 0x400, ResY, true);
		injector::WriteMemory(0x5BF64C - 0x400, ResY, true);
		injector::WriteMemory(0x5BF663 - 0x400, ResY, true);
		injector::WriteMemory(0x5BF67A - 0x400, ResY, true);
		injector::WriteMemory(0x5BF691 - 0x400, ResY, true);
		injector::WriteMemory(0x5BF6A8 - 0x400, ResY, true);


		injector::WriteMemory(0x7FF77C, ResX, true);
		injector::WriteMemory(0x86E77C, ResX, true);
		injector::WriteMemory(0x86E7B4, ResX, true);
		injector::WriteMemory(0x86F870, ResX, true);
		injector::WriteMemory(0x870980, ResX, true);

		injector::WriteMemory(0x7FF780, ResY, true);
		injector::WriteMemory(0x86E780, ResY, true);
		injector::WriteMemory(0x86E7B8, ResY, true);
		injector::WriteMemory(0x86F874, ResY, true);
		injector::WriteMemory(0x870984, ResY, true);

		//HUD
		if (HudFix)
		{
			//CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&hud_handler, NULL, 0, NULL);

			float hud_multiplier_x = (1.0f / ResX * (ResY / 480.0f)) * 2.0f;
			float hud_position_x = 640.0f / (640.0f * hud_multiplier_x);

			injector::WriteMemory<float>(0x79AC10 + 0x4, hud_multiplier_x, true);
			//CPatch::SetFloat(0x79AC14, hud_multiplier_y);

			injector::WriteMemory<float>(0x51B3CB - 0x520, hud_position_x, true);
			injector::WriteMemory<float>(0x5368C8 - 0x460, hud_position_x, true);
			injector::WriteMemory<float>(0x536A99 - 0x460, hud_position_x, true);
			injector::WriteMemory<float>(0x536CC9 - 0x460, hud_position_x, true);
			injector::WriteMemory<float>(0x537011 - 0x460, hud_position_x, true);
			injector::WriteMemory<float>(0x48B640 - 0xA0, hud_position_x, true);
			injector::WriteMemory<float>(0x50B4F5 - 0x5C0, hud_position_x, true);
			injector::WriteMemory<float>(0x797D50 + 0x1C, hud_position_x, true);

			injector::WriteMemory<float>(0x797D58 + 0x1C, hud_position_x * 2.0f, true);

			injector::WriteMemory<float>(0x5CBEF5 - 0x400, (float)ResX, true); // borders
			injector::WriteMemory<float>(0x5CBF05 - 0x400, (float)ResX, true);
			injector::WriteMemory<float>(0x5CBE89 - 0x400, (float)ResX, true);
			injector::WriteMemory<float>(0x5CBEA1 - 0x400, (float)ResX, true);
		}

		if (DisableCutsceneBorders)
		{
			injector::WriteMemory<float>(0x5CBEF5 - 0x400, 0.0f, true); // borders
			injector::WriteMemory<float>(0x5CBF05 - 0x400, 0.0f, true);
			injector::WriteMemory<float>(0x5CBE89 - 0x400, 0.0f, true);
			injector::WriteMemory<float>(0x5CBEA1 - 0x400, 0.0f, true);
		}

		if (horFOV && verFOV)
		{
			injector::WriteMemory<float>(0x7A27DC + 0x8, horFOV, true);
			injector::WriteMemory<float>(0x7A27E0 + 0x8, horFOV, true);
			injector::WriteMemory<float>(0x7A27D8 + 0x8, verFOV, true);
		}
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


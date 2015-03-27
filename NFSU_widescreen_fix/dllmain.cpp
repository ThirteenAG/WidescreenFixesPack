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
float horFOV, verFOV;
bool bHudMode;
DWORD WINAPI HudHandler(LPVOID);
float hud_multiplier_x, hud_multiplier_y;
float hud_position_x;
float MinimapPosX, MinimapPosY;

void Init()
{
	CIniReader iniReader("nfsu_res.ini");
	ResX = iniReader.ReadInteger("MAIN", "ResX", 0);
	ResY = iniReader.ReadInteger("MAIN", "ResY", 0);
	HudFix = iniReader.ReadInteger("MAIN", "HudFix", 1) == 1;
	horFOV = iniReader.ReadFloat("MAIN", "horFOV", 0.0f);
	verFOV = iniReader.ReadFloat("MAIN", "verFOV", 0.0f);
	bHudMode = iniReader.ReadInteger("MAIN", "HudMode", 1) == 1;
	DisableCutsceneBorders = iniReader.ReadInteger("MAIN", "DisableCutsceneBorders", 1) == 1;	

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
			hud_multiplier_x = (1.0f / ResX * (ResY / 480.0f)) * 2.0f;
			hud_multiplier_y = *(float*)0x6CCBA4;
			hud_position_x = 640.0f / (640.0f * hud_multiplier_x);

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

			//minimap position
			MinimapPosX = hud_position_x;
			MinimapPosY = 240.0f;
			injector::WriteMemory(0x58E5BD + 0x2, &MinimapPosX, true);
			injector::WriteMemory(0x58E5C6 + 0x2, &MinimapPosY, true);

			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&HudHandler, NULL, 0, NULL);
		}

		if (horFOV && verFOV)
		{
			injector::WriteMemory<float>(0x6B7C70, horFOV, true);
			injector::WriteMemory<float>(0x6B7C74, horFOV, true);
			injector::WriteMemory<float>(0x6B7C6C, verFOV, true);
		}
		else
		{
			//16:9 excluisve fov hack
			if ((float)ResX / (float)ResY == 16.0f/9.0f)
			{
				injector::WriteMemory<float>(0x40DE5C, (1.0f * ((float)ResX / (float)ResY)) / (4.0f / 3.0f), true);
				injector::WriteMemory<float>(0x6B7C6C, 0.90909088f / 1.05f, true);
			}
		}

}


DWORD WINAPI HudHandler(LPVOID)
{
	CIniReader iniReader("nfsu_res.ini");
	while (true)
	{
		Sleep(0);

		if ((GetAsyncKeyState(VK_F2) & 1))
		{
			bHudMode = !bHudMode;
			iniReader.WriteInteger("MAIN", "HudMode", bHudMode);
			while ((GetAsyncKeyState(VK_F2) & 0x8000) > 0) { Sleep(0); }
		}

		if (!bHudMode)
		{
			Sleep(1);
			if (*(float*)0x4F1E2D != hud_position_x)
			{
				injector::WriteMemory<float>(0x6CC914, hud_multiplier_x, true);
				injector::WriteMemory<float>(0x6CCBA4, hud_multiplier_y, true);

				injector::WriteMemory<float>(0x400000 + 0xF1E2D, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0xF20A3, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0xF232F, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0xF26E5, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0xF28AF, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0xF3147, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0x14646B, hud_position_x, true);
				MinimapPosX = hud_position_x;
				MinimapPosY = 240.0f;
			}
		}
		else
		{
			Sleep(1);
			if (*(unsigned char*)0x7041C4 != 1)
			{
				if (*(float*)0x4F1E2D == hud_position_x)
				{
					injector::WriteMemory<float>(0x6CC914, hud_multiplier_x / 1.7f, true);
					injector::WriteMemory<float>(0x6CCBA4, hud_multiplier_y / 1.7f, true);

					injector::WriteMemory<float>(0x400000 + 0xF1E2D, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0xF20A3, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0xF232F, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0xF26E5, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0xF28AF, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0xF3147, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 +0x14646B, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					MinimapPosX = (hud_position_x * 2.0f) * 1.7f * 1.1f;
					MinimapPosY = 730.0f;
				}
			}
			else
			{
				if (*(float*)0x4F1E2D != hud_position_x)
				{
					injector::WriteMemory<float>(0x6CC914, hud_multiplier_x, true);
					injector::WriteMemory<float>(0x6CCBA4, hud_multiplier_y, true);

					injector::WriteMemory<float>(0x400000 + 0xF1E2D, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0xF20A3, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0xF232F, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0xF26E5, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0xF28AF, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0xF3147, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0x14646B, hud_position_x, true);
					MinimapPosX = hud_position_x;
					MinimapPosY = 240.0f;
				}
			}
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


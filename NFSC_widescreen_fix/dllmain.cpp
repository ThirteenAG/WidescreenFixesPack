#include "stdafx.h"
#include "stdio.h"
#include <windows.h>
#include "..\includes\injector\injector.hpp"
#include "..\includes\IniReader.h"

HWND hWnd;

bool HudFix;
int ResX;
int ResY;
float horFOV, verFOV;
bool bHudMode;
DWORD WINAPI HudHandler(LPVOID);
float hud_multiplier_x, hud_multiplier_y;
float hud_position_x;
float MinimapPosX, MinimapPosY;
float verFovCorrectionFactor;

void Init()
{
	CIniReader iniReader("nfsc_res.ini");
	ResX = iniReader.ReadInteger("MAIN", "ResX", 0);
	ResY = iniReader.ReadInteger("MAIN", "ResY", 0);
	HudFix = iniReader.ReadInteger("MAIN", "HudFix", 1) == 1;
	verFovCorrectionFactor = iniReader.ReadFloat("MAIN", "verFovCorrectionFactor", 0.03f);
	bHudMode = iniReader.ReadInteger("MAIN", "HudMode", 1) == 1;
	bool bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) == 1;
	bool bFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1) == 1;

	if (!ResX || !ResY) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		ResX = info.rcMonitor.right - info.rcMonitor.left;
		ResY = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	if (*(DWORD*)0x712ADF == 640u)
	{
		//game
		injector::WriteMemory(0x712ADD + 0x2, ResX, true);
		injector::WriteMemory(0x712AE3 + 0x2, ResY, true);
		injector::WriteMemory(0x712AF4 + 0x2, ResX, true);
		injector::WriteMemory(0x712AFA + 0x2, ResY, true);
		injector::WriteMemory(0x712B0B + 0x2, ResX, true);
		injector::WriteMemory(0x712B11 + 0x2, ResY, true);
		injector::WriteMemory(0x712B22 + 0x2, ResX, true);
		injector::WriteMemory(0x712B28 + 0x2, ResY, true);
		injector::WriteMemory(0x712B39 + 0x2, ResX, true);
		injector::WriteMemory(0x712B3F + 0x2, ResY, true);
		injector::WriteMemory(0x712B50 + 0x2, ResX, true);
		injector::WriteMemory(0x712B56 + 0x2, ResY, true);
		injector::WriteMemory(0x712BBD + 0x2, ResX, true);
		injector::WriteMemory(0x712BC3 + 0x2, ResY, true);
		injector::WriteMemory(0x0712BD4 + 0x2, ResX, true);
		injector::WriteMemory(0x0712BDA + 0x2, ResY, true);
		injector::WriteMemory(0x0712BEB + 0x2, ResX, true);
		injector::WriteMemory(0x0712BF1 + 0x2, ResY, true);
		injector::WriteMemory(0x0712C02 + 0x2, ResX, true);
		injector::WriteMemory(0x0712C08 + 0x2, ResY, true);
		injector::WriteMemory(0x0712C19 + 0x2, ResX, true);
		injector::WriteMemory(0x0712C1F + 0x2, ResY, true);
		injector::WriteMemory(0x0712C30 + 0x2, ResX, true);
		injector::WriteMemory(0x0712C36 + 0x2, ResY, true);

		injector::WriteMemory(0xA63F80, ResX, true);
		injector::WriteMemory(0xAB04E4, ResX, true);
		injector::WriteMemory(0xAB0500, ResX, true);
		injector::WriteMemory(0xAB0538, ResX, true);
		injector::WriteMemory(0xAB05A8, ResX, true);
		injector::WriteMemory(0xAB06FC, ResX, true);
		injector::WriteMemory(0xAB0AC8, ResX, true);

		injector::WriteMemory(0xA63F84, ResY, true);
		injector::WriteMemory(0xAB04E8, ResY, true);
		injector::WriteMemory(0xAB0504, ResY, true);
		injector::WriteMemory(0xAB053C, ResY, true);
		injector::WriteMemory(0xAB05AC, ResY, true);
		injector::WriteMemory(0xAB0700, ResY, true);
		injector::WriteMemory(0xAB0ACC, ResY, true);

		injector::WriteMemory<float>(0x9E9B68, (float)ResX, true);
		injector::WriteMemory<float>(0x9E8F84, (float)ResY, true);

		//HUD
		if (HudFix)
		{
			hud_multiplier_x = (1.0f / ResX * (ResY / 480.0f)) * 2.0f;
			hud_multiplier_y = *(float*)0x9E8F88;
			hud_position_x = 640.0f / (640.0f * hud_multiplier_x);

			injector::WriteMemory<float>(0x9E8F8C, hud_multiplier_x, true);
			//injector::WriteMemory<float>(0x9E8F88, hud_multiplier_y);

			injector::WriteMemory<float>(0x400000 + 0x198DC0, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x198FB7, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x199416, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x1996AE, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x1A18BE, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x1D2B46, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x6604AC, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x5C778C, hud_position_x, true);

			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&HudHandler, NULL, 0, NULL);
		}

		if (verFovCorrectionFactor)
		{
			injector::WriteMemory(0x71B8DA + 2, &horFOV, true);
			injector::WriteMemory(0x71B923 + 2, &verFOV, true);

			horFOV = 1.0f / ((1.0f * ((float)ResX / (float)ResY)) / (4.0f / 3.0f));
			verFOV = horFOV - verFovCorrectionFactor;
		}

		if (bHudWidescreenMode) // thanks to dyons
		{
			injector::WriteMemory<char>(0x5DC508, 0x94, true);
			injector::WriteMemory<char>(0x5D52B3, 0x94, true);
			injector::WriteMemory<char>(0x5B6BAE, 0x75, true);
			injector::WriteMemory<char>(0x5B6B5B, 0x75, true);
		}

		if (bFMVWidescreenMode)
		{
			injector::WriteMemory<float>(0x598EBE + 1, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
			injector::WriteMemory<float>(0x598EC3 + 1, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
			injector::WriteMemory<float>(0x598EC8 + 1, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
			injector::WriteMemory<float>(0x598ECD + 1, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
		}
	}
	else
	{

	}
}

bool NotInCutsceneOrMenu()
{
	if (*(unsigned char*)0xA97DE4 != 0) //in game menu
	{
		if (*(unsigned char*)0xA97A80 == 0) // cutscene
		{
			return true;
		}
	}
	return false;
}

DWORD WINAPI HudHandler(LPVOID)
{
	CIniReader iniReader("nfsc_res.ini");
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
			if (*(float*)0x598DC0 != hud_position_x)
			{
				injector::WriteMemory<float>(0x9E8F8C, hud_multiplier_x, true);
				injector::WriteMemory<float>(0x9E8F88, hud_multiplier_y, true);

				injector::WriteMemory<float>(0x400000 + 0x198DC0, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0x198FB7, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0x199416, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0x1996AE, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0x1A18BE, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0x1D2B46, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0x6604AC, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0x5C778C, hud_position_x, true);

				MinimapPosX = hud_position_x;
				MinimapPosY = 240.0f;
				injector::WriteMemory<float>(0x598FB7, MinimapPosX, true);
				injector::WriteMemory<float>(0x599416, MinimapPosX, true);
				injector::WriteMemory<float>(0x599421, MinimapPosY, true);
				injector::WriteMemory<float>(0x598FC2, MinimapPosY, true);

				injector::WriteMemory(0x9D5E1C, 0x00642578, true); // "x%d" string
			}
		}
		else
		{
			Sleep(1);
			if (NotInCutsceneOrMenu())
			{
				if (*(float*)0x598DC0 == hud_position_x)
				{

					injector::WriteMemory<float>(0x9E8F8C, hud_multiplier_x / 1.7f, true);
					injector::WriteMemory<float>(0x9E8F88, hud_multiplier_y / 1.7f, true);

					injector::WriteMemory<float>(0x400000 + 0x198DC0, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0x198FB7, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0x199416, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0x1996AE, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0x1A18BE, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0x1D2B46, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0x6604AC, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0x5C778C, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);

					MinimapPosX = (hud_position_x * 2.0f) * 1.7f * 1.07f;
					MinimapPosY = 560.0f;
					injector::WriteMemory<float>(0x598FB7, MinimapPosX, true);
					injector::WriteMemory<float>(0x599416, MinimapPosX, true);
					injector::WriteMemory<float>(0x5D2B46, MinimapPosX, true);
					injector::WriteMemory<float>(0x599421, MinimapPosY, true);
					injector::WriteMemory<float>(0x598FC2, MinimapPosY, true);
					injector::WriteMemory<float>(0x5D2B4E, MinimapPosY, true);

					injector::WriteMemory(0x9D5E1C, 0x00202020, true);  // "x%d" string
				}
			}
			else
			{
				if (*(float*)0x598DC0 != hud_position_x)
				{
					injector::WriteMemory<float>(0x9E8F8C, hud_multiplier_x, true);
					injector::WriteMemory<float>(0x9E8F88, hud_multiplier_y, true);

					injector::WriteMemory<float>(0x400000 + 0x198DC0, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0x198FB7, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0x199416, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0x1996AE, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0x1A18BE, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0x1D2B46, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0x6604AC, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0x5C778C, hud_position_x, true);

					MinimapPosX = hud_position_x;
					MinimapPosY = 240.0f;
					injector::WriteMemory<float>(0x598FB7, MinimapPosX, true);
					injector::WriteMemory<float>(0x599416, MinimapPosX, true);
					injector::WriteMemory<float>(0x599421, MinimapPosY, true);
					injector::WriteMemory<float>(0x598FC2, MinimapPosY, true);

					injector::WriteMemory(0x9D5E1C, 0x00642578, true); // "x%d" string
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
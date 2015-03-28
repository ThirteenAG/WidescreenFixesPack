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
DWORD WINAPI HudHandler(LPVOID); DWORD WINAPI HudHandler2(LPVOID);
float hud_multiplier_x, hud_multiplier_y;
float hud_position_x;
float MinimapPosX, MinimapPosY;
float verFovCorrectionFactor;

void Init()
{
	CIniReader iniReader("nfsu2_res.ini");
	ResX = iniReader.ReadInteger("MAIN", "ResX", 0);
	ResY = iniReader.ReadInteger("MAIN", "ResY", 0);
	HudFix = iniReader.ReadInteger("MAIN", "HudFix", 1) == 1;
	verFovCorrectionFactor = iniReader.ReadFloat("MAIN", "verFovCorrectionFactor", 0.04f);
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
			hud_multiplier_x = (1.0f / ResX * (ResY / 480.0f)) * 2.0f;
			hud_multiplier_y = *(float*)0x79AC14;
			hud_position_x = 640.0f / (640.0f * hud_multiplier_x);

			injector::WriteMemory<float>(0x79AC10, hud_multiplier_x, true);
			//CPatch::SetFloat(0x79AC14, hud_multiplier_y);

			injector::WriteMemory<float>(0x51B3CB, hud_position_x, true);
			injector::WriteMemory<float>(0x5368C8, hud_position_x, true);
			//injector::WriteMemory<float>(0x536A99, hud_position_x, true); //minimap
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


			injector::WriteMemory<float>(0x5C726A, (float)ResX, true); // radar mask
			injector::WriteMemory<float>(0x5C727A, (float)ResX, true);
			injector::WriteMemory<float>(0x5C7282, (float)ResY, true);
			injector::WriteMemory<float>(0x5C7292, (float)ResY, true);


			//minimap position
			MinimapPosX = hud_position_x;
			MinimapPosY = 240.0f;
			injector::WriteMemory<float>(0x536A99, MinimapPosX, true); //minimap
			injector::WriteMemory<float>(0x536AA4, MinimapPosY, true); //minimap

			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&HudHandler, NULL, 0, NULL);
		}

		if (DisableCutsceneBorders)
		{
			injector::WriteMemory<float>(0x5CBEF5, 0.0f, true); // borders
			injector::WriteMemory<float>(0x5CBF05, 0.0f, true);
			injector::WriteMemory<float>(0x5CBE89, 0.0f, true);
			injector::WriteMemory<float>(0x5CBEA1, 0.0f, true);
		}

		if (verFovCorrectionFactor)
		{
			injector::WriteMemory(0x5C7FE8 + 2, &horFOV, true);
			injector::WriteMemory(0x5C801F + 2, &verFOV, true);

			horFOV = 1.0f / ((1.0f * ((float)ResX / (float)ResY)) / (4.0f / 3.0f));
			verFOV = horFOV - verFovCorrectionFactor;
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
			hud_multiplier_x = (1.0f / ResX * (ResY / 480.0f)) * 2.0f;
			hud_multiplier_y = *(float*)0x79AC14 + 0x4;
			hud_position_x = 640.0f / (640.0f * hud_multiplier_x);

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


			injector::WriteMemory<float>(0x5C726A - 0x400, (float)ResX, true); // radar mask
			injector::WriteMemory<float>(0x5C727A - 0x400, (float)ResX, true);
			injector::WriteMemory<float>(0x5C7282 - 0x400, (float)ResY, true);
			injector::WriteMemory<float>(0x5C7292 - 0x400, (float)ResY, true);

			//minimap position
			MinimapPosX = hud_position_x;
			MinimapPosY = 240.0f;
			injector::WriteMemory<float>(0x536A99 - 0x460, MinimapPosX, true); //minimap
			injector::WriteMemory<float>(0x536AA4 - 0x460, MinimapPosY, true); //minimap

			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&HudHandler2, NULL, 0, NULL);
		}

		if (DisableCutsceneBorders)
		{
			injector::WriteMemory<float>(0x5CBEF5 - 0x400, 0.0f, true); // borders
			injector::WriteMemory<float>(0x5CBF05 - 0x400, 0.0f, true);
			injector::WriteMemory<float>(0x5CBE89 - 0x400, 0.0f, true);
			injector::WriteMemory<float>(0x5CBEA1 - 0x400, 0.0f, true);
		}

		if (verFovCorrectionFactor)
		{
			injector::WriteMemory(0x5C7BE8 + 2, &horFOV, true);
			injector::WriteMemory(0x5C7C1F + 2, &verFOV, true);

			horFOV = 1.0f / ((1.0f * ((float)ResX / (float)ResY)) / (4.0f / 3.0f));
			verFOV = horFOV - verFovCorrectionFactor;
		}
	}
}


DWORD WINAPI HudHandler(LPVOID)
{
	CIniReader iniReader("nfsu2_res.ini");
	while (true)
	{
		Sleep(0);

		if ((GetAsyncKeyState(VK_F1) & 1))
		{
			bHudMode = !bHudMode;
			iniReader.WriteInteger("MAIN", "HudMode", bHudMode);
			while ((GetAsyncKeyState(VK_F1) & 0x8000) > 0) { Sleep(0); }
		}

		if (!bHudMode)
		{
			Sleep(1);
			if (*(float*)0x51B3CB != hud_position_x)
			{
				injector::WriteMemory<float>(0x79AC10, hud_multiplier_x, true);
				injector::WriteMemory<float>(0x79AC14, hud_multiplier_y, true);

				injector::WriteMemory<float>(0x51B3CB, hud_position_x, true);
				injector::WriteMemory<float>(0x5368C8, hud_position_x, true);
				//injector::WriteMemory<float>(0x536A99, hud_position_x, true);
				injector::WriteMemory<float>(0x536CC9, hud_position_x, true);
				injector::WriteMemory<float>(0x537011, hud_position_x, true);
				injector::WriteMemory<float>(0x48B640, hud_position_x, true);
				injector::WriteMemory<float>(0x50B4F5, hud_position_x, true);
				injector::WriteMemory<float>(0x797D50, hud_position_x, true);

				MinimapPosX = hud_position_x;
				MinimapPosY = 240.0f;
				injector::WriteMemory<float>(0x536A99, MinimapPosX, true); //minimap
				injector::WriteMemory<float>(0x536AA4, MinimapPosY, true); //minimap
			}
		}
		else
		{
			Sleep(1);
			if (*(unsigned char*)0x874E58 != 1)
			{
				if (*(float*)0x51B3CB == hud_position_x)
				{
					injector::WriteMemory<float>(0x79AC10, hud_multiplier_x / 1.7f, true);
					injector::WriteMemory<float>(0x79AC14, hud_multiplier_y / 1.7f, true);

					injector::WriteMemory<float>(0x51B3CB, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x5368C8, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					//injector::WriteMemory<float>(0x536A99, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x536CC9, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x537011, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x48B640, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x50B4F5, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x797D50, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);

					MinimapPosX = (hud_position_x * 2.0f) * 1.7f * 1.07f;
					MinimapPosY = 550.0f;
					injector::WriteMemory<float>(0x536A99, MinimapPosX, true); //minimap
					injector::WriteMemory<float>(0x536AA4, MinimapPosY, true); //minimap
				}
			}
			else
			{
				if (*(float*)0x51B3CB != hud_position_x)
				{
					injector::WriteMemory<float>(0x79AC10, hud_multiplier_x, true);
					injector::WriteMemory<float>(0x79AC14, hud_multiplier_y, true);

					injector::WriteMemory<float>(0x51B3CB, hud_position_x, true);
					injector::WriteMemory<float>(0x5368C8, hud_position_x, true);
					//injector::WriteMemory<float>(0x536A99, hud_position_x, true);
					injector::WriteMemory<float>(0x536CC9, hud_position_x, true);
					injector::WriteMemory<float>(0x537011, hud_position_x, true);
					injector::WriteMemory<float>(0x48B640, hud_position_x, true);
					injector::WriteMemory<float>(0x50B4F5, hud_position_x, true);
					injector::WriteMemory<float>(0x797D50, hud_position_x, true);

					MinimapPosX = hud_position_x;
					MinimapPosY = 240.0f;
					injector::WriteMemory<float>(0x536A99, MinimapPosX, true); //minimap
					injector::WriteMemory<float>(0x536AA4, MinimapPosY, true); //minimap
				}
			}
		}
	}
	return 0;
}

DWORD WINAPI HudHandler2(LPVOID)
{
	CIniReader iniReader("nfsu2_res.ini");
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
			if (*(float*)0x51AEAB != hud_position_x)
			{
				injector::WriteMemory<float>(0x79AC10 + 0x4, hud_multiplier_x, true);
				injector::WriteMemory<float>(0x79AC14 + 0x4, hud_multiplier_y, true);

				injector::WriteMemory<float>(0x51B3CB - 0x520, hud_position_x, true);
				injector::WriteMemory<float>(0x5368C8 - 0x460, hud_position_x, true);
				injector::WriteMemory<float>(0x536A99 - 0x460, hud_position_x, true);
				injector::WriteMemory<float>(0x536CC9 - 0x460, hud_position_x, true);
				injector::WriteMemory<float>(0x537011 - 0x460, hud_position_x, true);
				injector::WriteMemory<float>(0x48B640 - 0xA0, hud_position_x, true);
				injector::WriteMemory<float>(0x50B4F5 - 0x5C0, hud_position_x, true);
				injector::WriteMemory<float>(0x797D50 + 0x1C, hud_position_x, true);

				MinimapPosX = hud_position_x;
				MinimapPosY = 240.0f;
				injector::WriteMemory<float>(0x536A99 - 0x460, MinimapPosX, true); //minimap
				injector::WriteMemory<float>(0x536AA4 - 0x460, MinimapPosY, true); //minimap
			}
		}
		else
		{
			Sleep(1);
			if (*(unsigned char*)0x874E58 != 1)
			{
				if (*(float*)0x51AEAB == hud_position_x)
				{
					injector::WriteMemory<float>(0x79AC10 + 0x4, hud_multiplier_x / 1.7f, true);
					injector::WriteMemory<float>(0x79AC14 + 0x4, hud_multiplier_y / 1.7f, true);

					injector::WriteMemory<float>(0x51B3CB - 0x520, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x5368C8 - 0x460, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x536A99 - 0x460, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x536CC9 - 0x460, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x537011 - 0x460, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x48B640 - 0xA0,  hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x50B4F5 - 0x5C0, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x797D50 + 0x1C,  hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);

					MinimapPosX = (hud_position_x * 2.0f) * 1.7f * 1.07f;
					MinimapPosY = 550.0f;
					injector::WriteMemory<float>(0x536A99 - 0x460, MinimapPosX, true); //minimap
					injector::WriteMemory<float>(0x536AA4 - 0x460, MinimapPosY, true); //minimap
				}
			}
			else
			{
				if (*(float*)0x51AEAB != hud_position_x)
				{
					injector::WriteMemory<float>(0x79AC10 + 0x4, hud_multiplier_x, true);
					injector::WriteMemory<float>(0x79AC14 + 0x4, hud_multiplier_y, true);

					injector::WriteMemory<float>(0x51B3CB - 0x520, hud_position_x, true);
					injector::WriteMemory<float>(0x5368C8 - 0x460, hud_position_x, true);
					injector::WriteMemory<float>(0x536A99 - 0x460, hud_position_x, true);
					injector::WriteMemory<float>(0x536CC9 - 0x460, hud_position_x, true);
					injector::WriteMemory<float>(0x537011 - 0x460, hud_position_x, true);
					injector::WriteMemory<float>(0x48B640 - 0xA0, hud_position_x, true);
					injector::WriteMemory<float>(0x50B4F5 - 0x5C0, hud_position_x, true);
					injector::WriteMemory<float>(0x797D50 + 0x1C, hud_position_x, true);

					MinimapPosX = hud_position_x;
					MinimapPosY = 240.0f;
					injector::WriteMemory<float>(0x536A99 - 0x460, MinimapPosX, true); //minimap
					injector::WriteMemory<float>(0x536AA4 - 0x460, MinimapPosY, true); //minimap
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


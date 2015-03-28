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

void Init()
{
	CIniReader iniReader("nfsc_res.ini");
	ResX = iniReader.ReadInteger("MAIN", "ResX", 0);
	ResY = iniReader.ReadInteger("MAIN", "ResY", 0);
	HudFix = iniReader.ReadInteger("MAIN", "HudFix", 1) == 1;
	horFOV = iniReader.ReadFloat("MAIN", "horFOV", 0.0f);
	verFOV = iniReader.ReadFloat("MAIN", "verFOV", 0.0f);

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
			//CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&hud_handler, NULL, 0, NULL);

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
		}

		if (horFOV && verFOV)
		{
			injector::WriteMemory(0x71B883 + 2, &horFOV, true);
			injector::WriteMemory(0x71B8AE + 2, &verFOV, true);
			injector::WriteMemory(0x71B8EC + 2, &verFOV, true);
		}
		else
		{
			injector::WriteMemory(0x71B8DA + 2, &horFOV, true);
			injector::WriteMemory(0x71B923 + 2, &verFOV, true);

			horFOV = 1.0f / ((1.0f * ((float)ResX / (float)ResY)) / (4.0f / 3.0f));
			verFOV = horFOV - 0.03f;

			/*//16:9 excluisve fov hack
			if ((float)ResX / (float)ResY == 16.0f / 9.0f)
			{
				injector::WriteMemory(0x71B923 + 2, &horFOV, true);
				injector::WriteMemory(0x71B8AE + 2, &verFOV, true);
				injector::WriteMemory(0x71B8EC + 2, &verFOV, true);

				horFOV = (1.0f * ((float)ResX / (float)ResY)) / (4.0f / 3.0f);
				verFOV = 0.5f / 1.05f;
			}
			else if ((float)ResX / (float)ResY == 16.0f / 10.0f)
			{
				injector::WriteMemory(0x71B923 + 2, &horFOV, true);
				injector::WriteMemory(0x71B8AE + 2, &verFOV, true);
				injector::WriteMemory(0x71B8EC + 2, &verFOV, true);
				
				horFOV = (1.0f * ((float)ResX / (float)ResY)) / (4.0f / 3.0f);
				verFOV = 0.5f / 1.08f;
			}*/
		}
	}
	else
	{

	}
}


DWORD WINAPI HudHandler(LPVOID)
{
	CIniReader iniReader("nfsmw_res.ini");
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
			if (*(float*)0x56FED4 != hud_position_x)
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

				/*MinimapPosX = hud_position_x;
				MinimapPosY = 240.0f;
				injector::WriteMemory<float>(0x59A127, MinimapPosX, true);
				injector::WriteMemory<float>(0x59A5B2, MinimapPosX, true);
				injector::WriteMemory<float>(0x59A132, MinimapPosY, true);
				injector::WriteMemory<float>(0x59A5BD, MinimapPosY, true);*/
			}
		}
		else
		{
			Sleep(1);
			if (*(unsigned char*)0x988DEC != 0)
			{
				if (*(float*)0x56FED4 == hud_position_x)
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

					/*MinimapPosX = (hud_position_x * 2.0f) * 1.7f * 1.07f;
					MinimapPosY = 20.0f;
					injector::WriteMemory<float>(0x59A127, MinimapPosX, true);
					injector::WriteMemory<float>(0x59A5B2, MinimapPosX, true);
					injector::WriteMemory<float>(0x59A132, MinimapPosY, true);
					injector::WriteMemory<float>(0x59A5BD, MinimapPosY, true);

					injector::WriteMemory<float>(0x400000 + 0x1A44CC, 20.0f, true); //text*/
				}
			}
			else
			{
				if (*(float*)0x56FED4 != hud_position_x)
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

					/*MinimapPosX = hud_position_x;
					MinimapPosY = 240.0f;
					injector::WriteMemory<float>(0x59A127, MinimapPosX, true);
					injector::WriteMemory<float>(0x59A5B2, MinimapPosX, true);
					injector::WriteMemory<float>(0x59A132, MinimapPosY, true);
					injector::WriteMemory<float>(0x59A5BD, MinimapPosY, true);*/
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
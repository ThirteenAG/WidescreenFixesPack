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
DWORD* _8F1CA0 = (DWORD*)0x8F1CA0;

void Init()
{
	CIniReader iniReader("nfsmw_res.ini");
	ResX = iniReader.ReadInteger("MAIN", "ResX", 0);
	ResY = iniReader.ReadInteger("MAIN", "ResY", 0);
	HudFix = iniReader.ReadInteger("MAIN", "HudFix", 1) == 1;
	verFovCorrectionFactor = iniReader.ReadFloat("MAIN", "verFovCorrectionFactor", 0.06f);
	bHudMode = iniReader.ReadInteger("MAIN", "HudMode", 1) == 1;
	int ShadowsRes = iniReader.ReadInteger("MAIN", "ShadowsRes", 1024);
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

	//game
	injector::WriteMemory(0x6C27ED + 0x2, ResX, true);
	injector::WriteMemory(0x6C27F3 + 0x2, ResY, true);
	injector::WriteMemory(0x6C2804 + 0x2, ResX, true);
	injector::WriteMemory(0x6C280A + 0x2, ResY, true);
	injector::WriteMemory(0x6C281B + 0x2, ResX, true);
	injector::WriteMemory(0x6C2821 + 0x2, ResY, true);
	injector::WriteMemory(0x6C2832 + 0x2, ResX, true);
	injector::WriteMemory(0x6C2838 + 0x2, ResY, true);
	injector::WriteMemory(0x6C2849 + 0x2, ResX, true);
	injector::WriteMemory(0x6C284F + 0x2, ResY, true);
	injector::WriteMemory(0x6C2860 + 0x2, ResX, true);
	injector::WriteMemory(0x6C2866 + 0x2, ResY, true);
	injector::WriteMemory(0x6C28CD + 0x2, ResX, true);
	injector::WriteMemory(0x6C28D3 + 0x2, ResY, true);
	injector::WriteMemory(0x6C28E4 + 0x2, ResX, true);
	injector::WriteMemory(0x6C28EA + 0x2, ResY, true);
	injector::WriteMemory(0x6C28FB + 0x2, ResX, true);
	injector::WriteMemory(0x6C2901 + 0x2, ResY, true);
	injector::WriteMemory(0x6C2912 + 0x2, ResX, true);
	injector::WriteMemory(0x6C2918 + 0x2, ResY, true);
	injector::WriteMemory(0x6C2929 + 0x2, ResX, true);
	injector::WriteMemory(0x6C292F + 0x2, ResY, true);
	injector::WriteMemory(0x6C2940 + 0x2, ResX, true);
	injector::WriteMemory(0x6C2946 + 0x2, ResY, true);

		injector::WriteMemory(0x4F9ADC + 0x400000, ResX, true);
		injector::WriteMemory(0x53DACC + 0x400000, ResX, true);
		injector::WriteMemory(0x53DB04 + 0x400000, ResX, true);
		injector::WriteMemory(0x53E82C + 0x400000, ResX, true);
		injector::WriteMemory(0x582BE4 + 0x400000, ResX, true);
													   
		injector::WriteMemory(0x4F9AE0 + 0x400000, ResY, true);
		injector::WriteMemory(0x53DAD0 + 0x400000, ResY, true);
		injector::WriteMemory(0x53DB08 + 0x400000, ResY, true);
		injector::WriteMemory(0x53E830 + 0x400000, ResY, true);
		injector::WriteMemory(0x582BE8 + 0x400000, ResY, true);

		injector::WriteMemory<float>(0x8AE8F8, (float)ResX, true);
		injector::WriteMemory<float>(0x8AF2C0, (float)ResY, true);

		if (ShadowsRes) {
			
			injector::WriteMemory(0x6C86B0 + 0x1, ShadowsRes, true);
			injector::WriteMemory(0x6C86C0 + 0x1, ShadowsRes, true);
			injector::WriteMemory(0x6C8786 + 0x1, ShadowsRes, true);
			injector::WriteMemory(0x6C878B + 0x1, ShadowsRes, true);
			injector::WriteMemory(0x6C87B8 + 0x1, ShadowsRes, true);
			injector::WriteMemory(0x6C87BD + 0x1, ShadowsRes, true);


			injector::WriteMemory(0x6C1499 + 0x2, _8F1CA0, true);
			injector::WriteMemory(0x6C14DB + 0x2, _8F1CA0, true);
			injector::WriteMemory(0x6C1510 + 0xC6 + 0x1, _8F1CA0, true);
			injector::WriteMemory(0x6C1510 + 0x146 + 0x1, _8F1CA0, true);
			injector::WriteMemory(0x6C1510 + 0x1C6 + 0x1, _8F1CA0, true);
			injector::WriteMemory(0x6C16F8 + 0x1, _8F1CA0, true);
			injector::WriteMemory(0x6C1990 + 0x101 + 0x2, _8F1CA0, true);
			injector::WriteMemory(0x6C1B60 + 0x5 + 0x2, _8F1CA0, true);
			injector::WriteMemory(0x6C5090 + 0x2, _8F1CA0, true);
			injector::WriteMemory(0x6C50E0 + 0x2, _8F1CA0, true);
			injector::WriteMemory(0x6C5E60 + 0xA2 + 0x2, _8F1CA0, true);
			injector::WriteMemory(0x6C5E60 + 0xEC + 0x2, _8F1CA0, true);
			injector::WriteMemory(0x6CFCE0 + 0xCF + 0x2, _8F1CA0, true);
			injector::WriteMemory(0x6CFCE0 + 0x114 + 0x2, _8F1CA0, true);
			injector::WriteMemory(0x6D2100 + 0x1, _8F1CA0, true);
			injector::WriteMemory(0x6D5C10 + 0x24 + 0x1, _8F1CA0, true);
			injector::WriteMemory(0x6D5DDC + 0x1, _8F1CA0, true);
			injector::WriteMemory(0x6DF91B + 0x1, _8F1CA0, true);
			injector::WriteMemory(0x6DF8F0 + 0xA9 + 0x2, _8F1CA0, true);
			injector::WriteMemory(0x5009D0 + 0x1E6199 + 0x2, _8F1CA0, true);
		}

		//HUD
		if (HudFix)
		{
			hud_multiplier_x = (1.0f / ResX * (ResY / 480.0f)) * 2.0f;
			hud_multiplier_y = *(float*)0x8AF9A0;
			hud_position_x = 640.0f / (640.0f * hud_multiplier_x);

			injector::WriteMemory<float>(0x8AF9A4, hud_multiplier_x, true);
			//injector::WriteMemory<float>(0x8AF9A0, hud_multiplier_y, true);

			injector::WriteMemory<float>(0x400000 + 0x16FED4, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x184EF6, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x199E80, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x19A127, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x19A5B2, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x19A845, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x1A44CC, hud_position_x, true);
			injector::WriteMemory<float>(0x400000 + 0x494B40, hud_position_x, true);

			//mirror position fix
			injector::WriteMemory<float>(0x6E70C0, ((640.0f / (640.0f * hud_multiplier_x)) + 130.0f), true);
			injector::WriteMemory<float>(0x6E70FF, ((640.0f / (640.0f * hud_multiplier_x)) + 130.0f), true);
			injector::WriteMemory<float>(0x6E70D3, ((640.0f / (640.0f * hud_multiplier_x)) - 130.0f), true);
			injector::WriteMemory<float>(0x6E70E9, ((640.0f / (640.0f * hud_multiplier_x)) - 130.0f), true);

			//minimap position
			MinimapPosX = hud_position_x;
			MinimapPosY = 240.0f;
			injector::WriteMemory<float>(0x59A127, MinimapPosX, true);
			injector::WriteMemory<float>(0x59A5B2, MinimapPosX, true);
			injector::WriteMemory<float>(0x59A132, MinimapPosY, true);
			injector::WriteMemory<float>(0x59A5BD, MinimapPosY, true);

			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&HudHandler, NULL, 0, NULL);
		}

		if (verFovCorrectionFactor)
		{
			injector::WriteMemory(0x6CF566 + 2, &horFOV, true);
			injector::WriteMemory(0x6CF5DC + 2, &verFOV, true);

			horFOV = 1.0f / ((1.0f * ((float)ResX / (float)ResY)) / (4.0f / 3.0f));
			verFOV = horFOV - verFovCorrectionFactor;
		}

		if (bHudWidescreenMode) // thanks to dyons
		{
			injector::WriteMemory<char>(0x5696CD, 0x20, true);
			injector::WriteMemory<char>(0x57CB84, 0x20, true);
			injector::WriteMemory<char>(0x58D885, 0x20, true);

			injector::MakeNOP(0x58D890, 2, true);
			injector::MakeNOP(0x58D8A2, 2, true);
		}

		if (bFMVWidescreenMode)
		{
			injector::WriteMemory<float>(0x59A029 + 1, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
			injector::WriteMemory<float>(0x59A02E + 1, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
			injector::WriteMemory<float>(0x59A033 + 1, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
			injector::WriteMemory<float>(0x59A038 + 1, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
		}
}

bool NotInCutsceneOrMenu()
{
	if (*(unsigned char*)0x988DEC != 0) //in game menu
	{
		if (*(unsigned char*)0x91A0D4 != 0) // cutscene
		{
			return true;
		}
	}
	return false;
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
				injector::WriteMemory<float>(0x8AF9A4, hud_multiplier_x, true);
				injector::WriteMemory<float>(0x8AF9A0, hud_multiplier_y, true);

				injector::WriteMemory<float>(0x400000 + 0x16FED4, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0x184EF6, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0x199E80, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0x19A127, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0x19A5B2, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0x19A845, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0x1A44CC, hud_position_x, true);
				injector::WriteMemory<float>(0x400000 + 0x494B40, hud_position_x, true);

				MinimapPosX = hud_position_x;
				MinimapPosY = 240.0f;
				injector::WriteMemory<float>(0x59A127, MinimapPosX, true);
				injector::WriteMemory<float>(0x59A5B2, MinimapPosX, true);
				injector::WriteMemory<float>(0x59A132, MinimapPosY, true);
				injector::WriteMemory<float>(0x59A5BD, MinimapPosY, true);

				injector::WriteMemory(0x8A04A0, 0x00642578, true); // "x%d" string
			}
		}
		else
		{
			Sleep(1);
			if (NotInCutsceneOrMenu())
			{
				if (*(float*)0x56FED4 == hud_position_x)
				{

					injector::WriteMemory<float>(0x8AF9A4, hud_multiplier_x / 1.7f, true);
					injector::WriteMemory<float>(0x8AF9A0, hud_multiplier_y / 1.7f, true);

					injector::WriteMemory<float>(0x400000 + 0x16FED4, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0x184EF6, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0x199E80, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0x19A127, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0x19A5B2, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0x19A845, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0x1A44CC, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);
					injector::WriteMemory<float>(0x400000 + 0x494B40, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true);

					MinimapPosX = (hud_position_x * 2.0f) * 1.7f * 1.07f;
					MinimapPosY = 560.0f;
					injector::WriteMemory<float>(0x59A127, MinimapPosX, true);
					injector::WriteMemory<float>(0x59A5B2, MinimapPosX, true);
					injector::WriteMemory<float>(0x59A132, MinimapPosY, true);
					injector::WriteMemory<float>(0x59A5BD, MinimapPosY, true);

					injector::WriteMemory<float>(0x400000 + 0x1A44CC, hud_position_x * 2.0f + 290.0f * ((float)ResX * (1.0f / 1920.0f)), true); //text

					injector::WriteMemory(0x8A04A0, 0x00202020, true);  // "x%d" string
				}
			}
			else
			{
				if (*(float*)0x56FED4 != hud_position_x)
				{
					injector::WriteMemory<float>(0x8AF9A4, hud_multiplier_x, true);
					injector::WriteMemory<float>(0x8AF9A0, hud_multiplier_y, true);

					injector::WriteMemory<float>(0x400000 + 0x16FED4, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0x184EF6, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0x199E80, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0x19A127, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0x19A5B2, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0x19A845, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0x1A44CC, hud_position_x, true);
					injector::WriteMemory<float>(0x400000 + 0x494B40, hud_position_x, true);

					MinimapPosX = hud_position_x;
					MinimapPosY = 240.0f;
					injector::WriteMemory<float>(0x59A127, MinimapPosX, true);
					injector::WriteMemory<float>(0x59A5B2, MinimapPosX, true);
					injector::WriteMemory<float>(0x59A132, MinimapPosY, true);
					injector::WriteMemory<float>(0x59A5BD, MinimapPosY, true);

					injector::WriteMemory(0x8A04A0, 0x00642578, true); // "x%d" string
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


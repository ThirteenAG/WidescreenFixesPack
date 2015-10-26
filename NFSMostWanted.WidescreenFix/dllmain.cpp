#include "..\includes\stdafx.h"
#include "..\includes\hooking\Hooking.Patterns.h"

HWND hWnd;

bool HudFix;
int ResX;
int ResY;
float horFOV, verFOV;
float fHudScaleX, fHudScaleY;
float fHudPosX;
float MinimapPosX, MinimapPosY;
float verFovCorrectionFactor;
DWORD* dword_8F1CA0;
DWORD jmpAddr;

void __declspec(naked) WidescreenHudHook()
{
	_asm
	{
		mov [esi + 0Ch], ebx
		mov byte ptr ds:[esi + 10h], 1
		jmp jmpAddr
	}
}

void Init()
{
	CIniReader iniReader("");
	ResX = iniReader.ReadInteger("MAIN", "ResX", 0);
	ResY = iniReader.ReadInteger("MAIN", "ResY", 0);
	HudFix = iniReader.ReadInteger("MAIN", "HudFix", 1) == 1;
	verFovCorrectionFactor = iniReader.ReadFloat("MAIN", "verFovCorrectionFactor", 0.06f);
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

	for (size_t i = 0; i < 2; i++)
	{
		//game
		DWORD* dword_6C27ED = hook::pattern("C7 00 80 02 00 00 C7 01 E0 01 00 00 C2 08 00").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_6C27ED, ResX, true);
		DWORD dword_6C27F3 = (DWORD)dword_6C27ED + 6;
		injector::WriteMemory(dword_6C27F3, ResY, true);
		DWORD* dword_6C2804 = hook::pattern("C7 02 20 03 00 00 C7 00 58 02 00 00 C2 08 00").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_6C2804, ResX, true);
		DWORD dword_6C280A = (DWORD)dword_6C2804 + 6;
		injector::WriteMemory(dword_6C280A, ResY, true);
		DWORD* dword_6C281B = hook::pattern("C7 01 00 04 00 00 C7 02 00 03 00 00 C2 08 00").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_6C281B, ResX, true);
		DWORD dword_6C2821 = (DWORD)dword_6C281B + 6;
		injector::WriteMemory(dword_6C2821, ResY, true);
		DWORD* dword_6C2832 = hook::pattern("C7 00 00 05 00 00 C7 01 C0 03 00 00 C2 08 00").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_6C2832, ResX, true);
		DWORD dword_6C2838 = (DWORD)dword_6C2832 + 6;
		injector::WriteMemory(dword_6C2838, ResY, true);
		DWORD* dword_6C2849 = hook::pattern("C7 02 00 05 00 00 C7 00 00 04 00 00 C2 08 00").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_6C2849, ResX, true);
		DWORD dword_6C284F = (DWORD)dword_6C2849 + 6;
		injector::WriteMemory(dword_6C284F, ResY, true);
		DWORD* dword_6C2860 = hook::pattern("C7 01 40 06 00 00 C7 02 B0 04 00 00 C2 08 00").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_6C2860, ResX, true);
		DWORD dword_6C2866 = (DWORD)dword_6C2860 + 6;
		injector::WriteMemory(dword_6C2866, ResY, true);
	}

		/*injector::WriteMemory(0x4F9ADC + 0x400000, ResX, true);
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
		injector::WriteMemory<float>(0x8AF2C0, (float)ResY, true);*/

		if (ShadowsRes) 
		{
			dword_8F1CA0 = *hook::pattern("8B 14 85 ? ? ? ? 0F AF 56 5C C1 FA 0F 89 56 5C").get(0).get<DWORD*>(3);
			dword_8F1CA0 += 0x1D4;

			DWORD* dword_6C86B0 = hook::pattern("B8 00 04 00 00 C3").get(1).get<DWORD>(1);
			injector::WriteMemory(dword_6C86B0, ShadowsRes, true);
			DWORD* dword_6C86C0 = hook::pattern("B8 00 04 00 00 C3").get(1).get<DWORD>(1);
			injector::WriteMemory(dword_6C86C0, ShadowsRes, true);
			DWORD* dword_6C8786 = hook::pattern("68 00 04 00 00 68 00 04 00 00 50 FF 51 5C 85 C0 7C 32").get(0).get<DWORD>(1);
			injector::WriteMemory(dword_6C8786, ShadowsRes, true);
			DWORD dword_6C878B = (DWORD)dword_6C8786 + 5;
			injector::WriteMemory(dword_6C878B, ShadowsRes, true);
			DWORD* dword_6C87B8 = hook::pattern("68 00 04 00 00 68 00 04 00 00 50 FF 52 5C 85 C0 7D 36").get(0).get<DWORD>(1);
			injector::WriteMemory(dword_6C87B8, ShadowsRes, true);
			DWORD dword_6C87BD = (DWORD)dword_6C87B8 + 5;
			injector::WriteMemory(dword_6C87BD, ShadowsRes, true);

			DWORD* dword_93D898 = *hook::pattern("A1 ? ? ? ? 49 3D 02 10 00 00 89 0D ? ? ? ?").get(0).get<DWORD*>(1);
			char TempStr[10];
			sprintf(TempStr, "%x %x %x %x", ((DWORD)dword_93D898 >> 0) & 0xff, ((DWORD)dword_93D898 >> 8) & 0xff, ((DWORD)dword_93D898 >> 16) & 0xff, ((DWORD)dword_93D898 >> 24) & 0xff);

			for (size_t i = 0; i < 20; i++)
			{
				DWORD* dword_93D898 = hook::pattern(TempStr).get(0).get<DWORD>(0);
				injector::WriteMemory(dword_93D898, dword_8F1CA0, true);
			}
		}

		//HUD
		if (HudFix)
		{
			fHudScaleX = (1.0f / ResX * (ResY / 480.0f)) * 2.0f;
			fHudPosX = 640.0f / (640.0f * fHudScaleX);

			DWORD* dword_8AF9A4 = *hook::pattern("D8 0D ? ? ? ? D8 25 ? ? ? ? D9 5C 24 20 D9 46 04").get(0).get<DWORD*>(2);
			injector::WriteMemory<float>(dword_8AF9A4, fHudScaleX, true);

			//fHudScaleY = *(float*)0x8AF9A0;
			//injector::WriteMemory<float>(0x8AF9A0, fHudScaleY, true);

			for (size_t i = 0; i < 6; i++)
			{
				DWORD* dword_56FED4 = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43").get(0).get<DWORD>(7);
				injector::WriteMemory<float>(dword_56FED4, fHudPosX, true);
			}

			DWORD* dword_5A44CC = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43").get(0).get<DWORD>(4);
			injector::WriteMemory<float>(dword_5A44CC, fHudPosX, true);
			DWORD* dword_894B40 = *hook::pattern("D8 25 ? ? ? ? D9 5C 24 14 DB 05 ? ? ? ? D8 25 ? ? ? ? D9 5C 24 1C 74 20").get(0).get<DWORD*>(2);
			injector::WriteMemory<float>(dword_894B40, fHudPosX, true);

			//mirror position fix
			DWORD* dword_6E70C0 = hook::pattern("C7 44 24 70 00 00 E1 43 C7 44 24 74 00 00 98 41 C7 84 24 80 00 00 00 00 00 3E 43").get(0).get<DWORD>(4);
			DWORD dword_6E70FF = (DWORD)dword_6E70C0 + 63;
			DWORD dword_6E70D3 = (DWORD)dword_6E70C0 + 19;
			DWORD dword_6E70E9 = (DWORD)dword_6E70C0 + 41;
			injector::WriteMemory<float>(dword_6E70C0, (fHudPosX - 320.0f) + 190.0f, true);
			injector::WriteMemory<float>(dword_6E70FF, (fHudPosX - 320.0f) + 190.0f, true);
			injector::WriteMemory<float>(dword_6E70D3, (fHudPosX - 320.0f) + 450.0f, true);
			injector::WriteMemory<float>(dword_6E70E9, (fHudPosX - 320.0f) + 450.0f, true);
		}

		if (verFovCorrectionFactor)
		{
			horFOV = 1.0f / ((1.0f * ((float)ResX / (float)ResY)) / (4.0f / 3.0f));
			verFOV = horFOV - verFovCorrectionFactor;

			DWORD* dword_6CF568 = hook::pattern("D8 3D ? ? ? ? D9 5C 24 20 DB 44 24 30 D8 4C 24 2C").get(0).get<DWORD>(2);
			injector::WriteMemory(dword_6CF568, &horFOV, true);
			DWORD* dword_6CF5DE = hook::pattern("D8 3D ? ? ? ? D9 44 24 20 D8 64 24 24 D8 7C 24 20 D9 C9 D9 E0 D9 5E 54").get(0).get<DWORD>(2);
			injector::WriteMemory(dword_6CF5DE, &verFOV, true);
		}

		if (bHudWidescreenMode) 
		{
			DWORD* dword_56D675 = hook::pattern("89 5E 0C 88 5E 10 E8 ? ? ? ? 88 5E 19 88 5E 22").get(0).get<DWORD>(0);
			jmpAddr = (DWORD)dword_56D675 + 6;
			injector::MakeJMP(dword_56D675, WidescreenHudHook, true);

			DWORD* dword_57CB82 = hook::pattern("3A 55 34 0F 85 0B 02 00 00 A1 ? ? ? ?").get(0).get<DWORD>(0);
			injector::WriteMemory<DWORD>(dword_57CB82, 0x0F01FA80, true);
			DWORD* dword_5696CB = hook::pattern("8A 41 34 38 86 30 03 00 00 74 52 84 C0").get(0).get<DWORD>(0);
			injector::WriteMemory<DWORD>(dword_5696CB, 0x389001B0, true);
			DWORD* dword_58D883 = hook::pattern("8A 40 34 5F 5E 5D 3B CB 5B 75 12").get(0).get<DWORD>(0);
			injector::WriteMemory<DWORD>(dword_58D883, 0x5F9001B0, true);
			DWORD* dword_56885A = hook::pattern("38 48 34 74 31 8B 4E 38 68 7E 78 8E 90").get(0).get<DWORD>(0); //wrong way
			injector::WriteMemory<DWORD>(dword_56885A, 0x7401F980, true);
		}

		if (bFMVWidescreenMode)
		{
			DWORD* dword_59A02A = hook::pattern("68 00 00 80 3F 68 00 00 00 3F 68 00 00 00 3F 68 00 00 00 BF 68 00 00 00 BF 8B CB E8 ? ? ? ? 8B 4C 24 18").get(0).get<DWORD>(6);
			injector::WriteMemory<float>(dword_59A02A + 0, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
			injector::WriteMemory<float>((DWORD)dword_59A02A + 5, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
			injector::WriteMemory<float>((DWORD)dword_59A02A + 10, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
			injector::WriteMemory<float>((DWORD)dword_59A02A + 15, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
		}
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}


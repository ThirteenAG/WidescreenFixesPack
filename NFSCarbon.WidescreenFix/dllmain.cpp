#include "..\includes\stdafx.h"
#include "..\includes\hooking\Hooking.Patterns.h"
#include <iostream>
#include <string>
HWND hWnd;
bool bDelay;

bool bFixHUD, bFixFOV;
int ResX;
int ResY;
float hor3DScale, ver3DScale;
bool bHudMode;
float fHudScaleX, fHudScaleY;
float fHudPosX;
float MinimapPosX, MinimapPosY;
DWORD jmpAddr, jmpAddr2;
char* szCustomUserFilesDirectoryInGameDir;

HRESULT WINAPI SHGetFolderPathAHook(HWND /*hwnd*/, int /*csidl*/, HANDLE /*hToken*/, DWORD /*dwFlags*/, LPSTR pszPath)
{
	CreateDirectory(szCustomUserFilesDirectoryInGameDir, NULL);
	strcpy(pszPath, szCustomUserFilesDirectoryInGameDir);
	return S_OK;
}

void __declspec(naked) FOVHook()
{
	_asm
	{
		fld ds : ver3DScale
		mov     eax, [ebp + 8]
		cmp     eax, 1
		jmp jmpAddr2
	}
}

void WINAPI SetWindowLongHook(HWND hWndl, int nIndex, LONG dwNewLong)
{
	dwNewLong |= WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;

	SetWindowLong(hWndl, nIndex, dwNewLong);
	SetWindowPos(hWndl, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

DWORD WINAPI Init(LPVOID)
{
	CIniReader iniReader("");
	ResX = iniReader.ReadInteger("MAIN", "ResX", 0);
	ResY = iniReader.ReadInteger("MAIN", "ResY", 0);
	bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
	bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;
	bHudMode = iniReader.ReadInteger("MAIN", "HudMode", 1) != 0;
	bool bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) != 0;
	bool bFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1) != 0;
	float fPiPPos1 = iniReader.ReadFloat("HUD", "PiPPos1", 0.203125f);
	float fPiPPos2 = iniReader.ReadFloat("HUD", "PiPPos2", 0.001953f);
	float fRearviewMirrorPos1 = iniReader.ReadFloat("HUD", "RearviewMirrorPos1", -0.308594f);
	float fRearviewMirrorPos2 = iniReader.ReadFloat("HUD", "RearviewMirrorPos2", 0.654053f);
	int nWindowedMode = iniReader.ReadInteger("MISC", "WindowedMode", 0);
	bool bSkipIntro = iniReader.ReadInteger("MISC", "SkipIntro", 1) != 0;
	bool bLightingFix = iniReader.ReadInteger("MISC", "LightingFix", 0) != 0;
	bool bCarShadowFix = iniReader.ReadInteger("MISC", "CarShadowFix", 0) != 0;
	bool bXbox360Scaling = iniReader.ReadInteger("MAIN", "Xbox360Scaling", 1) != 0;
	szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "0");
	bool bCustomUsrDir = false;
	if (iniReader.ReadInteger("MISC", "CustomUserFilesDirectoryInGameDir", 0) != 0)
		bCustomUsrDir = true;

	if (!ResX || !ResY) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		ResX = info.rcMonitor.right - info.rcMonitor.left;
		ResY = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	auto pattern = hook::pattern("C7 00 80 02 00 00 C7 01 E0 01 00 00 C2 08 00");
	if (!(pattern.size() > 0) && !bDelay)
	{
		bDelay = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
		return 0;
	}

	if (bDelay)
	{
		while (!(pattern.size() > 0))
			pattern = hook::pattern("C7 00 80 02 00 00 C7 01 E0 01 00 00 C2 08 00");
	}

	for (size_t i = 0; i < 2; i++)
	{
		//addresses from MW
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

	DWORD* dword_9E9B68 = *hook::pattern("D8 0D ? ? ? ? DA 74 24 18 E8 ? ? ? ? 89 46 04 EB 03").get(0).get<DWORD*>(2);
	injector::WriteMemory<float>(dword_9E9B68, (float)ResX, true);
	DWORD* dword_9E8F84 = *hook::pattern("D8 0D ? ? ? ? DA 74 24 10 DA 64 24 14 E8 ? ? ? ? 89 46 08").get(0).get<DWORD*>(2);
	injector::WriteMemory<float>(dword_9E8F84, (float)ResY, true);

	//HUD
	if (bFixHUD)
	{
		fHudScaleX = (1.0f / ResX * (ResY / 480.0f)) * 2.0f;
		fHudPosX = 640.0f / (640.0f * fHudScaleX);

		DWORD* dword_9E8F8C = *hook::pattern("D8 0D ? ? ? ? D8 25 ? ? ? ? D9 5C 24 20 D9 46 04 D8 0D ? ? ? ? D8 25 ? ? ? ? D9 E0").get(0).get<DWORD*>(2);
		injector::WriteMemory<float>(dword_9E8F8C, fHudScaleX, true);

		//fHudScaleY = *(float*)0x9E8F88;
		//injector::WriteMemory<float>(0x9E8F88, fHudScaleY);

		for (size_t i = 0; i < 4; i++)
		{
			DWORD* dword_598DC0 = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43").get(0).get<DWORD>(7);
			injector::WriteMemory<float>(dword_598DC0, fHudPosX, true);
		}

		for (size_t i = 0; i < 2; i++)
		{
			DWORD* dword_5A18BA = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43").get(0).get<DWORD>(4);
			injector::WriteMemory<float>(dword_5A18BA, fHudPosX, true);
		}

		DWORD* dword_A604AC = *hook::pattern("D8 05 ? ? ? ? 89 44 24 18 D9 44 24 18").get(0).get<DWORD*>(2);
		injector::WriteMemory<float>(dword_A604AC, fHudPosX, true);

		DWORD* dword_9C778C = *hook::pattern("D8 25 ? ? ? ? 8D 46 34 50 D9 5C 24 20").get(0).get<DWORD*>(2);
		injector::WriteMemory<float>(dword_9C778C, fHudPosX, true);

		//mini_map_route fix
		DWORD* dword_9D5F3C = *hook::pattern("D8 05 ? ? ? ? D9 5C 24 7C D9 86 B8 00 00 00 D8 05 ? ? ? ? D9 9C 24 80 00 00 00").get(0).get<DWORD*>(2);
		injector::WriteMemory<float>(dword_9D5F3C, (fHudPosX - 320.0f) + 384.0f, true);
	}

	if (bFixFOV)
	{
		hor3DScale = 1.0f / ((1.0f * ((float)ResX / (float)ResY)) / (4.0f / 3.0f));
		ver3DScale = 0.75f;

		DWORD* dword_71B8DA = hook::pattern("D8 3D ? ? ? ? D9 5C 24 30 DB 44 24 20 D8 4C 24 2C").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_71B8DA, &hor3DScale, true);

		DWORD* dword_71B858 = hook::pattern("DB 05 ? ? ? ? 8B 45 08 83 F8 01 DA 35 ? ? ? ? D9 5C 24 24").get(0).get<DWORD>(0);
		jmpAddr2 = (DWORD)dword_71B858 + 18;
		injector::MakeJMP(dword_71B858, FOVHook, true);

		// FOV being different in menus and in-game fix
		static float f06 = 0.6f;
		DWORD* dword_71B8EC = hook::pattern("D8 0D ? ? ? ? E8 ? ? ? ? 8B E8 55 E8").get(1).get<DWORD>(2);
		injector::WriteMemory(dword_71B8EC, &f06, true);

		static float f1234 = 1.25f;
		DWORD* dword_71B923 = hook::pattern("D8 3D ? ? ? ? D9 5C 24 2C 8B 54 24 2C 52 50 55").get(0).get<DWORD>(2);
		injector::WriteMemory(dword_71B923, &f1234, true);

		if (bXbox360Scaling)
		{
			hor3DScale /= 1.0511562719f;
		}
	}

	if (bHudWidescreenMode)
	{
		DWORD* dword_5DC508 = hook::pattern("0F 95 C1 3A C1 75 2B 8B 0D ? ? ? ? 3B CE").get(0).get<DWORD>(1);
		injector::WriteMemory<uchar>(dword_5DC508, 0x94, true);
		DWORD* dword_5D52B3 = hook::pattern("0F 95 C0 3A C8 0F 84 C7 00 00 00 84 C0 88 86 41 03 00 00").get(0).get<DWORD>(1);
		injector::WriteMemory<uchar>(dword_5D52B3, 0x94, true);
		DWORD* dword_5B6BAE = hook::pattern("74 07 68 ? ? ? ? EB 05 68 ? ? ? ? E8 ? ? ? ? 8B ? ? ? ? ? 8B E8 8B 03 83 C4 04").get(0).get<DWORD>(0);
		injector::WriteMemory<uchar>(dword_5B6BAE, 0x75, true);
		DWORD* dword_5B6B5B = hook::pattern("74 07 68 ? ? ? ? EB 05 68 ? ? ? ? E8 ? ? ? ? 8B ? ? ? ? ? 89 44 24 14 8B 03 83 C4 04").get(0).get<DWORD>(0);
		injector::WriteMemory<uchar>(dword_5B6B5B, 0x75, true);

		//PiP/rearview mirror
		DWORD* dword_750CF3 = hook::pattern("68 00 00 D0 BE 68 89 88 50 3F 8D 44 24 10 68 00 00 34 3F").get(0).get<DWORD>(1);
		injector::WriteMemory<float>(dword_750CF3, fRearviewMirrorPos1, true);
		DWORD dword_750D01 = (DWORD)dword_750CF3 + 14;
		injector::WriteMemory<float>(dword_750D01, fRearviewMirrorPos2, true);

		DWORD* dword_750DE7 = hook::pattern("68 CD CC 8C 3E 68 33 33 33 3F 8D 54 24 10 68 F4 FD D4 3C 52").get(0).get<DWORD>(1);
		injector::WriteMemory<float>(dword_750DE7, fPiPPos1, true);
		DWORD dword_750DF5 = (DWORD)dword_750DE7 + 14;
		injector::WriteMemory<float>(dword_750DF5, fPiPPos2, true);
	}

	if (bFMVWidescreenMode)
	{
		DWORD* dword_598EB9 = hook::pattern("68 00 00 80 3F 68 00 00 00 3F 68 00 00 00 3F 68 00 00 00 BF 68 00 00 00 BF 8B CB E8 ? ? ? ? 8B CB C7").get(0).get<DWORD>(6);
		injector::WriteMemory<float>(dword_598EB9 + 0, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
		injector::WriteMemory<float>((DWORD)dword_598EB9 + 5, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
		injector::WriteMemory<float>((DWORD)dword_598EB9 + 10, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
		injector::WriteMemory<float>((DWORD)dword_598EB9 + 15, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
	}

	if (bLightingFix)
	{
		DWORD* dword_7497B9 = hook::pattern("8B 0D ? ? ? ? 83 F8 06 89 0D ? ? ? ? C7 05 ? ? ? ? CD CC CC 3E").get(1).get<DWORD>(2);
		static float f25 = 2.5f;
		injector::WriteMemory((DWORD)dword_7497B9, &f25, true);
		static float f15 = 1.5f;
		DWORD* dword_748A70 = hook::pattern("A1 ? ? ? ? A3 ? ? ? ? 83 3D ? ? ? ? 06 C7 05 ? ? ? ? CD CC CC 3E").get(1).get<DWORD>(1);
		injector::WriteMemory(dword_748A70, &f15, true);
		DWORD* dword_73E7CB = *hook::pattern("C7 05 ? ? ? ? CD CC CC 3E B8 ? ? ? ? 74 05 B8 ? ? ? ? C3").get(0).get<DWORD*>(11);
		injector::WriteMemory<float>(dword_73E7CB, 1.6f, true);
	}

	if (bCarShadowFix)
	{
		DWORD* dword_7BEA3A = hook::pattern("D8 05 ? ? ? ? DC C0 E8 ? ? ? ? 85 C0 7F 04 33 C0").get(0).get<DWORD>(2);
		static float f60 = 60.0f;
		injector::WriteMemory(dword_7BEA3A, &f60, true);
	}

	if (bCustomUsrDir)
	{
		char			moduleName[MAX_PATH];
		GetModuleFileName(NULL, moduleName, MAX_PATH);
		char* tempPointer = strrchr(moduleName, '\\');
		*(tempPointer + 1) = '\0';
		strcat(moduleName, szCustomUserFilesDirectoryInGameDir);
		strcpy(szCustomUserFilesDirectoryInGameDir, moduleName);

		auto pattern = hook::pattern("50 6A 00 6A 00 68 ? 80 00 00 6A 00");
		for (size_t i = 0; i < pattern.size(); i++)
		{
			DWORD* dword_6CBF17 = pattern.get(i).get<DWORD>(12);
			if (*(BYTE*)dword_6CBF17 != 0xFF)
				dword_6CBF17 = pattern.get(i).get<DWORD>(14);

			injector::MakeCALL((DWORD)dword_6CBF17, SHGetFolderPathAHook, true);
			injector::MakeNOP((DWORD)dword_6CBF17 + 5, 1, true);
		}
	}

	if (nWindowedMode)
	{
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		int DesktopResX = info.rcMonitor.right - info.rcMonitor.left;
		int DesktopResY = info.rcMonitor.bottom - info.rcMonitor.top;

		static tagRECT REKT;
		REKT.left = (LONG)(((float)DesktopResX / 2.0f) - ((float)ResX / 2.0f));
		REKT.top = (LONG)(((float)DesktopResY / 2.0f) - ((float)ResY / 2.0f));
		REKT.right = (LONG)(REKT.left + ResX);
		REKT.bottom = (LONG)(REKT.top + ResY);

		auto pattern = hook::pattern("A1 ? ? ? ? 33 FF 3B C7 74 ?"); //0xAB0AD4
		injector::WriteMemory(*pattern.get(2).get<uint32_t*>(1), 1, true);

		pattern = hook::pattern("B8 64 00 00 00 89 44 24 28"); //0x7309B4
		injector::MakeNOP(pattern.get(0).get<uint32_t>(27), 3, true);
		struct WindowedMode
		{
			void operator()(injector::reg_pack& regs)
			{
				*(uint32_t*)(regs.esp + 0x28) = REKT.left;
				*(uint32_t*)(regs.esp + 0x2C) = REKT.top;
				regs.eax = REKT.right;
				regs.ecx = REKT.bottom;
			}
		}; injector::MakeInline<WindowedMode>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(16));

		if (nWindowedMode > 1)
		{
			auto pattern = hook::pattern("68 00 00 00 10 6A F0 50"); //0x730B8A
			injector::MakeNOP(pattern.get(0).get<uint32_t>(8), 6, true);
			injector::MakeCALL(pattern.get(0).get<uint32_t>(8), SetWindowLongHook, true);
		}

		if (bSkipIntro)
		{
			auto pattern = hook::pattern("A3 ? ? ? ? A1 ? ? ? ? 6A 20 50"); //0xA9E6D8
			injector::WriteMemory(*pattern.get(2).get<uint32_t*>(1), 1, true);
		}
	}

	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init(NULL);
	}
	return TRUE;
}
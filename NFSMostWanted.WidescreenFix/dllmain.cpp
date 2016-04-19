#include "..\includes\stdafx.h"
#include "..\includes\hooking\Hooking.Patterns.h"
HWND hWnd;
bool bDelay;

bool bFixHUD, bFixFOV;
int ResX;
int ResY;
float hor3DScale, ver3DScale;
float fHudScaleX, fHudScaleY;
float fHudPosX;
float MinimapPosX, MinimapPosY;
DWORD* dword_8F1CA0;
DWORD jmpAddr, jmpAddr2;
char* szCustomUserFilesDirectoryInGameDir;
static float f06 = 0.6f;

HRESULT WINAPI SHGetFolderPathAHook(HWND /*hwnd*/, int /*csidl*/, HANDLE /*hToken*/, DWORD /*dwFlags*/, LPSTR pszPath)
{
	CreateDirectory(szCustomUserFilesDirectoryInGameDir, NULL);
	strcpy(pszPath, szCustomUserFilesDirectoryInGameDir);
	return S_OK;
}

void __declspec(naked) WidescreenHudHook()
{
	_asm
	{
		mov [esi + 0Ch], ebx
		mov byte ptr ds:[esi + 10h], 1
		jmp jmpAddr
	}
}

void __declspec(naked) FOVHook()
{
	_asm
	{
		fld ds : ver3DScale
		jmp jmpAddr2
	}
}

DWORD WINAPI Init(LPVOID)
{
	CIniReader iniReader("");
	ResX = iniReader.ReadInteger("MAIN", "ResX", 0);
	ResY = iniReader.ReadInteger("MAIN", "ResY", 0);
	bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) == 1;
	bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) == 1;
	bool bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) == 1;
	bool bFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1) == 1;
	int nScaling = iniReader.ReadInteger("MAIN", "Scaling", 2);
	int ShadowsRes = iniReader.ReadInteger("MISC", "ShadowsRes", 1024);
	bool bShadowsFix = iniReader.ReadInteger("MISC", "ShadowsFix", 1) == 1;
	bool bRearviewMirrorFix = iniReader.ReadInteger("MISC", "RearviewMirrorFix", 1) == 1;
	szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "");
	bool bCustomUsrDir = false;
	if (strncmp(szCustomUserFilesDirectoryInGameDir, "0", 1) != 0)
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
				DWORD* dword__93D898 = hook::pattern(TempStr).get(0).get<DWORD>(0);
				injector::WriteMemory(dword__93D898, dword_8F1CA0, true);
			}
		}

		//HUD
		if (bFixHUD)
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

			if (bRearviewMirrorFix)
			{
				injector::WriteMemory<float>(dword_6E70C0, (fHudPosX - 320.0f) + 450.0f, true);
				injector::WriteMemory<float>(dword_6E70FF, (fHudPosX - 320.0f) + 450.0f, true);
				injector::WriteMemory<float>(dword_6E70D3, (fHudPosX - 320.0f) + 190.0f, true);
				injector::WriteMemory<float>(dword_6E70E9, (fHudPosX - 320.0f) + 190.0f, true);
			}

			//mouse cursor fix
			static float fCursorFix = 480.0f * ((float)ResX / (float)ResY);
			DWORD* dword_6C9C45 = hook::pattern("D8 0D ? ? ? ? DA 74 24 18 E8 ? ? ? ? 89 46 04").get(0).get<DWORD>(2);
			injector::WriteMemory(dword_6C9C45, &fCursorFix, true);
		}

		if (bFixFOV)
		{
			hor3DScale = 1.0f / ((1.0f * ((float)ResX / (float)ResY)) / (4.0f / 3.0f));
			ver3DScale = 0.75f;

			DWORD* dword_6CF566 = hook::pattern("D8 3D ? ? ? ? D9 5C 24 20 DB 44 24 30 D8 4C 24 2C").get(0).get<DWORD>(2);
			injector::WriteMemory(dword_6CF566, &hor3DScale, true);

			DWORD* dword_6CF4F0 = hook::pattern("DB 40 18 DA 70 14").get(0).get<DWORD>(0);
			jmpAddr2 = (DWORD)dword_6CF4F0 + 6;
			injector::MakeJMP(dword_6CF4F0, FOVHook, true);

			// FOV being different in menus and in-game fix
			DWORD* dword_6CF578 = hook::pattern("D8 0D ? ? ? ? E8 ? ? ? ? 8B F8 57 E8").get(1).get<DWORD>(2);
			injector::WriteMemory(dword_6CF578, &f06, true);

			static float f1234 = 1.234f;
			DWORD* dword_6CF5DC = hook::pattern("D8 3D ? ? ? ? D9 44 24 20 D8 64 24 24").get(0).get<DWORD>(2);
			injector::WriteMemory(dword_6CF5DC, &f1234, true);

			if (nScaling)
			{
				hor3DScale /= 1.0511562719f;
				f1234 = 1.25f;
				if (nScaling == 2)
					f1234 = 1.315f;
			}
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

		if (bShadowsFix)
		{
			//dynamic shadow fix that stops them from disappearing when going into tunnels, under bridges by Aero_
			DWORD* dword_6DE377 = hook::pattern("75 3B C7 05 ? ? ? ? 00 00 80 3F").get(0).get<DWORD>(0);
			injector::MakeNOP(dword_6DE377, 2, true);
			injector::WriteMemory((DWORD)dword_6DE377 + 8, 0, true);

			//Car shadow opacity
			DWORD* dword_8A0E50 = *hook::pattern("D9 05 ? ? ? ? 8B 54 24 70 D9 1A E9 D1").get(0).get<DWORD*>(2);
			injector::WriteMemory(dword_8A0E50, 60.0f, true);
		}

		if (bRearviewMirrorFix)
		{
			DWORD* dword_6CFB72 = hook::pattern("75 66 53 E8 ? ? ? ? 83 C4 04 84 C0 74 59").get(0).get<DWORD>(0);
			injector::MakeNOP(dword_6CFB72, 2, true);
			DWORD* dword_6CFBC5 = hook::pattern("75 0D 53 E8 ? ? ? ? 83 C4 04 84 C0 75 06 89 1D").get(0).get<DWORD>(0);
			injector::MakeNOP(dword_6CFBC5, 2, true);

			DWORD* dword_595DDA = hook::pattern("83 F8 02 74 2D 83 F8 03 74 28 83 F8 04 74 23 83 F8 05 74 1E 83 F8 06 74 19").get(0).get<DWORD>(2);
			injector::WriteMemory<uchar>(dword_595DDA, 4, true);
			injector::WriteMemory<uchar>((DWORD)dword_595DDA + 5, 4, true);

			//scaling
			if (bFixFOV)
			{
				DWORD* dword_476823 = hook::pattern("66 C7 80 C4 00 00 00 20 4E").get(0).get<DWORD>(7);
				injector::WriteMemory<short>(dword_476823, 14750, true);

				f06 = 0.4f;
				DWORD* dword_8AF994 = *hook::pattern("D9 44 24 24 D8 0D ? ? ? ? D9 5C 24 24").get(4).get<DWORD*>(6);
				injector::WriteMemory<float>(dword_8AF994, 1.725f, true);
			}

			//render
			DWORD* dword_4FAEB0 = hook::pattern("75 ? 83 CE 20 8B 7C 24 10 57 52").get(0).get<DWORD>(0);
			injector::WriteMemory<uchar>(dword_4FAEB0, 0xEB, true);

			DWORD* dword_6BFE33 = hook::pattern("74 22 8B 0D ? ? ? ? 85").get(0).get<DWORD>(0);
			injector::MakeNOP(dword_6BFE33, 2, true);

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


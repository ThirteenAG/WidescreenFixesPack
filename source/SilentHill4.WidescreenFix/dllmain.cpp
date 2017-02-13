#include "stdafx.h"

HWND hWnd;
bool bDelay;

struct Screen
{
	int32_t Width;
	int32_t Height;
	float fWidth;
	float fHeight;
	float fFieldOfView;
	float fAspectRatio;
	int32_t Width43;
	float fWidth43;
	float fHudScale;
	float fHudOffset;
	float fFMVScale;
} Screen;

LONG WINAPI RegQueryValueExAHook(HKEY hKey, LPCTSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
	if (strstr(lpValueName, "Install Path") || strstr(lpValueName, "Movie Install"))
	{
		if (lpData == NULL)
		{
			char temp[MAX_PATH];
			GetModuleFileName(NULL, temp, MAX_PATH);
			*(strrchr(temp, '\\') + 1) = '\0';
			*lpcbData = std::size(temp);
			*lpType = 1;
		}
		else
		{
			GetModuleFileName(NULL, (char*)lpData, MAX_PATH);
			*(strrchr((char*)lpData, '\\') + 1) = '\0';
		}
		return ERROR_SUCCESS;
	}
	return RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

LONG WINAPI RegOpenKeyAHook(HKEY hKey, LPCTSTR lpSubKey, PHKEY phkResult)
{
	if (strstr(lpSubKey, "Konami"))
		return ERROR_SUCCESS;
	else
		return RegOpenKeyA(hKey, lpSubKey, phkResult);
}

LONG WINAPI RegSetValueExAHook(HKEY hKey, LPCTSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE *lpData, DWORD cbData)
{
	return ERROR_SUCCESS;
}

LONG WINAPI RegDeleteValueAHook(HKEY hKey, LPCTSTR lpValueName)
{
	return ERROR_SUCCESS;
}

LONG WINAPI RegCloseKeyHook(HKEY hKey)
{
	return ERROR_SUCCESS;
}

injector::hook_back<void(__cdecl*)(float, float, float, float, float, float, float, float, float, float)> hbFMV;
void __cdecl FMVHook(float X1, float Y1, float X2, float Y2, float a5, float a6, float a7, float a8, float a9, float a10)
{
	float fAR = (Screen.fAspectRatio > (16.0f / 9.0f)) ? (16.0f / 9.0f) : Screen.fAspectRatio;
	static float fOffset = ((480.0f * fAR) - 640.0f) / 2.0f; //106.0
	Y1 -= fOffset / (4.0f / 3.0f);
	Y2 += fOffset / (4.0f / 3.0f);
	X1 -= fOffset;
	X2 += fOffset;
	return hbFMV.fun(X1, Y1, X2, Y2, a5, a6, a7, a8, a9, a10);
}

injector::hook_back<void(__cdecl*)(int, float, float, float, int, unsigned int)> hbInGameOverlay;
void __cdecl InGameOverlayHook(int a1, float a2, float a3, float a4, int a5, unsigned int a6)
{
	float fOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;

	return hbInGameOverlay.fun(a1 - fOffset, a2, a3, a4, a5, a6);
}

injector::hook_back<void(__cdecl*)(float, float, float, float, float, float, float, float, float, float)> hbOverlayIntro;
void __cdecl OverlayIntroHook(float X1, float Y1, float X2, float Y2, float a5, float a6, float a7, float a8, float a9, float a10)
{
	float fOffset = ((480.0f * Screen.fAspectRatio) - 800.0f) / 2.0f;
	hbOverlayIntro.fun(X1 - fOffset, Y1, X2, Y2, a5, a6, a7, a8, a9, a10);
}

injector::hook_back<void(__cdecl*)(float, float, float, float, float, float, float, float, float, float)> hbOverlays;
void __cdecl OverlaysHook(float X1, float Y1, float X2, float Y2, float a5, float a6, float a7, float a8, float a9, float a10)
{

	float fOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
	if ((X1 == 0.0f) && (X2 == 640.0f || X2 == 512.0f))
		return hbOverlays.fun(X1 - fOffset, Y1, X2 + (fOffset * 2.0f), Y2, a5, a6, a7, a8, a9, a10);
	else
		return hbOverlays.fun(X1, Y1, X2, Y2, a5, a6, a7, a8, a9, a10);
}

DWORD WINAPI Init(LPVOID)
{
	auto pattern = hook::pattern("BF 94 00 00 00 8B C7");
	if (!(pattern.size() > 0) && !bDelay)
	{
		bDelay = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
		return 0;
	}

	if (bDelay)
	{
		while (!(pattern.size() > 0))
			pattern = hook::pattern("BF 94 00 00 00 8B C7");
	}

	CIniReader iniReader("");
	Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
	Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
	bool bFix2D = iniReader.ReadInteger("MAIN", "Fix2D", 1) != 0;
	bool bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;
	bool bDisableCutsceneBorders = iniReader.ReadInteger("MAIN", "DisableCutsceneBorders", 1) != 0;
	bool bIncreaseBackgroundRes = iniReader.ReadInteger("MAIN", "IncreaseBackgroundRes", 1) != 0;
	bool bCutsceneFrameRateFix = iniReader.ReadInteger("MAIN", "CutsceneFrameRateFix", 1) != 0;
	bool bDisableCheckSpec = iniReader.ReadInteger("MAIN", "DisableCheckSpec", 1) != 0;
	bool bDisableRegistryDependency = iniReader.ReadInteger("MISC", "DisableRegistryDependency", 1) != 0;

	if (!Screen.Width || !Screen.Height) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		Screen.Width = info.rcMonitor.right - info.rcMonitor.left;
		Screen.Height = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	Screen.fWidth = static_cast<float>(Screen.Width);
	Screen.fHeight = static_cast<float>(Screen.Height);
	Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
	Screen.fFieldOfView = ((1.0f / Screen.fAspectRatio) * (4.0f / 3.0f));
	Screen.fHudScale = ((1.0f / Screen.fAspectRatio) * (4.0f / 3.0f));

	//Resolution
	char pattern_str[50];
	union {
		uint32_t* Int;
		unsigned char Byte[4];
	} dword_temp;

	dword_temp.Int = *hook::pattern("8B 0D ? ? ? ? 6A 00 50 51").get(0).get<uint32_t*>(2);
	static auto nWidthPtr = dword_temp.Int;
	sprintf(pattern_str, "%02X %02X %02X %02X %02X %02X %c %c %c %c", 0xC7, 0x05, dword_temp.Byte[0], dword_temp.Byte[1], dword_temp.Byte[2], dword_temp.Byte[3], '?', '?', '?', '?');
	pattern = hook::pattern(pattern_str); //0x4141E3

	for (size_t i = 0; i < pattern.size(); i++)
	{
		injector::WriteMemory(pattern.get(i).get<uint32_t>(6), Screen.Width, true);
	}

	dword_temp.Int = *hook::pattern("8B 0D ? ? ? ? 89 44 24 1C 39").get(0).get<uint32_t*>(2);
	static auto nHeightPtr = dword_temp.Int;
	sprintf(pattern_str, "%02X %02X %02X %02X %02X %02X %c %c %c %c", 0xC7, 0x05, dword_temp.Byte[0], dword_temp.Byte[1], dword_temp.Byte[2], dword_temp.Byte[3], '?', '?', '?', '?');
	pattern = hook::pattern(pattern_str); //0x4141ED

	for (size_t i = 0; i < pattern.size(); i++)
	{
		injector::WriteMemory(pattern.get(i).get<uint32_t>(6), Screen.Height, true);
	}

	pattern = hook::pattern("89 35 ? ? ? ? 33 C0 5E C3"); //0x414A38
	struct SetResHook
	{
		void operator()(injector::reg_pack&)
		{
			*nWidthPtr  = Screen.Width;
			*nHeightPtr = Screen.Height;
		}
	}; injector::MakeInline<SetResHook>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(6));

	if (bFixFOV)
	{
		pattern = hook::pattern("83 C4 14 68 ? ? ? ? E8 ? ? ? ? 68"); //0x55BC74 
		injector::MakeNOP(pattern.get(0).get<uint32_t>(8), 5, true);
		injector::WriteMemory<float>(pattern.get(0).get<uint32_t>(14), Screen.fFieldOfView, true);

		pattern = hook::pattern("C7 46 48 ? ? ? ? C7 46 44"); //0x41D295
		injector::WriteMemory<float>(pattern.get(0).get<uint32_t>(3), 0.78125f * Screen.fHudScale, true);
	}

	if (bFix2D)
	{
		static float fMenuScale = 0.00390625f * Screen.fHudScale;
		pattern = hook::pattern("D8 0D ? ? ? ? 6A 01 8D 54 24 1C 52 D9 5C 24 10 6A 5E"); //0x404E4C + 2
		injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &fMenuScale, true); // Menu Width

		// Text fix
		pattern = hook::pattern("C7 44 24 34 00 00 80 BF D9 5C"); //0x404EBB
		injector::WriteMemory<float>(pattern.get(0).get<uint32_t>(4), -1.0f * Screen.fHudScale, true); // Text X Pos
		pattern = hook::pattern("C7 44 24 24 CD CC 4C 3B D9 44 24 14"); //0x404E91
		injector::WriteMemory<float>(pattern.get(0).get<uint32_t>(4), 0.003125f * Screen.fHudScale, true); // Text Width

		//FMV
		pattern = hook::pattern("E8 ? ? ? ? 8B 14 B5 ? ? ? ? A1"); //0x412EAF
		hbFMV.fun = injector::MakeCALL(pattern.get(0).get<uint32_t>(0), FMVHook, true).get();

		//Width
		//sub_563BF0
		auto sub_563BF0 = (uint32_t)hook::pattern("8B 44 24 04 53 8B 5C 24 18 55 8B").get(0).get<uint32_t>(0);
		pattern = hook::pattern("E8 ? ? ? ?");
		for (size_t i = 0, j = 1; i < pattern.size(); ++i)
		{
			auto addr = pattern.get(i).get<uint32_t>(0);
			auto dest = injector::GetBranchDestination(addr, true).as_int();
			if (dest == sub_563BF0)
			{
				j++;

				if (j == 33 || j == 71) //menu and save menu backgrounds, 1 -> 0x4053C8... http://pastebin.com/Hv6TdTLh
					continue;

				if (j < 44 || j > 48)
					hbOverlays.fun = injector::MakeCALL(pattern.get(i).get<uint32_t>(0), OverlaysHook, true).get();
				else
					hbOverlayIntro.fun = injector::MakeCALL(pattern.get(i).get<uint32_t>(0), OverlayIntroHook, true).get();
			}
		}

		pattern = hook::pattern("52 6A 00 E8 ? ? ? ? 83 C4 1C C3"); //0x563CC0
		//hbInGameOverlay.fun = injector::MakeCALL(pattern.get(0).get<uint32_t>(3), InGameOverlayHook, true).get();
		hbInGameOverlay.fun = injector::MakeCALL(0x433CAF/*pattern.get(0).get<uint32_t>(3)*/, InGameOverlayHook, true).get();
		hbInGameOverlay.fun = injector::MakeCALL(0x433D2D/*pattern.get(0).get<uint32_t>(3)*/, InGameOverlayHook, true).get();
		
		//Overlay 1 (intro)
		//pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 34 8B 4C 24 38 6A 01"); //0x566B2A
		//hbOverlayIntro.fun = injector::MakeCALL(pattern.get(0).get<uint32_t>(0), OverlayIntroHook, true).get();

		//Overlay 2 (cutscenes)
		//pattern = hook::pattern(""); //0x565DFB
		//hbOverlay1.fun = injector::MakeCALL(0x566B15, Overlay1Hook).get();
		//injector::MakeCALL(0x565DFB, Overlay1Hook, true);

		//Overlay 3 (transition)
		//pattern = hook::pattern("E8 ? ? ? ? 6A 00 8B 44 24 34 6A 01 8B 4C 24 34 6A 00 6A 00"); //0x5660C5
		//hbOverlay1.fun = injector::MakeCALL(0x566B15, Overlay1Hook).get();

		//Cutscene Borders
		static float fBorderWidth = 4096.0f;
		pattern = hook::pattern("DB 05 ? ? ? ? D9 5C 24 08 DB 05 ? ? ? ? D9 5C 24 04 DB 05 ? ? ? ? D9 1C 24"); //0x4F4926
		injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &fBorderWidth, true);
		injector::WriteMemory(pattern.get(1).get<uint32_t>(2), &fBorderWidth, true);
		static float fBorderPosX = -2048.0f;
		injector::WriteMemory(pattern.get(0).get<uint32_t>(22), &fBorderPosX, true);
		injector::WriteMemory(pattern.get(1).get<uint32_t>(22), &fBorderPosX, true);

		//Map Zoom
		static float fMapZoom = (0.5f / ((4.0f / 3.0f) / (Screen.fAspectRatio)));
		pattern = hook::pattern("D8 0D ? ? ? ? 51 52 6A 00 D9 5C"); //0x0051D6DB
		injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &fMapZoom, true);

		//Film Effect (loading animation & bathroom hole)
		static float fFilmEffectWidth = 0.1f * ((4.0f / 3.0f) / (Screen.fAspectRatio));
		pattern = hook::pattern("D8 0D ? ? ? ? D8 44 24 14 D9 1C 24 E8 ? ? ? ? 89 44 24 30"); //0x0056807F
		injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &fFilmEffectWidth, true);
		pattern = hook::pattern("D8 0D ? ? ? ? 8B 44 24 38 89 44 24 34 83 C4 28"); //0x005680F3 
		injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &fFilmEffectWidth, true);
		pattern = hook::pattern("D8 0D ? ? ? ? D8 44 24 14 D9 1C 24 E8 ? ? ? ? 89 44 24 38"); //0x00568193 
		injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &fFilmEffectWidth, true);
		pattern = hook::pattern("D8 0D ? ? ? ? 8B 4C 24 38 83 C4 28 89 4C 24 0C D8 44 24 04"); //0x00568207 
		injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &fFilmEffectWidth, true);

		static float fFilmEffectPos = 0.0f - ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
		pattern = hook::pattern("C7 44 24 0C 00 00 00 00 C7 44 24 04 00 00 00 00 DF E0"); //0x00567FF8 
		injector::WriteMemory(pattern.get(0).get<uint32_t>(4), &fFilmEffectPos, true);
		//injector::WriteMemory(pattern.get(0).get<uint32_t>(12), &fFilmEffectPos, true);
	}

	if (bDisableCutsceneBorders)
	{
		static uint32_t nOff = 0;
		pattern = hook::pattern("A1 ? ? ? ? 85 C0 74 ? 6A 02 68"); //0x4F4A0A
		injector::WriteMemory(pattern.get(0).get<uint32_t>(1), &nOff, true);
		injector::WriteMemory(pattern.get(1).get<uint32_t>(1), &nOff, true);
		//injector::WriteMemory(0x565053+1, 2048 - 48, true); //borders pos
	}

	if (bIncreaseBackgroundRes)
	{
		pattern = hook::pattern("B8 ? ? ? ? 89 4C 24 28 8B 0D ? ? ? ? 89 44 24 2C"); //0x403320
		injector::WriteMemory(pattern.get(0).get<uint32_t>(1), Screen.Width, true);
		pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 51 E8 ? ? ? ? 8B 15 ? ? ? ? 83 C4 50"); //0x40362E
		injector::WriteMemory(pattern.get(0).get<uint32_t>(1), Screen.Width, true);
		injector::WriteMemory(pattern.get(0).get<uint32_t>(6), Screen.Width, true);

		pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 89 35 7C 39 2E 01"); //0055A699
		injector::WriteMemory<float>(pattern.get(0).get<uint32_t>(0x06), Screen.fWidth - 0.5f, true);
		injector::WriteMemory<float>(pattern.get(0).get<uint32_t>(0x52), Screen.fWidth - 0.5f, true);
		injector::WriteMemory<float>(pattern.get(0).get<uint32_t>(0x8A), Screen.fWidth - 0.5f, true);
		injector::WriteMemory<float>(pattern.get(0).get<uint32_t>(0x94), Screen.fWidth - 0.5f, true);
	}

	if (bCutsceneFrameRateFix)
	{
		pattern = hook::pattern("0F 94 C0 A3 ? ? ? ? EB ? C3"); //0x004EFA85
		injector::WriteMemory<uint8_t>(pattern.get(0).get<uint32_t>(1), 0x95, true);
	}

	if (bDisableCheckSpec)
	{
		pattern = hook::pattern("8B 15 ? ? ? ? A1 ? ? ? ? 8B 0D ? ? ? ? 89 17 66"); //0x00414688 
		injector::MakeNOP(pattern.get(0).get<uint32_t>(0), 6, true);
	}

	if (bDisableRegistryDependency)
	{
		auto RegIATpat = hook::pattern("FF 15 ? ? ? ? 8B 44 24 00 50");
		if (RegIATpat.size() > 0)
		{
			uintptr_t* RegIAT = *RegIATpat.get(0).get<uintptr_t*>(2); //0x413D67
			injector::WriteMemory(&RegIAT[0], RegQueryValueExAHook, true);
			injector::WriteMemory(&RegIAT[1], RegOpenKeyAHook, true);
			injector::WriteMemory(&RegIAT[2], RegSetValueExAHook, true);
			injector::WriteMemory(&RegIAT[3], RegDeleteValueAHook, true);
			injector::WriteMemory(&RegIAT[4], RegCloseKeyHook, true);
		}
	}

	//double vision issue
	pattern = hook::pattern("50 51 E8 ? ? ? ? 83 C4 10 C3"); //0x565AEC
	injector::MakeNOP(pattern.get(0).get<uint32_t>(2), 5, true);

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
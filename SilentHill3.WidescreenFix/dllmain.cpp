#include "..\includes\stdafx.h"
#include "..\includes\hooking\Hooking.Patterns.h"

HWND hWnd;
bool bDelay;
bool bConfigTool;

struct Screen
{
	int Width;
	int Height;
	float fWidth;
	float fHeight;
	float fFieldOfView;
	float fAspectRatio;
	float fHudOffset;
	int Width43;
} Screen;

LONG WINAPI RegQueryValueExAHook(HKEY hKey, LPCTSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
	if (strstr(lpValueName, "movie") || strstr(lpValueName, "sound") || strstr(lpValueName, "data") || strstr(lpValueName, "save") || strstr(lpValueName, "installdir"))
	{
		if (lpData == NULL)
		{
			char temp[MAX_PATH];
			GetModuleFileName(NULL, temp, MAX_PATH);
			*(strrchr(temp, '\\') + 1) = '\0';
			strstr(lpValueName, "save") ? strcat_s(temp, "savedata\\") : 0;
			*lpcbData = std::size(temp);
			*lpType = 1;
		}
		else
		{
			GetModuleFileName(NULL, (char*)lpData, MAX_PATH);
			*(strrchr((char*)lpData, '\\') + 1) = '\0';
			strstr(lpValueName, "save") ? strcat((char*)lpData, "savedata\\") : 0;
		}
		return ERROR_SUCCESS;
	}
	return RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}


LONG WINAPI RegOpenKeyExAHook(HKEY hKey, LPCTSTR lpSubKey, DWORD ulOptions, REGSAM  samDesired, PHKEY phkResult)
{
	if (strstr(lpSubKey, "KONAMI"))
		return ERROR_SUCCESS;

	return RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

LONG WINAPI RegDeleteKeyAHook(HKEY hKey, LPCTSTR lpSubKey)
{
	return ERROR_SUCCESS;
}

LONG WINAPI RegEnumKeyAHook(HKEY hKey, DWORD dwIndex, LPTSTR lpName, DWORD cchName)
{
	return ERROR_SUCCESS;
}

LONG WINAPI RegOpenKeyAHook(HKEY hKey, LPCTSTR lpSubKey, PHKEY phkResult)
{
	return ERROR_SUCCESS;
}

LONG WINAPI RegQueryValueAHook(HKEY hKey, LPCTSTR lpSubKey, LPTSTR lpValue, PLONG lpcbValue)
{
	return ERROR_SUCCESS;
}

LONG WINAPI RegCreateKeyExAHook(HKEY hKey, LPCTSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{
	return ERROR_SUCCESS;
}

LONG WINAPI RegSetValueExAHook(HKEY hKey, LPCTSTR lpValueName,DWORD Reserved, DWORD dwType, const BYTE *lpData, DWORD cbData)
{
	return ERROR_SUCCESS;
}

LONG WINAPI RegCloseKeyHook(HKEY hKey)
{
	return ERROR_SUCCESS;
}

void PatchRegistryFuncs()
{
	auto RegIATpat = hook::pattern("50 53 53 FF 75 F4 C7 45 F0 10 00 00 00 FF 15");
	if (RegIATpat.size() > 0)
	{
		uintptr_t* RegIAT = *RegIATpat.get(0).get<uintptr_t*>(15); //0x4232E4 in config tool
		injector::WriteMemory(&RegIAT[0], RegQueryValueExAHook, true);
		injector::WriteMemory(&RegIAT[1], RegOpenKeyExAHook, true);
		injector::WriteMemory(&RegIAT[2], RegDeleteKeyAHook, true);
		injector::WriteMemory(&RegIAT[3], RegEnumKeyAHook, true);
		injector::WriteMemory(&RegIAT[4], RegOpenKeyAHook, true);
		injector::WriteMemory(&RegIAT[5], RegQueryValueAHook, true);
		injector::WriteMemory(&RegIAT[6], RegCreateKeyExAHook, true);
		injector::WriteMemory(&RegIAT[7], RegSetValueExAHook, true);
		injector::WriteMemory(&RegIAT[8], RegCloseKeyHook, true);
		bConfigTool = true;
	}
	else
	{
		RegIATpat = hook::pattern("68 02 00 00 80 FF 15");
		if (RegIATpat.size() > 0)
		{
			uintptr_t* RegIAT = *RegIATpat.get(0).get<uintptr_t*>(7); //0x5E9709 in game exe
			injector::WriteMemory(&RegIAT[0], RegOpenKeyExAHook, true);
			injector::WriteMemory(&RegIAT[1], RegQueryValueExAHook, true);
			injector::WriteMemory(&RegIAT[2], RegOpenKeyAHook, true);
			injector::WriteMemory(&RegIAT[3], RegCloseKeyHook, true);
		}
	}
}

DWORD WINAPI Init(LPVOID)
{
	auto pattern = hook::pattern("89 65 E8 8B F4 89 3E 56");
	if (!(pattern.size() > 0) && !bDelay)
	{
		bDelay = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
		return 0;
	}

	if (bDelay)
	{
		while (!(pattern.size() > 0))
			pattern = hook::pattern("89 65 E8 8B F4 89 3E 56");
	}

	PatchRegistryFuncs();
	if (bConfigTool)
		return 0;

	CIniReader iniReader("");
	Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
	Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
	static bool bFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1) != 0;
	bool bDisableCutsceneBorders = iniReader.ReadInteger("MISC", "DisableCutsceneBorders", 1) != 0;
	bool bDisableSafeMode = iniReader.ReadInteger("MISC", "DisableSafeMode", 1) != 0;
	uint32_t nStatusScreenRes = iniReader.ReadInteger("MISC", "StatusScreenRes", 512);
	uint32_t nShadowsRes = iniReader.ReadInteger("MISC", "ShadowsRes", 1024);
	uint32_t nDOFRes = iniReader.ReadInteger("MISC", "DOFRes", 1024);

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
	//Screen.fHudOffset = (0.5f / ((4.0f / 3.0f) / (Screen.fAspectRatio)));
	//Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));


	//Resolution
	static int32_t* ResXAddr1; 
	static int32_t* ResXAddr2;
	static int32_t* ResXAddr3;
	static int32_t* ResXAddr4;
	static int32_t* ResYAddr1;
	static int32_t* ResYAddr2;
	static int32_t* ResYAddr3;
	static int32_t* ResYAddr4;
	static float FMVOffset1;
	static float FMVOffset2;
	static float FMVOffset3;

	struct SetResHook
	{
		void operator()(injector::reg_pack&)
		{
			//*ResXAddr1 = Screen.Width;
			//*ResYAddr1 = Screen.Height;
			*ResXAddr2 = Screen.Width;
			*ResYAddr2 = Screen.Height;
			*ResXAddr3 = Screen.Width;
			*ResYAddr3 = Screen.Height;
			*ResXAddr4 = Screen.Width;
			*ResYAddr4 = Screen.Height;

			if (!bFMVWidescreenMode)
				FMVOffset1 = (float)*ResXAddr1 / (Screen.fWidth / ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f));
			else
				FMVOffset1 = (float)*ResXAddr1 / (Screen.fWidth / ((Screen.fWidth - Screen.fHeight * (964.0f / 622.0f)) / 2.0f));
			FMVOffset2 = *ResXAddr1 - FMVOffset1;
			FMVOffset3 = (float)*ResYAddr1;
		}
	};
	
	ResXAddr1 = *hook::pattern("A1 ? ? ? ? 53 8B 5C 24 0C 3B C3 55 8B 6C 24 14").get(0).get<int32_t*>(1); //72BFD0 rendering res
	ResYAddr1 = ResXAddr1 + 1; //72BFD4
	ResXAddr2 = *hook::pattern("BF ? ? ? ? F3 AB 52").get(0).get<int32_t*>(1); //0072C664 
	ResYAddr2 = ResXAddr2 + 1; //0072C668
	ResXAddr3 = *hook::pattern("8B 0D ? ? ? ? 8B 44 24 04 3B C8 8B 4C 24 08 75 08 39 0D").get(0).get<int32_t*>(2); //0072C780 
	ResYAddr3 = ResXAddr3 + 1; //0072C784
	ResXAddr4 = ResYAddr3 + 1; //0072C788 
	ResYAddr4 = ResXAddr4 + 1; //0072C78C

	//pattern = hook::pattern("A3 ? ? ? ? 89 0D ? ? ? ? 33 C0 C3");
	//injector::MakeInline<SetResHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(11)); //402B68
	//pattern = hook::pattern("89 1D ? ? ? ? 89 2D ? ? ? ? E8 ? ? ? ? 8B");
	//injector::MakeInline<SetResHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(12)); //402E3A
	//pattern = hook::pattern("89 35 ? ? ? ? 89 3D ? ? ? ? E8 ? ? ? ? 8B");
	//injector::MakeInline<SetResHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(12)); //402E61
	//pattern = hook::pattern("A3 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? 83 C4 04");
	//injector::MakeInline<SetResHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(10)); //402E93


	pattern = hook::pattern("89 15 ? ? ? ? 8B 10 50 FF 52 3C 85 C0");
	injector::MakeInline<SetResHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6)); //419622
	pattern = hook::pattern("89 0D ? ? ? ? 89 15 ? ? ? ? E8");
	injector::MakeInline<SetResHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(12)); //4197DE
	pattern = hook::pattern("89 35 ? ? ? ? 5E 33 C0 5B 83 C4 10");
	injector::MakeInline<SetResHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6)); //419804


	pattern = hook::pattern("A3 ? ? ? ? 89 0D ? ? ? ? A3 ? ? ? ? 89 0D ? ? ? ? C6 05 ? ? ? ? 01");
	injector::MakeInline<SetResHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(22)); //416AFA
	pattern = hook::pattern("89 0D ? ? ? ? 89 0D ? ? ? ? C6 05 ? ? ? ? 01 33 C0 C3");
	injector::MakeInline<SetResHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(12)); //416B4D
	pattern = hook::pattern("A3 ? ? ? ? 89 0D ? ? ? ? A3 ? ? ? ? C7 05 ? ? ? ? 3C 00");
	injector::MakeInline<SetResHook>(pattern.get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(5+11)); //418C57


	pattern = hook::pattern("A3 ? ? ? ? 89 0D ? ? ? ? 33 C0 C3");
	injector::MakeInline<SetResHook>(pattern.get(1).get<uint32_t>(0), pattern.get(1).get<uint32_t>(11)); //416B78

	//FOV
	Screen.fFieldOfView = (((4.0f / 3.0f) / (Screen.fAspectRatio)));
	pattern = hook::pattern("D9 05 ? ? ? ? D8 0D ? ? ? ? C3"); //43AB95
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &Screen.fFieldOfView, true);

	//Removes the black bars visible on the side of the screen
	static float f0 = 0.0f;
	pattern = hook::pattern("D8 0D ? ? ? ? DC C0 D9 54 24 0C D9 5C 24 04 E8"); //41BE64
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &f0, true);
	pattern = hook::pattern("D8 0D ? ? ? ? A3 ? ? ? ? A1 ? ? ? ? 6A 00 D9 5C 24 14"); //41BB2D
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &f0, true);

	//FMVs
	SetResHook();
	pattern = hook::pattern("D9 05 ? ? ? ? D8 25 ? ? ? ? 83 C8 FF"); //4045B9
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &FMVOffset1, true);
	pattern = hook::pattern("D9 05 ? ? ? ? C7 44 24 0C 00 00 00 00 D8 25"); //004045C8 
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &FMVOffset2, true);

	if (bFMVWidescreenMode)
	{
		static uint32_t f0 = 0.0f;
		pattern = hook::pattern("D9 05 ? ? ? ? 89 44 24 14 D8 25 ? ? ? ? C7"); //4045E2
		injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &f0, true);
		
		pattern = hook::pattern("D9 05 ? ? ? ? D8 25 ? ? ? ? D9 C3 D9 5C"); //404606
		injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &FMVOffset3, true);
	}

	//Menus
	//Mouse Cursor Fix
	pattern = hook::pattern("6A 02 E8 ? ? ? ? 83 C4 04 85 C0 74 07");
	injector::WriteMemory<uint8_t>(pattern.get(0).get<uint32_t>(12), 0xEB, true); // 43AB8C
	injector::WriteMemory<uint8_t>(pattern.get(1).get<uint32_t>(12), 0xEB, true); // 43ABCC
	injector::MakeCALL(hook::pattern("E8 ? ? ? ? D9 9D 9C FE FF FF E8 ? ? ? ? 8D 85 5C FE").get(0).get<uint32_t>(0), pattern.get(1).get<uint32_t>(0), true); //4DDB94 call 0043ABC0

	//Menu Width
	static float fMenuWidthFactor = (1.0f / (480.0f * (Screen.fAspectRatio / 10.0f))) / 2.0f / 2.0f / 2.0f;
	static float fMenuXPos = 256.0f * (Screen.fAspectRatio / (4.0f / 3.0f));
	pattern = hook::pattern("DE C9 D8 0D ? ? ? ? 59 C3"); //682BBE
	injector::WriteMemory(pattern.get(2).get<uint32_t>(4), &fMenuWidthFactor, true);
	injector::WriteMemory(pattern.get(2).get<uint32_t>(-4), &fMenuXPos, true);
	
	pattern = hook::pattern("51 E8 ? ? ? ? 89 44 24 00 DB 44 24 00 DB 44");
	uintptr_t dword_413510 = (uintptr_t)pattern.get(0).get<uintptr_t>(0);
	uintptr_t dword_682BA0 = (uintptr_t)pattern.get(2).get<uintptr_t>(0);
	injector::MakeCALL(dword_413510 + 1, hook::pattern("A1 ? ? ? ? 85 C0 74 06 A1 ? ? ? ? C3 E9").get(0).get<uintptr_t>(0)); //402BA0
	pattern = hook::pattern("E8 ? ? ? ?");
	for (size_t i = 0, j = 0; i < pattern.size(); ++i) //http://pastebin.com/p03E9Vw1
	{
		if (injector::GetBranchDestination(pattern.get(i).get<uintptr_t>(0), true).as_int() == dword_682BA0)
		{
			if ((j > 0 && j <= 7) || (j >= 22 && j <= 26) || j == 75 || j == 76)
			{
				injector::MakeCALL(pattern.get(i).get<uintptr_t>(0), dword_413510, true);
			}
			++j;
		}
	}

	//inventory background size
	pattern = hook::pattern("B8 00 01 00 00 BA 00 FF FF FF 8B E8"); //5DAFE5
	injector::WriteMemory(pattern.get(0).get<uint32_t>(1), 1024, true);
	injector::WriteMemory(pattern.get(0).get<uint32_t>(6), -1024, true);

	//window style
	pattern = hook::pattern("68 00 03 00 00 FF 15 ? ? ? ? 5E"); //403042
	injector::WriteMemory(pattern.get(0).get<uint32_t>(1), 0, true);

	if (bDisableCutsceneBorders)
	{
		pattern = hook::pattern("D9 05 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00 D8 C9"); //41BB4B
		injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &f0, true);
	}

	if (bDisableSafeMode)
	{
		static char* nullstr = "";
		pattern = hook::pattern("BF 01 00 00 00 68 ? ? ? ? 68 ? ? ? ? 6A 01"); //5F0A39
		injector::WriteMemory(pattern.get(0).get<uint32_t>(1), 0, true);
		injector::WriteMemory(pattern.get(0).get<uint32_t>(6), &nullstr, true);
	}

	if (nStatusScreenRes)
	{
		pattern = hook::pattern("8B 3C 85 ? ? ? ? 8B 04 85 ? ? ? ? 56 6A 00 6A 15 6A 01 6A 01"); //6B1444 //6B1434
		injector::WriteMemory(*pattern.get(0).get<uint32_t*>(3), nStatusScreenRes, true);
		injector::WriteMemory(*pattern.get(0).get<uint32_t*>(10), nStatusScreenRes, true);
		pattern = hook::pattern("BE ? ? ? ? 56 56 89 5C 24 24 89 5C 24 28 89 5C 24 20 8B 08 50"); //41DE57
		injector::WriteMemory(pattern.get(0).get<uint32_t>(1), nStatusScreenRes, true);
		pattern = hook::pattern("BE ? ? ? ? 8D A4 24 00 00 00 00 8B 2C 01 89 28 83 C0 04 4E"); //41E165
		injector::WriteMemory(pattern.get(0).get<uint32_t>(1), nStatusScreenRes, true);

		pattern = hook::pattern("68 ? ? ? ? 6A 02 6A 05 50 FF 91 20 01 00 00 A1 ? ? ? ? 8B 10 6A 01"); 
		uintptr_t dword_6B0D00 = *pattern.get(1).get<uintptr_t>(1);
		injector::WriteMemory(dword_6B0D00 + 0x00, static_cast<float>(nStatusScreenRes), true); //006B0D00
		injector::WriteMemory(dword_6B0D00 + 0x30, static_cast<float>(nStatusScreenRes), true); //006B0D30 
		injector::WriteMemory(dword_6B0D00 + 0x34, static_cast<float>(nStatusScreenRes), true); //006B0D34 
		injector::WriteMemory(dword_6B0D00 + 0x4C, static_cast<float>(nStatusScreenRes), true); //006B0D4C 
	}

	if (nShadowsRes)
	{
		pattern = hook::pattern("C7 05 ? ? ? ? 00 00 80 43");
		for (size_t i = 0; i < pattern.size(); ++i) //http://pastebin.com/geuM13M3
		{
			if (i != 0 && i <= 16)
			{
				injector::WriteMemory(pattern.get(i).get<uint32_t>(6), static_cast<float>(nShadowsRes), true);
			}
		}

		pattern = hook::pattern("68 00 01 00 00 68 00 01 00 00");
		for (size_t i = 0; i < pattern.size(); ++i) //http://pastebin.com/PwbVt6uy
		{
			if (i == 3 || i == 4 || i == 5)
			{
				injector::WriteMemory(pattern.get(i).get<uint32_t>(1), nShadowsRes, true);
				injector::WriteMemory(pattern.get(i).get<uint32_t>(6), nShadowsRes, true);
			}
		}
	}

	if (nDOFRes)
	{
		pattern = hook::pattern("8B 3C 85 ? ? ? ? 8B 04 85 ? ? ? ? 56 6A 00 6A 15 6A 01 6A 01 ?"); //6B1444 //6B1434
		auto dword_6B1444 = *pattern.get(0).get<uint32_t*>(3);
		injector::WriteMemory(dword_6B1444 + 2, nDOFRes, true);
		injector::WriteMemory(dword_6B1444 + 3, nDOFRes, true);
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

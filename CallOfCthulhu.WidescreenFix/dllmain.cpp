#include "..\includes\stdafx.h"
#include "..\includes\hooking\Hooking.Patterns.h"

HWND hWnd;
bool bDelay;

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

	CIniReader iniReader("");
	static float verScale = iniReader.ReadFloat("MAIN", "VerticalScale", 0.75f);
	static float FOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 0.75f);

	pattern = hook::module_pattern(GetModuleHandle("d3dx9_28.dll"), "F3 0F 59 3D ? ? ? ? F3 0F 5C CE F3 0F 59 CF F3 0F 59 D9");
	injector::WriteMemory(pattern.get(2).get<uint32_t>(4), &verScale, true);

	pattern = hook::module_pattern(GetModuleHandle("d3dx9_28.dll"), "D8 0D ? ? ? ? 89 45 F4 8D 45 FC 89 45 F0");
	injector::WriteMemory(pattern.get(0).get<uint32_t>(2), &FOVFactor, true);
	
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

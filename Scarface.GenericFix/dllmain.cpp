#include "..\includes\stdafx.h"
#include "..\includes\hooking\Hooking.Patterns.h"

DWORD WINAPI Thread(LPVOID)
{
	//alt-tab crash fix
	auto pattern = hook::pattern("0F 85 21 01 00 00 83 7C 24 64 00").get(0).get<DWORD>(0); //0x654378 
	injector::WriteMemory<unsigned short>(pattern, 0xE990, true);
	auto pattern2 = hook::pattern("0F 85 4B 01 00 00 83 7C 24 64 01").get(0).get<DWORD>(0); //0x65434E
	injector::WriteMemory<unsigned short>(pattern2, 0xE990, true);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
	}
	return TRUE;
}

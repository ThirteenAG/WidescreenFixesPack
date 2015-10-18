#include "..\includes\stdafx.h"
#include "..\includes\hooking\Hooking.Patterns.h"

DWORD nWidth;
DWORD nHeight;
DWORD* pfAspectRatioX;

DWORD WINAPI PatchAR(LPVOID)
{
	Sleep(1000);

	pfAspectRatioX = *hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 8B 7C 24 10 6A 65 8B F0 57 89 74 24 28").get(0).get<DWORD*>(1);

	nWidth = *(DWORD*)((DWORD)pfAspectRatioX + 0x8);
	nHeight = *(DWORD*)((DWORD)pfAspectRatioX + 0xC);

	injector::WriteMemory<float>((DWORD)pfAspectRatioX + 0x904, static_cast<float>(nWidth), true);
	injector::WriteMemory<float>((DWORD)pfAspectRatioX + 0x908, static_cast<float>(nHeight), true);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&PatchAR, NULL, 0, NULL);
	}
	return TRUE;
}
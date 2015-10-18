#include "..\includes\stdafx.h"

int* nWidth = (int *)0x55710C;
int* nHeight = (int *)0x557108;
float fAspectRatio;

DWORD WINAPI Thread(LPVOID)
{
	do
		Sleep(0);
	while (!*nWidth);

	fAspectRatio = (float)*nWidth / (float)*nHeight;
	injector::WriteMemory<float>(0x5B01A8, ((1.0f / fAspectRatio) * (4.0f / 3.0f)), true); // some kind of aspect ratio multiplier

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

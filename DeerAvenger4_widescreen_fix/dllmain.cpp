#include "stdafx.h"
#include "..\includes\CPatch.h"

int* g_Width = (int *)0x55710C;
int* g_Height = (int *)0x557108;
float aspect_ratio;

DWORD WINAPI Thread(LPVOID)
{
	do
	Sleep(0);
	while (!((float)*g_Width));

	aspect_ratio = (float)*g_Width / (float)*g_Height;
	CPatch::SetFloat(0x5B01A8, (1.0f / aspect_ratio) * (4.0f / 3.0f)); // some kind of aspect ratio multiplier

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

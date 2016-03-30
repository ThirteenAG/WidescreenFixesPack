#include "..\includes\stdafx.h"
#include "..\includes\CPatch.h"

float fAlignmentFix;
float fGameSpeed;
float fHudFix;
DWORD* nWidth = (DWORD *)0x8F5FF0;
DWORD* nHeight = (DWORD *)0x8F5FF4;

DWORD WINAPI Init(LPVOID)
{
	CIniReader iniReader("");
	int HUD_PATCH = iniReader.ReadInteger("MAIN", "HUD_PATCH", 0);
	fGameSpeed = iniReader.ReadFloat("MAIN", "GameSpeed", 128.0f);
	
	while (!*nWidth)
		Sleep(0);

	if (HUD_PATCH)
	{
		fAlignmentFix = (0.5f * ((4.0f / 3.0f) / (static_cast<float>(*nWidth) / static_cast<float>(*nHeight)))) - 0.03f;
		if ((static_cast<float>(*nWidth) / static_cast<float>(*nHeight)) > 1.4f)
		{
			CPatch::SetFloat(0x6CF048, (1.0f / (480.0f * (static_cast<float>(*nWidth) / static_cast<float>(*nHeight)))));
			CPatch::SetPointer(0x654DDA + 0x4, &fAlignmentFix);
			//CPatch::SetFloat(0x61E3D9 + 1, (float)res_y);
		}
	}

	if (fGameSpeed)
	{
		CPatch::SetPointer(0x40C930 + 0x2 + 0x32, &fGameSpeed);
		CPatch::SetPointer(0x40C930 + 0x2 + 0x64, &fGameSpeed);
		CPatch::SetPointer(0x40C930 + 0x2 + 0x4D, &fGameSpeed);
	}
	return 0;
}





BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
	}
	return TRUE;
}
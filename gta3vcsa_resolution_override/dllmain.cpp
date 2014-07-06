#include "stdafx.h"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"

#define gGameVersion (*(unsigned int *)0x601048)
#define GTA_3_1_0     0x3A83126F
#define GTA_3_1_1     0x3F8CCCCD
#define GTA_3_STEAM     0x47BDA5
#define GTA_VC_1_0     0x53FF1B8B
#define GTA_VC_STEAM     0xF04F883
#define GTA_VC_1_1     0x783EE8
#define GTA_SA_1_0     0xCE8B168B
#define GTA_SA_1_1     0x0
#define GTA_SA_STEAM     0x1

HWND hWnd;
int res_x;
int res_y;


DWORD WINAPI Thread(LPVOID)
{
	while (true)
	{
		Sleep(0);
		if (*(DWORD*)0x5C1E70 == 0x53E58955) break; //GTA_3_1_0
		else if (*(DWORD*)0x5C2130 == 0x53E58955) break; //GTA_3_1_1
		else if (*(DWORD*)0x5C6FD0 == 0x53E58955) break; //GTA_3_STEAM

		if (*(DWORD*)0x667BF0 == 0x53E58955) break; //GTA_VC_1_0
		else if (*(DWORD*)0x667C40 == 0x53E58955) break; //GTA_VC_1_1
		else if (*(DWORD*)0x666BA0 == 0x53E58955) break; //GTA_VC_STEAM

		if (*(DWORD*)0x82457C == 0x94BF || *(DWORD*)0x8245BC == 0x94BF) break; //GTA_SA_1_0
		else if (*(DWORD*)0x8252FC == 0x94BF || *(DWORD*)0x82533C == 0x94BF) break; //GTA_SA_1_1
		else if (*(DWORD*)0x85EC4A == 0x94BF) break; //GTA_SA_STEAM
	}

	CIniReader iniReader("gta3vcsa_res.ini");
	int enable = iniReader.ReadInteger("MAIN", "Enable", 0);
	res_x = iniReader.ReadInteger("MAIN", "X", 0);
	res_y = iniReader.ReadInteger("MAIN", "Y", 0);

	if (!enable)
	{
		return 0;
	}

	if (!res_x || !res_y) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		res_x = info.rcMonitor.right - info.rcMonitor.left;
		res_y = info.rcMonitor.bottom - info.rcMonitor.top;
	}
	switch (gGameVersion)
	{
	case GTA_3_1_0:
		CPatch::Nop(0x4890FA, 5);
		CPatch::SetInt(0x581E5B + 0x3, res_x);
		CPatch::SetInt(0x581E64 + 0x4, res_y);
		CPatch::SetChar(0x581E6E + 0x4, 32u);
		CPatch::SetChar(0x581E9A, 0xEBu); //jl      short loc_581E40 > jmp      short loc_581E40
		CPatch::Nop(0x581E37, 5);
		break;

	case GTA_3_1_1:

		break;

	case GTA_3_STEAM:
		CPatch::Nop(0x48920C, 5);
		CPatch::SetInt(0x58208B + 0x3, res_x);
		CPatch::SetInt(0x582094 + 0x4, res_y);
		CPatch::SetChar(0x58209E + 0x4, 32u);
		CPatch::SetChar(0x5820CA, 0xEBu); //jl      short loc_581E40 > jmp      short loc_581E40
		CPatch::Nop(0x582067, 5);
		break;

	case GTA_VC_1_0:
		CPatch::SetChar(0x490095 + 0x2, 22u);
		CPatch::SetInt(0x600E7B + 0x3, res_x);
		CPatch::SetInt(0x600E84 + 0x4, res_y);
		CPatch::SetChar(0x600E8E + 0x4, 32u);
		CPatch::SetChar(0x600EBA, 0xEBu); //jl      short loc_600E60 > jmp      short loc_600E60
		CPatch::Nop(0x600E57, 5);
		break;

	case GTA_VC_1_1:

		break;

	case GTA_VC_STEAM:
		CPatch::SetChar(0x48FFA5 + 0x2, 22);
		CPatch::SetInt(0x600ADB + 0x3, res_x);
		CPatch::SetInt(0x600AE4 + 0x4, res_y);
		CPatch::SetChar(0x600AEE + 0x4, 32);
		CPatch::SetChar(0x600B1A, 0xEBu); //jl      short loc_600E60 > jmp      short loc_600E60
		CPatch::Nop(0x600AB7, 5);
		break;

	case GTA_SA_1_0:
		CPatch::SetInt(0x746362 + 0x1, res_x);
		CPatch::SetInt(0x746367 + 0x1, res_y);
		CPatch::SetInt(0x74636C + 0x1, 32);
		CPatch::SetUShort(0x7462AB, 0xB2E9); //jz      loc_746362 > jmp      loc_746362
		CPatch::SetInt(0x7462AB + 2, 0x00);
		CPatch::SetUShort(0x7462B3, 0xAAE9);
		CPatch::SetInt(0x7462B3 + 2, 0x00);
		break;

	case GTA_SA_1_1:

		break;

	case GTA_SA_STEAM:

		break;
	}
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

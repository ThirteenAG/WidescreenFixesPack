#include "..\includes\stdafx.h"
#include "..\includes\CPatch.h"

#define gGameVersion (*(unsigned int *)0x601048)
#define SRS_1_1     3280669732
HWND hWnd;
void patch_res();
DWORD jmpAddress;

float hud_stretch_new;
float hud_stretch_new2;

int res_x;
int res_y;

float fres_x;
float fres_y;

void __declspec(naked)patch_res()
{
	_asm
	{
		//mov[esp + 14], eax
		//call 00655080
		//mov[esp + 18], eax
		mov eax, res_x
		mov[esp + 14h], eax
			mov eax, res_y
			mov[esp + 18h], eax
			mov jmpAddress, 0x403D33
			jmp jmpAddress
	}
}


void Init()
{
	CIniReader iniReader("");
	res_x = iniReader.ReadInteger("MAIN", "X", 0);
	res_y = iniReader.ReadInteger("MAIN", "Y", 0);
	int HUD_PATCH = iniReader.ReadInteger("MAIN", "HUD_PATCH", 0);

	if (!res_x || !res_y) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		res_x = info.rcMonitor.right - info.rcMonitor.left;
		res_y = info.rcMonitor.bottom - info.rcMonitor.top;
	}
	fres_x = (float)res_x;
	fres_y = (float)res_y;

	if ((fres_x / fres_y) >= 1.4f)
	{
		CPatch::RedirectJump(0x403D26, patch_res);

		CPatch::Nop(0x63A878, 4); // corrupting .cfg reading because it makes my resolution patches useless
								  // it also makes impossible to change graphics settings, should be fixable, but i haven't found any solution
								  //CPatch::RedirectJump(0x63A872, patch_res2);

		hud_stretch_new = 1.0f / fres_x;
		hud_stretch_new2 = 1.0f / fres_y;

		CPatch::SetFloat(0x4DDD6B + 0x1, fres_x);
		CPatch::SetFloat(0x4DDD66 + 0x1, fres_y);

		CPatch::SetFloat(0x4DB642 + 0x1, fres_x);
		CPatch::SetFloat(0x4DB63D + 0x1, fres_y);

		CPatch::SetFloat(0x4DB723 + 0x3, fres_x);
		CPatch::SetFloat(0x4DB72A + 0x3, fres_y);

		CPatch::SetFloat(0x5002F7 + 0x1, fres_x);
		CPatch::SetFloat(0x5002F2 + 0x1, fres_y);

		CPatch::SetFloat(0x567397 + 0x6, fres_x);
		CPatch::SetFloat(0x5673A1 + 0x6, fres_y);

		CPatch::SetFloat(0x57B0AC + 0x6, fres_x);
		CPatch::SetFloat(0x57B0B6 + 0x6, fres_y);

		CPatch::SetFloat(0x5EAC83 + 0x4, fres_x);
		CPatch::SetFloat(0x5EAC8B + 0x4, fres_y);

		CPatch::SetFloat(0x61EAE9 + 0x3, fres_x);
		CPatch::SetFloat(0x61EAF0 + 0x3, fres_y);


		CPatch::SetFloat(0x400000 + 0xFFF72, fres_x);
		CPatch::SetFloat(0x400000 + 0x34820C, fres_x);

		CPatch::SetFloat(0x400000 + 0xFFF72 - 0x5, fres_y);
		CPatch::SetFloat(0x400000 + 0x34820C - 0x4, fres_y);


		CPatch::Nop(0x569770 + 0x48, 6);
		CPatch::Nop(0x569770 + 0x100, 6);
		CPatch::Nop(0x569940 + 0xDE, 6);
		CPatch::Nop(0x569AD0 + 0xB8, 6);

		CPatch::Nop(0x569770 + 0x51, 6);
		CPatch::Nop(0x569770 + 0x10C, 5);
		CPatch::Nop(0x569940 + 0xE7, 6);
		CPatch::Nop(0x569AD0 + 0xC1, 6);
		CPatch::Nop(0x569CBD, 6);

		CPatch::SetFloat(0x7F6478, fres_x);
		//float res_cutscene = fres_y / (4.0f / 3.0f);
		//CPatch::SetFloat(0x7F647C, res_cutscene);

		if (HUD_PATCH)
		{
			CPatch::SetFloat(0x7468B8, hud_stretch_new);
			CPatch::SetFloat(0x7468B4, hud_stretch_new2);
		}

		CPatch::SetPointer(0x4CFBEB + 0x2, &hud_stretch_new);
		CPatch::SetPointer(0x4CFBF8 + 0x2, &hud_stretch_new2);

	}
	return;
}





BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}
#include "stdafx.h"
#include "..\includes\CPatch.h"

HWND hWnd;
DWORD jmpAddress;

void Init()
{
	CIniReader iniReader("");
	int res_x = iniReader.ReadInteger("MAIN", "X", 0);
	int res_y = iniReader.ReadInteger("MAIN", "Y", 0);
	//int hud_patch = iniReader.ReadInteger("MAIN", "HUD_PATCH", 0);

	if (!res_x || !res_y) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		res_x = info.rcMonitor.right - info.rcMonitor.left;
		res_y = info.rcMonitor.bottom - info.rcMonitor.top;
	}


	//float hud_multiplier_x = 1.0 / res_x * 2.0f;
	//float hud_multiplier_y = 1.0 / res_y * 2.0f;

	//float orig_multiplier_x = 1.0 / 640 * 2.0;
	//float orig_multiplier_y = 1.0 / 480 * 2.0;
	CPatch::SetInt(0x403DA2, res_x);
	CPatch::SetInt(0x403DAC, res_y);

	CPatch::SetInt(0x40949A, res_x);
	CPatch::SetInt(0x409495, res_y);

	CPatch::SetInt(0x432B8C, res_x);
	CPatch::SetInt(0x432B91, res_y);

	CPatch::SetInt(0x4553FC, res_x);
	CPatch::SetInt(0x455406, res_y);

	CPatch::SetInt(0x90B9DC, res_x);
	CPatch::SetInt(0x90B9E0, res_y);

	CPatch::SetInt(0xA0A930, res_x);
	CPatch::SetInt(0xA0A934, res_y);

	CPatch::SetFloat(0x552487 + 0x1, (float)res_x / (float)res_y);
	CPatch::SetFloat(0x55248E + 0x1, (float)res_x / (float)res_y);


	/*	//HUD
	if (hud_patch) {
	if (!((char)*(DWORD*)0xA8F2DD == 0)) {
	patch(0x9E8F8C, &hud_multiplier_x, 4);
	patch(0x9E8F88, &hud_multiplier_y, 4);
	}
	else {
	patch(0x9E8F8C, &orig_multiplier_x, 4);
	patch(0x9E8F88, &orig_multiplier_y, 4);
	}
	*/
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}
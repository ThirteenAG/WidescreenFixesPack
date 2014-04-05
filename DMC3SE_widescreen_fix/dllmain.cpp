#include "stdafx.h"
#include "stdio.h"
#include <windows.h>
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"

HWND hWnd;
float aspect_ratio;
float text_multiplier;
DWORD fullscreen;

void Init()
{
	CIniReader iniReader("DMC3SE_res.ini");
	int res_x = iniReader.ReadInteger("MAIN", "X", 0);
	int res_y = iniReader.ReadInteger("MAIN", "Y", 0);
	int hud_patch = iniReader.ReadInteger("MAIN", "HUD_PATCH", 0);

	if (!res_x || !res_y) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		res_x = info.rcMonitor.right - info.rcMonitor.left;
		res_y = info.rcMonitor.bottom - info.rcMonitor.top;
	}
	aspect_ratio = (float)res_x / (float)res_y;
	//game
	CPatch::SetInt(0x4028DE + 0x2, res_x);
	CPatch::SetInt(0x4028E4 + 0x2, res_y);

	CPatch::SetChar(0x4028C2, 0xEBu);

	CPatch::SetInt(0x40A7E2 + 0x6, res_x);
	CPatch::SetInt(0x40A7EC + 0x6, res_y);

	CPatch::SetInt(0x781394, res_x);
	CPatch::SetInt(0x781398, res_y);

	CPatch::SetChar(0x6CDB87, 0xC7u);
	CPatch::SetChar(0x6CDB87 + 0x1, 0x47u);
	CPatch::SetChar(0x6CDB87 + 0x2, 0x60u);
	CPatch::SetChar(0x6CDB87 + 0x3, 0x00u);
	CPatch::SetChar(0x6CDB87 + 0x4, 0x00u);
	CPatch::SetChar(0x6CDB87 + 0x5, 0x40u);
	CPatch::SetChar(0x6CDB87 + 0x6, 0x3Fu);

	CPatch::SetChar(0x6CDBB9, 0x70u);

	float hud_multiplier_x = (1.0 / 640.0 / 0.8) / (640.0 / 480.0); //(1.0 / (res_y * (res_x/res_y)) / 0.8) / 0.8;//1.0 / 640.0 / 0.8 / ((float)res_x/(float)res_y);
	//float hud_multiplier_y = 1.0 / res_y / 0.8;

	//float orig_multiplier_x = 1.0 / 640 / 0.8; //1/640/0.8
	//float orig_multiplier_y = 1.0 / 480 / 0.8;

	text_multiplier = (1.0 / 640.0 / 0.4) / (640.0 / 480.0);

		//HUD
		if (hud_patch && aspect_ratio >= 1.4f)
		{
			CPatch::SetFloat(0x7491DC, hud_multiplier_x);
			CPatch::SetPointer(0x406E40 - 0x6B + 0x2, &text_multiplier);
			CPatch::SetPointer(0x406E40 - 0x45 + 0x2, &text_multiplier);
		}
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}
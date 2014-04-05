#include "stdafx.h"
#include "stdio.h"
#include <windows.h>
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"

HWND hWnd;

int hud_patch;
int res_x;
int res_y;
DWORD WINAPI hud_handler(LPVOID);

void Init()
{
	CIniReader iniReader("gun_res.ini");
	res_x = iniReader.ReadInteger("MAIN", "X", 0);
	res_y = iniReader.ReadInteger("MAIN", "Y", 0);
	hud_patch = iniReader.ReadInteger("MAIN", "HUD_PATCH", 0);

	if (!res_x || !res_y) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		res_x = info.rcMonitor.right - info.rcMonitor.left;
		res_y = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	//game
	/*patch(0x66AB61+0x1, &res_x, 4);
	patch(0x66AB6B+0x1, &res_y, 4);*/
	/*patch(0x66AB78+0x1, &res_x, 4);
	patch(0x66AB7F+0x1, &res_y, 4);
	patch(0x66AB87+0x1, &res_x, 4);
	patch(0x66AB8E+0x1, &res_y, 4);
	patch(0x66AB96+0x1, &res_x, 4);
	patch(0x66AB9D+0x1, &res_y, 4);
	patch(0x66ABA5+0x1, &res_x, 4);
	patch(0x66ABAD+0x1, &res_y, 4);
	patch(0x66ABB4+0x1, &res_x, 4);*/

	CPatch::Nop(0x50540A, 5); //nop res
	CPatch::Nop(0x51942A, 5);
	CPatch::Nop(0x5F9113, 5);

	CPatch::Nop(0x50541D, 5);
	CPatch::Nop(0x51943D, 5);
	CPatch::Nop(0x5F9131, 5);

	CPatch::Nop(0x4BA674, 5); //nop AR

		CPatch::SetInt(0x6B759C, res_x);
		CPatch::SetInt(0x6B75A0, res_y);

		float aspect_ratio = (float)res_x / (float)res_y;

		CPatch::SetFloat(0x750FAC, aspect_ratio);
		//HUD
		float hud_multiplier_x = 1.0f / res_x * (res_y / 480.0f);
		//float hud_multiplier_y = 1.0f / res_y * (res_x / 640.0f); //1.0 / res_y;

		//float orig_multiplier_x = 1.0f / 640.0f;
		//float orig_multiplier_y = 1.0f / 480.0f;
		if (hud_patch)
		{
			CPatch::SetFloat(0x6814CC, hud_multiplier_x);
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
#include "stdafx.h"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"
HWND hWnd;

int res_x, res_x43, res_y43, res_y;
int res_x_default = 640;
int res_y_default = 480;
float hud_multiplier_x, hud_multiplier_y;

void Init()
{
	CIniReader iniReader("");
	res_x = iniReader.ReadInteger("MAIN", "X", 0);
	res_y = iniReader.ReadInteger("MAIN", "Y", 0);

	if (!res_x || !res_y) {
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		res_x = info.rcMonitor.right - info.rcMonitor.left;
		res_y = info.rcMonitor.bottom - info.rcMonitor.top;
	}

	if (*(DWORD*)0x4F8436 == 640)
	{
		CPatch::SetUInt(0x4F8435 + 0x1, res_x);
		CPatch::SetUInt(0x4F843A + 0x6, res_y);
	}
	else 
	{
		CPatch::Nop(0x4F3F27, 2);
		CPatch::Nop(0x4F3F3B, 14);

		res_x43 = static_cast<int>(res_x / (4.0f / 3.0f));
		res_y43 = static_cast<int>(res_y * (4.0f / 3.0f));

		CPatch::SetUInt(0x4F5495 + 0x1, res_x);
		CPatch::SetUInt(0x4F549A + 0x6, res_y43);

		CPatch::SetUInt(0x4F3F2F + 0x1, res_x);
		CPatch::SetUInt(0x4F3F36 + 0x1, res_y43);

		hud_multiplier_x = 1.0f / res_x * (res_y / 480.0f);
		hud_multiplier_y = 1.0f / res_x * (res_y43 / 480.0f);

			CPatch::SetFloat(0x516958, hud_multiplier_x);
			CPatch::SetFloat(0x516954, hud_multiplier_y);

			CPatch::SetPointer(0x457244, &res_x43);
			CPatch::SetPointer(0x457270, &res_x43);
			CPatch::SetPointer(0x457293, &res_x43);
			CPatch::SetPointer(0x4572BE, &res_x43);

			CPatch::SetPointer(0x457254 + 0x3, &res_y);
			CPatch::SetPointer(0x45727B + 0x3, &res_y);
			CPatch::SetPointer(0x4572A2 + 0x3, &res_y);
			CPatch::SetPointer(0x4572CD + 0x3, &res_y);

			//main menu wheel fix
			CPatch::SetPointer(0x4578B0 + 0x2BC + 0x3, &res_x_default);
			CPatch::SetPointer(0x4578B0 + 0x2E9 + 0x3, &res_x_default);
			CPatch::SetPointer(0x4578B0 + 0x314 + 0x3, &res_x_default);
			CPatch::SetPointer(0x4578B0 + 0x33F + 0x3, &res_x_default);

			CPatch::SetPointer(0x4578B0 + 0x2D4 + 0x3, &res_y_default);
			CPatch::SetPointer(0x4578B0 + 0x2FC + 0x3, &res_y_default);
			CPatch::SetPointer(0x4578B0 + 0x327 + 0x3, &res_y_default);
			CPatch::SetPointer(0x4578B0 + 0x352 + 0x3, &res_y_default);

			CPatch::SetUChar(0x4F4003 + 0x1, 32);

			//CPatch::SetUInt(0x4F3F2F + 0x1, res_x43); //crop screen
			CPatch::SetUInt(0x4F3F36 + 0x1, res_y43); //crop screen
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
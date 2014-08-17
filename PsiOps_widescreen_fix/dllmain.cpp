#include "stdafx.h"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"
HWND hWnd;

int hud_patch;
int res_x;
int res_y;
float fHudScale, fHudPos;

void Init()
{
	CIniReader iniReader("");
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

	CPatch::Nop(0x7A3B5E, 7);
	CPatch::SetUInt(0x7A3B72 + 0x2, res_x);
	CPatch::SetUInt(0x7A3B7B + 0x2, res_y);

	if (hud_patch)
	{
		fHudPos = (-1.0f * (4.0f / 3.0f)) / (static_cast<float>(res_x) / static_cast<float>(res_y));
		fHudScale = (2.0f * (4.0f / 3.0f)) / (static_cast<float>(res_x) / static_cast<float>(res_y));

		CPatch::SetFloat(0x756CF8 + 0x1, fHudPos);
		CPatch::SetPointer(0x756D23 + 0x2, &fHudScale);
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
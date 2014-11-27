#include "stdafx.h"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"
HWND hWnd;

int res_x;
int res_y;

int view_stretch_address = 0x61F0B4;
float view_stretch;

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

	if (!(*(float *)view_stretch_address == 0.75f)) // Currently works only for 1.06.001
		return;

	//fixing menu
	CPatch::SetUInt(0x483F0C, res_x);
	CPatch::SetUInt(0x484130, res_x);
	CPatch::SetUInt(0x48D9B8, res_x);
	CPatch::SetUInt(0x48D9CD, res_x);

	CPatch::SetUInt(0x483F13, res_y);
	CPatch::SetUInt(0x48413A, res_y);
	CPatch::SetUInt(0x48D9C0, res_y);
	CPatch::SetUInt(0x48D9D6, res_y);

	//fixing game
	CPatch::SetUChar(0x48D92E, 0xEB);
	CPatch::SetUInt(0x48D925, res_x);
	CPatch::SetUInt(0x48D92A, res_y);

	/*CPatch::SetPointer(0x43A019 + 0x1, &res_x);
	CPatch::SetPointer(0x43A01F + 0x1, &res_y);

	//CPatch::SetPointer(0x439B5A + 0x1, &res_x);
	//CPatch::SetPointer(0x439B6D + 0x1, &res_y);

	//CPatch::SetPointer(0x439C89 + 0x1, &res_x);
	//CPatch::SetPointer(0x439C91 + 0x1, &res_y);

	CPatch::SetPointer(0x45FB8A + 0x2, &res_x);
	CPatch::SetPointer(0x45FBCE + 0x2, &res_x);
	CPatch::SetPointer(0x45FBC8 + 0x2, &res_y);

	//CPatch::SetPointer(0x439B02 + 0x2, &res_x);
	//CPatch::SetPointer(0x439B19 + 0x2, &res_y);*/

	view_stretch = (static_cast<float>(res_y) / static_cast<float>(res_x));

	CPatch::SetFloat(view_stretch_address, view_stretch);
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}
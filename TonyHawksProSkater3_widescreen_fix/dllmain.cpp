#include "stdafx.h"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"
HWND hWnd;

int res_x, res_x43, res_y43, res_y;
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

	res_x43 = static_cast<int>(res_x / (4.0f / 3.0f));
	res_y43 = static_cast<int>(res_y * (4.0f / 3.0f));

	CPatch::SetUInt(0x40B150 + 0x1F9 + 0x6, res_x); //, 320h 
	CPatch::SetUInt(0x40B150 + 0x235 + 0x6, res_x); //, 400h 
	CPatch::SetUInt(0x40B150 + 0x271 + 0x6, res_x); //, 500h 
	CPatch::SetUInt(0x40B150 + 0x2A5 + 0x6, res_x); //, 280h 
	CPatch::SetUInt(0x40B4F0 + 0xC8 + 0x6, res_x); //, 640  
	//CPatch::SetUInt(0x40B4F0 + 0x345 + 0x6, res_x); //, eax  
	CPatch::SetUInt(0x40B150 + 0x203 + 0x6, res_y43); // 258h
	CPatch::SetUInt(0x40B150 + 0x23F + 0x6, res_y43); // 300h
	CPatch::SetUInt(0x40B150 + 0x27B + 0x6, res_y43); // 400h
	CPatch::SetUInt(0x40B150 + 0x2AF + 0x6, res_y43); // 1E0h
	CPatch::SetUInt(0x40B4F0 + 0xD2 + 0x6, res_y43); // 1E0h
	//CPatch::SetUInt(0x40B4F0 + 0x35B + 0x6, res_y); // eax 

	hud_multiplier_x = 1.0f / res_x * (res_y / 480.0f);
	hud_multiplier_y = 1.0f / res_x * (res_y43 / 480.0f);

	CPatch::SetFloat(0x58DDC8, hud_multiplier_x);
	CPatch::SetFloat(0x58DDC4, hud_multiplier_y);

		//54FFFB - FOV multipl?

}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}
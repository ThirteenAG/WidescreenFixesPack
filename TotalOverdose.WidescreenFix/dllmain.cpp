#include "..\includes\stdafx.h"
#include "..\includes\CPatch.h"

HWND hWnd;

int res_x;
int res_y;

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

	if ((*(DWORD*)0x45E93B == 640))
	{
		CPatch::SetUInt(0x45E939 + 0x2, res_x);
		CPatch::SetUInt(0x45E93F + 0x3, res_y);

		CPatch::SetUInt(0x45E9A1 + 0x2, res_x);
		CPatch::SetUInt(0x45E9A7 + 0x3, res_y);

		CPatch::SetUInt(0x45EA09 + 0x2, res_x);
		CPatch::SetUInt(0x45EA0F + 0x3, res_y);

		CPatch::SetUInt(0x45EA71 + 0x2, res_x);
		CPatch::SetUInt(0x45EA77 + 0x3, res_y);

		CPatch::SetUInt(0x45EAD9 + 0x2, res_x);
		CPatch::SetUInt(0x45EADF + 0x3, res_y);


		CPatch::SetUInt(0x45EB3D + 0x1, res_x);
		CPatch::SetUInt(0x45EB48 + 0x1, res_y);

		CPatch::SetFloat(0x9B38EC, static_cast<float>(res_x) / static_cast<float>(res_y));
	}
	else
	{
		/*CPatch::SetUInt(0x946A37 + 0x4, res_x);
		CPatch::SetUInt(0x946A3F + 0x4, res_y);

		CPatch::SetUInt(0x9469FE + 0x4, res_x);
		CPatch::SetUInt(0x946A06 + 0x4, res_y);

		CPatch::SetUInt(0x9469C5 + 0x4, res_x);
		CPatch::SetUInt(0x9469CD + 0x4, res_y);

		CPatch::SetUInt(0x946995 + 0x4, res_x);
		CPatch::SetUInt(0x94699D + 0x4, res_y);

		CPatch::SetUInt(0x946964 + 0x4, res_x);
		CPatch::SetUInt(0x94696C + 0x4, res_y);


		//CPatch::SetUInt(0x45EB3D + 0x1, res_x);
		//CPatch::SetUInt(0x45EB48 + 0x1, res_y);
		CPatch::RedirectJump(0x41FD70, asm_patch);

		CPatch::SetFloat(0x9B38EC, static_cast<float>(res_x) / static_cast<float>(res_y));*/
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
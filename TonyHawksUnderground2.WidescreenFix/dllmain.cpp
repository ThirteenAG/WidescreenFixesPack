#include "..\includes\stdafx.h"
#include "..\includes\CPatch.h"

#define _USE_MATH_DEFINES
#include "math.h"
HWND hWnd;

int hud_patch;
int res_x;
int res_y;
float fres_x, fres_y;
float fDynamicScreenFieldOfViewScale;
float fAspectRatio;
float hud_multiplier_x;

#define DEGREE_TO_RADIAN(fAngle) \
	((fAngle)* (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) \
	((fAngle)* 180.0f / (float)M_PI)
#define SCREEN_FOV_HORIZONTAL		124.59155f
#define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / (4.0f / 3.0f))))

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

	if ((*(DWORD*)0x4E2932 == 640))
	{
		CPatch::SetUInt(0x67FBCC, res_x);
		CPatch::SetUInt(0x67FBD0, res_y);

		fres_x = static_cast<float>(res_x);
		fres_y = static_cast<float>(res_y);

		/*CPatch::SetPointer(0x4370CB + 0x4, &fres_x);
		CPatch::SetPointer(0x4370F7 + 0x4, &fres_y);

		CPatch::SetPointer(0x4370CB + 0x4, &fres_x);
		CPatch::SetPointer(0x4370F7 + 0x4, &fres_y);*/

		CPatch::SetUInt(0x4D88AC + 0x1, res_x);
		CPatch::SetUInt(0x4D88CE + 0x4, res_x);
		CPatch::SetUInt(0x4D88DA + 0x4, res_x);
		CPatch::SetUInt(0x4D88E2 + 0x4, res_x);

		CPatch::SetUInt(0x4D88EE + 0x4, res_y);
		CPatch::SetUInt(0x4D88F6 + 0x4, res_y);
		CPatch::SetUInt(0x4D88FE + 0x4, res_y);

		fAspectRatio = fres_x / fres_y;
		fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * fAspectRatio));

		CPatch::SetFloat(0x649804, fDynamicScreenFieldOfViewScale);
		CPatch::SetPointer(0x4A0860 + 0x2, &fAspectRatio);

		hud_multiplier_x = 1.0f / res_x * (res_y / 480.0f);
		if (hud_patch)
		{
			CPatch::SetPointer(0x4D8A30 + 0x2, &hud_multiplier_x);
			CPatch::SetPointer(0x4B6CF8 + 0x2, &hud_multiplier_x);

			CPatch::Nop(0x4D87F0, 6);
			CPatch::Nop(0x4D8A58, 6);
			DWORD nHudAlignment = static_cast<DWORD>((fres_x - (fres_y * (4.0f / 3.0f))) / 2);
			CPatch::SetUInt(0x787D88, nHudAlignment);
		}
	}
	else
	{
		CPatch::SetUInt(0x67EBCC, res_x);
		CPatch::SetUInt(0x67EBD0, res_y);

		fres_x = static_cast<float>(res_x);
		fres_y = static_cast<float>(res_y);

		CPatch::SetUInt(0x4D873C + 0x1, res_x);
		CPatch::SetUInt(0x4D875E + 0x4, res_x);
		CPatch::SetUInt(0x4D876A + 0x4, res_x);
		CPatch::SetUInt(0x4D8772 + 0x4, res_x);

		CPatch::SetUInt(0x4D877E + 0x4, res_y);
		CPatch::SetUInt(0x4D8786 + 0x4, res_y);
		CPatch::SetUInt(0x4D878E + 0x4, res_y);

		fAspectRatio = fres_x / fres_y;
		fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * fAspectRatio));

		CPatch::SetFloat(0x648804, fDynamicScreenFieldOfViewScale);
		CPatch::SetPointer(0x4A0790 + 0x2, &fAspectRatio);

		hud_multiplier_x = 1.0f / res_x * (res_y / 480.0f);
		if (hud_patch)
		{
			CPatch::SetPointer(0x4D88C0 + 0x2, &hud_multiplier_x);
			CPatch::SetPointer(0x4B6BA8 + 0x2, &hud_multiplier_x);

			CPatch::Nop(0x4D8680, 6);
			CPatch::Nop(0x4D88E8, 6);
			DWORD nHudAlignment = static_cast<DWORD>((fres_x - (fres_y * (4.0f / 3.0f))) / 2);
			CPatch::SetUInt(0x786D88, nHudAlignment);
		}
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
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

#define DEGREE_TO_RADIAN(fAngle) \
	((fAngle)* (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) \
	((fAngle)* 180.0f / (float)M_PI)
#define SCREEN_FOV_HORIZONTAL		1.9f
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
	CPatch::Nop(0x43EFB0 + 0x18E, 6);
	CPatch::Nop(0x43EFB0 + 0x1DD, 6);
	CPatch::Nop(0x542A90 + 0x149, 6);
	CPatch::SetUInt(0x5A0280, res_x);

	CPatch::Nop(0x43EFB0 + 0x194, 6);
	CPatch::Nop(0x43EFB0 + 0x1F3, 6);
	CPatch::Nop(0x542A90 + 0x14F, 6);
	CPatch::SetUInt(0x5A0284, res_y);

	fres_x = static_cast<float>(res_x);
	fres_y = static_cast<float>(res_y);

	fAspectRatio = fres_x / fres_y;
	fDynamicScreenFieldOfViewScale = 180.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * fAspectRatio));

	CPatch::SetPointer(0x45DF17 + 0x2, &fDynamicScreenFieldOfViewScale);
	CPatch::SetFloat(0x4616E2 + 0x1, fAspectRatio);
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}
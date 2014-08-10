#include "stdafx.h"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"
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
#define SCREEN_FOV_HORIZONTAL		124.59155f
#define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / (4.0f / 3.0f))))

DWORD WINAPI Init(LPVOID)
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


	fres_x = static_cast<float>(res_x);
	fres_y = static_cast<float>(res_y);

	while (!(*(DWORD*)0x4AE0C1 == 1280))
	{
		Sleep(0);
	}

	CPatch::SetUInt(0x4AE0BD + 4, res_x);
	CPatch::SetUInt(0x4AE0C5 + 4, res_x);
	CPatch::SetUInt(0x4AE0CD + 4, res_x);

	CPatch::SetUInt(0x4AE09C + 4, res_y);
	CPatch::SetUInt(0x4AE0A4 + 4, res_y);
	CPatch::SetUInt(0x4AE0AC + 4, res_y);
	CPatch::SetUInt(0x4AE093 + 1, res_y);

	fAspectRatio = fres_x / fres_y;
	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * fAspectRatio));

	CPatch::SetFloat(0x5F4D4C, fDynamicScreenFieldOfViewScale);
	CPatch::SetFloat(0x472D00 + 0x1, fAspectRatio);

	float hud_multiplier_x = 1.0f / res_x * (res_y / 480.0f);
	if (hud_patch)
	{
		CPatch::SetFloat(0x5F685C, hud_multiplier_x);

		CPatch::Nop(0x4AE00A, 6);
		CPatch::Nop(0x4AE361, 6);
		DWORD nHudAlignment = static_cast<DWORD>((fres_x - (fres_y * (4.0f / 3.0f))) / 4);
		CPatch::SetUInt(0x6D3804, nHudAlignment);
	}
	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
	}
	return TRUE;
}
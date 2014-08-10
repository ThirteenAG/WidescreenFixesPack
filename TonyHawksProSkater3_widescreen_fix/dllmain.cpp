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
#define SCREEN_FOV_HORIZONTAL		114.59155f
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

	CPatch::SetUInt(0x40B150 + 0x1F9 + 0x6, res_x); //, 320h 
	CPatch::SetUInt(0x40B150 + 0x235 + 0x6, res_x); //, 400h 
	CPatch::SetUInt(0x40B150 + 0x271 + 0x6, res_x); //, 500h 
	CPatch::SetUInt(0x40B150 + 0x2A5 + 0x6, res_x); //, 280h 
	CPatch::SetUInt(0x40B4F0 + 0xC8 + 0x6, res_x); //, 640  
	//CPatch::SetUInt(0x40B4F0 + 0x345 + 0x6, res_x); //, eax  
	CPatch::SetUInt(0x40B150 + 0x203 + 0x6, res_y); // 258h
	CPatch::SetUInt(0x40B150 + 0x23F + 0x6, res_y); // 300h
	CPatch::SetUInt(0x40B150 + 0x27B + 0x6, res_y); // 400h
	CPatch::SetUInt(0x40B150 + 0x2AF + 0x6, res_y); // 1E0h
	CPatch::SetUInt(0x40B4F0 + 0xD2 + 0x6, res_y); // 1E0h
	//CPatch::SetUInt(0x40B4F0 + 0x35B + 0x6, res_y); // eax 

	float hud_multiplier_x = 1.0f / res_x * (res_y / 480.0f);
	//if (hud_patch)
	//{
	CPatch::SetFloat(0x58DDC8, hud_multiplier_x);
	//}
	//58D748??
	//54FFFB - FOV multipl


	/*fres_x = static_cast<float>(res_x);
	fres_y = static_cast<float>(res_y);

	CPatch::SetPointer(0x4370CB + 0x4, &fres_x);
	CPatch::SetPointer(0x4370F7 + 0x4, &fres_y);

	CPatch::SetPointer(0x4370CB + 0x4, &fres_x);
	CPatch::SetPointer(0x4370F7 + 0x4, &fres_y);

	CPatch::SetUInt(0x53510D+1, res_x);
	CPatch::SetUInt(0x53511A+4, res_x);
	CPatch::SetUInt(0x535122+4, res_x);
	CPatch::SetUInt(0x53512A+4, res_x);

	CPatch::SetUInt(0x53514A + 4, res_y);
	CPatch::SetUInt(0x535152 + 4, res_y);
	CPatch::SetUInt(0x53515A + 4, res_y);

	fAspectRatio = fres_x / fres_y;
	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * fAspectRatio));

	CPatch::SetFloat(0x691FA8, fDynamicScreenFieldOfViewScale);
	CPatch::SetPointer(0x4EDA60 + 0x2, &fAspectRatio);*/
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}
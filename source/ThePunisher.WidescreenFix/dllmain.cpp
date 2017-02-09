#include "stdafx.h"
#include "..\includes\CPatch.h"

#define _USE_MATH_DEFINES
#include "math.h"
HWND hWnd;

int res_x, res_y, hud_res_x, hud_res_y;
float fres_x, fres_y;
float fDynamicScreenFieldOfViewScale;
float fAspectRatio;
float hud_multiplier_x;

#define DEGREE_TO_RADIAN(fAngle) \
	((fAngle)* (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) \
	((fAngle)* 180.0f / (float)M_PI)
#define SCREEN_FOV_HORIZONTAL		60.0f
#define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / (4.0f / 3.0f))))

void _declspec(naked) asm_res_fix1()
{
	_asm
	{
		mov     eax, hud_res_x
		ret
	}
}

void _declspec(naked) asm_res_fix2()
{
	_asm
	{
		mov     eax, hud_res_y
		ret
	}
}

void Init()
{
	CIniReader iniReader("");
	res_x = iniReader.ReadInteger("MAIN", "X", 0);
	res_y = iniReader.ReadInteger("MAIN", "Y", 0);
	hud_res_x = iniReader.ReadInteger("MAIN", "HUD_RESX", 1280);
	hud_res_y = iniReader.ReadInteger("MAIN", "HUD_RESY", 960);

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

	fAspectRatio = fres_x / fres_y;
	fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * fAspectRatio));

	if ((*(DWORD*)0x401F60 == 640)) // HOODLUM 2,48 MB (2 605 056 bytes)
	{
		CPatch::RedirectCall(0x465360, asm_res_fix1);
		CPatch::RedirectCall(0x46536A, asm_res_fix2);

		CPatch::RedirectCall(0x477D37, asm_res_fix1);
		CPatch::RedirectCall(0x477D31, asm_res_fix2);

		CPatch::Nop(0x4DD6CB, 2);
		CPatch::Nop(0x4DD6ED, 2);
		CPatch::SetUInt(0x4DD6CD + 0x3, res_x);
		CPatch::SetUInt(0x4DD6EF + 0x4, res_y);

		CPatch::SetFloat(0x4922AC + 0x6, fDynamicScreenFieldOfViewScale);
		CPatch::SetFloat(0x4922DE + 0x6, fDynamicScreenFieldOfViewScale);
		CPatch::SetFloat(0x493590 + 0x6, fDynamicScreenFieldOfViewScale);
		CPatch::SetFloat(0x4B73A8 + 0x6, fDynamicScreenFieldOfViewScale);
		CPatch::SetFloat(0x4B7CDB + 0x6, fDynamicScreenFieldOfViewScale);
		CPatch::SetFloat(0x4B8585 + 0x6, fDynamicScreenFieldOfViewScale);

		CPatch::SetFloat(0x403DD4 + 0x6, fDynamicScreenFieldOfViewScale);
		CPatch::SetFloat(0x404340 + 0x1, fDynamicScreenFieldOfViewScale);
		CPatch::SetFloat(0x404E95 + 0x1, fDynamicScreenFieldOfViewScale);
		CPatch::SetFloat(0x406E6B + 0x6, fDynamicScreenFieldOfViewScale);
		CPatch::SetFloat(0x4072C4 + 0x6, fDynamicScreenFieldOfViewScale);
		CPatch::SetFloat(0x4C4CA2 + 0x4, fDynamicScreenFieldOfViewScale);

		CPatch::Nop(0x4DD5B9, 5); // no intro
	}
	else
	{
		if ((*(DWORD*)0x401F70 == 640) && (*(DWORD*)0x514871 != 750)) // unknown 2,44 MB (2 562 048 bytes)
		{
			CPatch::RedirectCall(0x465290, asm_res_fix1);
			CPatch::RedirectCall(0x46529A, asm_res_fix2);

			CPatch::RedirectCall(0x477D81, asm_res_fix1);
			CPatch::RedirectCall(0x477D87, asm_res_fix2);

			CPatch::Nop(0x4DD877, 2);
			CPatch::Nop(0x4DD899, 2);
			CPatch::SetUInt(0x4DD879 + 0x3, res_x);
			CPatch::SetUInt(0x4DD89B + 0x4, res_y);

			CPatch::SetFloat(0x49236C + 0x6, fDynamicScreenFieldOfViewScale); //to do
			CPatch::SetFloat(0x49239E + 0x6, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x493650 + 0x6, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x4B7508 + 0x6, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x4B7E3B + 0x6, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x4B86E5 + 0x6, fDynamicScreenFieldOfViewScale);

			CPatch::SetFloat(0x403DE4 + 0x6, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x404350 + 0x1, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x404EA5 + 0x1, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x406E7B + 0x6, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x4072D4 + 0x6, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x4C4DE2 + 0x4, fDynamicScreenFieldOfViewScale);

			CPatch::Nop(0x4DD74B, 5); // no intro
		}
		else
		{
			//Unpacked with ProcDump32       (C) G-RoM, Lorian & Stone - 1998, 1999 2,59 MB (2 719 744 bytes)

			CPatch::RedirectCall(0x465380, asm_res_fix1);
			CPatch::RedirectCall(0x46538A, asm_res_fix2);

			CPatch::RedirectCall(0x477E57, asm_res_fix1);
			CPatch::RedirectCall(0x477E51, asm_res_fix2);

			CPatch::Nop(0x4DD9EC, 2);
			CPatch::Nop(0x4DDA0E, 2);
			CPatch::SetUInt(0x4DD9EE + 0x3, res_x);
			CPatch::SetUInt(0x4DDA10 + 0x4, res_y);

			CPatch::SetFloat(0x4924CC + 0x6, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x4924CC + 0x6, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x4924FE + 0x6, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x4B7658 + 0x6, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x4B7F8B + 0x6, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x4B8835 + 0x6, fDynamicScreenFieldOfViewScale);

			CPatch::SetFloat(0x403DE4 + 0x6, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x404350 + 0x1, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x404EA5 + 0x1, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x406E7B + 0x6, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x4072D4 + 0x6, fDynamicScreenFieldOfViewScale);
			CPatch::SetFloat(0x4C4F52 + 0x4, fDynamicScreenFieldOfViewScale);

			CPatch::Nop(0x4DD8A9, 5); // no intro
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
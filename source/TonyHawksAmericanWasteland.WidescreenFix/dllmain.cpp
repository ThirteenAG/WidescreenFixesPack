#include "stdafx.h"
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

    if ((*(DWORD*)0x53511E == 1280))
    {
        CPatch::SetUInt(0x6D2224, res_x);
        CPatch::SetUInt(0x6D2228, res_y);

        fres_x = static_cast<float>(res_x);
        fres_y = static_cast<float>(res_y);

        CPatch::SetPointer(0x4370CB + 0x4, &fres_x);
        CPatch::SetPointer(0x4370F7 + 0x4, &fres_y);

        CPatch::SetPointer(0x4370CB + 0x4, &fres_x);
        CPatch::SetPointer(0x4370F7 + 0x4, &fres_y);

        CPatch::SetUInt(0x53510D + 0x1, res_x);
        CPatch::SetUInt(0x53511A + 0x4, res_x);
        CPatch::SetUInt(0x535122 + 0x4, res_x);
        CPatch::SetUInt(0x53512A + 0x4, res_x);

        CPatch::SetUInt(0x53514A + 0x4, res_y);
        CPatch::SetUInt(0x535152 + 0x4, res_y);
        CPatch::SetUInt(0x53515A + 0x4, res_y);

        fAspectRatio = fres_x / fres_y;
        fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * fAspectRatio));

        CPatch::SetFloat(0x691FA8, fDynamicScreenFieldOfViewScale);
        CPatch::SetPointer(0x4EDA60 + 0x2, &fAspectRatio);

        hud_multiplier_x = 1.0f / res_x * (res_y / 480.0f);
        if (hud_patch)
        {
            //CPatch::SetFloat(0x694E1C, hud_multiplier_x);
            //CPatch::SetPointer(0x50D20E + 0x4, &hud_multiplier_x);
            CPatch::SetPointer(0x5352B1 + 0x2, &hud_multiplier_x);
        }

        CPatch::Nop(0x535035, 6);
        CPatch::Nop(0x5352D9, 10);
        DWORD nHudAlignment = static_cast<DWORD>((fres_x - (fres_y * (4.0f / 3.0f))) / 2);
        CPatch::SetUInt(0x7F3D5C, nHudAlignment);
    }
    else
    {
        CPatch::SetUInt(0x6D2224, res_x);
        CPatch::SetUInt(0x6D2228, res_y);

        fres_x = static_cast<float>(res_x);
        fres_y = static_cast<float>(res_y);

        CPatch::SetPointer(0x4370CB + 0x4, &fres_x);
        CPatch::SetPointer(0x4370F7 + 0x4, &fres_y);

        CPatch::SetPointer(0x4370CB + 0x4, &fres_x);
        CPatch::SetPointer(0x4370F7 + 0x4, &fres_y);

        CPatch::SetUInt(0x53510D + 0x1 + 0x240, res_x);
        CPatch::SetUInt(0x53511A + 0x4 + 0x240, res_x);
        CPatch::SetUInt(0x535122 + 0x4 + 0x240, res_x);
        CPatch::SetUInt(0x53512A + 0x4 + 0x240, res_x);

        CPatch::SetUInt(0x53514A + 0x4 + 0x240, res_y);
        CPatch::SetUInt(0x535152 + 0x4 + 0x240, res_y);
        CPatch::SetUInt(0x53515A + 0x4 + 0x240, res_y);

        fAspectRatio = fres_x / fres_y;
        fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * fAspectRatio));

        CPatch::SetFloat(0x691FB0, fDynamicScreenFieldOfViewScale);
        CPatch::SetPointer(0x4EDA80 + 0x2, &fAspectRatio);

        hud_multiplier_x = 1.0f / res_x * (res_y / 480.0f);
        if (hud_patch)
        {
            //CPatch::SetFloat(0x694E1C, hud_multiplier_x);
            //CPatch::SetPointer(0x50D20E + 0x4, &hud_multiplier_x);
            CPatch::SetPointer(0x5354F1 + 0x2, &hud_multiplier_x);

            CPatch::Nop(0x535275, 6);
            CPatch::Nop(0x535519, 10);
            DWORD nHudAlignment = static_cast<DWORD>((fres_x - (fres_y * (4.0f / 3.0f))) / 2);
            CPatch::SetUInt(0x7F3D5C, nHudAlignment);
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
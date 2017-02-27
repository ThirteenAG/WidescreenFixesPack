#include "stdafx.h"

#define _USE_MATH_DEFINES
#include "math.h"
#define DEGREE_TO_RADIAN(fAngle) ((fAngle)* (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) ((fAngle)* 180.0f / (float)M_PI)
#define SCREEN_AR_NARROW (4.0f / 3.0f)
#define SCREEN_FOV_HORIZONTAL 90.0f
#define SCREEN_FOV_VERTICAL (2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))

HWND hWnd;
bool bDelay;

struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fDynamicScreenFieldOfViewScale;
    float fAspectRatio;
    float fHudOffset;
    int Width43;
    float fWidth43;
} Screen;

DWORD WINAPI Init(LPVOID)
{
    auto pattern = hook::pattern("55 8B EC 81 EC 80 00 00 00");
    if (!(pattern.size() > 0) && !bDelay)
    {
        bDelay = true;
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
        return 0;
    }

    if (bDelay)
    {
        while (!(pattern.size() > 0))
            pattern = hook::pattern("55 8B EC 81 EC 80 00 00 00");
    }

    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", -1);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", -1);

    if (!Screen.Width || !Screen.Height) {
        HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO info;
        info.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(monitor, &info);
        Screen.Width = info.rcMonitor.right - info.rcMonitor.left;
        Screen.Height = info.rcMonitor.bottom - info.rcMonitor.top;
    }

    static auto pAspectRatio = *hook::pattern("D9 05 ? ? ? ? 53 8B 5D 08 8B 03 57 51").get_first<float*>(2);
    static auto pFOV = *hook::pattern("D9 05 ? ? ? ? 8B 03 51 8B CB D9 1C 24 ").get_first<float*>(2);

    pattern = hook::pattern("89 86 6C 01 00 00 8B 45 F4"); //0x5E4C55
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            if (Screen.Width == -1 || Screen.Height == -1)
            {
                Screen.Width = *(uint32_t*)(regs.ebp - 0x10);
                Screen.Height = *(uint32_t*)(regs.ebp - 0x0C);
            }
            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
            Screen.fWidth43 = static_cast<float>(Screen.Width43);
            Screen.fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * (Screen.fWidth / Screen.fHeight))) * (1.0f / SCREEN_FOV_HORIZONTAL);

            *(uint32_t*)(regs.esi + 0x16C) = Screen.Width;
            *(uint32_t*)(regs.esi + 0x170) = Screen.Height;
            *pAspectRatio = Screen.fAspectRatio;
            *pFOV = Screen.fDynamicScreenFieldOfViewScale * 1.308f;
        }
    }; injector::MakeInline<SetResHook>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(15));

    pattern = hook::pattern("8B 44 24 04 89 41 10 B0 01 C2 04 00"); //0x57B954
    struct MenuAspectRatioSwitchHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ecx + 0x10) = 2; //*(uint32_t*)(regs.esp + 0x04);
        }
    }; injector::MakeInline<MenuAspectRatioSwitchHook>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(7));

    //pattern = hook::pattern(""); //0x
    //struct HudHook
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        regs.edi = regs.ebx | (regs.edi << 8);
    //        //*(float*)(regs.edx - 0x04) /= 2.0f;
    //    }
    //}; injector::MakeInline<HudHook>(0x5F379F, 0x5F379F + 7/*pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(7)*/);

    return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Init(NULL);
    }
    return TRUE;
}

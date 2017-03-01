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

enum ScreenModes
{
    LETTERBOX = 0,
    UNKNOWN = 1,
    WIDE16BY9 = 2
};

void __fastcall sub_5F3627(uintptr_t _this, uint32_t edx, uintptr_t a2, uintptr_t a3)
{
    //DWORD* unk_933E1C = (DWORD*)0x933E1C;
    //uintptr_t v3 = unk_933E1C[(*(uintptr_t*)(_this + 4)) + 257];
    size_t j = 0;
    //float w = *(float *)(v3 + 0x3F4);
   // float h = *(float *)(v3 + 0x3F8);
    float v4 = 1.0f / Screen.fWidth;
    float v5 = 1.0f / Screen.fHeight;
    uintptr_t v6 = *(uintptr_t*)(a2 + 8);
    if (*(uint32_t*)a2)
    {
        do
        {
            *(float*)(a3 + 00) = *(float*)(v6 + 16);
            *(float*)(a3 + 04) = *(float*)(v6 + 20) + *(float *)(v6 + 28);
            *(float*)(a3 + 16) = *(float*)(v6 + 32);
            *(float*)(a3 + 20) = *(float*)(v6 + 44);
            *(float*)(a3 + 24) = *(float*)(v6 + 16);
            *(float*)(a3 + 28) = *(float*)(v6 + 20);
            *(float*)(a3 + 40) = *(float*)(v6 + 32);
            *(float*)(a3 + 44) = *(float*)(v6 + 36);
            *(float*)(a3 + 48) = *(float*)(v6 + 24) + *(float *)(v6 + 16);
            *(float*)(a3 + 52) = *(float*)(v6 + 20) + *(float *)(v6 + 28);
            *(float*)(a3 + 64) = *(float*)(v6 + 40);
            *(float*)(a3 + 68) = *(float*)(v6 + 44);
            *(float*)(a3 + 72) = *(float*)(v6 + 24) + *(float *)(v6 + 16);
            *(float*)(a3 + 76) = *(float*)(v6 + 20);
            *(float*)(a3 + 88) = *(float*)(v6 + 40);
            *(float*)(a3 + 92) = *(float*)(v6 + 36);

            uintptr_t v8 = a3 + 4;

            struct flt
            {
                float f1;
                float f2;
            };

            flt floats[4];

            for (size_t i = 0; i < 4; i++)
            {
                floats[i].f1 = *(float*)(v8 - 4);
                floats[i].f2 = *(float*)(v8 - 0);
                *(float*)(v8 - 4) = (((*(float*)(v8 - 4) * 2.0f) - 1.0f) - v4);
                *(float*)(v8 + 0) = ((1.0f - (*(float*)v8 * 2.0f)) - (0.0f - v5));
                uint32_t v9 = (uint32_t)(*(float*)(v6 + 8) * 255.0f);
                uint32_t v10 = (uint32_t)(*(float*)(v6 + 4) * 255.0f) & 0xFF | ((((uint32_t)(*(float*)(v6 + 12) * 255.0f) << 8) | (uint32_t)(*(float*)v6 * 255.0) & 0xFF) << 8);
                *(uint32_t*)(v8 + 8) = (uint8_t)v9 | (v10 << 8);
                *(uint32_t*)(v8 + 4) = 0;
                v8 += 24;
            }

            uintptr_t v8a = a3 + 4;
            for (size_t i = 0; i < 4; i++)
            {
                if ((floats[0].f1 == 0.0f && floats[0].f2 == 1.0f) && (floats[1].f1 == 0.0f && floats[1].f2 == 0.0f) &&
                    (floats[2].f1 == 1.0f && floats[2].f2 == 1.0f) && (floats[3].f1 == 1.0f && floats[3].f2 == 0.0f))
                {
                    //to-do: fmv fix / radar
                    *(float *)(v8a - 4) -= 0.5f;
                }
                else
                {
                    *(float *)(v8a - 4) /= (Screen.fAspectRatio / (16.0f / 9.0f));
                }
                v8a += 24;
            }

            ++j;
            a3 += 96;
            v6 += 48;
        } while (j < *(uint32_t*)a2);
    }
}

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
            *(uint32_t*)(regs.ecx + 0x10) = WIDE16BY9; //*(uint32_t*)(regs.esp + 0x04);
        }
    }; injector::MakeInline<MenuAspectRatioSwitchHook>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(7));

    injector::MakeCALL(0x5F75AD, sub_5F3627, true);

    //4016B4 - radar blips
    //4B84F1 - radardisc
    //4B9EF1 - radar renders here

    //pattern = hook::pattern(""); //0x4B8522
    //struct RadarDiscHook
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        //*(float*)(regs.ebp - 0x30) = 0.0f;
    //        //*(float*)(regs.ebp - 0x2C) = 0.0f;
    //        //
    //        //*(float*)(regs.ebp - 0x28) = 1.1f;
    //        //*(float*)(regs.ebp - 0x24) = 1.0f;
    //
    //        *(uintptr_t*)(regs.ebp - 0x14) = *(uintptr_t*)(regs.ebx + 0x2C);
    //    }
    //}; injector::MakeInline<RadarDiscHook>(0x4B8522/*pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(7)*/);


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

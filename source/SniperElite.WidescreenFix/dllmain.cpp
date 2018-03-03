#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t Width43;
    float fWidth43;
    float fHudOffset;
} Screen;

float __fastcall sub_4140E0Hook(int _this, float a2, float a3, float a4, float a5)
{
    if (a2 == 0.0f && a4 == Screen.fWidth)
    {
        a2 += Screen.fHudOffset;
        a4 -= Screen.fHudOffset * 2.0f;
    }

    float v5 = a2 + a4;
    float v7 = a3 + a5;

    *(float *)(_this + 0x00) = a2;
    *(float *)(_this + 0x04) = v5;
    *(float *)(_this + 0x08) = v5;
    *(float *)(_this + 0x0C) = a2;
    *(float *)(_this + 0x10) = a3;
    *(float *)(_this + 0x14) = a3;
    *(float *)(_this + 0x18) = v7;
    *(float *)(_this + 0x1C) = v7;

    *(uint8_t*)(_this + 0x59) = 1;
    return a3;
}

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("BF 94 00 00 00 8B C7");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
    bool bFixText = iniReader.ReadInteger("MAIN", "FixText", 1) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.Width43);

    pattern = hook::pattern("C7 05 ? ? ? ? 00 03 00 00 C7 05 ? ? ? ? 00 04 00 00"); //622B17
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(6), Screen.Height, true);
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(16), Screen.Width, true);
    
    pattern = hook::pattern("8B 0D ? ? ? ? 8B 35 ? ? ? ? 8B C1 99"); //48B0D6
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = Screen.Width;
            regs.esi = Screen.Height;
        }
    }; injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(12));

    pattern = hook::pattern("8B 74 24 10 57 8B 7C 24 10 50 6A 00"); //48B076
    struct SetResHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edi = Screen.Width;
            regs.esi = Screen.Height;
        }
    }; injector::MakeInline<SetResHook2>(pattern.count(1).get(0).get<uint32_t>(1), pattern.count(1).get(0).get<uint32_t>(9));
    injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(0), 0x57, true); //push edi

    pattern = hook::pattern("D9 44 24 04 8B 44 24 04 D9 05"); //4152C0
    struct SetScalingHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)&regs.eax = Screen.fAspectRatio;
        }
    }; injector::MakeInline<SetScalingHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(8));
    injector::WriteMemory(*pattern.count(1).get(0).get<uint32_t*>(10), Screen.fAspectRatio, true);
    injector::WriteMemory(*pattern.count(1).get(0).get<uint32_t*>(15), Screen.fAspectRatio, true);

    if (bFixFOV)
    {
        #undef SCREEN_FOV_HORIZONTAL
        #undef SCREEN_FOV_VERTICAL
        #define SCREEN_FOV_HORIZONTAL 75.0f
        #define SCREEN_FOV_VERTICAL (2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / (4.0f / 3.0f))))
        float fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * Screen.fAspectRatio));
        pattern = hook::pattern("D8 0D ? ? ? ? 83 C4 04 6A 00 68 00 00 80 3F"); //4A15F6
        injector::WriteMemory(*pattern.count(1).get(0).get<uint32_t*>(2), fDynamicScreenFieldOfViewScale, true);
    }

    if (bFixHUD)
    {
        Screen.fHudOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

        pattern = hook::pattern("E8 ? ? ? ? 6A 01 B9 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 8A"); //4D5839
        injector::MakeCALL(pattern.count(1).get(0).get<uint32_t>(0), sub_4140E0Hook, true); //intro screen

        static float fHudScale2 = (0.0009765625f / Screen.fAspectRatio) * (4.0f / 3.0f);
        pattern = hook::pattern("D8 0D ? ? ? ? D9 5E 7C D9 44 24 0C"); //502232
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &fHudScale2, true); //text size and radar arrows

        pattern = hook::pattern("8B 47 04 89 46 44 83 3D ? ? ? ? 01"); //5021C2
        struct HudHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esi + 0x28) += Screen.fHudOffset;
                *(float*)(regs.esi + 0x30) -= Screen.fHudOffset * 2.0f;

                regs.eax = *(uintptr_t*)(regs.edi + 0x4);
                *(uintptr_t*)(regs.esi + 0x44) = regs.eax;
            }
        }; injector::MakeInline<HudHook2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));

        if (bFixText)
        {
            pattern = hook::pattern("DB 05 ? ? ? ? D8 0F D9 1F"); //40E4FD
            struct TextHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(float*)regs.edi *= Screen.fWidth43;
                    *(float*)regs.edi += Screen.fHudOffset;
                }
            }; injector::MakeInline<TextHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(10));

            static int n0 = 0;
            pattern = hook::pattern("DB 05 ? ? ? ? 53 55 57 D9 5C 24 14"); //4D2166
            injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &n0, true);
        }
    }
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
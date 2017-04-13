#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    int32_t Width43;
    float fAspectRatio;
    float fAspectRatioDiff;
    float fFieldOfView;
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
} Screen;

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("8B 4C B4 20 89 15");

    if (pattern.empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
    static bool bCenterMenu = iniReader.ReadInteger("MAIN", "CenterMenu", 1) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fAspectRatioDiff = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
  
    //Resolution
    static int32_t* dword_5A0280 = *pattern.get_first<int32_t*>(6);
    static int32_t* dword_5A0284 = *pattern.get_first<int32_t*>(12);
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_5A0280 = Screen.Width;
            *dword_5A0284 = Screen.Height;
        }
    };
    pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_5A0280)));
    for (size_t i = 0; i < pattern.size(); ++i)
        injector::MakeInline<SetResHook>(pattern.get(i).get<uint32_t>(0), pattern.get(i).get<uint32_t>(6));

    pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_5A0284)));
    for (size_t i = 0; i < pattern.size(); ++i)
        injector::MakeInline<SetResHook>(pattern.get(i).get<uint32_t>(0), pattern.get(i).get<uint32_t>(6));

    pattern = hook::pattern(pattern_str(0xA3, to_bytes(dword_5A0284)));
    injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(6));

    //Aspect Ratio
    pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 33 FF 57 68"); //4616E2
    injector::WriteMemory(pattern.get_first(1), Screen.fAspectRatio, true);

    //FOV
    float fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);
    Screen.fFieldOfView = 180.0f * fDynamicScreenFieldOfViewScale;

    pattern = hook::pattern("D9 05 ? ? ? ? D8 35 ? ? ? ? DE C9 DC C0 D9 96 A4"); //0x45DF17 + 0x2
    injector::WriteMemory(pattern.get_first(2), &Screen.fFieldOfView, true);

    //HUD
    if (bFixHUD)
    {
        auto sub_473520 = [](uintptr_t _this, uint32_t edx, uintptr_t a2)
        {
            *(float*)(a2 + 0x20) = *(float*)(a2 + 0x14) - (*(float*)(_this + 0x128) + 1.0f) * *(float*)(a2 + 0x28) * *(float*)(_this + 0x130) * 0.5f;
            *(float*)(a2 + 0x24) = *(float*)(a2 + 0x18) - (*(float*)(_this + 0x12C) + 1.0f) * *(float*)(a2 + 0x2C) * *(float*)(_this + 0x134) * 0.5f;

            if (bCenterMenu)
                *(float*)(a2 + 0x20) += Screen.fHudOffsetReal;
        };

        //47355C - hud pos
        //injector::MakeCALL(0x45A43D, sub_473520, true); //text pos incl. pause menu
        //injector::MakeCALL(0x47218F, sub_473520, true);
        //injector::MakeCALL(0x472240, sub_473520, true);
        //injector::MakeCALL(0x4722E4, sub_473520, true); // menu graphics + text

        pattern = hook::pattern("E8 ? ? ? ? 8B 46 08 80 CC 20"); //0x4722E4
        injector::MakeCALL(pattern.get_first(0), static_cast<void(__fastcall *)(uintptr_t, uint32_t, uintptr_t)>(sub_473520), true);

        pattern = hook::pattern("8B 44 24 04 8B 54 24 08 89");
        struct HUDHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)&regs.eax = *(float*)(regs.esp + 0x4) * Screen.fAspectRatioDiff;
                *(float*)&regs.edx = *(float*)(regs.esp + 0x8);
            }
        }; 

        for (size_t i = 1; i < 6; i++) // from 2nd match to 6th
        {
            //1 2 - menu text scale
            //3 - ingame text scale
            //4 - ?? menu is a bit messed up
            //5 - menu graphics scale
            //6 - ???
            //7 - ???
            //8 9 - ??

            if (i != 0 && i != 11 && i != 12 && i != 13 && i != 14 && i != 15 && i != 6 && i != 7 && i != 8 && i != 9)
                injector::MakeInline<HUDHook>(pattern.get(i).get<void*>(0), pattern.get(i).get<void*>(6));
        }
    }

    //Tony Hawk's Pro Skater 4 was unable to initialize graphics engine.
    pattern = hook::pattern("68 ? ? ? ? 55 55 51 52"); //531C1C
    auto pData = *pattern.get_first<char*>(1);
    injector::WriteMemoryRaw(pData, "PT5SAEDZWPK0NCMG", std::size("PT5SAEDZWPK0NCMG"), true);

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
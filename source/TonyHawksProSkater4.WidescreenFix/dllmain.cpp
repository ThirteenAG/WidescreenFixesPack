#include "stdafx.h"
#include <random>

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
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
    bool bRandomSongOrderFix = iniReader.ReadInteger("MAIN", "RandomSongOrderFix", 1) != 0;

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
    pattern = hook::pattern("D9 96 A4 00 00 00 5E 59 C3");
    struct FovHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float fov = 0.0f;
            _asm {fst dword ptr[fov]}
            *(float*)(regs.esi + 0xA4) = AdjustFOV(fov, Screen.fAspectRatio);
        }
    }; injector::MakeInline<FovHook>(pattern.get_first(0), pattern.get_first(6));

    //HUD
    if (bFixHUD)
    {
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
            //0 - not relevant
            //1 2 - menu text scale
            //3 - ingame text scale
            //4 - menu graphics scale
            //5 - menu graphics scale
            //6 - ???
            //7 - ???
            //8 9 - ??
            injector::MakeInline<HUDHook>(pattern.get(i).get<void*>(0), pattern.get(i).get<void*>(6));
        }

        pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? 83 E2 10 89 2D"); // 0x43F2D0+6
        injector::WriteMemory(pattern.get_first(6), static_cast<int32_t>(Screen.fHudOffsetReal) + 32, true);
    }

    if (bRandomSongOrderFix)
    {
        pattern = hook::pattern("E8 ? ? ? ? 8B E8 8B 87 D0");
        struct RandomHook
        {
            void operator()(injector::reg_pack& regs)
            {
                std::mt19937 r{ std::random_device{}() };
                std::uniform_int_distribution<uint32_t> uid(0, regs.eax);
                regs.eax = uid(r);
            }
        }; injector::MakeInline<RandomHook>(pattern.get_first(0));
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
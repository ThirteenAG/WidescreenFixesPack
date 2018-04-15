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
    int32_t FOV;
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
} Screen;

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("53 55 56 57 52 6A 00");

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
    pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? B0 01 5F 5E"); //40B349
    static int32_t* dword_851084 = *pattern.get_first<int32_t*>(2);
    static int32_t* dword_851088 = *pattern.get_first<int32_t*>(12);
    injector::MakeNOP(pattern.get_first(-12), 2, true);

    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_851084 = Screen.Width;
            *dword_851088 = Screen.Height;
        }
    };
    injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(20));
    pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 89 2D"); //40B5B8
    injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(20));
    pattern = hook::pattern("A3 ? ? ? ? 8B 04 AE 85 C0"); //40B835 40B84B
    injector::MakeInline<SetResHook>(pattern.count(2).get(0).get<void*>(0));
    injector::MakeInline<SetResHook>(pattern.count(2).get(1).get<void*>(0));

    //Aspect Ratio
    pattern = hook::pattern("89 4E 68 8B 50 04 89 56 6C 8B 46 04"); //0x5591B1
    struct AspectRatioHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esi + 0x68) = *(float*)&regs.ecx / Screen.fAspectRatioDiff;
            *(float*)&regs.edx = *(float*)(regs.eax + 0x04);
        }
    }; injector::MakeInline<AspectRatioHook>(pattern.get_first(0), pattern.get_first(6));

    //HUD
    if (bFixHUD)
    {
        pattern = hook::pattern("D8 0D ? ? ? ? 8B CF 89 5C 24 50 D8"); //0x58DDC8
        injector::WriteMemory(*pattern.get_first<float*>(2), Screen.fHUDScaleX, true);

        pattern = hook::pattern("C1 E6 08 0B F2 89 70 64 A1"); //0x4F65AE
        struct HUDHook //sub_4F62A0
        {
            void operator()(injector::reg_pack& regs)
            {
                if (*(float*)(regs.eax + 0x00) == 0.0f && *(float*)(regs.eax + 0x1C) == 0.0f
                    && (int32_t)(*(float*)(regs.eax + 0x38)) == Screen.Width43
                    && (int32_t)(*(float*)(regs.eax + 0x54)) == Screen.Width43)
                {
                    //blood overlay, maybe more
                    //*(float*)(regs.eax + 0x00) += Screen.fHudOffsetReal;
                    //*(float*)(regs.eax + 0x1C) += Screen.fHudOffsetReal;
                    *(float*)(regs.eax + 0x38) += Screen.fHudOffsetReal * 2.0f;
                    *(float*)(regs.eax + 0x54) += Screen.fHudOffsetReal * 2.0f;
                }
                else
                {
                    *(float*)(regs.eax + 0x00) += Screen.fHudOffsetReal;
                    *(float*)(regs.eax + 0x1C) += Screen.fHudOffsetReal;
                    *(float*)(regs.eax + 0x38) += Screen.fHudOffsetReal;
                    *(float*)(regs.eax + 0x54) += Screen.fHudOffsetReal;
                }

                regs.esi |= regs.edx;
                *(uint32_t*)(regs.eax + 0x64) = regs.esi;
            }
        }; injector::MakeInline<HUDHook>(pattern.get_first(0));

        pattern = hook::pattern("8B 81 A4 00 00 00 89 46"); //0x4F66F0
        struct HUDHook2 //sub_4F65E0
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esi - 0x04) += Screen.fHudOffsetReal;
                regs.eax = *(uint32_t*)(regs.ecx + 0xA4);
            }
        }; injector::MakeInline<HUDHook2>(pattern.get_first(0), pattern.get_first(6));
    }

    if (bRandomSongOrderFix)
    {
        pattern = hook::pattern("E8 ? ? ? ? 8B 96 E8");
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
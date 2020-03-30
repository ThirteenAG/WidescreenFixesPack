#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    int32_t Width43;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHudScaleX;
    float fHudPosX;
} Screen;

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
    bool bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;
    bool bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) == 1;
    bool bFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1) == 1;
    int nScaling = iniReader.ReadInteger("MAIN", "Scaling", 2);
    int ShadowsRes = iniReader.ReadInteger("MISC", "ShadowsRes", 1024);
    bool bShadowsFix = iniReader.ReadInteger("MISC", "ShadowsFix", 1) == 1;
    bool bRearviewMirrorFix = iniReader.ReadInteger("MISC", "RearviewMirrorFix", 1) == 1;
    static auto szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "");
    bool bWriteSettingsToFile = iniReader.ReadInteger("MISC", "WriteSettingsToFile", 1) != 0;
    static int nImproveGamepadSupport = iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0);
    static float fLeftStickDeadzone = iniReader.ReadFloat("MISC", "LeftStickDeadzone", 10.0f);
    static float fRainDropletsScale = iniReader.ReadFloat("MISC", "RainDropletsScale", 0.5f);
    if (szCustomUserFilesDirectoryInGameDir.empty() || szCustomUserFilesDirectoryInGameDir == "0")
        szCustomUserFilesDirectoryInGameDir.clear();

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fHudScaleX = (1.0f / Screen.fWidth * (Screen.fHeight / 480.0f)) * 2.0f;
    Screen.fHudPosX = 640.0f / (640.0f * Screen.fHudScaleX);

    for (size_t i = 0; i < 2; i++)
    {
        //game
        uint32_t* dword_6C27ED = hook::pattern("C7 00 80 02 00 00 C7 01 E0 01 00 00 C2 08 00").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_6C27ED, Screen.Width, true);
        uint32_t dword_6C27F3 = (uint32_t)dword_6C27ED + 6;
        injector::WriteMemory(dword_6C27F3, Screen.Height, true);
        uint32_t* dword_6C2804 = hook::pattern("C7 02 20 03 00 00 C7 00 58 02 00 00 C2 08 00").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_6C2804, Screen.Width, true);
        uint32_t dword_6C280A = (uint32_t)dword_6C2804 + 6;
        injector::WriteMemory(dword_6C280A, Screen.Height, true);
        uint32_t* dword_6C281B = hook::pattern("C7 01 00 04 00 00 C7 02 00 03 00 00 C2 08 00").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_6C281B, Screen.Width, true);
        uint32_t dword_6C2821 = (uint32_t)dword_6C281B + 6;
        injector::WriteMemory(dword_6C2821, Screen.Height, true);
        uint32_t* dword_6C2832 = hook::pattern("C7 00 00 05 00 00 C7 01 C0 03 00 00 C2 08 00").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_6C2832, Screen.Width, true);
        uint32_t dword_6C2838 = (uint32_t)dword_6C2832 + 6;
        injector::WriteMemory(dword_6C2838, Screen.Height, true);
        uint32_t* dword_6C2849 = hook::pattern("C7 02 00 05 00 00 C7 00 00 04 00 00 C2 08 00").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_6C2849, Screen.Width, true);
        uint32_t dword_6C284F = (uint32_t)dword_6C2849 + 6;
        injector::WriteMemory(dword_6C284F, Screen.Height, true);
        uint32_t* dword_6C2860 = hook::pattern("C7 01 40 06 00 00 C7 02 B0 04 00 00 C2 08 00").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_6C2860, Screen.Width, true);
        uint32_t dword_6C2866 = (uint32_t)dword_6C2860 + 6;
        injector::WriteMemory(dword_6C2866, Screen.Height, true);
    }

    //restores missing geometry, causes bugs
    //uint32_t* dword_6C69A7 = hook::pattern("A0 ? ? ? ? 84 C0 74 ? B3 01").count(1).get(0).get<uint32_t>(0);
    //injector::MakeNOP(dword_6C69A7, 5, true);
    //injector::WriteMemory<uint16_t>(dword_6C69A7, 0x00B0i16, true); //mov al,00

    //Autosculpt scaling
    uint32_t* dword_6C9C45 = *hook::pattern("D8 0D ? ? ? ? DA 74 24 18 E8 ? ? ? ? 89 46 04 EB 03").count(1).get(0).get<uint32_t*>(2);
    injector::WriteMemory<float>(dword_6C9C45, 480.0f * Screen.fAspectRatio, true);

    //Arrest blur
    uint32_t* dword_6D4C1B = *hook::pattern("D8 0D ? ? ? ? 8B 4C 24 18 8B 54 24 1C").count(1).get(0).get<uint32_t*>(2);
    injector::WriteMemory<float>(dword_6D4C1B, (1.0f / 640.0f) * ((4.0f / 3.0f) / Screen.fAspectRatio), true);

    //Rain droplets
    static float fRainScaleX = ((0.75f / Screen.fAspectRatio) * (4.0f / 3.0f));
    auto pattern = hook::pattern("D9 44 24 0C D8 44 24 10 8B 4C 24 08 8B 44 24 10 8B D1");
    struct RainDropletsHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float esp0C = *(float*)(regs.esp + 0x0C);
            float esp10 = *(float*)(regs.esp + 0x10);
            _asm fld dword ptr[esp0C]
                _asm fmul dword ptr[fRainScaleX]
                _asm fmul dword ptr[fRainDropletsScale]
                _asm fadd dword ptr[esp10]
        }
    }; injector::MakeInline<RainDropletsHook>(pattern.get_first(0), pattern.get_first(8)); //6D480D

    struct RainDropletsYScaleHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float esp08 = *(float*)(regs.esp + 0x08);
            _asm fmul dword ptr[fRainDropletsScale]
                _asm fadd dword ptr[esp08]
                * (uintptr_t*)(regs.esp + 0x38) = regs.ecx;
        }
    }; injector::MakeInline<RainDropletsYScaleHook>(pattern.get_first(30), pattern.get_first(30 + 8)); //6D482B


    if (ShadowsRes)
    {
        auto dword_8F1CA0 = *hook::pattern("8B 14 85 ? ? ? ? 0F AF 56 5C C1 FA 0F 89 56 5C").count(1).get(0).get<uint32_t*>(3);
        dword_8F1CA0 += 0x1D4;

        uint32_t* dword_6C86B0 = hook::pattern("B8 00 04 00 00 C3").count(2).get(1).get<uint32_t>(1);
        injector::WriteMemory(dword_6C86B0, ShadowsRes, true);
        uint32_t* dword_6C86C0 = hook::pattern("B8 00 04 00 00 C3").count(2).get(1).get<uint32_t>(1);
        injector::WriteMemory(dword_6C86C0, ShadowsRes, true);
        uint32_t* dword_6C8786 = hook::pattern("68 00 04 00 00 68 00 04 00 00 50 FF 51 5C 85 C0 7C 32").count(1).get(0).get<uint32_t>(1);
        injector::WriteMemory(dword_6C8786, ShadowsRes, true);
        uint32_t dword_6C878B = (uint32_t)dword_6C8786 + 5;
        injector::WriteMemory(dword_6C878B, ShadowsRes, true);
        uint32_t* dword_6C87B8 = hook::pattern("68 00 04 00 00 68 00 04 00 00 50 FF 52 5C 85 C0 7D 36").count(1).get(0).get<uint32_t>(1);
        injector::WriteMemory(dword_6C87B8, ShadowsRes, true);
        uint32_t dword_6C87BD = (uint32_t)dword_6C87B8 + 5;
        injector::WriteMemory(dword_6C87BD, ShadowsRes, true);

        uint32_t* dword_93D898 = *hook::pattern("A1 ? ? ? ? 49 3D 02 10 00 00 89 0D").count(1).get(0).get<uint32_t*>(1);
        //char TempStr[10];
        //sprintf(TempStr, "%x %x %x %x", ((DWORD)dword_93D898 >> 0) & 0xff, ((DWORD)dword_93D898 >> 8) & 0xff, ((DWORD)dword_93D898 >> 16) & 0xff, ((DWORD)dword_93D898 >> 24) & 0xff);

        for (size_t i = 0; i < 20; i++)
        {
            uint32_t* dword__93D898 = hook::pattern(pattern_str(to_bytes(dword_93D898))).count(1).get(0).get<uint32_t>(0);
            injector::WriteMemory(dword__93D898, dword_8F1CA0, true);
        }
    }

    //HUD
    if (bFixHUD)
    {
        uint32_t* dword_8AF9A4 = *hook::pattern("D8 0D ? ? ? ? D8 25 ? ? ? ? D9 5C 24 20 D9 46 04").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory<float>(dword_8AF9A4, Screen.fHudScaleX, true);

        //fHudScaleY = *(float*)0x8AF9A0;
        //injector::WriteMemory<float>(0x8AF9A0, fHudScaleY, true);

        for (size_t i = 0; i < 6; i++)
        {
            uint32_t* dword_56FED4 = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43").count(1).get(0).get<uint32_t>(7);
            injector::WriteMemory<float>(dword_56FED4, Screen.fHudPosX, true);
        }

        uint32_t* dword_5A44CC = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43").count(1).get(0).get<uint32_t>(4);
        injector::WriteMemory<float>(dword_5A44CC, Screen.fHudPosX, true);
        uint32_t* dword_894B40 = *hook::pattern("D8 25 ? ? ? ? D9 5C 24 14 DB 05 ? ? ? ? D8 25 ? ? ? ? D9 5C 24 1C 74 20").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory<float>(dword_894B40, Screen.fHudPosX, true);

        //mirror position fix
        uint32_t* dword_6E70C0 = hook::pattern("C7 44 24 70 00 00 E1 43 C7 44 24 74 00 00 98 41 C7 84 24 80 00 00 00 00 00 3E 43").count(1).get(0).get<uint32_t>(4);
        uint32_t dword_6E70FF = (uint32_t)dword_6E70C0 + 63;
        uint32_t dword_6E70D3 = (uint32_t)dword_6E70C0 + 19;
        uint32_t dword_6E70E9 = (uint32_t)dword_6E70C0 + 41;
        injector::WriteMemory<float>(dword_6E70C0, (Screen.fHudPosX - 320.0f) + 190.0f, true);
        injector::WriteMemory<float>(dword_6E70FF, (Screen.fHudPosX - 320.0f) + 190.0f, true);
        injector::WriteMemory<float>(dword_6E70D3, (Screen.fHudPosX - 320.0f) + 450.0f, true);
        injector::WriteMemory<float>(dword_6E70E9, (Screen.fHudPosX - 320.0f) + 450.0f, true);

        if (bRearviewMirrorFix)
        {
            injector::WriteMemory<float>(dword_6E70C0, (Screen.fHudPosX - 320.0f) + 450.0f, true);
            injector::WriteMemory<float>(dword_6E70FF, (Screen.fHudPosX - 320.0f) + 450.0f, true);
            injector::WriteMemory<float>(dword_6E70D3, (Screen.fHudPosX - 320.0f) + 190.0f, true);
            injector::WriteMemory<float>(dword_6E70E9, (Screen.fHudPosX - 320.0f) + 190.0f, true);
        }
    }

    if (bFixFOV)
    {
        static float hor3DScale = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
        static float ver3DScale = 0.75f;
        static float mirrorScale = 0.4f;
        static float f1234 = 1.25f;
        static float f06 = 0.6f;
        static float f1 = 1.0f; // horizontal for vehicle reflection
        static float flt1 = 0.0f;
        static float flt2 = 0.0f;
        static float flt3 = 0.0f;

        if (nScaling)
        {
            hor3DScale /= 1.0511562719f;
            f1234 = 1.25f;
            if (nScaling == 2)
                f1234 = 1.315f;
        }

        uint32_t* dword_6CF4F0 = hook::pattern("DB 40 18 DA 70 14").count(1).get(0).get<uint32_t>(0);
        struct FOVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)(regs.edi + 0x60);

                if (regs.ecx == 1 || regs.ecx == 4) //Headlights stretching, reflections etc 
                {
                    flt1 = hor3DScale;
                    flt2 = f06;
                    flt3 = f1234;
                }
                else
                {
                    if (regs.ecx > 10)
                    {
                        flt1 = f1;
                        flt2 = 0.5f;
                        flt3 = 1.0f;
                        _asm fld ds : f1
                        return;
                    }
                    else
                    {
                        flt1 = 1.0f;
                        flt2 = 0.5f;
                        flt3 = 1.0f;
                    }
                }

                if (regs.ecx == 3) //if rearview mirror
                    _asm fld ds : mirrorScale
                else
                    _asm fld ds : ver3DScale
            }
        }; injector::MakeInline<FOVHook>((uint32_t)dword_6CF4F0 - 6, (uint32_t)dword_6CF4F0 + 6);
        injector::WriteMemory(dword_6CF4F0, 0x9001F983, true); //cmp     ecx, 1

        uint32_t* dword_6CF566 = hook::pattern("D8 3D ? ? ? ? D9 5C 24 20 DB 44 24 30 D8 4C 24 2C").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_6CF566, &flt1, true);

        // FOV being different in menus and in-game fix
        uint32_t* dword_6CF578 = hook::pattern("D8 0D ? ? ? ? E8 ? ? ? ? 8B F8 57 E8").count(2).get(1).get<uint32_t>(2);
        injector::WriteMemory(dword_6CF578, &flt2, true);

        uint32_t* dword_6CF5DC = hook::pattern("D8 3D ? ? ? ? D9 44 24 20 D8 64 24 24").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_6CF5DC, &flt3, true);

        //Fixes vehicle reflection so that they're no longer broken and look exactly as they do without the widescreen fix.
        static uint16_t dx = 16400;
        uint32_t* dword_6DA8AE = hook::pattern("66 8B 15 ? ? ? ? 66 89 93 C4 00 00 00").count(1).get(0).get<uint32_t>(3);
        injector::WriteMemory(dword_6DA8AE, &dx, true);

        //Shadow pop-in fix
        uint32_t* dword_8910C4 = hook::pattern("D8 0D ? ? ? ? D9 5C 24 ? E8 ? ? ? ? 8A").count(1).get(0).get<uint32_t>(2);
        static float f360 = 360.0f;
        injector::WriteMemory((uint32_t)dword_8910C4, &f360, true);

        //Shadow tearing fix
        auto pattern = hook::pattern("0F B7 ? C4 00 00 00");
        static float ShadowRatio;
        ShadowRatio = (Screen.fHeight / Screen.fWidth) / 0.85f;
        struct ShadowFOVHookEAX
        {
            void operator()(injector::reg_pack& regs)
            {
                int ebxC4 = *(int*)(regs.ebx + 0xC4);
                regs.eax = (ebxC4 / ShadowRatio);
            }
        };
        struct ShadowFOVHookECX
        {
            void operator()(injector::reg_pack& regs)
            {
                int ebxC4 = *(int*)(regs.ebx + 0xC4);
                regs.ecx = (ebxC4 / ShadowRatio);
            }
        };
        struct ShadowFOVHookEDX
        {
            void operator()(injector::reg_pack& regs)
            {
                int ebxC4 = *(int*)(regs.ebx + 0xC4);
                regs.edx = (ebxC4 / ShadowRatio);
            }
        };
        injector::MakeInline<ShadowFOVHookEAX>(pattern.count(15).get(11).get<uint32_t>(0), pattern.count(15).get(11).get<uint32_t>(7));
        injector::MakeInline<ShadowFOVHookECX>(pattern.count(15).get(12).get<uint32_t>(0), pattern.count(15).get(12).get<uint32_t>(7));
        injector::MakeInline<ShadowFOVHookECX>(pattern.count(15).get(13).get<uint32_t>(0), pattern.count(15).get(13).get<uint32_t>(7));
        injector::MakeInline<ShadowFOVHookEDX>(pattern.count(15).get(14).get<uint32_t>(0), pattern.count(15).get(14).get<uint32_t>(7));
    }

    uint32_t* dword_57CB82 = hook::pattern("3A 55 34 0F 85 0B 02 00 00 A1").count(1).get(0).get<uint32_t>(0); // HUD
    uint32_t* dword_5696CB = hook::pattern("8A 41 34 38 86 30 03 00 00 74 52 84 C0").count(1).get(0).get<uint32_t>(0); // HUD
    uint32_t* dword_58D883 = hook::pattern("8A 40 34 5F 5E 5D 3B CB 5B 75 12").count(1).get(0).get<uint32_t>(0); // EA Trax
    uint32_t* dword_56885A = hook::pattern("38 48 34 74 31 8B 4E 38 68 7E 78 8E 90").count(1).get(0).get<uint32_t>(0); // Wrong Way Sign
    if (bHudWidescreenMode)
    {
        injector::WriteMemory<uint32_t>(dword_57CB82, 0x0F01FA80, true);
        injector::WriteMemory<uint32_t>(dword_5696CB, 0x389001B0, true);
        injector::WriteMemory<uint32_t>(dword_58D883, 0x5F9001B0, true);
        injector::WriteMemory<uint32_t>(dword_56885A, 0x7401F980, true);

        //Widescreen Splash
        pattern = hook::pattern("8B 46 10 8B 3D ? ? ? ? 53 50");
        injector::MakeNOP(pattern.get_first(-2), 2, true);
        pattern = hook::pattern("E8 ? ? ? ? 84 C0 B8 ? ? ? ? 75 ? B8 ? ? ? ? C3");
        auto aWs_mw_ls_splas = *pattern.count(2).get(1).get<char*>(8);
        auto aMw_ls_splash_0 = *pattern.count(2).get(1).get<char*>(15);
        injector::WriteMemoryRaw(aMw_ls_splash_0, aWs_mw_ls_splas, strlen(aWs_mw_ls_splas), true);

        //issue #343 HD Compatible For Optimal Gaming Logo
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 0C E8 ? ? ? ? 85 C0 75 ? 8B 46");
        injector::MakeCALL(pattern.get_first(0), injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 8B 46 10 68 6D 44 CF 13"), true), true);
    }
    else
    {
        injector::WriteMemory<uint32_t>(dword_57CB82, 0x0F00FA80, true);
        injector::WriteMemory<uint32_t>(dword_5696CB, 0x389000B0, true);
        injector::WriteMemory<uint32_t>(dword_58D883, 0x5F9000B0, true);
        injector::WriteMemory<uint32_t>(dword_56885A, 0x7400F980, true);
    }

    if (bFMVWidescreenMode)
    {
        uint32_t* dword_59A02A = hook::pattern("68 00 00 80 3F 68 00 00 00 3F 68 00 00 00 3F 68 00 00 00 BF 68 00 00 00 BF 8B CB E8 ? ? ? ? 8B 4C 24 18").count(1).get(0).get<uint32_t>(6);
        injector::WriteMemory<float>(dword_59A02A + 0, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
        injector::WriteMemory<float>((uint32_t)dword_59A02A + 5, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
        injector::WriteMemory<float>((uint32_t)dword_59A02A + 10, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
        injector::WriteMemory<float>((uint32_t)dword_59A02A + 15, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
    }

    if (bShadowsFix)
    {
        //dynamic shadow fix that stops them from disappearing when going into tunnels, under bridges by Aero_
        uint32_t* dword_6DE377 = hook::pattern("75 3B C7 05 ? ? ? ? 00 00 80 3F").count(1).get(0).get<uint32_t>(0);
        injector::MakeNOP(dword_6DE377, 2, true);
        injector::WriteMemory((uint32_t)dword_6DE377 + 8, 0, true);

        //Car shadow opacity
        uint32_t* dword_8A0E50 = *hook::pattern("D9 05 ? ? ? ? 8B 54 24 70 D9 1A E9 D1").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory(dword_8A0E50, 60.0f, true);
    }

    if (bRearviewMirrorFix)
    {
        uint32_t* dword_6CFB72 = hook::pattern("75 66 53 E8 ? ? ? ? 83 C4 04 84 C0 74 59").count(1).get(0).get<uint32_t>(0);
        injector::MakeNOP(dword_6CFB72, 2, true);
        uint32_t* dword_6CFBC5 = hook::pattern("75 0D 53 E8 ? ? ? ? 83 C4 04 84 C0 75 06 89 1D").count(1).get(0).get<uint32_t>(0);
        injector::MakeNOP(dword_6CFBC5, 2, true);

        uint32_t* dword_595DDA = hook::pattern("83 F8 02 74 2D 83 F8 03 74 28 83 F8 04 74 23 83 F8 05 74 1E 83 F8 06 74 19").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory<uint8_t>(dword_595DDA, 4, true);
        injector::WriteMemory<uint8_t>((uint32_t)dword_595DDA + 5, 4, true);

        uint32_t* dword_6BFE33 = hook::pattern("74 22 8B 0D ? ? ? ? 85").count(1).get(0).get<uint32_t>(0);
        injector::MakeNOP(dword_6BFE33, 2, true);
    }

    if (!szCustomUserFilesDirectoryInGameDir.empty())
    {
        szCustomUserFilesDirectoryInGameDir = GetExeModulePath<std::string>() + szCustomUserFilesDirectoryInGameDir;

        auto SHGetFolderPathAHook = [](HWND /*hwnd*/, int /*csidl*/, HANDLE /*hToken*/, DWORD /*dwFlags*/, LPSTR pszPath) -> HRESULT
        {
            CreateDirectoryA(szCustomUserFilesDirectoryInGameDir.c_str(), NULL);
            strcpy(pszPath, szCustomUserFilesDirectoryInGameDir.c_str());
            return S_OK;
        };

        auto pattern = hook::pattern("50 6A 00 6A 00 68 ? 80 00 00 6A 00");
        for (size_t i = 0; i < pattern.size(); i++)
        {
            uint32_t* dword_6CBF17 = pattern.get(i).get<uint32_t>(12);
            if (*(BYTE*)dword_6CBF17 != 0xFF)
                dword_6CBF17 = pattern.get(i).get<uint32_t>(14);

            injector::MakeCALL((uint32_t)dword_6CBF17, static_cast<HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR)>(SHGetFolderPathAHook), true);
            injector::MakeNOP((uint32_t)dword_6CBF17 + 5, 1, true);
        }
    }

    if (nImproveGamepadSupport)
    {
        pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 51 A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 04 89 04 24 85 C0 C7 44 24 ? ? ? ? ? 74 22 8B 4C 24 24 8B 54 24 20 51");
        static auto CreateResourceFile = (void*(*)(const char* ResourceFileName, int32_t ResourceFileType, int, int, int)) pattern.get_first(0); //0x0065FD30
        pattern = hook::pattern("8B 44 24 04 56 8B F1 8B 4C 24 0C 89 46 34 89 4E 38 FF 05 ? ? ? ? 8B 46 3C 85 C0 75 14 8B 56 10 C1 EA 03 81 E2 ? ? ? ? 52 8B CE");
        static auto ResourceFileBeginLoading = (void(__thiscall *)(void* rsc, int a1, int a2)) pattern.get_first(0); //0x006616F0;
        static auto LoadResourceFile = [](const char* ResourceFileName, int32_t ResourceFileType, int32_t nUnk1 = 0, int32_t nUnk2 = 0, int32_t nUnk3 = 0, int32_t nUnk4 = 0, int32_t nUnk5 = 0)
        {
            auto r = CreateResourceFile(ResourceFileName, ResourceFileType, nUnk1, nUnk2, nUnk3);
            ResourceFileBeginLoading(r, nUnk4, nUnk5);
        };

        static auto TPKPath = GetThisModulePath<std::string>().substr(GetExeModulePath<std::string>().length());

        if (nImproveGamepadSupport == 1)
            TPKPath += "buttons-xbox.tpk";
        else if (nImproveGamepadSupport == 2)
            TPKPath += "buttons-playstation.tpk";

        static injector::hook_back<void(__cdecl*)()> hb_662B30;
        auto LoadTPK = []()
        {
            LoadResourceFile(TPKPath.c_str(), 1);
            return hb_662B30.fun();
        };

        pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 56 57 B9 ? ? ? ? E8"); //0x6660B6
        hb_662B30.fun = injector::MakeCALL(pattern.get_first(0), static_cast<void(__cdecl*)()>(LoadTPK), true).get();

        //cursor
        //constexpr float cursorScale = 1.0f * (128.0f / 16.0f);
        //pattern = hook::pattern("C7 84 24 34 02 00 00 00 00 80 3F D9"); //5704F8
        //injector::WriteMemory<float>(pattern.get_first(7), cursorScale, true);
        //injector::WriteMemory<float>(pattern.get_first(36), cursorScale, true);
        //injector::WriteMemory<float>(pattern.get_first(47), cursorScale, true);
        //injector::WriteMemory<float>(pattern.get_first(69), cursorScale, true);

        struct PadState
        {
            int32_t LSAxis1;
            int32_t LSAxis2;
            int32_t LTRT;
            int32_t A;
            int32_t B;
            int32_t X;
            int32_t Y;
            int32_t LB;
            int32_t RB;
            int32_t Select;
            int32_t Start;
            int32_t LSClick;
            int32_t RSClick;
        };

        pattern = hook::pattern("C7 45 E0 01 00 00 00 89 5D E4"); //0x7F2AE2
        static uintptr_t ButtonsState = (uintptr_t)*hook::pattern("0F B6 54 24 04 33 C0 B9").count(1).get(0).get<uint32_t*>(8); //0x514B90
        static int32_t* nGameState = (int32_t*)*hook::pattern("83 3D ? ? ? ? 06 ? ? A1").count(1).get(0).get<uint32_t*>(2); //0x925E90
        struct CatchPad
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uintptr_t*)(regs.ebp - 0x20) = 1; //mov     dword ptr [ebp-20h], 1

                PadState* PadKeyPresses = (PadState*)(regs.esi + 0x234); //dpad is PadKeyPresses+0x220

                //Keyboard 
                //008F3584 2
                //008F3514 3 
                //008F34F8 4 
                if (PadKeyPresses != nullptr && PadKeyPresses != (PadState*)0x1)
                {
                    if (PadKeyPresses->Select) //3
                    {
                        *(int32_t*)(ButtonsState + 0xCC) = 1;
                    }
                    else
                    {
                        *(int32_t*)(ButtonsState + 0xCC) = 0;
                    }

                    if (PadKeyPresses->X) //2
                    {
                        *(int32_t*)(ButtonsState + 0x13C) = 1;
                    }
                    else
                    {
                        *(int32_t*)(ButtonsState + 0x13C) = 0;
                    }
                    if (PadKeyPresses->Start) //4
                    {
                        *(int32_t*)(ButtonsState + 0xB0) = 1;
                    }
                    else
                    {
                        *(int32_t*)(ButtonsState + 0xB0) = 0;
                    }
                    if (PadKeyPresses->LSClick && PadKeyPresses->RSClick)
                    {
                        if (*nGameState == 3)
                        {
                            keybd_event(BYTE(VkKeyScan('Q')), 0, 0, 0);
                            keybd_event(BYTE(VkKeyScan('Q')), 0, KEYEVENTF_KEYUP, 0);
                        }
                    }
                }
            }
        }; injector::MakeInline<CatchPad>(pattern.get_first(0), pattern.get_first(7));

        const char* ControlsTexts[] = { " 2", " 3", " 4", " 5", " 6", " 7", " 8", " 9", " 10", " 1", " Up", " Down", " Left", " Right", "X Rotation", "Y Rotation", "X Axis", "Y Axis", "Z Axis", "Hat Switch" };
        const char* ControlsTextsXBOX[] = { "B", "X", "Y", "LB", "RB", "View (Select)", "Menu (Start)", "Left stick", "Right stick", "A", "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right", "Right stick Left/Right", "Right stick Up/Down", "Left stick Left/Right", "Left stick Up/Down", "LT / RT", "D-pad" };
        const char* ControlsTextsPS[] = { "Circle", "Square", "Triangle", "L1", "R1", "Select", "Start", "L3", "R3", "Cross", "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right", "Right stick Left/Right", "Right stick Up/Down", "Left stick Left/Right", "Left stick Up/Down", "L2 / R2", "D-pad" };

        static std::vector<std::string> Texts(ControlsTexts, std::end(ControlsTexts));
        static std::vector<std::string> TextsXBOX(ControlsTextsXBOX, std::end(ControlsTextsXBOX));
        static std::vector<std::string> TextsPS(ControlsTextsPS, std::end(ControlsTextsPS));

        pattern = hook::pattern("8B 44 24 1C 50 E8 ? ? ? ? 83 C4 10 5F 5E 5B C3"); //0x6282D3
        injector::WriteMemory<uint8_t>(pattern.get_first(16 + 5), 0xC3, true);
        struct Buttons
        {
            void operator()(injector::reg_pack& regs)
            {
                auto pszStr = *(char**)(regs.esp + 4);
                auto it = std::find_if(Texts.begin(), Texts.end(), [&](const std::string& str) { std::string s(pszStr); return s.find(str) != std::string::npos; });
                auto i = std::distance(Texts.begin(), it);

                if (it != Texts.end())
                {
                    if (nImproveGamepadSupport != 2)
                        strcpy(pszStr, TextsXBOX[i].c_str());
                    else
                        strcpy(pszStr, TextsPS[i].c_str());
                }
            }
        }; injector::MakeInline<Buttons>(pattern.get_first(16));

        pattern = hook::pattern("5E D8 0D ? ? ? ? 59 C2 08 00"); //0x628A51
        static auto unk_91F7F4 = *hook::get_pattern<void**>("3D ? ? ? ? 89 7C 24 14 0F 8C ? ? ? ? 5F", 1);
        struct MenuRemap
        {
            void operator()(injector::reg_pack& regs)
            {
                static const auto f0078125 = 0.0078125f;
                _asm fmul dword ptr[f0078125]

                    auto dword_91FABC = &unk_91F7F4[178];
                auto dword_91FAF0 = &unk_91F7F4[191];
                auto dword_91FC90 = &unk_91F7F4[295];

                *(uint32_t*)(dword_91FABC) = 0; // "Enter"; changed B to A
                *(uint32_t*)(dword_91FAF0) = 1; // "ESC"; changed X to B
                *(uint32_t*)(dword_91FC90) = 3; // "1"; changed A to Y
            }
        }; injector::MakeInline<MenuRemap>(pattern.get_first(1), pattern.get_first(7));
    }

    if (fLeftStickDeadzone)
    {
        // DInput [ 0 32767 | 32768 65535 ] 
        fLeftStickDeadzone /= 200.0f;

        pattern = hook::pattern("89 86 34 02 00 00 8D 45 D4"); //0x7F29B2
        struct DeadzoneHookX
        {
            void operator()(injector::reg_pack& regs)
            {
                double dStickState = (double)regs.eax / 65535.0;
                dStickState -= 0.5;
                if (std::abs(dStickState) <= fLeftStickDeadzone)
                    dStickState = 0.0;
                dStickState += 0.5;
                *(uint32_t*)(regs.esi + 0x234) = (uint32_t)(dStickState * 65535.0);
            }
        }; injector::MakeInline<DeadzoneHookX>(pattern.get_first(0), pattern.get_first(6));

        struct DeadzoneHookY
        {
            void operator()(injector::reg_pack& regs)
            {
                double dStickState = (double)regs.edx / 65535.0;
                dStickState -= 0.5;
                if (std::abs(dStickState) <= fLeftStickDeadzone)
                    dStickState = 0.0;
                dStickState += 0.5;
                *(uint32_t*)(regs.esi + 0x238) = (uint32_t)(dStickState * 65535.0);
            }
        }; injector::MakeInline<DeadzoneHookY>(pattern.get_first(18 + 0), pattern.get_first(18 + 6));
    }

    if (bWriteSettingsToFile)
    {
        char szSettingsSavePath[MAX_PATH];
        auto GetFolderPathpattern = hook::pattern("50 6A 00 6A 00 68 ? 80 00 00 6A 00");
        uintptr_t GetFolderPathCallDest = injector::GetBranchDestination(GetFolderPathpattern.count(1).get(0).get<uintptr_t>(14), true).as_int();
        injector::stdcall<HRESULT(HWND, int, HANDLE, DWORD, LPSTR)>::call(GetFolderPathCallDest, NULL, 0x8005, NULL, NULL, szSettingsSavePath);
        strcat(szSettingsSavePath, "\\NFS Most Wanted");
        strcat(szSettingsSavePath, "\\Settings.ini");

        RegistryWrapper("Need for Speed", szSettingsSavePath);
        auto RegIAT = *hook::pattern("FF 15 ? ? ? ? 8D 54 24 04 52").get(0).get<uintptr_t*>(2);
        injector::WriteMemory(&RegIAT[0], RegistryWrapper::RegCreateKeyA, true);
        injector::WriteMemory(&RegIAT[1], RegistryWrapper::RegOpenKeyExA, true);
        injector::WriteMemory(&RegIAT[2], RegistryWrapper::RegCloseKey, true);
        injector::WriteMemory(&RegIAT[3], RegistryWrapper::RegSetValueExA, true);
        injector::WriteMemory(&RegIAT[4], RegistryWrapper::RegQueryValueExA, true);
        RegistryWrapper::AddPathWriter("Install Dir", "InstallDir", "Path");
        RegistryWrapper::AddDefault("@", "INSERTYOURCDKEYHERE");
        RegistryWrapper::AddDefault("CD Drive", "D:\\");
        RegistryWrapper::AddDefault("CacheSize", "2936691712");
        RegistryWrapper::AddDefault("SwapSize", "73400320");
        RegistryWrapper::AddDefault("Language", "English");
        RegistryWrapper::AddDefault("StreamingInstall", "0");
        RegistryWrapper::AddDefault("VTMode", "0");
        RegistryWrapper::AddDefault("VERSION", "0");
        RegistryWrapper::AddDefault("SIZE", "0");
        RegistryWrapper::AddDefault("g_CarEnvironmentMapEnable", "3");
        RegistryWrapper::AddDefault("g_CarEnvironmentMapUpdateData", "1");
        RegistryWrapper::AddDefault("g_RoadReflectionEnable", "3");
        RegistryWrapper::AddDefault("g_MotionBlurEnable", "1");
        RegistryWrapper::AddDefault("g_ParticleSystemEnable", "1");
        RegistryWrapper::AddDefault("g_WorldLodLevel", "3");
        RegistryWrapper::AddDefault("g_CarLodLevel", "1");
        RegistryWrapper::AddDefault("g_OverBrightEnable", "1");
        RegistryWrapper::AddDefault("g_FSAALevel", "7");
        RegistryWrapper::AddDefault("g_RainEnable", "1");
        RegistryWrapper::AddDefault("g_TextureFiltering", "2");
        RegistryWrapper::AddDefault("g_RacingResolution", "1");
        RegistryWrapper::AddDefault("g_PerformanceLevel", "5");
        RegistryWrapper::AddDefault("g_VSyncOn", "0");
        RegistryWrapper::AddDefault("g_ShadowDetail", "2");
        RegistryWrapper::AddDefault("g_VisualTreatment", "1");
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("C7 00 80 02 00 00 C7 01 E0 01 00 00 C2 08 00"));
        });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}

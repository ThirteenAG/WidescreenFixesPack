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
    int32_t nScaling = iniReader.ReadInteger("MAIN", "Scaling", 2);
    bool bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) != 0;
    int32_t nFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1);
    int32_t nWindowedMode = iniReader.ReadInteger("MISC", "WindowedMode", 0);
    bool bSkipIntro = iniReader.ReadInteger("MISC", "SkipIntro", 1) != 0;
    bool bLightingFix = iniReader.ReadInteger("MISC", "LightingFix", 0) != 0;
    bool bCarShadowFix = iniReader.ReadInteger("MISC", "CarShadowFix", 0) != 0;
    bool bExperimentalCrashFix = iniReader.ReadInteger("MISC", "CrashFix", 0) != 0;
    static auto szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "0");
    bool bWriteSettingsToFile = iniReader.ReadInteger("MISC", "WriteSettingsToFile", 1) != 0;
    static int32_t nImproveGamepadSupport = iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0);
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

    //Resolution
    for (size_t i = 0; i < 2; i++)
    {
        //addresses from MW
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

    //Autosculpt scaling
    uint32_t* dword_9E9B68 = *hook::pattern("D8 0D ? ? ? ? DA 74 24 18 E8 ? ? ? ? 89 46 04 EB 03").count(1).get(0).get<uint32_t*>(2);
    injector::WriteMemory<float>(dword_9E9B68, 480.0f * Screen.fAspectRatio, true);

    //Mouse cursor
    //uint32_t* dword_9E8F84 = *hook::pattern("D8 0D ? ? ? ? DA 74 24 10 DA 64 24 14 E8 ? ? ? ? 89 46 08").count(1).get(0).get<uint32_t*>(2);
    //injector::WriteMemory<float>(dword_9E8F84, (float)Screen.Height, true);

    //Arrest blur
    uint32_t* dword_9D0584 = *hook::pattern("D8 0D ? ? ? ? 8B 4C 24 14 8B 54").count(1).get(0).get<uint32_t*>(2);
    injector::WriteMemory<float>(dword_9D0584, (1.0f / 640.0f) * ((4.0f / 3.0f) / Screen.fAspectRatio), true);

    //Autosave icon
    uint32_t* dword_5CD4B0 = *hook::pattern("FF 24 85 ? ? ? ? 8B 0D ? ? ? ? 8B 11 6A 00 FF 92 98 00 00 00").count(1).get(0).get<uint32_t*>(3);
    injector::WriteMemory(dword_5CD4B0 + 2, hook::pattern("8B CE E8 ? ? ? ? 8B 0D ? ? ? ? 8B F1 E8").count(1).get(0).get<uint32_t*>(0), true);

    //Water Reflections fix
    static uint32_t n768 = 768;
    static uint32_t n320 = 320;
    uint32_t* dword_71A9B1 = hook::pattern("8B 0D ? ? ? ? B8 56 55 55 55 89 15 ? ? ? ? F7 E9").count(2).get(1).get<uint32_t>(2);
    injector::WriteMemory(dword_71A9B1, &n768, true);
    auto pattern = hook::pattern("8B ? ? ? ? ? ? 50 FF ? ? 85 C0");
    uint32_t* dword_71AA22 = pattern.count(4).get(2).get<uint32_t>(2);
    uint32_t* dword_71AA72 = pattern.count(4).get(3).get<uint32_t>(2);
    injector::WriteMemory(dword_71AA22, &n320, true);
    injector::WriteMemory(dword_71AA72, &n320, true);

    //EA HD
    uint32_t* dword_9D51D8 = *hook::pattern("6A 01 6A 14 68 ? ? ? ? 8B CE").count(1).get(0).get<uint32_t*>(5);
    uint32_t* dword_5BB83B = hook::pattern("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 85 C0").count(5).get(4).get<uint32_t>(1);
    injector::WriteMemory(dword_5BB83B, dword_9D51D8, true);
    pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 5E 59 C3");
    uint32_t* dword_5BD4A3 = pattern.count(3).get(1).get<uint32_t>(1);
    injector::WriteMemory(dword_5BD4A3, dword_9D51D8, true);
    uint32_t* dword_5BD4B0 = pattern.count(3).get(2).get<uint32_t>(1);
    injector::WriteMemory(dword_5BD4B0, dword_9D51D8, true);
    injector::WriteMemory(hook::pattern("68 ? ? ? ? E8 ? ? ? ? 8B 4E 0C 50 51").count(3).get(2).get<uint32_t>(1), 0, true);

    //Lighting Fix Update (mirror)
    pattern = hook::pattern("C7 05 ? ? ? ? 01 00 00 00 C7 05 ? ? ? ? 00 00 80");
    uint32_t* dword_72E382 = pattern.count(1).get(0).get<uint32_t>(6);
    injector::WriteMemory(dword_72E382, 0, true);
    uint32_t* dword_748C99 = hook::pattern("0F 85 A5 00 00 00 A1 ? ? ? ? C7 44 24 14 00 00 00 00").count(1).get(0).get<uint32_t>(1);
    injector::WriteMemory<uint8_t>(dword_748C99, 0x84, true);
    pattern = hook::pattern("7E 52 8B 0D ? ? ? ? 83 79 04 01");
    uint32_t* dword_748D4B = pattern.count(1).get(0).get<uint32_t>(0);
    injector::MakeNOP(dword_748D4B, 2, true);
    uint32_t* dword_748D57 = pattern.count(1).get(0).get<uint32_t>(12);
    injector::WriteMemory<uint8_t>(dword_748D57, 0x74, true);
    uint32_t* dword_748D5A = pattern.count(1).get(0).get<uint32_t>(15);
    uint32_t* dword_AB0958 = *hook::pattern("A1 ? ? ? ? 3B C6 74 0C 8B 08 50 FF 51 08 89 35").get(0).get<uint32_t*>(1);
    injector::WriteMemory(dword_748D5A, dword_AB0958, true);
    uint32_t* dword_748D66 = pattern.count(1).get(0).get<uint32_t>(27);
    uint32_t* dword_AB0C04 = *hook::pattern("68 ? ? ? ? 6A 00 6A 15 6A 01 6A 01").get(0).get<uint32_t*>(1);
    injector::WriteMemory(dword_748D66, dword_AB0C04, true);
    uint32_t* dword_748D6E = pattern.count(1).get(0).get<uint32_t>(35);
    uint32_t* dword_AB095C = *hook::pattern("A1 ? ? ? ? 8B 08 68 ? ? ? ? 6A 00 50 FF 51 48").count(4).get(3).get<uint32_t*>(1);
    injector::WriteMemory(dword_748D6E, dword_AB095C, true);
    uint32_t* dword_748D74 = pattern.count(1).get(0).get<uint32_t>(41);
    injector::WriteMemory<uint8_t>(dword_748D74, 0x59, true);
    // activates 00AB0C04 pointer
    pattern = hook::pattern("E8 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 85 C0 7E 05");
    injector::MakeNOP(pattern.count(2).get(1).get<uint32_t>(22), 2, true); //72B4E1
    // prevents crash for Intel gpu
    pattern = hook::pattern("75 2E A1 ? ? ? ? 8B 10 6A 00 68 ? ? ? ? 6A 00");
    injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 2, true); //715180    
    //activates 00AB0C04
    pattern = hook::pattern("7E 05 E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8");
    injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 2, true); // 0071B3E6

    //PiP pixelation
    uint32_t* dword_70DB0C = hook::pattern("8B 0D ? ? ? ? B8 56 55 55 55").count(1).get(0).get<uint32_t>(2);
    injector::WriteMemory(dword_70DB0C, &Screen.Width43, true);

    //World map cursor
    uint32_t* dword_570DCD = hook::pattern("75 33 D9 44 24 14 D8 5C 24 08 DF E0 F6 C4 41").count(1).get(0).get<uint32_t>(0);
    injector::MakeNOP(dword_570DCD, 2, true);
    uint32_t* dword_570DDC = hook::pattern("7A 24 D9 44 24 18 D8 5C 24 04 DF E0").count(1).get(0).get<uint32_t>(0);
    injector::MakeNOP(dword_570DDC, 2, true);

    //Rain droplets
    static float fRainScaleX = ((0.75f / Screen.fAspectRatio) * (4.0f / 3.0f));
    pattern = hook::pattern("D9 44 24 08 D8 44 24 10 8B 4C 24 0C 8B 44 24 10 8B D1");
    struct RainDropletsHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float esp08 = *(float*)(regs.esp + 0x08);
            float esp10 = *(float*)(regs.esp + 0x10);
            _asm {fld  dword ptr[esp08]}
            _asm {fmul dword ptr[fRainScaleX]}
            _asm {fmul dword ptr[fRainDropletsScale]}
            _asm {fadd dword ptr[esp10]}
        }
    }; injector::MakeInline<RainDropletsHook>(pattern.get_first(0), pattern.get_first(8)); //0x722E78

    struct RainDropletsYScaleHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float esp0C = *(float*)(regs.esp + 0x0C);
            _asm {fmul dword ptr[fRainDropletsScale]}
            _asm {fadd dword ptr[esp0C]}
            *(uintptr_t*)(regs.esp + 0x34) = regs.eax;
        }
    }; injector::MakeInline<RainDropletsYScaleHook>(pattern.get_first(36), pattern.get_first(36 + 8)); //0x722E9C

    //For ini options
    auto GetFolderPathpattern = hook::pattern("50 6A 00 6A 00 68 ? 80 00 00 6A 00");

    //HUD
    if (bFixHUD)
    {
        Screen.fHudScaleX = (1.0f / Screen.Width * (Screen.Height / 480.0f)) * 2.0f;
        Screen.fHudPosX = 640.0f / (640.0f * Screen.fHudScaleX);

        uint32_t* dword_9E8F8C = *hook::pattern("D8 0D ? ? ? ? D8 25 ? ? ? ? D9 5C 24 20 D9 46 04 D8 0D ? ? ? ? D8 25 ? ? ? ? D9 E0").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory<float>(dword_9E8F8C, Screen.fHudScaleX, true);

        //fHudScaleY = *(float*)0x9E8F88;
        //injector::WriteMemory<float>(0x9E8F88, fHudScaleY);

        for (size_t i = 0; i < 4; i++)
        {
            uint32_t* dword_598DC0 = hook::pattern("C7 ? ? ? ? 00 00 00 00 A0 43 C7 ? ? ? ? 00 00 00 00 70 43").count(1).get(0).get<uint32_t>(7);
            injector::WriteMemory<float>(dword_598DC0, Screen.fHudPosX, true);
        }

        for (size_t i = 0; i < 2; i++)
        {
            uint32_t* dword_5A18BA = hook::pattern("C7 ? ? ? 00 00 A0 43 C7 ? ? ? 00 00 70 43").count(1).get(0).get<uint32_t>(4);
            injector::WriteMemory<float>(dword_5A18BA, Screen.fHudPosX, true);
        }

        uint32_t* dword_A604AC = *hook::pattern("D8 05 ? ? ? ? 89 44 24 18 D9 44 24 18").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory<float>(dword_A604AC, Screen.fHudPosX, true);

        uint32_t* dword_9C778C = *hook::pattern("D8 25 ? ? ? ? 8D 46 34 50 D9 5C 24 20").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory<float>(dword_9C778C, Screen.fHudPosX, true);

        //mini_map_route fix
        uint32_t* dword_9D5F3C = *hook::pattern("D8 05 ? ? ? ? D9 5C 24 7C D9 86 B8 00 00 00 D8 05 ? ? ? ? D9 9C 24 80 00 00 00").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory<float>(dword_9D5F3C, (Screen.fHudPosX - 320.0f) + 384.0f, true);
    }

    if (bFixFOV)
    {
        static float hor3DScale = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
        static float ver3DScale = 0.75f;
        static float mirrorScale = 0.675f;
        static float f1234 = 1.25f;
        static float f06 = 0.6f;
        static float flt1 = 0.0f;
        static float flt2 = 0.0f;
        static float flt3 = 0.0f;

        uint32_t* dword_71B858 = hook::pattern("DB 05 ? ? ? ? 8B 45 08 83 F8 01 DA 35 ? ? ? ? D9 5C 24 24").count(1).get(0).get<uint32_t>(0);
        struct FOVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)(regs.ebp + 8);

                if (regs.eax == 1 || regs.eax == 4) //Headlights stretching, reflections etc 
                {
                    flt1 = hor3DScale;
                    flt2 = f06;
                    flt3 = f1234;
                }
                else
                {
                    flt1 = 1.0f;
                    flt2 = 0.5f;
                    flt3 = 1.0f;
                }

                if (regs.eax == 3) //if rearview mirror
                    _asm fld ds : mirrorScale
                else
                    _asm fld ds : ver3DScale
            }
        }; injector::MakeInline<FOVHook>((uint32_t)dword_71B858, (uint32_t)dword_71B858 + 18);
        injector::WriteMemory((uint32_t)dword_71B858 + 5, 0x9001F883, true); //cmp     eax, 1

        uint32_t* dword_71B8DA = hook::pattern("D8 3D ? ? ? ? D9 5C 24 30 DB 44 24 20 D8 4C 24 2C").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_71B8DA, &flt1, true);

        // FOV being different in menus and in-game fix
        uint32_t* dword_71B8EC = hook::pattern("D8 0D ? ? ? ? E8 ? ? ? ? 8B E8 55 E8").count(2).get(1).get<uint32_t>(2);
        injector::WriteMemory(dword_71B8EC, &flt2, true);

        uint32_t* dword_71B923 = hook::pattern("D8 3D ? ? ? ? D9 5C 24 2C 8B 54 24 2C 52 50 55").count(1).get(0).get<uint32_t>(2);
        injector::WriteMemory(dword_71B923, &flt3, true);

        if (nScaling)
        {
            hor3DScale /= 1.0511562719f;
            if (nScaling == 2)
                f1234 = 1.315f;
        }
    }

    if (bHudWidescreenMode)
    {
        uint32_t* dword_5DC508 = hook::pattern("0F 95 C1 3A C1 75 2B 8B 0D ? ? ? ? 3B CE").count(1).get(0).get<uint32_t>(1);
        injector::WriteMemory<uint8_t>(dword_5DC508, 0x94, true);
        uint32_t* dword_5D52B3 = hook::pattern("0F 95 C0 3A C8 0F 84 C7 00 00 00 84 C0 88 86 41 03 00 00").count(1).get(0).get<uint32_t>(1);
        injector::WriteMemory<uint8_t>(dword_5D52B3, 0x94, true);
        uint32_t* dword_5B6BAE = hook::pattern("74 07 68 ? ? ? ? EB 05 68 ? ? ? ? E8 ? ? ? ? 8B ? ? ? ? ? 8B E8 8B 03 83 C4 04").count(1).get(0).get<uint32_t>(0);
        injector::WriteMemory<uint8_t>(dword_5B6BAE, 0x75, true);
        uint32_t* dword_5B6B5B = hook::pattern("74 07 68 ? ? ? ? EB 05 68 ? ? ? ? E8 ? ? ? ? 8B ? ? ? ? ? 89 44 24 14 8B 03 83 C4 04").count(1).get(0).get<uint32_t>(0);
        injector::WriteMemory<uint8_t>(dword_5B6B5B, 0x75, true);

        //PiP/rearview mirror
        uint32_t* dword_750DE7 = hook::pattern("68 CD CC 8C 3E 68 33 33 33 3F 8D 54 24 10 68 F4 FD D4 3C 52").count(1).get(0).get<uint32_t>(1);
        injector::WriteMemory<float>(dword_750DE7, 0.2799999714f, true);
        uint32_t dword_750DF5 = (uint32_t)dword_750DE7 + 14;
        injector::WriteMemory<float>(dword_750DF5, -0.1649999917f, true);

        static float fMirrorScaling = 1.0f * ((4.0f / 3.0f) / ((float)Screen.Width / (float)Screen.Height));
        pattern = hook::pattern("D9 5E 4C 89 46 5C 5E 5B C3");
        struct MirrorFix
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float *)(regs.esi + 0x00) = *(float *)(regs.esi + 0x00) * fMirrorScaling;
                *(float *)(regs.esi + 0x18) = *(float *)(regs.esi + 0x18) * fMirrorScaling;
                *(float *)(regs.esi + 0x30) = *(float *)(regs.esi + 0x30) * fMirrorScaling;
                *(float *)(regs.esi + 0x48) = *(float *)(regs.esi + 0x48) * fMirrorScaling;
            }
        }; injector::MakeInline<MirrorFix>(pattern.get_first(6)); //750C23
        injector::WriteMemory(pattern.get_first(6 + 5), 0x90C35B5E, true); //pop esi pop ebx ret
    }

    if (nFMVWidescreenMode)
    {
        if (nFMVWidescreenMode > 1)
        {
            uint32_t* dword_598EB9 = hook::pattern("68 00 00 80 3F 68 00 00 00 3F 68 00 00 00 3F 68 00 00 00 BF 68 00 00 00 BF 8B CB E8 ? ? ? ? 8B CB C7").count(1).get(0).get<uint32_t>(6);
            injector::WriteMemory<float>((uint32_t)dword_598EB9 + 0, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
            injector::WriteMemory<float>((uint32_t)dword_598EB9 + 5, (0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
            injector::WriteMemory<float>((uint32_t)dword_598EB9 + 10, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
            injector::WriteMemory<float>((uint32_t)dword_598EB9 + 15, -(0.5f / ((4.0f / 3.0f) / (16.0f / 9.0f))), true);
        }
        else
        {
            //Native Fullscreen FMVs
            uint32_t* dword_5AB6D7 = hook::pattern("74 3C ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 68 D1 BC D5 FF").count(1).get(0).get<uint32_t>(0);
            injector::WriteMemory<uint8_t>(dword_5AB6D7, 0xEB, true);
            uint32_t* dword_5BB818 = hook::pattern("74 6A 8B CE E8 ? ? ? ? 84 C0 75 5F 68 ? ? ? ? 68").count(1).get(0).get<uint32_t>(0);
            injector::MakeNOP(dword_5BB818, 2, true);
            uint32_t* dword_5BD4A1 = hook::pattern("74 0D 68 ? ? ? ? E8 ? ? ? ? 5E 59 C3").count(1).get(0).get<uint32_t>(0);
            injector::MakeNOP(dword_5BD4A1, 2, true);
        }
    }

    if (bLightingFix)
    {
        uint32_t* dword_7497B9 = hook::pattern("8B 0D ? ? ? ? 83 F8 06 89 0D ? ? ? ? C7 05 ? ? ? ? CD CC CC 3E").count(2).get(1).get<uint32_t>(2);
        static float f25 = 2.5f;
        injector::WriteMemory((uint32_t)dword_7497B9, &f25, true);
        static float f15 = 1.5f;
        uint32_t* dword_748A70 = hook::pattern("A1 ? ? ? ? A3 ? ? ? ? 83 3D ? ? ? ? 06 C7 05 ? ? ? ? CD CC CC 3E").count(2).get(1).get<uint32_t>(1);
        injector::WriteMemory(dword_748A70, &f15, true);
        uint32_t* dword_73E7CB = *hook::pattern("C7 05 ? ? ? ? CD CC CC 3E B8 ? ? ? ? 74 05 B8 ? ? ? ? C3").count(1).get(0).get<uint32_t*>(11);
        injector::WriteMemory<float>(dword_73E7CB, 1.6f, true);
    }

    if (bCarShadowFix)
    {
        uint32_t* dword_7BEA3A = hook::pattern("D8 05 ? ? ? ? DC C0 E8 ? ? ? ? 85 C0 7F 04 33 C0").count(1).get(0).get<uint32_t>(2);
        static float f60 = 60.0f;
        injector::WriteMemory(dword_7BEA3A, &f60, true);
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

        for (size_t i = 0; i < GetFolderPathpattern.size(); i++)
        {
            uint32_t* dword_6CBF17 = GetFolderPathpattern.get(i).get<uint32_t>(12);
            if (*(BYTE*)dword_6CBF17 != 0xFF)
                dword_6CBF17 = GetFolderPathpattern.get(i).get<uint32_t>(14);

            injector::MakeCALL((uint32_t)dword_6CBF17, static_cast<HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR)>(SHGetFolderPathAHook), true);
            injector::MakeNOP((uint32_t)dword_6CBF17 + 5, 1, true);
        }
    }

    if (nWindowedMode)
    {
        int32_t DesktopX = 0;
        int32_t DesktopY = 0;
        std::tie(DesktopX, DesktopY) = GetDesktopRes();

        static tagRECT REKT;
        REKT.left = (LONG)(((float)DesktopX / 2.0f) - ((float)Screen.Width / 2.0f));
        REKT.top = (LONG)(((float)DesktopY / 2.0f) - ((float)Screen.Height / 2.0f));
        REKT.right = (LONG)(REKT.left + Screen.Width);
        REKT.bottom = (LONG)(REKT.top + Screen.Height);

        auto pattern = hook::pattern("A1 ? ? ? ? 33 FF 3B C7 74 ?"); //0xAB0AD4
        injector::WriteMemory(*pattern.count(3).get(2).get<uint32_t*>(1), 1, true);

        pattern = hook::pattern("B8 64 00 00 00 89 44 24 28"); //0x7309B4
        injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(27), 3, true);
        struct WindowedMode
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uint32_t*)(regs.esp + 0x28) = REKT.left;
                *(uint32_t*)(regs.esp + 0x2C) = REKT.top;
                regs.eax = REKT.right;
                regs.ecx = REKT.bottom;
            }
        }; injector::MakeInline<WindowedMode>(pattern.get_first(0), pattern.get_first(16));

        if (nWindowedMode > 1)
        {
            auto SetWindowLongHook = [](HWND hWndl, int nIndex, LONG dwNewLong)
            {
                dwNewLong |= WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;

                SetWindowLong(hWndl, nIndex, dwNewLong);
                SetWindowPos(hWndl, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
            };

            auto pattern = hook::pattern("68 00 00 00 10 6A F0 50"); //0x730B8A
            injector::MakeNOP(pattern.get_first(8), 6, true);
            injector::MakeCALL(pattern.get_first(8), static_cast<void(WINAPI*)(HWND, int, LONG)>(SetWindowLongHook), true);
        }

        if (bSkipIntro)
        {
            auto pattern = hook::pattern("A3 ? ? ? ? A1 ? ? ? ? 6A 20 50"); //0xA9E6D8
            injector::WriteMemory(*pattern.count(3).get(2).get<uint32_t*>(1), 1, true);
            pattern = hook::pattern("A1 ? ? ? ? 85 C0 74 ? 57 8B F8 E8"); //0xA97BC0
            injector::WriteMemory(*pattern.count(1).get(0).get<uint32_t*>(1), 1, true);
        }
    }

    if (bExperimentalCrashFix)
    {
        auto pattern = hook::pattern("75 0B 8B 06 8B CE FF 50 14 3B D8"); //0x59606D
        injector::MakeNOP(pattern.get_first(0), 2, true);
        pattern = hook::pattern("74 20 8B 44 24 20 8B 55 00 6A 00 50 6A 00"); //0x5960A9
        injector::MakeNOP(pattern.get_first(0), 2, true);
    }

    if (nImproveGamepadSupport)
    {
        pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 51 A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 04 89 04 24 85 C0 C7 44 24 ? ? ? ? ? 74 22 8B 4C 24 24 8B 54 24 20 51");
        static auto CreateResourceFile = (void*(*)(const char* ResourceFileName, int32_t ResourceFileType, int, int, int)) pattern.get_first(0); //0x006B32C0
        pattern = hook::pattern("8B 44 24 04 56 8B F1 8B 4C 24 0C 89 46 34 89 4E 38 FF 05 ? ? ? ? 8B 46 3C 85 C0 75 14 8B 56 10 C1 EA 03 81 E2 ? ? ? ? 52 8B CE");
        static auto ResourceFileBeginLoading = (void(__thiscall *)(void* rsc, int a1, int a2)) pattern.get_first(0); //0x006B5910;
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

        static injector::hook_back<void(__cdecl*)()> hb_6B6D40;
        auto LoadTPK = []()
        {
            LoadResourceFile(TPKPath.c_str(), 1);
            return hb_6B6D40.fun();
        };

        pattern = hook::pattern("E8 ? ? ? ? 8B 35 ? ? ? ? 6A 04 FF D6 83 C4 04 85 C0 74 08 C7 00"); //0x6B7736
        hb_6B6D40.fun = injector::MakeCALL(pattern.get_first(0), static_cast<void(__cdecl*)()>(LoadTPK), true).get();

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

        pattern = hook::pattern("C7 45 E0 01 00 00 00 89 5D E4"); //0x892BFD
        static uintptr_t ButtonsState = (uintptr_t)*hook::pattern("0F B6 54 24 04 33 C0 B9").get(0).get<uint32_t*>(8); //0xA5E058
        static int32_t* nGameState = (int32_t*)*hook::pattern("83 3D ? ? ? ? 06 ? ? A1").get(0).get<uint32_t*>(2); //0xA99BBC
        struct CatchPad
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uintptr_t*)(regs.ebp - 0x20) = 1; //mov     dword ptr [ebp-20h], 1

                PadState* PadKeyPresses = (PadState*)(regs.esi + 0x234); //dpad is PadKeyPresses+0x220

                //Keyboard 
                //00A5E15C T
                //00A5E124 M
                //00A5E1B0 1
                //00A5E194 2
                //00A5E140 3 
                //00A5E178 4 
                //00A5E1CC 9 
                //00A5E1E8 0 
                //00A5E108 Space
                //00A5E0D0 ESC
                //00A5E0B4 Enter
                if (PadKeyPresses != nullptr && PadKeyPresses != (PadState*)0x1)
                {
                    if (PadKeyPresses->Select)
                    {
                        *(int32_t*)(ButtonsState + 0xE8) = 1;
                    }
                    else
                    {
                        *(int32_t*)(ButtonsState + 0xE8) = 0;
                    }

                    if (PadKeyPresses->X)
                    {
                        *(int32_t*)(ButtonsState + 0x13C) = 1;
                    }
                    else
                    {
                        *(int32_t*)(ButtonsState + 0x13C) = 0;
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

        pattern = hook::pattern("68 04 01 00 00 51 E8 ? ? ? ? 83 C4 10 5F 5E C3"); //0x679B53
        injector::WriteMemory<uint8_t>(pattern.get(0).get<uint32_t>(16 + 5), 0xC3, true);
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

        pattern = hook::pattern("8B 0F 8B 54 0E 08 DB 44 90 0C"); //0x67A466
        static auto unk_A987EC = *hook::get_pattern<void**>("81 FA ? ? ? ? 89 74 24 18 ? ? ? ? ? ? 8B 44 24 14", 2);
        struct MenuRemap
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ecx = *(uint32_t*)regs.edi;
                regs.edx = *(uint32_t*)(regs.esi + regs.ecx + 0x08);

                auto dword_A9882C = &unk_A987EC[16];
                auto dword_A98834 = &unk_A987EC[18];
                auto dword_A9883C = &unk_A987EC[20];
                auto dword_A98874 = &unk_A987EC[34];

                *(uint32_t*)(*(uint32_t*)dword_A9882C + 0x20) = 0; // "Enter"; changed B to A
                *(uint32_t*)(*(uint32_t*)dword_A98834 + 0x20) = 1; // "ESC"; changed X to B
                *(uint32_t*)(*(uint32_t*)dword_A9883C + 0x20) = 7; // "SPC"; changed RS to Start
                *(uint32_t*)(*(uint32_t*)dword_A98874 + 0x20) = 3; // "1"; changed A to Y
            }
        }; injector::MakeInline<MenuRemap>(pattern.get_first(0), pattern.get_first(6));
    }

    if (fLeftStickDeadzone)
    {
        // DInput [ 0 32767 | 32768 65535 ] 
        fLeftStickDeadzone /= 200.0f;

        pattern = hook::pattern("89 86 34 02 00 00 8D 45 D4"); //0x892ACD 0x892ADF
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
        static CIniReader RegistryReader;

        auto RegCreateKeyAHook = [](HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult) -> LONG
        {
            return 1;
        };

        auto RegOpenKeyExAHook = [](HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult) -> LONG
        {
            if (RegistryReader.ReadString("MAIN", "DisplayName", "").length() == 0)
            {
                RegistryReader.WriteString("MAIN", "@", "INSERTYOURCDKEYHERE");
                RegistryReader.WriteString("MAIN", "DisplayName", "Need for Speed Carbon");
                RegistryReader.WriteString("MAIN", "Installed From", "C:\\");
                RegistryReader.WriteString("MAIN", "Registration", "SOFTWARE\\Electronic Arts\\Electronic Arts\\Need for Speed Carbon\\ergc");
                RegistryReader.WriteString("MAIN", "CacheSize", "5697825792");
                RegistryReader.WriteString("MAIN", "SwapSize", "73400320");
                RegistryReader.WriteString("MAIN", "Language", "English US");
                RegistryReader.WriteString("MAIN", "Locale", "en_us");
                RegistryReader.WriteString("MAIN", "CD Drive", "C:\\");
                RegistryReader.WriteString("MAIN", "Product GUID", "{259C0ABB-A3B2-4D70-008F-BF7EE491B70B}");
                RegistryReader.WriteString("MAIN", "Region", "NA");
            }
            return ERROR_SUCCESS;
        };

        auto RegCloseKeyHook = [](HKEY hKey) -> LONG
        {
            return ERROR_SUCCESS;
        };

        auto RegSetValueExAHook = [](HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE* lpData, DWORD cbData) -> LONG
        {
            RegistryReader.WriteInteger("MAIN", (char*)lpValueName, *(char*)lpData);
            return ERROR_SUCCESS;
        };

        auto RegQueryValueExAHook = [](HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) -> LONG
        {
            if (!lpValueName) //cd key
            {
                auto str = RegistryReader.ReadString("MAIN", "@", "");
                strncpy((char*)lpData, str.c_str(), *lpcbData);
                return ERROR_SUCCESS;
            }

            if (*lpType == REG_SZ)
            {
                if (strstr(lpValueName, "Install Dir"))
                {
                    GetModuleFileNameA(NULL, (char*)lpData, MAX_PATH);
                    *(strrchr((char*)lpData, '\\') + 1) = '\0';
                    return ERROR_SUCCESS;
                }

                auto str = RegistryReader.ReadString("MAIN", (char*)lpValueName, "");
                strncpy((char*)lpData, str.c_str(), *lpcbData);
            }
            else
            {
                DWORD nValue = RegistryReader.ReadInteger("MAIN", (char*)lpValueName, 0);
                injector::WriteMemory(lpData, nValue, true);
            }

            return ERROR_SUCCESS;
        };

        char szSettingsSavePath[MAX_PATH];
        uintptr_t GetFolderPathCallDest = injector::GetBranchDestination(GetFolderPathpattern.get(0).get<uintptr_t>(14), true).as_int();
        injector::stdcall<HRESULT(HWND, int, HANDLE, DWORD, LPSTR)>::call(GetFolderPathCallDest, NULL, 0x8005, NULL, NULL, szSettingsSavePath);
        strcat(szSettingsSavePath, "\\NFS Carbon");
        strcat(szSettingsSavePath, "\\Settings.ini");
        RegistryReader.SetIniPath(szSettingsSavePath);
        uintptr_t* RegIAT = *hook::pattern("FF 15 ? ? ? ? 8D 54 24 04 52").get(0).get<uintptr_t*>(2); //0x711C0F
        injector::WriteMemory(&RegIAT[0], static_cast<LONG(WINAPI*)(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)>(RegCreateKeyAHook), true);
        injector::WriteMemory(&RegIAT[1], static_cast<LONG(WINAPI*)(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)>(RegOpenKeyExAHook), true);
        injector::WriteMemory(&RegIAT[2], static_cast<LONG(WINAPI*)(HKEY hKey)>(RegCloseKeyHook), true);
        injector::WriteMemory(&RegIAT[3], static_cast<LONG(WINAPI*)(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE* lpData, DWORD cbData)>(RegSetValueExAHook), true);
        injector::WriteMemory(&RegIAT[4], static_cast<LONG(WINAPI*)(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)>(RegQueryValueExAHook), true);
        pattern = hook::pattern("C7 05 ? ? ? ? 00 00 00 00 8B 44 24 04 50 E8"); //0x71D117 
        injector::MakeNOP(pattern.get(0).get<uintptr_t>(0), 10, true); //stops settings reset at startup
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

    }
    return TRUE;
}
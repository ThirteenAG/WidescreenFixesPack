#include "stdafx.h"

static bool bAccessedPostRace;
uint32_t StuffToCompare = 0;
bool __stdcall memory_readable(void *ptr, size_t byteCount)
{
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(ptr, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) == 0)
        return false;

    if (mbi.State != MEM_COMMIT)
        return false;

    if (mbi.Protect == PAGE_NOACCESS || mbi.Protect == PAGE_EXECUTE)
        return false;

    // This checks that the start of memory block is in the same "region" as the
    // end. If it isn't you "simplify" the problem into checking that the rest of
    // the memory is readable.
    size_t blockOffset = (size_t)((char *)ptr - (char *)mbi.AllocationBase);
    size_t blockBytesPostPtr = mbi.RegionSize - blockOffset;

    if (blockBytesPostPtr < byteCount)
        return memory_readable((char *)ptr + blockBytesPostPtr,
                               byteCount - blockBytesPostPtr);

    return true;
}

uint32_t DamageModelFixExit;
void __declspec(naked) DamageModelMemoryCheck()
{
    _asm
    {
        push edi
        mov edi, [esp + 8]
        mov StuffToCompare, edi
    }

    if (memory_readable((void*)StuffToCompare, 8))
        _asm jmp DamageModelFixExit

        _asm
    {
        pop edi
        retn
    }
}

uint32_t loc_5C479A;
uint32_t loc_5C47B0;
void __declspec(naked) ExitPostRaceFixPropagator()
{
    if (bAccessedPostRace)
        _asm jmp loc_5C479A
        else
            _asm jmp loc_5C47B0
        }

void __declspec(naked) ExitPostRaceFixPart2()
{
    _asm
    {
        mov bAccessedPostRace, 0
        pop esi
        retn 4
    }
}

void Init()
{
    FreeConsole();

    //Stop settings reset after crash
    auto pattern = hook::pattern("C7 44 24 ? ? ? ? ? FF 15 ? ? ? ? 8D 54 24 0C 52");
    injector::WriteMemory(pattern.get_first(4), 0, true);

    //Enables HD_GROUP texture on start screen
    uint32_t* dword_7E2C02 = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 8B F8 8B 46 0C 57").count(1).get(0).get<uint32_t>(1);
    injector::WriteMemory(dword_7E2C02, &"ENABLEHD", true);

    CIniReader iniReader("");
    auto bResDetect = iniReader.ReadInteger("MultiFix", "ResDetect", 1) != 0;
    auto bPostRaceFix = iniReader.ReadInteger("MultiFix", "PostRaceFix", 1) != 0;
    auto bFramerateUncap = iniReader.ReadInteger("MultiFix", "FramerateUncap", 1) != 0;
    auto bAntiTrackStreamerCrash = iniReader.ReadInteger("MultiFix", "AntiTrackStreamerCrash", 1) != 0;
    auto bAntiDamageModelCrash = iniReader.ReadInteger("MultiFix", "AntiDamageModelCrash", 1) != 0;

    if (bResDetect)
    {
        struct ScreenRes
        {
            uint32_t ResX;
            uint32_t ResY;
        };

        std::vector<std::string> list;
        GetResolutionsList(list);
        static std::vector<ScreenRes> list2;
        for (auto s : list)
        {
            ScreenRes r;
            sscanf_s(s.c_str(), "%dx%d", &r.ResX, &r.ResY);
            list2.emplace_back(r);
        }

        auto pattern = hook::pattern("83 C0 08 83 F8 58 72 E8 E9 ? ? ? ? 8B 53 3C 8B 43 10"); //0x70B3F0
        injector::WriteMemory(pattern.get_first(-0x6), &list2[0].ResX, true); //0x70B3EA
        injector::WriteMemory(pattern.get_first(-0xE), &list2[0].ResY, true); //0x70B3E2
        injector::WriteMemory<uint8_t>(pattern.get_first(0x5), 0xFFi8, true); //0x70B3F5
    }

    // PostRaceStateManagerFix
    if (bPostRaceFix)
    {
        auto pattern = hook::pattern("C6 44 24 ? ? E8 ? ? ? ? 6A 0A");
        struct PostRaceFix1
        {
            void operator()(injector::reg_pack& regs)
            {
                bAccessedPostRace = true;
                *(uint8_t*)(regs.esp + 0x30) = 4;
            }
        }; injector::MakeInline<PostRaceFix1>(pattern.get_first(0));

        pattern = hook::pattern("C7 46 ? ? ? ? ? E8 ? ? ? ? C6 86 ? ? ? ? ? 5E C2 04 00");
        injector::MakeJMP(pattern.get_first(19), ExitPostRaceFixPart2, true); //0x005C47EA
        pattern = hook::pattern("80 BE ? ? ? ? ? E9");
        injector::MakeJMP(pattern.count_hint(6).get(2).get<void>(7), ExitPostRaceFixPropagator, true); //0x004CEAAD

        DamageModelFixExit = (uint32_t)hook::get_pattern("85 FF 74 2C 8B 44 24 0C 56 50 8B CF", 0); //0x0058DC15
        loc_5C479A = (uint32_t)hook::get_pattern("75 0B C6 86 ? ? ? ? ? 5E C2 04 00", 0);//0x5C479A
        loc_5C47B0 = (uint32_t)hook::get_pattern("8B 46 04 8B 4C 24 08 8B 16 89 46 14 8B 82 ? ? ? ? 89 4E 04 8B CE FF D0 8B 8E", 0); //0x5C47B0
    }

    if (bFramerateUncap) // Framerate unlock
    {
        auto pattern = hook::pattern("83 3D ? ? ? ? ? 75 05 E8 ? ? ? ? 8B 0D ? ? ? ? 85 C9 74 0F 6A 09");
        if (!pattern.empty())
            injector::MakeJMP(pattern.get_first(0), pattern.get_first(14), true); //0x004B42FA, 0x4B4308
    }

    if (bAntiTrackStreamerCrash)
    {
        auto pattern = hook::pattern("80 7C 24 ? ? 74 10 68 ? ? ? ? 55");
        injector::MakeJMP(pattern.get_first(0), pattern.get_first(23), true); //0x00748A09, 0x748A20
        pattern = hook::pattern("75 11 01 6C 24 1C 83 43 28 01");
        injector::MakeNOP(pattern.get_first(0), 2, true); //0x007489FD
    }

    if (bAntiDamageModelCrash)
    {
        auto pattern = hook::pattern("57 8B 7C 24 08 85 FF 74 2C 8B 44 24 0C 56");
        injector::MakeJMP(pattern.get_first(0), DamageModelMemoryCheck, true); //0x58DC10
    }

    bool bFixAspectRatio = iniReader.ReadInteger("MAIN", "FixAspectRatio", 1) != 0;
    static int32_t nScaling = iniReader.ReadInteger("MAIN", "Scaling", 1);
    bool bFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1) != 0;
    bool bConsoleHUDSize = iniReader.ReadInteger("MAIN", "ConsoleHUDSize", 0) != 0;
    bool bGammaFix = iniReader.ReadInteger("MISC", "GammaFix", 1) != 0;
    bool bSkipIntro = iniReader.ReadInteger("MISC", "SkipIntro", 0) != 0;
    static int32_t nWindowedMode = iniReader.ReadInteger("MISC", "WindowedMode", 0);
    static int32_t nImproveGamepadSupport = iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0);
    static float fLeftStickDeadzone = iniReader.ReadFloat("MISC", "LeftStickDeadzone", 10.0f);
    bool bWriteSettingsToFile = iniReader.ReadInteger("MISC", "WriteSettingsToFile", 0) != 0;
    bool bDisableMotionBlur = iniReader.ReadInteger("MISC", "DisableMotionBlur", 0) != 0;
    bool bBrakeLightFix = iniReader.ReadInteger("MISC", "BrakeLightFix", 1) != 0;
    static int32_t nShadowRes = iniReader.ReadInteger("MISC", "ShadowRes", 2048);
    static auto szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "0");
    static int SimRate = iniReader.ReadInteger("MISC", "SimRate", -1);
    if (szCustomUserFilesDirectoryInGameDir.empty() || szCustomUserFilesDirectoryInGameDir == "0")
        szCustomUserFilesDirectoryInGameDir.clear();

    if (bFixAspectRatio)
    {
        // Real-Time Aspect Ratio Calculation
        static uint32_t* dword_BBADB4 = *hook::pattern("C7 05 ? ? ? ? 03 00 00 00 89 3D  ? ? ? ? 89 3D").get(0).get<uint32_t*>(35); // ResX
        static uint32_t* dword_BBADB8 = *hook::pattern("C7 05 ? ? ? ? 03 00 00 00 89 3D  ? ? ? ? 89 3D").get(0).get<uint32_t*>(41); // ResY
        static float fScreenAspectRatio, temp_xmm0;

        auto pattern = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 11 04 24 D9 04 24 83 C4 0C");
        struct AspectRatioHook
        {
            void operator()(injector::reg_pack& regs)
            {
                _asm movss dword ptr ds : [temp_xmm0], xmm0 // moves xmm0 to temporary location so contents aren't lost
                auto ResX = *(float*)(dword_BBADB4);
                auto ResY = *(float*)(dword_BBADB8);
                auto esp00 = regs.esp;
                fScreenAspectRatio = (ResX / ResY) * 0.5625f;

                _asm
                {
                    movss xmm0, dword ptr ds : [temp_xmm0] // restores xmm0
                    movss dword ptr ds : [esp00], xmm0
                    fld dword ptr ds : [esp00]
                }
            }
        }; injector::MakeInline<AspectRatioHook>(pattern.get_first(8), pattern.get_first(16)); // 4BCB35


        // Force 16:9 FOV, HUD & FMV Scaling
        {
            static float fRatio = 1.777777777f;
            auto pattern = hook::pattern("A1 ? ? ? ? 83 EC 0C 80 B8 ? ? ? ? 00 ? ? B9 ? ? ? ? E8");
            uint32_t* dword_4BCAEF = pattern.count(1).get(0).get<uint32_t>(15);
            injector::WriteMemory<uint8_t>(dword_4BCAEF, 0xEB, true); // jmp
            uint32_t* dword_4BCB39 = pattern.count(1).get(0).get<uint32_t>(89);
            injector::WriteMemory(dword_4BCB39, &fRatio, true);
            uint32_t* dword_6FF9E3 = hook::pattern("D9 C9 DF F1 DD D8 ? ? 0F B7 C3").count(1).get(0).get<uint32_t>(6);
            injector::MakeNOP(dword_6FF9E3, 2, true); // 2 nops
            uint32_t* dword_4B51EB = hook::pattern("74 ? 8B 16 8B 42 ? FF D0 5E 5B").count(1).get(0).get<uint32_t>(0);
            injector::WriteMemory<uint8_t>(dword_4B51EB, 0xEB, true); // jmp
        }

        // FOV Width & Height
        {
            static float dword_9FA868, temp_xmm0;

            auto pattern = hook::pattern("0F 57 C0 F3 0F 10 ? ? ? ? ? F3 0F 10 ? ? ? F3 0F 10 ? ? ? ? ? 8D 7E");
            struct FOVHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    _asm movss dword ptr ds : [temp_xmm0], xmm0 // moves xmm0 to temporary location so contents aren't lost
                    int ebp08 = *(int*)(regs.ebp + 0x08);
                    float esp48 = *(float*)(regs.esp + 0x48);

                    if (ebp08 == 1) // jne
                        *(float*)(regs.esp + 0x48) = (esp48 / fScreenAspectRatio);

                    _asm
                    {
                        movss xmm0, dword ptr ds : [temp_xmm0] // restores xmm0
                        xorps xmm0, xmm0
                        movss xmm1, dword ptr ds : [dword_9FA868]
                    }
                }
            }; injector::MakeInline<FOVHook>(pattern.get_first(0), pattern.get_first(11)); // 6FFB1A

            if (nScaling)
            {
                static double XB360_Hor;
                static double XB360_Ver;

                if (nScaling == 1)
                {
                    XB360_Hor = 1.26;
                    XB360_Ver = 1.50;
                }
                if (nScaling >= 2)
                {
                    XB360_Hor = 1.30;
                    XB360_Ver = 1.48;
                }

                auto pattern = hook::pattern("DC 0D ? ? ? ? D9 5C 24 ? D9 44 24 ? DC 0D ? ? ? ? ? ? D9 44 24 ? DC 0D");
                uint32_t* dword_6FFA12 = pattern.count(2).get(0).get<uint32_t>(2);
                injector::WriteMemory(dword_6FFA12, &XB360_Hor, true);
                uint32_t* dword_6FFA20 = pattern.count(2).get(0).get<uint32_t>(16);
                injector::WriteMemory(dword_6FFA20, &XB360_Ver, true);
            }
        }

        // HUD Width
        {
            static double dbl_HUDWidth = (4.0 / 3.0f);

            auto pattern = hook::pattern("DC 3D ? ? ? ? D9 5C 24 0C F3 0F 10");
            struct HUDWidthHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    _asm
                    {
                        fdivr qword ptr ds : [dbl_HUDWidth]
                        fdiv dword ptr ds : [fScreenAspectRatio]
                    }
                }
            }; injector::MakeInline<HUDWidthHook>(pattern.get_first(0), pattern.get_first(6)); // 4B44F6
        }

        // FMV Width & Height
        {
            static float Width1 = -0.75f;
            static float Width2 = 0.75f;
            static float Height1 = 0.75f;
            static float Height2 = -0.75f;
            static float FMVWidthLeft, FMVWidthRight;

            if (bFMVWidescreenMode)
            {
                Width1 = -1.00f;
                Width2 = 1.00f;
                Height1 = 1.00f;
                Height2 = -1.00f;
            }

            // Real-Time FMV Aspect Ratio Calculation
            auto pattern = hook::pattern("80 BC 24 ? ? ? ? 00 DE F9 D9");
            struct FMVHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    int espB0 = *(int*)(regs.esp + 0xB0);

                    _asm
                    {
                        fld dword ptr ds : [Width1] // Loads Width1 value
                        fdiv dword ptr ds : [fScreenAspectRatio] // Divides by ScreenAspectRatio
                        fstp dword ptr ds : [FMVWidthLeft] // Stores final result
                        fld dword ptr ds : [Width2] // Loads Width2 value
                        fdiv dword ptr ds : [fScreenAspectRatio] // Divides by ScreenAspectRatio
                        fstp dword ptr ds : [FMVWidthRight] // Stores final result
                        cmp byte ptr ds : [espB0], 0x00
                    }
                }
            }; injector::MakeInline<FMVHook>(pattern.get_first(0), pattern.get_first(8)); // 70235A

            {
                auto pattern = hook::pattern("F3 0F 10 ? ? ? ? ? F3 0F 10 ? ? ? ? ? 0F 57 C0 F3 0F 10");
                uint32_t* dword_702224 = pattern.count(1).get(0).get<uint32_t>(4);
                injector::WriteMemory(dword_702224, &Height1, true);
                uint32_t* dword_70222C = pattern.count(1).get(0).get<uint32_t>(12);
                injector::WriteMemory(dword_70222C, &FMVWidthLeft, true);
                uint32_t* dword_702237 = pattern.count(1).get(0).get<uint32_t>(23);
                injector::WriteMemory(dword_702237, &FMVWidthRight, true);
                uint32_t* dword_702261 = pattern.count(1).get(0).get<uint32_t>(65);
                injector::WriteMemory(dword_702261, &Height2, true);
            }
        }

        // Crowd Culling Fix
        {
            static float CrowdCullDist;

            auto pattern = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 11 44 24 7C ? ? ? ? ? ? 2B C3 83 E8 01");
            struct CrowdCullHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    CrowdCullDist = (0.6f / fScreenAspectRatio);
                    *(float*)(regs.esp + 0x7C) = CrowdCullDist;
                }
            }; injector::MakeInline<CrowdCullHook>(pattern.get_first(0), pattern.get_first(8)); // 76CECB
        }
    }

    //if (bHUDWidescreenMode)
    {
        // Real-Time Aspect Ratio Calculation
        static uint32_t* dword_BBADB4 = *hook::pattern("C7 05 ? ? ? ? 03 00 00 00 89 3D  ? ? ? ? 89 3D").get(0).get<uint32_t*>(35); // ResX
        static uint32_t* dword_BBADB8 = *hook::pattern("C7 05 ? ? ? ? 03 00 00 00 89 3D  ? ? ? ? 89 3D").get(0).get<uint32_t*>(41); // ResY
        static float fScreenAspectRatio;

        auto pattern = hook::pattern("0F B6 C0 89 01 B0 01");
        struct HUDWidescreenModeHook
        {
            void operator()(injector::reg_pack& regs)
            {
                auto ResX = *(float*)(dword_BBADB4);
                auto ResY = *(float*)(dword_BBADB8);
                fScreenAspectRatio = (ResX / ResY);

                if (fScreenAspectRatio >= 1.7777777f)
                {
                    regs.eax = (int)1;
                    *(int*)(regs.ecx) = regs.eax;
                }
                else
                {
                    regs.eax = (int)0;
                    *(int*)(regs.ecx) = regs.eax;
                }
            }
        }; injector::MakeInline<HUDWidescreenModeHook>(pattern.count(7).get(0).get<uint32_t>(0)); // 44C332
    }

    if (bConsoleHUDSize)
    {
        static constexpr float HUDSize = 1.0875f;
        uint32_t* dword_4B4455 = hook::pattern("F3 0F 10 ? ? ? ? ? 8B 3D ? ? ? ? C6 05 ? ? ? ? 00").count(1).get(0).get<uint32_t>(4);
        injector::WriteMemory(dword_4B4455, &HUDSize, true);
    }

    if (bGammaFix)
    {
        static uint32_t* dword_AC6F0C = *hook::pattern("A1 ? ? ? ? 8B 4C 24 04 8B D0 B8 67 66 66 66").get(0).get<uint32_t*>(1); // Gamma Integer
        static uint32_t* dword_AA9630 = *hook::pattern("F3 0F 11 44 24 04 D8 64 24 04 C6 ? 01 D9 5C").get(0).get<uint32_t*>(29); // Gamma Float
        injector::WriteMemory(dword_AA9630, 1.0f, true); // sets default brightness to 50%

        static float NewBrightness;

        auto pattern = hook::pattern("55 8B EC 83 E4 F8 83 EC 10 ? ? ? ? ? ? ? ? 56 57 33 FF");
        struct GammaHook
        {
            void operator()(injector::reg_pack& regs)
            {
                auto GammaInteger = *(int*)(dword_AC6F0C);
                auto GammaFloat = *(float*)(dword_AA9630);
                NewBrightness = 100.0f / (GammaInteger * 2.0f);
                GammaFloat = NewBrightness;

                _asm
                {
                    movss xmm0, [GammaFloat]
                }
            }
        }; injector::MakeInline<GammaHook>(pattern.count(1).get(0).get<uint32_t>(9), pattern.count(1).get(0).get<uint32_t>(17)); // 4B3E89
    }

    if (bSkipIntro)
    {
        // EA Bumper
        uint32_t* dword_6FC24A = hook::pattern("68 ? ? ? ? 6A ? FF D2 D9 EE").count(1).get(0).get<uint32_t>(1);
        injector::WriteMemory(dword_6FC24A, &"SkipThis", true);

        // Attract
        uint32_t* dword_6FC264 = hook::pattern("68 ? ? ? ? 6A ? 8B CE FF D2").count(1).get(0).get<uint32_t>(1);
        injector::WriteMemory(dword_6FC264, &"SkipThis", true);
    }

    if (nWindowedMode)
    {
        pattern = hook::pattern("83 C4 18 8B 0B 55 52 8B 54 24 24 55 55 2B F2");//0x70E3C8 anchor
        uint32_t* dword_70E3E9 = pattern.count(1).get(0).get<uint32_t>(0x21);
        uint32_t* dword_70E39B = pattern.count(1).get(0).get<uint32_t>(-0x2D);
        uint32_t* WindowedMode_AC6EFC = *hook::pattern("8B 7C 24 28 2B 7C 24 20 8B 44 24 2C 2B 44 24 24 8B 15 ? ? ? ? F7 DA").count(1).get(0).get<uint32_t*>(0x12); //0x70E5D1 anchor, 0x70E5E3 dereference

        // hook the offending functions
        injector::MakeNOP(dword_70E3E9, 6, true);
        injector::MakeCALL(dword_70E3E9, WindowedModeWrapper::CreateWindowExA_Hook, true);
        injector::MakeNOP(dword_70E39B, 6, true);
        injector::MakeCALL(dword_70E39B, WindowedModeWrapper::AdjustWindowRect_Hook, true);
        // enable windowed mode variable
        *WindowedMode_AC6EFC = 1;

        if (nWindowedMode > 1)
            WindowedModeWrapper::bBorderlessWindowed = false;
        if (nWindowedMode > 2) // TODO: implement dynamic resizing (like in MW)
            WindowedModeWrapper::bEnableWindowResize = true;

        // actually what enforces the windowed mode
        auto pattern = hook::pattern("89 5D 3C 89 5D 18 89 5D 44"); //0x708379
        struct WindowedMode
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uint32_t*)(regs.ebp + 0x3C) = 1;
                *(uint32_t*)(regs.ebp + 0x18) = regs.ebx;
            }
        }; injector::MakeInline<WindowedMode>(pattern.get_first(0), pattern.get_first(6));

        pattern = hook::pattern("A3 ? ? ? ? 89 35 ? ? ? ? 88 1D ? ? ? ? B9 ? ? ? ? 74 0B 6A 15");
        static auto Width = *pattern.get_first<int32_t*>(1);
        static auto Height = *pattern.get_first<int32_t*>(7);
        struct ResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *Width = regs.eax;
                *Height = regs.esi;
        
                tagRECT rc;
                auto[DesktopResW, DesktopResH] = GetDesktopRes();
                rc.left = (LONG)(((float)DesktopResW / 2.0f) - ((float)*Width / 2.0f));
                rc.top = (LONG)(((float)DesktopResH / 2.0f) - ((float)*Height / 2.0f));
                rc.right = *Width;
                rc.bottom = *Height;
                SetWindowPos(WindowedModeWrapper::GameHWND, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
            }
        }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(11));
    }

    if (nImproveGamepadSupport)
    {
        auto pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 51 A1 ? ? ? ? 33 C4 50 8D 44 24 08 64 A3 ? ? ? ? A1 ? ? ? ? 50");
        static auto CreateResourceFile = (void*(*)(const char* ResourceFileName, int32_t ResourceFileType, int, int, int)) pattern.get_first(0); //0x006D6DE0
        pattern = hook::pattern("8B 44 24 04 56 8B F1 8B 4C 24 0C 89 46 38 89 4E 3C");
        static auto ResourceFileBeginLoading = (void(__thiscall *)(void* rsc, int a1, int a2)) pattern.get_first(0); //0x006D9430
        static auto LoadResourceFile = [](const char* ResourceFileName, int32_t ResourceFileType, int32_t nUnk1 = 0, int32_t nUnk2 = 0, int32_t nUnk3 = 0, int32_t nUnk4 = 0, int32_t nUnk5 = 0)
        {
            auto r = CreateResourceFile(ResourceFileName, ResourceFileType, nUnk1, nUnk2, nUnk3);
            ResourceFileBeginLoading(r, nUnk4, nUnk5);
        };

        if (nImproveGamepadSupport < 3)
        {
            static auto TPKPath = GetThisModulePath<std::string>().substr(GetExeModulePath<std::string>().length());

            if (nImproveGamepadSupport == 1)
                TPKPath += "buttons-xbox.tpk";
            else if (nImproveGamepadSupport == 2)
                TPKPath += "buttons-playstation.tpk";

            static injector::hook_back<void(__cdecl*)()> hb_6D98B0;
            auto LoadTPK = []()
            {
                LoadResourceFile(TPKPath.c_str(), 1);
                return hb_6D98B0.fun();
            };

            pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 8B 35 ? ? ? ? 6A 04 FF D6 83 C4 04 85 C0"); //0x6DABDE
            hb_6D98B0.fun = injector::MakeCALL(pattern.get_first(0), static_cast<void(__cdecl*)()>(LoadTPK), true).get();
        }

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

        pattern = hook::pattern("8B 0F 8B 54 0E 08 DB 44 90 0C"); //0x6A5E4C
        static auto unk_ABA3E4 = *hook::get_pattern<void**>("81 FE ? ? ? ? 0F 8C ? ? ? ? 8B 44 24 14 83 B8", 2);
        struct MenuRemap
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ecx = *(uint32_t*)regs.edi;
                regs.edx = *(uint32_t*)(regs.esi + regs.ecx + 0x08);

                auto dword_ABA4C4 = &unk_ABA3E4[26];
                auto dword_ABA454 = &unk_ABA3E4[28];
                *(uint32_t*)(*(uint32_t*)dword_ABA4C4 + 0x20) = 4; // changes RB to LB
                *(uint32_t*)(*(uint32_t*)dword_ABA454 + 0x20) = 5; // changes LB to RB
            }
        }; injector::MakeInline<MenuRemap>(pattern.get_first(0), pattern.get_first(6));

        static injector::hook_back<int32_t(__cdecl*)(char*)> hb_4366E0;
        auto padFix = [](char*) -> int32_t
        {
            return 0x2A5E19E0;
        };
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 3D ? ? ? ? 0F 87 ? ? ? ? 0F 84 ? ? ? ? 3D");
        hb_4366E0.fun = injector::MakeCALL(pattern.get_first(0), static_cast<int32_t(__cdecl*)(char*)>(padFix), true).get();

        // Enables controls for FE photo mode
        //pattern = hook::pattern("D9 EE 6A 00 51 D9 1C 24 8B 8E D0 00 00 00 E8 ? ? ? ? D9 EE 6A 00 "); //0x59890A
        //injector::WriteMemory(pattern.get_first(0), 0xC3595E90, true); //pop esi  pop ecx  ret

        // Start menu text
        uint32_t* dword_7E2C9A = hook::pattern("68 ? ? ? ? 50 51 E8 ? ? ? ? 8D 54 24 2C").count(1).get(0).get<uint32_t>(1);
        if (nImproveGamepadSupport != 2)
            injector::WriteMemory(dword_7E2C9A, 0x25C22853, true); //"Press START to begin" (Xbox)
        else
            injector::WriteMemory(dword_7E2C9A, 0x703A92CC, true); //"Press START button" (PlayStation)
    }

    if (fLeftStickDeadzone)
    {
        // DInput [ 0 32767 | 32768 65535 ]
        fLeftStickDeadzone /= 200.0f;

        auto pattern = hook::pattern("89 86 34 02 00 00 8D 45 D4"); //0x892ACD 0x892ADF
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

    if (SimRate)
    {
        if (SimRate < 0)
        {
            if (SimRate == -1)
                SimRate = GetDesktopRefreshRate();
            else if (SimRate == -2)
                SimRate = GetDesktopRefreshRate() * 2;
            else
                SimRate = 60;
        }

        // limit rate to avoid issues...
        if (SimRate > 360)
            SimRate = 360;
        if (SimRate < 60)
            SimRate = 60;

        static float FrameTime = 1.0f / SimRate;
        static double dFrameTime = 1.0 / SimRate;
        //static float fSimRate = (float)SimRate;
        static double dSimRate = (double)SimRate;

        // Frame times
        // PrepareRealTimestep() NTSC video mode framerate, .text
        uint32_t* dword_6D8B8F = hook::pattern("D9 05 ? ? ? ? B9 ? ? ? ? D8 44 24 14 D9 5C 24 14").count(1).get(0).get<uint32_t>(53); //0x6D8B5A anchor, 0x6D8B8F pointer write
        // MainLoop double frametime (FPS lock) .rdata
        double* dbl_9FABF8 = *hook::pattern("D8 44 24 08 D9 5C 24 20 EB 47 83 3D").count(1).get(0).get<double*>(0x1B); //0x006DAEE8 anchor, 0x6DAF03 location dereference
        // MainLoop float frametime (FPS lock) .rdata
        float* flt_98C218 = *hook::pattern("D8 44 24 08 D9 5C 24 20 EB 47 83 3D").count(1).get(0).get<float*>(0x30); //0x006DAEE8 anchor, 0x6DAF18 location dereference
        // FullSpeedMode frametime (10x speed) .rdata
        float* flt_98D868 = *hook::pattern("D8 44 24 08 D9 5C 24 20 EB 47 83 3D").count(1).get(0).get<float*>(0x47); //0x006DAEE8 anchor, 0x6DAF2F location dereference
        // Unknown frametime 1 .rdata
        float* flt_996F48 = *hook::pattern("D8 66 7C D9 5C 24 14 D9 44 24 14 D9 05 ? ? ? ? DF F1").count(1).get(0).get<float*>(0xD); //0x74F3F4 anchor, 0x74F401 location dereference
        // Unknown frametime 2 .rdata
        double* dbl_996F40 = *hook::pattern("D8 66 7C D9 5C 24 14 D9 44 24 14 D9 05 ? ? ? ? DF F1").count(1).get(0).get<double*>(0x33); //0x74F3F4 anchor, 0x74F427 location dereference
        // Unknown frametime 3 .rdata
        float* flt_996F3C = *hook::pattern("D8 66 7C D9 5C 24 14 D9 44 24 14 D9 05 ? ? ? ? DF F1").count(1).get(0).get<float*>(0x41); //0x74F3F4 anchor, 0x0074F435 location dereference
        // NIS related frametime .rdata
        float* flt_9FB408 = *hook::pattern("D9 EE 51 D9 1C 24 E8 ? ? ? ? D9 05 ? ? ? ? D9 1C 24").count(1).get(0).get<float*>(0xD); //0x4D9B7F anchor, 0x004D9B8C location dereference
        // Sim::QueueEvents frametime .rdata (this affects gameplay smoothness noticeably)
        float* flt_9EE934 = *hook::pattern("D9 46 1C 8B 10 83 EC 08 D9 5C 24 04 8B C8 D9 05 ? ? ? ?").count(1).get(0).get<float*>(16); //0x004CE576 anchor, 0x004CE586 location dereference

        injector::WriteMemory(dword_6D8B8F, &dSimRate, true);
        injector::WriteMemory(dbl_9FABF8, dFrameTime, true);
        injector::WriteMemory(flt_98C218, FrameTime, true);
        injector::WriteMemory(flt_98D868, FrameTime * 10.0f, true);
        injector::WriteMemory(flt_996F48, FrameTime, true);
        injector::WriteMemory(dbl_996F40, -dFrameTime, true);
        injector::WriteMemory(flt_996F3C, -FrameTime, true);
        injector::WriteMemory(flt_9FB408, FrameTime, true);
        injector::WriteMemory(flt_9EE934, FrameTime, true);

        // Frame rates
        // TODO: if any issues arise, figure out where 60.0 values are used and update the constants...
        
        // scale boost grip values with frametime to make staging (drag burnout) normal difficulty and not broken and too easy (.rdata)
        // using 60fps as the basis for scaling
        constexpr double ScaleFPS = 60.0;
        double* dbl_9FB3B8 = *hook::pattern("D9 83 80 03 00 00 DC 05 ? ? ? ? D9 5C 24 14").count(1).get(0).get<double*>(8); //0x483707 anchor, 0x48370F location dereference, value at 0x009FB3B8
        double* dbl_9FB3C0 = *hook::pattern("D9 83 80 03 00 00 DC 25 ? ? ? ? D9 5C 24 14").count(1).get(0).get<double*>(8); //0x4836DD anchor, 0x4836E3 location dereference, value at 0x009FB3C0

        static double BoostGripAdder = ((*dbl_9FB3B8) / (1 / ScaleFPS)) * dFrameTime;
        injector::WriteMemory(dbl_9FB3B8, BoostGripAdder, true);

        static double BoostGripSubtractor = ((*dbl_9FB3C0) / (1 / ScaleFPS)) * dFrameTime;
        injector::WriteMemory(dbl_9FB3C0, BoostGripSubtractor, true);
    }

    if (bWriteSettingsToFile)
    {
      struct LazyHook
      {
        static void hook()
        {
          auto GetFolderPathpattern = hook::pattern("50 6A 00 6A 00 68 ? 80 00 00 6A 00");
          while (!injector::GetBranchDestination(GetFolderPathpattern.get(0).get<uintptr_t>(14), true).as_int())
            std::this_thread::sleep_for(std::chrono::seconds(1));

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

          auto[DesktopResW, DesktopResH] = GetDesktopRes();
          char szSettingsSavePath[MAX_PATH];
          uintptr_t GetFolderPathCallDest = injector::GetBranchDestination(GetFolderPathpattern.get(0).get<uintptr_t>(14), true).as_int();
          if (GetFolderPathCallDest)
          {
            injector::stdcall<HRESULT(HWND, int, HANDLE, DWORD, LPSTR)>::call(GetFolderPathCallDest, NULL, 0x8005, NULL, NULL, szSettingsSavePath);
            strcat(szSettingsSavePath, "\\NFS ProStreet");
            strcat(szSettingsSavePath, "\\Settings.ini");

            RegistryWrapper("Need for Speed", szSettingsSavePath);
            auto RegIAT = *hook::pattern("FF 15 ? ? ? ? 8D 54 24 40 52 68 3F 00 0F 00").get(0).get<uintptr_t*>(2);
            injector::WriteMemory(&RegIAT[0], RegistryWrapper::RegCreateKeyA, true);
            injector::WriteMemory(&RegIAT[1], RegistryWrapper::RegOpenKeyExA, true);
            injector::WriteMemory(&RegIAT[2], RegistryWrapper::RegCloseKey, true);
            injector::WriteMemory(&RegIAT[3], RegistryWrapper::RegSetValueExA, true);
            injector::WriteMemory(&RegIAT[4], RegistryWrapper::RegQueryValueExA, true);
            RegistryWrapper::AddPathWriter("Install Dir", "InstallDir", "Path");
            RegistryWrapper::AddDefault("@", "INSERTYOURCDKEYHERE");
            RegistryWrapper::AddDefault("CD Drive", "D:\\");
            RegistryWrapper::AddDefault("CacheSize", "5697825792");
            RegistryWrapper::AddDefault("SwapSize", "73400320");
            RegistryWrapper::AddDefault("Language", "Engish (US)");
            RegistryWrapper::AddDefault("StreamingInstall", "0");
            RegistryWrapper::AddDefault("FirstTime", "0");
            RegistryWrapper::AddDefault("g_CarEffects", "2");
            RegistryWrapper::AddDefault("g_WorldFXLevel", "3");
            RegistryWrapper::AddDefault("g_RoadReflectionEnable", "1");
            RegistryWrapper::AddDefault("g_WorldLodLevel", "2");
            RegistryWrapper::AddDefault("g_CarLodLevel", "0");
            RegistryWrapper::AddDefault("g_FSAALevel", "3");
            RegistryWrapper::AddDefault("g_RainEnable", "1");
            RegistryWrapper::AddDefault("g_TextureFiltering", "2");
            RegistryWrapper::AddDefault("g_SmokeEnable", "1");
            RegistryWrapper::AddDefault("g_CarDamageDetail", "2");
            RegistryWrapper::AddDefault("g_PerformanceLevel", "0");
            RegistryWrapper::AddDefault("g_VSyncOn", "0");
            RegistryWrapper::AddDefault("g_ShadowEnable", "3");
            RegistryWrapper::AddDefault("g_ShaderDetailLevel", "1");
            RegistryWrapper::AddDefault("g_AudioDetail", "0");
            RegistryWrapper::AddDefault("g_Brightness", "68");
            RegistryWrapper::AddDefault("g_AudioMode", "1");
            RegistryWrapper::AddDefault("g_Width", std::to_string(DesktopResW));
            RegistryWrapper::AddDefault("g_Height", std::to_string(DesktopResH));
            RegistryWrapper::AddDefault("g_Refresh", "60");
          }
        }
      };

      std::thread(LazyHook::hook).detach();
    }

    if (bDisableMotionBlur)
    {
        pattern = hook::pattern("74 73 F3 0F 10 84 24 ? ? ? ? F3 0F 11 44 24");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); //4B153E jmp 4B15B3
    }

    if (bBrakeLightFix)
    {
        static constexpr double d0 = 0.0f;
        pattern = hook::pattern("DD 05 ? ? ? ? D9 C9 DF F1 DD D8 77 15 8B 86"); //717C9A
        injector::WriteMemory(pattern.get_first(2), &d0, true);
    }

    if (nShadowRes)
    {
        pattern = hook::pattern("B8 ? ? ? ? EB 0C B8 ? ? ? ? EB 05 B8 ? ? ? ? 57 6A 00");
        injector::WriteMemory(pattern.get_first(1), nShadowRes, true);
        injector::WriteMemory(pattern.get_first(8), nShadowRes, true);
        injector::WriteMemory(pattern.get_first(15), nShadowRes, true);
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("C7 46 ? ? ? ? ? E8 ? ? ? ? C6 86 ? ? ? ? ? 5E C2 04 00"));
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

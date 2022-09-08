#include "stdafx.h"

uintptr_t RangeStart, RangeEnd;
hook::pattern GetPattern(std::string_view pattern)
{
    auto p = hook::pattern(pattern);
    if (p.empty())
        return hook::pattern(RangeStart, RangeEnd, pattern);
    return p;
}

uint32_t ShadowLevel = 3;
static uint32_t ShadowsRes = 1024;
static uint32_t ShadowsResX = 3072;
static float fShadowsRes = 1024.0;
static float fShadowsResX = 3072.0;
static uint32_t iniShadowsRes = 1024;

void UpdateShadowsRes()
{
    ShadowsRes = iniShadowsRes;
    fShadowsRes = ShadowsRes;
    if (fShadowsRes > (16384.0f / (float)ShadowLevel))
        fShadowsRes = (16384.0f / (float)ShadowLevel);
    ShadowsRes = (uint32_t)fShadowsRes;
    fShadowsResX = (float)ShadowLevel * fShadowsRes;
    ShadowsResX = (uint32_t)fShadowsResX;
}

// entrypoint 0x00793E24
uint32_t ShadowTexCaveExit = 0x00793E2D;
void __declspec(naked) ShadowTexCave()
{
    _asm mov ShadowLevel, eax

    // Update ShadowsRes dynamically during texture creation! This is to ensure that when the user increases the shadow quality it won't fail due to too high resolution!
    UpdateShadowsRes();

    _asm
    {
        mov eax, ShadowsResX
        mov ecx, iniShadowsRes
        push ecx
        push eax
        jmp ShadowTexCaveExit
    }
}

void InitRes()
{
    struct ScreenRes
    {
        uint32_t ResX;
        uint32_t ResY;
        uint32_t LangHash;
    };

    static std::vector<std::string> list;
    static std::vector<ScreenRes> list2;
    static std::vector<uint32_t> list3;
    GetResolutionsList(list);
    for (auto& s : list)
    {
        ScreenRes r;
        sscanf_s(s.c_str(), "%dx%d", &r.ResX, &r.ResY);
        list2.emplace_back(r);
    }
    list3.resize(list2.size() + 1);
    size_t i = 0;
    for (auto& s : list2)
    {
        list3[i] = ((uint32_t)(&list2[i]));
        ++i;
    }
    list3[i] = 0;

    auto pattern = GetPattern("E8 ? ? ? ? 33 FF 39 ? ? ? ? ? 76"); //0x587291
    injector::MakeNOP(pattern.get_first(0), 5, true);
    injector::WriteMemory(*pattern.get_first<void*>(9), list3.size() - 1, true);//0x12AA194

    pattern = GetPattern("68 D7 13 00 00");
    injector::WriteMemory(pattern.get_first(24), &list3[0], true); //0x57A5B5
    pattern = GetPattern("8B 14 8D ? ? ? ? 8B 42 04 50");
    injector::WriteMemory(pattern.get_first(3), &list3[0], true); //0x57A5D1
    pattern = GetPattern("8B 04 8D ? ? ? ? 8B 50");
    injector::WriteMemory(pattern.get_first(3), &list3[0], true); //0x57A5EE
    pattern = GetPattern("8B 04 BD ? ? ? ? 8B 48");
    injector::WriteMemory(pattern.get_first(3), &list3[0], true); //0x5872A3
    pattern = GetPattern("8B 14 8D ? ? ? ? 39 3A");
    injector::WriteMemory(pattern.get_first(3), &list3[0], true); //0x5872F3

    pattern = GetPattern("8B 0C 85 ? ? ? ? 89 4F 6C 8B 46 10"); //0x74006F
    injector::WriteMemory(pattern.get_first(3), &list2[0], true);
    injector::WriteMemory(pattern.get_first(19), &list2[0].ResY, true);//0x74007F

    pattern = GetPattern("E8 ? ? ? ? 89 44 BC ? 83 C7 01 83 C4 04"); //0x5872AB
    struct GetPackedStringHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(char**)&regs.eax = list[regs.edi].data();
        }
    }; injector::MakeInline<GetPackedStringHook>(pattern.get_first(0));
}

void Init1()
{
    //Stop settings reset after crash
    auto pattern = GetPattern("C7 44 24 ? ? ? ? ? FF 15 ? ? ? ? 8D 54 24 0C 52");
    injector::WriteMemory(pattern.get_first(4), 0, true);

    CIniReader iniReader("");
    static float fLeftStickDeadzone = iniReader.ReadFloat("MISC", "LeftStickDeadzone", 10.0f);
    static float fBloomIntensity = iniReader.ReadFloat("GRAPHICS", "BloomIntensity", 1.0f);

    pattern = GetPattern("D9 05 ? ? ? ? 50 83 EC 08 D9 5C 24 04"); //0x766FC5
    injector::WriteMemory(pattern.get_first(2), &fBloomIntensity, true);

    if (fLeftStickDeadzone)
    {
        // DInput [ 0 32767 | 32768 65535 ]
        fLeftStickDeadzone /= 200.0f;

        auto pattern = GetPattern("89 86 34 02 00 00 8D 45 D4");
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
}

void Init2()
{
    CIniReader iniReader("");
    static bool bResDetect = iniReader.ReadInteger("MultiFix", "ResDetect", 1) != 0;
    static bool bSkipIntro = iniReader.ReadInteger("MISC", "SkipIntro", 1) != 0;
    static int32_t nImproveGamepadSupport = iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0);

    auto pattern = GetPattern("C6 ? 98 00 00 00 01 A1 ? ? ? ? 85 C0 74 05 E8"); //0x575604
    static auto dword_12BB200 = *pattern.get_first<uint32_t*>(25);
    auto rpattern = hook::range_pattern((uintptr_t)pattern.get_first(-100), (uintptr_t)pattern.get_first(0), "A1 ? ? ? ? 33 C4 89 84 24"); //0x5755C4
    static auto dword_1272378 = *rpattern.get_first<uint32_t*>(1);

    struct MoviesHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *dword_1272378;
            *dword_12BB200 = 0;
            static bool bOnce = false;
            if (!bOnce)
            {
                if (bResDetect)
                    InitRes();
                if (bSkipIntro)
                    *dword_12BB200 = 1;
            }
            bOnce = true;
        }
    }; injector::MakeInline<MoviesHook>(rpattern.get_first(0));

    if (nImproveGamepadSupport)
    {
        auto pattern = GetPattern("E8 ? ? ? ? 8B 4E 04 83 C4 20 89 46 08 8B 15 ? ? ? ? 51 52 68 0B 20 00 00");
        static auto CreateResourceFile = (void*(*)(const char* ResourceFileName, int32_t ResourceFileType, int, int, int)) injector::GetBranchDestination(pattern.get_first(0)).as_int();
        pattern = GetPattern("E8 ? ? ? ? 5B B0 01 5F 88 46 14 C6 46 15 00 5E C3");
        static auto ResourceFileBeginLoading = (void(__thiscall*)(void* rsc, int a1, int a2)) injector::GetBranchDestination(pattern.get_first(0)).as_int();
        pattern = GetPattern("E8 ? ? ? ? 8A 54 24 44 66 8B 4C 24 40 88 56 0E 89 46 08 66 89 4E 0C C7 46 18 00 00 00 00");
        static auto SharedStringPoolAllocate = (void*(__cdecl*)(const char* ResourceFileName)) injector::GetBranchDestination(pattern.get_first(0)).as_int();
        pattern = GetPattern("8B 3D ? ? ? ? 6A 00 6A 00 6A 00 6A 01");
        static auto dword_CC9364 = *pattern.get_first<uint32_t*>(2);
        static auto LoadResourceFile = [](const char* ResourceFileName, int32_t ResourceFileType = 1, int32_t nUnk1 = 0, int32_t nUnk2 = 0, int32_t nUnk3 = 0, int32_t nUnk4 = 0, int32_t nUnk5 = 0)
        {
            auto r = CreateResourceFile(ResourceFileName, ResourceFileType, nUnk1, nUnk2, nUnk3);
            *(uint32_t*)((uint32_t)r + 0x28) = 0x2000;
            *(uint32_t*)((uint32_t)r + 0x2C) = *dword_CC9364;
            *(uint32_t*)((uint32_t)r + 0x24) = (uint32_t)SharedStringPoolAllocate(ResourceFileName);
            ResourceFileBeginLoading(r, nUnk4, nUnk5);
        };

        if (nImproveGamepadSupport < 3)
        {
            static auto TPKPath = GetThisModulePath<std::string>().substr(GetExeModulePath<std::string>().length());

            if (nImproveGamepadSupport == 1)
                TPKPath += "buttons-xbox.tpk";
            else if (nImproveGamepadSupport == 2)
                TPKPath += "buttons-playstation.tpk";

            struct LoadTPK
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.edi = *dword_CC9364;
                    LoadResourceFile(TPKPath.c_str());
                }
            }; injector::MakeInline<LoadTPK>(pattern.get_first(0), pattern.get_first(6));
        }

        const char* ControlsTexts[] = { " 2", " 3", " 4", " 5", " 6", " 7", " 8", " 9", " 10", " 1", " Up", " Down", " Left", " Right", "X Rotation", "Y Rotation", "X Axis", "Y Axis", "Z Axis", "Hat Switch" };
        const char* ControlsTextsXBOX[] = { "B", "X", "Y", "LB", "RB", "View (Select)", "Menu (Start)", "Left stick", "Right stick", "A", "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right", "Right stick Left/Right", "Right stick Up/Down", "Left stick Left/Right", "Left stick Up/Down", "LT / RT", "D-pad" };
        const char* ControlsTextsPS[] = { "Circle", "Square", "Triangle", "L1", "R1", "Select", "Start", "L3", "R3", "Cross", "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right", "Right stick Left/Right", "Right stick Up/Down", "Left stick Left/Right", "Left stick Up/Down", "L2 / R2", "D-pad" };

        static std::vector<std::string> Texts(ControlsTexts, std::end(ControlsTexts));
        static std::vector<std::string> TextsXBOX(ControlsTextsXBOX, std::end(ControlsTextsXBOX));
        static std::vector<std::string> TextsPS(ControlsTextsPS, std::end(ControlsTextsPS));

        pattern = GetPattern("68 04 01 00 00 51 E8 ? ? ? ? 83 C4 10 5F 5E C3"); //0x679B53
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

        static injector::hook_back<int32_t(__stdcall*)(int, int)> hb_666790;
        auto padFix = [](int, int) -> int32_t
        {
            return 0x2A5E19E0;
        };
        pattern = GetPattern("E8 ? ? ? ? 3D ? ? ? ? 0F 87 ? ? ? ? 0F 84 ? ? ? ? 3D ? ? ? ? 0F 87 ? ? ? ? 0F 84 ? ? ? ? 3D ? ? ? ? 0F 87"); //0x670B7A
        hb_666790.fun = injector::MakeCALL(pattern.get_first(0), static_cast<int32_t(__stdcall*)(int, int)>(padFix), true).get();

        //Remap
        enum NFSUC_ACTION
        {
            GAME_ACTION_GAS,
            GAME_ACTION_BRAKE,
            GAME_ACTION_STEERLEFT,
            GAME_ACTION_STEERRIGHT,
            GAME_ACTION_HANDBRAKE,
            GAME_ACTION_GAMEBREAKER,
            GAME_ACTION_NOS,
            GAME_ACTION_SHIFTDOWN,
            GAME_ACTION_SHIFTUP,
            GAME_ACTION_SHIFTFIRST,
            GAME_ACTION_SHIFTSECOND,
            GAME_ACTION_SHIFTTHIRD,
            GAME_ACTION_SHIFTFOURTH,
            GAME_ACTION_SHIFTFIFTH,
            GAME_ACTION_SHIFTSIXTH,
            GAME_ACTION_SHIFTREVERSE,
            GAME_ACTION_SHIFTNEUTRAL,
            GAME_ACTION_RESET,
            CAM_ACTION_CHANGE,
            CAM_ACTION_LOOKBACK,
            HUD_ACTION_MESSENGER,
            HUD_ACTION_PAD_LEFT,
            HUD_ACTION_RACE_NOW,
            HUD_ACTION_SCREENSHOT,
            HUD_ACTION_PAUSEREQUEST,
            FE_ACTION_LEFT,
            FE_ACTION_RIGHT,
            FE_ACTION_UP,
            FE_ACTION_DOWN,
            FE_ACTION_RLEFT,
            FE_ACTION_RRIGHT,
            FE_ACTION_RUP,
            FE_ACTION_RDOWN,
            FE_ACTION_ACCEPT,
            FE_ACTION_CANCEL,
            FE_ACTION_START,
            FE_ACTION_LTRIG,
            FE_ACTION_RTRIG,
            FE_ACTION_B0,
            FE_ACTION_B1,
            FE_ACTION_B2,
            FE_ACTION_B3,
            FE_ACTION_B4,
            FE_ACTION_B5,
            HUD_ACTION_SKIPNIS,
            HUD_ACTION_NEXTSONG,
            SC_W,
            SC_A,
            SC_S,
            SC_D,
            FE_ACTION_RTRIGGER,
            FE_ACTION_LTRIGGER,
            SC_BACKSLASH,
            DEBUG_ACTION_TOGGLECAM,
            DEBUG_ACTION_TOGGLECARCOLOR,
            SC_BACK,
            SC_K,
            SC_NUMPAD1,
            SC_NUMPAD2,
            SC_NUMPAD3,
            SC_NUMPAD4,
            SC_NUMPAD5,
            SC_NUMPAD6,
            SC_NUMPAD7,
            SC_NUMPAD8,
            SC_NUMPAD9,
            SC_NUMPAD0,
            SC_DECIMAL,
            SC_SUBTRACT,
            SC_ADD
        };

        struct ButtonStruct
        {
            uint32_t v1;
            uint32_t v2;
            uint32_t v3;
            uint32_t v4;
            uint32_t v5;
            uint32_t v6;
            uint32_t v7;
            uint32_t v8;
            uint32_t v9;
            uint32_t v10;
            uint32_t v11;
            uint32_t v12;
        };

        struct ButtonsTable
        {
            char* str;
            ButtonStruct* ptr;
        };

        enum ButtonType
        {
            X_AXIS,
            Y_AXIS,
            BUTTON
        };

        enum PadBtn
        {
            X = 2,
            Y = 3,
            AXIS_Y = 6,
            AXIS_X = 7,
            LS = 8,
            RS = 9
        };

        static auto pBtnTable = *hook::get_pattern<ButtonsTable*>("8D 34 F5 ? ? ? ? 75 10 8B B9 ? ? ? ? 8D 7C 3F 02", 3);
        pattern = GetPattern("F3 0F 2A 44 90 0C"); //0x65C4A1
        struct RemapHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ecx = *(uint32_t*)(regs.ebx + 0x00);
                regs.edx = *(uint32_t*)(regs.edi + regs.ecx + 0x08);

                pBtnTable[FE_ACTION_RUP].ptr->v7 = PadBtn::AXIS_Y;
                pBtnTable[FE_ACTION_RUP].ptr->v9 = ButtonType::Y_AXIS;

                pBtnTable[FE_ACTION_RLEFT].ptr->v7 = PadBtn::AXIS_Y;
                pBtnTable[FE_ACTION_RLEFT].ptr->v9 = ButtonType::X_AXIS;

                pBtnTable[FE_ACTION_RDOWN].ptr->v7 = PadBtn::AXIS_X;
                pBtnTable[FE_ACTION_RDOWN].ptr->v9 = ButtonType::Y_AXIS;

                pBtnTable[FE_ACTION_RRIGHT].ptr->v7 = PadBtn::AXIS_X;
                pBtnTable[FE_ACTION_RRIGHT].ptr->v9 = ButtonType::X_AXIS;

                pBtnTable[FE_ACTION_B4].ptr->v9 = PadBtn::X;

                pBtnTable[FE_ACTION_B5].ptr->v7 = ButtonType::BUTTON;
                pBtnTable[FE_ACTION_B5].ptr->v9 = PadBtn::Y;

                pBtnTable[FE_ACTION_B3].ptr->v9 = PadBtn::RS;

                pBtnTable[FE_ACTION_B2].ptr->v9 = PadBtn::LS;
            }
        }; injector::MakeInline<RemapHook>(pattern.get_first(-6), pattern.get_first(0));
    }
}

void Init3()
{
    CIniReader iniReader("");
    bool bWriteSettingsToFile = iniReader.ReadInteger("MISC", "WriteSettingsToFile", 1) != 0;
    static auto szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "0");
    if (szCustomUserFilesDirectoryInGameDir.empty() || szCustomUserFilesDirectoryInGameDir == "0")
        szCustomUserFilesDirectoryInGameDir.clear();

    auto GetFolderPathpattern = GetPattern("50 6A 00 6A 00 68 ? 80 00 00 6A 00");
    if (!szCustomUserFilesDirectoryInGameDir.empty())
    {
        szCustomUserFilesDirectoryInGameDir = GetExeModulePath<std::string>() + szCustomUserFilesDirectoryInGameDir;

        auto SHGetFolderPathAHook = [](HWND /*hwnd*/, int /*csidl*/, HANDLE /*hToken*/, DWORD /*dwFlags*/, LPSTR pszPath)->HRESULT
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

    if (bWriteSettingsToFile)
    {
        auto[DesktopResW, DesktopResH] = GetDesktopRes();
        char szSettingsSavePath[MAX_PATH];
        uintptr_t GetFolderPathCallDest = injector::GetBranchDestination(GetFolderPathpattern.get(0).get<uintptr_t>(14), true).as_int();

        MEMORY_BASIC_INFORMATION mbi;
        VirtualQuery((PVOID)GetFolderPathCallDest, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
        HMODULE hm = NULL;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&Init3, &hm);

        if (mbi.AllocationBase == hm || mbi.AllocationBase == GetModuleHandle(L"shfolder"))
        {
            injector::stdcall<HRESULT(HWND, int, HANDLE, DWORD, LPSTR)>::call(GetFolderPathCallDest, NULL, 0x8005, NULL, NULL, szSettingsSavePath);
            strcat(szSettingsSavePath, "\\NFS Undercover");
            strcat(szSettingsSavePath, "\\Settings.ini");

            RegistryWrapper("Need for Speed", szSettingsSavePath);
            auto RegIAT = *GetPattern("FF 15 ? ? ? ? 8D 54 24 40 52 68 3F 00 0F 00").get(0).get<uintptr_t*>(2);
            injector::WriteMemory(&RegIAT[0], RegistryWrapper::RegCreateKeyA, true);
            injector::WriteMemory(&RegIAT[2], RegistryWrapper::RegOpenKeyExA, true);
            injector::WriteMemory(&RegIAT[4], RegistryWrapper::RegCloseKey, true);
            injector::WriteMemory(&RegIAT[1], RegistryWrapper::RegSetValueExA, true);
            injector::WriteMemory(&RegIAT[3], RegistryWrapper::RegQueryValueExA, true);
            RegistryWrapper::AddPathWriter("Install Dir", "InstallDir", "Path");
            RegistryWrapper::AddDefault("@", "INSERTYOURCDKEYHERE");
            RegistryWrapper::AddDefault("CD Drive", "D:\\");
            RegistryWrapper::AddDefault("FirstTime", "0");
            RegistryWrapper::AddDefault("CacheSize", "5697825792");
            RegistryWrapper::AddDefault("SwapSize", "73400320");
            RegistryWrapper::AddDefault("Language", "Engish (US)");
            RegistryWrapper::AddDefault("StreamingInstall", "0");
            RegistryWrapper::AddDefault("g_CarEffects", "3");
            RegistryWrapper::AddDefault("g_WorldFXLevel", "3");
            RegistryWrapper::AddDefault("g_RoadReflectionEnable", "0");
            RegistryWrapper::AddDefault("g_WorldLodLevel", "2");
            RegistryWrapper::AddDefault("g_CarLodLevel", "0");
            RegistryWrapper::AddDefault("g_FSAALevel", "3");
            RegistryWrapper::AddDefault("g_RainEnable", "0");
            RegistryWrapper::AddDefault("g_TextureFiltering", "2");
            RegistryWrapper::AddDefault("g_PerformanceLevel", "0");
            RegistryWrapper::AddDefault("g_VSyncOn", "0");
            RegistryWrapper::AddDefault("g_ShadowEnable", "3");
            RegistryWrapper::AddDefault("g_SmokeEnable", "1");
            RegistryWrapper::AddDefault("g_Brightness", "68");
            RegistryWrapper::AddDefault("g_ShaderDetailLevel", "0");
            RegistryWrapper::AddDefault("g_AudioDetail", "0");
            RegistryWrapper::AddDefault("g_AudioMode", "1");
            RegistryWrapper::AddDefault("g_Width", std::to_string(DesktopResW));
            RegistryWrapper::AddDefault("g_Height", std::to_string(DesktopResH));
            RegistryWrapper::AddDefault("g_Refresh", "60");
            RegistryWrapper::AddDefault("g_CarDamageDetail", "2");
            RegistryWrapper::AddDefault("AllowR32FAA", "0");
            RegistryWrapper::AddDefault("ForceR32AA", "0");

            // get the ShadowLevel setting from the settings ini -- it's important to get this before the game uses it
            CIniReader shadowini(szSettingsSavePath);
            ShadowLevel = shadowini.ReadInteger("Need for Speed Undercover", "g_ShadowEnable", 3);
        }
        else
        {
            auto msgboxID = MessageBox(
                                NULL,
                                (LPCWSTR)L"WriteSettingsToFile option will not work with your exe version. Use different exe or disable that option.\nDo you want to disable it now?",
                                (LPCWSTR)L"NFSUndercover.GenericFix",
                                MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON1
                            );

            if (msgboxID == IDYES)
            {
                iniReader.WriteInteger("MISC", "WriteSettingsToFile", 0);
                bWriteSettingsToFile = false;
            }
        }
    }
    else
    {
        // get the ShadowLevel setting from the registry -- it's important to get this before the game uses it
        HKEY hUCKey = 0;
        DWORD type = REG_DWORD;
        DWORD cbtype = REG_DWORD;
        LSTATUS status = RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\EA Games\\Need for Speed Undercover", &hUCKey);
        if (status == ERROR_SUCCESS)
        {
            RegQueryValueExA(hUCKey, "g_ShadowEnable", NULL, &type, (LPBYTE)&ShadowLevel, &cbtype);
            RegCloseKey(hUCKey);
        }
    }
}

void Init4()
{
    CIniReader iniReader("");

    bool bScaling = iniReader.ReadInteger("MAIN", "Scaling", 0) != 0;
    static int32_t nWindowedMode = iniReader.ReadInteger("MISC", "WindowedMode", 0);

    // HUD Refresh
    auto pattern = GetPattern("38 86 E3 00 00 00 5F");
    injector::MakeNOP(pattern.get_first(7), 2, true);

    // 00793251 = Widescreen FMV call. Will fix them later
    // HUD Width
    static float fHudScale = 1.0f;
    static float fHudWidth = 1280.0f;
    static float fHudXPos = -1.0f;
    pattern = GetPattern("E8 ? ? ? ? 84 C0 74 0A F3 0F 10 05");
    auto dword_C21B00 = *pattern.get_first<void*>(13);
    pattern = GetPattern(pattern_str(0xF3, 0x0F, 0x10, 0x05, to_bytes(dword_C21B00))); //movss   xmm0, ds:dword_C21B00
    for (size_t i = 0; i < pattern.size(); ++i)
    {
        injector::WriteMemory(pattern.get(i).get<uint32_t>(4), &fHudWidth, true);
    }

    static auto hGameHWND = *hook::get_pattern<HWND*>("8B 0D ? ? ? ? 6A 01 51 8B C8", 2);
    pattern = GetPattern("C7 40 ? ? ? ? ? 88 50 20");
    static uint32_t* Width = nullptr;
    static uint32_t* Height = nullptr;
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.eax + 0x14) = 1;
            if (!Width || !Height)
            {
                Width = (uint32_t*)(regs.eax + 0x18);
                Height = (uint32_t*)(regs.eax + 0x1C);
            }
            else
            {
                float fAspectRatio = static_cast<float>(*Width) / static_cast<float>(*Height);
                if (fAspectRatio <= 1.34)
                {
                    fHudScale = 1280.0f / (720.0f * fAspectRatio);
                    fHudWidth = 1280.0f / fHudScale;
                    fHudXPos = -0.75f * fHudScale;
                }
                else
                {
                    fHudScale = 1280.0f / (720.0f * fAspectRatio);
                    fHudWidth = 1280.0f / fHudScale;
                    fHudXPos = -1.0f * fHudScale;
                }
            }

            if (nWindowedMode)
            {
                tagRECT rc;
                auto[DesktopResW, DesktopResH] = GetDesktopRes();
                rc.left = (LONG)(((float)DesktopResW / 2.0f) - ((float)*Width / 2.0f));
                rc.top = (LONG)(((float)DesktopResH / 2.0f) - ((float)*Height / 2.0f));
                rc.right = *Width;
                rc.bottom = *Height;
                SetWindowPos(*hGameHWND, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
                if (nWindowedMode > 1)
                {
                    SetWindowLong(*hGameHWND, GWL_STYLE, GetWindowLong(*hGameHWND, GWL_STYLE) | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU);
                    SetWindowPos(*hGameHWND, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
                }
                else
                {
                    SetWindowLong(*hGameHWND, GWL_STYLE, GetWindowLong(*hGameHWND, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
                }
            }
        }
    }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(7));

    // Force 16:9 HUD settings if aspect ratio is greater than 4:3
    pattern = GetPattern("D9 05 ? ? ? ? D9 C9 DF F1 DD D8 ? ?  B8 ? ? ? ? C3 33 C0 C3");
    struct HUDWidescreenModeHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float fAspectRatio = static_cast<float>(*Width) / static_cast<float>(*Height);
            if (fAspectRatio <= 1.333333373f)
                regs.eax = 0;
            else
                regs.eax = 1;
        }
    };
    injector::MakeInline<HUDWidescreenModeHook>(pattern.count(1).get(0).get<uint32_t>(12), pattern.count(1).get(0).get<uint32_t>(19)); // 7454F1

    //HUD X Pos
    pattern = GetPattern("F3 0F 11 44 24 0C E8 ? ? ? ? 0F 57 C0 F3 0F 10 0D ? ? ? ? F3 0F 10 15");
    injector::WriteMemory(pattern.get_first(26), &fHudXPos, true);

    // HUD Render Fix
    static float fRenderWidth_99999 = 99999.0f;
    pattern = hook::pattern("E8 ? ? ? ? 84 C0 74 0A F3 0F");
    uint32_t* dword_781C61 = pattern.count(13).get(3).get<uint32_t>(13);
    injector::WriteMemory(dword_781C61, &fRenderWidth_99999, true);
    uint32_t* dword_7B1DB3 = pattern.count(13).get(11).get<uint32_t>(13);
    injector::WriteMemory(dword_7B1DB3, &fRenderWidth_99999, true);

    pattern = GetPattern("F3 0F 11 86 8C 05 00 00 F3 0F 11 86 90 05 00 00");
    struct HUDRenderHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esi + 0x58C) = -99999.0f;
        }
    };
    injector::MakeInline<HUDRenderHook>(pattern.count(8).get(0).get<uint32_t>(0), pattern.count(8).get(0).get<uint32_t>(8)); // 781C7D
    injector::MakeInline<HUDRenderHook>(pattern.count(8).get(6).get<uint32_t>(0), pattern.count(8).get(6).get<uint32_t>(8)); // 7B1DD5

    // GPS Map Background Width
    static float fWidthPositive_99999 = 99999.0f;
    static float fWidthNegative_99999 = -99999.0f;

    pattern = GetPattern("E8 ? ? ? ? D9 5C 24 0C E8 ? ? ? ? D9 5C 24 10 6A 00");
    struct GPSMapBackgroundHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.esp = (fWidthPositive_99999);
            float esp00 = regs.esp;
            _asm fld dword ptr ds : [esp00]
        }
    };
    struct GPSMapBackgroundHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            float esp04 = (regs.esp + 0x04);

            _asm
            {
                fld dword ptr ds : [fWidthNegative_99999]
                fst dword ptr ds : [esp04]
            }
        }
    };
    injector::MakeInline<GPSMapBackgroundHook1>(pattern.count(1).get(0).get<uint32_t>(0)); // 830301
    injector::MakeInline<GPSMapBackgroundHook2>(pattern.count(1).get(0).get<uint32_t>(126), pattern.count(1).get(0).get<uint32_t>(132)); // 83037F

    // Transition Overlay Width
    pattern = GetPattern("E8 ? ? ? ? D9 5C 24 14 E8 ? ? ? ? D9 5C 24 1C");
    struct TransitionOverlayHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.esp = (fWidthPositive_99999);
            float esp00 = regs.esp;
            _asm fld dword ptr ds : [esp00]
        }
    }; injector::MakeInline<TransitionOverlayHook>(pattern.count(1).get(0).get<uint32_t>(0)); // 561A00

    uint32_t* dword_561AA0 = hook::pattern("D9 05 ? ? ? ? D9 54 24 04 D9 1C 24 E8 ? ? ? ? 8B 0D").count(1).get(0).get<uint32_t>(2);
    injector::WriteMemory(dword_561AA0, &fWidthNegative_99999, true);

    // Force 16:9 FOV Settings
    static float fVertFOV = 1.777777777f;
    uint32_t* dword_76B7FF = hook::pattern("A1 ? ? ? ? 83 EC 08 80 B8 ? ? ? ? 00 ? ? 6A 14 B9").count(1).get(0).get<uint32_t>(15);
    injector::WriteMemory<uint8_t>(dword_76B7FF, 0xEB, true); // jmp
    uint32_t* dword_74A81D = hook::pattern("D9 C9 DF F1 DD D8 ? ? F2 0F 10 0D").count(1).get(0).get<uint32_t>(6);
    injector::MakeNOP(dword_74A81D, 2, true); // 2 nops
    uint32_t* dword_76B84C = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 11 04 24 D9 04 24 83 C4 08").count(1).get(0).get<uint32_t>(4);
    injector::WriteMemory(dword_76B84C, &fVertFOV, true); // 2 nops

    // FOV Scaling
    if (bScaling)
    {
        static constexpr double Corrected_Hor = 1.3f;
        uint32_t* dword_76B7FF = hook::pattern("F2 0F 10 0D ? ? ? ? 0F B7 C5 F2 0F 2A").count(1).get(0).get<uint32_t>(55);
        injector::WriteMemory(dword_76B7FF, &Corrected_Hor, true);
    }

    // FOV
    pattern = GetPattern("F3 0F 10 44 24 38 F3 0F 10 4C 24 28"); //0x74A959
    struct FOVHook
    {
        void operator()(injector::reg_pack& regs)
        {

            float esp38 = *(float*)(regs.esp + 0x38);
            *(float*)(regs.esp + 0x38) = (esp38 * fHudScale);
            esp38 = *(float*)(regs.esp + 0x38);
            _asm movss xmm0, esp38
        }
    }; injector::MakeInline<FOVHook>(pattern.get_first(0), pattern.get_first(6));

    if (nWindowedMode)
    {
        pattern = GetPattern("89 5D 3C 89 5D 18 89 5D 44"); //0x708379
        struct WindowedMode
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uint32_t*)(regs.ebp + 0x3C) = 1;
                *(uint32_t*)(regs.ebp + 0x18) = regs.ebx;
            }
        }; injector::MakeInline<WindowedMode>(pattern.get_first(0), pattern.get_first(6));

        pattern = GetPattern("C7 06 ? ? ? ? C7 46 ? ? ? ? ? 83 79 3C 00 57");
        struct WindowedModeRect
        {
            void operator()(injector::reg_pack& regs)
            {
                tagRECT rc;
                auto[DesktopResW, DesktopResH] = GetDesktopRes();

                rc.left = 0;
                rc.top = 0;
                rc.right = DesktopResW;
                rc.bottom = DesktopResH;

                *(uint32_t*)(regs.esi + 0x00) = rc.left;
                *(uint32_t*)(regs.esi + 0x04) = rc.top;
                *(uint32_t*)(regs.esi + 0x08) = rc.right;
                *(uint32_t*)(regs.esi + 0x0C) = rc.bottom;
            }
        }; injector::MakeInline<WindowedModeRect>(pattern.get_first(0), pattern.get_first(13));
        pattern = GetPattern("89 46 08 83 79 3C 00 74 08 8B 49 54");
        injector::MakeNOP(pattern.get_first(0), 3, true);
        pattern = GetPattern("89 46 08 89 4E 0C E8 ? ? ? ? 83 C4 08 8B F8 6A 00");
        injector::MakeNOP(pattern.get_first(0), 6, true);
    }

    // Cascade Shadow Maps - resolution and scale adjustments
    ShadowsRes = iniReader.ReadInteger("GRAPHICS", "ShadowsRes", 1024);
    iniShadowsRes = ShadowsRes; // save the ShadowsRes from the ini
    static float CSMScale = iniReader.ReadFloat("GRAPHICS", "CSMScale", 1.0f);
    static float CSMScaleNear = iniReader.ReadFloat("GRAPHICS", "CSMScaleNear", 5.0f) * CSMScale;
    static float CSMScaleMid = iniReader.ReadFloat("GRAPHICS", "CSMScaleMid", 30.0f) * CSMScale;
    static float CSMScaleFar = iniReader.ReadFloat("GRAPHICS", "CSMScaleFar", 170.0f) * CSMScale;

    // limit the resolution - maximum possible resolution is 16384, but as the game has (up to) 3 cascades along the X axis, the res is limited by that
    UpdateShadowsRes();
    if (ShadowsRes < 1)
        ShadowsRes = 1;

    // 0x00793E21 - shadow texture creation
    pattern = hook::pattern("51 6A 01 68 00 04 00 00 C1 E0 0A");

    // 0x00793E24
    uint32_t* ShadowTexCaveEntry = pattern.count(1).get(0).get<uint32_t>(3);
    ShadowTexCaveExit = (uint32_t)ShadowTexCaveEntry + 9;

    injector::MakeJMP(ShadowTexCaveEntry, ShadowTexCave, true);

    // 0x79230D - shadow rendering
    pattern = hook::pattern("8B 46 08 0F 57 C0 C1 E0 0A");

    struct ShadowsResHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.esp + 4) = (uint32_t)(fShadowsRes * (float)(regs.eax));
            *(uint32_t*)(regs.esp + 0xC) = ShadowsRes;
            *(uint32_t*)(regs.esp + 0x10) = iniShadowsRes;
        }
    }; injector::MakeInline<ShadowsResHook2>(pattern.get_first(6), pattern.get_first(26));

    // 0x00780DC0 - shadow cascade scale - CSM::Update()
    pattern = hook::pattern("55 8B EC 83 E4 F0 81 EC 24 02 00 00 F3 0F 10 05 ? ? ? ?");
    //static float* CSM_fScaleLerp_D531B4 = *pattern.count(1).get(0).get<float*>(16);

    // 0x00D53F38 - float CSM::fCSMScaleLevel[] -- individual cascade scales
    static float* CSM_fCSMScaleLevel = *pattern.count(1).get(0).get<float*>(47);
    // disable writes to the array -- MIGHT BREAK CUTSCENES as they have their own scales!
    uint32_t* dword_780DEB = pattern.count(1).get(0).get<uint32_t>(43);
    uint32_t* dword_780E29 = pattern.count(1).get(0).get<uint32_t>(105);
    uint32_t* dword_780E31 = pattern.count(1).get(0).get<uint32_t>(113);
    injector::MakeNOP(dword_780DEB, 8);
    injector::MakeNOP(dword_780E29, 8);
    injector::MakeNOP(dword_780E31, 8);

    CSM_fCSMScaleLevel[0] = CSMScaleNear;
    CSM_fCSMScaleLevel[1] = CSMScaleMid;
    CSM_fCSMScaleLevel[2] = CSMScaleFar;


    // 0x00780663 - force isShadowMapMeshVisible() to true 
    pattern = hook::pattern("F2 0F 5A D2 F3 0F 5A D2 0F 5A D9 66 0F 2F D3 77 15");
    uint32_t* dword_780672 = pattern.count(0).get(0).get<uint32_t>(15);
    uint32_t* dword_780681 = (uint32_t*)((uint32_t)dword_780672 + 9);
    injector::MakeJMP(dword_780672, dword_780681, true);

    // Shadow resolution during CSM::Update() -- some distant shadows are affected by this
    pattern = hook::pattern("68 00 04 00 00 68 00 04 00 00");
    uint32_t* dword_780FFF = pattern.count(0).get(0).get<uint32_t>(10);

    auto CSMUpdateHook = [](uint32_t, uint32_t)->uint32_t
    {
        return ShadowsRes;
    };

    injector::MakeCALL(dword_780FFF, static_cast<uint32_t(__cdecl*)(uint32_t, uint32_t)>(CSMUpdateHook), true);
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        //securom compatibility
        auto ModuleStart = (uintptr_t)GetModuleHandle(NULL);
        MEMORY_BASIC_INFORMATION mbi;
        auto pattern = hook::pattern("FF 25"); //jmp
        for (size_t i = 0; i < pattern.size(); i++)
        {
            auto addr = injector::ReadMemory<uintptr_t>(pattern.get(i).get<void>(2), true);
            VirtualQuery((PVOID)addr, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
            if (RangeEnd < addr && mbi.Protect == PAGE_EXECUTE_WRITECOPY && mbi.AllocationBase == (PVOID)ModuleStart)
            {
                RangeStart = (uintptr_t)mbi.BaseAddress;
                RangeEnd = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
            }
        }

        CallbackHandler::RegisterCallback(Init1, hook::range_pattern(ModuleStart, RangeEnd, "C7 44 24 ? ? ? ? ? FF 15 ? ? ? ? 8D 54 24 0C 52"));
        CallbackHandler::RegisterCallback(Init2, hook::range_pattern(ModuleStart, RangeEnd, "E8 ? ? ? ? 8B 4E 04 83 C4 20 89 46 08 8B 15 ? ? ? ? 51 52 68 0B 20 00 00"));
        CallbackHandler::RegisterCallback(Init3, hook::range_pattern(ModuleStart, RangeEnd, "FF 15 ? ? ? ? 8D 54 24 40 52 68 3F 00 0F 00"));
        CallbackHandler::RegisterCallback(Init4, hook::range_pattern(ModuleStart, RangeEnd, "8B 0D ? ? ? ? 6A 01 51 8B C8"));
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
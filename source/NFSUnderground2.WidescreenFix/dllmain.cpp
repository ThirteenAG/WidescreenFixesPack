#include "stdafx.h"

import ComVars;
import Frontend;
import Rendering;
import NOSTrailFix;

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);

    bool bSkipIntro = iniReader.ReadInteger("MISC", "SkipIntro", 0) != 0;

    static std::filesystem::path CustomUserDir;
    static auto szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "0");
    if (szCustomUserFilesDirectoryInGameDir.empty() || szCustomUserFilesDirectoryInGameDir == "0")
        szCustomUserFilesDirectoryInGameDir.clear();

    bool bWriteSettingsToFile = iniReader.ReadInteger("MISC", "WriteSettingsToFile", 1) != 0;
    static int nImproveGamepadSupport = iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0);
    static float fLeftStickDeadzone = iniReader.ReadFloat("MISC", "LeftStickDeadzone", 10.0f);
    nFPSLimit = iniReader.ReadInteger("MISC", "FPSLimit", -1);
    bool bHighFPSCutscenes = iniReader.ReadInteger("MISC", "HighFPSCutscenes", 1) != 0;
    bool bSingleCoreAffinity = iniReader.ReadInteger("MISC", "SingleCoreAffinity", 0) != 0;
    bool bNoOpticalDriveFix = iniReader.ReadInteger("MISC", "NoOpticalDriveFix", 1) != 0;
    int nWindowedMode = iniReader.ReadInteger("MISC", "WindowedMode", 0);

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    // clamp the size because below 32x32 the game crashes!
    if (Screen.Width < 32)
        Screen.Width = 32;

    if (Screen.Height < 32)
        Screen.Height = 32;

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fHudScaleX = (1.0f / Screen.fWidth * (Screen.fHeight / 480.0f)) * 2.0f;
    Screen.fHudPosX = 640.0f / (640.0f * Screen.fHudScaleX);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;

    //game
    uint32_t* dword_5BF62F = hook::pattern("C7 00 80 02 00 00 C7 01 E0 01 00 00").count(1).get(0).get<uint32_t>(2);
    //injector::MakeNOP(dword_5BF62F - 10 - 13, 13, true);
    injector::WriteMemory(dword_5BF62F, Screen.Width, true);
    uint32_t* dword_5BF646 = hook::pattern("C7 02 20 03 00 00 C7 00 58 02 00 00").count(1).get(0).get<uint32_t>(2);
    injector::WriteMemory(dword_5BF646, Screen.Width, true);
    uint32_t* dword_5BF65D = hook::pattern("C7 01 00 04 00 00 C7 02 00 03 00 00").count(1).get(0).get<uint32_t>(2);
    injector::WriteMemory(dword_5BF65D, Screen.Width, true);
    uint32_t* dword_5BF674 = hook::pattern("C7 00 00 05 00 00 C7 01 C0 03 00 00").count(1).get(0).get<uint32_t>(2);
    injector::WriteMemory(dword_5BF674, Screen.Width, true);
    uint32_t* dword_5BF68B = hook::pattern("C7 02 00 05 00 00 C7 00 00 04 00 00").count(1).get(0).get<uint32_t>(2);
    injector::WriteMemory(dword_5BF68B, Screen.Width, true);
    uint32_t* dword_5BF6A2 = hook::pattern("C7 01 40 06 00 00 C7 02 B0 04 00 00").count(1).get(0).get<uint32_t>(2);
    injector::WriteMemory(dword_5BF6A2, Screen.Width, true);

    uint32_t dword_5BF635 = (uint32_t)dword_5BF62F + 6;
    injector::WriteMemory(dword_5BF635, Screen.Height, true);
    uint32_t dword_5BF64C = (uint32_t)dword_5BF646 + 6;
    injector::WriteMemory(dword_5BF64C, Screen.Height, true);
    uint32_t dword_5BF663 = (uint32_t)dword_5BF65D + 6;
    injector::WriteMemory(dword_5BF663, Screen.Height, true);
    uint32_t dword_5BF67A = (uint32_t)dword_5BF674 + 6;
    injector::WriteMemory(dword_5BF67A, Screen.Height, true);
    uint32_t dword_5BF691 = (uint32_t)dword_5BF68B + 6;
    injector::WriteMemory(dword_5BF691, Screen.Height, true);
    uint32_t dword_5BF6A8 = (uint32_t)dword_5BF6A2 + 6;
    injector::WriteMemory(dword_5BF6A8, Screen.Height, true);

    InitFrontend();
    InitRendering();
    InitNOSTrailFix();

    if (bSkipIntro)
    {
        static constexpr auto SkipThis = "SkipThis";
        auto pattern = hook::pattern("8B F1 50 89 74 24 08 E8 ? ? ? ? 8B 46 04 68 ? ? ? ? 68 ? ? ? ? 50");
        uint32_t* dword_4A888B = pattern.count(3).get(0).get<uint32_t>(16);
        uint32_t* dword_4A895B = pattern.count(3).get(1).get<uint32_t>(16);
        injector::WriteMemory(dword_4A888B, &SkipThis, true); // FMV Opening
        injector::WriteMemory(dword_4A895B, &SkipThis, true); // EA Bumper

        // THX
        uint32_t* dword_4A8CEB = hook::pattern("56 57 8B F1 50 89 74 24 0C E8 ? ? ? ? 68 ? ? ? ? 33 C0").count(1).get(0).get<uint32_t>(15);
        injector::WriteMemory(dword_4A8CEB, &SkipThis, true);

        // PSA
        uint32_t* dword_4A8B74 = hook::pattern("C7 06 ? ? ? ? E8 ? ? ? ? 8B 46 04 68 ? ? ? ? 68 ? ? ? ? 50").count(1).get(0).get<uint32_t>(15);
        injector::WriteMemory(dword_4A8B74, &SkipThis, true);
    }

    if (!szCustomUserFilesDirectoryInGameDir.empty())
    {
        CustomUserDir = GetExeModulePath<std::filesystem::path>();
        CustomUserDir.append(szCustomUserFilesDirectoryInGameDir);

        auto SHGetFolderPathAHook = [](HWND /*hwnd*/, int /*csidl*/, HANDLE /*hToken*/, DWORD /*dwFlags*/, LPSTR pszPath) -> HRESULT
        {
            CreateDirectoryW((LPCWSTR)(CustomUserDir.u16string().c_str()), NULL);
            memcpy(pszPath, CustomUserDir.u8string().data(), CustomUserDir.u8string().size() + 1);

            return S_OK;
        };

        auto pattern = hook::pattern("50 6A 00 6A 00 68 ? 80 00 00 6A 00");
        uint32_t* dword_6CBF17 = pattern.count(1).get(0).get<uint32_t>(12);
        injector::MakeCALL((uint32_t)dword_6CBF17, static_cast<HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR)>(SHGetFolderPathAHook), true);
        injector::MakeNOP((uint32_t)dword_6CBF17 + 5, 1, true);
    }

    if (nImproveGamepadSupport)
    {
        auto pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 51 A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 04 89 44 24 00 85 C0 C7 44 24 ? ? ? ? ? 74 22 8B 4C 24 24");
        static auto CreateResourceFile = (void* (*)(const char* ResourceFileName, int32_t ResourceFileType, int, int, int)) pattern.get_first(0); //0x0057CEA0
        pattern = hook::pattern("8B 44 24 04 56 8B F1 8B 4C 24 0C 89 86 ? ? ? ? 89 8E");
        static auto ResourceFileBeginLoading = (void(__thiscall*)(void* rsc, int a1, int a2)) pattern.get_first(0); //0x0057BCA0
        static auto LoadResourceFile = [](const char* ResourceFileName, int32_t ResourceFileType, int32_t nUnk1 = 0, int32_t nUnk2 = 0, int32_t nUnk3 = 0, int32_t nUnk4 = 0, int32_t nUnk5 = 0)
        {
            auto r = CreateResourceFile(ResourceFileName, ResourceFileType, nUnk1, nUnk2, nUnk3);
            ResourceFileBeginLoading(r, nUnk4, nUnk5);
        };

        if (nImproveGamepadSupport < 4)
        {
            static auto TPKPath = GetThisModulePath<std::string>().substr(GetExeModulePath<std::string>().length()) + "test.tpk";

            static injector::hook_back<void(__cdecl*)()> hb_57FA60;
            auto LoadTPK = []()
            {
                if (std::filesystem::exists(TPKPath))
                {
                    LoadResourceFile(TPKPath.c_str(), 1);
                }
                return hb_57FA60.fun();
            };

            pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 5F C6 05 ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 5E"); //0x57ECEF
            hb_57FA60.fun = injector::MakeCALL(pattern.get_first(0), static_cast<void(__cdecl*)()>(LoadTPK), true).get();
        }

        struct PadState
        {
            int32_t LSAxis1;
            int32_t LSAxis2;
            int32_t LTRT;
            int32_t RSAxis1;
            int32_t RSAxis2;
            uint8_t unk[28];
            int8_t A;
            int8_t B;
            int8_t X;
            int8_t Y;
            int8_t LB;
            int8_t RB;
            int8_t Select;
            int8_t Start;
            int8_t LSClick;
            int8_t RSClick;
        };

        static bool Zstate, Pstate, Tstate, Dstate, Qstate, Cstate, LBRACKETstate, RBRACKETstate;
        static int32_t* nGameState = (int32_t*)*hook::pattern("83 3D ? ? ? ? 06 ? ? A1").count(1).get(0).get<uint32_t*>(2);
        pattern = hook::pattern("B9 ? ? ? ? BE ? ? ? ? F3 A5 5F 5E 5B C3");
        struct CatchPad
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ecx = 0x88;
                PadState* PadKeyPresses = (PadState*)(regs.edi);

                if (PadKeyPresses != nullptr && PadKeyPresses != (PadState*)0x1 && *nGameState == 3)
                {
                    if (PadKeyPresses->LSClick && PadKeyPresses->RSClick)
                    {
                        if (!Qstate)
                        {
                            keybd_event(BYTE(VkKeyScan('Q')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                            keybd_event(BYTE(VkKeyScan('Q')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                        }
                        Qstate = true;
                    }
                    else
                        Qstate = false;

                    /*if (PadKeyPresses->Y)
                    {
                        if (!Zstate)
                        {
                            keybd_event(BYTE(VkKeyScan('P')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                            keybd_event(BYTE(VkKeyScan('P')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                        }
                        Zstate = true;
                    }
                    else
                        Zstate = false;
                        */
                }
            }
        }; injector::MakeInline<CatchPad>(pattern.get_first(0));

        if (nImproveGamepadSupport < 4)
        {
            const char* MenuTexts[] = { "Quit Game", "Continue", "Back", "Reset Keys To Default", "Activate GPS", "Deactivate GPS", "Install Package", "Install Part", "Install Paint", "Install Decal", "Reset To Default", "Delete Tuned Car", "Logoff", "Cancel Changes", "Customize", "Host LAN Server", "Read Message", "Delete", "Test N2O Purge", "Accept", "Reset to default", "Select", "Open/Close Doors", "Open/Close Hood" }; // "Tip",
            const char* MenuTextsPC[] = { "[Q] Quit Game", "[Enter] Continue", "[Esc] Back", "[P] Reset Keys To Default", "[C] Activate GPS", "[C] Deactivate GPS", "[Enter] Install Package", "[Enter] Install Part", "[Enter] Install Paint", "[Enter] Install Decal", "[C] Reset To Default", "[C] Delete Tuned Car", "[Esc] Logoff", "[C] Cancel Changes", "[C] Customize", "[C] Host LAN Server", "[Enter] Read Message", "[C] Delete", "[C] Test N2O Purge", "[Enter] Accept", "[C] Reset to default", "[Enter] Select", "[C] Open/Close Doors", "[C] Open/Close Hood" };
            const char* MenuTextsXBOX[] = { "(LS+RS) Quit Game", "(A) Continue", "(B) Back", "(Y) Reset Keys To Default", "(X) Activate GPS", "(X) Deactivate GPS", "(A) Install Package", "(A) Install Part", "(A) Install Paint", "(A) Install Decal", "(X) Reset To Default", "(X) Delete Tuned Car", "(B) Logoff", "(X) Cancel Changes", "(X) Customize", "(X) Host LAN Server", "(A) Read Message", "(X) Delete", "(X) Test N2O Purge", "(A) Accept", "(X) Reset to default", "(A) Select", "(X) Open/Close Doors", "(X) Open/Close Hood" };
            const char* MenuTextsPS[] = { "(L3+R3) Quit Game", "(Cross) Continue", "(Circle) Back", "(Triangle) Reset Keys To Default", "(Square) Activate GPS", "(Square) Deactivate GPS", "(Cross) Install Package", "(Cross) Install Part", "(Cross) Install Paint", "(Cross) Install Decal", "(Square) Reset To Default", "(Square) Delete Tuned Car", "(Circle) Logoff", "(Square) Cancel Changes", "(Square) Customize", "(Square) Host LAN Server", "(Cross) Read Message", "(Square) Delete", "(Square) Test N2O Purge", "(Cross) Accept", "(Square) Reset to default", "(Cross) Select", "(Square) Open/Close Doors", "(Square) Open/Close Hood" };

            static std::vector<std::string> vMenuStrings(MenuTexts, std::end(MenuTexts));
            static std::vector<std::string> vMenuStringsPC(MenuTextsPC, std::end(MenuTextsPC));
            static std::vector<std::string> vMenuStringsXBOX(MenuTextsXBOX, std::end(MenuTextsXBOX));
            static std::vector<std::string> vMenuStringsPS(MenuTextsPS, std::end(MenuTextsPS));

            pattern = hook::pattern("8B 0D ? ? ? ? 85 C9 50 74 12 8B 44"); //0x5124DD (v1.1)
            static auto dword_8383DC = *pattern.count(1).get(0).get<uint32_t>(2);
            struct MenuText
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.ecx = dword_8383DC;

                    auto pszStr = (char*)regs.eax;
                    auto it = std::find(vMenuStrings.begin(), vMenuStrings.end(), pszStr);
                    auto i = std::distance(vMenuStrings.begin(), it);

                    if (it != vMenuStrings.end())
                    {
                        if (nImproveGamepadSupport == 3)
                            regs.eax = (uint32_t)vMenuStringsPC[i].c_str();
                        else if (nImproveGamepadSupport != 2)
                            regs.eax = (uint32_t)vMenuStringsXBOX[i].c_str();
                        else
                            regs.eax = (uint32_t)vMenuStringsPS[i].c_str();
                    }
                }
            }; injector::MakeInline<MenuText>(pattern.get_first(0), pattern.get_first(6));
        }

        // Start menu text
        uint32_t* dword_4A91E7 = hook::pattern("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 50 55 E8").count(3).get(1).get<uint32_t>(1);
        if (nImproveGamepadSupport != 2)
            injector::WriteMemory(dword_4A91E7, 0xD18D4C4C, true); //"Please press START to begin" (Xbox)
        else
            injector::WriteMemory(dword_4A91E7, 0xDC64C04C, true); //"Press START Button" (PlayStation)

        // FrontEnd button remap (through game code, not key emulation)
        pattern = hook::pattern("89 4E ? 89 4E ? 8B E8"); // 005C1A07
        struct FrontEndRemap
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uintptr_t*)(regs.esi + 0x08) = regs.ecx;
                *(uintptr_t*)(regs.esi + 0x0C) = regs.ecx;

                int Y_Button = (regs.eax + 0x5064);
                *(uintptr_t*)Y_Button = 0x2E; // FE Action "P"
                int Start = (regs.eax + 0x5074);
                *(uintptr_t*)Start = 0x24; // FE Action "Comma"
                int LB = (regs.eax + 0x5068);
                *(uintptr_t*)LB = 0x26; // FE Action "Left Bracket"
                int RB = (regs.eax + 0x506C);
                *(uintptr_t*)RB = 0x27; // FE Action "Right Bracket"
            }
        };
        injector::MakeInline<FrontEndRemap>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
    }

    const wchar_t* ControlsTexts[] = { L" 2", L" 3", L" 4", L" 5", L" 6", L" 7", L" 8", L" 9", L" 10", L" 1", L" Up", L" Down", L" Left", L" Right", L"X Rotation", L"Y Rotation", L"X Axis", L"Y Axis", L"Z Axis", L"Hat Switch" };
    const wchar_t* ControlsTextsXBOX[] = { L"B", L"X", L"Y", L"LB", L"RB", L"View (Select)", L"Menu (Start)", L"Left stick", L"Right stick", L"A", L"D-pad Up", L"D-pad Down", L"D-pad Left", L"D-pad Right", L"Right stick Left/Right", L"Right stick Up/Down", L"Left stick Left/Right", L"Left stick Up/Down", L"LT / RT", L"D-pad" };
    const wchar_t* ControlsTextsPS[] = { L"Circle", L"Square", L"Triangle", L"L1", L"R1", L"Select", L"Start", L"L3", L"R3", L"Cross", L"D-pad Up", L"D-pad Down", L"D-pad Left", L"D-pad Right", L"Right stick Left/Right", L"Right stick Up/Down", L"Left stick Left/Right", L"Left stick Up/Down", L"L2 / R2", L"D-pad" };

    static std::vector<std::wstring> Texts(ControlsTexts, std::end(ControlsTexts));
    static std::vector<std::wstring> TextsXBOX(ControlsTextsXBOX, std::end(ControlsTextsXBOX));
    static std::vector<std::wstring> TextsPS(ControlsTextsPS, std::end(ControlsTextsPS));

    auto pattern = hook::pattern("66 83 7C 24 08 00 5F 74 13 8D 4C 24 04 51"); //0x4ACF01
    injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(0), 0x5F, true); //pop     edi
    static auto dword_4ACF1D = pattern.count(1).get(0).get<uint32_t>(28);
    struct Buttons
    {
        void operator()(injector::reg_pack& regs)
        {
            auto pszStr = (wchar_t*)(regs.esp + 0x4);
            if (wcslen(pszStr))
            {
                if (nImproveGamepadSupport)
                {
                    auto it = std::find_if(Texts.begin(), Texts.end(), [&](const std::wstring& str) { std::wstring s(pszStr); return s.find(str) != std::wstring::npos; });
                    auto i = std::distance(Texts.begin(), it);

                    if (it != Texts.end())
                    {
                        const std::wstring& text = (nImproveGamepadSupport != 2) ? TextsXBOX[i] : TextsPS[i];
                        wcscpy_s(pszStr, text.length() + 1, text.c_str());
                    }
                }
            }
            else
                *(uintptr_t*)(regs.esp - 4) = (uintptr_t)dword_4ACF1D;
        }
    }; injector::MakeInline<Buttons>(pattern.get_first(1), pattern.get_first(9));

    if (fLeftStickDeadzone)
    {
        // [ -10000 | 10000 ]
        static int32_t nLeftStickDeadzone = static_cast<int32_t>(fLeftStickDeadzone * 100.0f);
        pattern = hook::pattern("85 F6 7D 38 53 68 ? ? ? ? E8"); //0x5C877D
        static auto loc_5C87B9 = (uint32_t)hook::get_pattern("83 FB 01 75 ? B9 04 00 00 00");
        static auto dword_86FFC0 = *hook::get_pattern<int32_t*>("8D 95 ? ? ? ? 8D 3C 28 8B", 2);
        struct DeadzoneHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.esi = regs.eax;
                if (*(int32_t*)&regs.esi >= 0)
                {
                    int32_t dStickStateX = *(int32_t*)(dword_86FFC0 + 0);
                    int32_t dStickStateY = *(int32_t*)(dword_86FFC0 + 1);

                    *(int32_t*)(dword_86FFC0 + 0) = (std::abs(dStickStateX) <= nLeftStickDeadzone) ? 0 : dStickStateX;
                    *(int32_t*)(dword_86FFC0 + 1) = (std::abs(dStickStateY) <= nLeftStickDeadzone) ? 0 : dStickStateY;

                    *(uint32_t*)regs.esp = loc_5C87B9;
                }
            }
        }; injector::MakeInline<DeadzoneHook>(pattern.get_first(-2), pattern.get_first(4));
    }

    if (nFPSLimit)
    {
        if (nFPSLimit < 0)
        {
            if (nFPSLimit == -1)
                nFPSLimit = GetDesktopRefreshRate();
            else if (nFPSLimit == -2)
                nFPSLimit = GetDesktopRefreshRate() * 2;
            else
                nFPSLimit = 60;
        }

        static float FrameTime = 1.0f / nFPSLimit;
        uint32_t* dword_865558 = *hook::pattern("D9 05 ? ? ? ? B9 64 00 00 00 D8 64").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory(dword_865558, FrameTime, true);
        uint32_t* dword_7FB710 = *hook::pattern("D9 05 ? ? ? ? D8 74 ? ? D9 1D ? ? ? ? C3").count(1).get(0).get<uint32_t*>(33);
        injector::WriteMemory(dword_7FB710, FrameTime, true);
        // Video mode frametime
        float* flt_7875BC = *hook::pattern("8B 44 24 14 D9 05 ? ? ? ? 8B 0D ? ? ? ? D8 44 24 14 8B 15 ? ? ? ? D9 05 ? ? ? ?").count(1).get(0).get<float*>(0x55); //0x57EB8B anchor, 0x57EBE0 dereference
        injector::WriteMemory(flt_7875BC, FrameTime, true);
        // a function in eDisplayFrame (particle effects?) frametime
        uint32_t* dword_40A744 = hook::pattern("68 89 88 88 3C").count(1).get(0).get<uint32_t>(1);
        injector::WriteMemory(dword_40A744, FrameTime, true);
        // Smokeable frametime
        float* flt_7FC858 = *hook::pattern("8B F1 E8 ? ? ? ? 8A 86 06 05 00 00 84 C0 0F 84 ? ? ? ?").count(1).get(0).get<float*>(0x2F); //0x5B1DE0 anchor, 0x005B1E0F dereference
        injector::WriteMemory(flt_7FC858, FrameTime, true);

        // GAME BUGFIX: disable player steering autocentering to prevent sticky input
        // same thing as NFSU fix, check its dllmain.cpp for more info
        uint32_t* dword_416D82 = hook::pattern("DD D8 D9 44 24 18 D9 54 24 1C D9 E1 D8 1D ? ? ? ? DF E0 F6 C4 05").count(1).get(0).get<uint32_t>(0x17); //0x416D6B anchor
        injector::WriteMemory<uint8_t>(dword_416D82, 0xEB, true);
    }

    if (bHighFPSCutscenes)
    {
        static int AnimSceneFPS = 60;
        static float fAnimSceneFPS = 60.0f;

        if (nFPSLimit > 0)
            AnimSceneFPS = nFPSLimit;

        if (AnimSceneFPS % 30)
            AnimSceneFPS = AnimSceneFPS - (AnimSceneFPS % 30);

        // car physics are buggy above 60 FPS
        // TODO: bugfix this if possible
        if (AnimSceneFPS > 60)
            AnimSceneFPS = 60;

        if (AnimSceneFPS < 30)
            AnimSceneFPS = 30;

        fAnimSceneFPS = (float)AnimSceneFPS;

        uint32_t* dword_435FA4 = hook::pattern("7C ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 ? 5F 5E").count(1).get(0).get<uint32_t>(3);
        injector::WriteMemory(dword_435FA4, fAnimSceneFPS, true);
        uint32_t* dword_7A572C = *hook::pattern("8B 15 ? ? ? ? 52 A3 ? ? ? ? E8 ? ? ? ? 83 C4 14").count(1).get(0).get<uint32_t*>(2);
        injector::WriteMemory(dword_7A572C, fAnimSceneFPS, true);
    }

    if (bSingleCoreAffinity)
    {
        if (AffinityChanges::Init())
        {
            IATHook::Replace(GetModuleHandleA(NULL), "KERNEL32.DLL",
                std::forward_as_tuple("CreateThread", AffinityChanges::CreateThread_GameThread)
            );
        }
    }

    if (bNoOpticalDriveFix)
    {
        auto pattern = hook::pattern("56 8B 35 ? ? ? ? A1 ? ? ? ? 85 C0 ? ? E8");
        uint32_t* dword_5C0D3F = pattern.count(1).get(0).get<uint32_t>(15);
        injector::WriteMemory<uint8_t>(dword_5C0D3F, 0x78, true);
        uint32_t* dword_5C0D54 = pattern.count(1).get(0).get<uint32_t>(36);
        injector::WriteMemory<uint8_t>(dword_5C0D54, 0x00, true);
    }

    if (bWriteSettingsToFile)
    {
        std::filesystem::path SettingsSavePath;
        if (!szCustomUserFilesDirectoryInGameDir.empty())
            SettingsSavePath = CustomUserDir;

        auto GetFolderPathpattern = hook::pattern("50 6A 00 6A 00 68 ? 80 00 00 6A 00");
        uintptr_t GetFolderPathCallDest = injector::GetBranchDestination(GetFolderPathpattern.count(1).get(0).get<uintptr_t>(12), true).as_int();

        if (GetFolderPathCallDest && szCustomUserFilesDirectoryInGameDir.empty())
        {
            char szSettingsSavePath[MAX_PATH];
            injector::stdcall<HRESULT(HWND, int, HANDLE, DWORD, LPSTR)>::call(GetFolderPathCallDest, NULL, 0x8005, NULL, NULL, szSettingsSavePath);
            SettingsSavePath = szSettingsSavePath;
        }

        SettingsSavePath.append("NFS Underground 2");
        SettingsSavePath.append("Settings.ini");

        if (GetFolderPathCallDest || !szCustomUserFilesDirectoryInGameDir.empty())
        {
            RegistryWrapper("Need for Speed", SettingsSavePath);
            uintptr_t* RegIAT = *hook::pattern("FF 15 ? ? ? ? 8D 54 24 04 52").get(0).get<uintptr_t*>(2);
            injector::WriteMemory(&RegIAT[0], RegistryWrapper::RegCreateKeyA, true);
            injector::WriteMemory(&RegIAT[-1], RegistryWrapper::RegOpenKeyA, true);
            injector::WriteMemory(&RegIAT[-2], RegistryWrapper::RegOpenKeyExA, true);
            injector::WriteMemory(&RegIAT[1], RegistryWrapper::RegCloseKey, true);
            injector::WriteMemory(&RegIAT[-4], RegistryWrapper::RegSetValueExA, true);
            injector::WriteMemory(&RegIAT[-3], RegistryWrapper::RegQueryValueExA, true);
            RegistryWrapper::AddPathWriter("Install Dir", "InstallDir", "Path");
            RegistryWrapper::AddDefault("@", "INSERTYOURCDKEYHERE");
            RegistryWrapper::AddDefault("CD Drive", "D:\\");
            RegistryWrapper::AddDefault("CacheSize", "1879040000");
            RegistryWrapper::AddDefault("SwapSize", "0");
            RegistryWrapper::AddDefault("Language", "English US");
            RegistryWrapper::AddDefault("StreamingInstall", "0");
            RegistryWrapper::AddDefault("g_CarEnvironmentMapEnable", "3");
            RegistryWrapper::AddDefault("g_CarEnvironmentMapUpdateData", "1");
            RegistryWrapper::AddDefault("g_CarShadowEnable", "2");
            RegistryWrapper::AddDefault("g_CarHeadlightEnable", "1");
            RegistryWrapper::AddDefault("g_CarLightingEnable", "0");
            RegistryWrapper::AddDefault("g_CarDamageEnable", "0");
            RegistryWrapper::AddDefault("g_CrowdEnable", "1");
            RegistryWrapper::AddDefault("g_RoadReflectionEnable", "3");
            RegistryWrapper::AddDefault("g_FogEnable", "1");
            RegistryWrapper::AddDefault("g_MotionBlurEnable", "1");
            RegistryWrapper::AddDefault("g_LightStreaksEnable", "1");
            RegistryWrapper::AddDefault("g_LightGlowEnable", "1");
            RegistryWrapper::AddDefault("g_AnimatedTextureEnable", "1");
            RegistryWrapper::AddDefault("g_ParticleSystemEnable", "1");
            RegistryWrapper::AddDefault("g_DepthOfFieldEnable", "1");
            RegistryWrapper::AddDefault("g_WorldLodLevel", "3");
            RegistryWrapper::AddDefault("g_CarLodLevel", "1");
            RegistryWrapper::AddDefault("g_OverBrightEnable", "1");
            RegistryWrapper::AddDefault("g_BleachByPassEnable", "1");
            RegistryWrapper::AddDefault("g_TintingEnable", "1");
            RegistryWrapper::AddDefault("g_FSAALevel", "7");
            RegistryWrapper::AddDefault("g_HorizonFogEnable", "1");
            RegistryWrapper::AddDefault("g_RainEnable", "1");
            RegistryWrapper::AddDefault("g_TextureFiltering", "2");
            RegistryWrapper::AddDefault("g_RacingResolution", "4");
            RegistryWrapper::AddDefault("g_PerformanceLevel", "5");
            RegistryWrapper::AddDefault("g_VSyncOn", "0");
        }
    }

    // windowed mode
    if (nWindowedMode)
    {
        uint32_t* dword_5D2795 = hook::pattern("A1 ? ? ? ? 68 00 00 00 10").count(1).get(0).get<uint32_t>(0);
        uint32_t* dword_5D27A8 = (uint32_t*)((uint32_t)dword_5D2795 + 0x13);

        uint32_t* dword_5D2674 = hook::pattern("68 00 00 00 80 68 ? ? ? ? 68 ? ? ? ? 56 FF 15").count(1).get(0).get<uint32_t>(16);
        uint32_t* dword_5D2638 = hook::pattern("68 00 00 00 80 8D 4C 24 20 8D 43 64 51 89 54 24 2C 89 44 24 30 FF 15").count(1).get(0).get<uint32_t>(21);
        uint32_t* WindowedMode_87098C = *hook::pattern("B9 0E 00 00 00 BF ? ? ? ? F3 AB A1 ? ? ? ?").count(1).get(0).get<uint32_t*>(13);

        // skip SetWindowLong because it messes things up
        injector::MakeJMP(dword_5D2795, dword_5D27A8, true);
        // hook the offending functions
        injector::MakeNOP(dword_5D2674, 6, true);
        injector::MakeCALL(dword_5D2674, WindowedModeWrapper::CreateWindowExA_Hook, true);
        injector::MakeNOP(dword_5D2638, 6, true);
        injector::MakeCALL(dword_5D2638, WindowedModeWrapper::AdjustWindowRect_Hook, true);
        // enable windowed mode variable
        *WindowedMode_87098C = 1;

        switch (nWindowedMode)
        {
            case 5:
                WindowedModeWrapper::bStretchWindow = true;
                break;
            case 4:
                WindowedModeWrapper::bScaleWindow = true;
                break;
            case 3:  // TODO: implement dynamic resizing (like in MW)
                WindowedModeWrapper::bEnableWindowResize = true;
            case 2:
                WindowedModeWrapper::bBorderlessWindowed = false;
                break;
            default:
                break;
        }
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("C7 00 80 02 00 00 C7 01 E0 01 00 00"));
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

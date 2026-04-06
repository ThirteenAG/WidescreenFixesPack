#include "stdafx.h"
#include <format>

import ComVars;
import Frontend;
import Rendering;
import NOSTrailFix;
import Shadows;

bool* DrawHUD_57CAA8 = (bool*)0x57CAA8;

void updateValues(const float& newWidth, const float& newHeight)
{
    //Screen resolution
    Screen.Width = static_cast<int32_t>(newWidth);
    Screen.Height = static_cast<int32_t>(newHeight);

    // clamp the size because below 32x32 the game crashes!
    if (Screen.Width < 32)
        Screen.Width = 32;

    if (Screen.Height < 32)
        Screen.Height = 32;

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fHudScaleX = (1.0f / Screen.fWidth * (Screen.fHeight / 480.0f)) * 2.0f;
    Screen.fHudPosX = 640.0f / (640.0f * Screen.fHudScaleX);

    //Autosculpt scaling
    *AutosculptScale_8AE8F8 = 480.0f * Screen.fAspectRatio;

    //Arrest blur
    *ArrestBlurScale_8AFA08 = (1.0f / 640.0f) * ((4.0f / 3.0f) / Screen.fAspectRatio);

    //Rain droplets
    fRainScaleX = ((0.75f / Screen.fAspectRatio) * (4.0f / 3.0f));

    if (bFixFOV)
    {
        hor3DScale = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
        if (nScaling)
            hor3DScale /= 1.047485948f;
    }

    if (bFixHUD)
    {
        *HudScaleX_8AF9A4 = Screen.fHudScaleX;
        *FE_Xpos_894B40 = Screen.fHudPosX;
    }

    if (ShadowRes::Resolution)
        ShadowRes::update(ShadowRes::Resolution);

    if (FEScale::bEnabled)
        FEScale::Update();
}

void __stdcall RacingResolution_Hook(int* width, int* height)
{
    *width = Screen.Width;
    *height = Screen.Height;
}

// cave at 0x6E726B - in eDisplayFrame
// for skipping shader recompilation
uint32_t FastWndReset_Exit_True = 0x6E728D;
uint32_t FastWndReset_Exit_False = 0x6E7272;
uint32_t* ResetWnd_982C39 = (uint32_t*)0x00982C39;
void __declspec(naked) FastWndReset_Cave()
{
    if (bIsResizing)
        _asm jmp FastWndReset_Exit_True
    _asm
    {
        mov eax, ResetWnd_982C39
        mov al, byte ptr[eax]
        test al, al
        jmp FastWndReset_Exit_False
    }
}

// cave at 0x6E72C6 - in eDisplayFrame
// at the end of the reset procedure
uint32_t FastWndReset_Finish_Exit = 0x006E72CD;
void __declspec(naked) FastWndReset_Finish_Cave()
{
    bIsResizing = false;
    *DrawHUD_57CAA8 = true;
    _asm
    {
        mov eax, ResetWnd_982C39
        mov[eax], 0
        jmp FastWndReset_Finish_Exit
    }
}

unsigned int GameWndProcAddr = 0;
LRESULT(WINAPI* GameWndProc)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WSFixWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_SIZE:
        {
            bIsResizing = true;
            *DrawHUD_57CAA8 = false;
            updateValues((float)LOWORD(lParam), (float)HIWORD(lParam));
        }
        return TRUE;
    }

    return GameWndProc(hWnd, msg, wParam, lParam);
}

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);

    bool bSkipIntro = iniReader.ReadInteger("MISC", "SkipIntro", 0) != 0;
    bool bFixResolutionText = iniReader.ReadInteger("MISC", "FixResolutionText", 1) != 0;

    static auto szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "");
    static std::filesystem::path CustomUserDir;
    if (szCustomUserFilesDirectoryInGameDir.empty() || szCustomUserFilesDirectoryInGameDir == "0")
        szCustomUserFilesDirectoryInGameDir.clear();

    bool bWriteSettingsToFile = iniReader.ReadInteger("MISC", "WriteSettingsToFile", 1) != 0;
    static int nImproveGamepadSupport = iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0);
    bool bForceHighSpecAudio = iniReader.ReadInteger("MISC", "ForceHighSpecAudio", 1) != 0;
    static float fLeftStickDeadzone = iniReader.ReadFloat("MISC", "LeftStickDeadzone", 10.0f);
    SimRate = iniReader.ReadInteger("MISC", "SimRate", -1);
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
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fHudScaleX = (1.0f / Screen.fWidth * (Screen.fHeight / 480.0f)) * 2.0f;
    Screen.fHudPosX = 640.0f / (640.0f * Screen.fHudScaleX);

    //Screen resolution
    for (size_t i = 0; i < 2; i++)
    {
        uint32_t* sub_6C27D0 = hook::pattern("A1 ? ? ? ? 83 F8 05 0F ? ? 00 00 00 FF 24 85 ? ? ? ? 8B 44 24 04").count(1).get(0).get<uint32_t>(0);
        injector::MakeJMP(sub_6C27D0, RacingResolution_Hook, true);
    }

    InitFrontend();
    InitRendering();
    InitNOSTrailFix();
    InitShadows();

    if (bForceHighSpecAudio)
    {
        uint32_t* dword_4C41F5 = hook::pattern("C7 05 ? ? ? ? ? ? 00 00 A1 ? ? ? ? 85 C0 ? ? A3 ? ? ? ? 8D 44 24 08").count(1).get(0).get<uint32_t>(6);
        injector::WriteMemory<int>(dword_4C41F5, 44100, true);
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
        for (size_t i = 0; i < pattern.size(); i++)
        {
            uint32_t* dword_6CBF17 = pattern.get(i).get<uint32_t>(12);
            if (*(BYTE*)dword_6CBF17 != 0xFF)
                dword_6CBF17 = pattern.get(i).get<uint32_t>(14);

            injector::MakeCALL((uint32_t)dword_6CBF17, static_cast<HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR)>(SHGetFolderPathAHook), true);
            injector::MakeNOP((uint32_t)dword_6CBF17 + 5, 1, true);
        }
    }

    if (bSkipIntro)
    {
        static auto counter = 0;
        static auto og_value = 0;
        auto pattern = hook::pattern("A1 ? ? ? ? 85 C0 74 1C 8B 45 04");
        static uint32_t* dword_926144 = *pattern.get_first<uint32_t*>(1);
        struct SkipIntroHook
        {
            void operator()(injector::reg_pack& regs)
            {
                if (counter < 3)
                {
                    if (counter == 0)
                        og_value = *dword_926144;
                    *dword_926144 = 1;
                    counter++;
                }
                else
                {
                    *dword_926144 = og_value;
                }

                regs.eax = *(uint32_t*)dword_926144;
            }
        }; injector::MakeInline<SkipIntroHook>(pattern.get_first(0));
    }

    if (nImproveGamepadSupport)
    {
        auto pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 51 A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 04 89 04 24 85 C0 C7 44 24 ? ? ? ? ? 74 22 8B 4C 24 24 8B 54 24 20 51");
        static auto CreateResourceFile = (void* (*)(const char* ResourceFileName, int32_t ResourceFileType, int, int, int)) pattern.get_first(0); //0x0065FD30
        pattern = hook::pattern("8B 44 24 04 56 8B F1 8B 4C 24 0C 89 46 34 89 4E 38 FF 05 ? ? ? ? 8B 46 3C 85 C0 75 14 8B 56 10 C1 EA 03 81 E2 ? ? ? ? 52 8B CE");
        static auto ResourceFileBeginLoading = (void(__thiscall*)(void* rsc, int a1, int a2)) pattern.get_first(0); //0x006616F0;
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

            static injector::hook_back<void(__cdecl*)()> hb_662B30;
            auto LoadTPK = []()
            {
                if (std::filesystem::exists(TPKPath))
                {
                    LoadResourceFile(TPKPath.c_str(), 1);
                }
                return hb_662B30.fun();
            };

            pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 56 57 B9 ? ? ? ? E8"); //0x6660B6
            hb_662B30.fun = injector::MakeCALL(pattern.get_first(0), static_cast<void(__cdecl*)()>(LoadTPK), true).get();

            /*
            cursor
            constexpr float cursorScale = 1.0f * (128.0f / 16.0f);
            pattern = hook::pattern("C7 84 24 34 02 00 00 00 00 80 3F D9"); //5704F8
            injector::WriteMemory<float>(pattern.get_first(7), cursorScale, true);
            injector::WriteMemory<float>(pattern.get_first(36), cursorScale, true);
            injector::WriteMemory<float>(pattern.get_first(47), cursorScale, true);
            injector::WriteMemory<float>(pattern.get_first(69), cursorScale, true);
            */
        }

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
                    const std::string& text = (nImproveGamepadSupport != 2) ? TextsXBOX[i] : TextsPS[i];
                    strcpy_s(pszStr, text.length() + 1, text.c_str());
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

        // Start menu text
        uint32_t* dword_5A313D = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 51 E8 ? ? ? ? 83 C4 ? 3B FB").count(1).get(0).get<uint32_t>(1);
        if (nImproveGamepadSupport != 2)
            injector::WriteMemory(dword_5A313D, 0xD18D4C4C, true); //"Press START to begin" (Xbox)
        else
            injector::WriteMemory(dword_5A313D, 0xDC64C04C, true); //"Press START button" (PlayStation)
    }

    if (nWindowedMode)
    {
        uint32_t* dword_6E6D77 = hook::pattern("A1 ? ? ? ? 68 00 00 00 10 6A F0 50").count(1).get(0).get<uint32_t>(0);
        uint32_t* dword_6E6D8A = (uint32_t*)((uint32_t)dword_6E6D77 + 0x13);
        uint32_t* dword_6E6C94 = hook::pattern("FF 15 ? ? ? ? 50 A3 ? ? ? ? FF 15 ? ? ? ? 39 1D ? ? ? ? 75 0E").count(1).get(0).get<uint32_t>(0);
        uint32_t* dword_6E6C3A = hook::pattern("FF 15 ? ? ? ? 6A 0D 6A 0D E8 ? ? ? ? 50 68 04 67 DD 08").count(1).get(0).get<uint32_t>(0);
        uint32_t* dword_982BF0 = *(uint32_t**)((uint32_t)dword_6E6C94 + 0x14);

        // skip SetWindowLong because it messes things up
        injector::MakeJMP(dword_6E6D77, dword_6E6D8A, true);
        // hook the offending functions
        injector::MakeNOP(dword_6E6C94, 6, true);
        injector::MakeCALL(dword_6E6C94, WindowedModeWrapper::CreateWindowExA_Hook, true);
        injector::MakeNOP(dword_6E6C3A, 6, true);
        injector::MakeCALL(dword_6E6C3A, WindowedModeWrapper::AdjustWindowRect_Hook, true);

        *(int*)dword_982BF0 = 1;

        switch (nWindowedMode)
        {
            case 5:
                WindowedModeWrapper::bStretchWindow = true;
                break;
            case 4:
                WindowedModeWrapper::bScaleWindow = true;
                break;
            case 3:
            case 2:
                WindowedModeWrapper::bBorderlessWindowed = false;
                break;
            default:
                break;
        }

        if (nWindowedMode == 3)
        {
            WindowedModeWrapper::bEnableWindowResize = true;

            // dereference the current WndProc from the game executable and write to the function pointer (to maximize compatibility)
            uint32_t* wndproc_addr = hook::pattern("C7 44 24 44 ? ? ? ? 89 5C 24 48 89 5C 24 4C").count(1).get(0).get<uint32_t>(4);
            GameWndProcAddr = *(unsigned int*)wndproc_addr;
            GameWndProc = (LRESULT(WINAPI*)(HWND, UINT, WPARAM, LPARAM))GameWndProcAddr;
            injector::WriteMemory<unsigned int>(wndproc_addr, (unsigned int)&WSFixWndProc, true);

            DWORD oldprotect = 0;
            DrawHUD_57CAA8 = hook::pattern("8B 41 0C BD 01 00 00 00 3B C5").count(1).get(0).get<bool>(4);
            ResetWnd_982C39 = *hook::pattern("A0 ? ? ? ? 84 C0 74 59 E8 ? ? ? ?").count(1).get(0).get<uint32_t*>(1);
            injector::UnprotectMemory(DrawHUD_57CAA8, 1, oldprotect);

            // cave entrypoints
            uint32_t* dword_6E726B = hook::pattern("A0 ? ? ? ? 84 C0 74 59 E8 ? ? ? ?").count(1).get(0).get<uint32_t>(0);
            uint32_t* dword_6E72C6 = hook::pattern("C6 05 ? ? ? ? 00 39 2D ? ? ? ? 89 2D ? ? ? ?").count(1).get(0).get<uint32_t>(0);;
            FastWndReset_Exit_True = ((uint32_t)dword_6E726B) + 0x22;
            FastWndReset_Exit_False = ((uint32_t)dword_6E726B) + 0x7;
            FastWndReset_Finish_Exit = ((uint32_t)dword_6E72C6) + 0x7;

            injector::MakeJMP(dword_6E726B, FastWndReset_Cave, true);
            injector::MakeJMP(dword_6E72C6, FastWndReset_Finish_Cave, true);
        }
    }

    if (fLeftStickDeadzone)
    {
        // DInput [ 0 32767 | 32768 65535 ] 
        fLeftStickDeadzone /= 200.0f;

        auto pattern = hook::pattern("89 86 34 02 00 00 8D 45 D4"); //0x7F29B2
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

        // this shouldn't be necessary - if the game frametime/rate is matched with the sim frametime/rate, then everything is fine.
        // limit rate to avoid issues...
        //if (SimRate > 360)
        //    SimRate = 360;
        //if (SimRate < 60)
        //    SimRate = 60;

        static float FrameTime = 1.0f / SimRate;
        //static float fnFPSLimit = (float)SimRate;

        // Frame times
        // PrepareRealTimestep() NTSC video mode frametime .rdata
        uint32_t* dword_6612EC = hook::pattern("D9 05 ? ? ? ? B9 ? ? ? ? D8 44 24 14 D9 5C 24 14").count(1).get(0).get<uint32_t>(53); //0x006612B7 anchor
        // MainLoop frametime .text
        float* flt_666100 = hook::pattern("E8 ? ? ? ? 68 89 88 88 3C").count(1).get(0).get<float>(6);
        // World_Service UglyTimestepHack initial state .data
        float* flt_903290 = *hook::pattern("8B 0D ? ? ? ? 85 C9 C7 05 ? ? ? ? 00 00 00 00 74 05").count(1).get(0).get<float*>(10); //0x0075AAD8 dereference
        // GetDebugRealTime() NTSC frametime 1 .text
        uint32_t* dword_65C78F = hook::pattern("83 EC 08 A1 ? ? ? ? 89 44 24 04 A1 ? ? ? ? 85 C0 75 08").count(1).get(0).get<uint32_t>(0x1F); //0x0065C770 anchor
        // GetDebugRealTime() NTSC frametime 2 .text
        uint32_t* dword_65C7D9 = hook::pattern("83 EC 08 A1 ? ? ? ? 89 44 24 04 A1 ? ? ? ? 85 C0 75 08").count(1).get(0).get<uint32_t>(0x69); //0x0065C770 anchor

        injector::WriteMemory(dword_6612EC, &FrameTime, true);
        //injector::WriteMemory(flt_8970F0, FrameTime, true);
        injector::WriteMemory(flt_666100, FrameTime, true);
        *flt_903290 = FrameTime;
        injector::WriteMemory(dword_65C78F, &FrameTime, true);
        injector::WriteMemory(dword_65C7D9, &FrameTime, true);

        // Frame rates
        // TODO: if any issues arise, figure out where 60.0 values are used and update the constants...
    }

    if (bFixResolutionText)
    {
        uintptr_t loc_51B81F = reinterpret_cast<uintptr_t>(hook::pattern("B8 12 7B 66 76 8B 77 30 85").get_first(0)) + 0xC;

        // Video Options Resolution text
        static std::wstring CurrResString;
        struct ResolutionTextFix
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uint32_t*)(regs.esi + 0x1C) |= 0x400000;
                *(uint32_t*)(regs.esi + 0x60) = 0;

                CurrResString = std::format(L"{}x{}", Screen.Width, Screen.Height);
                *(wchar_t**)(regs.esi + 0x64) = CurrResString.data();   // or .c_str()
            }
        }; injector::MakeInline<ResolutionTextFix>(loc_51B81F, loc_51B81F + 0xA);

        uintptr_t loc_5297B0 = reinterpret_cast<uintptr_t>(hook::pattern("8B F0 83 C4 04 89 74 24 10 85 F6 C7 44 24 20 01 00 00 00 74 11 6A 01").get_first(0)) + 0x1E;
        uintptr_t VOptionsVTable = *reinterpret_cast<uintptr_t*>(loc_5297B0 + 2);
        uintptr_t loc_528430 = *reinterpret_cast<uintptr_t*>(VOptionsVTable);
        uintptr_t loc_528431 = loc_528430 + 1;
        uintptr_t loc_528433 = loc_528430 + 3;

        static uintptr_t VOResFreeAddr = static_cast<uintptr_t>(injector::GetBranchDestination(loc_528433));

        struct ResolutionTextFreeHook
        {
            void operator()(injector::reg_pack& regs)
            {
                // [obj + 0x30] = FEString
                uintptr_t FEString = *(uintptr_t*)(regs.ecx + 0x30);
                *(wchar_t**)(FEString + 0x64) = nullptr; // null the string so it doesn't get freed by the game!

                regs.esi = regs.ecx;

                reinterpret_cast<void(__thiscall*)(uintptr_t)>(VOResFreeAddr)(regs.ecx);
            }
        }; injector::MakeInline<ResolutionTextFreeHook>(loc_528431, loc_528431 + 7);
    }

    if (bWriteSettingsToFile)
    {
        std::filesystem::path SettingsSavePath;
        if (!szCustomUserFilesDirectoryInGameDir.empty())
            SettingsSavePath = CustomUserDir;

        auto GetFolderPathpattern = hook::pattern("50 6A 00 6A 00 68 ? 80 00 00 6A 00");
        uintptr_t GetFolderPathCallDest = injector::GetBranchDestination(GetFolderPathpattern.count(1).get(0).get<uintptr_t>(14), true).as_int();

        if (GetFolderPathCallDest && szCustomUserFilesDirectoryInGameDir.empty())
        {
            char szSettingsSavePath[MAX_PATH];
            injector::stdcall<HRESULT(HWND, int, HANDLE, DWORD, LPSTR)>::call(GetFolderPathCallDest, NULL, 0x8005, NULL, NULL, szSettingsSavePath);
            SettingsSavePath = szSettingsSavePath;
        }

        SettingsSavePath.append("NFS Most Wanted");
        SettingsSavePath.append("Settings.ini");

        if (GetFolderPathCallDest || !szCustomUserFilesDirectoryInGameDir.empty())
        {
            RegistryWrapper("Need for Speed", SettingsSavePath);
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
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("C7 00 80 02 00 00 C7 01 E0 01 00 00 C2 08 00"));
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

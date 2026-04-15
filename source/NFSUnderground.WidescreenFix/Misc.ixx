module;

#include <stdafx.h>
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>
#include "usercall.hpp"

export module Misc;

import ComVars;

SafetyHookInline shsub_4F3030 = {};
void __cdecl sub_4F3030(int a1, int a2)
{
    if (!bHideCursorForMouseLook)
        return shsub_4F3030.unsafe_ccall(a1, a2);
}

void* (*CreateResourceFile)(const char* ResourceFileName, int32_t ResourceFileType, int, int, int);
void(__fastcall* ResourceFileBeginLoading)(int a1, void* rsc, int a2);
void LoadResourceFile(const char* ResourceFileName, int32_t ResourceFileType, int32_t nUnk1 = 0, int32_t nUnk2 = 0, int32_t nUnk3 = 0, int32_t nUnk4 = 0, int32_t nUnk5 = 0)
{
    auto r = CreateResourceFile(ResourceFileName, ResourceFileType, nUnk1, nUnk2, nUnk3);
    usercall::Call<usercall::reg::eax, usercall::reg::ecx, usercall::reg::edx>(ResourceFileBeginLoading, nUnk4, nUnk5, r);
}

namespace IntroFMVScreen
{
    SafetyHookInline shNotificationMessage = {};
    void __fastcall NotificationMessage(void* FEPackage, void* edx, int a2, void* FEObject, int a4, int a5)
    {
        // pop the package once for the intro skip
        static bool bOnceFlag = false;
        if (!bOnceFlag)
        {
            bOnceFlag = true;
            cFEng::PopPackage(*(char**)((uintptr_t)FEPackage + 0xC));
            return;
        }

        return shNotificationMessage.unsafe_fastcall(FEPackage, edx, a2, FEObject, a4, a5);
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEventAsync() += []()
        {
            CIniReader iniReader("");
            bool bSkipIntro = iniReader.ReadInteger("MISC", "SkipIntro", 0) != 0;
            bool bShowLangSelect = iniReader.ReadInteger("MISC", "ShowLangSelect", 0) != 0;

            static int nImproveGamepadSupport = iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0);
            static float fLeftStickDeadzone = iniReader.ReadFloat("MISC", "LeftStickDeadzone", 10.0f);
            static float fRightStickDeadzone = std::clamp(iniReader.ReadFloat("MISC", "RightStickDeadzone", 10.0f), 0.0f, 100.0f);
            uint32_t nForcedAudioSampleRate = iniReader.ReadInteger("MISC", "ForcedAudioSampleRate", 44100);
            static int nFPSLimit = iniReader.ReadInteger("MISC", "FPSLimit", -1);

            WFP::onGameInitEvent() += []()
            {
                SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)CreateIconFromResourceICO(IDR_NFSUICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON)));
                SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)CreateIconFromResourceICO(IDR_NFSUICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON)));

            };

            if (bSkipIntro)
            {
                // patch bootflow

                // LS_IntroFMV.fng NEEDS to be there in order to hear the splash screen music!
                // This means that the intro video technically still plays, which means e3_title.mad still needs to be in the movies folder
                static const char* BootFlowScreens[] = { "LS_IntroFMV.fng", "LS_Splash_PC.fng", "MU_UG_NEwOrLoad_PC2.fng" , "BootFlowEnd" };
                static const char* BootFlowScreensLangSelect[] = { "LS_LangSelect.fng", "LS_IntroFMV.fng", "LS_Splash_PC.fng", "MU_UG_NEwOrLoad_PC2.fng" , "BootFlowEnd" };

                const char** outScreens = BootFlowScreens;
                const char** outLastScreen = &BootFlowScreens[_countof(BootFlowScreens) - 1];
                if (bShowLangSelect)
                {
                    outScreens = BootFlowScreensLangSelect;
                    outLastScreen = &BootFlowScreensLangSelect[_countof(BootFlowScreensLangSelect) - 1];
                }

                uintptr_t loc_4DE21A = reinterpret_cast<uintptr_t>(hook::pattern("89 36 89 76 04 A1 ? ? ? ? 83 F8 05").get_first(0)) + 0x15;
                uintptr_t loc_4DE273 = loc_4DE21A + 0x59;
                uintptr_t loc_4DE2A0 = reinterpret_cast<uintptr_t>(hook::pattern("8B 4E 04 89 01 83 C7 04 81 FF ? ? ? ? 89 46 04 89 48 04 89 30 7C C8 8B 36").get_first(0)) + 8;

                // force US Retail bootflow
                injector::MakeJMP(loc_4DE21A, loc_4DE273);

                // list start
                injector::WriteMemory<const char**>(loc_4DE273 + 1, outScreens, true);

                // list end
                injector::WriteMemory<const char**>(loc_4DE2A0 + 2, outLastScreen, true);

                // IntroFMV NotificationMessage Hook -- for skipping the video for the first time it plays
                auto pattern = hook::pattern("8B 44 24 ? 3D ? ? ? ? 56 74");
                IntroFMVScreen::shNotificationMessage = safetyhook::create_inline(pattern.get_first(0), IntroFMVScreen::NotificationMessage);
            }
            else if (bShowLangSelect)
            {
                static const char* BootFlowScreensLangSelect[] = { "LS_LangSelect.fng", "LS_EAlogo.fng", "LS_PSAMovie.fng", "LS_IntroFMV.fng", "LS_Splash_PC.fng", "MU_UG_NEwOrLoad_PC2.fng" , "BootFlowEnd" };

                uintptr_t loc_4DE21A = reinterpret_cast<uintptr_t>(hook::pattern("89 36 89 76 04 A1 ? ? ? ? 83 F8 05").get_first(0)) + 0x15;
                uintptr_t loc_4DE273 = loc_4DE21A + 0x59;
                uintptr_t loc_4DE2A0 = reinterpret_cast<uintptr_t>(hook::pattern("8B 4E 04 89 01 83 C7 04 81 FF ? ? ? ? 89 46 04 89 48 04 89 30 7C C8 8B 36").get_first(0)) + 8;

                // force US Retail bootflow
                injector::MakeJMP(loc_4DE21A, loc_4DE273);

                // list start
                injector::WriteMemory<const char**>(loc_4DE273 + 1, BootFlowScreensLangSelect, true);

                // list end
                injector::WriteMemory<const char**>(loc_4DE2A0 + 2, &BootFlowScreensLangSelect[_countof(BootFlowScreensLangSelect) - 1], true);
            }

            if (nImproveGamepadSupport)
            {
                auto pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 A1 ? ? ? ? 57");
                CreateResourceFile = (void* (*)(const char* ResourceFileName, int32_t ResourceFileType, int, int, int)) pattern.get_first(0); //0x004482F0
                pattern = hook::pattern("56 8B F2 89 8E 98 00 00 00 8B 0D ? ? ? ? 89 86 94 00 00 00 8B 86 9C 00 00 00");
                ResourceFileBeginLoading = (void(__fastcall*)(int a1, void* rsc, int a2)) pattern.get_first(0); //0x00448110;

                if (nImproveGamepadSupport < 3)
                {
                    static auto TPKPath = GetThisModulePath<std::string>().substr(GetExeModulePath<std::string>().length());

                    if (nImproveGamepadSupport == 1)
                        TPKPath += "buttons-xbox.tpk";
                    else if (nImproveGamepadSupport == 2)
                        TPKPath += "buttons-playstation.tpk";

                    static injector::hook_back<void(__cdecl*)()> hb_448600;
                    auto LoadTPK = []()
                    {
                        if (std::filesystem::exists(TPKPath))
                        {
                            LoadResourceFile(TPKPath.c_str(), 1);
                        }
                        return hb_448600.fun();
                    };

                    pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 6A 00 6A 00 6A 00 68 F0 59 00 00"); //0x447280
                    hb_448600.fun = injector::MakeCALL(pattern.get_first(0), static_cast<void(__cdecl*)()>(LoadTPK), true).get();
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

                static bool Zstate, Pstate, Tstate, Dstate, Qstate, Cstate;
                pattern = hook::pattern("7C ? 5F 5D 5E 33 C0 5B C2 08 00"); //0x41989E
                injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(8 + 5), 0x900008C2, true);
                //static uintptr_t ButtonsState = (uintptr_t)*hook::pattern("").count(1).get(0).get<uint32_t*>(8); //0x
                struct CatchPad
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        PadState* PadKeyPresses = *(PadState**)(regs.esp + 0x4);
                        //Keyboard
                        //006F9358 backspace
                        //006F931C enter
                        if (PadKeyPresses != nullptr && PadKeyPresses != (PadState*)0x1 && nGameState == 3)
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

                            if (PadKeyPresses->Y)
                            {
                                if (!Zstate)
                                {
                                    keybd_event(BYTE(VkKeyScan('Z')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                                    keybd_event(BYTE(VkKeyScan('Z')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                                }
                                Zstate = true;
                            }
                            else
                                Zstate = false;

                            if (PadKeyPresses->X)
                            {
                                if (!Pstate)
                                {
                                    keybd_event(BYTE(VkKeyScan('P')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                                    keybd_event(BYTE(VkKeyScan('P')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                                }
                                Pstate = true;
                            }
                            else
                                Pstate = false;

                            if (PadKeyPresses->LSClick && !PadKeyPresses->RSClick)
                            {
                                if (!Tstate)
                                {
                                    keybd_event(BYTE(VkKeyScan('T')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                                    keybd_event(BYTE(VkKeyScan('T')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                                }
                                Tstate = true;
                            }
                            else
                                Tstate = false;

                            if (PadKeyPresses->RSClick && !PadKeyPresses->LSClick)
                            {
                                if (!Dstate)
                                {
                                    keybd_event(BYTE(VkKeyScan('D')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                                    keybd_event(BYTE(VkKeyScan('D')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                                }
                                Dstate = true;
                            }
                            else
                                Dstate = false;

                            if (PadKeyPresses->Select)
                            {
                                if (!Cstate)
                                {
                                    keybd_event(BYTE(VkKeyScan('C')), 0, KEYEVENTF_EXTENDEDKEY, 0);
                                    keybd_event(BYTE(VkKeyScan('C')), 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
                                }
                                Cstate = true;
                            }
                            else
                                Cstate = false;
                        }
                    }
                }; injector::MakeInline<CatchPad>(pattern.get_first(8));

                const wchar_t* ControlsTexts[] = { L" 0", L" 1", L" 2", L" 3", L" 4", L" 5", L" 6", L" 7", L" 8", L" 9", L" Up", L" Down", L" Left", L" Right", L"X Rotation", L"Y Rotation", L"X Axis", L"Y Axis", L"Z Axis", L"Hat Switch" };
                const wchar_t* ControlsTextsXBOX[] = { L"A", L"B", L"X", L"Y", L"LB", L"RB", L"View (Select)", L"Menu (Start)", L"Left stick", L"Right stick", L"D-pad Up", L"D-pad Down", L"D-pad Left", L"D-pad Right", L"Right stick Left/Right", L"Right stick Up/Down", L"Left stick Left/Right", L"Left stick Up/Down", L"LT / RT", L"D-pad" };
                const wchar_t* ControlsTextsPS[] = { L"Cross", L"Circle", L"Square", L"Triangle", L"L1", L"R1", L"Select", L"Start", L"L3", L"R3", L"D-pad Up", L"D-pad Down", L"D-pad Left", L"D-pad Right", L"Right stick Left/Right", L"Right stick Up/Down", L"Left stick Left/Right", L"Left stick Up/Down", L"L2 / R2", L"D-pad" };

                static std::vector<std::wstring> Texts(ControlsTexts, std::end(ControlsTexts));
                static std::vector<std::wstring> TextsXBOX(ControlsTextsXBOX, std::end(ControlsTextsXBOX));
                static std::vector<std::wstring> TextsPS(ControlsTextsPS, std::end(ControlsTextsPS));

                pattern = hook::pattern("8D 43 60 8D 74 24 10 E8 ? ? ? ? 8B"); //0x4148F6
                struct Buttons
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.eax = regs.ebx + 0x60;
                        regs.esi = regs.esp + 0x10;

                        auto pszStr = (wchar_t*)(regs.esp + 0x10);
                        auto it = std::find_if(Texts.begin(), Texts.end(), [&](const std::wstring& str) { std::wstring s(pszStr); return s.find(str) != std::wstring::npos; });
                        auto i = std::distance(Texts.begin(), it);

                        if (it != Texts.end())
                        {
                            const std::wstring& text = (nImproveGamepadSupport != 2) ? TextsXBOX[i] : TextsPS[i];
                            wcscpy_s(pszStr, text.length() + 1, text.c_str());
                        }
                    }
                }; if (pattern.size() > 0) { injector::MakeInline<Buttons>(pattern.get_first(0), pattern.get_first(7)); }

                // FrontEnd button remap (through game code, not key emulation)
                pattern = hook::pattern("8B 86 ? ? ? ? 8B 0C ? ? ? ? ? 85 C9 0F"); // 004071C5
                struct FrontEndRemap
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.eax = *(uintptr_t*)(regs.esi + 0x0130);

                        int LB = (regs.esi + 0x0130) - (0x88);
                        *(uintptr_t*)LB = 0x1A; // FE Action "Comma"
                        int RB = (regs.esi + 0x0130) - (0x84);
                        *(uintptr_t*)RB = 0x19; // FE Action "Period"
                    }
                };
                injector::MakeInline<FrontEndRemap>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
            }

            {
                static auto dword_71D8A8 = *hook::get_pattern<int32_t*>("8D 95 ? ? ? ? 8B F2", 2);

                auto pattern = hook::pattern("8B F0 85 F6 7D ? 8B 7C 24");
                static auto JoyStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    DIJOYSTATE* state = (DIJOYSTATE*)(&dword_71D8A8[regs.ebp]);

                    // [ -10000 | 10000 ]

                    int32_t nLeftStickDeadzone = static_cast<int32_t>(fLeftStickDeadzone * 100.0f);
                    int32_t nRightStickDeadzone = static_cast<int32_t>(fRightStickDeadzone * 100.0f);

                    state->lX = (std::abs(state->lX) <= nLeftStickDeadzone) ? 0 : state->lX;
                    state->lY = (std::abs(state->lY) <= nLeftStickDeadzone) ? 0 : state->lY;

                    state->lRx = (std::abs(state->lRx) <= nRightStickDeadzone) ? 0 : state->lRx;
                    state->lRy = (std::abs(state->lRy) <= nRightStickDeadzone) ? 0 : state->lRy;

                    g_RightStick.RawX = state->lRx;
                    g_RightStick.RawY = state->lRy;

                    double rx = (double)state->lRx / 10000.0;
                    g_RightStick.X = (float)rx;

                    double ry = (double)state->lRy / 10000.0;
                    g_RightStick.Y = (float)ry;
                });

                pattern = hook::pattern("8B 44 24 ? 8B 4C 24 ? 8B 53");
                static auto MouseStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (hWnd != GetForegroundWindow())
                        return;

                    MouseData* state = (MouseData*)(regs.esp + 0x4);
                    g_Mouse.DeltaX = state->DeltaX;
                    g_Mouse.DeltaY = state->DeltaY;
                    g_Mouse.Wheel = state->Wheel;
                });

                pattern = hook::pattern("55 8B EC 83 E4 ? 81 EC ? ? ? ? A1 ? ? ? ? 53 8B 5D");
                shsub_4F3030 = safetyhook::create_inline(pattern.get_first(), sub_4F3030);
            }

            if (nForcedAudioSampleRate)
            {
                uintptr_t loc_532B15 = reinterpret_cast<uintptr_t>(hook::pattern("C7 05 ? ? ? ? 22 56 00 00 A1 ? ? ? ? 85 C0").get_first(0)) + 0xA;
                uintptr_t dword_735610 = *(uintptr_t*)(loc_532B15 + 1);
                *(uint32_t*)dword_735610 = nForcedAudioSampleRate;
            }

            //__mbclen to strlen, "---" bug fix
            auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 85 C0 76 21 8D 43 60");
            if (!pattern.empty())
            {
                injector::MakeCALL(pattern.get_first(0), strlen, true);
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

                // the game limits FPS 2x over the frametime (or just reports it that way to D3D) -- this is the real game framerate here!
                static float FrameTime = 1.0f / nFPSLimit;
                // Video mode frametime
                uint32_t* dword_6CC7B0 = *hook::pattern("83 EC 10 A1 ? ? ? ? 89 44 24 04").count(1).get(0).get<uint32_t*>(31);
                injector::WriteMemory(dword_6CC7B0, FrameTime, true);
                // RealTimestep frametime
                uint32_t* dword_6F0890 = *hook::pattern("99 D9 05 ? ? ? ? B9 64 00 00 00").count(1).get(0).get<uint32_t*>(3);
                injector::WriteMemory(dword_6F0890, FrameTime, true);
                uint32_t* dword_6CCDEC = *hook::pattern("99 D9 05 ? ? ? ? B9 64 00 00 00").count(1).get(0).get<uint32_t*>(63);
                injector::WriteMemory(dword_6CCDEC, FrameTime * 2.0f, true);
                // a function in eDisplayFrame (particle effects?) frametime
                uint32_t* dword_40A744 = hook::pattern("68 89 88 88 3C").count(1).get(0).get<uint32_t>(1);
                injector::WriteMemory(dword_40A744, FrameTime, true);
                // something related to framerate and/or seconds. This value has to be an integer multiple of 60, otherwise the game can freeze. This affects some gameplay features such as NOS and menus.
                uint32_t* dword_6CC8B0 = *hook::pattern("83 E1 01 0B F9 D9 44 24 28 D8 1D ? ? ? ?").count(1).get(0).get<uint32_t*>(11);
                static float FrameSeconds = static_cast<float>(nFPSLimit);

                if (nFPSLimit % 60)
                    FrameSeconds = static_cast<float>(nFPSLimit - (nFPSLimit % 60));

                // needed to avoid crashes, anything above 120 seems to be problematic at the moment...
                if (FrameSeconds > 120.0f)
                    FrameSeconds = 120.0f;

                if (FrameSeconds < 60.0)
                    FrameSeconds = 60.0;
                injector::WriteMemory(dword_6CC8B0, FrameSeconds, true);
                // another frametime -- seems to affect some gameplay elements...
                uint32_t* dword_6B5C08 = *hook::pattern("DF E0 F6 C4 41 7A ? 8A 44 24 12 D9 05 ? ? ? ?").count(1).get(0).get<uint32_t*>(13);
                injector::WriteMemory(dword_6B5C08, FrameTime, true);

                // GAME BUGFIX: fix sticky steering (especially on high FPS)
                // kill the autocentering!
                uint32_t* dword_460A3D = hook::pattern("D9 54 24 14 D9 E1 D8 1D ? ? ? ? DF E0 F6 C4 05 7A 08").count(1).get(0).get<uint32_t>(0x11); //0x460A2C anchor
                injector::WriteMemory<uint8_t>(dword_460A3D, 0xEB, true);

                // explanation:
                // this is a native game bug which can sadly happen on a bone-stock game, it's just been exaggerated by the higher FPS and faster input
                // what happens is this: if the steering curve isn't finished with the steering (if the wheels aren't centered), it will outright ignore the other direction you're pressing
                // to exaggerate the issue even further: nop out the instruction at 00460DFA which will make the steering extremely slow, then try steering all the way to the left, then quickly go and hold right. Notice how it *won't* continue to go right.
                // so to summarize in a chain of events:
                // 1. game controller sets steering value (-1.0 to 1.0)
                // 2. World::DoTimestep updates the player info and somewhere
                // 3. Somewhere in that chain it calls PlayerSteering::DoUndergroundSteering with the steering value as argument
                // 4. Steering curves get processed at PlayerSteering::CalculateSteeringSpeed                                          <-- THIS IS WHERE THE BUG HAPPENS (or shortly thereafter)
                // 5. Steering value gets passed to the car
                // Skipping 4. is possible by enabling the bool at 00736514, but that is intended exclusively for wheel input and not gamepad/keyboard input.
            }

            // Disable the best lap time counter at the end of the race for better ultrawide support
            // (this element is not visible during 16:9 play)
            auto loc_49A222 = hook::pattern("8A 44 24 24 8B 6C 24 68 8B 7D 0C 83 CE FF 84 C0 8D 4C 24 24 74 ? 6B F6 21");
            pattern = hook::pattern("0F 84 B8 00 00 00 8D 4C 24 44");
            injector::MakeNOP(pattern.get_first(0), 6, true);
            injector::MakeJMP(pattern.get_first(0), loc_49A222.get_first(0), true);

            {
                auto pattern = hook::pattern("E8 ? ? ? ? 8B 2D ? ? ? ? BF");
                static auto EAXSoundUpdateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    *(float*)(regs.esp + 0x0) = std::clamp(*(float*)(regs.esp + 0x0), 1.0f / 60.0f, std::numeric_limits<float>::max());
                });
            }
        };
    }
} Misc;
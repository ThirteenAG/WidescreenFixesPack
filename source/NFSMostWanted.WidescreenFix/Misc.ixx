module;

#include <stdafx.h>
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

export module Misc;

import ComVars;

SafetyHookInline shsub_56FDB0 = {};
void __cdecl sub_56FDB0(int a1, int a2)
{
    if (!bHideCursorForMouseLook)
        return shsub_56FDB0.unsafe_ccall(a1, a2);
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
            static int nImproveGamepadSupport = iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0);
            bool bForceHighSpecAudio = iniReader.ReadInteger("MISC", "ForceHighSpecAudio", 1) != 0;
            static float fLeftStickDeadzone = iniReader.ReadFloat("MISC", "LeftStickDeadzone", 10.0f);
            static float fRightStickDeadzone = iniReader.ReadFloat("MISC", "RightStickDeadzone", 10.0f);
            SimRate = iniReader.ReadInteger("MISC", "SimRate", -1);

            if (bForceHighSpecAudio)
            {
                uint32_t* dword_4C41F5 = hook::pattern("C7 05 ? ? ? ? ? ? 00 00 A1 ? ? ? ? 85 C0 ? ? A3 ? ? ? ? 8D 44 24 08").count(1).get(0).get<uint32_t>(6);
                injector::WriteMemory<int>(dword_4C41F5, 44100, true);
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
                        _asm {fmul dword ptr[f0078125]}

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

            {
                auto pattern = hook::pattern("8B 85 ? ? ? ? 3B 86");
                static auto JoyStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    DIJOYSTATE* state = (DIJOYSTATE*)(regs.ebp - 0x13C);

                    g_RightStick.RawX = state->lRx;
                    g_RightStick.RawY = state->lRy;

                    float dz = (fRightStickDeadzone > 0.0f) ? fRightStickDeadzone / 200.0f : 0.0f;

                    double rx = ((double)state->lRx - 32767.5) / 32767.5;
                    if (std::abs(rx) <= dz) rx = 0.0;
                    g_RightStick.X = (float)rx;

                    double ry = ((double)state->lRy - 32767.5) / 32767.5;
                    if (std::abs(ry) <= dz) ry = 0.0;
                    g_RightStick.Y = (float)ry;
                });

                pattern = hook::pattern("8B 45 ? 3B C7 75 ? 3B D7");
                static auto MouseStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    MouseData* state = (MouseData*)(regs.ebp - 0x14);
                    g_Mouse.DeltaX = state->DeltaX;
                    g_Mouse.DeltaY = state->DeltaY;
                    g_Mouse.Wheel = state->Wheel;
                });

                pattern = hook::pattern("55 8B EC 83 E4 ? 81 EC ? ? ? ? A0 ? ? ? ? 84 C0 53 56 57");
                shsub_56FDB0 = safetyhook::create_inline(pattern.get_first(), sub_56FDB0);
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
        };
    }
} Misc;
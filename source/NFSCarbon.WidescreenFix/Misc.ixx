module;

#include <stdafx.h>
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>

export module Misc;

import ComVars;

SafetyHookInline shsub_585290 = {};
void __cdecl sub_585290(int a1, int a2)
{
    if (!bHideCursorForMouseLook || GameFlowManager::IsPaused())
        return shsub_585290.unsafe_ccall(a1, a2);
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
            bool bExperimentalCrashFix = iniReader.ReadInteger("MISC", "CrashFix", 1) != 0;
            static int32_t nImproveGamepadSupport = iniReader.ReadInteger("MISC", "ImproveGamepadSupport", 0);
            bool bExpandControllerOptions = iniReader.ReadInteger("MISC", "ExpandControllerOptions", 0) != 0;
            static float fLeftStickDeadzone = iniReader.ReadFloat("MISC", "LeftStickDeadzone", 10.0f);
            static float fRightStickDeadzone = iniReader.ReadFloat("MISC", "RightStickDeadzone", 10.0f);
            SimRate = iniReader.ReadInteger("MISC", "SimRate", -1);
            bool bCarShadowFix = iniReader.ReadInteger("GRAPHICS", "CarShadowFix", 1) != 0;

            if (bCarShadowFix)
            {
                uint32_t* dword_7BEA3A = hook::pattern("D8 05 ? ? ? ? DC C0 E8 ? ? ? ? 85 C0 7F 04 33 C0").count(1).get(0).get<uint32_t>(2);
                static float f60 = 60.0f;
                injector::WriteMemory(dword_7BEA3A, &f60, true);
            }

            if (bSkipIntro)
            {
                static auto counter = 0;
                static auto og_value = 0;
                auto pattern = hook::pattern("8B 0D ? ? ? ? 53 33 DB 3B CB ? ? 8B 47 04");
                static uint32_t* dword_A9E6D8 = *pattern.get_first<uint32_t*>(2);
                struct SkipIntroHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        if (counter < 3)
                        {
                            if (counter == 0)
                                og_value = *dword_A9E6D8;
                            *dword_A9E6D8 = 1;
                            counter++;
                        }
                        else
                        {
                            *dword_A9E6D8 = og_value;
                        }

                        regs.ecx = *(uint32_t*)dword_A9E6D8;
                    }
                }; injector::MakeInline<SkipIntroHook>(pattern.get_first(0), pattern.get_first(6));;
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
                auto pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 51 A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 04 89 04 24 85 C0 C7 44 24 ? ? ? ? ? 74 22 8B 4C 24 24 8B 54 24 20 51");
                static auto CreateResourceFile = (void* (*)(const char* ResourceFileName, int32_t ResourceFileType, int, int, int)) pattern.get_first(0); //0x006B32C0
                pattern = hook::pattern("8B 44 24 04 56 8B F1 8B 4C 24 0C 89 46 34 89 4E 38 FF 05 ? ? ? ? 8B 46 3C 85 C0 75 14 8B 56 10 C1 EA 03 81 E2 ? ? ? ? 52 8B CE");
                static auto ResourceFileBeginLoading = (void(__thiscall*)(void* rsc, int a1, int a2)) pattern.get_first(0); //0x006B5910;
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

                    static injector::hook_back<void(__cdecl*)()> hb_6B6D40;
                    auto LoadTPK = []()
                    {
                        LoadResourceFile(TPKPath.c_str(), 1);
                        return hb_6B6D40.fun();
                    };

                    pattern = hook::pattern("E8 ? ? ? ? 8B 35 ? ? ? ? 6A 04 FF D6 83 C4 04 85 C0 74 08 C7 00"); //0x6B7736
                    hb_6B6D40.fun = injector::MakeCALL(pattern.get_first(0), static_cast<void(__cdecl*)()>(LoadTPK), true).get();

                    /*
                    //cursor
                    constexpr float cursorScale = 1.0f * (128.0f / 16.0f);
                    pattern = hook::pattern("C7 44 24 74 00 00 80 3F D9 5C 24 60"); //585424
                    injector::WriteMemory<float>(pattern.get_first(4), cursorScale, true);
                    injector::WriteMemory<float>(pattern.get_first(24), cursorScale, true);
                    injector::WriteMemory<float>(pattern.get_first(35), cursorScale, true);
                    injector::WriteMemory<float>(pattern.get_first(57), cursorScale, true);
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
                            const std::string& text = (nImproveGamepadSupport != 2) ? TextsXBOX[i] : TextsPS[i];
                            strcpy_s(pszStr, text.length() + 1, text.c_str());
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

                // Start menu text
                uint32_t* dword_8577AC = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 51 E8 ? ? ? ? 83 C4 14 E8").count(1).get(0).get<uint32_t>(1);
                if (nImproveGamepadSupport != 2)
                    injector::WriteMemory(dword_8577AC, 0x186AAECC, true); //"Press START to begin" (Xbox)
                else
                    injector::WriteMemory(dword_8577AC, 0x703A92CC, true); //"Press START button" (PlayStation)
            }

            if (bExpandControllerOptions)
            {
                uint32_t* dword_692539 = hook::pattern("B8 14 00 00 00 C2 08 00 B8").count(1).get(0).get<uint32_t>(1);
                injector::WriteMemory((uint32_t)dword_692539, 0x1D, true); //lists all 29 options in the controller config menu
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

                pattern = hook::pattern("8B 45 ? 3B C7 75");
                static auto MouseStateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    MouseData* state = (MouseData*)(regs.ebp - 0x14);
                    g_Mouse.DeltaX = state->DeltaX;
                    g_Mouse.DeltaY = state->DeltaY;
                    g_Mouse.Wheel = state->Wheel;
                });

                pattern = hook::pattern("55 8B EC 83 E4 ? 81 EC ? ? ? ? A0 ? ? ? ? 84 C0 53 56 57 C6 44 24");
                shsub_585290 = safetyhook::create_inline(pattern.get_first(), sub_585290);
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
                static float fSimRate = (float)SimRate;

                // Frame times
                // PrepareRealTimestep() NTSC video mode frametime, .rdata
                //float* flt_9CBC14 = *hook::pattern("D9 05 ? ? ? ? B9 ? ? ? ? D8 44 24 14 D9 5C 24 14").count(1).get(0).get<float*>(53); //0x006B4CFB
                uint32_t* dword_6B4D30 = hook::pattern("D9 05 ? ? ? ? B9 ? ? ? ? D8 44 24 14 D9 5C 24 14").count(1).get(0).get<uint32_t>(53); //0x006B4CFB
                // MainLoop frametime (FPS lock) .text
                float* flt_6B79E3 = hook::pattern("C7 44 24 1C 89 88 88 3C").count(1).get(0).get<float>(4); //0x6B79DF
                // FullSpeedMode frametime (10x speed) .text
                float* flt_6B79F4 = hook::pattern("C7 44 24 1C AB AA 2A 3E").count(1).get(0).get<float>(4); //0x6B79F0
                // Unknown frametime 1 .text
                float* flt_764A42 = hook::pattern("C7 46 14 89 88 88 3C").count(1).get(0).get<float>(3); //0x00764A3F
                // Unknown frametime 2 .text
                float* flt_76AD08 = hook::pattern("68 89 88 88 3C").count(1).get(0).get<float>(1); //0x76AD07
                // ESimTask frametime .rdata
                float* flt_9DB360 = *hook::pattern("83 C4 04 D9 46 14 42 D8 05 ? ? ? ?").count(1).get(0).get<float*>(9); //0x672EFB
                // Unknown frametime 3 .rdata (in some data structure, these values may need to be scaled up from 60.0 accordingly)
                //float* flt_9E24F0 = hook::pattern("00 00 70 42 89 88 88 3C 00 00 20 40 00 00 E0 40").count(1).get(0).get<float>(4); //0x9E24EC
                // Sim::QueueEvents frametime .text (this affects gameplay smoothness noticeably)
                float* flt_9EBB6C = *hook::pattern("D9 46 1C 8B 46 24 83 F8 03 D8 0D ? ? ? ?").count(1).get(0).get<float*>(11); //0x0076AD57

                //injector::WriteMemory(flt_9CBC14, FrameTime, true);
                injector::WriteMemory(dword_6B4D30, &FrameTime, true);
                injector::WriteMemory(flt_6B79E3, FrameTime, true);
                injector::WriteMemory(flt_6B79F4, FrameTime * 10.0f, true);
                injector::WriteMemory(flt_764A42, FrameTime, true);
                injector::WriteMemory(flt_76AD08, FrameTime, true);
                injector::WriteMemory(flt_9DB360, FrameTime, true);
                //injector::WriteMemory(flt_9E24F0, FrameTime, true);
                injector::WriteMemory(flt_9EBB6C, FrameTime, true);

                // Frame rates
                // Unknown framerate 1 .rdata (in some data structure, these values may need to be scaled up from 60.0 accordingly)
                //float* flt_9E24EC = hook::pattern("00 00 70 42 89 88 88 3C").count(1).get(0).get<float>(0); //0x9E24EC
                // Unknown framerate 2 .rdata (in some data structure, these values may need to be scaled up from 60.0 accordingly)
                //float* flt_9E2500 = hook::pattern("00 00 70 42 89 88 88 3C").count(1).get(0).get<float>(20); //0x9E24EC

                //injector::WriteMemory(flt_9E24EC, fSimRate, true);
                //injector::WriteMemory(flt_9E2500, fSimRate, true);

                // NOTE: drift scoring system has 60.0 values... it may be affected by this... it needs thorough testing to see if parts like that are affected!

                // fix WorldMap cursor & scale the update constants with frametime to keep consistent movement
                // WorldMap::UpdateAnalogInput()
                auto pattern = hook::pattern("D9 05 ? ? ? ? 83 EC 08 D8 0D ? ? ? ? 56 8B F1");//0x0058F770 anchor
                // WorldMap framerate .text
                uint32_t* dword_58F779 = pattern.count(1).get(0).get<uint32_t>(0xB); //0x0058F779
                // constants
                float* flt_9CEDF4 = *pattern.count(1).get(0).get<float*>(0x1B); //0x0058F789 dereference
                uint32_t* dword_58F78F = pattern.count(1).get(0).get<uint32_t>(0x21); //0x0058F78F
                uint32_t* dword_58F79E = pattern.count(1).get(0).get<uint32_t>(0x2E); //0x0058F79E
                uint32_t* dword_58F7A8 = pattern.count(1).get(0).get<uint32_t>(0x38); //0x0058F7A8
                uint32_t* dword_58F7B5 = pattern.count(1).get(0).get<uint32_t>(0x45); //0x0058F7B5

                injector::WriteMemory(dword_58F779, &fSimRate, true);

                // Scalar target FPS
                constexpr float TargetFPS = 60.0f;

                static float WorldMapConst5 = ((*flt_9CEDF4) / (1.0f / TargetFPS)) * FrameTime;
                injector::WriteMemory(flt_9CEDF4, WorldMapConst5, true);

                static float WorldMapConst1 = ((*(float*)dword_58F78F) / (1.0f / TargetFPS)) * FrameTime;
                injector::WriteMemory(dword_58F78F, &WorldMapConst1, true);

                static float WorldMapConst2 = ((*(float*)dword_58F79E) / (1.0f / TargetFPS)) * FrameTime;
                injector::WriteMemory(dword_58F79E, &WorldMapConst2, true);

                static float WorldMapConst3 = ((*(float*)dword_58F7A8) / (1.0f / TargetFPS)) * FrameTime;
                injector::WriteMemory(dword_58F7A8, &WorldMapConst3, true);

                static float WorldMapConst4 = ((*(float*)dword_58F7B5) / (1.0f / TargetFPS)) * FrameTime;
                injector::WriteMemory(dword_58F7B5, &WorldMapConst4, true);
            }
        };
    }
} Misc;
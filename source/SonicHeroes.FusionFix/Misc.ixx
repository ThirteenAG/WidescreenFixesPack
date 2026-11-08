module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "LegacyHooks.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Misc;

import ComVars;
import Rendering;
import Camera;
import FrameRate;
import Updates;
import Cursor;
import Hud;
import Intro;

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {

            auto& settings = FixSettings();
            asi_settings::LiveLink::Start();
            SonicCursor::Initialize();
            // STUB: removed code authored by REDACTED
            Screen.Width = settings.Int("MAIN.ResX");
            Screen.Height = settings.Int("MAIN.ResY");
            Screen.fConditionalAspectRatio = settings.Float("MAIN.MinAspectRatio");
            const float factor = settings.Float("MAIN.FOVFactor");
            iniZoomFactor = std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f;
            GameplayFov::Factor.store(iniZoomFactor, std::memory_order_relaxed);
            settings.OnChange("MAIN.FOVFactor", []()
            {
                const float value = FixSettings().Float("MAIN.FOVFactor");
                iniZoomFactor = std::isfinite(value) ? std::clamp(value, 0.5f, 2.0f) : 1.0f;
                GameplayFov::Factor.store(iniZoomFactor, std::memory_order_relaxed);
                GameplayFov::RefreshPending.store(true, std::memory_order_relaxed);
                UpdateViewScales();
            });
            const int windowMode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
            const int borderlessScaling = std::clamp(settings.Int("MAIN.BorderlessScaling"), 0, 2);
            const int nWindowedMode = windowMode == 0 ? 0 : (windowMode == 1 ? (settings.Bool("MAIN.WindowResize") ? 3 : 2) : (borderlessScaling == 1 ? 4 : (borderlessScaling == 2 ? 1 : 5)));
            WindowedModeWrapper::EnableDpiAwareness();
            fShadowScale = settings.Float("MISC.ShadowScale");
            static uint32_t ShadowRes = settings.Int("GRAPHICS.ShadowResolution");
            static float ClipRange = settings.Float("MISC.ClipRange");
            static uint32_t WarpSwitchEffectMode = settings.Int("MISC.WarpSwitchEffectMode");
            static bool bDisableMouseInput = settings.Bool("MISC.DisableMouseInput");
            static bool bDisableFrameSkipping = settings.Bool("MISC.DisableFrameSkipping");
            static bool bRestoreDemos = settings.Bool("MISC.RestoreDemos");
            static bool bDisableQuitDialog = settings.Bool("MISC.DisableQuitDialog");
            static bool bDisableLoadingTimer = settings.Bool("MISC.DisableLoadingTimer");
            static bool bDisableSubtitles = settings.Bool("MISC.DisableSubtitles");
            static bool bIncreaseObjectDistance = settings.Bool("MISC.IncreaseObjectDistance");
            static uint8_t MinObjDistance = settings.Int("MISC.MinObjDistance") & 0xFF;
            static float fObjDistanceScale = settings.Float("GRAPHICS.DrawDistance");
            static bool bBetterFrameSync = settings.Bool("MISC.BetterFrameSync");
            BetterSync::mFPSLimitMode = (BetterSync::FrameLimiter::FPSLimitMode)settings.Int("MISC.FPSLimitMode");
            BetterSync::fFPSLimit = settings.Float("MAIN.FPSLimit");
    
            // STUB: removed code authored by REDACTED
            static auto szCustomUserFilesDirectoryInGameDir = settings.String("MISC.CustomUserFilesDirectoryInGameDir");
            if (szCustomUserFilesDirectoryInGameDir.empty() || szCustomUserFilesDirectoryInGameDir == "0")
                szCustomUserFilesDirectoryInGameDir.clear();

            // STUB: SkipFE code authored by REDACTED removed
            static bool bSkipFE = settings.Bool("SkipFE.Enabled");
            static int32_t SysMode = settings.Int("SkipFE.SystemMode");


            if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
                std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
            // Cover monitor renders at the monitor's own size, so the window never stretches the picture.
            if (nWindowedMode == 5)
                std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
            if (Screen.Width <= 0 || Screen.Height <= 0)
                std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            // STUB: removed code authored by REDACTED
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));

            UpdateViewScales();

            Screen.WidthFMV = Screen.Width;
            Screen.HeightFMV = Screen.Height;
    

            if (Screen.fAspectRatio < (4.0f / 3.0f))
            {
                Screen.HeightFMV = (int)(Screen.HeightFMV * (Screen.fAspectRatio / (4.0f / 3.0f)));
            }
            else
            {
                Screen.WidthFMV = (int)(Screen.WidthFMV / (Screen.fAspectRatio / (4.0f / 3.0f)));
            }

            // STUB: resolution variable code authored by REDACTED removed








            // STUB: screen size / lens flare code authored by REDACTED removed

            auto pattern = hook::pattern("0F BF 4E ? 0F BF C0 89 44 ? ? DB 44 ? ? 89 4C ? ? 85 DB"); // 662C2D
            struct ResHook6
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.ecx = *(uint16_t*)(regs.esi + 0x1E);

                    if (Screen.AspectRatioAffected)
                    {
                        if (Screen.PreserveHorizontalPosition == true)
                        {
                            regs.eax -= Screen.OffsetX;
                            if (Screen.ContinuePositionChange != false)
                            {
                                Screen.PreserveHorizontalPosition = true;
                            }
                            else
                            {
                                Screen.PreserveHorizontalPosition = false;
                            }
                        }
                        regs.eax += Screen.OffsetX;
                        regs.ecx += Screen.OffsetY;
                        if (Screen.ContinuePositionChange != false)
                        {
                            Screen.AspectRatioAffected = true;
                            Screen.ContinuePositionChange -= 1;
                        }
                        else
                        {
                            Screen.AspectRatioAffected = false;
                        }
                    }
                }
            };

            injector::MakeInline<ResHook6>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(7));
            pattern = hook::pattern("66 8B 46 1C 66 85 C0 75 29 66 39 46 1E 75 23 8D 0C DD"); // 662BFB
            injector::WriteMemory<uint8_t>(reinterpret_cast<uint8_t*>(pattern.count(1).get(0).get<int8_t>(7)), 0xEB, true);

            GameplayState.SetAddress(*hook::pattern("8B 46 04 53 55 33 ED 3B C5 57 89 2E BF 01 00 00 00 75 16").get_first<uintptr_t*>(0x14));
            GameplayFov::CurrentCamera = reinterpret_cast<decltype(GameplayFov::CurrentCamera)>(hook::pattern("A1 ? ? ? ? 85 C0 74 04 8B 40 28 C3 A1 ? ? ? ? 85 C0 75 ?").get_first());
            GameplayFov::MarkDirty = reinterpret_cast<decltype(GameplayFov::MarkDirty)>(hook::pattern("8B 44 24 04 33 C9 8B 90 A0 00 00 00 8A 4A 03 F6 C1 03 75 ?").get_first());
            RenderCamera.SetAddress(*hook::pattern("89 3D ? ? ? ? 8B 47 04 50 E8 ? ? ? ? 8D 8C 24 ? ? ? ? 50 51 E8").get_first<uintptr_t*>(2));
            const auto projectionCall = hook::pattern("8D 54 24 44 52 6A 03 D9 E0 D9 9C 24 ? ? ? ? E8 ? ? ? ? 8B 6F 60 A1").get_first(16);
            GameplayFov::SetTransform = reinterpret_cast<decltype(GameplayFov::SetTransform)>(injector::GetBranchDestination(projectionCall).as_int());
            injector::MakeCALL(projectionCall, GameplayFov::SetProjection, true);

            uintptr_t loc_64AFDC = 0;
            pattern = hook::pattern("E9 ? ? ? ? D9 42 68 D8 48 04 D9 42 68 D8 48 08 D9 5C 24 40");
            if (!pattern.empty()) // Tsonic_win2.exe: its own widescreen camera code starts here and is replaced as a whole
                loc_64AFDC = reinterpret_cast<uintptr_t>(pattern.get_first());
            else
                loc_64AFDC = reinterpret_cast<uintptr_t>(hook::pattern("D9 42 68 D8 08 D9 42 68 D8 48 04 D9 42 68 D8 48 08").get_first());
            struct CutOffAreaHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    float edx68 = *(float*)(regs.edx + 0x68) / Screen.fCameraScaleX;
                    float eax00 = *(float*)(regs.eax + 0);
                    float eax04 = *(float*)(regs.eax + 4);
                    float eax08 = *(float*)(regs.eax + 8);
                    _asm
                    {
                        fld     dword ptr[edx68]
                        fmul    dword ptr[eax00]
                        fld     dword ptr[edx68]
                        fmul    dword ptr[eax04]
                        fld     dword ptr[edx68]
                        fmul    dword ptr[eax08]
                    }
                }
            };

            struct CutOffAreaHookY
            {
                void operator()(injector::reg_pack& regs)
                {
                    float yScale = *(float*)(regs.edx + 0x6C) / Screen.fZoomFactor * *(float*)(regs.eax + 0x10);
                }
            };

            struct CutOffAreaHookY2
            {
                void operator()(injector::reg_pack& regs)
                {
                    float yScale = *(float*)(regs.edx + 0x6C) / Screen.fZoomFactor * *(float*)(regs.eax + 0x14);
                }
            };

            struct CutOffAreaHookY3
            {
                void operator()(injector::reg_pack& regs)
                {
                    float yScale = *(float*)(regs.edx + 0x6C) / Screen.fZoomFactor * *(float*)(regs.eax + 0x18);
                }
            };

            injector::MakeInline<CutOffAreaHook>(loc_64AFDC, loc_64AFDC + 17);
            injector::MakeInline<CutOffAreaHookY>(loc_64AFDC + 42, loc_64AFDC + 48);
            injector::MakeInline<CutOffAreaHookY2>(loc_64AFDC + 54, loc_64AFDC + 60);
            injector::MakeInline<CutOffAreaHookY3>(loc_64AFDC + 64, loc_64AFDC + 70);

            // STUB: HUD scale exceptions authored by REDACTED removed

            // 2P Interface
            pattern = hook::pattern("8B 48 60 DB 41 0C 83 C4 ? BF ? ? ? ? BE ? ? ? ? D9 5C"); // 0x422C2B
            struct HUDHor
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.ecx = *(int*)(regs.eax + 0x60);
                    float ResolutionX = (float)(*(int*)(regs.ecx + 0x0C)) * Screen.fHudScale;
                    _asm
                    {
                        fld dword ptr[ResolutionX]
                    }
                }
            };

            struct HUDVert
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.ebp |= -1;
                    float ResolutionY = (float)(*(int*)(regs.ecx + 0x10)) * Screen.fHudZoomFactor;
                    _asm
                    {
                        fld dword ptr[ResolutionY]
                    }
                }
            };

            injector::MakeInline<HUDHor>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
            injector::MakeInline<HUDVert>(pattern.count(1).get(0).get<uint32_t>(23), pattern.count(1).get(0).get<uint32_t>(29));

            pattern = hook::pattern("D9 44 ? ? D8 64 ? ? 6A 04 68 ? ? ? ? 6A 04 D9 15 ? ? ? ? D9 1D ? ? ? ? D9 44"); // 0x422A85

            struct HUDPos
            {
                void operator()(injector::reg_pack& regs)
                {
                    Screen.AspectRatioAffected = true;
                    float Temp = (*(float*)(regs.esp + 0x0C)) - (*(float*)(regs.esp + 0x14));
                    _asm
                    {
                        fld dword ptr[Temp]
                    }
                }
            };

            injector::MakeInline<HUDPos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(8));

            pattern = hook::pattern("DB 05 ? ? ? ? D8 0F D8 0D ? ? ? ? D9 9E ? ? ? ? 8B 57 04 89 96 ? ? ? ?"); // 0x4543FF
            injector::WriteMemory<decltype(&Screen.Width43)>(reinterpret_cast<decltype(&Screen.Width43)*>(pattern.count(1).get(0).get<uint32_t>(2)), &Screen.Width43, true);

            pattern = hook::pattern("DB 05 ? ? ? ? D9 5C ? ? DB 05 ? ? ? ? D9 5C ? ? E8 ? ? ? ? D9 05 ? ? ? ?"); // 0x4548B1

            struct CreditPicturePos
            {
                void operator()(injector::reg_pack& regs)
                {
                    Screen.AspectRatioAffected = true;
                    _asm
                    {
                        fild dword ptr[Screen.Width43]
                    }
                }
            };

            injector::MakeInline<CreditPicturePos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
            injector::WriteMemory<decltype(&Screen.HeightHUD)>(reinterpret_cast<decltype(&Screen.HeightHUD)*>(pattern.count(1).get(0).get<uint32_t>(12)), &Screen.HeightHUD, true);

            pattern = hook::pattern("8B D1 C1 EA 10 0F B6 C6 C1 E0 08 0F B6 F9 0B C7 89 4C ? ? C1 E0 08"); // 0x454FFD

            struct CreditPicturePos2
            {
                void operator()(injector::reg_pack& regs)
                {
                    Screen.AspectRatioAffected = true;
                    regs.edx = regs.ecx;
                    regs.edx >>= 10;
                }
            };

            injector::MakeInline<CreditPicturePos2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(5));

            pattern = hook::pattern("8B 44 ? ? EB 10 8B 1D ? ? ? ? 8B 2D ? ? ? ? 89 5C ? ?"); //0x6445C0

            injector::WriteMemory<decltype(&Screen.WidthFMV)>(reinterpret_cast<decltype(&Screen.WidthFMV)*>(pattern.count(1).get(0).get<uint32_t>(8)), &Screen.WidthFMV, true);
            injector::WriteMemory<decltype(&Screen.HeightFMV)>(reinterpret_cast<decltype(&Screen.HeightFMV)*>(pattern.count(1).get(0).get<uint32_t>(14)), &Screen.HeightFMV, true);
    
            // Mission text

            struct MissionTextHor
            {
                void operator()(injector::reg_pack& regs)
                {
                    Screen.AspectRatioAffected = true;
                    float ResolutionX = (float)(*(int*)(regs.ecx + 0x0C)) * Screen.fHudScale;
                    _asm
                    {
                        fld dword ptr[ResolutionX]
                    }
                    regs.esi = *(int*)(regs.esi + 0x98);
                }
            };

            struct MissionTextVert
            {
                void operator()(injector::reg_pack& regs)
                {
                    float ResolutionY = (float)(*(int*)(regs.ecx + 0x10)) * Screen.fHudZoomFactor;
                    _asm
                    {
                        fld dword ptr[ResolutionY]
                    }
                    *(float*)(regs.esp + 0x10) = ResolutionY;
                }
            };

            pattern = hook::pattern("DB 41 0C 8B B6 ? ? ? ? 83 C4 08 83 EE 02"); // 0x44296C
            injector::MakeInline<MissionTextHor>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(9));
            injector::MakeInline<MissionTextVert>(pattern.count(1).get(0).get<uint32_t>(19), pattern.count(1).get(0).get<uint32_t>(26));

            struct NowLoadingTextPos
            {
                void operator()(injector::reg_pack& regs)
                {
                    Screen.AspectRatioAffected = true;
                    float eax80 = *(float*)(regs.eax + 0x80);
                    _asm
                    {
                        fdiv dword ptr[eax80]
                    }
                }
            };
            pattern = hook::pattern("DB 05 ? ? ? ? D9 5C ? ? DB 47 ? D8 4C ? ? D8 0D ? ? ? ? D9 5C ? ? DB 05 ? ? ? ?");
            injector::MakeInline<NowLoadingTextPos>(pattern.count(1).get(0).get<uint32_t>(69), pattern.count(1).get(0).get<uint32_t>(75));

            struct PowerupIcon1
            {
                void operator()(injector::reg_pack& regs)
                {
                    Screen.AspectRatioAffected = true;
                    regs.ecx = *(int*)(regs.eax + 0x60);
                    float ResolutionX = (float)(*(int*)(regs.ecx + 0x0C)) * Screen.fHudScale;
                    _asm
                    {
                        fld dword ptr[ResolutionX]
                    }
                }
            };

            struct PowerupIcon2
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.ecx = *(int*)(regs.eax + 0x60);
                    float ResolutionX = (float)(*(int*)(regs.ecx + 0x0C)) * Screen.fHudScale;
                    _asm
                    {
                        fld dword ptr[ResolutionX]
                    }
                }
            };

            struct PowerupIcon3
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.edx = *(int*)(regs.eax + 0x60);
                    float ResolutionY = (float)(*(int*)(regs.edx + 0x10)) * Screen.fHudZoomFactor;
                    _asm
                    {
                        fld dword ptr[ResolutionY]
                    }
                }
            };

            struct PowerupIcon4
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.edi = 1;
                    _asm
                    {
                        fmul dword ptr[Screen.fHudScale]
                    }
                }
            };

            pattern = hook::pattern("8B 48 60 DB 41 0C 6A 04 68 ? ? ? ? 6A 04 D8 4C ? ?"); //0x479AEA
            injector::MakeInline<PowerupIcon1>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
            pattern = hook::pattern("8B 48 60 DB 41 0C D9 5C ? ? E8 ? ? ? ? 8B 50 60 DB 42 10 D9 5C ? ? E8 ? ? ? ? D9 05"); // 0x479EB1
            injector::MakeInline<PowerupIcon2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
            injector::MakeInline<PowerupIcon3>(pattern.count(1).get(0).get<uint32_t>(15), pattern.count(1).get(0).get<uint32_t>(21));
            pattern = hook::pattern("DB 40 0C 8B 54 ? ? 8B 44 ? ? 8B 4D 34 D8 0D ? ? ? ? 4A 89 54 ? ? BF 01 00 00 00 3B C7"); // 0x479F26
            injector::MakeInline<PowerupIcon4>(pattern.count(1).get(0).get<uint32_t>(25), pattern.count(1).get(0).get<uint32_t>(30));

            // Results screen
            pattern = hook::pattern("8B 48 60 DB 41 0C E8 ? ? ? ? 8B 50 60 DB 42 10 D9 5C ? ? E8 ? ? ? ? D9 05 ? ? ? ? BE"); // 0x438CDD
            struct ResultsHor
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.ecx = *(int*)(regs.eax + 0x60);
                    float ResolutionX = (float)(*(int*)(regs.ecx + 0x0C)) * Screen.fHudScale;
                    _asm
                    {
                        fld dword ptr[ResolutionX]
                    }
                }
            };

            struct ResultsVert
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.edx = *(int*)(regs.eax + 0x60);
                    float ResolutionY = (float)(*(int*)(regs.edx + 0x10)) * Screen.fHudZoomFactor;
                    _asm
                    {
                        fld dword ptr[ResolutionY]
                    }
                }
            };

            struct ResultsPos
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.eax <<= 8;
                    regs.eax |= regs.ecx;
                    Screen.AspectRatioAffected = true;
                }
            };

            injector::MakeInline<ResultsHor>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
            injector::MakeInline<ResultsVert>(pattern.count(1).get(0).get<uint32_t>(11), pattern.count(1).get(0).get<uint32_t>(17));
            pattern = hook::pattern("8A 44 B4 ? C1 E0 08 0B C1 C1 E0 08 0B C2 A3"); //0x438E42
            injector::MakeInline<ResultsPos>(pattern.count(1).get(0).get<uint32_t>(4), pattern.count(1).get(0).get<uint32_t>(9));

            // Special Stage
            // STUB: dust effect code authored by REDACTED removed

            pattern = hook::pattern("DB 05 ? ? ? ? 8B 00 D9 84 ? ? ? ? ? 53 55 D8 C9 57 50 6A 01 D8 0D ? ? ? ? D9 9C"); // 0x45894A
            injector::MakeInline<CreditPicturePos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
            injector::WriteMemory<decltype(&Screen.HeightHUD)>(reinterpret_cast<decltype(&Screen.HeightHUD)*>(pattern.count(1).get(0).get<uint32_t>(38)), &Screen.HeightHUD, true);

            pattern = hook::pattern("DB 05 ? ? ? ? A1 ? ? ? ? 83 C4 08 D9 54 ? ? D8 8C"); // 0x526F83
            injector::MakeInline<CreditPicturePos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
            injector::WriteMemory<decltype(&Screen.HeightHUD)>(reinterpret_cast<decltype(&Screen.HeightHUD)*>(pattern.count(1).get(0).get<uint32_t>(40)), &Screen.HeightHUD, true);

            injector::WriteMemory<decltype(&Screen.Width43)>(reinterpret_cast<decltype(&Screen.Width43)*>(pattern.count(1).get(0).get<uint32_t>(236)), &Screen.Width43, true);


            pattern = hook::pattern("E8 ? ? ? ? 83 C4 10 E8 ? ? ? ? 5F C3 CC CC CC CC CC CC 83 EC 0C 56 33 F6 89 74"); // 0x527470

            struct MeteorRightPos
            {
                void operator()(injector::reg_pack& regs)
                {
                    if (regs.eax != 0)
                    {
                        Screen.PreserveHorizontalPosition = true;
                        Screen.ContinuePositionChange = 3;
                    }
                }
            };

            injector::MakeInline<MeteorRightPos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(5));
            injector::WriteMemory<uint16_t>(reinterpret_cast<uint16_t*>(pattern.count(1).get(0).get<uint16_t>(5)), 0xD1EB, true);
            injector::MakeNOP(pattern.count(1).get(0).get<uint16_t>(7), 1, true);

            pattern = hook::pattern("DB 05 ? ? ? ? A1 ? ? ? ? 0F B6 9C 24 ? ? ? ? D9 54 24 ? C1 E3 18"); // 0x5263DE

            injector::MakeInline<CreditPicturePos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
            injector::WriteMemory<decltype(&Screen.HeightHUD)>(reinterpret_cast<decltype(&Screen.HeightHUD)*>(pattern.count(1).get(0).get<uint32_t>(63)), &Screen.HeightHUD, true);

            pattern = hook::pattern("DB 05 ? ? ? ? A1 ? ? ? ? 83 C4 08 D9 54 24 ? BF ? ? ? ? D8 8C 24 ? ? ? ? 8D 74 24 ?"); // 0x526297

            injector::MakeInline<CreditPicturePos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
            injector::WriteMemory<decltype(&Screen.HeightHUD)>(reinterpret_cast<decltype(&Screen.HeightHUD)*>(pattern.count(1).get(0).get<uint32_t>(49)), &Screen.HeightHUD, true);

            pattern = hook::pattern("85 C0 74 0F 8B 46 28 85 C0 75 08 C7 44 24 04 00 80 FD 43 8A 42 1E 84 C0 74 0E"); // 0x527936

            struct LevelUpPos1
            {
                void operator()(injector::reg_pack& regs)
                {
                    Screen.PreserveHorizontalPosition = false;
                    regs.eax = *(int*)(regs.esi + 0x28);
                    if (regs.eax == false) // REDACTED change reverted
                    {
                        Screen.PreserveHorizontalPosition = true;
                        *(float*)(regs.esp + 4) = 507.0f;
                    }
                }
            };

            injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 4, true);
            injector::MakeInline<LevelUpPos1>(pattern.count(1).get(0).get<uint32_t>(4), pattern.count(1).get(0).get<uint32_t>(19));

            struct LevelUpPos2
            {
                void operator()(injector::reg_pack& regs)
                {
                    Screen.AspectRatioAffected = false;
                    Screen.ContinuePositionChange = true;
                    regs.eax = *(int*)(regs.esi + 0x2C);
                    if (regs.eax < 95)
                    {
                        Screen.ContinuePositionChange = true;
                    }
                    regs.ecx = *(int*)(regs.esi + 0x48);
                }
            };

            injector::MakeInline<LevelUpPos2>(pattern.count(1).get(0).get<uint32_t>(40), pattern.count(1).get(0).get<uint32_t>(46));


            // STUB: shadow fix, custom user files directory, windowed mode and mouse input code authored by REDACTED removed

            if (WarpSwitchEffectMode)
            {

                uintptr_t loc_41BAD5 = reinterpret_cast<uintptr_t>(hook::pattern("83 7F 50 04 75 0D 6A 02 6A 0A E8 ? ? ? ? 6A ? EB 0B 6A 05 6A 0A E8 ? ? ? ? 6A 02 6A 0B").get_first(0)) + 0x10;

                // STUB: removed code authored by REDACTED

                injector::WriteMemory<uint8_t>(reinterpret_cast<uint8_t*>(loc_41BAD5), WarpSwitchEffectMode, true);
            }

            // STUB: frame skipping, demo restore, SkipFE, disc check, shadow resolution, quit dialog, clip range,
            // 2P / special stage / results / Advertise window / staff roll scaling, window icon, loading timer and
            // object distance code authored by REDACTED removed

            BetterSync::bEnabled = bBetterFrameSync;
            // STUB: BetterSync frame sync hook authored by REDACTED removed
            WFP::onGameProcessEvent() += []() { RWGTA::UpdateCheck::Tick(); };

            SonicHud::Init(&Screen.fWidth, &Screen.fHeight, &Screen.fHudScale, &GameplayFov::Factor, &GameplayFov::ProjectionAdjusted);
            SonicIntro::Init();
            InitUpdates();
        };
    }
} Misc;

module;

#include <stdafx.h>
#include <atomic>
#include "FixSettings.hpp"
#include "Shared/D3D8Compatibility.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Misc;

import ComVars;
import Hud;
import Camera;
import Movies;
import Cursor;
import Window;
import DeveloperOverlay;
import Updates;

std::atomic<int> FrameLimit{ 60 };

void UpdateFrameLimit()
{
    const int limit = FixSettings().Int("MAIN.FPSLimit");
    FrameLimit.store(limit < 0 ? int(GetDesktopRefreshRate()) : limit > 0 ? std::clamp(limit, 30, 1000) : 0, std::memory_order_relaxed);
}

// The start-up sequence: legal screen, Rebellion movie, then the intro movie; skipping goes straight to the intro movie,
// which the game's own button skips
SafetyHookMid StartupSequence;

void InstallIntroSkip()
{
    auto pattern = hook::pattern("A0 ? ? ? ? 84 C0 0F 84 ? ? ? ? A1 ? ? ? ? 85 C0 8B 0D"); //4D5920
    StartupState.SetAddress(*pattern.get_first<int32_t*>(14));
    StartupStep.SetAddress(*pattern.get_first<uint32_t*>(22));
    StartupSequence = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&)
    {
        if (StartupState == 0 && StartupStep < 4)
            StartupStep = 4;
    });
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            FMV::Mode.store(settings.Int("MAIN.FMVWidescreenMode"), std::memory_order_relaxed);
            settings.OnChange("MAIN.FMVWidescreenMode", []()
            {
                FMV::Mode.store(FixSettings().Int("MAIN.FMVWidescreenMode"), std::memory_order_relaxed);
            });
            asi_settings::LiveLink::Start();
            SniperCursor::Initialize();
            UpdateFrameLimit();
            settings.OnChange("MAIN.FPSLimit", UpdateFrameLimit);
            D3D8Compatibility::Initialize(false, false, []() { return FrameLimit.load(std::memory_order_relaxed); });
            SniperOverlay::Init();
            WFP::onGameProcessEvent() += []() { RWGTA::UpdateCheck::Tick(); };
            SniperOverlay::Tick = []() { WFP::onGameProcessEvent().executeAll(); };
            if (settings.Bool("MAIN.SkipIntro"))
                InstallIntroSkip();
            Screen.Width = settings.Int("MAIN.ResX");
            Screen.Height = settings.Int("MAIN.ResY");
            static const auto& fFOVFactor = settings.Float("MAIN.FOVFactor");

            if (settings.Int("MAIN.WindowMode") == 2 || Screen.Width <= 0 || Screen.Height <= 0)
                std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
            if (Screen.Width <= 0 || Screen.Height <= 0)
                std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
            Screen.fWidth43 = static_cast<float>(Screen.Width43);
            SniperWindow::Init(Screen.Width, Screen.Height);

            {
                auto pattern = hook::pattern("C7 ? ? ? ? ? ? ? ? ? C7 ? ? ? ? ? ? ? ? ? C7 ? ? ? ? ? ? ? ? ? C7 ? ? ? ? ? ? ? ? ? 50 A1"); //622B17
                injector::WriteMemory<int32_t>(pattern.count(1).get(0).get<int32_t>(6), Screen.Height, true);
                injector::WriteMemory<int32_t>(pattern.count(1).get(0).get<int32_t>(16), Screen.Width, true);

                pattern = hook::pattern("8B ? ? ? ? ? 8B ? ? ? ? ? 8B ? 99 2B ? D1 ? 89"); //48B0D6
                struct SetResHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.ecx = Screen.Width;
                        regs.esi = Screen.Height;
                    }
                }; injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(12));

                pattern = hook::pattern("8B ? ? ? 57 8B ? ? ? 50 6A"); //48B076
                struct SetResHook2
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.edi = Screen.Width;
                        regs.esi = Screen.Height;
                    }
                }; injector::MakeInline<SetResHook2>(pattern.count(1).get(0).get<uint32_t>(1), pattern.count(1).get(0).get<uint32_t>(9));
                injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint8_t>(0), 0x57, true); //push edi
            }

            {
                auto pattern = hook::pattern("D9 ? ? ? 8B ? ? ? D9 ? ? ? ? ? A3 ? ? ? ? DA"); //4152C0
                struct SetScalingHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        std::memcpy(&regs.eax, &Screen.fAspectRatio, sizeof(float));
                    }
                }; injector::MakeInline<SetScalingHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(8));
                injector::WriteMemory<float>(*pattern.count(1).get(0).get<float*>(10), Screen.fAspectRatio, true);
                injector::WriteMemory<float>(*pattern.count(1).get(0).get<float*>(15), Screen.fAspectRatio, true);
            }

            {
                auto pattern = hook::pattern("D8 ? ? ? ? ? 83 ? ? 6A ? 68 ? ? ? ? 51 D9 ? ? E8 ? ? ? ? 83 ? ? 83"); //4A15F6
                GameplayFOV.SetAddress(*pattern.count(1).get(0).get<float*>(2));
                auto updateFOV = []()
                {
                    const float factor = std::isfinite(fFOVFactor) ? std::clamp(fFOVFactor, 0.5f, 2.0f) : 1.0f;
                    const float native = AdjustFOV(75.0f, Screen.fAspectRatio);
                    GameplayFOV = std::clamp(native * factor, 1.0f, std::max(native, 150.0f)); //at most 150 degrees across
                };
                updateFOV();
                settings.OnChange("MAIN.FOVFactor", updateFOV);
            }

            {
                const auto cutsceneFOV = hook::get_pattern<uint8_t>("E8 ? ? ? ? 83 ? ? C6 ? ? ? ? ? ? F6 ? ? ? 74"); //4749FE
                const auto cutsceneFOVEvent = hook::get_pattern<uint8_t>("51 51 D9 ? ? E8 ? ? ? ? 83 ? ?", 5); //478A7A
                const auto cutsceneFOVCapture = hook::get_pattern<uint8_t>("D8 ? ? C6 ? ? ? DC ? D9"); //4789A1
                Camera::SetProjection = reinterpret_cast<decltype(Camera::SetProjection)>(injector::GetBranchDestination(cutsceneFOV, true).as_int());
                injector::MakeCALL(cutsceneFOV, Camera::Projection, true);
                injector::MakeCALL(cutsceneFOVEvent, Camera::Projection, true);
                Camera::CutsceneFOVCapture = safetyhook::create_mid(cutsceneFOVCapture, Camera::CaptureCutsceneFOV);

                const auto cinematicFraming = hook::get_pattern<uint8_t>("D9 ? ? ? ? ? D8 ? ? ? ? ? D8 ? ? ? ? ? D8 ? ? ? ? ? D8 ? ? ? ? ? D9 ? D8 ? D8 ? ? ? ? ? DE ? C3"); //4797B0
                if (auto* band = *reinterpret_cast<const float**>(cinematicFraming + 0x0E))
                    CinematicBand.SetAddress(const_cast<float*>(band));
                Camera::Update();
                FixSettings().OnChange("MAIN.CutsceneBorders", Camera::Update);
                for (size_t offset : { 0x8, 0xCA }) //subtitle position and bar height
                    injector::WriteMemory<const float*>(reinterpret_cast<const float**>(cinematicFraming + offset), &Camera::CinematicAspect, true);
                injector::WriteMemory<const float*>(reinterpret_cast<const float**>(cinematicFraming + 0x38), &Camera::LetterboxProbe, true); //marks letterboxed scenes
                if (auto* bars = *reinterpret_cast<void***>(cinematicFraming + 0xB1))
                    BarsOverride.SetAddress(bars);
                Camera::CinematicBars = safetyhook::create_inline(cinematicFraming + 0xB0, Camera::DrawCinematicBars);
                const auto subtitle = hook::get_pattern("83 EC 10 C7 04 24 9A 99 99 3D C7 44 24 08 44 8B 6C 3F E8"); //475080
                Camera::Subtitle = safetyhook::create_inline(subtitle, Camera::DrawSubtitle);

                const auto cameraEffects = hook::get_pattern<uint8_t>("83 ? ? DB ? ? ? ? ? A1 ? ? ? ? 83 ? ? D9"); //600570
                if (auto* band = *reinterpret_cast<int32_t**>(cameraEffects + 0x0A))
                    EffectBand.SetAddress(band);
                if (auto* alpha = *reinterpret_cast<uint8_t**>(cameraEffects + 0x9D + 3))
                    EffectAlpha.SetAddress(alpha);
                injector::WriteMemory<const float*>(reinterpret_cast<const float**>(cameraEffects + 0x51), &Camera::EffectAspect, true);
                Camera::EffectBars = safetyhook::create_inline(cameraEffects, Camera::DrawEffectBars);
            }

            Screen.fHudOffset = (Screen.fWidth - Screen.Width43) / 2.0f;

            {

                auto pattern = hook::pattern("51 56 8B ? ? ? 85 ? 0F ? ? ? ? ? 80"); //408EE0
                Textures::RegisterTexture = safetyhook::create_inline(pattern.get_first(), Textures::Register);

                pattern = hook::pattern("56 8B ? 56 E8 ? ? ? ? 8A"); //48B140
                uint8_t* drawHudQuad = pattern.count(1).get(0).get<uint8_t>(0);
                QueueHudQuad = reinterpret_cast<decltype(QueueHudQuad)>(injector::GetBranchDestination(drawHudQuad + 4, true).as_int());
                injector::MakeCALL(drawHudQuad + 4, sub_48B140Hook, true);

                pattern = hook::pattern("BE ? ? ? ? F3 ? A1 ? ? ? ? 8B"); //49B05D
                auto* blurQuad = *pattern.count(1).get(0).get<HudBlur*>(1);
                const float maskOffset = Screen.fHudOffset / Screen.fWidth43;
                for (size_t i = 0; i < 4; ++i)
                    injector::WriteMemory<float>(&blurQuad[i].maskU, (i == 1 || i == 2) ? 1.0f + maskOffset : -maskOffset, true); //scope and binocular blur masks keep their aspect

                const auto quadVertices = hook::get_pattern<uint8_t>("8B ? 89 ? ? 8B ? 8B ? ? 6B ? ? 8B"); //487E97
                injector::MakeInline<HudVertexHook<0>>(quadVertices, quadVertices + 0x14);
                injector::MakeInline<HudVertexHook<1>>(quadVertices + 0x69, quadVertices + 0x7E);
                injector::MakeInline<HudVertexHook<2>>(quadVertices + 0xD0, quadVertices + 0xE5);
                injector::MakeInline<HudVertexHook<3>>(quadVertices + 0x12F, quadVertices + 0x14C); //Replace the native XYZRHW coordinate writes with half-pixel alignment.

                pattern = hook::pattern("83 ? ? 56 8B ? 8B ? ? 85 ? 57 74 ? 8B ? 85"); //507C40
                uint8_t* drawScopeReticle = pattern.count(1).get(0).get<uint8_t>(0);
                static constexpr int32_t horizontalCalls[] = { 0x31F, 0x3BC, 0x3E9 };
                static constexpr int32_t verticalCalls[] = { 0x33C, 0x401, 0x416 };
                for (size_t i = 0; i < _countof(ReticleHorizontal); ++i)
                    ReticleHorizontal[i] = safetyhook::create_mid(drawScopeReticle + horizontalCalls[i], AdjustReticleHorizontal);
                for (size_t i = 0; i < _countof(ReticleVertical); ++i)
                    ReticleVertical[i] = safetyhook::create_mid(drawScopeReticle + verticalCalls[i], AdjustReticleVertical);

                static float fHudScale2 = (0.0009765625f / Screen.fAspectRatio) * (4.0f / 3.0f);
                pattern = hook::pattern("D8 ? ? ? ? ? D9 ? ? D9 ? ? ? D8 ? ? ? ? ? D9 ? ? ? ? ? D9 ? ? ? D8"); //502232
                injector::WriteMemory<float*>(pattern.count(1).get(0).get<float*>(2), &fHudScale2, true); //text size and radar arrows

                pattern = hook::pattern("8B ? ? 89 ? ? 83 ? ? ? ? ? ? 75 ? A1"); //5021C2
                struct HudHook2
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(float*)(regs.esi + 0x28) += Screen.fHudOffset;
                        *(float*)(regs.esi + 0x30) -= Screen.fHudOffset * 2.0f;

                        regs.eax = *(uintptr_t*)(regs.edi + 0x4);
                        *(uintptr_t*)(regs.esi + 0x44) = regs.eax;
                    }
                }; injector::MakeInline<HudHook2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));

                Hud::Update();
                settings.OnChange("MAIN.HudAspectRatioConstraint", Hud::Update);
                const auto heartRatePanelDraw = hook::get_pattern<uint8_t>("8B 15 ? ? ? ? 56 8B F1 8B 46 60 85 C0 74 ? 8B 48 04 85 C9 74 ? 83 B9 88 00 00 00 01"); //504840
                LocalPlayers.SetAddress(*reinterpret_cast<int32_t**>(heartRatePanelDraw + 2));
                Hud::IsPlayerScoped = reinterpret_cast<decltype(Hud::IsPlayerScoped)>(injector::GetBranchDestination(heartRatePanelDraw + 0x27, true).as_int());
                Hud::HeartRatePanelDraw = reinterpret_cast<uintptr_t>(heartRatePanelDraw);
                Hud::Anchor = safetyhook::create_mid(hook::get_pattern("8B 46 08 85 C0 74 2A 8B 46 0C 33"), Hud::AnchorElement); //50227B

                auto minimapIcons = hook::pattern("E8 ? ? ? ? 8B ? 8B ? 04 8B ? ? 89 ? ? ? 8B ? ? 8D"); //50BB91, 50C10B
                for (size_t i = 0; i < 2; ++i)
                    UI::MinimapIcons[i] = safetyhook::create_mid(minimapIcons.count(2).get(i).get<uint8_t>(5), UI::AlignMinimapIcon);

                uint8_t* minimapPlayers = nullptr;
                auto minimapPlayersSIG = hook::pattern("D9 ? ? ? 8B ? 8B ? ? 8B ? ? ? ? ? D8"); //50BE0C
                if (!minimapPlayersSIG.empty())
                    minimapPlayers = minimapPlayersSIG.get_first<uint8_t>();
                else
                    minimapPlayers = hook::get_pattern<uint8_t>("8B ? 8B ? ? 8D ? ? ? 89 ? ? ? 50 8D ? ? ? 51");

                UI::MinimapIcons[2] = safetyhook::create_mid(minimapPlayers, UI::AlignMinimapIcon);
                const auto minimapRect = hook::get_pattern<uint8_t>("D9 ? ? ? ? ? 5E 5B 83 ? ? C3", 6); //50907A
                UI::MinimapRect = safetyhook::create_mid(minimapRect, UI::SquareMinimap);
                const auto minimapCenter = hook::get_pattern<uint8_t>("D9 ? ? ? D8 ? ? ? ? ? D9 ? ? ? ? ? D8 ? ? ? ? ? D9 ? ? ? ? ? E8"); //50B30C
                UI::MinimapCenter = safetyhook::create_mid(minimapCenter, UI::CenterMinimap);

                const auto fontClip = hook::get_pattern<uint8_t>("8B ? ? ? 83 ? ? 53 8B ? ? ? 85 ? 56"); //40E450
                const auto printText = hook::get_pattern<uint8_t>("83 ? ? A0 ? ? ? ? 84 ? 55 57"); //40F570
                const auto nativeWidth = *hook::get_pattern<uintptr_t>("DB ? ? ? ? ? D8 ? D9 ? DB ? ? ? ? ? D8 ? D9", 2); //40E4FD
                UI::RedirectWidth(fontClip, printText + 0x650, nativeWidth, &UI::FontWidth); //clip, glyphs and text output; end before text height
                UI::PixelClip = safetyhook::create_inline(fontClip, UI::GetPixelClip);
                UI::PrintText = safetyhook::create_inline(printText, UI::Print);
                SniperUpdates::Init(reinterpret_cast<decltype(SniperUpdates::Print)>(&UI::Print));

                const auto cutsceneSubtitles = hook::get_pattern<uint8_t>("A1 ? ? ? ? 83 ? ? 85 ? 74 ? 8A"); //5DB9C0
                UI::RedirectWidth(cutsceneSubtitles, cutsceneSubtitles + 0xC4, nativeWidth, &Screen.Width43); //cutscenes font scale follows height

                const auto menuDraw = hook::get_pattern<uint8_t>("8B 0D ? ? ? ? E9 ? ? ? ? ? ? ? ? ? 8B 0D"); //4819F0
                const auto menuItems = hook::get_pattern<uint8_t>("51 56 8B ? 8B ? ? 50 E8 ? ? ? ? 0F ? ? ? 8B"); //47DFC0
                const auto frontendItems = hook::get_pattern<uint8_t>("56 57 8B ? 8B ? FF ? ? 8B ? ? 8B ? 3B"); //4AF0A0
                const auto frontendLayout = hook::get_pattern<uint8_t>("A1 ? ? ? ? 56 8B ? ? ? 50 8B"); //4D5DF0
                const auto frontendEnd = hook::get_pattern<uint8_t>("83 ? ? A1 ? ? ? ? 89 ? ? ? A0 ? ? ? ? 84 ? 0F ? ? ? ? ? A1 ? ? ? ? 84"); //4FE850

                const auto menuFontScale = hook::get_pattern<uint8_t>("DB ? ? ? ? ? D8 ? ? ? ? ? DB ? ? ? ? ? D8 ? ? ? ? ? D9"); //4D4340
                UI::RedirectWidth(menuItems, menuDraw, nativeWidth, &Screen.Width43); //controls and hitboxes; end before menu dispatch
                UI::RedirectWidth(frontendItems, menuFontScale + 0x30, nativeWidth, &Screen.Width43); //frontend controls through font scale
                UI::RedirectWidth(frontendLayout, frontendEnd, nativeWidth, &Screen.Width43); //frontend layout through menu text sizing
                UI::MenuDraw = safetyhook::create_inline(menuDraw, UI::DrawMenu);
                UI::MenuDrawOverlay = safetyhook::create_inline(menuDraw + 0x10, UI::Draw<UI::Menu, UI::MenuDrawOverlay, char>);

                const auto loadingImage = hook::get_pattern<uint8_t>("A1 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 6A ? 6A"); //4D49E0
                if (auto* loadingTexture = *reinterpret_cast<int32_t**>(loadingImage + 1))
                    LoadingTexture.SetAddress(loadingTexture);
                UI::FlushQuads = reinterpret_cast<decltype(UI::FlushQuads)>(injector::GetBranchDestination(loadingImage + 0x66, true).as_int());

                const auto loadingFrame = hook::get_pattern<uint8_t>("83 ? ? DB ? ? ? ? ? 56 8B ? ? ? 83"); //4D4770
                const auto loadingText = hook::get_pattern<uint8_t>("83 ? ? DB ? ? ? ? ? A1 ? ? ? ? 56"); //4D4840
                const auto loadingProgress = hook::get_pattern<uint8_t>("83 ? ? DB ? ? ? ? ? 83 ? ? DB ? ? ? ? ? 8D"); //4D4930
                const auto loadingPercent = hook::get_pattern<uint8_t>("83 ? ? DB ? ? ? ? ? 56 83"); //4D4CF0
                UI::RedirectWidth(loadingFrame, loadingImage, nativeWidth, &Screen.Width43); //frame, text and progress; exclude background image
                UI::RedirectWidth(loadingPercent, loadingPercent + 0x160, nativeWidth, &Screen.Width43); //percentage overlay; end before loading setup
                UI::LoadingFrame = safetyhook::create_inline(loadingFrame, UI::Draw<UI::Menu, UI::LoadingFrame, char, int32_t>);
                UI::LoadingText = safetyhook::create_inline(loadingText, UI::Draw<UI::Menu, UI::LoadingText, int32_t, int32_t>);
                UI::LoadingProgress = safetyhook::create_inline(loadingProgress, UI::Draw<UI::Menu, UI::LoadingProgress, int32_t, float>);
                UI::LoadingPercent = safetyhook::create_inline(loadingPercent, UI::Draw<UI::Menu, UI::LoadingPercent, int32_t, float>);

                const auto infoDraw = hook::get_pattern<uint8_t>("A0 ? ? ? ? 83 ? ? 84 ? 53 55 56 57 0F"); //4ADBA0
                uint8_t* infoItems = nullptr;
                auto infoItemsSIG = hook::pattern("A0 ? ? ? ? 83 ? ? 84 ? 75 ? A1 ? ? ? ? 85 ? 75 ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 ? ? 84 ? 74 ? ? A1 ? ? ? ? 85"); //4AC2B0
                if (!infoItemsSIG.empty())
                    infoItems = infoItemsSIG.get_first<uint8_t>();
                else
                    infoItems = hook::get_pattern<uint8_t>("A1 ? ? ? ? 83 EC ? 85 C0 0F 84 ? ? ? ? ? ? ? ? C1 E0");

                uint8_t* infoEnd = nullptr;
                auto infoEndSIG = hook::pattern("A0 ? ? ? ? 83 ? ? 84 ? 0F ? ? ? ? ? A0 ? ? ? ? 84 ? 75"); //4AE290
                if (!infoEndSIG.empty())
                    infoEnd = infoEndSIG.get_first<uint8_t>();
                else
                    infoEnd = hook::get_pattern<uint8_t>("A0 ? ? ? ? 83 ? ? 84 ? 0F ? ? ? ? ? 53 8A");

                UI::RedirectWidth(infoItems, infoEnd, nativeWidth, &Screen.Width43); //map, markers and frame; end after info draw
                UI::InfoScreen = safetyhook::create_inline(infoDraw, UI::Draw<UI::Info, UI::InfoScreen, char>);

                pattern = hook::pattern("A0 ? ? ? ? 83 ? ? 84 ? 0F ? ? ? ? ? DB"); //481B50
                UI::MouseUpdate = safetyhook::create_mid(pattern.get_first(), UI::UpdateMouse);
                pattern = hook::pattern("8B ? ? ? 50 8B ? ? ? 8B ? ? 8B ? 8B ? ? ? 51"); //486BB0
                UI::DrawLine = safetyhook::create_inline(pattern.get_first(), UI::Line);

                pattern = hook::pattern("8D ? ? ? 52 E8 ? ? ? ? 83 ? ? 8B ? E8"); //4C7F24
                auto profileViewportCall = pattern.get_first<uint8_t>(5);
                UI::SetViewingArea = reinterpret_cast<decltype(UI::SetViewingArea)>(injector::GetBranchDestination(profileViewportCall, true).as_int());
                injector::MakeCALL(profileViewportCall, UI::ProfileViewingArea, true); //profile viewport

                pattern = hook::pattern("8B ? ? ? ? ? A1 ? ? ? ? 8B ? ? ? 52 8B"); //46BD66
                const float cursorScale = Screen.fHeight / 960.0f;
                auto* cursorHeight = *pattern.get_first<float*>(2);
                auto* cursorWidth = *pattern.get_first<float*>(7);
                injector::WriteMemory(cursorWidth, *cursorWidth * cursorScale, true);
                injector::WriteMemory(cursorHeight, *cursorHeight * cursorScale, true); //cursor size follows the resolution

                const auto actionManagerDraw = hook::get_pattern<uint8_t>("D8 ? ? ? ? ? 50 D9 ? ? ? E8 ? ? ? ? 50", 2); //5E880C
                const auto textObjectDraw = hook::get_pattern<uint8_t>("D8 ? ? ? ? ? 89 ? ? ? 8B ? ? ? D8 ? ? ? 89", 2); //4FEDF1
                UI::ActionManagerTextScale = **reinterpret_cast<float**>(actionManagerDraw) * UI::ScaleText();
                UI::GameplayTextObjectScale = **reinterpret_cast<float**>(textObjectDraw) * UI::ScaleText();
                injector::WriteMemory(actionManagerDraw, &UI::ActionManagerTextScale, true); //compass pointer, interaction hints
                injector::WriteMemory(textObjectDraw, &UI::GameplayTextObjectScale, true); //NPC quotes, objective alerts, weapon and scoring status
            }

            {
                const auto movieDraw = hook::get_pattern<uint8_t>("A1 ? ? ? ? 83 ? ? 0F ? ? ? ? ? 50 E8 ? ? ? ? 83 ? ? 84"); //628280
                MovieHandle.SetAddress(*reinterpret_cast<int32_t**>(movieDraw + 1));
                const auto streamStorage = hook::get_pattern<uint8_t>("8B ? ? ? ? ? 8B ? ? 85 ? 74 ? 8A ? ? C3"); //41367A
                MovieStreams.SetAddress(*reinterpret_cast<FMV::Stream****>(streamStorage + 2));
                const auto movieRect = hook::get_pattern<uint8_t>("E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? B9 ? ? ? ? E8 ? ? ? ? A1"); //6282CE
                FMV::ScreenRect = safetyhook::create_mid(movieRect, FMV::FitToScreen);
            }


        };
    }
} Misc;

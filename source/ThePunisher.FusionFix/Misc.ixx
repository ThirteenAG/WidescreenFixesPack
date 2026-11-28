module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include <atomic>


export module Misc;

import NativeMenu;
import MenuLayout;
import Updates;
import Window;
import DeveloperOverlay;
import ComVars;
import Hud;
import Frontend;
import Cutscene;
import Camera;
import Pad;




export std::atomic<int> PunisherFrameLimit{ 60 };

export int GetPunisherFrameLimit()
{
    const int limit = PunisherFrameLimit.load(std::memory_order_relaxed);
    return limit == -1 ? PunisherWindow::MonitorRefreshRate.load(std::memory_order_relaxed) : limit;
}

void UpdateFrameLimit()
{
    const int limit = PunisherNumericSetting("MAIN.FPSLimit", 60);
    PunisherFrameLimit.store(limit > 0 ? std::clamp(limit, 30, 1000) : std::max(limit, -1), std::memory_order_relaxed);
}

void RemoveNativeFrameWait()
{
    static const float minimumFrameTime = 0.0f;
    const auto timing = hook::pattern("D9 15 ? ? ? ? D9 15 ? ? ? ? D9 05 ? ? ? ? D8 1D ? ? ? ? DF E0 F6 C4 05 7A 47 83 FF 01 DD D8 D9 05 ? ? ? ? D8 25 ? ? ? ?").get_first<uint8_t>();
    const auto retry = hook::range_pattern(reinterpret_cast<uintptr_t>(timing), reinterpret_cast<uintptr_t>(timing + 0x70),
        "DD D8 D9 05 ? ? ? ? EB 1B D9 05").get_first<uint8_t>();
    // Preserve the elapsed-time integration and the maximum step. Present supplies the wait for the selected cap.
    injector::WriteMemory<const float*>(timing + 0x14, &minimumFrameTime, true);
    injector::WriteMemory<const float*>(timing + 0x26, &minimumFrameTime, true);
    injector::WriteMemory<const float*>(retry + 4, &minimumFrameTime, true);
}

void MatchMouseDecayToElapsedTime()
{
    static float mouseDecay = 0.95f;
    const auto grip = hook::pattern("F7 40 34 00 00 00 04 75 0C D8 0D ? ? ? ? D9 15 ? ? ? ? D8 15 ? ? ? ? DF E0 F6 C4 05 7A 16 8B 8C 91 D0 00 00 00 F7 41 34 00 00 00 02 75 06 D8 0D").get_first<uint8_t>();
    const auto interrogation = hook::pattern("DF E0 F6 C4 41 75 06 D8 0D ? ? ? ? D8 15 ? ? ? ? DF E0 F6 C4 05 7A 06 D8 0D ? ? ? ? D8 C1 8D 44 24 28").get_first<uint8_t>();
    const auto step = hook::pattern("D9 15 ? ? ? ? A1 ? ? ? ? D8 35 ? ? ? ? 8B 0D ? ? ? ? 51 40 A3 ? ? ? ? D9 1D ? ? ? ? 8B 15 ? ? ? ? 52 E8 ? ? ? ? A1 ? ? ? ? 83 C4 08 85 C0").get_first<uint8_t>();
    static const auto realElapsed = *reinterpret_cast<const float* const*>(step + 2);
    static auto update = safetyhook::create_mid(step + 0x30, [](SafetyHookContext&)
    {
        const float elapsed = *realElapsed;
        mouseDecay = std::isfinite(elapsed) ? std::pow(0.95f, std::clamp(elapsed, 0.0f, 0.25f) * 60.0f) : 0.95f;
    });

    // Only these two mouse interactions use the time-adjusted weight.
    injector::WriteMemory<const float*>(grip + 0x0B, &mouseDecay, true);
    injector::WriteMemory<const float*>(grip + 0x34, &mouseDecay, true);
    injector::WriteMemory<const float*>(interrogation + 0x09, &mouseDecay, true);
    injector::WriteMemory<const float*>(interrogation + 0x1C, &mouseDecay, true);
}

void InitPatches()
{
    hook::pattern pattern;
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    ObservePunisherSetting("MAIN.FPSLimit", UpdateFrameLimit);
    RemoveNativeFrameWait();
    MatchMouseDecayToElapsedTime();
    Screen.Width = PunisherNumericSetting("MAIN.ResX", 0);
    Screen.Height = PunisherNumericSetting("MAIN.ResY", 0);
    const bool skipIntro = settings.Get("MAIN.SkipIntro") != "0";
    ObservePunisherSetting("MAIN.HudAspectRatioConstraint", []()
    {
        const auto parsed = ParseWidescreenHudOffset(FixSettings().Get("MAIN.HudAspectRatioConstraint"));
        const float aspect = parsed && std::isfinite(*parsed) && *parsed > 0
            ? std::clamp(*parsed, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f;
        RequestedHudAspect.store(aspect, std::memory_order_release);
        HudCoordinatesPending.store(true, std::memory_order_release);
    });

    if (PunisherNumericSetting("MAIN.WindowMode", 0) == 2 || Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
    {
        Screen.Width = 640;
        Screen.Height = 480;
    }

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fAspectRatioDiff = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

    //Presets
    static constexpr int32_t heights[] = { 480, 600, 768, 960 };
    Screen.PresetHeight = heights[0];
    for (auto height : heights)
    {
        if (height <= Screen.Height)
            Screen.PresetHeight = height;
    }

    switch (Screen.PresetHeight)
    {
    case 480:
        Screen.PresetWidth = 640;
        break;
    case 600:
        Screen.PresetWidth = 800;
        break;
    case 768:
        Screen.PresetWidth = 1024;
        break;
    case 960:
        Screen.PresetWidth = 1280;
        break;
    default:
        Screen.PresetWidth = 1280;
        Screen.PresetHeight = 960;
        break;
    }

    Screen.PresetFactorX = Screen.fWidth / static_cast<float>(Screen.PresetWidth);
    Screen.PresetFactorY = Screen.fHeight / static_cast<float>(Screen.PresetHeight);

    InitHud();

    InitFrontend();

    if (skipIntro)
    {
        pattern = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 6A 00 E8 ? ? ? ?");
        injector::MakeNOP(pattern.get_first(), 5, true);
    }

    Camera::Init();

    Cutscene::Init();
    PunisherWindow::Init(Screen.Width, Screen.Height);
    PunisherOverlay::Init();
    PunMenuLayout::Init(Screen.Width, Screen.Height);
    PunMenu::Init();
    PunisherUpdates::Init();
    PunisherPad::Init();
}


class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += InitPatches;
    }
} Misc;

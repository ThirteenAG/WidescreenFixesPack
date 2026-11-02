module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Misc;

import ComVars;
import Camera;
import Window;
import Device;
import DeveloperOverlay;
import Movie;
import Cinema;
import Hud;
import Frontend;
import Updates;
import Resolution;
import Input;
import PostFX;
import Gamepad;
import FreeCamera;

SafetyHookInline sub_5EE430_hook{};
void __fastcall sub_5EE430(int _this, void* edx, float a2)
{
    static const auto& factor = FixSettings().Float("MAIN.FOVFactor");
    const uintptr_t controller = Controller.is_initialized()
        ? Controller.get() : 0;
    // Scripted camera flag of the game controller; horizontal scale of the frustum
    const bool scripted = controller && *reinterpret_cast<uint8_t*>(controller + 464);
    a2 = TrueCrimeCamera::Frustum(a2, *reinterpret_cast<float*>(_this + 12),
        TrueCrimeCinema::CameraAspect(Screen.fAspectRatio), scripted || TrueCrimeCinema::Active() ? 1.0f : factor);
    return sub_5EE430_hook.unsafe_fastcall<void>(_this, edx, a2);
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            asi_settings::LiveLink::Start();
            TrueCrimeWindow::Mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
            const auto [width, height] = TrueCrimeResolution::Startup();
            TrueCrimeResolution::Init(width, height);
            TrueCrimeWindow::Init(Screen.Width, Screen.Height);
            TrueCrimeOverlay::Init();
            TrueCrimeInput::Init();
            TrueCrimeGamepad::Init();
            TrueCrimeFreeCamera::Init();
            TrueCrimeUpdates::Init();
            TrueCrimeDevice::Initialize([]()
            {
                const int requested = FixSettings().Int("MAIN.FPSLimit");
                if (requested == -1)
                {
                    MONITORINFOEXW monitor{};
                    monitor.cbSize = sizeof(monitor);
                    DEVMODEW mode{};
                    mode.dmSize = sizeof(mode);
                    if (GetMonitorInfoW(MonitorFromWindow(TrueCrimeWindow::Window, MONITOR_DEFAULTTOPRIMARY), &monitor) &&
                        EnumDisplaySettingsW(monitor.szDevice, ENUM_CURRENT_SETTINGS, &mode))
                        return static_cast<int>(std::max(30ul, mode.dmDisplayFrequency));
                    return 60;
                }
                return requested <= 0 ? 0 : std::clamp(requested, 30, 1000);
            });
            TrueCrimeMovie::Init();
            TrueCrimeCinema::Init();
            TrueCrimeHud::Init();
            TrueCrimeFrontend::Init();
            TrueCrimePostFX::Init();

            // Projection: Hor+ field of view
            auto pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 64 F3 0F 10 75");
            if (pattern.empty())
                pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 64 D9 45 08 53 56 8B F1 D8 4E 04 8B 46 30 8B 4E 2C");
            sub_5EE430_hook = safetyhook::create_inline(pattern.get_first(0), sub_5EE430);
        };
    }
} Misc;

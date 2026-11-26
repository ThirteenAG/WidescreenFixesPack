module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/MaxPayne/FrameRate.hpp"

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
static std::atomic<float> GameplayFov{ 1.0f };
static std::atomic<int> RequestedFrameLimit{ 60 };

void __fastcall sub_5EE430(int _this, void* edx, float a2)
{
    const uintptr_t controller = Controller.is_initialized()
        ? Controller.get() : 0;
    // Scripted camera flag of the game controller; horizontal scale of the frustum
    const bool scripted = controller && *reinterpret_cast<uint8_t*>(controller + 464);
    a2 = TrueCrimeCamera::Frustum(a2, *reinterpret_cast<float*>(_this + 12),
        TrueCrimeCinema::CameraAspect(Screen.fAspectRatio), scripted || TrueCrimeCinema::Active() ? 1.0f : GameplayFov.load());
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
            TrueCrimeWindow::Mode = std::clamp(ReadNumericSetting("MAIN.WindowMode", 0), 0, 2);
            const auto [width, height] = TrueCrimeResolution::Startup();
            TrueCrimeResolution::Init(width, height);
            TrueCrimeWindow::Init(Screen.Width, Screen.Height);
            TrueCrimeOverlay::Init();
            TrueCrimeInput::Init();
            TrueCrimeGamepad::Init();
            TrueCrimeFreeCamera::Init();
            TrueCrimeUpdates::Init();
            const auto refreshFov = []()
            {
                static std::mutex mutex;
                const std::lock_guard lock(mutex);
                GameplayFov.store(ReadNumericSetting("MAIN.FOVFactor", 1.0f));
            };
            const auto refreshFrameLimit = []()
            {
                static std::mutex mutex;
                const std::lock_guard lock(mutex);
                RequestedFrameLimit.store(ReadNumericSetting("MAIN.FPSLimit", 60));
            };
            FixSettings().OnChange("MAIN.FOVFactor", refreshFov);
            FixSettings().OnChange("MAIN.FPSLimit", refreshFrameLimit);
            refreshFov();
            refreshFrameLimit();
            TrueCrimeDevice::Initialize([]()
            {
                const int requested = RequestedFrameLimit.load();
                if (requested == -1)
                    return static_cast<int>(MaxPayneFrameRate::MonitorRate(TrueCrimeWindow::Window.load()));
                return requested <= 0 ? 0 : std::clamp(requested, 30, 1000);
            });
            TrueCrimeMovie::Init();
            TrueCrimeCinema::Init();
            TrueCrimeHud::Init();
            TrueCrimeFrontend::Init();
            TrueCrimePostFX::Init();

            // Projection: Hor+ field of view
            auto pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 64 F3 0F 10 75");
            if (pattern.empty()) // TrueCrimeMB.exe
                pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 64 D9 45 08 53 56 8B F1 D8 4E 04 8B 46 30 8B 4E 2C");
            sub_5EE430_hook = safetyhook::create_inline(pattern.get_first(0), sub_5EE430);
        };
    }
} Misc;

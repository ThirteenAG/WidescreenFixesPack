module;

#include <stdafx.h>
#include <atomic>
#include "FixSettings.hpp"

export module FrameRate;

import ComVars;

// AI and physics misbehave above 60 FPS, so the limit never goes higher.
constexpr int MaxFrameRate = 60;

std::atomic<int> RequestedFrameLimit{ MaxFrameRate };

float FrameInterval()
{
    int limit = RequestedFrameLimit.load();
    if (limit <= 0)
    {
        static HMONITOR previousMonitor{};
        static ULONGLONG nextCheck{};
        static int refresh{ MaxFrameRate };
        const auto monitor = MonitorFromWindow(hGameWindow, MONITOR_DEFAULTTONEAREST);
        const auto now = GetTickCount64();
        if (monitor != previousMonitor || now >= nextCheck)
        {
            MONITORINFOEXW info{ sizeof(info) };
            DEVMODEW mode{ .dmSize = sizeof(mode) };
            if (GetMonitorInfoW(monitor, &info) && EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1)
                refresh = static_cast<int>(mode.dmDisplayFrequency);
            else if (monitor != previousMonitor)
                refresh = MaxFrameRate;
            previousMonitor = monitor;
            nextCheck = now + 500;
        }
        limit = refresh;
    }
    return 1.0f / float(std::clamp(limit, 1, MaxFrameRate));
}

class FrameRate
{
public:
    FrameRate()
    {
        WFP::onInitEvent() += []()
        {
            ObserveFixSetting("MAIN.FPSLimit", asi_settings::detail::ParseInt, [](int value) { RequestedFrameLimit.store(value); });

            // startup crash fix, same as the -ll command line switch
            {
                auto pattern = hook::pattern("74 ? 68 ? ? ? ? 53 FF D7");
                injector::MakeNOP(pattern.get_first(), 2, true);
            }

            auto framePattern = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 89 45");
            // Keep the elapsed time measured by the limiter, including any scheduler overshoot.
            // The native maximum-delta clamp still runs before simulation advances.
            auto waited = hook::pattern("83 C4 04 EB D1 8B 4D DC 89 4D EC EB 13 D9 45 EC D8 5D C4 DF E0 F6 C4 41 75 06 8B 55 C4 89 55 EC");
            injector::MakeNOP(waited.get_first(5), 8, true);
            static auto DeltaTimeHook = safetyhook::create_mid(framePattern.get_first(5), [](SafetyHookContext& regs)
            {
                const auto& state = UObject::controllerState;
                if (state.name == L"s_CrackSafe")
                {
                    *(float*)&regs.eax = 1.0f / 30.0f;
                    return;
                }

                if (bLoadingScreenActive.is_initialized() && bLoadingScreenActive)
                {
                    *(float*)&regs.eax = 1.0f / 9999.0f;
                    return;
                }

                *(float*)&regs.eax = FrameInterval();
            });
        };
    }
} FrameRate;

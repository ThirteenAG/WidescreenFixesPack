module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module FrameRate;

import ComVars;

// AI and physics misbehave above 60 FPS, so the limit never goes higher.
constexpr int MaxFrameRate = 60;

float FrameInterval = 0.0f;
HMONITOR FrameLimitMonitor = nullptr;
void UpdateFrameLimit()
{
    auto limit = FixSettings().Int("MAIN.FPSLimit");
    FrameLimitMonitor = MonitorFromWindow(hGameWindow, MONITOR_DEFAULTTOPRIMARY);
    if (limit <= 0)
    {
        MONITORINFOEXW monitor{ sizeof(monitor) };
        DEVMODEW mode{ .dmSize = sizeof(mode) };
        limit = GetMonitorInfoW(FrameLimitMonitor, &monitor) && EnumDisplaySettingsW(monitor.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1 ? int(mode.dmDisplayFrequency) : MaxFrameRate;
    }
    FrameInterval = 1.0f / float(std::clamp(limit, 1, MaxFrameRate));
}

class FrameRate
{
public:
    FrameRate()
    {
        WFP::onInitEvent() += []()
        {
            UpdateFrameLimit();
            FixSettings().OnChange("MAIN.FPSLimit", UpdateFrameLimit);
            WFP::onGameProcessEvent() += []()
            {
                if (FixSettings().Int("MAIN.FPSLimit") <= 0 && MonitorFromWindow(hGameWindow, MONITOR_DEFAULTTOPRIMARY) != FrameLimitMonitor)
                    UpdateFrameLimit();
            };

            // startup crash fix, same as the -ll command line switch
            {
                auto pattern = hook::pattern("74 ? 68 ? ? ? ? 53 FF D7");
                injector::MakeNOP(pattern.get_first(), 2, true);
            }

            auto framePattern = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 89 45");
            static auto DeltaTimeHook = safetyhook::create_mid(framePattern.get_first(5), [](SafetyHookContext& regs)
            {
                auto ECoopPlayerControllerWithLabel = UObject::GetState(L"ECoopPlayerControllerWithLabel");
                constexpr auto s_CrackSafeTurnLeft = L"s_CrackSafe224";
                constexpr auto s_CrackSafeTurnRight = L"s_CrackSafe248";

                if (ECoopPlayerControllerWithLabel == s_CrackSafeTurnLeft || ECoopPlayerControllerWithLabel == s_CrackSafeTurnRight)
                {
                    *(float*)&regs.eax = 1.0f / 30.0f;
                    return;
                }

                if (bLoadingScreenActive.is_initialized() && bLoadingScreenActive)
                {
                    *(float*)&regs.eax = 1.0f / 9999.0f;
                    return;
                }

                *(float*)&regs.eax = FrameInterval;
            });
        };
    }
} FrameRate;

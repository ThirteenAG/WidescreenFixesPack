module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module FrameRate;

import ComVars;

namespace
{
    float Interval()
    {
        const int limit = FixSettings().Int("MAIN.FPSLimit");
        if (limit == 0)
            return 0.0f;
        if (limit > 0)
            return 1.0f / std::clamp(limit, 30, 1000);
        static HMONITOR previous = nullptr;
        static ULONGLONG checked = 0;
        static float refresh = 60.0f;
        const auto monitor = MonitorFromWindow(hGameWindow, MONITOR_DEFAULTTOPRIMARY);
        const auto now = GetTickCount64();
        if (monitor != previous || now - checked > 2000)
        {
            previous = monitor;
            checked = now;
            MONITORINFOEXW info{};
            info.cbSize = sizeof(info);
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            if (GetMonitorInfoW(monitor, &info) && EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1)
                refresh = static_cast<float>(mode.dmDisplayFrequency);
        }
        return 1.0f / refresh;
    }
}

export void InitFrameRate()
{
    // The native main loop caches its minimum interval at startup. Refresh it before each frame's pacing.
    auto pattern = hook::pattern("DD 55 D0 DC 65 C8 8B CF DD 55 B8 D9 5D EC");
    static auto frame = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        *reinterpret_cast<float*>(regs.ebp - 0x24) = Interval();
    });
}

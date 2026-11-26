#pragma once

#include <algorithm>
#include <atomic>
#include <charconv>
#include <windows.h>

namespace SplinterCellFrameLimit
{
    inline std::atomic<int> requested{ -1 };
    inline float minFrameTime = 1.0f / 60.0f;

    inline void ReadSetting()
    {
        const auto text = FixSettings().Get("MAIN.FPSLimit");
        int value = -1;
        std::from_chars(text.data(), text.data() + text.size(), value);
        requested.store(value);
    }

    // Only the engine thread writes the float used by the native frame limiter.
    inline void Update(HWND window)
    {
        int limit = requested.load();
        if (limit < 0)
        {
            static HMONITOR lastMonitor = nullptr;
            static ULONGLONG nextCheck = 0;
            static int refresh = 60;
            const auto monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
            const auto now = GetTickCount64();
            if (monitor != lastMonitor || now >= nextCheck)
            {
                MONITORINFOEXW info{};
                info.cbSize = sizeof(info);
                DEVMODEW mode{};
                mode.dmSize = sizeof(mode);
                if (GetMonitorInfoW(monitor, &info) && EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) &&
                    mode.dmDisplayFrequency > 1)
                    refresh = static_cast<int>(mode.dmDisplayFrequency);
                else if (monitor != lastMonitor)
                    refresh = 60;
                lastMonitor = monitor;
                nextCheck = now + 500;
            }
            minFrameTime = 1.0f / static_cast<float>(refresh);
            return;
        }
        minFrameTime = limit > 0 ? 1.0f / static_cast<float>(std::clamp(limit, 30, 1000)) : 0.0f;
    }
}

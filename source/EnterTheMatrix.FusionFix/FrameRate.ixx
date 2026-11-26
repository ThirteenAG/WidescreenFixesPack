module;

#include <stdafx.h>
#include <atomic>
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"

export module FrameRate;

export namespace MatrixFrameRate
{
    std::atomic<int> Requested{ 0 };

    void Init()
    {
        WatchMatrixNumber("MAIN.FPSLimit", Requested);
    }

    void Frame(HWND window)
    {
        const int requested = Requested.load(std::memory_order_relaxed);
        if (requested >= 0)
        {
            D3D9Options::frameLimit = requested ? std::clamp(requested, 30, 1000) : 0;
            return;
        }
        static HMONITOR previous = nullptr;
        static ULONGLONG lastCheck = 0;
        static int refresh = 60;
        const auto monitor = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
        const auto now = GetTickCount64();
        if (monitor != previous || now - lastCheck >= 2000)
        {
            previous = monitor;
            lastCheck = now;
            MONITORINFOEXW info{};
            info.cbSize = sizeof(info);
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            refresh = GetMonitorInfoW(monitor, &info) && EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1 ? int(mode.dmDisplayFrequency) : 60;
        }
        D3D9Options::frameLimit = refresh;
    }
}

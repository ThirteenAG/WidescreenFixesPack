#pragma once

#include <Windows.h>

namespace RWGTA
{
    inline int WindowRefreshRate(HWND window)
    {
        static HMONITOR monitor = nullptr;
        static ULONGLONG nextQuery = 0;
        static int rate = 60;
        const auto current = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
        const auto now = GetTickCount64();
        if (current == monitor && now < nextQuery)
            return rate;

        monitor = current;
        nextQuery = now + 1000;
        MONITORINFOEXW info{ sizeof(info) };
        DEVMODEW mode{};
        mode.dmSize = sizeof(mode);
        const auto display = GetMonitorInfoW(current, &info) ? info.szDevice : nullptr;
        rate = EnumDisplaySettingsW(display, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency >= 30 ?
            static_cast<int>(mode.dmDisplayFrequency) : 60;
        return rate;
    }
}

#pragma once
#include <windows.h>
#include <algorithm>
#include <cstdint>

[[nodiscard]] uint32_t GetDesktopRefreshRate() noexcept;

// The frame rate limit of both Max Payne games (MAIN.FPSLimit): 0 = unlimited (the games' own behaviour, they run on
// variable frame times without a cap), -1 = the monitor's refresh rate, else frames per second, 30 to 1000. nMaxFPS caps
// the refresh rate and the number.
// Called once a frame before the frame is shown. Schedule from the previous frame's actual time and carry the
// fractional counter tick so delays cannot bunch up subsequent frames.
namespace MaxPayneFrameRate
{
    inline LONGLONG Frequency()
    {
        static LONGLONG frequency = []() { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f.QuadPart; }();
        return frequency;
    }

    inline LONGLONG Now()
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return now.QuadPart;
    }

    // Sleeps on a high resolution timer until shortly before the target, then spins the rest
    inline void WaitUntil(LONGLONG target)
    {
        static HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

        constexpr double SpinTime = 0.0015;
        double seconds = double(target - Now()) / double(Frequency());
        if (seconds > SpinTime)
        {
            LARGE_INTEGER due;
            due.QuadPart = -static_cast<LONGLONG>((seconds - SpinTime) * 10000000.0);
            if (timer && SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
                WaitForSingleObject(timer, INFINITE);
            else
                Sleep(static_cast<DWORD>((seconds - SpinTime) * 1000.0));
        }

        while (Now() < target)
            YieldProcessor();
    }

    inline uint32_t MonitorRate(HWND window)
    {
        static HMONITOR previousMonitor = nullptr;
        static uint32_t refresh = 60;
        static ULONGLONG queryAfter = 0;
        const auto monitor = window ? MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST) : nullptr;
        const auto now = GetTickCount64();
        if (monitor != previousMonitor || now >= queryAfter)
        {
            previousMonitor = monitor;
            queryAfter = now + 500;
            MONITORINFOEXW info{};
            info.cbSize = sizeof(info);
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            if (monitor && GetMonitorInfoW(monitor, &info) &&
                EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1)
            {
                refresh = mode.dmDisplayFrequency;
            }
            else
            {
                const auto desktop = GetDesktopRefreshRate();
                refresh = desktop > 1 ? desktop : 60;
            }
        }
        return refresh;
    }

    inline void Limit(int32_t nFPSLimit, int32_t nMaxFPS = 1000, HWND window = nullptr)
    {
        static LONGLONG previousPresent = 0;
        static LONGLONG remainder = 0;
        static int32_t previousFPS = 0;

        if (nFPSLimit == 0)
        {
            previousFPS = 0;
            return;
        }

        const auto requested = nFPSLimit < 0 ? int32_t(MonitorRate(window)) : std::clamp(nFPSLimit, 30, 1000);
        const int32_t nFPS = std::clamp(requested, 1, std::max<int32_t>(nMaxFPS, 1));
        auto now = Now();
        if (!previousFPS)
        {
            previousFPS = nFPS;
            previousPresent = now;
            remainder = 0;
            return;
        }
        if (nFPS != previousFPS)
        {
            previousFPS = nFPS;
            remainder = 0;
        }

        const auto ticks = Frequency() + remainder;
        const auto target = previousPresent + ticks / nFPS;
        remainder = ticks % nFPS;
        if (now < target)
        {
            WaitUntil(target);
            now = Now();
        }
        previousPresent = now;
    }
}

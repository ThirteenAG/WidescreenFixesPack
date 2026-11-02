module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"
#include <atomic>
#include <intrin.h>


export module FrameRate;

import ComVars;

export namespace DPLFrameRate
{
    HWND Window = nullptr;
    LARGE_INTEGER frequency{};
    HANDLE timer = nullptr;
    double deadline = 0.0;
    int previousLimit = -2;
    HMONITOR previousMonitor = nullptr;
    DWORD refreshTick = 0;
    int refreshRate = 60;
    int64_t statisticsStart = 0;
    uint32_t statisticsFrames = 0;
    float actualFPS = 0.0f;
    int MonitorRate()
    {
        const auto monitor = MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY);
        const auto now = GetTickCount();
        if (monitor != previousMonitor || now - refreshTick >= 1000)
        {
            previousMonitor = monitor;
            refreshTick = now;
            MONITORINFOEXW info{};
            info.cbSize = sizeof(info);
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            refreshRate = GetMonitorInfoW(monitor, &info) &&
                EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1 ?
                std::clamp<int>(mode.dmDisplayFrequency, 30, 1000) : 60;
        }
        return refreshRate;
    }

    int64_t Counter()
    {
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        return now.QuadPart;
    }

    void Pace(int limit)
    {
        if (limit == -1) limit = MonitorRate();
        const auto now = Counter();
        if (limit <= 0)
        {
            deadline = 0.0;
            previousLimit = 0;
            return;
        }
        const double interval = static_cast<double>(frequency.QuadPart) / limit;
        if (limit != previousLimit || deadline <= 0.0 || now > deadline + interval * 4.0)
            deadline = static_cast<double>(now);
        previousLimit = limit;
        for (;;)
        {
            const double remaining = (deadline - Counter()) / frequency.QuadPart;
            if (remaining <= 0.0) break;
            if (remaining > 0.001)
            {
                LARGE_INTEGER due{};
                due.QuadPart = -std::max<int64_t>(1, static_cast<int64_t>((remaining - 0.0003) * 10000000.0));
                if (timer && SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
                    WaitForSingleObject(timer, INFINITE);
                else
                    Sleep(static_cast<DWORD>(std::max(1.0, remaining * 1000.0 - 1.0)));
            }
            else
                YieldProcessor();
        }
        deadline = std::max(deadline + interval, static_cast<double>(Counter()) + interval);
    }


    void Frame(HWND window)
    {
        static const bool initialized = []()
        {
            QueryPerformanceFrequency(&frequency);
            timer = CreateWaitableTimerExW(nullptr, nullptr, 0x2, TIMER_ALL_ACCESS);
            if (!timer) timer = CreateWaitableTimerW(nullptr, FALSE, nullptr);
            return frequency.QuadPart > 0;
        }();
        if (!initialized) return;
        Window = window;
        const int requested = FixSettings().Int("MAIN.FPSLimit");
        Pace(requested == -1 || requested == 0 ? requested : std::clamp(requested, 30, 1000));
        const auto now = Counter();
        ++statisticsFrames;
        if (!statisticsStart) { statisticsStart = now; statisticsFrames = 0; }
        else if (now - statisticsStart >= frequency.QuadPart)
        {
            actualFPS = static_cast<float>(statisticsFrames * static_cast<double>(frequency.QuadPart) / (now - statisticsStart));
            statisticsStart = now; statisticsFrames = 0;
        }
    }
}

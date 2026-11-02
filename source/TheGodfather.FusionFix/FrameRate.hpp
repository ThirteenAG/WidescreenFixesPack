#pragma once
#include <windows.h>
#include <algorithm>
#include <cstdint>

namespace GodfatherFrameRate
{
    inline HWND(*window)(){};

    inline uint32_t RefreshRate(HWND target)
    {
        MONITORINFOEXW monitor{};
        monitor.cbSize = sizeof(monitor);
        DEVMODEW mode{};
        mode.dmSize = sizeof(mode);
        return GetMonitorInfoW(MonitorFromWindow(target, MONITOR_DEFAULTTOPRIMARY), &monitor)
            && EnumDisplaySettingsExW(monitor.szDevice, ENUM_CURRENT_SETTINGS, &mode, 0) && mode.dmDisplayFrequency > 1
            ? mode.dmDisplayFrequency : 60;
    }

    inline LONGLONG Frequency()
    {
        static const LONGLONG frequency = []() { LARGE_INTEGER value{}; QueryPerformanceFrequency(&value); return value.QuadPart; }();
        return frequency;
    }

    inline LONGLONG Now()
    {
        LARGE_INTEGER value{};
        QueryPerformanceCounter(&value);
        return value.QuadPart;
    }

    struct State
    {
        HANDLE timer = nullptr;
        bool timerChecked = false;
        LONGLONG next = 0;
        int rate = 0;
        uint32_t refresh = 60;
        ULONGLONG checked = 0;
        ~State()
        {
            if (timer)
                CloseHandle(timer);
        }
    };

    inline void WaitUntil(State& state, LONGLONG target)
    {
        const auto frequency = Frequency();
        const auto remaining = target - Now();
        if (frequency <= 0 || remaining <= 0 || remaining > frequency)
            return;
        const auto sleep = remaining - frequency / 1000;
        if (sleep > 0)
        {
            if (!state.timerChecked)
            {
                state.timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
                state.timerChecked = true;
            }
            LARGE_INTEGER due{ .QuadPart = -(sleep * 10000000 / frequency) };
            if (state.timer && SetWaitableTimer(state.timer, &due, 0, nullptr, nullptr, FALSE))
                WaitForSingleObject(state.timer, 1000);
            else
                Sleep(DWORD(sleep * 1000 / frequency));
        }
        while (Now() < target)
            YieldProcessor();
    }

    inline int Rate(int limit, uint32_t refresh)
    {
        return limit < 0 ? int(std::clamp(refresh, 1u, 1000u)) : std::clamp(limit, 30, 1000);
    }

    inline void Limit(int limit)
    {
        thread_local State state;
        if (!limit || Frequency() <= 0)
        {
            state.next = 0;
            state.rate = 0;
            return;
        }
        if (limit < 0 && GetTickCount64() - state.checked > 2000)
        {
            state.refresh = RefreshRate(window ? window() : nullptr);
            state.checked = GetTickCount64();
        }
        const auto rate = Rate(limit, state.refresh);
        const auto period = Frequency() / rate;
        const auto now = Now();
        if (!state.next || state.rate != rate || now - state.next > period * 4)
        {
            state.next = now + period;
            state.rate = rate;
            return;
        }
        WaitUntil(state, state.next);
        state.next += period;
    }
}

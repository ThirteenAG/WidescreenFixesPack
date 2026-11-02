module;
#include <stdafx.h>
#include <windows.h>
#include <algorithm>
#include <cstdint>

export module FrameRate;


export namespace PsiOpsFrameRate
{
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
        HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
        LONGLONG next = 0;
        int rate = 0;
        uint32_t refresh = 60;
        ULONGLONG checked = 0;
        HMONITOR monitor = nullptr;
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
        if (frequency <= 0 || remaining <= 0 || remaining > frequency / 4)
            return;
        const auto sleep = remaining - frequency / 1000;
        if (sleep > 0)
        {
            LARGE_INTEGER due{ .QuadPart = -(sleep * 10000000 / frequency) };
            if (state.timer && SetWaitableTimer(state.timer, &due, 0, nullptr, nullptr, FALSE))
                WaitForSingleObject(state.timer, 100);
            else
                Sleep(DWORD(sleep * 1000 / frequency));
        }
        while (Now() < target)
            YieldProcessor();
    }

    void Limit(int limit, HWND window)
    {
        thread_local State state;
        if (!limit || Frequency() <= 0)
        {
            state.next = 0;
            state.rate = 0;
            return;
        }
        if (limit < 0)
        {
            const auto monitor = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
            const auto now = GetTickCount64();
            if (monitor != state.monitor || now - state.checked >= 2000)
            {
                state.monitor = monitor;
                state.checked = now;
                MONITORINFOEXW info{};
                info.cbSize = sizeof(info);
                DEVMODEW mode{};
                mode.dmSize = sizeof(mode);
                state.refresh = GetMonitorInfoW(monitor, &info) &&
                    EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1 ?
                    mode.dmDisplayFrequency : 60;
            }
        }
        const auto rate = std::clamp(limit < 0 ? int(state.refresh) : limit, 30, 1000);
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


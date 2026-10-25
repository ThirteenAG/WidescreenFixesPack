#pragma once
#include <windows.h>
#include <algorithm>
#include <cstdint>

uint32_t GetDesktopRefreshRate();

// The frame rate limit of both Max Payne games (MAIN.FPSLimit): 0 = unlimited (the games' own behaviour, they run on
// variable frame times without a cap), -1 = the monitor's refresh rate, else frames per second, 30 to 1000.
// Called once a frame before the frame is shown. Each frame gets its time slot after the one before, so the pace
// doesn't drift; a frame that came late (loading, a hitch) starts the count anew instead of rushing the next ones.
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

    inline void Limit(int32_t nFPSLimit)
    {
        static LONGLONG next = 0;
        if (nFPSLimit == 0)
        {
            next = 0;
            return;
        }

        // the refresh rate is looked up again now and then, it changes with the video mode
        static uint32_t nRefreshRate = 60;
        static ULONGLONG nRefreshRateTime = 0;
        if (nFPSLimit < 0 && GetTickCount64() - nRefreshRateTime > 2000)
        {
            nRefreshRate = GetDesktopRefreshRate();
            if (nRefreshRate < 30)
                nRefreshRate = 60;
            nRefreshRateTime = GetTickCount64();
        }

        int32_t nFPS = nFPSLimit < 0 ? int32_t(nRefreshRate) : std::clamp(nFPSLimit, 30, 1000);
        LONGLONG period = Frequency() / nFPS;
        LONGLONG now = Now();
        if (next == 0 || now - next > period * 4)
        {
            next = now + period;
            return;
        }

        WaitUntil(next);
        next += period;
    }
}

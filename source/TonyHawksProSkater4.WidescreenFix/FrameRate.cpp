#include "stdafx.h"
#include "FixSettings.hpp"
#include "FrameRate.hpp"

namespace
{
    int FrameLimit = 0;

    struct FramePacer
    {
        HANDLE timer = nullptr;
        double frequency = 0.0;
        double next = 0.0;

        FramePacer()
        {
            LARGE_INTEGER value{};
            QueryPerformanceFrequency(&value);
            frequency = static_cast<double>(value.QuadPart);
            timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
            if (!timer)
                timer = CreateWaitableTimerW(nullptr, FALSE, nullptr);
        }

        ~FramePacer()
        {
            if (timer)
                CloseHandle(timer);
        }

        double Now() const
        {
            LARGE_INTEGER value{};
            QueryPerformanceCounter(&value);
            return static_cast<double>(value.QuadPart);
        }

        void Wait()
        {
            if (frequency <= 0.0)
                return;
            const double period = frequency / FrameLimit;
            const double now = Now();
            if (next == 0.0 || now > next + period)
                next = now + period;
            const double seconds = (next - now) / frequency;
            constexpr double spinTime = 0.0015;
            if (seconds > spinTime)
            {
                LARGE_INTEGER due{};
                due.QuadPart = -static_cast<LONGLONG>((seconds - spinTime) * 10000000.0);
                if (timer && SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
                    WaitForSingleObject(timer, INFINITE);
                else
                    Sleep(static_cast<DWORD>((seconds - spinTime) * 1000.0));
            }
            while (Now() < next)
                YieldProcessor();
            next += period;
        }
    };
}

void InitFrameRateLimit()
{
    int limit = FixSettings().Int("MAIN.FPSLimit");
    if (limit == -1)
    {
        DEVMODEW mode{};
        mode.dmSize = sizeof(mode);
        limit = EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1
            ? static_cast<int>(mode.dmDisplayFrequency) : 60;
    }
    FrameLimit = limit > 0 ? std::clamp(limit, 30, 1000) : 0;
}

void SyncFrameRateLimit()
{
    if (FrameLimit <= 0)
        return;
    static FramePacer pacer;
    pacer.Wait();
}

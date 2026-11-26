#pragma once

#include <windows.h>
#include <chrono>

namespace ResidentEvil
{
    class SlowFramePacer
    {
        using Clock = std::chrono::steady_clock;
        using Seconds = std::chrono::duration<double>;
        using Milliseconds = std::chrono::milliseconds;

    public:
        ~SlowFramePacer()
        {
            if (timer)
                CloseHandle(timer);
        }

        void Limit(int fps)
        {
            if (fps <= 0)
            {
                previousLimit = 0;
                return;
            }

            const auto now = Clock::now();
            const auto deadline = previousFrame + std::chrono::duration_cast<Clock::duration>(Seconds(1.0 / fps));
            if (fps == previousLimit && now < deadline)
            {
                const auto remaining = deadline - now;
                constexpr Milliseconds spinTime{ 2 };
                if (remaining > spinTime)
                {
                    if (!timer)
                        timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
                    const auto sleepTime = remaining - spinTime;
                    const auto milliseconds = std::chrono::duration_cast<Milliseconds>(sleepTime).count();
                    LARGE_INTEGER due{};
                    due.QuadPart = -std::chrono::duration_cast<std::chrono::duration<LONGLONG, std::ratio<1, 10000000>>>(sleepTime).count();
                    if (timer && due.QuadPart < 0 && SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
                        WaitForSingleObject(timer, static_cast<DWORD>(milliseconds) + 4);
                    else
                        Sleep(static_cast<DWORD>(milliseconds));
                }
                while (Clock::now() < deadline)
                    YieldProcessor();
            }

            previousLimit = fps;
            previousFrame = Clock::now();
        }

    private:
        int previousLimit{};
        Clock::time_point previousFrame{};
        HANDLE timer{};
    };
}

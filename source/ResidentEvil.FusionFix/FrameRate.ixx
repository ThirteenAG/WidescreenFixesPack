module;

#include <stdafx.h>
#include <atomic>
#include <intrin.h>
#include "FixSettings.hpp"

export module FrameRate;

export namespace REFrameRate
{
    HWND Window = nullptr;
    SafetyHookMid variableClock;
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
    std::atomic<bool> variable{ false };
    std::atomic<int> requestedLimit{ 60 };
    bool updatedRelease = false;
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
                // Bounded wait: a frame never waits longer than its own interval, whatever happens to the timer
                if (timer && SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
                    WaitForSingleObject(timer, static_cast<DWORD>(remaining * 1000.0) + 2);
                else
                    Sleep(static_cast<DWORD>(std::max(1.0, remaining * 1000.0 - 1.0)));
            }
            else
                YieldProcessor();
        }
        deadline += interval;
        const auto wake = Counter();
        if (deadline <= wake)
            deadline = wake + interval;
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
        // The clock sets the flag again on every update while Variable mode runs
        if (variable.exchange(false, std::memory_order_relaxed))
        {
            Pace(requestedLimit.load(std::memory_order_relaxed));
        }
        else
            deadline = 0.0;
        // The measured rate is shown only by the developer overlay
        if constexpr (!WFP::DeveloperOverlay)
            return;
        const auto now = Counter();
        ++statisticsFrames;
        if (!statisticsStart) { statisticsStart = now; statisticsFrames = 0; }
        else if (now - statisticsStart >= frequency.QuadPart)
        {
            actualFPS = static_cast<float>(statisticsFrames * static_cast<double>(frequency.QuadPart) / (now - statisticsStart));
            statisticsStart = now; statisticsFrames = 0;
        }
    }
    void Install()
    {
        BindLiveInt("MAIN.FPSLimit", [](int limit)
        {
            requestedLimit.store(limit == -1 || limit == 0 ? limit : std::clamp(limit, 30, 1000), std::memory_order_relaxed);
        });

        auto current = hook::pattern("F3 0F 10 4D 38 F3 0F 5E 4D 3C F2 0F 59 44 24 ? 0F 28 D1");
        auto original = hook::pattern("F3 0F 10 4B 38 F3 0F 5E 4B 3C 66 0F 5A C0 F3 0F 11 4C 24 ? F3 0F 5C C8");
        updatedRelease = current.size() == 1;
        auto* site = updatedRelease ? current.get_first(10) : original.get_first(10);
        variableClock = safetyhook::create_mid(site, [](SafetyHookContext& regs)
        {
            // Only Variable mode reaches this threshold; its target is 120. Movies keep this mode but set their own
            // 30 fps target, so they stay native. Preserve elapsed time, time scale and fixed 30/60 clocks.
            const auto clock = updatedRelease ? regs.ebp : regs.ebx;
            const bool unlocked = *reinterpret_cast<const float*>(clock + 0x3C) == 120.0f;
            variable.store(unlocked, std::memory_order_relaxed);
            if (unlocked)
                regs.xmm1.f32[0] = 0.0f;
        });
    }

    void Shutdown()
    {
        if (timer) CloseHandle(timer);
        timer = nullptr;
    }

}

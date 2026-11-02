module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <array>
#include <atomic>
#include <intrin.h>

export module FrameRate;

import ComVars;

namespace FrameRate
{
    SafetyHookInline timerHook;
    std::atomic<int> requested{ 60 };
    std::array<uintptr_t, 2> frameCallers{};
    LARGE_INTEGER frequency{};
    HANDLE timer = nullptr;
    double deadline = 0.0;
    int previousLimit = -2;
    HMONITOR previousMonitor = nullptr;
    DWORD refreshTick = 0;
    int refreshRate = 60;
    int64_t statisticsStart = 0;
    uint32_t statisticsFrames = 0;
    std::atomic<float> actualFPS{ 0.0f };

    int MonitorRate()
    {
        const auto monitor = MonitorFromWindow(WindowedModeWrapper::GameHWND, MONITOR_DEFAULTTOPRIMARY);
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
        deadline += interval;
    }

    uint32_t WINAPI Update(void* graph, uint32_t minimum, uint32_t maximum)
    {
        const auto caller = reinterpret_cast<uintptr_t>(_ReturnAddress());
        if (caller != frameCallers[0] && caller != frameCallers[1])
            return timerHook.unsafe_stdcall<uint32_t>(graph, minimum, maximum);
        Pace(requested.load(std::memory_order_relaxed));
        const auto now = Counter();
        ++statisticsFrames;
        if (!statisticsStart)
        {
            statisticsStart = now;
            statisticsFrames = 0;
        }
        else if (now - statisticsStart >= frequency.QuadPart)
        {
            actualFPS.store(static_cast<float>(statisticsFrames * static_cast<double>(frequency.QuadPart) /
                (now - statisticsStart)), std::memory_order_relaxed);
            statisticsStart = now;
            statisticsFrames = 0;
        }
        // Keep the engine's integer-millisecond simulation delta positive.
        // Its original maximum still bounds a delayed or resumed frame.
        return timerHook.unsafe_stdcall<uint32_t>(graph, 1u, maximum);
    }
}

export void InitFrameRate()
{
    using namespace FrameRate;
    auto site = hook::pattern("6A 10 8B 08 50 FF 91 F0 00 00 00 33 ED 89 44 24 10");
    const bool old = !site.empty(); // 1.0
    if (!old)
        site = hook::pattern("6A 10 8B 08 50 FF 91 00 01 00 00 33 ED 89 44 24 10"); // 1.1/1.2
    auto call = reinterpret_cast<uintptr_t>(site.get_first());
    auto race = old ? hook::pattern("6A 53 6A 01 50 8B 08 FF 91 F0 00 00 00 89 44 24 ?")
        : hook::pattern("6A 53 6A 01 50 8B 08 FF 91 00 01 00 00 89 44 24 ?");
    auto raceCall = reinterpret_cast<uintptr_t>(race.get_first());
    auto engine = GetLS3DF();
    using GetGraph = void* (__cdecl*)();
    auto getGraph = engine ? reinterpret_cast<GetGraph>(GetProcAddress(engine, "GetIGraph")) : nullptr;
    auto graph = getGraph ? getGraph() : nullptr;
    if (!graph || !QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0)
    {
        OutputDebugStringA("Mafia: frame pacing API unavailable.\n");
        FixSettings().Availability("MAIN.FPSLimit", [](std::string_view) { return false; });
        return;
    }
    const auto offset = old ? 0xF0u : 0x100u;
    frameCallers = { call + 11, raceCall + 13 };
    auto table = *reinterpret_cast<void***>(graph);
    timerHook = safetyhook::create_inline(table[offset / sizeof(uintptr_t)], Update);
    if (!timerHook)
    {
        OutputDebugStringA("Mafia: frame pacing hook unavailable.\n");
        FixSettings().Availability("MAIN.FPSLimit", [](std::string_view) { return false; });
        return;
    }
    timer = CreateWaitableTimerExW(nullptr, nullptr, 0x2, TIMER_ALL_ACCESS);
    if (!timer) timer = CreateWaitableTimerW(nullptr, FALSE, nullptr);
    const auto changed = []()
    {
        int value = FixSettings().Int("MAIN.FPSLimit");
        FrameRate::requested.store(value == -1 || value == 0 ? value : std::clamp(value, 30, 1000), std::memory_order_relaxed);
    };
    changed();
    FixSettings().OnChange("MAIN.FPSLimit", changed);
}

export float GetFrameRate()
{
    return FrameRate::actualFPS.load(std::memory_order_relaxed);
}

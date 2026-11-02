module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include <atomic>

export module Fixes;

import ComVars;

void* (__thiscall* pCreateFrameTimer)(void*, float, float) = nullptr;

SafetyHookInline shSwitchTimeOfDay;
void __cdecl SwitchTimeOfDay(int)
{
    // Native preset 2 selects the rainy sky, lighting and water environment.
    // Let the engine create/update its resources at the usual setup points.
    shSwitchTimeOfDay.ccall<void>(2);
}

float fFPSLimit = 60.0f;
float* FrameTimer = nullptr;
void(__thiscall* pResetFrameTimer)(void*) = nullptr;
std::atomic<bool> FrameLimitPending{ false };
HMONITOR FrameLimitMonitor = nullptr;
bool FrameLimitFollowsMonitor = false;
SafetyHookInline shUpdateFrameTimer;

struct FrameClockState
{
    void* timer = nullptr;
    LONGLONG origin = 0, counter = 0;
    double rolled = 0.0;
    float elapsed = 0.0f;
    HANDLE waitable = nullptr;
    bool waitableAttempted = false;
} FrameClock;

void WaitForFrameDeadline(LARGE_INTEGER& now, LONGLONG deadline, LONGLONG frequency)
{
    if (deadline <= now.QuadPart || deadline - now.QuadPart > frequency / 10)
        return;
    if (!FrameClock.waitableAttempted)
    {
        FrameClock.waitableAttempted = true;
        FrameClock.waitable = CreateWaitableTimerExW(nullptr, nullptr, 2, TIMER_MODIFY_STATE | SYNCHRONIZE);
    }
    while (now.QuadPart < deadline)
    {
        const auto seconds = static_cast<double>(deadline - now.QuadPart) / frequency;
        if (seconds > 0.001 && FrameClock.waitable)
        {
            LARGE_INTEGER due{};
            due.QuadPart = -static_cast<LONGLONG>((seconds - 0.0003) * 10000000.0);
            if (SetWaitableTimer(FrameClock.waitable, &due, 0, nullptr, nullptr, FALSE))
                WaitForSingleObject(FrameClock.waitable, static_cast<DWORD>(std::ceil(seconds * 1000.0)) + 1);
            else
                Sleep(0);
        }
        else if (seconds > 0.001)
            Sleep(0);
        else
            YieldProcessor();
        if (!QueryPerformanceCounter(&now))
            return;
    }
}

double __fastcall UpdateFrameTimer(void* timer, void*)
{
    auto fields = static_cast<float*>(timer);
    auto counters = static_cast<LARGE_INTEGER*>(timer);
    const auto precise = timer && timer == FrameTimer && reinterpret_cast<uint32_t*>(timer)[9] &&
        counters[0].QuadPart > 0 && !counters[2].QuadPart && std::isfinite(fields[15]);
    double before = 0.0;
    bool corrected = false;
    if (precise)
    {
        LARGE_INTEGER now{};
        if (QueryPerformanceCounter(&now) && now.QuadPart >= counters[1].QuadPart)
        {
            if (FrameClock.timer != timer || FrameClock.counter != counters[1].QuadPart || FrameClock.elapsed != fields[15])
            {
                FrameClock.timer = timer;
                FrameClock.origin = counters[1].QuadPart;
                FrameClock.rolled = -static_cast<double>(fields[15]);
            }
            if (fFPSLimit > 0.0f && reinterpret_cast<const uint8_t*>(timer)[56])
            {
                const auto seconds = FrameClock.rolled + static_cast<double>(fields[16]) + fields[12];
                const auto deadline = FrameClock.origin + static_cast<LONGLONG>(std::ceil(seconds * counters[0].QuadPart));
                WaitForFrameDeadline(now, deadline, counters[0].QuadPart);
            }
            // Rebase from an integer epoch: repeated float additions lose time during the polling loop.
            before = static_cast<double>(now.QuadPart - FrameClock.origin) / counters[0].QuadPart - FrameClock.rolled;
            injector::WriteMemory<float>(fields + 15, static_cast<float>(before), true);
            injector::WriteMemory<LARGE_INTEGER>(counters + 1, now, true);
            corrected = true;
        }
    }
    if (timer == FrameTimer && fFPSLimit == 0.0f)
    {
        // Permit render-only frames between updates without changing simulation time.
        injector::WriteMemory<uint8_t>(reinterpret_cast<uint8_t*>(timer) + 56, 0, true);
    }
    const auto result = shUpdateFrameTimer.thiscall<double>(timer);
    if (corrected)
    {
        if (fields[15] < before - 0.5)
            FrameClock.rolled += 1.0;
        FrameClock.counter = counters[1].QuadPart;
        FrameClock.elapsed = fields[15];
    }
    return result;
}

float FrameRateLimit()
{
    auto limit = FixSettings().Int("MAIN.FPSLimit");
    FrameLimitFollowsMonitor = limit == -1;
    if (limit == -1)
    {
        MONITORINFOEXW monitor{ sizeof(monitor) };
        DEVMODEW mode{};
        mode.dmSize = sizeof(mode);
        FrameLimitMonitor = MonitorFromWindow(hWnd.get(), MONITOR_DEFAULTTOPRIMARY);
        if (GetMonitorInfoW(FrameLimitMonitor, &monitor) &&
            EnumDisplaySettingsW(monitor.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1)
            return static_cast<float>(std::clamp<DWORD>(mode.dmDisplayFrequency, 30, 1000));
        return 60.0f;
    }
    return limit == 0 ? 0.0f : static_cast<float>(std::clamp(limit, 30, 1000));
}

void SetFrameTimerRate()
{
    FrameClock.timer = nullptr;
    if (!FrameTimer)
        return;
    injector::WriteMemory<float>(FrameTimer + 10, fFPSLimit > 0.0f ? fFPSLimit : 1000.0f, true);
    injector::WriteMemory<float>(FrameTimer + 11, fFPSLimit > 0.0f ? fFPSLimit : 30.0f, true);
    // Keep a positive update interval so the native timer can emit render-only ticks.
    injector::WriteMemory<float>(FrameTimer + 12, fFPSLimit > 0.0f ? 1.0f / fFPSLimit : 1.0f / 1000.0f, true);
    injector::WriteMemory<float>(FrameTimer + 13, fFPSLimit > 0.0f ? 1.0f / fFPSLimit : 1.0f / 30.0f, true);
}

void* __fastcall CreateFrameTimer(void* timer, void*, float, float)
{
    // nglSetFrameLock normally constructs a timer at 60 / frameLock FPS.
    // Supply the configured rate directly, for both frame-lock modes.
    fFPSLimit = FrameRateLimit();
    auto result = pCreateFrameTimer(timer, fFPSLimit > 0.0f ? fFPSLimit : 1000.0f,
        fFPSLimit > 0.0f ? fFPSLimit : 30.0f);
    FrameTimer = static_cast<float*>(result);
    SetFrameTimerRate();
    return result;
}

class Fixes
{
public:
    Fixes()
    {
        WFP::onShutdownEvent() += []()
        {
            if (FrameClock.waitable)
            {
                CloseHandle(FrameClock.waitable);
                FrameClock.waitable = nullptr;
            }
        };
        WFP::onInitEvent() += []
        {
            hook::pattern pattern;
            auto& settings = FixSettings();
            // Developer weather override, compiled in only on request.
            // Rain particles and mission thunder effects may be script-driven.
#ifdef USM_DEBUG_ALWAYS_RAIN
            {
                auto pattern = hook::pattern("64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 8B 44 24 10 64 89 25 00 00 00 00 83 EC 30 53 55 33 DB 3B C3 56 57"); //0x408790
                shSwitchTimeOfDay = safetyhook::create_inline(pattern.get_first(), SwitchTimeOfDay);
            }
#endif

            if (FixSettings().Bool("MAIN.SkipIntro") != 0)
            {
                // Retain the frontend state advance after the four startup logos.
                pattern = hook::pattern("39 6E ? 0F 85"); //0x635DAE
                auto resume = pattern.get_first();
                pattern = hook::pattern("6A ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 ? 84 C0 75 ? 6A ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 ? 84 C0 75 ? 6A ? 68"); //0x635D55
                injector::MakeJMP(pattern.get_first(), resume, true);

                // Select the frontend's existing start-screen path after menu
                // initialization, bypassing activation of the copyright screen.
                pattern = hook::pattern("38 1D ? ? ? ? 8B 46 0C 89 46 34 74 51 83 F8 FF"); // 0x6488F3 + 12
                injector::MakeNOP(pattern.get_first(12), 2, true);
            }

            auto create = hook::pattern("E8 ? ? ? ? EB ? 33 C0 A3 ? ? ? ? E8 ? ? ? ? 6A");
            auto recreate = hook::pattern("E8 ? ? ? ? EB ? 33 C0 8B C8 A3 ? ? ? ? E8 ? ? ? ? 89 35");
            pResetFrameTimer = reinterpret_cast<decltype(pResetFrameTimer)>(injector::GetBranchDestination(recreate.get_first(16)).as_int());
            auto update = hook::pattern("83 EC 10 53 56 8B F1 8B 46 24 33 DB 3B C3");
            shUpdateFrameTimer = safetyhook::create_inline(update.get_first(), UpdateFrameTimer);
            pCreateFrameTimer = reinterpret_cast<decltype(pCreateFrameTimer)>(injector::MakeCALL(create.get_first(), CreateFrameTimer, true).get<void>());
            injector::MakeCALL(recreate.get_first(), CreateFrameTimer, true);
            settings.OnChange("MAIN.FPSLimit", []()
            {
                FrameLimitPending.store(true);
            });
            WFP::onGameProcessEvent() += []()
            {
                const auto monitorChanged = FrameLimitFollowsMonitor &&
                    MonitorFromWindow(hWnd.get(), MONITOR_DEFAULTTOPRIMARY) != FrameLimitMonitor;
                if (FrameLimitPending.exchange(false) || monitorChanged)
                {
                    fFPSLimit = FrameRateLimit();
                    SetFrameTimerRate();
                    if (FrameTimer)
                        pResetFrameTimer(FrameTimer);
                }
            };
        };
    }
} Fixes;

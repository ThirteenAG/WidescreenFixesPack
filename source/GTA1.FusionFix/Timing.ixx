module;

#include <stdafx.h>

export module Timing;

import ComVars;
import Log;

export namespace GTA1Timing
{
    SafetyHookInline WaitHook;
    HANDLE Timer = nullptr;
    double Frequency = [] { LARGE_INTEGER value{}; QueryPerformanceFrequency(&value); return double(value.QuadPart); }();
    double Deadline = 0;
    bool Pending = false;                   // a step ran; its frame waits for the deadline
    double Now()
    {
        LARGE_INTEGER value{}; QueryPerformanceCounter(&value);
        return value.QuadPart / Frequency;
    }
    void WaitForDeadline()
    {
        const double now = Now(), interval = 3 / 70.0;
        if (!Deadline || now - Deadline > interval) Deadline = now;
        Deadline += interval;
        for (double remaining = Deadline - Now(); remaining > 0; remaining = Deadline - Now())
        {
            if (remaining > 0.001)
            {
                LARGE_INTEGER due{};
                due.QuadPart = -LONGLONG((remaining - 0.0005) * 10000000.0);
                if (SetWaitableTimer(Timer, &due, 0, nullptr, nullptr, FALSE))
                    WaitForSingleObject(Timer, INFINITE);
                else Sleep(1);
            }
            else SwitchToThread();
        }
        nTimerTicks = 0;
        if (Log::Enabled)
        {
            static double previous = 0, shortest = 1, longest = 0, total = 0;
            static unsigned samples = 0;
            const double finished = Now();
            if (previous && samples < 120)
            {
                const double duration = finished - previous;
                shortest = std::min(shortest, duration); longest = std::max(longest, duration); total += duration;
                if (++samples == 120)
                {
                    char message[160]{};
                    sprintf_s(message, "GTA1 pacing: 120 steps mean=%.2fms min=%.2fms max=%.2fms", total * 1000 / samples, shortest * 1000, longest * 1000);
                    Log::Write(message);
                }
            }
            previous = finished;
        }
    }
    char __cdecl Wait(unsigned ticks)
    {
        if (!bTimerActive || !nTimerEnabled || !Available() || ticks != 3)
        { Deadline = 0; Pending = false; return WaitHook.call<char>(ticks); }
        // Keep the original three 70 Hz timer ticks per simulation step.
        // Miles' callback counter discarded the fractional remainder and busy
        // waited on the Windows timer's coarse cadence, producing uneven steps.
        // The game shows a frame at the start of the next loop, after rendering
        // it, so a wait here leaves every frame late by its varying render time.
        // The step runs at once; its frame waits for the deadline before it is
        // shown. Without a frame in between, the step waits here.
        if (Pending) WaitForDeadline();
        Pending = true;
        nTimerTicks = 0;
        return char(ticks);
    }
    // The time of the step being simulated: paced steps are exactly one interval
    // apart, whenever the step itself started.
    double Clock() { return Pending && Deadline ? Deadline : Now(); }
    // The gameplay frame is about to be shown.
    void BeforePresent()
    {
        if (!Pending) return;
        Pending = false;
        WaitForDeadline();
    }
    void Install()
    {
        // The tick wait; the same globals at the same offsets in every build.
        auto wait = GTA1Build::Find("A0 ? ? ? ? 84 C0 74 1F A1 ? ? ? ? 85 C0 74 16 8B 44 24 04 39 05 ? ? ? ? 72 F8 C7 05",
            "A0 ? ? ? ? 84 C0 74 21 A1 ? ? ? ? 85 C0 74 18 8B 44 24 04 8B 0D ? ? ? ? 3B C1 77 F6 C7 05",
            "A0 ? ? ? ? 84 C0 74 24 A1 ? ? ? ? 85 C0 74 1B 8B 44 24 04 8B 0D ? ? ? ? 3B C8 73 08 39 05");

        Timer = CreateWaitableTimerExW(nullptr, nullptr, 0x2, TIMER_ALL_ACCESS);
        if (!Timer) Timer = CreateWaitableTimerW(nullptr, FALSE, nullptr);
        if (!Timer || !Frequency) return;
        bTimerActive.SetAddress(*wait.get_first<uint8_t*>(1)); nTimerEnabled.SetAddress(*wait.get_first<int*>(10));
        nTimerTicks.SetAddress(*wait.get_first<int*>(24));
        WaitHook = safetyhook::create_inline(wait.get_first(), Wait);
        Log::Write("GTA1 precise simulation frame pacing installed (70/3 Hz).");
    }
}

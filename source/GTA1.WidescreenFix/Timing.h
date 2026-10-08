#pragma once

namespace GTA1Timing
{
    inline SafetyHookInline WaitHook;
    inline uint8_t* Active = nullptr;
    inline int *Enabled = nullptr, *NativeTicks = nullptr;
    inline HANDLE Timer = nullptr;
    inline double Frequency = 0, Deadline = 0;
    inline double Now()
    {
        LARGE_INTEGER value{}; QueryPerformanceCounter(&value);
        return value.QuadPart / Frequency;
    }
    inline char __cdecl Wait(unsigned ticks)
    {
        if (!*Active || !*Enabled || !GTA1Input::Available() || ticks != 3)
        { Deadline = 0; return WaitHook.call<char>(ticks); }
        // Keep the original three 70 Hz timer ticks per simulation step.
        // Miles' callback counter discarded the fractional remainder and busy
        // waited on the Windows timer's coarse cadence, producing uneven steps.
        const double now = Now(), interval = ticks / 70.0;
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
        *NativeTicks = 0;
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
        return char(ticks);
    }
    inline void Install()
    {
        CIniReader ini("");
        if (!ini.ReadBoolean("MAIN", "FixFramePacing", true)) return;
        // The tick wait; the same globals at the same offsets in every build.
        auto wait = GTA1Build::Find("A0 ? ? ? ? 84 C0 74 1F A1 ? ? ? ? 85 C0 74 16 8B 44 24 04 39 05 ? ? ? ? 72 F8 C7 05",
            "A0 ? ? ? ? 84 C0 74 21 A1 ? ? ? ? 85 C0 74 18 8B 44 24 04 8B 0D ? ? ? ? 3B C1 77 F6 C7 05",
            "A0 ? ? ? ? 84 C0 74 24 A1 ? ? ? ? 85 C0 74 1B 8B 44 24 04 8B 0D ? ? ? ? 3B C8 73 08 39 05");
        if (wait.size() != 1) { Log::Write("GTA1 frame pacing signature unavailable."); return; }
        LARGE_INTEGER frequency{}; QueryPerformanceFrequency(&frequency); Frequency = double(frequency.QuadPart);
        Timer = CreateWaitableTimerExW(nullptr, nullptr, 0x2, TIMER_ALL_ACCESS);
        if (!Timer) Timer = CreateWaitableTimerW(nullptr, FALSE, nullptr);
        if (!Timer || !Frequency) return;
        Active = *wait.get_first<uint8_t*>(1); Enabled = *wait.get_first<int*>(10);
        NativeTicks = *wait.get_first<int*>(24);
        WaitHook = safetyhook::create_inline(wait.get_first(), Wait);
        Log::Write("GTA1 precise simulation frame pacing installed (70/3 Hz).");
    }
}

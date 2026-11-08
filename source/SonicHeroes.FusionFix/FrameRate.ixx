module;

#include <stdafx.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

export module FrameRate;

import ComVars;

// The game runs one step of its logic per 1/60 s frame and waits for the next frame on a coarse multimedia timer
// event, which lets frames come early or late by a few milliseconds. The fix waits for the next 1/60 s itself, on a
// precise schedule. Movies keep the game's own wait: the movie decoder follows that timer.
export namespace FrameRate
{
    constexpr int64_t Rate = 60;

    inline void(__cdecl* GameWait)() = nullptr;
    inline SafetyHookMid MovieHook;
    inline bool MovieFrame = false;
    inline HANDLE Timer = nullptr;
    inline int64_t Frequency = 0;
    inline int64_t Next = 0;

    inline int64_t Now()
    {
        LARGE_INTEGER ticks{};
        QueryPerformanceCounter(&ticks);
        return ticks.QuadPart;
    }

    // Sleeps on the high-resolution timer until shortly before the deadline, then spins to it
    inline void WaitUntil(int64_t deadline)
    {
        const int64_t margin = Frequency / 1000;
        for (int64_t now = Now(); now < deadline; now = Now())
        {
            const int64_t left = deadline - now;
            if (Timer && left > margin * 2)
            {
                LARGE_INTEGER due;
                due.QuadPart = -std::max<int64_t>(1, (left - margin) * 10'000'000 / Frequency);
                if (SetWaitableTimer(Timer, &due, 0, nullptr, nullptr, FALSE))
                    WaitForSingleObject(Timer, INFINITE);
            }
            else if (!Timer && left > margin * 3)
                Sleep(1);
            else if (left > margin)
                Sleep(0);
            else
                YieldProcessor();
        }
    }

    inline void __cdecl Wait()
    {
        WFP::onGameProcessEvent().executeAll();

        if (MovieFrame)
        {
            MovieFrame = false;
            Next = 0;
            GameWait();
            return;
        }

        const int64_t period = Frequency / Rate;
        const int64_t now = Now();
        // A frame that took too long (loading, a dragged window) starts a new schedule instead of rushing to catch up
        if (Next == 0 || now - Next > period * 2)
            Next = now;
        WaitUntil(Next);
        Next += period;
    }

    void Init()
    {
        LARGE_INTEGER ticksPerSecond{};
        QueryPerformanceFrequency(&ticksPerSecond);
        Frequency = ticksPerSecond.QuadPart;
        Timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
        if (!Timer)
            timeBeginPeriod(1);

        // Once per frame in the main loop, before the frame's logic
        auto frame = hook::pattern("FF 0D ? ? ? ? E8 ? ? ? ? 83 3D ? ? ? ? 01 0F 8E");
        GameWait = reinterpret_cast<decltype(GameWait)>(injector::GetBranchDestination(frame.get_first(6)).as_int());
        injector::MakeCALL(frame.get_first(6), Wait, true);

        // The movie player's update
        MovieHook = safetyhook::create_mid(hook::get_pattern("83 EC 1C 53 56 8B F0 C7 44 24 0C 00 00 00 00 E8"), [](SafetyHookContext&)
        {
            MovieFrame = true;
        });
    }
}

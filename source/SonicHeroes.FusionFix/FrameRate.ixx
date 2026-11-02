module;

#include <stdafx.h>

export module FrameRate;

import ComVars;

#pragma runtime_checks("", off)

export namespace BetterSync
{
    float fFPSLimit = 60.0f;

    // code from: https://github.com/ThirteenAG/d3d8-wrapper/blob/c2509f26d22c0813bb6eced277defe67628acd90/source/dllmain.cpp#L55
    class FrameLimiter
    {
    private:
        static inline double TIME_Frequency = 0.0;
        static inline double TIME_Ticks = 0.0;
        static inline double TIME_Frametime = 0.0;

    public:
        enum FPSLimitMode { FPS_NONE, FPS_REALTIME, FPS_ACCURATE };
        static void Init(FPSLimitMode mode)
        {
            LARGE_INTEGER frequency;

            QueryPerformanceFrequency(&frequency);
            static constexpr auto TICKS_PER_FRAME = 1;
            auto TICKS_PER_SECOND = (TICKS_PER_FRAME * fFPSLimit);
            if (mode == FPS_ACCURATE)
            {
                TIME_Frametime = 1000.0 / (double)fFPSLimit;
                TIME_Frequency = (double)frequency.QuadPart / 1000.0; // ticks are milliseconds
            }
            else // FPS_REALTIME
            {
                TIME_Frequency = (double)frequency.QuadPart / (double)TICKS_PER_SECOND; // ticks are 1/n frames (n = fFPSLimit)
            }
            Ticks();
        }
        static DWORD Sync_RT()
        {
            DWORD lastTicks, currentTicks;
            LARGE_INTEGER counter;

            QueryPerformanceCounter(&counter);
            lastTicks = (DWORD)TIME_Ticks;
            TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
            currentTicks = (DWORD)TIME_Ticks;

            return (currentTicks > lastTicks) ? currentTicks - lastTicks : 0;
        }
        static DWORD Sync_SLP()
        {
            LARGE_INTEGER counter;
            QueryPerformanceCounter(&counter);
            double millis_current = (double)counter.QuadPart / TIME_Frequency;
            double millis_delta = millis_current - TIME_Ticks;
            if (TIME_Frametime <= millis_delta)
            {
                TIME_Ticks = millis_current;
                return 1;
            }
            else if (TIME_Frametime - millis_delta > 2.0) // > 2ms
                Sleep(1); // Sleep for ~1ms
            else
                Sleep(0); // yield thread's time-slice (does not actually sleep)

            return 0;
        }

    private:
        static void Ticks()
        {
            LARGE_INTEGER counter;
            QueryPerformanceCounter(&counter);
            TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
        }
    };

    FrameLimiter::FPSLimitMode mFPSLimitMode = FrameLimiter::FPSLimitMode::FPS_ACCURATE;

    uintptr_t sub_6C4FC0 = 0;
    bool bEnabled = false;

    // Once per frame, where the game waits for its next frame
    void CustomSyncFunc()
    {
        WFP::onGameProcessEvent().executeAll();

        if (!bEnabled || (SyncMovieID > 0))
            return reinterpret_cast<void(*)()>(sub_6C4FC0)();

        if (mFPSLimitMode == FrameLimiter::FPSLimitMode::FPS_REALTIME)
            while (!FrameLimiter::Sync_RT());
        else if (mFPSLimitMode == FrameLimiter::FPSLimitMode::FPS_ACCURATE)
            while (!FrameLimiter::Sync_SLP());
    }
}

#pragma runtime_checks("", restore)

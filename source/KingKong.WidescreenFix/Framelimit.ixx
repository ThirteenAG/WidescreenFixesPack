module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib") // needed for timeBeginPeriod()/timeEndPeriod()

export module Framelimit;

import ComVars;

export class FrameLimiter
{
public:
    enum FPSLimitMode
    {
        FPS_NONE, FPS_REALTIME, FPS_ACCURATE
    };
    FPSLimitMode mFPSLimitMode = FPS_NONE;
private:
    double TIME_Frequency = 0.0;
    double TIME_Ticks = 0.0;
    double TIME_Frametime = 0.0;
    float  fFPSLimit = 0.0f;
    bool   bFpsLimitWasUpdated = false;
public:
    void Init(FPSLimitMode mode, float fps_limit)
    {
        bFpsLimitWasUpdated = true;
        mFPSLimitMode = mode;
        fFPSLimit = fps_limit;

        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        static constexpr auto TICKS_PER_FRAME = 1;
        auto TICKS_PER_SECOND = (TICKS_PER_FRAME * fFPSLimit);
        if (mFPSLimitMode == FPS_ACCURATE)
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
    DWORD Sync_RT()
    {
        if (bFpsLimitWasUpdated)
        {
            bFpsLimitWasUpdated = false;
            return 1;
        }

        DWORD lastTicks, currentTicks;
        LARGE_INTEGER counter;

        QueryPerformanceCounter(&counter);
        lastTicks = (DWORD)TIME_Ticks;
        TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
        currentTicks = (DWORD)TIME_Ticks;

        return (currentTicks > lastTicks) ? currentTicks - lastTicks : 0;
    }
    DWORD Sync_SLP()
    {
        if (bFpsLimitWasUpdated)
        {
            bFpsLimitWasUpdated = false;
            return 1;
        }

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
    void Sync()
    {
        if (mFPSLimitMode == FPS_REALTIME)
            while (!Sync_RT());
        else if (mFPSLimitMode == FPS_ACCURATE)
            while (!Sync_SLP());
    }
private:
    void Ticks()
    {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
    }
};

export void InitFramelimit()
{
    auto frame = find_pattern("83 EC 08 56 57 8B F1", "89 3D ? ? ? ? F7 87");
    auto level = find_pattern("B9 ? ? ? ? 3B 0D ? ? ? ? 1B D2 F7 DA");
    if (frame.empty() || level.empty())
    {
        OutputDebugStringA("King Kong: frame limiter signatures unavailable.\n");
        return;
    }

    static FrameLimiter limiter;
    static float previousRate = 0.0f;
    static int previousMode = 0;
    static bool timerPeriod = false;
    static auto fpslimiter = safetyhook::create_mid(frame.get_first(), [](SafetyHookContext&)
    {
        static const auto& requested = FixSettings().Int("MAIN.FPSLimit");
        static const auto& method = FixSettings().Int("MAIN.FPSLimitMode");
        const int mode = method == 2 ? 2 : 1;
        static HMONITOR previousMonitor = nullptr;
        static float refreshRate = 60.0f;
        if (requested == -1)
        {
            const auto window = WindowedModeWrapper::GameHWND ? WindowedModeWrapper::GameHWND : GetActiveWindow();
            const auto monitor = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
            if (previousMonitor != monitor)
            {
                previousMonitor = monitor;
                MONITORINFOEXW info{};
                info.cbSize = sizeof(info);
                DEVMODEW display{};
                display.dmSize = sizeof(display);
                refreshRate = GetMonitorInfoW(monitor, &info) && EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &display) &&
                    display.dmDisplayFrequency > 1 ? static_cast<float>(display.dmDisplayFrequency) : 60.0f;
            }
        }
        float rate = requested == -1 ? refreshRate : static_cast<float>(requested);
        if (!std::isfinite(rate) || rate <= 0.0f)
        {
            previousRate = 0.0f;
            return;
        }
        rate = std::clamp(rate, 30.0f, 1000.0f);
        // These two native sequences require 30 FPS even with a higher user cap.
        if (gCurrentLevel == 0xF858 || gCurrentLevel == 0x0213)
            rate = 30.0f;
        if (mode == 2 && !timerPeriod)
        {
            timeBeginPeriod(1);
            timerPeriod = true;
        }
        if (rate != previousRate || mode != previousMode)
        {
            previousRate = rate;
            previousMode = mode;
            limiter.Init(mode == 2 ? FrameLimiter::FPS_ACCURATE : FrameLimiter::FPS_REALTIME, rate);
        }
        limiter.Sync();
    });
    static auto fpslimiterlevel = safetyhook::create_mid(level.get_first(), [](SafetyHookContext& regs)
    {
        gCurrentLevel = regs.eax & 0x7FFFF;
    });
    if (!fpslimiter || !fpslimiterlevel)
    {
        fpslimiter.reset();
        fpslimiterlevel.reset();
        OutputDebugStringA("King Kong: frame limiter hooks unavailable.\n");
    }
}

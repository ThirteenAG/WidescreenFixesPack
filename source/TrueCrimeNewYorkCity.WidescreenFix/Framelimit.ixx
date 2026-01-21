module;

#include <stdafx.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

export module Framelimit;

import ComVars;
import Speedhack;

class FrameLimiter
{
public:
    enum FPSLimitMode { FPS_NONE, FPS_REALTIME, FPS_ACCURATE };
    FPSLimitMode mFPSLimitMode = FPS_NONE;
private:
    double TIME_Frequency = 0.0;
    double TIME_Ticks = 0.0;
    double TIME_Frametime = 0.0;
    float  fFPSLimit = 0.0f;
public:
    void Init(FPSLimitMode mode, float fps_limit)
    {
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
        DWORD lastTicks, currentTicks;
        LARGE_INTEGER counter;
        QueryRealPerformanceCounter(&counter);
        lastTicks = (DWORD)TIME_Ticks;
        TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
        currentTicks = (DWORD)TIME_Ticks;

        return (currentTicks > lastTicks) ? currentTicks - lastTicks : 0;
    }
    DWORD Sync_SLP()
    {
        LARGE_INTEGER counter;
        QueryRealPerformanceCounter(&counter);
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
        QueryRealPerformanceCounter(&counter);
        TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
    }
} FpsLimiter;

SafetyHookInline shsub_648AC0 = {};
void __cdecl sub_648AC0(int a1)
{
    return shsub_648AC0.unsafe_ccall(nFrameLimitType ? 0 : a1);
}

void (__cdecl* sub_62B450)() = nullptr;
void __stdcall Thread(LPVOID a1)
{
    LARGE_INTEGER frequency, currentTime;
    double lastTime, elapsed;
    const double msPerFrame = 1000.0 / fFpsLimit; // ms per frame at the requested FPS
    // account for game speed factor, then apply the 0.5 scaling
    const double targetFrameTime = (msPerFrame / fGameSpeedFactor) * 0.5; // ms

    QueryPerformanceFrequency(&frequency);
    QueryRealPerformanceCounter(&currentTime);
    lastTime = (double)currentTime.QuadPart / frequency.QuadPart * 1000.0;

    while (1)
    {
        QueryRealPerformanceCounter(&currentTime);
        elapsed = ((double)currentTime.QuadPart / frequency.QuadPart * 1000.0) - lastTime;

        if (elapsed >= targetFrameTime)
        {
            lastTime += targetFrameTime;
            sub_62B450();
        }
        else
        {
            Sleep(1);
        }
    }
}

export void InitFrameLimiter()
{
    if (nFrameLimitType > 0)
    {
        fFpsLimit *= fGameSpeedFactor;

        auto mode = (nFrameLimitType == 2) ? FrameLimiter::FPSLimitMode::FPS_ACCURATE : FrameLimiter::FPSLimitMode::FPS_REALTIME;
        if (mode == FrameLimiter::FPSLimitMode::FPS_ACCURATE)
            timeBeginPeriod(1);

        FpsLimiter.Init(mode, fFpsLimit);
    }

    auto pattern = hook::pattern("A1 ? ? ? ? 83 EC 1C");
    shsub_648AC0 = safetyhook::create_inline(pattern.get_first(0), sub_648AC0);

    pattern = hook::pattern("8B 76 ? 8B 16 53");
    static auto FPSLimiterPresent = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (nFrameLimitType > 0 && fFpsLimit)
            FpsLimiter.Sync();
    });

    pattern = hook::pattern("A1 ? ? ? ? 83 C0 01 A3 ? ? ? ? A1");
    sub_62B450 = (decltype(sub_62B450))pattern.get_first();

    pattern = hook::pattern("56 8B 35 ? ? ? ? 57 8B 3D ? ? ? ? 8B FF");
    injector::MakeJMP(pattern.get_first(), Thread, true);

    // unify game speed and cutscene speed
    if (fFpsLimit >= 60.0f)
    {
        pattern = hook::pattern("0F 84 ? ? ? ? 80 3D ? ? ? ? ? 75 ? 84 DB");
        injector::MakeNOP(pattern.get_first(0), 6, true);
        injector::MakeNOP(pattern.get_first(13), 2, true);

        // change menu map cursor sensitivity
        static const float fMenuCursorSens = 27.5f / 2.0f;
        pattern = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 10 4C 24 ? F3 0F 59 C8 F3 0F 58 4E");
        injector::WriteMemory(pattern.get_first(4), &fMenuCursorSens, true);

        // improve camera rotation with gamepad
        pattern = hook::pattern("51 F3 0F 11 04 24 68 ? ? ? ? 8D 44 24 ? 68 ? ? ? ? 50 E8 ? ? ? ? 0F 28 08 0F 28 C1 0F 59 C1 0F 28 D0 0F C6 D0 FF 0F 28 D8 0F C6 D8 AA");
        static auto slerpHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            regs.xmm0.f32[0] *= 2.0f;
        });
    }

    InitSpeedhack();
}
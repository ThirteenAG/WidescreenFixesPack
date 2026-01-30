module;

#include <stdafx.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

export module Framelimit;

import ComVars;

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
        QueryPerformanceCounter(&counter);
        lastTicks = (DWORD)TIME_Ticks;
        TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
        currentTicks = (DWORD)TIME_Ticks;

        return (currentTicks > lastTicks) ? currentTicks - lastTicks : 0;
    }
    DWORD Sync_SLP()
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
} FpsLimiter, FpsLimiter30;

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

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&currentTime);
    lastTime = (double)currentTime.QuadPart / frequency.QuadPart * 1000.0;

    while (1)
    {
        QueryPerformanceCounter(&currentTime);
        elapsed = ((double)currentTime.QuadPart / frequency.QuadPart * 1000.0) - lastTime;

        if (elapsed >= msPerFrame)
        {
            lastTime += msPerFrame;
            sub_62B450();
        }
        else
        {
            Sleep(1);
        }
    }
}

bool GameNeeds30FPS()
{
    if (*nLoading != 0 || *bCutscene || *bPause)
        return false;

    static std::vector<std::pair<eCaseNames, eMissionNames_BC03>> missionList =
    {
        { BC03, BC3_M6 },
    };

    if (std::any_of(missionList.begin(), missionList.end(), [](const auto& p) { return CurrentCaseName == p.first && CurrentMissionName == p.second; }))
        return true;

    return false;
}

export void InitFrameLimiter()
{
    // Native frame limiter fix
    auto pattern = hook::pattern("A1 ? ? ? ? 83 C0 01 A3 ? ? ? ? A1");
    sub_62B450 = (decltype(sub_62B450))pattern.get_first();

    pattern = hook::pattern("A1 ? ? ? ? 83 EC 1C");
    shsub_648AC0 = safetyhook::create_inline(pattern.get_first(0), sub_648AC0);

    pattern = hook::pattern("56 8B 35 ? ? ? ? 57 8B 3D ? ? ? ? 8B FF");
    injector::MakeJMP(pattern.get_first(), Thread, true);

    if (fFpsLimit >= 60.0f)
    {
        if (nFrameLimitType > 0)
        {
            auto mode = (nFrameLimitType == 2) ? FrameLimiter::FPSLimitMode::FPS_ACCURATE : FrameLimiter::FPSLimitMode::FPS_REALTIME;
            if (mode == FrameLimiter::FPSLimitMode::FPS_ACCURATE)
                timeBeginPeriod(1);

            FpsLimiter.Init(mode, fFpsLimit);
            FpsLimiter30.Init(mode, 30.0f);

            auto pattern = hook::pattern("8B 76 ? 8B 16 53");
            static auto FPSLimiterPresent = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (!GameNeeds30FPS())
                    FpsLimiter.Sync();
                else
                    FpsLimiter30.Sync();
            });
        }

        // Tick rate
        pattern = hook::pattern("BE ? ? ? ? F3 0F 11 4C 24 ? F3 0F 11 44 24 ? 89 74 24 ? DB 44 24");
        injector::MakeNOP(pattern.get_first(), 5, true);
        static auto TickRateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (!GameNeeds30FPS())
                regs.esi = 1;
            else
                regs.esi = 2;
        });

        // Clamp
        pattern = hook::pattern("83 FE 04 74 ? 83 C6 01");
        injector::WriteMemory<uint8_t>(pattern.get_first(2), 2, true);

        pattern = hook::pattern("83 FE 04 89 74 24");
        injector::WriteMemory<uint8_t>(pattern.get_first(2), 2, true);

        static float fTargetTimeStep = (30.0f / fFpsLimit) * (1.0f / ((1.0f / 60.0f) * 1000.0f));

        {
            //pattern = hook::pattern("D9 05 ? ? ? ? DF F1 DD D8 76 ? E8 ? ? ? ? D8 0D ? ? ? ? D8 05 ? ? ? ? D9 5C 24 ? EB 0E F3 0F 10 05 ? ? ? ? F3 0F 11 44 24 ? 8B 4C 24");
            //injector::WriteMemory(0x45B131, &fTargetTimeStep, true); //fld     flt_6CF0A0
            //pattern = hook::pattern("D9 05 ? ? ? ? DF F1 DD D8 76 ? E8 ? ? ? ? D8 0D ? ? ? ? D8 05 ? ? ? ? D9 5C 24 ? EB 0E F3 0F 10 05 ? ? ? ? F3 0F 11 44 24 ? F3 0F 10 05");
            //injector::WriteMemory(0x45B1A7, &fTargetTimeStep, true); //fld     flt_6CF0A0
        }

        // Possibly loading screens
        {
            //pattern = hook::pattern("D8 0D ? ? ? ? D9 05 ? ? ? ? D9 C9 DF F1 DD D8 0F 86");
            //injector::WriteMemory(0x499680, &fTargetTimeStep, true); //fmul    flt_6CF0A0
            //pattern = hook::pattern("D8 0D ? ? ? ? D9 05 ? ? ? ? D9 C9 DF F1 DD D8 76 ? B8 01 00 00 00");
            //injector::WriteMemory(0x49A08D, &fTargetTimeStep, true); //fmul    flt_6CF0A0
        }

        // Main timing loop
        {
            pattern = hook::pattern("E8 ? ? ? ? 89 44 24 ? E8 ? ? ? ? 80 3D");
            static auto setTargetForMain = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (!GameNeeds30FPS())
                    fTargetTimeStep = (30.0f / fFpsLimit) * (1.0f / ((1.0f / 60.0f) * 1000.0f));
                else
                    fTargetTimeStep = (1.0f / ((1.0f / 60.0f) * 1000.0f));
            });

            pattern = hook::pattern("D8 0D ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 11 44 24 ? D9 5C 24");
            injector::WriteMemory(pattern.get_first(2), &fTargetTimeStep, true); //fmul    flt_6CF0A0
            pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 ? 0F 85");
            injector::WriteMemory(pattern.get_first(2), &fTargetTimeStep, true); //fmul    flt_6CF0A0
            pattern = hook::pattern("D8 0D ? ? ? ? A1 ? ? ? ? 85 C0");
            injector::WriteMemory(pattern.get_first(2), &fTargetTimeStep, true); //fmul    flt_6CF0A0
            pattern = hook::pattern("D8 0D ? ? ? ? F3 0F 2A C6");
            injector::WriteMemory(pattern.get_first(2), &fTargetTimeStep, true); //fmul    flt_6CF0A0
            pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 ? 0F 84");
            injector::WriteMemory(pattern.get_first(2), &fTargetTimeStep, true); //fmul    flt_6CF0A0
        }

        {
            //pattern = hook::pattern("D8 0D ? ? ? ? D9 05 ? ? ? ? D9 C9 DF F1 DD D8 72");
            //injector::WriteMemory(0x4A910C, &fTargetTimeStep, true); //fmul    flt_6CF0A0
        }

        {
            //pattern = hook::pattern("D8 0D ? ? ? ? D9 05 ? ? ? ? D9 C9 DF F1 DD D8 76 ? A1");
            //injector::WriteMemory(0x5C09D0, &fTargetTimeStep, true); //fmul    flt_6CF0A0
        }

        {
            //pattern = hook::pattern("D8 0D ? ? ? ? D9 05 ? ? ? ? D9 C9 DF F1 DD D8 0F 82");
            //injector::WriteMemory(0x5D8571, &fTargetTimeStep, true); //fmul    flt_6CF0A0
        }

        {
            //pattern = hook::pattern("0F 2F 05 ? ? ? ? 76 ? 6A 00 6A 00");
            //injector::WriteMemory(0x5F5489, &fTargetTimeStep, true); //comiss  xmm0, flt_6CF0A0
        }

        // change menu map cursor sensitivity
        static const float fMenuCursorSens = 27.5f / 2.0f;
        pattern = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 10 4C 24 ? F3 0F 59 C8 F3 0F 58 4E");
        injector::WriteMemory(pattern.get_first(4), &fMenuCursorSens, true);

        // improve camera rotation with gamepad
        pattern = hook::pattern("51 F3 0F 11 04 24 68 ? ? ? ? 8D 44 24 ? 68 ? ? ? ? 50 E8 ? ? ? ? 0F 28 08 0F 28 C1 0F 59 C1 0F 28 D0 0F C6 D0 FF 0F 28 D8 0F C6 D8 AA");
        static auto slerpHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (CurrentCameraMode == CameraOnFoot)
                regs.xmm0.f32[0] *= 1.5f;
        });
    }
}
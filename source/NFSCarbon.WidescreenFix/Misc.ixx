module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>

export module Misc;

import ComVars;
import Input;

namespace MoviePlayback
{
    int (__thiscall* GetBufferedBytes)(void* reader) = nullptr;
    int (__thiscall* GetStreamState)(void* reader) = nullptr;

    int __fastcall WaitForAudio(void* reader, void*)
    {
        // Movie startup waits for video/subtitle data, but checks audio only once.
        // If audio arrives later, its decoder and renderer are never connected.
        // The unconsumed audio then fills the shared stream and starves video.
        const DWORD start = GetTickCount();
        int bufferedBytes = GetBufferedBytes(reader);
        while (bufferedBytes == 0)
        {
            // State 2 is the terminal stream state checked by the game's other
            // startup waits. Bound the wait for missing or malformed audio too.
            if (GetStreamState(reader) == 2 || GetTickCount() - start >= 5000)
                return GetBufferedBytes(reader);

            // File reads use completion routines; the wait must be alertable.
            SleepEx(1, TRUE);
            bufferedBytes = GetBufferedBytes(reader);
        }
        return bufferedBytes;
    }
}

// Frame rate. The game simulates in fixed steps of 1/60 s and has no frame rate limit, so above 60 FPS
// some frames got no simulation step and the game stuttered. The step is set to the frame rate limit,
// frames are paced to it, and every frame gets one step.
namespace FrameRate
{
    float fStepTime = 1.0f / 60.0f;
    float fStepRate = 60.0f;

    void PreciseWaitUntil(LARGE_INTEGER target)
    {
        static LARGE_INTEGER frequency = []() { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
        static HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double seconds = double(target.QuadPart - now.QuadPart) / double(frequency.QuadPart);

        constexpr double SpinTime = 0.0015;
        if (seconds > SpinTime)
        {
            LARGE_INTEGER due;
            due.QuadPart = -static_cast<LONGLONG>((seconds - SpinTime) * 10000000.0);
            if (timer && SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
                WaitForSingleObject(timer, INFINITE);
            else
                Sleep(static_cast<DWORD>((seconds - SpinTime) * 1000.0));
        }

        do
        {
            YieldProcessor();
            QueryPerformanceCounter(&now);
        } while (now.QuadPart < target.QuadPart);
    }

    // Called before every frame is shown
    void LimitFrameRate()
    {
        static LARGE_INTEGER frequency = []() { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
        static LARGE_INTEGER next = {};

        LONGLONG period = static_cast<LONGLONG>(double(frequency.QuadPart) * fStepTime);
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        if (next.QuadPart == 0 || now.QuadPart - next.QuadPart > period * 4)
        {
            // First frame, or far behind after a hitch or loading
            next.QuadPart = now.QuadPart + period;
            return;
        }

        PreciseWaitUntil(next);
        next.QuadPart += period;
    }

    // Sim::Service(dt) runs a step for every whole step time that has passed. With dt close to one
    // step, timing noise gave 0 or 2 steps instead of 1. The time is rounded to the nearest step here;
    // the difference is carried over, so the simulation keeps real time within half a step.
    int32_t* SimActive = nullptr;
    float* Accumulator = nullptr;
    float Carry = 0.0f;

    void PrepareStep(float& dt)
    {
        if (!*SimActive)
        {
            Carry = 0.0f;
            return;
        }

        float previous = *Accumulator;
        float due = previous + dt + Carry;
        int steps = static_cast<int>(std::floor(due / fStepTime + 0.5f));
        if (steps <= 0)
        {
            Carry = 0.0f;
            dt = due - previous;
            return;
        }

        // A little more than the steps, so that the game's truncation gives exactly this many
        float target = (static_cast<float>(steps) + 0.001f) * fStepTime;
        Carry = due - target;
        dt = target - previous;
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            bool bSkipIntro = FixSettings().Bool("MAIN.SkipIntro");
            int nFPSLimit = FixSettings().Int("MAIN.FPSLimit");

            // Movies could start silent and then freeze
            {
                auto audioPattern = hook::pattern("8B 8E 80 01 00 00 3B CB 74 ? E8 ? ? ? ? 85 C0 7E ? FF 76 44"); // 0x888908
                auto statePattern = hook::pattern("8B 8E 5C 01 00 00 E8 ? ? ? ? 83 F8 02 74 ? 8B 8E 5C 01 00 00"); // 0x88883E
                if (audioPattern.size() == 1 && statePattern.size() == 1)
                {
                    auto audioCall = audioPattern.get_first(10);
                    MoviePlayback::GetBufferedBytes = (decltype(MoviePlayback::GetBufferedBytes))injector::GetBranchDestination(audioCall).as_int();
                    MoviePlayback::GetStreamState = (decltype(MoviePlayback::GetStreamState))injector::GetBranchDestination(statePattern.get_first(6)).as_int();
                    injector::MakeCALL(audioCall, MoviePlayback::WaitForAudio, true);
                }
            }

            // Car shadow opacity of the Xbox 360 version
            {
                uint32_t* dword_7BEA3A = hook::pattern("D8 05 ? ? ? ? DC C0 E8 ? ? ? ? 85 C0 7F 04 33 C0").count(1).get(0).get<uint32_t>(2);
                static float f60 = 60.0f;
                injector::WriteMemory(dword_7BEA3A, &f60, true);
            }

            // EA Nation is gone: the game no longer logs in by itself, which showed "Connecting to EA Nation"
            {
                auto pattern = hook::pattern("A0 ? ? ? ? 84 C0 74 34 A0 ? ? ? ? 84 C0 75 2B E8");
                if (!pattern.empty())
                    injector::WriteMemory<uint8_t>(pattern.get_first(7), 0xEB, true); // jz -> jmp
            }

            // Saved progress could be lost after loading a profile, often followed by a crash. Every saveable
            // block (GameplayData, RaceData, FEPlayerCarDB) is stored and found again under an id read from
            // +0x10 of its object, but the constructors put the name hash at +0xC: the id is whatever malloc
            // left there, so on newer Windows it rarely matched when loading. The blocks have different sizes,
            // so MemcardManager::LoadSaveData finds them by size alone.
            {
                auto pattern = hook::pattern("75 0B 8B 06 8B CE FF 50 14 3B D8"); //0x59606D
                if (!pattern.empty())
                    injector::MakeNOP(pattern.get_first(0), 2, true);
            }

            if (bSkipIntro)
            {
                // The intro movies: the EA logo, the PSA and the attract movie are skipped as if the game's
                // own "skip movies" setting was on. The movie play message holds the movie's name at +0x24.
                auto pattern = hook::pattern("8B 0D ? ? ? ? 53 33 DB 3B CB ? ? 8B 47 04");
                static uint32_t* pSkipMovies = *pattern.get_first<uint32_t*>(2);
                struct SkipIntroHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.ecx = *pSkipMovies;

                        auto name = *reinterpret_cast<const char**>(regs.eax + 0x24);
                        if (name)
                        {
                            for (auto movie : { "EAHD_Bumper", "PSA_", "Attract" })
                            {
                                if (_strnicmp(name, movie, strlen(movie)) == 0)
                                    regs.ecx = 1;
                            }
                        }
                    }
                }; injector::MakeInline<SkipIntroHook>(pattern.get_first(0), pattern.get_first(6));

                // The title screen continues by itself, straight to the main menu (see Frontend's SplashScreen hooks)
                Input::SetSkipTitleScreen(true);
            }

            // Frame rate limit and simulation rate: -1 = monitor refresh rate, -2 = twice the refresh rate.
            // The game needs a fixed rate, so it can't be unlimited.
            if (nFPSLimit == -2)
                nFPSLimit = GetDesktopRefreshRate() * 2;
            else if (nFPSLimit <= 0)
                nFPSLimit = GetDesktopRefreshRate();
            SimRate = std::clamp(nFPSLimit, 30, 1000);
            FrameRate::fStepRate = static_cast<float>(SimRate);
            FrameRate::fStepTime = 1.0f / FrameRate::fStepRate;

            // Sim::Service: step time and rate
            static ProtectedGameRef<float> StepTime;
            StepTime.SetAddress(*hook::pattern("D9 46 1C 8B 46 24 83 F8 03 D8 0D ? ? ? ?").count(1).get(0).get<float*>(11));
            StepTime = FrameRate::fStepTime;

            auto pattern = hook::pattern("64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 A1 ? ? ? ? 64 89 25 00 00 00 00 83 EC 18 56 57 33");
            static ProtectedGameRef<float> StepRate;
            StepRate.SetAddress(*pattern.get_first<float*>(0x57));
            StepRate = FrameRate::fStepRate;

            FrameRate::SimActive = *pattern.get_first<int32_t*>(15);
            FrameRate::Accumulator = *pattern.get_first<float*>(0x27);
            static auto SimServiceHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                FrameRate::PrepareStep(*reinterpret_cast<float*>(regs.esp + 4));
            });

            // Sim tasks and the simulation timer count in steps of 1/60 s
            static ProtectedGameRef<float> TaskStepTime;
            TaskStepTime.SetAddress(*hook::pattern("83 C4 04 D9 46 14 42 D8 05 ? ? ? ?").count(1).get(0).get<float*>(9));
            TaskStepTime = FrameRate::fStepTime;

            static ProtectedGameRef<float> TimerStepTime;
            TimerStepTime.SetAddress(hook::pattern("C7 46 14 89 88 88 3C").count(1).get(0).get<float>(3));
            TimerStepTime = FrameRate::fStepTime;

            static ProtectedGameRef<float> TimeScaleStep;
            TimeScaleStep.SetAddress(hook::pattern("68 89 88 88 3C").count(1).get(0).get<float>(1));
            TimeScaleStep = FrameRate::fStepTime;

            // Debug modes of the main loop: fixed step, and ten times the speed
            static ProtectedGameRef<float> FixedFrameTime;
            FixedFrameTime.SetAddress(hook::pattern("C7 44 24 1C 89 88 88 3C").count(1).get(0).get<float>(4));
            FixedFrameTime = FrameRate::fStepTime;

            static ProtectedGameRef<float> FullSpeedFrameTime;
            FullSpeedFrameTime.SetAddress(hook::pattern("C7 44 24 1C AB AA 2A 3E").count(1).get(0).get<float>(4));
            FullSpeedFrameTime = FrameRate::fStepTime * 10.0f;

            // Not while loading: the loading loop services the file transfers once per pass, so waiting
            // there would only make loading slower
            WFP::onEndScene() += []()
            {
                if (!bInMiniMainLoop)
                    FrameRate::LimitFrameRate();
            };
        };
    }
} Misc;

module;

#include <stdafx.h>
#include "TimingMath.hpp"
#include "FrameRate.hpp"
#include "FloatingPoint.hpp"
#include "FixSettings.hpp"
#include <atomic>
#include <stdexcept>

export module Timing;

import Window;

import ComVars;

namespace GodfatherTiming
{
    SafetyHookInline updateHook, resetHook;
    int(__cdecl* enabled)(){};
    bool(__cdecl* paused)(){};
    std::atomic<int> limit{ 30 };

    struct State
    {
        void* owner = nullptr;
        LONGLONG previous = 0;
        Remainders remainder;
    };
    thread_local State state;

    template <typename T> T Read(GameRef<T>& pointer)
    {
        return pointer;
    }

    template <typename T> void Write(GameRef<T>& pointer, T value)
    {
        pointer = value;
    }

    unsigned char __fastcall Update(void* self, void*, int parameter)
    {
        Godfather::FloatingPointScope floatingPoint;
        const auto now = GodfatherFrameRate::Now();
        const auto oldTicks = Read(Timer_ticks);
        const auto oldMicroseconds = Read(Timer_gameMicroseconds);
        const auto oldScaled = Read(Timer_scaled);
        const auto result = updateHook.unsafe_thiscall<unsigned char>(self, parameter);
        if (enabled())
        {
            const auto rate = GodfatherFrameRate::Frequency();
            const double elapsed = state.owner == self && state.previous > 0 && now >= state.previous && rate > 0
                ? double(now - state.previous) / double(rate) : double(Read(Timer_unscaled));
            if (state.owner != self)
                state.remainder = {};
            const auto frame = Advance(state.remainder, elapsed, Read(Timer_quantum), Read(Timer_frequency), Read(Timer_maximumTicks),
                Read(Timer_scale), paused(), oldTicks, oldMicroseconds, oldScaled);
            if (frame.valid)
            {
                Write(Timer_ticks, frame.ticks);
                Write(Timer_tickError, frame.tickError);
                Write(Timer_milliseconds, frame.milliseconds);
                Write(Timer_seconds, frame.seconds);
                Write(Timer_unscaled, frame.elapsed);
                if (frame.running)
                {
                    Write(Timer_scaled, frame.scaled);
                    Write(Timer_gameMicroseconds, frame.gameMicroseconds);
                    Write(Timer_gameMilliseconds, frame.gameMilliseconds);
                    Write(Timer_gameSeconds, frame.gameSeconds);
                    Write(Timer_elapsedMilliseconds, frame.elapsedMilliseconds);
                }
            }
        }
        state.owner = self;
        state.previous = now;
        static std::once_flag gameInit;
        std::call_once(gameInit, []()
        {
            WFP::onGameInitEvent().executeAll();
        });
        WFP::onGameProcessEvent().executeAll();
        return result;
    }

    int __cdecl Reset()
    {
        state = {};
        GodfatherFrameRate::Limit(0);
        return resetHook.unsafe_ccall<int>();
    }

    void Pace()
    {
        Godfather::FloatingPointScope floatingPoint;
        GodfatherFrameRate::Limit(limit.load(std::memory_order_relaxed));
    }

    void WINAPI NativeSleep(DWORD)
    {
        // Presentation owns the frame deadline, including frames without a native sleep.
    }

    decltype(&NativeSleep) nativeSleep = NativeSleep;

    export void Init()
    {
        auto initialize = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 D9 05 ? ? ? ? 56 D8 0D ? ? ? ? 33 F6 89 35 ? ? ? ? 89 35 ? ? ? ? D9 15 ? ? ? ? 89 35 ? ? ? ? D9 1D ? ? ? ? 89 35 ? ? ? ? D9 05 ? ? ? ? E8 ? ? ? ? D9 05 ? ? ? ? D8 0D ? ? ? ? A3 ? ? ? ? 39 35 ? ? ? ? D9 1D ? ? ? ? C7 05 ? ? ? ? 00 00 80 3F 89 35 ? ? ? ? 89 35 ? ? ? ? 89 35 ? ? ? ? 89 35 ? ? ? ? C7 05 ? ? ? ? 05 00 00 00");
        auto update = hook::pattern("83 EC 0C 53 55 8B D9 E8 ? ? ? ? 8B E8 E8 ? ? ? ? 85 C0 0F 84 ? ? ? ? 8B 53 0C 8B C5 2B C2 85 C0 89 44 24 08 DB 44 24 08 56 7D 06 D8 05 ? ? ? ? D8 35 ? ? ? ? D8 25 ? ? ? ?");
        auto rate = hook::pattern("85 F6 D8 35 ? ? ? ? 89 74 24 10 D9 1D ? ? ? ? DB 44 24 10 7D 06 D8 05 ? ? ? ? D9 54 24 10 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1D ? ? ? ?");
        auto pause = hook::pattern("E8 ? ? ? ? 85 C0 0F 84 ? ? ? ? E8 ? ? ? ? 84 C0 0F 85 ? ? ? ? D9 05 ? ? ? ? D8 0D ? ? ? ?");
        auto sleep = hook::pattern("DD D8 50 FF 15 ? ? ? ? EB 02 DD D8 A0 ? ? ? ? 84 C0");
        enabled = reinterpret_cast<decltype(enabled)>(injector::GetBranchDestination(update.get_first<uint8_t>(14)).as_int());
        paused = reinterpret_cast<decltype(paused)>(injector::GetBranchDestination(pause.get_first<uint8_t>(13)).as_int());
        Timer_quantum.SetAddress(*initialize.get_first<float*>(24));
        Timer_ticks.SetAddress(*initialize.get_first<uint32_t*>(39));
        Timer_milliseconds.SetAddress(*initialize.get_first<uint32_t*>(45));
        Timer_unscaled.SetAddress(*initialize.get_first<float*>(51));
        Timer_seconds.SetAddress(*initialize.get_first<float*>(57));
        Timer_scaled.SetAddress(*initialize.get_first<float*>(63));
        Timer_elapsedMilliseconds.SetAddress(*initialize.get_first<uint32_t*>(97));
        Timer_scale.SetAddress(*initialize.get_first<float*>(115));
        Timer_gameMilliseconds.SetAddress(*initialize.get_first<uint32_t*>(125));
        Timer_gameMicroseconds.SetAddress(*initialize.get_first<uint64_t*>(131));
        Timer_gameSeconds.SetAddress(*initialize.get_first<float*>(143));
        Timer_maximumTicks.SetAddress(*initialize.get_first<uint32_t*>(149));
        Timer_tickError.SetAddress(*update.get_first<float*>(61));
        Timer_frequency.SetAddress(*rate.get_first<float*>(4));
        updateHook = safetyhook::create_inline(update.get_first(), Update, SafetyHookInline::StartDisabled);
        resetHook = safetyhook::create_inline(initialize.get_first(), Reset, SafetyHookInline::StartDisabled);
        (void)updateHook.enable();
        (void)resetHook.enable();
        GodfatherFrameRate::window = []() { return GodfatherWindow::window; };
        limit.store(FixSettings().Int("MAIN.FPSLimit"), std::memory_order_relaxed);
        FixSettings().OnChange("MAIN.FPSLimit", []()
        {
            limit.store(FixSettings().Int("MAIN.FPSLimit"), std::memory_order_relaxed);
        });
        injector::WriteMemory<decltype(nativeSleep)*>(sleep.get_first<decltype(nativeSleep)*>(5), &nativeSleep, true);
        GodfatherWindow::pace = Pace;
    }
}

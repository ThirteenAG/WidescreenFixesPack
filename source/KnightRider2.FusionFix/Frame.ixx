module;
#include <stdafx.h>
#include "Shared/MaxPayne/FrameRate.hpp"
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"

export module Frame;
import ComVars;


export namespace KnightRider2Frame
{
    inline std::atomic<int> Limit{ 60 };
    inline SafetyHookInline ReadyHook, PresentHook;
    inline void (*OverlayFrame)(uint8_t*) = nullptr;
    inline bool (*InputBlocked)() = nullptr;
    inline void (*InputFrame)() = nullptr;
    inline void (*KeyboardInput)() = nullptr;

    inline bool __fastcall Ready(uint8_t* application, void*)
    {
        if (InputFrame)
            InputFrame();
        CursorConfinement::Frame(WindowedModeWrapper::GameHWND);
        MaxPayneFrameRate::Limit(Limit.load(std::memory_order_relaxed));
        // The native interval is replaced by the precise limiter, before the game samples its clock.
        *reinterpret_cast<float*>(application + 108) = 0.0f;
        const float elapsed = *reinterpret_cast<float*>(application + 88);
        const auto ready = ReadyHook.thiscall<bool>(application);
        if (InputBlocked && InputBlocked())
        {
            // Freeze the shared game clock as well as state updates, so every cached timestamp agrees on resume.
            *reinterpret_cast<float*>(application + 88) = elapsed;
            *reinterpret_cast<float*>(application + 112) = elapsed;
        }
        return ready;
    }

    inline void __fastcall Present(uint8_t* renderer, void*)
    {
        // The renderer has a second, busy-wait cap (26 FPS by default), independent of the application timer.
        NativeCap = 0.0f;
        if (OverlayFrame)
            OverlayFrame(renderer);
        PresentHook.thiscall<void>(renderer);
    }

    inline void Init()
    {
        const auto update = []()
        {
            Limit.store(std::clamp(FixSettings().Int("MAIN.FPSLimit"), -1, 1000), std::memory_order_relaxed);
        };
        update();
        FixSettings().OnChange("MAIN.FPSLimit", update);
        // A 2 ms minimum advances game time faster than real time above 500 Hz, including idle polls.
        auto minimum = hook::pattern("C7 46 54 6F 12 03 3B D9 5E 50 D9 46 58 D8 46 54");
        injector::MakeNOP(minimum.get_first(), 7);
        auto ready = hook::pattern("56 8B F1 81 7E 50 00 00 80 BF 75 10 E8 ? ? ? ? D9 5E 50");
        ReadyHook = safetyhook::create_inline(ready.get_first(), Ready);
        auto present = hook::pattern("51 D9 05 ? ? ? ? 56 D8 1D ? ? ? ? 8B F1 DF E0 F6 C4 41 75");
        NativeCap.SetAddress(*present.get_first<float*>(3));
        PresentHook = safetyhook::create_inline(present.get_first(), Present);
    }
}

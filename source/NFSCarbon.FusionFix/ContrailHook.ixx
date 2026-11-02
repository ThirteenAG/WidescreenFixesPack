module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module ContrailHook;

import ComVars;

// Xenon effects (contrails, sparks) are spawned by the render code once per frame, so their density grew
// with the frame rate. Each kind of effect has a gate that opens at a fixed rate; all its effects of a frame
// spawn while it's open. The gate is the same for every car and emitter: the game passes no piggyback effect
// for contrails (always null) and loops over the child effects of a spark emitter with the same one, so
// timers per pointer would have dropped all but one car or child effect.
export namespace XenonEffectRate
{
    struct Gate
    {
        float interval;
        float time = 0.0f;
        bool due = true;

        void Advance(float dt)
        {
            time += dt;
            due = time >= interval;
            if (due)
                time = std::min(time - interval, interval);    // no burst after a hitch
        }
    };

    Gate Contrails = { 1.0f / 30.0f };  // console rate, the consoles ran at 30 FPS
    Gate Sparks = { 1.0f / 60.0f };     // as at 60 FPS

    void Init()
    {
        static std::once_flag once;
        std::call_once(once, []()
        {
            WFP::onGameProcessEvent() += []()
            {
                using Clock = std::chrono::steady_clock;
                static auto last = Clock::now();
                auto now = Clock::now();
                float dt = std::clamp(std::chrono::duration<float>(now - last).count(), 0.0f, 0.1f);
                last = now;

                Contrails.Advance(dt);
                Sparks.Advance(dt);
            };
        });
    }
}

class ContrailHook
{
private:
    static inline uintptr_t AddXenonEffect = 0;
    static inline const int* pDisableContrails = nullptr;

    static void AddXenonEffect_Contrail_Hook(void* piggyback_fx, void* spec, bMatrix4* mat, bVector4* vel, float intensity)
    {
        // The game's intensity is kept: (speed - 35 m/s) / 30, from 0.1 to 0.75
        if (!*pDisableContrails && XenonEffectRate::Contrails.due)
            reinterpret_cast<void(*)(void*, void*, bMatrix4*, bVector4*, float)>(AddXenonEffect)(piggyback_fx, spec, mat, vel, intensity);
    }

public:
    ContrailHook()
    {
        WFP::onInitEvent() += []()
        {
            pDisableContrails = &FixSettings().Int("GRAPHICS.DisableContrails");
            XenonEffectRate::Init();

            uintptr_t loc_7E139A = reinterpret_cast<uintptr_t>(hook::pattern("8A 87 9C 01 00 00 84 C0").get_first(0)) + 0x119;
            AddXenonEffect = static_cast<uintptr_t>(injector::GetBranchDestination(loc_7E139A));
            injector::MakeCALL(loc_7E139A, AddXenonEffect_Contrail_Hook, true);
        };
    }
} ContrailHook;

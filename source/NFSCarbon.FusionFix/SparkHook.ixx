module;

#include <stdafx.h>

export module SparkHook;

import ComVars;
import ContrailHook;

// Sparks and other Xenon effects of collisions: spawned at most 60 times a second, as at 60 FPS (see
// ContrailHook for the rate gate). The game passes an intensity of 1.0 itself.
class SparkHook
{
private:
    static inline uintptr_t AddXenonEffect = 0;

    static void AddXenonEffect_Spark_Hook(void* piggyback_fx, void* spec, bMatrix4* mat, bVector4* vel, float intensity)
    {
        if (XenonEffectRate::Sparks.due)
            reinterpret_cast<void(*)(void*, void*, bMatrix4*, bVector4*, float)>(AddXenonEffect)(piggyback_fx, spec, mat, vel, intensity);
    }

public:
    SparkHook()
    {
        WFP::onInitEvent() += []()
        {
            XenonEffectRate::Init();

            // STUB: spark effect hook-up authored by REDACTED removed
            (void)&AddXenonEffect_Spark_Hook;

        };
    }
} SparkHook;

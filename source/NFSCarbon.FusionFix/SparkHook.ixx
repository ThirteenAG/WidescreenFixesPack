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
            // The AddXenonEffect call of the collision effects, with an intensity of 1.0
            auto sparkCall = hook::pattern("68 00 00 80 3F 8D 4E 20 51 52 50 8B 46 40 50 E8").get_first(15);
            AddXenonEffect = injector::GetBranchDestination(sparkCall).as_int();
            injector::MakeCALL(sparkCall, AddXenonEffect_Spark_Hook, true);
        };
    }
} SparkHook;

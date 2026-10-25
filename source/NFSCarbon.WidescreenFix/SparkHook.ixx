module;

#include <stdafx.h>
#include "../Shared/PatternPreflight.hpp"

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
            [&]()
            {
                if (!PatternPreflight::Check("NFSCarbon SparkHook", {
                    { "8B 4E 3C 57 68 37 E6 40 FE E8", 1, false },
                }))
                    return;

                XenonEffectRate::Init();

                uintptr_t loc_755C99 = reinterpret_cast<uintptr_t>(hook::pattern("8B 4E 3C 57 68 37 E6 40 FE E8").get_first(0)) + 0x39;
                AddXenonEffect = static_cast<uintptr_t>(injector::GetBranchDestination(loc_755C99));
                injector::MakeCALL(loc_755C99, AddXenonEffect_Spark_Hook, true);
            }();

        };
    }
} SparkHook;

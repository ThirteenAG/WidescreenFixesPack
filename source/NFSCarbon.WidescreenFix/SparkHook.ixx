module;

#include <stdafx.h>

export module SparkHook;

import ComVars;

namespace SparkHook
{
    bool bLimitSparkRate = false;
    float fSparkTargetFPS = 60.0f;
    float fSparkIntensity = 1.0f;

    uintptr_t AddXenonEffect = 0x00754CD0;

    uint32_t SparkFrameDelay = 1;

    uint32_t SparkFC = 0;
    uint32_t RenderConnFC = 0;
    void AddXenonEffect_Spark_Hook(void* piggyback_fx, void* spec, bMatrix4* mat, bVector4* vel, float intensity)
    {
        if (!bLimitSparkRate)
            return reinterpret_cast<void(*)(void*, void*, bMatrix4*, bVector4*, float)>(AddXenonEffect)(piggyback_fx, spec, mat, vel, fSparkIntensity);

        // TODO: this could use actual timers instead of a simple framecounter to make it better, but it is good enough for this purpose
        if ((SparkFC + SparkFrameDelay) <= RenderConnFC)
        {
            if (SparkFC != RenderConnFC)
            {
                SparkFC = RenderConnFC;
                reinterpret_cast<void(*)(void*, void*, bMatrix4*, bVector4*, float)>(AddXenonEffect)(piggyback_fx, spec, mat, vel, fSparkIntensity);
            }
        }
        RenderConnFC++;
    }
}

export void InitSparkHook()
{
    CIniReader iniReader("");
    SparkHook::bLimitSparkRate = iniReader.ReadInteger("Sparks", "LimitSparkRate", 0) != 0;
    SparkHook::fSparkTargetFPS = iniReader.ReadFloat("Sparks", "SparkTargetFPS", 60.0f);
    SparkHook::fSparkIntensity = iniReader.ReadFloat("Sparks", "SparkIntensity", 1.0f);

    if (SparkHook::bLimitSparkRate)
    {
        uintptr_t loc_6B4D30 = reinterpret_cast<uintptr_t>(hook::pattern("D9 05 ? ? ? ? B9 ? ? ? ? D8 44 24 14 D9 5C 24 14").get_first(0)) + 0x35;
        static float fGameTargetFPS = 1.0f / **reinterpret_cast<float**>(loc_6B4D30);

        if (fGameTargetFPS != SparkHook::fSparkTargetFPS)
        {
            // TODO: use a timer instead of this because of rounding errors!
            float fSparkFrameDelay = (fGameTargetFPS / SparkHook::fSparkTargetFPS);
            SparkHook::SparkFrameDelay = static_cast<uint32_t>(round(fSparkFrameDelay));

            uintptr_t loc_755C99 = reinterpret_cast<uintptr_t>(hook::pattern("8B 4E 3C 57 68 37 E6 40 FE E8").get_first(0)) + 0x39;
            SparkHook::AddXenonEffect = static_cast<uintptr_t>(injector::GetBranchDestination(loc_755C99));
            injector::MakeCALL(loc_755C99, SparkHook::AddXenonEffect_Spark_Hook, true);
        }
    }
    else
    {
        uintptr_t loc_755C8A = reinterpret_cast<uintptr_t>(hook::pattern("8B 4E 3C 57 68 37 E6 40 FE E8").get_first(0)) + 0x2A;
        injector::WriteMemory<float>(loc_755C8A + 1, SparkHook::fSparkIntensity, true);
    }
}